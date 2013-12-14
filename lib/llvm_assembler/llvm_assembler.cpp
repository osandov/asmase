#include "llvm/ADT/SmallString.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCObjectFileInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCTargetAsmParser.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/TargetRegistry.h"
using namespace llvm;

#include "assembler.h"
#include "llvm_assembler.h"

/** The size of the output SmallString. */
static const int OUTPUT_BUFFER_SIZE = 2048;

/** Return the text section (i.e., machine code) for an object file. */
static error_code GetTextSection(object::ObjectFile &ObjFile,
                                 StringRef &Result) {
    error_code Err;
    object::section_iterator it = ObjFile.begin_sections();
    while (it != ObjFile.end_sections()) {
        bool IsText;
        Err = it->isText(IsText);
        if (Err)
            return Err;
        if (IsText)
            return it->getContents(Result);
        it.increment(Err);
        if (Err)
            return Err;
    }
    return error_code();
}

/** LLVM implementation of an assembler. */
struct assembler {
    const char *ProgName;
    std::string ArchName;
    std::string TripleName;
    cl::list<std::string> MAttrs;
    std::string MCPU;

    const Target *TheTarget;
    OwningPtr<MCRegisterInfo> MRI;
    OwningPtr<MCAsmInfo> MAI;
    OwningPtr<MCInstrInfo> MCII;
    OwningPtr<MCSubtargetInfo> STI;

    assembler(const char *ProgName, const std::string &ArchName,
              const std::string &TripleName,
              const cl::list<std::string> &MAttrs,
              const std::string &MCPU);

    int AssembleInput(SourceMgr &SrcMgr, MCContext &Ctx, MCStreamer &Str);
};

assembler::assembler(const char *_PN, const std::string &_AN,
                     const std::string &_TN, const cl::list<std::string> &_MA,
                     const std::string &_MCPU)
        : ProgName(_PN), ArchName(_AN), TripleName(_TN), MAttrs(_MA),
          MCPU(_MCPU)
{
    if (TripleName.empty())
        TripleName = sys::getDefaultTargetTriple();
    Triple TheTriple(Triple::normalize(TripleName));
    TripleName = TheTriple.getTriple();

    std::string Error;
    TheTarget = TargetRegistry::lookupTarget(ArchName, TheTriple, Error);
    if (!TheTarget) {
        errs() << ProgName << ": " << Error;
        assert(!"Could not get target");
    }

    MRI.reset(TheTarget->createMCRegInfo(TripleName));
    assert(MRI && "Unable to create target register info!");

    MAI.reset(TheTarget->createMCAsmInfo(*MRI, TripleName));
    assert(MAI && "Unable to create target asm info!");

    MCII.reset(TheTarget->createMCInstrInfo());
    assert(MCII && "Unable to create target instruction info!");

    std::string FeaturesStr;
    if (MAttrs.size()) {
        SubtargetFeatures Features;
        for (unsigned i = 0; i != MAttrs.size(); ++i)
            Features.AddFeature(MAttrs[i]);
        FeaturesStr = Features.getString();
    }
    STI.reset(TheTarget->createMCSubtargetInfo(TripleName, MCPU, FeaturesStr));
    assert(STI && "Unable to create subtarget info!");
}

int assembler::AssembleInput(SourceMgr &SrcMgr, MCContext &Ctx, MCStreamer &Str)
{
    OwningPtr<MCAsmParser> Parser(createMCAsmParser(SrcMgr, Ctx, Str, *MAI));
    OwningPtr<MCTargetAsmParser>
        TAP(TheTarget->createMCAsmParser(*STI, *Parser, *MCII));

    if (!TAP) {
        errs() << ProgName
               << ": error: this target does not support assembly parsing.\n";
        return 1;
    }

    Parser->setTargetParser(*TAP);

    return Parser->Run(false);
}

extern "C" {

/* See assembler.h. */
struct assembler *create_assembler()
{
    Parameters Params = GetParameters();
    return new assembler(Params.ProgName, Params.ArchName, Params.TripleName,
                         Params.MAttrs, Params.MCPU);
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
    MemoryBuffer *InputBuffer =
        MemoryBuffer::getMemBufferCopy(StringRef(in), "assembly");

    SourceMgr SrcMgr;
    SrcMgr.AddNewSourceBuffer(InputBuffer, SMLoc());

    OwningPtr<MCObjectFileInfo> MOFI(new MCObjectFileInfo());
    MCContext Ctx(ctx->MAI.get(), ctx->MRI.get(), MOFI.get(), &SrcMgr);
    MOFI->InitMCObjectFileInfo(ctx->TripleName, Reloc::Default, CodeModel::Default, Ctx);

    SmallString<OUTPUT_BUFFER_SIZE> OutputString;
    raw_svector_ostream OutputStream(OutputString);

    MCCodeEmitter *CE =
        ctx->TheTarget->createMCCodeEmitter(*ctx->MCII, *ctx->MRI,
                                            *ctx->STI, Ctx);
    MCAsmBackend *MAB =
        ctx->TheTarget->createMCAsmBackend(*ctx->MRI, ctx->TripleName,
                                           ctx->MCPU);
    OwningPtr<MCStreamer> Streamer(createPureStreamer(Ctx, *MAB, OutputStream, CE));

    if (ctx->AssembleInput(SrcMgr, Ctx, *Streamer) == 0) {
        OutputStream.flush();
        MemoryBuffer *OutputBuffer =
            MemoryBuffer::getMemBuffer(OutputString, "machine code", false);
        OwningPtr<object::ObjectFile>
            ObjFile(object::ObjectFile::createELFObjectFile(OutputBuffer));

        StringRef TextSection;
        error_code Err;
        Err = GetTextSection(*ObjFile, TextSection);
        if (!Err) {
            if (TextSection.size() > *out_size) {
                *out_size = TextSection.size();
                *out = (unsigned char *) realloc(*out, *out_size);
                assert(*out && "Could not reallocate buffer");
            }
            memcpy(*out, TextSection.data(), TextSection.size());
            return TextSection.size();
        }
    }

    return -1;
}

}
