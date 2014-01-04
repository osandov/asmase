#ifndef ASMASE_ARCH_ARM_USER_REGISTERS_H
#define ASMASE_ARCH_ARM_USER_REGISTERS_H

#include <cinttypes>

class UserRegisters {
public:
    // General-purpose registers
    uint32_t r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10;
    uint32_t fp, ip, sp, lr, pc;

    // Condition codes
    uint32_t cpsr;
};

#define r11 fp
#define r12 ip
#define r13 sp
#define r14 lr
#define r15 pc

#endif /* ASMASE_ARCH_ARM_USER_REGISTERS_H */
