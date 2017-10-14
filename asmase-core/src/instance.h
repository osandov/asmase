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
    Nan::SetPrototypeMethod(tpl, "getRegisterSets", GetRegisterSets);
    Nan::SetPrototypeMethod(tpl, "getRegisters", GetRegisters);
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

  static NAN_METHOD(GetRegisterSets) {
    Instance* obj = Nan::ObjectWrap::Unwrap<Instance>(info.Holder());
    info.GetReturnValue().Set(Nan::New(asmase_get_register_sets(obj->instance_)));
  }

  static Nan::MaybeLocal<v8::Value> GetRegisterValue(const struct asmase_register* reg) {
    v8::Local<v8::Object> buffer;
    switch (reg->type) {
    case ASMASE_REGISTER_U8:
      buffer = Nan::NewBuffer(1).ToLocalChecked();
      memcpy(node::Buffer::Data(buffer), &reg->u8, 1);
      goto bignum;
    case ASMASE_REGISTER_U16:
      buffer = Nan::NewBuffer(2).ToLocalChecked();
      memcpy(node::Buffer::Data(buffer), &reg->u16, 2);
      goto bignum;
    case ASMASE_REGISTER_U32:
      buffer = Nan::NewBuffer(4).ToLocalChecked();
      memcpy(node::Buffer::Data(buffer), &reg->u32, 4);
      goto bignum;
    case ASMASE_REGISTER_U64:
      buffer = Nan::NewBuffer(8).ToLocalChecked();
      memcpy(node::Buffer::Data(buffer), &reg->u64, 8);
      goto bignum;
    case ASMASE_REGISTER_U128:
      buffer = Nan::NewBuffer(16).ToLocalChecked();
      memcpy(node::Buffer::Data(buffer), &reg->u128, 16);
      goto bignum;
    case ASMASE_REGISTER_FLOAT80:
      /* TODO: this loses precision, we need a Float80 type */
      return Nan::New((double)reg->float80);
    default:
      Nan::ThrowError(v8::String::Concat(Nan::New("unknown register type ").ToLocalChecked(),
                                         Nan::New(reg->type)->ToString()));
      return Nan::MaybeLocal<v8::Value>();
    }

bignum:
    v8::Local<v8::Object> opt = Nan::New<v8::Object>();
#ifdef ASMASE_LITTLE_ENDIAN
    v8::Local<v8::String> endian = Nan::New("little").ToLocalChecked();
#else
    v8::Local<v8::String> endian = Nan::New("big").ToLocalChecked();
#endif
    Nan::Set(opt, Nan::New("endian").ToLocalChecked(), endian);
    Nan::Set(opt, Nan::New("size").ToLocalChecked(), Nan::New("auto").ToLocalChecked());
    const int argc = 2;
    v8::Local<v8::Value> argv[argc] = {buffer, opt};
    return Nan::CallAsFunction(Nan::New(BigNumFromBuffer),
                               Nan::GetCurrentContext()->Global(),
                               argc, argv);
  }

  static Nan::MaybeLocal<v8::Set> GetRegisterBits(const struct asmase_register* reg) {
    uint64_t value;
    switch (reg->type) {
    case ASMASE_REGISTER_U8:
      value = reg->u8;
      break;
    case ASMASE_REGISTER_U16:
      value = reg->u16;
      break;
    case ASMASE_REGISTER_U32:
      value = reg->u32;
      break;
    case ASMASE_REGISTER_U64:
      value = reg->u64;
      break;
    default:
      Nan::ThrowError(v8::String::Concat(Nan::New("status register bits on unexpected type ").ToLocalChecked(),
                                         Nan::New(reg->type)->ToString()));
      return Nan::MaybeLocal<v8::Set>();
    }

    v8::Local<v8::Set> set = v8::Set::New(v8::Isolate::GetCurrent());
    for (size_t i = 0; i < reg->num_status_bits; i++) {
      char* flag = asmase_status_register_format(&reg->status_bits[i], value);
      if (!flag) {
        ThrowAsmaseError(errno);
        return Nan::MaybeLocal<v8::Set>();
      }
      if (!*flag) {
        free(flag);
        continue;
      }
      if (set->Add(Nan::GetCurrentContext(), Nan::New(flag).ToLocalChecked()).IsEmpty()) {
        return Nan::MaybeLocal<v8::Set>();
      }
      free(flag);
    }
    return set;
  }

  static NAN_METHOD(GetRegisters) {
    Instance* obj = Nan::ObjectWrap::Unwrap<Instance>(info.Holder());
    if (!info[0]->IsNumber()) {
      Nan::ThrowTypeError("regSets must be a number");
      return;
    }
    int reg_sets = Nan::To<int>(info[0]).FromJust();
    struct asmase_register *regs;
    size_t num_regs;
    int ret = asmase_get_registers(obj->instance_, reg_sets, &regs, &num_regs);
    if (ret == -1) {
      ThrowAsmaseError(errno);
      return;
    }
    v8::Local<v8::Object> regsObj = Nan::New<v8::Object>();
    for (size_t i = 0; i < num_regs; i++) {
      v8::Local<v8::Object> regObj = Nan::New<v8::Object>();
      Nan::Set(regObj, Nan::New("type").ToLocalChecked(), Nan::New(regs[i].type));
      Nan::Set(regObj, Nan::New("set").ToLocalChecked(), Nan::New(regs[i].set));
      v8::Local<v8::Value> value;
      if (!Instance::GetRegisterValue(&regs[i]).ToLocal(&value)) {
        goto err;
      }
      Nan::Set(regObj, Nan::New("value").ToLocalChecked(), value);
      if (regs[i].num_status_bits) {
        v8::Local<v8::Set> bits;
        if (!Instance::GetRegisterBits(&regs[i]).ToLocal(&bits)) {
          goto err;
        }
        Nan::Set(regObj, Nan::New("bits").ToLocalChecked(), bits);
      }
      Nan::Set(regsObj, Nan::New(regs[i].name).ToLocalChecked(), regObj);
    }

    info.GetReturnValue().Set(regsObj);
err:
    free(regs);
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
