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

#include <libasmase.h>
#include <nan.h>

static Nan::Persistent<v8::Object> AsmaseError;
static Nan::Persistent<v8::Object> AssemblerError;
static Nan::Persistent<v8::Object> BigNumFromBuffer;

static void ThrowAsmaseError(int errnum)
{
  const int argc = 1;
  v8::Local<v8::Value> argv[argc] = {Nan::New(strerror(errnum)).ToLocalChecked()};
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

// NAN_MODULE_INIT doesn't include the module parameter as of NAN 2.7.0.
void InitAll(Nan::ADDON_REGISTER_FUNCTION_ARGS_TYPE target, v8::Local<v8::Value> module) {
  libasmase_init();

  v8::Local<v8::Object> require = Nan::To<v8::Object>(Nan::Get(Nan::To<v8::Object>(module).ToLocalChecked(), Nan::New("require").ToLocalChecked()).ToLocalChecked()).ToLocalChecked();
  const int argc = 1;
  v8::Local<v8::Value> argv[argc] = {Nan::New("bignum").ToLocalChecked()};
  v8::Local<v8::Object> bignum = Nan::To<v8::Object>(Nan::CallAsFunction(require, Nan::To<v8::Object>(module).ToLocalChecked(), argc, argv).ToLocalChecked()).ToLocalChecked();
  BigNumFromBuffer.Reset(Nan::To<v8::Object>(Nan::Get(bignum, Nan::New("fromBuffer").ToLocalChecked()).ToLocalChecked()).ToLocalChecked());

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
