#ifndef ASMASE_MEMORY_STREAMER_H
#define ASMASE_MEMORY_STREAMER_H

#include <algorithm>
#include <cstring>

#include <sys/ptrace.h>
#include <sys/types.h>

class MemoryStreamer {
    pid_t pid;
    long _buffer;
    unsigned char *buffer;
    unsigned char *address;
    size_t offset;

public:
    MemoryStreamer(pid_t pid, void *address)
        : pid{pid},
          buffer{reinterpret_cast<unsigned char *>(&_buffer)},
          address{reinterpret_cast<unsigned char *>(address)}, offset{0} {}

    void *getAddress() const { return reinterpret_cast<void *>(address); }

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
                    fprintf(stderr, "cannot access memory at address %p\n",
                            address + outOffset);
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
