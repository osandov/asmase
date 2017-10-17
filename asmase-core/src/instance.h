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

#ifndef BINDING_INSTANCE
#define BINDING_INSTANCE

#include <cerrno>
#include <cfloat>
#include <cinttypes>
#include <string>
#include <unordered_map>
#include <libasmase.h>
#include <nan.h>
#include <sys/wait.h>

static std::string signame(int sig) {
  static const std::unordered_map<int, const std::string> names{
    {SIGHUP, "SIGHUP"},
    {SIGINT, "SIGINT"},
    {SIGQUIT, "SIGQUIT"},
    {SIGILL, "SIGILL"},
    {SIGTRAP, "SIGTRAP"},
    {SIGABRT, "SIGABRT"},
    {SIGIOT, "SIGIOT"},
    {SIGBUS, "SIGBUS"},
    {SIGFPE, "SIGFPE"},
    {SIGKILL, "SIGKILL"},
    {SIGUSR1, "SIGUSR1"},
    {SIGSEGV, "SIGSEGV"},
    {SIGUSR2, "SIGUSR2"},
    {SIGPIPE, "SIGPIPE"},
    {SIGALRM, "SIGALRM"},
    {SIGTERM, "SIGTERM"},
    {SIGCHLD, "SIGCHLD"},
    {SIGSTKFLT, "SIGSTKFLT"},
    {SIGCONT, "SIGCONT"},
    {SIGSTOP, "SIGSTOP"},
    {SIGTSTP, "SIGTSTP"},
    {SIGTTIN, "SIGTTIN"},
    {SIGTTOU, "SIGTTOU"},
    {SIGURG, "SIGURG"},
    {SIGXCPU, "SIGXCPU"},
    {SIGXFSZ, "SIGXFSZ"},
    {SIGVTALRM, "SIGVTALRM"},
    {SIGPROF, "SIGPROF"},
    {SIGWINCH, "SIGWINCH"},
    {SIGIO, "SIGIO"},
    {SIGPOLL, "SIGPOLL"},
    {SIGPWR, "SIGPWR"},
    {SIGSYS, "SIGSYS"},
  };

  const auto iter = names.find(sig);
  if (iter == names.end()) {
    return std::to_string(sig);
  } else {
    return iter->second;
  }
}

class Instance : public Nan::ObjectWrap {
public:
  static NAN_MODULE_INIT(Init) {
    v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
    tpl->SetClassName(Nan::New("Instance").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tpl, "getPid", GetPid);
    Nan::SetPrototypeMethod(tpl, "getRegister", GetRegister);
    Nan::SetPrototypeMethod(tpl, "executeCode", ExecuteCode);
    Nan::SetPrototypeMethod(tpl, "readMemory", ReadMemory);

    constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());
    Nan::Set(target, Nan::New("Instance").ToLocalChecked(),
             Nan::GetFunction(tpl).ToLocalChecked());
  }

