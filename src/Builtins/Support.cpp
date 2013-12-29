#include "Builtins/AST.h"
#include "Builtins/ErrorContext.h"
#include "Builtins/Support.h"

namespace Builtins {

bool checkValueType(const ValueAST &value, ValueType type,
                    const char *errorMsg, ErrorContext &errorContext)
{
    if (value.getType() != type) {
        errorContext.printMessage(errorMsg, value.getStart());
        return true;
    } else
        return false;
}

bool wantsHelp(const std::vector<std::unique_ptr<ValueAST>> &args)
{
    return args.size() >= 1 && args[0]->getType() == ValueType::IDENTIFIER &&
           args[0]->getIdentifier() == "help";
}

}
