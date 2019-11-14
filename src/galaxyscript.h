// Copyright (c) 2017-2019 The GalaxyCash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef GALAXYCASH_EXT_SCRIPT_H
#define GALAXYCASH_EXT_SCRIPT_H

#include "hash.h"
#include "random.h"
#include "serialize.h"
#include <util.h>
#include <utilstrencodings.h>
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

enum {
    AddressSpace_Global = 0,
    AddressSpace_This,
    AddressSpace_Super,
    AddressSpace_Frame,
    AddressSpace_Block
};

typedef enum CVMOp {
    CVMOp_Nop = 0,
    CVMOp_Push,
    CVMOp_Pop,
    CVMOp_Key,
    CVMOp_Index,
    CVMOp_Decl,
    CVMOp_Make,
    CVMOp_Assign,
    CVMOp_Copy,
    CVMOp_Load,
    CVMOp_Call,
    CVMOp_Syscall,
    CVMOp_New,
    CVMOp_Delete,
    CVMOp_Binay,
    CVMOp_Unary,
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
        Kind_Variable,
        Kind_Reference,
        Kind_Pointer,
        Kind_Function,
        Kind_String,
        Kind_Number,
        Kind_Array,
        Kind_Object,
        Kind_Module
    };
    enum Flag {
        Flag_None = 0,
        Flag_Integer = BIT(0),
        Flag_Float = BIT(1),
        Flag_Double = BIT(2),
        Flag_Boolean = BIT(3),
        Flag_Bignum = BIT(4),
        Flag_Unsigned = BIT(5),
        Flag_Constant = BIT(6),
        Flag_Import = BIT(7),
        Flag_Export = BIT(8),
        Flag_Native = BIT(9),
        Flag_Buildin = BIT(10),
        Flag_Property = BIT(11),
        Flag_Async = BIT(12),
        Flag_Void = BIT(13),
        Flag_Symbol = BIT(14),
        Flag_Callable = BIT(15)
    };
    
    Kind kind;
    std::string name;
    std::vector<char> data;
    std::vector<CVMValue*> values;
    std::unordered_map<std::string, CVMValue*> variables;
    CVMValue *module;
    CVMValue *ns;
    CVMValue *value;
    CVMValue *prototype, *arguments, *constants;
    size_t refCounter;
    uint8_t bits;
    uint64_t flags;

    CVMValue() : refCounter(1) {
        SetNull();
    }
    CVMValue(CVMValue *ns, const std::string &name, const Kind kind, const uint8_t bits = 64, const uint64_t flags = 0, const std::vector<char> &data = std::vector<char>()) : refCounter(1) {
        SetNull();

        this->ns = ns;
        this->name = name;
        this->kind = kind;
        this->bits = bits;
        this->flags = flags;
        this->data = data;

        /*if (!IsReference() && !IsNumber() && !IsString() && !IsSymbol() && !IsPointer() && !IsVariable()) {
            if (IsObject()) {
                constants = AddArray("__gs_constants__");
                AddVariable("__gs_this__",  this);
                AddReference("this",  "__gs_this__");
                AddVariable("__gs_super__",  this);
                AddReference("super",  "__gs_super__");

                prototype = AddVariable("__gs_prototype__", nullptr);
                AddReference("prototype",  "__gs_prototype__");
            }
            if (IsFunction()) {
                AddVariable("__gs_this__",  ns);

                arguments = AddArray("__gs_arguments__");
                AddReference("arguments", "__gs_arguments__");

                AddVariable("__gs_return__");
            }
        }*/
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
        if (prototype) prototype->Drop();
        if (arguments) arguments->Drop();
        if (constants) constants->Drop();
        if (value) value->Drop();
        if (ns) ns->Drop();
        if (module) module->Drop();
    }

    virtual bool Encode(std::vector<char> &data);
    virtual bool Decode(const std::vector<char> &data);

    virtual void SetNull();

    inline bool IsNull() const {
        if (kind == Kind_Null) return true;
        else if (kind == Kind_Variable) return (value == nullptr);
        else if (kind == Kind_Reference) return name.empty();
        else if (kind == Kind_Pointer) return data.empty();
        return false;
    }
    inline bool IsPointer() const { return (kind == Kind_String); }
    inline bool IsString() const { return (kind == Kind_String); }
    inline bool IsSymbol() const { return (kind == Kind_String) && (flags & Flag_Symbol); }
    inline bool IsNumber() const { return (kind == Kind_Number); }
    inline bool IsReference() const { return (kind == Kind_Reference); }
    inline bool IsVariable() const { return (kind == Kind_Variable); }
    inline bool IsPrimitive() const { return IsString() || IsNumber() || IsPointer() || IsReference() || IsVariable(); }
    inline bool IsObject() const { return (kind == Kind_Object); }
    inline bool IsArray() const { return (kind == Kind_Array); }
    inline bool IsFunction() const { return (kind == Kind_Function); }
    inline bool IsCallable() const { return (flags & Flag_Callable); }
    

    inline std::string GetName() const { return name; }
    inline std::string GetFullname() const { return (ns) ? (ns->GetFullname().empty() ? GetName() : ns->GetFullname() + (!GetName().empty() ? "." + GetName() : GetName())) : GetName(); }

    inline bool IsCopable() const { return kind != Kind_Module; }
    inline CVMValue *Grab() { refCounter++; return this;}
    inline CVMValue *Drop() { refCounter--; if (refCounter == 0) { delete this; return nullptr; } return this; }

    virtual void Init(CVMValue *initializer);
    virtual void Assign(CVMValue *value);

    inline CVMValue *SetKeyValue(const std::string &name, CVMValue *value) {
        if (variables.count(name) && variables[name] != nullptr) { if (value) {variables[name]->Assign(value);} else {variables[name]->SetNull();} return variables[name]; }
        variables[name] = new CVMValue(this, name, Kind_Variable, 0, Flag_Property);
        if (value) variables[name]->Assign(value);
        return variables[name];
    }

    inline CVMValue *GetKeyValue(const std::string &name) {
        if (!variables.count(name))
            return SetKeyValue(name, nullptr);
        return variables[name];
    }


    inline size_t Bytes() const {
        return data.size();
    }

    inline size_t Bits() const {
        return Bytes() * 8;
    }


    inline std::string AsVariableName() {
        if (kind == Kind_Reference) return GetFullname();
        return AsSource();
    }

    inline bool AsBoolean() const { 
        if (kind == Kind_Number) {
            switch (bits)
            {
                case 8: { 
                    if (flags & Flag_Integer) {
                        if (flags & Flag_Unsigned)
                            return *((uint8_t*) data.data()) > 0;
                        else
                            return *((int8_t*) data.data()) > 0;
                    } 
                    else if (flags & Flag_Boolean) return *((uint8_t*) data.data()) > 0;
                    else return false;
                } 
                break;
                case 16: { 
                    if (flags & Flag_Integer) {
                        if (flags & Flag_Unsigned)
                            return *((uint16_t*) data.data()) > 0;
                        else
                            return *((int16_t*) data.data()) > 0;
                    } 
                    else return false;
                } 
                break;
                case 32: { 
                    if (flags & Flag_Integer) {
                        if (flags & Flag_Unsigned)
                            return *((uint32_t*) data.data()) > 0;
                        else
                            return *((int32_t*) data.data()) > 0;
                    } 
                    else return false;
                } 
                break;
                case 64: { 
                    if (flags & Flag_Integer) {
                        if (flags & Flag_Unsigned)
                            return *((uint64_t*) data.data()) > 0;
                        else
                            return *((int64_t*) data.data()) > 0;
                    } 
                    else return false;
                } 
                break;
                default: return false;
            }
        }
        else if (kind == Kind_Pointer)
            return ((void *) data.data()) != nullptr;
        else if (kind == Kind_Reference)
            return (value != nullptr);
        return false;
    }

    inline int8_t AsInt8() const { 
        if (kind == Kind_Number) {
            switch (bits)
            {
                case 8: { 
                    if (flags & Flag_Integer) {
                        if (flags & Flag_Unsigned)
                            return (int8_t) *((uint8_t*) data.data());
                        else
                            return *((int8_t*) data.data());
                    } 
                    else if (flags & Flag_Boolean) return *((uint8_t*) data.data()) > 0 ? 1 : 0;
                    else return 0;
                } 
                break;
                case 16: { 
                    if (flags & Flag_Integer) {
                        if (flags & Flag_Unsigned)
                            return (int8_t) *((uint16_t*) data.data());
                        else
                            return (int8_t) *((int16_t*) data.data());
                    } 
                    else return 0;
                } 
                break;
                case 32: { 
                    if (flags & Flag_Integer) {
                        if (flags & Flag_Unsigned)
                            return (int8_t) *((uint32_t*) data.data());
                        else
                            return (int8_t) *((int32_t*) data.data());
                    } 
                    else return 0;
                } 
                break;
                case 64: { 
                    if (flags & Flag_Integer) {
                        if (flags & Flag_Unsigned)
                            return (int8_t) *((uint64_t*) data.data());
                        else
                            return (int8_t) *((int64_t*) data.data());
                    } 
                    else return 0;
                } 
                break;
                default: return 0;
            }
        }
        return 0;
    }   
    
    inline int16_t AsInt16() const { 
        if (kind == Kind_Number) {
            switch (bits)
            {
                case 8: { 
                    if (flags & Flag_Integer) {
                        if (flags & Flag_Unsigned)
                            return (int16_t) *((uint8_t*) data.data());
                        else
                            return (int16_t) *((int8_t*) data.data());
                    } 
                    else if (flags & Flag_Boolean) return *((uint8_t*) data.data()) > 0 ? 1 : 0;
                    else return 0;
                } 
                break;
                case 16: { 
                    if (flags & Flag_Integer) {
                        if (flags & Flag_Unsigned)
                            return (int16_t) *((uint16_t*) data.data());
                        else
                            return *((int16_t*) data.data());
                    } 
                    else if (flags & Flag_Float) {
                        return (int16_t) *((float*) data.data());
                    }
                    else return 0;
                } 
                break;
                case 32: { 
                    if (flags & Flag_Integer) {
                        if (flags & Flag_Unsigned)
                            return (int16_t) *((uint32_t*) data.data());
                        else
                            return (int16_t) *((int32_t*) data.data());
                    } 
                    else if (flags & Flag_Float) {
                        return (int16_t) *((float*) data.data());
                    }
                    else return 0;
                } 
                break;
                case 64: { 
                    if (flags & Flag_Integer) {
                        if (flags & Flag_Unsigned)
                            return (int16_t) *((uint64_t*) data.data());
                        else
                            return (int16_t) *((int64_t*) data.data());
                    } 
                    else if (flags & Flag_Double) {
                        return (int16_t) *((double*) data.data());
                    }
                    else return 0;
                } 
                break;
                default: return 0;
            }
        }
        return 0;
    }  
    
    inline int32_t AsInt32() const { 

        if (kind == Kind_Number) {
            switch (bits)
            {
                case 8: { 
                    if (flags & Flag_Integer) {
                        if (flags & Flag_Unsigned)
                            return (int32_t) *((uint8_t*) data.data());
                        else
                            return (int32_t) *((int8_t*) data.data());
                    } 
                    else if (flags & Flag_Boolean) return *((uint8_t*) data.data()) > 0 ? 1 : 0;
                    else return 0;
                } 
                break;
                case 16: { 
                    if (flags & Flag_Integer) {
                        if (flags & Flag_Unsigned)
                            return (int32_t) *((uint16_t*) data.data());
                        else
                            return (int32_t) *((int16_t*) data.data());
                    } 
                    else if (flags & Flag_Float) {
                        return (int32_t) *((float*) data.data());
                    }
                    else return 0;
                } 
                break;
                case 32: { 
                    if (flags & Flag_Integer) {
                        if (flags & Flag_Unsigned)
                            return (int32_t) *((uint32_t*) data.data());
                        else
                            return *((int32_t*) data.data());
                    } 
                    else if (flags & Flag_Float) {
                        return (int32_t) *((float*) data.data());
                    }
                    else return 0;
                } 
                break;
                case 64: { 
                    if (flags & Flag_Integer) {
                        if (flags & Flag_Unsigned)
                            return (int32_t) *((uint64_t*) data.data());
                        else
                            return (int32_t) *((int64_t*) data.data());
                    } 
                    else if (flags & Flag_Double) {
                        return (int32_t) *((double*) data.data());
                    }
                    else return 0;
                } 
                break;
                default: return 0;
            }
        }
        return 0;
    } 
        
    inline int32_t AsUInt32() const { 
        if (kind == Kind_Number) {
            switch (bits)
            {
                case 8: { 
                    if (flags & Flag_Integer) {
                        if (flags & Flag_Unsigned)
                            return (uint32_t) *((uint8_t*) data.data());
                        else
                            return (uint32_t) *((int8_t*) data.data());
                    } 
                    else if (flags & Flag_Boolean) return *((uint8_t*) data.data()) > 0 ? 1 : 0;
                    else return 0;
                } 
                break;
                case 16: { 
                    if (flags & Flag_Integer) {
                        if (flags & Flag_Unsigned)
                            return (uint32_t) *((uint16_t*) data.data());
                        else
                            return (uint32_t) *((int16_t*) data.data());
                    } 
                    else if (flags & Flag_Float) {
                        return (uint32_t) *((float*) data.data());
                    }
                    else return 0;
                } 
                break;
                case 32: { 
                    if (flags & Flag_Integer) {
                        if (flags & Flag_Unsigned)
                            return *((uint32_t*) data.data());
                        else
                            return (uint32_t) *((int32_t*) data.data());
                    } 
                    else if (flags & Flag_Float) {
                        return (uint32_t) *((float*) data.data());
                    }
                    else return 0;
                } 
                break;
                case 64: { 
                    if (flags & Flag_Integer) {
                        if (flags & Flag_Unsigned)
                            return (uint32_t) *((uint64_t*) data.data());
                        else
                            return (uint32_t) *((int64_t*) data.data());
                    } 
                    else if (flags & Flag_Double) {
                        return (uint32_t) *((double*) data.data());
                    }
                    else return 0;
                } 
                break;
                default: return 0;
            }
        }
        return 0;
    } 

    inline int64_t AsInt64() const { 
        if (kind == Kind_Number) {
            switch (bits)
            {
                case 8: { 
                    if (flags & Flag_Integer) {
                        if (flags & Flag_Unsigned)
                            return (int64_t) *((uint8_t*) data.data());
                        else
                            return (int64_t) *((int8_t*) data.data());
                    } 
                    else if (flags & Flag_Boolean) return *((uint8_t*) data.data()) > 0 ? 1 : 0;
                    else return 0;
                } 
                break;
                case 16: { 
                    if (flags & Flag_Integer) {
                        if (flags & Flag_Unsigned)
                            return (int64_t) *((uint16_t*) data.data());
                        else
                            return (int64_t) *((int16_t*) data.data());
                    }
                    else if (flags & Flag_Float) {
                        return (int64_t) *((float*) data.data());
                    }
                    else return 0;
                } 
                break;
                case 32: { 
                    if (flags & Flag_Integer) {
                        if (flags & Flag_Unsigned)
                            return (int64_t) *((uint32_t*) data.data());
                        else
                            return (int64_t) *((int32_t*) data.data());
                    } 
                    else if (flags & Flag_Float) {
                        return (int64_t) *((float*) data.data());
                    }
                    else return 0;
                } 
                break;
                case 64: { 
                    if (flags & Flag_Integer) {
                        if (flags & Flag_Unsigned)
                            return (int64_t) *((uint64_t*) data.data());
                        else
                            return *((int64_t*) data.data());
                    } 
                    else if (flags & Flag_Double) {
                        return (int64_t) *((double*) data.data());
                    }
                    else return 0;
                } 
                break;
                default: return 0;
            }
        }
        return 0;
    }    
        
    inline uint64_t AsUInt64() const { 
        if (kind == Kind_Number) {
            switch (bits)
            {
                case 8: { 
                    if (flags & Flag_Integer) {
                        if (flags & Flag_Unsigned)
                            return (uint64_t) *((uint8_t*) data.data());
                        else
                            return (uint64_t) *((int8_t*) data.data());
                    } 
                    else if (flags & Flag_Boolean) return *((uint8_t*) data.data()) > 0 ? 1 : 0;
                    else return 0;
                } 
                break;
                case 16: { 
                    if (flags & Flag_Integer) {
                        if (flags & Flag_Unsigned)
                            return (uint64_t) *((uint16_t*) data.data());
                        else
                            return (uint64_t) *((int16_t*) data.data());
                    }
                    else if (flags & Flag_Float) {
                        return (uint64_t) *((float*) data.data());
                    }
                    else return 0;
                } 
                break;
                case 32: { 
                    if (flags & Flag_Integer) {
                        if (flags & Flag_Unsigned)
                            return (uint64_t) *((uint32_t*) data.data());
                        else
                            return (uint64_t) *((int32_t*) data.data());
                    } 
                    else if (flags & Flag_Float) {
                        return (uint64_t) *((float*) data.data());
                    }
                    else return 0;
                } 
                break;
                case 64: { 
                    if (flags & Flag_Integer) {
                        if (flags & Flag_Unsigned)
                            return *((uint64_t*) data.data());
                        else
                            return (uint64_t) *((int64_t*) data.data());
                    } 
                    else if (flags & Flag_Double) {
                        return (uint64_t) *((double*) data.data());
                    }
                    else return 0;
                } 
                break;
                default: return 0;
            }
        }
        return 0;
    }    

    inline float AsFloat() const { 
        if (kind == Kind_Number) {
            switch (bits)
            {
                case 8: { 
                    if (flags & Flag_Integer) {
                        if (flags & Flag_Unsigned)
                            return (float) *((uint8_t*) data.data());
                        else
                            return (float) *((int8_t*) data.data());
                    } 
                    else if (flags & Flag_Boolean) return *((uint8_t*) data.data()) > 0 ? 1.0f : 0.0f;
                    else return 0.0f;
                } 
                break;
                case 16: { 
                    if (flags & Flag_Integer) {
                        if (flags & Flag_Unsigned)
                            return (float) *((uint16_t*) data.data());
                        else
                            return (float) *((int16_t*) data.data());
                    } 
                    else if (flags & Flag_Float) {
                        return *((float*) data.data());
                    }
                    else return 0.0f;
                } 
                break;
                case 32: { 
                    if (flags & Flag_Integer) {
                        if (flags & Flag_Unsigned)
                            return (float) *((uint32_t*) data.data());
                        else
                            return (float) *((int32_t*) data.data());
                    } 
                    else if (flags & Flag_Float) {
                        return *((float*) data.data());
                    }
                    else return 0.0f;
                } 
                break;
                case 64: { 
                    if (flags & Flag_Integer) {
                        if (flags & Flag_Unsigned)
                            return (float) *((uint64_t*) data.data());
                        else
                            return (float) *((int64_t*) data.data());
                    } 
                    else if (flags & Flag_Double) {
                        return (float) *((double*) data.data());
                    }
                    else return 0.0f;
                } 
                break;
                default: return 0.0f;
            }
        }
        return 0.0f;
    }  
     
    
    inline float AsDouble() const { 
        if (kind == Kind_Number) {
            switch (bits)
            {
                case 8: { 
                    if (flags & Flag_Integer) {
                        if (flags & Flag_Unsigned)
                            return (double) *((uint8_t*) data.data());
                        else
                            return (double) *((int8_t*) data.data());
                    } 
                    else if (flags & Flag_Boolean) return *((uint8_t*) data.data()) > 0 ? 1.0 : 0.0;
                    else return 0.0;
                } 
                break;
                case 16: { 
                    if (flags & Flag_Integer) {
                        if (flags & Flag_Unsigned)
                            return (double) *((uint16_t*) data.data());
                        else
                            return (double) *((int16_t*) data.data());
                    } 
                    else if (flags & Flag_Float) {
                        return (double) *((float*) data.data());
                    }
                    else return 0.0;
                } 
                break;
                case 32: { 
                    if (flags & Flag_Integer) {
                        if (flags & Flag_Unsigned)
                            return (double) *((uint32_t*) data.data());
                        else
                            return (double) *((int32_t*) data.data());
                    } 
                    else if (flags & Flag_Float) {
                        return (double) *((float*) data.data());
                    }
                    else return 0.0;
                } 
                break;
                case 64: { 
                    if (flags & Flag_Integer) {
                        if (flags & Flag_Unsigned)
                            return (double) *((uint64_t*) data.data());
                        else
                            return (double) *((int64_t*) data.data());
                    } 
                    else if (flags & Flag_Double) {
                        return *((double*) data.data());
                    }
                    else return 0.0;
                } 
                break;
                default: return 0.0;
            }
        }
        return 0.0;
    }  
         
    inline uint256 AsBignum() const { 
        uint256 ret;
        if (kind == Kind_Number) {
            if (flags & Flag_Bignum) {
                return *((uint256*) data.data());
            } else 
                return uint256(AsUInt64());
        } else if (kind == Kind_String) return uint256S(AsString());
        return ret;
    }  

    inline std::string AsString() const {
        std::string ret;
        if (kind == Kind_String) {
            return std::string(data.begin(), data.end());
        }
        else if (kind == Kind_Number) {
            if (flags & Flag_Bignum) return ((uint256*) data.data())->GetHex();

            std::ostringstream ss(ret);
            ss.imbue(std::locale::classic());

            switch (bits)
            {
                case 8: {if (flags & Flag_Boolean) ss << (*((uint8_t*) data.data()) > 0 ? "true" : "false"); else if (flags & Flag_Unsigned) ss << *((uint8_t*)data.data()); else ss << *((int8_t*)data.data()); }break;
                case 16: {if (flags & Flag_Float) ss << *((float*)data.data()); else if (flags & Flag_Unsigned) ss << *((uint16_t*)data.data()); else ss << *((int16_t*)data.data()); }break;
                case 32: {if (flags & Flag_Float) ss << *((float*)data.data()); else if (flags & Flag_Unsigned) ss << *((uint32_t*)data.data()); else ss << *((int32_t*)data.data()); }break;
                case 64: {if (flags & Flag_Double) ss << *((double*)data.data()); else if (flags & Flag_Unsigned) ss << *((uint64_t*)data.data()); else ss << *((int64_t*)data.data()); }break;
                default: ss << 0;
            }
        }
        else if (kind == Kind_Pointer) {
            std::ostringstream ss(ret);
            ss.imbue(std::locale::classic());
            ss << (void *) data.data();
        }
        else if (kind == Kind_Reference) {
            return GetFullname();
        }
        else if (kind == Kind_Object) return "[object Object]";
        else if (kind == Kind_Array) return "[object Array]";
        else if (kind == Kind_Function) return "[object Function]";
        return ret;
    }

    std::string OpcodeAsString(std::vector<char>::const_iterator &it);

    static inline std::string ScopedName(const std::string scope, const std::string &value) {
        return scope.empty() ? value : scope + "." + value;
    }

    static inline std::string ScopedNameSelect(const bool first, const std::string &scopeFirst, const std::string &scopeSecond, const std::string &name) {
        return first ? ScopedName(scopeFirst, name) : ScopedName(scopeSecond, name);
    }

    inline std::string AsSource() {
        switch (kind)
        {
            case Kind_Null: return "null";
            case Kind_String: return std::string("new ") + (flags & Flag_Symbol ? "Symbol" : "String") + "(\"" + std::string(data.begin(), data.end()) + "\")";
            case Kind_Number:
            {
                if (flags & Flag_Boolean) {
                    return AsBoolean() ? "true" : "false";
                }
                else if (flags & Flag_Integer) {
                    return (flags & Flag_Unsigned) ? std::string("new Number(") + std::to_string(AsUInt64()) + ")" : "new Number(" + std::to_string(AsInt64()) + ")";
                }
                else if (flags & Flag_Float) {
                    return std::string("new Number(") + std::to_string(AsFloat()) + ")";
                }
                else if (flags & Flag_Double) {
                    return std::string("new Number(") + std::to_string(AsDouble()) + ")";
                }
                else if (flags & Flag_Bignum) {
                    return std::string("new Number(\"")+AsBignum().GetHex()+"\")";
                }
            } break;
            case Kind_Variable: 
            {
                if (ns && ns->IsFunction() && !(flags & Flag_Property)) {
                    return std::string("var ") + GetName() + " = " + (value ? value->AsSource() : "null");
                } else if (ns && ns->IsArray()) {
                    return value ? value->AsSource() : "null";
                } else 
                    return GetName() + ": " + (value ? value->AsSource() : "null");
            }
            break;
            case Kind_Reference:
            {
                return GetFullname();
            } break;
            case Kind_Array:
            {
                std::string ret = "[";
                for (std::vector<CVMValue*>::iterator it = values.begin(); it != values.end(); it++) ret += (*it)->AsSource();
                ret += "]";
                return ret;
            } break;
            case Kind_Object:
            {
                std::string ret = "{";
                for (std::unordered_map<std::string, CVMValue*>::iterator it = std::begin(variables); it != std::end(variables); it++) {
                    if (it != std::begin(variables)) ret += ", ";
                    ret += (*it).second ? (*it).second->AsSource() : ((*it).first + ": null");
                }
                ret += "}";
                return ret;
            } break;
            case Kind_Function:
            {
                std::string ret = "function() {";
                if (flags & Flag_Native) ret += "[native code]";
                else {
                    std::vector<char>::const_iterator it = data.begin();
                    while (it < data.end()) ret += OpcodeAsString(it);
                }
                ret += "}";
                return ret;
            } break;
        }
        return "";
    }

    inline size_t Length() const {
        if (kind == Kind_String) return data.size() - 1;
        else  return Bytes();
    }             
};

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

