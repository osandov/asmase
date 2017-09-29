#include <libasmase.h>
#include <nan.h>

static Nan::Persistent<v8::Object> BigNumFromBuffer;

#include "assembler.h"
#include "instance.h"

void InitAll(Nan::ADDON_REGISTER_FUNCTION_ARGS_TYPE target, v8::Local<v8::Value> module) {
  libasmase_init();

  v8::Local<v8::Object> require = Nan::To<v8::Object>(Nan::Get(Nan::To<v8::Object>(module).ToLocalChecked(), Nan::New("require").ToLocalChecked()).ToLocalChecked()).ToLocalChecked();
  const int argc = 1;
  v8::Local<v8::Value> argv[argc] = {Nan::New("bignum").ToLocalChecked()};
  v8::Local<v8::Object> bignum = Nan::To<v8::Object>(Nan::CallAsFunction(require, Nan::To<v8::Object>(module).ToLocalChecked(), argc, argv).ToLocalChecked()).ToLocalChecked();
  BigNumFromBuffer.Reset(Nan::To<v8::Object>(Nan::Get(bignum, Nan::New("fromBuffer").ToLocalChecked()).ToLocalChecked()).ToLocalChecked());

  Assembler::Init(target);
  Instance::Init(target);
}

NODE_MODULE(binding, InitAll);
