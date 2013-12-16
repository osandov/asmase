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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "builtins.h"
#include "memory.h"
#include "input.h"

#define BUILTIN_FUNC(func) static int builtin_##func(\
        struct assembler *asmb, struct tracee_info *tracee,\
        int argc, char *argv[])

#define NUM_BUILTINS (sizeof(builtins) / sizeof(*builtins))

typedef int (*builtin_func)(struct assembler *, struct tracee_info *, int, char **);

struct builtin {
    const char *command;
    builtin_func func;
    const char *help_string;
};

BUILTIN_FUNC(memory);
BUILTIN_FUNC(registers);
BUILTIN_FUNC(source);
BUILTIN_FUNC(help);
BUILTIN_FUNC(quit);
BUILTIN_FUNC(warranty);
BUILTIN_FUNC(copying);

void print_warranty();
void print_copying();

static struct builtin builtins[] = {
    {"memory",    builtin_memory, "dump memory contents"},
    {"mem",       builtin_memory, NULL},

    {"registers", builtin_registers, "dump register contents"},
    {"reg",       builtin_registers, NULL},

    {"source",    builtin_source, "redirect input to a given file"},

    {"help",      builtin_help, "print this help information"},
    {"h",         builtin_help, NULL},

    {"quit",      builtin_quit, "quit the program"},
    {"q",         builtin_quit, NULL},

    {"warranty",  builtin_warranty, "show warranty information"},
    {"copying",   builtin_copying, "show copying information"}
};

BUILTIN_FUNC(memory)
{
    static size_t repeat = 1;
    static enum mem_format format = FMT_HEXADECIMAL;
    static size_t size = sizeof(long);

    static const char *usage = "Usage: %s ADDR [REPEAT] [FORMAT] [SIZE]\n";

    void *addr;

    char *endptr;

    if (argc < 2 || argc > 5)
        goto err_usage;

    if (strcmp(argv[1], "help") == 0) {
        printf(usage, argv[0]);
        printf("Formats:\n"
               "  d    decimal\n"
               "  u    unsigned decimal\n"
               "  o    unsigned octal\n"
               "  x    unsigned hexadecimal\n"
               "  f    floating point\n"
               "  c    character\n");
        printf("Sizes:\n"
               "  b    byte (1 byte)\n"
               "  h    half word (2 bytes)\n"
               "  w    word (4 bytes)\n"
               "  g    giant (8 bytes)\n");
        return 0;
    }

    addr = (void*) strtoll(argv[1], &endptr, 0);
    if (*endptr != '\0')
        goto err_usage;

    if (argc >= 3) {
        size_t num_units_temp;
        num_units_temp = strtoll(argv[2], &endptr, 0);
        if (*endptr != '\0')
            goto err_usage;
        else
            repeat = num_units_temp;
    }

    if (argc >= 4) {
        const char *format_str = argv[3];
        if (strcmp(format_str, "d") == 0)
            format = FMT_DECIMAL;
        else if (strcmp(format_str, "u") == 0)
            format = FMT_UNSIGNED_DECIMAL;
        else if (strcmp(format_str, "o") == 0)
            format = FMT_OCTAL;
        else if (strcmp(format_str, "x") == 0)
            format = FMT_HEXADECIMAL;
        else if (strcmp(format_str, "f") == 0)
            format = FMT_FLOAT;
        else if (strcmp(format_str, "c") == 0) {
            format = FMT_CHARACTER;
            size = 1;
        } else if (strcmp(format_str, "a") == 0) {
            format = FMT_ADDRESS;
            size = sizeof(void*);
        } else if (strcmp(format_str, "s") == 0)
            format = FMT_STRING;
        else {
            fprintf(stderr, "Invalid format\n");
            return -1;
        }
    }

    if (argc >= 5) {
        const char *size_str = argv[4];
        if (format == FMT_STRING)
            goto err_usage;
        else if (strcmp(size_str, "b") == 0)
            size = SZ_BYTE;
        else if (strcmp(size_str, "h") == 0)
            size = SZ_HALFWORD;
        else if (strcmp(size_str, "w") == 0)
            size = SZ_WORD;
        else if (strcmp(size_str, "g") == 0)
            size = SZ_GIANT;
        else {
            fprintf(stderr, "Invalid size\n");
            return -1;
        }
    }

    if (format == FMT_FLOAT && size != SZ_WORD && size != SZ_GIANT) {
        fprintf(stderr, "Invalid size for float\n");
        return -1;
    }

    if (format == FMT_CHARACTER && size != SZ_BYTE) {
        fprintf(stderr, "Invalid size for character\n");
        return -1;
    }

    if (format == FMT_ADDRESS && size != sizeof(void*)) {
        fprintf(stderr, "Invalid size for address\n");
        return -1;
    }

    if (format == FMT_STRING) {
        if (dump_strings(tracee->pid, addr, repeat))
            return -1;
    } else {
        if (dump_memory(tracee->pid, addr, repeat, format, size))
            return -1;
    }

    return 0;

err_usage:
    fprintf(stderr, usage, argv[0]);
    return -1;
}

