#ifndef ASMASE_ARCH_X86_X86TRACEE_H
#define ASMASE_ARCH_X86_X86TRACEE_H

#include "Tracee.h"

class X86Tracee : public Tracee {
    virtual int setProgramCounter(void *pc);
    virtual int updateRegisters();

    virtual int printGeneralPurposeRegisters();
    virtual int printConditionCodeRegisters();
    virtual int printSegmentationRegisters();
    virtual int printFloatingPointRegisters();
    virtual int printExtraRegisters();

    /**
     * ptrace returns the floating point tag word as a simple bitmap of valid
     * or not; this reconstructs the processor's actual tag word from the
     * floating point stack itself.
     */
    void reconstructTagWord();

public:
    X86Tracee(pid_t pid, void *sharedMemory, size_t sharedSize);
};

#endif /* ASMASE_ARCH_X86_X86TRACEE_H */
