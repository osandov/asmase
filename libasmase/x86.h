/*
 * x86 architecture support.
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

#ifndef LIBASMASE_X86_H
#define LIBASMASE_X86_H

#include "arch_support.h"

DEFINE_ARCH_PTRACE_REGSETS2(
	struct user_regs_struct, NT_PRSTATUS,
#ifdef __x86_64__
	struct user_fpregs_struct, NT_FPREGSET
#else
	struct user_fpxregs_struct, NT_PRXFPREG
#endif
);

#endif /* LIBASMASE_X86_H */
