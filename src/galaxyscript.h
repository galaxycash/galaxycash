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

#include <galaxyscript-parser.h>

// GalaxyCash Scripting engine


inline std::string ScriptPointerAsString(void* p)
{
    std::string value;
    std::ostringstream s(value);
    s.imbue(std::locale::classic());
    s << p;
    return value;
}

typedef std::vector<std::string> CScriptStringArray;

class CScriptValue;
class CScriptVariable;
class CScriptPointer;
class CScriptObject;
class CScriptArray;
class CScriptFunction;
class CScriptAsyncFunction;
class CScriptNumber;
class CScriptModule;

class CScriptValue
{
public:
    typedef std::vector<CScriptValue*> ValueArray;

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
        TYPE_ASYNC_FUNCTION,
        TYPE_STRING,
        TYPE_NUMBER,
        TYPE_MODULE
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
        FLAG_BIGNUM = (1 << 15),
        FLAG_CALLABLE = (1 << 16),
        FLAG_ASYNC = (1 << 17),
        FLAG_HIDDEN = (1 << 18)
    };

    mutable size_t refCount;
    mutable CScriptValue* root;
    mutable uint32_t flags;
    mutable std::string name;
    mutable std::string value;
    mutable std::map<std::string, CScriptValue*> keys;
    mutable std::vector<CScriptValue*> values;

    CScriptValue();
    CScriptValue(CScriptValue* root, CScriptValue* prototype, const uint32_t flags = 0);
    virtual ~CScriptValue();

    inline CScriptValue* Unref()
    {
        refCount--;
        if (refCount == 0) {
            delete this;
            return nullptr;
        }
        return this;
    }

    inline CScriptValue* Ref()
    {
        refCount++;
        return this;
    }

    template <typename Ty>
    inline Ty& Cast()
    {
        return dynamic_cast<Ty*>(this);
    }

    template <typename Ty>
    inline const Ty& Cast() const
    {
        return dynamic_cast<Ty*>(this);
    }

    virtual void SetName(const std::string& name) { this->name = name; }
    virtual std::string& GetName() const { return name; }
    virtual std::string GetFullName() const { return root ? (root->GetFullName() + ":" + name) : name; }

    virtual void SetValue(const std::string& value) { this->value = value; }
    virtual std::string& GetValue() const { return value; }

    static CScriptValue* CreateTyped(uint8_t type, CScriptValue* prototype = nullptr, CScriptValue* root = Global());

    static CScriptValue* ReadValue(CDataStream& s, CScriptValue* root = Global())
    {
        uint8_t type;
        s >> type;


        CScriptValue* ret = CreateTyped(type);
        s >> ret->name;
        s >> ret->value;
        s >> ret->flags;

        uint32_t num;
        s >> num;
        for (uint32_t i = 0; i < num; i++) {
            std::string key;
            s >> key;
            ret->keys.insert(std::make_pair(key, ReadValue(s, ret)));
        }

        s >> num;
        for (uint32_t i = 0; i < num; i++) {
            ret->values.push_back(ReadValue(s, ret));
        }


        std::vector<char> vch;
        s >> vch;
        ret->UnserializeValue(vch);

        return ret;
    }

    static void WriteValue(CDataStream& s, CScriptValue* val)
    {
        if (!val)
            return;

        uint8_t type = val->Typeid();
        s << type;
        s << val->name;
        s << val->value;
        s << val->flags;

        uint32_t num = val->keys.size();
        s << num;
        for (std::map<std::string, CScriptValue*>::iterator it = val->keys.begin(); it != val->keys.end(); it++) {
            uint8_t type = (*it).second->Typeid();
            s << type;
            s << (*it).first;
            WriteValue(s, (*it).second);
        }

        num = val->values.size();
        s << num;
        for (std::vector<CScriptValue*>::iterator it = val->values.begin(); it != val->values.end(); it++) {
            uint8_t type = (*it)->Typeid();
            s << type;
            WriteValue(s, (*it));
        }


        std::vector<char> vch;
        val->SerializeValue(vch);
        s << vch;
    }

    static CScriptValue* Undefined();
    static CScriptValue* Null();
    static CScriptValue* Void();
    static CScriptValue* Global();

    virtual CScriptValue* Copy(CScriptValue* root = Global(), const uint32_t flags = 0) const { return new CScriptValue(root, const_cast<CScriptValue*>(this), Flags() | flags); }

    virtual void SerializeValue(std::vector<char>& vch) {}
    virtual void UnserializeValue(const std::vector<char>& vch) {}

    virtual CScriptValue* GetVariable(const std::string& name)
    {
        if (root) return root->GetVariable(name);
        return Undefined();
    }

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

    virtual bool HaveKey(const std::string& name) { return keys.count(name); }
    virtual bool HaveIndex(int64_t index) { return values.size() > index; }

    virtual CScriptValue* Push(CScriptValue* value)
    {
        values.push_back(value->Ref());
        return values.back();
    }

    virtual size_t Length()
    {
        return values.size();
    }

    virtual void SetNull();
    virtual void SetUndefined();
    virtual CScriptValue* Assign(CScriptValue* value);
    virtual CScriptValue* AsValue() { return this; }

    virtual bool IsBoolean() const { return false; }
    virtual bool AsBoolean() const { return false; }
    virtual bool IsInteger() const { return false; }
    virtual int64_t AsInteger() const { return 0; }
    virtual bool IsFloat() const { return false; }
    virtual double AsFloat() const { return 0.0; }
    virtual bool IsBignum() const { return false; }
    virtual uint256 AsBignum() const { return uint256(0); }
    virtual bool IsString() const { return Flags() & FLAG_SYMBOL || Flags() & FLAG_STRING; }
    virtual bool IsSymbol() const { return Flags() & FLAG_SYMBOL; }
    virtual std::string AsSymbol() const { return value; }
    virtual std::string AsString() const { return value; }
    virtual bool IsPointer() const { return false; }
    virtual void* AsPointer() const { return nullptr; }
    virtual bool IsObject() const { return false; }
    virtual CScriptObject* AsObject() { return nullptr; }
    virtual bool IsArray() const { return false; }
    virtual CScriptArray* AsArray() { return nullptr; }
    virtual bool IsFunction() const { return false; }
    virtual CScriptFunction* AsFunction() { return nullptr; }
    virtual bool IsAsyncFunction() const { return false; }
    virtual CScriptAsyncFunction* AsAsyncFunction() { return nullptr; }
    virtual bool IsModule() const { return false; }
    virtual CScriptModule* AsModule() { return nullptr; }
    virtual bool IsVariable() const { return false; }
    virtual bool IsProperty() const { return false; }
    virtual CScriptVariable* AsVariable() { return nullptr; }
    virtual bool IsNumber() const { return false; }
    virtual CScriptNumber* AsNumber() { return nullptr; }
    virtual bool IsUndefined() const { return Flags() & FLAG_INVALID; }
    virtual bool IsNull() const { return Flags() & FLAG_NULL; }
    virtual bool IsVoid() const { return Flags() & FLAG_VOID; }

    virtual CScriptValue* SetKeyValue(const std::string& key, CScriptValue* value, const uint32_t flags = 0);
    virtual CScriptValue* FindKeyValue(const std::string& key)
    {
        if (keys.count(key)) return keys[key];
        return nullptr;
    }

    virtual int CompareTo(CScriptValue* other);
};
typedef std::vector<CScriptValue*> CScriptValueArray;

