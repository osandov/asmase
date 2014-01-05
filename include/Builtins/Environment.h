/*
 * Environment class.
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

#ifndef ASMASE_BUILTINS_ENVIRONMENT_H
#define ASMASE_BUILTINS_ENVIRONMENT_H

#include <string>
#include <sys/types.h>

class Tracee;
class Inputter;

namespace Builtins {

class ErrorContext;
class ValueAST;

/** The environment in which to evaluate and run built-in commands. */
class Environment {
public:
    Tracee &tracee;
    Inputter &inputter;

    /** Error context for the input being run. */
    ErrorContext &errorContext;

    Environment(Tracee &tracee, Inputter &inputter, ErrorContext &errorContext)
        : tracee(tracee), inputter(inputter), errorContext(errorContext) {}

    /**
     * Look up a variable in the environment.
     * @return The value of the variable on success, null pointer on failure
     * and errorMsg is set.
     */
    ValueAST *lookupVariable(const std::string &var, std::string &errorMsg);
};

}

#endif /* ASMASE_BUILTINS_ENVIRONMENT_H */
