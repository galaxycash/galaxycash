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
#include <cstddef>
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

class CVMDeclare;
class CVMTypeinfo;
class CVMValue;
class CVMVariable;
class CVMCallable;
class CVMModule;
class CVMState;

typedef void (*CVMFunctionPrototype)(CVMState*);

class CVMData : public std::vector<char>
{
public:
    enum {
        DataBytes = 0,
        DataString,
        DataNumber
    };
    uint8_t type;

    CVMData() : type(DataBytes) {}
    CVMData(const CVMData& data) : std::vector<char>(data), type(data.type) {}
};

class CVMDeclare
{
public:
    enum {
        DeclarePrototype = 0,
        DeclareFunction,
        DeclareProperty,
        DeclareScope,
        DeclareVar,
        DeclareConst,
        DeclareLet = DeclareScope,
    };

    uint8_t as;
    std::string name;
    CVMDeclare* base;
    CVMTypeinfo* type;
    CVMModule* module;
    CVMData* data;
    intptr_t value;

    CVMDeclare() : base(nullptr), type(nullptr), as(DeclareStack), module(nullptr), value(nullptr) {}
    CVMDeclare(const CVMDeclare& declare) : base(declare.base), name(declare.name), as(declare.as), type(declare.type), module(declare.module), value(declare.value)
    {
    }
    ~CVMDeclare() {}

    void Rename(const std::string& name, CVMModule* module);


    CVMValue* Exec(CVMValue* state);
};

class CVMModule
{
public:
    std::string name;
    CVMValue* root;
    std::vector<CVMTypeinfo> types;
    std::vector<CVMData> datas;
    std::vector<CVMDeclare> symbols;


    CVMModule();
    CVMModule(const CVMModule& module);
    ~CVMModule();

    bool Link();
    bool Rename(const std::string& name);

    CVMValue* GetRoot();

    bool ExistsType(const std::string& name) const;
    CVMTypeinfo* DeclareType(const std::string& name, const CVMTypeinfo* type = nullptr);
    CVMTypeinfo* GetType(const std::string& name) const;

    bool ExistsVariable(const std::string& name) const;
    CVMDeclare* DeclareVariable(CVMDeclare* base, CVMTypeinfo* type, const std::string& name, uint8_t declare = CVMDeclare::DeclareVar);
    CVMDeclare* GetVariable(const std::string& name) const;

    bool ExistsFunction(const std::string& name) const;
    CVMDeclare* DeclareFunction(const std::string& name);
    CVMDeclare* GetFunction(const std::string& name) const;

    bool ExistsPrototype(const std::string& name) const;
    CVMDeclare* DeclarePrototype(const std::string& name);
    CVMDeclare* GetPrototype(const std::string& name) const;
};

