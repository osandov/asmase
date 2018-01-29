/*
 * Copyright (C) 2018 Omar Sandoval
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

#include <nan.h>

#include "assembler.h"

static Nan::Persistent<v8::Object> AssemblerError;

class Assembler : public Nan::ObjectWrap {
public:
  static NAN_MODULE_INIT(Init) {
    v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
    tpl->SetClassName(Nan::New("Assembler").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tpl, "assembleCode", AssembleCode);

    constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());
    Nan::Set(target, Nan::New("Assembler").ToLocalChecked(),
             Nan::GetFunction(tpl).ToLocalChecked());
  }

private:
  std::unique_ptr<AssemblerContext> assembler_;

  explicit Assembler(std::unique_ptr<AssemblerContext> assembler)
    : assembler_{std::move(assembler)} {}

  static NAN_METHOD(New) {
    if (info.IsConstructCall()) {
      std::unique_ptr<AssemblerContext> assembler;
      if (info[0]->IsUndefined()) {
        assembler.reset(AssemblerContext::createAssemblerContext());
      } else if (info[0]->IsString()) {
        Nan::Utf8String architecture(info[0]);
        assembler.reset(AssemblerContext::createAssemblerContext(*architecture));
      } else {
        Nan::ThrowTypeError("architecture must be a string");
        return;
      }
      if (!assembler) {
        Nan::ThrowError("Could not create assembler");
        return;
      }
      Assembler* obj = new Assembler(std::move(assembler));
      obj->Wrap(info.This());
      info.GetReturnValue().Set(info.This());
    } else {
      v8::Local<v8::Function> cons = Nan::New(constructor());
      info.GetReturnValue().Set(Nan::NewInstance(cons).ToLocalChecked());
    }
  }

  static NAN_METHOD(AssembleCode) {
    Assembler* obj = Nan::ObjectWrap::Unwrap<Assembler>(info.Holder());
    if (!info[0]->IsString()) {
      Nan::ThrowTypeError("code must be a string");
      return;
    }
    if (!info[1]->IsUndefined() && !info[1]->IsString()) {
      Nan::ThrowTypeError("filename must be a string");
      return;
    }
    if (!info[2]->IsUndefined() && !info[2]->IsNumber()) {
      Nan::ThrowTypeError("line must be a number");
      return;
    }
    Nan::Utf8String code(info[0]);
    int line = info[2]->IsUndefined() ? 1 : Nan::To<int>(info[2]).FromJust();
    std::pair<std::string, AssemblerResult> result;
    if (info[1]->IsUndefined()) {
      result = obj->assembler_->assembleCode("", line, *code);
    } else {
      Nan::Utf8String filename(info[1]);
      result = obj->assembler_->assembleCode(*filename, line, *code);
    }
    switch (result.second) {
    case AssemblerResult::Success:
      info.GetReturnValue().Set(Nan::CopyBuffer(result.first.c_str(), result.first.size()).ToLocalChecked());
      break;
    case AssemblerResult::Diagnostic:
      {
        const int argc = 1;
        v8::Local<v8::Value> argv[argc] = {Nan::New(result.first).ToLocalChecked()};
        Nan::ThrowError(Nan::CallAsConstructor(Nan::New(AssemblerError), argc, argv).ToLocalChecked());
      }
      break;
    case AssemblerResult::Error:
      Nan::ThrowError(Nan::New(result.first).ToLocalChecked());
      break;
    }
  }

  static inline Nan::Persistent<v8::Function> & constructor() {
    static Nan::Persistent<v8::Function> my_constructor;
    return my_constructor;
  }
};

NAN_MODULE_INIT(InitAll) {
  llvm::InitializeAllTargetInfos();
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmParsers();

  v8::Local<v8::String> scriptString = Nan::New("(function() { class AssemblerError extends Error {}; return AssemblerError; })()").ToLocalChecked();
  v8::Local<Nan::BoundScript> script = Nan::CompileScript(scriptString).ToLocalChecked();
  AssemblerError.Reset(Nan::To<v8::Object>(Nan::RunScript(script).ToLocalChecked()).ToLocalChecked());
  Nan::Set(target, Nan::New("AssemblerError").ToLocalChecked(), Nan::New(AssemblerError));

  Assembler::Init(target);
}

NODE_MODULE(addon, InitAll);
