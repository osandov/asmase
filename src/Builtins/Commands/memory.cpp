#include <cassert>
#include <unordered_map>

#include "Builtins/AST.h"
#include "Builtins/Commands.h"
#include "Builtins/Support.h"

#include "memory.h"

using Builtins::findWithDefault;

typedef int (*register_printer)(pid_t);

static std::unordered_map<std::string, enum mem_format> formatMap = {
    {"d", FMT_DECIMAL},
    {"u", FMT_UNSIGNED_DECIMAL},
    {"o", FMT_OCTAL},
    {"x", FMT_HEXADECIMAL},
    {"t", FMT_BINARY},
    {"f", FMT_FLOAT},
    {"c", FMT_CHARACTER},
    {"a", FMT_ADDRESS},
    {"s", FMT_STRING},
};

static std::unordered_map<std::string, size_t> sizeMap = {
    {"b", SZ_BYTE},
    {"h", SZ_HALFWORD},
    {"w", SZ_WORD},
    {"g", SZ_GIANT},
};

BUILTIN_FUNC(memory)
{
    static size_t repeat = 1;
    static enum mem_format format = FMT_HEXADECIMAL;
    static size_t size = sizeof(long);

    void *addr;

    if (args.size() < 1 || args.size() > 4) {
        // TODO: figure out how usage will work
        return 1;
    }

    // Address
    if (args[0]->getType() != Builtins::ValueType::INTEGER) {
        env.errorContext.printMessage("expected address", args[0]->getStart());
        return 1;
    }

    auto addressExpr =
        static_cast<const Builtins::IntegerExpr *>(args[0].get());

    addr = (void*) (intptr_t) addressExpr->getValue();

    // Repeat count
    if (args.size() > 1) {
        if (args[1]->getType() != Builtins::ValueType::INTEGER) {
            env.errorContext.printMessage("expected repeat count",
                                          args[1]->getStart());
            return 1;
        }

        auto repeatExpr =
            static_cast<const Builtins::IntegerExpr *>(args[1].get());

        repeat = repeatExpr->getValue();
    }

    // Format
    if (args.size() > 2) {
        if (args[2]->getType() != Builtins::ValueType::IDENTIFIER) {
            env.errorContext.printMessage("expected format specifier",
                                          args[2]->getStart());
            return 1;
        }

        auto formatExpr =
            static_cast<const Builtins::IdentifierExpr *>(args[2].get());
        const std::string &formatStr = formatExpr->getName();

        if (!formatMap.count(formatStr)) {
            env.errorContext.printMessage("invalid format specifier",
                                          formatExpr->getStart());
            return 1;
        }

        format = formatMap[formatStr];

        // Special-case hacks
        if (formatStr == "c")
            size = sizeof(char);
        else if (formatStr == "a")
            size = sizeof(void*);
    }

    // Size
    if (args.size() > 3) {
        if (args[3]->getType() != Builtins::ValueType::IDENTIFIER) {
            env.errorContext.printMessage("expected size specifier",
                                          args[3]->getStart());
            return 1;
        }

        // Sanity check
        if (format == FMT_STRING) {
            env.errorContext.printMessage("size is invalid with string format",
                                          args[3]->getStart());
            return 1;
        }

        auto sizeExpr =
            static_cast<const Builtins::IdentifierExpr *>(args[3].get());
        const std::string &sizeStr = sizeExpr->getName();

        if (!sizeMap.count(sizeStr)) {
            env.errorContext.printMessage("invalid size specifier",
                                          sizeExpr->getStart());
            return 1;
        }

        size = sizeMap[sizeStr];

    }

    // Sanity checks
    if (format == FMT_FLOAT && size != SZ_WORD && size != SZ_GIANT) {
        env.errorContext.printMessage("invalid size for float");
        return 1;
    }

    if (format == FMT_CHARACTER && size != SZ_BYTE) {
        env.errorContext.printMessage("invalid size for character");
        return 1;
    }

    if (format == FMT_ADDRESS && size != sizeof(void*)) {
        env.errorContext.printMessage("invalid size for address");
        return 1;
    }

    if (format == FMT_STRING) {
        if (dump_strings(env.pid, addr, repeat))
            return 1;
    } else {
        if (dump_memory(env.pid, addr, repeat, format, size))
            return 1;
    }

    return 0;
}
