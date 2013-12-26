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

#include <stdio.h>
#include <stdlib.h>

#include "assembler.h"
#include "builtins.h"
#include "input.h"
#include "tracing.h"

#define INITIAL_MC_SIZE 32
#define PS1 "asmase> "

static void print_instruction(const char *asm_buffer, unsigned char *mc_buffer,
                              size_t mc_length)
{
    printf("%s = [", asm_buffer);
    for (size_t i = 0; i < mc_length; ++i) {
        if (i > 0)
            printf(", ");
        printf("0x%02x", mc_buffer[i]);
    }
    printf("]\n");
}

int repl(struct assembler *asmb, struct tracee_info *tracee)
{
    int all_error = 0;

    char *asm_buffer = NULL;

    unsigned char *mc_buffer = NULL;
    size_t mc_buffer_size = 0;
    ssize_t mc_length;

    mc_buffer_size = INITIAL_MC_SIZE;
    if (!(mc_buffer = malloc(mc_buffer_size))) {
        perror("malloc");
        all_error = 1;
        goto out;
    }

    while ((asm_buffer = read_input_line(PS1))) {
        if (is_builtin(asm_buffer)) {
            int result = run_builtin(asmb, tracee, asm_buffer);
            if (result > 0)
                continue;
            else if (result < 0)
                goto out;
        } else {
            mc_length = assemble_instruction(asmb, asm_buffer,
                                             &mc_buffer, &mc_buffer_size);
            if (mc_length > 0) {
                print_instruction(asm_buffer, mc_buffer, mc_length);
                all_error = execute_instruction(tracee, mc_buffer, mc_length);
                if (all_error)
                    goto out;
            }
        }

        free(asm_buffer);
    }

    printf("\n");

out:
    free(mc_buffer);
    return all_error;
}

int main(int argc, char *argv[])
{
    struct assembler *asmb = NULL;
    int all_error = 0, error;

    struct tracee_info tracee;

    printf("asmase Copyright (C) 2013 Omar Sandoval\n"
           "This program comes with ABSOLUTELY NO WARRANTY; for details type `:warranty'.\n"
           "This is free software, and you are welcome to redistribute it\n"
           "under certain conditions; type `:copying' for details.\n");

    /* Only the tracer/parent returns from this. */
    if ((error = create_tracee(&tracee)))
        return error; /* Too early to goto out. */

    if ((error = init_assemblers())) {
        all_error = error;
        goto out;
    }

    if (!(asmb = create_assembler())) {
        all_error = 1;
        goto out;
    }

    init_input();

    if ((error = repl(asmb, &tracee)))
        all_error = error;

out:
    if (asmb)
        destroy_assembler(asmb);
    shutdown_input();
    shutdown_assemblers();
    cleanup_tracing(&tracee);
    return all_error;
}
