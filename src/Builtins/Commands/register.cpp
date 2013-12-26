#include <cassert>
#include <unordered_map>

#include "Builtins/AST.h"
#include "Builtins/Commands.h"
#include "Builtins/Init.h"

#include "tracing.h"

using Builtins::findWithDefault;

typedef int (*register_printer)(pid_t);

std::unordered_map<std::string, register_printer> regPrinters;

BUILTIN_INIT_FUNC(register)
{
    regPrinters[""] = &print_registers;

    regPrinters["general-purpose"] =
    regPrinters["general"]         =
    regPrinters["gp"]              =
    regPrinters["g"]               = print_general_purpose_registers;

    regPrinters["condition-codes"] =
    regPrinters["condition"]       =
    regPrinters["status"]          =
    regPrinters["flags"]           =
    regPrinters["cc"]              = print_condition_code_registers;

    regPrinters["floating-point"]  =
    regPrinters["floating"]        =
    regPrinters["fp"]              =
    regPrinters["f"]               = print_floating_point_registers;

    regPrinters["extra"]           =
    regPrinters["xr"]              =
    regPrinters["x"]               = print_extra_registers;

    regPrinters["segment"]         =
    regPrinters["seg"]             =
    regPrinters["s"]               = print_segment_registers;

    return 0;
}

BUILTIN_FUNC(register)
{
    register_printer regPrinter;

    if (args.size() == 0) {
        regPrinter = regPrinters[""];
        assert(regPrinter);
    } else if (args.size() == 1) {
        Builtins::ValueAST *value = args[0];
        if (value->getType() != Builtins::ValueType::IDENTIFIER) {
            env.errorContext.printMessage("expected register category",
                                          value->getStart());
            return 1;
        }

        const Builtins::IdentifierExpr *category =
            static_cast<const Builtins::IdentifierExpr*>(value);

        regPrinter = findWithDefault(regPrinters, category->getName(),
                                     (register_printer) NULL);

        if (!regPrinter) {
            env.errorContext.printMessage("unknown register category",
                                          value->getStart());
            return 1;
        }
    } else {
        return 1;
    }

    if (regPrinter(env.pid))
        return 1;

    return 0;
}
