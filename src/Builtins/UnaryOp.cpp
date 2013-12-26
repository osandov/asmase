#include <map>

#include "Builtins/AST.h"
#include "Builtins/Support.h"

namespace Builtins {

typedef ValueAST* (ValueAST::*UnaryOpFunction)(Environment &) const;
static std::map<UnaryOpcode, UnaryOpFunction> unaryFunctionMap = {
    {UnaryOpcode::PLUS,         &ValueAST::unaryPlus},
    {UnaryOpcode::MINUS,        &ValueAST::unaryMinus},
    {UnaryOpcode::LOGIC_NEGATE, &ValueAST::logicNegate},
    {UnaryOpcode::BIT_NEGATE,   &ValueAST::bitNegate},
};

ValueAST *UnaryOp::eval(Environment &env) const
{
    ValueAST *value = operand->eval(env);
    if (!value)
        return NULL;

    UnaryOpFunction func = findWithDefault(unaryFunctionMap, op,
                                           (UnaryOpFunction) NULL);
    assert(func);

    ValueAST *result = (value->*func)(env);

    if (result == (ValueAST *) -1) {
        env.errorContext.printMessage("invalid argument type to unary expression",
                                      getStart(), value->getStart(), value->getEnd());
        result = NULL;
    }

    delete value;
    return result;
}

}
