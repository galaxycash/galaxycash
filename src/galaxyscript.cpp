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

static CVMState g_defaultVMState;
static CVMState *g_currentVMState = &g_defaultVMState;

static void VMSetState(CVMState *state) {
    if (state) {
        g_currentVMState = state;
    } else {
        g_currentVMState = &g_defaultVMState;
    }
}


CVMTypeinfo::CVMTypeinfo() : super(nullptr), ctor(nullptr), dtor(nullptr) { SetNull(); }
CVMTypeinfo::CVMTypeinfo(CVMTypeinfo* type) : version(type->version), module(type->module), index(type->index), super(type->super ? new CVMTypeinfo(type->super) : nullptr), kind(type->kind), flags(type->flags), ctor(type->ctor ? new CVMTypeinfo(type->ctor) : nullptr), dtor(type->dtor ? new CVMTypeinfo(type->dtor) : nullptr)
{
}
CVMTypeinfo::~CVMTypeinfo()
{
    if (super) delete super;
    if (ctor) delete ctor;
    if (dtor) delete dtor;
}
void CVMTypeinfo::SetNull()
{
    version = 1;
    module.clear();
    kind = Kind_Null;
    flags = 0;
    bits = 0;
    if (super) delete super;
    if (ctor) delete ctor;
    if (dtor) delete dtor;
    super = ctor = dtor = nullptr;
}


void CVMDeclare::Rename(const std::string &name, CVMModule *module) {
    if (this->base) this->base->Rename(name, module);

    if (this->module && this->module->name == name)
        this->module = module;
}

CVMValue *CVMDeclare::Exec(CVMValue *state) {
    if (!state) state = g_currentVMState;

    CVMValue *value = nullptr;
    switch (this->as) {
        case DeclareProperty: 
        {
            CVMValue *object = state->Pop();
            if (object) {
                if (object->GetKeyValue(this->name)) {
                    value = object->GetKeyValue(this->name);
                    if (this->base) 
                        value->Assign(this->base->Exec(state));
                    if (this->type)
                        value->Assign(new CVMValue(object, *this->type, this->name, this->data ? *this->data : std::vector<char>()));
                } else 
                    value = object->SetKeyValue(this->name, this->base ? this->base->Exec(state) : new CVMValue(object, *this->type, this->name, this->data ? *this->data : std::vector<char>()));
            }
        } break;
        case DeclareVar: 
        {
            CVMValue *object = state->functions.empty() ? state->GetGlobal() : state->functions.top();
            if (object) {
                if (object->GetKeyValue(this->name)) {
                    value = object->GetKeyValue(this->name);
                    if (this->base) 
                        value->Assign(this->base->Exec(state));
                    if (this->type)
                        value->Assign(new CVMValue(object, *this->type, this->name, this->data ? *this->data : std::vector<char>()));
                } else 
                    value = object->SetKeyValue(this->name, this->base ? this->base->Exec(state) : new CVMValue(object, *this->type, this->name, this->data ? *this->data : std::vector<char>()));
            }
        } break;
        case DeclareConst: 
        {
            CVMValue *object = state->functions.empty() ? state->GetGlobal() : state->functions.top();
            if (object) {
                if (object->GetKeyValue(this->name)) {
                    value = object->GetKeyValue(this->name);
                } else 
                    value = object->SetKeyValue(this->name, this->base ? this->base->Exec(state) : new CVMValue(object, *this->type, this->name, this->data ? *this->data : std::vector<char>()));
            }
        } break;
        case DeclareScope:
        default:
        {
            if (state->GetScope()->GetKeyValue(this->name)) {
                value = state->GetScope()->GetKeyValue(this->name);
                if (this->base) 
                    value->Assign(this->base->Exec(state));
                if (this->type)
                    value->Assign(new CVMValue(state->GetScope(), *this->type, this->name, this->data ? *this->data : std::vector<char>()));
            } else 
                value = state->GetScope()->SetKeyValue(this->name, this->base ? this->base->Exec(state) : new CVMValue(state->GetScope(), *this->type, this->name, this->data ? *this->data : std::vector<char>()));
        }  break;
    }

    return value;
}

