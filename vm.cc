#include "vm.hpp"

#include "logger.hpp"

extern int g_builtinMethod_size;
extern Interpreter::BuiltinMethod g_builtinMethod[];

namespace Interpreter {

Executor::Executor():mScript(NULL) {
    RegisgerFunction(g_builtinMethod, g_builtinMethod_size);
}

bool Executor::Execute(scoped_ptr<Script> script,std::string& errmsg) {
    bool bRet = false;
    mScript = script;
    scoped_ptr<VMContext> context = new VMContext(VMContext::File, NULL);
    try
    {
        Execute(mScript->EntryPoint, context);
        bRet = true;
    }
    catch(const RuntimeException& e)
    {
        errmsg = e.what();
    }
    mScript = NULL;
    return bRet;
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

Value Executor::Execute(Instruction* ins, scoped_ptr<VMContext> ctx) {
    if (ctx->IsExecutedInterupt()) {
        LOG("Instruction execute interupted :" + ins->ToString());
        return ctx->GetReturnValue();
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
        Value newVal = valOld + val;
        ctx->SetVarValue(ins->Name, newVal);
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
        ctx->ReturnExecuted(Execute(mScript->GetInstruction(ins->Refs[0]), ctx));
        return Value();
    }

    case Instructions::kFORStatement: {
        scoped_ptr<VMContext> newCtx = new VMContext(VMContext::For, ctx.get());
        ExecuteForStatement(ins, newCtx);
        return Value();
    }
    case Instructions::kForInStatement: {
        scoped_ptr<VMContext> newCtx = new VMContext(VMContext::For, ctx.get());
        ExecuteForInStatement(ins, newCtx);
        return Value();
    }
    case Instructions::kSwitchCaseStatement: {
        scoped_ptr<VMContext> newCtx = new VMContext(VMContext::Switch, ctx.get());
        ExecuteSwitchStatement(ins, newCtx);
        return Value();
    }
    case Instructions::kBREAKStatement: {
        ctx->BreakExecuted();
        return Value();
    }
    case Instructions::kCONTINUEStatement: {
        ctx->ContinueExecuted();
        return Value();
    }
    case Instructions::kCreateMap:
        return ExecuteCreateMap(ins, ctx);
    case Instructions::kCreateArray:
        return ExecuteCreateArray(ins, ctx);
    case Instructions::kReadAt:
        return ExecuteArrayReadWrite(ins, ctx);
    case Instructions::kWriteAt:
        return ExecuteArrayReadWrite(ins, ctx);
    case Instructions::kSlice:
        return ExecuteSlice(ins, ctx);
    default:
        LOG("Unknown Instruction:" + ins->ToString());
        return Value();
    }
}

Value Executor::ExecuteIfStatement(Instruction* ins, scoped_ptr<VMContext> ctx) {
    Instruction* one = mScript->GetInstruction(ins->Refs[0]);
    Instruction* tow = mScript->GetInstruction(ins->Refs[1]);
    Instruction* three = mScript->GetInstruction(ins->Refs[2]);
    Value val = Execute(one, ctx);
    if (val.ToBoolen()) {
        return Value();
    }
    if (tow->OpCode != Instructions::kNop) {
        std::vector<Instruction*> branchs = mScript->GetInstructions(tow->Refs);
        std::vector<Instruction*>::iterator iter = branchs.begin();
        while (iter != branchs.end()) {
            val = Execute(*iter, ctx);
            if (val.ToBoolen()) {
                break;
            }
            if (ctx->IsExecutedInterupt()) {
                return Value();
            }
            iter++;
        }
    }
    if (val.ToBoolen()) {
        return Value();
    }
    if (three->OpCode == Instructions::kNop) {
        return Value();
    }
    Execute(three, ctx);
    return Value();
}

Value Executor::ExecuteArithmeticOperation(Instruction* ins, scoped_ptr<VMContext> ctx) {
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

Value Executor::ExecuteList(std::vector<Instruction*> insList, scoped_ptr<VMContext> ctx) {
    std::vector<Instruction*>::iterator iter = insList.begin();
    while (iter != insList.end()) {
        Execute(*iter, ctx);
        if (ctx->IsExecutedInterupt()) {
            break;
        }
        iter++;
    }
    return Value();
}

Value Executor::CallFunction(Instruction* ins, scoped_ptr<VMContext> ctx) {
    scoped_ptr<VMContext> newCtx = new VMContext(VMContext::Function, ctx.get());
    Instruction* func = ctx->GetFunction(ins->Name);
    if (func == NULL) {
        RUNTIME_FUNCTION method = GetBuiltinMethod(ins->Name);
        if (method == NULL) {
            throw RuntimeException("call unknown function name:" + ins->Name);
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
        Value val = method(actualValues, newCtx, this);
        return val;
    }
    Instruction* formalParamersList = mScript->GetInstruction(func->Refs[0]);
    Instruction* body = mScript->GetInstruction(func->Refs[1]);
    Instruction* actualParamerList = mScript->GetInstruction(ins->Refs[0]);

    if (actualParamerList->Refs.size() != actualParamerList->Refs.size()) {
        throw RuntimeException("actual parameters count not equal formal paramers");
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
    Value val = newCtx->GetReturnValue();
    return val;
}
Value Executor::CallScriptFunction(const std::string& name, std::vector<Value>& args,
                                   scoped_ptr<VMContext> ctx) {
    scoped_ptr<VMContext> newCtx = new VMContext(VMContext::Function, ctx.get());
    Instruction* func = ctx->GetFunction(name);
    Instruction* formalParamersList = mScript->GetInstruction(func->Refs[0]);
    Instruction* body = mScript->GetInstruction(func->Refs[1]);

    if (args.size() != formalParamersList->Refs.size()) {
        throw RuntimeException("actual parameters count not equal formal paramers");
    }
    std::vector<Instruction*> formalParamers = mScript->GetInstructions(formalParamersList->Refs);
    std::vector<Instruction*>::iterator iter = formalParamers.begin();
    int i = 0;
    while (iter != formalParamers.end()) {
        Execute(*iter, ctx);
        newCtx->SetVarValue((*iter)->Name, args[i]);
        i++;
        iter++;
    }
    Execute(body, newCtx);
    Value val = newCtx->GetReturnValue();
    return val;
}

Value Executor::ExecuteForStatement(Instruction* ins, scoped_ptr<VMContext> ctx) {
    Instruction* init = mScript->GetInstruction(ins->Refs[0]);
    Instruction* condition = mScript->GetInstruction(ins->Refs[1]);
    Instruction* after = mScript->GetInstruction(ins->Refs[2]);
    Instruction* block = mScript->GetInstruction(ins->Refs[3]);
    Execute(init, ctx);
    while (true) {
        Value val = Value(1l);
        if (!condition->IsNULL()) {
            val = Execute(condition, ctx);
        }
        if (!val.ToBoolen()) {
            break;
        }
        Execute(block, ctx);
        ctx->CleanContinueFlag();
        if (ctx->IsExecutedInterupt()) {
            break;
        }
        Execute(after, ctx);
    }
    return Value();
}

Value Executor::ExecuteForInStatement(Instruction* ins, scoped_ptr<VMContext> ctx) {
    Instruction* iter_able_obj = mScript->GetInstruction(ins->Refs[0]);
    Instruction* body = mScript->GetInstruction(ins->Refs[1]);
    std::list<std::string> key_val = split(ins->Name, ',');
    std::string key = key_val.front(), val = key_val.back();
    Value objVal = Execute(iter_able_obj, ctx);
    switch (objVal.Type) {
    case ValueType::kString: {
        for (size_t i = 0; i < objVal.bytes.size(); i++) {
            if (key.size() > 0) {
                ctx->SetVarValue(key, Value((long)i));
            }
            ctx->SetVarValue(val, Value((long)objVal.bytes[i]));
            Execute(body, ctx);
            ctx->CleanContinueFlag();
            if (ctx->IsExecutedInterupt()) {
                break;
            }
        }
    } break;
    case ValueType::kArray: {
        for (size_t i = 0; i < objVal._array.size(); i++) {
            if (key.size() > 0) {
                ctx->SetVarValue(key, Value((long)i));
            }
            ctx->SetVarValue(val, objVal._array[i]);
            Execute(body, ctx);
            ctx->CleanContinueFlag();
            if (ctx->IsExecutedInterupt()) {
                break;
            }
        }
    } break;
    case ValueType::kMap: {
        std::map<Value, Value>::iterator iter = objVal._map.begin();
        while (iter != objVal._map.end()) {
            if (key.size() > 0) {
                ctx->SetVarValue(key, iter->first);
            }
            ctx->SetVarValue(val, iter->second);
            iter++;
            Execute(body, ctx);
            ctx->CleanContinueFlag();
            if (ctx->IsExecutedInterupt()) {
                break;
            }
        }

    } break;

    default:
        break;
    }
    return Value();
}

Value Executor::ExecuteCreateMap(Instruction* ins, scoped_ptr<VMContext> ctx) {
    Instruction* list = mScript->GetInstruction(ins->Refs[0]);
    if (list->OpCode == Instructions::kNop) {
        return Value::make_map();
    }
    std::vector<Instruction*> items = mScript->GetInstructions(list->Refs);
    Value val = Value::make_map();
    std::vector<Instruction*>::iterator iter = items.begin();
    while (iter != items.end()) {
        Instruction* key = mScript->GetInstruction((*iter)->Refs[0]);
        Instruction* value = mScript->GetInstruction((*iter)->Refs[1]);
        Value keyVal = Execute(key, ctx);
        Value valVal = Execute(value, ctx);
        val._map[keyVal] = valVal;
        iter++;
    }
    return val;
}
Value Executor::ExecuteCreateArray(Instruction* ins, scoped_ptr<VMContext> ctx) {
    Instruction* list = mScript->GetInstruction(ins->Refs[0]);
    if (list->OpCode == Instructions::kNop) {
        return Value::make_array();
    }
    std::vector<Instruction*> items = mScript->GetInstructions(list->Refs);
    Value val = Value::make_array();
    std::vector<Instruction*>::iterator iter = items.begin();
    while (iter != items.end()) {
        val._array.push_back(Execute((*iter), ctx));
        iter++;
    }
    return val;
}
Value Executor::ExecuteSlice(Instruction* ins, scoped_ptr<VMContext> ctx) {
    Instruction* from = mScript->GetInstruction(ins->Refs[0]);
    Instruction* to = mScript->GetInstruction(ins->Refs[1]);
    Value fromVal = Execute(from, ctx);
    Value toVal = Execute(to, ctx);
    Value opObj = ctx->GetVarValue(ins->Name);
    return opObj.sub_slice(fromVal, toVal);
}
Value Executor::ExecuteArrayReadWrite(Instruction* ins, scoped_ptr<VMContext> ctx) {
    Instruction* where = mScript->GetInstruction(ins->Refs[0]);
    Value index = Execute(where, ctx);
    Value opObj = ctx->GetVarValue(ins->Name);
    if (ins->OpCode == Instructions::kReadAt) {
        return opObj[index];
    }
    Instruction* value = mScript->GetInstruction(ins->Refs[1]);
    Value eleVal = Execute(value, ctx);
    opObj.set_value(index, eleVal);
    ctx->SetVarValue(ins->Name, opObj);
    return opObj;
}

std::vector<Value> Executor::InstructionToValue(std::vector<Instruction*> insList, scoped_ptr<VMContext> ctx) {
    std::vector<Value> result;
    std::vector<Instruction*>::iterator iter = insList.begin();
    while (iter != insList.end()) {
        result.push_back(Execute(*iter, ctx));
        iter++;
    }
    return result;
}

Value Executor::ExecuteSwitchStatement(Instruction* ins, scoped_ptr<VMContext> ctx) {
    Instruction* value = mScript->GetInstruction(ins->Refs[0]);
    Instruction* cases = mScript->GetInstruction(ins->Refs[1]);
    Instruction* defaultBranch = mScript->GetInstruction(ins->Refs[2]);
    std::vector<Instruction*> case_array = mScript->GetInstructions(cases->Refs);
    Value val = Execute(value, ctx);
    std::vector<Instruction*>::iterator iter = case_array.begin();
    bool casehit = false;
    while (iter != case_array.end()) {
        std::vector<Instruction*> conditions =
                mScript->GetInstructions(mScript->GetInstruction((*iter)->Refs[0])->Refs);
        std::vector<Value> condition_values = InstructionToValue(conditions, ctx);
        Instruction* actions = mScript->GetInstruction((*iter)->Refs[1]);
        std::vector<Value>::iterator iter2 = condition_values.begin();
        bool found = false;
        while (iter2 != condition_values.end()) {
            if (val == *iter2) {
                found = true;
                break;
            }
            iter2++;
        }
        if (!found) {
            iter++;
            continue;
        }
        casehit = true;
        Execute(actions, ctx);
        if (ctx->IsExecutedInterupt()) {
            break;
        }
        iter++;
    }
    if (casehit) {
        return Value();
    }
    Execute(defaultBranch, ctx);
    return Value();
}

} // namespace Interpreter