class CScriptPointer : public CScriptValue
{
public:
    void* ptr_value;

    CScriptPointer();
    CScriptPointer(CScriptValue* root, CScriptValue* prototype, const uint32_t flags = 0);

    inline CScriptPointer* Ref()
    {
        CScriptValue::Ref();
        return this;
    }

    virtual CScriptValue* Copy(CScriptValue* root = Global(), const uint32_t flags = 0) const { return new CScriptPointer(root, const_cast<CScriptPointer*>(this), Flags() | flags); }

    virtual void SerializeValue(std::vector<char>& vch) {}
    virtual void UnserializeValue(const std::vector<char>& vch) { ptr_value = nullptr; }

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
    CScriptValue* var;

    CScriptVariable();
    CScriptVariable(CScriptValue* root, CScriptValue* prototype, const uint32_t flags = 0);

    inline CScriptVariable* Ref()
    {
        CScriptValue::Ref();
        return this;
    }

    virtual CScriptValue* Copy(CScriptValue* root = Global(), const uint32_t flags = 0) const { return new CScriptVariable(root, const_cast<CScriptVariable*>(this), this->Flags() | flags); }

    virtual void SerializeValue(std::vector<char>& vch)
    {
        CDataStream s(SER_DISK, PROTOCOL_VERSION);
        s << var->GetFullName();
        vch.insert(vch.end(), s.begin(), s.end());
    }
    virtual void UnserializeValue(const std::vector<char>& vch)
    {
        if (!vch.size())
            return;

        CDataStream s(SER_DISK, PROTOCOL_VERSION);
        s << vch;

        std::string fullname;
        s >> fullname;
        if (root) {
            var = root->GetVariable(fullname)->Ref();
        } else
            var = Undefined()->Ref();
    }

