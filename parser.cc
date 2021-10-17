#include "parser.hpp"

#include <sstream>

#include "logger.hpp"

namespace Interpreter {

Instruction* Parser::NULLObject() {
    return mScript->NULLInstruction();
}

Instruction* Parser::CreateObjectList(Instruction* element) {
    LOG(mScript->DumpInstruction(element, ""));
    return mScript->NewGroup(element);
}
Instruction* Parser::AddObjectToObjectListHead(Instruction* list, Instruction* element) {
    return mScript->AddGroupToHead(list, element);
}
Instruction* Parser::AddObjectToObjectList(Instruction* list, Instruction* element) {
    return mScript->AddGroup(list, element);
}

Instruction* Parser::VarDeclarationExpresion(const std::string& name, Instruction* value) {
    if (value == NULL) {
        value = NULLObject();
    }
    Instruction* obj = mScript->NewInstruction(value);
    obj->OpCode = Instructions::kNewVar;
    obj->Name = name;
    LOG(mScript->DumpInstruction(obj, ""));
    return obj;
}

Instruction* Parser::VarWriteExpresion(const std::string& name, Instruction* value) {
    Instruction* obj = mScript->NewInstruction(value);
    obj->OpCode = Instructions::kWriteVar;
    obj->Name = name;
    LOG(mScript->DumpInstruction(obj, ""));
    return obj;
}
Instruction* Parser::VarUpdateExpression(const std::string& name, Instruction* value,int opcode)
{
    Instruction*obj = mScript->NewInstruction();
    if(value != NULL){
        obj->Refs.push_back(value->key);
    }
    obj->Name = name;
    obj->OpCode = opcode;
    return obj;
}
Instruction* Parser::VarReadExpresion(const std::string& name) {
    Instruction* obj = mScript->NewInstruction();
    obj->OpCode = Instructions::kReadVar;
    obj->Name = name;
    LOG(mScript->DumpInstruction(obj, ""));
    return obj;
}

Instruction* Parser::CreateConst(const std::string& value) {
    return mScript->NewConst(value);
}
Instruction* Parser::CreateConst(long value) {
    return mScript->NewConst(value);
}
Instruction* Parser::CreateConst(double value) {
    return mScript->NewConst(value);
}

Instruction* Parser::CreateFunction(const std::string& name, Instruction* formalParameters,
                                    Instruction* body) {
    Instruction* obj = mScript->NewInstruction(formalParameters, body);
    obj->OpCode = Instructions::kNewFunction;
    obj->Name = name;
    LOG(mScript->DumpInstruction(obj, ""));
    return obj;
}

Instruction* Parser::CreateFunctionCall(const std::string& name, Instruction* actualParameters) {
    Instruction* obj = mScript->NewInstruction(actualParameters);
    obj->OpCode = Instructions::kCallFunction;
    obj->Name = name;
    LOG(mScript->DumpInstruction(obj, ""));
    return obj;
}
Instruction* Parser::CreateArithmeticOperation(Instruction* first, Instruction* second,
                                               int opcode) {
    if (opcode < Instructions::kADD || opcode > Instructions::kMAXArithmeticOP) {
        throw RuntimeException("opcode not invalid");
        return NULL;
    }
    if(second == NULL){
        second = NULLObject();
    }
    Instruction* obj = mScript->NewInstruction(first, second);
    obj->OpCode = opcode;
    LOG(mScript->DumpInstruction(obj, ""));
    return obj;
}
Instruction* Parser::CreateConditionExpresion(Instruction* condition, Instruction* action) {
    Instruction* obj = mScript->NewInstruction(condition, action);
    obj->OpCode = Instructions::kContitionExpression;
    LOG(mScript->DumpInstruction(obj, ""));
    return obj;
}

Instruction* Parser::CreateIFStatement(Instruction* one, Instruction* tow, Instruction* three) {
    if (tow == NULL) {
        tow = NULLObject();
    }
    if (three == NULL) {
        three = NULLObject();
    }
    Instruction* obj = mScript->NewInstruction(one, tow, three);
    obj->OpCode = Instructions::kIFStatement;
    LOG(mScript->DumpInstruction(obj, ""));
    return obj;
}

Instruction* Parser::CreateReturnStatement(Instruction* value) {
    Instruction* obj = mScript->NewInstruction(value);
    obj->OpCode = Instructions::kRETURNStatement;
    LOG(mScript->DumpInstruction(obj, ""));
    return obj;
}

Instruction* Parser::CreateForStatement(Instruction* init, Instruction* condition, Instruction* op,
                                        Instruction* body) {
    if (init == NULL) {
        init = NULLObject();
        condition = NULLObject();
        op = NULLObject();
    }
    Instruction* obj = mScript->NewInstruction(init, condition, op);
    obj->Refs.push_back(body->key);
    obj->OpCode = Instructions::kFORStatement;
    LOG(mScript->DumpInstruction(obj, ""));
    return obj;
}
Instruction* Parser::CreateBreakStatement() {
    Instruction* obj = mScript->NewInstruction();
    obj->OpCode = Instructions::kBREAKStatement;
    LOG(mScript->DumpInstruction(obj, ""));
    return obj;
}
Instruction* Parser::CreateContinueStatement() {
    Instruction* obj = mScript->NewInstruction();
    obj->OpCode = Instructions::kCONTINUEStatement;
    LOG(mScript->DumpInstruction(obj, ""));
    return obj;
}
} // namespace Interpreter