#include <cinttypes>
#include <cstdio>
#include <cstddef>
#include <iostream>

#include <asm/processor-flags.h>

#include "ProcessorFlags.h"
#include "RegisterInfo.h"
#include "Arch/X86/UserRegisters.h"
#include "Arch/X86/X86Tracee.h"

#define USER_REGISTER(reg) offsetof(UserRegisters, reg)
using RT = RegisterType;
using RC = RegisterCategory;
extern const RegisterInfo X86Registers = {
    .registers = {
        // General-purpose
        {RT::INT64, RC::GENERAL_PURPOSE, "%", "rax", USER_REGISTER(rax)},
        {RT::INT64, RC::GENERAL_PURPOSE, "%", "rcx", USER_REGISTER(rcx)},
        {RT::INT64, RC::GENERAL_PURPOSE, "%", "rdx", USER_REGISTER(rdx)},
        {RT::INT64, RC::GENERAL_PURPOSE, "%", "rbx", USER_REGISTER(rbx)},
        {RT::INT64, RC::GENERAL_PURPOSE, "%", "rsp", USER_REGISTER(rsp)},
        {RT::INT64, RC::GENERAL_PURPOSE, "%", "rbp", USER_REGISTER(rbp)},
        {RT::INT64, RC::GENERAL_PURPOSE, "%", "rsi", USER_REGISTER(rsi)},
        {RT::INT64, RC::GENERAL_PURPOSE, "%", "rdi", USER_REGISTER(rdi)},
        {RT::INT64, RC::GENERAL_PURPOSE, "%", "r8",  USER_REGISTER(r8)},
        {RT::INT64, RC::GENERAL_PURPOSE, "%", "r9",  USER_REGISTER(r9)},
        {RT::INT64, RC::GENERAL_PURPOSE, "%", "r10", USER_REGISTER(r10)},
        {RT::INT64, RC::GENERAL_PURPOSE, "%", "r11", USER_REGISTER(r11)},
        {RT::INT64, RC::GENERAL_PURPOSE, "%", "r12", USER_REGISTER(r12)},
        {RT::INT64, RC::GENERAL_PURPOSE, "%", "r13", USER_REGISTER(r13)},
        {RT::INT64, RC::GENERAL_PURPOSE, "%", "r14", USER_REGISTER(r14)},
        {RT::INT64, RC::GENERAL_PURPOSE, "%", "r15", USER_REGISTER(r15)},

        // Condition codes
        {RT::INT32, RC::CONDITION_CODE, "eflags", USER_REGISTER(eflags)},

        // Program counter
        {RT::INT64, RC::PROGRAM_COUNTER, "%", "rip", USER_REGISTER(rip)},

        // Segmentation
        {RT::INT16, RC::SEGMENTATION, "%", "cs", USER_REGISTER(cs)},
        {RT::INT16, RC::SEGMENTATION, "%", "ss", USER_REGISTER(ss)},
        {RT::INT16, RC::SEGMENTATION, "%", "ds", USER_REGISTER(ds)},
        {RT::INT16, RC::SEGMENTATION, "%", "es", USER_REGISTER(es)},
        {RT::INT16, RC::SEGMENTATION, "%", "fs", USER_REGISTER(fs)},
        {RT::INT16, RC::SEGMENTATION, "%", "gs", USER_REGISTER(gs)},
        {RT::INT64, RC::SEGMENTATION, "%", "fs.base", USER_REGISTER(fsBase)},
        {RT::INT64, RC::SEGMENTATION, "%", "gs.base", USER_REGISTER(gsBase)},

        // Floating-point
        {RT::LONG_DOUBLE, RC::FLOATING_POINT, "%", "st(0)", USER_REGISTER(st[0])},
        {RT::LONG_DOUBLE, RC::FLOATING_POINT, "%", "st(1)", USER_REGISTER(st[1])},
        {RT::LONG_DOUBLE, RC::FLOATING_POINT, "%", "st(2)", USER_REGISTER(st[2])},
        {RT::LONG_DOUBLE, RC::FLOATING_POINT, "%", "st(3)", USER_REGISTER(st[3])},
        {RT::LONG_DOUBLE, RC::FLOATING_POINT, "%", "st(4)", USER_REGISTER(st[4])},
        {RT::LONG_DOUBLE, RC::FLOATING_POINT, "%", "st(5)", USER_REGISTER(st[5])},
        {RT::LONG_DOUBLE, RC::FLOATING_POINT, "%", "st(6)", USER_REGISTER(st[6])},
        {RT::LONG_DOUBLE, RC::FLOATING_POINT, "%", "st(7)", USER_REGISTER(st[7])},

        // Floating-point status
        {RT::INT16, RC::FLOATING_POINT, "fcw", USER_REGISTER(fcw)},
        {RT::INT16, RC::FLOATING_POINT, "fsw", USER_REGISTER(fsw)},
        {RT::INT16, RC::FLOATING_POINT, "ftw", USER_REGISTER(ftw)},
        {RT::INT16, RC::FLOATING_POINT, "fop", USER_REGISTER(fop)},
        {RT::INT64, RC::FLOATING_POINT, "fip", USER_REGISTER(fip)},
        {RT::INT64, RC::FLOATING_POINT, "fdp", USER_REGISTER(fdp)},

        // Extra (MMX and SSE)
        {RT::INT64, RC::EXTRA, "%", "mm0", USER_REGISTER(st[0])},
        {RT::INT64, RC::EXTRA, "%", "mm1", USER_REGISTER(st[1])},
        {RT::INT64, RC::EXTRA, "%", "mm2", USER_REGISTER(st[2])},
        {RT::INT64, RC::EXTRA, "%", "mm3", USER_REGISTER(st[3])},
        {RT::INT64, RC::EXTRA, "%", "mm4", USER_REGISTER(st[4])},
        {RT::INT64, RC::EXTRA, "%", "mm5", USER_REGISTER(st[5])},
        {RT::INT64, RC::EXTRA, "%", "mm6", USER_REGISTER(st[6])},
        {RT::INT64, RC::EXTRA, "%", "mm7", USER_REGISTER(st[7])},

        {RT::INT128, RC::EXTRA, "%", "xmm0",  USER_REGISTER(xmm[0])},
        {RT::INT128, RC::EXTRA, "%", "xmm1",  USER_REGISTER(xmm[1])},
        {RT::INT128, RC::EXTRA, "%", "xmm2",  USER_REGISTER(xmm[2])},
        {RT::INT128, RC::EXTRA, "%", "xmm3",  USER_REGISTER(xmm[3])},
        {RT::INT128, RC::EXTRA, "%", "xmm4",  USER_REGISTER(xmm[4])},
        {RT::INT128, RC::EXTRA, "%", "xmm5",  USER_REGISTER(xmm[5])},
        {RT::INT128, RC::EXTRA, "%", "xmm6",  USER_REGISTER(xmm[6])},
        {RT::INT128, RC::EXTRA, "%", "xmm7",  USER_REGISTER(xmm[7])},
        {RT::INT128, RC::EXTRA, "%", "xmm8",  USER_REGISTER(xmm[8])},
        {RT::INT128, RC::EXTRA, "%", "xmm9",  USER_REGISTER(xmm[9])},
        {RT::INT128, RC::EXTRA, "%", "xmm10", USER_REGISTER(xmm[10])},
        {RT::INT128, RC::EXTRA, "%", "xmm11", USER_REGISTER(xmm[11])},
        {RT::INT128, RC::EXTRA, "%", "xmm12", USER_REGISTER(xmm[12])},
        {RT::INT128, RC::EXTRA, "%", "xmm13", USER_REGISTER(xmm[13])},
        {RT::INT128, RC::EXTRA, "%", "xmm14", USER_REGISTER(xmm[14])},
        {RT::INT128, RC::EXTRA, "%", "xmm15", USER_REGISTER(xmm[15])},

        // Extra status (SSE)
        {RT::INT32, RC::EXTRA, "mxcsr", USER_REGISTER(mxcsr)},
    },
};
#undef USER_REGISTER

