// Copyright (c) 2017-2019 The GalaxyCash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef GALAXYCASH_EXT_SCRIPT_H
#define GALAXYCASH_EXT_SCRIPT_H

#include "hash.h"
#include "random.h"
#include "serialize.h"
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstring>
#include <functional>
#include <map>
#include <memory>
#include <serialize.h>
#include <streams.h>
#include <unordered_map>
#include <util.h>
#include <utilstrencodings.h>
#include <vector>

// GalaxyCash Scripting engine


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
        Kind_Undefined = 0,
        Kind_Null,
        Kind_Variable,
        Kind_Pointer,
        Kind_Callable,
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
        Flag_Function = BIT(15),
        Flag_Block = BIT(16),
        Flag_Frame = BIT(17),
        Flag_Constructor = BIT(18),
        Flag_Destructor = BIT(19)
    };

    uint32_t version;
    std::string name;
    std::string module;
    std::vector<std::string> args;
    CVMTypeinfo* super;
    Kind kind;
    uint8_t bits; // For numbers
    uint64_t flags;
    int64_t addr;
    CVMTypeinfo* ctor;
    CVMTypeinfo* dtor;


    CVMTypeinfo() : super(nullptr), ctor(nullptr), dtor(nullptr) { SetNull(); }
    CVMTypeinfo(CVMTypeinfo* type) : version(type->version), name(type->name), super(type->super ? new CVMTypeinfo(type->super) : nullptr), kind(type->kind), flags(type->flags), ctor(type->ctor ? new CVMTypeinfo(type->ctor) : nullptr), dtor(type->dtor ? new CVMTypeinfo(type->dtor) : nullptr) {}

    inline void SetNull()
    {
        version = 1;
        name.clear();
        module.clear();
        args.clear();
        kind = Kind_Null;
        flags = addr = 0;
        bits = 0;
        if (super) delete super;
        if (ctor) delete ctor;
        if (dtor) delete dtor;
        super = ctor = dtor = nullptr;
    }

    inline uint64_t& Flags()
    {
        return flags;
    }

    inline const uint64_t& Flags() const
    {
        return flags;
    }

    inline bool IsNull()
    {
        return name.empty() && (kind == CVMTypeinfo::Kind_Null);
    }

    inline bool Extends() const
    {
        return (super != nullptr);
    }

    inline bool HaveConstructor() const
    {
        return (ctor != nullptr);
    }

    inline bool HaveDestructor() const
    {
        return (dtor != nullptr);
    }

    inline bool IsNull() const
    {
        return (kind == CVMTypeinfo::Kind_Null) && !(flags & Flag_Void);
    }

    inline bool IsUndefined() const
    {
        return (kind == CVMTypeinfo::Kind_Undefined);
    }

    inline bool IsVoid() const
    {
        return (kind == CVMTypeinfo::Kind_Null) && (flags & Flag_Void);
    }

    inline bool IsPointer() const { return (kind == Kind_String); }
    inline bool IsString() const { return (kind == Kind_String); }
    inline bool IsSymbol() const { return (kind == Kind_String) && (Flags() & Flag_Symbol); }
    inline bool IsNumber() const { return (kind == Kind_Number); }
    inline bool IsBoolean() const { return (kind == Kind_Number) && (Flags() & Flag_Boolean); }
    inline bool IsBignum() const { return (kind == Kind_Number) && (Flags() & Flag_Bignum); }
    inline bool IsFloat() const { return (kind == Kind_Number) && (Flags() & Flag_Float); }
    inline bool IsDouble() const { return (kind == Kind_Number) && (Flags() & Flag_Double); }
    inline bool IsInteger() const { return (kind == Kind_Number) && (!(Flags() & Flag_Boolean) && !(Flags() & Flag_Float) && !(Flags() & Flag_Double) && !(Flags() & Flag_Bignum)); }
    inline bool IsUnsigned() const { return (kind == Kind_Number) && (Flags() & Flag_Unsigned); }
    inline bool IsInt8() const { return IsInteger() && (bits == 8); }
    inline bool IsInt16() const { return IsInteger() && (bits == 16); }
    inline bool IsInt32() const { return IsInteger() && (bits == 32); }
    inline bool IsInt64() const { return IsInteger() && (bits == 64); }
    inline bool IsUInt8() const { return IsInteger() && (bits == 8); }
    inline bool IsUInt16() const { return IsInteger() && IsUnsigned() && (bits == 16); }
    inline bool IsUInt32() const { return IsInteger() && IsUnsigned() && (bits == 32); }
    inline bool IsUInt64() const { return IsInteger() && IsUnsigned() && (bits == 64); }
    inline bool IsVariable() const { return (kind == Kind_Variable); }
    inline bool IsProperty() const { return (kind == Kind_Variable) && (flags & Flag_Property); }
    inline bool IsPrimitive() const { return IsString() || IsNumber() || IsPointer() || IsVariable(); }
    inline bool IsObject() const { return (kind == Kind_Object); }
    inline bool IsArray() const { return (kind == Kind_Array); }
    inline bool IsFunction() const { return (kind == Kind_Callable) && (Flags() & Flag_Function); }
    inline bool IsBlock() const { return (kind == Kind_Callable) && (Flags() & Flag_Block); }
    inline bool IsFrame() const { return (kind == Kind_Callable) && (Flags() & Flag_Frame); }
    inline bool IsConstructor() const { return (kind == Kind_Callable) && (Flags() & Flag_Constructor); }
    inline bool IsDestructor() const { return (kind == Kind_Callable) && (Flags() & Flag_Destructor); }
    inline bool IsCallable() const { return (kind == Kind_Callable); }
    inline bool IsModule() const { return (kind == Kind_Module); }
    inline bool IsNative() const { return (flags & Flag_Native); }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(this->version);
        if (ser_action.ForRead()) {
            int8_t ff = 0;
            s >> ff;
            if (ff) {
                super = new CVMTypeinfo();
                READWRITE(*super);
            } else {
                if (super) delete super;
                super = nullptr;
            }
        } else {
            if (super) {
                s << int8_t(1);
                s << *super;
            } else {
                s << int8_t(0);
            }
        }
        READWRITE(name);
        READWRITE(*(int8_t*)&kind);
        READWRITE(flags);
        READWRITE(bits);
        if (ser_action.ForRead()) {
            int8_t ff = 0;
            s >> ff;
            if (ff) {
                ctor = new CVMTypeinfo();
                READWRITE(*ctor);
            } else {
                if (ctor) delete ctor;
                ctor = nullptr;
            }
            s >> ff;
            if (ff) {
                dtor = new CVMTypeinfo();
                READWRITE(*dtor);
            } else {
                if (dtor) delete dtor;
                dtor = nullptr;
            }
        } else {
            if (ctor) {
                s << int8_t(1);
                s << *ctor;
            } else {
                s << int8_t(0);
            }
            if (dtor) {
                s << int8_t(1);
                s << *dtor;
            } else {
                s << int8_t(0);
            }
        }
    }

    static CVMTypeinfo& UndefinedType();
    static CVMTypeinfo& NullType();
    static CVMTypeinfo& VoidType();
    static CVMTypeinfo& StringType();
    static CVMTypeinfo& SymbolType();
    static CVMTypeinfo& BooleanType();
    static CVMTypeinfo& IntegerType();
    static CVMTypeinfo& Int8Type();
    static CVMTypeinfo& Int16Type();
    static CVMTypeinfo& Int32Type();
    static CVMTypeinfo& Int64Type();
    static CVMTypeinfo& UInt8Type();
    static CVMTypeinfo& UInt16Type();
    static CVMTypeinfo& UInt32Type();
    static CVMTypeinfo& UInt64Type();
    static CVMTypeinfo& FloatType();
    static CVMTypeinfo& DoubleType();
    static CVMTypeinfo& BignumType();
    static CVMTypeinfo& ObjectType();
    static CVMTypeinfo& ArrayType();
    static CVMTypeinfo& FunctionType();
    static CVMTypeinfo& FrameType();
    static CVMTypeinfo& BlockType();
    static CVMTypeinfo& ModuleType();
    static CVMTypeinfo& VariableType();
    static CVMTypeinfo& PropertyType();
};

