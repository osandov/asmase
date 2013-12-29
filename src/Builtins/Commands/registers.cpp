#include <cassert>
#include <sstream>
#include <unordered_map>

#include "Builtins/AST.h"
#include "Builtins/Commands.h"
#include "Builtins/Environment.h"
#include "Builtins/ErrorContext.h"
#include "Builtins/Support.h"

#include "tracing.h"

using Builtins::findWithDefault;

/** Register printer function type. */
typedef int (*register_printer)(pid_t);

/** Lookup table from register category to register printer. */
static std::unordered_map<std::string, register_printer> regPrinters = {
    {"", &print_registers},

    {"general-purpose", print_general_purpose_registers},
    {"general",         print_general_purpose_registers},
    {"gp",              print_general_purpose_registers},
    {"g",               print_general_purpose_registers},

    {"condition-codes", print_condition_code_registers},
    {"condition",       print_condition_code_registers},
    {"status",          print_condition_code_registers},
    {"flags",           print_condition_code_registers},
    {"cc",              print_condition_code_registers},

    {"floating-point", print_floating_point_registers},
    {"floating",       print_floating_point_registers},
    {"fp",             print_floating_point_registers},
    {"f",              print_floating_point_registers},

    {"extra", print_extra_registers},
    {"xr",    print_extra_registers},
    {"x",     print_extra_registers},

    {"segment", print_segment_registers},
    {"seg",     print_segment_registers},
    {"s",       print_segment_registers},
};

static std::string getUsage(const std::string &commandName)
{
    std::stringstream ss;
    ss << "usage: " << commandName << " [CATEGORY]";
    return ss.str();
}

BUILTIN_FUNC(registers)
{
    register_printer regPrinter;

    if (wantsHelp(args)) {
        std::string usage = getUsage(commandName);
        printf("%s\n", usage.c_str());
        printf("Formats:\n"
               "  d -- decimal\n"
               "  u -- unsigned decimal\n"
               "  o -- unsigned octal\n"
               "  x -- unsigned hexadecimal\n"
               "  t -- unsigned binary\n"
               "  f -- floating point\n"
               "  c -- character\n");
        printf("Sizes:\n"
               "  b -- byte (1 byte)\n"
               "  h -- half word (2 bytes)\n"
               "  w -- word (4 bytes)\n"
               "  g -- giant (8 bytes)\n");
        return 0;
    }

    if (args.size() == 0) {
        regPrinter = regPrinters[""];
        assert(regPrinter);
    } else if (args.size() == 1) {
        if (checkValueType(*args[0], Builtins::ValueType::IDENTIFIER,
                           "expected register category", env.errorContext))
            return 1;

        const std::string &category = args[0]->getIdentifier();

        regPrinter = findWithDefault(regPrinters, category, nullptr);

        if (!regPrinter) {
            env.errorContext.printMessage("unknown register category",
                                          args[0]->getStart());
            return 1;
        }
    } else {
        std::string usage = getUsage(commandName);
        env.errorContext.printMessage(usage.c_str(), commandStart);
        return 1;
    }

    if (regPrinter(env.pid))
        return 1;

    return 0;
}
