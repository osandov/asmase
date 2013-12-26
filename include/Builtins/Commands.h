#ifndef ASMASE_BUILTINS_COMMANDS_H
#define ASMASE_BUILTINS_COMMANDS_H

#include <vector>

namespace Builtins {

class Environment;
class ValueAST;

}

typedef int (*BuiltinFunc)(const std::vector<Builtins::ValueAST*> &, Builtins::Environment &);
typedef int (*BuiltinInit)();

struct BuiltinCommand {
    BuiltinFunc func;
};

#define BUILTIN_FUNC(func) int builtin_##func(\
        const std::vector<Builtins::ValueAST*> &args, Builtins::Environment &env)

#define BUILTIN_INIT_FUNC(func) int builtin_init_##func()

BUILTIN_FUNC(register);
BUILTIN_INIT_FUNC(register);

#endif /* ASMASE_BUILTINS_COMMANDS_H */
