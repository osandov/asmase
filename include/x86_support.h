#ifndef ASMASE_X86_SUPPORT
#define ASMASE_X86_SUPPORT

#if defined(__x86_64__)

/* Hacks to allow more code reuse between x86 and x86-64 */
#define user_fpxregs_struct user_fpregs_struct
#define twd ftw
#define PTRACE_GETFPXREGS PTRACE_GETFPREGS

#define NUM_SSE_REGS 16

#elif defined(__i386__)
#define NUM_SSE_REGS 8

#else
#error "Unknown x86 variant"
#endif

/** Get the TOP field from an x87 status word. */
#define X87_ST_TOP(swd) (((swd) & 0x3800) >> 11)

/**
 * Convert a physical floating-point register number to logical (i.e., %st(x))
 * register.
 */
#define X87_PHYS_TO_LOG(x, top) (((x) - (top) + 8) % 8)

/**
 * Convert a logical floating-point register number to the physical register
 * number.
 */
#define X87_LOG_TO_PHYS(x, top) (((x) + (top)) % 8)

/**
 * Get the user register structure for a stopped, ptraced process. For x86 and
 * x86_64, this includes the general-purpose registers, instruction pointer,
 * flag register, and segment registers.
 * @return Zero on success, nonzero on failure.
 */
int get_user_regs(pid_t pid, struct user_regs_struct *regs);

/**
 * Get the user floating point/extra register structure for a stopped, ptraced
 * process. For x86 and x86_64, this includes the floating point stack/MMX
 * registers and SSE registers.
 * @return Zero on success, nonzero on failure.
 */
int get_user_fpxregs(pid_t pid, struct user_fpxregs_struct *fpxregs);

#endif /* ASMASE_X86_SUPPORT */
