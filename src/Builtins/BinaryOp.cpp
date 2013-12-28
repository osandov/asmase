#include <map>

#include "Builtins/AST.h"
#include "Builtins/Support.h"

namespace Builtins {

/** ValueAST member function type for applying a binary operator. */
typedef ValueAST* (ValueAST::*BinaryOpFunction)(const ValueAST *, Environment &) const;

/**
 * Lookup table from binary opcodes to the corresponding ValueAST member
 * function.
 */
static std::map<BinaryOpcode, BinaryOpFunction> binaryFunctionMap = {
    {BinaryOpcode::ADD,      &ValueAST::add},
    {BinaryOpcode::SUBTRACT, &ValueAST::subtract},
    {BinaryOpcode::MULTIPLY, &ValueAST::multiply},
    {BinaryOpcode::DIVIDE,   &ValueAST::divide},
    {BinaryOpcode::MOD,      &ValueAST::mod},

    {BinaryOpcode::EQUALS,                 &ValueAST::equals},
    {BinaryOpcode::NOT_EQUALS,             &ValueAST::notEquals},
    {BinaryOpcode::GREATER_THAN,           &ValueAST::greaterThan},
    {BinaryOpcode::LESS_THAN,              &ValueAST::lessThan},
    {BinaryOpcode::GREATER_THAN_OR_EQUALS, &ValueAST::greaterThanOrEquals},
    {BinaryOpcode::LESS_THAN_OR_EQUALS,    &ValueAST::lessThanOrEquals},

    {BinaryOpcode::LOGIC_AND,   &ValueAST::logicAnd},
    {BinaryOpcode::LOGIC_OR,    &ValueAST::logicOr},
    {BinaryOpcode::BIT_AND,     &ValueAST::bitAnd},
    {BinaryOpcode::BIT_OR,      &ValueAST::bitOr},
    {BinaryOpcode::BIT_XOR,     &ValueAST::bitXor},
    {BinaryOpcode::LEFT_SHIFT,  &ValueAST::leftShift},
    {BinaryOpcode::RIGHT_SHIFT, &ValueAST::rightShift},
};

// To evaluate a binary operation is to evaluate the two operands and apply the
// operator to them.
ValueAST *BinaryOp::eval(Environment &env) const
{
    std::unique_ptr<ValueAST> left(lhs->eval(env));
    std::unique_ptr<ValueAST> right(rhs->eval(env));
    if (!left || !right)
        return nullptr;

    BinaryOpFunction func = findWithDefault(binaryFunctionMap, op, nullptr);
    assert(func);

    ValueAST *result = (*left.*func)(right.get(), env);

    if (result == (ValueAST *) -1) {
        env.errorContext.printMessage("invalid operands to binary expression",
                                      getStart(), left->getStart(), left->getEnd(),
                                      right->getStart(), right->getEnd());
        result = nullptr;
    }

    // No need to adjust bounds; the implementation function returns an
    // expression with the LHS's start and RHS's end.

    return result;
}

}
