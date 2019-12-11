// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <chainparams.h>
#include <galaxycash.h>
#include <galaxyscript.h>
#include <galaxyscript-compiler.h>
#include <hash.h>
#include <memory>
#include <pow.h>
#include <stack>
#include <stdint.h>
#include <uint256.h>
#include <util.h>
#include <fstream>
#include <fs.h>
#include <locale>
#include <codecvt>
#include <string>

#define MAGIC_VALUE (uint32_t)('VAL\0')
#define MAGIC_CONST (uint32_t)('CNST')


struct CSym {
    std::string name;
    CVMTypeinfo type;
    size_t addr;
    std::vector<char> data;
    std::vector<CSym> values;
    std::map<std::string, CSym> variables;


    CSym() {}
    CSym(const CSym &sym) : name(sym.name), type(sym.type), addr(sym.addr), data(sym.data) {
        for (std::vector<CSym>::const_iterator it = sym.values.begin(); it != sym.values.end(); it++) {
            values.push_back(CSym((*it)));
        }
        for (std::map<std::string, CSym>::const_iterator it = std::begin(sym.variables); it != std::end(sym.variables); it++) {
            variables[it->first] = CSym(it->second);
        }
    }
    CSym &operator = (const CSym &sym) {
        name = sym.name;
        type = sym.type;
        addr = sym.addr;
        data = std::vector<char>(sym.data.begin(), sym.data.end());
        values.clear();
        for (std::vector<CSym>::const_iterator it = sym.values.begin(); it != sym.values.end(); it++) {
            values.push_back(CSym((*it)));
        }
        variables.clear();
        for (std::map<std::string, CSym>::const_iterator it = sym.variables.begin(); it != sym.variables.end(); it++) {
            variables[it->first] = CSym(it->second);
        }
        return *this;
    }
    bool operator <= (const CSym &rhs) const { return name <= rhs.name; }
    bool operator < (const CSym &rhs) const { return name < rhs.name; }
};


static CSym module;
static std::vector<char> code;
static std::vector<std::string> modules;
static std::vector<CVMTypeinfo> types;
static std::vector<CVMTypeinfo> variables;
static std::vector<CVMTypeinfo> functions;
static std::vector<CSym> syms;
static std::vector<CSym*> consts;
static std::stack<CSym*> scope;


void CCEmitInt8(int8_t val) {
    code.push_back(val);
}

void CCEmitInt16(int16_t val) {
    int16_t c = htole16(val);
    CCEmitInt8((int8_t) c);
    CCEmitInt8((int8_t) (c >> 8));
}

void CCEmitInt32(int32_t val) {
    int32_t c = htole32(val);
    CCEmitInt16((int16_t) c);
    CCEmitInt16((int16_t) (c >> 16));
}

void CCEmitInt64(int64_t val) {
    int64_t c = htole64(val);
    CCEmitInt32((int32_t) c);
    CCEmitInt32((int32_t) (c >> 32));
}

void CCEmitUInt8(uint8_t val) {
    code.push_back(*(char*)&val);
}

void CCEmitUInt16(uint16_t val) {
    uint16_t c = htole16(val);
    CCEmitUInt8((uint8_t) c);
    CCEmitUInt8((uint8_t) (c >> 8));
}

void CCEmitUInt32(uint32_t val) {
    uint32_t c = htole32(val);
    CCEmitUInt16((uint16_t) c);
    CCEmitUInt16((uint16_t) (c >> 16));
}

void CCEmitAddr(size_t val) {
    uint32_t c = htole32((uint32_t) val);
    CCEmitUInt16((uint16_t) c);
    CCEmitUInt16((uint16_t) (c >> 16));
}

void CCEmitUInt64(uint64_t val) {
    uint64_t c = htole64(val);
    CCEmitUInt32((uint32_t) c);
    CCEmitUInt32((uint32_t) (c >> 32));
}

void CCEmitCompactSize(uint64_t nSize)
{
    if (nSize < 253) {
        CCEmitUInt8(nSize);
    } else if (nSize <= std::numeric_limits<unsigned short>::max()) {
        CCEmitUInt8(253);
        CCEmitUInt16(nSize);
    } else if (nSize <= std::numeric_limits<unsigned int>::max()) {
        CCEmitUInt8(254);
        CCEmitUInt32(nSize);
    } else {
        CCEmitUInt8(255);
        CCEmitUInt64(nSize);
    }
}

void CCEmitBytes(std::vector<char> &bytes) {
    CCEmitCompactSize(bytes.size());
    code.insert(code.end(), bytes.begin(), bytes.end());
}

void CCEmitString(const std::string &val) {
    CCEmitCompactSize(val.size());
    code.insert(code.end(), val.begin(), val.end());
}

