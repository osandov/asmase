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

}
