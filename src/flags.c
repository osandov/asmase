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

#include <assert.h>
#include <stdio.h>

#include "flags.h"

static inline unsigned long long get_value(unsigned long long x,
                                           unsigned long long mask)
{
    /*
     * Shift both the mask and value down until the mask's lowest-order bit is
     * all the way at the bottom. Note that this will loop forever if the mask
     * is zero, so we assert that it isn't
     */

    assert(mask != 0);

    while (!(mask & 0x1)) {
        x >>= 1;
        mask >>= 1;
    }

    return x & mask;
}

void print_processor_flags(unsigned long long reg,
                           struct processor_flag *flags, size_t num_flags)
{
    printf("[");

    for (size_t i = 0; i < num_flags; ++i) {
        struct processor_flag *flag = &flags[i];
        unsigned long long value = get_value(reg, flag->mask);

        if (flag->variable_value) {
            if (value != flag->value_if_true)
                printf(" %s=%llu", flag->name, value);
        } else {
            if (value == flag->value_if_true)
                printf(" %s", flag->name);
        }
    }

    printf(" ]");
}
