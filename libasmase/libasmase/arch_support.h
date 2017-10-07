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

#include "internal.h"

/*
 * Architecture support requires defining a few things:
 *
 * 1. A fixed address where the memfd should be mapped (MEMFD_ADDR).
 * 2. A type for storing register values (struct arch_regs).
 * 3. A function to get the register values of a tracee (arch_get_regs()).
 * 4. A function to set the register values of a tracee (arch_set_regs()).
 * 5. The architecture registor descriptors (DEFINE_ARCH_REGISTERS() and
 *    REGISTER_DESCRIPTOR()).
 * 6. A trap instruction (arch_trap_instruction{,_len}).
 * 7. Code to bootstrap the tracee (arch_bootstrap_code{,_len}).
 */

void default_copy_register(const struct arch_register_descriptor *desc,
			   void *dst, const struct arch_regs *src);

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

#define __REGISTER_DESCRIPTOR(name_, type_, field, status_bits_, num_status_bits_,	\
			      fn) {							\
	.name = name_,									\
	.offset = offsetof(struct arch_regs, field),					\
	.size = ASMASE_REGISTER_TYPE_SIZEOF(type_),					\
	.status_bits = status_bits_,							\
	.num_status_bits = num_status_bits_,						\
	.type = type_,									\
	.copy_register_fn = fn,								\
}

#define REGISTER_DESCRIPTOR(name, type, field)	\
	__REGISTER_DESCRIPTOR(name, type, field, NULL, 0, default_copy_register)

#define STATUS_REGISTER_DESCRIPTOR_FN(reg, type, field, fn)		\
	__REGISTER_DESCRIPTOR(#reg, type, field, arch_##reg##_bits,	\
			      ARRAY_SIZE(arch_##reg##_bits), fn)

#define STATUS_REGISTER_DESCRIPTOR(reg, type, field)	\
	STATUS_REGISTER_DESCRIPTOR_FN(reg, type, field, default_copy_register)

#define USER_REGS_DESCRIPTOR_UINT(reg, bits)	\
	REGISTER_DESCRIPTOR(#reg, ASMASE_REGISTER_U##bits, regs.reg)

#define USER_REGS_DESCRIPTOR_U16(reg) USER_REGS_DESCRIPTOR_UINT(reg, 16)
#define USER_REGS_DESCRIPTOR_U32(reg) USER_REGS_DESCRIPTOR_UINT(reg, 32)
#define USER_REGS_DESCRIPTOR_U64(reg) USER_REGS_DESCRIPTOR_UINT(reg, 64)

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
