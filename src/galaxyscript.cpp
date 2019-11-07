// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <chainparams.h>
#include <galaxycash.h>
#include <hash.h>
#include <memory>
#include <pow.h>
#include <stack>
#include <stdint.h>
#include <uint256.h>
#include <util.h>

CScriptValue* CScriptValue::Global()
{
    static CScriptModule module;
    return &module;
}

CScriptValue* CScriptValue::Undefined()
{
    static CScriptValue undefinedTy(Global(), nullptr, FLAG_INVALID | FLAG_CONSTANT);
    return &undefinedTy;
}

CScriptValue* CScriptValue::Null()
{
    static CScriptValue nullTy(Global(), nullptr, FLAG_NULL | FLAG_CONSTANT);
    return &nullTy;
}

CScriptValue* CScriptValue::Void()
{
    static CScriptValue voidTy(Global(), nullptr, FLAG_VOID | FLAG_CONSTANT);
    return &voidTy;
}

CScriptValue::CScriptValue() : refCount(1)
{
}

CScriptValue::CScriptValue(CScriptValue* root, CScriptValue* prototype, const uint32_t flags) : root(root ? const_cast<CScriptValue*>(root) : nullptr), flags(flags)
{
    if (prototype && prototype != Null()) {
        for (std::map<std::string, CScriptValue*>::iterator it = prototype->keys.begin(); it != prototype->keys.end(); it++) {
            SetKeyValue((*it).first, (*it).second, 0);
        }
        for (std::vector<CScriptValue*>::iterator it = prototype->values.begin(); it != prototype->values.end(); it++) {
            Push((*it));
        }
    }
}

CScriptValue::~CScriptValue()
{
    for (std::map<std::string, CScriptValue*>::iterator it = keys.begin(); it != keys.end(); it++) {
        if ((*it).second) (*it).second->Unref();
    }
    for (std::vector<CScriptValue*>::iterator it = values.begin(); it != values.end(); it++) {
        if ((*it)) (*it)->Unref();
    }
}

void CScriptValue::SetNull()
{
    if (!IsNull()) flags |= FLAG_NULL;
    for (std::map<std::string, CScriptValue*>::iterator it = keys.begin(); it != keys.end(); it++) {
        (*it).second->Unref();
    }
    keys.clear();

    for (std::vector<CScriptValue*>::iterator it = values.begin(); it != values.end(); it++) {
        (*it)->Unref();
    }
    values.clear();
}

void CScriptValue::SetUndefined()
{
    if (!IsNull()) flags |= FLAG_INVALID;
    for (std::map<std::string, CScriptValue*>::iterator it = keys.begin(); it != keys.end(); it++) {
        if ((*it).second) (*it).second->Unref();
    }
    keys.clear();

    for (std::vector<CScriptValue*>::iterator it = values.begin(); it != values.end(); it++) {
        if ((*it)) (*it)->Unref();
    }
    values.clear();
}

CScriptValue* CScriptValue::Assign(CScriptValue* invalue)
{
    if (invalue) {
        if (invalue->IsNull()) {
            SetNull();
            return const_cast<CScriptValue*>(invalue);
        }
        if (invalue->IsUndefined()) {
            SetUndefined();
            return const_cast<CScriptValue*>(invalue);
        }
        for (std::map<std::string, CScriptValue*>::iterator it = invalue->keys.begin(); it != invalue->keys.end(); it++) {
            SetKeyValue((*it).first, (*it).second, 0);
        }
        for (std::vector<CScriptValue*>::iterator it = invalue->values.begin(); it != invalue->values.end(); it++) {
            Push((*it));
        }

        name = invalue->name;
        value = invalue->value;
        flags = invalue->flags;
    } else {
        SetNull();
    }
    return const_cast<CScriptValue*>(invalue);
}


