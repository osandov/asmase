#ifndef ASMASE_ARCH_X86_TRACEE_H
#define ASMASE_ARCH_X86_TRACEE_H

#include "Tracee.h"

class X86Tracee : public Tracee {
    int setProgramCounter(void *pc);

public:
    X86Tracee(pid_t pid, void *sharedMemory, size_t sharedSize);
};

struct xmm_t {
    uint64_t lo, hi;
};

struct UserRegisters {
    // General-purpose registers
    uint64_t rax, rcx, rdx, rbx;
    uint64_t rsp, rbp;
    uint64_t rsi, rdi;
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15;

    // Condition codes
    uint32_t eflags;

    // Program counter
    uint64_t rip;

    // Segment registers
    uint16_t cs, ss, ds, es, fs, gs;
    uint64_t fs_base, gs_base;


    // Floating-point
    long double st[8];
    uint16_t fcw, fsw, ftw, fop;
    uint64_t fip, fdp;

    // SSE
    xmm_t xmm[16];
    uint32_t mxcsr;
};

#endif /* ASMASE_ARCH_X86_TRACEE_H */
