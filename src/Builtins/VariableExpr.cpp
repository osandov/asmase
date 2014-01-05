/*
 * Implementation of evaluation of variables.
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
#include "Builtins/Environment.h"
#include "Builtins/ErrorContext.h"

namespace Builtins {

// To evaluate a variable is to get its value from the environment.
ValueAST *VariableExpr::eval(Environment &env) const
{
    std::string errorMsg;
    ValueAST *result = env.lookupVariable(name, errorMsg);
    
    if (result) {
        // Set the bounds of the value to the bounds of the original expression
        result->setStart(getStart());
        result->setEnd(getEnd());
    } else
        env.errorContext.printMessage(errorMsg.c_str(), getStart());

    return result;
}

}
