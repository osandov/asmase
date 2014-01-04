Tracee::Tracee(const RegisterInfo &regInfo, const std::string &trapInstruction,
               UserRegisters *registers,
               pid_t pid, void *sharedMemory, size_t sharedSize)
    : regInfo(regInfo), trapInstruction(trapInstruction), registers(registers),
      pid(pid), sharedMemory(sharedMemory), sharedSize(sharedSize) {}

Tracee::~Tracee() = default;
