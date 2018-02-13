/*
 * libasmase interactive assembly language library.
 *
 * Copyright (C) 2016-2018 Omar Sandoval
 *
 * This file is part of asmase.
 *
 * asmase is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * asmase is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with asmase.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBASMASE_H
#define LIBASMASE_H

#define SHMEM_SIZE 65536
#define X86_64_SHMEM_ADDR 0x7fff00000000UL
#define X86_64_REGS_SIZE 728

#ifndef __ASSEMBLER__

#ifdef __cplusplus
#include <cstddef>
#include <cstdint>
#else
#include <stddef.h>
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct asmase_instance;

/**
 * Flags for asmase_create_instance().
 */
enum asmase_create_flags {
	/*
	 * Close all file descriptors. File descriptors with the FD_CLOEXEC file
	 * descriptor flag are always closed whether this flag is set or not.
	 */
	ASMASE_SANDBOX_FDS = (1 << 0),
	/*
	 * Don't allow any syscalls.
	 */
	ASMASE_SANDBOX_SYSCALLS = (1 << 1),
};

/*
 * Enable all sandboxing flags.
 */
#define ASMASE_SANDBOX_ALL ((1 << 2) - 1)

/**
 * asmase_create_instance() - Create a new asmase instance asynchronously. The
 * instance must be waited on with asmase_poll() or asmase_wait(); the instance
 * will trap when it is ready.
 *
 * @flags: A bitmask of enum asmase_create_flags.
 *
 * Return: New asmase instance.
 */
struct asmase_instance *asmase_create_instance(int flags);

/**
 * asmase_destroy_instance() - Free all resources associated with an asmase
 * instance. The caller is responsible for killing and reaping the instance
 * process if it has not already exited.
 *
 * @a: asmase instance to destroy.
 */
void asmase_destroy_instance(struct asmase_instance *a);

/**
 * asmase_getpid() - Get the PID of an asmase instance.
 *
 * @a: asmase instance.
 *
 * Return: PID of the instance.
 */
pid_t asmase_getpid(const struct asmase_instance *a);

/**
 * asmase_get_shmem() - Get a pointer to the memory mapping shared with an
 * asmase instance.
 *
 * @a: asmase instance.
 *
 * Return: Address of the shared memory mapping.
 */
void *asmase_get_shmem(const struct asmase_instance *a);

/**
 * asmase_get_registers() - Get the registers of an asmase instance.
 *
 * @a: asmase instance.
 * @buf: Buffer where the register values will be returned.
 * @len: Length of @buf.
 *
 * Return: 0 on success, -1 on failure, in which case errno will be set
 */
int asmase_get_registers(const struct asmase_instance *a, void *buf,
			 size_t len);

/**
 * asmase_set_registers() - Set the registers of an asmase instance.
 *
 * @a: asmase instance.
 * @buf: Buffer of register values to set to.
 * @len: Length of @buf.
 *
 * Return: 0 on success, -1 on failure, in which case errno will be set
 */
int asmase_set_registers(struct asmase_instance *a, const void *buf,
			 size_t len);

/**
 * asmase_execute_code() - Asynchronously execute machine code on an asmase
 * instance.
 *
 * @a: asmase instance to execute on.
 * @code: Assembled machine code to execute.
 * @len: Length of the machine code.
 *
 * Return: 0 on success, -1 on failure, in which case errno will be set
 */
int asmase_execute_code(struct asmase_instance *a, const char *code,
			size_t len);

/**
 * asmase_wait() - Wait for an asmase instance to change state.
 *
 * @a: asmase instance to wait for.
 * @wstatus: Status of the asmase instance as returned by waitpid().
 *
 * Return: 1 on success, -1 on failure, in which case errno will be set
 */
int asmase_wait(struct asmase_instance *a, int *wstatus);

/**
 * asmase_poll() - Check whether an asmase instance has changed state.
 *
 * @a: asmase instance to check.
 * @wstatus: Status of the asmase instance as returned by waitpid().
 *
 * Return: 1 on success, 0 if the process hasn't changed state, or -1 on
 * failure, in which case errno will be set.
 */
int asmase_poll(struct asmase_instance *a, int *wstatus);

#ifdef __cplusplus
}
#endif

#endif /* __ASSEMBLER__ */

#endif /* LIBASMASE_H */