struct CVMModule {
    uint32_t version;
    uint32_t time;
    std::string name;
    uint64_t flags;

    std::vector<CVMTypeinfo> types;
    std::vector<CVMTypeinfo> variables;
    std::vector<CVMTypeinfo> functions;

    CVMModule() { SetNull(); }
    CVMModule(const CVMModule &mod) : version(mod.version), time(mod.time), name(mod.name), flags(mod.flags), types(mod.types), variables(mod.variables), functions(mod.functions) {    
    }

    inline void SetNull()
    {
        version = 1;
        time = 0;
        name.clear();
        flags = 0;
        types.clear();
        variables.clear();
        functions.clear();
    }

    inline bool IsNull() const
    {
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
    CVMUnaryOp_Not,              // !v
    CVMUnaryOp_Inv,              // -v
    CVMUnaryOp_Increment,        // v++
    CVMUnaryOp_Decrement,        // v--
    CVMUnaryOp_PreIncrement,     // ++v
    CVMUnaryOp_PreDecrement,     // --v
    CVMUnaryOp_Assign,           // =
    CVMUnaryOp_AssignAdd,        // +=
    CVMUnaryOp_AssignSub,        // -=
    CVMUnaryOp_AssignMul,        // *=
    CVMUnaryOp_AssignDiv,        // /=
    CVMUnaryOp_AssignMod,        // %=
    CVMUnaryOp_AssignBitwiseAnd, // &=
    CVMUnaryOp_AssignBitwiseOr,  // |=
    CVMUnaryOp_AssignXor,        // ^=
} CVMUnary;

typedef enum CVMBinaryOp {
    CVMBinaryOp_Add,                  /// +
    CVMBinaryOp_Sub,                  // -
    CVMBinaryOp_Mul,                  // *
    CVMBinaryOp_Div,                  // /
    CVMBinaryOp_Mod,                  // %
    CVMBinaryOp_And,                  // &&
    CVMBinaryOp_Or,                   // ||
    CVMBinaryOp_Xor,                  // ^
    CVMBinaryOp_BitwiseAnd,           // &
    CVMBinaryOp_BitwiseOr,            // |
    CVMBinaryOp_BitwiseLShift,        // <<
    CVMBinaryOp_BitwiseRShift,        // >>
    CVMBinaryOp_LogicalEqual,         // ==
    CVMBinaryOp_LogicalNotEqual,      // !=
    CVMBinaryOp_LogicalLess,          // <
    CVMBInaryOp_LogicalLessOrEqual,   // <=
    CVMBinaryOp_LogicalGreater,       // >
    CVMBInaryOp_LogicalGreaterOrEqual // >=
} CVMBinary;


typedef std::function<void(CVMState*)> CVMFunction;

class CVMValue
{
public:
    CVMValue* root;
    CVMValue* value;

    std::string name;
    std::vector<char> data;
    std::vector<CVMValue*> values;
    std::unordered_map<std::string, CVMValue*> variables;
    CVMTypeinfo type;
    size_t refCounter;

    CVMValue();
    CVMValue(CVMValue* root, const CVMTypeinfo& type, const std::string& name, const std::vector<char>& data = std::vector<char>());
    CVMValue(CVMValue* value);
    virtual ~CVMValue();

    static CVMValue* Global();
    static CVMValue* MakeBlock(CVMValue* root);
    static CVMValue* MakeVariable(CVMValue* root, const std::string& name, CVMValue* value);

    virtual bool Encode(std::vector<char>& data);
    virtual bool Decode(const std::vector<char>& data);

    virtual void SetNull();

    virtual uint64_t Flags() const { return type.flags; }

