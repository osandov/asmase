/*
 * Copyright (C) 2013 Omar Sandoval
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <llvm/ADT/SmallString.h>
#include <llvm/MC/MCAsmBackend.h>
#include <llvm/MC/MCAsmInfo.h>
#include <llvm/MC/MCContext.h>
#include <llvm/MC/MCInstrInfo.h>
#include <llvm/MC/MCObjectFileInfo.h>
#include <llvm/MC/MCRegisterInfo.h>
#include <llvm/MC/MCStreamer.h>
#include <llvm/MC/MCSubtargetInfo.h>
#include <llvm/MC/MCTargetAsmParser.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Support/ManagedStatic.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
using namespace llvm;

#include "assembler.h"
#include "input.h"

#ifndef LLVM_VERSION_MAJOR
#error "Please use at least LLVM 3.1"
#endif

/** The size of the output SmallString. */
static const int OUTPUT_BUFFER_SIZE = 2048;

/** Return the text section (i.e., machine code) for an object file. */
static error_code getTextSection(object::ObjectFile &objFile,
                                 StringRef &result) {
    error_code err;
    object::section_iterator it = objFile.begin_sections();
    while (it != objFile.end_sections()) {
        bool isText;
        err = it->isText(isText);
        if (err)
            return err;
        if (isText)
            return it->getContents(result);
        it.increment(err);
        if (err)
            return err;
    }
    return error_code();
}

/**
 * Diagnostic callback. We need this because we read input line by line so we
 * have to keep track of diagnostic information on our own in C-land (filename
 * and line number).
 */
static void asmaseDiagHandler(const SMDiagnostic &diag, void *dummy)
{
    struct source_file *file = get_current_file();

    SMDiagnostic diagnostic(
        *diag.getSourceMgr(),
        diag.getLoc(),
        file->filename,
        file->line,
        diag.getColumnNo(),
        diag.getKind(),
        diag.getMessage(),
        diag.getLineContents(),
        diag.getRanges(),
        diag.getFixIts()
    );

    diagnostic.print(NULL, errs());
}

/** LLVM implementation of an assembler. */
struct assembler {
    std::string tripleName;
    std::string cpu;

    const Target *target;
    OwningPtr<MCRegisterInfo> registerInfo;
    OwningPtr<MCAsmInfo> asmInfo;
    OwningPtr<MCInstrInfo> instrInfo;
    OwningPtr<MCSubtargetInfo> subtargetInfo;

    assembler();

    int assembleInput(SourceMgr &srcMgr, MCContext &mcCtx,
                      MCStreamer &streamer);
};

assembler::assembler()
        : tripleName(sys::getDefaultTargetTriple())
{
    std::string err;
    target = TargetRegistry::lookupTarget(tripleName, err);
    assert(target && "Could not get target");

    registerInfo.reset(target->createMCRegInfo(tripleName));
    assert(registerInfo && "Unable to create target register info!");

#if LLVM_VERSION_MAJOR > 3 || LLVM_VERSION_MINOR >= 4
    asmInfo.reset(target->createMCAsmInfo(*registerInfo, tripleName));
#else
    asmInfo.reset(target->createMCAsmInfo(tripleName));
#endif
    assert(asmInfo && "Unable to create target asm info!");

    instrInfo.reset(target->createMCInstrInfo());
    assert(instrInfo && "Unable to create target instruction info!");

    std::string features;
    subtargetInfo.reset(
        target->createMCSubtargetInfo(tripleName, cpu, features));
    assert(subtargetInfo && "Unable to create subtarget info!");
}

int assembler::assembleInput(SourceMgr &srcMgr, MCContext &mcCtx,
                             MCStreamer &streamer)
{
    OwningPtr<MCAsmParser> parser(
        createMCAsmParser(srcMgr, mcCtx, streamer, *asmInfo));

#if LLVM_VERSION_MAJOR > 3 || LLVM_VERSION_MINOR >= 4
    OwningPtr<MCTargetAsmParser> TAP(
        target->createMCAsmParser(*subtargetInfo, *parser, *instrInfo));
#else
    OwningPtr<MCTargetAsmParser> TAP(
        target->createMCAsmParser(*subtargetInfo, *parser));
#endif
    assert(TAP && "This target does not support assembly parsing");

    parser->setTargetParser(*TAP);

    return parser->Run(false);
}

extern "C" {

/* See assembler.h. */
int init_assemblers()
{
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmParser();

    return 0;
}

/* See assembler.h. */
void shutdown_assemblers()
{
    llvm_shutdown();
}

/* See assembler.h. */
struct assembler *create_assembler()
{
    return new assembler();
}

/* See assembler.h. */
void destroy_assembler(struct assembler *ctx)
{
    delete ctx;
}

/* See assembler.h. */
ssize_t assemble_instruction(struct assembler *ctx, const char *in,
                             unsigned char **out, size_t *out_size)
{
    MemoryBuffer *inputBuffer =
        MemoryBuffer::getMemBufferCopy(StringRef(in), "assembly");

    SourceMgr srcMgr;
    srcMgr.AddNewSourceBuffer(inputBuffer, SMLoc());
    srcMgr.setDiagHandler(asmaseDiagHandler);

    OwningPtr<MCObjectFileInfo> objectFileInfo(new MCObjectFileInfo());
#if LLVM_VERSION_MAJOR > 3 || LLVM_VERSION_MINOR >= 4
    MCContext mcCtx(ctx->asmInfo.get(), ctx->registerInfo.get(),
                    objectFileInfo.get(), &srcMgr);
#else
    MCContext mcCtx(*ctx->asmInfo, *ctx->registerInfo, objectFileInfo.get(),
                    &srcMgr);
#endif
    objectFileInfo->InitMCObjectFileInfo(ctx->tripleName, Reloc::Default,
                                         CodeModel::Default, mcCtx);

    SmallString<OUTPUT_BUFFER_SIZE> outputString;
    raw_svector_ostream outputStream(outputString);

    MCCodeEmitter *codeEmitter =
        ctx->target->createMCCodeEmitter(*ctx->instrInfo, *ctx->registerInfo,
                                         *ctx->subtargetInfo, mcCtx);

#if LLVM_VERSION_MAJOR > 3 || LLVM_VERSION_MINOR >= 4
    MCAsmBackend *MAB =
        ctx->target->createMCAsmBackend(*ctx->registerInfo, ctx->tripleName,
                                        ctx->cpu);
#else
    MCAsmBackend *MAB =
        ctx->target->createMCAsmBackend(ctx->tripleName, ctx->cpu);
#endif

    OwningPtr<MCStreamer> streamer(
        createPureStreamer(mcCtx, *MAB, outputStream, codeEmitter));

    if (ctx->assembleInput(srcMgr, mcCtx, *streamer) == 0) {
        outputStream.flush();
        MemoryBuffer *outputBuffer =
            MemoryBuffer::getMemBuffer(outputString, "machine code", false);
        OwningPtr<object::ObjectFile>
            objFile(object::ObjectFile::createELFObjectFile(outputBuffer));

        StringRef textSection;
        error_code err;
        err = getTextSection(*objFile, textSection);
        if (!err) {
            if (textSection.size() > *out_size) {
                *out_size = textSection.size();
                *out = (unsigned char *) realloc(*out, *out_size);
                assert(*out && "Could not reallocate buffer");
            }
            memcpy(*out, textSection.data(), textSection.size());
            return textSection.size();
        }
    }

    return -1;
}

}
