/*
 * Easy pretty-printing of status/control register bit flags.
 *
 * Copyright (C) 2013-2014 Omar Sandoval
 *
 * This file is part of asmase.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ASMASE_PROCESSOR_FLAGS_H
#define ASMASE_PROCESSOR_FLAGS_H

#include <cassert>
#include <cstdio>
#include <initializer_list>
#include <string>
#include <vector>

/**
 * A flag in a processor register.
 * @tparam The type of the register value.
 */
template <typename T>
class ProcessorFlag {
public:
    /** The name of the flag. */
    std::string name;

    /**
     * The mask in the register shifted down so the least-significant bit is
     * in the least-significant position.
     */
    T shiftedMask;

    /** Bits to shift the register down to match shiftedMask. */
    T shift;

    /**
     * Expected value of the flag. If alwaysPrint is true, the flag won't be
     * printed for this "typical" case. If alwaysPrint is false, the flag will
     * only be printed if the value matches this.
     */
    T expected;

    /**
     * If this is true, the value of the flag is printed instead of just the
     * name.
     */
    bool alwaysPrint;

    ProcessorFlag(const std::string &name, T mask, T expected,
                  bool alwaysPrint = false)
        : name{name}, expected{expected}, alwaysPrint{alwaysPrint}
    {
        // Assert that the mask isn't zero, because if it is, we will keep
        // shifting forever
        assert(mask != 0x0);

        shift = 0;
        while (!(mask & 0x1)) {
            mask >>= 1;
            ++shift;
        }

        shiftedMask = mask;
    }

    /** Bit flag constructor. */
    ProcessorFlag(const std::string &name, T mask)
        : ProcessorFlag{name, mask, 0x1, false} {}

    /** Get the value of the flag in the given register value. */
    T getValue(T reg) const
    {
        return (reg >> shift) & shiftedMask;
    }
};

/**
 * A set of processor flags.
 * @tparam The type of the register value.
 */
template <typename T>
class ProcessorFlags {
    /** The flags. */
    std::vector<ProcessorFlag<T>> flags;

public:
    ProcessorFlags(std::initializer_list<ProcessorFlag<T>> flags)
        : flags{flags} {}

    /** Pretty-print the set of flags for the given register value. */
    void printFlags(T reg)
    {
        printf("[");

        for (const ProcessorFlag<T> &flag : flags) {
            T flagValue = flag.getValue(reg);

            if (flag.alwaysPrint) {
                if (flagValue != flag.expected)
                    printf(" %s = %lld",
                        flag.name.c_str(), (long long) flagValue);
            } else {
                if (flagValue == flag.expected)
                    printf(" %s", flag.name.c_str());
            }
        }

        printf(" ]");
    }
};

#endif /* ASMASE_PROCESSOR_FLAGS_H */
