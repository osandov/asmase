/*
 * x86_64 support.
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

#include <errno.h>
#include <sys/ptrace.h>
#include <sys/uio.h>

#include "internal.h"

/* sizeof(struct user_regs_struct) */
#define USER_REGS_SIZE 216
/* sizeof(struct user_fpregs_struct) */
#define USER_FPREGS_SIZE 512
/* offsetof(struct user, regs.rip) */
#define RIP_OFFSET 128

__attribute__((visibility("default")))
int asmase_get_registers(const struct asmase_instance *a, void *buf,
			 size_t len)
{
	struct iovec iov;

	BUILD_BUG_ON(USER_REGS_SIZE + USER_FPREGS_SIZE != X86_64_REGS_SIZE);

	if (len != X86_64_REGS_SIZE) {
		errno = EINVAL;
		return -1;
	}

	iov.iov_base = buf;
	iov.iov_len = USER_REGS_SIZE;
	if (ptrace(PTRACE_GETREGSET, a->pid, NT_PRSTATUS, &iov) == -1)
		return -1;

	iov.iov_base = (char *)buf + USER_REGS_SIZE;
	iov.iov_len = USER_FPREGS_SIZE;
	if (ptrace(PTRACE_GETREGSET, a->pid, NT_FPREGSET, &iov) == -1)
		return -1;

	return 0;
}

__attribute__((visibility("default")))
int asmase_set_registers(struct asmase_instance *a, const void *buf,
			 size_t len)
{
	struct iovec iov;

	if (len != X86_64_REGS_SIZE) {
		errno = EINVAL;
		return -1;
	}

	iov.iov_base = (void *)buf;
	iov.iov_len = USER_REGS_SIZE;
	if (ptrace(PTRACE_SETREGSET, a->pid, NT_PRSTATUS, &iov) == -1)
		return -1;

	iov.iov_base = (char *)buf + USER_REGS_SIZE;
	iov.iov_len = USER_FPREGS_SIZE;
	if (ptrace(PTRACE_SETREGSET, a->pid, NT_FPREGSET, &iov) == -1)
		return -1;

	return 0;
}

int arch_reset_program_counter(struct asmase_instance *a)
{
	return ptrace(PTRACE_POKEUSER, a->pid, (void *)RIP_OFFSET,
		      (void *)X86_64_SHMEM_ADDR);
}
