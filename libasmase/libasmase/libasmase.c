/*
 * Core libasmase support.
 *
 * Copyright (C) 2016-2017 Omar Sandoval
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
#include <errno.h>
#include <fcntl.h>
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
#include <sys/uio.h>
#include <sys/wait.h>

#include "internal.h"

#ifndef MFD_CLOEXEC
#define MFD_CLOEXEC 0x0001U
#endif

/* 64k per tracee by default; 4k for code, the rest for the stack. */
#define MEMFD_SIZE 65536
#define CODE_MAX_SIZE 4096

__attribute__((visibility("default")))
int libasmase_init(void)
{
	libasmase_assembler_init();
	return 0;
}

static int create_memfd(struct asmase_instance *a)
{
	a->memfd_size = MEMFD_SIZE;
	a->memfd = syscall(SYS_memfd_create, "asmase", MFD_CLOEXEC);
	if (a->memfd == -1)
		return -1;

	if (ftruncate(a->memfd, a->memfd_size) == -1) {
		close(a->memfd);
		return -1;
	}

	return 0;
}

static pid_t fork_tracee(struct asmase_instance *a, int flags)
{
	pid_t pid = -1;

	pid = fork();
	if (pid == 0)
		tracee(a->memfd, a->memfd_size, flags); /* Doesn't return. */
	else
		return pid;
}

static int attach_to_tracee(struct asmase_instance *a)
{
	int wstatus;
	ssize_t sret;

	for (;;) {
		if (waitpid(a->pid, &wstatus, 0) == -1)
			return -1;

		/*
		 * If the tracee exited or was signaled before it requested to
		 * be ptrace'd, we can't attach to it.
		 */
		if (!WIFSTOPPED(wstatus)) {
			errno = ECHILD;
			return -1;
		}

		/*
		 * The tracee will fill in the address that it mapped the memfd
		 * into when it's done setting up.
		 */
		sret = pread(a->memfd, &a->memfd_addr, sizeof(a->memfd_addr), 0);
		if (sret == -1)
			return -1;
		assert(sret == sizeof(a->memfd_addr));
		if (a->memfd_addr)
			break;

		if (ptrace(PTRACE_CONT, a->pid, NULL, NULL) == -1)
			return -1;
	}

	if (ptrace(PTRACE_SETOPTIONS, a->pid, NULL, PTRACE_O_EXITKILL) == -1)
		return -1;

	return 0;
}

struct proc_map {
	unsigned long start, end;
	char *path;
	struct proc_map *next;
};

static void free_proc_maps(struct proc_map *maps)
{
	while (maps) {
		struct proc_map *next = maps->next;

		free(maps->path);
		free(maps);
		maps = next;
	}
}

static struct proc_map *read_proc_maps(pid_t pid)
{
	FILE *file;
	char path[50];
	struct proc_map *maps = NULL, *tail;
	int ret;

	snprintf(path, sizeof(path), "/proc/%ld/maps", (long)pid);

	file = fopen(path, "r");
	if (!file)
		return NULL;

	for (;;) {
		struct proc_map *map;
		unsigned long start, end;
		char c;

		ret = fscanf(file, "%lx-%lx %*c%*c%*c%*c %*x %*x:%*x %*u",
			     &start, &end);
		if (ret == EOF)
			break;

		map = malloc(sizeof(*map));
		if (!map)
			goto err;
		map->start = start;
		map->end = end;
		map->path = NULL;
		map->next = NULL;
		if (!maps) {
			maps = tail = map;
		} else {
			tail->next = map;
			tail = map;
		}

		ret = fscanf(file, "%*[ ]%m[^\n]", &map->path);
		if (ret == EOF) {
			if (!ferror(file))
				errno = EINVAL;
			goto err;
		}

		c = fgetc(file);
		if (c == EOF) {
			if (!ferror(file))
				errno = EINVAL;
			goto err;
		}
		assert(c == '\n');
	}
	if (ferror(file))
		goto err;

	fclose(file);
	return maps;

err:
	free_proc_maps(maps);
	fclose(file);
	return NULL;
}

/*
 * Unmap everything expect for the memfd and the vsyscall, which apparently
 * can't be unmapped.
 */
static bool should_munmap(struct proc_map *map)
{
	return (!map->path ||
		(map->path[0] != '/' && strcmp(map->path, "[vsyscall]") != 0) ||
		(map->path[0] == '/' && !strstartswith(map->path, "/memfd:asmase")));
}

static int do_munmap_tracee(struct asmase_instance *a,
			    struct asmase_assembler *as,
			    struct proc_map *maps)
{
	struct proc_map *map;
	int ret;

