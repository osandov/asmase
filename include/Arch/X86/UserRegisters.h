#ifndef ASMASE_ARCH_X86_USER_REGISTERS_H
#define ASMASE_ARCH_X86_USER_REGISTERS_H

#include <cinttypes>

struct xmm_t {
    uint64_t lo, hi; // Little-endian, low goes first
};

class UserRegisters {
public:
#ifdef __x86_64__
    static const int NUM_SSE_REGS = 16;
#else
    static const int NUM_SSE_REGS = 8;
#endif

    // General-purpose registers
#ifdef __x86_64__
    uint64_t rax, rcx, rdx, rbx;
    uint64_t rsp, rbp;
    uint64_t rsi, rdi;
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
#else
    uint32_t eax, ecx, edx, ebx;
    uint32_t esp, ebp;
    uint32_t esi, edi;
#endif

    // Condition codes
    uint32_t eflags;

    // Program counter
#ifdef __x86_64__
    uint64_t rip;
#else
    uint32_t eip;
#endif

    // Segment registers
    uint16_t cs, ss, ds, es, fs, gs;
#ifdef __x86_64__
    uint64_t fsBase, gsBase;
#endif


    // Floating-point
    long double st[8];
    uint16_t fcw, fsw, ftw, fop;
#ifdef __x86_64__
    uint64_t fip, fdp;
#else
    uint32_t fip;
    uint16_t fcs;
    uint32_t fdp;
    uint16_t fds;
#endif

    // SSE
    xmm_t xmm[NUM_SSE_REGS];
    uint32_t mxcsr;
};

/** Top physical register in x87 register stack. */
inline uint16_t x87_st_top(uint16_t fsw)
{
    return (fsw & 0x3800) >> 11;
}

/**
 * Convert a physical x87 register number to logical (i.e., %st(x)) register
 * number.
 */
inline uint16_t x87_phys_to_log(uint16_t index, uint16_t top)
{
    return (index - top + 8) % 8;
}

/** Convert a logical x87 register number to the physical register number. */
inline uint16_t x87_log_to_phys(uint16_t index, uint16_t top)
{
    return (index + top) % 8;
}

/** Determine the proper tag for a given x87 register value. */
inline uint16_t x87_tag(long double st)
{
    struct x87_float {
        uint64_t fraction : 63;
        uint64_t integer : 1;
        uint64_t exponent : 15;
        uint64_t sign : 1;
    } *fp = reinterpret_cast<struct x87_float *>(&st);

    if (fp->exponent == 0x7fff)
        return 2; // Special
    else if (fp->exponent == 0x0000) {
        if (fp->fraction == 0 && !fp->integer)
            return 1; // Zero
        else
            return 2; // Special
    } else {
        if (fp->integer)
            return 0; // Valid
        else
            return 2; // Special
    }
}

#endif /* ASMASE_ARCH_X86_USER_REGISTERS_H */