    inline bool IsNull() const
    {
        if (type.kind == CVMTypeinfo::Kind_Null)
            return true;
        else if (type.kind == CVMTypeinfo::Kind_Variable)
            return name.empty() || (value == nullptr);
        else if (type.kind == CVMTypeinfo::Kind_Pointer)
            return data.empty();
        return false;
    }

    inline bool IsUndefined() const
    {
        if (type.kind == CVMTypeinfo::Kind_Undefined) return true;
        return false;
    }

    inline bool IsVoid() const
    {
        return type.IsVoid();
    }

    inline bool IsPointer() const { return (type.kind == CVMTypeinfo::Kind_String); }
    inline bool IsString() const { return (type.kind == CVMTypeinfo::Kind_String); }
    inline bool IsSymbol() const { return (type.kind == CVMTypeinfo::Kind_String) && (Flags() & CVMTypeinfo::Flag_Symbol); }
    inline bool IsNumber() const { return (type.kind == CVMTypeinfo::Kind_Number); }
    inline bool IsBoolean() const { return (type.kind == CVMTypeinfo::Kind_Number) && (Flags() & CVMTypeinfo::Flag_Boolean); }
    inline bool IsBignum() const { return (type.kind == CVMTypeinfo::Kind_Number) && (Flags() & CVMTypeinfo::Flag_Bignum); }
    inline bool IsFloat() const { return (type.kind == CVMTypeinfo::Kind_Number) && (Flags() & CVMTypeinfo::Flag_Float); }
    inline bool IsDouble() const { return (type.kind == CVMTypeinfo::Kind_Number) && (Flags() & CVMTypeinfo::Flag_Double); }
    inline bool IsInteger() const { return (type.kind == CVMTypeinfo::Kind_Number) && (!(Flags() & CVMTypeinfo::Flag_Boolean) && !(Flags() & CVMTypeinfo::Flag_Float) && !(Flags() & CVMTypeinfo::Flag_Double) && !(Flags() & CVMTypeinfo::Flag_Bignum)); }
    inline bool IsUnsigned() const { return (type.kind == CVMTypeinfo::Kind_Number) && (Flags() & CVMTypeinfo::Flag_Unsigned); }
    inline bool IsInt8() const { return IsInteger() && (Bits() == 8); }
    inline bool IsInt16() const { return IsInteger() && (Bits() == 16); }
    inline bool IsInt32() const { return IsInteger() && (Bits() == 32); }
    inline bool IsInt64() const { return IsInteger() && (Bits() == 64); }
    inline bool IsUInt8() const { return IsInteger() && (Bits() == 8); }
    inline bool IsUInt16() const { return IsInteger() && IsUnsigned() && (Bits() == 16); }
    inline bool IsUInt32() const { return IsInteger() && IsUnsigned() && (Bits() == 32); }
    inline bool IsUInt64() const { return IsInteger() && IsUnsigned() && (Bits() == 64); }
    inline bool IsVariable() const { return (type.kind == CVMTypeinfo::Kind_Variable); }
    inline bool IsProperty() const { return (type.kind == CVMTypeinfo::Kind_Variable) && (type.flags & CVMTypeinfo::Flag_Property); }
    inline bool IsPrimitive() const { return IsString() || IsNumber() || IsPointer() || IsVariable(); }
    inline bool IsObject() const { return (type.kind == CVMTypeinfo::Kind_Object); }
    inline bool IsArray() const { return (type.kind == CVMTypeinfo::Kind_Array); }
    inline bool IsFunction() const { return (type.kind == CVMTypeinfo::Kind_Callable) && (Flags() & CVMTypeinfo::Flag_Function); }
    inline bool IsBlock() const { return (type.kind == CVMTypeinfo::Kind_Callable) && (Flags() & CVMTypeinfo::Flag_Block); }
    inline bool IsFrame() const { return (type.kind == CVMTypeinfo::Kind_Callable) && (Flags() & CVMTypeinfo::Flag_Frame); }
    inline bool IsCallable() const { return (type.kind == CVMTypeinfo::Kind_Callable); }
    inline bool IsModule() const { return (type.kind == CVMTypeinfo::Kind_Module); }

    static inline std::string RandName()
    {
        char name[17];
        memset(name, 0, sizeof(name));
        GetRandBytes((unsigned char*)name, sizeof(name) - 1);
        return name;
    }
    inline std::string GetName() const { return name; }
    inline std::string GetFullname() const { return (root) ? (root->GetFullname().empty() ? GetName() : root->GetFullname() + (!GetName().empty() ? "." + GetName() : GetName())) : GetName(); }

    inline bool IsCopable() const { return (type.kind != CVMTypeinfo::Kind_Module); }

    inline CVMValue* Grab()
    {
        refCounter++;
        return this;
    }
    inline CVMValue* Drop()
    {
        refCounter--;
        if (refCounter < 1 && this == Global()) refCounter = 1;
        if (refCounter == 0 && this != Global()) {
            delete this;
            return nullptr;
        }
        return this;
    }

    virtual void Init(CVMValue* initializer);
    virtual void Assign(CVMValue* value);

    inline CVMValue* SetKeyValue(const std::string& name, CVMValue* value)
    {
        if (variables.count(name) && variables[name] != nullptr) {
            if (value) {
                variables[name]->Assign(value);
            } else {
                variables[name]->SetNull();
            }
            return variables[name];
        }
        variables[name] = new CVMValue(this, CVMTypeinfo::PropertyType(), name);
        if (value) variables[name]->Assign(value);
        return variables[name];
    }

    inline CVMValue* SetKeyValue2(const std::string& name, CVMValue* value)
    {
        if (variables.count(name) && variables[name] != nullptr) {
            if (value) {
                variables[name]->Assign(value);
            } else {
                variables[name]->SetNull();
            }
            return variables[name];
        }
        variables[name] = new CVMValue(this, CVMTypeinfo::VariableType(), name);
        if (value) variables[name]->value = value->Grab();
        return variables[name];
    }

