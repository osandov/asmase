/*
 * Copyright (C) 2017-2018 Omar Sandoval
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

#include <cfloat>
#include <cinttypes>
#include <list>
#include <tuple>
#include <unordered_map>

#include <libasmase.h>
#include <nan.h>
#include <uv.h>
#include <sys/wait.h>

static Nan::Global<v8::Object> AsmaseError;
static Nan::Global<v8::Object> Promise;
static Nan::Global<v8::Map> Registers;

static std::unordered_map<std::string, const struct asmase_register_descriptor*> registers_table;

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

static v8::Local<v8::Object> wstatusObject(int wstatus) {
  v8::Local<v8::Object> obj = Nan::New<v8::Object>();
  if (WIFEXITED(wstatus)) {
    Nan::Set(obj, Nan::New("state").ToLocalChecked(),
        Nan::New("exited").ToLocalChecked());
    Nan::Set(obj, Nan::New("exitStatus").ToLocalChecked(),
        Nan::New(WEXITSTATUS(wstatus)));
  } else if (WIFSIGNALED(wstatus)) {
    Nan::Set(obj, Nan::New("state").ToLocalChecked(),
        Nan::New("signaled").ToLocalChecked());
    Nan::Set(obj, Nan::New("signal").ToLocalChecked(),
        Nan::New(signame(WTERMSIG(wstatus))).ToLocalChecked());
  } else if (WIFSTOPPED(wstatus)) {
    Nan::Set(obj, Nan::New("state").ToLocalChecked(),
        Nan::New("stopped").ToLocalChecked());
    Nan::Set(obj, Nan::New("signal").ToLocalChecked(),
        Nan::New(signame(WSTOPSIG(wstatus))).ToLocalChecked());
  } else if (WIFCONTINUED(wstatus)) {
    Nan::Set(obj, Nan::New("state").ToLocalChecked(),
        Nan::New("continued").ToLocalChecked());
  } else {
    assert(false);
  }
  return obj;
}

static v8::Local<v8::Value> NewAsmaseError(int errnum)
{
  const int argc = 1;
  v8::Local<v8::Value> argv[argc] = {Nan::New(strerror(errnum)).ToLocalChecked()};
  return Nan::CallAsConstructor(Nan::New(AsmaseError), argc, argv).ToLocalChecked();
}

static void ThrowAsmaseError(int errnum)
{
  Nan::ThrowError(NewAsmaseError(errnum));
}

static void ThrowAsmaseError(const std::string& error)
{
  const int argc = 1;
  v8::Local<v8::Value> argv[argc] = {Nan::New(error).ToLocalChecked()};
  Nan::ThrowError(Nan::CallAsConstructor(Nan::New(AsmaseError), argc, argv).ToLocalChecked());
}

class Instance : public Nan::ObjectWrap {
private:
  // Instance, isCreate, resolve, reject
  static std::list<std::tuple<Nan::Persistent<v8::Object>, bool, Nan::Callback, Nan::Callback>> callbacks;
  static std::list<pid_t> destroyed;
  static uv_signal_t sigchld;

  struct asmase_instance* instance_;
  bool reaped_;

  explicit Instance() : instance_(nullptr), reaped_(false) {}

  ~Instance() {
    if (instance_)
      killAndDestroy();
  }

  bool checkInstance() {
    if (instance_) {
      return true;
    } else {
      Nan::ThrowError("Instance was destroyed or constructed directly");
      return false;
    }
  }

  void killAndDestroy() {
    assert(instance_);
    if (!reaped_) {
      pid_t pid = asmase_getpid(instance_);
      // This should never fail, but if it does, there's nothing we can do about
      // it.
      if (::kill(pid, SIGKILL) == 0) {
        auto it = destroyed.emplace(destroyed.begin(), pid);
        pollDestroyed(it);
      }
    }
    asmase_destroy_instance(instance_);
    instance_ = nullptr;
  }

  static NAN_METHOD(New) {
    if (info.IsConstructCall()) {
      Instance* obj = new Instance();
      obj->Wrap(info.This());
      info.GetReturnValue().Set(info.This());
    } else {
      v8::Local<v8::Function> cons = Nan::New(constructor());
      info.GetReturnValue().Set(Nan::NewInstance(cons).ToLocalChecked());
    }
  }

  static NAN_METHOD(Destroy) {
    Instance* obj = Nan::ObjectWrap::Unwrap<Instance>(info.Holder());
    if (!obj->checkInstance()) {
      return;
    }
    obj->killAndDestroy();
  }

  static NAN_METHOD(ExecuteCodeExecutor) {
    v8::Local<v8::Function> resolve = Nan::To<v8::Function>(info[0]).ToLocalChecked();
    v8::Local<v8::Function> reject = Nan::To<v8::Function>(info[1]).ToLocalChecked();

    v8::Local<v8::Object> data = Nan::To<v8::Object>(info.Data()).ToLocalChecked();
    v8::Local<v8::Value> buffer = Nan::Get(data, Nan::New(1)).ToLocalChecked();
    if (!node::Buffer::HasInstance(buffer)) {
      const int argc = 1;
      v8::Local<v8::Value> argv[argc] = {Nan::TypeError("code must be a buffer")};
      Nan::MakeCallback(Nan::GetCurrentContext()->Global(), reject, argc, argv);
      return;
    }

    v8::Local<v8::Object> obj = Nan::To<v8::Object>(Nan::Get(data, Nan::New(0)).ToLocalChecked()).ToLocalChecked();
    struct asmase_instance* instance = Nan::ObjectWrap::Unwrap<Instance>(obj)->instance_;
    char* code = node::Buffer::Data(buffer);
    size_t len = node::Buffer::Length(buffer);
    if (asmase_execute_code(instance, code, len) == -1) {
      const int argc = 1;
      v8::Local<v8::Value> argv[argc] = {NewAsmaseError(errno)};
      Nan::MakeCallback(Nan::GetCurrentContext()->Global(), reject, argc, argv);
      return;
    }

    auto it = callbacks.emplace(callbacks.begin(), obj, false, resolve, reject);
    pollInstance(it);
  }

  static NAN_METHOD(ExecuteCode) {
    Instance* obj = Nan::ObjectWrap::Unwrap<Instance>(info.Holder());
    if (!obj->checkInstance()) {
      return;
    }
    v8::Local<v8::Array> data = Nan::New<v8::Array>(2);
    Nan::Set(data, Nan::New(0), info.Holder());
    Nan::Set(data, Nan::New(1), info[0]);
    v8::Local<v8::FunctionTemplate> tpl =
      Nan::New<v8::FunctionTemplate>(Instance::ExecuteCodeExecutor, data);
    v8::Local<v8::Function> fn = tpl->GetFunction();
    const int argc = 1;
    v8::Local<v8::Value> argv[argc] = {fn};
    v8::Local<v8::Value> promise;
    if (Nan::CallAsConstructor(Nan::New(Promise), argc, argv).ToLocal(&promise)) {
      info.GetReturnValue().Set(promise);
    }
  }

  static NAN_METHOD(ExecuteCodeSync) {
    Instance* obj = Nan::ObjectWrap::Unwrap<Instance>(info.Holder());
    if (!obj->checkInstance()) {
      return;
    }
    v8::Local<v8::Value> buffer = info[0];
    if (!node::Buffer::HasInstance(buffer)) {
      Nan::ThrowTypeError("code must be a buffer");
      return;
    }

    struct asmase_instance* instance = obj->instance_;
    char* code = node::Buffer::Data(buffer);
    size_t len = node::Buffer::Length(buffer);
    if (asmase_execute_code(instance, code, len) == -1) {
      ThrowAsmaseError(errno);
      return;
    }
    int wstatus;
    pid_t ret = asmase_wait(instance, &wstatus);
    if (ret == -1) {
      ThrowAsmaseError(errno);
      return;
    }
    assert(ret > 0);
    info.GetReturnValue().Set(wstatusObject(wstatus));
  }

  static NAN_METHOD(GetPid) {
    Instance* obj = Nan::ObjectWrap::Unwrap<Instance>(info.Holder());
    if (!obj->checkInstance()) {
      return;
    }
    info.GetReturnValue().Set(Nan::New<v8::Uint32>(asmase_getpid(obj->instance_)));
  }

  static Nan::MaybeLocal<v8::Array> GetRegisterBits(const struct asmase_register_descriptor* reg,
      const union asmase_register_value* value) {
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
    if (!obj->checkInstance()) {
      return;
    }
    if (!info[0]->IsString()) {
      Nan::ThrowTypeError("register must be a string");
      return;
    }

    Nan::Utf8String code(info[0]);
    const auto iter = registers_table.find(*code);
    if (iter == registers_table.end()) {
      ThrowAsmaseError(std::string{"unknown register: "} + *code);
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

  static NAN_METHOD(ReadMemory) {
    Instance* obj = Nan::ObjectWrap::Unwrap<Instance>(info.Holder());
    if (!obj->checkInstance()) {
      return;
    }
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
      char* end;
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

  static inline Nan::Persistent<v8::Function>& constructor() {
    static Nan::Persistent<v8::Function> my_constructor;
    return my_constructor;
  }

  static void sigchldHandler(uv_signal_t* handle, int signum) {
    {
      Nan::HandleScope scope;

      auto it = callbacks.begin();
      while (it != callbacks.end()) {
        it = pollInstance(it);
      }
    }

    auto it = destroyed.begin();
    while (it != destroyed.end()) {
      it = pollDestroyed(it);
    }
  }

  static auto pollInstance(decltype(callbacks)::iterator it) -> decltype(callbacks)::iterator {
    v8::Local<v8::Object> obj = Nan::New(std::get<0>(*it));
    bool isCreate = std::get<1>(*it);
    Nan::Callback& resolve = std::get<2>(*it);
    Nan::Callback& reject = std::get<3>(*it);
    struct asmase_instance* instance = Nan::ObjectWrap::Unwrap<Instance>(obj)->instance_;
    if (!instance) {
      const int argc = 1;
      v8::Local<v8::Value> argv[argc] = {Nan::Error("Instance was destroyed")};
      reject.Call(argc, argv);
      return callbacks.erase(it);
    }
    int wstatus;
    pid_t ret = asmase_poll(instance, &wstatus);
    if (ret > 0) {
      if (WIFEXITED(wstatus) || WIFSIGNALED(wstatus)) {
        Nan::ObjectWrap::Unwrap<Instance>(obj)->reaped_ = true;
      }
      const int argc = 1;
      v8::Local<v8::Value> argv[argc] = {isCreate ? obj : wstatusObject(wstatus)};
      resolve.Call(argc, argv);
      return callbacks.erase(it);
    } else if (ret == 0) {
      return ++it;
    } else {
      const int argc = 1;
      v8::Local<v8::Value> argv[argc] = {NewAsmaseError(errno)};
      reject.Call(argc, argv);
      return callbacks.erase(it);
    }
    return ++it;
  }

  static auto pollDestroyed(decltype(destroyed)::iterator it) ->decltype(destroyed)::iterator {
    pid_t pid = *it;
    int wstatus;
    // This should never fail, but if it does, there's nothing we can do about
    // it.
    pid_t ret = ::waitpid(pid, &wstatus, WNOHANG);
    if ((ret > 0 && (WIFEXITED(wstatus) || WIFSIGNALED(wstatus))) || ret < 0) {
      return destroyed.erase(it);
    } else {
      return ++it;
    }
  }

  static Nan::MaybeLocal<v8::Object> NewInstance(v8::Local<v8::Value> arg) {
    int flags = arg->IsUndefined() ? 0 : Nan::To<int>(arg).FromJust();
    struct asmase_instance* instance = asmase_create_instance(flags);
    if (!instance) {
      return Nan::MaybeLocal<v8::Object>();
    }
    v8::Local<v8::Function> cons = Nan::New(constructor());
    v8::Local<v8::Object> obj = Nan::NewInstance(cons).ToLocalChecked();
    Nan::ObjectWrap::Unwrap<Instance>(obj)->instance_ = instance;
    return obj;
  }

  static NAN_METHOD(CreateInstanceExecutor) {
    v8::Local<v8::Function> resolve = Nan::To<v8::Function>(info[0]).ToLocalChecked();
    v8::Local<v8::Function> reject = Nan::To<v8::Function>(info[1]).ToLocalChecked();
    v8::Local<v8::Object> obj;
    if (Instance::NewInstance(info.Data()).ToLocal(&obj)) {
      auto it = callbacks.emplace(callbacks.begin(), obj, true, resolve, reject);
      pollInstance(it);
    } else {
      const int argc = 1;
      v8::Local<v8::Value> argv[argc] = {NewAsmaseError(errno)};
      Nan::MakeCallback(Nan::GetCurrentContext()->Global(), reject, argc, argv);
    }
  }

public:
  static NAN_MODULE_INIT(Init) {
    v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
    tpl->SetClassName(Nan::New("Instance").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tpl, "destroy", Destroy);
    Nan::SetPrototypeMethod(tpl, "executeCode", ExecuteCode);
    Nan::SetPrototypeMethod(tpl, "executeCodeSync", ExecuteCodeSync);
    Nan::SetPrototypeMethod(tpl, "getPid", GetPid);
    Nan::SetPrototypeMethod(tpl, "getRegister", GetRegister);
    Nan::SetPrototypeMethod(tpl, "readMemory", ReadMemory);

    constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());
    Nan::Set(target, Nan::New("Instance").ToLocalChecked(),
             Nan::GetFunction(tpl).ToLocalChecked());

    if (uv_signal_init(uv_default_loop(), &sigchld) < 0 ||
        uv_signal_start(&sigchld, sigchldHandler, SIGCHLD) < 0) {
      Nan::ThrowError("Could not set up SIGCHLD handler");
    }
  }

  static NAN_METHOD(CreateInstance) {
    v8::Local<v8::FunctionTemplate> tpl =
      Nan::New<v8::FunctionTemplate>(Instance::CreateInstanceExecutor, info[0]);
    v8::Local<v8::Function> fn = tpl->GetFunction();
    const int argc = 1;
    v8::Local<v8::Value> argv[argc] = {fn};
    v8::Local<v8::Value> promise;
    if (Nan::CallAsConstructor(Nan::New(Promise), argc, argv).ToLocal(&promise)) {
      info.GetReturnValue().Set(promise);
    }
  }

  static NAN_METHOD(CreateInstanceSync) {
    v8::Local<v8::Object> obj;
    if (!Instance::NewInstance(info[0]).ToLocal(&obj)) {
      ThrowAsmaseError(errno);
      return;
    }
    int wstatus;
    pid_t ret = asmase_wait(Nan::ObjectWrap::Unwrap<Instance>(obj)->instance_, &wstatus);
    if (ret == -1) {
      ThrowAsmaseError(errno);
      return;
    }
    assert(ret > 0);
    info.GetReturnValue().Set(obj);
  }
};

decltype(Instance::callbacks) Instance::callbacks;
decltype(Instance::destroyed) Instance::destroyed;
uv_signal_t Instance::sigchld;

NAN_MODULE_INIT(InitAll) {
  Promise.Reset(Nan::To<v8::Object>(Nan::Get(Nan::GetCurrentContext()->Global(),
        Nan::New("Promise").ToLocalChecked()).ToLocalChecked()).ToLocalChecked());

  v8::Local<v8::String> scriptString = Nan::New("(function() { class AsmaseError extends Error {}; return AsmaseError; })()").ToLocalChecked();
  v8::Local<Nan::BoundScript> script = Nan::CompileScript(scriptString).ToLocalChecked();
  AsmaseError.Reset(Nan::To<v8::Object>(Nan::RunScript(script).ToLocalChecked()).ToLocalChecked());
  Nan::Set(target, Nan::New("AsmaseError").ToLocalChecked(), Nan::New(AsmaseError));

  v8::Local<v8::Map> registers = v8::Map::New(v8::Isolate::GetCurrent());
  for (size_t i = 0; i < asmase_num_registers; i++) {
    const struct asmase_register_descriptor* reg = &asmase_registers[i];

    v8::Local<v8::Object> regObj = Nan::New<v8::Object>();
    Nan::Set(regObj, Nan::New("type").ToLocalChecked(), Nan::New(reg->type));
    Nan::Set(regObj, Nan::New("set").ToLocalChecked(), Nan::New(reg->set));
    registers = registers->Set(Nan::GetCurrentContext(), Nan::New(reg->name).ToLocalChecked(), regObj).ToLocalChecked();
    registers_table[reg->name] = reg;
  }
  Registers.Reset(registers);
  Nan::Set(target, Nan::New("registers").ToLocalChecked(), Nan::New(Registers));

  Instance::Init(target);
  Nan::SetMethod(target, "createInstance", Instance::CreateInstance);
  Nan::SetMethod(target, "createInstanceSync", Instance::CreateInstanceSync);
}

NODE_MODULE(addon, InitAll);
