/*
 * Architecture-independent part of getting register values.
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

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/uio.h>
#include <sys/user.h>

#include "internal.h"

__attribute__((visibility("default")))
char *asmase_status_register_format(const struct asmase_status_register_bits *bits,
				    uintmax_t value)
{
	uint8_t bits_value;
	char *str;
	int ret;

	bits_value = asmase_status_register_value(bits, value);

	if (bits->mask == 0x1) {
		if (bits_value)
			return strdup(bits->name);
		else
			return strdup("");
	}

	if (bits->values)
		ret = asprintf(&str, "%s=%s", bits->name, bits->values[bits_value]);
	else
		ret = asprintf(&str, "%s=0x%x", bits->name, bits_value);
	if (ret == -1)
		return NULL;

	return str;
}

__attribute__((visibility("default")))
int asmase_get_register_sets(const struct asmase_instance *a)
{
	int ret = ASMASE_REGISTERS_PROGRAM_COUNTER;

	if (arch_num_segment_regs)
		ret |= ASMASE_REGISTERS_SEGMENT;

	if (arch_num_general_purpose_regs)
		ret |= ASMASE_REGISTERS_GENERAL_PURPOSE;
	if (arch_num_status_regs)
		ret |= ASMASE_REGISTERS_STATUS;

	if (arch_num_floating_point_regs)
		ret |= ASMASE_REGISTERS_FLOATING_POINT;
	if (arch_num_floating_point_status_regs)
		ret |= ASMASE_REGISTERS_FLOATING_POINT_STATUS;

	if (arch_num_vector_regs)
		ret |= ASMASE_REGISTERS_VECTOR;
	if (arch_num_vector_status_regs)
		ret |= ASMASE_REGISTERS_VECTOR_STATUS;

	return ret;
}

static int for_each_reg_set(int reg_sets,
			    int (*fn)(enum asmase_register_set set,
				      const struct arch_register_descriptor *regs,
				      size_t num_regs, void *arg),
			     void *data)
{
	int ret;

	if (reg_sets & ASMASE_REGISTERS_PROGRAM_COUNTER) {
		ret = fn(ASMASE_REGISTERS_PROGRAM_COUNTER,
			 &arch_program_counter_reg, 1, data);
		if (ret)
			return ret;
	}
	if (reg_sets & ASMASE_REGISTERS_SEGMENT) {
		ret = fn(ASMASE_REGISTERS_SEGMENT, arch_segment_regs,
			 arch_num_segment_regs, data);
		if (ret)
			return ret;
	}
	if (reg_sets & ASMASE_REGISTERS_GENERAL_PURPOSE) {
		ret = fn(ASMASE_REGISTERS_GENERAL_PURPOSE,
			 arch_general_purpose_regs,
			 arch_num_general_purpose_regs, data);
		if (ret)
			return ret;
	}
	if (reg_sets & ASMASE_REGISTERS_STATUS) {
		ret = fn(ASMASE_REGISTERS_STATUS, arch_status_regs,
			 arch_num_status_regs, data);
		if (ret)
			return ret;
	}
	if (reg_sets & ASMASE_REGISTERS_FLOATING_POINT) {
		ret = fn(ASMASE_REGISTERS_FLOATING_POINT,
			 arch_floating_point_regs, arch_num_floating_point_regs,
			 data);
		if (ret)
			return ret;
	}
	if (reg_sets & ASMASE_REGISTERS_FLOATING_POINT_STATUS) {
		ret = fn(ASMASE_REGISTERS_FLOATING_POINT_STATUS,
			 arch_floating_point_status_regs,
			 arch_num_floating_point_status_regs, data);
		if (ret)
			return ret;
	}
	if (reg_sets & ASMASE_REGISTERS_VECTOR) {
		ret = fn(ASMASE_REGISTERS_VECTOR, arch_vector_regs,
			 arch_num_vector_regs, data);
		if (ret)
			return ret;
	}
	if (reg_sets & ASMASE_REGISTERS_VECTOR_STATUS) {
		ret = fn(ASMASE_REGISTERS_VECTOR, arch_vector_status_regs,
			 arch_num_vector_status_regs, data);
		if (ret)
			return ret;
	}

	return 0;
}

struct get_registers_data {
	struct arch_ptrace_regsets regsets;
	bool need_regset[ARCH_NUM_PTRACE_REGSETS];

	struct asmase_register *regs;
	size_t num_regs;
	size_t i;
};

static int prepare_regs(enum asmase_register_set set,
			const struct arch_register_descriptor *regs,
			size_t num_regs, void *arg)
{
	struct get_registers_data *data = arg;
	size_t i;

	data->num_regs += num_regs;

	for (i = 0; i < num_regs; i++)
		data->need_regset[regs[i].ptrace_regset] = true;

	return 0;
}

static inline void copy_register(enum asmase_register_set set,
				 struct asmase_register *reg,
				 const struct arch_register_descriptor *desc,
				 void *buf)
{
	reg->name = desc->name;
	reg->set = set;
	reg->type = desc->type;
	reg->status_bits = desc->status_bits;
	reg->num_status_bits = desc->num_status_bits;
	memcpy(&reg->u32, ((char *)buf) + desc->offset, desc->size);
}

static int emit_regs(enum asmase_register_set set,
		     const struct arch_register_descriptor *regs,
		     size_t num_regs, void *arg)
{
	struct get_registers_data *data = arg;
	void *buf;
	size_t i;

	for (i = 0; i < num_regs; i++) {
		buf = ((char *)&data->regsets +
		       arch_ptrace_regset_offsetof(regs[i].ptrace_regset));
		copy_register(set, &data->regs[data->i++], &regs[i], buf);
	}

	return 0;
}

__attribute__((visibility("default")))
int asmase_get_registers(const struct asmase_instance *a, int reg_sets,
			 struct asmase_register **regs, size_t *num_regs)
{
	struct get_registers_data data = {};
	size_t i;

	if (reg_sets & ~ASMASE_REGISTERS_ALL) {
		errno = EINVAL;
		return -1;
	}

	for_each_reg_set(reg_sets, prepare_regs, &data);

	for (i = 0; i < ARCH_NUM_PTRACE_REGSETS; i++) {
		struct iovec iov;
		long ret;

		if (!data.need_regset[i])
			continue;

		iov.iov_base = ((char *)&data.regsets +
				arch_ptrace_regset_offsetof(i));
		iov.iov_len = arch_ptrace_regset_sizeof(i);
		ret = ptrace(PTRACE_GETREGSET, a->pid, arch_ptrace_regset_nt(i),
			     &iov);
		if (ret == -1)
			return ret;
	}

	data.regs = malloc(sizeof(*data.regs) * data.num_regs);
	if (!data.regs)
		return -1;

	for_each_reg_set(reg_sets, emit_regs, &data);

	*regs = data.regs;
	*num_regs = data.num_regs;

	return 0;
}
