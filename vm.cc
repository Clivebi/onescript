#include "vm.hpp"

#include "logger.hpp"
#define COUNT_OF(a) (sizeof(a) / sizeof(a[0]))

extern Interpreter::BuiltinMethod g_builtinMethod[2];

namespace Interpreter {
BuiltinValue g_builtinVar[] = {
        {"false", Value(0l)},
        {"true", Value(1l)},
};

bool Context::CheckVarName(const std::string& name) {
    for (int i = 0; i < COUNT_OF(g_builtinVar); i++) {
        if (g_builtinVar[i].name == name) {
            return false;
        }
    }
    return true;
}

void Context::builtinVar() {
    for (int i = 0; i < COUNT_OF(g_builtinVar); i++) {
        mVars[g_builtinVar[i].name] = g_builtinVar[i].val;
    }
}

Executor::Executor() {
    RegisgerFunction(g_builtinMethod, 2);
}

void Executor::Execute(Script* script) {
    mScript = script;
    Context* context = new Context(Context::File, NULL);
    Execute(mScript->EntryPoint, context);
}

void Executor::RegisgerFunction(BuiltinMethod methods[], int count) {
    for (int i = 0; i < count; i++) {
        mBuiltinMethods[methods[i].name] = methods[i].func;
    }
}

RUNTIME_FUNCTION Executor::GetBuiltinMethod(const std::string& name) {
    std::map<std::string, RUNTIME_FUNCTION>::iterator iter = mBuiltinMethods.find(name);
    if (iter == mBuiltinMethods.end()) {
        return NULL;
    }
    return iter->second;
}

Value Executor::Execute(Instruction* ins, Context* ctx) {
    if (ctx->Flags & Context::FLAGS_RETURN) {
        assert(ctx->isInFunctionBody());
        LOG("被return跳过执行:" + ins->ToString());
        return ctx->ReturnValue;
    }
    if (ctx->Flags & (Context::FLAGS_BREAK | Context::FLAGS_CONTINUE)) {
        assert(ctx->isInForStatement());
        LOG("被break/continue跳过执行:" + ins->ToString());
        return Value();
    }
    if (ins->OpCode >= Instructions::kADD && ins->OpCode <= Instructions::kMAXArithmeticOP) {
        return ExecuteArithmeticOperation(ins, ctx);
    }
    switch (ins->OpCode) {
    case Instructions::kNop:
        return Value();
    case Instructions::kConst:
        return mScript->GetConstValue(ins->Refs[0]);
    case Instructions::kNewVar: {
        ctx->AddVar(ins->Name);
        if (ins->Refs.size() == 1) {
            Value initValue = Execute(mScript->GetInstruction(ins->Refs[0]), ctx);
            ctx->SetVarValue(ins->Name, initValue);
            return initValue;
        }
        return Value();
    }
    case Instructions::kReadVar:
        return ctx->GetVarValue(ins->Name);
    case Instructions::kWriteVar: {
        Value val = Execute(mScript->GetInstruction(ins->Refs.front()), ctx);
        ctx->SetVarValue(ins->Name, val);
        return Value();
    }
    case Instructions::kADDWrite: {
        Value valOld = ctx->GetVarValue(ins->Name);
        Value val = Execute(mScript->GetInstruction(ins->Refs.front()), ctx);
        ctx->SetVarValue(ins->Name, valOld + val);
        return Value();
    }
    case Instructions::kSUBWrite: {
        Value valOld = ctx->GetVarValue(ins->Name);
        Value val = Execute(mScript->GetInstruction(ins->Refs.front()), ctx);
        ctx->SetVarValue(ins->Name, valOld - val);
        return Value();
    }
    case Instructions::kDIVWrite: {
        Value valOld = ctx->GetVarValue(ins->Name);
        Value val = Execute(mScript->GetInstruction(ins->Refs.front()), ctx);
        ctx->SetVarValue(ins->Name, valOld * val);
        return Value();
    }
    case Instructions::kMULWrite: {
        Value valOld = ctx->GetVarValue(ins->Name);
        Value val = Execute(mScript->GetInstruction(ins->Refs.front()), ctx);
        ctx->SetVarValue(ins->Name, valOld / val);
        return Value();
    }
    case Instructions::kINCWrite: {
        Value val = ctx->GetVarValue(ins->Name);
        if (val.Type != ValueType::kInteger) {
            throw RuntimeException("++ not implement for this object type object:" + ins->Name);
        }
        val.Integer += 1;
        ctx->SetVarValue(ins->Name, val);
        return Value();
    }
    case Instructions::kDECWrite: {
        Value val = ctx->GetVarValue(ins->Name);
        if (val.Type != ValueType::kInteger) {
            throw RuntimeException("++ not implement for this object type object:" + ins->Name);
        }
        val.Integer += 1;
        ctx->SetVarValue(ins->Name, val);
        return Value();
    }
    case Instructions::kNewFunction: {
        ctx->AddFunction(ins);
        return Value();
    }
    case Instructions::kCallFunction: {
        return CallFunction(ins, ctx);
    }

    case Instructions::kGroup: {
        ExecuteList(mScript->GetInstructions(ins->Refs), ctx);
        return Value();
    }

    case Instructions::kIFStatement: {
        ExecuteIfStatement(ins, ctx);
        return Value();
    }

    //return indcate the action executed or not
    case Instructions::kContitionExpression: {
        Value val = Execute(mScript->GetInstruction(ins->Refs[0]), ctx);
        if (val.ToBoolen()) {
            Execute(mScript->GetInstruction(ins->Refs[1]), ctx);
        }
        return val;
    }

    case Instructions::kRETURNStatement: {
        if (!ctx->isInFunctionBody()) {
            throw RuntimeException("return statement must in the function body");
        }
        ctx->ReturnValue = Execute(mScript->GetInstruction(ins->Refs[0]), ctx);
        ctx->Flags |= Context::FLAGS_RETURN;
        return Value();
    }

    case Instructions::kFORStatement: {
        Context* newCtx = new Context(Context::For, ctx);
        ExecuteForStatement(ins, newCtx);
        delete newCtx;
        return Value();
    }
    case Instructions::kBREAKStatement: {
        if (!ctx->isInForStatement()) {
            throw RuntimeException("break statement must in the for block");
        }
        ctx->Flags |= Context::FLAGS_BREAK;
        return Value();
    }
    case Instructions::kCONTINUEStatement: {
        if (!ctx->isInForStatement()) {
            throw RuntimeException("continue statement must in the for block");
        }
        ctx->Flags |= Context::FLAGS_CONTINUE;
        return Value();
    }
    default:
        assert(false);
    }
}

void Executor::ExecuteIfStatement(Instruction* ins, Context* ctx) {
    Instruction* one = mScript->GetInstruction(ins->Refs[0]);
    Instruction* tow = mScript->GetInstruction(ins->Refs[1]);
    Instruction* three = mScript->GetInstruction(ins->Refs[2]);
    Value val = Execute(one, ctx);
    if (val.ToBoolen()) {
        return;
    }
    if (tow->OpCode != Instructions::kNop) {
        std::vector<Instruction*> branchs = mScript->GetInstructions(tow->Refs);
        std::vector<Instruction*>::iterator iter = branchs.begin();
        while (iter != branchs.end()) {
            val = Execute(*iter, ctx);
            if (val.ToBoolen()) {
                break;
            }
            if (ctx->Flags & Context::FLAGS_RETURN) {
                return;
            }
            iter++;
        }
    }
    if (val.ToBoolen()) {
        return;
    }
    if (three->OpCode == Instructions::kNop) {
        return;
    }
    Execute(three, ctx);
}

Value Executor::ExecuteArithmeticOperation(Instruction* ins, Context* ctx) {
    Instruction* first = mScript->GetInstruction(ins->Refs[0]);
    Value firstVal = Execute(first, ctx);
    if (ins->OpCode == Instructions::kNOT) {
        return Value(!firstVal.ToBoolen());
    }
    Instruction* second = mScript->GetInstruction(ins->Refs[1]);
    Value secondVal = Execute(second, ctx);

    switch (ins->OpCode) {
    case Instructions::kADD:
        return firstVal + secondVal;
    case Instructions::kSUB:
        return firstVal - secondVal;
    case Instructions::kMUL:
        return firstVal * secondVal;
    case Instructions::kDIV:
        return firstVal / secondVal;
    case Instructions::kMOD:
        return firstVal % secondVal;
    case Instructions::kGT:
        return Value(firstVal > secondVal);
    case Instructions::kGE:
        return Value(firstVal >= secondVal);
    case Instructions::kLT:
        return Value(firstVal < secondVal);
    case Instructions::kLE:
        return Value(firstVal <= secondVal);
    case Instructions::kEQ:
        return Value(firstVal == secondVal);
    case Instructions::kNE:
        return Value(firstVal != secondVal);
    default:
        LOG("Unknow OpCode:" + ins->ToString());
        return Value();
    }
}

void Executor::ExecuteList(std::vector<Instruction*> insList, Context* ctx) {
    std::vector<Instruction*>::iterator iter = insList.begin();
    while (iter != insList.end()) {
        Execute(*iter, ctx);
        if (ctx->Flags & Context::FLAGS_RETURN) {
            break;
        }
        if (ctx->isInForStatement() && ctx->Flags & Context::FLAGS_CONTINUE) {
            break;
        }
        iter++;
    }
}

Value Executor::CallFunction(Instruction* ins, Context* ctx) {
    Context* newCtx = new Context(Context::Function, ctx);
    Instruction* func = ctx->GetFunction(ins->Name);
    if (func == NULL) {
        RUNTIME_FUNCTION method = GetBuiltinMethod(ins->Name);
        if (method == NULL) {
            return Value();
        }
        Instruction* actualParamerList = mScript->GetInstruction(ins->Refs[0]);
        std::vector<Value> actualValues;
        std::vector<Instruction*> actualParamers =
                mScript->GetInstructions(actualParamerList->Refs);
        std::vector<Instruction*>::iterator iter = actualParamers.begin();
        while (iter != actualParamers.end()) {
            actualValues.push_back(Execute(*iter, ctx));
            iter++;
        }
        return method(actualValues);
    }
    Instruction* formalParamersList = mScript->GetInstruction(func->Refs[0]);
    Instruction* body = mScript->GetInstruction(func->Refs[1]);
    Instruction* actualParamerList = mScript->GetInstruction(ins->Refs[0]);

    if (actualParamerList->Refs.size() != actualParamerList->Refs.size()) {
        throw RuntimeException("actual parameters count not equal formal paramers");
        return Value();
    }
    std::vector<Instruction*> actualParamers = mScript->GetInstructions(actualParamerList->Refs);
    std::vector<Value> actualValues;
    std::vector<Instruction*>::iterator iter = actualParamers.begin();
    while (iter != actualParamers.end()) {
        actualValues.push_back(Execute(*iter, ctx));
        iter++;
    }
    std::vector<Instruction*> formalParamers = mScript->GetInstructions(formalParamersList->Refs);
    iter = formalParamers.begin();
    int i = 0;
    while (iter != formalParamers.end()) {
        Execute(*iter, newCtx);
        newCtx->SetVarValue((*iter)->Name, actualValues[i]);
        i++;
        iter++;
    }
    Execute(body, newCtx);
    Value val = newCtx->ReturnValue;
    delete newCtx;
    return val;
}

void Executor::ExecuteForStatement(Instruction* ins, Context* ctx) {
    std::vector<Instruction*> insList = mScript->GetInstructions(ins->Refs);
    if (insList[0]->OpCode != Instructions::kNop) {
        Execute(insList[0], ctx);
    }
    while (true) {
        Value val = Value(1l);
        if (insList[1]->OpCode != Instructions::kNop) {
            val = Execute(insList[1], ctx);
        }
        if (!val.ToBoolen()) {
            break;
        }
        Execute(insList[3], ctx);
        if (ctx->Flags & Context::FLAGS_BREAK) {
            break;
        }
        if (ctx->Flags & Context::FLAGS_RETURN) {
            break;
        }
        if (insList[2]->OpCode != Instructions::kNop) {
            Execute(insList[2], ctx);
        }
        ctx->Flags = 0;
    }
}

} // namespace Interpreter