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
