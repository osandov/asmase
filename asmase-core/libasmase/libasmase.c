/*
 * Core libasmase support.
 *
 * Copyright (C) 2016-2018 Omar Sandoval
 *
 * This file is part of asmase.
 *
 * asmase is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * asmase is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with asmase.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <spawn.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "internal.h"

static int create_shmem(struct asmase_instance *a)
{
	void *addr;
	int fd;

	fd = syscall(SYS_memfd_create, "asmase", 0);
	if (fd == -1)
		return -1;

	if (ftruncate(fd, SHMEM_SIZE) == -1) {
		close(fd);
		return -1;
	}

	addr = mmap(NULL, SHMEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (addr == MAP_FAILED) {
		close(fd);
		return -1;
	}

	a->shmem = addr;

	return fd;
}

static pid_t fork_tracee(int memfd, int flags)
{
	pid_t pid;

	pid = fork();
	if (pid == 0)
		tracee(memfd, flags); /* Doesn't return. */
	else
		return pid;
}

__attribute__((visibility("default")))
struct asmase_instance *asmase_create_instance(int flags)
{
	struct asmase_instance *a;
	int memfd;

	if (flags & ~(ASMASE_SANDBOX_ALL)) {
		errno = EINVAL;
		return NULL;
	}

	a = malloc(sizeof(*a));
	if (!a)
		return NULL;
	a->shmem = NULL;

	memfd = create_shmem(a);
	if (memfd == -1) {
		asmase_destroy_instance(a);
		return NULL;
	}

	a->pid = fork_tracee(memfd, flags);
	if (a->pid == -1) {
		close(memfd);
		asmase_destroy_instance(a);
		return NULL;
	}
	close(memfd);

	a->flags = flags;
	a->state = ASMASE_INSTANCE_STATE_NEW;

	return a;
}

__attribute__((visibility("default")))
void asmase_destroy_instance(struct asmase_instance *a)
{
	if (a->shmem)
		munmap(a->shmem, SHMEM_SIZE);
	free(a);
}

__attribute__((visibility("default")))
pid_t asmase_getpid(const struct asmase_instance *a)
{
	return a->pid;
}

__attribute__((visibility("default")))
void *asmase_get_shmem(const struct asmase_instance *a)
{
	return a->shmem;
}

__attribute__((visibility("default")))
int asmase_execute_code(struct asmase_instance *a, const char *code, size_t len)
{
	size_t total_len;
	bool overflow;

	if (a->state != ASMASE_INSTANCE_STATE_READY) {
		errno = EINVAL;
		return -1;
	}

	overflow = __builtin_add_overflow(len, arch_trap_instruction_len,
					  &total_len);
	if (overflow || total_len > CODE_MAX_SIZE) {
		errno = E2BIG;
		return -1;
	}

	memcpy(a->shmem, code, len);
	memcpy((char *)a->shmem + len, arch_trap_instruction,
	       arch_trap_instruction_len);

	if (arch_reset_program_counter(a) == -1) {
		if (errno == ESRCH)
			return 0;
		return -1;
	}
	if (ptrace(PTRACE_CONT, a->pid, NULL, NULL) == -1) {
		if (errno == ESRCH)
			return 0;
		return -1;
	}

	a->state = ASMASE_INSTANCE_STATE_RUNNING;

	return 0;
}

static int check_maps(struct asmase_instance *a)
{
	FILE *file;
	char path[50];
	int ret;

	snprintf(path, sizeof(path), "/proc/%ld/maps", (long)a->pid);

	file = fopen(path, "r");
	if (!file)
		return -1;

	for (;;) {
		unsigned long start, end;
		char *path;
		char c;

		ret = fscanf(file, "%lx-%lx %*c%*c%*c%*c %*x %*x:%*x %*u",
			     &start, &end);
		if (ret == EOF)
			break;

		ret = fscanf(file, "%*[ ]%m[^\n]", &path);
		if (ret == EOF) {
			if (!ferror(file))
				errno = EINVAL;
			goto err;
		}

		if (strstartswith(path, "/memfd:asmase")) {
			if (start != X86_64_SHMEM_ADDR ||
			    (end - start) != SHMEM_SIZE) {
				errno = EADDRNOTAVAIL;
				goto err;
			}
		} else if (strcmp(path, "[vsyscall]") != 0) {
			errno = EADDRINUSE;
			goto err;
		}

		c = fgetc(file);
		if (c != '\n') {
			if (!ferror(file))
				errno = EINVAL;
			goto err;
		}
	}
	if (ferror(file))
		goto err;

	fclose(file);
	return 0;

err:
	fclose(file);
	return -1;
}

