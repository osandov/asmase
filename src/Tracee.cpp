#include <algorithm>
#include <csignal>
#include <cstring>
#include <iostream>
#include <utility>

#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/wait.h>

#include "RegisterInfo.h"
#include "Tracee.h"

std::vector<std::pair<RegisterCategory, Tracee::RegisterCategoryPrinter>>
Tracee::categoryPrinters = {
    {RegisterCategory::GENERAL_PURPOSE, &Tracee::printGeneralPurposeRegisters},
    {RegisterCategory::PROGRAM_COUNTER, &Tracee::printProgramCounterRegisters},
    {RegisterCategory::CONDITION_CODE,  &Tracee::printConditionCodeRegisters},
    {RegisterCategory::FLOATING_POINT,  &Tracee::printFloatingPointRegisters},
    {RegisterCategory::EXTRA,           &Tracee::printExtraRegisters},
    {RegisterCategory::SEGMENTATION,    &Tracee::printSegmentationRegisters},
};

int Tracee::executeInstruction(const std::string &instruction)
{
    unsigned char *shared = (unsigned char *) sharedMemory;
    if (instruction.size() + trapInstruction.size() >= sharedSize) {
        std::cerr << "instruction too long\n";
        return 1;
    }

    memcpy(shared, instruction.c_str(), instruction.size());
    shared += instruction.size();
    memcpy(shared, trapInstruction.c_str(), trapInstruction.size());

    int wait_status;

    if (setProgramCounter(sharedMemory))
        return 1;

retry:
    if (ptrace(PTRACE_CONT, pid, nullptr, 0) == -1) {
        perror("ptrace");
        std::cerr << "could not continue tracee\n";
        return 1;
    }

    if (waitpid(pid, &wait_status, 0) == -1) {
        perror("waitpid");
        std::cerr << "could not wait for tracee\n";
        return 1;
    }

    if (WIFEXITED(wait_status)) {
        std::cerr << "tracee exited with status "
                  << WEXITSTATUS(wait_status) << '\n';
        return 1;
    } else if (WIFSIGNALED(wait_status)) {
        std::cerr << "tracee was terminated ("
                  << strsignal(WTERMSIG(wait_status)) << ")\n";
        return 1;
    } else if (WIFSTOPPED(wait_status)) {
        int signal = WSTOPSIG(wait_status);
        switch (signal) {
            case SIGTRAP:
                break;
            case SIGWINCH:
                // We don't want to be interrupted if the window changes size,
                // so continue the process and keep waiting
                goto retry;
            default:
                std::cout << "tracee was stopped ("
                          << strsignal(WSTOPSIG(wait_status)) << ")\n";
                return 0;
        }
    } else if (WIFCONTINUED(wait_status)) {
        std::cerr << "tracee continued\n";
        return 1;
    } else {
        std::cerr << "tracee disappeared\n";
        return 1;
    }

    return 0;
}

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

int Tracee::printGeneralPurposeRegisters()
{
    std::cerr << "no general-purpose registers on this architecture\n";
    return 1;
}

int Tracee::printConditionCodeRegisters()
{
    std::cerr << "no condition code registers on this architecture\n";
    return 1;
}

int Tracee::printProgramCounterRegisters()
{
    std::cerr << "no program counter registers on this architecture\n";
    return 1;
}

int Tracee::printSegmentationRegisters()
{
    std::cerr << "no segmentation registers on this architecture\n";
    return 1;
}

int Tracee::printFloatingPointRegisters()
{
    std::cerr << "no floating point registers on this architecture\n";
    return 1;
}

int Tracee::printExtraRegisters()
{
    std::cerr << "no extra registers on this architecture\n";
    return 1;
}

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
                std::cout << '\n';
            firstPrinted = false;

            if ((error = (this->*categoryPrinter.second)()))
                all_error = error;
        }
    }

    return all_error;
}

static void traceeProcess() __attribute__((noreturn));
static void installTracerSignalHandlers();

std::shared_ptr<Tracee> Tracee::createTracee()
{
    pid_t pid;
    void *sharedPage;
    size_t pageSize = sysconf(_SC_PAGESIZE);

    sharedPage = mmap(nullptr, pageSize, PROT_READ | PROT_WRITE | PROT_EXEC,
                      MAP_ANONYMOUS | MAP_SHARED, -1, 0);

    if (!sharedPage) {
        perror("mmap");
        std::cerr << "could not create shared memory\n";
        return {nullptr};
    }

    if ((pid = fork()) == -1) {
        perror("fork");
        std::cerr << "could not fork tracee\n";
        return {nullptr};
    }

    if (pid == 0)
        traceeProcess(); // This never returns

    installTracerSignalHandlers();

    Tracee *platformTracee = createPlatformTracee(pid, sharedPage, pageSize);
    return std::shared_ptr<Tracee>{platformTracee};
}

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

static void installTracerSignalHandlers()
{
    struct sigaction act;
    act.sa_handler = SIG_IGN;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);

    // Ignore SIGINT so the user can break out of the tracee
    sigaction(SIGINT, &act, nullptr);
}
