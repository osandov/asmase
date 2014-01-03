#include <cinttypes>
#include <cstddef>
#include <cstring>
#include <iostream>

#include <sys/ptrace.h>
#include <sys/user.h>

#include "Arch/X86Tracee.h"
#include "RegisterInfo.h"

#define USER_REGISTER(reg) offsetof(UserRegisters, reg)
using RT = RegisterType;
using RC = RegisterCategory;
static const RegisterInfo X86Registers = {
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
        {RT::INT32, RC::STATUS, "eflags", USER_REGISTER(eflags)},

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
        {RT::INT16, RC::FLOATING_POINT | RC::STATUS, "fcw", USER_REGISTER(fcw)},
        {RT::INT16, RC::FLOATING_POINT | RC::STATUS, "fsw", USER_REGISTER(fsw)},
        {RT::INT16, RC::FLOATING_POINT | RC::STATUS, "ftw", USER_REGISTER(ftw)},
        {RT::INT16, RC::FLOATING_POINT | RC::STATUS, "fop", USER_REGISTER(fop)},
        {RT::INT64, RC::FLOATING_POINT | RC::STATUS, "fip", USER_REGISTER(fip)},
        {RT::INT64, RC::FLOATING_POINT | RC::STATUS, "fdp", USER_REGISTER(fdp)},

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
        {RT::INT32, RC::EXTRA | RC::STATUS, "mxcsr", USER_REGISTER(mxcsr)},
    },
};
#undef USER_REGISTER

static const std::string X86TrapInstruction = "\xcc";

X86Tracee::X86Tracee(pid_t pid, void *sharedMemory, size_t sharedSize)
    : Tracee(X86Registers, X86TrapInstruction, new UserRegisters,
             pid, sharedMemory, sharedSize) {}

int X86Tracee::setProgramCounter(void *pc)
{
    struct user_regs_struct regs;

    if (ptrace(PTRACE_GETREGS, pid, NULL, &regs) == -1) {
        perror("ptrace");
        std::cerr << "could not get program counter\n";
        return 1;
    }

    regs.rip = (unsigned long long) pc;

    if (ptrace(PTRACE_SETREGS, pid, NULL, &regs) == -1) {
        perror("ptrace");
        std::cerr << "could not set program counter\n";
        return 1;
    }

    return 0;
}

template <typename T>
void copyRegister(T *dest, void *src)
{
    memcpy(dest, src, sizeof(T));
}

int X86Tracee::updateRegisters()
{
    struct user_regs_struct regs;
    struct user_fpregs_struct fpregs;

    if (ptrace(PTRACE_GETREGS, pid, nullptr, &regs) == -1 ||
        ptrace(PTRACE_GETFPREGS, pid, nullptr, &fpregs) == -1) {
        perror("ptrace");
        std::cerr << "could not get registers\n";
        return 1;
    }

    copyRegister(&registers->rax, &regs.rax);
    copyRegister(&registers->rcx, &regs.rcx);
    copyRegister(&registers->rdx, &regs.rdx);
    copyRegister(&registers->rbx, &regs.rbx);
    copyRegister(&registers->rsp, &regs.rsp);
    copyRegister(&registers->rbp, &regs.rbp);
    copyRegister(&registers->rsi, &regs.rsi);
    copyRegister(&registers->rdi, &regs.rdi);
    copyRegister(&registers->r8,  &regs.r8);
    copyRegister(&registers->r9,  &regs.r9);
    copyRegister(&registers->r10, &regs.r10);
    copyRegister(&registers->r11, &regs.r11);
    copyRegister(&registers->r12, &regs.r12);
    copyRegister(&registers->r13, &regs.r13);
    copyRegister(&registers->r14, &regs.r14);
    copyRegister(&registers->r15, &regs.r15);

    copyRegister(&registers->eflags, &regs.eflags);
    copyRegister(&registers->rip, &regs.rip);

    copyRegister(&registers->cs, &regs.cs);
    copyRegister(&registers->ss, &regs.ss);
    copyRegister(&registers->ds, &regs.ds);
    copyRegister(&registers->es, &regs.es);
    copyRegister(&registers->fs, &regs.fs);
    copyRegister(&registers->gs, &regs.gs);

    copyRegister(&registers->fsBase, &regs.fs_base);
    copyRegister(&registers->gsBase, &regs.gs_base);

    for (int i = 0; i < 8; ++i)
        copyRegister(&registers->st[i], &fpregs.st_space[4 * i]);
    copyRegister(&registers->fcw, &fpregs.cwd);
    copyRegister(&registers->fsw, &fpregs.swd);
    copyRegister(&registers->ftw, &fpregs.ftw);
    copyRegister(&registers->fop, &fpregs.fop);
    copyRegister(&registers->fip, &fpregs.rip);
    copyRegister(&registers->fdp, &fpregs.rdp);

    for (int i = 0; i < 16; ++i)
        copyRegister(&registers->xmm[i], &fpregs.xmm_space[4 * i]);
    copyRegister(&registers->mxcsr, &fpregs.mxcsr);

    return 0;
}

Tracee *createPlatformTracee(pid_t pid, void *sharedMemory, size_t sharedSize)
{
    return new X86Tracee{pid, sharedMemory, sharedSize};
}

#include "Tracee.inc"
