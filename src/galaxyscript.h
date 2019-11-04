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
#include <serialize.h>
#include <streams.h>
#include <unordered_map>
#include <vector>

#include <galaxyscript-parser.h>

// GalaxyCash Scripting engine


inline std::string ScriptPointerAsString(const void* p)
{
    std::string value;
    std::ostringstream s(value);
    s.imbue(std::locale::classic());
    s << p;
    return value;
}

typedef std::vector<std::string> CScriptStringArray;

class CScriptValue;
typedef std::shared_ptr<CScriptValue> CScriptValueRef;
typedef std::vector<CScriptValueRef> CScriptValueArray;

class CScriptVariable;
typedef std::shared_ptr<CScriptVariable> CScriptVariableRef;
typedef std::vector<CScriptVariableRef> CScriptVariableArray;

class CScriptPrototype;
typedef std::shared_ptr<CScriptPrototype> CScriptPrototypeRef;
typedef std::vector<CScriptPrototypeRef> CScriptPrototypeArray;

class CScriptObject;
typedef std::shared_ptr<CScriptObject> CScriptObjectRef;
typedef std::vector<CScriptObjectRef> CScriptObjectArray;

class CScriptArray;
typedef std::shared_ptr<CScriptArray> CScriptArrayRef;

class CScriptFunction;
typedef std::shared_ptr<CScriptFunction> CScriptFunctionRef;
typedef std::vector<CScriptFunctionRef> CScriptFunctionArray;


class CScriptValue : public std::enable_shared_from_this<CScriptValue>
{
public:
    typedef std::shared_ptr<CScriptValue> Ref;
    typedef std::vector<Ref> ValueArray;

    enum {
        TYPE_UNDEFINED = 0,
        TYPE_NULL,
        TYPE_VOID,
        TYPE_POINTER,
        TYPE_SYMBOL,
        TYPE_VARIABLE,
        TYPE_PROPERTY,
        TYPE_OBJECT,
        TYPE_ARRAY,
        TYPE_FUNCTION,
        TYPE_STRING,
        TYPE_NUMBER
    };


    enum InitFlags {
        FLAG_NATIVE = (1 << 0),
        FLAG_BUILDIN = (1 << 1),
        FLAG_INVALID = (1 << 2),
        FLAG_VOID = (1 << 3),
        FLAG_NULL = (1 << 4),
        FLAG_CONSTANT = (1 << 5),
        FLAG_WRITE = (1 << 6),
        FLAG_IMPORT = (1 << 7),
        FLAG_EXPORT = (1 << 8),
        FLAG_CONFIGURABLE = (1 << 9),
        FLAG_ENUMERABLE = (1 << 10),
        FLAG_STRING = (1 << 11),
        FLAG_SYMBOL = (1 << 12),
        FLAG_BOOLEAN = (1 << 13),
        FLAG_REAL = (1 << 14),
        FLAG_BIGNUM = (1 << 15)
    };

    mutable Ref root;
    mutable uint32_t flags;
    mutable std::string value;
    mutable std::map<std::string, Ref> keys;
    mutable std::vector<Ref> values;


    CScriptValue();
    CScriptValue(const Ref& root, const Ref& prototype, const uint32_t flags = 0);
    virtual ~CScriptValue();


    static Ref CreateTyped(const uint8_t type, const Ref& prototype = nullptr, const Ref& root = Global());

    static Ref ReadValue(CDataStream& s, const Ref& root = Global())
    {
        uint8_t type;
        s >> type;

        Ref ret = CreateTyped(type);

        s >> ret->flags;
        s >> ret->value;

        uint32_t num;
        s >> num;
        for (uint32_t i = 0; i < num; i++) {
            uint8_t type;
            s >> type;

            std::string key;
            s >> key;

            CScriptValueRef keyValue = ReadValue(s, ret);

            ret->keys.insert(std::make_pair(key, keyValue));
        }

        s >> num;
        for (uint32_t i = 0; i < num; i++) {
            uint8_t type;
            s >> type;

            CScriptValueRef newValue = ReadValue(s, ret);

            ret->values.push_back(newValue);
        }


        std::vector<char> vch;
        s >> vch;
        ret->UnserializeValue(vch);

        return ret;
    }

