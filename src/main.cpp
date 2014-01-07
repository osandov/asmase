/*
 * Driver for asmase.
 *
 * Copyright (C) 2013-2014 Omar Sandoval
 *
 * This file is part of asmase.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <iomanip>
#include <iostream>

#include "Assembler.h"
#include "Builtins.h"
#include "Inputter.h"
#include "Tracee.h"

int main(int argc, char *argv[])
{
    std::cout <<
        "asmase Copyright (C) 2013-2014 Omar Sandoval\n"
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

        line.resize(line.size() - 1); // Trim off the newline

        if (isBuiltin(line)) {
            if (runBuiltin(line, *tracee, inputter) < 0)
                break;
        } else {
            bytestring machineCode;

            int error = assembler.assembleInstruction(line, machineCode, inputter);
            if (error || machineCode.empty())
                continue;

            std::cout << line << " = [";
            for (size_t i = 0; i < machineCode.size(); ++i) {
                if (i > 0)
                    std::cout << ", ";
                std::cout << "0x" << std::hex << std::setw(2)
                          << std::setfill('0') << (int) machineCode[i];
            }
            std::cout << "]\n";

            error = tracee->executeInstruction(machineCode);
            if (error < 0)
                break;
        }
    }

    return 0;
}
