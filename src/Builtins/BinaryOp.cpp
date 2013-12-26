#include "Builtins/AST.h"
#include "Builtins/Init.h"

namespace Builtins {

ValueAST *BinaryOp::eval(Environment &env) const
{
    ValueAST *left = lhs->eval(env);
    ValueAST *right = rhs->eval(env);
    if (!left || !right)
        return NULL;

    BinaryOpFunction func = findWithDefault(binaryFunctionMap, op,
                                            (BinaryOpFunction) NULL);
    assert(func);

    ValueAST *result = (left->*func)(right, env);

    if (result == (ValueAST *) -1) {
        env.errorContext.printMessage("invalid operands to binary expression",
                                      getStart(), left->getStart(), left->getEnd(),
                                      right->getStart(), right->getEnd());
        result = NULL;
    }

    delete left;
    delete right;
    return result;
}

}
