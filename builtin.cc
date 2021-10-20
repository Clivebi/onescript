#include <iostream>

#include "vm.hpp"

Interpreter::Value Println(std::vector<Interpreter::Value>& values) {
    std::string result;
    for (std::vector<Interpreter::Value>::iterator iter = values.begin(); iter != values.end();
         iter++) {
        result += iter->ToString();
        result += " ";
    }
    std::cout << result << "\n" << std::endl;
    return Interpreter::Value();
}

Interpreter::Value Len(std::vector<Interpreter::Value>& values) {
    Interpreter::Value& arg = values.front();
    assert(values.size() == 1);
    return Interpreter::Value((long)arg.length());
}

Interpreter::Value TypeOf(std::vector<Interpreter::Value>& values) {
    Interpreter::Value& arg = values.front();
    assert(values.size() == 1);
    return Interpreter::Value(arg.TypeName());
}

Interpreter::Value ToString(std::vector<Interpreter::Value>& values) {
    Interpreter::Value& arg = values.front();
    assert(values.size() == 1);
    return Interpreter::Value(arg.ToString());
}

int g_builtinMethod_size = 4;

Interpreter::BuiltinMethod g_builtinMethod[4] = {
        {"Println", Println}, {"TypeOf", TypeOf}, {"len", Len}
        ,"ToString",ToString};
