/*
 * Internal definitions which are shared with assembly code.
 *
 * Copyright (C) 2017 Omar Sandoval
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

#ifndef LIBASMASE_ASM_H
#define LIBASMASE_ASM_H

#ifdef __x86_64__
#define MEMFD_ADDR 0x7fff00000000UL
#else
#error
#endif

/* 64k per tracee by default; 4k for code, the rest for the stack. */
#define MEMFD_SIZE 65536
#define CODE_MAX_SIZE 4096

#endif /* LIBASMASE_ASM_H */