CScriptValue* CScriptValue::SetKeyValue(const std::string& key, CScriptValue* value, const uint32_t flags)
{
    if (!keys.count(key)) {
        keys[key] = new CScriptVariable(this, value, flags);
        keys[key]->SetName(key);
    } else
        keys[key]->Assign(value);

    return const_cast<CScriptValue*>(value);
}

CScriptValue* CScriptValue::CreateTyped(const uint8_t type, CScriptValue* prototype, CScriptValue* root)
{
    uint32_t flags = 0;
    if (type == TYPE_UNDEFINED)
        flags |= FLAG_INVALID;
    else if (type == TYPE_NULL)
        flags |= FLAG_NULL;
    else if (type == TYPE_VOID)
        flags |= FLAG_VOID;
    else if (type == TYPE_SYMBOL)
        flags |= FLAG_SYMBOL | FLAG_STRING;
    else if (type == TYPE_STRING)
        flags |= FLAG_STRING;


    switch (type) {
    case TYPE_POINTER:
        return (CScriptValue*)(new CScriptPointer(root, prototype, flags));
    case TYPE_VARIABLE:
    case TYPE_PROPERTY:
        return (CScriptValue*)(new CScriptVariable(root, prototype, flags));
    case TYPE_OBJECT:
        return (CScriptValue*)(new CScriptObject(root, prototype, flags));
    case TYPE_ARRAY:
        return (CScriptValue*)(new CScriptArray(root, prototype, flags));
    case TYPE_FUNCTION:
        return (CScriptValue*)(new CScriptFunction(root, prototype, flags));
    case TYPE_ASYNC_FUNCTION:
        return (CScriptValue*)(new CScriptAsyncFunction(root, prototype, flags));
    case TYPE_MODULE:
        return (CScriptValue*)(new CScriptModule(root, prototype, flags));
    case TYPE_NUMBER:
        return (CScriptValue*)(new CScriptNumber(root, prototype, flags));
    case TYPE_STRING:
        return (CScriptValue*)(new CScriptString(root, prototype, flags));
    case TYPE_SYMBOL:
        return (CScriptValue*)(new CScriptSymbol(root, prototype, flags));
    case TYPE_UNDEFINED:
    case TYPE_NULL:
    case TYPE_VOID:
    default:
        return new CScriptValue(root, prototype, flags);
    }
}

CScriptObject::CScriptObject() : CScriptValue()
{
    this->prototype = SetOwnPropertyT<CScriptObject>("prototype", nullptr);
}

CScriptObject::CScriptObject(CScriptValue* root, CScriptValue* inprototype, const uint32_t flags) : CScriptValue(root, inprototype && inprototype->IsObject() ? inprototype->AsObject() : Null(), flags), objectType(inprototype && inprototype->IsObject() ? inprototype->AsObject()->objectType : "Object")
{
    CScriptValue* pprototype = inprototype ? inprototype->Copy(this) : nullptr;
    this->prototype = SetOwnPropertyT<CScriptObject>("prototype", pprototype);
}


std::string CScriptObject::ToString() const
{
    std::string ret = "{";
    for (std::map<std::string, CScriptValue*>::iterator it = keys.begin(); it != keys.end(); it++) {
        ret += (*it).first + ": " + ((*it).second ? (*it).second->ToString() : "undefined");
    }
    ret += "}";
    return ret;
}


CScriptArray::CScriptArray(CScriptValue* root, CScriptValue* inprototype, const uint32_t flags) : CScriptObject(root, inprototype, flags)
{
    this->objectType = "Array";
}

std::string CScriptArray::ToString() const
{
    std::string ret = "[";
    for (std::vector<CScriptValue*>::iterator it = values.begin(); it != values.end(); it++) {
        if (it != values.begin()) ret += ", ";
        ret += (*it)->ToString();
    }
    ret += "]";
    return ret;
}

bool CScriptArray::Includes(CScriptValue* value, size_t from) const
{
    if (from >= this->values.size()) return false;
    for (size_t i = from; i < this->values.size(); i++) {
        if (this->values[i] == value) return true;
    }
    return false;
}


