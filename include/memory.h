#ifndef ASMASE_MEMORY_H
#define ASMASE_MEMORY_H

enum unit_size {
    SZ_BYTE     = 1,
    SZ_HALFWORD = 2,
    SZ_WORD     = 4,
    SZ_GIANT    = 8
};

enum mem_format {
    FMT_DECIMAL,
    FMT_UNSIGNED_DECIMAL,
    FMT_OCTAL,
    FMT_HEXADECIMAL,
    FMT_FLOAT,
    FMT_CHARACTER,
    FMT_ADDRESS,
    FMT_STRING
};

int dump_memory(pid_t pid, void *addr, size_t repeat,
                enum mem_format format, size_t size);

int dump_strings(pid_t pid, void *addr, size_t repeat);

#endif /* ASMASE_MEMORY_H */
