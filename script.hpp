#pragma once
#include <assert.h>

#include <list>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include "exception.hpp"
#include "value.hpp"

namespace Interpreter {
namespace Instructions {
const int kNop = 0;
#define CODE_BASE (0)
const int kConst = CODE_BASE + 1;
const int kNewVar = CODE_BASE + 2;
const int kReadVar = CODE_BASE + 3;
const int kWriteVar = CODE_BASE + 4;
const int kNewFunction = CODE_BASE + 5;
const int kCallFunction = CODE_BASE + 6;
const int kReadAt = CODE_BASE + 7;
const int kWriteAt = CODE_BASE + 8;
const int kGroup = CODE_BASE + 9;
const int kContitionExpression = CODE_BASE + 10;
const int kIFStatement = CODE_BASE + 11;
const int kRETURNStatement = CODE_BASE + 12;
const int kFORStatement = CODE_BASE + 13;
const int kCONTINUEStatement = CODE_BASE + 14;
const int kBREAKStatement = CODE_BASE + 15;
const int kCreateMap = CODE_BASE + 16;
const int kCreateArray = CODE_BASE + 17;
const int kSlice = CODE_BASE + 18;
const int kForInStatement = CODE_BASE + 19;
const int kSwitchCaseStatement = CODE_BASE + 20;

const int kArithmeticOP = 0x100;
const int kADD = kArithmeticOP + 1;
const int kSUB = kArithmeticOP + 2;
const int kMUL = kArithmeticOP + 3;
const int kDIV = kArithmeticOP + 4;
const int kMOD = kArithmeticOP + 5;
const int kGT = kArithmeticOP + 6;
const int kGE = kArithmeticOP + 7;
const int kLT = kArithmeticOP + 8;
const int kLE = kArithmeticOP + 9;
const int kEQ = kArithmeticOP + 10;
const int kNE = kArithmeticOP + 11;
const int kNOT = kArithmeticOP + 12;
const int kMAXArithmeticOP = kArithmeticOP + 12;

const int kADDWrite = 0x201;
const int kSUBWrite = 0x202;
const int kMULWrite = 0x203;
const int kDIVWrite = 0x204;
const int kINCWrite = 0x205;
const int kDECWrite = 0x206;
}; // namespace Instructions

class Instruction {
public:
    int OpCode;
    long key;
    std::string Name;
    std::vector<long> Refs;

public:
    Instruction() : OpCode(Instructions::kNop),key(0) {}
    Instruction(Instruction* one):key(0) { Refs.push_back(one->key); }
    Instruction(Instruction* one, Instruction* tow):key(0) {
        Refs.push_back(one->key);
        Refs.push_back(tow->key);
    }
    Instruction(Instruction* one, Instruction* tow, Instruction* three):key(0) {
        Refs.push_back(one->key);
        Refs.push_back(tow->key);
        Refs.push_back(three->key);
    }
    Instruction(Instruction* one, Instruction* tow, Instruction* three,Instruction*four):key(0) {
        Refs.push_back(one->key);
        Refs.push_back(tow->key);
        Refs.push_back(three->key);
        Refs.push_back(four->key);
    }
    bool IsNULL() const { return OpCode == Instructions::kNop; }
    std::string ToString() const {
        if (OpCode >= Instructions::kADD && OpCode <= Instructions::kMAXArithmeticOP) {
            return "Arithmetic Operation";
        }
        if (OpCode >= Instructions::kADDWrite && OpCode <= Instructions::kDECWrite) {
            return "Update Var:" + Name;
        }
        switch (OpCode) {
        case Instructions::kNop:
            return "Nop";
        case Instructions::kConst:
            return "Create Const:";
        case Instructions::kNewVar:
            return "Create Var:" + Name;
        case Instructions::kReadVar:
            return "Read Var:" + Name;
        case Instructions::kWriteVar:
            return "Write Var:" + Name;
        case Instructions::kNewFunction:
            return "Create Function:" + Name;
        case Instructions::kCallFunction:
            return "Call Function:" + Name;
        case Instructions::kGroup:
            return "Instruction List";
        case Instructions::kContitionExpression:
            return "ContitionExpression";
        case Instructions::kIFStatement:
            return "if Statement";
        case Instructions::kRETURNStatement:
            return "return Statement";
        case Instructions::kFORStatement:
            return "for Statement";
        case Instructions::kBREAKStatement:
            return "break Statement";
        case Instructions::kCONTINUEStatement:
            return "continue Statement";
        case Instructions::kReadAt:
            return "read at index";
        case Instructions::kWriteAt:
            return "write at index";
        case Instructions::kCreateMap:
            return "Create Map";
        case Instructions::kCreateArray:
            return "Create Array";
        case Instructions::kSlice:
            return "Slice Array";
        case Instructions::kForInStatement:
            return "for in statement";
        case Instructions::kSwitchCaseStatement:
            return "switch -- case statement";

        default:
            return "Unknown Op";
        }
    }
};

class Script : public CRefCountedThreadSafe<Script> {
public:
    Instruction* EntryPoint;

