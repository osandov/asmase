#ifndef ASMASE_BUILTINS_COMMANDS_H
#define ASMASE_BUILTINS_COMMANDS_H

#include <vector>

namespace Builtins {

class Environment;
class ValueAST;

}

typedef int (*BuiltinFunc)(const std::vector<Builtins::ValueAST*> &, Builtins::Environment &);

#define BUILTIN_FUNC(func) int builtin_##func(\
        const std::vector<Builtins::ValueAST*> &args, Builtins::Environment &env)

BUILTIN_FUNC(source);
BUILTIN_FUNC(memory);
BUILTIN_FUNC(register);
BUILTIN_FUNC(warranty);
BUILTIN_FUNC(copying);

#endif /* ASMASE_BUILTINS_COMMANDS_H */
