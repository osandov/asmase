/*
 * Assembler implementation built on the LLVM MC layer.
 *
 * Copyright (C) 2013-2018 Omar Sandoval
 *
 * This file is part of asmase.
 *
 * asmase is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * asmase is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with asmase.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <llvm/Config/llvm-config.h>
#ifndef LLVM_VERSION_MAJOR
#error "Please use at least LLVM 3.1"
#endif
#include <llvm/ADT/SmallString.h>
#include <llvm/MC/MCAsmBackend.h>
#include <llvm/MC/MCAsmInfo.h>
#include <llvm/MC/MCContext.h>
#include <llvm/MC/MCInstrInfo.h>
#include <llvm/MC/MCObjectFileInfo.h>
#include <llvm/MC/MCRegisterInfo.h>
#include <llvm/MC/MCStreamer.h>
#include <llvm/MC/MCSubtargetInfo.h>
#if LLVM_VERSION_MAJOR > 3 || (LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR >= 9)
#include <llvm/MC/MCParser/MCTargetAsmParser.h>
#else
#include <llvm/MC/MCTargetAsmParser.h>
#endif
#include <llvm/Object/ObjectFile.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/ManagedStatic.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
using namespace llvm;

#include <cstdlib>
#include <cstring>
#include <unordered_map>

#if LLVM_VERSION_MAJOR > 3 || (LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR >= 5)
#include <system_error>
#include <memory>
using std::error_code;
using std::error_category;
using std::system_category;
#define OwningPtr std::unique_ptr
#endif

/* The reserved size of the output SmallString. */
static const int OUTPUT_BUFFER_SIZE = 4096;

static const std::unordered_map<std::string, std::string> architectureTriples{
  {"x86_64", "x86_64-unknown-linux-gnu"},
  {"arm", "armv6--linux-gnueabihf"},
};

class DiagHandlerContext {
public:
  StringRef filename;
  int line;
  std::string output;

  DiagHandlerContext(const char *filename, int line)
    : filename{filename}, line{line} {}
};

static void asmaseDiagHandler(const SMDiagnostic &diag, void *arg) {
  DiagHandlerContext& context = *static_cast<DiagHandlerContext *>(arg);

  SMDiagnostic diagnostic{
    *diag.getSourceMgr(),
      diag.getLoc(),
      context.filename,
      context.line,
      diag.getColumnNo(),
      diag.getKind(),
      diag.getMessage(),
      diag.getLineContents(),
      diag.getRanges(),
#if LLVM_VERSION_MAJOR > 3 || (LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR >= 3)
      diag.getFixIts()
#endif
  };

  raw_string_ostream outputStream{context.output};
  diagnostic.print(nullptr, outputStream, false);
}

enum class AssemblerResult {
  Success,
  Diagnostic,
  Error,
};

class AssemblerContext {
private:
  /* LLVM state that can be reused. */
  const std::string tripleName, mcpu, features;
  const Triple triple;
  const Target* target;
  std::unique_ptr<const MCSubtargetInfo> subtargetInfo;
  std::unique_ptr<const MCRegisterInfo> registerInfo;
  std::unique_ptr<const MCAsmInfo> asmInfo;
  std::unique_ptr<const MCInstrInfo> instrInfo;

  static std::pair<std::string, AssemblerResult> getTextSection(object::ObjectFile& objFile);

  AssemblerContext(const std::string& tripleName, const Target* target,
      std::unique_ptr<const MCSubtargetInfo> subtargetInfo,
      std::unique_ptr<const MCRegisterInfo> registerInfo,
      std::unique_ptr<const MCAsmInfo> asmInfo,
      std::unique_ptr<const MCInstrInfo> instrInfo)
    : tripleName{tripleName}, triple{tripleName}, target{target},
    subtargetInfo{std::move(subtargetInfo)},
    registerInfo{std::move(registerInfo)}, asmInfo{std::move(asmInfo)},
    instrInfo{std::move(instrInfo)} {}

  static AssemblerContext* createAssemblerContextFromTriple(const std::string& tripleName);

public:
  static AssemblerContext* createAssemblerContext();
  static AssemblerContext* createAssemblerContext(const std::string& architecture);

  std::pair<std::string, AssemblerResult> assembleCode(const char *filename,
      int line, const char *asm_code);
};

