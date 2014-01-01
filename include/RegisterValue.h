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

struct RegisterValue {
    RegisterType type;

protected:
    RegisterValue(RegisterType type) : type{type} {}

public:
    uint8_t getInt8() const;
    uint16_t getInt16() const;
    uint32_t getInt32() const;
    uint64_t getInt64() const;
    my_uint128 getInt128() const;
    float getFloat() const;
    double getDouble() const;
    long double getLongDouble() const;
};

struct Int8RegisterValue : public RegisterValue {
    uint8_t value;

    Int8RegisterValue(uint8_t value)
        : RegisterValue{RegisterType::INT8}, value{value} {}
};

struct Int16RegisterValue : public RegisterValue {
    uint16_t value;

    Int16RegisterValue(uint16_t value)
        : RegisterValue{RegisterType::INT16}, value{value} {}
};

struct Int32RegisterValue : public RegisterValue {
    uint32_t value;

    Int32RegisterValue(uint32_t value)
        : RegisterValue{RegisterType::INT32}, value{value} {}
};

struct Int64RegisterValue : public RegisterValue {
    uint64_t value;

    Int64RegisterValue(uint64_t value)
        : RegisterValue{RegisterType::INT64}, value{value} {}
};

struct Int128RegisterValue : public RegisterValue {
    my_uint128 value;

    Int128RegisterValue(my_uint128 value)
        : RegisterValue{RegisterType::INT128}, value{value.lo, value.hi} {}
};

struct FloatRegisterValue : public RegisterValue {
    float value;

    FloatRegisterValue(float value)
        : RegisterValue{RegisterType::FLOAT}, value{value} {}
};

struct DoubleRegisterValue : public RegisterValue {
    double value;

    DoubleRegisterValue(double value)
        : RegisterValue{RegisterType::DOUBLE}, value{value} {}
};

struct LongDoubleRegisterValue : public RegisterValue {
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
