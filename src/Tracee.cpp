/*
 * Architecture-independent tracee implementation. This includes the core logic
 * for manipulating the tracee process.
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

#include <algorithm>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <utility>

#include <unistd.h>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/wait.h>

#include "RegisterInfo.h"
#include "Tracee.h"

std::vector<std::pair<RegisterCategory, Tracee::RegisterCategoryPrinter>>
Tracee::categoryPrinters = {
    {RegisterCategory::GENERAL_PURPOSE, &Tracee::printGeneralPurposeRegisters},
    {RegisterCategory::CONDITION_CODE,  &Tracee::printConditionCodeRegisters},
    {RegisterCategory::FLOATING_POINT,  &Tracee::printFloatingPointRegisters},
    {RegisterCategory::EXTRA,           &Tracee::printExtraRegisters},
    {RegisterCategory::SEGMENTATION,    &Tracee::printSegmentationRegisters},
};

/* See Tracee.h. */
int Tracee::executeInstruction(const bytestring &machineCode)
{
    unsigned char *shared = reinterpret_cast<unsigned char *>(sharedMemory);
    const bytestring &trapInstruction = getTrapInstruction();

    if (machineCode.size() + trapInstruction.size() >= sharedSize) {
        fprintf(stderr, "instruction too long\n");
        return 1;
    }

    memcpy(shared, machineCode.c_str(), machineCode.size());
    shared += machineCode.size();
    memcpy(shared, trapInstruction.c_str(), trapInstruction.size());

    int waitStatus;

    if (setProgramCounter(sharedMemory))
        return -1;

retry:
    if (ptrace(PTRACE_CONT, pid, nullptr, 0) == -1) {
        perror("ptrace");
        fprintf(stderr, "could not continue tracee\n");
        return -1;
    }

    if (waitpid(pid, &waitStatus, 0) == -1) {
        perror("waitpid");
        fprintf(stderr, "could not wait for tracee\n");
        return -1;
    }

    if (WIFEXITED(waitStatus)) {
        fprintf(stderr, "tracee exited with status %d\n",
            WEXITSTATUS(waitStatus));
        return -1;
    } else if (WIFSIGNALED(waitStatus)) {
        fprintf(stderr, "tracee was terminated (%s)\n",
            strsignal(WTERMSIG(waitStatus)));
        return -1;
    } else if (WIFSTOPPED(waitStatus)) {
        int signal = WSTOPSIG(waitStatus);
        switch (signal) {
            case SIGTRAP:
                break;
            case SIGWINCH:
                // We don't want to be interrupted if the window changes size,
                // so continue the process and keep waiting
                goto retry;
            default:
                printf("tracee was stopped (%s)\n", 
                    strsignal(WSTOPSIG(waitStatus)));
                return 0;
        }
    } else if (WIFCONTINUED(waitStatus)) {
        fprintf(stderr, "tracee continued\n");
        return -1;
    } else {
        fprintf(stderr, "tracee disappeared\n");
        return -1;
    }

    return 0;
}

/* See Tracee.h. */
void Tracee::printInstruction(const bytestring &machineCode)
{
    printf("[");
    for (size_t i = 0; i < machineCode.size(); ++i) {
        if (i > 0)
            printf(", ");
        printf(PRINTFx8, machineCode[i]);
    }
    printf("]");
}

/* See Tracee.h. */
std::shared_ptr<RegisterValue> Tracee::getRegisterValue(const std::string &regName)
{
    auto registerHasName =
        [regName](const RegisterDesc &reg) { return reg.name == regName; };

    auto reg = std::find_if(std::begin(regInfo.registers),
                            std::end(regInfo.registers),
                            registerHasName);

    if (reg == std::end(regInfo.registers))
        return {nullptr};
    else {
        updateRegisters();
        return std::shared_ptr<RegisterValue>{reg->getValue(*registers)};
    }
}

/* See Tracee.h. */
int Tracee::printGeneralPurposeRegisters()
{
    fprintf(stderr, "no general-purpose registers on this architecture\n");
    return 1;
}

/* See Tracee.h. */
int Tracee::printConditionCodeRegisters()
{
    fprintf(stderr, "no condition code registers on this architecture\n");
    return 1;
}

/* See Tracee.h. */
int Tracee::printSegmentationRegisters()
{
    fprintf(stderr, "no segmentation registers on this architecture\n");
    return 1;
}

/* See Tracee.h. */
int Tracee::printFloatingPointRegisters()
{
    fprintf(stderr, "no floating point registers on this architecture\n");
    return 1;
}

/* See Tracee.h. */
int Tracee::printExtraRegisters()
{
    fprintf(stderr, "no extra registers on this architecture\n");
    return 1;
}

/* See Tracee.h. */
int Tracee::printRegisters(RegisterCategory categories)
{
    bool firstPrinted = true;
    int all_error = 0;
    int error;

    if (any(categories))
        updateRegisters();

    for (auto &categoryPrinter : categoryPrinters) {
        if (any(categories & categoryPrinter.first)) {
            if (!firstPrinted)
                printf("\n");
            firstPrinted = false;

            if ((error = (this->*categoryPrinter.second)()))
                all_error = error;
        }
    }

    return all_error;
}

/** Entry point for the tracee. Request to be ptraced and trap immediately. */
static void traceeProcess() __attribute__((noreturn));

/** Set up an signal handlers needed by the tracer. */
static void installTracerSignalHandlers();

/* See Tracee.h. */
std::shared_ptr<Tracee> Tracee::createTracee()
{
    pid_t pid;
    void *sharedPage;
    size_t pageSize = sysconf(_SC_PAGESIZE);

    sharedPage = mmap(nullptr, pageSize, PROT_READ | PROT_WRITE | PROT_EXEC,
                      MAP_ANONYMOUS | MAP_SHARED, -1, 0);

    if (!sharedPage) {
        perror("mmap");
        fprintf(stderr, "could not create shared memory\n");
        return {nullptr};
    }

    if ((pid = fork()) == -1) {
        perror("fork");
        fprintf(stderr, "could not fork tracee\n");
        return {nullptr};
    }

    if (pid == 0)
        traceeProcess(); // This never returns

    installTracerSignalHandlers();

    Tracee *platformTracee = createPlatformTracee(pid, sharedPage, pageSize);
    return std::shared_ptr<Tracee>{platformTracee};
}

/* See above. */
static void traceeProcess()
{
    if (ptrace(PTRACE_TRACEME, -1, nullptr, nullptr) == -1) {
        perror("ptrace");
        abort();
    }

    if (raise(SIGTRAP)) {
        perror("raise");
        abort();
    }

    // We shouldn't make it here, but if we do...
    abort();
}

/* See above. */
static void installTracerSignalHandlers()
{
    struct sigaction act;
    act.sa_handler = SIG_IGN;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);

    // Ignore SIGINT so the user can break out of the tracee
    sigaction(SIGINT, &act, nullptr);
}
