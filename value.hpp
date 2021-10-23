
#pragma once
#include <stdio.h>
#include <stdlib.h>

#include <list>
#include <map>
#include <string>
#include <vector>

#include "base.hpp"
#include "exception.hpp"
#include "logger.hpp"

namespace Interpreter {

class Resource : public CRefCountedThreadSafe<Resource> {
public:
    virtual ~Resource() { Close(); }
    virtual void Close() {};
    virtual bool IsAvaliable() = 0;
};

class FileResource : public Resource {
public:
    FILE* mFile;

public:
    explicit FileResource(FILE* f) : Resource(), mFile(f) {}
    ~FileResource() { Close(); }
    void Close() {
        if (mFile != NULL) {
            fclose(mFile);
            mFile = NULL;
        }
    }
    bool IsAvaliable() { return mFile != NULL; }
};

typedef scoped_refptr<Resource> Resource_ptr;

namespace ValueType {
const int kNULL = 0;
const int kInteger = 1;
const int kFloat = 2;
const int kString = 3;
const int kBytes = 4;
const int kArray = 5;
const int kMap = 6;
const int kResource = 10;
}; // namespace ValueType

class Instruction;
class Value {
public:
    int Type;
    union {
        long Integer;
        double Float;
    };
    std::string bytes;
    std::vector<Value> _array;
    std::map<Value, Value> _map;
    Resource_ptr resource;
    Value(bool val) : Type(ValueType::kInteger), resource(NULL) {
        Integer = 0;
        if (val) {
            Integer = 1;
        }
    }
    static Value make_map() {
        Value ret = Value();
        ret.Type = ValueType::kMap;
        ret._map.clear();
        return ret;
    }
    static Value make_array() {
        Value ret = Value();
        ret.Type = ValueType::kArray;
        ret._array.clear();
        return ret;
    }
    static Value make_bytes(std::string bytes) {
        Value ret = Value();
        ret.Type = ValueType::kBytes;
        ret.bytes = bytes;
        return ret;
    }
    Value(const Value& right) : resource(NULL) {
        Type = right.Type;
        Integer = right.Integer;
        bytes = right.bytes;
        _map = right._map;
        _array = right._array;
        resource = right.resource;
    }
    Value& operator=(const Value& right) {
        Type = right.Type;
        Integer = right.Integer;
        bytes = right.bytes;
        _map = right._map;
        _array = right._array;
        resource = right.resource;
        return *this;
    }
    Value() : Type(ValueType::kNULL), bytes(), Integer(0), _map(), _array(), resource(NULL) {}
    Value(const char* val) : Type(ValueType::kString), bytes(val), Integer(0), resource(NULL) {}
    Value(const std::string& val)
            : Type(ValueType::kString), bytes(val), Integer(0), resource(NULL) {}
    Value(const long& val) : Type(ValueType::kInteger), bytes(), Integer(val), resource(NULL) {}
    Value(const double& val) : Type(ValueType::kFloat), bytes(), Float(val), resource(NULL) {}
    Value(const std::vector<Value>& val)
            : Type(ValueType::kArray), bytes(), Integer(0), _array(val), _map(), resource(NULL) {}
    Value(const std::map<Value, Value>& val)
            : Type(ValueType::kArray), bytes(), Integer(0), _array(), _map(val), resource(NULL) {}
    Value(Resource* res)
            : Type(ValueType::kResource),
              resource(make_scoped_refptr(res)),
              bytes(),
              Integer(0),
              _array(),
              _map() {}
    Value operator+(Value& right) {
        if (this->Type == ValueType::kString && right.Type == ValueType::kString) {
            return Value(bytes + right.bytes);
        }
        if (this->Type == ValueType::kBytes && right.Type == ValueType::kBytes) {
            return make_bytes(bytes + right.bytes);
        }
        if (!IsArithmeticOperationEnabled(right)) {
            throw RuntimeException("arithmetic operation not avaliable for this value ");
        }
        if (Type == ValueType::kFloat || right.Type == ValueType::kFloat) {
            return Value(ToFloat() + right.ToFloat());
        }
        return Value(ToInteger() + right.ToInteger());
    }

