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
    uint32_t r0;  // a1
    uint32_t r1;  // a2
    uint32_t r2;  // a3
    uint32_t r3;  // a4
    uint32_t r4;  // v1
    uint32_t r5;  // v2
    uint32_t r6;  // v3
    uint32_t r7;  // v4
    uint32_t r8;  // v5
    uint32_t r9;  // v6, sb
    uint32_t r10; // v7, sl
    uint32_t r11; // v8, fp
    uint32_t r12; // ip
    uint32_t r13; // sp
    uint32_t r14; // lr
    uint32_t r15; // pc

    // Condition codes
    uint32_t cpsr;
};

#endif /* ASMASE_ARCH_ARM_USER_REGISTERS_H */
