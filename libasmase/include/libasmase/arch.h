/*
 * Architecture-dependent libasmase features.
 *
 * Copyright (C) 2016 Omar Sandoval
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

#ifndef LIBASMASE_ARCH_H
#define LIBASMASE_ARCH_H

#ifdef __x86_64__
#define ASMASE_HAVE_INT128 1
#define ASMASE_HAVE_FLOAT80 1
#endif

#ifdef __i386__
#define ASMASE_HAVE_FLOAT80 1
#endif

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define ASMASE_LITTLE_ENDIAN 1
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define ASMASE_BIG_ENDIAN 1
#else /* __ORDER_PDP_ENDIAN__? */
#error
#endif

#endif /* LIBASMASE_ARCH_H */
