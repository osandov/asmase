#include <elf.h>
#include <stdio.h>
#include <string.h>

#include <sys/ptrace.h>
#include <sys/uio.h>
#include <sys/user.h>

#include <asm/processor-flags.h>

#include "tracing.h"

struct processor_flag {
    const char *name;
    unsigned long long mask;
};

struct processor_flag all_eflags[] = {
    {"CF",  X86_EFLAGS_CF},   {"PF",   X86_EFLAGS_PF},
    {"AF",  X86_EFLAGS_AF},   {"ZF",   X86_EFLAGS_ZF},
    {"SF",  X86_EFLAGS_SF},   {"TF",   X86_EFLAGS_TF},
    {"IF",  X86_EFLAGS_IF},   {"DF",   X86_EFLAGS_DF},
    {"OF",  X86_EFLAGS_OF},   {"IOPL", X86_EFLAGS_IOPL},
    {"NT",  X86_EFLAGS_NT},   {"RF",   X86_EFLAGS_RF},
    {"VM",  X86_EFLAGS_VM},   {"AC",   X86_EFLAGS_AC},
    {"VIF", X86_EFLAGS_VIF},  {"VIP",  X86_EFLAGS_VIP},
    {"ID",  X86_EFLAGS_ID}
};

struct processor_flag fpcr_flags[] = {
    {"RC=RN",  0x0000}, {"RC=R-",  0x2000},
    {"RC=R+",  0x4000}, {"RC=RZ",  0x6000},
    {"PC=SGL", 0x0000}, {"PC=DBL", 0x0080},
    {"PC=EXT", 0x00c0}, {"EM=PM",  0x0020},
    {"EM=UM",  0x0010}, {"EM=OM",  0x0008},
    {"EM=ZM",  0x0004}, {"EM=DM",  0x0002},
    {"EM=IM",  0x0001}
};

struct processor_flag fpsr_flags[] = {
    {"EF=IE", 0x0001}, {"EF=DE", 0x0002},
    {"EF=ZE", 0x0004}, {"EF=OE", 0x0008},
    {"EF=UE", 0x0010}, {"EF=PE", 0x0020},
    {"SF",    0x0040}, {"ES",    0x0080},
    {"C3",    0x4000}, {"C2",    0x0400},
    {"C1",    0x0200}, {"C0",    0x0100},
    {"B",     0x8000}, {"TOP",  0x3800}
};

static int get_user_regs(pid_t pid, struct user_regs_struct *regs)
{
    struct iovec iov = {.iov_base = regs, .iov_len = sizeof(*regs)};
    int error;
    if ((error = ptrace(PTRACE_GETREGSET, pid, NT_PRSTATUS, &iov)) == -1) {
        perror("ptrace");
        return 1;
    }
    return 0;
}

static int get_user_fpregs(pid_t pid, struct user_fpregs_struct *fpregs)
{
    struct iovec iov = {.iov_base = fpregs, .iov_len = sizeof(*fpregs)};
    int error;
    if ((error = ptrace(PTRACE_GETREGSET, pid, NT_FPREGSET, &iov)) == -1) {
        perror("ptrace");
        return 1;
    }
    return 0;
}

void *get_program_counter(pid_t pid)
{
    struct user_regs_struct regs;

    if (get_user_regs(pid, &regs))
        return NULL;

    return (void*) regs.rip;
}

int set_program_counter(pid_t pid, void *pc)
{
    struct user_regs_struct regs;
    struct iovec iov = {.iov_base = &regs, .iov_len = sizeof(regs)};

    if (get_user_regs(pid, &regs))
        return 1;

    regs.rip = (unsigned long) pc;

    return ptrace(PTRACE_SETREGSET, pid, NT_PRSTATUS, &iov);
}

static void print_user_regs(struct user_regs_struct *regs)
{
    printf("%%rax = 0x%016llx    %%rcx = 0x%016llx\n"
           "%%rdx = 0x%016llx    %%rbx = 0x%016llx\n"
           "%%rsp = 0x%016llx    %%rbp = 0x%016llx\n"
           "%%rsi = 0x%016llx    %%rdi = 0x%016llx\n"
           "%%r8  = 0x%016llx    %%r9  = 0x%016llx\n"
           "%%r10 = 0x%016llx    %%r11 = 0x%016llx\n"
           "%%r12 = 0x%016llx    %%r13 = 0x%016llx\n"
           "%%r14 = 0x%016llx    %%r15 = 0x%016llx\n",
           regs->rax, regs->rcx, regs->rdx, regs->rbx,
           regs->rsp, regs->rbp, regs->rsi, regs->rdi,
           regs->r8, regs->r9, regs->r10, regs->r11,
           regs->r12, regs->r13, regs->r14, regs->r15);
}

