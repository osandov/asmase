/*
 * Copyright (C) 2013 Omar Sandoval
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <cassert>
#include <sstream>
#include <unordered_map>

#include "Builtins/AST.h"
#include "Builtins/Commands.h"
#include "Builtins/Environment.h"
#include "Builtins/ErrorContext.h"
#include "Builtins/Support.h"

#include "memory.h"

/** Lookup table from format specifier to format constant. */
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

/** Lookup table from size specifier to represented size. */
static std::unordered_map<std::string, size_t> sizeMap = {
    {"b", SZ_BYTE},
    {"h", SZ_HALFWORD},
    {"w", SZ_WORD},
    {"g", SZ_GIANT},
};

static std::string getUsage(const std::string &commandName)
{
    std::stringstream ss;
    ss << "usage: " << commandName << " ";
    ss << "ADDR [REPEAT] [FORMAT] [SIZE]";
    return ss.str();
}

BUILTIN_FUNC(memory)
{
    static size_t repeat = 1;
    static enum mem_format format = FMT_HEXADECIMAL;
    static size_t size = sizeof(long);

    void *addr;

    if (wantsHelp(args)) {
        std::string usage = getUsage(commandName);
        printf("%s\n", usage.c_str());
        printf("Categories:\n"
               "  gp  -- General purpose registers\n"
               "  cc  -- Condition code/status flag registers\n"
               "  fp  -- Floating point registers\n"
               "  x   -- Extra registers\n"
               "  seg -- Segment registers\n");
        return 0;
    }

    if (args.size() < 1 || args.size() > 4) {
        std::string usage = getUsage(commandName);
        env.errorContext.printMessage(usage.c_str(), commandStart);
        return 1;
    }

    // Address
    if (checkValueType(*args[0], Builtins::ValueType::INTEGER,
                       "expected address", env.errorContext))
        return 1;

    addr = (void *) args[0]->getInteger();

    // Repeat count
    if (args.size() > 1) {
        if (checkValueType(*args[1], Builtins::ValueType::INTEGER,
                           "expected repeat count", env.errorContext))
            return 1;

        repeat = args[1]->getInteger();
    }

    // Format
    if (args.size() > 2) {
        if (checkValueType(*args[2], Builtins::ValueType::IDENTIFIER,
                           "expected format specifier", env.errorContext))
            return 1;

        const std::string &formatStr = args[2]->getIdentifier();

        if (!formatMap.count(formatStr)) {
            env.errorContext.printMessage("invalid format specifier",
                                          args[2]->getStart());
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
        if (checkValueType(*args[3], Builtins::ValueType::IDENTIFIER,
                           "expected size specifier", env.errorContext))
            return 1;

        // Sanity check
        if (format == FMT_STRING) {
            env.errorContext.printMessage("size is invalid with string format",
                                          args[3]->getStart());
            return 1;
        }

        const std::string &sizeStr = args[3]->getIdentifier();

        if (!sizeMap.count(sizeStr)) {
            env.errorContext.printMessage("invalid size specifier",
                                          args[3]->getStart());
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