    const Value& operator+=(const Value& right) {
        if (this->Type == ValueType::kString) {
            switch (right.Type) {
            case ValueType::kString:
                this->bytes += right.bytes;
                return *this;
            case ValueType::kFloat:
            case ValueType::kInteger:
                this->bytes += right.ToString();
                return *this;
            default:
                throw RuntimeException("+= operation not avaliable for right value ");
            }
        }
        if (this->Type == ValueType::kBytes) {
            switch (right.Type) {
            case ValueType::kBytes:
                this->bytes += right.bytes;
                return *this;
            case ValueType::kFloat:
            case ValueType::kInteger:
                this->bytes += (unsigned char)right.ToInteger();
                return *this;
            default:
                throw RuntimeException("+= operation not avaliable for right value ");
            }
        }
        if (!IsArithmeticOperationEnabled(right)) {
            throw RuntimeException("arithmetic operation not avaliable for this value ");
        }
        if (Type == ValueType::kFloat) {
            Float += right.ToFloat();
            return *this;
        }
        Integer += right.ToInteger();
        return *this;
    }

    Value operator-(Value& right) {
        if (!IsArithmeticOperationEnabled(right)) {
            throw RuntimeException("arithmetic operation not avaliable for this value ");
        }
        if (Type == ValueType::kFloat || right.Type == ValueType::kFloat) {
            return Value(ToFloat() - right.ToFloat());
        }
        return Value(ToInteger() - right.ToInteger());
    }

    Value operator-=(Value& right) {
        if (!IsArithmeticOperationEnabled(right)) {
            throw RuntimeException("arithmetic operation not avaliable for this value ");
        }
        if (Type == ValueType::kFloat) {
            Float -= right.ToFloat();
            return *this;
        }
        Integer -= right.ToInteger();
        return *this;
    }

    Value operator*(Value& right) {
        if (!IsArithmeticOperationEnabled(right)) {
            throw RuntimeException("arithmetic operation not avaliable for this value ");
        }
        if (Type == ValueType::kFloat || right.Type == ValueType::kFloat) {
            return Value(ToFloat() * right.ToFloat());
        }
        return Value(ToInteger() * right.ToInteger());
    }

    Value operator*=(Value& right) {
        if (!IsArithmeticOperationEnabled(right)) {
            throw RuntimeException("arithmetic operation not avaliable for this value ");
        }
        if (Type == ValueType::kFloat) {
            Float *= right.ToFloat();
            return *this;
        }
        Integer *= right.ToInteger();
        return *this;
    }

    Value operator/(Value& right) {
        if (!IsArithmeticOperationEnabled(right)) {
            throw RuntimeException("arithmetic operation not avaliable for this value ");
        }
        if (Type == ValueType::kFloat || right.Type == ValueType::kFloat) {
            return Value(ToFloat() / right.ToFloat());
        }
        return Value(ToInteger() / right.ToInteger());
    }
    Value operator/=(Value& right) {
        if (!IsArithmeticOperationEnabled(right)) {
            throw RuntimeException("arithmetic operation not avaliable for this value ");
        }
        if (Type == ValueType::kFloat) {
            Float /= right.ToFloat();
            return *this;
        }
        Integer /= right.ToInteger();
        return *this;
    }

    Value operator%(Value& right) {
        if (!IsArithmeticOperationEnabled(right)) {
            throw RuntimeException("arithmetic operation not avaliable for this value ");
        }
        return Value(ToInteger() % right.ToInteger());
    }

