/*
 * Architecture-agnostic descriptor for a register.
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

#ifndef ASMASE_REGISTER_DESC_H
#define ASMASE_REGISTER_DESC_H

#include "RegisterCategory.h"
#include "RegisterValue.h"

class UserRegisters;

/** Register descriptor. */
class RegisterDesc {
public:
    /** The type of the register value. */
    RegisterType type;

    /** Category to which the register belongs. */
    RegisterCategory category;

    /**
     * Optional prefix for the register name for displaying. Ignored when
     * searching.
     */
    std::string prefix;

    /** Name of the register. */
    std::string name;

    /** Offset of the register value in the UserRegisters structure. */
    size_t offset;

    RegisterDesc(RegisterType type, RegisterCategory category,
                 const std::string &prefix, const std::string &name,
                 size_t offset)
        : type{type}, category{category}, prefix{prefix}, name{name},
          offset{offset} {}

    /** Empty prefix constructor. */
    RegisterDesc(RegisterType type, RegisterCategory category,
                 const std::string &name, size_t offset)
        : RegisterDesc{type, category, "", name, offset} {}

    /** Get the value of the register from the UserRegisters structure. */
    RegisterValue *getValue(const UserRegisters &regs) const
    {
        switch (type) {
            case RegisterType::INT8:
                return new Int8RegisterValue{getRaw<uint8_t>(regs)};
            case RegisterType::INT16:
                return new Int16RegisterValue{getRaw<uint16_t>(regs)};
            case RegisterType::INT32:
                return new Int32RegisterValue{getRaw<uint32_t>(regs)};
            case RegisterType::INT64:
                return new Int64RegisterValue{getRaw<uint64_t>(regs)};
            case RegisterType::INT128:
                return new Int128RegisterValue{getRaw<my_uint128>(regs)};
            case RegisterType::FLOAT:
                return new FloatRegisterValue{getRaw<float>(regs)};
            case RegisterType::DOUBLE:
                return new DoubleRegisterValue{getRaw<double>(regs)};
            case RegisterType::LONG_DOUBLE:
                return new LongDoubleRegisterValue{getRaw<long double>(regs)};
            default:
                return nullptr;
        }
    }

private:
    template <typename T>
    T getRaw(const UserRegisters &regs) const
    {
        auto reg = reinterpret_cast<const unsigned char *>(&regs) + offset;
        return *reinterpret_cast<const T *>(reg);
    }
};

#endif /* ASMASE_REGISTER_DESC_H */
