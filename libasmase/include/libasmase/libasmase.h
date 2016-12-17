/*
 * libasmase interface.
 *
 * Copyright (C) 2016 Omar Sandoval
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

#include <stddef.h>
#include <stdint.h>
#include <sys/uio.h>

#include <libasmase/arch.h>
#include <libasmase/assembler.h>

/**
 * libasmase_init() - Initialize libasmase.
 *
 * This function must be called before the rest of libasmase.
 *
 * Return: 0 on success, -1 on failure, in which case errno will be set
 */
int libasmase_init(void);

struct asmase_instance;

/**
 * enum asmase_register_set - Bitmask of classes of registers.
 */
enum asmase_register_set {
	ASMASE_REGISTERS_PROGRAM_COUNTER = (1 << 0),
	ASMASE_REGISTERS_SEGMENT = (1 << 1),
	ASMASE_REGISTERS_GENERAL_PURPOSE = (1 << 2),
	ASMASE_REGISTERS_STATUS = (1 << 3),
	ASMASE_REGISTERS_FLOATING_POINT = (1 << 4),
	ASMASE_REGISTERS_FLOATING_POINT_STATUS = (1 << 5),
	ASMASE_REGISTERS_VECTOR = (1 << 6),
	ASMASE_REGISTERS_VECTOR_STATUS = (1 << 7),
};

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
 * asmase_status_register_value() - Get the value of a flag in a status
 * register.
 *
 * @bits: The set of bits.
 * @value: The status register value.
 *
 * Return: The value of the flag bits.
 */
static inline uint8_t
asmase_status_register_value(const struct asmase_status_register_bits *bits,
			     uintmax_t value)
{
	return (value >> bits->shift) & bits->mask;
}

/**
 * asmase_status_register_format() - Format the value of a flag in a status
 * register.
 *
 * @bits: The set of bits.
 * @value: The status register value.
 *
 * Return: Allocated string on success (must be freed with free()); NULL on
 * failure.
 */
char *asmase_status_register_format(const struct asmase_status_register_bits *bits,
				    uintmax_t value);

/**
 * enum asmase_register_type - Types of registers.
 */
enum asmase_register_type {
	ASMASE_REGISTER_U8 = 1,
	ASMASE_REGISTER_U16,
	ASMASE_REGISTER_U32,
	ASMASE_REGISTER_U64,
	ASMASE_REGISTER_U128,
#ifdef ASMASE_HAVE_FLOAT80
	ASMASE_REGISTER_FLOAT80,
#endif
};

/**
 * struct asmase_register - State of a register.
 */
struct asmase_register {
	/**
	 * @name: Register name.
	 */
	const char *name;

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

	/**
	 * @type: Register type.
	 */
	enum asmase_register_type type;

	/*
	 * Register value depending on @type.
	 */
	union {
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
};

/**
 * asmase_create_instance() - Create a new asmase instance.
 *
 * Return: New asmase instance.
 */
struct asmase_instance *asmase_create_instance(void);

/**
 * asmase_destroy_instance() - Free all resources associated with an asmase
 * instance.
 *
 * @a: asmase instance to destroy.
 */
void asmase_destroy_instance(struct asmase_instance *a);

/**
 * asmase_execute_code() - Execute machine code on an asmase instance.
 *
 * @a: asmase instance to execute on.
 * @code: Assembled machine code to execute.
 * @len: Length of the machine code.
 * @wstatus: Status of the asmase instance as returned by waitpid().
 *
 * Return: 0 on success, -1 on failure, in which case errno will be set
 */
int asmase_execute_code(const struct asmase_instance *a,
			const unsigned char *code, size_t len, int *wstatus);

/**
 * asmase_get_register_sets() - Get the types of registers available on an
 * asmase instance.
 *
 * @a: asmase instance.
 *
 * Return: A bitmask of enum asmase_register_set values representing the
 * available types of registers.
 */
int asmase_get_register_sets(const struct asmase_instance *a);

/**
 * asmase_get_registers() - Get the value of the registers of an asmase
 * instance.
 *
 * @a: asmase instance.
 * @reg_sets: Bitmask of enum asmase_register_set values representing the
 * registers to get.
 * @regs: Allocated list of registers; must be freed with free().
 * @num_regs: Number of registers in the returned list.
 *
 * Return: 0 on success, -1 on failure, in which case errno will be set.
 */
int asmase_get_registers(const struct asmase_instance *a, int reg_sets,
			 struct asmase_register **regs, size_t *num_regs);

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

#endif /* LIBASMASE_H */