    bool operator<(const Value& right) const {
        if (Type == ValueType::kString && right.Type == ValueType::kString) {
            return bytes < right.bytes;
        }
        if (!IsArithmeticOperationEnabled(right)) {
            throw RuntimeException("arithmetic operation not avaliable for this value ");
        }
        return ToFloat() < right.ToFloat();
    }

    bool operator<=(const Value& right) const {
        if (Type == ValueType::kString && right.Type == ValueType::kString) {
            return bytes <= right.bytes;
        }
        if (!IsArithmeticOperationEnabled(right)) {
            throw RuntimeException("arithmetic operation not avaliable for this value ");
        }
        return ToFloat() <= right.ToFloat();
    }
    bool operator>(const Value& right) const {
        if (Type == ValueType::kString && right.Type == ValueType::kString) {
            return bytes > right.bytes;
        }
        if (!IsArithmeticOperationEnabled(right)) {
            throw RuntimeException("arithmetic operation not avaliable for this value ");
        }
        return ToFloat() > right.ToFloat();
    }
    bool operator>=(const Value& right) const {
        if (Type == ValueType::kString && right.Type == ValueType::kString) {
            return bytes >= right.bytes;
        }
        if (!IsArithmeticOperationEnabled(right)) {
            throw RuntimeException("arithmetic operation not avaliable for this value ");
        }
        return ToFloat() >= right.ToFloat();
    }

    bool operator==(const Value& right) const {
        if (IsArithmeticOperationEnabled(right)) {
            return ToFloat() == right.ToFloat();
        }
        if (Type != right.Type) {
            return false;
        }
        switch (Type) {
        case ValueType::kString:
        case ValueType::kBytes:
            return bytes == right.bytes;
        case ValueType::kNULL:
            return true;
        case ValueType::kResource:
            return resource.get() == right.resource.get();
        case ValueType::kArray:
            return _array == right._array;
        case ValueType::kMap:
            return _map == right._map;
        default:
            return false;
        }
    }

    bool operator!=(const Value& right) const {
        return !(*this == right);
    }

    Value operator|(const Value& right) const {
        if (Type != ValueType::kInteger || right.Type != ValueType::kInteger) {
            throw RuntimeException("| operation only can used on Integer ");
        }
        return Value(Integer | right.Integer);
    }

    Value operator&(const Value& right) const {
        if (Type != ValueType::kInteger || right.Type != ValueType::kInteger) {
            throw RuntimeException("& operation only can used on Integer ");
        }
        return Value(Integer & right.Integer);
    }

    Value operator^(const Value& right) const {
        if (Type != ValueType::kInteger || right.Type != ValueType::kInteger) {
            throw RuntimeException("^ operation only can used on Integer ");
        }
        return Value(Integer ^ right.Integer);
    }
    Value operator<<(const Value& right) const {
        if (Type != ValueType::kInteger || right.Type != ValueType::kInteger) {
            throw RuntimeException("^ operation only can used on Integer ");
        }
        return Value(Integer << right.Integer);
    }
    Value operator>>(const Value& right) const {
        if (Type != ValueType::kInteger || right.Type != ValueType::kInteger) {
            throw RuntimeException("^ operation only can used on Integer ");
        }
        return Value(Integer >> right.Integer);
    }

    const Value& operator<<=(const Value& right) {
        if (Type != ValueType::kInteger || right.Type != ValueType::kInteger) {
            throw RuntimeException("^ operation only can used on Integer ");
        }
        Integer <<= right.Integer;
        return *this;
    }
    const Value& operator>>=(const Value& right) {
        if (Type != ValueType::kInteger || right.Type != ValueType::kInteger) {
            throw RuntimeException("^ operation only can used on Integer ");
        }
        Integer >>= right.Integer;
        return *this;
    }

    Value& operator|=(const Value& right) {
        if (Type != ValueType::kInteger || right.Type != ValueType::kInteger) {
            throw RuntimeException("|= operation only can used on Integer ");
        }
        Integer |= right.Integer;
        return *this;
    }