    inline CVMValue* GetKeyValue(const std::string& name)
    {
        if (!variables.count(name))
            return SetKeyValue(name, nullptr);
        return variables[name];
    }

    inline void SetArrayElement(int index, CVMValue* value)
    {
        if (index >= 0) {
            if (index >= values.size()) values.resize(index + 1);
            if (value) {
                values[index] = new CVMValue(this, value->type, RandName());
                values[index]->Assign(value);
            } else {
                values[index] = nullptr;
            }
        }
    }

    inline CVMValue* GetArrayElement(int index) const
    {
        if (index >= 0 && index < values.size()) return values[index];
        return nullptr;
    }

    inline size_t Bytes() const
    {
        return data.size();
    }

    inline size_t Bits() const
    {
        return type.bits;
    }

    inline std::string AsVariableName()
    {
        return AsSource();
    }

    inline bool AsBoolean() const
    {
        if (type.kind == CVMTypeinfo::Kind_Number) {
            switch (type.bits) {
            case 8: {
                if (type.flags & CVMTypeinfo::Flag_Integer) {
                    if (type.flags & CVMTypeinfo::Flag_Unsigned)
                        return *((uint8_t*)data.data()) > 0;
                    else
                        return *((int8_t*)data.data()) > 0;
                } else if (type.flags & CVMTypeinfo::Flag_Boolean)
                    return *((uint8_t*)data.data()) > 0;
                else
                    return false;
            } break;
            case 16: {
                if (type.flags & CVMTypeinfo::Flag_Integer) {
                    if (type.flags & CVMTypeinfo::Flag_Unsigned)
                        return *((uint16_t*)data.data()) > 0;
                    else
                        return *((int16_t*)data.data()) > 0;
                } else
                    return false;
            } break;
            case 32: {
                if (type.flags & CVMTypeinfo::Flag_Integer) {
                    if (type.flags & CVMTypeinfo::Flag_Unsigned)
                        return *((uint32_t*)data.data()) > 0;
                    else
                        return *((int32_t*)data.data()) > 0;
                } else
                    return false;
            } break;
            case 64: {
                if (type.flags & CVMTypeinfo::Flag_Integer) {
                    if (type.flags & CVMTypeinfo::Flag_Unsigned)
                        return *((uint64_t*)data.data()) > 0;
                    else
                        return *((int64_t*)data.data()) > 0;
                } else
                    return false;
            } break;
            default:
                return false;
            }
        } else if (type.kind == CVMTypeinfo::Kind_Pointer)
            return ((void*)data.data()) != nullptr;
        else if (type.kind == CVMTypeinfo::Kind_Variable)
            return (value != nullptr);
        return false;
    }

    inline int8_t AsInt8() const
    {
        if (type.kind == CVMTypeinfo::Kind_Number) {
            switch (type.bits) {
            case 8: {
                if (type.flags & CVMTypeinfo::Flag_Integer) {
                    if (type.flags & CVMTypeinfo::Flag_Unsigned)
                        return (int8_t) * ((uint8_t*)data.data());
                    else
                        return *((int8_t*)data.data());
                } else if (type.flags & CVMTypeinfo::Flag_Boolean)
                    return *((uint8_t*)data.data()) > 0 ? 1 : 0;
                else
                    return 0;
            } break;
            case 16: {
                if (type.flags & CVMTypeinfo::Flag_Integer) {
                    if (type.flags & CVMTypeinfo::Flag_Unsigned)
                        return (int8_t) * ((uint16_t*)data.data());
                    else
                        return (int8_t) * ((int16_t*)data.data());
                } else
                    return 0;
            } break;
            case 32: {
                if (type.flags & CVMTypeinfo::Flag_Integer) {
                    if (type.flags & CVMTypeinfo::Flag_Unsigned)
                        return (int8_t) * ((uint32_t*)data.data());
                    else
                        return (int8_t) * ((int32_t*)data.data());
                } else
                    return 0;
            } break;
            case 64: {
                if (type.flags & CVMTypeinfo::Flag_Integer) {
                    if (type.flags & CVMTypeinfo::Flag_Unsigned)
                        return (int8_t) * ((uint64_t*)data.data());
                    else
                        return (int8_t) * ((int64_t*)data.data());
                } else
                    return 0;
            } break;
            default:
                return 0;
            }
        }
        return 0;
    }

    inline int16_t AsInt16() const
    {
        if (type.kind == CVMTypeinfo::Kind_Number) {
            switch (type.bits) {
            case 8: {
                if (type.flags & CVMTypeinfo::Flag_Integer) {
                    if (type.flags & CVMTypeinfo::Flag_Unsigned)
                        return (int16_t) * ((uint8_t*)data.data());
                    else
                        return (int16_t) * ((int8_t*)data.data());
                } else if (type.flags & CVMTypeinfo::Flag_Boolean)
                    return *((uint8_t*)data.data()) > 0 ? 1 : 0;
                else
                    return 0;
            } break;
            case 16: {
                if (type.flags & CVMTypeinfo::Flag_Integer) {
                    if (type.flags & CVMTypeinfo::Flag_Unsigned)
                        return (int16_t) * ((uint16_t*)data.data());
                    else
                        return *((int16_t*)data.data());
                } else if (type.flags & CVMTypeinfo::Flag_Float) {
                    return (int16_t) * ((float*)data.data());
                } else
                    return 0;
            } break;
            case 32: {
                if (type.flags & CVMTypeinfo::Flag_Integer) {
                    if (type.flags & CVMTypeinfo::Flag_Unsigned)
                        return (int16_t) * ((uint32_t*)data.data());
                    else
                        return (int16_t) * ((int32_t*)data.data());
                } else if (type.flags & CVMTypeinfo::Flag_Float) {
                    return (int16_t) * ((float*)data.data());
                } else
                    return 0;
            } break;
            case 64: {
                if (type.flags & CVMTypeinfo::Flag_Integer) {
                    if (type.flags & CVMTypeinfo::Flag_Unsigned)
                        return (int16_t) * ((uint64_t*)data.data());
                    else
                        return (int16_t) * ((int64_t*)data.data());
                } else if (type.flags & CVMTypeinfo::Flag_Double) {
                    return (int16_t) * ((double*)data.data());
                } else
                    return 0;
            } break;
            default:
                return 0;
            }
        }
        return 0;
    }

