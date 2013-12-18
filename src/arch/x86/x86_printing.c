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

#include "flags.h"
#include "tracing.h"
#include "x86_support.h"

/** Flags in the eflags registers. */
static struct processor_flag eflags_flags[] = {
    BIT_FLAG("CF", X86_EFLAGS_CF), /* Carry flag */
    BIT_FLAG("PF", X86_EFLAGS_PF), /* Parity flag */
    BIT_FLAG("AF", X86_EFLAGS_AF), /* Adjust flag */
    BIT_FLAG("ZF", X86_EFLAGS_ZF), /* Zero flag */
    BIT_FLAG("SF", X86_EFLAGS_SF), /* Sign flag */
    BIT_FLAG("TF", X86_EFLAGS_TF), /* Trap flag */
    BIT_FLAG("IF", X86_EFLAGS_IF), /* Interruption flag */
    BIT_FLAG("DF", X86_EFLAGS_DF), /* Direction flag */
    BIT_FLAG("OF", X86_EFLAGS_OF), /* Overflow flag */
    {"IOPL", X86_EFLAGS_IOPL, 0x0, 1}, /* I/O privilege level */
    BIT_FLAG("NT",  X86_EFLAGS_NT), /* Nested task flag */
    BIT_FLAG("RF",  X86_EFLAGS_RF), /* Resume flag */
    BIT_FLAG("VM",  X86_EFLAGS_VM), /* Virtual-8086 mode */
    BIT_FLAG("AC",  X86_EFLAGS_AC), /* Alignment check */
    BIT_FLAG("VIF", X86_EFLAGS_VIF), /* Virtual interrupt flag */
    BIT_FLAG("VIP", X86_EFLAGS_VIP), /* Virtual interrupt pending flag */
    BIT_FLAG("ID",  X86_EFLAGS_ID) /* Identification flag */
};

/** Flags in the fpcr register (floating point control register). */
static struct processor_flag fpcr_flags[] = {
    /* Rounding mode */
    {"RC=RN", 0x6000, 0x0, 0}, /* Round to nearest */
    {"RC=R-", 0x6000, 0x1, 0}, /* Round toward negative infinity */
    {"RC=R+", 0x6000, 0x2, 0}, /* Round toward positive infinity */
    {"RC=RZ", 0x6000, 0x3, 0}, /* Round toward zero */

    /* Rounding precision */
    {"PC=SGL", 0x00c0, 0x0, 0}, /* Single-precision */
    {"PC=DBL", 0x00c0, 0x2, 0}, /* Double-precision */
    {"PC=EXT", 0x00c0, 0x3, 0}, /* Extended-precision */

    /* Exception enables */
    BIT_FLAG("EM=PM", 0x0020), /* Precision */
    BIT_FLAG("EM=UM", 0x0010), /* Underflow */
    BIT_FLAG("EM=OM", 0x0008), /* Overflow */
    BIT_FLAG("EM=ZM", 0x0004), /* Zero-divide */
    BIT_FLAG("EM=DM", 0x0002), /* Denormalized operand */
    BIT_FLAG("EM=IM", 0x0001) /* Invalid operation */
};

/** Flags in the fpsr register (floating point status register). */
static struct processor_flag fpsr_flags[] = {
    {"TOP", 0x3800, 0x8, 1}, /* Top of the floating-point stack */

    BIT_FLAG("B", 0x8000), /* FPU busy */
    
    /* Condition bits */
    BIT_FLAG("C0", 0x0100),
    BIT_FLAG("C1", 0x0200),
    BIT_FLAG("C2", 0x0400),
    BIT_FLAG("C3", 0x4000),

    BIT_FLAG("ES", 0x0080), /* Exception summary status */
    BIT_FLAG("SF", 0x0040), /* Stack fault */

    /* Exceptions */
    BIT_FLAG("EF=PE", 0x0020), /* Precision */
    BIT_FLAG("EF=UE", 0x0010), /* Underflow */
    BIT_FLAG("EF=OE", 0x0008), /* Overflow */
    BIT_FLAG("EF=ZE", 0x0004), /* Zero-divide */
    BIT_FLAG("EF=DE", 0x0002), /* Denormalized operand */
    BIT_FLAG("EF=IE", 0x0001) /* Invalid operation */
};

/** Flags in the mxcsr register (SSE control/status register). */
static struct processor_flag mxcsr_flags[] = {
    BIT_FLAG("FZ",    0x8000), /* Flush to zero */

    /* Rounding mode */
    {"RC=RN", 0x6000, 0x0, 0},
    {"RC=R-", 0x6000, 0x1, 0},
    {"RC=R+", 0x6000, 0x2, 0},
    {"RC=RZ", 0x6000, 0x3, 0},

    /* Exception enables */
    BIT_FLAG("EM=PM", 0x1000), /* Precision */
    BIT_FLAG("EM=UM", 0x0800), /* Underflow */
    BIT_FLAG("EM=OM", 0x0400), /* Overflow */
    BIT_FLAG("EM=ZM", 0x0200), /* Zero-divide */
    BIT_FLAG("EM=DM", 0x0100), /* Denormalized operand */
    BIT_FLAG("EM=IM", 0x0080), /* Invalid operation */

    BIT_FLAG("DAZ",   0x0040), /* Denormals are zero */

    /* Exceptions */
    BIT_FLAG("EF=PE", 0x0020), /* Precision */
    BIT_FLAG("EF=UE", 0x0010), /* Underflow */
    BIT_FLAG("EF=OE", 0x0008), /* Overflow */
    BIT_FLAG("EF=ZE", 0x0004), /* Zero-divide */
    BIT_FLAG("EF=DE", 0x0002), /* Denormalized operand */
    BIT_FLAG("EF=IE", 0x0001) /* Invalid operation */
};

