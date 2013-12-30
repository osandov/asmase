#ifndef ASMASE_REGISTER_INFO_H
#define ASMASE_REGISTER_INFO_H

#include <string>
#include <vector>

struct UserRegisters;

/** Register descriptor. */
struct RegisterD {
    std::string name;
    size_t offset;
    std::string prefix;

#if 0
    template <typename T>
    T getUnsafe(const UserRegisters &regs);
#endif
};

/** Registor descriptor with a type. */
template <typename T>
struct RegisterDT : RegisterD {
    RegisterDT(const std::string &name, size_t offset)
        : RegisterD{name, offset} {}
    RegisterDT(const std::string &prefix, const std::string &name, size_t offset)
        : RegisterD{name, offset, prefix} {}

    T get(const UserRegisters &regs)
    {
        auto reg = reinterpret_cast<const unsigned char *>(&regs) + offset;
        return *reinterpret_cast<const T *>(reg);
    }
};

struct RegisterInfo {
    std::vector<RegisterD> generalPurpose;
    std::vector<RegisterD> conditionCodes;
    RegisterD programCounter;

    std::vector<RegisterD> segmentation;

    std::vector<RegisterD> floatingPoint;
    std::vector<RegisterD> floatingPointStatus;

    std::vector<RegisterD> extra;
    std::vector<RegisterD> extraStatus;
};

enum class RegisterCategory {
    GENERAL_PURPOSE,
    CONDITION_CODE,
    SEGMENTATION,
    FLOATING_POINT,
    EXTRA,
};

#endif /* ASMASE_REGISTER_INFO_H */
