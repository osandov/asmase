/*
 * Useful macros and helper functions (mostly copied from the Linux kernel).
 *
 * Copyright (C) 2016-2017 Omar Sandoval
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

#ifndef LIBASMASE_UTIL_H
#define LIBASMASE_UTIL_H

#include <errno.h>
#include <limits.h>
#include <stdlib.h>

#define FIELD_SIZEOF(t, f) (sizeof(((t*)0)->f))
#define FIELD_TYPEOF(t, f) typeof(((t*)0)->f)

#define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2*!!(condition)]))
#define BUILD_BUG_ON_ZERO(e) (sizeof(struct { int:-!!(e); }))

#define __same_type(a, b) __builtin_types_compatible_p(typeof(a), typeof(b))
#define __must_be_array(a) BUILD_BUG_ON_ZERO(__same_type((a), &(a)[0]))
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]) + __must_be_array(arr))

#define __round_mask(x, y) ((__typeof__(x))((y)-1))
#define round_up(x, y) ((((x)-1) | __round_mask(x, y))+1)

static inline bool strstartswith(const char *haystack, const char *needle)
{
	return strncmp(haystack, needle, strlen(needle)) == 0;
}

static inline long simple_strtol(const char *str)
{
	char *end;
	long ret;

	ret = strtol(str, &end, 10);
	if (end == str || *end) {
		errno = EINVAL;
		return 0;
	}
	return ret;
}

static inline unsigned long simple_strtoul(const char *str)
{
	char *end;
	unsigned long ret;

	ret = strtoul(str, &end, 10);
	if (end == str || *end) {
		errno = EINVAL;
		return 0;
	}
	return ret;
}

static inline int simple_strtoi(const char *str)
{
	long ret;

	errno = 0;
	ret = simple_strtol(str);
	if (errno)
		return ret;
	if (ret < INT_MIN || ret > INT_MAX) {
		errno = EOVERFLOW;
		return 0;
	}
	return ret;
}

#endif /* LIBASMASE_UTIL_H */