CVMValue *VMMakeFrameNamespace(CVMValue *callable);

class CVMState {
public:
    enum {
        MAX_STACK = 64,
        MAX_SCOPE = 32
    };


    struct Frame {
        size_t refCount;
        size_t pc;
        Frame *caller;
        CVMValue *module;
        CVMValue *ns, *rtn, *ths, *arguments;
        CVMValue *callable;
        std::stack<CVMValue> scope;

        Frame() : refCount(1) {
            MakeFrame(nullptr, 0, nullptr, nullptr, nullptr);
        }
        Frame(Frame *caller, Frame *frame) : refCount(1) {
            MakeFrame(caller, frame ? frame->pc : 0, frame ? frame->ths : nullptr, frame ? frame->callable : nullptr, frame ? frame->arguments : nullptr);
        }
        Frame(Frame *caller, const size_t pc, CVMValue *ths, CVMValue *callable) : refCount(1)  {
            MakeFrame(caller, pc, ths, callable, nullptr);
        }
        ~Frame() {
            if (module) module->Drop();
            if (ns) ns->Drop();
            if (caller) caller->Drop();
            if (rtn) rtn->Drop();
            if (ths) ths->Drop();
            if (arguments) arguments->Drop();
            if (callable) callable->Drop();
        }

        Frame *Grab() {
            refCount++;
            return this;
        }

        Frame *Drop() {
            refCount--;
            if (refCount == 0) { delete this; return nullptr; }
            return this;
        }

        void MakeFrame(Frame *caller, const size_t pc, CVMValue *ths, CVMValue *callable, CVMValue *args) {
            assert(callable != nullptr);
            this->module = callable->module->Grab();
            this->ns = VMMakeFrameNamespace(callable);
            this->caller = caller ? caller->Grab() : nullptr;
            this->callable = callable->Grab();
            this->pc = pc;
            this->ths = this->ns->GetKeyValue("__gs_this__")->Grab();
            if (ths) this->ths->Assign(ths);
            this->rtn = this->ns->GetKeyValue("__gs_return__")->Grab();
            this->arguments = this->ns->GetKeyValue("__gs_arguments__")->Grab();
            if (args) this->arguments->Assign(args);
        }
    };

    size_t                 bp, dp;
    std::stack<CVMValue*>  stack;
    std::stack<Frame>      frame;


    CVMState();
    CVMState(CVMState *state);
    virtual ~CVMState();

    inline void Flush() {
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