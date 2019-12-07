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

#define BIT(x) (1 << x)

std::string VMRandName()
{
    char name[17];
    memset(name, 0, sizeof(name));
    GetRandBytes((unsigned char*)name, sizeof(name) - 1);
    return name;
}

void VMNullConstructor(CVMState* state)
{
    LogPrint(BCLog::SCRIPT, "%s", strprintf("Call %s, caller %s", __func__, state->frame.top().caller ? state->frame.top().caller->callable->type.name.c_str() : "null"));

    CVMState::Frame& frame = state->frame.top();
    CVMValue* value = state->Pop();
    if (value) {
        value->SetNull();
        value->type = CVMTypeinfo::NullType();
    }
}

CVMTypeinfo& CVMTypeinfo::NullType()
{
    static CVMTypeinfo type;
    static bool init = false;
    if (!init) {
        type.name = "null";
        type.module = "std";
        type.kind = CVMTypeinfo::Kind_Null;
        type.flags = CVMTypeinfo::Flag_Native;

        static CVMTypeinfo ctor;
        ctor.name = "__gs_null_constructor__";
        ctor.module = "std";
        ctor.kind = CVMTypeinfo::Kind_Callable;
        ctor.flags = CVMTypeinfo::Flag_Function | CVMTypeinfo::Flag_Constructor | CVMTypeinfo::Flag_Native;
        ctor.addr = (int64_t)(void*)&VMNullConstructor;

        type.ctor = &ctor;

        init = true;
    }
    return type;
}

void VMUndefinedConstructor(CVMState* state)
{
    LogPrint(BCLog::SCRIPT, "%s", strprintf("Call %s, caller %s", __func__, state->frame.top().caller ? state->frame.top().caller->callable->type.name.c_str() : "null"));

    CVMState::Frame& frame = state->frame.top();
    CVMValue* value = state->Pop();
    if (value) {
        value->SetNull();
        value->type = CVMTypeinfo::UndefinedType();
    }
}

CVMTypeinfo& CVMTypeinfo::UndefinedType()
{
    static CVMTypeinfo type;
    static bool init = false;
    if (!init) {
        type.name = "undefined";
        type.module = "std";
        type.kind = CVMTypeinfo::Kind_Undefined;
        type.flags = CVMTypeinfo::Flag_Native;

        static CVMTypeinfo ctor;
        ctor.name = "__gs_undefined_constructor__";
        ctor.module = "std";
        ctor.kind = CVMTypeinfo::Kind_Callable;
        ctor.flags = CVMTypeinfo::Flag_Function | CVMTypeinfo::Flag_Constructor | CVMTypeinfo::Flag_Native;
        ctor.addr = (int64_t)(void*)&VMUndefinedConstructor;

        type.ctor = &ctor;

        init = true;
    }
    return type;
}

void VMVoidConstructor(CVMState* state)
{
    LogPrint(BCLog::SCRIPT, "%s", strprintf("Call %s, caller %s", __func__, state->frame.top().caller ? state->frame.top().caller->callable->type.name.c_str() : "null"));

    CVMState::Frame& frame = state->frame.top();
    CVMValue* value = state->Pop();
    if (value) {
        value->SetNull();
        value->type = CVMTypeinfo::VoidType();
    }
}

CVMTypeinfo& CVMTypeinfo::VoidType()
{
    static CVMTypeinfo type;
    static bool init = false;
    if (!init) {
        type.name = "void";
        type.module = "std";
        type.kind = CVMTypeinfo::Kind_Null;
        type.flags = CVMTypeinfo::Flag_Native | CVMTypeinfo::Flag_Void;

        static CVMTypeinfo ctor;
        ctor.name = "__gs_void_constructor__";
        ctor.module = "std";
        ctor.kind = CVMTypeinfo::Kind_Callable;
        ctor.flags = CVMTypeinfo::Flag_Function | CVMTypeinfo::Flag_Constructor | CVMTypeinfo::Flag_Native;
        ctor.addr = (int64_t)(void*)&VMVoidConstructor;

        type.ctor = &ctor;

        init = true;
    }
    return type;
}

