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

#ifndef ASMASE_ASSEMBLER_H
#define ASMASE_ASSEMBLER_H

#include <sys/types.h>

/** An opaque handle for an assembler. */
struct assembler;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the assembler system, including parsing command line parameters.
 */
int init_assemblers(int argc, char **argv);

/** Shutdown and clean up for the assembler system. */
void shutdown_assemblers();

/** Create a new assembler. */
struct assembler *create_assembler();

/** Destroy an assembler. */
void destroy_assembler(struct assembler *ctx);

/**
 * Assemble an instruction to machine code.
 * @param in The assembly instruction.
 * @param in_size The length of the assembly instruction in characters.
 * @param out A pointer to a buffer in which to return the machine code. This
 * buffer may be realloc'd if it is not big enough to hold the machine code.
 * @param out_size A pointer to the size of the out buffer. This may be updated
 * to reflect a resizing of the buffer.
 * @return The length in bytes of the machine code generated.
 */
ssize_t assemble_instruction(struct assembler *ctx, const char *in,
                             unsigned char **out, size_t *out_size);

#ifdef __cplusplus
}
#endif

#endif /* ASMASE_ASSEMBLER_H */
