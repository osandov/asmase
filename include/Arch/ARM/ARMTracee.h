#ifndef ASMASE_ARCH_ARM_ARMTRACEE_H
#define ASMASE_ARCH_ARM_ARMTRACEE_H

#include "Tracee.h"

class ARMTracee : public Tracee {
    virtual int setProgramCounter(void *pc);
    virtual int updateRegisters();

    virtual int printGeneralPurposeRegisters();
    virtual int printConditionCodeRegisters();
    virtual int printProgramCounterRegisters();

public:
    ARMTracee(pid_t pid, void *sharedMemory, size_t sharedSize);
};

#endif /* ASMASE_ARCH_ARM_ARMTRACEE_H */
