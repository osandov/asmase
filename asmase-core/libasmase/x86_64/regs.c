/*
 * x86_64 register support.
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

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <asm/processor-flags.h>

#include "../arch_support.h"

int arch_get_regs(pid_t pid, struct arch_regs *regs)
{
	struct iovec iov;

	iov.iov_base = &regs->regs;
	iov.iov_len = sizeof(regs->regs);
	if (ptrace(PTRACE_GETREGSET, pid, NT_PRSTATUS, &iov) == -1)
		return -1;

	iov.iov_base = &regs->fpregs;
	iov.iov_len = sizeof(regs->fpregs);
	if (ptrace(PTRACE_GETREGSET, pid, NT_FPREGSET, &iov) == -1)
		return -1;

	return 0;
}

int arch_set_regs(pid_t pid, const struct arch_regs *regs)
{
	struct iovec iov;

	iov.iov_base = (void *)&regs->regs;
	iov.iov_len = sizeof(regs->regs);
	if (ptrace(PTRACE_SETREGSET, pid, NT_PRSTATUS, &iov) == -1)
		return -1;

	iov.iov_base = (void *)&regs->fpregs;
	iov.iov_len = sizeof(regs->fpregs);
	if (ptrace(PTRACE_SETREGSET, pid, NT_FPREGSET, &iov) == -1)
		return -1;

	return 0;
}

const struct arch_register_descriptor arch_program_counter_reg =
	USER_REGS_DESCRIPTOR_U64(rip);

DEFINE_ARCH_REGISTERS(segment,
	USER_REGS_DESCRIPTOR_U16(cs),
	USER_REGS_DESCRIPTOR_U16(ss),
	USER_REGS_DESCRIPTOR_U16(ds),
	USER_REGS_DESCRIPTOR_U16(es),
	USER_REGS_DESCRIPTOR_U16(fs),
	USER_REGS_DESCRIPTOR_U16(gs),
	USER_REGS_DESCRIPTOR_U64(fs_base),
	USER_REGS_DESCRIPTOR_U64(gs_base),
);

DEFINE_ARCH_REGISTERS(general_purpose,
	USER_REGS_DESCRIPTOR_U64(rax),
	USER_REGS_DESCRIPTOR_U64(rcx),
	USER_REGS_DESCRIPTOR_U64(rdx),
	USER_REGS_DESCRIPTOR_U64(rbx),
	USER_REGS_DESCRIPTOR_U64(rsp),
	USER_REGS_DESCRIPTOR_U64(rbp),
	USER_REGS_DESCRIPTOR_U64(rsi),
	USER_REGS_DESCRIPTOR_U64(rdi),
	USER_REGS_DESCRIPTOR_U64(r8),
	USER_REGS_DESCRIPTOR_U64(r9),
	USER_REGS_DESCRIPTOR_U64(r10),
	USER_REGS_DESCRIPTOR_U64(r11),
	USER_REGS_DESCRIPTOR_U64(r12),
	USER_REGS_DESCRIPTOR_U64(r13),
	USER_REGS_DESCRIPTOR_U64(r14),
	USER_REGS_DESCRIPTOR_U64(r15),
);

DEFINE_STATUS_REGISTER_BITS(eflags,
	STATUS_REGISTER_FLAG("CF", X86_EFLAGS_CF_BIT), /* Carry flag */
	STATUS_REGISTER_FLAG("PF", X86_EFLAGS_PF_BIT), /* Parity flag */
	STATUS_REGISTER_FLAG("AF", X86_EFLAGS_AF_BIT), /* Adjust flag */
	STATUS_REGISTER_FLAG("ZF", X86_EFLAGS_ZF_BIT), /* Zero flag */
	STATUS_REGISTER_FLAG("SF", X86_EFLAGS_SF_BIT), /* Sign flag */
	STATUS_REGISTER_FLAG("TF", X86_EFLAGS_TF_BIT), /* Trap flag */
	STATUS_REGISTER_FLAG("IF", X86_EFLAGS_IF_BIT), /* Interruption flag */
	STATUS_REGISTER_FLAG("DF", X86_EFLAGS_DF_BIT), /* Direction flag */
	STATUS_REGISTER_FLAG("OF", X86_EFLAGS_OF_BIT), /* Overflow flag */
	STATUS_REGISTER_BITS("IOPL", X86_EFLAGS_IOPL_BIT, 0x3), /* I/O privilege level */
	STATUS_REGISTER_FLAG("NT", X86_EFLAGS_NT_BIT), /* Nested task flag */
	STATUS_REGISTER_FLAG("RF", X86_EFLAGS_RF_BIT), /* Resume flag */
	STATUS_REGISTER_FLAG("VM", X86_EFLAGS_VM_BIT), /* Virtual-8086 mode */
	STATUS_REGISTER_FLAG("AC", X86_EFLAGS_AC_BIT), /* Alignment check */
	STATUS_REGISTER_FLAG("VIF", X86_EFLAGS_VIF_BIT), /* Virtual interrupt flag */
	STATUS_REGISTER_FLAG("VIP", X86_EFLAGS_VIP_BIT), /* Virtual interrupt pending flag */
	STATUS_REGISTER_FLAG("ID", X86_EFLAGS_ID_BIT), /* Identification flag */
);

