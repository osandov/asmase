#include "Builtins/AST.h"
#include "Builtins/Init.h"

namespace Builtins {

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
