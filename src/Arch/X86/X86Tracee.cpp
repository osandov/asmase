/*
 * Implementation of an x86 tracee (either IA-32 or AMD64).
 *
 * Copyright (C) 2013-2014 Omar Sandoval
 *
 * This file is part of asmase.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <cstdio>
#include <cstring>

#include <sys/ptrace.h>
#include <sys/user.h>

#include "RegisterInfo.h"
#include "Arch/X86/X86Tracee.h"
#include "Arch/X86/UserRegisters.h"

extern const RegisterInfo X86Registers;
static const bytestring X86TrapInstruction = {0xcc};

X86Tracee::X86Tracee(pid_t pid, void *sharedMemory, size_t sharedSize)
    : Tracee{X86Registers, X86TrapInstruction, new UserRegisters,
             pid, sharedMemory, sharedSize} {}

int X86Tracee::setProgramCounter(void *pc)
{
    struct user_regs_struct regs;

    if (ptrace(PTRACE_GETREGS, pid, nullptr, &regs) == -1) {
        perror("ptrace");
        fprintf(stderr, "could not get program counter\n");
        return 1;
    }

#ifdef __x86_64__
    regs.rip = (unsigned long long) pc;
#else
    regs.eip = (long) pc;
#endif

    if (ptrace(PTRACE_SETREGS, pid, nullptr, &regs) == -1) {
        perror("ptrace");
        fprintf(stderr, "could not set program counter\n");
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
#ifdef __x86_64
#define user_fpxregs_struct user_fpregs_struct
#define PTRACE_GETFPXREGS PTRACE_GETFPREGS
#endif
    struct user_regs_struct regs;
    struct user_fpxregs_struct fpxregs;

    if (ptrace(PTRACE_GETREGS, pid, nullptr, &regs) == -1 ||
        ptrace(PTRACE_GETFPXREGS, pid, nullptr, &fpxregs) == -1) {
        perror("ptrace");
        fprintf(stderr, "could not get registers\n");
        return 1;
    }

#ifdef __x86_64__
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
#else
    copyRegister(&registers->eax, &regs.eax);
    copyRegister(&registers->ecx, &regs.ecx);
    copyRegister(&registers->edx, &regs.edx);
    copyRegister(&registers->ebx, &regs.ebx);
    copyRegister(&registers->esp, &regs.esp);
    copyRegister(&registers->ebp, &regs.ebp);
    copyRegister(&registers->esi, &regs.esi);
    copyRegister(&registers->edi, &regs.edi);
#endif

    copyRegister(&registers->eflags, &regs.eflags);

#ifdef __x86_64__
    copyRegister(&registers->rip, &regs.rip);
#else
    copyRegister(&registers->eip, &regs.eip);
#endif

#ifdef __x86_64
    copyRegister(&registers->cs, &regs.cs);
    copyRegister(&registers->ss, &regs.ss);
    copyRegister(&registers->ds, &regs.ds);
    copyRegister(&registers->es, &regs.es);
    copyRegister(&registers->fs, &regs.fs);
    copyRegister(&registers->gs, &regs.gs);
#else
    copyRegister(&registers->cs, &regs.xcs);
    copyRegister(&registers->ss, &regs.xss);
    copyRegister(&registers->ds, &regs.xds);
    copyRegister(&registers->es, &regs.xes);
    copyRegister(&registers->fs, &regs.xfs);
    copyRegister(&registers->gs, &regs.xgs);
#endif

#ifdef __x86_64__
    copyRegister(&registers->fsBase, &regs.fs_base);
    copyRegister(&registers->gsBase, &regs.gs_base);
#endif

    for (int i = 0; i < 8; ++i)
        copyRegister(&registers->st[i], &fpxregs.st_space[4 * i]);
    copyRegister(&registers->fcw, &fpxregs.cwd);
    copyRegister(&registers->fsw, &fpxregs.swd);
#ifdef __x86_64__
    copyRegister(&registers->ftw, &fpxregs.ftw);
#else
    copyRegister(&registers->ftw, &fpxregs.twd);
#endif
    copyRegister(&registers->fop, &fpxregs.fop);
#ifdef __x86_64__
    copyRegister(&registers->fip, &fpxregs.rip);
    copyRegister(&registers->fdp, &fpxregs.rdp);
#else
    copyRegister(&registers->fip, &fpxregs.fip);
    copyRegister(&registers->fcs, &fpxregs.fcs);
    copyRegister(&registers->fdp, &fpxregs.foo);
    copyRegister(&registers->fds, &fpxregs.fos);
#endif

    for (int i = 0; i < UserRegisters::NUM_SSE_REGS; ++i)
        copyRegister(&registers->xmm[i], &fpxregs.xmm_space[4 * i]);
    copyRegister(&registers->mxcsr, &fpxregs.mxcsr);

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
