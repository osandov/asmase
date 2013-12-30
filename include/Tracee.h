#ifndef ASMASE_TRACEE_H
#define ASMASE_TRACEE_H

#include <string>
#include <memory>

#include <sys/types.h>

struct RegisterInfo;
enum class RegisterCategory;

class Tracee {
protected:
    // Architecture-dependent information
    const RegisterInfo &regInfo;
    const std::string &trapInstruction;

    pid_t pid;
    void *sharedMemory;
    size_t sharedSize;

    virtual int setProgramCounter(void *pc) = 0;

public:
    Tracee(const RegisterInfo &regInfo, const std::string &trapInstruction,
           pid_t pid, void *sharedMemory, size_t sharedSize)
        : regInfo(regInfo), trapInstruction(trapInstruction),
          pid(pid), sharedMemory(sharedMemory), sharedSize(sharedSize) {}

    int executeInstruction(const std::string &instruction);

    int printRegisters(RegisterCategory category);
};

std::shared_ptr<Tracee> createTracee();

#endif /* ASMASE_TRACEE_H */
