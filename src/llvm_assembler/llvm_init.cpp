/*
 * Copyright (C) 2013 Omar Sandoval
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
using namespace llvm;

#include "assembler.h"
#include "llvm_assembler.h"

static const char *ProgName = "asmase";

static cl::opt<std::string>
ArchName("arch", cl::desc("Target arch to assemble for, "
                          "see -version for available targets"));

static cl::opt<std::string>
TripleName("triple", cl::desc("Target triple to assemble for, "
                              "see -version for available targets"));

static cl::opt<std::string>
MCPU("mcpu",
     cl::desc("Target a specific cpu type (-mcpu=help for details)"),
     cl::value_desc("cpu-name"),
     cl::init(""));

static cl::list<std::string>
MAttrs("mattr",
  cl::CommaSeparated,
  cl::desc("Target specific attributes (-mattr=help for details)"),
  cl::value_desc("a1,+a2,-a3,..."));

Parameters::Parameters(const char *_PN,
                       const std::string &_AN,
                       const std::string &_TN,
                       const cl::list<std::string> &_MA,
                       const std::string &_MCPU)
        : ProgName(_PN), ArchName(_AN), TripleName(_TN), MAttrs(_MA),
          MCPU(_MCPU)
{
}

Parameters GetParameters()
{
    return Parameters(ProgName, ArchName, TripleName, MAttrs, MCPU);
}

extern "C" {

/* See assembler.h. */
int init_assemblers(int argc, char **argv)
{
    // Print a stack trace if we signal out.
    sys::PrintStackTraceOnErrorSignal();
    PrettyStackTraceProgram X(argc, argv);

    // Initialize targets and assembly printers/parsers.
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllDisassemblers();

    // Register the target printer for --version.
    cl::AddExtraVersionPrinter(TargetRegistry::printRegisteredTargetsForVersion);

    cl::ParseCommandLineOptions(argc, argv, "llvm-powered assembly interpreter\n");
    TripleName = Triple::normalize(TripleName);

    return 0;
}

/* See assembler.h. */
void shutdown_assemblers()
{
    llvm_shutdown();
}

}
