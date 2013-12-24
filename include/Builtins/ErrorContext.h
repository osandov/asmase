#ifndef ASMASE_ERROR_CONTEXT_H
#define ASMASE_ERROR_CONTEXT_H

#include <llvm/Support/SourceMgr.h>

/** Context for printing errors when parsing or evaluating. */
class ErrorContext {
    /**
     * Dummy source manager used to construct diagnostics. The implementation
     * of SMDiagnostic currently never touches the SourceMgr you give it, so
     * this is fine.
     */
    llvm::SourceMgr dummySourceMgr;

    /** The line that we're handling. */
    const char *line;

    /** Offset to apply to incoming column numbers. */
    int offset;

public:
    /**
     * Create a new error context for the given line.
     * @param offset Input columns might be off by a constant number (for
     * example, when we trim the leading ':' for a built-in). This offset is
     * added to the column number every time.
     */
    ErrorContext(const char *line, int offset = 0)
        : line(line), offset(offset) {}

    void printMessage(const char *msg);

    /** Print an error message with a caret at the given column. */
    void printMessage(const char *msg, int column);

    void printMessage(const char *msg, int column, int start, int end);

    void printMessage(const char *msg, int column,
                      int start1, int end1, int start2, int end2);
};

#endif /* ASMASE_ERROR_CONTEXT_H */
