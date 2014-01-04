#include <iostream>

#include "Assembler.h"
#include "Builtins.h"
#include "Inputter.h"
#include "Tracee.h"

int main(int argc, char *argv[])
{
    std::cout <<
        "asmase Copyright (C) 2013 Omar Sandoval\n"
        "This program comes with ABSOLUTELY NO WARRANTY; for details type `:warranty'.\n"
        "This is free software, and you are welcome to redistribute it\n"
        "under certain conditions; type `:copying' for details.\n";

    std::shared_ptr<Tracee> tracee{Tracee::createTracee()};
    if (!tracee)
        return 1;

    Inputter inputter;

    std::shared_ptr<AssemblerContext>
        assemblerContext{Assembler::createAssemblerContext()};
    if (!assemblerContext)
        return 1;
    Assembler assembler{assemblerContext};

    for (;;) {
        std::string line = inputter.readLine("asmase> ");
        if (line.empty()) {
            std::cout << std::endl;
            break;
        }

        if (isBuiltin(line)) {
            if (runBuiltin(line, *tracee, inputter) < 0)
                break;
        } else {
            std::string machineCode;
            int error = assembler.assembleInstruction(line, machineCode, inputter);
            if (error)
                continue;
            tracee->executeInstruction(machineCode);
        }
    }

    return 0;
}