    virtual bool IsVariable() const { return true; }
    virtual bool IsProperty() const { return true; }
    virtual CScriptVariable* AsVariable() { return this; }

    virtual bool AsBoolean() const { return var ? var->AsBoolean() : false; }
    virtual int64_t AsInteger() const { return var ? var->AsInteger() : 0; }
    virtual double AsFloat() const { return var ? var->AsFloat() : 0.0; }
    virtual std::string AsString() const { return var ? var->AsString() : "undefined"; }
    virtual std::string AsSymbol() const { return var ? var->AsSymbol() : "undefined"; }
    virtual void* AsPointer() const { return var ? var->AsPointer() : (void*)(this); }
    virtual uint256 AsBignum() const { return var ? var->AsBignum() : uint256(); }
    virtual CScriptObject* AsObject() { return var ? var->AsObject() : nullptr; }
    virtual CScriptArray* AsArray() { return var ? var->AsArray() : nullptr; }
    virtual CScriptFunction* AsFunction() { return var ? var->AsFunction() : nullptr; }
    virtual CScriptModule* AsModule() { return var ? var->AsModule() : nullptr; }
    virtual CScriptNumber* AsNumber() { return var ? var->AsNumber() : nullptr; }

    virtual bool IsBoolean() const { return var ? var->IsBoolean() : false; }
    virtual bool IsInteger() const { return var ? var->IsInteger() : false; }
    virtual bool IsFloat() const { return var ? var->IsFloat() : false; }
    virtual bool IsBignum() const { return var ? var->IsBignum() : false; }
    virtual bool IsString() const { return var ? var->IsString() : false; }
    virtual bool IsSymbol() const { return var ? var->IsSymbol() : false; }
    virtual bool IsPointer() const { return var ? var->IsPointer() : false; }
    virtual bool IsObject() const { return var ? var->IsObject() : false; }
    virtual bool IsArray() const { return var ? var->IsArray() : false; }
    virtual bool IsFunction() const { return var ? var->IsFunction() : false; }
    virtual bool IsModule() const { return var ? var->IsModule() : false; }
    virtual bool IsNumber() const { return var ? var->IsNumber() : false; }
    virtual bool IsUndefined() const { return var ? var->IsUndefined() : false; }
    virtual bool IsNull() const { return var ? var->IsNull() : true; }
    virtual bool IsVoid() const { return var ? var->IsVoid() : false; }

