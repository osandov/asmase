/*
 * source built-in command for redirecting input to a file.
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

#include <cstdio>
#include <sstream>

#include "Builtins/AST.h"
#include "Builtins/Commands.h"
#include "Builtins/Environment.h"
#include "Builtins/ErrorContext.h"
#include "Builtins/Support.h"

#include "Inputter.h"

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

    if (env.inputter.redirectInput(filename))
        return 1;

    return 0;
}
