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
#include <unordered_map>
#include <vector>

// GalaxyCash Scripting engine


enum CScriptTypes {
    TYPE_UNDEFINED = 0,
    TYPE_VARIABLE,
    TYPE_NULL,
    TYPE_VOID,
    TYPE_POINTER,
    TYPE_PROTOTYPE,
    TYPE_OBJECT,
    TYPE_ARRAY,
    TYPE_CLASS,
    TYPE_STRING,
    TYPE_NUMBER,
    TYPE_CALLABLE,
    TYPE_FUNCTION,
    TYPE_CONSTRUCTOR,
    TYPE_DESTRUCTOR,
    TYPE_METHOD,
    TYPE_OPERATOR,
    TYPE_SETTER,
    TYPE_GETTER
};

std::string TypeNames[] = {
    "undefined",
    "variable"
    "null",
    "void",
    "pointer",
    "prototype",
    "object",
    "array",
    "class",
    "string"
    "number",
    "boolean",
    "integer",
    "float",
    "bignum",
    "callable",
    "function",
    "constructor",
    "destructor",
    "method",
    "operator",
    "setter",
    "getter"};


enum CScriptValueFlags {
    TYPE_BUILDIN = (1 << 0),
    TYPE_IMPORT = (1 << 1),
    TYPE_EXPORT = (1 << 2),
    TYPE_PRIMITIVE = (1 << 3),
    TYPE_CONSTANT = (1 << 4),
    TYPE_GLOBAL = (1 << 5),
    TYPE_LOCAL = (1 << 6),
    TYPE_NATIVE = (1 << 7),
    TYPE_PRIVATE = (1 << 8),
    TYPE_UNSIGNED = (1 << 9),
    TYPE_ARGUMENTS = (1 << 10),
    TYPE_THIS = (1 << 11),
    TYPE_SUPER = (1 << 12),
    TYPE_READ = (1 << 13),
    TYPE_WRITE = (1 << 14),
    TYPE_CONFIGURABLE = (1 << 15),
    TYPE_ENUMERABLE = (1 << 16),
    TYPE_FLOAT = (1 << 17),
    TYPE_BIGNUM = (1 << 18),
    TYPE_BOOLEAN = (1 << 19),
    TYPE_UNICODE = (1 << 20)
};


class CScriptModule;
typedef std::shared_ptr<CScriptModule> CScriptModuleRef;

class CScriptValue;
typedef std::shared_ptr<CScriptValue> CScriptValueRef;

class CScriptLiteral;
typedef std::shared_ptr<CScriptLiteral> CScriptLiteralRef;

class CScriptString;
typedef std::shared_ptr<CScriptString> CScriptStringRef;

class CScriptNumber;
typedef std::shared_ptr<CScriptNumber> CScriptNumberRef;

class CScriptPointer;
typedef std::shared_ptr<CScriptPointer> CScriptPointerRef;

class CScriptVariable;
typedef std::shared_ptr<CScriptVariable> CScriptVariableRef;

class CScriptProperty;
typedef std::shared_ptr<CScriptProperty> CScriptPropertyRef;

class CScriptPrototype;
typedef std::shared_ptr<CScriptPrototype> CScriptPrototypeRef;

class CScriptCallable;
typedef std::shared_ptr<CScriptCallable> CScriptCallableRef;

class CScriptFunction;
typedef std::shared_ptr<CScriptFunction> CScriptFunctionRef;

class CScriptObject;
typedef std::shared_ptr<CScriptObject> CScriptObjectRef;

class CScriptClass;
typedef std::shared_ptr<CScriptClass> CScriptClasstRef;

class CScriptArray;
typedef std::shared_ptr<CScriptArray> CScriptArrayRef;

class CScriptContext
{
public:
    mutable std::vector<CScriptValueRef> mempool;
    mutable std::unordered_map<uint256, CScriptModuleRef> modules;
    mutable std::unordered_map<uint256, CScriptValueRef> values;
    mutable std::unordered_map<std::string, uint256> names;
    mutable std::unordered_map<int64_t, uint256> integerConstants;
    mutable std::unordered_map<double, uint256> doubleConstants;
    mutable std::unordered_map<std::string, uint256> stringConstants;
    mutable std::unordered_map<void*, uint256> pointerConstants;

