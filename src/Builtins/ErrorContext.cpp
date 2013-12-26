#include <llvm/Support/raw_ostream.h>
using namespace llvm;

#include "Builtins/ErrorContext.h"

namespace Builtins {

/* See Builtins/ErrorContext.h. */
void ErrorContext::printMessage(const char *msg)
{
    SMDiagnostic diagnostic("<stdin>", SourceMgr::DK_Error, msg);
    diagnostic.print(NULL, errs());
}

/* See Builtins/ErrorContext.h. */
void ErrorContext::printMessage(const char *msg, int column)
{
    column += offset;
    SMDiagnostic diagnostic(
        dummySourceMgr,
        SMLoc::getFromPointer(line + column),
        filename,
        lineno,
        column,
        SourceMgr::DK_Error,
        msg,
        line,
        None);
    diagnostic.print(NULL, errs());
}

/* See Builtins/ErrorContext.h. */
void ErrorContext::printMessage(const char *msg, int column, int start, int end)
{
    column += offset;
    start += offset;
    end += offset;

    ArrayRef<std::pair<unsigned, unsigned> > ranges(
        std::pair<unsigned, unsigned>(start, end));

    SMDiagnostic diagnostic(
        dummySourceMgr,
        SMLoc::getFromPointer(line + column),
        filename,
        lineno,
        column,
        SourceMgr::DK_Error,
        msg,
        line,
        ranges);
    diagnostic.print(NULL, errs());
}

/* See Builtins/ErrorContext.h. */
void ErrorContext::printMessage(const char *msg, int column,
                                int start1, int end1, int start2, int end2)
{
    std::vector<std::pair<unsigned, unsigned> > ranges;
    column += offset;
    start1 += offset;
    end1 += offset;
    start2 += offset;
    end2 += offset;

    ranges.emplace_back(start1, end1);
    ranges.emplace_back(start2, end2);
    SMDiagnostic diagnostic(
        dummySourceMgr,
        SMLoc::getFromPointer(line + column),
        filename,
        lineno,
        column,
        SourceMgr::DK_Error,
        msg,
        line,
        ArrayRef<std::pair<unsigned, unsigned> >(ranges));
    diagnostic.print(NULL, errs());
}

}