CVMDeclare *VMDeclareFromValue(CVMModule *module, CVMValue *value) {
    CVMDeclare *declare = module->DeclareVariable(value->name);
    declare->base = value->root ? VMDeclareFromValue(value->root->module, value->root) : nullptr;
    declare->name = value->name;
    declare->type = value->type.module->GetType(value->type.name);
    declare->data = value->data.empty() ? nullptr : module->DeclareData(value->data);
    declare->as = value->type.IsProperty() ? CVMDeclare::DeclareProperty : CVMDeclare::DeclareVar;
    return declare;
}


CVMModule::CVMModule() {}
CVMModule::CVMModule(const CVMModule &module) : name(module.name) {
    for (size_t i = 0; i < module.types.size(); i++) {
        types.push_back(CVMTypeinfo(&module.types[i]));
    }
    for (size_t i = 0; i < module.symbols.size(); i++) {
        symbols.push_back(module.symbols[i]);
    }
}
CVMModule::~CVMModule() {}

bool CVMModule::Link() {
    return true;
}

bool CVMModule::Rename(const std::string &name) {
    std::string oldName = this->name; this->name = name;
    for (size_t i = 0; i < types.size(); i++) {
        if (types[i].module && types[i].module->name == oldName) types[i].module = this;
    }
    for (size_t i = 0; i < symbols.size(); i++) {
        symbols[i].Rename(oldName, this);
    }
    return Link();
}

CVMTypeinfo *CVMModule::DeclareType(const std::string &name) {
    for (size_t i = 0; i < types.size(); i++) {
        if (types[i].name == name) return &types[i];
    }

    CVMTypeinfo type;
    type.name = name;
    type.index = types.size();
    type.module = this;
    types.push_back(type);
    return &types[types.size() - 1];
}

bool CVMModule::ExistsType(const std::string &name) const {
    for (size_t i = 0; i < types.size(); i++) {
        if (types[i].name == name) return true;
    }
    return false;
}

bool CVMModule::GetType(const std::string &name) const {
    for (size_t i = 0; i < types.size(); i++) {
        if (types[i].name == name) return &types[i];
    }
    return nullptr;
}

CVMDeclare *CVMModule::DeclareVariable(CVMDeclare *base, CVMTypeinfo *type, const std::string &name, const uint8_t as = CVMDeclare::DeclareVar) {
    if (as != CVMDeclare::DeclareProperty) {
        for (size_t i = 0; i < symbols.size(); i++) {
            if (symbols[i].name == name) return &symbols[i];
        }
    }

    CVMDeclare sym;
    sym.base = base;
    sym.type = type;
    sym.name = name;
    sym.as = as;
    sym.index = symbols.size();
    sym.module = this;
    symbols.push_back(sym);
    return &symbols[symbols.size() - 1];
}

bool CVMModule::ExistsVariable(const std::string &name) const {
    for (size_t i = 0; i < symbols.size(); i++) {
        if (symbols[i].name == name) return true;
    }
    return false;
}

CVMDeclare *CVMModule::GetVariable(const std::string &name) const {
    for (size_t i = 0; i < symbols.size(); i++) {
        if (symbols[i].name == name) return &symbols[i];
    }
    return nullptr;
}

CVMModule g_stdmodule;
CVMTypeinfo *undefinedType = nullptr;
CVMValue *undefinedValue = nullptr;
CVMTypeinfo *nullType = nullptr;
CVMValue *nullValue = nullptr;
CVMTypeinfo *voidType = nullptr;
CVMValue *voidValue = nullptr;
CVMTypeinfo *trueType = nullptr;
CVMValue *trueValue = nullptr;
CVMTypeinfo *falseType = nullptr;
CVMValue *falseValue = nullptr;

