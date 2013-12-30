#ifndef ASMASE_ASSEMBLER_H
#define ASMASE_ASSEMBLER_H

#include <memory>
#include <string>

class Inputter;

struct AssemblerContext;

std::shared_ptr<AssemblerContext> createAssemblerContext();

class Assembler {
    std::shared_ptr<AssemblerContext> context;

public:
    Assembler(std::shared_ptr<AssemblerContext> &context)
        : context(context) {}

    int assembleInstruction(const std::string &instruction,
                            std::string &machineCodeOut,
                            const Inputter &inputter);
};

#endif /* ASMASE_ASSEMBLER_H */
