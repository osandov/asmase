#include <map>

#include "Builtins/AST.h"
#include "Builtins/Support.h"

namespace Builtins {

/** ValueAST member function type for applying a unaryinary operator. */
typedef ValueAST* (ValueAST::*UnaryOpFunction)(Environment &) const;

/**
 * Lookup table from unary opcodes to the corresponding ValueAST member
 * function.
 */
static std::map<UnaryOpcode, UnaryOpFunction> unaryFunctionMap = {
    {UnaryOpcode::PLUS,         &ValueAST::unaryPlus},
    {UnaryOpcode::MINUS,        &ValueAST::unaryMinus},
    {UnaryOpcode::LOGIC_NEGATE, &ValueAST::logicNegate},
    {UnaryOpcode::BIT_NEGATE,   &ValueAST::bitNegate},
};

// To evaluate a unary operator is to evaluate the operand and apply the
// operator to it.
ValueAST *UnaryOp::eval(Environment &env) const
{
    std::unique_ptr<ValueAST> value(operand->eval(env));
    if (!value)
        return nullptr;

    UnaryOpFunction func = findWithDefault(unaryFunctionMap, op, nullptr);
    assert(func);

    ValueAST *result = (*value.*func)(env);

    if (result == (ValueAST *) -1) {
        env.errorContext.printMessage("invalid argument type to unary expression",
                                      getStart(), value->getStart(), value->getEnd());
        result = nullptr;
    }

    return result;
}

}
