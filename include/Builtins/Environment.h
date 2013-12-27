#ifndef ASMASE_BUILTINS_ENVIRONMENT_H
#define ASMASE_BUILTINS_ENVIRONMENT_H

#include <string>
#include <sys/types.h>

struct assembler;
struct tracee_info;

namespace Builtins {

class ErrorContext;
class ValueAST;

/** The environement in which to evaluate and run built-in commands. */
class Environment {
public:
    /** Assembler context. */
    struct assembler *asmb;

    /** Tracee PID. */
    pid_t pid;

    /** Page shared with the tracee. */
    void *shared_page;

    /** Error context for the input being run. */
    ErrorContext &errorContext;

    Environment(struct assembler *asmb, struct tracee_info *tracee,
                ErrorContext &errorContext);

    /**
     * Look up a variable in the environment.
     * @return The value of the variable on success, null pointer on failure
     * and errorMsg is set.
     */
    ValueAST *lookupVariable(const std::string &var, std::string &errorMsg);
};

}

#endif /* ASMASE_BUILTINS_ENVIRONMENT_H */
