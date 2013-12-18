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
#include "x86_support.h"

/* See x86_support.h. */
int get_user_regs(pid_t pid, struct user_regs_struct *regs)
{
    int error;
    if ((error = ptrace(PTRACE_GETREGS, pid, NULL, regs)) == -1) {
        perror("ptrace");
        return 1;
    }
    return 0;
}

static inline unsigned short x87_tag(unsigned char *raw)
{
    struct x87_float {
        unsigned long long fraction : 63;
        unsigned long long integer : 1;
        unsigned long long exponent : 15;
        unsigned long long sign : 1;
    } *fp = (struct x87_float *) raw;

    if (fp->exponent == 0x7fff)
        return 2; /* Special */
    else if (fp->exponent == 0x0000) {
        if (fp->fraction == 0 && !fp->integer)
            return 1; /* Zero */
        else
            return 2; /* Special */
    } else {
        if (fp->integer)
            return 0; /* Valid */
        else
            return 2; /* Special */
    }
}

/* See x86_support.h. */
int get_user_fpxregs(pid_t pid, struct user_fpxregs_struct *fpxregs)
{
    int error;
    unsigned char *st_space;
    unsigned short valid_bits, top;

    if ((error = ptrace(PTRACE_GETFPXREGS, pid, NULL, fpxregs)) == -1) {
        perror("ptrace");
        return 1;
    }

    /* Reconstruct tag word */
    st_space = (unsigned char *) fpxregs->st_space;
    top = X87_ST_TOP(fpxregs->swd);
    valid_bits = fpxregs->twd;
    fpxregs->twd = 0;

    for (unsigned short physical = 0; physical < 8; ++physical) {
        unsigned short tag;
        unsigned short logical = X87_PHYS_TO_LOG(physical, top);

        if (valid_bits & (1 << physical))
            tag = x87_tag(&st_space[16 * logical]);
        else
            tag = 0x3;
        fpxregs->twd |= tag << (2 * physical);
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