    CScriptContext();
    virtual ~CScriptContext();


    inline uint256 combine(const uint256& a, const uint256& b)
    {
        uint256 buf;
        CSHA256().Write(a.begin(), sizeof(a)).Write(b.begin(), sizeof(b)).Finalize(buf.begin());
        return buf;
    }

    virtual CScriptValueRef NewVariable(const std::string& name, const CScripValueRef& value);
    virtual CScriptValueRef NewVariable(const CScriptValueRef& name, const CScripValueRef& value);

    virtual bool Exists(const std::string& name) const;
    virtual uint256 UniqueID(const std::string& name);
    virtual bool Exists(const uint256& uuid) const;
    virtual std::string Name(const uint256& uuid);
    virtual CScriptValueRef Value(const uint256& uuid);
    virtual CScriptModuleRef Module(const uint256& uuid);
    virtual CScriptModuleRef Import(const std::string& name);
    virtual void Export(const CScriptModuleRef& module, const std::string& name);
    virtual CScriptModuleRef Scope() const;
    virtual CScriptValueRef Void() const;
    virtual CScriptValueRef Null() const;
    virtual CScriptValueRef Zero() const;
    virtual CScriptValueRef One() const;
    virtual CScriptValueRef ConstantInteger(int64_t value) const;
};
typedef std::shared_ptr<CScriptContext> CScriptContextRef;


class CScriptValue : public std::enable_shared_from_this<CScriptValue>
{
public:
    typedef std::shared_ptr<CScriptValue> Ref;


    CScriptModuleRef module;
    CScriptValueRef scope;
    uint256 uuid;

    std::vector<char> data;
    int32_t flags;

    CScriptValue() : flags(0), uuid(GenUUID()) {}
    CScriptValue(const int32_t flags) : flags(flags), uuid(GenUUID()) {}
    CScriptValue(const Ref& value, const int32_t flags) : flags(value->flags | flags), uuid(value->GenUUID())
    {
    }
    virtual ~CScriptValue() {}

    static uint256 GenUUID()
    {
        return GetRandHash();
    }

    static std::string GenName()
    {
        return GenUUID();
    }

