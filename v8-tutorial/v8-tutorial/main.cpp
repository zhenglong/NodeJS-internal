#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <iostream>

#include "libplatform/libplatform.h"
#include "v8.h"

#include "ast/ast.h"
#include "ast/prettyprinter.h"
#include "parsing/parsing.h"
#include "parsing/parse-info.h"


using namespace v8;
namespace i = v8::internal;

i::Isolate* asInternal(Isolate *isolate) {
    return reinterpret_cast<i::Isolate*>(isolate);
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
        auto internalIsolate = asInternal(isolate);
        i::Factory *factory = internalIsolate->factory();
        i::ParseInfo info(internalIsolate);
        i::Handle<i::String> source = factory->InternalizeUtf8String("var a=1,b=2,c=3,d=4; var e = a+b*(c+d);");
        ScriptOriginOptions options;
        info.CreateScript(internalIsolate, source, options);
        auto res = internal::parsing::ParseProgram(&info, asInternal(isolate));
        printf("%d\n", res);
        std::cout << i::AstPrinter(info.stack_limit())
        .PrintProgram(info.literal())
        << std::endl;
    }

    isolate->Dispose();
    V8::Dispose();
    V8::ShutdownPlatform();
    return 0;
}
