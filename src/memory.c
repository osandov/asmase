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

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/ptrace.h>

#include "memory.h"

static void dump_decimal(unsigned char *buf, size_t size);
static void dump_unsigned_decimal(unsigned char *buf, size_t size);
static void dump_octal(unsigned char *buf, size_t size);
static void dump_hexadecimal(unsigned char *buf, size_t size);
static void dump_float(unsigned char *buf, size_t size);
static void dump_character(unsigned char *buf, size_t size);
static void dump_address(unsigned char *buf, size_t size);
static void escape_character(unsigned char c, char output[5]);

int dump_memory(pid_t pid, void *addr, size_t repeat,
                enum mem_format format, size_t size)
{
    long buffer;

    for (size_t offset = 0; offset < (repeat * size); offset += size) {
        unsigned char *p;

        /* Split up rows */
        size_t row;
        if (format == FMT_OCTAL)
            row = (size <= SZ_HALFWORD) ? 0x8 : 0x10;
        else if (format == FMT_FLOAT)
            row = (size == SZ_WORD) ? 0x10 : 0x20;
        else if (format == FMT_CHARACTER)
            row = 0x8;
        else
            row = (size == SZ_BYTE) ? 0x8 : 0x10;

        if ((offset % row) == 0) {
            if (offset > 0)
                printf("\n");
            printf("%p: ", addr + offset);
        }

        if ((offset % sizeof(long)) == 0) {
            errno = 0;
            buffer = ptrace(PTRACE_PEEKDATA, pid, addr + offset, NULL);
            if (errno) {
                printf("\n");
                fprintf(stderr, "Cannot access memory at address %p\n",
                        addr + offset);
                return 1;
            }
        }

        p = ((unsigned char *) &buffer) + (offset % sizeof(long));

        switch (format) {
            case FMT_DECIMAL:
                dump_decimal(p, size);
                break;
            case FMT_UNSIGNED_DECIMAL:
                dump_unsigned_decimal(p, size);
                break;
            case FMT_OCTAL:
                dump_octal(p, size);
                break;
            case FMT_HEXADECIMAL:
                dump_hexadecimal(p, size);
                break;
            case FMT_FLOAT:
                dump_float(p, size);
                break;
            case FMT_CHARACTER:
                dump_character(p, size);
                break;
            case FMT_ADDRESS:
                dump_address(p, size);
                break;
            default:
                assert(0 && "Invalid format");
                break;
        }
    }
    printf("\n");

    return 0;
}

int dump_strings(pid_t pid, void *addr, size_t repeat)
{
    char escaped[5];
    size_t strings_printed = 0;
    size_t offset = 0;
    long buffer;

    while (strings_printed < repeat) {
        unsigned char *p;

        /* Print address when printing a new string */
        if (offset == 0)
            printf("%p: \"", addr);

        if ((offset % sizeof(long)) == 0) {
            errno = 0;
            buffer = ptrace(PTRACE_PEEKDATA, pid, addr + offset, NULL);
            if (errno) {
                printf("\n");
                fprintf(stderr, "Cannot access memory at address %p\n",
                        addr + offset);
                return 1;
            }
        }

        p = ((unsigned char *) &buffer) + (offset % sizeof(long));

        if (*p) {
            escape_character(*p, escaped);
            printf("%s", escaped);
            ++offset;
        } else {
            printf("\"\n");
            ++strings_printed;
            addr += offset + 1;
            offset = 0;
        }
    }

    return 0;
}

static void dump_decimal(unsigned char *buf, size_t size)
{
    signed long long decimal;

    /*
     * Field widths represent the maximum length of the string plus spacing (2
     * spaces for half-words, 4 for everything else)
     */
    switch (size) {
        case SZ_BYTE:
            decimal = *((int8_t *) buf);
            printf("%-8lld", decimal);
            break;
        case SZ_HALFWORD:
            decimal = *((int16_t *) buf);
            printf("%-8lld", decimal);
            break;
        case SZ_WORD:
            decimal = *((int32_t *) buf);
            printf("%-15lld", decimal);
            break;
        case SZ_GIANT:
            decimal = *((int64_t *) buf);
            printf("%-24lld", decimal);
            break;
    }
}

