#ifndef ASIP_LLVM_ASSEMBLER_H
#define ASIP_LLVM_ASSEMBLER_H

/** A container for parsed command line parameters. */
struct Parameters {
    const char *ProgName;
    const std::string &ArchName;
    const std::string &TripleName;
    const cl::list<std::string> &MAttrs;
    const std::string &MCPU;

    Parameters(const char *PN,
               const std::string &AN,
               const std::string &TN,
               const cl::list<std::string> &MA,
               const std::string &MCPU);
};

/**
 * Return a structure containing all parsed command line parameters. This
 * must be called after init_assemblers.
 */
Parameters GetParameters();

#endif /* ASIP_LLVM_ASSEMBLER_H */