void CCEmitType(const CVMTypeinfo &typeinfo) {
    CCEmitUInt8(typeinfo.version);
    CCEmitUInt8((uint8_t)typeinfo.kind);
    CCEmitString(typeinfo.name);
    CCEmitUInt8(typeinfo.bits);
    CCEmitUInt64(typeinfo.flags);
    if (typeinfo.super) {
        CCEmitUInt8(1);
        CCEmitType(*typeinfo.super);
    } else {
        CCEmitUInt8(0);
    }
    if (typeinfo.ctor) {
        CCEmitUInt8(1);
        CCEmitType(*typeinfo.ctor);
    } else {
        CCEmitUInt8(0);
    }
    if (typeinfo.dtor) {
        CCEmitUInt8(1);
        CCEmitType(*typeinfo.dtor);
    } else {
        CCEmitUInt8(0);
    }   
}

void CCEmitSym(CSym* sym) {
    sym->addr = code.size();
    CCEmitAddr(MAGIC_VALUE);
    CCEmitAddr(sym->addr);
    CCEmitString(sym->name);
    CCEmitType(sym->type);
    CCEmitBytes(sym->data);

    CCEmitCompactSize(sym->values.size());
    for (std::vector<CSym>::iterator it = sym->values.begin(); it != sym->values.end(); it++) {
        CCEmitSym(&(*it));
    }

    CCEmitCompactSize(sym->variables.size());
    for (std::map<std::string, CSym>::iterator it = sym->variables.begin(); it != sym->variables.end(); it++) {
        CCEmitSym(&(*it).second);
    }
}

void CCEmitConst(CSym *sym) {
    CCEmitSym(sym);
}

void CCEmitBinaryOp(CVMBinaryOp op, CSym *lvalue, CSym *rvalue) {

}

void CCEmitUnaryOp(CVMUnaryOp op, CSym *sym) {
    
}

size_t CCAsAddr(std::vector<char>::iterator &c) {
    assert(code.size() >= 4);
    return le32toh(((uint32_t) (*c) | (uint32_t) (*(c + 1)) << 8 | (uint32_t)(*(c + 2)) << 16 | (uint32_t)(*(c + 3)) << 24));
}

std::string CCGenName() {
    char name[17];
    memset(name, 0, sizeof(name));
    GetRandBytes((unsigned char *) name, 16);
    if (scope.size() && scope.top()->variables.count(name)) return CCGenName();
    return name;
}

CVMTypeinfo *CCGenType(const std::string &name = std::string()) {
    for (size_t i = 0; i < types.size(); i++) {
        if  (types[i].name == name) 
            return &types[i];
    }

    CVMTypeinfo type;
    type.name = name;
    types.push_back(type);
    return &types[types.size() - 1];
}

CSym *CCGenValue(std::string name = std::string()) {
    if (scope.size()) {
        CSym *root = scope.top();
        CSym sym;
        if (name.empty()) {
            sym.name = CCGenName();
        } else {
            sym.name = name;
        }
        root->variables[sym.name] = sym;
        return &root->variables[sym.name];
    }
    return nullptr;
}

CSym *CCGenRequire(std::string name = std::string(), const std::string &module = std::string()) {
    if (scope.size()) {
        CSym *root = scope.top();
        CSym sym;
        if (name.empty()) {
            sym.name = CCGenName();
        } else {
            sym.name = name;
        }
        sym.type.SetNull();
        sym.type.kind = CVMTypeinfo::Kind_Module;
        sym.type.name = module;
        sym.name = name;

        if (find(modules.begin(), modules.end(), module) == modules.end())
            modules.push_back(module);

        root->variables[sym.name] = sym;
        return &root->variables[sym.name];
    }
    return nullptr;
}

void CCPushScope(CSym *sym) {
    if (sym) {
        scope.push(sym);
    } else {
        scope.push(&module);
    }
}

CSym *CCPopScope() {
    if (scope.size()) {
        CSym *sym = scope.top(); scope.pop();
        return sym;
    }
    return nullptr;
}

CSym *CCTopScope() {
    if (scope.size()) return scope.top();
    return nullptr;
}

static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

bool CCLoadSourceFile(const fs::path &path, std::vector<wchar_t> &buffer) {
    std::string spath = converter.to_bytes(path.c_str());
    std::ifstream file(spath, std::ios_base::in);
    if (!file.is_open())
        return false;

    std::stringstream ss;
    ss << file.rdbuf();
    file.close();

    std::wstring wcode = converter.from_bytes(ss.str().c_str());

    buffer.insert(buffer.end(), wcode.begin(), wcode.end());
    return true;
}

static std::list<fs::path> files;

bool CCCompileBuffer(std::vector<wchar_t> buffer) {
    return false;
}

CVMValue *CCCompileSource(const std::string &code) {
    std::wstring wcode = converter.from_bytes(code.c_str());
    std::vector<wchar_t> buffer(wcode.begin(), wcode.end());
    if (CCCompileBuffer(buffer)) {
        return 0;
    }
    error("%s : Failed to compile source\n", __func__);
    return nullptr;
}

CVMValue *CCCompileFile(const std::string &spath) {
    std::vector<wchar_t> buffer;
    if (!CCLoadSourceFile(fs::path(spath), buffer)) {
        error("%s : Failed load source code from file %s\n", __func__, spath.c_str());
        return nullptr;
    }
    if (CCCompileBuffer(buffer)) {
        return 0;
    }
    error("%s : Failed to compile file %s\n", __func__, spath.c_str());
    return nullptr;
}