class CVMTypeinfo
{
public:
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
        Flag_Destructor = BIT(19),
        Flag_Element = BIT(20),
        Flag_Literal = BIT(21),
        Flag_Prototype = BIT(22)
    };

    uint32_t version;
    std::string name;
    CVMModule* module;
    size_t index;
    CVMTypeinfo* super;
    Kind kind;
    uint8_t bits; // For numbers
    uint64_t flags;
    std::vector<char> value;
    CVMTypeinfo* ctor;
    CVMTypeinfo* dtor;


    CVMTypeinfo();
    CVMTypeinfo(CVMTypeinfo* type);
    ~CVMTypeinfo();

    CVMTypeinfo& operator=(const CVMTypeinfo& type)
    {
        version = type.version;
        name = type.name;
        module = type.module;
        index = type.index;
        super = type.super;
        kind = type.kind;
        bits = type.bits;
        flags = type.flags;
        value = type.value;
        ctor = type.ctor;
        dtor = type.dtor;
        return *this;
    }


    inline CVMTypeinfo& InsertInt8(int8_t v)
    {
        value.push_back(v);
        return *this;
    }
    inline CVMTypeinfo& InsertUInt8(uint8_t v)
    {
        value.push_back(*(int8_t*)&v);
        return *this;
    }
    inline CVMTypeinfo& InsertInt16(int16_t v)
    {
        int16_t c = htole16(v);
        InsertInt8((int8_t)c);
        InsertInt8((int8_t)(c >> 8));
        return *this;
    }
    inline CVMTypeinfo& InsertUInt16(uint16_t v)
    {
        uint16_t c = htole16(v);
        InsertUInt8((uint8_t)c);
        InsertUInt8((uint8_t)(c >> 8));
        return *this;
    }
    inline CVMTypeinfo& InsertInt32(int32_t v)
    {
        int32_t c = htole32(v);
        InsertInt16((int16_t)c);
        InsertInt16((int16_t)(c >> 16));
        return *this;
    }
    inline CVMTypeinfo& InsertUInt32(uint32_t v)
    {
        uint32_t c = htole32(v);
        InsertUInt16((uint16_t)c);
        InsertUInt16((uint16_t)(c >> 16));
        return *this;
    }
    inline CVMTypeinfo& InsertInt64(int64_t v)
    {
        int64_t c = htole64(v);
        InsertInt32((int32_t)c);
        InsertInt32((int32_t)(c >> 32));
        return *this;
    }
    inline CVMTypeinfo& InsertUInt64(uint64_t v)
    {
        uint64_t c = htole64(v);
        InsertUInt32((uint32_t)c);
        InsertUInt32((uint32_t)(c >> 32));
        return *this;
    }
    inline CVMTypeinfo& InsertFloat(float v)
    {
        return InsertUInt32(*(uint32_t*)&v);
    }
    inline CVMTypeinfo& InsertDouble(double v)
    {
        return InsertUInt64(*(uint64_t*)&v);
    }
    template <typename Type>
    inline CVMTypeinfo& InsertVarInt(Type n)
    {
        unsigned char tmp[(sizeof(n) * 8 + 6) / 7];
        int len = 0;
        while (true) {
            tmp[len] = (n & 0x7F) | (len ? 0x80 : 0x00);
            if (n <= 0x7F)
                break;
            n = (n >> 7) - 1;
            len++;
        }
        do {
            InsertUInt8(tmp[len]);
        } while (len--);
        return *this;
    }
    inline CVMTypeinfo& InsertIntptr(intptr_t v)
    {
        return InsertInt64((int64_t)v);
    }

    int8_t GetInt8(int64_t ofs = 0) const
    {
        return *(value.data() + ofs);
    }
    uint8_t GetUInt8(int64_t ofs = 0) const
    {
        return *((uint8_t*)value.data() + ofs);
    }
    int16_t GetInt16(int64_t ofs = 0) const
    {
        return le16toh(*(int16_t*)(value.data() + ofs));
    }
    uint16_t GetUInt16(int64_t ofs = 0) const
    {
        return le16toh(*(uint16_t*)(value.data() + ofs));
    }
    int32_t GetInt32(int64_t ofs = 0) const
    {
        return le32toh(*(int32_t*)(value.data() + ofs));
    }
    uint32_t GetUInt32(int64_t ofs = 0) const
    {
        return le32toh(*(uint32_t*)(value.data() + ofs));
    }
    int64_t GetInt64(int64_t ofs = 0) const
    {
        return le64toh(*(int64_t*)(value.data() + ofs));
    }
    uint64_t GetUInt64(int64_t ofs = 0) const
    {
        return le64toh(*(uint64_t*)(value.data() + ofs));
    }
    intptr_t GetIntptr(int64_t ofs = 0) const
    {
        return (intptr_t)le64toh(*(int64_t*)(value.data() + ofs));
    }

    template <typename Type>
    inline Type GetVarInt(int64_t ofs = 0) const
    {
        Type n = 0;
        while (true) {
            unsigned char chData = GetUInt8(ofs++);
            if (n > (std::numeric_limits<I>::max() >> 7)) {
                throw std::ios_base::failure("ReadVarInt(): size too large");
            }
            n = (n << 7) | (chData & 0x7F);
            if (chData & 0x80) {
                if (n == std::numeric_limits<I>::max()) {
                    throw std::ios_base::failure("ReadVarInt(): size too large");
                }
                n++;
            } else {
                return n;
            }
        }
    }

    static CVMTypeinfo LiteralArray(const std::vector<CVMValue*>& values);
    static CVMTypeinfo LiteralObject(const std::vector<std::pair<std::string, CVMValue*>>& keys);
    static CVMTypeinfo LiteralBytesVector(const std::vector<char>& bytes);
    static CVMTypeinfo LiteralString(const std::string& value);
    static CVMTypeinfo LiteralBoolean(const bool value);
    static CVMTypeinfo LiteralInt8(const int8_t value);
    static CVMTypeinfo LiteralUInt8(const uint8_t value);
    static CVMTypeinfo LiteralInt16(const int16_t value);
    static CVMTypeinfo LiteralUInt16(const uint16_t value);
    static CVMTypeinfo LiteralInt32(const int32_t value);
    static CVMTypeinfo LiteralUInt32(const uint32_t value);
    static CVMTypeinfo LiteralInt64(const int64_t value);
    static CVMTypeinfo LiteralUInt64(const uint64_t value);
    static CVMTypeinfo LiteralFloat(const float value);
    static CVMTypeinfo LiteralDouble(const double value);

    inline CVMTypeinfo& InsertRawBytes(const char* bytes, int64_t size)
    {
        int64_t ofs = 0;
        while (ofs < size) {
            InsertInt8(*(bytes + ofs));
            ofs++;
        }
        return *this;
    }
    inline CVMTypeinfo& InsertBytes(const char* bytes, int64_t size)
    {
        InsertUInt32(size);
        InsertRawBytes(bytes, size);
        return *this;
    }

    inline CVMTypeinfo& InsertRawByteVector(const std::vector<char>& bytes)
    {
        value.insert(value.end(), bytes.begin(), bytes.end());
        return *this;
    }
    inline CVMTypeinfo& InsertByteVector(const std::vector<char>& bytes)
    {
        InsertUInt32(bytes.size());
        InsertRawByteVector(bytes);
        return *this;
    }

    inline CVMTypeinfo& InsertRawString(const std::string& str)
    {
        value.insert(value.end(), str.begin(), str.end());
        return *this;
    }
    inline CVMTypeinfo& InsertString(const std::string& str)
    {
        InsertUInt32(str.length() + 1);
        InsertRawString(str);
        return *this;
    }

    void SetNull();

    inline const uint64_t Flags() const
    {
        if (super)
            return super->Flags() | flags;
        return flags;
    }
    inline const uint8_t Bits() const
    {
        if (super)
            return std::max(super->Bits(), bits);
        return bits;
    }


    inline bool Extends() const
    {
        return (super != nullptr);
    }

    inline bool HaveConstructor() const
    {
        return (ctor != nullptr);
    }
    inline bool HaveConstructors() const
    {
        return (ctor != nullptr) || (super && super->HaveConstructors());
    }

    inline bool HaveDestructor() const
    {
        return (dtor != nullptr);
    }
    inline bool HaveDestructors() const
    {
        return (dtor != nullptr) || (super && super->HaveDestructors());
    }

    inline bool IsNull() const
    {
        const uint64_t ff = Flags();
        return (kind == CVMTypeinfo::Kind_Null) && !(ff & Flag_Void);
    }

    inline bool IsUndefined() const
    {
        return (kind == CVMTypeinfo::Kind_Undefined);
    }

    inline bool IsVoid() const
    {
        const uint64_t ff = Flags();
        return (kind == CVMTypeinfo::Kind_Null) && (ff & Flag_Void);
    }

    inline bool IsPointer() const
    {
        return (kind == Kind_Pointer);
    }
    inline bool IsString() const
    {
        return (kind == Kind_String);
    }
    inline bool IsSymbol() const
    {
        const uint64_t ff = Flags();
        return (kind == Kind_String) && (ff & Flag_Symbol);
    }
    inline bool IsNumber() const
    {
        return (kind == Kind_Number);
    }
    inline bool IsBoolean() const
    {
        const uint64_t ff = Flags();
        return (kind == Kind_Number) && (ff & Flag_Boolean);
    }
    inline bool IsBignum() const
    {
        const uint64_t ff = Flags();
        return (kind == Kind_Number) && (ff & Flag_Bignum);
    }
    inline bool IsFloat() const
    {
        const uint64_t ff = Flags();
        return (kind == Kind_Number) && (ff & Flag_Float);
    }
    inline bool IsDouble() const
    {
        const uint64_t ff = Flags();
        return (kind == Kind_Number) && (ff & Flag_Double);
    }
    inline bool IsInteger() const
    {
        const uint64_t ff = Flags();
        if (kind != Kind_Number)
            return false;
        else if (ff & Flag_Boolean)
            return false;
        else if (ff & Flag_Float)
            return false;
        else if (ff & Flag_Double)
            return false;
        else if (ff & Flag_Bignum)
            return false;
        else
            return true;
    }
    inline bool IsUnsigned() const
    {
        const uint64_t ff = Flags();
        return (ff & Flag_Unsigned);
    }
    inline bool IsInt8() const
    {
        const uint8_t bb = Bits();
        return IsInteger() && (bb == 8);
    }
    inline bool IsInt16() const
    {
        const uint8_t bb = Bits();
        return IsInteger() && (bb == 16);
    }
    inline bool IsInt32() const
    {
        const uint8_t bb = Bits();
        return IsInteger() && (bb == 32);
    }
    inline bool IsInt64() const
    {
        const uint8_t bb = Bits();
        return IsInteger() && (bb == 64);
    }
    inline bool IsUInt8() const
    {
        const uint8_t bb = Bits();
        return IsInteger() && (bb == 8);
    }
    inline bool IsUInt16() const
    {
        const uint8_t bb = Bits();
        return IsInteger() && IsUnsigned() && (bb == 16);
    }
    inline bool IsUInt32() const
    {
        const uint8_t bb = Bits();
        return IsInteger() && IsUnsigned() && (bb == 32);
    }
    inline bool IsUInt64() const
    {
        const uint8_t bb = Bits();
        return IsInteger() && IsUnsigned() && (bb == 64);
    }
    inline bool IsVariable() const
    {
        return (kind == Kind_Variable);
    }
    inline bool IsProperty() const
    {
        return (kind == Kind_Variable) && (flags & Flag_Property);
    }
    inline bool IsPrimitive() const
    {
        return (IsString() || IsNumber() || IsPointer() || IsLiteral()) || IsVariable();
    }
    inline bool IsObject() const
    {
        return (kind == Kind_Object);
    }
    inline bool IsArray() const
    {
        return (kind == Kind_Array);
    }
    inline bool IsArrayElement() const
    {
        const uint64_t ff = Flags();
        return (kind == Kind_Variable) && (ff & Flag_Element);
    }
    inline bool IsFunction() const
    {
        const uint64_t ff = Flags();
        return (kind == Kind_Callable) && (ff & Flag_Function);
    }
    inline bool IsAsync() const
    {
        const uint64_t ff = Flags();
        return (ff & Flag_Async);
    }
    inline bool IsBlock() const
    {
        const uint64_t ff = Flags();
        return (kind == Kind_Callable) && (ff & Flag_Block);
    }
    inline bool IsFrame() const
    {
        const uint64_t ff = Flags();
        return (kind == Kind_Callable) && (ff & Flag_Frame);
    }
    inline bool IsConstructor() const
    {
        const uint64_t ff = Flags();
        return (kind == Kind_Callable) && (ff & Flag_Constructor);
    }
    inline bool IsDestructor() const
    {
        const uint64_t ff = Flags();
        return (kind == Kind_Callable) && (ff & Flag_Destructor);
    }
    inline bool IsCallable() const
    {
        return (kind == Kind_Callable);
    }
    inline bool IsModule() const
    {
        return (kind == Kind_Module);
    }
    inline bool IsNative() const
    {
        const uint64_t ff = Flags();
        return (ff & Flag_Native);
    }
    inline bool IsLiteral() const
    {
        const uint64_t ff = Flags();
        return (ff & Flag_Literal);
    }

    static CVMTypeinfo* UndefinedType();
    static CVMTypeinfo* NullType();
    static CVMTypeinfo* VoidType();
    static CVMTypeinfo* StringType();
    static CVMTypeinfo* SymbolType();
    static CVMTypeinfo* BooleanType();
    static CVMTypeinfo* IntegerType();
    static CVMTypeinfo* Int8Type();
    static CVMTypeinfo* Int16Type();
    static CVMTypeinfo* Int32Type();
    static CVMTypeinfo* Int64Type();
    static CVMTypeinfo* UInt8Type();
    static CVMTypeinfo* UInt16Type();
    static CVMTypeinfo* UInt32Type();
    static CVMTypeinfo* UInt64Type();
    static CVMTypeinfo* FloatType();
    static CVMTypeinfo* DoubleType();
    static CVMTypeinfo* BignumType();
    static CVMTypeinfo* ObjectType();
    static CVMTypeinfo* ArrayType();
    static CVMTypeinfo* FunctionType();
    static CVMTypeinfo* AsyncFunctionType();
    static CVMTypeinfo* PrototypeType();
    static CVMTypeinfo* FrameType();
    static CVMTypeinfo* BlockType();
    static CVMTypeinfo* ModuleType();
    static CVMTypeinfo* VariableType();
    static CVMTypeinfo* PropertyType();
    static CVMTypeinfo* ElementType();
};

