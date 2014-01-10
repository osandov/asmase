/*
 * ARM tracee class.
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

#ifndef ASMASE_ARCH_ARM_ARMTRACEE_H
#define ASMASE_ARCH_ARM_ARMTRACEE_H

#include "Tracee.h"

class ARMTracee : public Tracee {
    virtual const bytestring &getTrapInstruction();

    virtual int setProgramCounter(void *pc);
    virtual int updateRegisters();

    virtual int printGeneralPurposeRegisters();
    virtual int printConditionCodeRegisters();

public:
    ARMTracee(pid_t pid, void *sharedMemory, size_t sharedSize);

    virtual void printInstruction(const bytestring &machineCode);
};

#endif /* ASMASE_ARCH_ARM_ARMTRACEE_H */
