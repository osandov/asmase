/*
 * ErrorContext implementation. Uses LLVM for printing diagnostics.
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

#include <llvm/Support/raw_ostream.h>
using namespace llvm;

#include "Builtins/ErrorContext.h"

namespace Builtins {

/* See Builtins/ErrorContext.h. */
void ErrorContext::printMessage(const char *msg)
{
    SMDiagnostic diagnostic{"<stdin>", SourceMgr::DK_Error, msg};
    diagnostic.print(nullptr, errs());
}

/* See Builtins/ErrorContext.h. */
void ErrorContext::printMessage(const char *msg, int column)
{
    column += offset;
    SMDiagnostic diagnostic{
        dummySourceMgr,
        SMLoc::getFromPointer(line + column),
        filename,
        lineno,
        column,
        SourceMgr::DK_Error,
        msg,
        line,
#if LLVM_VERSION_MAJOR > 3 || LLVM_VERSION_MINOR >= 3
        None
#else
        ArrayRef<std::pair<unsigned, unsigned>>{}
#endif
    };
    diagnostic.print(nullptr, errs());
}

/* See Builtins/ErrorContext.h. */
void ErrorContext::printMessage(const char *msg, int column, int start, int end)
{
    column += offset;
    start += offset;
    end += offset;

    ArrayRef<std::pair<unsigned, unsigned>> ranges{
        std::pair<unsigned, unsigned>{start, end}};

    SMDiagnostic diagnostic{
        dummySourceMgr,
        SMLoc::getFromPointer(line + column),
        filename,
        lineno,
        column,
        SourceMgr::DK_Error,
        msg,
        line,
        ranges
    };
    diagnostic.print(nullptr, errs());
}

/* See Builtins/ErrorContext.h. */
void ErrorContext::printMessage(const char *msg, int column,
                                int start1, int end1, int start2, int end2)
{
    std::vector<std::pair<unsigned, unsigned>> ranges;
    column += offset;
    start1 += offset;
    end1 += offset;
    start2 += offset;
    end2 += offset;

    ranges.emplace_back(start1, end1);
    ranges.emplace_back(start2, end2);
    SMDiagnostic diagnostic{
        dummySourceMgr,
        SMLoc::getFromPointer(line + column),
        filename,
        lineno,
        column,
        SourceMgr::DK_Error,
        msg,
        line,
        ArrayRef<std::pair<unsigned, unsigned>>{ranges}
    };
    diagnostic.print(nullptr, errs());
}

}
