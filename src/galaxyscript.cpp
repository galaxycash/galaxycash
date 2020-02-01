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

#define BIT(x) (1 << x)

std::string VMRandName()
{
    char name[17];
    memset(name, 0, sizeof(name));
    GetRandBytes((unsigned char*)name, sizeof(name) - 1);
    return name;
}

CJSReference* CJSObject::NewReference(const std::string& name)
{
    CJSReference* ref = this->AddOwnProperty(name, CJSPropertyDescriptor(CJSPropertyDescriptor::Value, new CJSReference()))->descriptor.value->AsReference();
    return ref;
}

CJSValue* CJSReference::GetValue()
{
    if (value) return value;
    if (root && !name.empty()) {
        CJSValue* tree = root;
        while (!value && tree) {
            if (tree->IsObject()) {
                CJSObject* scope = tree->AsObject();
                value = scope->GetOwnPropertyValue(name);
            }
            tree = tree->root;
        }
        return value ? value->Grab() : new CJSNull();
    }
    return new CJSNull();
}