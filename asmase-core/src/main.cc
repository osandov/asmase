/*
 * Copyright (C) 2017 Omar Sandoval
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

#include <unordered_map>
#include <libasmase.h>
#include <nan.h>

static std::unordered_map<std::string, const struct asmase_register_descriptor *> registers_table;

static Nan::Persistent<v8::Object> AsmaseError;
static Nan::Persistent<v8::Object> AssemblerError;

static void ThrowAsmaseError(int errnum)
{
  const int argc = 1;
  v8::Local<v8::Value> argv[argc] = {Nan::New(strerror(errnum)).ToLocalChecked()};
  Nan::ThrowError(Nan::CallAsConstructor(Nan::New(AsmaseError), argc, argv).ToLocalChecked());
}

static void ThrowAsmaseError(const std::string& error)
{
  const int argc = 1;
  v8::Local<v8::Value> argv[argc] = {Nan::New(error).ToLocalChecked()};
  Nan::ThrowError(Nan::CallAsConstructor(Nan::New(AsmaseError), argc, argv).ToLocalChecked());
}

static void ThrowAssemblerError(const char *diagnostic)
{
  const int argc = 1;
  v8::Local<v8::Value> argv[argc] = {Nan::New(diagnostic).ToLocalChecked()};
  Nan::ThrowError(Nan::CallAsConstructor(Nan::New(AssemblerError), argc, argv).ToLocalChecked());
}

#include "assembler.h"
#include "instance.h"

NAN_MODULE_INIT(InitAll) {
  libasmase_init();

  for (size_t i = 0; i < asmase_num_registers; i++) {
    registers_table[asmase_registers[i].name] = &asmase_registers[i];
  }

  {
    v8::Local<v8::String> scriptString = Nan::New("class AsmaseError extends Error {}; AsmaseError").ToLocalChecked();
    v8::Local<Nan::BoundScript> script = Nan::CompileScript(scriptString).ToLocalChecked();
    AsmaseError.Reset(Nan::To<v8::Object>(Nan::RunScript(script).ToLocalChecked()).ToLocalChecked());
  }

  {
    v8::Local<v8::String> scriptString = Nan::New("class AssemblerError extends Error {}; AssemblerError").ToLocalChecked();
    v8::Local<Nan::BoundScript> script = Nan::CompileScript(scriptString).ToLocalChecked();
    AssemblerError.Reset(Nan::To<v8::Object>(Nan::RunScript(script).ToLocalChecked()).ToLocalChecked());
  }

  Nan::Set(target, Nan::New("AsmaseError").ToLocalChecked(), Nan::New(AsmaseError));
  Nan::Set(target, Nan::New("AssemblerError").ToLocalChecked(), Nan::New(AssemblerError));

  Assembler::Init(target);
  Instance::Init(target);
}

NODE_MODULE(binding, InitAll);