/** Flags in the eflags register. */
static ProcessorFlags<decltype(UserRegisters::eflags)> eflagsFlags = {
    {"CF", X86_EFLAGS_CF}, // Carry flag
    {"PF", X86_EFLAGS_PF}, // Parity flag
    {"AF", X86_EFLAGS_AF}, // Adjust flag
    {"ZF", X86_EFLAGS_ZF}, // Zero flag
    {"SF", X86_EFLAGS_SF}, // Sign flag
    {"TF", X86_EFLAGS_TF}, // Trap flag
    {"IF", X86_EFLAGS_IF}, // Interruption flag
    {"DF", X86_EFLAGS_DF}, // Direction flag
    {"OF", X86_EFLAGS_OF}, // Overflow flag
    {"IOPL", X86_EFLAGS_IOPL, 0x0, true}, // I/O privilege level
    {"NT", X86_EFLAGS_NT}, // Nested task flag
    {"RF", X86_EFLAGS_RF}, // Resume flag
    {"VM", X86_EFLAGS_VM}, // Virtual-8086 mode
    {"AC", X86_EFLAGS_AC}, // Alignment check
    {"VIF", X86_EFLAGS_VIF}, // Virtual interrupt flag
    {"VIP", X86_EFLAGS_VIP}, // Virtual interrupt pending flag
    {"ID", X86_EFLAGS_ID}, // Identification flag
};

