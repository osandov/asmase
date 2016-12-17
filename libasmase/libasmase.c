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

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ptrace.h>
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

/*
 * Entry point for the tracee. Requests to be ptraced and traps.
 *
 * TODO: sandboxing. seccomp() disabling all syscalls should suffice, but for
 * defense-in-depth, let's close all fds and unmap as much memory as possible.
 * Wihtout sandboxing, might also want to do something special with ptrace() for
 * clone and exec.
 */
static void tracee(void) __attribute__((noreturn));
static void tracee(void)
{
	if (ptrace(PTRACE_TRACEME, -1, NULL, NULL) == -1) {
		perror("ptrace");
		abort();
	}

	if (raise(SIGTRAP)) {
		perror("raise");
		abort();
	}

	/* We shouldn't make it here. */
	abort();
}

static int attach_to_tracee(struct asmase_instance *a)
{
	if (waitpid(a->pid, NULL, 0) == -1)
		return -1;

	/*
	 * This was introduced in Linux 3.8, so it might make sense to make this
	 * a non-fatal error.
	 */
	if (ptrace(PTRACE_SETOPTIONS, a->pid, NULL, PTRACE_O_EXITKILL) == -1)
		return -1;

	return 0;
}

__attribute__((visibility("default")))
struct asmase_instance *asmase_create_instance(void)
{
	struct asmase_instance *a;

	a = malloc(sizeof(*a));
	if (!a)
		return NULL;

	a->shared_size = sysconf(_SC_PAGESIZE);
	a->shared_mem = mmap(NULL, a->shared_size,
			     PROT_READ | PROT_WRITE | PROT_EXEC,
			     MAP_ANONYMOUS | MAP_SHARED, -1, 0);
	if (a->shared_mem == MAP_FAILED) {
		free(a);
		return NULL;
	}

	a->pid = fork();
	if (a->pid == -1) {
		munmap(a->shared_mem, a->shared_size);
		free(a);
		return NULL;
	}

	if (a->pid == 0)
		tracee(); /* This doesn't return. */

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
	munmap(a->shared_mem, a->shared_size);
	free(a);
}

__attribute__((visibility("default")))
int asmase_execute_code(const struct asmase_instance *a,
			const unsigned char *code, size_t len, int *wstatus)
{
	unsigned char *p = a->shared_mem;
	size_t total_len;
	bool overflow;

	overflow = __builtin_add_overflow(len, arch_trap_instruction_len,
					  &total_len);
	if (overflow || total_len >= a->shared_size) {
		errno = E2BIG;
		return -1;
	}

	memcpy(p, code, len);
	p += len;
	memcpy(p, arch_trap_instruction, arch_trap_instruction_len);

	if (arch_set_tracee_program_counter(a->pid, a->shared_mem) == -1)
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
