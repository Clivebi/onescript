#include "vm.hpp"
#include <iostream>

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

Interpreter::Value TypeOf(std::vector<Interpreter::Value>& values) {
    Interpreter::Value& arg = values.front();
    assert(values.size() == 1);
    switch (arg.Type) {
    case Interpreter::ValueType::kNULL:
        return Interpreter::Value("NULL");
    case Interpreter::ValueType::kInteger:
        return Interpreter::Value("Integer");
    case Interpreter::ValueType::kFloat:
        return Interpreter::Value("Float");
    case Interpreter::ValueType::kString:
        return Interpreter::Value("String");

    default:
        return Interpreter::Value("Unknown");
    }
}

Interpreter::BuiltinMethod g_builtinMethod[2] = {
        {"Println", Println},
        {"TypeOf",TypeOf}
};