    virtual CScriptValue* AsValue()
    {
        if (var)
            return (var);
        else
            return Undefined();
    }
    virtual CScriptValue* Assign(CScriptValue* value);

    virtual std::string ToString() const { return var ? var->ToString() : "undefined"; }
    virtual uint8_t Typeid() const { return TYPE_VARIABLE; }
    virtual std::string Typename() const { return var ? var->Typename() : "undefined"; }
};
typedef std::vector<CScriptVariable*> CScriptVariableArray;

class CScriptObject : public CScriptValue
{
public:
    typedef std::pair<std::string, CScriptValue*> KeyValue;

    std::string objectType;
    CScriptObject* prototype;

    CScriptObject();
    CScriptObject(CScriptValue* root, CScriptValue* prototype, const uint32_t flags = 0);

    inline CScriptObject* Ref()
    {
        CScriptValue::Ref();
        return this;
    }

    virtual CScriptValue* Copy(CScriptValue* root = Global(), const uint32_t flags = 0) const { return new CScriptObject(root, const_cast<CScriptObject*>(this), Flags() | flags); }

    virtual void SerializeValue(std::vector<char>& vch)
    {
        CDataStream s(SER_DISK, PROTOCOL_VERSION);
        s << objectType;
        WriteValue(s, prototype);
        vch.insert(vch.end(), s.begin(), s.end());
    }
    virtual void UnserializeValue(const std::vector<char>& vch)
    {
        if (!vch.size())
            return;

        CDataStream s(SER_DISK, PROTOCOL_VERSION);
        s << vch;
        s >> objectType;
        CScriptValue* ptype = ReadValue(s, this);
        if (ptype && ptype->IsObject())
            prototype = ptype->Ref()->AsObject();
        else if (ptype)
            delete ptype;
    }

    bool SetPrototype(CScriptObject* other);
    CScriptObject* GetPrototype() { return prototype; }
    bool IsPrototypeOf(CScriptObject* other) { return (other && other->GetPrototype() == prototype) ? true : false; }

    CScriptValue* SetOwnProperty(const std::string& name, CScriptValue* value)
    {
        if (HasOwnProperty(name)) {
            keys[name]->Assign(value);
            return keys[name];
        }
        keys[name] = new CScriptVariable(this, nullptr, 0);
        keys[name]->Assign(value);
        keys[name]->SetName(name);
        return keys[name];
    }

    template <typename Ty>
    Ty* SetOwnPropertyT(const std::string& name, CScriptValue* value)
    {
        return dynamic_cast<Ty*>(SetOwnProperty(name, value));
    }

    CScriptValue* GetOwnProperty(const std::string& name)
    {
        if (!keys.count(name)) return Null();
        return keys[name];
    }

    template <typename Ty>
    Ty* GetOwnPropertyT(const std::string& name)
    {
        return dynamic_cast<Ty*>(GetOwnProperty(name));
    }

    bool HasOwnProperty(const std::string& name)
    {
        if (keys.count(name))
            return keys[name] != nullptr;
        return false;
    }

    virtual CScriptObject* AsObject() { return this; }

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
    CScriptArray();
    CScriptArray(CScriptValue* root, CScriptValue* prototype, const uint32_t flags = 0);

    inline CScriptArray* Ref()
    {
        CScriptValue::Ref();
        return this;
    }

    virtual CScriptValue* Copy(CScriptValue* root = Global(), const uint32_t flags = 0) const { return new CScriptArray(root, const_cast<CScriptArray*>(this), Flags() | flags); }

    virtual bool IsArray() const { return true; }
    virtual CScriptArray* AsArray() { return this; }

    virtual bool AsBoolean() const { return !values.empty(); }
    virtual int64_t AsInteger() const { return (int64_t)value.size(); }
    virtual double AsFloat() const { return (double)value.size(); }
    virtual std::string AsString() const { return ToString(); }

    virtual uint8_t Typeid() const { return TYPE_ARRAY; }
    virtual std::string ToString() const;