enum CVMOp {
    CVMOp_Nop = 0,
    CVMOp_Declare,
    CVMOp_Push,
    CVMOp_PushScope,
    CVMOp_PushThis,
    CVMOp_PushSuper,
    CVMOp_PushValue,
    CVMOp_PushIndex,
    CVMOp_PushLiteral,
    CVMOp_Pop,
    CVMOp_EnterScope,
    CVMOp_LeaveScope,
    CVMOp_EnterFrame,
    CVMOp_LeaveFrame,
    CVMOp_EnterBlock,
    CVMOp_LeaveBlock,
    CVMOp_Call,
    CVMOp_Syscall,
    CVMOp_New,
    CVMOp_Load,
    CVMOp_Assign,
    CVMOp_Delete,
    CVMOp_Binay,
    CVMOp_Unary,
    CVMOp_Postfix,
    CVMOp_Prefix,
    CVMOp_Bitwise,
    CVMOp_Logical,
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
};

enum CVMUnaryOp {
    CVMUnaryOp_Not = 0, // !v
    CVMUnaryOp_Inv,     // -v
};

enum CVMPrefixOp {
    CVMPrefixOp_Inc = 0, // ++v
    CVMPrefixOp_Dec      // --v
};

enum CVMPostfixOp {
    CVMPostfixOp_Inc = 0, // v++
    CVMPostfixOp_Dec      // v--
};

enum CVMLogicalOp {
    CVMLogicalOp_Equal,          // ==
    CVMLogicalOp_NotEqual,       // !=
    CVMLogicalOp_Less,           // <
    CVMLogicalOp_LessOrEqual,    // <=
    CVMLogicalOp_Greater,        // >
    CVMLogicalOp_GreaterOrEqual, // >=
    CVMLogicalOp_And,            // &&
    CVMLogicalOp_Or,             // ||
};

enum CVMBitwiseOp {
    CVMBitwiseOp_And = 0, // &
    CVMBitwiseOp_Or,      // |
    CVMBitwiseOp_LShift,  // <<
    CVMBitwiseOp_RShift,  // >>
};

enum CVMBinaryOp {
    CVMBinaryOp_Add, // +
    CVMBinaryOp_Sub, // -
    CVMBinaryOp_Mul, // *
    CVMBinaryOp_Div, // /
    CVMBinaryOp_Mod, // %
    CVMBinaryOp_Xor, // ^
};


typedef std::function<void(CVMState*)> CVMFunction;

class CVMValue
{
public:
    CVMValue* root;
    CVMValue* super;
    CVMValue* value;

    std::string name;
    size_t index;
    intptr_t address;
    std::vector<CVMValue*> values;
    std::unordered_map<std::string, CVMValue*> variables;
    CVMTypeinfo* type;
    size_t refCounter;

    CVMValue();
    CVMValue(CVMValue* root, const CVMTypeinfo& type, const std::string& name, const std::vector<char>& data = std::vector<char>());
    CVMValue(CVMValue* value);
    virtual ~CVMValue();

    inline CVMValue* AsSuper() { return super ? super : this; }
    inline CVMModule* AsModule() { return (CVMModule*)address; }
    inline CVMValue* ToValue() const
    {
        if (type == CVMTypeinfo::PropertyType()) return value;
        if (type == CVMTypeinfo::VariableType()) return value;
        return this;
    }

    virtual bool Encode(std::vector<char>& data);
    virtual bool Decode(const std::vector<char>& data);

    virtual void SetNull();

    virtual uint64_t Flags() const { return type->flags; }

    inline bool IsNull() const
    {
        return type->IsNull() && (super && super->IsNull());
    }

    inline bool IsUndefined() const
    {
        return type->IsUndefined() && (super && super->IsUndefined());
    }

    inline bool IsVoid() const
    {
        return type->IsVoid() && (super && super->IsVoid());
    }

    static inline std::string RandName()
    {
        char name[17];
        memset(name, 0, sizeof(name));
        GetRandBytes((unsigned char*)name, sizeof(name) - 1);
        return name;
    }
    inline std::string GetName() const { return name; }
    inline std::string GetFullname() const { return (root) ? (root->GetFullname().empty() ? GetName() : root->GetFullname() + (!GetName().empty() ? "." + GetName() : GetName())) : GetName(); }

    inline bool IsCopable() const { return !type->IsModule(); }

