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
