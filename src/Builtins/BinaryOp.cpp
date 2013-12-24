#include "Builtins/AST.h"

namespace Builtins {

const ValueAST *BinaryOp::eval(Environment &env) const
{
    const ValueAST *left = lhs->eval(env);
    const ValueAST *right = rhs->eval(env);
    if (!left || !right)
        return NULL;

    const ValueAST *result;
    switch (op) {
        case BinaryOp::ADD:
            result = left->add(right, env);
            break;
        case BinaryOp::SUBTRACT:
            result = left->subtract(right, env);
            break;
        case BinaryOp::MULTIPLY:
            result = left->multiply(right, env);
            break;
        case BinaryOp::DIVIDE:
            result = left->divide(right, env);
            break;
        case BinaryOp::MOD:
            result = left->mod(right, env);
            break;
        case BinaryOp::EQUALS:
            result = left->equals(right, env);
            break;
        case BinaryOp::NOT_EQUALS:
            result = left->notEquals(right, env);
            break;
        case BinaryOp::GREATER_THAN:
            result = left->greaterThan(right, env);
            break;
        case BinaryOp::LESS_THAN:
            result = left->lessThan(right, env);
            break;
        case BinaryOp::GREATER_THAN_OR_EQUALS:
            result = left->greaterThanOrEquals(right, env);
            break;
        case BinaryOp::LESS_THAN_OR_EQUALS:
            result = left->lessThanOrEquals(right, env);
            break;
        case BinaryOp::LOGIC_AND:
            result = left->logicAnd(right, env);
            break;
        case BinaryOp::LOGIC_OR:
            result = left->logicOr(right, env);
            break;
        case BinaryOp::BIT_AND:
            result = left->bitAnd(right, env);
            break;
        case BinaryOp::BIT_OR:
            result = left->bitOr(right, env);
            break;
        case BinaryOp::BIT_XOR:
            result = left->bitXor(right, env);
            break;
        case BinaryOp::LEFT_SHIFT:
            result = left->leftShift(right, env);
            break;
        case BinaryOp::RIGHT_SHIFT:
            result = left->rightShift(right, env);
            break;
        case BinaryOp::NONE:
            assert(false);
            return NULL;
    }

    if (result == (ValueAST *) -1) {
        env.errorContext.printMessage("invalid operands to binary expression",
                                      getStart(), left->getStart(), left->getEnd(),
                                      right->getStart(), right->getEnd());
        return NULL;
    } else
        return result;
}

}
