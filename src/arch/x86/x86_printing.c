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

#include <asm/processor-flags.h>

#include "tracing.h"

#if defined(__x86_64__)
#define user_fpxregs_struct user_fpregs_struct
#define NUM_SSE_REGS 16
#elif defined(__i386__)
#define NUM_SSE_REGS 8
#else
#error "Unknown x86 variant"
#endif

/** See x86_tracing.c. */
int get_user_regs(pid_t pid, struct user_regs_struct *regs);

/** See x86_tracing.c. */
int get_user_fpxregs(pid_t pid, struct user_fpxregs_struct *fpxregs);

/** Print the general purpose registers. */
static void print_user_regs(struct user_regs_struct *regs);

/** Print the floating point context. */
static void print_user_fpregs(struct user_fpxregs_struct *fpxregs);

/** Print the extra registers from the floating point structure. */
static void print_user_fpxregs(struct user_fpxregs_struct *fpxregs);

/** Print the eflags register. */
static void print_eflags(unsigned long long eflags);

/** Flag in a processor register. */
struct processor_flag {
    /** The name of the flag. */
    const char *name;

    /** Mask in the register. */
    unsigned long long mask;
};

/** Flags in the eflags registers. */
struct processor_flag all_eflags[] = {
    {"CF",  X86_EFLAGS_CF},  {"PF",   X86_EFLAGS_PF},
    {"AF",  X86_EFLAGS_AF},  {"ZF",   X86_EFLAGS_ZF},
    {"SF",  X86_EFLAGS_SF},  {"TF",   X86_EFLAGS_TF},
    {"IF",  X86_EFLAGS_IF},  {"DF",   X86_EFLAGS_DF},
    {"OF",  X86_EFLAGS_OF},  {"IOPL", X86_EFLAGS_IOPL},
    {"NT",  X86_EFLAGS_NT},  {"RF",   X86_EFLAGS_RF},
    {"VM",  X86_EFLAGS_VM},  {"AC",   X86_EFLAGS_AC},
    {"VIF", X86_EFLAGS_VIF}, {"VIP",  X86_EFLAGS_VIP},
    {"ID",  X86_EFLAGS_ID}
};

/** Flags in the fpcr register (floating point control register). */
struct processor_flag fpcr_flags[] = {
    {"RC=RN",  0x0000}, {"RC=R-",  0x2000},
    {"RC=R+",  0x4000}, {"RC=RZ",  0x6000},
    {"PC=SGL", 0x0000}, {"PC=DBL", 0x0080},
    {"PC=EXT", 0x00c0}, {"EM=PM",  0x0020},
    {"EM=UM",  0x0010}, {"EM=OM",  0x0008},
    {"EM=ZM",  0x0004}, {"EM=DM",  0x0002},
    {"EM=IM",  0x0001}
};

/** Flags in the fpsr register (floating point status register). */
struct processor_flag fpsr_flags[] = {
    {"EF=IE", 0x0001}, {"EF=DE", 0x0002},
    {"EF=ZE", 0x0004}, {"EF=OE", 0x0008},
    {"EF=UE", 0x0010}, {"EF=PE", 0x0020},
    {"SF",    0x0040}, {"ES",    0x0080},
    {"C3",    0x4000}, {"C2",    0x0400},
    {"C1",    0x0200}, {"C0",    0x0100},
    {"B",     0x8000}, {"TOP",   0x3800}
};

/* See tracing.h. */
int print_registers(pid_t pid)
{
    struct user_regs_struct regs;

    if (get_user_regs(pid, &regs))
        return 1;

    print_user_regs(&regs);

    printf("\n");
#if defined(__i386__)
    printf("%%eip = 0x%08lx\n", regs.eip);
#elif defined(__x86_64__)
    printf("%%rip = 0x%016llx\n", regs.rip);
#endif

    printf("\n");
    print_eflags(regs.eflags);

    return 0;
}

/* See tracing.h. */
int print_general_purpose_registers(pid_t pid)
{
    struct user_regs_struct regs;

    if (get_user_regs(pid, &regs))
        return 1;

    print_user_regs(&regs);

    return 0;
}

/* See tracing.h. */
int print_condition_code_registers(pid_t pid)
{
    struct user_regs_struct regs;

    if (get_user_regs(pid, &regs))
        return 1;

    print_eflags(regs.eflags);

    return 0;
}