void VMStringConstructor(CVMState* state)
{
    LogPrint(BCLog::SCRIPT, "%s", strprintf("Call %s, caller %s", __func__, state->frame.top().caller ? state->frame.top().caller->callable->type.name.c_str() : "null"));

    CVMState::Frame& frame = state->frame.top();
    CVMValue* value = state->Pop();
    if (value) {
        value->SetNull();
        value->type = CVMTypeinfo::StringType();
        if (frame.arguments->Length() > 0) value->Assign(frame.arguments->GetArrayElement(0));
    }
}

CVMTypeinfo& CVMTypeinfo::StringType()
{
    static CVMTypeinfo type;
    static bool init = false;
    if (!init) {
        type.name = "String";
        type.module = "std";
        type.kind = CVMTypeinfo::Kind_String;
        type.flags = CVMTypeinfo::Flag_Native;

        static CVMTypeinfo ctor;
        ctor.name = "__gs_string_constructor__";
        ctor.module = "std";
        ctor.kind = CVMTypeinfo::Kind_Callable;
        ctor.flags = CVMTypeinfo::Flag_Function | CVMTypeinfo::Flag_Constructor | CVMTypeinfo::Flag_Native;
        ctor.addr = (int64_t)(void*)&VMStringConstructor;

        type.ctor = &ctor;

        init = true;
    }
    return type;
}

void VMSymbolConstructor(CVMState* state)
{
    LogPrint(BCLog::SCRIPT, "%s", strprintf("Call %s, caller %s", __func__, state->frame.top().caller ? state->frame.top().caller->callable->type.name.c_str() : "null"));

    CVMState::Frame& frame = state->frame.top();
    CVMValue* value = state->Pop();
    if (value) {
        value->SetNull();
        value->type = CVMTypeinfo::SymbolType();
        if (frame.arguments->Length() > 0) value->Assign(frame.arguments->GetArrayElement(0));
    }
}

CVMTypeinfo& CVMTypeinfo::SymbolType()
{
    static CVMTypeinfo type;
    static bool init = false;
    if (!init) {
        type.name = "Symbol";
        type.module = "std";
        type.kind = CVMTypeinfo::Kind_String;
        type.flags = CVMTypeinfo::Flag_Native | CVMTypeinfo::Flag_Symbol;

        static CVMTypeinfo ctor;
        ctor.name = "__gs_symbol_constructor__";
        ctor.module = "std";
        ctor.kind = CVMTypeinfo::Kind_Callable;
        ctor.flags = CVMTypeinfo::Flag_Function | CVMTypeinfo::Flag_Constructor | CVMTypeinfo::Flag_Native;
        ctor.addr = (int64_t)(void*)&VMSymbolConstructor;

        type.ctor = &ctor;

        init = true;
    }
    return type;
}

void VMArrayConstructor(CVMState* state)
{
    LogPrint(BCLog::SCRIPT, "%s", strprintf("Call %s, caller %s", __func__, state->frame.top().caller ? state->frame.top().caller->callable->type.name.c_str() : "null"));

    CVMState::Frame& frame = state->frame.top();
    CVMValue* value = state->Pop();
    if (value) {
        value->SetNull();
        value->type = CVMTypeinfo::ArrayType();
        if (frame.arguments->Length() > 0) value->Assign(frame.arguments);
    }
}

CVMTypeinfo& CVMTypeinfo::ArrayType()
{
    static CVMTypeinfo type;
    static bool init = false;
    if (!init) {
        type.name = "Array";
        type.module = "std";
        type.kind = CVMTypeinfo::Kind_Array;
        type.flags = CVMTypeinfo::Flag_Native;

        static CVMTypeinfo ctor;
        ctor.name = "__gs_array_constructor__";
        ctor.module = "std";
        ctor.kind = CVMTypeinfo::Kind_Callable;
        ctor.flags = CVMTypeinfo::Flag_Function | CVMTypeinfo::Flag_Constructor | CVMTypeinfo::Flag_Native;
        ctor.addr = (int64_t)(void*)&VMArrayConstructor;

        type.ctor = &ctor;

        init = true;
    }
    return type;
}

void VMObjectConstructor(CVMState* state)
{
    LogPrint(BCLog::SCRIPT, "%s", strprintf("Call %s, caller %s", __func__, state->frame.top().caller ? state->frame.top().caller->callable->type.name.c_str() : "null"));

    CVMState::Frame& frame = state->frame.top();
    CVMValue* value = state->Pop();
    if (value) {
        value->SetNull();
        value->type = CVMTypeinfo::ObjectType();
        if (frame.arguments->Length() > 0) value->Assign(frame.arguments);
    }
}