    Value& operator&=(const Value& right) {
        if (Type != ValueType::kInteger || right.Type != ValueType::kInteger) {
            throw RuntimeException("&= operation only can used on Integer ");
        }
        Integer &= right.Integer;
        return *this;
    }

    Value& operator^=(const Value& right) {
        if (Type != ValueType::kInteger || right.Type != ValueType::kInteger) {
            throw RuntimeException("^= operation only can used on Integer ");
        }
        Integer ^= right.Integer;
        return *this;
    }

    Value operator~() const {
        if (Type != ValueType::kInteger) {
            throw RuntimeException("~ operation only can used on Integer ");
        }
        return Value(~Integer);
    }

    Value operator[](int index) {
        if (Type == ValueType::kString || Type == ValueType::kBytes) {
            if (index >= bytes.size()) {
                throw RuntimeException("index of string out of range");
            }
            return Value((long)bytes[index]);
        }
        if (Type == ValueType::kArray) {
            if (index >= _array.size()) {
                throw RuntimeException("index of string out of range");
            }
            return _array[index];
        }
        throw RuntimeException("value not support index operation");
    }

    //a= b[1:24]
    //a= b[:24]
    //a= b[4:]

    long length() {
        if (Type == ValueType::kString) {
            return (long)bytes.size();
        }
        if (Type == ValueType::kArray) {
            return (long)_array.size();
        }
        if (Type == ValueType::kMap) {
            return (long)_map.size();
        }
        if (Type == ValueType::kBytes) {
            return (long)bytes.size();
        }
        throw RuntimeException("this value type not have length ");
    }

    Value sub_slice(const Value& f, const Value& t) {
        size_t from = 0, to = 0;
        if (Type != ValueType::kString && Type != ValueType::kArray) {
            throw RuntimeException("the value type must slice able");
        }
        if (f.Type == ValueType::kNULL) {
            from = 0;
        } else if (f.Type == ValueType::kInteger) {
            from = f.Integer;
        } else {
            throw RuntimeException("the index key type must a Integer");
        }
        if (t.Type == ValueType::kNULL) {
            to = length();
        } else if (f.Type == ValueType::kInteger) {
            to = t.Integer;
        } else {
            throw RuntimeException("the index key type must a Integer");
        }
        if (to > length() || from > length()) {
            throw RuntimeException("index out of range");
        }

        if (Type == ValueType::kString) {
            std::string sub = bytes.substr(from, to - from);
            return Value(sub);
        }
        if (Type == ValueType::kBytes) {
            std::string sub = bytes.substr(from, to - from);
            return make_bytes(sub);
        }
        std::vector<Value> result;
        for (size_t i = from; i < to; i++) {
            result.push_back(_array[i]);
        }
        return Value(result);
    }

    Value operator[](Value key) {
        if (Type == ValueType::kString || Type == ValueType::kArray || Type == ValueType::kBytes) {
            if (key.Type != ValueType::kInteger) {
                throw RuntimeException("the index key type must a Integer");
            }
            if (Type == ValueType::kString || Type == ValueType::kBytes) {
                if (key.Integer >= bytes.size()) {
                    throw RuntimeException("index of string(bytes) out of range");
                }
                return Value((long)bytes[key.Integer]);
            }
            if (key.Integer >= _array.size()) {
                throw RuntimeException("index of array out of range");
            }
            return Value(_array[key.Integer]);
        }
        if (Type != ValueType::kMap) {
            throw RuntimeException("value not support index operation");
        }
        std::map<Value, Value>::iterator iter = _map.find(key);
        if (iter != _map.end()) {
            return iter->second;
        }
        return Value();
    }

