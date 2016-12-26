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
#include <errno.h>
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

__attribute__((visibility("default")))
int libasmase_init(void)
{
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

static char *tracee_path(void)
{
	const char *libexec;
	char *path;
	int ret;

	libexec = getenv("ASMASE_LIBEXEC");
	if (!libexec)
		libexec = LIBEXEC;

	ret = asprintf(&path, "%s/asmase_tracee", libexec);
	if (ret == -1)
		return NULL;
	return path;
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
	}

	return argv;
err:
	argv_free(argv);
	return NULL;
}

static pid_t exec_tracee(struct asmase_instance *a, int flags)
{
	posix_spawn_file_actions_t file_actions;
	posix_spawnattr_t attr;
	char *path;
	char **argv;
	char **envp;
	char *empty_environ[] = {NULL};
	pid_t pid = -1;

	errno = posix_spawn_file_actions_init(&file_actions);
	if (errno)
		goto out;

	errno = posix_spawnattr_init(&attr);
	if (errno)
		goto out_file_actions;

	path = tracee_path();
	if (!path)
		goto out_spawnattr;

	argv = tracee_argv(a, flags);
	if (!argv)
		goto out_path;

	envp = (flags & ASMASE_SANDBOX_ENVIRON) ? empty_environ : environ;

	errno = posix_spawn(&pid, path, &file_actions, &attr, argv, envp);
	if (errno)
		goto out_argv;

out_argv:
	argv_free(argv);
out_path:
	free(path);
out_spawnattr:
	posix_spawnattr_destroy(&attr);
out_file_actions:
	posix_spawn_file_actions_destroy(&file_actions);
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

__attribute__((visibility("default")))
struct asmase_instance *asmase_create_instance(int flags)
{
	struct asmase_instance *a;

	if (flags & ~(ASMASE_SANDBOX_FDS | ASMASE_SANDBOX_SYSCALLS |
		      ASMASE_SANDBOX_ENVIRON)) {
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

	if (attach_to_tracee(a) == -1) {
		int saved_errno = errno;
		asmase_destroy_instance(a);
		errno = saved_errno;
		return NULL;
	}

	return a;
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
pid_t asmase_get_pid(const struct asmase_instance *a)
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

retry:
	if (ptrace(PTRACE_CONT, a->pid, NULL, NULL) == -1)
		return -1;

	if (waitpid(a->pid, wstatus, 0) == -1)
		return -1;

	if (WIFSTOPPED(*wstatus) && WSTOPSIG(*wstatus) == SIGWINCH)
		goto retry;

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
