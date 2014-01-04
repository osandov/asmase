#ifndef ASMASE_PROCESSOR_FLAGS_H
#define ASMASE_PROCESSOR_FLAGS_H

#include <cassert>
#include <initializer_list>
#include <string>
#include <vector>

template <typename T>
struct ProcessorFlag {
    std::string name;
    T shiftedMask, shift;
    T expected;
    bool alwaysPrint;

    ProcessorFlag(const std::string &name, T mask, T expected,
                  bool alwaysPrint = false)
        : name(name), expected(expected), alwaysPrint(alwaysPrint)
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

    // Bit flag
    ProcessorFlag(const std::string &name, T mask)
        : ProcessorFlag{name, mask, 0x1, false} {}

    T getValue(T flags) const
    {
        return (flags >> shift) & shiftedMask;
    }
};

template <typename T>
class ProcessorFlags {
    std::vector<ProcessorFlag<T>> flags;

public:
    ProcessorFlags(std::initializer_list<ProcessorFlag<T>> flags)
        : flags(flags) {}

    void printFlags(T value)
    {
        std::cout << '[';

        for (const ProcessorFlag<T> &flag : flags) {
            T flagValue = flag.getValue(value);

            if (flag.alwaysPrint) {
                if (flagValue != flag.expected)
                    std::cout << ' ' << flag.name << '=' << flagValue;
            } else {
                if (flagValue == flag.expected)
                    std::cout << ' ' << flag.name;
            }
        }

        std::cout << " ]";
    }
};

#endif /* ASMASE_PROCESSOR_FLAGS_H */