/** Print the general purpose registers. */
static void print_user_regs(struct user_regs_struct *regs);

/** Print the floating point context. */
static void print_user_fpregs(struct user_fpxregs_struct *fpxregs);

/** Print the extra registers from the floating point structure. */
static void print_user_fpxregs(struct user_fpxregs_struct *fpxregs);

/** Print the eflags register. */
static void print_eflags(unsigned long long eflags);

/* See tracing.h. */
int print_registers(pid_t pid)
{
    struct user_regs_struct regs;

    if (get_user_regs(pid, &regs))
        return 1;

    print_user_regs(&regs);
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
           "%%esi = 0x%08lx    %%edi = 0x%08lx\n"
           "%%eip = 0x%08lx\n",
           regs->eax, regs->ecx, regs->edx, regs->ebx,
           regs->esp, regs->ebp, regs->esi, regs->edi,
           regs->eip);
#elif defined(__x86_64__)
    printf("%%rax = 0x%016llx    %%rcx = 0x%016llx\n"
           "%%rdx = 0x%016llx    %%rbx = 0x%016llx\n"
           "%%rsp = 0x%016llx    %%rbp = 0x%016llx\n"
           "%%rsi = 0x%016llx    %%rdi = 0x%016llx\n"
           "%%r8  = 0x%016llx    %%r9  = 0x%016llx\n"
           "%%r10 = 0x%016llx    %%r11 = 0x%016llx\n"
           "%%r12 = 0x%016llx    %%r13 = 0x%016llx\n"
           "%%r14 = 0x%016llx    %%r15 = 0x%016llx\n"
           "%%rip = 0x%016llx\n",
           regs->rax, regs->rcx, regs->rdx, regs->rbx,
           regs->rsp, regs->rbp, regs->rsi, regs->rdi,
           regs->r8,  regs->r9,  regs->r10, regs->r11,
           regs->r12, regs->r13, regs->r14, regs->r15,
           regs->rip);
#endif
}

/* See above. */
static void print_user_fpregs(struct user_fpxregs_struct *fpxregs)
{
    unsigned char *st_space = (unsigned char *) fpxregs->st_space;
    unsigned short top = X87_ST_TOP(fpxregs->swd);

    printf("fcw = 0x%04hx = ", fpxregs->cwd);
    print_processor_flags(fpxregs->cwd, fpcr_flags, NUM_FLAGS(fpcr_flags));
    printf("\n");

    printf("fsw = 0x%04hx = ", fpxregs->swd);
    print_processor_flags(fpxregs->swd, fpsr_flags, NUM_FLAGS(fpsr_flags));
    printf("\n");

    printf("ftw = 0x%04hx\n", fpxregs->twd);

    for (short physical = 7; physical >= 0; --physical) {
        unsigned short logical = X87_PHYS_TO_LOG(physical, top);
        long double fp = *((long double *) &st_space[16 * logical]);
        unsigned short tag =
            (fpxregs->twd & (0x3 << 2 * physical)) >> 2 * physical;

        printf("\n");

        printf("R%hd = ", physical);
#define FPFMT "%20.18LF"
        switch (tag) {
            case 0x0:
                printf("(valid)   " FPFMT, fp);
                break;
            case 0x1:
                printf("(zero)    " FPFMT, fp);
                break;
            case 0x2:
                printf("(special) " FPFMT, fp);
                break;
            case 0x3:
                printf("(empty)");
                break;
        }
#undef FPFMT

        if (tag != 0x3)
            printf(", %%st(%d)", logical);
    }
    printf("\n");

    printf("\n");
#if defined(__x86_64__)
    printf("fip = 0x%016llx    fdp = 0x%016llx\n", fpxregs->rip, fpxregs->rdp);
#elif defined(__i386__)
    printf("fip = 0x%04lx:0x%08lx    fdp = 0x%04lx:0x%08lx\n",
           fpxregs->fcs, fpxregs->fip, fpxregs->fos, fpxregs->foo);
#endif
    printf("fop = 0x%04x\n", fpxregs->fop & 0x7ff);
}

/* See above. */
static void print_user_fpxregs(struct user_fpxregs_struct *fpxregs)
{
    unsigned char *st_space = (unsigned char *) fpxregs->st_space;
    unsigned char *xmm_space = (unsigned char *) fpxregs->xmm_space;
    unsigned int mxcsr = fpxregs->mxcsr;

    printf("mxcsr = 0x%08x = ", mxcsr);
    print_processor_flags(mxcsr, mxcsr_flags, NUM_FLAGS(mxcsr_flags));
    printf("\n");

    for (int i = 0; i < 8; ++i) {
        unsigned long long mm;
        if (i % 2 == 0)
            printf("\n");
        else
            printf("     ");
        mm = *((unsigned long long *) &st_space[16 * i]);
        printf("%%mm%d = 0x%016llx", i, mm);
    }
    printf("\n");

    printf("\n");
    for (int i = 0; i < NUM_SSE_REGS; ++i) {
        unsigned long long xmmh, xmml;
        xmmh = *((unsigned long long *) &xmm_space[16 * i + 8]);
        xmml = *((unsigned long long *) &xmm_space[16 * i]);
        printf("%%xmm%-2d = 0x%016llx%016llx\n", i, xmmh, xmml);
    }
}

/* See above. */
static void print_eflags(unsigned long long eflags)
{
    printf("eflags = 0x%08llx = ", eflags);
    print_processor_flags(eflags, eflags_flags, NUM_FLAGS(eflags_flags));
    printf("\n");
}
