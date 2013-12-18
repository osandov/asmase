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

#ifndef ASMASE_INPUT_H
#define ASMASE_INPUT_H

#include <limits.h>

/** File we are reading from and our current position in that file. */
struct source_file {
    char filename[NAME_MAX + 1];
    int line;
    FILE *file;
};

#ifdef __cplusplus
extern "C" {
#endif

/** Initialize the input system. */
void init_input();

/** Shutdown and clean up for the input system. */
void shutdown_input();

/** Return information for the file we're currently reading from. */
struct source_file *get_current_file();

/**
 * Read a line of (potentially redirected input). The returned buffer must be
 * freed.
 */
char *read_input_line(const char *prompt);

/**
 * Redirect input to the file at the given path until EOF.
 * @return Zero on success, nonzero on failure.
 */
int redirect_input(const char *path);

#ifdef __cplusplus
}
#endif

#endif /* ASMASE_INPUT_H */
