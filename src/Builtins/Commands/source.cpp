#include "Builtins/AST.h"
#include "Builtins/Commands.h"

#include "input.h"

BUILTIN_FUNC(source)
{
    if (args.size() != 1) {
        // TODO: figure out how usage will work
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
