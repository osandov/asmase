#include "Builtins/AST.h"

namespace Builtins {

const ValueAST *UnaryOp::eval(Environment &env) const
{
    const ValueAST *value = operand->eval(env);
    if (!value)
        return NULL;

    const ValueAST *result;
    switch (op) {
        case UnaryOp::PLUS:
            result = value->unaryPlus(env);
            break;
        case UnaryOp::MINUS:
            result = value->unaryMinus(env);
            break;
        case UnaryOp::LOGIC_NEGATE:
            result = value->logicNegate(env);
            break;
        case UnaryOp::BIT_NEGATE:
            result = value->bitNegate(env);
            break;
        case UnaryOp::NONE:
            assert(false);
            return NULL;
    }

    if (result == (ValueAST *) -1) {
        env.errorContext.printMessage("invalid argument type to unary expression",
                                      getStart(), value->getStart(), value->getEnd());
        return NULL;
    } else
        return result;
}

}