    inline CVMValue* Grab()
    {
        refCounter++;
        return this;
    }
    inline CVMValue* Drop()
    {
        refCounter--;
        if (refCounter <= 0) {
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
            return nullptr;
        return variables[name];
    }

    inline void SetArrayElement(int64_t index, CVMValue* value)
    {
        if (index >= 0) {
            if (index >= values.size()) {
                values.resize(index + 1);
                values[index] = new CVMValue(this, CVMTypeinfo::ElementType(), RandName());
            }
            values[index]->Assign(value);
        }
    }

    inline CVMValue* GetArrayElement(int64_t index)
    {
        if (index > 0 && index < values.size()) return values[index];
        return nullptr;
    }

    inline size_t Bytes() const
    {
        return std::max(super ? super->Bytes() : size_t(0), size_t(type->value.size()));
    }

    inline size_t Bits() const
    {
        return std::max(super ? super->Bits() : size_t(0), size_t(Bits()));
    }

    inline char* Data()
    {
        return type->value.data();
    }
    inline const char* Data() const
    {
        return type->value.data();
    }

    inline std::string AsVariableName()
    {
        return AsSource();
    }

    inline bool AsBoolean() const
    {
        if (type->kind == CVMTypeinfo::Kind_Number) {
            switch (Bits()) {
            case 8: {
                if (type->flags & CVMTypeinfo::Flag_Integer) {
                    if (type->flags & CVMTypeinfo::Flag_Unsigned)
                        return *((uint8_t*)Data()) > 0;
                    else
                        return *((int8_t*)Data()) > 0;
                } else if (type->flags & CVMTypeinfo::Flag_Boolean)
                    return *((uint8_t*)Data()) > 0;
                else
                    return false;
            } break;
            case 16: {
                if (type->flags & CVMTypeinfo::Flag_Integer) {
                    if (type->flags & CVMTypeinfo::Flag_Unsigned)
                        return *((uint16_t*)Data()) > 0;
                    else
                        return *((int16_t*)Data()) > 0;
                } else
                    return false;
            } break;
            case 32: {
                if (type->flags & CVMTypeinfo::Flag_Integer) {
                    if (type->flags & CVMTypeinfo::Flag_Unsigned)
                        return *((uint32_t*)Data()) > 0;
                    else
                        return *((int32_t*)Data()) > 0;
                } else
                    return false;
            } break;
            case 64: {
                if (type->flags & CVMTypeinfo::Flag_Integer) {
                    if (type->flags & CVMTypeinfo::Flag_Unsigned)
                        return *((uint64_t*)Data()) > 0;
                    else
                        return *((int64_t*)Data()) > 0;
                } else
                    return false;
            } break;
            default:
                return false;
            }
        } else if (type->kind == CVMTypeinfo::Kind_Pointer)
            return ((void*)Data()) != nullptr;
        else if (type->kind == CVMTypeinfo::Kind_Variable)
            return (value != nullptr);
        return false;
    }

    inline int8_t AsInt8() const
    {
        if (type->kind == CVMTypeinfo::Kind_Number) {
            switch (Bits()) {
            case 8: {
                if (type->flags & CVMTypeinfo::Flag_Integer) {
                    if (type->flags & CVMTypeinfo::Flag_Unsigned)
                        return (int8_t) * ((uint8_t*)Data());
                    else
                        return *((int8_t*)Data());
                } else if (type->flags & CVMTypeinfo::Flag_Boolean)
                    return *((uint8_t*)Data()) > 0 ? 1 : 0;
                else
                    return 0;
            } break;
            case 16: {
                if (type->flags & CVMTypeinfo::Flag_Integer) {
                    if (type->flags & CVMTypeinfo::Flag_Unsigned)
                        return (int8_t) * ((uint16_t*)Data());
                    else
                        return (int8_t) * ((int16_t*)Data());
                } else
                    return 0;
            } break;
            case 32: {
                if (type->flags & CVMTypeinfo::Flag_Integer) {
                    if (type->flags & CVMTypeinfo::Flag_Unsigned)
                        return (int8_t) * ((uint32_t*)Data());
                    else
                        return (int8_t) * ((int32_t*)Data());
                } else
                    return 0;
            } break;
            case 64: {
                if (type->flags & CVMTypeinfo::Flag_Integer) {
                    if (type->flags & CVMTypeinfo::Flag_Unsigned)
                        return (int8_t) * ((uint64_t*)Data());
                    else
                        return (int8_t) * ((int64_t*)Data());
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
        if (type->kind == CVMTypeinfo::Kind_Number) {
            switch (Bits()) {
            case 8: {
                if (type->flags & CVMTypeinfo::Flag_Integer) {
                    if (type->flags & CVMTypeinfo::Flag_Unsigned)
                        return (int16_t) * ((uint8_t*)Data());
                    else
                        return (int16_t) * ((int8_t*)Data());
                } else if (type->flags & CVMTypeinfo::Flag_Boolean)
                    return *((uint8_t*)Data()) > 0 ? 1 : 0;
                else
                    return 0;
            } break;
            case 16: {
                if (type->flags & CVMTypeinfo::Flag_Integer) {
                    if (type->flags & CVMTypeinfo::Flag_Unsigned)
                        return (int16_t) * ((uint16_t*)Data());
                    else
                        return *((int16_t*)Data());
                } else if (type->flags & CVMTypeinfo::Flag_Float) {
                    return (int16_t) * ((float*)Data());
                } else
                    return 0;
            } break;
            case 32: {
                if (type->flags & CVMTypeinfo::Flag_Integer) {
                    if (type->flags & CVMTypeinfo::Flag_Unsigned)
                        return (int16_t) * ((uint32_t*)Data());
                    else
                        return (int16_t) * ((int32_t*)Data());
                } else if (type->flags & CVMTypeinfo::Flag_Float) {
                    return (int16_t) * ((float*)Data());
                } else
                    return 0;
            } break;
            case 64: {
                if (type->flags & CVMTypeinfo::Flag_Integer) {
                    if (type->flags & CVMTypeinfo::Flag_Unsigned)
                        return (int16_t) * ((uint64_t*)Data());
                    else
                        return (int16_t) * ((int64_t*)Data());
                } else if (type->flags & CVMTypeinfo::Flag_Double) {
                    return (int16_t) * ((double*)Data());
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
        if (type->kind == CVMTypeinfo::Kind_Number) {
            switch (Bits()) {
            case 8: {
                if (type->flags & CVMTypeinfo::Flag_Integer) {
                    if (type->flags & CVMTypeinfo::Flag_Unsigned)
                        return (int32_t) * ((uint8_t*)Data());
                    else
                        return (int32_t) * ((int8_t*)Data());
                } else if (type->flags & CVMTypeinfo::Flag_Boolean)
                    return *((uint8_t*)Data()) > 0 ? 1 : 0;
                else
                    return 0;
            } break;
            case 16: {
                if (type->flags & CVMTypeinfo::Flag_Integer) {
                    if (type->flags & CVMTypeinfo::Flag_Unsigned)
                        return (int32_t) * ((uint16_t*)Data());
                    else
                        return (int32_t) * ((int16_t*)Data());
                } else if (type->flags & CVMTypeinfo::Flag_Float) {
                    return (int32_t) * ((float*)Data());
                } else
                    return 0;
            } break;
            case 32: {
                if (type->flags & CVMTypeinfo::Flag_Integer) {
                    if (type->flags & CVMTypeinfo::Flag_Unsigned)
                        return (int32_t) * ((uint32_t*)Data());
                    else
                        return *((int32_t*)Data());
                } else if (type->flags & CVMTypeinfo::Flag_Float) {
                    return (int32_t) * ((float*)Data());
                } else
                    return 0;
            } break;
            case 64: {
                if (type->flags & CVMTypeinfo::Flag_Integer) {
                    if (type->flags & CVMTypeinfo::Flag_Unsigned)
                        return (int32_t) * ((uint64_t*)Data());
                    else
                        return (int32_t) * ((int64_t*)Data());
                } else if (type->flags & CVMTypeinfo::Flag_Double) {
                    return (int32_t) * ((double*)Data());
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
        if (type->kind == CVMTypeinfo::Kind_Number) {
            switch (Bits()) {
            case 8: {
                if (type->flags & CVMTypeinfo::Flag_Integer) {
                    if (type->flags & CVMTypeinfo::Flag_Unsigned)
                        return (uint32_t) * ((uint8_t*)Data());
                    else
                        return (uint32_t) * ((int8_t*)Data());
                } else if (type->flags & CVMTypeinfo::Flag_Boolean)
                    return *((uint8_t*)Data()) > 0 ? 1 : 0;
                else
                    return 0;
            } break;
            case 16: {
                if (type->flags & CVMTypeinfo::Flag_Integer) {
                    if (type->flags & CVMTypeinfo::Flag_Unsigned)
                        return (uint32_t) * ((uint16_t*)Data());
                    else
                        return (uint32_t) * ((int16_t*)Data());
                } else if (type->flags & CVMTypeinfo::Flag_Float) {
                    return (uint32_t) * ((float*)Data());
                } else
                    return 0;
            } break;
            case 32: {
                if (type->flags & CVMTypeinfo::Flag_Integer) {
                    if (type->flags & CVMTypeinfo::Flag_Unsigned)
                        return *((uint32_t*)Data());
                    else
                        return (uint32_t) * ((int32_t*)Data());
                } else if (type->flags & CVMTypeinfo::Flag_Float) {
                    return (uint32_t) * ((float*)Data());
                } else
                    return 0;
            } break;
            case 64: {
                if (type->flags & CVMTypeinfo::Flag_Integer) {
                    if (type->flags & CVMTypeinfo::Flag_Unsigned)
                        return (uint32_t) * ((uint64_t*)Data());
                    else
                        return (uint32_t) * ((int64_t*)Data());
                } else if (type->flags & CVMTypeinfo::Flag_Double) {
                    return (uint32_t) * ((double*)Data());
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
        if (type->kind == CVMTypeinfo::Kind_Number) {
            switch (Bits()) {
            case 8: {
                if (type->flags & CVMTypeinfo::Flag_Integer) {
                    if (type->flags & CVMTypeinfo::Flag_Unsigned)
                        return (int64_t) * ((uint8_t*)Data());
                    else
                        return (int64_t) * ((int8_t*)Data());
                } else if (type->flags & CVMTypeinfo::Flag_Boolean)
                    return *((uint8_t*)Data()) > 0 ? 1 : 0;
                else
                    return 0;
            } break;
            case 16: {
                if (type->flags & CVMTypeinfo::Flag_Integer) {
                    if (type->flags & CVMTypeinfo::Flag_Unsigned)
                        return (int64_t) * ((uint16_t*)Data());
                    else
                        return (int64_t) * ((int16_t*)Data());
                } else if (type->flags & CVMTypeinfo::Flag_Float) {
                    return (int64_t) * ((float*)Data());
                } else
                    return 0;
            } break;
            case 32: {
                if (type->flags & CVMTypeinfo::Flag_Integer) {
                    if (type->flags & CVMTypeinfo::Flag_Unsigned)
                        return (int64_t) * ((uint32_t*)Data());
                    else
                        return (int64_t) * ((int32_t*)Data());
                } else if (type->flags & CVMTypeinfo::Flag_Float) {
                    return (int64_t) * ((float*)Data());
                } else
                    return 0;
            } break;
            case 64: {
                if (type->flags & CVMTypeinfo::Flag_Integer) {
                    if (type->flags & CVMTypeinfo::Flag_Unsigned)
                        return (int64_t) * ((uint64_t*)Data());
                    else
                        return *((int64_t*)Data());
                } else if (type->flags & CVMTypeinfo::Flag_Double) {
                    return (int64_t) * ((double*)Data());
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
        if (type->kind == CVMTypeinfo::Kind_Number) {
            switch (Bits()) {
            case 8: {
                if (type->flags & CVMTypeinfo::Flag_Integer) {
                    if (type->flags & CVMTypeinfo::Flag_Unsigned)
                        return (uint64_t) * ((uint8_t*)Data());
                    else
                        return (uint64_t) * ((int8_t*)Data());
                } else if (type->flags & CVMTypeinfo::Flag_Boolean)
                    return *((uint8_t*)Data()) > 0 ? 1 : 0;
                else
                    return 0;
            } break;
            case 16: {
                if (type->flags & CVMTypeinfo::Flag_Integer) {
                    if (type->flags & CVMTypeinfo::Flag_Unsigned)
                        return (uint64_t) * ((uint16_t*)Data());
                    else
                        return (uint64_t) * ((int16_t*)Data());
                } else if (type->flags & CVMTypeinfo::Flag_Float) {
                    return (uint64_t) * ((float*)Data());
                } else
                    return 0;
            } break;
            case 32: {
                if (type->flags & CVMTypeinfo::Flag_Integer) {
                    if (type->flags & CVMTypeinfo::Flag_Unsigned)
                        return (uint64_t) * ((uint32_t*)Data());
                    else
                        return (uint64_t) * ((int32_t*)Data());
                } else if (type->flags & CVMTypeinfo::Flag_Float) {
                    return (uint64_t) * ((float*)Data());
                } else
                    return 0;
            } break;
            case 64: {
                if (type->flags & CVMTypeinfo::Flag_Integer) {
                    if (type->flags & CVMTypeinfo::Flag_Unsigned)
                        return *((uint64_t*)Data());
                    else
                        return (uint64_t) * ((int64_t*)Data());
                } else if (type->flags & CVMTypeinfo::Flag_Double) {
                    return (uint64_t) * ((double*)Data());
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
        if (type->kind == CVMTypeinfo::Kind_Number) {
            switch (Bits()) {
            case 8: {
                if (type->flags & CVMTypeinfo::Flag_Integer) {
                    if (type->flags & CVMTypeinfo::Flag_Unsigned)
                        return (float)*((uint8_t*)Data());
                    else
                        return (float)*((int8_t*)Data());
                } else if (type->flags & CVMTypeinfo::Flag_Boolean)
                    return *((uint8_t*)Data()) > 0 ? 1.0f : 0.0f;
                else
                    return 0.0f;
            } break;
            case 16: {
                if (type->flags & CVMTypeinfo::Flag_Integer) {
                    if (type->flags & CVMTypeinfo::Flag_Unsigned)
                        return (float)*((uint16_t*)Data());
                    else
                        return (float)*((int16_t*)Data());
                } else if (type->flags & CVMTypeinfo::Flag_Float) {
                    return *((float*)Data());
                } else
                    return 0.0f;
            } break;
            case 32: {
                if (type->flags & CVMTypeinfo::Flag_Integer) {
                    if (type->flags & CVMTypeinfo::Flag_Unsigned)
                        return (float)*((uint32_t*)Data());
                    else
                        return (float)*((int32_t*)Data());
                } else if (type->flags & CVMTypeinfo::Flag_Float) {
                    return *((float*)Data());
                } else
                    return 0.0f;
            } break;
            case 64: {
                if (type->flags & CVMTypeinfo::Flag_Integer) {
                    if (type->flags & CVMTypeinfo::Flag_Unsigned)
                        return (float)*((uint64_t*)Data());
                    else
                        return (float)*((int64_t*)Data());
                } else if (type->flags & CVMTypeinfo::Flag_Double) {
                    return (float)*((double*)Data());
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
        if (type->kind == CVMTypeinfo::Kind_Number) {
            switch (Bits()) {
            case 8: {
                if (type->flags & CVMTypeinfo::Flag_Integer) {
                    if (type->flags & CVMTypeinfo::Flag_Unsigned)
                        return (double)*((uint8_t*)Data());
                    else
                        return (double)*((int8_t*)Data());
                } else if (type->flags & CVMTypeinfo::Flag_Boolean)
                    return *((uint8_t*)Data()) > 0 ? 1.0 : 0.0;
                else
                    return 0.0;
            } break;
            case 16: {
                if (type->flags & CVMTypeinfo::Flag_Integer) {
                    if (type->flags & CVMTypeinfo::Flag_Unsigned)
                        return (double)*((uint16_t*)Data());
                    else
                        return (double)*((int16_t*)Data());
                } else if (type->flags & CVMTypeinfo::Flag_Float) {
                    return (double)*((float*)Data());
                } else
                    return 0.0;
            } break;
            case 32: {
                if (type->flags & CVMTypeinfo::Flag_Integer) {
                    if (type->flags & CVMTypeinfo::Flag_Unsigned)
                        return (double)*((uint32_t*)Data());
                    else
                        return (double)*((int32_t*)Data());
                } else if (type->flags & CVMTypeinfo::Flag_Float) {
                    return (double)*((float*)Data());
                } else
                    return 0.0;
            } break;
            case 64: {
                if (type->flags & CVMTypeinfo::Flag_Integer) {
                    if (type->flags & CVMTypeinfo::Flag_Unsigned)
                        return (double)*((uint64_t*)Data());
                    else
                        return (double)*((int64_t*)Data());
                } else if (type->flags & CVMTypeinfo::Flag_Double) {
                    return *((double*)Data());
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
        if (type->kind == CVMTypeinfo::Kind_Number) {
            if (type->flags & CVMTypeinfo::Flag_Bignum) {
                return *((uint256*)Data());
            } else
                return uint256(AsUInt64());
        } else if (type->kind == CVMTypeinfo::Kind_String)
            return uint256S(AsString());
        return ret;
    }

    inline std::string AsString() const
    {
        std::string ret;
        if (type->kind == CVMTypeinfo::Kind_String) {
            return std::string(type->value.begin(), type->value.end());
        } else if (type->kind == CVMTypeinfo::Kind_Number) {
            if (type->flags & CVMTypeinfo::Flag_Bignum) return ((uint256*)Data())->GetHex();

            std::ostringstream ss(ret);
            ss.imbue(std::locale::classic());

            switch (Bits()) {
            case 8: {
                if (type->flags & CVMTypeinfo::Flag_Boolean)
                    ss << (*((uint8_t*)Data()) > 0 ? "true" : "false");
                else if (type->flags & CVMTypeinfo::Flag_Unsigned)
                    ss << *((uint8_t*)Data());
                else
                    ss << *((int8_t*)Data());
            } break;
            case 16: {
                if (type->flags & CVMTypeinfo::Flag_Float)
                    ss << *((float*)Data());
                else if (type->flags & CVMTypeinfo::Flag_Unsigned)
                    ss << *((uint16_t*)Data());
                else
                    ss << *((int16_t*)Data());
            } break;
            case 32: {
                if (type->flags & CVMTypeinfo::Flag_Float)
                    ss << *((float*)Data());
                else if (type->flags & CVMTypeinfo::Flag_Unsigned)
                    ss << *((uint32_t*)Data());
                else
                    ss << *((int32_t*)Data());
            } break;
            case 64: {
                if (type->flags & CVMTypeinfo::Flag_Double)
                    ss << *((double*)Data());
                else if (type->flags & CVMTypeinfo::Flag_Unsigned)
                    ss << *((uint64_t*)Data());
                else
                    ss << *((int64_t*)Data());
            } break;
            default:
                ss << 0;
            }
        } else if (type->kind == CVMTypeinfo::Kind_Pointer) {
            std::ostringstream ss(ret);
            ss.imbue(std::locale::classic());
            ss << (void*)Data();
        } else if (type->kind == CVMTypeinfo::Kind_Variable) {
            return GetFullname();
        } else if (type->kind == CVMTypeinfo::Kind_Object)
            return "[object Object]";
        else if (type->kind == CVMTypeinfo::Kind_Array)
            return "[object Array]";
        else if (type->kind == CVMTypeinfo::Kind_Callable && type->flags & CVMTypeinfo::Flag_Function)
            return "[object Callable]";
        else if (type->kind == CVMTypeinfo::Kind_Callable && type->flags & CVMTypeinfo::Flag_Block)
            return "[object Block]";
        else if (type->kind == CVMTypeinfo::Kind_Callable && type->flags & CVMTypeinfo::Flag_Frame)
            return "[object Frame]";
        else if (type->kind == CVMTypeinfo::Kind_Callable)
            return "[object Callable]";
        else if (type->kind == CVMTypeinfo::Kind_Null)
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
        switch (type->kind) {
        case CVMTypeinfo::Kind_Null:
            return "null";
        case CVMTypeinfo::Kind_Undefined:
            return "undefined";
        case CVMTypeinfo::Kind_String:
            return std::string("new ") + (type->flags & CVMTypeinfo::Flag_Symbol ? "Symbol" : "String") + "(\"" + std::string(type->value.begin(), type->value.end()) + "\")";
        case CVMTypeinfo::Kind_Number: {
            if (type->flags & CVMTypeinfo::Flag_Boolean) {
                return AsBoolean() ? "true" : "false";
            } else if (type->flags & CVMTypeinfo::Flag_Integer) {
                return (type->flags & CVMTypeinfo::Flag_Unsigned) ? std::string("new Number(") + std::to_string(AsUInt64()) + ")" : "new Number(" + std::to_string(AsInt64()) + ")";
            } else if (type->flags & CVMTypeinfo::Flag_Float) {
                return std::string("new Number(") + std::to_string(AsFloat()) + ")";
            } else if (type->flags & CVMTypeinfo::Flag_Double) {
                return std::string("new Number(") + std::to_string(AsDouble()) + ")";
            } else if (type->flags & CVMTypeinfo::Flag_Bignum) {
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
            if (type->flags & CVMTypeinfo::Flag_Function) {
                std::string ret = "function()";
                if (type->flags & CVMTypeinfo::Flag_Native)
                    ret += " { [native code] } ";
                else {
                    std::vector<char>::const_iterator it = type->value.begin();
                    while (it < type->value.end())
                        ret += OpcodeAsString(it);
                }
                return ret;
            } else if (type->flags & CVMTypeinfo::Flag_Frame) {
                std::string ret = "";
                if (type->flags & CVMTypeinfo::Flag_Native)
                    ret += " [native code] ";
                else {
                    std::vector<char>::const_iterator it = type->value.begin();
                    while (it < type->value.end())
                        ret += OpcodeAsString(it);
                }
                return ret;
            } else if (type->flags & CVMTypeinfo::Flag_Block) {
                std::string ret = " { ";
                if (type->flags & CVMTypeinfo::Flag_Native)
                    ret += " [native code] ";
                else {
                    std::vector<char>::const_iterator it = type->value.begin();
                    while (it < type->value.end())
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
        if (type->kind == CVMTypeinfo::Kind_String)
            return Bytes() - 1;
        else if (type->kind == CVMTypeinfo::Kind_Array)
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

    struct Block {
        CVMValue* module;
        size_t offset, size;

        Block() : offset(0), size(0) {}
        Block(const Block& block) : offset(block.offset), size(block.size) {}
        Block(const size_t offset, const size_t size) : offset(offset), size(size) {}
    };

    struct Loop {
        Block start, end;
        Block condition, huition;

        Loop() {}
        Loop(size_t address, size_t end) {}
    };

    struct Scope {
        Scope* top;
        CVMValue* value;

        Scope() : top(nullptr), value(nullptr) {}
        Scope(Scope* top, CVMValue* value) : top(top), value(value ? value->Grab() : nullptr) {}
        Scope(Scope& scope) : top(scope.top), value(scope.value ? scope.value->Grab() : nullptr) {}
        ~Scope()
        {
            if (value) value->Drop();
        }
    };

    struct Frame {
        size_t refCount;
        size_t pc;
        Frame* caller;

        CVMValue *value, *thisValue, *superValue, *arguments;
        CVMValue* callable;
        std::stack<Loop> loops;


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
            if (thisValue) thisValue->Drop();
            if (superValue) superValue->Drop();
            if (arguments) arguments->Drop();
            if (callable) callable->Drop();
            if (value) value->Drop();
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

        Loop* GetLoop()
        {
            if (loops.empty()) return nullptr;
            return loops.top();
        }

        Loop* EnterLoop(const Loop& loop)
        {
            loops.push(loop);
            return &loops.top();
        }

        Loop* LeaveLoop()
        {
            if (!loops.empty()) loops.pop();
            return loops.empty() ? nullptr : &loops.top();
        }

        std::string RandName() const
        {
            char name[17];
            memset(name, 0, sizeof(name));
            GetRandBytes((unsigned char*)name, sizeof(name) - 1);
            return name;
        }

        void MakeFrame(Frame* caller, const size_t pc, CVMValue* thisArg, CVMValue* callable, CVMValue* args)
        {
            assert(callable != nullptr);
            this->value = new CVMValue(callable->Grab(), CVMTypeinfo::FrameType(), RandName());
            this->caller = caller ? caller->Grab() : nullptr;
            this->callable = callable->Grab();
            this->pc = pc;
            this->thisValue = this->value->SetKeyValue("this", thisArg ? thisArg : this->callable);
            this->superValue = this->value->SetKeyValue("super", thisArg ? thisArg->AsSuper() : this->callable->AsSuper());
            this->arguments = this->value->SetKeyValue("arguments", (args && args->type.IsArray()) ? args : nullptr);
            this->loops.clear();
        }
    };

    size_t bp, dp;
    std::stack<CVMValue*> stack;
    std::stack<Frame> frames;
    std::stack<Scope> scopes;
    std::stack<CVMValue*> objects, functions, prototypes;


    CVMState();
    CVMState(CVMState* state);
    virtual ~CVMState();

    inline void Flush()
    {
        bp = dp = 0;
        while (stack.size())
            stack.pop();
        while (scopes.size())
            scopes.pop();
        while (frames.size())
            frames.pop();
    }

    inline void EnterScope(CVMValue* newscope)
    {
        scopes.push(Scope(scopes.empty() ? nullptr : &scopes.top(), newscope));
    }

    inline void LeaveScope()
    {
        if (scopes.size() > 1) scopes.pop();
    }

    CVMValue* GetGlobal();
    inline CVMValue* GetFrame()
    {
        return frames.empty() ? GetGlobal() : frames.top()->value;
    }
    inline CVMValue* GetScope()
    {
        return scopes.empty() ? GetFrame()->value : scopes.top()->value;
    }
    inline void PushPrototype(CVMObject* val)
    {
        prototype.push(val);
    }
    inline void PopPrototype() { prototypes.pop(); }
    inline CVMObject* GetPrototype()
    {
        return prototypes.empty() ? nullptr : prototypes.top();
    }
    inline void PushObject(CVMObject* val)
    {
        objects.push(val);
    }
    inline void PopObject() { objects.pop(); }
    inline CVMObject* GetObject()
    {
        return objects.empty() ? nullptr : objects.top();
    }
    inline void PushFunction(CVMObject* val)
    {
        functions.push(val);
    }
    inline void PopFunction() { functions.pop(); }
    inline CVMObject* GetFunction()
    {
        return functions.empty() ? nullptr : functions.top();
    }

    void PushStack();
    void PushNull();
    void PushUndefined();
    void PushTrue();
    void PushFalse();
    void PushIndex(int64_t index);
    void PushValue(const std::string& key);
    void PushSymbol(const std::string& key);
    void PushType();

    inline void Push(CVMValue* value)
    {
        assert(stack.size() < MAX_STACK);
        if (value) value->Grab();
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
        return value ? value->Drop() : nullptr;
    }
};

void* CVMDynlibOpen(const std::string& path);
void CVMDynlibClose(void* dynlib);
void* CVMDynlibAddr(void* dynlib, const std::string& name);


CVMValue* VMLoadModule(CVMValue* module, const std::string& name);
CVMValue* VMSaveModule(CVMValue* module, const std::string& name);
CVMValue* VMAddModule(const std::string& name);
CVMValue* VMGetModule(const std::string& name);
bool VMCall(CVMState* state, CVMValue* callable);
bool VMCall(CVMValue* callable);
bool VMRunSource(const std::string& source);
bool VMRunFile(const std::string& file);

class CJSType
{
public:
    enum {
        Undefined = 0,
        Null,
        String,
        Symbol,
        Number,
        Bigint,
        Boolean,
        Object,
        Reference
    };

    enum {
        Buildin = (1 << 0),
        Primitive = (1 << 1),
        Array = (1 << 2),
        Typed = (1 << 3),
        Callable = (1 << 4),
        Function = (1 << 5),
        Constructor = (1 << 6),
        Destructor = (1 << 7),
        Setter = (1 << 8),
        Getter = (1 << 9),
        Module = (1 << 10),
        Async = (1 << 11),
        Arrow = (1 << 12),
        Constant = (1 << 13),
        Import = (1 << 14),
        Export = (1 << 15),
        Invisible = (1 << 16)
    };

    uint8_t id;
    uint32_t flags;
    std::string name;
    CJSType* super;

    inline CJSType() : id(0), flags(0), super(nullptr) {}
    inline CJSType(const uint8_t id, const uint32_t flags, const std::string& name, const CJSType* super) : id(id), flags(flags), name(name), super(super ? new CJSType(*super) : nullptr) {}
    inline CJSType(const CJSType& type) : id(type.id), flags(type.flags), name(type.name), super(type.super ? new CJSType(*type.super) : nullptr) {}
    inline CJSType(const CJSType* type) : id(type->id), flags(type->flags), name(type->name), super(type->super ? new CJSType(*type->super) : nullptr) {}
    ~CJSType()
    {
        if (super) delete super;
    }

    inline uint32_t GetFlags() const
    {
        return super ? (super->flags | flags) : flags;
    }

    inline CJSType& operator=(const CJSType& type)
    {
        id = type.id;
        flags = type.flags;
        name = type.name;
        if (super) delete super;
        super = type.super ? new CJSType(*type.super) : nullptr;
        return *this;
    }

    inline bool operator<(const CJSType& type) const
    {
        return id < type.id && (name != type.name) && (flags != type.flags);
    }
    inline bool operator==(const CJSType& type) const { return id == type.id && flags == type.flags && name == type.name; }
    inline bool operator!=(const CJSType& type) const { return !(*this == type); }
};

class CJSValue
{
public:
    CJSValue* root;
    uint32_t refs;

    CJSValue();
    CJSValue(CJSValue* value);
    virtual ~CJSValue();

    virtual CJSValue* SetRootValue(CJSValue* root);
    CJSValue* GetRootValue();

    virtual CJSValue* New()
    {
        return new CJSValue();
    }

    virtual CJSValue* Copy()
    {
        return new CJSValue(this);
    }
    virtual CJSValue* GetValue()
    {
        return this;
    }

    virtual CJSValue* Assign(CJSValue* value)
    {
        return this;
    }

    virtual CJSValue* AssignSymbol(const std::string& name, CJSValue* value)
    {
        if (name == "this") Assign(value);
        return this;
    }

    virtual CJSValue* ReplaceSymbol(const std::string& name, CJSValue* value)
    {
        if (name == "__root__") SetRootValue(value);
        return this;
    }

    virtual CJSValue* ResolveSymbol(const std::string& name)
    {
        if (name == "__root__") return root;
        return this;
    }

    virtual const CJSType* GetType() const
    {
        static CJSType type(CJSType::Undefined, 0, "undefined", nullptr);
        return &type;
    }

    inline bool CheckTypeId(const uint8_t id)
    {
        const CJSType* type = GetType();
        while (type) {
            if (type->id == id) return true;
            type = type->super;
        }
        return false;
    }

    inline bool CheckTypeFlags(const uint8_t flags)
    {
        const CJSType* type = GetType();
        while (type) {
            if (type->flags & flags) return true;
            type = type->super;
        }
        return false;
    }

    inline bool IsUndefined() const
    {
        return CheckTypeFlags(CJSType::Undefined);
    }
    inline bool IsNull() const
    {
        return CheckTypeFlags(CJSType::Null);
    }
    inline bool IsUndefinedOrNull() const
    {
        return IsUndefined() || IsNull();
    }

    inline bool IsBuildin() const
    {
        return CheckTypeFlags(CJSType::Buildin);
    }
    inline bool IsPrimitive() const
    {
        return CheckTypeFlags(CJSType::Primitive);
    }
    inline bool IsCallable() const
    {
        return CheckTypeFlags(CJSType::Callable);
    }
    inline bool IsFunction() const
    {
        return CheckTypeFlags(CJSType::Function);
    }
    inline bool IsArrow() const
    {
        return CheckTypeFlags(CJSType::Arrow);
    }
    inline bool IsAsync() const
    {
        return CheckTypeFlags(CJSType::Async);
    }
    inline bool IsConstructor() const
    {
        return CheckTypeFlags(CJSType::Constructor);
    }
    inline bool IsDestructor() const
    {
        return CheckTypeFlags(CJSType::Destructor);
    }
    inline bool IsSetter() const
    {
        return CheckTypeFlags(CJSType::Setter);
    }
    inline bool IsGetter() const
    {
        return CheckTypeFlags(CJSType::Getter);
    }
    inline bool IsModule() const
    {
        return CheckTypeFlags(CJSType::Module);
    }
    inline bool IsArray() const
    {
        return CheckTypeFlags(CJSType::Array);
    }
    inline bool IsObject() const
    {
        return CheckTypeId(CJSType::Object);
    }
    inline bool IsString() const
    {
        return CheckTypeId(CJSType::String);
    }
    inline bool IsSymbol() const
    {
        return CheckTypeId(CJSType::Symbol);
    }
    inline bool IsBigint() const
    {
        return CheckTypeId(CJSType::Bigint);
    }
    inline bool IsBoolean() const
    {
        return CheckTypeId(CJSType::Boolean);
    }

    virtual bool AsBoolean() const { return false; }

    virtual int8_t AsInt8() const { return 0; }
    virtual uint8_t AsUInt8() const { return 0; }
    virtual int16_t AsInt16() const { return 0; }
    virtual uint16_t AsUInt16() const { return 0; }
    virtual int32_t AsInt32() const { return 0; }
    virtual uint32_t AsUInt32() const { return 0; }
    virtual int64_t AsInt64() const { return 0; }
    virtual uint64_t AsUInt64() const { return 0; }

    virtual float AsFloat() const { return 0.0f; }
    virtual double AsDouble() const { return 0.0; }

    virtual std::string AsString() const
    {
        if (IsUndefined()) return "undefined";
        if (IsNull()) return "null";
        return "";
    }

    inline CJSValue* Grab()
    {
        refs++;
        return this
    }
    inline CJSValue* Drop()
    {
        if (refs == 1) {
            refs--;
            delete this;
            return nullptr;
        }
        refs--;
        return this;
    }
};


class CJSNull : public CJSValue
{
public:
    CJSNull();

    virtual const CJSType* GetType() const
    {
        static CJSType type(CJSType::Null, CJSType::Buildin | CJSType::Primitive, "null", nullptr);
        return &type;
    }
};

class CJSNumber : public CJSValue
{
public:
    enum {
        Unsigned = (1 << 4),
        Real = (1 << 5)
    };

    CJSType type;
    uint8_t bits;
    void* mem;

    CJSNumber();
    CJSNumber(CJSValue* value);
    virtual ~CJSNumber();

    virtual CJSValue* New()
    {
        return new CJSNumber();
    }

    virtual CJSValue* Copy()
    {
        return new CJSNumber(this);
    }

    void AllocNumber(const uint8_t bits, const bool isreal = false, const bool isunsigned = false);
    void FreeNumber();

    virtual const CJSType* GetType() const
    {
        return &type;
    }
};

class CJSBoolean : public CJSValue
{
public:
    bool value;

    CJSBoolean();
    CJSBoolean(CJSValue* value);
    virtual ~CJSBoolean();

    virtual CJSValue* New()
    {
        return new CJSBoolean();
    }

    virtual CJSValue* Copy()
    {
        return new CJSBoolean(this);
    }

    virtual const CJSType* GetType() const
    {
        static CJSType type(CJSType::Boolean, CJSType::Buildin | CJSType::Primitive, "boolean", nullptr);
        return &type;
    }
};

class CJSString : public CJSValue
{
public:
    std::string value;

    CJSString();
    CJSString(const std::string& str);
    CJSString(CJSValue* value);
    virtual ~CJSString();

    virtual CJSValue* New()
    {
        return new CJSString();
    }

    virtual CJSValue* Copy()
    {
        return new CJSString(this);
    }

    virtual const CJSType* GetType() const
    {
        static CJSType type(CJSType::String, CJSType::Buildin | CJSType::Primitive, "string", nullptr);
        return &type;
    }
};

class CJSSymbol : public CJSString
{
public:
    CJSSymbol();
    CJSSymbol(const std::string& str);
    CJSSymbol(CJSString* str);
    CJSSymbol(CJSValue* value);
    virtual ~CJSSymbol();

    virtual CJSValue* New()
    {
        return new CJSSymbol();
    }
    virtual CJSValue* Copy()
    {
        return new CJSSymbol(this);
    }

    virtual const CJSType* GetType() const
    {
        static const CJSType* strType = CJSString::GetType();
        static CJSType type(CJSType::Symbol, 0, "symbol", strType);
        return &type;
    }
};

class CJSPropertyDescriptor
{
public:
    enum {
        ENUMERABLE = (1 << 0),
        CONFIGURABLE = (1 << 1),
        SETTER = (1 << 2),
        GETTER = (1 << 3),
        SETTER_GETTER = (SETTER | GETTER),
        VALUE = (1 << 4)
    };

    mutable uint32_t flags;
    mutable CJSValue* value;
    mutable CJSValue* setter;
    mutable CJSValue* getter;

    CJSPropertyDescriptor();
    CJSPropertyDescriptor(const uint32_t flags, CJSValue* value, CJSValue* setter = nullptr, CJSValue* getter = nullptr);
    CJSPropertyDescriptor(const CJSPropertyDescriptor& descriptor);
    ~CJSPropertyDescriptor();
};

class CJSProperty
{
public:
    mutable std::string name;
    mutable CJSPropertyDescriptor descriptor;


    CJSProperty();
    CJSProperty(const std::string& name, const CJSPropertyDescriptor& descriptor);
    CJSProperty(const CJSProperty& property);
    virtual ~CJSProperty();
};

class CJSReference : public CJSValue
{
public:
    CJSValue* value;

    CJSReference();
    CJSReference(CJSValue* value);
    virtual ~CJSReference();

    virtual CJSValue* New()
    {
        return new CJSReference();
    }
    virtual CJSValue* Copy()
    {
        return new CJSReference(this);
    }
    virtual CJSValue* GetValue()
    {
        return value ? value : new CJSNull();
    }

    virtual const CJSType* GetType() const
    {
        static CJSType type(CJSType::Reference, CJSType::Buildin | CJSType::Primitive, "reference", nullptr);
        return &type;
    }
};

class CJSObject : public CJSValue
{
public:
    std::vector<CJSProperty> props;

    CJSObject();
    CJSObject(CJSValue* value);
    virtual ~CJSObject();

    virtual CJSValue* New()
    {
        return new CJSObject();
    }
    virtual CJSValue* Copy()
    {
        return new CJSObject(this);
    }
    virtual const CJSType* GetType() const
    {
        static CJSType type(CJSType::Object, CJSType::Buildin, "object", nullptr);
        return &type;
    }
};


class CJSModule : public CJSObject
{
public:
    CJSModule();
    CJSModule(const std::string& name);
    CJSModule(CJSValue* value);
    virtual ~CJSModule();

    virtual bool LoadModule(const std::string& name);
    virtual bool SaveModule(const std::string& name);

    virtual CJSValue* New()
    {
        return new CJSModule();
    }
    virtual CJSValue* Copy()
    {
        return new CJSModule(this);
    }
    virtual const CJSType* GetType() const
    {
        static CJSType type(CJSType::Object, CJSType::Buildin | CJSType::Module, "object", nullptr);
        return &type;
    }
};

#endif