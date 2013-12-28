#include "Builtins/AST.h"

namespace Builtins {

// To evaluate a variable is to get its value from the environment.
ValueAST *VariableExpr::eval(Environment &env) const
{
    std::string errorMsg;
    ValueAST *result = env.lookupVariable(name, errorMsg);
    
    if (result) {
        // Set the bounds of the value to the bounds of the original expression
        result->setStart(getStart());
        result->setEnd(getEnd());
    } else
        env.errorContext.printMessage(errorMsg.c_str(), getStart());

    return result;
}

}
