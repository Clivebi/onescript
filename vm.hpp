#pragma once
#include <map>
#include <string>
#include <vector>

#include "exception.hpp"
#include "logger.hpp"
#include "script.hpp"
#include "value.hpp"
#include "vmcontext.hpp"
#define VERSION ("0.1")
namespace Interpreter {

class Executor;

typedef Value (*RUNTIME_FUNCTION)(std::vector<Value>& values, VMContext* ctx, Executor* vm);

typedef struct _BuiltinMethod {
    std::string name;
    RUNTIME_FUNCTION func;
} BuiltinMethod;

class ExecutorCallback {
public:
    virtual scoped_refptr<Script> LoadScript(const char* name) = 0;
};

class Executor {
public:
    Executor(ExecutorCallback* callback);

public:
    bool Execute(scoped_refptr<Script> script, std::string& errmsg, bool showWarning = false);
    void RegisgerFunction(BuiltinMethod methods[], int count);
    Value CallScriptFunction(const std::string& name, std::vector<Value>& value, VMContext* ctx);
    void RequireScript(const std::string& name, scoped_refptr<VMContext> ctx);
    Value GetAvailableFunction(VMContext* ctx);

protected:
    Value Execute(const Instruction* ins, scoped_refptr<VMContext> ctx);
    Value ExecuteList(std::vector<const Instruction*> insList, scoped_refptr<VMContext> ctx);
    Value CallFunction(const Instruction* ins, scoped_refptr<VMContext> ctx);
    Value ExecuteIfStatement(const Instruction* ins, scoped_refptr<VMContext> ctx);
    Value ExecuteForStatement(const Instruction* ins, scoped_refptr<VMContext> ctx);
    Value ExecuteForInStatement(const Instruction* ins, scoped_refptr<VMContext> ctx);
    Value ExecuteArithmeticOperation(const Instruction* ins, scoped_refptr<VMContext> ctx);
    Value ExecuteXUpdate(const Instruction* ins, scoped_refptr<VMContext> ctx);
    Value ExecuteCreateMap(const Instruction* ins, scoped_refptr<VMContext> ctx);
    Value ExecuteCreateArray(const Instruction* ins, scoped_refptr<VMContext> ctx);
    Value ExecuteSlice(const Instruction* ins, scoped_refptr<VMContext> ctx);
    Value ExecuteArrayReadWrite(const Instruction* ins, scoped_refptr<VMContext> ctx);
    Value ExecuteSwitchStatement(const Instruction* ins, scoped_refptr<VMContext> ctx);
    RUNTIME_FUNCTION GetBuiltinMethod(const std::string& name);
    const Instruction* GetInstruction(Instruction::keyType key);
    std::vector<const Instruction*> GetInstructions(std::vector<Instruction::keyType> keys);
    Value GetConstValue(Instruction::keyType key);
    std::vector<Value> InstructionToValue(std::vector<const Instruction*> ins,
                                          scoped_refptr<VMContext> ctx);

protected:
    ExecutorCallback* mCallback;
    std::list<scoped_refptr<Script>> mScriptList;
    std::map<std::string, RUNTIME_FUNCTION> mBuiltinMethods;
};
} // namespace Interpreter