CVMTypeinfo& CVMTypeinfo::ObjectType()
{
    static CVMTypeinfo type;
    static bool init = false;
    if (!init) {
        type.name = "Object";
        type.module = "std";
        type.kind = CVMTypeinfo::Kind_Object;
        type.flags = CVMTypeinfo::Flag_Native;

        static CVMTypeinfo ctor;
        ctor.name = "__gs_object_constructor__";
        ctor.module = "std";
        ctor.kind = CVMTypeinfo::Kind_Callable;
        ctor.flags = CVMTypeinfo::Flag_Function | CVMTypeinfo::Flag_Constructor | CVMTypeinfo::Flag_Native;
        ctor.addr = (int64_t)(void*)&VMObjectConstructor;

        type.ctor = &ctor;

        init = true;
    }
    return type;
}

/*
CVMValue::CVMValue() : refCounter(1) {
    SetNull();
}

CVMValue::CVMValue(CVMValue *root, const CModuleTypeinfo &type, const std::string &name, const uint64_t flags = 0, const std::vector<char> &data = std::vector<char>()) : refCounter(1) {
    SetNull();

    this->root = root;
    this->name = name;
    this->kind = kind;
    this->bits = bits;
    this->flags = flags;
    this->data = data;
}

CVMValue::CVMValue(CVMValue *value) : refCounter(1)
{
    Init(value);
}

CVMValue::~CVMValue() {
    for (std::vector <CVMValue*>::iterator it = values.begin(); it != values.end(); it++) {
        (*it)->Drop();
    }
    for (std::unordered_map <std::string, CVMValue*>::iterator it = variables.begin(); it != variables.end(); it++) {
        (*it).second->Drop();
    }
    if (prototype) prototype->Drop();
    if (arguments) arguments->Drop();
    if (constants) constants->Drop();
    if (value) value->Drop();
    if (root) root->Drop();
    if (module) module->Drop();
}

void CVMValue::Init(CVMValue *initializer) {
    if (initializer) {      
        if (initializer->ns) ns = initializer->ns->Grab();
        name = initializer->name;
        kind = initializer->kind;
        data.clear();
        if (initializer->data.size()) data.insert(data.begin(), initializer->data.begin(), initializer->data.end());

            
        for (std::vector <CVMValue*>::iterator it = initializer->values.begin(); it != initializer->values.end(); it++) {
            this->values.push_back((*it) ? ((*it)->IsCopable() ? new CVMValue((*it)) : (*it)->Grab()) : nullptr);
        }
        for (std::unordered_map <std::string, CVMValue*>::iterator it = initializer->variables.begin(); it != initializer->variables.end(); it++) {
            this->variables.insert(std::make_pair((*it).first, (*it).second ? ((*it).second->IsCopable() ? new CVMValue((*it).second) : (*it).second->Grab()) : nullptr));
        }
    } else {
        ns = nullptr;
        name.clear();
        kind = Kind_Null;
        data.clear();
        values.clear();
        variables.clear();
    }
}

void CVMValue::Assign(CVMValue *value) {
    if (IsReference() || IsVariable()) {
        this->value = value->Grab();
    }
}


int8_t VMDecode8(CVMState *state, CVMBytecode::const_iterator &code) {
    int8_t value = *(code + state->frame.top().pc); state->frame.top().pc++;
    return value;
}

uint8_t VMDecodeU8(CVMState *state, CVMBytecode::const_iterator &code) {
    int8_t value = (int8_t) *(code + state->frame.top().pc); state->frame.top().pc++;
    return *((uint8_t *) &value);
}

int16_t VMDecode16(CVMState *state, CVMBytecode::const_iterator &code) {
    return le16toh(((int16_t) VMDecode8(state, code)) | (((int16_t) VMDecode8(state, code)) << 8));
}

uint16_t VMDecodeU16(CVMState *state, CVMBytecode::const_iterator &code) {
    return le16toh(((uint16_t) VMDecodeU8(state, code)) | (((uint16_t) VMDecodeU8(state, code)) << 8));
}

int32_t VMDecode32(CVMState *state, CVMBytecode::const_iterator &code) {
    return le32toh(((int32_t) VMDecode16(state, code)) | (((int32_t) VMDecode16(state, code)) << 16));
}

uint32_t VMDecodeU32(CVMState *state, CVMBytecode::const_iterator &code) {
    return le32toh(((uint32_t) VMDecodeU16(state, code)) | (((uint32_t) VMDecodeU16(state, code)) << 16));
}

size_t VMDecodeAddr(CVMState *state, CVMBytecode::const_iterator &code) {
    return le32toh(((size_t) VMDecodeU16(state, code)) | (((size_t) VMDecodeU16(state, code)) << 16));
}

int64_t VMDecode64(CVMState *state, CVMBytecode::const_iterator &code) {
    return le64toh(((int64_t) VMDecode32(state, code)) | (((int64_t) VMDecode32(state, code)) << 32));
}

uint64_t VMDecodeU64(CVMState *state, CVMBytecode::const_iterator &code) {
    return le32toh(((uint64_t) VMDecodeU32(state, code)) | (((uint64_t) VMDecodeU32(state, code)) << 32));
}

uint16_t VMDecodeInstr(CVMState *state, CVMBytecode::const_iterator &code) {
    uint16_t instr = le16toh(((uint16_t) VMDecodeU8(state, code)) | (((uint16_t) VMDecodeU8(state, code)) << 8));
    return instr;
}

std::string VMDecodeString(CVMState *state, CVMBytecode::const_iterator &code) {
    size_t len = VMDecodeAddr(state, code);
    std::string str = std::string(code + state->frame.top().pc, code + state->frame.top().pc + len);
    state->frame.top().pc += len;
    return str;
}

CVMValue *VMDecodeValue(CVMState *state, CVMBytecode::const_iterator &code) {
    size_t addr = VMDecodeAddr(state, code)
    size_t size = VMDecodeAddr(state, code);
    std::vector<char> data(code + addr, code + addr + size); state->frame.top().pc += 8;

    CVMValue *value = new CVMValue();
    if (value->Decode(data)) {
        return value;
    }

    error("%s : VMValue Decode FAILED pc: %d, sp %i, dp %i", __func__, state->frame.top().pc, state->stack.size() - 1, state->dp, state->bp);
    delete value;
    return nullptr;
}


CVMValue *VMValue(CVMState *state, const std::string &name, const bool g = false) {
    CVMState::Frame &frame = state->frame.top();
    CVMValue *space = frame.callable; if (g && space->ns) space = space->ns;
    return space->variables.count(name) ? space->variables[name] : nullptr;
}


void VMPush(CVMState *state, CVMValue *value) {
    assert(state->stack.size() < CVMState::MAX_STACK);
    state->stack.push(value);
}

CVMValue *VMPop(CVMState *state) {
    assert(state->stack.size() > 0);
    CVMValue *value = state->stack.top(); state->stack.pop();
    return value;
}
CVMValue *VMTop(CVMState *state) {
    assert(state->stack.size() >= 1);
    return state->stack.top();
}


CVMState::CVMState() { 
    Flush();
}

CVMState::~CVMState() {
}

bool VMStep(CVMState *state, CVMValue *callable, CVMState::Frame &frame, CVMBytecode::const_iterator &code) {
    uint16_t instr = VMDecodeInstr(state, code);
    uint8_t op = (uint8_t) (instr); // opcode
    uint8_t md = (uint8_t) (instr >> 8); // modifier (FP DP и прочее колдунство)
    switch (op)
    {
        case CVMOp_Push: 
        {   
            assert(state->stack.size() < CVMState::MAX_STACK);
        } break;
        case CVMOp_Pop: 
        {   
            assert(state->stack.size() > 0);              
            state->stack.pop();
        } break;
        case CVMOp_Load: 
        {
            assert(state->stack.size() < CVMState::MAX_STACK);
            state->stack.push(VMDecodeValue(state, code));
        } break;
        default:
        case CVMOp_Nop: return true;
    }

    return true;
}

bool VMCall(CVMState *state, CVMValue *callable) {
    assert(state != nullptr);

    CVMState::Frame newFrame;

    state->Flush();
    CVMState::Frame &frame = state->frame.top();
    CVMBytecode::const_iterator code = callable->data.begin();
    while ((code + frame.pc) < callable->data.end()) {
        if (!VMStep(state, callable, frame, code))
            return false;
    }

    return true;
}

bool VMCall(CVMValue *callable) {
    CVMState state;
    return VMCall(&state, callable);
}
*/