AssemblerContext* AssemblerContext::createAssemblerContextFromTriple(const std::string& tripleName)
{
  std::string err;
  const Target* target = TargetRegistry::lookupTarget(tripleName, err);
  if (!target)
    return nullptr;
  std::unique_ptr<const MCSubtargetInfo> subtargetInfo{
      target->createMCSubtargetInfo(tripleName, "", "")};
  if (!subtargetInfo)
    return nullptr;
  std::unique_ptr<const MCRegisterInfo> registerInfo{
    target->createMCRegInfo(tripleName)};
  if (!registerInfo)
    return nullptr;
  std::unique_ptr<const MCAsmInfo> asmInfo{
#if LLVM_VERSION_MAJOR > 3 || (LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR >= 4)
  target->createMCAsmInfo(*registerInfo, tripleName)
#else
  target->createMCAsmInfo(tripleName)
#endif
  };
  if (!asmInfo)
    return nullptr;
  std::unique_ptr<const MCInstrInfo> instrInfo{
    target->createMCInstrInfo()};
  if (!instrInfo)
    return nullptr;

  return new AssemblerContext{tripleName, target, std::move(subtargetInfo), std::move(registerInfo), std::move(asmInfo), std::move(instrInfo)};
}

AssemblerContext* AssemblerContext::createAssemblerContext() {
  return createAssemblerContextFromTriple(sys::getDefaultTargetTriple());
}

AssemblerContext* AssemblerContext::createAssemblerContext(const std::string& architecture) {
  auto it = architectureTriples.find(architecture);
  if (it == architectureTriples.end())
    return nullptr;
  return createAssemblerContextFromTriple(it->second);
}

