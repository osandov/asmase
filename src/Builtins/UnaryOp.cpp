/*
 * Implementation of evaluation of a unary operator.
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

#include <map>

#include "Builtins/AST.h"
#include "Builtins/Environment.h"
#include "Builtins/ErrorContext.h"
#include "Builtins/Support.h"

namespace Builtins {

/** ValueAST member function type for applying a unaryinary operator. */
typedef ValueAST* (ValueAST::*UnaryOpFunction)(Environment &) const;

/**
 * Lookup table from unary opcodes to the corresponding ValueAST member
 * function.
 */
static std::map<UnaryOpcode, UnaryOpFunction> unaryFunctionMap = {
    {UnaryOpcode::PLUS,         &ValueAST::unaryPlus},
    {UnaryOpcode::MINUS,        &ValueAST::unaryMinus},
    {UnaryOpcode::LOGIC_NEGATE, &ValueAST::logicNegate},
    {UnaryOpcode::BIT_NEGATE,   &ValueAST::bitNegate},
};

// To evaluate a unary operator is to evaluate the operand and apply the
// operator to it.
ValueAST *UnaryOp::eval(Environment &env) const
{
    std::unique_ptr<ValueAST> value(operand->eval(env));
    if (!value)
        return nullptr;

    UnaryOpFunction func = findWithDefault(unaryFunctionMap, op, nullptr);
    assert(func);

    ValueAST *result = (*value.*func)(env);

    if (result == (ValueAST *) -1) {
        env.errorContext.printMessage("invalid argument type to unary expression",
                                      getStart(), value->getStart(), value->getEnd());
        result = nullptr;
    }

    // Adjust the bounds of the evaluated expression to reflect the applied
    // operator as well
    if (result)
        result->setStart(getStart());

    return result;
}

}
