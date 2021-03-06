/*
 * Implementation of an ARM tracee. EABI (non-thumb) is assumed at this point.
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
#include "Arch/ARM/ARMTracee.h"
#include "Arch/ARM/UserRegisters.h"

extern const RegisterInfo ARMRegisters;
static const bytestring ARMTrapInstruction = {0xf0, 0x01, 0xf0, 0xe7};

ARMTracee::ARMTracee(pid_t pid, void *sharedMemory, size_t sharedSize)
    : Tracee{ARMRegisters, new UserRegisters, pid, sharedMemory, sharedSize} {}

const bytestring &ARMTracee::getTrapInstruction()
{
    return ARMTrapInstruction;
}

int ARMTracee::setProgramCounter(void *pc)
{
    struct user_regs regs;

    if (ptrace(PTRACE_GETREGS, pid, nullptr, &regs) == -1) {
        perror("ptrace");
        fprintf(stderr, "could not get program counter\n");
        return 1;
    }

    // Hack, since we can't include <asm/ptrace.h> because it redefines all of
    // the ptrace requests
#define ARM_pc uregs[15]
    regs.ARM_pc = (unsigned long) pc;
#undef ARM_pc

    if (ptrace(PTRACE_SETREGS, pid, nullptr, &regs) == -1) {
        perror("ptrace");
        fprintf(stderr, "could not set program counter\n");
        return 1;
    }

    return 0;
}

int ARMTracee::updateRegisters()
{
    struct user_regs regs;

    if (ptrace(PTRACE_GETREGS, pid, nullptr, &regs) == -1) {
        perror("ptrace");
        fprintf(stderr, "could not get registers\n");
        return 1;
    }

    memcpy(registers.get(), &regs, sizeof(unsigned long) * 17);

    return 0;
}

void ARMTracee::printInstruction(const bytestring &machineCode)
{
    if (machineCode.size() % 4) {
        Tracee::printInstruction(machineCode);
        return;
    }

    printf("[");
    for (size_t i = 0; i < machineCode.size(); i += 4) {
        if (i > 0)
            printf(", ");
        uint32_t word;
        word = *reinterpret_cast<const uint32_t *>(machineCode.data() + i);
        printf(PRINTFx32, word);
    }
    printf("]");
}

/* See Tracee.h. */
Tracee *Tracee::createPlatformTracee(pid_t pid, void *sharedMemory,
                                     size_t sharedSize)
{
    return new ARMTracee{pid, sharedMemory, sharedSize};
}

#include "Tracee.inc"
