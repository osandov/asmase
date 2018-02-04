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

#ifdef __cplusplus
#include <cstddef>
#include <cstdint>
#else
#include <stddef.h>
#include <stdint.h>
#endif
#include <sys/types.h>
#include <sys/uio.h>

#ifdef __x86_64__
#define ASMASE_HAVE_INT128 1
#define ASMASE_HAVE_FLOAT80 1
#endif

#ifdef __i386__
#define ASMASE_HAVE_FLOAT80 1
#endif

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define ASMASE_LITTLE_ENDIAN 1
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define ASMASE_BIG_ENDIAN 1
#else /* __ORDER_PDP_ENDIAN__? */
#error
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct asmase_instance;

/**
 * struct asmase_status_register_bits - A set of bits in a status register.
 *
 * This can be used to parse the bits in a status register (like eflags on x86
 * or cpsr on ARM) in order to obtain a decoded human-readable description.
 *
 * A typical case is @mask = 1, which represents a simple bit flag.
 */
struct asmase_status_register_bits {
	const char *name;

	const char * const *values;

	uint8_t shift;

	uint8_t mask;
};

/**
 * enum asmase_register_set - Bitmask of classes of registers.
 */
enum asmase_register_set {
	ASMASE_REGISTERS_PROGRAM_COUNTER,
	ASMASE_REGISTERS_SEGMENT,
	ASMASE_REGISTERS_GENERAL_PURPOSE,
	ASMASE_REGISTERS_STATUS,
	ASMASE_REGISTERS_FLOATING_POINT,
	ASMASE_REGISTERS_FLOATING_POINT_STATUS,
	ASMASE_REGISTERS_VECTOR,
	ASMASE_REGISTERS_VECTOR_STATUS,
};

/**
 * enum asmase_register_type - Types of registers.
 */
enum asmase_register_type {
	ASMASE_REGISTER_U8,
	ASMASE_REGISTER_U16,
	ASMASE_REGISTER_U32,
	ASMASE_REGISTER_U64,
	ASMASE_REGISTER_U128,
#ifdef ASMASE_HAVE_FLOAT80
	ASMASE_REGISTER_FLOAT80,
#endif
};

struct asmase_register_descriptor {
	/**
	 * @name: Register name.
	 */
	const char *name;

	/**
	 * @set: Register set this register belongs to.
	 */
	enum asmase_register_set set;

	/**
	 * @type: Register type.
	 */
	enum asmase_register_type type;

	/**
	 * @status: Array of struct asmase_status_register_bits that can be used
	 * to decode this register.
	 */
	const struct asmase_status_register_bits *status_bits;

	/**
	 * @num_status_bits: Number of elements in the @status_bits array. If
	 * this is zero, then there is nothing to decode.
	 */
	size_t num_status_bits;

	/* Internal. */
	size_t offset;
	void (*copy_register_fn)(const struct asmase_register_descriptor *,
				 void *, const struct arch_regs *);
};

union asmase_register_value {
	uint8_t u8;
	uint16_t u16;
	uint32_t u32;
	uint64_t u64;

#ifdef ASMASE_HAVE_INT128
	unsigned __int128 u128;
#endif
	struct {
#ifdef ASMASE_LITTLE_ENDIAN
		uint64_t u128_lo, u128_hi;
#else
		uint64_t u128_hi, u128_lo;
#endif
	};

#ifdef ASMASE_HAVE_FLOAT80
	long double float80;
#endif
};

extern const struct asmase_register_descriptor asmase_registers[];
extern const size_t asmase_num_registers;

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
 * asmase_get_memory_range() - Get the contiguous memory range used by an asmase
 * instance.
 *
 * @a: asmase instance.
 * @start: Starting address of the memory range.
 * @length: Length of the memory range.
 */
void asmase_get_memory_range(const struct asmase_instance *a,
			     uintptr_t *start, size_t *length);

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

/**
 * asmase_get_register() - Get the value of a register of an asmase instance.
 *
 * @a: asmase instance.
 * @reg: Register to get.
 * @value: Returned value.
 */
void asmase_get_register(const struct asmase_instance *a,
			 const struct asmase_register_descriptor *reg,
			 union asmase_register_value *value);

/**
 * asmase_set_register() - Set the value of a register of an asmase instance.
 *
 * @a: asmase instance.
 * @reg: Register to set.
 * @value: Value to set the register to.
 *
 * Return: 0 on success, -1 on failure, in which case errno will be set
 */
int asmase_set_register(struct asmase_instance *a,
			const struct asmase_register_descriptor *reg,
			const union asmase_register_value *value);

/**
 * asmase_status_register_format() - Format the value of a flag in a status
 * register.
 *
 * @reg: The status register.
 * @bits: The set of bits.
 * @value: The status register value.
 *
 * Return: Allocated string on success (must be freed with free()); NULL on
 * failure.
 */
char *asmase_status_register_format(const struct asmase_register_descriptor *reg,
				    const struct asmase_status_register_bits *bits,
				    const union asmase_register_value *value);

/**
 * asmase_readv_memory() - Read ranges of memory from an asmase instance.
 *
 * See process_vm_readv().
 *
 * @a: asmase instance.
 * @local_iov: iovec of addresses to read into.
 * @liovcnt: Number of elements in @local_iov.
 * @remote_iov: iovec of addresses to read from.
 * @riovcnt: Number of elements in @remote_iov.
 *
 * Return: Number of bytes read on success, -1 on failure, in which case errno
 * will be set.
 */
ssize_t asmase_readv_memory(const struct asmase_instance *a,
			    const struct iovec *local_iov, size_t liovcnt,
			    const struct iovec *remote_iov, size_t riovcnt);

/**
 * asmase_read_memory() - Read a range of memory from an asmase instance.
 *
 * @a: asmase instance.
 * @buf: Buffer to read into.
 * @addr: Memory address to read from.
 * @len: Amount of memory to read.
 *
 * Return: Number of bytes read on success, -1 on failure, in which case errno
 * will be set.
 */
ssize_t asmase_read_memory(const struct asmase_instance *a, void *buf,
			   void *addr, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* LIBASMASE_H */
