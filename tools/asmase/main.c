#include <stdio.h>
#include <stdlib.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "assembler.h"
#include "builtins.h"
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

    using_history();

    mc_buffer_size = INITIAL_MC_SIZE;
    if (!(mc_buffer = malloc(mc_buffer_size))) {
        perror("malloc");
        all_error = 1;
        goto out;
    }

    while ((asm_buffer = readline(PS1))) {
        add_history(asm_buffer);

        if (is_builtin(asm_buffer)) {
            int result = run_builtin(asmb, tracee, asm_buffer);
            if (result < 0)
                continue;
            else if (result > 0)
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
    }

    printf("\n");

out:
    free(mc_buffer);
    clear_history();
    return all_error;
}

int main(int argc, char *argv[])
{
    struct assembler *asmb = NULL;
    int all_error = 0, error;

    struct tracee_info tracee;

    /** Only the tracer/parent returns from this. */
    if ((error = create_tracee(&tracee)))
        return error; /* Too early to goto out. */

    if ((error = init_assemblers(argc, argv))) {
        all_error = error;
        goto out;
    }

    if (!(asmb = create_assembler())) {
        all_error = 1;
        goto out;
    }

    if ((error = repl(asmb, &tracee)))
        all_error = error;

out:
    if (asmb)
        destroy_assembler(asmb);
    shutdown_assemblers();
    cleanup_tracing(&tracee);
    return all_error;
}
