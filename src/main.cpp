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

#include <cstdio>
#include <cstdlib>
#include <getopt.h>
#include <iomanip>

#include "Assembler.h"
#include "Builtins.h"
#include "Inputter.h"
#include "Support.h"
#include "Tracee.h"

static const char *progname;

void usage(bool error)
{
    fprintf(error ? stderr : stdout, "Usage: %s [-hv]\n", progname);
}

void version()
{
    printf(
        "asmase %s Copyright (C) 2013-2014 Omar Sandoval\n"
        "This program comes with ABSOLUTELY NO WARRANTY; for details type `:warranty'.\n"
        "This is free software, and you are welcome to redistribute it\n"
        "under certain conditions; type `:copying' for details.\n",
	ASMASE_VERSION);
}

int main(int argc, char *argv[])
{
    int c;

    static struct option long_options[] = {
        {"version", no_argument, nullptr, 'v'},
        {"help",    no_argument, nullptr, 'h'},
    };

    progname = argv[0];

    for (;;) {
        c = getopt_long(argc, argv, "vh", long_options, nullptr);
        if (c == -1)
            break;

        switch (c) {
        case 'v':
            version();
            return 0;
        case 'h':
            printf("asmase assembly REPL %s\n\n", ASMASE_VERSION);
            usage(false);
            printf("\n");
            printf("For more information, type `:help` from within asmase, or consult the README.\n");
            return 0;
        case '?':
        default:
            usage(true);
            return 2;
        }
    }

    version();

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
            printf("\n");
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

            printf("%s = ", line.c_str());
            tracee->printInstruction(machineCode);
            printf("\n");

            error = tracee->executeInstruction(machineCode);
            if (error < 0)
                break;
        }
    }

    return 0;
}
