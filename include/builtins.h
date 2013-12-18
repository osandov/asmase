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

#ifndef ASMASE_BUILTINS_H
#define ASMASE_BUILTINS_H

#include "assembler.h"
#include "tracing.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Return whether the given string is a command line builtin. */
int is_builtin(const char *str);

/**
 * Run a command line builtin.
 * @return -1 on error, 0 on success, 1 on exit.
 */
int run_builtin(struct assembler *asmb, struct tracee_info *tracee, char *str);

#ifdef __cplusplus
}
#endif

#endif /* ASMASE_BUILTINS_H */
