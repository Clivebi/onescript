
#pragma once
#include <stdlib.h>

#include <map>
#include <string>
#include <vector>

#include "exception.hpp"

namespace Interpreter {

namespace ValueType {
const int kNULL = 0;
const int kInteger = 1;
const int kFloat = 2;
const int kString = 3;
const int kInstructionPointer = 4;
}; // namespace ValueType

class Instruction;
class Value {
public:
    int Type;
    union {
        long Integer;
        double Float;
        Instruction* Ins;
    };
    std::string bytes;
    Value(bool val) : Type(ValueType::kInteger) {
        Integer = 0;
        if (val) {
            Integer = 1;
        }
    }
    Value() : Type(ValueType::kNULL), bytes(), Integer(0) {}
    Value(const char* val) : Type(ValueType::kString), bytes(val), Integer(0) {}
    Value(const std::string& val) : Type(ValueType::kString), bytes(val), Integer(0) {}
    Value(const long& val) : Type(ValueType::kInteger), bytes(), Integer(val) {}
    Value(const double& val) : Type(ValueType::kFloat), bytes(), Float(val) {}
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
    bool operator<(Value& right) {
        if (!IsArithmeticOperationEnabled(right)) {
            throw RuntimeException("arithmetic operation not avaliable for this value ");
        }
        return ToFloat() < right.ToFloat();
    }
    bool operator<=(Value& right) {
        if (!IsArithmeticOperationEnabled(right)) {
            throw RuntimeException("arithmetic operation not avaliable for this value ");
        }
        return ToFloat() <= right.ToFloat();
    }
    bool operator>(Value& right) {
        if (!IsArithmeticOperationEnabled(right)) {
            throw RuntimeException("arithmetic operation not avaliable for this value ");
        }
        return ToFloat() > right.ToFloat();
    }
    bool operator>=(Value& right) {
        if (!IsArithmeticOperationEnabled(right)) {
            throw RuntimeException("arithmetic operation not avaliable for this value ");
        }
        return ToFloat() >= right.ToFloat();
    }
    bool operator==(Value& right) {
        if (IsArithmeticOperationEnabled(right)) {
            return ToFloat() == right.ToFloat();
        }
        if (Type == ValueType::kString && right.Type == ValueType::kString) {
            return bytes == right.bytes;
        }
        if (Type == ValueType::kInstructionPointer &&
            right.Type == ValueType::kInstructionPointer) {
            return Ins == right.Ins;
        }
        return false;
    }
    bool operator!=(Value& right) {
        if (IsArithmeticOperationEnabled(right)) {
            return ToFloat() != right.ToFloat();
        }
        if (Type == ValueType::kString && right.Type == ValueType::kString) {
            return bytes != right.bytes;
        }
        if (Type == ValueType::kInstructionPointer &&
            right.Type == ValueType::kInstructionPointer) {
            return Ins != right.Ins;
        }
        return true;
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

    std::string ToString() {
        char buffer[16] = {0};
        switch (Type) {
        case ValueType::kString:
            return bytes;
        case ValueType::kInteger:
            snprintf(buffer, 16, "%ld", Integer);
            return buffer;
        case ValueType::kNULL:
            return "NULL";
        case ValueType::kFloat:
            snprintf(buffer, 16, "%f", Float);
            return buffer;

        default:
            return "";
        }
    }

protected:
    long ToInteger() {
        if (Type == ValueType::kFloat) {
            return (long)Float;
        }
        if (Type == ValueType::kInteger) {
            return Integer;
        }
        return 0;
    }

    double ToFloat() {
        if (Type == ValueType::kFloat) {
            return Float;
        }
        if (Type == ValueType::kInteger) {
            return (double)Integer;
        }
        return 0;
    }

private:
    bool IsArithmeticOperationEnabled(Value& right) {
        return (Type == ValueType::kFloat || Type == ValueType::kInteger) &&
               (right.Type == ValueType::kFloat || right.Type == ValueType::kInteger);
    }
};
} // namespace Interpreter