    explicit Script() {
        EntryPoint = NULL;
        mInstructionKey = 1;
        mConstKey = 1;
        mInstructionTable[0] = new Instruction();
        mInstructionBase = 0;
        mConstBase = 0;
    }
    ~Script() {
        for (std::map<long, Instruction*>::iterator iter = mInstructionTable.begin();
             iter != mInstructionTable.end(); iter++) {
            delete (iter->second);
        }
        mInstructionTable.clear();
        mConstTable.clear();
    }

protected:
    long mInstructionKey;
    long mConstKey;
    long mInstructionBase;
    long mConstBase;
    std::map<long, Instruction*> mInstructionTable;
    std::map<long, Value> mConstTable;

public:
    void RelocateInstruction(long newbase, long newConstbase) {
        if(mInstructionBase != 0 || mConstBase != 0){
            throw RuntimeException("script can only Relocate once");
        }
        for (std::map<long, Instruction*>::iterator iter = mInstructionTable.begin();
             iter != mInstructionTable.end(); iter++) {
             Instruction* ptr = iter->second;
             if(ptr->OpCode == Instructions::kConst){
                 assert(ptr->Refs[0] < mConstKey);
                 ptr->Refs[0] = ptr->Refs[0]+newConstbase;
             }else{
                 for(size_t i =0; i< ptr->Refs.size();i++){
                     assert(ptr->Refs[i] < mInstructionKey);
                     ptr->Refs[i] = ptr->Refs[i]+newbase;
                 }
             }
             ptr->key += newbase;
        }
        mInstructionBase = newbase;
        mConstBase = newConstbase;
    }

    bool IsContainInstruction(long key) {
        return key >= mInstructionBase && key < GetNextInstructionKey();
    }
    bool IsContainConst(long key) { return key >= mConstBase && key < GetNextConstKey(); }
    long GetNextInstructionKey() { return mInstructionBase + mInstructionKey; }
    long GetNextConstKey() { return mConstBase + mConstKey; }

public:
    Instruction* NewGroup(Instruction* element) {
        Instruction* ins = new Instruction(element);
        ins->OpCode = Instructions::kGroup;
        ins->key = mInstructionKey;
        mInstructionKey++;
        mInstructionTable[ins->key] = ins;
        return ins;
    }
    Instruction* AddToGroup(Instruction* group, Instruction* element) {
        assert(group->OpCode == Instructions::kGroup);
        group->Refs.push_back(element->key);
        return group;
    }
    Instruction* NULLInstruction() { return mInstructionTable[0]; }
    Instruction* NewInstruction() {
        Instruction* ins = new Instruction();
        ins->key = mInstructionKey;
        mInstructionKey++;
        mInstructionTable[ins->key] = ins;
        return ins;
    }
    Instruction* NewInstruction(Instruction* one) {
        Instruction* ins = new Instruction(one);
        ins->key = mInstructionKey;
        mInstructionKey++;
        mInstructionTable[ins->key] = ins;
        return ins;
    }
    Instruction* NewInstruction(Instruction* one, Instruction* tow) {
        Instruction* ins = new Instruction(one, tow);
        ins->key = mInstructionKey;
        mInstructionKey++;
        mInstructionTable[ins->key] = ins;
        return ins;
    }
    Instruction* NewInstruction(Instruction* one, Instruction* tow, Instruction* three) {
        Instruction* ins = new Instruction(one, tow, three);
        ins->key = mInstructionKey;
        mInstructionKey++;
        mInstructionTable[ins->key] = ins;
        return ins;
    }
    Instruction* NewInstruction(Instruction* one, Instruction* tow, Instruction* three,Instruction*four) {
        Instruction* ins = new Instruction(one, tow, three,four);
        ins->key = mInstructionKey;
        mInstructionKey++;
        mInstructionTable[ins->key] = ins;
        return ins;
    }
    //TODO use const value pool
    Instruction* NewConst(const std::string& value) {
        Value val = Value(value);
        long key = mConstKey;
        mConstKey++;
        mConstTable[key] = val;
        Instruction* ins = NewInstruction();
        ins->OpCode = Instructions::kConst;
        ins->Refs.push_back(key);
        return ins;
    }
    Instruction* NewConst(long value) {
        Value val = Value(value);
        long key = mConstKey;
        mConstKey++;
        mConstTable[key] = val;
        Instruction* ins = NewInstruction();
        ins->OpCode = Instructions::kConst;
        ins->Refs.push_back(key);
        return ins;
    }
    Instruction* NewConst(double value) {
        Value val = Value(value);
        long key = mConstKey;
        mConstKey++;
        mConstTable[key] = val;
        Instruction* ins = NewInstruction();
        ins->OpCode = Instructions::kConst;
        ins->Refs.push_back(key);
        return ins;
    }

    Value GetConstValue(long key) { return mConstTable[key-mConstBase]; }

    const Instruction* GetInstruction(long key) { return mInstructionTable[key-mInstructionBase]; }

    std::vector<const Instruction*> GetInstructions(std::vector<long> keys) {
        std::vector<const Instruction*> result;
        for (std::vector<long>::iterator iter = keys.begin(); iter != keys.end(); iter++) {
            result.push_back(mInstructionTable[*iter-mInstructionBase]);
        }
        return result;
    }

    std::string DumpInstruction(const Instruction* ins, std::string prefix) {
        std::stringstream stream;
        stream << prefix;
        if (ins->OpCode == Instructions::kConst) {
            stream << ins->key << " " << ins->ToString() << GetConstValue(ins->Refs[0]).ToString()
                   << std::endl;
            return stream.str();
        }
        stream << ins->key << " " << ins->ToString() << std::endl;
        if (ins->Refs.size() > 0) {
            std::vector<const Instruction*> subs = GetInstructions(ins->Refs);
            std::vector<const Instruction*>::iterator iter = subs.begin();
            while (iter != subs.end()) {
                stream << DumpInstruction(*iter, prefix + "\t");
                iter++;
            }
        }
        return stream.str();
    }
};
} // namespace Interpreter