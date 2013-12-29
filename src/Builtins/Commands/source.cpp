#include <sstream>

#include "Builtins/AST.h"
#include "Builtins/Commands.h"
#include "Builtins/Environment.h"
#include "Builtins/ErrorContext.h"
#include "Builtins/Support.h"

#include "input.h"

static std::string getUsage(const std::string &commandName)
{
    std::stringstream ss;
    ss << "usage: " << commandName << " FILE";
    return ss.str();
}

BUILTIN_FUNC(source)
{
    if (wantsHelp(args)) {
        std::string usage = getUsage(commandName);
        printf("%s\n", usage.c_str());
        return 0;
    }

    if (args.size() != 1) {
        std::string usage = getUsage(commandName);
        env.errorContext.printMessage(usage.c_str(), commandStart);
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
