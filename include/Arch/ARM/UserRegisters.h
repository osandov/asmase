/*
 * ARM registers.
 *
 * Copyright (C) 2013-2014 Omar Sandoval
 *
 * This file is part of asmase.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ASMASE_ARCH_ARM_USER_REGISTERS_H
#define ASMASE_ARCH_ARM_USER_REGISTERS_H

#include <cinttypes>

class UserRegisters {
public:
    // General-purpose registers
    uint32_t r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10;
    uint32_t fp, ip, sp, lr, pc;

    // Condition codes
    uint32_t cpsr;
};

#define r11 fp
#define r12 ip
#define r13 sp
#define r14 lr
#define r15 pc

#endif /* ASMASE_ARCH_ARM_USER_REGISTERS_H */
