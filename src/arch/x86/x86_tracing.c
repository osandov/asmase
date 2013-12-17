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

#include "tracing.h"

#if defined(__x86_64__)
#define user_fpxregs_struct user_fpregs_struct
#define PTRACE_GETFPXREGS PTRACE_GETFPREGS
#elif !defined(__i386__)
#error "Unknown x86 variant"
#endif

/**
 * Get the user register structure for a stopped, ptraced process. For x86 and
 * x86_64, this includes the general-purpose registers, instruction pointer,
 * flag register, and segment registers.
 * @return Zero on succes, nonzero on failure.
 */
int get_user_regs(pid_t pid, struct user_regs_struct *regs)
{
    int error;
    if ((error = ptrace(PTRACE_GETREGS, pid, NULL, regs)) == -1) {
        perror("ptrace");
        return 1;
    }
    return 0;
}

/**
 * Get the user floating point/extra register structure for a stopped, ptraced
 * process. For x86 and x86_64, this includes the floating point stack/MMX
 * registers and SSE registers.
 * @return Zero on succes, nonzero on failure.
 */
int get_user_fpxregs(pid_t pid, struct user_fpxregs_struct *fpxregs)
{
    int error;
    if ((error = ptrace(PTRACE_GETFPXREGS, pid, NULL, fpxregs)) == -1) {
        perror("ptrace");
        return 1;
    }
    return 0;
}

/* See tracing.h. */
int generate_sigtrap(unsigned char *buffer, size_t n)
{
    static const char software_interrupt[] = {0xcc}; /* int $0x3 */

    if (n < sizeof(software_interrupt)) {
        fprintf(stderr, "Not enough space for int $0x3\n");
        return 1;
    }

    memcpy(buffer, software_interrupt, sizeof(software_interrupt));

    return 0;
}

/* See tracing.h. */
void *get_program_counter(pid_t pid)
{
    struct user_regs_struct regs;

    if (get_user_regs(pid, &regs))
        return NULL;

#if defined(__i386__)
    return (void*) regs.eip;
#elif defined(__x86_64__)
    return (void*) regs.rip;
#endif
}

/* See tracing.h. */
int set_program_counter(pid_t pid, void *pc)
{
    struct user_regs_struct regs;
    int error;

    if (get_user_regs(pid, &regs))
        return 1;

#if defined(__i386__)
    regs.eip = (unsigned long) pc;
#elif defined(__x86_64__)
    regs.rip = (unsigned long long) pc;
#endif

    if ((error = ptrace(PTRACE_SETREGS, pid, NULL, &regs)) == -1) {
        perror("ptrace");
        return 1;
    }
    return 0;
}
