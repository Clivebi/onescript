
#pragma once
#include <stdio.h>
#include <stdlib.h>

#include <list>
#include <map>
#include <string>
#include <vector>

#include "exception.hpp"
#include "logger.hpp"

namespace Interpreter {

class RefBase {
private:
    int mRef;

public:
    RefBase() : mRef(0) {}
    virtual ~RefBase() {}
    virtual int AddRef() { return mRef++; }
    virtual int Release() {
        if (0 == --mRef) {
            delete this;
            return 0;
        }
        return mRef;
    }
};

class Resource : public RefBase {
public:
    virtual ~Resource() { Close(); }
    virtual void Close() {};
    virtual bool IsAvaliable() = 0;
};

template <class T>
class scoped_ptr {
private:
    T* mPtr;

public:
    scoped_ptr(T* ptr) : mPtr(ptr) {
        if (mPtr != NULL) mPtr->AddRef();
    }
    ~scoped_ptr() {
        if (mPtr != NULL) mPtr->Release();
    }

    scoped_ptr(const scoped_ptr& res) {
        mPtr = res.mPtr;
        if (mPtr != NULL) {
            mPtr->AddRef();
        }
    }

    scoped_ptr& operator=(const scoped_ptr& right) {
        if (mPtr != NULL) {
            mPtr->Release();
            mPtr = NULL;
        }
        mPtr = right.mPtr;
        if (mPtr != NULL) {
            mPtr->AddRef();
        }
        return *this;
    }
    T* get() { return mPtr; }

    T* operator->() { return mPtr; }

    T* Detach() {
        Resource* ret = mPtr;
        mPtr = NULL;
        return ret;
    }
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

typedef scoped_ptr<Resource> AutoCloseResource;

namespace ValueType {
const int kNULL = 0;
const int kInteger = 1;
const int kFloat = 2;
const int kString = 3;
const int kArray = 4;
const int kMap = 5;
const int kResource = 6;
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
    AutoCloseResource resource;
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

    Value(const AutoCloseResource& res)
            : Type(ValueType::kResource), resource(res), bytes(), Integer(0), _array(), _map() {}
    Value(Resource* res)
            : Type(ValueType::kResource),
              resource(AutoCloseResource(res)),
              bytes(),
              Integer(0),
              _array(),
              _map() {}
    Value operator+(Value& right) {
        if (this->Type == ValueType::kString && right.Type == ValueType::kString) {
            return Value(bytes + right.bytes);
        }
        if (!IsArithmeticOperationEnabled(right)) {
            throw RuntimeException("arithmetic operation not avaliable for this value ");
        }
        if (Type == ValueType::kFloat || right.Type == ValueType::kFloat) {
            return Value(ToFloat() + right.ToFloat());
        }
        return Value(ToInteger() + right.ToInteger());
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
    Value operator*(Value& right) {
        if (!IsArithmeticOperationEnabled(right)) {
            throw RuntimeException("arithmetic operation not avaliable for this value ");
        }
        if (Type == ValueType::kFloat || right.Type == ValueType::kFloat) {
            return Value(ToFloat() * right.ToFloat());
        }
        return Value(ToInteger() * right.ToInteger());
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
        if (Type == ValueType::kString && right.Type == ValueType::kString) {
            return bytes == right.bytes;
        }
        return false;
    }
    bool operator!=(const Value& right) const {
        if (IsArithmeticOperationEnabled(right)) {
            return ToFloat() != right.ToFloat();
        }
        if (Type == ValueType::kString && right.Type == ValueType::kString) {
            return bytes != right.bytes;
        }
        return true;
    }

    Value operator[](int index) {
        if (Type == ValueType::kString) {
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
        throw RuntimeException("this value type not have length ");
    }

    Value sub_slice(Value f, Value t) {
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
            to = f.Integer;
        } else {
            throw RuntimeException("the index key type must a Integer");
        }
        if (to > length() || from > length()) {
            throw RuntimeException("index of string out of range");
        }

        if (Type == ValueType::kString) {
            std::string sub = bytes.substr(from, to - from);
            return Value(sub);
        }
        std::vector<Value> result;
        for (size_t i = from; i < to; i++) {
            result.push_back(_array[i]);
        }
        return Value(result);
    }

    Value operator[](Value key) {
        if (Type == ValueType::kString || Type == ValueType::kArray) {
            if (key.Type != ValueType::kInteger) {
                throw RuntimeException("the index key type must a Integer");
            }
            if (Type == ValueType::kString) {
                if (key.Integer >= bytes.size()) {
                    throw RuntimeException("index of string out of range");
                }
                return Value((long)bytes[key.Integer]);
            }
            if (key.Integer >= _array.size()) {
                throw RuntimeException("index of string out of range");
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
        if (Type == ValueType::kString || Type == ValueType::kArray) {
            if (key.Type != ValueType::kInteger) {
                throw RuntimeException("the index key type must a Integer");
            }
            if (Type == ValueType::kString) {
                if (key.Integer >= bytes.size()) {
                    throw RuntimeException("index of string out of range");
                }
                bytes[key.Integer] = (char)val.ToInteger();
                return;
            }
            if (key.Integer >= _array.size()) {
                throw RuntimeException("index of string out of range");
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
        if (Type == ValueType::kString) {
            return bytes.size() != 0;
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
            return "String";
        case ValueType::kInteger:
            return "Integer";
        case ValueType::kNULL:
            return "NULL";
        case ValueType::kFloat:
            return "Float";
        case ValueType::kArray:
            return "Array";
        case ValueType::kMap:
            return "Map";
        case ValueType::kResource:
            return "Resource";
        default:
            return "Unknown";
        }
    }

    std::string ToString() const {
        char buffer[16] = {0};
        switch (Type) {
        case ValueType::kString:
            return "\"" + bytes + "\"";
        case ValueType::kInteger:
            snprintf(buffer, 16, "%ld", Integer);
            return buffer;
        case ValueType::kNULL:
            return "NULL";
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