    inline int32_t AsInt32() const
    {
        if (type.kind == CVMTypeinfo::Kind_Number) {
            switch (type.bits) {
            case 8: {
                if (type.flags & CVMTypeinfo::Flag_Integer) {
                    if (type.flags & CVMTypeinfo::Flag_Unsigned)
                        return (int32_t) * ((uint8_t*)data.data());
                    else
                        return (int32_t) * ((int8_t*)data.data());
                } else if (type.flags & CVMTypeinfo::Flag_Boolean)
                    return *((uint8_t*)data.data()) > 0 ? 1 : 0;
                else
                    return 0;
            } break;
            case 16: {
                if (type.flags & CVMTypeinfo::Flag_Integer) {
                    if (type.flags & CVMTypeinfo::Flag_Unsigned)
                        return (int32_t) * ((uint16_t*)data.data());
                    else
                        return (int32_t) * ((int16_t*)data.data());
                } else if (type.flags & CVMTypeinfo::Flag_Float) {
                    return (int32_t) * ((float*)data.data());
                } else
                    return 0;
            } break;
            case 32: {
                if (type.flags & CVMTypeinfo::Flag_Integer) {
                    if (type.flags & CVMTypeinfo::Flag_Unsigned)
                        return (int32_t) * ((uint32_t*)data.data());
                    else
                        return *((int32_t*)data.data());
                } else if (type.flags & CVMTypeinfo::Flag_Float) {
                    return (int32_t) * ((float*)data.data());
                } else
                    return 0;
            } break;
            case 64: {
                if (type.flags & CVMTypeinfo::Flag_Integer) {
                    if (type.flags & CVMTypeinfo::Flag_Unsigned)
                        return (int32_t) * ((uint64_t*)data.data());
                    else
                        return (int32_t) * ((int64_t*)data.data());
                } else if (type.flags & CVMTypeinfo::Flag_Double) {
                    return (int32_t) * ((double*)data.data());
                } else
                    return 0;
            } break;
            default:
                return 0;
            }
        }
        return 0;
    }

    inline int32_t AsUInt32() const
    {
        if (type.kind == CVMTypeinfo::Kind_Number) {
            switch (type.bits) {
            case 8: {
                if (type.flags & CVMTypeinfo::Flag_Integer) {
                    if (type.flags & CVMTypeinfo::Flag_Unsigned)
                        return (uint32_t) * ((uint8_t*)data.data());
                    else
                        return (uint32_t) * ((int8_t*)data.data());
                } else if (type.flags & CVMTypeinfo::Flag_Boolean)
                    return *((uint8_t*)data.data()) > 0 ? 1 : 0;
                else
                    return 0;
            } break;
            case 16: {
                if (type.flags & CVMTypeinfo::Flag_Integer) {
                    if (type.flags & CVMTypeinfo::Flag_Unsigned)
                        return (uint32_t) * ((uint16_t*)data.data());
                    else
                        return (uint32_t) * ((int16_t*)data.data());
                } else if (type.flags & CVMTypeinfo::Flag_Float) {
                    return (uint32_t) * ((float*)data.data());
                } else
                    return 0;
            } break;
            case 32: {
                if (type.flags & CVMTypeinfo::Flag_Integer) {
                    if (type.flags & CVMTypeinfo::Flag_Unsigned)
                        return *((uint32_t*)data.data());
                    else
                        return (uint32_t) * ((int32_t*)data.data());
                } else if (type.flags & CVMTypeinfo::Flag_Float) {
                    return (uint32_t) * ((float*)data.data());
                } else
                    return 0;
            } break;
            case 64: {
                if (type.flags & CVMTypeinfo::Flag_Integer) {
                    if (type.flags & CVMTypeinfo::Flag_Unsigned)
                        return (uint32_t) * ((uint64_t*)data.data());
                    else
                        return (uint32_t) * ((int64_t*)data.data());
                } else if (type.flags & CVMTypeinfo::Flag_Double) {
                    return (uint32_t) * ((double*)data.data());
                } else
                    return 0;
            } break;
            default:
                return 0;
            }
        }
        return 0;
    }

    inline int64_t AsInt64() const
    {
        if (type.kind == CVMTypeinfo::Kind_Number) {
            switch (type.bits) {
            case 8: {
                if (type.flags & CVMTypeinfo::Flag_Integer) {
                    if (type.flags & CVMTypeinfo::Flag_Unsigned)
                        return (int64_t) * ((uint8_t*)data.data());
                    else
                        return (int64_t) * ((int8_t*)data.data());
                } else if (type.flags & CVMTypeinfo::Flag_Boolean)
                    return *((uint8_t*)data.data()) > 0 ? 1 : 0;
                else
                    return 0;
            } break;
            case 16: {
                if (type.flags & CVMTypeinfo::Flag_Integer) {
                    if (type.flags & CVMTypeinfo::Flag_Unsigned)
                        return (int64_t) * ((uint16_t*)data.data());
                    else
                        return (int64_t) * ((int16_t*)data.data());
                } else if (type.flags & CVMTypeinfo::Flag_Float) {
                    return (int64_t) * ((float*)data.data());
                } else
                    return 0;
            } break;
            case 32: {
                if (type.flags & CVMTypeinfo::Flag_Integer) {
                    if (type.flags & CVMTypeinfo::Flag_Unsigned)
                        return (int64_t) * ((uint32_t*)data.data());
                    else
                        return (int64_t) * ((int32_t*)data.data());
                } else if (type.flags & CVMTypeinfo::Flag_Float) {
                    return (int64_t) * ((float*)data.data());
                } else
                    return 0;
            } break;
            case 64: {
                if (type.flags & CVMTypeinfo::Flag_Integer) {
                    if (type.flags & CVMTypeinfo::Flag_Unsigned)
                        return (int64_t) * ((uint64_t*)data.data());
                    else
                        return *((int64_t*)data.data());
                } else if (type.flags & CVMTypeinfo::Flag_Double) {
                    return (int64_t) * ((double*)data.data());
                } else
                    return 0;
            } break;
            default:
                return 0;
            }
        }
        return 0;
    }

