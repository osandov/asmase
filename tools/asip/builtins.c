#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "builtins.h"

#define BUILTIN_FUNC(func) static int builtin_##func(\
        struct assembler *asmb, struct tracee_info *tracee,\
        int argc, char *argv[])

typedef int (*builtin_func)(struct assembler *, struct tracee_info *, int, char **);

struct builtin {
    const char *command;
    builtin_func func;
};

BUILTIN_FUNC(quit)
{
    return 1;
}

static int category_matches(const char *str, const char **categories)
{
    const char **category;
    for (category = categories; *category; ++category) {
        if (strcmp(str, *category) == 0)
            return 1;
    }
    return 0;
}

BUILTIN_FUNC(registers)
{
    static const char *general_purpose[] =
        {"general-purpose", "general", "gp", "g", NULL};
    static const char *condition_code[] =
        {"condition-codes", "condition", "status", "flags", "cc", NULL};
    static const char *floating_point[] =
        {"floating-point", "floating", "fp", "f", NULL};
    static const char *extra[] = {"extra", "xr", "x", NULL};
    static const char *segment[] = {"segment", "ss", NULL};

    if (argc > 2) {
        fprintf(stderr, "Usage: %s [CATEGORY]\n", argv[0]);
        return -1;
    }

    if (argc == 1) {
        if (print_registers(tracee->pid))
            return -1;
    } else if (category_matches(argv[1], general_purpose)) {
        if (print_general_purpose_registers(tracee->pid))
            return -1;
    } else if (category_matches(argv[1], condition_code)) {
        if (print_condition_code_registers(tracee->pid))
            return -1;
    } else if (category_matches(argv[1], floating_point)) {
        if (print_floating_point_registers(tracee->pid))
            return -1;
    } else if (category_matches(argv[1], extra)) {
        if (print_extra_registers(tracee->pid))
            return -1;
    } else if (category_matches(argv[1], segment)) {
        if (print_segment_registers(tracee->pid))
            return -1;
    } else {
        fprintf(stderr, "Unknown register category `%s'\n", argv[1]);
        return -1;
    }

    return 0;
}

static struct builtin builtins[] = {
    {"reg", builtin_registers},
    {"registers", builtin_registers},
    {"q", builtin_quit},
    {"quit", builtin_quit}
};

int is_builtin(const char *str)
{
    return str[0] == ':';
}

int run_builtin(struct assembler *asmb, struct tracee_info *tracee, char *str)
{
    static char **argv = NULL;
    static int argv_size = 0;
    int argc = 0;
    char *token, *saveptr;

    /* Skip the initial ":" */
    ++str;

    while ((token = strtok_r(str, " ", &saveptr))) {
        if (argc >= argv_size) {
            argv_size = (argv_size) ? argv_size * 2 : 4;
            argv = realloc(argv, argv_size * sizeof(*argv));
            if (!argv)
                return 1;
        }
        argv[argc++] = token;
        str = NULL;
    }

    if (argc == 0)
        return -1;

    for (size_t i = 0; i < sizeof(builtins) / sizeof(*builtins); ++i) {
        if (strcmp(argv[0], builtins[i].command) == 0)
            return builtins[i].func(asmb, tracee, argc, argv);
    }

    fprintf(stderr, "Unknown command `%s'\n", argv[0]);

    return -1;
}
