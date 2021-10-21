#pragma once
#include <map>
#include <string>
#include <vector>

#include "script.hpp"
#include "value.hpp"

namespace Interpreter {

class VMContext:public RefBase {
public:
    enum Type {
        File,
        Function,
        For,
        Switch,
    };

protected:
    enum {
        CONTINUE_FLAG = (1 << 0),
        BREAK_FLAG = (1 << 1),
        RETURN_FLAG = (1 << 2),
        EXIT_FLAG = (1 << 3),
    };

    VMContext* mParent;
    Type mType;
    int mDeepth;
    int mFlags;
    Value mReturnValue;
    VMContext();
    VMContext(const VMContext&);
    VMContext& operator = (const VMContext&);
public:
    VMContext(Type type, VMContext* Parent);
    ~VMContext();

    bool IsExecutedInterupt() { return (mFlags & 0xFF); }
    void CleanContinueFlag() { mFlags &= 0xFE; }
    void BreakExecuted();
    void ContinueExecuted();
    void ExitExecuted(Value exitCode);
    void ReturnExecuted(Value retVal);
    Value GetReturnValue(){return mReturnValue;}

    bool IsReturnAvaliable() { return IsInFunctionContext(); }
    bool IsBreakAvaliable() { return mType == For || mType == Switch; }
    bool IsContinueAvaliable() { return mType == For; }
    bool IsInFunctionContext();

    void AddVar(const std::string& name);
    void SetVarValue(const std::string& name, Value value);
    Value GetVarValue(const std::string& name);

    void AddFunction(Instruction* function);
    Instruction* GetFunction(const std::string& name);

protected:
    void LoadBuiltinVar();
    bool IsBuiltinVarName(const std::string& name);

private:
    std::map<std::string, Value> mVars;
    std::map<std::string, Instruction*> mFunctions;
};

} // namespace Interpreter