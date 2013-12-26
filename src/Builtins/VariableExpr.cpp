#include "Builtins/AST.h"

namespace Builtins {

ValueAST *VariableExpr::eval(Environment &env) const
{
    std::string errorMsg;
    ValueAST *result = env.lookupVariable(name, errorMsg);
    if (!result)
        env.errorContext.printMessage(errorMsg.c_str(), getStart());
    return result;
}

}
