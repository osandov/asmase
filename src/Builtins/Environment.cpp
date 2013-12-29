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

#include "assembler.h"
#include "tracing.h"

namespace Builtins {

Environment::Environment(struct assembler *asmb, struct tracee_info *tracee,
                         ErrorContext &errorContext)
    : asmb(asmb), pid(tracee->pid), shared_page(tracee->shared_page),
        errorContext(errorContext) {}

// Currently, the only variables are registers.
ValueAST *Environment::lookupVariable(const std::string &var,
                                      std::string &errorMsg)
{
    struct register_value regval;
    if (get_register_value(pid, var.c_str() + 1, &regval)) {
        errorMsg = "unknown variable";
        return nullptr;
    }

    switch (regval.type) {
        case REGISTER_INTEGER:
            return new IntegerExpr(0, 0, regval.integer);
        case REGISTER_FLOATING:
            return new FloatExpr(0, 0, regval.integer);
    }

    assert(false);
    return nullptr;
}

}
