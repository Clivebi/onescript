#pragma once
#include <map>
#include <string>
#include <vector>

#include "exception.hpp"
#include "logger.hpp"
#include "script.hpp"
#include "value.hpp"
#include "vmcontext.hpp"

namespace Interpreter {

class Executor;

typedef Value (*RUNTIME_FUNCTION)(std::vector<Value>& values,VMContext* ctx,Executor* vm);

typedef struct _BuiltinMethod {
    std::string name;
    RUNTIME_FUNCTION func;
} BuiltinMethod;

class Executor {
public:
    Executor();

public:
    void Execute(Script* script);
    void RegisgerFunction(BuiltinMethod methods[], int count);
    Value CallScriptFunction(const std::string& name,std::vector<Value>& value,VMContext* ctx);

protected:
    Value Execute(Instruction* ins, VMContext* ctx);
    Value ExecuteList(std::vector<Instruction*> insList, VMContext* ctx);
    Value CallFunction(Instruction* ins, VMContext* ctx);
    Value ExecuteIfStatement(Instruction* ins, VMContext* ctx);
    Value ExecuteForStatement(Instruction* ins, VMContext* ctx);
    Value ExecuteForInStatement(Instruction* ins, VMContext* ctx);
    Value ExecuteArithmeticOperation(Instruction* ins, VMContext* ctx);
    Value ExecuteCreateMap(Instruction* ins, VMContext* ctx);
    Value ExecuteCreateArray(Instruction* ins, VMContext* ctx);
    Value ExecuteSlice(Instruction* ins, VMContext* ctx);
    Value ExecuteArrayReadWrite(Instruction* ins, VMContext* ctx);
    Value ExecuteSwitchStatement(Instruction* ins, VMContext* ctx);
    RUNTIME_FUNCTION GetBuiltinMethod(const std::string& name);

    std::vector<Value> InstructionToValue(std::vector<Instruction*> ins, VMContext* ctx);

protected:
    Script* mScript;
    std::map<std::string, RUNTIME_FUNCTION> mBuiltinMethods;
};
} // namespace Interpreter