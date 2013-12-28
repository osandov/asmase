#include "Builtins/AST.h"

namespace Builtins {

// To evaluate a parenthetical expression is simply to evaluate the enclosed
// expression
ValueAST *ParenExpr::eval(Environment &env) const
{
    ValueAST *value = expr->eval(env);
    if (!value)
        return nullptr;

    value->setStart(getStart());
    value->setEnd(getEnd());
    return value;
}

}
