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

bool IsIntegerArray(const std::vector<Value>& values) {
    std::vector<Value>::const_iterator iter = values.begin();
    while (iter != values.end()) {
        if (iter->Type != ValueType::kInteger) {
            return false;
        }

        iter++;
    }
    return true;
}

void AppendIntegerArrayToBytes(Value& val, const std::vector<Value>& values) {
    std::vector<Value>::const_iterator iter = values.begin();
    while (iter != values.end()) {
        val.bytes.append(1, (unsigned char)iter->Integer);
        iter++;
    }
}

Value Append(std::vector<Value>& values, scoped_refptr<VMContext> ctx, Executor* vm) {
    Value to = values.front();
    if (to.Type == ValueType::kArray) {
        std::vector<Value>::iterator iter = values.begin();
        iter++;
        while (iter != values.end()) {
            to._array.push_back(*iter);
            iter++;
        }
        return to;
    }
    if (to.Type == ValueType::kBytes) {
        std::vector<Value>::iterator iter = values.begin();
        iter++;
        while (iter != values.end()) {
            switch (iter->Type) {
            case ValueType::kBytes:
                to.bytes += iter->bytes;
                break;
            case ValueType::kInteger:
                to.bytes.append(1, (unsigned char)iter->Integer);
                break;
            case ValueType::kArray:
                if (!IsIntegerArray(iter->_array)) {
                    throw RuntimeException("only Integer Array can append to bytes");
                }
                AppendIntegerArrayToBytes(to, iter->_array);
                break;
            default:
                throw RuntimeException("some value can't append to bytes");
            }
            iter++;
        }
        return to;
    }
    throw RuntimeException("first append value must an array or bytes");
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

Value Require(std::vector<Value>& values, scoped_refptr<VMContext> ctx, Executor* vm) {
    if (values.size() != 1) {
        throw RuntimeException("require invalid parameter count");
    }
    if (!ctx->IsTop()) {
        throw RuntimeException("require must called in top context");
    }
    Value name = values.front();
    if (name.Type != ValueType::kString) {
        throw RuntimeException("require code parameter type must a integer");
    }
    vm->RequireScript(name.bytes, ctx);
    return Value();
}

Value MakeBytes(std::vector<Value>& values, scoped_refptr<VMContext> ctx, Executor* vm) {
    if (values.size() == 0) {
        return Value::make_bytes("");
    }
    if (values.size() == 1) {
        if (values[0].Type == ValueType::kString || values[0].Type == ValueType::kBytes) {
            return Value::make_bytes(values[0].bytes);
        }
        if (values[0].Type == ValueType::kArray) {
            if (!IsIntegerArray(values[0]._array)) {
                throw RuntimeException("make bytes must use Integer Array");
            }
            Value ret = Value::make_bytes("");
            AppendIntegerArrayToBytes(ret, values[0]._array);
            return ret;
        }
    }
    if (!IsIntegerArray(values)) {
        throw RuntimeException("make bytes must use Integer");
    }
    Value ret = Value::make_bytes("");
    AppendIntegerArrayToBytes(ret, values);
    return ret;
}

Value MakeString(std::vector<Value>& values, scoped_refptr<VMContext> ctx, Executor* vm) {
    if (values.size() == 0) {
        return Value("");
    }
    if (values.size() == 1) {
        if (values[0].Type == ValueType::kString || values[0].Type == ValueType::kBytes) {
            return Value(values[0].bytes);
        }
        if (values[0].Type == ValueType::kFloat || values[0].Type == ValueType::kInteger) {
            return Value(values[0].ToString());
        }
        if (values[0].Type == ValueType::kArray) {
            if (!IsIntegerArray(values[0]._array)) {
                throw RuntimeException("convert to string must use Integer Array");
            }
            Value ret = Value::make_bytes("");
            AppendIntegerArrayToBytes(ret, values[0]._array);
            ret.Type = ValueType::kString;
            return ret;
        }
    }
    if (!IsIntegerArray(values)) {
        throw RuntimeException("convert to string must use Integer Array or Bytes");
    }
    Value ret = Value::make_bytes("");
    AppendIntegerArrayToBytes(ret, values);
    ret.Type = ValueType::kString;
    return ret;
}

bool IsHexChar(char c) {
    if (c >= '0' && c <= '9') {
        return true;
    }
    if (c >= 'a' && c <= 'f') {
        return true;
    }
    if (c >= 'A' && c <= 'F') {
        return true;
    }
    return false;
}

Value BytesFromHexString(std::vector<Value>& values, scoped_refptr<VMContext> ctx, Executor* vm) {
    if (values.size() != 1) {
        throw RuntimeException("BytesFromHexString invalid parameter count");
    }
    Value& arg = values.front();
    if (arg.Type != ValueType::kString) {
        throw RuntimeException("BytesFromHexString invalid parameter type");
    }
    if (arg.length() % 2 || arg.length() == 0) {
        throw RuntimeException("BytesFromHexString string length must be a multiple of 2");
    }
    size_t i = 0;
    char buf[3] = {0};
    std::string result = "";
    for (; i < arg.bytes.size(); i += 2) {
        buf[0] = arg.bytes[i];
        buf[1] = arg.bytes[i + 1];
        if (!IsHexChar(buf[0]) || !IsHexChar(buf[1])) {
            throw RuntimeException("BytesFromHexString is not a valid hex string");
        }
        unsigned char val = strtol(buf, NULL, 16);
        result.append(1, val);
    }
    return Value::make_bytes(result);
}

int g_builtinMethod_size = 11;

BuiltinMethod g_builtinMethod[11] = {{"exit", Exit},
                                     {"len", Len},
                                     {"append", Append},
                                     {"require", Require},
                                     {"bytes", MakeBytes},
                                     {"string", MakeString},
                                     {"close", Close},
                                     {"typeof", TypeOf},
                                     {"Println", Println},
                                     {"ToString", ToString},
                                     {"BytesFromHexString", BytesFromHexString}};
