#ifndef ASMASE_TRACEE_H
#define ASMASE_TRACEE_H

#include <string>
#include <memory>
#include <utility>

#include <sys/types.h>

class RegisterDesc;
class RegisterInfo;
class RegisterValue;
enum class RegisterCategory;
class UserRegisters;

class Tracee {
    typedef int (Tracee::*RegisterCategoryPrinter)();
    static std::vector<std::pair<RegisterCategory, RegisterCategoryPrinter>>
        categoryPrinters;

    static Tracee *createPlatformTracee(pid_t pid, void *sharedMemory,
                                        size_t sharedSize);

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

    virtual int printGeneralPurposeRegisters();
    virtual int printConditionCodeRegisters();
    virtual int printProgramCounterRegisters();
    virtual int printSegmentationRegisters();
    virtual int printFloatingPointRegisters();
    virtual int printExtraRegisters();

public:
    Tracee(const RegisterInfo &regInfo, const std::string &trapInstruction,
           UserRegisters *registers,
           pid_t pid, void *sharedMemory, size_t sharedSize);
    ~Tracee();

    int executeInstruction(const std::string &instruction);

    int printRegisters(RegisterCategory categories);

    std::shared_ptr<RegisterValue> getRegisterValue(const std::string &regName);

    pid_t getPid() const { return pid; }

    static std::shared_ptr<Tracee> createTracee();
};

#endif /* ASMASE_TRACEE_H */