private:
  struct asmase_instance* instance_;

  explicit Instance(struct asmase_instance *instance) : instance_(instance) {}

  ~Instance() {
    asmase_destroy_instance(instance_);
  }

  static NAN_METHOD(New) {
    if (info.IsConstructCall()) {
      int flags = info[0]->IsUndefined() ? 0 : Nan::To<int>(info[0]).FromJust();
      struct asmase_instance* instance = asmase_create_instance(flags);
      if (!instance) {
        ThrowAsmaseError(errno);
        return;
      }
      Instance *obj = new Instance(instance);
      obj->Wrap(info.This());
      info.GetReturnValue().Set(info.This());
    } else {
      v8::Local<v8::Function> cons = Nan::New(constructor());
      info.GetReturnValue().Set(Nan::NewInstance(cons).ToLocalChecked());
    }
  }

  static NAN_METHOD(GetPid) {
    Instance* obj = Nan::ObjectWrap::Unwrap<Instance>(info.Holder());
    info.GetReturnValue().Set(Nan::New(asmase_getpid(obj->instance_)));
  }

  static Nan::MaybeLocal<v8::Array> GetRegisterBits(const struct asmase_register_descriptor* reg, const union asmase_register_value* value) {
    v8::Local<v8::Array> bits = Nan::New<v8::Array>();
    for (size_t i = 0; i < reg->num_status_bits; i++) {
      char* flag = asmase_status_register_format(reg, &reg->status_bits[i], value);
      if (!flag) {
        ThrowAsmaseError(errno);
        return Nan::MaybeLocal<v8::Array>();
      }
      if (!*flag) {
        free(flag);
        continue;
      }
      Nan::Set(bits, bits->Length(), Nan::New(flag).ToLocalChecked());
      free(flag);
    }
    return bits;
  }

  static NAN_METHOD(GetRegister) {
    Instance* obj = Nan::ObjectWrap::Unwrap<Instance>(info.Holder());
    if (!info[0]->IsString()) {
      Nan::ThrowTypeError("register must be a string");
      return;
    }

    Nan::Utf8String code(info[0]);
    const auto iter = registers_table.find(*code);
    if (iter == registers_table.end()) {
      std::string error{"unknown register: "};
      error += *code;
      ThrowAsmaseError(error);
      return;
    }

    const struct asmase_register_descriptor* reg = iter->second;
    union asmase_register_value value;
    asmase_get_register(obj->instance_, reg, &value);
    char buf[40];
    switch (reg->type) {
    case ASMASE_REGISTER_U8:
      sprintf(buf, "0x%02" PRIx8, value.u8);
      break;
    case ASMASE_REGISTER_U16:
      sprintf(buf, "0x%04" PRIx16, value.u16);
      break;
    case ASMASE_REGISTER_U32:
      sprintf(buf, "0x%08" PRIx32, value.u32);
      break;
    case ASMASE_REGISTER_U64:
      sprintf(buf, "0x%016" PRIx64, value.u64);
      break;
    case ASMASE_REGISTER_U128:
      sprintf(buf, "0x%016" PRIx64 "%016" PRIx64, value.u128_hi, value.u128_lo);
      break;
    case ASMASE_REGISTER_FLOAT80:
      sprintf(buf, "%.*Lf", DECIMAL_DIG, value.float80);
      break;
    default:
      assert(false && "unknown register type");
      return;
    }

    v8::Local<v8::Object> result = Nan::New<v8::Object>();
    Nan::Set(result, Nan::New("type").ToLocalChecked(), Nan::New(reg->type));
    Nan::Set(result, Nan::New("set").ToLocalChecked(), Nan::New(reg->set));
    Nan::Set(result, Nan::New("value").ToLocalChecked(), Nan::New(buf).ToLocalChecked());
    if (reg->num_status_bits) {
      v8::Local<v8::Array> bits;
      if (!Instance::GetRegisterBits(reg, &value).ToLocal(&bits)) {
        return;
      }
      Nan::Set(result, Nan::New("bits").ToLocalChecked(), bits);
    }

    info.GetReturnValue().Set(result);
  }

  static v8::Local<v8::Object> WstatusObject(int wstatus) {
    v8::Local<v8::Object> obj = Nan::New<v8::Object>();
    const char *state;
    if (WIFEXITED(wstatus)) {
      state = "exited";
      Nan::Set(obj, Nan::New("exitstatus").ToLocalChecked(),
               Nan::New(WEXITSTATUS(wstatus)));
    } else if (WIFSIGNALED(wstatus)) {
      state = "signaled";
      Nan::Set(obj, Nan::New("termsig").ToLocalChecked(),
               Nan::New(signame(WTERMSIG(wstatus))).ToLocalChecked());
      Nan::Set(obj, Nan::New("coredump").ToLocalChecked(),
               Nan::New<v8::Boolean>(WCOREDUMP(wstatus)));
    } else if (WIFSTOPPED(wstatus)) {
      state = "stopped";
      Nan::Set(obj, Nan::New("stopsig").ToLocalChecked(),
               Nan::New(signame(WSTOPSIG(wstatus))).ToLocalChecked());
    } else if (WIFCONTINUED(wstatus)) {
      state = "continued";
    } else {
      assert(false);
    }
    Nan::Set(obj, Nan::New("state").ToLocalChecked(),
             Nan::New(state).ToLocalChecked());
    return obj;
  }

  static NAN_METHOD(ExecuteCode) {
    Instance* obj = Nan::ObjectWrap::Unwrap<Instance>(info.Holder());
    if (!node::Buffer::HasInstance(info[0])) {
      Nan::ThrowTypeError("code must be a buffer");
      return;
    }
    char* code = node::Buffer::Data(info[0]);
    size_t len = node::Buffer::Length(info[0]);
    int wstatus;
    int ret = asmase_execute_code(obj->instance_, code, len, &wstatus);
    if (ret == -1) {
      ThrowAsmaseError(errno);
      return;
    }
    info.GetReturnValue().Set(WstatusObject(wstatus));
  }

  static NAN_METHOD(ReadMemory) {
    Instance* obj = Nan::ObjectWrap::Unwrap<Instance>(info.Holder());
    if (!info[0]->IsNumber() && !info[0]->IsString()) {
      Nan::ThrowTypeError("addr must be a number or a string");
      return;
    }
    if (!info[1]->IsNumber()) {
      Nan::ThrowTypeError("len must be a number");
      return;
    }
    intptr_t addr;
    if (info[0]->IsString()) {
      Nan::Utf8String addrStr(info[0]);
      char *end;
      errno = 0;
      addr = strtoul(*addrStr, &end, 0);
      if (errno == ERANGE) {
        Nan::ThrowRangeError("addr is too big");
        return;
      } else if (!**addrStr || *end) {
        Nan::ThrowError("addr is invalid");
        return;
      }
    } else {
      addr = Nan::To<int64_t>(info[0]).FromJust();
    }
    size_t len = Nan::To<int64_t>(info[1]).FromJust();
    v8::Local<v8::Object> buffer;
    if (!Nan::NewBuffer(len).ToLocal(&buffer)) {
      return;
    }
    int ret = asmase_read_memory(obj->instance_, node::Buffer::Data(buffer),
                                 reinterpret_cast<void*>(addr), len);
    if (ret == -1) {
      ThrowAsmaseError(errno);
      return;
    }
    info.GetReturnValue().Set(buffer);
  }

  static inline Nan::Persistent<v8::Function> & constructor() {
    static Nan::Persistent<v8::Function> my_constructor;
    return my_constructor;
  }
};

#endif /* BINDING_INSTANCE */
