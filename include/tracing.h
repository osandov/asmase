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

#ifndef ASMASE_TRACING_H
#define ASMASE_TRACING_H

#include <sys/types.h>

/** Handle for a tracee. */
struct tracee_info {
    pid_t pid;
    void *shared_page;
};

enum register_value_type {
    REGISTER_INTEGER,
    REGISTER_FLOATING
};

struct register_value {
    enum register_value_type type;

    union {
        long integer;
        double floating;
    };
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create the process to trace and torture, filling in the tracee information
 * (its PID and the shared executable page).
 * @return Zero on success, nonzero on failure.
 */
int create_tracee(struct tracee_info *tracee);

/** Do any cleanup after creating a tracee. */
void cleanup_tracing(struct tracee_info *tracee);

/** Execute the given machine code on the tracee. */
int execute_instruction(struct tracee_info *tracee, unsigned char *mc_buffer,
                        size_t mc_length);

/**
 * Inject machine code to generate a SIGTRAP at the given buffer.
 * @param buffer Address at which to put the machine code.
 * @param n The size of the buffer.
 * @return Zero on success, nonzero on failure (e.g., buffer not big enough).
 */
int generate_sigtrap(unsigned char *buffer, size_t n);

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
 * Gets the value stored in a given register.
 * @return Zero on success, nonzero on failure.
 */
int get_register_value(pid_t pid, const char *reg_name,
                       struct register_value *val_out);

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

#ifdef __cplusplus
}
#endif

#endif /* ASMASE_TRACING_H */
