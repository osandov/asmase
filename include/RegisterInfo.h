#ifndef ASMASE_REGISTER_INFO_H
#define ASMASE_REGISTER_INFO_H

#include <string>
#include <vector>

#include "RegisterDesc.h"

struct RegisterInfo {
    std::vector<RegisterDesc> registers;
};

#endif /* ASMASE_REGISTER_INFO_H */
