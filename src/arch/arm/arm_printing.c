/*
 * Copyright (C) 2013 Omar Sandoval
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>

#include <sys/ptrace.h>
#include <sys/user.h>

#include <asm/ptrace.h>

#include "flags.h"
#include "tracing.h"

/** Flags in the cpsr registers. */
struct processor_flag cpsr_flags[] = {
    BIT_FLAG("N", 0x80000000), /* Negative/less than bit */
    BIT_FLAG("Z", 0x40000000), /* Zero bit */
    BIT_FLAG("C", 0x20000000), /* Carry/borrow/extend bit */
    BIT_FLAG("V", 0x10000000), /* Overflow bit */
    BIT_FLAG("Q", 0x08000000), /* Sticky overflow bit */
    BIT_FLAG("J", 0x01000000), /* Java bit */
    {"DNM", 0x00f00000, 0x0, 1}, /* Do Not Modify bits */
    {"GE", 0x000f0000, 0x0, 1}, /* Greater-than-or-equal bits */

    /* If-Then state */
    {"IT_cond", 0x0000e000, 0x0, 1}, /* Current If-Then block */
    BIT_FLAG("a", 0x00001000),
    BIT_FLAG("b", 0x00000800),
    BIT_FLAG("c", 0x00000400),
    BIT_FLAG("d", 0x00400000),
    BIT_FLAG("e", 0x00200000),

    BIT_FLAG("E", 0x00000200), /* Endianess */
    BIT_FLAG("A", 0x00000100), /* Imprecise data abort disable bit */
    BIT_FLAG("I", 0x00000080), /* IRQ disable bit */
    BIT_FLAG("F", 0x00000040), /* FIQ disable bit */
    BIT_FLAG("T", 0x00000020), /* Thumb state bit */
    BIT_FLAG("T", 0x00000020), /* Thumb state bit */

    /* Mode bits */
    {"M=User",       0x0000001f, 0x00000010, 0},
    {"M=FIQ",        0x0000001f, 0x00000011, 0},
    {"M=IRQ",        0x0000001f, 0x00000012, 0},
    {"M=Supervisor", 0x0000001f, 0x00000013, 0},
    {"M=Abort",      0x0000001f, 0x00000017, 0},
    {"M=Undefined",  0x0000001f, 0x0000001b, 0},
    {"M=System",     0x0000001f, 0x0000001f, 0}
};

/** See arm_tracing.c. */
int get_user_regs(pid_t pid, struct user_regs *regs);

/** See arm_tracing.c. */
int get_user_fpregs(pid_t pid, struct user_fpregs *fpregs);

/** Print the general purpose registers. */
static void print_user_regs(struct user_regs *regs);

/** Print the cpsr register. */
static void print_cpsr(unsigned long cpsr);

/* See tracing.h. */
int print_registers(pid_t pid)
{
    struct user_regs regs;

    if (get_user_regs(pid, &regs))
        return 1;

    print_user_regs(&regs);
    printf("\n");
    print_cpsr(regs.ARM_cpsr);

    return 0;
}

/* See tracing.h. */
int print_general_purpose_registers(pid_t pid)
{
    struct user_regs regs;

    if (get_user_regs(pid, &regs))
        return 1;

    print_user_regs(&regs);

    return 0;
}

/* See tracing.h. */
int print_condition_code_registers(pid_t pid)
{
    struct user_regs regs;

    if (get_user_regs(pid, &regs))
        return 1;

    print_cpsr(regs.ARM_cpsr);

    return 0;
}

/* See tracing.h. */
int print_floating_point_registers(pid_t pid)
{
    fprintf(stderr, "Not yet implemented\n");
    return 1;
}

/* See tracing.h. */
int print_extra_registers(pid_t pid)
{
    fprintf(stderr, "Not yet implemented\n");
    return 1;
}

/* See tracing.h. */
int print_segment_registers(pid_t pid)
{
    fprintf(stderr, "Not yet implemented\n");
    return 1;
}

/* See above. */
static void print_user_regs(struct user_regs *regs)
{
    printf("r0  = 0x%08lx    r1 = 0x%08lx\n"
           "r2  = 0x%08lx    r3 = 0x%08lx\n"
           "r4  = 0x%08lx    r5 = 0x%08lx\n"
           "r6  = 0x%08lx    r7 = 0x%08lx\n"
           "r8  = 0x%08lx    r9 = 0x%08lx\n"
           "r10 = 0x%08lx    fp = 0x%08lx\n"
           "ip  = 0x%08lx    sp = 0x%08lx\n"
           "lr  = 0x%08lx    pc = 0x%08lx\n",
           regs->ARM_r0, regs->ARM_r1, regs->ARM_r2,  regs->ARM_r3, 
           regs->ARM_r4, regs->ARM_r5, regs->ARM_r6,  regs->ARM_r7, 
           regs->ARM_r8, regs->ARM_r9, regs->ARM_r10, regs->ARM_fp, 
           regs->ARM_ip, regs->ARM_sp, regs->ARM_lr,  regs->ARM_pc);
}

static void print_cpsr(unsigned long cpsr)
{
    printf("cpsr = 0x%08lx = ", cpsr);
    print_processor_flags(cpsr, cpsr_flags, NUM_FLAGS(cpsr_flags));
    printf("\n");
}
