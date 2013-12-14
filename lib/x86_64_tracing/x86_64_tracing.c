#include <elf.h>
#include <stdio.h>

#include <sys/ptrace.h>
#include <sys/uio.h>
#include <sys/user.h>

#include "tracing.h"

/**
 * Get the user register structure for a stopped, ptraced process. For x86_64,
 * this includes the general-purpose registers, instruction pointer, flag
 * register, and segment registers.
 * @return Zero on succes, nonzero on failure.
 */
int get_user_regs(pid_t pid, struct user_regs_struct *regs)
{
    struct iovec iov = {.iov_base = regs, .iov_len = sizeof(*regs)};
    int error;
    if ((error = ptrace(PTRACE_GETREGSET, pid, NT_PRSTATUS, &iov)) == -1) {
        perror("ptrace");
        return 1;
    }
    return 0;
}

/**
 * Get the user floating point/extra register structure for a stopped, ptraced
 * process. For x86_64, this includes the floating point stack/MMX registers
 * and SSE registers.
 * @return Zero on succes, nonzero on failure.
 */
int get_user_fpregs(pid_t pid, struct user_fpregs_struct *fpregs)
{
    struct iovec iov = {.iov_base = fpregs, .iov_len = sizeof(*fpregs)};
    int error;
    if ((error = ptrace(PTRACE_GETREGSET, pid, NT_FPREGSET, &iov)) == -1) {
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

    return (void*) regs.rip;
}

/* See tracing.h. */
int set_program_counter(pid_t pid, void *pc)
{
    struct user_regs_struct regs;
    struct iovec iov = {.iov_base = &regs, .iov_len = sizeof(regs)};

    if (get_user_regs(pid, &regs))
        return 1;

    regs.rip = (unsigned long) pc;

    return ptrace(PTRACE_SETREGSET, pid, NT_PRSTATUS, &iov);
}
