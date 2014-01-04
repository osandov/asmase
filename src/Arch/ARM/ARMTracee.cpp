#include <cstring>
#include <iostream>

#include <sys/ptrace.h>
#include <sys/user.h>

#include "RegisterInfo.h"
#include "Arch/ARM/ARMTracee.h"
#include "Arch/ARM/UserRegisters.h"

extern const RegisterInfo ARMRegisters;
static const std::string ARMTrapInstruction = "\xf0\x01\xf0\xe7";

ARMTracee::ARMTracee(pid_t pid, void *sharedMemory, size_t sharedSize)
    : Tracee{ARMRegisters, ARMTrapInstruction, new UserRegisters,
             pid, sharedMemory, sharedSize} {}

int ARMTracee::setProgramCounter(void *pc)
{
    struct user_regs regs;

    if (ptrace(PTRACE_GETREGS, pid, nullptr, &regs) == -1) {
        perror("ptrace");
        std::cerr << "could not get program counter\n";
        return 1;
    }

    // Hack, since we can't include <asm/ptrace.h> because it redefines all of
    // the ptrace requests
#define ARM_pc uregs[15]
    regs.ARM_pc = (unsigned long) pc;
#undef ARM_pc

    if (ptrace(PTRACE_SETREGS, pid, nullptr, &regs) == -1) {
        perror("ptrace");
        std::cerr << "could not set program counter\n";
        return 1;
    }

    return 0;
}

int ARMTracee::updateRegisters()
{
    struct user_regs regs;

    if (ptrace(PTRACE_GETREGS, pid, nullptr, &regs) == -1) {
        perror("ptrace");
        std::cerr << "could not get registers\n";
        return 1;
    }

    memcpy(registers.get(), &regs, sizeof(unsigned long) * 17);

    return 0;
}

/* See Tracee.h. */
Tracee *Tracee::createPlatformTracee(pid_t pid, void *sharedMemory,
                                     size_t sharedSize)
{
    return new ARMTracee{pid, sharedMemory, sharedSize};
}

#include "Tracee.inc"
