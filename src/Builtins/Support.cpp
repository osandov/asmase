/*
 * Common support for the built-in system.
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

#include "Builtins/AST.h"
#include "Builtins/ErrorContext.h"
#include "Builtins/Support.h"

namespace Builtins {

bool checkValueType(const ValueAST &value, ValueType type,
                    const char *errorMsg, ErrorContext &errorContext)
{
    if (value.getType() != type) {
        errorContext.printMessage(errorMsg, value.getStart());
        return true;
    } else
        return false;
}

bool wantsHelp(const std::vector<std::unique_ptr<ValueAST>> &args)
{
    return args.size() >= 1 && args[0]->getType() == ValueType::IDENTIFIER &&
           args[0]->getIdentifier() == "help";
}

std::string escapeCharacter(char c, bool escapeSingleQuote,
                            bool escapeDoubleQuote, bool escapeBackslash)
{
    if (c == '\0') // Null
        return "\\0";
    else if (c == '\a') // Bell
        return "\\a";
    else if (c == '\b') // Backspace
        return "\\b";
    else if (c == '\t') // Horizontal tab
        return "\\t";
    else if (c == '\n') // New line
        return "\\n";
    else if (c == '\v') // Vertical tab
        return "\\v";
    else if (c == '\f') // Form feed
        return "\\f";
    else if (c == '\r') // Carriage return
        return "\\r";
    else if (c == '\'' && escapeSingleQuote) // Single-quote
        return "\\'";
    else if (c == '"' && escapeDoubleQuote) // Double-quote
        return "\\\"";
    else if (c == '\\' && escapeBackslash) // Backslash
        return "\\\\";
    else if (isprint(c)) // Other printable characters
        return std::string{c};
    else { // Anything else should be an escaped hex sequence
        char escaped[5];
        snprintf(escaped, sizeof(escaped), "\\x%02x",
            (int) (unsigned char) c);
        return std::string{escaped};
    }
}

}
