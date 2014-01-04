#ifndef ASMASE_BUILTINS_ENVIRONMENT_H
#define ASMASE_BUILTINS_ENVIRONMENT_H

#include <string>
#include <sys/types.h>

class Tracee;
class Inputter;

namespace Builtins {

class ErrorContext;
class ValueAST;

/** The environement in which to evaluate and run built-in commands. */
class Environment {
public:
    Tracee &tracee;
    Inputter &inputter;

    /** Error context for the input being run. */
    ErrorContext &errorContext;

    Environment(Tracee &tracee, Inputter &inputter, ErrorContext &errorContext)
        : tracee(tracee), inputter(inputter), errorContext(errorContext) {}

    /**
     * Look up a variable in the environment.
     * @return The value of the variable on success, null pointer on failure
     * and errorMsg is set.
     */
    ValueAST *lookupVariable(const std::string &var, std::string &errorMsg);
};

}

#endif /* ASMASE_BUILTINS_ENVIRONMENT_H */