DEFINE_ARCH_REGISTERS(status,
	STATUS_REGISTER_DESCRIPTOR(eflags, ASMASE_REGISTER_U64, regs.eflags),
);

/* Top physical register in x87 register stack. */
static inline uint16_t x87_st_top(uint16_t fsw)
{
	return (fsw & 0x3800) >> 11;
}

/*
 * Convert a physical x87 register number (i.e., Ri) to a logical (i.e., %st(i))
 * register number.
 */
static inline uint16_t x87_phys_to_log(uint16_t index, uint16_t top)
{
	return (index - top + 8) % 8;
}

static void x87_copy_register(const struct arch_register_descriptor *desc,
			      void *dst, const struct arch_regs *src)
{
	uint16_t fsw = src->fpregs.swd;
	const unsigned int *st_space = src->fpregs.st_space;
	uint16_t top = x87_st_top(fsw);
	uint16_t physical, logical;

	physical = desc->offset;
	logical = x87_phys_to_log(physical, top);
	*(long double *)dst = *(long double *)&st_space[4 * logical];
}

#define X87_REGISTER_DESCRIPTOR(n) {		\
	.name = "R"#n,				\
	.offset = n,				\
	.size = sizeof(long double),		\
	.type = ASMASE_REGISTER_FLOAT80,	\
	.copy_register_fn = x87_copy_register,	\
}

DEFINE_ARCH_REGISTERS(floating_point,
	X87_REGISTER_DESCRIPTOR(7),
	X87_REGISTER_DESCRIPTOR(6),
	X87_REGISTER_DESCRIPTOR(5),
	X87_REGISTER_DESCRIPTOR(4),
	X87_REGISTER_DESCRIPTOR(3),
	X87_REGISTER_DESCRIPTOR(2),
	X87_REGISTER_DESCRIPTOR(1),
	X87_REGISTER_DESCRIPTOR(0),
);

DEFINE_STATUS_REGISTER_BITS(fcw,
	/* Exception enables */
	STATUS_REGISTER_FLAG("EM=IM", 0), /* Invalid operation */
	STATUS_REGISTER_FLAG("EM=DM", 1), /* Denormalized operand */
	STATUS_REGISTER_FLAG("EM=ZM", 2), /* Zero-divide */
	STATUS_REGISTER_FLAG("EM=OM", 3), /* Overflow */
	STATUS_REGISTER_FLAG("EM=UM", 4), /* Underflow */
	STATUS_REGISTER_FLAG("EM=PM", 5), /* Precision */

	/*
	 * Rounding precision: single, (reserved), double, or extended
	 */
	STATUS_REGISTER_BIT_VALUES("PC", 8, 0x3, "SGL", "", "DBL", "EXT"),

	/*
	 * Rounding mode: to nearest, toward negative infinity, toward positive
	 * infinity, or toward zero
	 */
	STATUS_REGISTER_BIT_VALUES("RC", 10, 0x3, "RN", "R-", "R+", "RZ"),
);

