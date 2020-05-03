
#include <iostream>

#include "libplatform/libplatform.h"
#include "v8.h"

using namespace v8;

void handler(const FunctionCallbackInfo<Value>& info) {
    auto isolate = info.GetIsolate();
    auto val = info[0]->IntegerValue(isolate->GetCurrentContext()).ToChecked();
    std::cout << val << std::endl;
    auto return_val = info.GetReturnValue();

    //return_val.Set<String>(Persistent<String>(isolate, String::NewFromUtf8(info.GetIsolate(), "done")));
    return_val.Set<String>(String::NewFromUtf8(info.GetIsolate(), "done"));
}

int main(int argc, char* argv[])
{
    V8::InitializeICUDefaultLocation(argv[0]);
    auto platform = platform::NewDefaultPlatform();
    V8::InitializePlatform(platform.get());
    V8::Initialize();

    Isolate::CreateParams create_params;
    create_params.array_buffer_allocator = ArrayBuffer::Allocator::NewDefaultAllocator();
    Isolate* isolate = Isolate::New(create_params);
    {
        Isolate::Scope isolate_scope(isolate);

        HandleScope handle_scope(isolate);

        Local<ObjectTemplate> global = ObjectTemplate::New(isolate);
        global->Set(String::NewFromUtf8(isolate, "cppHandler", NewStringType::kNormal).ToLocalChecked(), FunctionTemplate::New(isolate, handler));
        Local<Context> context = Context::New(isolate, nullptr, global);
        Context::Scope context_scope(context);

        auto callback = FunctionTemplate::New(isolate, handler);

        auto sourceStr = R"(
            function add(num) { 
                return function(v) { 
                    return (num + v); 
                } 
            } 
            var addFive = add(5); 
            new Promise(function(resolve) { 
                resolve(addFive(10)); 
            }).then(cppHandler)
            .then(function(res) { 
                return res + ' modified'; 
            });)";
        Local<String> source = String::NewFromUtf8(isolate, sourceStr, NewStringType::kNormal)
            .ToLocalChecked();

        Local<Script> script = Script::Compile(context, source).ToLocalChecked();

        Local<Value> result = script->Run(context).ToLocalChecked();

        Promise* pro = Promise::Cast(*result);
        std::cout << pro->State() << std::endl;
        auto res = pro->Result();
        String::Utf8Value str(isolate, res);
        std::cout << *str << std::endl;
    }

    isolate->Dispose();
    V8::Dispose();
    V8::ShutdownPlatform();
    delete create_params.array_buffer_allocator;
    return 0;
}
