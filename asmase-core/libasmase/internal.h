/*
 * Internal libasmase interface.
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

#ifndef LIBASMASE_INTERNAL_H
#define LIBASMASE_INTERNAL_H

#include "arch.h"
/* 64k per tracee by default; 4k for code, the rest for the stack. */
#define MEMFD_SIZE 65536
#define CODE_MAX_SIZE 4096

#ifndef __ASSEMBLER__
#include <elf.h>
#include <stddef.h>
#include <sys/types.h>

#include <libasmase.h>

#include "util.h"

void tracee(int memfd, int flags) __attribute__((noreturn));

enum asmase_instance_state {
	ASMASE_INSTANCE_STATE_NEW,
	ASMASE_INSTANCE_STATE_READY,
	ASMASE_INSTANCE_STATE_RUNNING,
	ASMASE_INSTANCE_STATE_EXITED,
};

struct asmase_instance {
	/** @pid: PID of the tracee. */
	pid_t pid;

	/** @memfd: memfd mapped into the tracee. */
	int memfd;

	/** @regs: buffer of register values. */
	struct arch_regs regs;

	/** @flags: flags instance was created with. */
	int flags;

	/** @state: state of the tracee. */
	enum asmase_instance_state state;
};

/**
 * arch_trap_instruction - machine code to raise SIGTRAP
 */
extern const unsigned char arch_trap_instruction[];
extern const size_t arch_trap_instruction_len;

/**
 * arch_bootstrap_code - machine code to bootstrap a new tracee
 *
 * This should be a position-independent function which doesn't return and takes
 * the arguments specified by arch_bootstrap_func.
 *
 * Before a tracee process can be used, it must:
 * 1. Map the memfd at MEMFD_ADDR.
 * 2. Close the memfd.
 * 3. Reset the instruction pointer and stack pointer to lie within the memfd
 *    mapping.
 * 4. Unmap all memory other than the memfd mapping.
 * 5. Optionally set up seccomp.
 * 6. Reset all registers, either to 0 or to some other clean state.
 *
 * Before calling the bootstrap code, the tracee copies the code into the memfd
 * and maps the memfd at an initial location. Because the final location (i.e.,
 * MEMFD_ADDR) might already be occupied by another mapping in the process, the
 * initial location may not be the same as the final location. If it is not,
 * then the tracee guarantees that it does not overlap with the final location.
 * Therefore, the bootstrap code can always safely map the memfd at MEMFD_ADDR
 * with MAP_FIXED and then jump to the memfd mapping, preserving the instruction
 * pointer's current offset from the beginning of the bootstrap code.
 */
extern const unsigned char arch_bootstrap_code[];
extern const size_t arch_bootstrap_code_len;

/**
 * arch_bootstrap_func - function signature of arch_bootstrap_code
 */
typedef void (*arch_bootstrap_func)(int memfd, bool seccomp) __attribute__((noreturn));

int arch_get_regs(pid_t pid, struct arch_regs *regs);
int arch_set_regs(pid_t pid, const struct arch_regs *regs);
#endif /* __ASSEMBLER__ */

#endif /* LIBASMASE_INTERNAL_H */
