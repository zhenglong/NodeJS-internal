#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include "libplatform/libplatform.h"
#include "v8.h"

using namespace v8;

int age = 41;

void doit(const FunctionCallbackInfo<Value>& args) {
  String::Utf8Value str(args.GetIsolate(), args[0]);
  printf("doit argument = %s...\n", *str);
  args.GetReturnValue().Set(String::NewFromUtf8(args.GetIsolate(), "done", NewStringType::kNormal).ToLocalChecked());
}

void ageGetter(Local<String> property, const PropertyCallbackInfo<Value>& info) {
  info.GetReturnValue().Set(age);
}

void ageSetter(Local<String> property, Local<Value> value, const PropertyCallbackInfo<void>& info) {
  age = value->Int32Value(info.GetIsolate()->GetCurrentContext()).FromMaybe(-1);
}

void propertyListener(Local<String> name, const PropertyCallbackInfo<Value>& info) {
  String::Utf8Value utf8_value(info.GetIsolate(), name);
  std::string key = std::string(*utf8_value);
  printf("ageListener called for nam %s.\n", key.c_str());
}

int main(int argc, char* argv[]) {
    V8::InitializeExternalStartupData(argv[0]);
    auto platform = platform::NewDefaultPlatform();
    V8::InitializePlatform(platform.get());
    V8::Initialize();

    Isolate::CreateParams create_params;
    create_params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
    Isolate* isolate = Isolate::New(create_params);
    {
        Isolate::Scope isolate_scope(isolate);
        HandleScope handle_scope(isolate);

        Local<ObjectTemplate> global = ObjectTemplate::New(isolate);
        global->Set(String::NewFromUtf8(isolate, "doit", NewStringType::kNormal).ToLocalChecked(),
                FunctionTemplate::New(isolate, doit));
        global->SetAccessor(String::NewFromUtf8(isolate, "age", NewStringType::kNormal).ToLocalChecked(),
                ageGetter,
                ageSetter);
        

        Local<Context> context = Context::New(isolate, NULL, global);
        Context::Scope context_scope(context);

        const char *js = "age = 40; doit(age);";
        Local<String> source = String::NewFromUtf8(isolate, js, NewStringType::kNormal).ToLocalChecked();
        Local<Script> script = Script::Compile(context, source).ToLocalChecked();
        Local<Value> result = script->Run(context).ToLocalChecked();

        String::Utf8Value utf8(isolate, result);
        printf("%s\n", *utf8);
    }

    isolate->Dispose();
    V8::Dispose();
    V8::ShutdownPlatform();
    return 0;
}
