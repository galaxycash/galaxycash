// Copyright (c) 2017-2019 The GalaxyCash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef GALAXYCASH_EXT_SCRIPT_H
#define GALAXYCASH_EXT_SCRIPT_H

#include "hash.h"
#include "random.h"
#include "serialize.h"
#include "util.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstring>
#include <map>
#include <memory>
#include <serialize.h>
#include <streams.h>
#include <unordered_map>
#include <vector>
#include <functional>

#include <galaxyscript-parser.h>

// GalaxyCash Scripting engine

typedef std::vector<char> CVMBytecode;

typedef std::vector<char> CScriptData;
typedef std::vector<CScriptData> CScriptDataArray;



#include "compat/endian.h"

class CVMValue;
class CVMModule;
class CVMState;

typedef void (*CVMFunctionPrototype)(CVMState*);

typedef enum CVMOp {
    CVMOp_Nop = 0,
    CVMOp_Asm,
    CVMOp_Push,
    CVMOp_Pop,
    CVMOp_Key,
    CVMOp_Index,
    CVMOp_Copy,
    CVMOp_Move,
    CVMOp_Load,
    CVMOp_Call,
    CVMOp_Syscall,
    CVMOp_New,
    CVMOp_Delete,
    CVMOp_Assign,
    CVMOp_Binay,
    CVMOp_Unary,
    CVMOp_Decl,
    CVMOp_Compare,
    CVMOp_Jump,
    CVMOp_JumpEqual,
    CVMOp_JumpNotEqual,
    CVMOp_JumpZero,
    CVMOp_JumpNonZero,
    CVMOp_JumpLess,
    CVMOp_JumpGreater,
    CVMOp_JumpLessOrEqual,
    CVMOp_JumpGreaterOrEqual
} CVMOpcode;

typedef enum CVMBinaryOp {
    CVMBinaryOp_Add,
    CVMBinaryOp_Sub,
    CVMBinaryOp_Mul,
    CVMBinaryOp_Div,
    CVMBinaryOp_Mod,
    CVMBinaryOp_And,
    CVMBinaryOp_Or,
    CVMBinaryOp_Xor
} CVMBinary;


typedef std::function<void(CVMState*)> CVMFunction;

class CVMValue {
public:
    enum Kind {
        Kind_Null = 0,
        Kind_Reference,
        Kind_Pointer,
        Kind_Function,
        Kind_Symbol,
        Kind_String,
        Kind_Number,
        Kind_Array,
        Kind_Object,
        Kind_Module
    };

    enum Number {
        Number_Int8,
        Number_Boolean = Number_Int8,
        Number_Int16,
        Number_Int32,
        Number_Int64,
        Number_Float,
        Number_Double,
        Number_Bignum
    };

    
    Kind kind;
    Number number;
    std::string name;
    std::vector<char> data;
    std::vector<CVMValue*> values;
    std::unordered_map<std::string, CVMValue*> variables;
    void *pointer;
    CVMModule *space;
    CVMValue *value;
    size_t refCounter;

    static CVMValue null;


    CVMValue() : refCounter(1) {
        SetNull();
    }
    CVMValue(CVMModule *space, const std::string &name, const Kind kind, const Number number) : refCounter(1) {
        SetNull();

        this->space = space;
        this->name = name;
        this->kind = kind;
        this->number = number;
    }
    CVMValue(CVMValue *value) : refCounter(1)
    {
        Init(value);
    }
    virtual ~CVMValue() {
        for (std::vector <CVMValue*>::iterator it = values.begin(); it != values.end(); it++) {
            (*it)->Drop();
        }
        for (std::unordered_map <std::string, CVMValue*>::iterator it = variables.begin(); it != variables.end(); it++) {
            (*it).second->Drop();
        }
    }

    virtual bool Encode(std::vector<char> &data);
    virtual bool Decode(const std::vector<char> &data);

