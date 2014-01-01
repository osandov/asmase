#ifndef ASMASE_TRACEE_H
#define ASMASE_TRACEE_H

#include <string>
#include <memory>

#include <sys/types.h>

struct RegisterDesc;
struct RegisterInfo;
struct RegisterValue;
enum class RegisterCategory;
struct UserRegisters;

class Tracee {
protected:
    // Architecture-dependent information
    const RegisterInfo &regInfo;
    const std::string &trapInstruction;
    std::unique_ptr<UserRegisters> registers;

    pid_t pid;
    void *sharedMemory;
    size_t sharedSize;

    virtual int setProgramCounter(void *pc) = 0;
    virtual int updateRegisters() = 0;

public:
    Tracee(const RegisterInfo &regInfo, const std::string &trapInstruction,
            UserRegisters *registers, pid_t pid,
            void *sharedMemory, size_t sharedSize);
    ~Tracee();

    int executeInstruction(const std::string &instruction);

    int printRegisters(RegisterCategory category);

    std::shared_ptr<RegisterValue> getRegisterValue(const std::string &regName);

    pid_t getPid() const { return pid; }
};

std::shared_ptr<Tracee> createTracee();

#endif /* ASMASE_TRACEE_H */
