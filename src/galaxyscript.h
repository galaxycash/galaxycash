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


typedef std::function<void(CVMState*)> CVMNativeFunction;


void* CJSDynlibOpen(const std::string& path);
void CJSDynlibClose(void* dynlib);
void* CJSDynlibAddr(void* dynlib, const std::string& name);

std::string CJSGenName();

class CJSValue;
class CJSNull;
class CJSUndefined;
class CJSPrimitive;
class CJSObject;
class CJSCallable;
class CJSFunction;
class CJSArray;
class CJSString;
class CJSSymbol;
class CJSNumber;
class CJSBigint;
class CJSBoolean;
class CJSReference;
class CJSArray;
class CJSModule;

class CJSType
{
public:
    enum {
        Undefined = 0,
        Null,
        Primitive,
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
        Reserved0 = (1 << 1),
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
    mutable CJSValue* root;
    mutable uint32_t refs;

    CJSValue();
    CJSValue(CJSValue* value);
    virtual ~CJSValue();

    static CJSValue* UndefinedValue();
    static CJSValue* NullValue();
    static CJSValue* ZeroValue();
    static CJSValue* TrueValue();
    static CJSValue* FalseValue();

    virtual CJSValue* SetRootValue(CJSValue* root);
    virtual CJSValue* GetRootValue();

    virtual CJSValue* AsValue() { return this; }
    virtual const CJSValue* AsValue() const { return this; }

    virtual CJSPrimitive* AsPrimitive() { return nullptr; }
    virtual const CJSPrimitive* AsPrimitive() const { return nullptr; }
    virtual bool HasPrimitive() const { return false; }

    virtual CJSObject* AsObject() { return nullptr; }
    virtual const CJSObject* AsObject() const { return nullptr; }
    virtual bool HasObject() const { return false; }

    virtual CJSCallable* AsCallable() { return nullptr; }
    virtual const CJSCallable* AsCallable() const { return nullptr; }
    virtual bool HasCallable() const { return false; }

    virtual CJSFunction* AsFunction() { return nullptr; }
    virtual const CJSFunction* AsFunction() const { return nullptr; }
    virtual bool HasFunction() const { return false; }

    virtual CJSReference* AsReference() { return nullptr; }
    virtual const CJSReference* AsReference() const { return nullptr; }
    virtual bool HasReference() const { return false; }

    virtual CJSModule* AsModule() { return nullptr; }
    virtual const CJSModule* AsModule() const { return nullptr; }
    virtual bool HasModule() const { return false; }

    virtual CJSNumber* AsNumber() { return nullptr; }
    virtual const CJSNumber* AsNumber() const { return nullptr; }
    virtual bool HasNumber() const { return false; }

    virtual CJSBoolean* AsBoolean() { return nullptr; }
    virtual const CJSBoolean* AsBoolean() const { return nullptr; }
    virtual bool HasBoolean() const { return false; }
    virtual bool HasTrue() const { return false; }
    inline bool HasFalse() const { return !HasTrue(); }

    virtual CJSString* AsString() { return nullptr; }
    virtual const CJSString* AsString() const { return nullptr; }
    virtual bool HasString() const { return false; }

    virtual CJSSymbol* AsSymbol() { return nullptr; }
    virtual const CJSSymbol* AsSymbol() const { return nullptr; }
    virtual bool HasSymbol() const { return false; }

    virtual CJSUndefined* AsUndefined() { return nullptr; }
    virtual const CJSUndefined* AsUndefined() const { return nullptr; }
    virtual bool HasUndefined() const { return false; }

    virtual CJSNull* AsNull() { return nullptr; }
    virtual const CJSNull* AsNull() const { return nullptr; }
    virtual bool HasNull() const { return false; }

    virtual CJSBigint* AsBigint() { return nullptr; }
    virtual const CJSBigint* AsBigint() const { return nullptr; }
    virtual bool HasBigint() const { return false; }

    virtual CJSArray* AsArray() { return nullptr; }
    virtual const CJSArray* AsArray() const { return nullptr; }
    virtual bool HasArray() const { return false; }

    virtual bool CanConvertToBoolean() const { return true; }
    virtual bool CanConvertToString() const { return true; }
    virtual bool CanConvertToNumber() const { return true; }

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

    static CJSValue* GlobalScope();