    static void WriteValue(CDataStream& s, const Ref& val)
    {
        if (!val)
            return;

        uint8_t type = val->Typeid();
        s << type;
        s << val->flags;
        s << val->value;

        uint32_t num = val->keys.size();
        s << num;
        for (std::map<std::string, Ref>::iterator it = val->keys.begin(); it != val->keys.end(); it++) {
            uint8_t type = (*it).second->Typeid();
            s << type;
            s << (*it).first;
            WriteValue(s, (*it).second);
        }

        num = val->values.size();
        s << num;
        for (std::vector<Ref>::iterator it = val->values.begin(); it != val->values.end(); it++) {
            uint8_t type = (*it)->Typeid();
            s << type;
            WriteValue(s, (*it));
        }


        std::vector<char> vch;
        val->SerializeValue(vch);
        s << vch;
    }

    static Ref Undefined();
    static Ref Null();
    static Ref Void();
    static Ref Global();

    static Ref Pointer(const void* value, const Ref& root = Global(), const uint32_t flags = 0);
    static Ref Boolean(const bool value, const Ref& root = Global(), const uint32_t flags = 0);
    static Ref Integer(const int64_t value, const Ref& root = Global(), const uint32_t flags = 0);
    static Ref Float(const double value, const Ref& root = Global(), const uint32_t flags = 0);
    static Ref String(const std::string& value, const Ref& root = Global(), uint32_t flags = 0);
    static Ref Symbol(const std::string& value, const Ref& root = Global(), uint32_t flags = 0);
    static Ref Variable(const Ref& value, const Ref& root = Global(), uint32_t flags = 0);
    static Ref Property(const Ref& object, const Ref& value, uint32_t flags = 0);
    static Ref Array(const ValueArray& initializer, const Ref& root = Global(), uint32_t flags = 0);
    static Ref Object(const Ref& root = Global(), uint32_t flags = 0);
    static Ref Function(const Ref& root = Global(), uint32_t flags = 0);
    static Ref Create(const Ref& prototype, const Ref& root = Global(), uint32_t flags = 0);
    static Ref Prototype(const Ref& value, const Ref& root = Global(), uint32_t flags = 0);


    virtual Ref Copy(const Ref& root = Global(), const uint32_t flags = 0) { return std::make_shared<CScriptValue>(root, shared_from_this(), this->Flags() | flags); }

    virtual void SerializeValue(std::vector<char>& vch) {}
    virtual void UnserializeValue(std::vector<char>& vch) {}

    virtual uint32_t Flags() const { return flags; }
    virtual uint8_t Typeid() const
    {
        if (flags & FLAG_VOID)
            return TYPE_VOID;
        else if (flags & FLAG_NULL)
            return TYPE_NULL;
        return TYPE_UNDEFINED;
    }

    virtual std::string ToString() const
    {
        if (flags & FLAG_VOID)
            return "void";
        else if (flags & FLAG_NULL)
            return "null";
        return "undefined";
    }
    virtual std::string Typename() const
    {
        if (flags & FLAG_VOID)
            return "void";
        else if (flags & FLAG_NULL)
            return "null";
        return "undefined";
    }

    virtual bool HaveKey(const std::string& name) const { return keys.count(name); }
    virtual bool HaveIndex(const int64_t index) const { return values.size() > index; }

    virtual Ref Push(const Ref& value)
    {
        values.push_back(value);
        return value;
    }

    virtual size_t Length() const
    {
        return values.size();
    }

    virtual Ref Assign(const CScriptValueRef& value);
    virtual Ref AsValue() { return std::shared_ptr<CScriptValue>(this); }

    virtual bool IsNumber() const { return false; }
    virtual bool IsBoolean() const { return false; }
    virtual bool AsBoolean() const { return false; }
    virtual bool IsInteger() const { return false; }
    virtual int64_t AsInteger() const { return 0; }
    virtual bool IsFloat() const { return false; }
    virtual double AsFloat() const { return 0.0; }
    virtual bool IsBignum() const { return false; }
    virtual uint256 AsBignum() const { return uint256(0); }
    virtual bool IsString() const { return Flags() & FLAG_SYMBOL || Flags() & FLAG_STRING; }
    virtual std::string AsString() const { return value; }
    virtual bool IsPointer() const { return false; }
    virtual void* AsPointer() const { return nullptr; }
    virtual bool IsObject() const { return false; }
    virtual CScriptObjectRef AsObject() const { return nullptr; }
    virtual bool IsArray() const { return false; }
    virtual CScriptArrayRef AsArray() const { return nullptr; }
    virtual bool IsFunction() const { return false; }
    virtual CScriptFunctionRef AsFunction() const { return nullptr; }

