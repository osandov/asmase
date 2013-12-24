#ifndef ASMASE_BUILTIN_ENVIRONMENT_H
#define ASMASE_BUILTIN_ENVIRONMENT_H

class ErrorContext;

class Environment {
public:
    ErrorContext &errorContext;
    Environment(ErrorContext &errorContext) : errorContext(errorContext) {}
};

#endif /* ASMASE_BUILTIN_ENVIRONMENT_H */