    virtual void SetNull();
    virtual bool IsNull() const;

    bool IsCopable() const { return kind != Kind_Module && kind != Kind_Function; }
    CVMValue *Grab() { refCounter++; return this;}
    CVMValue *Drop() { refCounter--; if (refCounter == 0) { delete this; return &null; } return this; }
    CVMValue *Unref() { if (kind == Kind_Reference) return (value ? value->Unref() : &null); return this; }

    bool IsReference() const { return (kind == Kind_Reference); }

    virtual void Init(CVMValue *initializer);
    virtual void Assign(CVMValue *value);

    void SetKeyValue(const std::string &name, CVMValue *value) {
        variables[name] = value;
    }

    CVMValue *GetKeyValue(const std::string &name) {
        if (!variables.count(name)) return &null;
        return variables[name];
    }


    size_t Bytes() const {
        return data.size();
    }

    size_t Bits() const {
        return Bytes() * 8;
    }

    bool AsBoolean() const { 
        if (kind == Kind_Number) {
            switch (number)
            {
                case Number_Int8: return data[0] > 0;
                case Number_Int16: return *((int16_t *)data.data()) > 0;
                case Number_Int32: return *((int32_t *)data.data()) > 0;
                case Number_Int64: return *((int64_t *)data.data()) > 0;
                case Number_Float: return *((float *)data.data()) > 0.0f;
                case Number_Double: return *((double *)data.data()) > 0.0;
                default: return false;
            }
        }
        else if (kind == Kind_Pointer)
            return ((void *) data.data()) != nullptr;

        return false;
    }

    int8_t AsInt8() const { 
        if (kind == Kind_Number) {
            switch (number)
            {
                case Number_Int8: return data[0];
                default: return 0;
            }
        }
        return 0;
    }   
    
    int16_t AsInt16() const { 
        if (kind == Kind_Number) {
            switch (number)
            {
                case Number_Int8: return data[0];
                case Number_Int16: return *((int16_t*)data.data());
                default: return 0;
            }
        }
        return 0;
    }  
    
    int32_t AsInt32() const { 
        if (kind == Kind_Number) {
            switch (number)
            {
                case Number_Int8: return data[0];
                case Number_Int16: return *((int16_t*)data.data());
                case Number_Int32: return *((int32_t*)data.data());
                case Number_Float: return *((float*)data.data());
                default: return 0;
            }
        }
        return 0;
    } 
    
    int64_t AsInt64() const { 
        if (kind == Kind_Number) {
            switch (number)
            {
                case Number_Int8: return data[0];
                case Number_Int16: return *((int16_t*)data.data());
                case Number_Int32: return *((int32_t*)data.data());
                case Number_Int64: return *((int64_t*)data.data());
                case Number_Float: return *((float*)data.data());
                case Number_Double: return *((double*)data.data());
                default: return 0;
            }
        }
        return 0;
    }    
    
    float AsFloat() const { 
        if (kind == Kind_Number) {
            switch (number)
            {
                case Number_Int8: return data[0];
                case Number_Int16: return *((int16_t*)data.data());
                case Number_Int32: return *((int32_t*)data.data());
                case Number_Int64: return *((int64_t*)data.data());
                case Number_Float: return *((float*)data.data());
                case Number_Double: return *((double*)data.data());
                default: return 0.0f;
            }
        }
        return 0.0f;
    }  
     
    double AsDouble() const { 
        if (kind == Kind_Number) {
            switch (number)
            {
                case Number_Int8: return data[0];
                case Number_Int16: return *((int16_t*)data.data());
                case Number_Int32: return *((int32_t*)data.data());
                case Number_Int64: return *((int64_t*)data.data());
                case Number_Float: return *((float*)data.data());
                case Number_Double: return *((double*)data.data());
                default: return 0.0;
            }
        }
        return 0.0;
    } 
         