    virtual Ref AddKeyValue(const std::string& key, const Ref& value, const uint32_t flags = 0);
    virtual Ref FindKeyValue(const std::string& key) const
    {
        if (keys.count(key)) return keys[key];
        return nullptr;
    }

    virtual int CompareTo(const Ref& other) const;
};

class CScriptPointer : public CScriptValue
{
public:
    void* ptr_value;

    CScriptPointer(const CScriptValueRef& root, const CScriptValueRef& prototype, const uint32_t flags = 0);

    virtual CScriptValueRef Copy(const CScriptValueRef& root = Global(), const uint32_t flags = 0) { return std::make_shared<CScriptPointer>(root, std::static_pointer_cast<CScriptValue>(shared_from_this()), this->Flags() | flags); }
    virtual void SerializeValue(std::vector<char>& vch) {}
    virtual void UnserializeValue(std::vector<char>& vch) { ptr_value = nullptr; }

    virtual bool IsPointer() const { return true; }
    virtual bool AsBoolean() const { return ptr_value ? true : false; }
    virtual int64_t AsInteger() const { return (int64_t)ptr_value; }
    virtual double AsFloat() const { return (double)AsInteger(); }
    virtual std::string AsString() const { return ToString(); }

    virtual uint8_t Typeid() const { return TYPE_POINTER; }
    virtual std::string ToString() const;
    virtual std::string Typename() const { return "number"; }
};

class CScriptVariable : public CScriptValue
{
public:
    std::string varName;
    CScriptValueRef varValue;

    CScriptVariable(const CScriptValueRef& root, const CScriptValueRef& prototype, const uint32_t flags = 0);

    virtual CScriptValueRef Copy(const CScriptValueRef& root = Global(), const uint32_t flags = 0) { return std::make_shared<CScriptVariable>(root, std::static_pointer_cast<CScriptValue>(shared_from_this()), this->Flags() | flags); }
    virtual void SerializeValue(std::vector<char>& vch)
    {
        CDataStream s(SER_DISK, PROTOCOL_VERSION);
        s << varName;
        WriteValue(s, varValue);
        vch.insert(vch.end(), s.begin(), s.end());
    }
    virtual void UnserializeValue(std::vector<char>& vch)
    {
        if (!vch.size())
            return;

        CDataStream s(*vch.begin(), *vch.end(), SER_DISK, PROTOCOL_VERSION);
        s >> varName;
        varValue = ReadValue(s, AsValue());
    }

    virtual bool IsVariable() const { return true; }
    virtual bool IsProperty() const { return true; }
    virtual bool AsBoolean() const { return (varValue && varValue != CScriptValue::Null() && varValue != CScriptValue::Undefined()) ? true : false; }
    virtual int64_t AsInteger() const { return varValue ? varValue->AsInteger() : 0; }
    virtual double AsFloat() const { return varValue ? varValue->AsFloat() : 0.0; }
    virtual std::string AsString() const { return varValue ? varValue->ToString() : "undefined"; }

    virtual CScriptValueRef AsValue() { return varValue; }

    virtual uint8_t Typeid() const { return TYPE_VARIABLE; }
    virtual std::string ToString() const;
    virtual std::string Typename() const { return varValue ? varValue->Typename() : "undefined"; }
};

class CScriptObject : public CScriptValue
{
public:
    typedef std::pair<std::string, CScriptValueRef> KeyValue;
    typedef std::shared_ptr<CScriptObject> Ref;

    std::string objectType;
    CScriptObjectRef prototype;

    CScriptObject(const CScriptValueRef& root, const CScriptValueRef& prototype, const uint32_t flags = 0);
    CScriptObject(const CScriptValueRef& root, const std::vector<KeyValue>& keys, const uint32_t flags = 0);

    virtual CScriptValueRef Copy(const CScriptValueRef& root = Global(), const uint32_t flags = 0) { return std::make_shared<CScriptObject>(root, std::static_pointer_cast<CScriptValue>(shared_from_this()), this->Flags() | flags); }
    virtual void SerializeValue(std::vector<char>& vch)
    {
        CDataStream s(SER_DISK, PROTOCOL_VERSION);
        s << objectType;
        WriteValue(s, prototype->AsValue());
        vch.insert(vch.end(), s.begin(), s.end());
    }
    virtual void UnserializeValue(std::vector<char>& vch)
    {
        if (!vch.size())
            return;

        CDataStream s(*vch.begin(), *vch.end(), SER_DISK, PROTOCOL_VERSION);
        s >> objectType;
        prototype = ReadValue(s, AsValue());
    }

