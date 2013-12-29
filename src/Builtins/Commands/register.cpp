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

BUILTIN_FUNC(register)
{
    register_printer regPrinter;

    if (args.size() == 0) {
        regPrinter = regPrinters[""];
        assert(regPrinter);
    } else if (args.size() == 1) {
        Builtins::ValueAST *value = args[0].get();
        if (value->getType() != Builtins::ValueType::IDENTIFIER) {
            env.errorContext.printMessage("expected register category",
                                          value->getStart());
            return 1;
        }

        auto category = static_cast<const Builtins::IdentifierExpr *>(value);

        regPrinter = findWithDefault(regPrinters, category->getName(), nullptr);

        if (!regPrinter) {
            env.errorContext.printMessage("unknown register category",
                                          value->getStart());
            return 1;
        }
    } else {
        std::stringstream ss;
        ss << "usage: " << commandName << " [CATEGORY]";
        env.errorContext.printMessage(ss.str().c_str(), commandStart);
        return 1;
    }

    if (regPrinter(env.pid))
        return 1;

    return 0;
}
