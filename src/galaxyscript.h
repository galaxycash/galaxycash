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

/*

typedef std::vector<char> CVMBytecode;

typedef std::vector<char> CScriptData;
typedef std::vector<CScriptData> CScriptDataArray;
typedef std::vector<std::string> CStringVector;



#include "compat/endian.h"

class CVMValue;
class CVMModule;
class CVMState;

typedef void (*CVMFunctionPrototype)(CVMState*);

enum {
    AddressSpace_Root = 0,
    AddressSpace_This,
    AddressSpace_Scope
};


struct CVMTypeinfo {
    enum Kind {
        Kind_Null = 0,
        Kind_Variable,
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
        Flag_Callable = BIT(15),
        Flag_Block = BIT(16)
    };

    uint32_t        version;
    std::string     name;
    std::string     module;
    CVMTypeinfo *   super;
    Kind            kind;
    uint8_t         bits; // For numbers
    uint64_t        flags;
    CVMTypeinfo *   ctor;
    CVMTypeinfo *   dtor;



    CVMTypeinfo() : super(nullptr), ctor(nullptr), dtor(nullptr) { SetNull(); }
    CVMTypeinfo(CVMTypeinfo *type) : version(type->version), name(type->name), super(type->super ? new CVMTypeinfo(type->super) : nullptr), kind(type->kind), flags(type->flags) {}

    inline void SetNull() {
        version = 1;
        name.clear();
        kind = Kind_Null;
        flags = 0;
        bits = 0;
        if (super) delete super;
        super = ctor = dtor = nullptr;
    }

    inline bool IsNull() {
        return name.empty() && (kind == Kind_Null);
    }
    
    ADD_SERIALIZE_METHODS;
    
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(this->version);
        READWRITE(super);
        READWRITE(name);
        READWRITE(*(int8_t*)&kind);
        READWRITE(flags);
        READWRITE(bits);
        READWRITE(ctor);
        READWIRTE(dtor);
    }

    static CVMTypeinfo &UndefinedType();
    static CVMTypeinfo &NullType();
    static CVMTypeinfo &VoidType();
    static CVMTypeinfo &StringType();
    static CVMTypeinfo &SymbolType();
    static CVMTypeinfo &BooleanType();
    static CVMTypeinfo &IntegerType();
    static CVMTypeinfo &Int8Type();
    static CVMTypeinfo &Int16Type();
    static CVMTypeinfo &Int32Type();
    static CVMTypeinfo &Int64Type();
    static CVMTypeinfo &UInt8Type();
    static CVMTypeinfo &UInt16Type();
    static CVMTypeinfo &UInt32Type();
    static CVMTypeinfo &UInt64Type();
    static CVMTypeinfo &FloatType();
    static CVMTypeinfo &DoubleType();
    static CVMTypeinfo &BignumType();
    static CVMTypeinfo &ObjectType();
    static CVMTypeinfo &ArrayType();
    static CVMTypeinfo &FunctionType();
    static CVMTypeinfo &ModuleType();
};

struct CVMModuleHeader {
    uint32_t    version;
    uint32_t    time;
    std::string name;
    uint64_t    flags;

    std::vector<CVMTypeinfo> types;
    std::vector<CVMTypeinfo> variables;
    std::vector<CVMTypeinfo> functions;
    
    CModuleHeader() { SetNull(); }

    inline void SetNull() {
        version = 1;
        time = 0;
        name.clear();
        flags = 0;
        types.clear();
        variables.clear();
        functions.clear();
    }

    inline bool IsNull() const {
        return (time == 0) && name.empty();
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(this->version);
        READWRITE(time);
        READWRITE(name);
        READWRITE(flags);
        READWRITE(types);
        READWRITE(variables);
        READWRITE(functions);
    }
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

typedef enum CVMUnaryOp {
    CVMUnaryOp_Not, // !v
    CVMUnaryOp_Inv, // -v
    CVMUnaryOp_Increment, // v++
    CVMUnaryOp_Decrement, // v--
    CVMUnaryOp_PreIncrement, // ++v
    CVMUnaryOp_PreDecrement, // --v
    CVMUnaryOp_Assign, // =
    CVMUnaryOp_AssignAdd, // +=
    CVMUnaryOp_AssignSub, // -=
    CVMUnaryOp_AssignMul, // *=
    CVMUnaryOp_AssignDiv, // /=
    CVMUnaryOp_AssignMod, // %=
    CVMUnaryOp_AssignBitwiseAnd, // &=
    CVMUnaryOp_AssignBitwiseOr, // |=
    CVMUnaryOp_AssignXor, // ^=
} CVMUnary;

typedef enum CVMBinaryOp {
    CVMBinaryOp_Add, /// +
    CVMBinaryOp_Sub, // -
    CVMBinaryOp_Mul, // *
    CVMBinaryOp_Div, // /
    CVMBinaryOp_Mod, // %
    CVMBinaryOp_And, // &&
    CVMBinaryOp_Or, // ||
    CVMBinaryOp_Xor, // ^
    CVMBinaryOp_BitwiseAnd, // &
    CVMBinaryOp_BitwiseOr, // |
    CVMBinaryOp_BitwiseLShift, // <<
    CVMBinaryOp_BitwiseRShift, // >>
    CVMBinaryOp_LogicalEqual, // ==
    CVMBinaryOp_LogicalNotEqual, // !=
    CVMBinaryOp_LogicalLess, // <
    CVMBInaryOp_LogicalLessOrEqual, // <=
    CVMBinaryOp_LogicalGreater, // >
    CVMBInaryOp_LogicalGreaterOrEqual // >=
} CVMBinary;


typedef std::function<void(CVMState*)> CVMFunction;

class CVMValue {
public:
    CVMValue *root;
    CVMValue *value;

    std::string name;
    std::vector<char> data;
    std::vector<CVMValue*> values;
    std::unordered_map<std::string, CVMValue*> variables;
    CVMTypeinfo type;
    size_t refCounter;
    uint8_t bits;
    uint64_t flags;

    CVMValue();
    CVMValue(CVMValue *root, const CVMTypeinfo &type, const std::string &name, const uint64_t flags = 0, const std::vector<char> &data = std::vector<char>());
    CVMValue(CVMValue *value);
    virtual ~CVMValue();

    static CVMValue *MakeBlock(CVMValue *root);
    static CVMValue *MakeVariable(CVMValue *root, const std::string &name, CVMValue *value);
    static CVMValue *MakeReference(CVMValue *root, const std::string &name, CVMValue *value);

    virtual bool Encode(std::vector<char> &data);
    virtual bool Decode(const std::vector<char> &data);

    virtual void SetNull();

    virtual uint64_t Flags() const { return flags; }

    inline bool IsNull() const {
        if (kind == Kind_Null) return true;
        else if (kind == Kind_Variable) return (value == nullptr);
        else if (kind == Kind_Reference) return name.empty();
        else if (kind == Kind_Pointer) return data.empty();
        return false;
    }
    inline bool IsBlock() const { return (flags & Flag_Block)}
    inline bool IsPointer() const { return (kind == Kind_String); }
    inline bool IsString() const { return (kind == Kind_String); }
    inline bool IsSymbol() const { return (kind == Kind_String) && (Flags() & Flag_Symbol); }
    inline bool IsNumber() const { return (kind == Kind_Number); }
    inline bool IsBoolean() const { return (kind == Kind_Number) && (Flags() & Flag_Boolean); }
    inline bool IsBignum() const { return (kind == Kind_Number) && (Flags() & Flag_Bignum); }
    inline bool IsFloat() const { return (kind == Kind_Number) && (Flags() & Flag_Float); }
    inline bool IsDouble() const { return (kind == Kind_Number) && (Flags() & Flag_Double); }
    inline bool IsInteger() const { return (kind == Kind_Number) && (!(Flags() & Flag_Boolean) && !(Flags() & Flag_Float) && !(Flags() & Flag_Double) && !(Flags() & Flag_Bignum))); }
    inline bool IsUnsigned() const { return (kind == Kind_Number) && (Flags() & Flag_Unsigned); }
    inline bool IsInt8() const { return IsInteger() && (Bits() == 8); }
    inline bool IsInt16() const { return IsInteger() && (Bits() == 16); }
    inline bool IsInt32() const { return IsInteger() && (Bits() == 32); }
    inline bool IsInt64() const { return IsInteger() && (Bits() == 64); }
    inline bool IsUInt8() const { return IsInteger() && (Bits() == 8); }
    inline bool IsUInt16() const { return IsInteger() && IsUnsigned() && (Bits() == 16); }
    inline bool IsUInt32() const { return IsInteger() && IsUnsigned() && (Bits() == 32); }
    inline bool IsUInt64() const { return IsInteger() && IsUnsigned() && (Bits() == 64); }
    inline bool IsVariable() const { return (kind == Kind_Variable); }
    inline bool IsPrimitive() const { return IsString() || IsNumber() || IsPointer() || IsReference() || IsVariable(); }
    inline bool IsObject() const { return (kind == Kind_Object); }
    inline bool IsArray() const { return (kind == Kind_Array); }
    inline bool IsFunction() const { return (kind == Kind_Function); }
    inline bool IsCallable() const { return (flags & Flag_Callable); }
    

    inline std::string GetName() const { return name; }
    inline std::string GetFullname() const { return (root) ? (root->GetFullname().empty() ? GetName() : root->GetFullname() + (!GetName().empty() ? "." + GetName() : GetName())) : GetName(); }

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

    inline CVMValue *SetKeyValue2(const std::string &name, CVMValue *value) {
        if (variables.count(name) && variables[name] != nullptr) { if (value) {variables[name]->Assign(value);} else {variables[name]->SetNull();} return variables[name]; }
        variables[name] = new CVMValue(this, name, Kind_Variable, 0, Flag_Property);
        if (value) variables[name]->value = value->Grab();
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
typedef std::vector<CVMValue*> CValueVector;


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
        std::stack<CVMValue*> scope;

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
            while (scope.size()) {
                scope.top()->Drop(); scope.pop();
            }
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
            this->ns = CVMValue::MakeBlock(callable);
            this->scope.push(this->ns->Grab());
            this->caller = caller ? caller->Grab() : nullptr;
            this->callable = callable->Grab();
            this->pc = pc;
            this->ths = this->ns->SetKeyValue("__gs_this__", ths)->Grab();
            this->rtn = this->ns->SetKeyValue("__gs_return__", CVMValue())->Grab();
            this->arguments = this->ns->SetKeyValue("__gs_arguments__", args ? args : new CVMValue(this->ns, CVMTypeinfo::ArrayType(), "__gs_arguments__", CVMValue::Flag_Buildin))->Grab();
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

CVMValue *VMLoadModule(CVMValue *module, const std::string &name);
CVMValue *VMSaveModule(CVMValue *module, const std::string &name);
CVMValue *VMAddModule(const std::string &name);
CVMValue *VMGetModule(const std::string &name);
bool VMCall(CVMState *state, CVMValue *callable);
bool VMCall(CVMValue *callable);
bool VMRunSource(const std::string &source);
bool VMRunFile(const std::string &file);
*/

#endif