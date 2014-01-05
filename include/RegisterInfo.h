/*
 * Architecture-agnostic representation of an architecture's set of registers.
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

#ifndef ASMASE_REGISTER_INFO_H
#define ASMASE_REGISTER_INFO_H

#include <string>
#include <vector>

#include "RegisterDesc.h"

/** Information on the registers for an architecture. */
class RegisterInfo {
public:
    /** List of registers. */
    std::vector<RegisterDesc> registers;
};

#endif /* ASMASE_REGISTER_INFO_H */
