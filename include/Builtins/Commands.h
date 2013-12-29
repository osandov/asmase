#ifndef ASMASE_BUILTINS_COMMANDS_H
#define ASMASE_BUILTINS_COMMANDS_H

#include <memory>
#include <vector>

namespace Builtins {

class Environment;
class ValueAST;

}

/** Built-in command function pointer type. */
typedef int (*BuiltinFunc)(const std::vector<std::unique_ptr<Builtins::ValueAST>> &,
                           const std::string &, int, int, Builtins::Environment &);

/** Declare a built-in command function. */
#define BUILTIN_FUNC(func) int builtin_##func(\
        const std::vector<std::unique_ptr<Builtins::ValueAST>> &args, \
        const std::string &commandName, int commandStart, int commandEnd, \
        Builtins::Environment &env)

BUILTIN_FUNC(source);
BUILTIN_FUNC(memory);
BUILTIN_FUNC(registers);
BUILTIN_FUNC(warranty);
BUILTIN_FUNC(copying);

#endif /* ASMASE_BUILTINS_COMMANDS_H */
