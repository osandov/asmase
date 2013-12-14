#include <elf.h>
#include <stdio.h>

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