    virtual bool Includes(CScriptValue* value, size_t from = 0) const;
};

typedef CScriptValue* (*CScriptNativeFunction)(CScriptValue* thisArg, CScriptArray* args);
CScriptValue* ScriptGetArrayLength(CScriptValue* thisArg, CScriptArray* args);


typedef std::vector<char> CScriptBytecode;

typedef std::vector<char> CScriptData;
typedef std::vector<CScriptData> CScriptDataArray;

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

    mutable Ref caller;
    mutable std::vector<CScriptValue*> stack;
    mutable CScriptFunction* scope;
    mutable size_t pc;


    size_t Length() { return stack.size(); }

    void Push(CScriptValue* value)
    {
        stack.push_back(value);
    }
    CScriptValue* Pop()
    {
        if (stack.size() == 0) return nullptr;
        CScriptValue* val = stack[0];
        stack.erase(stack.begin());
        return val;
    }
    CScriptValue* Top()
    {
        if (stack.size() == 0) return nullptr;
        return stack[0];
    }
};
typedef std::shared_ptr<CScriptFrame> CScriptFrameRef;


class CScriptFunction : public CScriptObject
{
public:
    CScriptDataArray datas;
    CScriptValueArray literals;
    CScriptBytecode code;
    CScriptNativeFunction native;
    CScriptVariable* thisArg;
    CScriptValue *functionName, *displayName;
    CScriptFunction* caller;
    CScriptFunction* length;
    CScriptArray* arguments;

    CScriptFunction();
    CScriptFunction(CScriptValue* root, CScriptValue* prototype, const uint32_t flags = 0);


    inline CScriptFunction* Ref()
    {
        CScriptValue::Ref();
        return this;
    }

    virtual CScriptValue* GetVariable(const std::string& name)
    {
        if (GetFullName() == name) return this;
        for (std::map<std::string, CScriptValue*>::iterator it = keys.begin(); it != keys.end(); it++) {
            if ((*it).second && ((*it).second->GetFullName() == name)) return (*it).second;
        }
        return (root != nullptr) ? root->GetVariable(name) : Undefined();
    }


    void SetName(const std::string& name);
    std::string& GetName() const;

    void SetDisplayName(const std::string& name);
    std::string& GetDisplayName() const;

    virtual CScriptValue* Copy(CScriptValue* root = Global(), const uint32_t flags = 0) const { return new CScriptFunction(root, const_cast<CScriptFunction*>(this), Flags() | flags); }

    virtual void SerializeValue(std::vector<char>& vch)
    {
        CDataStream s(SER_DISK, PROTOCOL_VERSION);
        CScriptObject::SerializeValue(vch);
    }
    virtual void UnserializeValue(const std::vector<char>& vch)
    {
        if (!vch.size())
            return;
        CScriptObject::UnserializeValue(vch);
    }


    virtual bool IsFunction() const { return true; }
    virtual CScriptFunction* AsFunction() { return this; }

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
        code.insert(code.end(), vch.begin(), vch.end());
    }
    void PushStr(const std::string& str)
    {
        PushI32(str.length() + 1);
        code.insert(code.end(), str.begin(), str.end());
    }

    template <typename Ty>
    void PushT(Ty& value)
    {
        CDataStream ss(SER_DISK, PROTOCOL_VERSION);
        ss << value;
        code.insert(code.end(), ss.begin(), ss.end());
    }

    virtual bool Call(CScriptFrameRef& frame);

    virtual uint8_t Typeid() const { return TYPE_FUNCTION; }
    virtual std::string ToString() const;
};

class CScriptAsyncFunction : public CScriptFunction
{
public:
    CScriptAsyncFunction();
    CScriptAsyncFunction(CScriptValue* root, CScriptValue* prototype, const uint32_t flags = 0);

    virtual CScriptValue* Copy(CScriptValue* root = Global(), const uint32_t flags = 0) const { return new CScriptAsyncFunction(root, const_cast<CScriptAsyncFunction*>(this), Flags() | flags); }

