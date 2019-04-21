#include "services/golomb_set.h"
#include <napi.h>

using namespace Napi;

class GolombSetObject : public Napi::ObjectWrap<GolombSetObject>
{
  public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    static Napi::Object NewInstance(Napi::Env env, Napi::Value arg);
    GolombSetObject(const Napi::CallbackInfo &info);
    ~GolombSetObject();

  private:
    static Napi::FunctionReference constructor;
    Napi::Value CheckPassword(const Napi::CallbackInfo &info);
    Napi::Value CheckHash(const Napi::CallbackInfo &info);
    Napi::Value InitFromTextFile(const Napi::CallbackInfo &info);
    Napi::Value InitFromDb(const Napi::CallbackInfo &info);
    GolombSet<uint64_t> *golomb_set_;
};

Napi::FunctionReference GolombSetObject::constructor;

Napi::Object GolombSetObject::Init(Napi::Env env, Napi::Object exports)
{
    Napi::HandleScope scope(env);

    Napi::Function func = DefineClass(env, "GolombSetObject", {
                                                                  InstanceMethod("checkPassword", &GolombSetObject::CheckPassword),
                                                                  InstanceMethod("checkHash", &GolombSetObject::CheckHash),
                                                                  InstanceMethod("initFromTextfile", &GolombSetObject::InitFromTextFile),
                                                                  InstanceMethod("initFromDb", &GolombSetObject::InitFromDb),
                                                              });

    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();
    exports.Set("GolombSetObject", func);
    return exports;
}

GolombSetObject::GolombSetObject(const Napi::CallbackInfo &info) : Napi::ObjectWrap<GolombSetObject>(info)
{
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);
    OpenSSL_add_all_digests();
    if (info.Length() >= 1 && info[0].IsNumber())
    {
        this->golomb_set_ = new GolombSet<uint64_t>(info[0].As<Napi::Number>());
    }
    else
    {
        this->golomb_set_ = new GolombSet<uint64_t>();
    }
};

GolombSetObject::~GolombSetObject()
{
    EVP_cleanup();
    delete this->golomb_set_;
}

Napi::Object GolombSetObject::NewInstance(Napi::Env env, Napi::Value arg)
{
    Napi::EscapableHandleScope scope(env);
    Napi::Object obj = constructor.New({arg});
    return scope.Escape(napi_value(obj)).ToObject();
}

Napi::Value GolombSetObject::CheckPassword(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsString())
    {
        return Napi::Boolean::New(env, false);
    }

    return Napi::Boolean::New(env, this->golomb_set_->check_password(info[0].As<Napi::String>()));
}

Napi::Value GolombSetObject::CheckHash(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsString())
    {
        return Napi::Boolean::New(env, false);
    }

    return Napi::Boolean::New(env, this->golomb_set_->check_hash(info[0].As<Napi::String>()));
}

Napi::Value GolombSetObject::InitFromDb(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsString())
    {
        throw Napi::Error::New(env, "Bad arguments");
    }
    this->golomb_set_->init_from_dbfile(info[0].As<Napi::String>());
    return Napi::Boolean::New(env, true);
}

Napi::Value GolombSetObject::InitFromTextFile(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsString())
    {
        throw Napi::Error::New(env, "Bad arguments");
    }

    this->golomb_set_->init_from_textfile(info[0].As<Napi::String>(), info[1].As<Napi::String>());
    return Napi::Boolean::New(info.Env(), true);
}

Napi::Object CreateObject(const Napi::CallbackInfo &info)
{
    return GolombSetObject::NewInstance(info.Env(), info[0]);
}

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
    Napi::Object new_exports = Napi::Function::New(env, CreateObject, "CreateObject");
    return GolombSetObject::Init(env, new_exports);
}

NODE_API_MODULE(pwned_passwords, Init)