    bool SetPrototype(const CScriptObjectRef& other);
    CScriptObjectRef GetPrototype() const;
    bool IsPrototypeOf(const CScriptObjectRef& other) const;

    bool SetOwnProperty(const std::string& name, const CScriptValueRef& value);
    CScriptValueRef GetOwnProperty(const std::string& name) const;
    bool HasOwnProperty(const std::string& name) const
    {
        return keys.count(name);
    }

    virtual CScriptStringArray Keys();
    virtual CScriptValueArray Values();

    virtual Ref AsObject() const { return std::dynamic_pointer_cast<CScriptObject>(shared_from_this()); }

    virtual bool IsObject() const { return true; }
    virtual bool AsBoolean() const { return !keys.empty(); }
    virtual int64_t AsInteger() const { return (int64_t)keys.size(); }
    virtual double AsFloat() const { return (double)keys.size(); }
    virtual std::string AsString() const { return ToString(); }

    virtual uint8_t Typeid() const { return TYPE_OBJECT; }
    virtual std::string ToString() const;
    virtual std::string Typename() const { return std::string("[object ") + objectType + "]"; }
};

class CScriptArray : public CScriptObject
{
public:
    typedef std::shared_ptr<CScriptArray> Ref;

    CScriptArray(const CScriptValueRef& root, const CScriptValueRef& prototype, const uint32_t flags = 0);

    virtual CScriptValueRef Copy(const CScriptValueRef& root = Global(), const uint32_t flags = 0) { return std::make_shared<CScriptArray>(root, std::static_pointer_cast<CScriptValue>(shared_from_this()), this->Flags() | flags); }

    virtual bool IsArray() const { return true; }
    virtual Ref AsArray() const { return std::dynamic_pointer_cast<CScriptArray>(shared_from_this()); }

    virtual bool AsBoolean() const { return !values.empty(); }
    virtual int64_t AsInteger() const { return (int64_t)value.size(); }
    virtual double AsFloat() const { return (double)value.size(); }
    virtual std::string AsString() const { return ToString(); }

    virtual uint8_t Typeid() const { return TYPE_ARRAY; }
    virtual std::string ToString() const;

    virtual bool Includes(const CScriptValueRef& value, size_t from = 0) const;
};
typedef std::shared_ptr<CScriptArray> CScriptArrayRef;

typedef CScriptValueRef (*CScriptNativeFunction)(CScriptValueRef thisArg, CScriptValueRef args);
CScriptValueRef ScriptGetArrayLength(const CScriptValueRef thisArg, const CScriptArrayRef args);


typedef std::vector<char> CScriptBytecode;

#include "compat/endian.h"

class CScriptFrame
{
public:
    typedef std::shared_ptr<CScriptFrame> Ref;

    enum {
        RUNNING = 0,
        PAUSED,
        BREAKED,
        FINISHED
    };

    Ref caller;
    mutable std::vector<CScriptValueRef> stack;
    CScriptFunctionRef scope;
    size_t pc;


    size_t Length() const { return stack.size(); }

    void Push(const CScriptValueRef& value)
    {
        stack.push_back(value);
    }
    CScriptValueRef Pop() const
    {
        if (stack.size() == 0) return nullptr;
        CScriptValueRef val = stack[0];
        stack.erase(stack.begin());
        return val;
    }
    CScriptValueRef Top() const
    {
        if (stack.size() == 0) return nullptr;
        return stack[0];
    }
};
typedef std::shared_ptr<CScriptFrame> CScriptFrameRef;

class CScriptFunction : public CScriptObject
{
public:
    typedef std::shared_ptr<CScriptFunction> Ref;

    std::string name;
    CScriptBytecode code;
    CScriptNativeFunction native;
    CScriptArrayRef arguments;

    CScriptFunction(const CScriptValueRef& root, const CScriptValueRef& prototype, const uint32_t flags = 0);

    virtual CScriptValueRef Copy(const CScriptValueRef& root = Global(), const uint32_t flags = 0) { return std::make_shared<CScriptFunction>(root, std::static_pointer_cast<CScriptValue>(shared_from_this()), this->Flags() | flags); }

