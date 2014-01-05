/*
 * Utility class for streaming memory from a tracee.
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

#ifndef ASMASE_MEMORY_STREAMER_H
#define ASMASE_MEMORY_STREAMER_H

#include <algorithm>
#include <cstring>
#include <iostream>

#include <sys/ptrace.h>
#include <sys/types.h>

/** Class for reading memory from a tracee element by element. */
class MemoryStreamer {
    /** PID of the tracee. */
    pid_t pid;

    /** Last word read by ptrace. */
    long _buffer;

    /** Pointer to last word read by ptrace. */
    unsigned char *buffer;

    /** Next address at which to read. */
    unsigned char *address;

    /** Offset in the buffer of remaining unread data. */
    size_t offset;

public:
    /**
     * Create a memory streamer for the given process starting at the given
     * address.
     */
    MemoryStreamer(pid_t pid, void *address)
        : pid{pid},
          buffer{reinterpret_cast<unsigned char *>(&_buffer)},
          address{reinterpret_cast<unsigned char *>(address)}, offset{0} {}

    /** Get the next address to be read from. */
    void *getAddress() const { return reinterpret_cast<void *>(address); }

    /**
     * Read an element from the tracee.
     * @return Zero on success, nonzero on failure.
     */
    template <typename T>
    int next(T &out)
    {
        auto outBuffer = reinterpret_cast<unsigned char *>(&out);
        size_t outOffset = 0;
        while (outOffset < sizeof(T)) {
            if (offset % sizeof(long) == 0) {
                errno = 0;
                _buffer = ptrace(PTRACE_PEEKDATA, pid, address + outOffset,
                                 nullptr);
                if (errno) {
                    std::cerr << "cannot access memory at address "
                              << address + outOffset << '\n';
                    return 1;
                }

                offset = 0;
            }

            size_t amount = std::min(sizeof(T) - outOffset, sizeof(long) - offset);
            memcpy(outBuffer + outOffset, buffer + offset, amount);

            offset += amount;
            outOffset += amount;
        }

        address += sizeof(T);

        return 0;
    }
};

#endif /* ASMASE_MEMORY_STREAMER_H */