    inline CScriptAsyncFunction* Ref()
    {
        CScriptValue::Ref();
        return this;
    }


    virtual bool IsAsyncFunction() const { return true; }
    virtual CScriptAsyncFunction* AsAsyncFunction() { return this; }

    virtual bool Call(CScriptFrameRef& frame);

    virtual uint32_t Flags() const { return flags | FLAG_ASYNC; }
    virtual uint8_t Typeid() const { return TYPE_ASYNC_FUNCTION; }
    virtual std::string ToString() const;
};

class CScriptString : public CScriptObject
{
public:
    CScriptString();
    CScriptString(CScriptValue* root, CScriptValue* prototype, const uint32_t flags = 0);


    inline CScriptString* Ref()
    {
        CScriptValue::Ref();
        return this;
    }

    virtual CScriptValue* Copy(CScriptValue* root = Global(), const uint32_t flags = 0) const { return new CScriptString(root, const_cast<CScriptString*>(this), Flags() | flags); }

    virtual CScriptValue* Assign(CScriptValue* value);

    virtual uint8_t Typeid() const { return TYPE_STRING; }
    virtual std::string ToString() const { return value; }
    virtual std::string Typename() const { return "[object String]"; }
};

class CScriptSymbol : public CScriptString
{
public:
    CScriptSymbol();
    CScriptSymbol(CScriptValue* root, CScriptValue* prototype, const uint32_t flags = 0);


    inline CScriptSymbol* Ref()
    {
        CScriptValue::Ref();
        return this;
    }


    virtual CScriptValue* Copy(CScriptValue* root = Global(), const uint32_t flags = 0) const { return new CScriptSymbol(root, const_cast<CScriptSymbol*>(this), Flags() | flags); }

    virtual uint8_t Typeid() const { return TYPE_SYMBOL; }
    virtual std::string Typename() const { return "[object Symbol]"; }
};

class CScriptNumber : public CScriptObject
{
public:
    uint256 bn;
    int64_t n;

    CScriptNumber();
    CScriptNumber(CScriptValue* root, CScriptValue* prototype, const uint32_t flags = 0);

    inline CScriptNumber* Ref()
    {
        CScriptValue::Ref();
        return this;
    }

    virtual CScriptValue* Copy(CScriptValue* root = Global(), const uint32_t flags = 0) const { return new CScriptNumber(root, const_cast<CScriptNumber*>(this), Flags() | flags); }

    virtual bool IsNumber() const { return true; }
    virtual CScriptNumber* AsNumber() { return this; }


