#include <cinttypes>
#include <cstddef>

#include <sys/ptrace.h>
#include <sys/user.h>

#include "Arch/X86Tracee.h"
#include "RegisterInfo.h"

static const RegisterInfo X86Registers = {
    .generalPurpose = {
        RegisterDT<uint64_t>{"%", "rax", offsetof(UserRegisters, rax)},
        RegisterDT<uint64_t>{"%", "rcx", offsetof(UserRegisters, rcx)},
        RegisterDT<uint64_t>{"%", "rdx", offsetof(UserRegisters, rdx)},
        RegisterDT<uint64_t>{"%", "rbx", offsetof(UserRegisters, rbx)},
        RegisterDT<uint64_t>{"%", "rsp", offsetof(UserRegisters, rbp)},
        RegisterDT<uint64_t>{"%", "rsi", offsetof(UserRegisters, rdi)},
        RegisterDT<uint64_t>{"%", "r8",  offsetof(UserRegisters, r8)},
        RegisterDT<uint64_t>{"%", "r9",  offsetof(UserRegisters, r9)},
        RegisterDT<uint64_t>{"%", "r10", offsetof(UserRegisters, r10)},
        RegisterDT<uint64_t>{"%", "r11", offsetof(UserRegisters, r11)},
        RegisterDT<uint64_t>{"%", "r12", offsetof(UserRegisters, r12)},
        RegisterDT<uint64_t>{"%", "r13", offsetof(UserRegisters, r13)},
        RegisterDT<uint64_t>{"%", "r14", offsetof(UserRegisters, r14)},
        RegisterDT<uint64_t>{"%", "r15", offsetof(UserRegisters, r15)},
    },

    .conditionCodes = {RegisterDT<uint32_t>{"eflags", offsetof(UserRegisters, eflags)}},

    .programCounter = RegisterDT<uint64_t>{"%", "rip", offsetof(UserRegisters, rip)},

    .segmentation = {
        RegisterDT<uint16_t>{"%", "cs", offsetof(UserRegisters, cs)},
        RegisterDT<uint16_t>{"%", "ss", offsetof(UserRegisters, ss)},
        RegisterDT<uint16_t>{"%", "ds", offsetof(UserRegisters, ds)},
        RegisterDT<uint16_t>{"%", "es", offsetof(UserRegisters, es)},
        RegisterDT<uint16_t>{"%", "fs", offsetof(UserRegisters, fs)},
        RegisterDT<uint16_t>{"%", "gs", offsetof(UserRegisters, gs)},
        RegisterDT<uint64_t>{"%", "fs.base", offsetof(UserRegisters, fs_base)},
        RegisterDT<uint64_t>{"%", "gs.base", offsetof(UserRegisters, gs_base)},
    },

    .floatingPoint = {
        RegisterDT<long double>{"%", "st(0)", offsetof(UserRegisters, st[0])},
        RegisterDT<long double>{"%", "st(1)", offsetof(UserRegisters, st[1])},
        RegisterDT<long double>{"%", "st(2)", offsetof(UserRegisters, st[2])},
        RegisterDT<long double>{"%", "st(3)", offsetof(UserRegisters, st[3])},
        RegisterDT<long double>{"%", "st(4)", offsetof(UserRegisters, st[4])},
        RegisterDT<long double>{"%", "st(5)", offsetof(UserRegisters, st[5])},
        RegisterDT<long double>{"%", "st(6)", offsetof(UserRegisters, st[6])},
        RegisterDT<long double>{"%", "st(7)", offsetof(UserRegisters, st[7])},
    },

    .floatingPointStatus = {
        RegisterDT<uint16_t>{"fcw", offsetof(UserRegisters, fcw)},
        RegisterDT<uint16_t>{"fsw", offsetof(UserRegisters, fsw)},
        RegisterDT<uint16_t>{"ftw", offsetof(UserRegisters, ftw)},
        RegisterDT<uint16_t>{"fop", offsetof(UserRegisters, fop)},
        RegisterDT<uint64_t>{"fip", offsetof(UserRegisters, fip)},
        RegisterDT<uint64_t>{"fdp", offsetof(UserRegisters, fdp)},
    },

    .extra = {
        RegisterDT<uint64_t>{"%", "mm0", offsetof(UserRegisters, st[0])},
        RegisterDT<uint64_t>{"%", "mm1", offsetof(UserRegisters, st[1])},
        RegisterDT<uint64_t>{"%", "mm2", offsetof(UserRegisters, st[2])},
        RegisterDT<uint64_t>{"%", "mm3", offsetof(UserRegisters, st[3])},
        RegisterDT<uint64_t>{"%", "mm4", offsetof(UserRegisters, st[4])},
        RegisterDT<uint64_t>{"%", "mm5", offsetof(UserRegisters, st[5])},
        RegisterDT<uint64_t>{"%", "mm6", offsetof(UserRegisters, st[6])},
        RegisterDT<uint64_t>{"%", "mm7", offsetof(UserRegisters, st[7])},

        RegisterDT<xmm_t>{"%", "xmm0", offsetof(UserRegisters, xmm[0])},
        RegisterDT<xmm_t>{"%", "xmm1", offsetof(UserRegisters, xmm[1])},
        RegisterDT<xmm_t>{"%", "xmm2", offsetof(UserRegisters, xmm[2])},
        RegisterDT<xmm_t>{"%", "xmm3", offsetof(UserRegisters, xmm[3])},
        RegisterDT<xmm_t>{"%", "xmm4", offsetof(UserRegisters, xmm[4])},
        RegisterDT<xmm_t>{"%", "xmm5", offsetof(UserRegisters, xmm[5])},
        RegisterDT<xmm_t>{"%", "xmm6", offsetof(UserRegisters, xmm[6])},
        RegisterDT<xmm_t>{"%", "xmm7", offsetof(UserRegisters, xmm[7])},
        RegisterDT<xmm_t>{"%", "xmm8", offsetof(UserRegisters, xmm[8])},
        RegisterDT<xmm_t>{"%", "xmm9", offsetof(UserRegisters, xmm[9])},
        RegisterDT<xmm_t>{"%", "xmm10", offsetof(UserRegisters, xmm[10])},
        RegisterDT<xmm_t>{"%", "xmm11", offsetof(UserRegisters, xmm[11])},
        RegisterDT<xmm_t>{"%", "xmm12", offsetof(UserRegisters, xmm[12])},
        RegisterDT<xmm_t>{"%", "xmm13", offsetof(UserRegisters, xmm[13])},
        RegisterDT<xmm_t>{"%", "xmm14", offsetof(UserRegisters, xmm[14])},
        RegisterDT<xmm_t>{"%", "xmm15", offsetof(UserRegisters, xmm[15])},
    },

    .extraStatus = {RegisterDT<uint32_t>{"mxcsr", offsetof(UserRegisters, mxcsr)}},
};

static const std::string X86TrapInstruction = "\xcc";

X86Tracee::X86Tracee(pid_t pid, void *sharedMemory, size_t sharedSize)
    : Tracee(X86Registers, X86TrapInstruction, pid, sharedMemory, sharedSize) {}

int X86Tracee::setProgramCounter(void *pc)
{
    struct user_regs_struct regs;

    if (ptrace(PTRACE_GETREGS, pid, NULL, &regs) == -1) {
        perror("ptrace");
        fprintf(stderr, "could not get program counter\n");
        return 1;
    }

    regs.rip = (unsigned long long) pc;

    if (ptrace(PTRACE_SETREGS, pid, NULL, &regs) == -1) {
        perror("ptrace");
        fprintf(stderr, "could not set program counter\n");
        return 1;
    }

    return 0;
}

Tracee *createPlatformTracee(pid_t pid, void *sharedMemory, size_t sharedSize)
{
    return new X86Tracee(pid, sharedMemory, sharedSize);
}