    const uint256& UniqueID() const
    {
        return uuid;
    }


    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(context->NameOfModule(module));
        std::vector<char> buffer;
        if (ser_action.ForRead()) {
            READWRITE(buffer);
            SerializeValue(buffer);
        } else {
            UnserializeValue(buffer);
            READWRITE(buffer);
        }
        READWRITE(data);
        READWRITE(uuid);
        READWRITE(flags);
        READWRITE(scope);
    }

    virtual void SerializeValue(std::vector<char>& buffer)
    {
    }
    virtual void UnserializeValue(std::vector<char>& buffer)
    {
    }

    bool isVariable() const { return false; }
    bool isValue() const { return true; }
    bool isPrototype() const { return (Type() == TYPE_PROTOTYPE); }
    bool isUndefined() const { return (Type() == TYPE_UNDEFINED); }
    bool isPrimitive() const { return (IsLitteral() || IsVoid() || IsNull()); }
    bool isNull() const { return (Type() == TYPE_NULL); }
    bool isVoid() const { return (Type() == TYPE_VOID); }
    bool isBoolean() const { return isNumber() && (NumberType() == NUMBER_BOOLEAN); }
    bool isInteger() const { return isNumber() && (NumberType() == NUMBER_INTEGER); }
    bool isFloat() const { return isNumber() && (NumberType() == NUMBER_FLOAT); }
    bool isNumber() const { return (Type() == TYPE_NUMBER); }
    bool isString() const { return (Type() == TYPE_STRING); }
    bool isPointer() const { return (Type() == TYPE_POINTER); }
    bool isArray() const { return (Type() == TYPE_ARRAY); }
    bool isObject() const { return (Type() == TYPE_OBJECT); }
    bool isConstant() const { return (flags & TYPE_CONSTANT); }
    bool isVisible() const { return (flags & TYPE_VISIBLITY); }
    bool isNative() const { return (flags & TYPE_NATIVE); }
    bool isCallable() const { return (Type() == TYPE_CALLABLE); }
    bool isFunction() const { return (Type() == TYPE_FUNCTION); }
    bool isImport() const { return (flags & TYPE_IMPORT); }
    bool isExport() const { return (flags & TYPE_EXPORT); }
    bool isReadable() const { return (flags & TYPE_READ); }
    bool isWritable() const { return (flags & TYPE_WRITE); }
    bool isConfigurable() const { return (flags & TYPE_CONFIGURABLE); }
    bool isEnumerable() const { return (flags & TYPE_ENUMERABLE); }
    bool isModule() const { return (flags & TYPE_MODULE); }
    bool isPubkey() const { return (Type() == TYPE_PUBKEY); }
    bool isPrivkey() const { return (Type() == TYPE_PRIVKEY); }
    bool isWifkey() const { return (Type() == TYPE_WIFKEY); }
    bool isAddress() const { return (Type() == TYPE_ADDRESS); }
    bool isTransaction() const { return (Type() == TYPE_TRANSACTION); }
    bool isBlock() const { return (Type() == TYPE_BLOCK); }
    bool isChain() const { return (Type() == TYPE_CHAIN); }
    bool isBignum() const { return (NumberType() == NUMBER_BIGNUM); }

    virtual int SymbolType() const { return SYMBOL_UNDEFINED; }
    virtual std::string SymbolTypeName() const { return SymbolTypeNames[SymbolType()]; }

    virtual int Type() const { return TYPE_NULL; }
    virtual std::string ValueTypeName() { return ValueTypeNames[Type()]; }

    virtual int NumberType() const { return NUMBER_INTEGER; }
    virtual std::string NumberTypeName() { return NumberTypeNames[NumberType()]; }


    virtual void FromValue(const Ref& value) {}
    virtual Ref AsValue() { return shared_from_this(); }
    virtual Ref AsValue() const { return shared_from_this(); }

    virtual Ref ValueByName(const std::string& id) const
    {
        return valueScope != nullptr ? valueModule->ValueByName(id) : MakeNull();
    }

    virtual Ref ValueByUniqueID(const std::string& id) const
    {
        return valueScope != nullptr ? valueModule->ValueUniqueID(id) : MakeNull();
    }

    virtual Ref Clone()
    {
        return std::make_shared<CScriptValue>(*this);
    }

    virtual void SetPointer(const void* p) {}
    virtual void* AsPointer() const { return nullptr; }

    virtual void SetBoolean(const bool value) {}
    virtual bool AsBoolean() const { return false; }

    virtual void SetInteger(const int64_t value);
    virtual int64_t AsInteger() const { return 0; }

    virtual void SetFloat(const double value) {}
    virtual double AsFloat() const { return 0.0; }

    virtual void SetString(const std::string& value) {}
    virtual std::string AsString() const { return ""; }

    virtual void SetBignum(const uint256& bignum) {}
    virtual uint256 AsBignum() const { return uint256(); }


    static Ref Parse(const Ref& value, const Ref& replacer);
    static Ref Stringify(const Ref& value, const Ref& replacer, const Ref& space);


    static Ref MakeVariable(const Ref& name, const Ref& value, int32_t flags = 0);
    static Ref MakeVariable(const std::string& name, const Ref& value, int32_t flags = 0);
    static Ref MakeUndefined();
    static Ref MakeNull() { return std::make_shared<CScriptValue>(); }
    static Ref MakeVoid();
    static Ref MakeBoolean(const bool value, int32_t flags = 0) { return std::make_shared<CScriptValue>(); }
    static Ref MakeInteger(const int64_t value, int32_t flags = 0) { return std::make_shared<CScriptValue>(); }
    static Ref MakeFloat(const double value, int32_t flags = 0) { return std::make_shared<CScriptValue>(); }
    static Ref MakeString(const std::string& value, int32_t flags = 0);
    static Ref MakePointer(void* p) { return std::make_shared<CScriptValue>(); }
    static Ref MakeArray() { return std::make_shared<CScriptValue>(); }
    static Ref MakeObject();
    static Ref MakePubkey() { return std::make_shared<CScriptValue>(); }
    static Ref MakePrivkey() { return std::make_shared<CScriptValue>(); }
    static Ref MakeWifkey() { return std::make_shared<CScriptValue>(); }
    static Ref MakeAddress() { return std::make_shared<CScriptValue>(); }
    static Ref MakeTransaction() { return std::make_shared<CScriptValue>(); }
    static Ref MakeBlock() { return std::make_shared<CScriptValue>(); }
    static Ref MakeChain() { return std::make_shared<CScriptValue>(); }
};