void VMInit() {
    undefinedType = g_stdmodule.DeclareType("undefined");
    undefinedValue = g_stdmodule.DeclareVariable(nullptr, undefinedType, "undefined", DeclareConst);

    nullType = g_stdmodule.DeclareType("null");
    nullValue = g_stdmodule.DeclareVariable(nullptr, nullType, "null", DeclareConst);

    voidType = g_stdmodule.DeclareType("void");
    voidValue = g_stdmodule.DeclareVariable(nullptr, voidType, "void", DeclareConst);         
}

CVMTypeinfo *CVMTypeinfo::ObjectType()
{
    static CVMTypeinfo *type = nullptr;
    if (!type) {
        CVMTypeinfo newtype;
        newtype.kind = CVMTypeinfo::Kind_Object;
        newtype.flags = 0;

        type = g_stdmodule.DeclareType("object", &newtype);
    }
    return type;
}

CVMTypeinfo *CVMTypeinfo::ArrayType()
{
    static CVMTypeinfo *type = nullptr;
    if (!type) {
        CVMTypeinfo newtype;
        newtype.kind = CVMTypeinfo::Kind_Array;
        newtype.flags = 0;

        type = g_stdmodule.DeclareType("array", &newtype);
    }
    return type;
}

CVMTypeinfo *CVMTypeinfo::FunctionType()
{
    static CVMTypeinfo *type = nullptr;
    if (!type) {
        CVMTypeinfo newtype;
        newtype.kind = CVMTypeinfo::Kind_Callable;
        newtype.flags = CVMTypeinfo::Flag_Function;

        type = g_stdmodule.DeclareType("function", &newtype);
    }
    return type;
}

CVMTypeinfo *CVMTypeinfo::AsyncFunctionType()
{
    static CVMTypeinfo *type = nullptr;
    if (!type) {
        CVMTypeinfo newtype;
        newtype.kind = CVMTypeinfo::Kind_Callable;
        newtype.flags = CVMTypeinfo::Flag_Function | CVMTypeinfo::Flag_Async;

        type = g_stdmodule.DeclareType("asyncfunction", &newtype);
    }
    return type;
}

void Buildin_IsPrototypeOf(CVMState *state) {
    CVMValue *object = state->Pop();
    CVMValue *prototype = state->Pop();
    if (*object->GetPrototype()->type == *prototype->type) {
        state->Push(g_trueValue);
    } else {
        state->Push(g_falseValue);
    }
}

void Buildin_PrototypeConstructor(CVMState *state) {
    static CVMValue *prototype = nullptr;
    if (!prototype) {
        prototype = new CVMValue(g_stdmodule.GetRoot(), CVMTypeinfo::ObjectType(), "object");
        prototype->SetKeyValue("prototype", prototype);
    }
    if (state->Top()) state->Top()->Assign(prototype);
}