    void set_value(Value key, Value val) {
        if (Type == ValueType::kString || Type == ValueType::kArray || Type == ValueType::kBytes) {
            if (key.Type != ValueType::kInteger) {
                throw RuntimeException("the index key type must a Integer");
            }
            if (Type == ValueType::kString || Type == ValueType::kBytes) {
                if (key.Integer >= bytes.size()) {
                    throw RuntimeException("index of string(bytes) out of range");
                }
                bytes[key.Integer] = (char)val.ToInteger();
                return;
            }
            if (key.Integer >= _array.size()) {
                throw RuntimeException("index of array out of range");
            }
            _array[key.Integer] = val;
            return;
        }
        if (Type != ValueType::kMap) {
            throw RuntimeException("value not support index operation");
        }
        _map[key] = val;
    }

    bool ToBoolen() {
        if (Type == ValueType::kNULL) {
            return false;
        }
        if (Type == ValueType::kFloat) {
            return ToFloat() != 0;
        }
        if (Type == ValueType::kInteger) {
            return ToInteger() != 0;
        }
        return true;
    }

    std::string TypeName() const {
        switch (Type) {
        case ValueType::kString:
            return "string";
        case ValueType::kInteger:
            return "integer";
        case ValueType::kNULL:
            return "nil";
        case ValueType::kFloat:
            return "float";
        case ValueType::kArray:
            return "array";
        case ValueType::kMap:
            return "map";
        case ValueType::kBytes:
            return "bytes";
        case ValueType::kResource:
            return "resource";
        default:
            return "Unknown";
        }
    }

    std::string ToString() const {
        char buffer[16] = {0};
        switch (Type) {
        case ValueType::kString:
            return bytes;
        case ValueType::kBytes: {
            std::string result = "";
            for (size_t i = 0; i < bytes.size(); i++) {
                snprintf(buffer, 16, "%02X", bytes[i]);
                result += buffer;
            }
            return result;
        }
        case ValueType::kInteger:
            snprintf(buffer, 16, "%ld", Integer);
            return buffer;
        case ValueType::kNULL:
            return "nil";
        case ValueType::kFloat:
            snprintf(buffer, 16, "%f", Float);
            return buffer;
        case ValueType::kArray: {
            std::vector<Value>::const_iterator iter = _array.begin();
            std::string ret = "[";
            while (iter != _array.end()) {
                ret += iter->ToString();
                ret += ",";
                iter++;
            }
            if (_array.size() > 0) {
                ret[ret.size() - 1] = ']';
            } else {
                ret += ']';
            }
            return ret;
        }

        case ValueType::kMap: {
            std::string ret = "{";
            std::map<Value, Value>::const_iterator iter = _map.begin();
            while (iter != _map.end()) {
                ret += (iter->first).ToString();
                ret += ":";
                ret += (iter->second).ToString();
                ret += ",";
                iter++;
            }
            if (_map.size() > 0) {
                ret[ret.size() - 1] = '}';
            } else {
                ret += '}';
            }
            return ret;
        }

        default:
            return "";
        }
    }

protected:
    long ToInteger() const {
        if (Type == ValueType::kFloat) {
            return (long)Float;
        }
        if (Type == ValueType::kInteger) {
            return Integer;
        }
        return 0;
    }

    double ToFloat() const {
        if (Type == ValueType::kFloat) {
            return Float;
        }
        if (Type == ValueType::kInteger) {
            return (double)Integer;
        }
        return 0;
    }

private:
    bool IsArithmeticOperationEnabled(const Value& right) const {
        return (Type == ValueType::kFloat || Type == ValueType::kInteger) &&
               (right.Type == ValueType::kFloat || right.Type == ValueType::kInteger);
    }
};

inline std::list<std::string> split(const std::string& text, char split_char) {
    std::list<std::string> result;
    std::string part = "";
    std::string::const_iterator iter = text.begin();
    while (iter != text.end()) {
        if (*iter == split_char) {
            result.push_back(part);
            part = "";
            iter++;
            continue;
        }
        part += (*iter);
        iter++;
    }
    result.push_back(part);
    return result;
}
} // namespace Interpreter