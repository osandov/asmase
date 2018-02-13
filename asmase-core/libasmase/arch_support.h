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

#endif /* LIBASMASE_ARCH_SUPPORT_H */