    virtual CJSValue* Scope()
    {
        if (root) {
            CJSValue* tree = root;
            while (tree) {
                if (tree->IsObject() && tree->HasOwnValue(This())) return tree;
                tree = tree->root;
            }
        }
        return GlobalScope();
    }
    virtual CJSValue* ThisScope()
    {
        if (IsObject())
            return This();

        return Scope();
    }

    virtual CJSValue* This() { return this; }
    virtual const CJSValue* This() const { return this; }
    virtual CJSValue* ThisRef() { return Grab(); }
    virtual const CJSValue* ThisRef() const { return Grab(); }
    virtual CJSValue* ValueRef();
    virtual const CJSValue* ValueRef() const;

    virtual bool HasOwnValue(CJSValue* value) const
    {
        if (!value) return false;
        return value == this;
    }

    virtual CJSValue* AssignValue(CJSValue* value);
    virtual CJSValue* AssignSymbol(const std::string& name, CJSValue* value);
    virtual CJSValue* AssignIndex(const int64_t index, CJSValue* value);

    virtual CJSValue* SetSymbol(const std::string& name, CJSValue* value)
    {
        if (name.empty())
            AssignValue(value);
    }
    virtual CJSValue* GetSymbol(const std::string& name)
    {
        if (name.empty())
            return This();
        return NullValue();
    }
    virtual CJSValue* GetSymbolRef(const std::string& name)
    {
        if (name.empty())
            return ValueRef();
        return NullValue();
    }

    virtual CJSValue* SetIndex(const int64_t index, CJSValue* value)
    {
        if (index == 0) AssignValue(value);
        return value ? value : NullValue();
    }
    virtual CJSValue* GetIndex(const int64_t index)
    {
        if (index == 0)
            return This();
        else
            return NullValue();
    }
    virtual CJSValue* GetIndexRef(const int64_t index)
    {
        if (index == 0)
            return ValueRef();
        else
            return NullValue();
    }

    virtual size_t GetLength() const { return 1; }

    virtual const CJSType* Type() const
    {
        return nullptr;
    }

    inline bool CheckTypeId(const uint8_t id, const bool recursive = true) const
    {
        const CJSType* type = Type();
        while (type) {
            if (type->id == id) return true;
            if (!recursive) break;
            type = type->super;
        }
        return false;
    }

    inline bool CheckTypeFlags(const uint32_t flags, const bool recursive = true) const
    {
        const CJSType* type = Type();
        while (type) {
            if (type->flags & flags) return true;
            if (!recursive) break;
            type = type->super;
        }
        return false;
    }

    inline bool IsUndefined() const
    {
        return HasUndefined() || CheckTypeId(CJSType::Undefined, false);
    }
    inline bool IsNull() const
    {
        return HasNull() || CheckTypeId(CJSType::Null, false);
    }
    inline bool IsUndefinedOrNull() const
    {
        return IsUndefined() || IsNull();
    }

    inline bool IsBuildin() const
    {
        return CheckTypeFlags(CJSType::Buildin, false);
    }
    inline bool IsPrimitive() const
    {
        return HasPrimitive() || CheckTypeId(CJSType::Primitive, false);
    }
    inline bool IsCallable() const
    {
        return HasCallable() || (CheckTypeId(CJSType::Object) && CheckTypeFlags(CJSType::Callable));
    }
    inline bool IsFunction() const
    {
        return HasFunction() || (IsCallable() && CheckTypeFlags(CJSType::Function));
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
        return HasModule() || (CheckTypeId(CJSType::Object) && CheckTypeFlags(CJSType::Module));
    }
    inline bool IsArray() const
    {
        return HasArray() || (CheckTypeId(CJSType::Object) && CheckTypeFlags(CJSType::Array));
    }
    inline bool IsObject() const
    {
        return HasObject() || CheckTypeId(CJSType::Object);
    }
    inline bool IsString() const
    {
        return HasString() || CheckTypeId(CJSType::String, false);
    }
    inline bool IsSymbol() const
    {
        return HasSymbol() || CheckTypeId(CJSType::Symbol, false);
    }
    inline bool IsBigint() const
    {
        return HasBigint() || CheckTypeId(CJSType::Bigint, false);
    }
    inline bool IsBoolean() const
    {
        return HasBoolean() || CheckTypeId(CJSType::Boolean, false);
    }

    virtual bool ToBoolean() const { return !IsUndefinedOrNull(); }

