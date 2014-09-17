/*
 * print built-in command for printing expressions.
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
#include "Builtins/Commands.h"
#include "Builtins/Support.h"

using Builtins::escapeCharacter;

BUILTIN_FUNC(print)
{
    for (auto &arg : args) {
        switch (arg->getType()) {
            case Builtins::ValueType::IDENTIFIER:
                printf("identifier: %s\n", arg->getIdentifier().c_str());
                break;
            case Builtins::ValueType::INTEGER:
                printf("integer: 0x%1$lx (%1$ld)\n", arg->getInteger());
                break;
            case Builtins::ValueType::FLOAT:
                printf("floating: %f\n", arg->getFloat());
                break;
            case Builtins::ValueType::STRING:
		printf("string: \"");
		for (char c : arg->getString()) {
			std::string escaped = escapeCharacter(c, false, true, true);
			printf("%s", escaped.c_str());
		}
                printf("\"\n");
                break;
        }
    }

    return 0;
}