/** Flags in the fcw register (floating point control word). */
static ProcessorFlags<decltype(UserRegisters::fcw)> fcwFlags = {
    // Rounding mode
    {"RC=RN", 0x6000, 0x0}, // Round to nearest
    {"RC=R-", 0x6000, 0x1}, // Round toward negative infinity
    {"RC=R+", 0x6000, 0x2}, // Round toward positive infinity
    {"RC=RZ", 0x6000, 0x3}, // Round toward zero

    // Rounding precision
    {"PC=SGL", 0x00c0, 0x0}, // Single-precision
    {"PC=DBL", 0x00c0, 0x2}, // Double-precision
    {"PC=EXT", 0x00c0, 0x3}, // Extended-precision

    // Exception enables
    {"EM=PM", 0x0020}, // Precision
    {"EM=UM", 0x0010}, // Underflow
    {"EM=OM", 0x0008}, // Overflow
    {"EM=ZM", 0x0004}, // Zero-divide
    {"EM=DM", 0x0002}, // Denormalized operand
    {"EM=IM", 0x0001}, // Invalid operation
};

/** Flags in the fsw register (floating point status word). */
static ProcessorFlags<decltype(UserRegisters::fsw)> fswFlags = {
    {"TOP", 0x3800, 0x8, true}, // Top of the floating point stack

    {"B", 0x8000}, // FPU busy
    
    // Condition bits
    {"C0", 0x0100},
    {"C1", 0x0200},
    {"C2", 0x0400},
    {"C3", 0x4000},

    {"ES", 0x0080}, // Exception summary status
    {"SF", 0x0040}, // Stack fault

    // Exceptions
    {"EF=PE", 0x0020}, // Precision
    {"EF=UE", 0x0010}, // Underflow
    {"EF=OE", 0x0008}, // Overflow
    {"EF=ZE", 0x0004}, // Zero-divide
    {"EF=DE", 0x0002}, // Denormalized operand
    {"EF=IE", 0x0001}, // Invalid operation
};

/** Flags in the mxcsr register (SSE control/status register). */
static ProcessorFlags<decltype(UserRegisters::mxcsr)> mxcsrFlags = {
    {"FZ", 0x8000}, // Flush to zero

    // Rounding mode
    {"RC=RN", 0x6000, 0x0},
    {"RC=R-", 0x6000, 0x1},
    {"RC=R+", 0x6000, 0x2},
    {"RC=RZ", 0x6000, 0x3},

    // Exception enables
    {"EM=PM", 0x1000}, // Precision
    {"EM=UM", 0x0800}, // Underflow
    {"EM=OM", 0x0400}, // Overflow
    {"EM=ZM", 0x0200}, // Zero-divide
    {"EM=DM", 0x0100}, // Denormalized operand
    {"EM=IM", 0x0080}, // Invalid operation

    {"DAZ", 0x0040}, // Denormals are zero

    // Exceptions
    {"EF=PE", 0x0020}, // Precision
    {"EF=UE", 0x0010}, // Underflow
    {"EF=OE", 0x0008}, // Overflow
    {"EF=ZE", 0x0004}, // Zero-divide
    {"EF=DE", 0x0002}, // Denormalized operand
    {"EF=IE", 0x0001}, // Invalid operation
};