std::pair<std::string, AssemblerResult> AssemblerContext::assembleCode(const char *filename,
    int line, const char *asm_code)
{
  // Set up the input
  SourceMgr srcMgr;
  srcMgr.AddNewSourceBuffer(
      MemoryBuffer::getMemBufferCopy(asm_code, "assembly"), SMLoc{});
  DiagHandlerContext diagHandlerCtx{filename, line};
  srcMgr.setDiagHandler(
      asmaseDiagHandler, static_cast<void *>(&diagHandlerCtx));

  // Set up the output
  SmallString<OUTPUT_BUFFER_SIZE> outputString;
  raw_svector_ostream outputStream{outputString};

  // Set up the context
  OwningPtr<MCObjectFileInfo> objectFileInfo{new MCObjectFileInfo()};
#if LLVM_VERSION_MAJOR > 3 || (LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR >= 4)
  MCContext mcCtx{asmInfo.get(), registerInfo.get(), objectFileInfo.get(), &srcMgr};
#else
  MCContext mcCtx{*asmInfo, *registerInfo, objectFileInfo.get(), &srcMgr};
#endif
#if LLVM_VERSION_MAJOR > 3 || (LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR >= 9)
  objectFileInfo->InitMCObjectFileInfo(triple, true, CodeModel::Default, mcCtx);
#elif LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR >= 7
  objectFileInfo->InitMCObjectFileInfo(triple, Reloc::Default,
      CodeModel::Default, mcCtx);
#else
  objectFileInfo->InitMCObjectFileInfo(tripleName, Reloc::Default,
      CodeModel::Default, mcCtx);
#endif

  // Set up the streamer
#if LLVM_VERSION_MAJOR > 3 || (LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR >= 7)
  MCCodeEmitter *codeEmitter =
    target->createMCCodeEmitter(*instrInfo, *registerInfo, mcCtx);
#elif LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR >= 2
  MCCodeEmitter *codeEmitter =
    target->createMCCodeEmitter(*instrInfo, *registerInfo, *subtargetInfo,
        mcCtx);
#else
  MCCodeEmitter *codeEmitter =
    target->createMCCodeEmitter(*instrInfo, *subtargetInfo, mcCtx);
#endif

#if LLVM_VERSION_MAJOR > 3 || (LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR >= 5)
  MCTargetOptions targetOptions;
#endif

#if LLVM_VERSION_MAJOR >= 5
  MCAsmBackend *MAB =
    target->createMCAsmBackend(*registerInfo, tripleName, mcpu, targetOptions);
#elif LLVM_VERSION_MAJOR > 3 || (LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR >= 4)
  MCAsmBackend *MAB =
    target->createMCAsmBackend(*registerInfo, tripleName, mcpu);
#elif LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR >= 2
  MCAsmBackend *MAB = target->createMCAsmBackend(tripleName, mcpu);
#else
  MCAsmBackend *MAB = target->createMCAsmBackend(tripleName);
#endif

#if LLVM_VERSION_MAJOR > 3 || (LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR >= 6)
  OwningPtr<MCStreamer> streamer{
    createELFStreamer(mcCtx, *MAB, outputStream, codeEmitter, true)};
#elif LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR == 4
  OwningPtr<MCStreamer> streamer{
    createELFStreamer(mcCtx, nullptr, *MAB, outputStream, codeEmitter, true,
        false)};
#else
  OwningPtr<MCStreamer> streamer{
    createELFStreamer(mcCtx, *MAB, outputStream, codeEmitter, true, false)};
#endif

  // Set up the parser
  OwningPtr<MCAsmParser> parser{
    createMCAsmParser(srcMgr, mcCtx, *streamer, *asmInfo)};

#if LLVM_VERSION_MAJOR > 3 || (LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR >= 5)
  OwningPtr<MCTargetAsmParser> TAP{
    target->createMCAsmParser(*subtargetInfo, *parser, *instrInfo,
        targetOptions)};
#elif LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR >= 4
  OwningPtr<MCTargetAsmParser> TAP{
    target->createMCAsmParser(*subtargetInfo, *parser, *instrInfo)};
#else
  OwningPtr<MCTargetAsmParser> TAP{
    target->createMCAsmParser(*subtargetInfo, *parser)};
#endif
  assert(TAP && "This target does not support assembly parsing");
  parser->setTargetParser(*TAP);

  if (parser->Run(false) != 0)
    return std::make_pair(diagHandlerCtx.output, AssemblerResult::Diagnostic);

#if LLVM_VERSION_MAJOR < 3 || (LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR < 8)
  outputStream.flush();
#endif
#if LLVM_VERSION_MAJOR > 3 || (LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR >= 6)
  std::unique_ptr<MemoryBuffer> outputBuffer{
    MemoryBuffer::getMemBuffer(outputString, "machine code", false)};
  auto objFileErr = object::ObjectFile::createELFObjectFile(outputBuffer->getMemBufferRef());
  if (error_code err = objFileErr.getError())
    return std::make_pair(err.message(), AssemblerResult::Error);
  std::unique_ptr<object::ObjectFile> objFile{objFileErr->release()};
#elif LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR >= 5
  std::unique_ptr<MemoryBuffer> outputBuffer{
    MemoryBuffer::getMemBuffer(outputString, "machine code", false)};
  auto objFileErr = object::ObjectFile::createELFObjectFile(outputBuffer);
  if (error_code err = objFileErr.getError())
    return std::make_pair(err.message(), AssemblerResult::Error);
  auto objFile = *objFileErr;
#else
  MemoryBuffer *outputBuffer =
    MemoryBuffer::getMemBuffer(outputString, "machine code", false);
  OwningPtr<object::ObjectFile>
    objFile{object::ObjectFile::createELFObjectFile(outputBuffer)};
#endif

  return getTextSection(*objFile);
}

/* Return the text section (i.e., machine code) for an object file. */
std::pair<std::string, AssemblerResult> AssemblerContext::getTextSection(object::ObjectFile& objFile) {
  error_code err;
#if LLVM_VERSION_MAJOR > 3 || (LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR >= 5)
  object::section_iterator it = objFile.section_begin();
  while (it != objFile.section_end()) {
#else
  object::section_iterator it = objFile.begin_sections();
  while (it != objFile.end_sections()) {
#endif
    bool isText;
#if LLVM_VERSION_MAJOR > 3 || (LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR >= 6)
    isText = it->isText();
#else
    err = it->isText(isText);
    if (err)
      return std::make_pair(err.message(), AssemblerResult::Error);
#endif
    if (isText) {
      StringRef contents;
      err = it->getContents(contents);
      if (err)
        return std::make_pair(err.message(), AssemblerResult::Error);
      return std::make_pair(contents, AssemblerResult::Success);
    }
#if LLVM_VERSION_MAJOR > 3 || (LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR >= 5)
    ++it;
#else
    it.increment(err);
    if (err)
      return std::make_pair(err.message(), AssemblerResult::Error);
#endif
  }
  return std::make_pair("Could not find assembled text section",
      AssemblerResult::Error);
}