static void dump_unsigned_decimal(unsigned char *buf, size_t size)
{
    unsigned long long unsigned_decimal;

    /*
     * Same field widths as signed even though we don't have to worry about the
     * negative sign
     */
    switch (size) {
        case SZ_BYTE:
            unsigned_decimal = *((uint8_t *) buf);
            printf("%-8lld", unsigned_decimal);
            break;
        case SZ_HALFWORD:
            unsigned_decimal = *((uint16_t *) buf);
            printf("%-8llu", unsigned_decimal);
            break;
        case SZ_WORD:
            unsigned_decimal = *((uint32_t *) buf);
            printf("%-15llu", unsigned_decimal);
            break;
        case SZ_GIANT:
            unsigned_decimal = *((uint64_t *) buf);
            printf("%-24llu", unsigned_decimal);
            break;
    }
}

static void dump_octal(unsigned char *buf, size_t size)
{
    unsigned long long octal;

    switch (size) {
        case SZ_BYTE:
            octal = *((uint8_t *) buf);
            printf("0%03llo", octal);
            printf("    ");
            break;
        case SZ_HALFWORD:
            octal = *((uint16_t *) buf);
            printf("0%06llo", octal);
            printf("    ");
            break;
        case SZ_WORD:
            octal = *((uint32_t *) buf);
            printf("0%011llo", octal);
            printf("    ");
            break;
        case SZ_GIANT:
            octal = *((uint64_t *) buf);
            printf("0%022llo", octal);
            printf("      ");
            break;
    }
}

static void dump_hexadecimal(unsigned char *buf, size_t size)
{
    unsigned long long hexadecimal;

    switch (size) {
        case SZ_BYTE:
            hexadecimal = *((uint8_t *) buf);
            printf("0x%02llx", hexadecimal);
            printf("    ");
            break;
        case SZ_HALFWORD:
            hexadecimal = *((uint16_t *) buf);
            printf("0x%04llx", hexadecimal);
            printf("  ");
            break;
        case SZ_WORD:
            hexadecimal = *((uint32_t *) buf);
            printf("0x%08llx", hexadecimal);
            printf("    ");
            break;
        case SZ_GIANT:
            hexadecimal = *((uint64_t *) buf);
            printf("0x%016llx", hexadecimal);
            printf("      ");
            break;
    }
}

static void dump_float(unsigned char *buf, size_t size)
{
    double floating = 0.0;

    assert(size == sizeof(float) || size == sizeof(double));

    switch (size) {
        case SZ_WORD:
            floating = *((float *) buf);
            break;
        case SZ_GIANT:
            floating = *((double *) buf);
            break;
    }

    printf("%-16g", floating);
}

static void dump_character(unsigned char *buf, size_t size)
{
    char escaped[7];
    size_t n;

    assert(size == 1);

    /* Escape the character and put it in quotes */
    escape_character(*buf, escaped + 1);
    n = strlen(escaped + 1) + 1;
    escaped[0] = escaped[n] = '\'';
    escaped[n + 1] = '\0';

    printf("%-8s", escaped);
}

static void dump_address(unsigned char *buf, size_t size)
{
    void *addr;
    assert(size == sizeof(void*));
    addr = *((void **) buf);
    printf("%-24p", addr);
}

static void escape_character(unsigned char c, char output[5])
{
    if (c == '\0') /* Null */
        sprintf(output, "\\0");
    else if (c == '\a') /* Bell */
        sprintf(output, "\\a");
    else if (c == '\b') /* Backspace */
        sprintf(output, "\\b");
    else if (c == '\t') /* Horizontal tab */
        sprintf(output, "\\t");
    else if (c == '\n') /* New line */
        sprintf(output, "\\n");
    else if (c == '\v') /* Vertical tab */
        sprintf(output, "\\v");
    else if (c == '\f') /* Form feed */
        sprintf(output, "\\f");
    else if (c == '\r') /* Carriage return */
        sprintf(output, "\\r");
    else if (isprint(c)) /* Other printable characters */
        sprintf(output, "%c", c);
    else /* Anything else should be an escaped hex sequence */
        sprintf(output, "\\x%02hhx", c);
}
