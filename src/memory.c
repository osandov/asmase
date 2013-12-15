#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/ptrace.h>

#include "memory.h"

static void dump_decimal(unsigned char *buf, size_t size);
static void dump_unsigned_decimal(unsigned char *buf, size_t size);
static void dump_octal(unsigned char *buf, size_t size);
static void dump_hexadecimal(unsigned char *buf, size_t size);
static void dump_float(unsigned char *buf, size_t size);
static void dump_character(unsigned char *buf, size_t size);
static void dump_address(unsigned char *buf, size_t size);

int dump_memory(pid_t pid, void *addr, size_t repeat,
                enum mem_format format, size_t size)
{
    long buffer;

    for (size_t offset = 0; offset < (repeat * size); offset += size) {
        unsigned char *p;

        size_t row = (size == SZ_BYTE) ? 0x8 : 0x10;

        /* Split up rows/columns */
        if ((offset % row) == 0) {
            if (offset > 0)
                printf("\n");
            printf("%p: ", addr + offset);
        } else {
            if (size == SZ_BYTE)
                printf("    ");
            else if (size == SZ_HALFWORD)
                printf("  ");
            else if (size == SZ_WORD || size == SZ_GIANT)
                printf("      ");
        }

        if (offset % sizeof(long) == 0) {
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
    fprintf(stderr, "Not implemented\n");
    return 1;
}

static void dump_decimal(unsigned char *buf, size_t size)
{
    signed long long decimal = 0;

    switch (size) {
        case SZ_BYTE:
            decimal = *((int8_t *) buf);
            break;
        case SZ_HALFWORD:
            decimal = *((int16_t *) buf);
            break;
        case SZ_WORD:
            decimal = *((int32_t *) buf);
            break;
        case SZ_GIANT:
            decimal = *((int64_t *) buf);
            break;
    }

    printf("%lld", decimal);
}

static void dump_unsigned_decimal(unsigned char *buf, size_t size)
{
    unsigned long long unsigned_decimal = 0;

    switch (size) {
        case SZ_BYTE:
            unsigned_decimal = *((uint8_t *) buf);
            break;
        case SZ_HALFWORD:
            unsigned_decimal = *((uint16_t *) buf);
            break;
        case SZ_WORD:
            unsigned_decimal = *((uint32_t *) buf);
            break;
        case SZ_GIANT:
            unsigned_decimal = *((uint64_t *) buf);
            break;
    }

    printf("%llu", unsigned_decimal);
}

static void dump_octal(unsigned char *buf, size_t size)
{
    unsigned long long octal = 0;

    switch (size) {
        case SZ_BYTE:
            octal = *((uint8_t *) buf);
            break;
        case SZ_HALFWORD:
            octal = *((uint16_t *) buf);
            break;
        case SZ_WORD:
            octal = *((uint32_t *) buf);
            break;
        case SZ_GIANT:
            octal = *((uint64_t *) buf);
            break;
    }

    printf("0%llo", octal);
}

static void dump_hexadecimal(unsigned char *buf, size_t size)
{
    unsigned long long hexadecimal;

    switch (size) {
        case SZ_BYTE:
            hexadecimal = *((uint8_t *) buf);
            printf("0x%02llx", hexadecimal);
            break;
        case SZ_HALFWORD:
            hexadecimal = *((uint16_t *) buf);
            printf("0x%04llx", hexadecimal);
            break;
        case SZ_WORD:
            hexadecimal = *((uint32_t *) buf);
            printf("0x%08llx", hexadecimal);
            break;
        case SZ_GIANT:
            hexadecimal = *((uint64_t *) buf);
            printf("0x%016llx", hexadecimal);
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

    printf("%f", floating);
}

static void dump_character(unsigned char *buf, size_t size)
{
    assert(size == 1);
    printf("%c", *buf);
}

static void dump_address(unsigned char *buf, size_t size)
{
    void *addr;
    assert(size == sizeof(void*));
    addr = *((void **) buf);
    printf("%p", addr);
}
