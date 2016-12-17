/*
 * asmase assembler interface.
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

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include <libasmase/libasmase.h>

static int print_registers(struct asmase_instance *a, int reg_set)
{
	struct asmase_register *regs;
	size_t num_regs, i, j;
	uintmax_t val = 0;
	int ret;

	ret = asmase_get_registers(a, reg_set, &regs, &num_regs);
	if (ret == -1) {
		perror("asmase_get_registers");
		return -1;
	}

	for (i = 0; i < num_regs; i++) {
		printf("    \"%s\" = ", regs[i].name);
		switch (regs[i].type) {
		case ASMASE_REGISTER_U8:
			printf("0x%02" PRIx8, regs[i].u8);
			val = regs[i].u8;
			break;
		case ASMASE_REGISTER_U16:
			printf("0x%04" PRIx16, regs[i].u16);
			val = regs[i].u16;
			break;
		case ASMASE_REGISTER_U32:
			printf("0x%08" PRIx32, regs[i].u32);
			val = regs[i].u32;
			break;
		case ASMASE_REGISTER_U64:
			printf("0x%016" PRIx64, regs[i].u64);
			val = regs[i].u64;
			break;
		case ASMASE_REGISTER_U128:
			printf("0x%016" PRIx64 "%016" PRIx64,
			       regs[i].u128_hi, regs[i].u128_lo);
#ifdef ASMASE_HAVE_INT128
			val = regs[i].u128;
#endif
			break;
#ifdef ASMASE_HAVE_FLOAT80
		case ASMASE_REGISTER_FLOAT80:
			printf("%20.18Lf", regs[i].float80);
			break;
#endif
		}
		for (j = 0; j < regs[i].num_status_bits; j++) {
			char *str;

			str = asmase_status_register_format(&regs[i].status_bits[j], val);
			if (!str) {
				free(regs);
				return -1;
			}

			if (str[0])
				printf(", %s", str);
			free(str);
		}
		printf("\n");
	}

	free(regs);

	return 0;
}

static int test_get_registers(struct asmase_instance *a)
{
	int regset;

	regset = asmase_get_register_sets(a);
	if (regset & ASMASE_REGISTERS_PROGRAM_COUNTER) {
		printf("Program counter:\n");
		print_registers(a, ASMASE_REGISTERS_PROGRAM_COUNTER);
	}
	if (regset & ASMASE_REGISTERS_SEGMENT) {
		printf("Segment:\n");
		print_registers(a, ASMASE_REGISTERS_SEGMENT);
	}
	if (regset & ASMASE_REGISTERS_GENERAL_PURPOSE) {
		printf("General purpose:\n");
		print_registers(a, ASMASE_REGISTERS_GENERAL_PURPOSE);
	}
	if (regset & ASMASE_REGISTERS_STATUS) {
		printf("Status:\n");
		print_registers(a, ASMASE_REGISTERS_STATUS);
	}
	if (regset & ASMASE_REGISTERS_FLOATING_POINT) {
		printf("Floating point:\n");
		print_registers(a, ASMASE_REGISTERS_FLOATING_POINT);
	}
	if (regset & ASMASE_REGISTERS_FLOATING_POINT_STATUS) {
		printf("Floating point status:\n");
		print_registers(a, ASMASE_REGISTERS_FLOATING_POINT_STATUS);
	}
	if (regset & ASMASE_REGISTERS_VECTOR) {
		printf("Vector:\n");
		print_registers(a, ASMASE_REGISTERS_VECTOR);
	}
	if (regset & ASMASE_REGISTERS_VECTOR_STATUS) {
		printf("Vector status:\n");
		print_registers(a, ASMASE_REGISTERS_VECTOR_STATUS);
	}

	printf("All registers:\n");
	print_registers(a, regset);

	return 0;
}

int main(void)
{
	struct asmase_instance *a;
	int status = EXIT_FAILURE;
	int ret;

	if (libasmase_init() == -1) {
		perror("libasmase_init");
		return EXIT_FAILURE;
	}

	a = asmase_create_instance();
	if (!a) {
		perror("asmase_create_instance");
		return EXIT_FAILURE;
	}

	ret = test_get_registers(a);
	if (ret)
		goto out;

	status = EXIT_SUCCESS;
out:
	asmase_destroy_instance(a);
	return status;
}
