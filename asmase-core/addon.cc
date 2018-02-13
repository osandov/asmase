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
    v8::Local<v8::Value> buffer = Nan::Get(data, 1).ToLocalChecked();
    if (!node::Buffer::HasInstance(buffer)) {
      const int argc = 1;
      v8::Local<v8::Value> argv[argc] = {Nan::TypeError("code must be a buffer")};
      Nan::MakeCallback(Nan::GetCurrentContext()->Global(), reject, argc, argv);
      return;
    }

    v8::Local<v8::Object> obj = Nan::To<v8::Object>(Nan::Get(data, 0).ToLocalChecked()).ToLocalChecked();
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
    Nan::Set(data, 0, info.Holder());
    Nan::Set(data, 1, info[0]);
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

  static NAN_METHOD(GetRegisters) {
    Instance* obj = Nan::ObjectWrap::Unwrap<Instance>(info.Holder());
    if (!obj->checkInstance()) {
      return;
    }
    v8::Local<v8::Value> buffer = info[0];
    if (!node::Buffer::HasInstance(buffer)) {
      Nan::ThrowTypeError("registers must be a buffer");
      return;
    }
    int ret = asmase_get_registers(obj->instance_, node::Buffer::Data(buffer),
        node::Buffer::Length(buffer));
    if (ret) {
      ThrowAsmaseError(errno);
    }
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
    v8::Local<v8::Object> memory = Nan::NewBuffer(
        static_cast<char*>(asmase_get_shmem(instance)),
        SHMEM_SIZE, [](char*, void*){}, nullptr).ToLocalChecked();
    Nan::Set(obj, Nan::New("memory").ToLocalChecked(), memory);
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
    Nan::SetPrototypeMethod(tpl, "getRegisters", GetRegisters);

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

  Instance::Init(target);
  Nan::SetMethod(target, "createInstance", Instance::CreateInstance);
  Nan::SetMethod(target, "createInstanceSync", Instance::CreateInstanceSync);
}

NODE_MODULE(addon, InitAll);
