#include <cstddef>
#include <iostream>

#include "ProcessorFlags.h"
#include "RegisterInfo.h"
#include "Support.h"
#include "Arch/ARM/UserRegisters.h"
#include "Arch/ARM/ARMTracee.h"

#define USER_REGISTER(reg) offsetof(UserRegisters, reg)
using RT = RegisterType;
using RC = RegisterCategory;
extern const RegisterInfo ARMRegisters = {
    .registers = {
        // General-purpose
        {RT::INT32, RC::GENERAL_PURPOSE, "r0",  USER_REGISTER(r0)},
        {RT::INT32, RC::GENERAL_PURPOSE, "r1",  USER_REGISTER(r1)},
        {RT::INT32, RC::GENERAL_PURPOSE, "r2",  USER_REGISTER(r2)},
        {RT::INT32, RC::GENERAL_PURPOSE, "r3",  USER_REGISTER(r3)},
        {RT::INT32, RC::GENERAL_PURPOSE, "r4",  USER_REGISTER(r4)},
        {RT::INT32, RC::GENERAL_PURPOSE, "r5",  USER_REGISTER(r5)},
        {RT::INT32, RC::GENERAL_PURPOSE, "r6",  USER_REGISTER(r6)},
        {RT::INT32, RC::GENERAL_PURPOSE, "r7",  USER_REGISTER(r7)},
        {RT::INT32, RC::GENERAL_PURPOSE, "r8",  USER_REGISTER(r8)},
        {RT::INT32, RC::GENERAL_PURPOSE, "r9",  USER_REGISTER(r9)},
        {RT::INT32, RC::GENERAL_PURPOSE, "r10", USER_REGISTER(r10)},
        {RT::INT32, RC::GENERAL_PURPOSE, "r11", USER_REGISTER(r11)},
        {RT::INT32, RC::GENERAL_PURPOSE, "r12", USER_REGISTER(r12)},
        {RT::INT32, RC::GENERAL_PURPOSE, "r13", USER_REGISTER(r13)},
        {RT::INT32, RC::GENERAL_PURPOSE, "r14", USER_REGISTER(r14)},
        {RT::INT32, RC::GENERAL_PURPOSE, "r15", USER_REGISTER(r15)},

        // Aliases for general-purpose registers
        {RT::INT32, RC::GENERAL_PURPOSE, "fp", USER_REGISTER(fp)},
        {RT::INT32, RC::GENERAL_PURPOSE, "ip", USER_REGISTER(ip)},
        {RT::INT32, RC::GENERAL_PURPOSE, "sp", USER_REGISTER(sp)},
        {RT::INT32, RC::GENERAL_PURPOSE, "lr", USER_REGISTER(lr)},
        {RT::INT32, RC::GENERAL_PURPOSE, "pc", USER_REGISTER(pc)},

         // Condition codes
        {RT::INT32, RC::CONDITION_CODE, "cpsr", USER_REGISTER(cpsr)},
    },
};

/** Flags in the cpsr register. */
static ProcessorFlags<decltype(UserRegisters::cpsr)> cpsrFlags = {
    {"N", 0x80000000}, // Negative/less than bit
    {"Z", 0x40000000}, // Zero bit
    {"C", 0x20000000}, // Carry/borrow/extend bit
    {"V", 0x10000000}, // Overflow bit
    {"Q", 0x08000000}, // Sticky overflow bit
    {"J", 0x01000000}, // Java bit
    {"DNM", 0x00f00000, 0x0, true}, // Do Not Modify bits
    {"GE", 0x000f0000, 0x0, true}, // Greater-than-or-equal bits

    // If-Then state
    {"IT_cond", 0x0000e000, 0x0, true}, // Current If-Then block
    {"a", 0x00001000},
    {"b", 0x00000800},
    {"c", 0x00000400},
    {"d", 0x00400000},
    {"e", 0x00200000},

    {"E", 0x00000200}, // Endianess
    {"A", 0x00000100}, // Imprecise data abort disable bit
    {"I", 0x00000080}, // IRQ disable bit
    {"F", 0x00000040}, // FIQ disable bit
    {"T", 0x00000020}, // Thumb state bit
    {"T", 0x00000020}, // Thumb state bit

    // Mode bits
    {"M=User",       0x0000001f, 0x00000010},
    {"M=FIQ",        0x0000001f, 0x00000011},
    {"M=IRQ",        0x0000001f, 0x00000012},
    {"M=Supervisor", 0x0000001f, 0x00000013},
    {"M=Abort",      0x0000001f, 0x00000017},
    {"M=Undefined",  0x0000001f, 0x0000001b},
    {"M=System",     0x0000001f, 0x0000001f},
};

int ARMTracee::printGeneralPurposeRegisters()
{
    printf("r0  = " PRINTFx32 "    r1 = " PRINTFx32 "\n"
           "r2  = " PRINTFx32 "    r3 = " PRINTFx32 "\n"
           "r4  = " PRINTFx32 "    r5 = " PRINTFx32 "\n"
           "r6  = " PRINTFx32 "    r7 = " PRINTFx32 "\n"
           "r8  = " PRINTFx32 "    r9 = " PRINTFx32 "\n"
           "r10 = " PRINTFx32 "    fp = " PRINTFx32 "\n"
           "ip  = " PRINTFx32 "    sp = " PRINTFx32 "\n"
           "lr  = " PRINTFx32 "    pc = " PRINTFx32 "\n",
           registers->r0, registers->r1, registers->r2,  registers->r3,
           registers->r4, registers->r5, registers->r6,  registers->r7,
           registers->r8, registers->r9, registers->r10, registers->fp,
           registers->ip, registers->sp, registers->lr, registers->pc);
    return 0;
};

int ARMTracee::printConditionCodeRegisters()
{
    printf("cpsr = " PRINTFx32 " = ", registers->cpsr);
    cpsrFlags.printFlags(registers->cpsr);
    std::cout << '\n';
    return 0;
}