    inline uint64_t AsUInt64() const
    {
        if (type.kind == CVMTypeinfo::Kind_Number) {
            switch (type.bits) {
            case 8: {
                if (type.flags & CVMTypeinfo::Flag_Integer) {
                    if (type.flags & CVMTypeinfo::Flag_Unsigned)
                        return (uint64_t) * ((uint8_t*)data.data());
                    else
                        return (uint64_t) * ((int8_t*)data.data());
                } else if (type.flags & CVMTypeinfo::Flag_Boolean)
                    return *((uint8_t*)data.data()) > 0 ? 1 : 0;
                else
                    return 0;
            } break;
            case 16: {
                if (type.flags & CVMTypeinfo::Flag_Integer) {
                    if (type.flags & CVMTypeinfo::Flag_Unsigned)
                        return (uint64_t) * ((uint16_t*)data.data());
                    else
                        return (uint64_t) * ((int16_t*)data.data());
                } else if (type.flags & CVMTypeinfo::Flag_Float) {
                    return (uint64_t) * ((float*)data.data());
                } else
                    return 0;
            } break;
            case 32: {
                if (type.flags & CVMTypeinfo::Flag_Integer) {
                    if (type.flags & CVMTypeinfo::Flag_Unsigned)
                        return (uint64_t) * ((uint32_t*)data.data());
                    else
                        return (uint64_t) * ((int32_t*)data.data());
                } else if (type.flags & CVMTypeinfo::Flag_Float) {
                    return (uint64_t) * ((float*)data.data());
                } else
                    return 0;
            } break;
            case 64: {
                if (type.flags & CVMTypeinfo::Flag_Integer) {
                    if (type.flags & CVMTypeinfo::Flag_Unsigned)
                        return *((uint64_t*)data.data());
                    else
                        return (uint64_t) * ((int64_t*)data.data());
                } else if (type.flags & CVMTypeinfo::Flag_Double) {
                    return (uint64_t) * ((double*)data.data());
                } else
                    return 0;
            } break;
            default:
                return 0;
            }
        }
        return 0;
    }

    inline float AsFloat() const
    {
        if (type.kind == CVMTypeinfo::Kind_Number) {
            switch (type.bits) {
            case 8: {
                if (type.flags & CVMTypeinfo::Flag_Integer) {
                    if (type.flags & CVMTypeinfo::Flag_Unsigned)
                        return (float)*((uint8_t*)data.data());
                    else
                        return (float)*((int8_t*)data.data());
                } else if (type.flags & CVMTypeinfo::Flag_Boolean)
                    return *((uint8_t*)data.data()) > 0 ? 1.0f : 0.0f;
                else
                    return 0.0f;
            } break;
            case 16: {
                if (type.flags & CVMTypeinfo::Flag_Integer) {
                    if (type.flags & CVMTypeinfo::Flag_Unsigned)
                        return (float)*((uint16_t*)data.data());
                    else
                        return (float)*((int16_t*)data.data());
                } else if (type.flags & CVMTypeinfo::Flag_Float) {
                    return *((float*)data.data());
                } else
                    return 0.0f;
            } break;
            case 32: {
                if (type.flags & CVMTypeinfo::Flag_Integer) {
                    if (type.flags & CVMTypeinfo::Flag_Unsigned)
                        return (float)*((uint32_t*)data.data());
                    else
                        return (float)*((int32_t*)data.data());
                } else if (type.flags & CVMTypeinfo::Flag_Float) {
                    return *((float*)data.data());
                } else
                    return 0.0f;
            } break;
            case 64: {
                if (type.flags & CVMTypeinfo::Flag_Integer) {
                    if (type.flags & CVMTypeinfo::Flag_Unsigned)
                        return (float)*((uint64_t*)data.data());
                    else
                        return (float)*((int64_t*)data.data());
                } else if (type.flags & CVMTypeinfo::Flag_Double) {
                    return (float)*((double*)data.data());
                } else
                    return 0.0f;
            } break;
            default:
                return 0.0f;
            }
        }
        return 0.0f;
    }


    inline float AsDouble() const
    {
        if (type.kind == CVMTypeinfo::Kind_Number) {
            switch (type.bits) {
            case 8: {
                if (type.flags & CVMTypeinfo::Flag_Integer) {
                    if (type.flags & CVMTypeinfo::Flag_Unsigned)
                        return (double)*((uint8_t*)data.data());
                    else
                        return (double)*((int8_t*)data.data());
                } else if (type.flags & CVMTypeinfo::Flag_Boolean)
                    return *((uint8_t*)data.data()) > 0 ? 1.0 : 0.0;
                else
                    return 0.0;
            } break;
            case 16: {
                if (type.flags & CVMTypeinfo::Flag_Integer) {
                    if (type.flags & CVMTypeinfo::Flag_Unsigned)
                        return (double)*((uint16_t*)data.data());
                    else
                        return (double)*((int16_t*)data.data());
                } else if (type.flags & CVMTypeinfo::Flag_Float) {
                    return (double)*((float*)data.data());
                } else
                    return 0.0;
            } break;
            case 32: {
                if (type.flags & CVMTypeinfo::Flag_Integer) {
                    if (type.flags & CVMTypeinfo::Flag_Unsigned)
                        return (double)*((uint32_t*)data.data());
                    else
                        return (double)*((int32_t*)data.data());
                } else if (type.flags & CVMTypeinfo::Flag_Float) {
                    return (double)*((float*)data.data());
                } else
                    return 0.0;
            } break;
            case 64: {
                if (type.flags & CVMTypeinfo::Flag_Integer) {
                    if (type.flags & CVMTypeinfo::Flag_Unsigned)
                        return (double)*((uint64_t*)data.data());
                    else
                        return (double)*((int64_t*)data.data());
                } else if (type.flags & CVMTypeinfo::Flag_Double) {
                    return *((double*)data.data());
                } else
                    return 0.0;
            } break;
            default:
                return 0.0;
            }
        }
        return 0.0;
    }

