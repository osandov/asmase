/*
 * Dynamically typed register values.
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

#ifndef ASMASE_REGISTER_VALUE_H
#define ASMASE_REGISTER_VALUE_H

#include <cassert>
#include <cinttypes>

enum class RegisterType {
    INT8,
    INT16,
    INT32,
    INT64,
    INT128,
    FLOAT,
    DOUBLE,
    LONG_DOUBLE,
};

struct my_uint128 {
    uint64_t lo, hi;
};

static_assert(sizeof(my_uint128) == 16, "my_uint128 is not packed");

/** Value of a register with a runtime type. */
class RegisterValue {
public:
    RegisterType type;

protected:
    RegisterValue(RegisterType type) : type{type} {}

public:
    // Cast to the runtime type and get the value.
    uint8_t getInt8() const;
    uint16_t getInt16() const;
    uint32_t getInt32() const;
    uint64_t getInt64() const;
    my_uint128 getInt128() const;
    float getFloat() const;
    double getDouble() const;
    long double getLongDouble() const;
};

class Int8RegisterValue : public RegisterValue {
public:
    uint8_t value;

    Int8RegisterValue(uint8_t value)
        : RegisterValue{RegisterType::INT8}, value{value} {}
};

class Int16RegisterValue : public RegisterValue {
public:
    uint16_t value;

    Int16RegisterValue(uint16_t value)
        : RegisterValue{RegisterType::INT16}, value{value} {}
};

class Int32RegisterValue : public RegisterValue {
public:
    uint32_t value;

    Int32RegisterValue(uint32_t value)
        : RegisterValue{RegisterType::INT32}, value{value} {}
};

class Int64RegisterValue : public RegisterValue {
public:
    uint64_t value;

    Int64RegisterValue(uint64_t value)
        : RegisterValue{RegisterType::INT64}, value{value} {}
};

class Int128RegisterValue : public RegisterValue {
public:
    my_uint128 value;

    Int128RegisterValue(my_uint128 value)
        : RegisterValue{RegisterType::INT128}, value{value.lo, value.hi} {}
};

class FloatRegisterValue : public RegisterValue {
public:
    float value;

    FloatRegisterValue(float value)
        : RegisterValue{RegisterType::FLOAT}, value{value} {}
};

class DoubleRegisterValue : public RegisterValue {
public:
    double value;

    DoubleRegisterValue(double value)
        : RegisterValue{RegisterType::DOUBLE}, value{value} {}
};

class LongDoubleRegisterValue : public RegisterValue {
public:
    long double value;

    LongDoubleRegisterValue(long double value)
        : RegisterValue{RegisterType::LONG_DOUBLE}, value{value} {}
};

inline uint8_t RegisterValue::getInt8() const
{
    assert(type == RegisterType::INT8);
    auto int8val = static_cast<const Int8RegisterValue *>(this);
    return int8val->value;
}

inline uint16_t RegisterValue::getInt16() const
{
    assert(type == RegisterType::INT16);
    auto int16val = static_cast<const Int16RegisterValue *>(this);
    return int16val->value;
}

inline uint32_t RegisterValue::getInt32() const
{
    assert(type == RegisterType::INT32);
    auto int32val = static_cast<const Int32RegisterValue *>(this);
    return int32val->value;
}

inline uint64_t RegisterValue::getInt64() const
{
    assert(type == RegisterType::INT64);
    auto int64val = static_cast<const Int64RegisterValue *>(this);
    return int64val->value;
}

inline my_uint128 RegisterValue::getInt128() const
{
    assert(type == RegisterType::INT128);
    auto int128val = static_cast<const Int128RegisterValue *>(this);
    return int128val->value;
}

inline float RegisterValue::getFloat() const
{
    assert(type == RegisterType::FLOAT);
    auto floatval = static_cast<const FloatRegisterValue *>(this);
    return floatval->value;
}

inline double RegisterValue::getDouble() const
{
    assert(type == RegisterType::DOUBLE);
    auto doubleval = static_cast<const DoubleRegisterValue *>(this);
    return doubleval->value;
}

inline long double RegisterValue::getLongDouble() const
{
    assert(type == RegisterType::LONG_DOUBLE);
    auto longdoubleval = static_cast<const LongDoubleRegisterValue *>(this);
    return longdoubleval->value;
}

#endif /* ASMASE_REGISTER_VALUE_H */
