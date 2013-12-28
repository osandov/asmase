#include <cassert>

#include "Builtins/AST.h"
#include "Builtins/Environment.h"

#include "assembler.h"
#include "tracing.h"

namespace Builtins {

Environment::Environment(struct assembler *asmb, struct tracee_info *tracee,
                         ErrorContext &errorContext)
    : asmb(asmb), pid(tracee->pid), shared_page(tracee->shared_page),
        errorContext(errorContext) {}

// Currently, the only variables are registers.
ValueAST *Environment::lookupVariable(const std::string &var,
                                      std::string &errorMsg)
{
    struct register_value regval;
    if (get_register_value(pid, var.c_str() + 1, &regval)) {
        errorMsg = "unknown variable";
        return nullptr;
    }

    switch (regval.type) {
        case REGISTER_INTEGER:
            return new IntegerExpr(0, 0, regval.integer);
        case REGISTER_FLOATING:
            return new FloatExpr(0, 0, regval.integer);
    }

    assert(false);
    return nullptr;
}

}
