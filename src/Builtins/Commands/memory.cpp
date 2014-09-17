/*
 * memory built-in command for inspecting tracee memory.
 *
 * Copyright (C) 2013-2014 Omar Sandoval
 *
 * This file is part of asmase.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <cinttypes>
#include <sstream>
#include <unordered_map>
#include <utility>

#include "Builtins/AST.h"
#include "Builtins/Commands.h"
#include "Builtins/Environment.h"
#include "Builtins/ErrorContext.h"
#include "Builtins/Support.h"

#include "MemoryStreamer.h"
#include "Support.h"
#include "Tracee.h"

using Builtins::escapeCharacter;

enum class Format {
    DECIMAL,
    UNSIGNED_DECIMAL,
    OCTAL,
    HEXADECIMAL,
    BINARY,
    FLOAT,
    CHARACTER,
    ADDRESS,
    STRING,
};

enum Size : size_t { // Intentionally weakly-typed enum
    BYTE = 1,
    HALFWORD = 2,
    WORD = 4,
    GIANT = 8,
};

/** Lookup table from format specifier to format constant. */
static std::unordered_map<std::string, Format> formatMap = {
    {"d", Format::DECIMAL},
    {"u", Format::UNSIGNED_DECIMAL},
    {"o", Format::OCTAL},
    {"x", Format::HEXADECIMAL},
    {"t", Format::BINARY},
    {"f", Format::FLOAT},
    {"a", Format::ADDRESS},
    {"c", Format::CHARACTER},
    {"s", Format::STRING},
};

/** Lookup table from size specifier to represented size. */
static std::unordered_map<std::string, size_t> sizeMap = {
    {"b", Size::BYTE},
    {"h", Size::HALFWORD},
    {"w", Size::WORD},
    {"g", Size::GIANT},
};

static std::string getUsage(const std::string &commandName)
{
    std::stringstream ss;
    ss << "usage: " << commandName << " ";
    ss << "[ADDR] [REPEAT] [FORMAT] [SIZE]";
    return ss.str();
}

template <typename T, typename U = T, typename Printer>
static int dumpMemoryWith(MemoryStreamer &memStr, size_t repeat,
                          int numColumns, Printer printer)
{
    T value;

    int column = numColumns;
    bool firstRow = true;
    for (size_t i = 0; i < repeat; ++i, ++column) {
        if (column >= numColumns) {
            if (!firstRow)
                printf("\n");
            printf("%p: ", memStr.getAddress());
            column = 0;
            firstRow = false;
        }

        if (memStr.next(value))
            return 1;

        printer((U) value);
    }
    printf("\n");

    return 0;
}

template <typename T, typename U = T>
static int printfMemory(MemoryStreamer &memStr, size_t repeat, const char *fmt,
                        int numColumns)
{
    auto printfer = [fmt](U value) { printf(fmt, (U) value); };

    return dumpMemoryWith<T, U>(memStr, repeat, numColumns, printfer);
}

template <typename T>
static int dumpBinary(MemoryStreamer &memStr, size_t repeat, int numColumns,
                      size_t numSpaces)
{
    std::string spaces(numSpaces, ' ');
    auto binaryPrinter = [spaces](T value)
    {
        std::string str;
        for (T mask = (T) 1 << (sizeof(T) * 8 - 1); mask; mask >>= 1)
            str += (value & mask) ? '1' : '0';
        printf("%s%s", str.c_str(), spaces.c_str());
    };

    return dumpMemoryWith<T>(memStr, repeat, numColumns, binaryPrinter);
}

static int dumpCharacters(MemoryStreamer &memStr, size_t repeat, int numColumns,
                          int fieldWidth)
{
    auto characterPrinter = [fieldWidth](char c)
    {
        std::stringstream ss;
        ss << '\'' + escapeCharacter(c, true, false, true) + '\'';
        printf("%-*s", fieldWidth, ss.str().c_str());
    };

    return dumpMemoryWith<char>(memStr, repeat, numColumns, characterPrinter);
}

static int dumpStrings(MemoryStreamer &memStr, size_t repeat)
{
    int error = 0;

    size_t stringsPrinted = 0;
    while (stringsPrinted < repeat && !error) {
        printf("%p: \"", memStr.getAddress());

        for (;;) {
            char c;
            if ((error = memStr.next(c)) || !c)
                break;
            printf("%s", escapeCharacter(c, false, true, true).c_str());
        }

        printf("\"\n");
        ++stringsPrinted;
    }

    return (error) ? 1 : 0;
}

