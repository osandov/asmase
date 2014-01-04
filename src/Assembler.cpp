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

#ifndef LLVM_VERSION_MAJOR
#error "Please use at least LLVM 3.1"
#endif

#include "Assembler.h"
#include "Inputter.h"

/** The size of the output SmallString. */
static const int OUTPUT_BUFFER_SIZE = 4096;

/** Return the text section (i.e., machine code) for an object file. */
static error_code getTextSection(object::ObjectFile &objFile,
                                 StringRef &result);

/**
 * Diagnostic callback. We need this because we read input line by line so we
 * have to keep track of diagnostic information on our own in C-land (filename
 * and line number).
 */
static void asmaseDiagHandler(const SMDiagnostic &diag, void *arg);

class AssemblerContext {
    static bool llvmIsInit;

public:
    std::string tripleName;
    std::string cpu;
    const Target *target;
    OwningPtr<MCRegisterInfo> registerInfo;
    OwningPtr<MCAsmInfo> asmInfo;
    OwningPtr<MCInstrInfo> instrInfo;

    AssemblerContext()
        : tripleName{sys::getDefaultTargetTriple()}
    {
        if (!llvmIsInit) {
            llvm::InitializeNativeTarget();
            llvm::InitializeNativeTargetAsmParser();
            llvmIsInit = true;
        }

        std::string err;
        target = TargetRegistry::lookupTarget(tripleName, err);
        assert(target && "Could not get target!");

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
    }
};

bool AssemblerContext::llvmIsInit = false;

std::shared_ptr<AssemblerContext> createAssemblerContext()
{
    return std::shared_ptr<AssemblerContext>{new AssemblerContext};
}

int Assembler::assembleInstruction(const std::string &instruction,
                                   std::string &machineCodeOut,
                                   const Inputter &inputter)
{
    const std::string &tripleName = context->tripleName;
    const std::string &mcpu = context->cpu;
    const Target *target = context->target;
    const MCRegisterInfo *registerInfo = context->registerInfo.get();
    const MCAsmInfo *asmInfo = context->asmInfo.get();
    const MCInstrInfo *instrInfo = context->instrInfo.get();

    // Set up the input
    MemoryBuffer *inputBuffer =
        MemoryBuffer::getMemBufferCopy(instruction, "assembly");

    SourceMgr srcMgr;
    srcMgr.AddNewSourceBuffer(inputBuffer, SMLoc());
    srcMgr.setDiagHandler(asmaseDiagHandler,
                          const_cast<void *>((const void *) &inputter));

    // Set up the output
    SmallString<OUTPUT_BUFFER_SIZE> outputString;
    raw_svector_ostream outputStream(outputString);

    // Set up the context
    OwningPtr<MCObjectFileInfo> objectFileInfo(new MCObjectFileInfo());
#if LLVM_VERSION_MAJOR > 3 || LLVM_VERSION_MINOR >= 4
    MCContext mcCtx(asmInfo, registerInfo, objectFileInfo.get(), &srcMgr);
#else
    MCContext mcCtx(*asmInfo, *registerInfo, objectFileInfo.get(), &srcMgr);
#endif
    objectFileInfo->InitMCObjectFileInfo(tripleName, Reloc::Default,
                                         CodeModel::Default, mcCtx);

    // Set up the streamer
    OwningPtr<MCSubtargetInfo> subtargetInfo;
    std::string features;
    subtargetInfo.reset(
        target->createMCSubtargetInfo(tripleName, mcpu, features));
    assert(subtargetInfo && "Unable to create subtarget info!");

    MCCodeEmitter *codeEmitter =
        target->createMCCodeEmitter(*instrInfo, *registerInfo, *subtargetInfo,
                                    mcCtx);
#if LLVM_VERSION_MAJOR > 3 || LLVM_VERSION_MINOR >= 4
    MCAsmBackend *MAB =
        target->createMCAsmBackend(*registerInfo, tripleName, mcpu);
#else
    MCAsmBackend *MAB =
        target->createMCAsmBackend(tripleName, mcpu);
#endif

    OwningPtr<MCStreamer> streamer(
        createPureStreamer(mcCtx, *MAB, outputStream, codeEmitter));

    // Set up the parser
    OwningPtr<MCAsmParser> parser(
        createMCAsmParser(srcMgr, mcCtx, *streamer, *asmInfo));

#if LLVM_VERSION_MAJOR > 3 || LLVM_VERSION_MINOR >= 4
    OwningPtr<MCTargetAsmParser> TAP(
        target->createMCAsmParser(*subtargetInfo, *parser, *instrInfo));
#else
    OwningPtr<MCTargetAsmParser> TAP(
        target->createMCAsmParser(*subtargetInfo, *parser));
#endif
    assert(TAP && "This target does not support assembly parsing");
    parser->setTargetParser(*TAP);

    if (parser->Run(false) == 0) {
        outputStream.flush();
        MemoryBuffer *outputBuffer =
            MemoryBuffer::getMemBuffer(outputString, "machine code", false);
        OwningPtr<object::ObjectFile>
            objFile(object::ObjectFile::createELFObjectFile(outputBuffer));

        StringRef textSection;
        error_code err;
        err = getTextSection(*objFile, textSection);
        if (!err) {
            machineCodeOut = textSection;
            return 0;
        }
    }

    return 1;
}

/* See above. */
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

/* See above. */
static void asmaseDiagHandler(const SMDiagnostic &diag, void *arg)
{
    const Inputter &inputter = *static_cast<const Inputter *>(arg);

    SMDiagnostic diagnostic{
        *diag.getSourceMgr(),
        diag.getLoc(),
        inputter.currentFilename().c_str(),
        inputter.currentLineno(),
        diag.getColumnNo(),
        diag.getKind(),
        diag.getMessage(),
        diag.getLineContents(),
        diag.getRanges(),
        diag.getFixIts()
    };
    diagnostic.print(nullptr, errs());
}
