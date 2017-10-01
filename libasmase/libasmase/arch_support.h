/*
 * Helpers for architecture support.
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

#ifndef LIBASMASE_ARCH_SUPPORT_H
#define LIBASMASE_ARCH_SUPPORT_H

#include <assert.h>
#include <stdbool.h>
#include <sys/user.h>

#include "util.h"

/*
 * Architecture support requires defining a few things:
 *
 * 1. A trap instruction (arch_trap_instruction{,_len}).
 * 2. A function to initialize all registers.
 * 3. A function to set the program counter of a tracee.
 * 4. A function to assemble a call to the munmap() syscall.
 * 5. The architecture ptrace register sets (DEFINE_ARCH_PTRACE_REGSETS<n>()).
 * 6. The architecture registor descriptors (DEFINE_ARCH_REGISTERS() and
 *    REGISTER_DESCRIPTOR()).
 */

struct arch_register_descriptor {
	const char *name;
	size_t offset;
	size_t size;
	const struct asmase_status_register_bits *status_bits;
	size_t num_status_bits;
	int ptrace_regset;
	enum asmase_register_type type;
	void (*copy_register_fn)(const struct arch_register_descriptor *, void *, void *);
};

void default_copy_register(const struct arch_register_descriptor *desc,
			   void *dst, void *src);

/**
 * DEFINE_ARCH_PTRACE_REGSETS<n> - Define the architecture ptrace register sets.
 *
 * @type<n>: Type of the nth register set.
 * @nt<n>: NT_* constant of the nth register set.
 */
