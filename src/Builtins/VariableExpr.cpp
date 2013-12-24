#include "Builtins/AST.h"

namespace Builtins {

ValueAST *VariableExpr::eval(Environment &env) const
{
    env.errorContext.printMessage("not implemented", getStart());
    return NULL;
}

}