    virtual int8_t ToInt8() const { return 0; }
    virtual uint8_t ToUInt8() const { return 0; }
    virtual int16_t ToInt16() const { return 0; }
    virtual uint16_t ToUInt16() const { return 0; }
    virtual int32_t ToInt32() const { return 0; }
    virtual uint32_t ToUInt32() const { return 0; }
    virtual int64_t ToInt64() const { return 0; }
    virtual uint64_t ToUInt64() const { return 0; }

    virtual float ToFloat() const { return 0.0f; }
    virtual double ToDouble() const { return 0.0; }

    virtual std::string ToTypename() const
    {
        std::vector<const CJSType*> tree;
        const CJSType* type = Type();
        if (type && type->super) {
            while (type) {
                tree.push_back(type);
                type = type->super;
            }

            std::string fullname;
            for (size_t i = tree.size() - 1; i >= 0; i--) {
                fullname += tree[i]->name + (i > 0 ? " " : "");
            }
            return fullname;
        }
        return type ? type->name : "undefined";
    }

    virtual std::string ToString() const
    {
        return ToTypename();
    }


    inline CJSValue* Grab()
    {
        refs++;
        return this;
    }

    inline const CJSValue* Grab() const
    {
        refs++;
        return this;
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
    inline const CJSValue* Drop() const
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

class CJSPrimitive : public CJSValue
{
public:
    CJSPrimitive() : CJSValue() {}

    static CJSPrimitive* Static()
    {
        static CJSPrimitive instance;
        static bool initialized = false;

        if (!initialized) {
            CJSValue* gs = GlobalScope();
            gs->SetSymbol("primitive", &instance);
            initialized = true;
        }

        return &instance;
    }
    static bool InitStatic()
    {
        return Static() != nullptr;
    }

    virtual CJSPrimitive* AsPrimitive() { return this; }
    virtual const CJSPrimitive* AsPrimitive() const { return this; }
    virtual bool HasPrimitive() const { return true; }

    virtual CJSValue* New() { return new CJSPrimitive(); }
    virtual CJSValue* Copy() { return new CJSPrimitive(); }

    static const CJSType* StaticType()
    {
        static CJSType type(CJSType::Primitive, CJSType::Buildin, "primitive", nullptr);
        return &type;
    }

    virtual const CJSType* Type() const
    {
        return StaticType();
    }
};

class CJSUndefined : public CJSPrimitive
{
public:
    CJSUndefined() : CJSPrimitive() {}

    static CJSUndefined* Static()
    {
        static CJSUndefined instance;
        static bool initialized = false;

        if (!initialized) {
            CJSValue* gs = GlobalScope();
            gs->SetSymbol("undefined", &instance);
            initialized = true;
        }

        return &instance;
    }
    static bool InitStatic()
    {
        return Static() != nullptr;
    }

    virtual CJSUndefined* AsUndefined() { return this; }
    virtual const CJSUndefined* AsUndefined() const { return this; }
    virtual bool HasUndefined() const { return true; }

    virtual CJSValue* New() { return new CJSUndefined(); }
    virtual CJSValue* Copy() { return new CJSUndefined(); }

    static const CJSType* StaticType()
    {
        static CJSType type(CJSType::Undefined, CJSType::Buildin, "undefined", CJSPrimitive::StaticType());
        return &type;
    }

    virtual const CJSType* Type() const
    {
        return StaticType();
    }
};

class CJSNull : public CJSPrimitive
{
public:
    CJSNull() : CJSPrimitive() {}

    static CJSNull* Static()
    {
        static CJSNull instance;
        static bool initialized = false;

        if (!initialized) {
            CJSValue* gs = GlobalScope();
            gs->SetSymbol("null", &instance);
            initialized = true;
        }

        return &instance;
    }
    static bool InitStatic()
    {
        return Static() != nullptr;
    }

    virtual bool HasNull() const { return true; }
    virtual CJSNull* AsNull() { return this; }
    virtual const CJSNull* AsNull() const { return this; }

    virtual CJSValue* New() { return new CJSNull(); }
    virtual CJSValue* Copy() { return new CJSNull(); }

    static const CJSType* StaticType()
    {
        static CJSType type(CJSType::Null, CJSType::Buildin, "null", CJSPrimitive::StaticType());
        return &type;
    }

    virtual const CJSType* Type() const
    {
        return StaticType();
    }
};

class CJSNumber : public CJSPrimitive
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
    CJSNumber(CJSNumber* value);
    virtual ~CJSNumber();

    static CJSNumber* Static()
    {
        static CJSNumber instance;
        static bool initialized = false;

        if (!initialized) {
            CJSValue* gs = GlobalScope();
            gs->SetSymbol("Number", &instance);
            initialized = true;
        }

        return &instance;
    }
    static bool InitStatic()
    {
        return Static() != nullptr;
    }

    virtual bool HasNumber() const { return true; }
    virtual CJSNumber* AsNumber() { return this; }
    virtual const CJSNumber* AsNumber() const { return this; }

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

    virtual const CJSType* Type() const
    {
        return &type;
    }
};

class CJSBoolean : public CJSPrimitive
{
public:
    bool value;

