#include "vm.hpp"

#include "logger.hpp"

extern int g_builtinMethod_size;
extern Interpreter::BuiltinMethod g_builtinMethod[];

namespace Interpreter {

Executor::Executor(ExecutorCallback* callback) : mScriptList(), mCallback(callback) {
    RegisgerFunction(g_builtinMethod, g_builtinMethod_size);
}

bool Executor::Execute(scoped_refptr<Script> script, std::string& errmsg, bool showWarning) {
    bool bRet = false;
    mScriptList.push_back(script);
    scoped_refptr<VMContext> context = new VMContext(VMContext::File, NULL);
    context->SetEnableWarning(showWarning);
    try {
        Execute(script->EntryPoint, context);
        bRet = true;
    } catch (const RuntimeException& e) {
        errmsg = e.what();
    }
    mScriptList.clear();
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

void Executor::RequireScript(const std::string& name, scoped_refptr<VMContext> ctx) {
    std::list<scoped_refptr<Script>>::iterator iter = mScriptList.begin();
    while (iter != mScriptList.end()) {
        if ((*iter)->Name == name) {
            return;
        }
        iter++;
    }

    if (mCallback) {
        scoped_refptr<Script> required = mCallback->LoadScript(name.c_str());
        if (required.get() == NULL) {
            throw RuntimeException("load script <" + name + "> failed");
        }
        scoped_refptr<Script> last = mScriptList.back();
        required->RelocateInstruction(last->GetNextInstructionKey() + 100,
                                      last->GetNextConstKey() + 100);
        mScriptList.push_back(required);
        Execute(required->EntryPoint, ctx);
    }
}

const Instruction* Executor::GetInstruction(Instruction::keyType key) {
    std::list<scoped_refptr<Script>>::iterator iter = mScriptList.begin();
    while (iter != mScriptList.end()) {
        if ((*iter)->IsContainInstruction(key)) {
            return (*iter)->GetInstruction(key);
        }
        iter++;
    }
    char buf[16] = {0};
    snprintf(buf, 16, "%d", key);
    throw RuntimeException(std::string("unknown instruction key:") + buf);
}

std::vector<const Instruction*> Executor::GetInstructions(std::vector<Instruction::keyType> keys) {
    std::list<scoped_refptr<Script>>::iterator iter = mScriptList.begin();
    while (iter != mScriptList.end()) {
        if ((*iter)->IsContainInstruction(keys[0])) {
            return (*iter)->GetInstructions(keys);
        }
        iter++;
    }
    char buf[16] = {0};
    snprintf(buf, 16, "%d", keys[0]);
    throw RuntimeException(std::string("unknown instruction key:") + buf);
}

Value Executor::GetConstValue(Instruction::keyType key) {
    std::list<scoped_refptr<Script>>::iterator iter = mScriptList.begin();
    while (iter != mScriptList.end()) {
        if ((*iter)->IsContainConst(key)) {
            return (*iter)->GetConstValue(key);
        }
        iter++;
    }
    throw RuntimeException("unknown const key");
}

Value Executor::Execute(const Instruction* ins, scoped_refptr<VMContext> ctx) {
    //LOG("execute " + ins->ToString());
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
        return GetConstValue(ins->Refs[0]);
    case Instructions::kNewVar: {
        ctx->AddVar(ins->Name);
        if (ins->Refs.size() == 1) {
            Value initValue = Execute(GetInstruction(ins->Refs[0]), ctx);
            ctx->SetVarValue(ins->Name, initValue);
            return initValue;
        }
        return Value();
    }
    case Instructions::kReadVar:
        return ctx->GetVarValue(ins->Name);
    case Instructions::kWriteVar: {
        Value val = Execute(GetInstruction(ins->Refs.front()), ctx);
        ctx->SetVarValue(ins->Name, val);
        return Value();
    }
    case Instructions::kADDWrite: {
        Value valOld = ctx->GetVarValue(ins->Name);
        Value val = Execute(GetInstruction(ins->Refs.front()), ctx);
        Value newVal = valOld + val;
        ctx->SetVarValue(ins->Name, newVal);
        return Value();
    }
    case Instructions::kSUBWrite: {
        Value valOld = ctx->GetVarValue(ins->Name);
        Value val = Execute(GetInstruction(ins->Refs.front()), ctx);
        ctx->SetVarValue(ins->Name, valOld - val);
        return Value();
    }
    case Instructions::kDIVWrite: {
        Value valOld = ctx->GetVarValue(ins->Name);
        Value val = Execute(GetInstruction(ins->Refs.front()), ctx);
        ctx->SetVarValue(ins->Name, valOld * val);
        return Value();
    }
    case Instructions::kMULWrite: {
        Value valOld = ctx->GetVarValue(ins->Name);
        Value val = Execute(GetInstruction(ins->Refs.front()), ctx);
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
        ExecuteList(GetInstructions(ins->Refs), ctx);
        return Value();
    }

    case Instructions::kIFStatement: {
        ExecuteIfStatement(ins, ctx);
        return Value();
    }

    //return indcate the action executed or not
    case Instructions::kContitionExpression: {
        Value val = Execute(GetInstruction(ins->Refs[0]), ctx);
        if (val.ToBoolen()) {
            Execute(GetInstruction(ins->Refs[1]), ctx);
        }
        return val;
    }

    case Instructions::kRETURNStatement: {
        ctx->ReturnExecuted(Execute(GetInstruction(ins->Refs[0]), ctx));
        return Value();
    }

    case Instructions::kFORStatement: {
        scoped_refptr<VMContext> newCtx = new VMContext(VMContext::For, ctx.get());
        ExecuteForStatement(ins, newCtx);
        return Value();
    }
    case Instructions::kForInStatement: {
        scoped_refptr<VMContext> newCtx = new VMContext(VMContext::For, ctx.get());
        ExecuteForInStatement(ins, newCtx);
        return Value();
    }
    case Instructions::kSwitchCaseStatement: {
        scoped_refptr<VMContext> newCtx = new VMContext(VMContext::Switch, ctx.get());
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

Value Executor::ExecuteIfStatement(const Instruction* ins, scoped_refptr<VMContext> ctx) {
    const Instruction* one = GetInstruction(ins->Refs[0]);
    const Instruction* tow = GetInstruction(ins->Refs[1]);
    const Instruction* three = GetInstruction(ins->Refs[2]);
    Value val = Execute(one, ctx);
    if (val.ToBoolen()) {
        return Value();
    }
    if (ctx->IsExecutedInterupt()) {
        return Value();
    }
    if (tow->OpCode != Instructions::kNop) {
        std::vector<const Instruction*> branchs = GetInstructions(tow->Refs);
        std::vector<const Instruction*>::iterator iter = branchs.begin();
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

Value Executor::ExecuteArithmeticOperation(const Instruction* ins, scoped_refptr<VMContext> ctx) {
    const Instruction* first = GetInstruction(ins->Refs[0]);
    Value firstVal = Execute(first, ctx);
    if (ins->OpCode == Instructions::kNOT) {
        return Value(!firstVal.ToBoolen());
    }
    const Instruction* second = GetInstruction(ins->Refs[1]);
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

Value Executor::ExecuteList(std::vector<const Instruction*> insList, scoped_refptr<VMContext> ctx) {
    std::vector<const Instruction*>::iterator iter = insList.begin();
    while (iter != insList.end()) {
        Execute(*iter, ctx);
        if (ctx->IsExecutedInterupt()) {
            break;
        }
        iter++;
    }
    return Value();
}

Value Executor::CallFunction(const Instruction* ins, scoped_refptr<VMContext> ctx) {
    scoped_refptr<VMContext> newCtx = new VMContext(VMContext::Function, ctx.get());
    const Instruction* func = ctx->GetFunction(ins->Name);
    if (func == NULL) {
        RUNTIME_FUNCTION method = GetBuiltinMethod(ins->Name);
        if (method == NULL) {
            throw RuntimeException("call unknown function name:" + ins->Name);
        }
        const Instruction* actualParamerList = GetInstruction(ins->Refs[0]);
        std::vector<Value> actualValues;
        std::vector<const Instruction*> actualParamers = GetInstructions(actualParamerList->Refs);
        std::vector<const Instruction*>::iterator iter = actualParamers.begin();
        while (iter != actualParamers.end()) {
            actualValues.push_back(Execute(*iter, ctx));
            if (ctx->IsExecutedInterupt()) {
                return Value();
            }
            iter++;
        }
        Value val = method(actualValues, ctx, this);
        return val;
    }
    const Instruction* formalParamersList = GetInstruction(func->Refs[0]);
    const Instruction* body = GetInstruction(func->Refs[1]);
    const Instruction* actualParamerList = GetInstruction(ins->Refs[0]);

    if (actualParamerList->Refs.size() != actualParamerList->Refs.size()) {
        throw RuntimeException("actual parameters count not equal formal paramers");
    }
    std::vector<const Instruction*> actualParamers = GetInstructions(actualParamerList->Refs);
    std::vector<Value> actualValues;
    std::vector<const Instruction*>::iterator iter = actualParamers.begin();
    while (iter != actualParamers.end()) {
        actualValues.push_back(Execute(*iter, ctx));
        if (ctx->IsExecutedInterupt()) {
            return Value();
        }
        iter++;
    }
    std::vector<const Instruction*> formalParamers = GetInstructions(formalParamersList->Refs);
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
                                   scoped_refptr<VMContext> ctx) {
    scoped_refptr<VMContext> newCtx = new VMContext(VMContext::Function, ctx.get());
    const Instruction* func = ctx->GetFunction(name);
    const Instruction* formalParamersList = GetInstruction(func->Refs[0]);
    const Instruction* body = GetInstruction(func->Refs[1]);

    if (args.size() != formalParamersList->Refs.size()) {
        throw RuntimeException("actual parameters count not equal formal paramers");
    }
    std::vector<const Instruction*> formalParamers = GetInstructions(formalParamersList->Refs);
    std::vector<const Instruction*>::iterator iter = formalParamers.begin();
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

Value Executor::ExecuteForStatement(const Instruction* ins, scoped_refptr<VMContext> ctx) {
    const Instruction* init = GetInstruction(ins->Refs[0]);
    const Instruction* condition = GetInstruction(ins->Refs[1]);
    const Instruction* after = GetInstruction(ins->Refs[2]);
    const Instruction* block = GetInstruction(ins->Refs[3]);
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

Value Executor::ExecuteForInStatement(const Instruction* ins, scoped_refptr<VMContext> ctx) {
    const Instruction* iter_able_obj = GetInstruction(ins->Refs[0]);
    const Instruction* body = GetInstruction(ins->Refs[1]);
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

Value Executor::ExecuteCreateMap(const Instruction* ins, scoped_refptr<VMContext> ctx) {
    const Instruction* list = GetInstruction(ins->Refs[0]);
    if (list->OpCode == Instructions::kNop) {
        return Value::make_map();
    }
    std::vector<const Instruction*> items = GetInstructions(list->Refs);
    Value val = Value::make_map();
    std::vector<const Instruction*>::iterator iter = items.begin();
    while (iter != items.end()) {
        const Instruction* key = GetInstruction((*iter)->Refs[0]);
        const Instruction* value = GetInstruction((*iter)->Refs[1]);
        Value keyVal = Execute(key, ctx);
        Value valVal = Execute(value, ctx);
        if (ctx->IsExecutedInterupt()) {
            return Value();
        }
        val._map[keyVal] = valVal;
        iter++;
    }
    return val;
}
Value Executor::ExecuteCreateArray(const Instruction* ins, scoped_refptr<VMContext> ctx) {
    const Instruction* list = GetInstruction(ins->Refs[0]);
    if (list->OpCode == Instructions::kNop) {
        return Value::make_array();
    }
    std::vector<const Instruction*> items = GetInstructions(list->Refs);
    Value val = Value::make_array();
    std::vector<const Instruction*>::iterator iter = items.begin();
    while (iter != items.end()) {
        val._array.push_back(Execute((*iter), ctx));
        if (ctx->IsExecutedInterupt()) {
            return Value();
        }
        iter++;
    }
    return val;
}
Value Executor::ExecuteSlice(const Instruction* ins, scoped_refptr<VMContext> ctx) {
    const Instruction* from = GetInstruction(ins->Refs[0]);
    const Instruction* to = GetInstruction(ins->Refs[1]);
    Value fromVal = Execute(from, ctx);
    Value toVal = Execute(to, ctx);
    Value opObj = ctx->GetVarValue(ins->Name);
    return opObj.sub_slice(fromVal, toVal);
}
Value Executor::ExecuteArrayReadWrite(const Instruction* ins, scoped_refptr<VMContext> ctx) {
    const Instruction* where = GetInstruction(ins->Refs[0]);
    Value index = Execute(where, ctx);
    Value opObj = ctx->GetVarValue(ins->Name);
    if (ins->OpCode == Instructions::kReadAt) {
        return opObj[index];
    }
    const Instruction* value = GetInstruction(ins->Refs[1]);
    Value eleVal = Execute(value, ctx);
    opObj.set_value(index, eleVal);
    ctx->SetVarValue(ins->Name, opObj);
    return opObj;
}

std::vector<Value> Executor::InstructionToValue(std::vector<const Instruction*> insList,
                                                scoped_refptr<VMContext> ctx) {
    std::vector<Value> result;
    std::vector<const Instruction*>::iterator iter = insList.begin();
    while (iter != insList.end()) {
        result.push_back(Execute(*iter, ctx));
        iter++;
    }
    return result;
}

Value Executor::ExecuteSwitchStatement(const Instruction* ins, scoped_refptr<VMContext> ctx) {
    const Instruction* value = GetInstruction(ins->Refs[0]);
    const Instruction* cases = GetInstruction(ins->Refs[1]);
    const Instruction* defaultBranch = GetInstruction(ins->Refs[2]);
    std::vector<const Instruction*> case_array = GetInstructions(cases->Refs);
    Value val = Execute(value, ctx);
    std::vector<const Instruction*>::iterator iter = case_array.begin();
    bool casehit = false;
    while (iter != case_array.end()) {
        std::vector<const Instruction*> conditions =
                GetInstructions(GetInstruction((*iter)->Refs[0])->Refs);
        std::vector<Value> condition_values = InstructionToValue(conditions, ctx);
        const Instruction* actions = GetInstruction((*iter)->Refs[1]);
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