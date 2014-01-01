#include "Assembler.h"
#include "Builtins.h"
#include "Inputter.h"
#include "Arch/X86Tracee.h"

int main(int argc, char *argv[])
{
    printf("asmase Copyright (C) 2013 Omar Sandoval\n"
           "This program comes with ABSOLUTELY NO WARRANTY; for details type `:warranty'.\n"
           "This is free software, and you are welcome to redistribute it\n"
           "under certain conditions; type `:copying' for details.\n");

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

        if (isBuiltin(line)) {
            int result = runBuiltin(line, *tracee, inputter);
            if (result < 0)
                break;
            else
                printf("%d\n", result);
        } else {
            std::string machineCode;
            int error = assembler.assembleInstruction(line, machineCode, inputter);
            if (error)
                continue;
            printf("%d\n", tracee->executeInstruction(machineCode));
        }
    }

    return 0;
}
