#ifndef ASMASE_REGISTER_INFO_H
#define ASMASE_REGISTER_INFO_H

#include <string>
#include <vector>

#include "RegisterDesc.h"

struct RegisterInfo {
    std::vector<RegisterDesc> generalPurpose;
    std::vector<RegisterDesc> conditionCodes;
    RegisterDesc programCounter;

    std::vector<RegisterDesc> segmentation;

    std::vector<RegisterDesc> floatingPoint;
    std::vector<RegisterDesc> floatingPointStatus;

    std::vector<RegisterDesc> extra;
    std::vector<RegisterDesc> extraStatus;
};

enum class RegisterCategory {
    GENERAL_PURPOSE,
    CONDITION_CODE,
    SEGMENTATION,
    FLOATING_POINT,
    EXTRA,
};

#endif /* ASMASE_REGISTER_INFO_H */
