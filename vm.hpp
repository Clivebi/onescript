#pragma once
#include <map>
#include <string>
#include <vector>

#include "exception.hpp"
#include "logger.hpp"
#include "script.hpp"
#include "value.hpp"

namespace Interpreter {

struct BuiltinValue {
    std::string name;
    Value val;
};

class Context {
public:
    enum ContextType {
        File,
        Function,
        For,
    };

    enum {
        FLAGS_RETURN = 1,
        FLAGS_BREAK = (1 << 1),
        FLAGS_CONTINUE = (1 << 2),
    };

    Value ReturnValue;
    int Flags;

protected:
    Context* mParent;
    ContextType mContextType;
    int mDeepth;
    Context();

public:
    Context(ContextType type, Context* Parent) : Flags(0) {
        mParent = Parent;
        mContextType = type;
        mDeepth = 0;
        if (mParent != NULL) {
            mDeepth = mParent->mDeepth + 1;
        }
        builtinVar();
    }

    bool isInForStatement() { return mContextType == For; }

    bool isInFunctionBody() {
        Context* seek = this;
        while (seek) {
            if (seek->mContextType == Function) {
                return true;
            }
            seek = seek->mParent;
        }
        return false;
    }

    bool CheckVarName(const std::string& name);

    void AddVar(const std::string& name) {
        if (!CheckVarName(name)) {
            return;
        }
        std::map<std::string, Value>::iterator iter = mVars.find(name);
        if (iter == mVars.end()) {
            mVars[name] = Value();
            return;
        }
        throw RuntimeException("variable already exist name:" + name);
    }

    void SetVarValue(const std::string& name, Value value) {
        if (!CheckVarName(name)) {
            return;
        }
        if (!SetVarValueIfExist(name, value)) {
            LOG("variable <" + name + "> not found,so new one!");
        }
        mVars[name] = value;
    }

    Value GetVarValue(const std::string& name) {
        Context* ctx = this;
        while (ctx != NULL) {
            std::map<std::string, Value>::iterator iter = ctx->mVars.find(name);
            if (iter != ctx->mVars.end()) {
                return iter->second;
            }
            ctx = ctx->mParent;
        }
        throw RuntimeException("variable not found :" + name);
    }

    void AddFunction(Instruction* obj) {
        if (mContextType != File) {
            throw RuntimeException("function declaration must in the top block name:" + obj->Name);
        }

        std::map<std::string, Instruction*>::iterator iter = mFunctions.find(obj->Name);
        if (iter == mFunctions.end()) {
            mFunctions[obj->Name] = obj;
            return;
        }
        throw RuntimeException("function already exit name:" + obj->Name);
    }

    Instruction* GetFunction(const std::string& name) {
        Context* ctx = this;
        while (ctx->mParent != NULL) {
            ctx = ctx->mParent;
        }
        std::map<std::string, Instruction*>::iterator iter = ctx->mFunctions.find(name);
        if (iter == ctx->mFunctions.end()) {
            if (mParent != NULL) {
                return mParent->GetFunction(name);
            }
            return NULL;
        }
        return iter->second;
    }

protected:
    void builtinVar();

    bool SetVarValueIfExist(const std::string& name, Value value) {
        std::map<std::string, Value>::iterator iter = mVars.find(name);
        if (iter == mVars.end()) {
            if (mParent != NULL) {
                return mParent->SetVarValueIfExist(name, value);
            }
            return false;
        }
        mVars[name] = value;
        return true;
    }

private:
    std::map<std::string, Value> mVars;
    std::map<std::string, Instruction*> mFunctions;
};

typedef Value (*RUNTIME_FUNCTION)(std::vector<Value>& values);

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

protected:
    Value Execute(Instruction* ins, Context* ctx);
    void ExecuteList(std::vector<Instruction*> insList, Context* ctx);
    Value CallFunction(Instruction* ins, Context* ctx);
    void ExecuteIfStatement(Instruction* ins, Context* ctx);
    void ExecuteForStatement(Instruction* ins, Context* ctx);
    void ExecuteForInStatement(Instruction* ins, Context* ctx);
    Value ExecuteArithmeticOperation(Instruction* ins, Context* ctx);
    Value ExecuteCreateMap(Instruction* ins, Context* ctx);
    Value ExecuteCreateArray(Instruction* ins, Context* ctx);
    Value ExecuteSlice(Instruction* ins, Context* ctx);
    Value ExecuteArrayReadWrite(Instruction* ins, Context* ctx);
    RUNTIME_FUNCTION GetBuiltinMethod(const std::string& name);

protected:
    Script* mScript;
    std::map<std::string, RUNTIME_FUNCTION> mBuiltinMethods;
};
} // namespace Interpreter