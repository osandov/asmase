/*
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libasmase/libasmase.h>

static int test_ok(struct asmase_assembler *as)
{
	unsigned char expected[] = {0xb8, 0x05, 0x00, 0x00, 0x00};
	char *out;
	size_t len;
	int ret;

	ret = asmase_assemble_code(as, "test_ok", 1, "movl $5, %eax", &out,
				   &len);
	if (ret == -1) {
		fprintf(stderr, "Internal error assembling test_ok\n");
		return -1;
	} else if (ret == 1) {
		fwrite(out, 1, len, stderr);
		free(out);
		return -1;
	}
	assert(ret == 0);

	if (len != sizeof(expected) || memcmp(out, expected, len) != 0) {
		fprintf(stderr, "machine code does not match expected\n");
		free(out);
		return -1;
	}

	free(out);
	return 0;
}

static int test_error(struct asmase_assembler *as)
{
	char *out;
	size_t len;
	int ret;

	ret = asmase_assemble_code(as, "test_error", 1, "foo $5, %eax", &out,
				   &len);
	if (ret == -1) {
		fprintf(stderr, "Internal error assembling test_ok\n");
		return -1;
	} else if (ret == 0) {
		fprintf(stderr, "Assembling succeeded when it should have failed\n");
		free(out);
		return -1;
	}
	assert(ret == 1);

	fwrite(out, 1, len, stderr);
	free(out);

	return 0;
}

int main(void)
{
	struct asmase_assembler *as;
	int status = EXIT_FAILURE;

	if (libasmase_init() == -1) {
		perror("libasmase_init");
		return EXIT_FAILURE;
	}

	as = asmase_create_assembler();
	if (!as)
		return EXIT_FAILURE;

	if (test_ok(as) == -1)
		goto out;

	if (test_error(as) == -1)
		goto out;

	status = EXIT_SUCCESS;
out:
	asmase_destroy_assembler(as);
	return status;
}