class CScriptLinkage : public CScriptValue
{
public:
    uint256 uuid;
    CScriptContextRef context;
    CScriptModuleRef module;
    CScriptValueRef value;

    CScriptLinkage() {}
    CScriptLinkage(const uint256& uuid, const CScriptContextRef& context, const CScriptModuleRef& module, const CScriptValueRef& value) : uuid(uuid), context(context), module(module), value(value)
    {
    }
    virtual int SymbolType() const { return value->SymbolType(); }
    virtual int Type() const { return value->Type(); }
    virtual int NumberType() const { return value->NumberType(); }

    virtual void SetValue(const CScriptValueRef& value) { this->value->SetValue(value); }
    virtual CScriptValueRef AsValue() { return value; }
    virtual CScriptValueRef AsValue() const { return value; }
};

class CScriptValueUndefined : public CScriptValue
{
public:
    virtual int SymbolType() const { return SYMBOL_UNDEFINED; }
    virtual int Type() const { return TYPE_NULL; }


    static CScriptValueRef Make() { return std::make_shared<CScriptValueUndefined>(); }
};

CScriptValueRef CScriptValue::MakeUndefined() { return CScriptValueUndefined::Make(); }

class CScriptValueVoid : public CScriptValue
{
public:
    virtual int Type() const { return TYPE_VOID; }
    static CScriptValueRef Make() { return std::make_shared<CScriptValueVoid>(); }
};
CScriptValueRef CScriptValue::MakeVoid() { return CScriptValueVoid::Make(); }

class CScriptString : public CScriptValue
{
public:
    std::string value;

    CScriptString(const std::string& value, int32_t flags) : CScriptValue(flags), value(value) {}
    virtual int Type() const { return TYPE_VOID; }
    static CScriptValueRef Make(const std::string& value, int32_t flags = 0) { return std::make_shared<CScriptString>(value, flags); }
};
CScriptValueRef CScriptValue::MakeString(const std::string& value, int32_t flags) { return CScriptString::Make(value, flags); }

class CScriptVariable : public CScriptValue
{
public:
    typedef std::shared_ptr<CScriptVariable> Ref;

    std::string name, valueUUID;
    CScriptValueRef value;

    CScriptVariable() : CScriptValue(), name(GenName()), value(CScriptValue::MakeNull()) {}
    CScriptVariable(const CScriptVariableRef variable, int32_t flags = 0) : name(variable->name), CScriptValue(variable, flags), value(variable->value == nullptr ? CScriptValue::MakeNull() : variable->value)
    {
    }
    CScriptVariable(const std::string& name, const CScriptValueRef& value, int32_t flags = 0) : CScriptValue(flags), name(name.empty() ? GenName() : name), value(value) {}
    virtual ~CScriptVariable() {}

    virtual void AssignVariable(const uint256& uuid)
    {
        if (valueScope) {
            valueUUID = uuid;
            value = valueScope->valueByUUID(uuid);
        }
    }

    virtual void SerializeValue(std::vector<char>& buffer)
    {
        CDataStream ss(buffer.begin(), buffer.end(), SER_DISK, PROTOCOL_VERSION);
        ss >> valueUUID;
        ss >> name;

        AssignVariable(valueUUID, value);
    }

    virtual void UnserializeValue(std::vector<char>& buffer)
    {
        CDataStream ss(SER_DISK, PROTOCOL_VERSION);
        ss << value != nullptr ? value->UniqueID() : "nullnullnullnull";
        ss << name;
    }

