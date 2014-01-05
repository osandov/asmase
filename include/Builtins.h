/*
 * Public interface to the built-in command system.
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

#ifndef ASMASE_BUILTINS_H
#define ASMASE_BUILTINS_H

class Inputter;
class Tracee;

/**
 * Return whether the given string should be interpreted as a command line
 * built-in.
 */
bool isBuiltin(const std::string &str);

/**
 * Run a command line built-in.
 * @return Positive on error, 0 on success, negative on exit.
 */
int runBuiltin(const std::string &str, Tracee &tracee, Inputter &inputter);

#endif /* ASMASE_BUILTINS_H */