    inline uint256 AsBignum() const
    {
        uint256 ret;
        if (type.kind == CVMTypeinfo::Kind_Number) {
            if (type.flags & CVMTypeinfo::Flag_Bignum) {
                return *((uint256*)data.data());
            } else
                return uint256(AsUInt64());
        } else if (type.kind == CVMTypeinfo::Kind_String)
            return uint256S(AsString());
        return ret;
    }

    inline std::string AsString() const
    {
        std::string ret;
        if (type.kind == CVMTypeinfo::Kind_String) {
            return std::string(data.begin(), data.end());
        } else if (type.kind == CVMTypeinfo::Kind_Number) {
            if (type.flags & CVMTypeinfo::Flag_Bignum) return ((uint256*)data.data())->GetHex();

            std::ostringstream ss(ret);
            ss.imbue(std::locale::classic());

            switch (type.bits) {
            case 8: {
                if (type.flags & CVMTypeinfo::Flag_Boolean)
                    ss << (*((uint8_t*)data.data()) > 0 ? "true" : "false");
                else if (type.flags & CVMTypeinfo::Flag_Unsigned)
                    ss << *((uint8_t*)data.data());
                else
                    ss << *((int8_t*)data.data());
            } break;
            case 16: {
                if (type.flags & CVMTypeinfo::Flag_Float)
                    ss << *((float*)data.data());
                else if (type.flags & CVMTypeinfo::Flag_Unsigned)
                    ss << *((uint16_t*)data.data());
                else
                    ss << *((int16_t*)data.data());
            } break;
            case 32: {
                if (type.flags & CVMTypeinfo::Flag_Float)
                    ss << *((float*)data.data());
                else if (type.flags & CVMTypeinfo::Flag_Unsigned)
                    ss << *((uint32_t*)data.data());
                else
                    ss << *((int32_t*)data.data());
            } break;
            case 64: {
                if (type.flags & CVMTypeinfo::Flag_Double)
                    ss << *((double*)data.data());
                else if (type.flags & CVMTypeinfo::Flag_Unsigned)
                    ss << *((uint64_t*)data.data());
                else
                    ss << *((int64_t*)data.data());
            } break;
            default:
                ss << 0;
            }
        } else if (type.kind == CVMTypeinfo::Kind_Pointer) {
            std::ostringstream ss(ret);
            ss.imbue(std::locale::classic());
            ss << (void*)data.data();
        } else if (type.kind == CVMTypeinfo::Kind_Variable) {
            return GetFullname();
        } else if (type.kind == CVMTypeinfo::Kind_Object)
            return "[object Object]";
        else if (type.kind == CVMTypeinfo::Kind_Array)
            return "[object Array]";
        else if (type.kind == CVMTypeinfo::Kind_Callable && type.flags & CVMTypeinfo::Flag_Function)
            return "[object Callable]";
        else if (type.kind == CVMTypeinfo::Kind_Callable && type.flags & CVMTypeinfo::Flag_Block)
            return "[object Block]";
        else if (type.kind == CVMTypeinfo::Kind_Callable && type.flags & CVMTypeinfo::Flag_Frame)
            return "[object Frame]";
        else if (type.kind == CVMTypeinfo::Kind_Callable)
            return "[object Callable]";
        else if (type.kind == CVMTypeinfo::Kind_Null)
            return "null";
        else
            ret = "undefined";
        return ret;
    }

    std::string OpcodeAsString(std::vector<char>::const_iterator& it);

    static inline std::string ScopedName(const std::string scope, const std::string& value)
    {
        return scope.empty() ? value : scope + "." + value;
    }

    static inline std::string ScopedNameSelect(const bool first, const std::string& scopeFirst, const std::string& scopeSecond, const std::string& name)
    {
        return first ? ScopedName(scopeFirst, name) : ScopedName(scopeSecond, name);
    }

