#include <iostream>

#include "vm.hpp"

using namespace Interpreter;

Value Println(std::vector<Value>& values, scoped_refptr<VMContext> ctx, Executor* vm) {
    std::string result;
    for (std::vector<Value>::iterator iter = values.begin(); iter != values.end(); iter++) {
        result += iter->ToString();
        result += " ";
    }
    std::cout << result << std::endl;
    return Value();
}

Value Len(std::vector<Value>& values, scoped_refptr<VMContext> ctx, Executor* vm) {
    Value& arg = values.front();
    assert(values.size() == 1);
    return Value((long)arg.length());
}

Value TypeOf(std::vector<Value>& values, scoped_refptr<VMContext> ctx, Executor* vm) {
    Value& arg = values.front();
    assert(values.size() == 1);
    return Value(arg.TypeName());
}

Value ToString(std::vector<Value>& values, scoped_refptr<VMContext> ctx, Executor* vm) {
    Value& arg = values.front();
    assert(values.size() == 1);
    return Value(arg.ToString());
}

Value Append(std::vector<Value>& values, scoped_refptr<VMContext> ctx, Executor* vm) {
    Value to = values.front();
    if (to.Type != ValueType::kArray) {
        throw RuntimeException("first append value must an array");
    }
    std::vector<Value>::iterator iter = values.begin();
    iter++;
    while (iter != values.end()) {
        to._array.push_back(*iter);
        iter++;
    }
    return to;
}

Value OpenFile(std::vector<Value>& values, scoped_refptr<VMContext> ctx, Executor* vm) {
    Value path = values.front();
    if (path.Type != ValueType::kString) {
        throw RuntimeException("OpenFile invalid parameter type");
    }
    FILE* hFile = fopen(path.bytes.c_str(), "r");
    if (hFile == NULL) {
        return Value();
    }
    return Value(new FileResource(hFile));
}

Value Read(std::vector<Value>& values, scoped_refptr<VMContext> ctx, Executor* vm) {
    Value res = values.front();
    if (res.Type != ValueType::kResource) {
        throw RuntimeException("Read invalid parameter type");
    }
    if (!res.resource->IsAvaliable()) {
        throw RuntimeException("Read resource not avaliable (may be closed)");
    }
    return Value();
}

Value Close(std::vector<Value>& values, scoped_refptr<VMContext> ctx, Executor* vm) {
    if (values.size() != 1) {
        throw RuntimeException("Close invalid parameter count");
    }
    Value res = values.front();
    if (res.Type != ValueType::kResource) {
        throw RuntimeException("Close invalid parameter type");
    }
    res.resource->Close();
    return Value();
}

Value Exit(std::vector<Value>& values, scoped_refptr<VMContext> ctx, Executor* vm) {
    if (values.size() != 1) {
        throw RuntimeException("exit invalid parameter count");
    }
    Value exitCode = values.front();
    if (exitCode.Type != ValueType::kInteger) {
        throw RuntimeException("exit code parameter type must a integer");
    }
    ctx->ExitExecuted(exitCode);
    return Value();
}

Value Require(std::vector<Value>& values, scoped_refptr<VMContext> ctx, Executor* vm){
    if (values.size() != 1) {
        throw RuntimeException("require invalid parameter count");
    }
    if(!ctx->IsTop()){
        throw RuntimeException("require must called in top context");
    }
    Value name = values.front();
    if (name.Type != ValueType::kString) {
        throw RuntimeException("require code parameter type must a integer");
    }
    vm->RequireScript(name.bytes,ctx);
    return Value();
}

int g_builtinMethod_size = 8;

BuiltinMethod g_builtinMethod[8] = {{"exit", Exit},  {"Println", Println},   {"TypeOf", TypeOf},
                                    {"len", Len},    {"ToString", ToString}, {"append", Append},
                                    {"Close", Close},
                                    {"require",Require}};
