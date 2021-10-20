#include "parser.hpp"

#include <sstream>

#include "logger.hpp"

namespace Interpreter {

Instruction* Parser::NULLObject() {
    return mScript->NULLInstruction();
}

Instruction* Parser::CreateObjectList(Instruction* element) {
    if (mLogInstruction) {
        LOG(mScript->DumpInstruction(element, ""));
    }
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
    if (mLogInstruction) {
        LOG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}

Instruction* Parser::VarWriteExpresion(const std::string& name, Instruction* value) {
    Instruction* obj = mScript->NewInstruction(value);
    obj->OpCode = Instructions::kWriteVar;
    obj->Name = name;
    if (mLogInstruction) {
        LOG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}
Instruction* Parser::VarUpdateExpression(const std::string& name, Instruction* value, int opcode) {
    Instruction* obj = mScript->NewInstruction();
    if (value != NULL) {
        obj->Refs.push_back(value->key);
    }
    obj->Name = name;
    obj->OpCode = opcode;
    if (mLogInstruction) {
        LOG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}
Instruction* Parser::VarReadExpresion(const std::string& name) {
    Instruction* obj = mScript->NewInstruction();
    obj->OpCode = Instructions::kReadVar;
    obj->Name = name;
    if (mLogInstruction) {
        LOG(mScript->DumpInstruction(obj, ""));
    }
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
    if (mLogInstruction) {
        LOG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}

Instruction* Parser::CreateFunctionCall(const std::string& name, Instruction* actualParameters) {
    Instruction* obj = mScript->NewInstruction(actualParameters);
    obj->OpCode = Instructions::kCallFunction;
    obj->Name = name;
    if (mLogInstruction) {
        LOG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}
Instruction* Parser::CreateArithmeticOperation(Instruction* first, Instruction* second,
                                               int opcode) {
    if (opcode < Instructions::kADD || opcode > Instructions::kMAXArithmeticOP) {
        throw RuntimeException("opcode not invalid");
        return NULL;
    }
    if (second == NULL) {
        second = NULLObject();
    }
    Instruction* obj = mScript->NewInstruction(first, second);
    obj->OpCode = opcode;
    if (mLogInstruction) {
        LOG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}
Instruction* Parser::CreateConditionExpresion(Instruction* condition, Instruction* action) {
    Instruction* obj = mScript->NewInstruction(condition, action);
    obj->OpCode = Instructions::kContitionExpression;
    if (mLogInstruction) {
        LOG(mScript->DumpInstruction(obj, ""));
    }
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
    if (mLogInstruction) {
        LOG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}

Instruction* Parser::CreateReturnStatement(Instruction* value) {
    Instruction* obj = mScript->NewInstruction(value);
    obj->OpCode = Instructions::kRETURNStatement;
    if (mLogInstruction) {
        LOG(mScript->DumpInstruction(obj, ""));
    }
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
    if (mLogInstruction) {
        LOG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}
Instruction* Parser::CreateBreakStatement() {
    Instruction* obj = mScript->NewInstruction();
    obj->OpCode = Instructions::kBREAKStatement;
    if (mLogInstruction) {
        LOG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}
Instruction* Parser::CreateContinueStatement() {
    Instruction* obj = mScript->NewInstruction();
    obj->OpCode = Instructions::kCONTINUEStatement;
    if (mLogInstruction) {
        LOG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}
Instruction* Parser::CreateMapItem(Instruction* key, Instruction* value) {
    Instruction* obj = mScript->NewInstruction(key, value);
    obj->OpCode = Instructions::kGroup;
    if (mLogInstruction) {
        LOG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}
Instruction* Parser::CreateMap(Instruction* list) {
    Instruction* obj = mScript->NewInstruction(list);
    obj->OpCode = Instructions::kCreateMap;
    if (mLogInstruction) {
        LOG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}
Instruction* Parser::CreateArray(Instruction* list) {
    Instruction* obj = mScript->NewInstruction(list);
    obj->OpCode = Instructions::kCreateArray;
    if (mLogInstruction) {
        LOG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}
Instruction* Parser::VarReadAtExpression(const std::string& name, Instruction* where) {
    Instruction* obj = mScript->NewInstruction(where);
    obj->OpCode = Instructions::kReadAt;
    obj->Name = name;
    if (mLogInstruction) {
        LOG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}
Instruction* Parser::VarUpdateAtExression(const std::string& name, Instruction* where,
                                          Instruction* value) {
    Instruction* obj = mScript->NewInstruction(where, value);
    obj->OpCode = Instructions::kWriteAt;
    obj->Name = name;
    if (mLogInstruction) {
        LOG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}
Instruction* Parser::VarSlice(const std::string& name, Instruction* from, Instruction* to) {
    if (from == NULL) {
        from = NULLObject();
    }
    if (to == NULL) {
        to = NULLObject();
    }
    Instruction* obj = mScript->NewInstruction(from, to);
    obj->OpCode = Instructions::kSlice;
    obj->Name = name;
    if (mLogInstruction) {
        LOG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}

Instruction* Parser::CreateForInStatement(const std::string& key, const std::string& val,
                                          Instruction* iterobj, Instruction* body) {
    std::string name = key;
    name += ",";
    name += val;
    Instruction* obj = mScript->NewInstruction(iterobj, body);
    obj->OpCode = Instructions::kForInStatement;
    obj->Name = name;
    if (mLogInstruction) {
        LOG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}
Instruction* Parser::CreateSwitchCaseStatement(Instruction* value, Instruction* cases,
                                               Instruction* defbranch) {
    if (defbranch == NULL) {
        defbranch = NULLObject();
    }
    Instruction* obj = mScript->NewInstruction(value, cases, defbranch);
    obj->OpCode = Instructions::kSwitchCaseStatement;
    if (mLogInstruction) {
        LOG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}
} // namespace Interpreter