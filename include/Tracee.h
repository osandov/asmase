/*
 * Tracee class.
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

#ifndef ASMASE_TRACEE_H
#define ASMASE_TRACEE_H

#include <memory>
#include <utility>

#include <sys/types.h>

#include "Support.h"

enum class RegisterCategory;
class RegisterDesc;
class RegisterInfo;
class RegisterValue;

/**
 * A platform-independent representation of registers (obviously
 * architecture-dependent). This provides the possibility for easier porting to
 * other *nix-es in the future and also allows us to avoid some of the stranger
 * layouts of registers returned by ptrace.
 */
class UserRegisters;

/**
 * Class encapsulating a tracee process. This process is used to execute
 * instructions given by the user.
 */
class Tracee {
    typedef int (Tracee::*RegisterCategoryPrinter)();
    static std::vector<std::pair<RegisterCategory, RegisterCategoryPrinter>>
        categoryPrinters;

    /** Create the tracee for the host platform. */
    static Tracee *createPlatformTracee(pid_t pid, void *sharedMemory,
                                        size_t sharedSize);

protected:
    // Architecture-dependent information
    /** Register information. */
    const RegisterInfo &regInfo;

    /** Instruction to use to trigger a software trap (i.e., breakpoint). */
    const bytestring &trapInstruction;

    /**
     * Actual values of the tracee's registers. This is updated lazily for
     * methods that need to know the register values.
     */
    const std::unique_ptr<UserRegisters> registers;

    /** PID of the tracee process. */
    pid_t pid;

    /** Start of memory shared with the tracee. */
    void *sharedMemory;

    /** Size of memory shared with the tracee. */
    size_t sharedSize;

    /**
     * Set the program counter of the tracee to the given location.
     * @return Zero on success, nonzero on failure.
     */
    virtual int setProgramCounter(void *pc) = 0;

    /**
     * Update the register values stored in the registers pointer.
     * @return Zero on success, nonzero on failure.
     */
    virtual int updateRegisters() = 0;

    // Register category printers. The default implementations assume that the
    // architecture does not have registers of that category.
    virtual int printGeneralPurposeRegisters();
    virtual int printConditionCodeRegisters();
    virtual int printSegmentationRegisters();
    virtual int printFloatingPointRegisters();
    virtual int printExtraRegisters();

public:
    Tracee(const RegisterInfo &regInfo, const bytestring &trapInstruction,
           UserRegisters *registers, pid_t pid, void *sharedMemory,
           size_t sharedSize);
    ~Tracee();

    pid_t getPid() const { return pid; }

    /**
     * Execute the given instruction on the tracee.
     * @return Zero on success, nonzero on failure.
     */
    int executeInstruction(const bytestring &machineCode);

    /**
     * Print the registers in the given categories (which may be a bitwise OR
     * of multiple categories).
     * @return Zero on success, nonzero on failure.
     */
    int printRegisters(RegisterCategory categories);

    /**
     * Get the current value of a register.
     * @return nullptr on error.
     */
    std::shared_ptr<RegisterValue> getRegisterValue(const std::string &regName);

    /**
     * Create a tracee process.
     * @return nullptr on error.
     */
    static std::shared_ptr<Tracee> createTracee();
};

#endif /* ASMASE_TRACEE_H */
