/*
 * libasmase assembler interface.
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

#ifndef LIBASMASE_ASSEMBLER_H
#define LIBASMASE_ASSEMBLER_H

#ifdef __cplusplus
#include <cstddef>
#else
#include <stddef.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct assembler_context;

/**
 * libasmase_assembler_init(void) - Initialize the libasmase assembler.
 *
 * This must be called before using the assembler interface.
 */
void libasmase_assembler_init(void);

/**
 * create_assembler_context() - Create a new assembler.
 *
 * Return: New assembler.
 */
struct assembler_context *create_assembler_context(void);

/**
 * destroy_assembler_context() - Free all resources associated with an
 * assembler.
 *
 * @ctx: Assembler to destroy.
 */
void destroy_assembler_context(struct assembler_context *ctx);

/**
 * assemble_code() - Assemble some assembly code to machine code.
 *
 * @ctx: Assembler.
 * @filename: Filename for diagnostics.
 * @line: Line number for diagnostics.
 * @asm_code: Assembly code.
 * @out: Allocated output buffer; must be freed with free().
 * @len: Length of the output buffer.
 *
 * Return: 0 if the code was successfully assembled, in which case @out will
 * contain the assembled machine code; 1 if there was an error with the assembly
 * code, in which case @out will contain a diagnostic message; or -1 if there
 * was an internal error, in which case @out will be NULL.
 */
int assemble_code(const struct assembler_context *ctx, const char *filename,
		  int line, const char *asm_code, char **out, size_t *len);

#ifdef __cplusplus
}
#endif

#endif /* LIBASMASE_ASSEMBLER_H */
