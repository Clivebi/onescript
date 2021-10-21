#include <iostream>

#include "vm.hpp"

using namespace Interpreter;

Value Println(std::vector<Value>& values, VMContext* ctx, Executor* vm) {
    std::string result;
    for (std::vector<Value>::iterator iter = values.begin(); iter != values.end(); iter++) {
        result += iter->ToString();
        result += " ";
    }
    std::cout << result << "\n" << std::endl;
    return Value();
}

Value Len(std::vector<Value>& values, VMContext* ctx, Executor* vm) {
    Value& arg = values.front();
    assert(values.size() == 1);
    return Value((long)arg.length());
}

Value TypeOf(std::vector<Value>& values, VMContext* ctx, Executor* vm) {
    Value& arg = values.front();
    assert(values.size() == 1);
    return Value(arg.TypeName());
}

Value ToString(std::vector<Value>& values, VMContext* ctx, Executor* vm) {
    Value& arg = values.front();
    assert(values.size() == 1);
    return Value(arg.ToString());
}

Value Append(std::vector<Value>& values, VMContext* ctx, Executor* vm) {
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

int g_builtinMethod_size = 5;

BuiltinMethod g_builtinMethod[5] = {{"Println", Println},
                                    {"TypeOf", TypeOf},
                                    {"len", Len},
                                    {"ToString", ToString},
                                    {"append", Append}};
