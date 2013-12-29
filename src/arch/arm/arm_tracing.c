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

#include "tracing.h"

/**
 * Get the user register structure for a stopped, ptraced process. For ARM,
 * this includes the general-purpose registers and program status registers.
 * @return Zero on success, nonzero on failure.
 */
int get_user_regs(pid_t pid, struct user_regs *regs)
{
    int error;
    if ((error = ptrace(PTRACE_GETREGS, pid, NULL, regs)) == -1) {
        perror("ptrace");
        return 1;
    }
    return 0;
}

int get_user_fpregs(pid_t pid, struct user_fpregs *fpregs)
{
    int error;
    if ((error = ptrace(PTRACE_GETFPREGS, pid, NULL, fpregs)) == -1) {
        perror("ptrace");
        return 1;
    }
    return 0;
}

/* See tracing.h. */
int generate_sigtrap(unsigned char *buffer, size_t n)
{
    static const char eabi_interrupt[] = {0xf0, 0x01, 0xf0, 0xe7};
    if (n < sizeof(eabi_interrupt)) {
        fprintf(stderr, "Not enough space for trap\n");
        return 1;
    }

    memcpy(buffer, eabi_interrupt, sizeof(eabi_interrupt));

    return 0;
}

/* See tracing.h. */
void *get_program_counter(pid_t pid)
{
    struct user_regs regs;

    if (get_user_regs(pid, &regs))
        return NULL;

    return (void*) regs.ARM_pc;
}

/* See tracing.h. */
int set_program_counter(pid_t pid, void *pc)
{
    struct user_regs regs;
    int error;

    if (get_user_regs(pid, &regs))
        return 1;

    regs.ARM_pc = (unsigned long) pc;

    if ((error = ptrace(PTRACE_SETREGS, pid, NULL, &regs)) == -1) {
        perror("ptrace");
        return 1;
    }

    return 0;
}

/* See tracing.h. */
int get_register_value(pid_t pid, const char *reg_name,
                       struct register_value *val_out)
{
#define INTEGER_REGISTER(name, member) \
    if (strcmp(reg_name, (name)) == 0) { \
        val_out->type = REGISTER_INTEGER; \
        val_out->integer = regs.member; \
        return 0; \
    }

    struct user_regs regs;

    if (get_user_regs(pid, &regs))
        return 1;

    INTEGER_REGISTER("r0", ARM_r0); INTEGER_REGISTER("r1", ARM_r1);
    INTEGER_REGISTER("r2", ARM_r2); INTEGER_REGISTER("r3", ARM_r3);
    INTEGER_REGISTER("r4", ARM_r4); INTEGER_REGISTER("r5", ARM_r5);
    INTEGER_REGISTER("r6", ARM_r6); INTEGER_REGISTER("r7", ARM_r7);
    INTEGER_REGISTER("r8", ARM_r8); INTEGER_REGISTER("r9", ARM_r9);
    INTEGER_REGISTER("r10", ARM_r10);

    INTEGER_REGISTER("r11", ARM_fp); INTEGER_REGISTER("fp", ARM_fp);
    INTEGER_REGISTER("r12", ARM_ip); INTEGER_REGISTER("ip", ARM_ip);
    INTEGER_REGISTER("r13", ARM_sp); INTEGER_REGISTER("sp", ARM_sp);
    INTEGER_REGISTER("r14", ARM_lr); INTEGER_REGISTER("lr", ARM_lr);
    INTEGER_REGISTER("r15", ARM_pc); INTEGER_REGISTER("pc", ARM_pc);

    INTEGER_REGISTER("cpsr", ARM_cpsr);

    return 1;
#undef INTEGER_REGISTER
}
