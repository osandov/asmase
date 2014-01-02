#include <cinttypes>
#include <cstddef>
#include <cstring>
#include <iostream>

#include <sys/ptrace.h>
#include <sys/user.h>

#include "Arch/X86Tracee.h"
#include "RegisterInfo.h"

static const RegisterInfo X86Registers = {
    .generalPurpose = {
        {RegisterType::INT64, "%", "rax", offsetof(UserRegisters, rax)},
        {RegisterType::INT64, "%", "rcx", offsetof(UserRegisters, rcx)},
        {RegisterType::INT64, "%", "rdx", offsetof(UserRegisters, rdx)},
        {RegisterType::INT64, "%", "rbx", offsetof(UserRegisters, rbx)},
        {RegisterType::INT64, "%", "rsp", offsetof(UserRegisters, rsp)},
        {RegisterType::INT64, "%", "rbp", offsetof(UserRegisters, rbp)},
        {RegisterType::INT64, "%", "rsi", offsetof(UserRegisters, rsi)},
        {RegisterType::INT64, "%", "rdi", offsetof(UserRegisters, rdi)},
        {RegisterType::INT64, "%", "r8",  offsetof(UserRegisters, r8)},
        {RegisterType::INT64, "%", "r9",  offsetof(UserRegisters, r9)},
        {RegisterType::INT64, "%", "r10", offsetof(UserRegisters, r10)},
        {RegisterType::INT64, "%", "r11", offsetof(UserRegisters, r11)},
        {RegisterType::INT64, "%", "r12", offsetof(UserRegisters, r12)},
        {RegisterType::INT64, "%", "r13", offsetof(UserRegisters, r13)},
        {RegisterType::INT64, "%", "r14", offsetof(UserRegisters, r14)},
        {RegisterType::INT64, "%", "r15", offsetof(UserRegisters, r15)},
    },

    .conditionCodes = {
        {RegisterType::INT32, "eflags", offsetof(UserRegisters, eflags)},
    },

    .programCounter = {RegisterType::INT64, "%", "rip", offsetof(UserRegisters, rip)},

    .segmentation = {
        {RegisterType::INT16, "%", "cs", offsetof(UserRegisters, cs)},
        {RegisterType::INT16, "%", "ss", offsetof(UserRegisters, ss)},
        {RegisterType::INT16, "%", "ds", offsetof(UserRegisters, ds)},
        {RegisterType::INT16, "%", "es", offsetof(UserRegisters, es)},
        {RegisterType::INT16, "%", "fs", offsetof(UserRegisters, fs)},
        {RegisterType::INT16, "%", "gs", offsetof(UserRegisters, gs)},
        {RegisterType::INT64, "%", "fs.base", offsetof(UserRegisters, fsBase)},
        {RegisterType::INT64, "%", "gs.base", offsetof(UserRegisters, gsBase)},
    },

    .floatingPoint = {
        {RegisterType::LONG_DOUBLE, "%", "st(0)", offsetof(UserRegisters, st[0])},
        {RegisterType::LONG_DOUBLE, "%", "st(1)", offsetof(UserRegisters, st[1])},
        {RegisterType::LONG_DOUBLE, "%", "st(2)", offsetof(UserRegisters, st[2])},
        {RegisterType::LONG_DOUBLE, "%", "st(3)", offsetof(UserRegisters, st[3])},
        {RegisterType::LONG_DOUBLE, "%", "st(4)", offsetof(UserRegisters, st[4])},
        {RegisterType::LONG_DOUBLE, "%", "st(5)", offsetof(UserRegisters, st[5])},
        {RegisterType::LONG_DOUBLE, "%", "st(6)", offsetof(UserRegisters, st[6])},
        {RegisterType::LONG_DOUBLE, "%", "st(7)", offsetof(UserRegisters, st[7])},
    },

    .floatingPointStatus = {
        {RegisterType::INT16, "fcw", offsetof(UserRegisters, fcw)},
        {RegisterType::INT16, "fsw", offsetof(UserRegisters, fsw)},
        {RegisterType::INT16, "ftw", offsetof(UserRegisters, ftw)},
        {RegisterType::INT16, "fop", offsetof(UserRegisters, fop)},
        {RegisterType::INT64, "fip", offsetof(UserRegisters, fip)},
        {RegisterType::INT64, "fdp", offsetof(UserRegisters, fdp)},
    },

    .extra = {
        {RegisterType::INT64, "%", "mm0", offsetof(UserRegisters, st[0])},
        {RegisterType::INT64, "%", "mm1", offsetof(UserRegisters, st[1])},
        {RegisterType::INT64, "%", "mm2", offsetof(UserRegisters, st[2])},
        {RegisterType::INT64, "%", "mm3", offsetof(UserRegisters, st[3])},
        {RegisterType::INT64, "%", "mm4", offsetof(UserRegisters, st[4])},
        {RegisterType::INT64, "%", "mm5", offsetof(UserRegisters, st[5])},
        {RegisterType::INT64, "%", "mm6", offsetof(UserRegisters, st[6])},
        {RegisterType::INT64, "%", "mm7", offsetof(UserRegisters, st[7])},

        {RegisterType::INT128, "%", "xmm0",  offsetof(UserRegisters, xmm[0])},
        {RegisterType::INT128, "%", "xmm1",  offsetof(UserRegisters, xmm[1])},
        {RegisterType::INT128, "%", "xmm2",  offsetof(UserRegisters, xmm[2])},
        {RegisterType::INT128, "%", "xmm3",  offsetof(UserRegisters, xmm[3])},
        {RegisterType::INT128, "%", "xmm4",  offsetof(UserRegisters, xmm[4])},
        {RegisterType::INT128, "%", "xmm5",  offsetof(UserRegisters, xmm[5])},
        {RegisterType::INT128, "%", "xmm6",  offsetof(UserRegisters, xmm[6])},
        {RegisterType::INT128, "%", "xmm7",  offsetof(UserRegisters, xmm[7])},
        {RegisterType::INT128, "%", "xmm8",  offsetof(UserRegisters, xmm[8])},
        {RegisterType::INT128, "%", "xmm9",  offsetof(UserRegisters, xmm[9])},
        {RegisterType::INT128, "%", "xmm10", offsetof(UserRegisters, xmm[10])},
        {RegisterType::INT128, "%", "xmm11", offsetof(UserRegisters, xmm[11])},
        {RegisterType::INT128, "%", "xmm12", offsetof(UserRegisters, xmm[12])},
        {RegisterType::INT128, "%", "xmm13", offsetof(UserRegisters, xmm[13])},
        {RegisterType::INT128, "%", "xmm14", offsetof(UserRegisters, xmm[14])},
        {RegisterType::INT128, "%", "xmm15", offsetof(UserRegisters, xmm[15])},
    },

    .extraStatus = {
        {RegisterType::INT32, "mxcsr", offsetof(UserRegisters, mxcsr)},
    },
};

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

    if (ptrace(PTRACE_GETREGS, pid, NULL, &regs) == -1 ||
        ptrace(PTRACE_GETFPREGS, pid, NULL, &fpregs) == -1) {
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
    return new X86Tracee(pid, sharedMemory, sharedSize);
}

#include "Tracee.inc"
