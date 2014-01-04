#ifndef ASMASE_REGISTER_DESC_H
#define ASMASE_REGISTER_DESC_H

#include "RegisterCategory.h"
#include "RegisterValue.h"

class UserRegisters;

/** Register descriptor. */
class RegisterDesc {
public:
    RegisterType type;
    RegisterCategory category;
    std::string prefix;
    std::string name;
    size_t offset;

    RegisterDesc(RegisterType type, RegisterCategory category,
                 const std::string &prefix, const std::string &name,
                 size_t offset)
        : type{type}, category{category}, prefix{prefix}, name{name},
          offset{offset} {}

    RegisterDesc(RegisterType type, RegisterCategory category,
                 const std::string &name, size_t offset)
        : RegisterDesc{type, category, "", name, offset} {}

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