    uint256 AsBignum() const { 
        uint256 ret;
        if (kind == Kind_Number) {
            switch (number)
            {
                case Number_Int8: ret.SetUInt8(data[0]); break;
                case Number_Int16: ret.SetUInt16(*((int16_t*)data.data())); break;
                case Number_Int32: ret.SetUInt32(*((int32_t*)data.data())); break;
                case Number_Int64: ret.SetUInt64(*((int64_t*)data.data())); break;
                case Number_Float: ret.SetUInt32((uint32_t) *((float*)data.data())); break;
                case Number_Double: ret.SetUInt64((uint64_t) *((double*)data.data())); break;
                default: ret.SetNull();
            }
        }
        return ret;
    }  

    std::string AsString() const {
        std::string ret;
        if (kind == Kind_String || kind == Kind_Symbol) {
            return std::string(data.begin(), data.end());
        }
        else if (kind == Kind_Number) {
            std::ostringstream ss(ret);
            ss.imbue(std::locale::classic());

            switch (number)
            {
                case Number_Int8: ss << data[0]; break;
                case Number_Int16: ss << *((int16_t*)data.data()); break;
                case Number_Int32: ss << *((int32_t*)data.data()); break;
                case Number_Int64: ss << *((int64_t*)data.data()); break;
                case Number_Float: ss << *((float*)data.data()); break;
                case Number_Double: ss << *((double*)data.data()); break;
                default: ss << 0;
            }
        }
        else if (kind == Kind_Pointer) {
            std::ostringstream ss(ret);
            ss.imbue(std::locale::classic());
            ss << (void *) data.data();
        }
        return ret;
    }

    size_t Length() const {
        if (kind == Kind_String || kind == Kind_Symbol) return data.size() - 1;
        else  return Bytes();
    }             
};

CVMValue CVMValue::null;


class CVMModule : public CVMValue {
public:
    static std::unordered_map<std::string, CVMModule*> modules;

    CVMModule();
    ~CVMModule();

    static CVMModule *CompileModuleFromSource(const std::string &name, const std::string &source);
    static CVMModule *CompileModuleFromFile(const std::string &name, const std::string &file);
    static CVMModule *LoadModule(const std::string &name);
    static bool       SaveModule(const std::string &name, CVMModule *module);
};

class CVMState {
public:
    enum {
        MAX_STACK = 64,
        MAX_SCOPE = 32
    };


    struct Frame {
        size_t pc;

        CVMValue *ths;
        CVMValue *caller;
        CVMValue *callable;

        Frame() : pc(0), ths(nullptr), caller(nullptr), callable(nullptr) {}
        Frame(Frame *frame) : pc(frame->pc), ths(frame->ths), caller(frame->caller), callable(frame->callable) {}
        Frame(const size_t pc, CVMValue *ths, CVMValue *caller, CVMValue *callable) : pc(pc), ths(ths), caller(caller), callable(callable) {}

        void Flush() {
            pc = 0;
            ths = caller = callable = nullptr;
        }
    };

    size_t                 bp, dp;
    std::stack<CVMValue*>  stack;
    std::stack<Frame>      frame;


    CVMState();
    CVMState(CVMState *state);
    virtual ~CVMState();

    inline void Flush() {
        if (frame.size()) frame.top().Flush();
        bp = dp = 0;
        while (stack.size()) stack.pop();
    }

    inline void Push(CVMValue *value) {
        assert(stack.size() < MAX_STACK);
        stack.push(value);
    }
    inline CVMValue *Top() {
        assert(stack.size() > 0);
        return stack.top();
    }
    inline CVMValue *Pop() {
        assert(stack.size() > 0);
        CVMValue *value = stack.top(); stack.pop();
        return value;
    }
};

bool VMCall(CVMState *state, CVMValue *callable);
bool VMCall(CVMValue *callable);
bool VMRunSource(const std::string &source);
bool VMRunFile(const std::string &file);

#endif