	for (map = maps; map; map = map->next) {
		char *out;
		size_t len;

		if (should_munmap(map)) {
			ret = arch_assemble_munmap(as, map->start,
						   map->end - map->start, &out, &len);
			if (ret == -1)
				return -1;

			ret = asmase_execute_code(a, out, len, NULL);
			free(out);
			if (ret == -1)
				return -1;
		}
	}

	return 0;
}

static int check_munmap(struct proc_map *maps)
{
	struct proc_map *map;

	for (map = maps; map; map = map->next) {
		if (should_munmap(map)) {
			/*
			 * Ugly overloading of this errno but it's at least
			 * unique.
			 */
			errno = EADDRINUSE;
			return -1;
		}
	}

	return 0;
}

static int munmap_tracee(struct asmase_instance *a)
{
	struct asmase_assembler *as;
	struct proc_map *maps = NULL;
	int saved_errno;
	int ret;

	as = asmase_create_assembler();
	if (!as) {
		ret = -1;
		goto out;
	}

	maps = read_proc_maps(a->pid);
	if (!maps) {
		ret = -1;
		goto out;
	}

	ret = do_munmap_tracee(a, as, maps);
	if (ret == -1)
		goto out;

	free_proc_maps(maps);
	maps = read_proc_maps(a->pid);
	if (!maps) {
		ret = -1;
		goto out;
	}

	ret = check_munmap(maps);

out:
	saved_errno = errno;
	free_proc_maps(maps);
	if (as)
		asmase_destroy_assembler(as);
	errno = saved_errno;
	return ret;
}

__attribute__((visibility("default")))
struct asmase_instance *asmase_create_instance(int flags)
{
	struct asmase_instance *a;
	int saved_errno;
	void *sp;

	if (flags & ~(ASMASE_SANDBOX_ALL)) {
		errno = EINVAL;
		return NULL;
	}

	a = malloc(sizeof(*a));
	if (!a)
		return NULL;

	if (create_memfd(a) == -1) {
		free(a);
		return NULL;
	}

	a->pid = fork_tracee(a, flags);
	if (a->pid == -1) {
		close(a->memfd);
		free(a);
		return NULL;
	}

	if (attach_to_tracee(a) == -1)
		goto err;

	if (munmap_tracee(a) == -1)
		goto err;

	/* Assumes a stack which grows down. */
	sp = (char *)a->memfd_addr + a->memfd_size - sizeof(long);
	if (arch_initialize_tracee_regs(a->pid, a->memfd_addr, sp) == -1)
		goto err;

	return a;

err:
	saved_errno = errno;
	asmase_destroy_instance(a);
	errno = saved_errno;
	return NULL;
}

__attribute__((visibility("default")))
void asmase_destroy_instance(struct asmase_instance *a)
{
	kill(a->pid, SIGKILL);
	waitpid(a->pid, NULL, 0);
	close(a->memfd);
	free(a);
}

__attribute__((visibility("default")))
pid_t asmase_getpid(const struct asmase_instance *a)
{
	return a->pid;
}

__attribute__((visibility("default")))
int asmase_execute_code(const struct asmase_instance *a,
			const char *code, size_t len, int *wstatus)
{
	struct iovec iov[] = {
		{(void *)code, len},
		{(void *)arch_trap_instruction, arch_trap_instruction_len},
	};
	ssize_t sret;
	size_t total_len;
	bool overflow;

	overflow = __builtin_add_overflow(len, arch_trap_instruction_len,
					  &total_len);
	if (overflow || total_len > CODE_MAX_SIZE) {
		errno = E2BIG;
		return -1;
	}

	sret = pwritev(a->memfd, iov, ARRAY_SIZE(iov), 0);
	if (sret == -1)
		return -1;

	if (arch_set_tracee_program_counter(a->pid, a->memfd_addr) == -1) {
		if (errno == ESRCH)
			goto wait;
		return -1;
	}
	if (ptrace(PTRACE_CONT, a->pid, NULL, NULL) == -1 && errno != ESRCH)
		return -1;

wait:
	if (waitpid(a->pid, wstatus, 0) == -1)
		return -1;

	return 0;
}

__attribute__((visibility("default")))
ssize_t asmase_readv_memory(const struct asmase_instance *a,
			    const struct iovec *local_iov, size_t liovcnt,
			    const struct iovec *remote_iov, size_t riovcnt)
{
	return process_vm_readv(a->pid, local_iov, liovcnt, remote_iov, riovcnt,
				0);
}

__attribute__((visibility("default")))
ssize_t asmase_read_memory(const struct asmase_instance *a, void *buf,
			   void *addr, size_t len)
{
	struct iovec local_iov = {
		.iov_base = buf,
		.iov_len = len,
	};
	struct iovec remote_iov = {
		.iov_base = addr,
		.iov_len = len,
	};

	return process_vm_readv(a->pid, &local_iov, 1, &remote_iov, 1, 0);
}
