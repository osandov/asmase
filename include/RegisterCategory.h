/*
 * Register categories.
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

#ifndef ASMASE_REGISTER_CATEGORY_H
#define ASMASE_REGISTER_CATEGORY_H

/** Bitmask-able register categories. */
enum class RegisterCategory : int {
    NONE            = 0x0,
    GENERAL_PURPOSE = 0x1,
    CONDITION_CODE  = 0x2,
    PROGRAM_COUNTER = 0x4,
    SEGMENTATION    = 0x8,
    FLOATING_POINT  = 0x10,
    EXTRA           = 0x20,
};

inline RegisterCategory operator|(RegisterCategory lhs, RegisterCategory rhs)
{
    return (RegisterCategory) ((int) lhs | (int) rhs);
}

inline RegisterCategory operator&(RegisterCategory lhs, RegisterCategory rhs)
{
    return (RegisterCategory) ((int) lhs & (int) rhs);
}

inline RegisterCategory operator~(RegisterCategory x)
{
    return (RegisterCategory) (~(int) x);
}

/** Return whether any bit in the category set is set. */
inline bool any(RegisterCategory x)
{
    return x != RegisterCategory::NONE;
}

#endif /* ASMASE_REGISTER_CATEGORY_H */