#define FTW_BIT_VALUES(n)	\
	STATUS_REGISTER_BIT_VALUES("TAG("#n")", 2 * n, 0x3, "Valid", "Zero", "Special", "Empty")

DEFINE_STATUS_REGISTER_BITS(ftw,
	FTW_BIT_VALUES(0),
	FTW_BIT_VALUES(1),
	FTW_BIT_VALUES(2),
	FTW_BIT_VALUES(3),
	FTW_BIT_VALUES(4),
	FTW_BIT_VALUES(5),
	FTW_BIT_VALUES(6),
	FTW_BIT_VALUES(7),
);

DEFINE_STATUS_REGISTER_BITS(fsw,
	/* Exceptions */
	STATUS_REGISTER_FLAG("EF=IE", 0), /* Invalid operation */
	STATUS_REGISTER_FLAG("EF=DE", 1), /* Denormalized operand */
	STATUS_REGISTER_FLAG("EF=ZE", 2), /* Zero-divide */
	STATUS_REGISTER_FLAG("EF=OE", 3), /* Overflow */
	STATUS_REGISTER_FLAG("EF=UE", 4), /* Underflow */
	STATUS_REGISTER_FLAG("EF=PE", 5), /* Precision */

	STATUS_REGISTER_FLAG("SF", 6), /* Stack fault */
	STATUS_REGISTER_FLAG("ES", 7), /* Exception summary status */

	/* Condition bits */
	STATUS_REGISTER_FLAG("C0", 8),
	STATUS_REGISTER_FLAG("C1", 9),
	STATUS_REGISTER_FLAG("C2", 10),
	STATUS_REGISTER_FLAG("C3", 14),

	/* Top of the floating point stack */
	STATUS_REGISTER_BITS("TOP", 11, 0x7),

	STATUS_REGISTER_FLAG("B", 15), /* FPU busy */
);

/* Based on "Recreating FSAVE format" in the Intel Instruction Set Reference. */
static inline uint16_t x87_tag(long double st)
{
	struct x87_float {
		uint64_t fraction : 63;
		uint64_t integer : 1;
		uint64_t exponent : 15;
		uint64_t sign : 1;
	} fp;

	memcpy(&fp, &st, sizeof(fp));
	if (fp.exponent == 0x7fff)
		return 2; /* Special */
	else if (fp.exponent == 0x0000) {
		if (fp.fraction == 0 && fp.integer == 0)
			return 1; /* Zero */
		else
			return 2; /* Special */
	} else {
		if (fp.integer)
			return 0; /* Valid */
		else
			return 2; /* Special */
	}
}

/*
 * ptrace() returns the abridged floating point tag word. This reconstructs the
 * processor's full tag word from the floating point stack itself.
 */
static void ftw_copy_register(const struct arch_register_descriptor *desc,
			      void *dst, const struct arch_regs *src)
{
	uint16_t fsw = src->fpregs.swd;
	uint16_t aftw = src->fpregs.ftw;
	const unsigned int *st_space = src->fpregs.st_space;
	uint16_t top = x87_st_top(fsw);
	uint16_t ftw = 0;
	uint16_t physical;

	for (physical = 0; physical < 8; physical++) {
		uint16_t logical = x87_phys_to_log(physical, top);
		uint16_t tag;

		if (aftw & (1 << physical))
			tag = x87_tag(*(long double *)&st_space[4 * logical]);
		else
			tag = 0x3;
		ftw |= tag << (2 * physical);
	}

	*(uint16_t *)dst = ftw;
}

DEFINE_ARCH_REGISTERS(floating_point_status,
	STATUS_REGISTER_DESCRIPTOR(fcw, ASMASE_REGISTER_U16, fpregs.cwd),
	STATUS_REGISTER_DESCRIPTOR(fsw, ASMASE_REGISTER_U16, fpregs.swd),
	STATUS_REGISTER_DESCRIPTOR_FN(ftw, ASMASE_REGISTER_U16, fpregs.ftw, ftw_copy_register),
	REGISTER_DESCRIPTOR("fip", ASMASE_REGISTER_U64, fpregs.rip),
	REGISTER_DESCRIPTOR("fdp", ASMASE_REGISTER_U64, fpregs.rdp),
	REGISTER_DESCRIPTOR("fop", ASMASE_REGISTER_U16, fpregs.fop),
);

#define MMX_REG_DESCRIPTOR(num)					\
	REGISTER_DESCRIPTOR("mm" #num, ASMASE_REGISTER_U64,	\
			    fpregs.st_space[4 * num])

#define XMM_REG_DESCRIPTOR(num)					\
	REGISTER_DESCRIPTOR("xmm" #num, ASMASE_REGISTER_U128,	\
			    fpregs.xmm_space[4 * num])

DEFINE_ARCH_REGISTERS(vector,
	MMX_REG_DESCRIPTOR(0),
	MMX_REG_DESCRIPTOR(1),
	MMX_REG_DESCRIPTOR(2),
	MMX_REG_DESCRIPTOR(3),
	MMX_REG_DESCRIPTOR(4),
	MMX_REG_DESCRIPTOR(5),
	MMX_REG_DESCRIPTOR(6),
	MMX_REG_DESCRIPTOR(7),
	XMM_REG_DESCRIPTOR(0),
	XMM_REG_DESCRIPTOR(1),
	XMM_REG_DESCRIPTOR(2),
	XMM_REG_DESCRIPTOR(3),
	XMM_REG_DESCRIPTOR(4),
	XMM_REG_DESCRIPTOR(5),
	XMM_REG_DESCRIPTOR(6),
	XMM_REG_DESCRIPTOR(7),
	XMM_REG_DESCRIPTOR(8),
	XMM_REG_DESCRIPTOR(9),
	XMM_REG_DESCRIPTOR(10),
	XMM_REG_DESCRIPTOR(11),
	XMM_REG_DESCRIPTOR(12),
	XMM_REG_DESCRIPTOR(13),
	XMM_REG_DESCRIPTOR(14),
	XMM_REG_DESCRIPTOR(15),
);

DEFINE_STATUS_REGISTER_BITS(mxcsr,
	/* Exceptions */
	STATUS_REGISTER_FLAG("EF=IE", 0), /* Invalid operation */
	STATUS_REGISTER_FLAG("EF=DE", 1), /* Denormalized operand */
	STATUS_REGISTER_FLAG("EF=ZE", 2), /* Zero-divide */
	STATUS_REGISTER_FLAG("EF=OE", 3), /* Overflow */
	STATUS_REGISTER_FLAG("EF=UE", 4), /* Underflow */
	STATUS_REGISTER_FLAG("EF=PE", 5), /* Precision */

	STATUS_REGISTER_FLAG("DAZ", 6), /* Denormals are zero */

	/* Exception enables */
	STATUS_REGISTER_FLAG("EM=IM", 7), /* Invalid operation */
	STATUS_REGISTER_FLAG("EM=DM", 8), /* Denormalized operand */
	STATUS_REGISTER_FLAG("EM=ZM", 9), /* Zero-divide */
	STATUS_REGISTER_FLAG("EM=OM", 10), /* Overflow */
	STATUS_REGISTER_FLAG("EM=UM", 11), /* Underflow */
	STATUS_REGISTER_FLAG("EM=PM", 12), /* Precision */

	/*
	* Rounding mode: to nearest, toward negative infinity, toward positive
	* infinity, or toward zero
	*/
	STATUS_REGISTER_BIT_VALUES("RC", 13, 0x3, "RN", "R-", "R+", "RZ"),

	STATUS_REGISTER_FLAG("FZ", 15), /* Flush to zero */
);

DEFINE_ARCH_REGISTERS(vector_status,
	STATUS_REGISTER_DESCRIPTOR(mxcsr, ASMASE_REGISTER_U32, fpregs.mxcsr),
);
