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

#include <stdio.h>
#include <stdlib.h>

#include <libasmase/libasmase.h>

static int test_memory(struct asmase_instance *a)
{
	char buf[20];
	ssize_t ret;

	ret = asmase_read_memory(a, buf, &ret, sizeof(buf));
	if (ret == -1) {
		perror("asmase_read_memory");
		return -1;
	}

	return 0;
}

int main(void)
{
	struct asmase_instance *a;
	int status = EXIT_FAILURE;
	int ret;

	a = asmase_create_instance();
	if (!a) {
		perror("asmase_create_instance");
		return EXIT_FAILURE;
	}

	ret = test_memory(a);
	if (ret)
		goto out;

	status = EXIT_SUCCESS;
out:
	asmase_destroy_instance(a);
	return status;
}
