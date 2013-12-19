/*
 * Copyright (C) 2013 Omar Sandoval
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ASMASE_MEMORY_H
#define ASMASE_MEMORY_H

enum unit_size {
    SZ_BYTE     = 1,
    SZ_HALFWORD = 2,
    SZ_WORD     = 4,
    SZ_GIANT    = 8
};

enum mem_format {
    FMT_DECIMAL,
    FMT_UNSIGNED_DECIMAL,
    FMT_OCTAL,
    FMT_HEXADECIMAL,
    FMT_BINARY,
    FMT_FLOAT,
    FMT_CHARACTER,
    FMT_ADDRESS,
    FMT_STRING
};

#ifdef __cplusplus
extern "C" {
#endif

int dump_memory(pid_t pid, void *addr, size_t repeat,
                enum mem_format format, size_t size);

int dump_strings(pid_t pid, void *addr, size_t repeat);

#ifdef __cplusplus
}
#endif

#endif /* ASMASE_MEMORY_H */
