#include <sstream>

#include "Builtins/AST.h"
#include "Builtins/Environment.h"
#include "Builtins/ErrorContext.h"
#include "Builtins/Commands.h"

#include "input.h"

BUILTIN_FUNC(source)
{
    if (args.size() != 1) {
        std::stringstream ss;
        ss << "usage: " << commandName << " FILE";
        env.errorContext.printMessage(ss.str().c_str(), commandStart);
        return 1;
    }

    if (args[0]->getType() != Builtins::ValueType::STRING) {
        env.errorContext.printMessage("expected filename string",
                                      args[0]->getStart());
        return 1;
    }

    auto filename = static_cast<const Builtins::StringExpr *>(args[0].get());

    if (redirect_input(filename->getStr().c_str()))
        return 1;

    return 0;
}
