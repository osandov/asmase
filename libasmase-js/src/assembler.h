#ifndef BINDING_ASSEMBLER_H
#define BINDING_ASSEMBLER_H

#include <cerrno>
#include <libasmase.h>
#include <nan.h>

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
  struct asmase_assembler* assembler_;

  explicit Assembler(struct asmase_assembler *assembler) : assembler_(assembler) {}

  ~Assembler() {
    asmase_destroy_assembler(assembler_);
  }

  static NAN_METHOD(New) {
    if (info.IsConstructCall()) {
      struct asmase_assembler* assembler = asmase_create_assembler();
      if (!assembler) {
        Nan::ThrowError(Nan::ErrnoException(errno));
        return;
      }
      Assembler *obj = new Assembler(assembler);
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
    char *out;
    size_t len;
    int ret;
    if (info[1]->IsUndefined()) {
      ret = asmase_assemble_code(obj->assembler_, "", line, *code, &out, &len);
    } else {
      Nan::Utf8String filename(info[1]);
      ret = asmase_assemble_code(obj->assembler_, *filename, line, *code, &out, &len);
    }
    if (ret == 0) {
      info.GetReturnValue().Set(Nan::NewBuffer(out, len).ToLocalChecked());
      // The new Buffer now owns out.
    } else if (ret == 1) {
      Nan::ThrowError(out);
      free(out);
    } else {
      Nan::ThrowError(Nan::ErrnoException(errno));
    }
  }

  static inline Nan::Persistent<v8::Function> & constructor() {
    static Nan::Persistent<v8::Function> my_constructor;
    return my_constructor;
  }
};

#endif /* BINDING_ASSEMBLER_H */