    inline std::string AsSource()
    {
        switch (type.kind) {
        case CVMTypeinfo::Kind_Null:
            return "null";
        case CVMTypeinfo::Kind_Undefined:
            return "undefined";
        case CVMTypeinfo::Kind_String:
            return std::string("new ") + (type.flags & CVMTypeinfo::Flag_Symbol ? "Symbol" : "String") + "(\"" + std::string(data.begin(), data.end()) + "\")";
        case CVMTypeinfo::Kind_Number: {
            if (type.flags & CVMTypeinfo::Flag_Boolean) {
                return AsBoolean() ? "true" : "false";
            } else if (type.flags & CVMTypeinfo::Flag_Integer) {
                return (type.flags & CVMTypeinfo::Flag_Unsigned) ? std::string("new Number(") + std::to_string(AsUInt64()) + ")" : "new Number(" + std::to_string(AsInt64()) + ")";
            } else if (type.flags & CVMTypeinfo::Flag_Float) {
                return std::string("new Number(") + std::to_string(AsFloat()) + ")";
            } else if (type.flags & CVMTypeinfo::Flag_Double) {
                return std::string("new Number(") + std::to_string(AsDouble()) + ")";
            } else if (type.flags & CVMTypeinfo::Flag_Bignum) {
                return std::string("new Number(\"") + AsBignum().GetHex() + "\")";
            }
        } break;
        case CVMTypeinfo::Kind_Variable: {
            return GetName() + ": " + (value ? value->AsSource() : "null");
        } break;
        case CVMTypeinfo::Kind_Array: {
            std::string ret = "[";
            for (std::vector<CVMValue*>::iterator it = values.begin(); it != values.end(); it++)
                ret += (*it)->AsSource();
            ret += "]";
            return ret;
        } break;
        case CVMTypeinfo::Kind_Object: {
            std::string ret = "{";
            for (std::unordered_map<std::string, CVMValue*>::iterator it = std::begin(variables); it != std::end(variables); it++) {
                if (it != std::begin(variables)) ret += ", ";
                ret += (*it).second ? (*it).second->AsSource() : ((*it).first + ": null");
            }
            ret += "}";
            return ret;
        } break;
        case CVMTypeinfo::Kind_Callable: {
            if (type.flags & CVMTypeinfo::Flag_Function) {
                std::string ret = "function()";
                if (type.flags & CVMTypeinfo::Flag_Native)
                    ret += " { [native code] } ";
                else {
                    std::vector<char>::const_iterator it = data.begin();
                    while (it < data.end())
                        ret += OpcodeAsString(it);
                }
                return ret;
            } else if (type.flags & CVMTypeinfo::Flag_Frame) {
                std::string ret = "";
                if (type.flags & CVMTypeinfo::Flag_Native)
                    ret += " [native code] ";
                else {
                    std::vector<char>::const_iterator it = data.begin();
                    while (it < data.end())
                        ret += OpcodeAsString(it);
                }
                return ret;
            } else if (type.flags & CVMTypeinfo::Flag_Block) {
                std::string ret = " { ";
                if (type.flags & CVMTypeinfo::Flag_Native)
                    ret += " [native code] ";
                else {
                    std::vector<char>::const_iterator it = data.begin();
                    while (it < data.end())
                        ret += OpcodeAsString(it);
                }
                ret += " } ";
                return ret;
            }
            return " [object Callable] ";
        } break;
        }
        return "";
    }

    inline size_t Length() const
    {
        if (type.kind == CVMTypeinfo::Kind_String)
            return data.size() - 1;
        else if (type.kind == CVMTypeinfo::Kind_Array)
            return values.size();
        else
            return Bytes();
    }
};
typedef std::vector<CVMValue*> CValueVector;


class CVMState
{
public:
    enum {
        MAX_STACK = 64,
        MAX_SCOPE = 32
    };


    struct Frame {
        size_t refCount;
        size_t pc;
        Frame* caller;
        CVMValue* frameValue;
        CVMValue *returnValue, *thisValue, *arguments;
        CVMValue* callable;
        std::stack<CVMValue*> scope;

        Frame() : refCount(1)
        {
            MakeFrame(nullptr, 0, nullptr, nullptr, nullptr);
        }
        Frame(Frame* caller, Frame* frame) : refCount(1)
        {
            MakeFrame(caller, frame ? frame->pc : 0, frame ? frame->thisValue : nullptr, frame ? frame->callable : nullptr, frame ? frame->arguments : nullptr);
        }
        Frame(Frame* caller, const size_t pc, CVMValue* ths, CVMValue* callable, CVMValue* arguments) : refCount(1)
        {
            MakeFrame(caller, pc, ths, callable, arguments);
        }
        ~Frame()
        {
            if (caller) caller->Drop();
            if (returnValue) returnValue->Drop();
            if (thisValue) thisValue->Drop();
            if (arguments) arguments->Drop();
            if (callable) callable->Drop();
            while (scope.size()) {
                scope.top()->Drop();
                scope.pop();
            }
            if (frameValue) frameValue->Drop();
        }

        Frame* Grab()
        {
            refCount++;
            return this;
        }

        Frame* Drop()
        {
            refCount--;
            if (refCount == 0) {
                delete this;
                return nullptr;
            }
            return this;
        }


        void PushScope(CVMValue* value)
        {
            scope.push(value->Grab());
        }

        CVMValue* TopScope()
        {
            return scope.top();
        }

        CVMValue* PopScope()
        {
            assert(scope.size() > 1);
            CVMValue* value = scope.top()->Drop();
            scope.pop();
            return value;
        }

        std::string RandName() const
        {
            char name[17];
            memset(name, 0, sizeof(name));
            GetRandBytes((unsigned char*)name, sizeof(name) - 1);
            return name;
        }

        void MakeFrame(Frame* caller, const size_t pc, CVMValue* thisValue, CVMValue* callable, CVMValue* args)
        {
            assert(callable != nullptr);
            this->frameValue = new CVMValue(callable->Grab(), CVMTypeinfo::FrameType(), RandName());
            this->scope.push(this->frameValue->Grab());
            this->caller = caller ? caller->Grab() : nullptr;
            this->callable = callable->Grab();
            this->pc = pc;
            this->thisValue = this->frameValue->SetKeyValue("this", thisValue ? thisValue : callable);
            this->returnValue = nullptr;
            this->arguments = this->frameValue->SetKeyValue("arguments", args);
        }
    };

    size_t bp, dp;
    std::stack<CVMValue*> stack;
    std::stack<Frame> frame;


    CVMState();
    CVMState(CVMState* state);
    virtual ~CVMState();

    inline void Flush()
    {
        bp = dp = 0;
        while (stack.size())
            stack.pop();
    }

    inline void Push(CVMValue* value)
    {
        assert(stack.size() < MAX_STACK);
        stack.push(value);
    }
    inline CVMValue* Top()
    {
        assert(stack.size() > 0);
        return stack.top();
    }
    inline CVMValue* Pop()
    {
        assert(stack.size() > 0);
        CVMValue* value = stack.top();
        stack.pop();
        return value;
    }
};

CVMValue* VMLoadModule(CVMValue* module, const std::string& name);
CVMValue* VMSaveModule(CVMValue* module, const std::string& name);
CVMValue* VMAddModule(const std::string& name);
CVMValue* VMGetModule(const std::string& name);
bool VMCall(CVMState* state, CVMValue* callable);
bool VMCall(CVMValue* callable);
bool VMRunSource(const std::string& source);
bool VMRunFile(const std::string& file);


#endif