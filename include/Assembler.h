#ifndef ASMASE_ASSEMBLER_H
#define ASMASE_ASSEMBLER_H

#include <memory>
#include <string>

class Inputter;

/** Opaque handle for an assembler context. */
class AssemblerContext;

class Assembler {
    /** The assembler context for this assembler. */
    const std::shared_ptr<AssemblerContext> context;

public:
    /** Create an assembler in the given context. */
    Assembler(std::shared_ptr<AssemblerContext> &context)
        : context{context} {}

    /**
     * Assemble the given assembly instruction to machine code.
     * @return Zero on success, nonzero on failure.
     */
    int assembleInstruction(const std::string &instruction,
                            std::string &machineCodeOut,
                            const Inputter &inputter);

    /**
     * Create an assembler context which can be used to construct an
     * assembler.
     * @return nullptr on error.
     */
    static std::shared_ptr<AssemblerContext> createAssemblerContext();
};

#endif /* ASMASE_ASSEMBLER_H */
