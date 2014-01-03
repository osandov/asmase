#ifndef ASMASE_REGISTER_CATEGORY_H
#define ASMASE_REGISTER_CATEGORY_H

enum class RegisterCategory : int {
    GENERAL_PURPOSE = 0x1,
    STATUS          = 0x2,
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

#endif /* ASMASE_REGISTER_CATEGORY_H */