    virtual void SerializeValue(std::vector<char>& vch)
    {
        CDataStream s(SER_DISK, PROTOCOL_VERSION);
        WriteValue(s, prototype->AsValue());
        vch.insert(vch.end(), s.begin(), s.end());
    }
    virtual void UnserializeValue(std::vector<char>& vch)
    {
        if (!vch.size())
            return;

        CDataStream s(*vch.begin(), *vch.end(), SER_DISK, PROTOCOL_VERSION);
        prototype = ReadValue(s, AsValue());
    }


    virtual bool IsFunction() const { return true; }
    virtual Ref AsFunction() const { return std::dynamic_pointer_cast<CScriptFunction>(shared_from_this()); }

    void PushI8(int8_t code) { this->code.push_back(code); }
    void PushI16(int16_t code)
    {
        code = htole16(code);
        char* v = (char*)&code;
        this->code.push_back(v[0]);
        this->code.push_back(v[1]);
    }
    void PushI32(int32_t code)
    {
        code = htole32(code);
        char* v = (char*)&code;
        this->code.push_back(v[0]);
        this->code.push_back(v[1]);
        this->code.push_back(v[2]);
        this->code.push_back(v[3]);
    }
    void PushI64(int64_t code)
    {
        code = htole64(code);
        char* v = (char*)&code;
        this->code.push_back(v[0]);
        this->code.push_back(v[1]);
        this->code.push_back(v[2]);
        this->code.push_back(v[3]);
        this->code.push_back(v[4]);
        this->code.push_back(v[5]);
        this->code.push_back(v[6]);
        this->code.push_back(v[7]);
    }
    void PushVCH(const std::vector<char>& vch)
    {
        this->code.insert(this->code.end(), vch.begin(), vch.end());
    }
    void PushStr(const std::string& str)
    {
        PushI32(str.length() + 1);
        this->code.insert(this->code.end(), str.begin(), str.end());
    }

    template <typename Ty>
    void PushT(const Ty& value)
    {
        CDataStream ss(SER_DISK, PROTOCOL_VERSION);
        ss << value;
        code.insert(code.begin() + code.size(), ss.begin(), ss.end());
    }

    virtual bool Call(const CScriptFrameRef& frame);

    virtual uint8_t Typeid() const { return TYPE_FUNCTION; }
    virtual std::string ToString() const;
};

class CScriptNumber : public CScriptObject
{
public:
    typedef std::shared_ptr<CScriptNumber> Ref;

    uint256 bn;
    int64_t n;

    CScriptNumber(const CScriptValueRef& root, const CScriptValueRef& prototype, const uint32_t flags = 0);

    virtual CScriptValueRef Copy(const CScriptValueRef& root = Global(), const uint32_t flags = 0) { return std::make_shared<CScriptNumber>(root, std::static_pointer_cast<CScriptValue>(shared_from_this()), this->Flags() | flags); }

    virtual bool IsNumber() const { return true; }
    virtual Ref AsNumber() const { return std::dynamic_pointer_cast<CScriptNumber>(shared_from_this()); }


    virtual bool IsInteger() const { return !(Flags() & FLAG_REAL) && !(Flags() & FLAG_BOOLEAN) && !(Flags() & FLAG_BIGNUM); }
    virtual int64_t AsInteger() const { return n; }
    virtual bool IsBoolean() const { return (Flags() & FLAG_BOOLEAN); }
    virtual void AssignBoolean(const bool number)
    {
        if (Flags() & FLAG_BOOLEAN) flags |= FLAG_BOOLEAN;
        n = number ? 1 : 0;
    }
    virtual bool AsBoolean() const { return n; }
    virtual bool IsFloat() const { return (Flags() & FLAG_REAL); }
    virtual void AssignFloat(const double number)
    {
        if (Flags() & FLAG_REAL) flags |= FLAG_REAL;
        *((double*)&n) = number;
    }
    virtual double AsFloat() const { return *((double*)&n); }
    virtual bool IsBignum() const { return (Flags() & FLAG_BIGNUM); }
    virtual void AssignBignum(const uint256& number)
    {
        if (Flags() & FLAG_BIGNUM) flags |= FLAG_BIGNUM;
        bn = number;
    }
    virtual uint256 AsBignum() const { return bn; }

    virtual uint8_t Typeid() const { return TYPE_NUMBER; }
    virtual std::string ToString() const;
    virtual std::string Typename() const { return "[object Number]"; }
};
typedef std::shared_ptr<CScriptArray> CScriptArrayRef;

#endif