static int doDump(pid_t pid, Builtins::ErrorContext &errorContext,
           void *address, size_t repeat, Format format, size_t size)
{
    MemoryStreamer memStr{pid, address};

    switch (format) {
        case Format::DECIMAL:
            switch (size) {
                case Size::BYTE:
                    return printfMemory<int8_t>(memStr, repeat, "%-8" PRIi8, 8);
                case Size::HALFWORD:
                    return printfMemory<int16_t>(memStr, repeat, "%-8" PRIi16, 8);
                case Size::WORD:
                    return printfMemory<int32_t>(memStr, repeat, "%-15" PRIi32, 4);
                case Size::GIANT:
                    return printfMemory<int64_t>(memStr, repeat, "%-24" PRIi64, 2);
            }
        case Format::UNSIGNED_DECIMAL:
            switch (size) {
                case Size::BYTE:
                    return printfMemory<uint8_t>(memStr, repeat, "%-8" PRIu8, 8);
                case Size::HALFWORD:
                    return printfMemory<uint16_t>(memStr, repeat, "%-8" PRIu16, 8);
                case Size::WORD:
                    return printfMemory<uint32_t>(memStr, repeat, "%-15" PRIu32, 4);
                case Size::GIANT:
                    return printfMemory<uint64_t>(memStr, repeat, "%-24" PRIu64, 2);
            }
        case Format::OCTAL:
            switch (size) {
                case Size::BYTE:
                    return printfMemory<uint8_t>(
                        memStr, repeat, "0%03" PRIo8 "    ", 8);
                case Size::HALFWORD:
                    return printfMemory<uint16_t>(
                        memStr, repeat, "0%06" PRIo16 "    ", 4);
                case Size::WORD:
                    return printfMemory<uint32_t>(
                        memStr, repeat, "0%011" PRIo32 "    ", 4);
                case Size::GIANT:
                    return printfMemory<uint64_t>(
                        memStr, repeat, "0%022" PRIo64 "      ", 2);
            }
        case Format::HEXADECIMAL:
            switch (size) {
                case Size::BYTE:
                    return printfMemory<uint8_t>(
                        memStr, repeat, PRINTFx8 "    ", 8);
                case Size::HALFWORD:
                    return printfMemory<uint16_t>(
                        memStr, repeat, PRINTFx16 "  ", 8);
                case Size::WORD:
                    return printfMemory<uint32_t>(
                        memStr, repeat, PRINTFx32 "    ", 4);
                case Size::GIANT:
                    return printfMemory<uint64_t>(
                        memStr, repeat, PRINTFx64 "      ", 2);
            }
        case Format::BINARY:
            switch (size) {
                case Size::BYTE:
                    return dumpBinary<uint8_t>(memStr, repeat, 8, 2);
                case Size::HALFWORD:
                    return dumpBinary<uint16_t>(memStr, repeat, 4, 2);
                case Size::WORD:
                    return dumpBinary<uint32_t>(memStr, repeat, 2, 4);
                case Size::GIANT:
                    return dumpBinary<uint64_t>(memStr, repeat, 1, 0);
            }
        case Format::FLOAT:
            switch (size) {
                case Size::WORD:
                    return printfMemory<float, double>(memStr, repeat, "%-16g", 4);
                case Size::GIANT:
                    return printfMemory<double>(memStr, repeat, "%-16g", 4);
                default:
                    errorContext.printMessage("invalid size for float");
                    return 1;
            }
        case Format::CHARACTER:
            switch (size) {
                case Size::BYTE:
                    return dumpCharacters(memStr, repeat, 8, 8);
                default:
                    errorContext.printMessage("invalid size for character");
                    return 1;
            }
        case Format::ADDRESS:
            if (size == sizeof(void*))
                return printfMemory<void*>(
                    memStr, repeat, "%-24p", 16 / sizeof(void*));
            else {
                errorContext.printMessage("invalid size for address");
                return 1;
            }
        case Format::STRING:
            return dumpStrings(memStr, repeat);
        default:
            return 1;
    }
}

BUILTIN_FUNC(memory)
{
    static size_t repeat = 1;
    static Format format = Format::HEXADECIMAL;
    static size_t size = sizeof(long);
    static void *address = nullptr;

    if (wantsHelp(args)) {
        std::string usage = getUsage(commandName);
        printf("%s\n", usage.c_str());
        printf(
            "Formats:\n"
            "  d -- decimal\n"
            "  u -- unsigned decimal\n"
            "  o -- unsigned octal\n"
            "  x -- unsigned hexadecimal\n"
            "  t -- unsigned binary\n"
            "  f -- floating point\n"
            "  a -- address\n"
            "  c -- character\n"
            "  s -- string\n");
        printf(
            "Sizes:\n"
            "  b -- byte (1 byte)\n"
            "  h -- half word (2 bytes)\n"
            "  w -- word (4 bytes)\n"
            "  g -- giant (8 bytes)\n");
        return 0;
    }

    if (args.size() > 4) {
        std::string usage = getUsage(commandName);
        env.errorContext.printMessage(usage.c_str(), commandStart);
        return 1;
    }

    // Address
    if (args.size() > 0) {
        if (checkValueType(*args[0], Builtins::ValueType::INTEGER,
                        "expected address", env.errorContext))
            return 1;

        address = reinterpret_cast<void *>(args[0]->getInteger());
    }

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

        // Special-case default sizes
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
        if (format == Format::STRING) {
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

    if (doDump(env.tracee.getPid(), env.errorContext, address, repeat, format, size))
        return 1;

    return 0;
}