CScriptFunction::CScriptFunction(CScriptValue* root, CScriptValue* inprototype, const uint32_t flags) : CScriptObject(root, inprototype, flags)
{
    this->objectType = "Function";
    if (HasOwnProperty("this"))
        this->thisArg = GetOwnPropertyT<CScriptVariable>("thisArg");
    else
        this->thisArg = SetOwnPropertyT<CScriptVariable>("thisArg", new CScriptVariable(this, nullptr, 0));

    if (HasOwnProperty("name"))
        this->functionName = GetOwnPropertyT<CScriptSymbol>("name");
    else
        this->functionName = SetOwnPropertyT<CScriptSymbol>("name", new CScriptSymbol(this, nullptr, 0));

    if (HasOwnProperty("displayName"))
        this->displayName = GetOwnPropertyT<CScriptSymbol>("displayName");
    else
        this->displayName = SetOwnPropertyT<CScriptSymbol>("displayName", new CScriptSymbol(this, nullptr, 0));

    if (HasOwnProperty("arguments"))
        this->arguments = GetOwnPropertyT<CScriptArray>("arguments");
    else
        this->arguments = SetOwnPropertyT<CScriptArray>("arguments", new CScriptArray(this, nullptr, 0));
}

std::string CScriptFunction::ToString() const
{
    std::string ret = "function(";

    ret += ") {";
    if (native) {
        ret += "[native code]";
    }
    ret += "}";
    return ret;
}

CScriptAsyncFunction::CScriptAsyncFunction(CScriptValue* root, CScriptValue* inprototype, const uint32_t flags) : CScriptFunction(root, inprototype, flags)
{
    this->objectType = "AsyncFunction";
}

std::string CScriptAsyncFunction::ToString() const
{
    return std::string("async ") + CScriptFunction::ToString();
}

CScriptNumber::CScriptNumber() : CScriptObject()
{
}

CScriptNumber::CScriptNumber(CScriptValue* root, CScriptValue* inprototype, const uint32_t flags) : CScriptObject(root, inprototype, flags)
{
    this->objectType = "Number";
    if (this->prototype && this->prototype->IsNumber())
        this->Assign(inprototype);
}

CScriptValue* CScriptNumber::Assign(CScriptValue* other)
{
    if (other == nullptr || other == Null() || other == Undefined() || other == Void() || !other->IsNumber()) {
        if (IsFloat())
            AssignFloat(0.0);
        else if (IsBignum())
            AssignBignum(uint256(0));
        else if (IsBoolean())
            AssignBoolean(false);
        else
            AssignInteger(0);
    } else {
        CScriptNumber* number = other->AsValue()->AsNumber();
        if (number->IsFloat())
            SetFloat(number->AsFloat());
        else if (number->IsBoolean())
            SetBoolean(number->AsBoolean());
        else if (number->IsBignum())
            SetBignum(number->AsBignum());
        else
            SetInteger(number->AsInteger());
    }
    return dynamic_cast<CScriptValue*>(const_cast<CScriptNumber*>(this));
}

CScriptModule::CScriptModule() : CScriptObject()
{
    this->objectType = "Module";
}

CScriptModule::CScriptModule(CScriptValue* root, CScriptValue* inprototype, const uint32_t flags) : CScriptObject(root, inprototype, flags)
{
    this->objectType = "Module";
}

/*CScriptFloat::CScriptFloat(const CScriptValueRef& root, const double value, const uint32_t flags) : CScriptValue(root, value, flags), float_value(value)
{
    std::ostringstream s(this->value);
    std::set_precission(10);
    s.imbue(std::locale::classic());
    s << value;
}

CScriptInteger::CScriptInteger(const CScriptValueRef& root, const int64_t value, const uint32_t flags) : CScriptValue(root, "", flags), int_value(value)
{
    std::ostringstream s(this->value);
    s.imbue(std::locale::classic());
    s << value;
}*/
