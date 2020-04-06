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

#include "parsing/scanner.h"
#include "parsing/scanner-character-streams.h"

#include "zone/accounting-allocator.h"
#include "zone/zone.h"

using namespace v8;
namespace i = v8::internal;

i::Isolate* asInternal(Isolate *isolate) {
    return reinterpret_cast<i::Isolate*>(isolate);
}

bool CanAccessLiteral(i::Token::Value token) {
  return token == i::Token::PRIVATE_NAME || token == i::Token::ILLEGAL ||
         token == i::Token::UNINITIALIZED || token == i::Token::REGEXP_LITERAL ||
         i::IsInRange(token, i::Token::NUMBER, i::Token::STRING) ||
         i::Token::IsAnyIdentifier(token) || i::Token::IsKeyword(token) ||
         i::IsInRange(token, i::Token::TEMPLATE_SPAN, i::Token::TEMPLATE_TAIL);
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
        i::Handle<i::String> source = factory->InternalizeUtf8String("var a=1,b=2,c=33,d='4';/* some comments */ var e = a+b*(c+d);");
        
        // 打印Token
        std::unique_ptr<i::Utf16CharacterStream> stream(i::ScannerStream::For(internalIsolate, source));
        i::Scanner scanner(stream.get(), false);
        scanner.Initialize();
        i::AccountingAllocator allocator;
        i::Zone zone(&allocator, "test-zone");
        scanner.Next();
        while (scanner.current_token() != i::Token::EOS) {
            std::cout << i::Token::Name(scanner.current_token());
            if (scanner.current_token() == i::Token::STRING) {
                std::cout << " " << scanner.CurrentLiteralAsCString(&zone) << std::endl;
            } else if (scanner.current_token() == i::Token::SMI) {
                std::cout << " " << scanner.smi_value() << std::endl;
            } else {
                std::cout << std::endl;
            }
            scanner.Next();
        }
        
        // 打印AST
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