int X86Tracee::printGeneralPurposeRegisters()
{
    printf("%%rax = 0x%016" PRIx64 "    %%rcx = 0x%016" PRIx64 "\n"
           "%%rdx = 0x%016" PRIx64 "    %%rbx = 0x%016" PRIx64 "\n"
           "%%rsp = 0x%016" PRIx64 "    %%rbp = 0x%016" PRIx64 "\n"
           "%%rsi = 0x%016" PRIx64 "    %%rdi = 0x%016" PRIx64 "\n"
           "%%r8  = 0x%016" PRIx64 "    %%r9  = 0x%016" PRIx64 "\n"
           "%%r10 = 0x%016" PRIx64 "    %%r11 = 0x%016" PRIx64 "\n"
           "%%r12 = 0x%016" PRIx64 "    %%r13 = 0x%016" PRIx64 "\n"
           "%%r14 = 0x%016" PRIx64 "    %%r15 = 0x%016" PRIx64 "\n",
           registers->rax, registers->rcx, registers->rdx, registers->rbx,
           registers->rsp, registers->rbp, registers->rsi, registers->rdi,
           registers->r8,  registers->r9,  registers->r10, registers->r11,
           registers->r12, registers->r13, registers->r14, registers->r15);
    return 0;
}

int X86Tracee::printConditionCodeRegisters()
{
    printf("eflags = 0x%08" PRIx32 " = ", registers->eflags);
    eflagsFlags.printFlags(registers->eflags);
    std::cout << '\n';
    return 0;
}

int X86Tracee::printProgramCounterRegisters()
{
    printf("%%rip = 0x%016" PRIx64 "\n", registers->rip);
    return 0;
}

int X86Tracee::printSegmentationRegisters()
{
    printf("%%ss = 0x%04" PRIx16 "    %%cs = 0x%04" PRIx16 "\n"
           "%%ds = 0x%04" PRIx16 "    %%es = 0x%04" PRIx16 "\n"
           "%%fs = 0x%04" PRIx16 "    %%gs = 0x%04" PRIx16 "\n"
           "fs.base = 0x%016" PRIx64 "\n"
           "gs.base = 0x%016" PRIx64 "\n",
           registers->ss, registers->cs, registers->ds, registers->es,
           registers->fs, registers->gs, registers->fsBase, registers->gsBase);
    return 0;
}

int X86Tracee::printFloatingPointRegisters()
{
    printf("fcw = 0x%04" PRIx16 " = ", registers->fcw);
    fcwFlags.printFlags(registers->fcw);
    std::cout << '\n';

    printf("fsw = 0x%04" PRIx16 " = ", registers->fsw);
    fswFlags.printFlags(registers->fsw);
    std::cout << '\n';

    printf("ftw = 0x%04" PRIx16 "\n", registers->ftw);
    std::cout << '\n';

    uint16_t top = x87_st_top(registers->fsw);
    for (int16_t physical = 7; physical >= 0; --physical) {
        uint16_t logical = x87_phys_to_log(physical, top);
        long double st = registers->st[logical];
        uint16_t tag = 
            (registers->ftw & (0x3 << 2 * physical)) >> 2 * physical;

        std::cout << 'R' << physical << " = ";

        switch (tag) {
            case 0x0:
                std::cout << "(valid) ";
                break;
            case 0x1:
                std::cout << "(zero) ";
                break;
            case 0x2:
                std::cout << "(special) ";
                break;
            case 0x3:
                std::cout << "(empty)\n";
                break;
        }

        if (tag != 0x3)
            std::cout << st << " %%st(" << logical << ")\n";
    }
    std::cout << '\n';

    printf("fip = 0x%016" PRIx64 "    fdp = 0x%016" PRIx64 "\n",
           registers->fip, registers->fdp);
    printf("fop = 0x%04" PRIx16 "\n", registers->fop);

    return 0;
}

int X86Tracee::printExtraRegisters()
{
    printf("mxcsr = 0x%08" PRIx32 " = ", registers->mxcsr);
    mxcsrFlags.printFlags(registers->mxcsr);
    std::cout << '\n';

    for (int i = 0; i < 8; ++i) {
        if (i % 2 == 0)
            printf("\n");
        else
            printf(" ");
        uint64_t mm = *reinterpret_cast<uint64_t *>(&registers->st[i]);
        printf("%%mm%d = 0x%016" PRIx64, i, mm);
    }
    std::cout << '\n';

    std::cout << '\n';
    for (int i = 0; i < 16; ++i)
        printf("%%xmm%-2d = 0x%016" PRIx64 "%016" PRIx64 "\n",
               i, registers->xmm[i].hi, registers->xmm[i].lo);

    return 0;
}