CVMTypeinfo *CVMTypeinfo::PrototypeType()
{
    static CVMTypeinfo *type = nullptr;
    if (!type) {
        CVMTypeinfo newtype;
        newtype.kind = CVMTypeinfo::Kind_Callable;
        newtype.flags = CVMTypeinfo::Flag_Function | CVMTypeinfo::Flag_Prototype;
        newtype.address = (int64_t) &PrototypeConstructor;

        type = g_stdmodule.DeclareType("prototype", &newtype);
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

bool CJSGetInstr(CJSState *state, CJSFunction *function, CJSInstr *instr) {
    if (state->pc + 1 <= function->size)  {
        instr->opcode = *(state->code + state->pc); 
        state->pc++;
    } else
        return false;

    if (state->pc + 8 <= function->size)  {
        instr->operand = le64toh(*((uint64_t*)((uint8_t *) state->code + state->pc))); 
        state->pc += 8;
    } else
        return false;

    if (state->pc + 4 <= function->size)  {
        instr->mask = le32toh(*((uint32_t*)((uint8_t *) state->code + state->pc))); 
        state->pc += 4;
    } else 
        return false;

    return true;
}

CJSValue *CJSGet(CJSState *state, int idx) {
    if (idx < 0 || idx >= state->size) return nullptr;
    return &state->stack[idx];
}

CJSValue *CJSSet(CJSState *state, int idx, CJSValue *value) {
    if (idx >= 0 || idx < state->size) {
        memcpy(&state->stack[idx], value, sizeof(CJSValue));
        return &state->stack[idx];
    }
    return nullptr;
}

CJSValue *CJSNewState() {
    CJSState *state = (CJSState *) memalloc(sizeof(CJSState));
    memset(state, 0, sizeof(CJSState));
    state->s = state->g = CJSNewValue(state, "__gs_main__");
    return state;
}

void CJSFreeState(CJSState *state) {
    if (state) {
        CJSFreeValue(state->g);
        free(state);
    }
}

CJSValue *CJSUndefinedValue() {
    static CJSValue *u = nullptr;
    if (!u)
    {
        static CJSValue v;
        memset(&v, 0, sizeof(v));
        v.rc = 0;
        v.type = CJSValue::Undefined;
        u = &v;
    }
    return u;
}

CJSValue *CJSNullValue() {
    static CJSValue *u = nullptr;
    if (!u)
    {
        static CJSValue v;
        memset(&v, 0, sizeof(v));
        v.rc = 0;
        v.type = CJSValue::Null;
        u = &v;
    }
    return u;
}

CJSValue *CJSNewValue(CJSState *state, const char *name, CJSValue *value) {
    CJSValue *s = state->s;
    if (!s) {
        CJSValue *newvalue = (CJSValue *) malloc(sizeof(CJSValue));
        memcpy(newvalue, value, sizeof(CJSValue));
        return newvalue;
    }
    CJSObject *o = (CJSObject *) s->data;
    if (!o) return CJSUndefinedValue();

    for (size_t i = 0; i < o->capacity; i++) {
        if (!stricmp(o->values[i].name, name)) return o->values[i].value;
    }

    if (o->capacity + 1 <= o->size) {
        o->capacity = o->capacity + 1;
        o->values[o->capacity - 1] = CJSRef(state, value);
        return o->values[o->capacity - 1];
    }
    
    size_t cap = o->capacity;
    CJSKeyValue *old = o->values;
    o->size = o->size + 6;
    o->capacity = o->capacity + 1;

    o->values = (CJSValue **) malloc(sizeof(CJSValue **) * o->size);
    memset(o->values, 0, sizeof(CJSValue **) * o->size);

    for (size_t i = 0; i < cap; i++) {
        o->values[i] = old[i];
    }
    o->values[o->capacity - 1] = CJSRef(state, value);
    
    free(old);
    return o->values[o->capacity - 1];
}

void CJSFreeObject(CJSValue *value) {
    if (value && value->type == CJSValue::Object) {
        CJSObject *o = (CJSObject *) value->data;
        if (o) {
            for (size_t i = 0; i < o->size; i++) {
                if (o->values[i].name) free(o->values[i].name);
                CJSFreeValue(o->values[i].value);
            }
            if (o->size) free(o->values);
            free(o);
        }
    }
}

void CJSFreeValue(CJSValue *value) {
    if (value->rc >= 1) {
        if (value->rc == 1) {
            if (value->type == CJSValue::Object) {
                CJSFreeObject((CJSObject *) value->data);
                free(value);
            }
            value->rc = 0;
            free(value);
        } else
            value->rc--;
    }
}

bool CJSExec(CJSState *state, CJSInstr *instr) {
    switch (instr->opcode)
    {
        case CJSInstr::PushTop: CJSPushValue(state, CJSGet(state, instr->operand)); break;
    }
    return false;
}

bool CJSCall(CJSState *state, CJSFunction *function) {
    if (function) {
        CJSSetFunction(state, CJSState::This, function);
        CJSSetFunction(state, CJSState::Super, function->super ? function->super : function);
        CJSSetFunction(state, CJSState::Calle, function);
    }

    CJSValue *calle = CJSGet(state, CJSState::Calle);
    if (!calle)
        return false;
    
    if (!function) {
        if (!calle->data) return false;
        function = (CJSFunction *) calle->data;
    }

    CJSInstr instr;
    while (state->pc < function->size) {
        if (!CJSGetInstr(state, function, &instr))
            break;
        
        if (!CJSExec(state, &instr))
            break;
    }

    return true;
}