/* See tracing.h. */
int print_floating_point_registers(pid_t pid)
{
    struct user_fpxregs_struct fpxregs;

    if (get_user_fpxregs(pid, &fpxregs))
        return 1;

    print_user_fpregs(&fpxregs);

    return 1;
}

/* See tracing.h. */
int print_extra_registers(pid_t pid)
{
    struct user_fpxregs_struct fpxregs;

    if (get_user_fpxregs(pid, &fpxregs))
        return 1;

    print_user_fpxregs(&fpxregs);

    return 1;
}

/* See tracing.h. */
int print_segment_registers(pid_t pid)
{
    struct user_regs_struct regs;

    if (get_user_regs(pid, &regs))
        return 1;

#if defined(__i386__)
    printf("%%ss = 0x%04lx    %%cs = 0x%04lx\n"
           "%%ds = 0x%04lx    %%es = 0x%04lx\n"
           "%%fs = 0x%04lx    %%gs = 0x%04lx\n",
           regs.xss, regs.xcs, regs.xds, regs.xes,
           regs.xfs, regs.xgs);
#elif defined(__x86_64__)
    printf("%%ss = 0x%04llx    %%cs = 0x%04llx\n"
           "%%ds = 0x%04llx    %%es = 0x%04llx\n"
           "%%fs = 0x%04llx    %%gs = 0x%04llx\n"
           "fs.base = 0x%016llx\n"
           "gs.base = 0x%016llx\n",
           regs.ss, regs.cs, regs.ds, regs.es,
           regs.fs, regs.gs, regs.fs_base, regs.gs_base);
#endif

    return 1;
}

/* See above. */
static void print_user_regs(struct user_regs_struct *regs)
{
#if defined(__i386__)
    printf("%%eax = 0x%08lx    %%ecx = 0x%08lx\n"
           "%%edx = 0x%08lx    %%ebx = 0x%08lx\n"
           "%%esp = 0x%08lx    %%ebp = 0x%08lx\n"
           "%%esi = 0x%08lx    %%edi = 0x%08lx\n",
           regs->eax, regs->ecx, regs->edx, regs->ebx,
           regs->esp, regs->ebp, regs->esi, regs->edi);
#elif defined(__x86_64__)
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
           regs->r8,  regs->r9,  regs->r10, regs->r11,
           regs->r12, regs->r13, regs->r14, regs->r15);
#endif
}

/* See above. */
static void print_user_fpregs(struct user_fpxregs_struct *fpxregs)
{
    printf("%%fpcr = 0x%04hx = [", fpxregs->cwd);
    for (size_t i = 0; i < sizeof(fpcr_flags) / sizeof(*fpcr_flags); ++i) {
        struct processor_flag *flag = &fpcr_flags[i];
        if (fpxregs->cwd & flag->mask)
                printf(" %s", flag->name);
    }
    printf(" ]\n");

    printf("%%fpsr = 0x%04hx = [", fpxregs->swd);
    for (size_t i = 0; i < sizeof(fpsr_flags) / sizeof(*fpsr_flags); ++i) {
        struct processor_flag *flag = &fpsr_flags[i];
        if (strcmp(flag->name, "TOP") == 0) /* Ugly special case. */
            printf(" TOP=%llu", ((fpxregs->swd & flag->mask) >> 11));
        else if (fpxregs->swd & flag->mask)
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
        st = *((long double *) &fpxregs->st_space[4 * i]);
        printf("%%st(%d) = %-16LF", i, st);
    }
    printf("\n");
}

/* See above. */
static void print_user_fpxregs(struct user_fpxregs_struct *fpxregs)
{
    for (int i = 0; i < 8; ++i) {
        unsigned long long mm;
        if (i > 0) {
            if (i % 2 == 0)
                printf("\n");
            else
                printf("     ");
        }
        mm = *((unsigned long long *) &fpxregs->st_space[4 * i]);
        printf("%%mm%d = 0x%016llx", i, mm);
    }
    printf("\n");

    printf("\n");
    for (int i = 0; i < NUM_SSE_REGS; ++i) {
        unsigned long long xmmh, xmml;
        xmmh = *((unsigned long long *) &fpxregs->xmm_space[4 * i + 2]);
        xmml = *((unsigned long long *) &fpxregs->xmm_space[4 * i]);
        printf("%%xmm%-2d = 0x%016llx%016llx\n", i, xmmh, xmml);
    }
}

/* See above. */
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
