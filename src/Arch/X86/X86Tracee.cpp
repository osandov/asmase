#include <cstring>
#include <iostream>

#include <sys/ptrace.h>
#include <sys/user.h>

#include "RegisterInfo.h"
#include "Arch/X86/X86Tracee.h"
#include "Arch/X86/UserRegisters.h"

extern const RegisterInfo X86Registers;
static const std::string X86TrapInstruction = "\xcc";

X86Tracee::X86Tracee(pid_t pid, void *sharedMemory, size_t sharedSize)
    : Tracee{X86Registers, X86TrapInstruction, new UserRegisters,
             pid, sharedMemory, sharedSize} {}

int X86Tracee::setProgramCounter(void *pc)
{
    struct user_regs_struct regs;

    if (ptrace(PTRACE_GETREGS, pid, nullptr, &regs) == -1) {
        perror("ptrace");
        std::cerr << "could not get program counter\n";
        return 1;
    }

    regs.rip = (unsigned long long) pc;

    if (ptrace(PTRACE_SETREGS, pid, nullptr, &regs) == -1) {
        perror("ptrace");
        std::cerr << "could not set program counter\n";
        return 1;
    }

    return 0;
}

template <typename T>
inline void copyRegister(T *dest, void *src)
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

    reconstructTagWord();

    return 0;
}

/* See X86Tracee.h. */
void X86Tracee::reconstructTagWord()
{
    uint16_t top = x87_st_top(registers->fsw);
    uint16_t valid_bits = registers->ftw;

    uint16_t ftw = 0;
    for (uint16_t physical = 0; physical < 8; ++physical) {
        uint16_t tag;
        uint16_t logical = x87_phys_to_log(physical, top);

        if (valid_bits & (1 << physical))
            tag = x87_tag(registers->st[logical]);
        else
            tag = 0x3;
        ftw |= tag << (2 * physical);
    }

    registers->ftw = ftw;
}

/* See Tracee.h. */
Tracee *Tracee::createPlatformTracee(pid_t pid, void *sharedMemory,
                                     size_t sharedSize)
{
    return new X86Tracee{pid, sharedMemory, sharedSize};
}

#include "Tracee.inc"
