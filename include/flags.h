#ifndef ASMASE_FLAGS_H
#define ASMASE_FLAGS_H

#include <sys/types.h>

/** Flag in a processor register. */
struct processor_flag {
    /** The name of the flag. */
    const char *name;

    /** Mask in the register. */
    unsigned long long mask;

    /** If the value after applying the mask is this, print it. */
    unsigned long long value_if_true;

    /**
     * Instead of only printing the masked register if it matches
     * value_if_true, just print the masked value if it isn't equal to
     * value_if_true.
     */
    int variable_value;
};

#define BIT_FLAG(name, mask) {name, mask, 0x1, 0}

#define NUM_FLAGS(x) (sizeof(x) / sizeof(struct processor_flag))

/**
 * Pretty-print a status register containing a given set of processor flags.
 */
void print_processor_flags(unsigned long long reg,
                           struct processor_flag *flags, size_t num_flags);

#endif /* ASMASE_FLAGS_H */