    virtual int Type() const { return SYMBOL_VARIABLE; }

    virtual int Type() const { return (value != nullptr) ? value->Type() : TYPE_NULL; }
    virtual std::string ValueTypeName() const { return ValueTypeNames[Type()]; }

    virtual void FromValue(const CScriptValueRef& value)
    {
        value->FromValue(value);
    }
    virtual CScriptValueRef AsValue() const { return value != nullptr ? value->AsValue() : CScriptValue::MakeNull(); }

    virtual Ref Clone()
    {
        return std::make_shared<CScriptVariable>(shared_from_this(), 0);
    }


    virtual bool isVariable() const { return true; }

    static CScriptValueRef Make(const std::string& name, const CScriptValueRef& value, int32_t flags = 0)
    {
        if (name.length() == 0) return CScriptValue::MakeNull();
        return std::make_shared<CScriptVariable>(name, value, flags);
    }
    static CScriptValueRef Make(const CScriptValueRef& name, const CScriptValueRef& value, int32_t flags = 0)
    {
        if (name == nullptr) return CScriptValue::MakeNull();
        std::string str = name->AsString();
        if (str.length() == 0) return CScriptValue::MakeNull();
        return std::make_shared<CScriptVariable>(str, value, flags);
    }
};

CScriptValueRef CScriptValue::MakeVariable(const CScriptValueRef& name, const CScriptValueRef& value, int32_t flags) { return CScriptVariable::Make(name, value, flags); }
CScriptValueRef CScriptValue::MakeVariable(const std::string& name, const CScriptValueRef& value, int32_t flags) { return CScriptVariable::Make(name, value, flags); }

class CScriptLitteral : public CScriptValue
{
public:
    uint8_t type;

    CScriptLitteral() : CScriptValue() {}
    CScriptLitteral(const uint8_t type, const int32_t flags) : type(type), CScriptValue(flags) {}
};

class CScriptSetter : public CScriptCallable
{
public:
    CScriptValueRef object;
    CScriptValueRef property;
    CScriptValueRef callable;

    CScriptSetter() : CScriptCallable(TYPE_SETTER) {}

    virtual int Type() const { return TYPE_CALLABLE; }

    virtual bool isSetter() const { return true; }
};

class CScriptProperty : public CScriptValue
{
public:
    typedef std::shared_ptr<CScriptProperty> Ref;

    CScriptValueRef name;
    CScriptValueRef value;
    CScriptValueRef setter, getter;

    CScriptProperty() : name(GenName()), value(MakeNull()), CScriptValue()
    {
    }
};

class CScriptPrototype : public CScriptValue
{
public:
    typedef std::shared_ptr<CScriptPrototype> Ref;
    typedef std::vector<CScriptValueRef> Elements;
    typedef std::map<std::string, CScriptPropertyRef> KeyValues;

    KeyValues keys;
    Elements elements;

    CScriptVariable() : CScriptValue(), name(GenName()), value(CScriptValue::MakeNull()) {}
    CScriptVariable(const CScriptVariableRef variable, int32_t flags = 0) : name(variable->name), CScriptValue(variable, flags), value(variable->value == nullptr ? CScriptValue::MakeNull() : variable->value)
    {
    }
    CScriptVariable(const std::string& name, const CScriptValueRef& value, int32_t flags = 0) : CScriptValue(flags), name(name.empty() ? GenName() : name), value(value) {}
    virtual ~CScriptVariable() {}

    void defineGetter(const std::string& name, const CScriptValueRef& value);
    inline void defineGetter(const CScriptValueRef& name, const CScriptValueRef& value) { defineGetter(name.AsString(), value); }
    void defineSetter(const std::string& name, const CScriptValueRef& value);
    inline void defineSetter(const CScriptValueRef& name, const CScriptValueRef& value) { defineSetter(name.AsString(), value); }
};

