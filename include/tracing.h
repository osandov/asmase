#ifndef ASIP_EXECUTION_H
#define ASIP_EXECUTION_H

#include <sys/types.h>

/** Handle for a tracee. */
struct tracee_info {
    pid_t pid;
    void *shared_page;
};

/**
 * Create the process to trace and torture, filling in the tracee information
 * (its PID and the shared executable page).
 * @return Zero on succes, nonzero on failure.
 */
int create_tracee(struct tracee_info *tracee);

/** Do any cleanup after creating a tracee. */
void cleanup_tracing(struct tracee_info *tracee);

/** Execute the given machine code on the tracee. */
int execute_instruction(struct tracee_info *tracee, unsigned char *mc_buffer,
                        size_t mc_length);

/**
 * Return the current program counter (a.k.a., instruction pointer) for a
 * stopped, ptraced process.
 * @return The program counter on success or NULL on failure.
 */
void *get_program_counter(pid_t pid);

/**
 * Set the program counter (a.k.a., instruction pointer) for a stopped, ptraced
 * process.
 * @return Zero on success, nonzero on failure.
 */
int set_program_counter(pid_t pid, void *pc);

/**
 * Print the `normal' registers for a stopped, ptraced process. This includes
 * the general-purpose registers and other, architecture-dependent registers.
 * @return Zero on success, nonzero on failure.
 */
int print_registers(pid_t pid);

/**
 * Print the general-purpose registers for a stopped, ptraced process.
 * @return Zero on success, nonzero on failure.
 */
int print_general_purpose_registers(pid_t pid);

/**
 * Print the condition codes (a.k.a., status flags) for a stopped, ptraced
 * process.
 * @return Zero on success, nonzero on failure.
 */
int print_condition_code_registers(pid_t pid);

/**
 * Print the floating point registers for a stopped, ptraced process.
 * @return Zero on success, nonzero on failure.
 */
int print_floating_point_registers(pid_t pid);

/**
 * Print the architecture-dependent extra registers for a stopped, ptraced
 * process.
 * @return Zero on success, nonzero on failure.
 */
int print_extra_registers(pid_t pid);

/**
 * Print the segment registers for a stopped, ptraced process.
 * @return Zero on success, nonzero on failure.
 */
int print_segment_registers(pid_t pid);

#endif /* ASIP_EXECUTION_H */
