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

#include "libasmase.h"

#include ARCH_HEADER

void libasmase_assembler_init(void);
void tracee(int memfd, size_t memfd_size, int flags) __attribute__((noreturn));

struct asmase_instance {
	/**
	 * @pid: PID of the tracee.
	 */
	pid_t pid;

	/**
	 * @memfd: memfd mapped into the tracee.
	 */
	int memfd;

	/**
	 * @memfd_size: size of the memfd mapping.
	 */
	size_t memfd_size;

	/**
	 * @memfd_addr: address of the memfd mapping in the tracee.
	 */
	void *memfd_addr;
};

extern const unsigned char arch_trap_instruction[];
extern const size_t arch_trap_instruction_len;

int arch_initialize_tracee_regs(pid_t pid, void *pc, void *sp);
int arch_set_tracee_program_counter(pid_t pid, void *pc);

int arch_assemble_munmap(struct asmase_assembler *as,
			 unsigned long munmap_start, unsigned long munmap_len,
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
