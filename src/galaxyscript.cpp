// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <chainparams.h>
#include <galaxycash.h>
#include <galaxyscript.h>
#include <hash.h>
#include <memory>
#include <pow.h>
#include <stack>
#include <stdint.h>
#include <uint256.h>
#include <util.h>

CScriptValue::CScriptValue()
{
}

CScriptValue::CScriptValue(const CScriptValueRef& root, const CScriptValueRef& prototype, const uint32_t flags) : root(root), flags(flags)
{
    if (prototype) {
        for (std::map<std::string, CScriptValueRef>::iterator it = prototype->keys.begin(); it != prototype->keys.end(); it++) {
        }
    }
}

CScriptValue::~CScriptValue()
{
}


CScriptValueRef CScriptValue::AddKeyValue(const std::string& key, const CScriptValueRef& value, const uint32_t flags)
{
    keys[key] = std::make_shared<CScriptVariable>(AsValue(), nullptr, flags);
    keys[key]->AsVariable()->varName = key;
    keys[key]->Assign(value);
    return value;
}

CScriptValueRef CScriptValue::CreateTyped(const uint8_t type, const CScriptValueRef& prototype, const CScriptValueRef& root)
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


    CScriptValueRef ret;
    switch (type) {
    case TYPE_POINTER:
        ret = std::make_shared<CScriptPointer>(root, prototype, flags);
        break;
    case TYPE_VARIABLE:
    case TYPE_PROPERTY:
        ret = std::make_shared<CScriptVariable>(root, prototype, flags);
        break;
    case TYPE_OBJECT:
        ret = std::make_shared<CScriptObject>(root, prototype, flags);
        break;
    case TYPE_ARRAY:
        ret = std::make_shared<CScriptArray>(root, prototype, flags);
        break;
    case TYPE_FUNCTION:
        ret = std::make_shared<CScriptFunction>(root, prototype, flags);
        break;
    case TYPE_NUMBER:
        ret = std::make_shared<CScriptNumber>(root, prototype, flags);
        break;
    case TYPE_STRING:
    case TYPE_SYMBOL:
        ret = std::make_shared<CScriptValue>(root, prototype, flags);
        break;
    case TYPE_UNDEFINED:
    case TYPE_NULL:
    case TYPE_VOID:
    default:
        ret = std::make_shared<CScriptValue>(root, prototype, flags);
    }
    return ret;
}


CScriptObject::CScriptObject(const CScriptValueRef& root, const CScriptValueRef& prototype, const uint32_t flags) : CScriptValue(root, prototype, flags), objectType(prototype ? prototype->objectType : "Object")
{
    prototype = AddKeyValue("prototype", prototype ? prototype->Copy(this->AsValue()) : this->Copy())->AsObject();
}


std::string CScriptObject::ToString() const
{
    std::string ret = "{";
    for (std::map<std::string, CScriptValueRef>::iterator it = keys.begin(); it != keys.end(); it++) {
        ret += (*it).first + ": " + ((*it).second ? (*it).second->ToString() : "undefined");
    }
    ret += "}";
    return ret;
}


CScriptArray::CScriptArray(const CScriptValueRef& root, const CScriptValueRef& prototype, const uint32_t flags) : CScriptObject(root, prototype, flags)
{
    this->objectType = "Array";
}

std::string CScriptArray::ToString() const
{
    std::string ret = "[";
    for (std::vector<CScriptValueRef>::iterator it = values.begin(); it != values.end(); it++) {
        if (it != values.begin()) ret += ", ";
        ret += (*it)->ToString();
    }
    ret += "]";
    return ret;
}

bool CScriptArray::Includes(const CScriptValueRef& value, size_t from) const
{
    if (from >= this->values.size()) return false;
    for (size_t i = from; i < this->values.size(); i++) {
        if (this->values[i] == value) return true;
    }
    return false;
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
