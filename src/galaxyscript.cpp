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

void CVMValue::Init(CVMValue *initializer) {
    space = value->space;
    name = value->name;
    kind = value->kind;
    number = value->number;
    pointer = value->pointer;
    data.clear();
    if (value->data.size()) data.insert(data.end(), value->data.begin(), value->data.end());

        
    for (std::vector <CVMValue*>::iterator it = value->values.begin(); it != value->values.end(); it++) {
        this->values.push_back((*it)->IsCopable() ? new CVMValue((*it)) : (*it)->Grab());
    }
    for (std::unordered_map <std::string, CVMValue*>::iterator it = value->variables.begin(); it != value->variables.end(); it++) {
        this->variables.insert(std::make_pair((*it).first, (*it).second->IsCopable() ? new CVMValue((*it).second) : (*it).second->Grab()));
    }
}

void CVMValue::Assign(CVMValue *value) {
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

CVMValue *VMLoad(CVMState *state, CVMBytecode::const_iterator &code) {
    uint32_t size = VMDecodeU32(state, code);
    std::vector<char> data(code + state->frame.top().pc, code + state->frame.top().pc + size);

    CVMValue *value = new CVMValue();
    if (value->Encode(data)) {
        state->frame.top().pc += size;
        return value;
    }

    error("%s : VMValue Encode FAILED pc: %d, sp %i, dp %i", __func__, state->frame.top().pc, state->stack.size(), state->dp, state->bp);
    delete value;
    return &CVMValue::null;
}

CVMValue *VMValue(CVMState *state, const std::string &name, const bool g = false) {
    CVMState::Frame &frame = state->frame.top();
    CVMValue *space = frame.callable; if (g && space->space) space = space->space;
    return space->variables.count(name) ? space->variables[name] : &CVMValue::null;
}

CVMValue *VMValueUnref(CVMState *state, const std::string &name, const bool g = false) {
    CVMValue *value = VMValue(state, name, g);
    return (value ? value->Unref() : &CVMValue::null);
}

CVMValue *VMValueKey(CVMState *state, const size_t addr, const std::string &name, const bool g = false) {
    CVMState::Frame &frame = state->frame.top();
    CVMValue *space = frame.callable; if (g && space->space) space = space->space;
    assert(space->values.size() > addr);
    return space->values[addr]->GetKeyValue(name);
}

CVMValue *VMValueKeyUnref(CVMState *state, const size_t addr, const std::string &name, const bool g = false) {
    CVMValue *key = VMValueKey(state, addr, name, g);
    return (key ? key->Unref() : &CVMValue::null);
}

CVMValue *VMValueIndex(CVMState *state, const size_t addr, const size_t index, const bool g = false) {
    CVMState::Frame &frame = state->frame.top();
    CVMValue *space = frame.callable; if (g && space->space) space = space->space;
    assert(space->values.size() > addr);
    CVMValue *value = *(std::begin(space->values) + addr);
    assert(value->values.size() > index);
    return *(std::begin(value->values) + index);
}

CVMValue *VMValueIndexUnref(CVMState *state, const size_t addr, const size_t index, const bool g = false) {
    CVMValue *key = VMValueIndex(state, addr, index, g);
    return (key ? key->Unref() : &CVMValue::null);
}

CVMValue *VMValueByAddr(CVMState *state, const size_t addr, const bool g = false) {
    CVMState::Frame &frame = state->frame.top();
    CVMValue *space = frame.callable; if (g && space->space) space = space->space;
    return *(std::begin(space->values) + addr);
}

CVMValue *VMValueByAddrUnref(CVMState *state, const size_t addr, const bool g = false) {
    CVMValue *value = VMValueByAddr(state, addr, g);
    return (value ? value->Unref() : &CVMValue::null);
}

void VMPush(CVMState *state, CVMValue *value) {
    assert(state->stack.size() < CVMState::MAX_STACK);
    state->stack.push(value);
}
void VMPushUnref(CVMState *state, CVMValue *value) {
    assert(state->stack.size() < CVMState::MAX_STACK);
    state->stack.push(value->Unref());
}
CVMValue *VMPop(CVMState *state) {
    assert(state->stack.size() > 0);
    CVMValue *value = state->stack.top(); state->stack.pop();
    return value;
}
CVMValue *VMPopUnref(CVMState *state) {
    CVMValue *value = VMPop(state);
    if (value->IsReference()) value = value->Unref();
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
        case CVMOp_Asm: 
        {   
            return error("asm instruction impl");
        } break;
        case CVMOp_Push: 
        {   
            assert(state->stack.size() < CVMState::MAX_STACK);
            size_t addr = VMDecodeAddr(state, code);
            if (md & BIT(2)) {
                std::string key = VMDecodeString(state, code);
                state->stack.push(VMValueKey(state, addr, key, (md & BIT(0))));
            } else if (md & BIT(1)) {
                size_t index = VMDecodeAddr(state, code);
                state->stack.push(VMValueIndex(state, addr, index, (md & BIT(0))));
            } else {
                state->stack.push(VMValueByAddr(state, addr, (md & BIT(0))));
            }
        } break;
        case CVMOp_Pop: 
        {   
            assert(state->stack.size() > 0);              
            state->stack.pop();
        } break;
        case CVMOp_Load: 
        {
            assert(state->stack.size() < CVMState::MAX_STACK);
            CVMValue *value = VMLoad(state, code);
            state->stack.push(value);
        } break;
        case CVMOp_Copy:
        {
            size_t addr = VMDecodeAddr(state, code);
            CVMValue *lvalue = VMValueByAddrUnref(state, addr, (md & BIT(0))); assert(lvalue != nullptr);
            CVMValue *rvalue = VMPop(state);
            lvalue->Init(rvalue);
        } break;
        case CVMOp_Move:
        {
            size_t addr = VMDecodeAddr(state, code);
            CVMValue *lvalue = VMValueByAddr(state, addr, (md & BIT(0))); assert(lvalue != nullptr);
            CVMValue *rvalue = VMPop(state);
            lvalue->Assign(rvalue);
        } break;
        default:
        case CVMOp_Nop: return true;
    }

    return true;
}

bool VMCall(CVMState *state, CVMValue *callable) {
    assert(state != nullptr);

    CVMState::Frame newFrame;
    newFrame.ths = callable;
    newFrame.caller = state->frame.size() ? state->frame.top().callable : nullptr;
    newFrame.callable = callable;
    state->frame.push(newFrame);

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