#define DEFINE_ARCH_PTRACE_REGSETS2(type1, nt1, type2, nt2)		\
enum {									\
	ARCH_PTRACE_##nt1,						\
	ARCH_PTRACE_##nt2,						\
	ARCH_NUM_PTRACE_REGSETS,					\
};									\
struct arch_ptrace_regsets {						\
	type1 ARCH_PTRACE_##nt1;					\
	type2 ARCH_PTRACE_##nt2;					\
};									\
static inline size_t arch_ptrace_regset_nt(int ptrace_regset)		\
{									\
	switch (ptrace_regset) {					\
	case ARCH_PTRACE_##nt1:						\
		return nt1;						\
	case ARCH_PTRACE_##nt2:						\
		return nt2;						\
	default:							\
		assert(false);						\
		return -1;						\
	}								\
}									\
static inline size_t arch_ptrace_regset_offsetof(int ptrace_regset)	\
{									\
	switch (ptrace_regset) {					\
	case ARCH_PTRACE_##nt1:						\
		return offsetof(struct arch_ptrace_regsets,		\
				ARCH_PTRACE_##nt1);			\
	case ARCH_PTRACE_##nt2:						\
		return offsetof(struct arch_ptrace_regsets,		\
				ARCH_PTRACE_##nt2);			\
	default:							\
		assert(false);						\
		return -1;						\
	}								\
}									\
static inline size_t arch_ptrace_regset_sizeof(int ptrace_regset)	\
{									\
	switch (ptrace_regset) {					\
	case ARCH_PTRACE_##nt1:						\
		return FIELD_SIZEOF(struct arch_ptrace_regsets,		\
				    ARCH_PTRACE_##nt1);			\
	case ARCH_PTRACE_##nt2:						\
		return FIELD_SIZEOF(struct arch_ptrace_regsets,		\
				    ARCH_PTRACE_##nt2);			\
	default:							\
		assert(false);						\
		return -1;						\
	}								\
}

#define __ASMASE_REGISTER_TYPE_SIZEOF(type)				\
	(((type) == ASMASE_REGISTER_U8) ? sizeof(uint8_t) :		\
	 ((type) == ASMASE_REGISTER_U16) ? sizeof(uint16_t) :		\
	 ((type) == ASMASE_REGISTER_U32) ? sizeof(uint32_t) :		\
	 ((type) == ASMASE_REGISTER_U64) ? sizeof(uint64_t) :		\
	 ((type) == ASMASE_REGISTER_U128) ? 2 * sizeof(uint64_t) :	\
	 ((type) == ASMASE_REGISTER_FLOAT80) ? sizeof(long double) :	\
	 0)

#define ASMASE_REGISTER_TYPE_SIZEOF(type)				\
	(__ASMASE_REGISTER_TYPE_SIZEOF(type) +				\
	 BUILD_BUG_ON_ZERO(__ASMASE_REGISTER_TYPE_SIZEOF(type) == 0))

#define ____REGISTER_DESCRIPTOR(name_, type_, nt, field, status_bits_,			\
				num_status_bits_, check_size, fn) {			\
	.name = name_,									\
	.offset = offsetof(FIELD_TYPEOF(struct arch_ptrace_regsets, nt), field),	\
	.size = ASMASE_REGISTER_TYPE_SIZEOF(type_) +					\
		BUILD_BUG_ON_ZERO(check_size &&						\
				  FIELD_SIZEOF(FIELD_TYPEOF(struct arch_ptrace_regsets, nt), field) !=	\
				  ASMASE_REGISTER_TYPE_SIZEOF(type_)),			\
	.status_bits = status_bits_,							\
	.num_status_bits = num_status_bits_,						\
	.ptrace_regset = nt,								\
	.type = type_,									\
	.copy_register_fn = fn,								\
}

#define __REGISTER_DESCRIPTOR(name, type, nt, field)	\
	____REGISTER_DESCRIPTOR(name, type, nt, field, NULL, 0, false, default_copy_register)

#define REGISTER_DESCRIPTOR(name, type, nt, field)	\
	____REGISTER_DESCRIPTOR(name, type, nt, field, NULL, 0, true, default_copy_register)

#define STATUS_REGISTER_DESCRIPTOR_FN(name, type, nt, field, fn)		\
	____REGISTER_DESCRIPTOR(#name, type, nt, field, arch_##name##_bits,	\
				ARRAY_SIZE(arch_##name##_bits), true, fn)

#define STATUS_REGISTER_DESCRIPTOR(name, type, nt, field)		\
	STATUS_REGISTER_DESCRIPTOR_FN(name, type, nt, field, default_copy_register)

#define __USER_REGS_DESCRIPTOR_UINT(reg, reg_name, bits) \
	REGISTER_DESCRIPTOR(reg_name, ASMASE_REGISTER_U##bits, ARCH_PTRACE_NT_PRSTATUS, reg)

#define USER_REGS_DESCRIPTOR_U16_NAMED(reg, name) __USER_REGS_DESCRIPTOR_UINT(reg, name, 16)
#define USER_REGS_DESCRIPTOR_U32_NAMED(reg, name) __USER_REGS_DESCRIPTOR_UINT(reg, name, 32)
#define USER_REGS_DESCRIPTOR_U64_NAMED(reg, name) __USER_REGS_DESCRIPTOR_UINT(reg, name, 64)

#define USER_REGS_DESCRIPTOR_U16(reg) USER_REGS_DESCRIPTOR_U16_NAMED(reg, #reg)
#define USER_REGS_DESCRIPTOR_U32(reg) USER_REGS_DESCRIPTOR_U32_NAMED(reg, #reg)
#define USER_REGS_DESCRIPTOR_U64(reg) USER_REGS_DESCRIPTOR_U64_NAMED(reg, #reg)

/**
 * DEFINE_ARCH_REGISTERS() - Define a set of architecture registers.
 *
 * @regset: Name of the register set (e.g., general_purpose, status).
 * @...: struct arch_register_descriptor initializers.
 */
#define DEFINE_ARCH_REGISTERS(regset, ...)						\
	const struct arch_register_descriptor arch_##regset##_regs[] = {__VA_ARGS__};	\
	const size_t arch_num_##regset##_regs = ARRAY_SIZE(arch_##regset##_regs)

#define DEFINE_STATUS_REGISTER_BITS(name, ...)						\
	const struct asmase_status_register_bits arch_##name##_bits[] = {__VA_ARGS__}

#define STATUS_REGISTER_BIT_VALUES(name_, shift_, mask_, ...) {	\
	.name = name_,						\
	.values = (const char *[]){__VA_ARGS__},		\
	.shift = shift_,					\
	.mask = mask_,						\
}

#define STATUS_REGISTER_BITS(name_, shift_, mask_) {	\
	.name = name_,					\
	.values = NULL,					\
	.shift = shift_,				\
	.mask = mask_,					\
}

#define STATUS_REGISTER_FLAG(name, shift) STATUS_REGISTER_BITS(name, shift, 0x1)

#endif /* LIBASMASE_ARCH_SUPPORT_H */