class CScriptArray : public CScriptValue
{
public:
    typedef std::shared_ptr<CScriptArray> Ref;
    typedef std::vector<CScriptValueRef> Elements;
    CScriptPrototypeRef prototype;
    CScriptValueRef length;
    Elements elements;

    CScriptArray() : CScriptValue()
    {
    }
    CScriptArray(const Elements& elements, int32_t flags = 0) : CScriptValueRef(flags)
    {
        for (Elements::iterator it = elements.begin(); it != elements.end(); it++) {
            this->elements.push_back(CScriptVariable::Make("", (*it)->AsValue()))
        }
    }
    virtual ~CScriptArray()
    {
    }
    virtual int Type() const { return TYPE_ARRAY; }

    virtual void SetElement(size_t index, const CScriptValueRef& value)
    {
        if (elements.size() > index) {
            elements[index] = value;
        } else {
            elements.resize(index + 1);
            elements[index] = value;
        }
    }

    virtual CScriptValueRef GetElement(size_t index)
    {
        if (elements.size() <= index) return CScriptValue::MakeNull();
        return elements[index];
    }

    virtual void SetArrayElement(const CScriptValueRef& index, const CScriptValueRef& value)
    {
        size_t idx = (index != nullptr) ? (size_t)index->AsInteger() : 0;

        if (idx > 0 && elements.size() <= idx) {
            elements.resize(idx + 1);
        }

        if (index != nullptr) elements[idx] = value;
    }

    virtual CScriptValueRef GetArrayElement(const CScriptValueRef& index)
    {
        size_t idx = index != nullptr ? (size_t)index->AsInteger() : 0;
        if (index == nullptr || elements.size() <= idx) return CScriptValue::MakeNull();
        return elements[idx];
    }

    virtual CScriptValueRef GetArrayLength()
    {
        return CScriptValue::MakeNull(); //MakeInteger(elements.size());
    }

    virtual CScriptValueRef Push(const CScriptValueRef& value)
    {
        return CScriptValue::MakeNull();
    }
};

class CScriptObject : public CScriptValue
{
public:
    typedef std::shared_ptr<CScriptObject> Ref;
    typedef std::map<std::string, CScriptValueRef> KeyValues;

    CScriptPrototypeRef prototype;
    KeyValues keys;

    virtual int Type() const { return TYPE_OBJECT; }
    virtual bool isObject() const { return true; }

    CScriptObject() : CScriptValue() {}
    CScriptObject(const int32_t flags) : CScriptValue(flags)
    {
    }
    virtual ~CScriptObject()
    {
    }

    virtual CScriptArrayRef Values()
    {
        CScriptArrayRef arr = CScriptArray::Make();
        for (KeyValues::iterator it = keys.begin(); it != keys.end(); it++) {
        }
    }


    virtual void SetProperty(const CScriptValueRef prop, const CScriptValueRef& value)
    {
        std::string name = prop->AsString();
        if (!name.empty())
            keys[name] = value;
    }

    virtual CScriptValueRef GetProperty(const CScriptValueRef prop)
    {
        std::string propertyName = prop->AsString();
        if (propertyName.empty())
            return CScriptValue::MakeNull();

        std::map<std::string, Ref>::iterator it = keys.find(propertyName);
        if (it != keys.end()) {
            return (*it).second;
        }
        return CScriptValue::MakeNull();
    }

    virtual Ref HasOwnProperty(const Ref& prop)
    {
        if (prop != nullptr && keys.find(prop->AsString()) != keys.end())
            return MakeBoolean(true);

        return MakeBoolean(false);
    }
};

CScriptValueRef CScriptValue::MakeObject()
{
    CScriptObjectRef res = std::make_shared<CScriptObject>();
    return res;
}


