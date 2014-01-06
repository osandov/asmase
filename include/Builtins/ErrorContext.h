/*
 * Error context class.
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

#ifndef ASMASE_BUILTINS_ERROR_CONTEXT_H
#define ASMASE_BUILTINS_ERROR_CONTEXT_H

#include <llvm/Support/SourceMgr.h>

namespace Builtins {

/** Context for printing errors when parsing or evaluating. */
class ErrorContext {
    /**
     * Dummy source manager used to construct diagnostics. The implementation
     * of SMDiagnostic currently never touches the SourceMgr you give it, so
     * this is fine.
     */
    llvm::SourceMgr dummySourceMgr;

    /** The name of the file that we're handling. */
    const char *filename;

    /** The line number that we're handling. */
    int lineno;

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
    ErrorContext(const char *filename, int lineno, const char *line, int offset = 0)
        : filename{filename}, lineno{lineno}, line{line}, offset{offset} {}

    /** Print an error message. */
    void printMessage(const char *msg);

    /** Print an error message with a caret at the given column. */
    void printMessage(const char *msg, int column);

    /** Print an error message with a caret and a single range. */
    void printMessage(const char *msg, int column, int start, int end);

    /** Print an error message with a caret and two ranges. */
    void printMessage(const char *msg, int column,
                      int start1, int end1, int start2, int end2);

    // It's probably not worth generalizing the printMessage function since
    // this handles 99% of cases we might want for the built-in system.
};

}

#endif /* ASMASE_BUILTINS_ERROR_CONTEXT_H */
