#ifndef ASMASE_BUILTIN_ENVIRONMENT_H
#define ASMASE_BUILTIN_ENVIRONMENT_H

#include <string>
#include <sys/types.h>

struct assembler;
struct tracee_info;

namespace Builtins {

class ErrorContext;
class ValueAST;

class Environment {
public:
    struct assembler *asmb;
    pid_t pid;
    void *shared_page;
    ErrorContext &errorContext;

    Environment(struct assembler *asmb, struct tracee_info *tracee,
                ErrorContext &errorContext);

    ValueAST *lookupVariable(const std::string &var, std::string &errorMsg);
};

}

#endif /* ASMASE_BUILTIN_ENVIRONMENT_H */
