#include "Assembler.h"
#include "Inputter.h"
#include "Arch/X86Tracee.h"

int main(int argc, char *argv[])
{
    std::shared_ptr<Tracee> tracee{createTracee()};
    if (!tracee)
        return 1;

    Inputter inputter;

    std::shared_ptr<AssemblerContext> assemblerContext{createAssemblerContext()};
    if (!assemblerContext)
        return 1;
    Assembler assembler{assemblerContext};

    for (;;) {
        std::string line = inputter.readLine("asmase> ");
        if (line.empty()) {
            printf("\n");
            break;
        }

        std::string machineCode;
        int error = assembler.assembleInstruction(line, machineCode, inputter);
        if (error)
            continue;

        printf("%d\n", tracee->executeInstruction(machineCode));
    }

    return 0;
}