static void print_eflags(unsigned long long eflags)
{
    printf("%%eflags = 0x%08llx = [", eflags);
    for (size_t i = 0; i < sizeof(all_eflags) / sizeof(*all_eflags); ++i) {
        struct processor_flag *flag = &all_eflags[i];
        if (strcmp(flag->name, "IOPL") == 0) { /* Ugly special case. */
            if (eflags & flag->mask)
                    printf(" IOPL=%llu", (eflags & flag->mask) >> 12);
        } else {
            if (eflags & flag->mask)
                    printf(" %s", flag->name);
        }
    }
    printf(" ]\n");
}

static void print_user_fpregs(struct user_fpregs_struct *fpregs)
{
    printf("%%fpcr = 0x%04hx = [", fpregs->cwd);
    for (size_t i = 0; i < sizeof(fpcr_flags) / sizeof(*fpcr_flags); ++i) {
        struct processor_flag *flag = &fpcr_flags[i];
        if (fpregs->cwd & flag->mask)
                printf(" %s", flag->name);
    }
    printf(" ]\n");

    printf("%%fpsr = 0x%04hx = [", fpregs->swd);
    for (size_t i = 0; i < sizeof(fpsr_flags) / sizeof(*fpsr_flags); ++i) {
        struct processor_flag *flag = &fpsr_flags[i];
        if (strcmp(flag->name, "TOP") == 0) /* Ugly special case. */
            printf(" TOP=%llu", ((fpregs->swd & flag->mask) >> 11));
        else if (fpregs->swd & flag->mask)
                    printf(" %s", flag->name);
    }
    printf(" ]\n");

    printf("\n");
    for (int i = 0; i < 8; ++i) {
        long double st;
        if (i > 0) {
            if (i % 2 == 0)
                printf("\n");
        }
        st = *((long double *) &fpregs->st_space[4 * i]);
        printf("%%st(%d) = %-16LF", i, st);
    }
    printf("\n");
}

static void print_user_fpxregs(struct user_fpregs_struct *fpregs)
{
    for (int i = 0; i < 8; ++i) {
        unsigned long long mm;
        if (i > 0) {
            if (i % 2 == 0)
                printf("\n");
            else
                printf("     ");
        }
        mm = *((unsigned long long *) &fpregs->st_space[4 * i]);
        printf("%%mm%d = 0x%016llx", i, mm);
    }
    printf("\n");

    printf("\n");
    for (int i = 0; i < 16; ++i) {
        unsigned long long xmmh, xmml;
        xmmh = *((unsigned long long *) &fpregs->xmm_space[4 * i + 2]);
        xmml = *((unsigned long long *) &fpregs->xmm_space[4 * i]);
        printf("%%xmm%-2d = 0x%016llx%016llx\n", i, xmmh, xmml);
    }
}

int print_registers(pid_t pid)
{
    struct user_regs_struct regs;

    if (get_user_regs(pid, &regs))
        return 1;

    print_user_regs(&regs);

    printf("\n");
    printf("%%rip = 0x%016llx\n", regs.rip);

    printf("\n");
    print_eflags(regs.eflags);

    return 0;
}

int print_general_purpose_registers(pid_t pid)
{
    struct user_regs_struct regs;

    if (get_user_regs(pid, &regs))
        return 1;

    print_user_regs(&regs);

    return 0;
}

int print_condition_code_registers(pid_t pid)
{
    struct user_regs_struct regs;

    if (get_user_regs(pid, &regs))
        return 1;

    print_eflags(regs.eflags);

    return 0;
}

int print_floating_point_registers(pid_t pid)
{
    struct user_fpregs_struct fpregs;

    if (get_user_fpregs(pid, &fpregs))
        return 1;

    print_user_fpregs(&fpregs);

    return 1;
}

int print_extra_registers(pid_t pid)
{
    struct user_fpregs_struct fpregs;

    if (get_user_fpregs(pid, &fpregs))
        return 1;

    print_user_fpxregs(&fpregs);

    return 1;
}

int print_segment_registers(pid_t pid)
{
    struct user_regs_struct regs;

    if (get_user_regs(pid, &regs))
        return 1;

    printf("%%ss = 0x%04llx    %%cs = 0x%04llx\n", regs.ss, regs.cs);

    return 1;
}
