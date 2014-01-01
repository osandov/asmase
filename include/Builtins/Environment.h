#ifndef ASMASE_BUILTINS_ENVIRONMENT_H
#define ASMASE_BUILTINS_ENVIRONMENT_H

#include <string>
#include <sys/types.h>

class Tracee;

namespace Builtins {

class ErrorContext;
class ValueAST;

/** The environement in which to evaluate and run built-in commands. */
class Environment {
public:
    Tracee &tracee;

    /** Error context for the input being run. */
    ErrorContext &errorContext;

    Environment(Tracee &tracee, ErrorContext &errorContext)
        : tracee(tracee), errorContext(errorContext) {}

    /**
     * Look up a variable in the environment.
     * @return The value of the variable on success, null pointer on failure
     * and errorMsg is set.
     */
    ValueAST *lookupVariable(const std::string &var, std::string &errorMsg);
};

}

#endif /* ASMASE_BUILTINS_ENVIRONMENT_H */
