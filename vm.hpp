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

typedef Value (*RUNTIME_FUNCTION)(std::vector<Value>& values, scoped_ptr<VMContext> ctx,
                                  Executor* vm);

typedef struct _BuiltinMethod {
    std::string name;
    RUNTIME_FUNCTION func;
} BuiltinMethod;

class Executor {
public:
    Executor();

public:
    bool Execute(scoped_ptr<Script> script, std::string& errmsg,bool showWarning=false);
    void RegisgerFunction(BuiltinMethod methods[], int count);
    Value CallScriptFunction(const std::string& name, std::vector<Value>& value,
                             scoped_ptr<VMContext> ctx);

protected:
    Value Execute(Instruction* ins, scoped_ptr<VMContext> ctx);
    Value ExecuteList(std::vector<Instruction*> insList, scoped_ptr<VMContext> ctx);
    Value CallFunction(Instruction* ins, scoped_ptr<VMContext> ctx);
    Value ExecuteIfStatement(Instruction* ins, scoped_ptr<VMContext> ctx);
    Value ExecuteForStatement(Instruction* ins, scoped_ptr<VMContext> ctx);
    Value ExecuteForInStatement(Instruction* ins, scoped_ptr<VMContext> ctx);
    Value ExecuteArithmeticOperation(Instruction* ins, scoped_ptr<VMContext> ctx);
    Value ExecuteCreateMap(Instruction* ins, scoped_ptr<VMContext> ctx);
    Value ExecuteCreateArray(Instruction* ins, scoped_ptr<VMContext> ctx);
    Value ExecuteSlice(Instruction* ins, scoped_ptr<VMContext> ctx);
    Value ExecuteArrayReadWrite(Instruction* ins, scoped_ptr<VMContext> ctx);
    Value ExecuteSwitchStatement(Instruction* ins, scoped_ptr<VMContext> ctx);
    RUNTIME_FUNCTION GetBuiltinMethod(const std::string& name);

    std::vector<Value> InstructionToValue(std::vector<Instruction*> ins, scoped_ptr<VMContext> ctx);

protected:
    scoped_ptr<Script> mScript;
    std::map<std::string, RUNTIME_FUNCTION> mBuiltinMethods;
};
} // namespace Interpreter