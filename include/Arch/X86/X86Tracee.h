#ifndef ASMASE_ARCH_X86_X86TRACEE_H
#define ASMASE_ARCH_X86_X86TRACEE_H

#include "Tracee.h"

class X86Tracee : public Tracee {
    int setProgramCounter(void *pc);
    int updateRegisters();

    int printGeneralPurposeRegisters();
    int printConditionCodeRegisters();
    int printProgramCounterRegisters();
    int printSegmentationRegisters();
    int printFloatingPointRegisters();
    int printExtraRegisters();

    void reconstructTagWord();

public:
    X86Tracee(pid_t pid, void *sharedMemory, size_t sharedSize);
};

#endif /* ASMASE_ARCH_X86_X86TRACEE_H */