    CJSBoolean() : CJSPrimitive(), value(false) {}
    CJSBoolean(CJSBoolean* other) : CJSPrimitive(), value(other->value) {}
    virtual ~CJSBoolean() {}

    static CJSBoolean* StaticTrue()
    {
        static CJSBoolean instance;
        static bool initialized = false;

        if (!initialized) {
            instance.value = true;

            CJSValue* gs = GlobalScope();
            gs->SetSymbol("true", &instance);
            initialized = true;
        }

        return &instance;
    }
    static CJSBoolean* StaticFalse()
    {
        static CJSBoolean instance;
        static bool initialized = false;

        if (!initialized) {
            instance.value = false;

            CJSValue* gs = GlobalScope();
            gs->SetSymbol("false", &instance);
            initialized = true;
        }

        return &instance;
    }
    static CJSBoolean* Static()
    {
        static CJSBoolean instance;
        static bool initialized = false;

        if (!initialized) {
            CJSValue* gs = GlobalScope();
            gs->SetSymbol("Boolean", &instance);
            initialized = true;
        }

        return &instance;
    }
    static bool InitStatic()
    {
        StaticTrue();
        StaticFalse();
        return Static() != nullptr;
    }

    virtual bool HasBoolean() const { return true; }
    virtual CJSBoolean* AsBoolean() { return this; }
    virtual const CJSBoolean* AsBoolean() const { return this; }
    virtual bool ToBoolean() const { return value; }
    virtual bool HasTrue() const { return value; }
    virtual bool HasFalse() const { return !value; }

    virtual CJSValue* New()
    {
        return new CJSBoolean();
    }

    virtual CJSValue* Copy()
    {
        return new CJSBoolean(this);
    }

    static const CJSType* StaticType()
    {
        static CJSType type(CJSType::Boolean, CJSType::Buildin, "Boolean", CJSPrimitive::StaticType());
        return &type;
    }
    virtual const CJSType* Type() const
    {
        return StaticType();
    }
};

class CJSString : public CJSPrimitive
{
public:
    std::string value;

    CJSString() : CJSPrimitive() {}
    CJSString(const std::string& str) : CJSPrimitive(), value(str) {}
    CJSString(CJSString* invalue) : CJSPrimitive(), value(invalue ? invalue->value : "") {}
    virtual ~CJSString() {}

    virtual CJSValue* New()
    {
        return new CJSString();
    }

    virtual CJSValue* Copy()
    {
        return new CJSString(this);
    }

    virtual bool HasString() const { return true; }
    virtual CJSString* AsString() { return this; }
    virtual const CJSString* AsString() const { return this; }

    virtual bool CanConvertToBoolean() const
    {
        return value == "0" || value == "1" || value == "true" || value == "false" || value == "null";
    }
    virtual bool CanConvertToString() const { return true; }
    virtual bool CanConvertToNumber() const
    {
        char* p;
        strtol(value.c_str(), &p, 10);
        if (*p != 0) {
            strtof(value.c_str(), &p, 10);
            if (*p != 0) {
                strtod(value.c_str(), &p, 10);
                if (*p != 0) return false;
            }
        }
        return true;
    }

    virtual std::string ToString() const
    {
        return value;
    }

    static const CJSType* StaticType()
    {
        static CJSType type(CJSType::String, CJSType::Buildin, "string", CJSPrimitive::StaticType());
        return &type;
    }

    virtual const CJSType* Type() const
    {
        return StaticType();
    }
};

class CJSSymbol : public CJSString
{
public:
    CJSSymbol() : CJSString() {}
    CJSSymbol(const std::string& str) : CJSString(str) {}
    CJSSymbol(CJSString* str) : CJSString(str) {}
    CJSSymbol(CJSSymbol* value) : CJSString(value) {}
    virtual ~CJSSymbol() {}