static int check_fds(struct asmase_instance *a)
{
	DIR *dir;
	char path[50];
	int ret = -1;

	snprintf(path, sizeof(path), "/proc/%ld/fd", (long)a->pid);

	dir = opendir(path);
	if (!dir)
		return -1;

	for (;;) {
		struct dirent *dirent;

		errno = 0;
		dirent = readdir(dir);
		if (!dirent) {
			if (!errno)
				ret = 0;
			break;
		}

		if (strcmp(dirent->d_name, ".") != 0 &&
		    strcmp(dirent->d_name, "..") != 0) {
			errno = EBADF;
			break;
		}
	}

	closedir(dir);
	return ret;
}

static int check_seccomp(struct asmase_instance *a)
{
	FILE *file;
	char buf[50];
	bool no_new_privs = false, seccomp = false;
	int ret;

	snprintf(buf, sizeof(buf), "/proc/%ld/status", (long)a->pid);

	file = fopen(buf, "r");
	if (!file)
		return -1;

	for (;;) {
		int status;

		if (!fgets(buf, sizeof(buf), file))
			break;

		ret = sscanf(buf, "NoNewPrivs: %d", &status);
		if (ret == EOF)
			break;
		if (ret == 1) {
			no_new_privs = status;
			continue;
		}

		ret = sscanf(buf, "Seccomp: %d", &status);
		if (ret == EOF)
			break;
		if (ret == 1) {
			seccomp = status == 2;
			continue;
		}
	}
	if (ferror(file)) {
		ret = -1;
		goto out;
	}

	if (!no_new_privs || !seccomp) {
		errno = EPERM;
		ret = -1;
		goto out;
	}

	ret = 0;
out:
	fclose(file);
	return ret;
}

static int asmase_finish_create(struct asmase_instance *a)
{
	if (ptrace(PTRACE_SETOPTIONS, a->pid, NULL, PTRACE_O_EXITKILL) == -1)
		return -1;

	memset(a->shmem, 0, SHMEM_SIZE);

	if (check_maps(a) == -1)
		return -1;

	if ((a->flags & ASMASE_SANDBOX_FDS) && check_fds(a) == -1)
		return -1;

	if ((a->flags & ASMASE_SANDBOX_SYSCALLS) && check_seccomp(a) == -1)
		return -1;

	return 0;
}

static int asmase_waitpid(struct asmase_instance *a, int *wstatus, int flags)
{
	pid_t ret;

again:
	ret = waitpid(a->pid, wstatus, flags);
	if (ret <= 0)
		return ret;

	if (a->state == ASMASE_INSTANCE_STATE_NEW) {
		if (WIFEXITED(*wstatus) || WIFSIGNALED(*wstatus)) {
			/*
			 * It's an error if the tracee exited while
			 * bootstrapping.
			 */
			a->state = ASMASE_INSTANCE_STATE_EXITED;
			errno = ECHILD;
			return -1;
		} else if (!WIFSTOPPED(*wstatus) || WSTOPSIG(*wstatus) != SIGTRAP ||
			   asmase_finish_create(a) == -1) {
			/*
			 * The tracee didn't bootstrap properly. Kill it and let
			 * the next call to waitpid() reap it.
			 */
			if (flags & WNOHANG)
				return 0;
			else
				goto again;
		}
	}

	if (WIFEXITED(*wstatus) || WIFSIGNALED(*wstatus)) {
		a->state = ASMASE_INSTANCE_STATE_EXITED;
		return 1;
	}

	a->state = ASMASE_INSTANCE_STATE_READY;

	return 1;
}

__attribute__((visibility("default")))
int asmase_poll(struct asmase_instance *a, int *wstatus)
{
	return asmase_waitpid(a, wstatus, WNOHANG);
}

__attribute__((visibility("default")))
int asmase_wait(struct asmase_instance *a, int *wstatus)
{
	return asmase_waitpid(a, wstatus, 0);
}
