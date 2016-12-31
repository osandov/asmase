/*
 * Core libasmase support.
 *
 * Copyright (C) 2016 Omar Sandoval
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
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <spawn.h>
#include <stdarg.h>
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

static char libasmase_path[PATH_MAX];

int main(int argc, char **argv);

__attribute__((visibility("default")))
int libasmase_init(void)
{
	Dl_info info;

	if (!dladdr(main, &info)) {
		errno = ENOEXEC;
		return -1;
	}

	if (!realpath(info.dli_fname, libasmase_path))
		return -1;

	libasmase_assembler_init();
	return 0;
}

static int create_memfd(struct asmase_instance *a)
{
	void *memfd_addr = NULL;

	a->memfd_size = sysconf(_SC_PAGESIZE);
	a->memfd = syscall(SYS_memfd_create, "asmase_tracee", 0);
	if (a->memfd == -1)
		return -1;

	/*
	 * The tracee will fill in the address that it mapped the memfd into
	 * when it's done setting up.
	 */
	if (pwrite(a->memfd, &memfd_addr, sizeof(memfd_addr), 0) == -1) {
		close(a->memfd);
		return -1;
	}

	return 0;
}

static char **argv_alloc(void)
{
	char **argv;

	argv = malloc(sizeof(char *));
	if (argv)
		argv[0] = NULL;
	return argv;
}

static void argv_free(char **argv)
{
	int i;

	for (i = 0; argv[i]; i++)
		free(argv[i]);
	free(argv);
}

static int argv_appendf(char ***argv, int *argc, const char *format, ...)
{
	va_list ap;
	char **tmp;
	char *arg;
	int ret;

	tmp = realloc(*argv, sizeof(char *) * (*argc + 2));
	if (!tmp)
		return -1;
	*argv = tmp;

	va_start(ap, format);
	ret = vasprintf(&arg, format, ap);
	if (ret == -1)
		return -1;

	(*argv)[(*argc)++] = arg;
	(*argv)[*argc] = NULL;
	return 0;
}

static char **tracee_argv(const struct asmase_instance *a, int flags)
{
	char **argv;
	int argc = 0;
	int ret;

	argv = argv_alloc();
	if (!argv)
		return NULL;

	ret = argv_appendf(&argv, &argc, "asmase_tracee");
	if (ret == -1)
		goto err;

	ret = argv_appendf(&argv, &argc, "--memfd=%d", a->memfd);
	if (ret == -1)
		goto err;

	ret = argv_appendf(&argv, &argc, "--memfd-size=%zu", a->memfd_size);
	if (ret == -1)
		goto err;

	if (flags & ASMASE_SANDBOX_FDS) {
		ret = argv_appendf(&argv, &argc, "--close-fds");
		if (ret == -1)
			goto err;
	}

	if (flags & ASMASE_SANDBOX_SYSCALLS) {
		ret = argv_appendf(&argv, &argc, "--seccomp");
		if (ret == -1)
			goto err;

		if (flags & ASMASE_MUNMAP_ALL) {
			ret = argv_appendf(&argv, &argc, "--allow-munmap");
			if (ret == -1)
				goto err;
		}
	}

	return argv;
err:
	argv_free(argv);
	return NULL;
}

static pid_t exec_tracee(struct asmase_instance *a, int flags)
{
	char **argv, **envp;
	char *empty_environ[] = {NULL};
	pid_t pid = -1;

	argv = tracee_argv(a, flags);
	if (!argv)
		goto out;

	envp = (flags & ASMASE_SANDBOX_ENVIRON) ? empty_environ : environ;

	errno = posix_spawn(&pid, libasmase_path, NULL, NULL, argv, envp);
	if (errno) {
		pid = -1;
		goto out_argv;
	}

out_argv:
	argv_free(argv);
out:
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

static int do_munmap_tracee(struct asmase_instance *a,
			    struct asmase_assembler *as,
			    struct proc_map *maps, int flags)
{
	struct proc_map *map;
	int ret;

	for (map = maps; map; map = map->next) {
		int mask;
		char *out;
		size_t len;

		if (map->path) {
			if (strstartswith(map->path, "/memfd:asmase_tracee"))
				mask = 0;
			else if (map->path[0] == '/')
				mask = ASMASE_MUNMAP_FILE;
			else if (strcmp(map->path, "[heap]") == 0)
				mask = ASMASE_MUNMAP_HEAP;
			else
				mask = 0;
		} else {
			mask = ASMASE_MUNMAP_ANON;
		}

		if (flags & mask) {
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

static int munmap_tracee(struct asmase_instance *a, int flags)
{
	struct asmase_assembler *as;
	struct proc_map *maps = NULL, *tail;
	FILE *file = NULL;
	char path[50];
	int ret = -1;

	as = asmase_create_assembler();
	if (!as)
		goto out;

	snprintf(path, sizeof(path), "/proc/%ld/maps", (long)a->pid);

	file = fopen(path, "r");
	if (!file)
		goto out;

	for (;;) {
		struct proc_map *map;
		unsigned long start, end;
		char c;

		ret = fscanf(file, "%lx-%lx %*c%*c%*c%*c %*x %*x:%*x %*u",
			     &start, &end);
		if (ret == EOF)
			break;

		map = malloc(sizeof(*map));
		if (!map) {
			ret = -1;
			goto out;
		}
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
			ret = -1;
			goto out;
		}

		c = fgetc(file);
		if (c == EOF) {
			if (!ferror(file))
				errno = EINVAL;
			ret = -1;
			goto out;
		}
		assert(c == '\n');
	}
	if (ferror(file)) {
		ret = -1;
		goto out;
	}

	ret = do_munmap_tracee(a, as, maps, flags);

out:
	while (maps) {
		struct proc_map *next = maps->next;

		free(maps->path);
		free(maps);
		maps = next;
	}
	if (file)
		fclose(file);
	if (as)
		asmase_destroy_assembler(as);
	return ret;
}

__attribute__((visibility("default")))
struct asmase_instance *asmase_create_instance(int flags)
{
	struct asmase_instance *a;
	int saved_errno;

	if (flags & ~(ASMASE_SANDBOX_ALL | ASMASE_MUNMAP_ALL)) {
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

	a->pid = exec_tracee(a, flags);
	if (a->pid == -1) {
		close(a->memfd);
		free(a);
		return NULL;
	}

	if (attach_to_tracee(a) == -1)
		goto err;

	if (flags & ASMASE_MUNMAP_ALL) {
		if (munmap_tracee(a, flags & ASMASE_MUNMAP_ALL) == -1)
			goto err;
	}

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
	if (overflow || total_len >= a->memfd_size) {
		errno = E2BIG;
		return -1;
	}

	sret = pwritev(a->memfd, iov, ARRAY_SIZE(iov), 0);
	if (sret == -1)
		return -1;

	if (arch_set_tracee_program_counter(a->pid, a->memfd_addr) == -1)
		return -1;

	if (ptrace(PTRACE_CONT, a->pid, NULL, NULL) == -1)
		return -1;

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
