/*
 * Internal libasmase interface.
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

#ifndef LIBASMASE_INTERNAL_H
#define LIBASMASE_INTERNAL_H

#include <elf.h>
#include <stddef.h>
#include <sys/types.h>

#include <libasmase.h>

#include "asm.h"
#include "x86.h"

void libasmase_assembler_init(void);
void tracee(int memfd, int flags) __attribute__((noreturn));

struct asmase_instance {
	/**
	 * @pid: PID of the tracee.
	 */
	pid_t pid;

	/**
	 * @memfd: memfd mapped into the tracee.
	 */
	int memfd;
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

int arch_initialize_tracee_regs(pid_t pid, void *pc, void *sp);
int arch_set_tracee_program_counter(pid_t pid, void *pc);

int arch_assemble_munmap(unsigned long munmap_start, unsigned long munmap_len,
			 char **out, size_t *len);

extern const struct arch_register_descriptor arch_program_counter_reg;

extern const struct arch_register_descriptor arch_segment_regs[];
extern const size_t arch_num_segment_regs;

extern const struct arch_register_descriptor arch_general_purpose_regs[];
extern const size_t arch_num_general_purpose_regs;
extern const struct arch_register_descriptor arch_status_regs[];
extern const size_t arch_num_status_regs;

extern const struct arch_register_descriptor arch_floating_point_regs[];
extern const size_t arch_num_floating_point_regs;
extern const struct arch_register_descriptor arch_floating_point_status_regs[];
extern const size_t arch_num_floating_point_status_regs;

extern const struct arch_register_descriptor arch_vector_regs[];
extern const size_t arch_num_vector_regs;
extern const struct arch_register_descriptor arch_vector_status_regs[];
extern const size_t arch_num_vector_status_regs;

#endif /* LIBASMASE_INTERNAL_H */
