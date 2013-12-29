#include <sstream>

#include "Builtins/AST.h"
#include "Builtins/Commands.h"
#include "Builtins/Environment.h"
#include "Builtins/ErrorContext.h"
#include "Builtins/Support.h"

#include "input.h"

BUILTIN_FUNC(source)
{
    if (args.size() != 1) {
        std::stringstream ss;
        ss << "usage: " << commandName << " FILE";
        env.errorContext.printMessage(ss.str().c_str(), commandStart);
        return 1;
    }

    if (checkValueType(*args[0], Builtins::ValueType::STRING,
                       "expected filename string", env.errorContext))
        return 1;

    const std::string &filename = args[0]->getString();

    if (redirect_input(filename.c_str()))
        return 1;

    return 0;
}