    virtual CJSValue* New()
    {
        return new CJSSymbol();
    }
    virtual CJSValue* Copy()
    {
        return new CJSSymbol(this);
    }

    virtual bool HasSymbol() const { return true; }
    virtual CJSSymbol* AsSymbol() { return this; }
    virtual const CJSSymbol* AsSymbol() const { return this; }

    static const CJSType* StaticType()
    {
        static CJSType type(CJSType::Symbol, 0, "symbol", CJSString::StaticType());
        return &type;
    }

    virtual const CJSType* Type() const
    {
        return StaticType();
    }
};

class CJSReference : public CJSPrimitive
{
public:
    std::string name;
    CJSValue* owner;
    CJSValue* value;

    CJSReference() : CJSPrimitive(), name(CJSGenName()), owner(nullptr), value(CJSUndefined::Static()->Grab())
    {
        SetRootValue(CJSValue::GlobalScope());
    }
    CJSReference(const std::string& name, CJSValue* root, CJSValue* owner, CJSValue* value) : CJSPrimitive(), name(name), owner(owner ? owner->Grab() : nullptr), value(value ? value->Grab() : CJSUndefined::Static()->Grab())
    {
        SetRootValue(root ? root : CJSValue::GlobalScope());
    }
    CJSReference(CJSReference* value) : CJSPrimitive(), name(value->name), owner(value->owner ? value->owner->Grab() : nullptr), value(value->value ? value->value->Grab() : CJSUndefined::Static()->Grab())
    {
        SetRootValue(CJSValue::GlobalScope());
    }
    virtual ~CJSReference()
    {
        if (owner) owner->Drop();
        if (value) value->Drop();
    }

    virtual CJSValue* ThisScope()
    {
        if (owner)
            return owner->ThisScope();
        else
            return CJSValue::ThisScope();
    }

    virtual bool HasReference() const { return true; }
    virtual CJSReference* AsReference() { return this; }
    virtual const CJSReference* AsReference() const { return this; }

    virtual CJSValue* New()
    {
        return new CJSReference();
    }
    virtual CJSValue* Copy()
    {
        return new CJSReference(this);
    }
    virtual CJSValue* GetValue();

    static const CJSType* StaticType()
    {
        static CJSType type(CJSType::Reference, CJSType::Buildin, "reference", CJSPrimitive::StaticType());
        return &type;
    }
    virtual const CJSType* Type() const
    {
        return StaticType();
    }
};

class CJSPropertyDescriptor
{
public:
    enum {
        Enumerable = (1 << 0),
        Configurable = (1 << 1),
        Setter = (1 << 2),
        Getter = (1 << 3),
        SetterAndGetter = (Setter | Getter),
        Value = (1 << 4)
    };

    mutable uint32_t flags;
    mutable CJSValue* owner;
    mutable CJSValue* value;
    mutable CJSValue* setter;
    mutable CJSValue* getter;
    mutable CJSValue* initial;

    inline CJSPropertyDescriptor() : flags(0), owner(nullptr), value(new CJSReference()), setter(nullptr), getter(nullptr), initial(nullptr)
    {
        value->AssignValue(CJSUndefined::Static());
    }
    inline CJSPropertyDescriptor(const uint32_t flags, CJSValue* owner = nullptr, CJSValue* value = nullptr, CJSValue* setter = nullptr, CJSValue* getter = nullptr, CJSValue* initializer = nullptr) : flags(flags), owner(owner), value(new CJSReference(owner, owner, value)), setter(setter), getter(getter), initial(initializer)
    {
    }
    inline CJSPropertyDescriptor(const CJSPropertyDescriptor& descriptor) : flags(descriptor.flags), value(descriptor.value), setter(descriptor.setter), getter(descriptor.getter), initial(descriptor.initial) {}
    inline ~CJSPropertyDescriptor() {}
};

class CJSProperty
{
public:
    mutable std::string name;
    mutable CJSPropertyDescriptor descriptor;


    inline CJSProperty() {}
    inline CJSProperty(const std::string& name, const CJSPropertyDescriptor& descriptor) : name(name), descriptor(descriptor) {}
    inline CJSProperty(const CJSProperty& property) : name(property.name), descriptor(property.descriptor) {}
    inline ~CJSProperty()
    {
    }
};


class CJSObject : public CJSValue
{
public:
    std::vector<CJSProperty> props;

    CJSObject();
    CJSObject(CJSValue* value);
    virtual ~CJSObject();

