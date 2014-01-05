/*
 * External definitions of built-in commands.
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

#ifndef ASMASE_BUILTINS_COMMANDS_H
#define ASMASE_BUILTINS_COMMANDS_H

#include <memory>
#include <vector>

namespace Builtins {

class Environment;
class ValueAST;

}

/** Built-in command function pointer type. */
typedef int (*BuiltinFunc)(const std::vector<std::unique_ptr<Builtins::ValueAST>> &,
                           const std::string &, int, int, Builtins::Environment &);

/** Declare a built-in command function. */
#define BUILTIN_FUNC(func) int builtin_##func(\
        const std::vector<std::unique_ptr<Builtins::ValueAST>> &args, \
        const std::string &commandName, int commandStart, int commandEnd, \
        Builtins::Environment &env)

BUILTIN_FUNC(source);
BUILTIN_FUNC(memory);
BUILTIN_FUNC(registers);
BUILTIN_FUNC(warranty);
BUILTIN_FUNC(copying);

#endif /* ASMASE_BUILTINS_COMMANDS_H */
