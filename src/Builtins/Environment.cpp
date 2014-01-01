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

#include "Builtins/AST.h"
#include "Builtins/Environment.h"

#include "Tracee.h"
#include "RegisterValue.h"

namespace Builtins {

// Currently, the only variables are registers.
ValueAST *Environment::lookupVariable(const std::string &var,
                                      std::string &errorMsg)
{
    std::string regName = var.substr(1);
    std::shared_ptr<RegisterValue> value = tracee.getRegisterValue(regName);
    if (value) {
        switch (value->type) {
            case RegisterType::INT8:
                return new IntegerExpr(0, 0, value->getInt8());
            case RegisterType::INT16:
                return new IntegerExpr(0, 0, value->getInt16());
            case RegisterType::INT32:
                return new IntegerExpr(0, 0, value->getInt32());
            case RegisterType::INT64:
                if (sizeof(long) >= 8)
                    return new IntegerExpr(0, 0, value->getInt64());
                else {
                    errorMsg = "register too big";
                    return nullptr;
                }
            case RegisterType::INT128:
                errorMsg = "register too big";
                return nullptr;
            case RegisterType::FLOAT:
                return new FloatExpr(0, 0, value->getFloat());
            case RegisterType::DOUBLE:
                return new FloatExpr(0, 0, value->getDouble());
            case RegisterType::LONG_DOUBLE:
                return new FloatExpr(0, 0, value->getLongDouble());
        }
    } else {
        errorMsg = "unknown variable";
        return nullptr;
    }
}

}