    virtual CJSObject* AsObject() { return this; }
    virtual const CJSObject* AsObject() const { return this; }

    virtual CJSReference* NewReference(const std::string& name);

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

    bool HasOwnProperty(const std::string& name) const;
    void SetOwnProperty(const std::string& name, const CJSProperty& prop);
    void SetOwnPropertyValue(const std::string& name, CJSValue* value);
    CJSProperty* GetOwnProperty(const std::string& name) const;
    CJSValue* GetOwnPropertyValue(const std::string& name) const;
    CJSProperty* AddOwnProperty(const std::string& name, const CJSPropertyDescriptor& descriptor);
};

class CJSOpcode
{
public:
    enum {
        Nope = 0,
        Push,
        PushThis,
        PushSuper,
        PushArgumentI,
        PushArgumentN,
        PushIndex,
        PushProperty,
        Pop,
        Ret,
        Store,
        Assign,
        StoreIndex,
        StoreProperty,
        AssignIndex,
        AssignProperty,
        Call,
        SysCall,
        IfElse,
        For,
        While,
        Break,
        Continue,
        Switch,
    };
    uint16_t id;
    uint16_t modifier;
    std::vector<uint8_t> data;

    inline CJSOpcode() : id(0), modifier(0) {}
    inline CJSOpcode(const uint16_t id, const uint16_t modifier, const std::vector<uint8_t>& data) : id(id), modifier(modifier), data(data) {}
    inline CJSOpcode(const CJSOpcode& opcode) : id(opcode.id), modifier(opcode.modifier), data(opcode.data) {}
};

class CJSCallable : public CJSObject
{
public:
    std::vector<CJSOpcode> opcodes;

    CJSCallable();
    CJSCallable(CJSCallable* callable);
    virtual ~CJSCallable();

    virtual CJSCallable* AsCallable() { return this; }
};

class CJSCallFrame
{
public:
    CJSCallFrame* caller;
    int callid;
    CJSCallable* callable;
    size_t pc;
    CJSObject* thisArg;
    std::vector<CJSObject*> args;
};

class CJSFunction : public CJSCallable
{
public:
    CJSFunction();
    CJSFunction(CJSFunction* other);
    virtual ~CJSFunction();

    bool IsAsyncFunction() const
    {
        return (GetType()->flags & CJSType::Async);
    }

    bool IsArrowFunction() const
    {
        return (GetType()->flags & CJSType::Arrow);
    }
};

class CJSModuleDeclare
{
public:
    enum {
        None = 0,
        Variable = 0,
        Function,
        Prototype
    };

    enum {
        NonWritable = (1 << 0),
        Constant = (1 << 1),
        Static = (1 << 2),
        Tempornary = (1 << 3),
        Unnamed = (1 << 4)
    };

    mutable uint8_t type;
    mutable uint32_t flags;
    mutable CJSProperty* location;

    CJSModuleDeclare() : type(None), flags(0), location(nullptr) {}
    CJSModuleDeclare(const CJSModuleDeclare& declare) : type(declare.type), flags(declare.flags), location(declare.location) {}
};

class CJSModule : public CJSObject
{
public:
    enum {
        ModuleDefault,
        ModuleNative,
        ModuleExecutable
    };

    std::string name;
    uint32_t minor, major;
    uint8_t typeOfModule;
    bool initialized, loaded;


    CJSModule();
    CJSModule(const std::string& name);
    CJSModule(CJSModule* module);
    virtual ~CJSModule();

    bool IsEmptyModule() const;
    bool IsBadModule() const;
    inline bool IsGoodModule() const { return !IsBadModule(); }

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
        static CJSType type(CJSType::Object, CJSType::Buildin | CJSType::Module, "module", CJSObject::GetType());
        return &type;
    }

    bool HasOwnVariable(const std::string& name) const;
    CJSValue* GetVariable(const std::string& name) const;
    CJSReference* GetVariableRef(const std::string& name) const;

    bool HasOwnFunction(const std::string& name) const;
    CJSFunction* GetFunction(const std::string& name) const;
    CJSReference* GetFunctionRef(const std::string& name) const;

    bool HasOwnPrototype(const std::string& name) const;
    CJSObject* GetPrototype(const std::string& name) const;
    CJSReference* GetPrototypeRef(const std::string& name) const;
    CJSObject* NewObject(const std::string& prototype) const;

    bool HasExecutable() const;
    CJSFunction* GetExecutable() const;

    bool Exec();
};

#endif