enum CScriptOpcode {
    SCRIPT_OP_NOP = 0,
    SCRIPT_OP_NEW_NUMBER,    // Allocate number
    SCRIPT_OP_NEW_STRING,    // Allocate string
    SCRIPT_OP_NEW_POINTER,   // Allocate pointer
    SCRIPT_OP_NEW_VARIABLE,  // Allocate variable
    SCRIPT_OP_NEW_ARRAY,     // Allocate new array
    SCRIPT_OP_NEW_CLASS,     // Allocate new class
    SCRIPT_OP_NEW_OBJECT,    // Allocate new object
    SCRIPT_OP_NEW_PROTOTYPE, // Allocate new prototype
    SCRIPT_OP_NEW_PROPERTY,  // Allocate new property
    SCRIPT_OP_NEW_FUNCTION,  // Allocate new function
    SCRIPT_OP_NEW_VARIABLE,  // Allocate new variable
    SCRIPT_OP_PRTSET,        // Prototype set
    SCRIPT_OP_PRTGET,        // Prototype get
    SCRIPT_OP_ARGSET,        // Argument set
    SCRIPT_OP_ARGSSET,       // Arguments set
    SCRIPT_OP_ARGGET,        // Argument get
    SCRIPT_OP_ARGSGET,       // Arguments get
    SCRIPT_OP_SCOPEGET,      // Scoped variable set
    SCRIPT_OP_SCOPESET,      // Scoped variable get
    SCRIPT_OP_THISGET,       // This scoped variable set
    SCRIPT_OP_THISSET,       // This scoped variable get
    SCRIPT_OP_DELETE,        // delete var
    SCRIPT_OP_PLUS,          // +
    SCRIPT_OP_MINUS,         // -
    SCRIPT_OP_MULTIPLY,      // *
    SCRIPT_OP_DIVIDE,        // /
    SCRIPT_OP_MOD,           // %
    SCRIPT_OP_CALL           // callable(this, args)
};

class CScriptCode
{
public:
    uint8_t opcode;
    std::vector<CScriptValueRef> args;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(opcode);
        READWRITE(args);
    }
};

class CScriptCallable : public CScriptValue
{
public:
    virtual bool isCallable() const { return true; }

    virtual CScriptValueRef Execute(const CScriptValueRef& thisArg, const CScriptValueRef& args, const CScriptValueRef& returnArg)
    {
        return MakeVoid();
    }
    virtual CScriptValueRef ExecuteOpcode(const CScriptOpcode& opcode)
    {
        return MakeVoid();
    }
};

class CScriptFunction : public CScriptCallable
{
public:
    CScriptValueRef caller;
    CScriptValueRef name, displayName;
    CScriptArrayRef arguments;

    virtual int Type() const { return TYPE_FUNCTION; }
    virtual bool isFunction() const { return true; }

    CScriptFunction() : CScriptCallable() {}
    CScriptFunction(const int32_t flags) : CScriptCallable(flags | TYPE_CALLABLE)
    {
    }
    virtual ~CScriptFunction()
    {
    }

    virtual CScriptValueRef Call(const CScriptValueRef& thisArg, const CScriptArrayRef& args) { return MakeVoid(); }
};


class CScriptModule : public CScriptObject
{
public:
    std::unordered_map<uint256, CScriptValueRef> storage;
    std::unordered_map<std::string, uint256> names;

    virtual int Type() const { return TYPE_OBJECT; }

    CScriptModule() : CScriptObject(TYPE_MODULE) {}
    CScriptModule(const int32_t flags) : CScriptObject(flags | TYPE_MODULE)
    {
    }
    virtual ~CScriptModule()
    {
    }

    virtual CScriptVariableRef NewVariable(const std::string& name, const CScriptValueRef& value, int32_t flags)
    {
        if (names.count(name)) return storage[names[name]];
        CScriptVariableRef variable = CScriptVariable::Make(name, value, flags);
        return storage[names[name]] = variable;
    }

    virtual void AssignName(const std::string& name, const uint256& uuid)
    {
        names[name] = uuid;
    }

    virtual void Assign(const uint256& uuid, const CScriptValueRef& value)
    {
        storage[uuid] = value;
    }

    virtual void Initialize()
    {
    }
};
typedef std::shared_ptr<CScriptModule> CScriptModuleRef;

class CScriptCodegen
{
public:
    CScriptCodegen();
    virtual ~CScriptCodegen();

    virtual CScriptModuleRef Make(CScriptParser* parser);
    virtual CScriptModuleRef MakeFile(const std::string& path);
};

#endif