static int in_string_array(const char *str, const char **arr)
{
    const char **entry;
    for (entry = arr; *entry; ++entry) {
        if (strcmp(str, *entry) == 0)
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
    static const char *segment[] = {"segment", "seg", NULL};

    const char *usage = "Usage: %s [CATEGORY]\n";

    if (argc > 2) {
        fprintf(stderr, usage, argv[0]);
        return -1;
    }

    if (argc == 1) {
        if (print_registers(tracee->pid))
            return -1;
    } else if (strcmp(argv[1], "help") == 0) {
        printf(usage, argv[0]);
        printf("Categories:\n"
               "  gp    General purpose registers\n"
               "  cc    Condition code/status flag registers\n"
               "  fp    Floating point registers\n"
               "  x     Extra registers\n"
               "  seg   Segment registers\n");
    } else if (in_string_array(argv[1], general_purpose)) {
        if (print_general_purpose_registers(tracee->pid))
            return -1;
    } else if (in_string_array(argv[1], condition_code)) {
        if (print_condition_code_registers(tracee->pid))
            return -1;
    } else if (in_string_array(argv[1], floating_point)) {
        if (print_floating_point_registers(tracee->pid))
            return -1;
    } else if (in_string_array(argv[1], extra)) {
        if (print_extra_registers(tracee->pid))
            return -1;
    } else if (in_string_array(argv[1], segment)) {
        if (print_segment_registers(tracee->pid))
            return -1;
    } else {
        fprintf(stderr, "Unknown register category `%s'\n", argv[1]);
        return -1;
    }

    return 0;
}

BUILTIN_FUNC(source)
{
    static const char *usage = "Usage: %s FILE\n";

    if (argc != 2) {
        fprintf(stderr, usage, argv[0]);
        return -1;
    }

    if (redirect_input(argv[1]))
        return -1;

    return 0;
}

BUILTIN_FUNC(help)
{
    for (size_t i = 0; i < NUM_BUILTINS; ++i) {
        printf("%-13s", builtins[i].command);
        if (builtins[i].help_string)
            printf("%s\n", builtins[i].help_string);
        else
            printf("(same as above)\n");
    }

    return 0;
}

BUILTIN_FUNC(quit)
{
    return 1;
}

BUILTIN_FUNC(warranty)
{
    print_warranty();
    return 0;
}

BUILTIN_FUNC(copying)
{
    print_copying();
    return 0;
}

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

    for (size_t i = 0; i < NUM_BUILTINS; ++i) {
        if (strcmp(argv[0], builtins[i].command) == 0)
            return builtins[i].func(asmb, tracee, argc, argv);
    }

    fprintf(stderr, "Unknown command `%s'\n", argv[0]);

    return -1;
}