    virtual bool IsInteger() const { return !(Flags() & FLAG_REAL) && !(Flags() & FLAG_BOOLEAN) && !(Flags() & FLAG_BIGNUM); }
    virtual void AssignInteger(int64_t number)
    {
        if (Flags() & FLAG_BIGNUM) {
            bn = uint256(number);
        } else if (Flags() & FLAG_REAL) {
            *((double*)&n) = (double)number;
        } else if (Flags() & FLAG_BOOLEAN) {
            n = number > 0 ? 1 : 0;
        } else {
            n = number;
        }
    }
    virtual void SetInteger(int64_t number)
    {
        n = number;
        if (flags & FLAG_BIGNUM) flags &= ~FLAG_BIGNUM;
        if (flags & FLAG_REAL) flags &= ~FLAG_REAL;
        if (flags & FLAG_BOOLEAN) flags &= ~FLAG_BOOLEAN;
    }
    virtual int64_t AsInteger() const
    {
        if (Flags() & FLAG_BIGNUM)
            return bn.GetUint64(0);
        return n;
    }
    virtual bool IsBoolean() const { return (Flags() & FLAG_BOOLEAN); }
    virtual void AssignBoolean(bool value)
    {
        if (Flags() & FLAG_BIGNUM) {
            bn = uint256(value ? 1 : 0);
        } else if (Flags() & FLAG_REAL) {
            *((double*)&n) = value ? 1.0 : 0.0;
        } else {
            n = value ? 1 : 0;
        }
    }
    virtual void SetBoolean(bool number)
    {
        n = number ? 1 : 0;
        if (flags & FLAG_BIGNUM) flags &= ~FLAG_BIGNUM;
        if (flags & FLAG_REAL) flags &= ~FLAG_REAL;
        if (!(flags & FLAG_BOOLEAN)) flags |= FLAG_BOOLEAN;
    }
    virtual bool AsBoolean() const
    {
        if (Flags() & FLAG_BIGNUM)
            return bn.GetUint64(0) > 0 ? true : false;
        return n > 0 ? true : false;
    }
    virtual bool IsFloat() const { return (Flags() & FLAG_REAL); }
    virtual void AssignFloat(double number)
    {
        if (Flags() & FLAG_BIGNUM) {
            bn = uint256((int64_t)number);
        } else if (Flags() & FLAG_REAL) {
            *((double*)&n) = number;
        } else if (Flags() & FLAG_BOOLEAN) {
            n = number > 0.0 ? 1 : 0;
        } else {
            n = (int64_t)number;
        }
    }
    virtual void SetFloat(double number)
    {
        *((double*)&n) = number;
        if (flags & FLAG_BIGNUM) flags &= ~FLAG_BIGNUM;
        if (!(flags & FLAG_REAL)) flags |= FLAG_REAL;
        if (flags & FLAG_BOOLEAN) flags &= ~FLAG_BOOLEAN;
    }
    virtual double AsFloat() const
    {
        if (Flags() & FLAG_BIGNUM)
            return bn.getdouble();
        return *((double*)&n);
    }
    virtual bool IsBignum() const { return (Flags() & FLAG_BIGNUM); }
    virtual void AssignBignum(const uint256& number)
    {
        if (Flags() & FLAG_BIGNUM) {
            bn = number;
        } else if (Flags() & FLAG_REAL) {
            *((double*)&n) = number.getdouble();
        } else if (Flags() & FLAG_BOOLEAN) {
            n = number.GetUint64(0) > 0 ? 1 : 0;
        } else {
            n = number.GetUint64(0);
        }
    }
    virtual void SetBignum(const uint256& number)
    {
        bn = number;
        if (!(flags & FLAG_BIGNUM)) flags |= FLAG_BIGNUM;
        if (flags & FLAG_REAL) flags &= ~FLAG_REAL;
        if (flags & FLAG_BOOLEAN) flags &= ~FLAG_BOOLEAN;
    }
    virtual uint256 AsBignum() const
    {
        if (Flags() & FLAG_BIGNUM)
            return bn;
        else
            return uint256(n);
    }

    virtual CScriptValue* Assign(CScriptValue* value);

    virtual uint8_t Typeid() const { return TYPE_NUMBER; }
    virtual std::string ToString() const;
    virtual std::string Typename() const { return "[object Number]"; }
};

class CScriptModule : public CScriptObject
{
public:
    CScriptFunction* entrypoint;

    CScriptModule();
    CScriptModule(CScriptValue* root, CScriptValue* prototype, const uint32_t flags = 0);
    ~CScriptModule();


    void initStdlib();

    bool Compile(const std::string& name, std::string& source, CScriptValue* root = nullptr);
    bool CompileFile(const std::string& name, std::string& path, CScriptValue* root = nullptr);

    inline CScriptModule* Ref()
    {
        CScriptValue::Ref();
        return this;
    }


    bool SetPrototype(CScriptObject* other) { return false; }

    bool HasProgram() const { return (entrypoint != nullptr); }
    CScriptFunction* Entrypoint() { return entrypoint; }

    virtual uint8_t Typeid() const { return TYPE_MODULE; }
    virtual std::string ToString() const;
    virtual std::string Typename() const { return "[object Module]"; }
};

#endif