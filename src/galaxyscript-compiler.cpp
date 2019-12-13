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


class CTok {
public:
    enum {
        Unknown = 0,
        Number,
        String,
        Keyword,
        Operator,
        Punctuation,
        Word,
        EndOfline
    };

    enum {
        None = 0,
        Litteral = (1 << 0),
        Float = (1 << 1),
        Unary = (1 << 2),
        Binary = (1 << 3),
        Unsigned = (1 << 4),
        Logical = (1 << 5),
        Hex = (1 << 6),
        Big = (1 << 7),
        Negative = (1 << 8),
        Assignment = (1 << 9)
    };

    CTok *      prv;
    uint8_t     type;
    uint32_t    flags;
    std::string value;
    std::string file;
    size_t      pos, line;

    CTok() : prv(nullptr), type(Unknown), flags(None), pos(0), line(0) {}
    CTok(const CTok &tok) : prv(nullptr), type(tok.type), flags(tok.flags), pos(tok.pos), line(tok.line) {
        if (tok.prv)
            this->prv = new CTok(*tok.prv);
    }
    ~CTok() {
        if (prv) delete prv;
    }

    CTok &operator = (const CTok &tok) {
        if (prv) delete prv; prv = nullptr;
        if (tok.prv) prv = new CTok(*tok.prv);
        type = tok.type;
        flags = tok.flags;
        value = tok.value;
        file = tok.file;
        pos = tok.pos;
        line = tok.line;
        return *this;
    }

    bool IsNull() const {
        return type == Unknown && flags == None;
    }

    void SetNull() {
        if (prv) delete prv;
        prv = nullptr;
        type = Unknown;
        flags = None;
        value.clear();
        file.clear();
        pos = 0;
        line = 0;
    }

    bool operator < (const CTok &tok) const { return (type < tok.type) || (value < tok.value); }
    bool operator == (const CTok &tok) const { return (type == tok.type) && (value == tok.value); }
};


static std::string g_keywords[] = {
    "var",
    "let",
    "const",
    "new",
    "delete",
    "void",
    "null",
    "true",
    "false",
    "undefined",
    "typeof",
    "instanceof",
    "in",
    "number",
    "string",
    "function",
    "array",
    "object",
    "if",
    "else",
    "for",
    "while",
    "do",
    "break",
    "continue",
    "return",
    "async",
    "await",
    "with",
    "switch",
    "case",
    "default",
    "this",
    "super",
    "try",
    "throw",
    "catch",
    "finally",
    "debugger",
    "class",
    "enum",
    "extends",
    "implements",
    "interface",
    "package",
    "private",
    "protected",
    "static",
    "import",
    "export",
    "yield"
};

static const uint32_t g_keywords_flags[] = {
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0
};

static const std::string g_operators[] = {
    ">>>=", "<<=", ">>=", ">>>", "!==", "===", "!=", "%=", "&&",
    "&=", "*=", "*", "++", "+=", "--", "-=", "<<", "<=", "==", ">=", ">>",
    "^=", "|=", "||", "!", "%", "&", "+", "-", "=",
    ">", "<", "^", "|", "~"
};

static const uint32_t g_operators_flags[] = {
    CTok::Unary, CTok::Unary, CTok::Unary, CTok::Binary, CTok::Binary|CTok::Logical, CTok::Binary|CTok::Logical, CTok::Binary|CTok::Logical, CTok::Unary, CTok::Binary|CTok::Logical,
    CTok::Unary, CTok::Unary, CTok::Binary, CTok::Unary, CTok::Unary,  CTok::Unary, CTok::Unary, CTok::Binary, CTok::Binary|CTok::Logical, CTok::Binary|CTok::Logical, CTok::Binary|CTok::Logical, CTok::Binary,
    CTok::Unary, CTok::Unary, CTok::Binary|CTok::Logical, CTok::Unary, CTok::Binary, CTok::Binary, CTok::Binary, CTok::Binary, CTok::Unary,
    CTok::Binary|CTok::Logical, CTok::Binary|CTok::Logical, CTok::Binary, CTok::Binary, CTok::Binary
};


static const std::string g_punctuations[] = {
    ".", ",", ";", ":", "[", "]", "{", "}"
};

static const uint32_t g_punctuations_flags[] = {
    0, 0, 0, 0, 0, 0, 0, 0
};


class CLexer {
public:
    CTok        cur;
    std::string buffer, file;
    size_t      pos, line;
    std::vector <std::string> errs;

    CLexer();
    CLexer(const std::string &file, const std::string &code);
    virtual ~CLexer();

    bool    Eof() const;

    char    PrevChar();
    char    LastChar();
    char    NextChar();
    char    Advance();
    void    SkipWhitespace();

    bool    ReadToken(CTok &tok);
    bool    ReadString(std::string &str);
    bool    ReadNumber(std::string &str, uint32_t &flags);
    void    UnreadToken(const CTok &tok);
    bool    MatchToken(const std::string &val);
    bool    MatchType(const uint8_t type);
    bool    CheckToken(const std::string &val);
};

CLexer::CLexer() : pos(0), line(0) {
}
CLexer::CLexer(const std::string &file, const std::string &code) : buffer(code), file(file), pos(0), line(0)
{}
CLexer::~CLexer()
{}

bool CLexer::Eof() const {
    if (buffer.empty()) return true;
    return pos >= buffer.length();
}

char CLexer::LastChar() {
    if (pos < buffer.length()) {
        return *(buffer.c_str() + pos);
    }
    return 0;
}

char CLexer::PrevChar() {
    if (buffer.length() > 0 && pos >= 0) {
        if (pos == 0) return *buffer.c_str();
        return *(buffer.c_str() + (pos - 1));
    }
    return 0;
}

char CLexer::NextChar() {
    if (!Eof()) {
        if (buffer.length() <= (pos + 1)) return 0;
        return *(buffer.c_str() + (pos + 1));
    }
    return 0;
}

char CLexer::Advance() {
    if (!Eof()) {
        pos++;
        if (!Eof())
            return *(buffer.c_str() + pos);
        return 0;
    }
    return 0;
}

bool IsWhitespace(int c) {
    if (c == ' ') return true;
    else if (c == '\t') return true;
    return false;
}

bool IsEndline(int c) {
    if (c == '\n') return true;
    else if (c == '\r') return true;
    return false;
}

void CLexer::SkipWhitespace() {

    while (!Eof() && IsWhitespace(LastChar())) {
        pos++;
    }
    cur.pos = pos;

    if (LastChar() == '/' && NextChar() == '/') {
        pos += 2;

        while (LastChar() != '\n' && LastChar() != '\r' && !Eof()) pos++;

        cur.pos = pos;
    }

    if (LastChar() == '/' && NextChar() == '*') {
        pos += 2;

        while (LastChar() != '*' && NextChar() != '/' && !Eof()) pos++;

        cur.pos = pos;
    }
}

bool CLexer::ReadString(std::string &tok) {
    if (Eof()) return false;
    const char *p = buffer.c_str() + pos;
    if (*p == '"') {
        tok.clear();
        p++; pos++;
        while (!Eof() && *p != '"') {
            tok += *p; p++; pos++;
        }
        if (!Eof() && *p == '"') {
            pos++;
        }
        return !tok.empty();
    }
    if (*p == '\'') {
        tok.clear();
        p++; pos++;
        while (!Eof() && *p != '\'') {
            tok += *p; p++; pos++;
        }
        if (!Eof() && *p == '\'') {
            pos++;
        }
        return !tok.empty();
    }
    return false;
}

bool CLexer::ReadNumber(std::string &tok, uint32_t &flags) {
    if (Eof()) return false;
    const char *p = buffer.c_str() + pos;
    if (isdigit(*p)) {
        tok.clear();
        tok += *p;
        p++; pos++;
        while (!Eof() && (isdigit(*p) || *p == '.')) {
            if (*p == '.')
                flags |= CTok::Float;
            tok += *p; p++; pos++;
        }
        flags |= CTok::Litteral;
        return !tok.empty();
    }
    if (*p == '.') {
        p++; pos++;
        if (p && isdigit(*p)) {
            tok = '.';
            flags |= CTok::Float;

            while (!Eof() && isdigit(*p)) {
                tok += *p; p++; pos++;
            }

            if (!tok.empty()) {
                return true;
            } else {
                pos--;
                return false;
            }
        } else {
            pos--;
        }
    }

    if (*p == '-') {
        p++; pos++;
        if (p && isdigit(*p)) {
            tok = '-';
            flags |= CTok::Negative;

            while (!Eof() && (isdigit(*p) || *p == '.')) {
                if (*p == '.')
                    flags |= CTok::Float;
                tok += *p; p++; pos++;
            }
            
            if (!tok.empty()) {
                return true;
            } else {
                pos--;
                return false;
            }
        } else {
            pos--;
        }
    }
     
    if (*p == 'u') {
        p++; pos++;
        if (p && isdigit(*p)) {
            tok.clear();
            flags |= CTok::Unsigned;

            while (!Eof() && isdigit(*p)) {
                tok += *p; p++; pos++;
            }
            
            if (!tok.empty()) {
                return true;
            } else {
                pos--;
                return false;
            }
        } else {
            pos--;
        }
    }

    return false;
}

bool MatchStr(const char *source, const char *value, int len) {
    if (!source || *source == '\0' || !value || *value == '\0') return false;
    return strncmp(source, value, len) == 0;
}

bool CLexer::ReadToken(CTok &tok) {
    CTok lst(cur);

    cur = CTok();
    cur.prv = new CTok(lst);
    cur.pos = pos;
    cur.file = file;
    cur.line = line;

    SkipWhitespace();

    if (Eof()) return false;

    if (LastChar() == '\n' || LastChar() == '\r') {
        cur.file = file;
        cur.line = line; line++;
        cur.pos = pos; pos++;
        cur.type = CTok::EndOfline;
        cur.flags = CTok::None;
        cur.value = "\n";
        tok = cur;
        return true;
    }


    if (ReadString(cur.value)) {
        cur.type = CTok::String;
        cur.flags = CTok::Litteral;
        tok = cur;
        return true;
    }

    if (ReadNumber(cur.value, cur.flags)) {
        cur.type = CTok::Number;
        tok = cur;
        return true;
    }

    const char *p = buffer.c_str() + pos;

    for (size_t i = 0; i < sizeof(g_operators) / sizeof(g_operators[0]); i++) {
        if (MatchStr(p, g_operators[i].c_str(), g_operators[i].length())) {
            cur.file = file;
            cur.line = line;
            cur.pos = pos;
            cur.type = CTok::Operator;
            cur.flags = g_operators_flags[i];
            cur.value = g_operators[i];
            pos += g_operators[i].length();
            tok = cur;
            return true;
        }
    }

    for (size_t i = 0; i < sizeof(g_punctuations) / sizeof(g_punctuations[0]); i++) {
        if (MatchStr(p, g_punctuations[i].c_str(), g_punctuations[i].length())) {
            cur.file = file;
            cur.line = line;
            cur.pos = pos;
            cur.type = CTok::Punctuation;
            cur.flags = g_punctuations_flags[i];
            cur.value = g_punctuations[i];
            pos += g_punctuations[i].length();
            tok = cur;
            return true;
        }
    }

    for (size_t i = 0; i < sizeof(g_keywords) / sizeof(g_keywords[0]); i++) {
        if (MatchStr(p, g_keywords[i].c_str(), g_keywords[i].length())) {
            cur.file = file;
            cur.line = line;
            cur.pos = pos;
            cur.type = CTok::Keyword;
            cur.flags = g_keywords_flags[i];
            cur.value = g_keywords[i];
            pos += g_keywords[i].length();
            tok = cur;
            return true;
        }
    }

    cur.type = CTok::Word;
    cur.flags = 0;
    while (!Eof() && *p && !IsWhitespace(*p) && !IsEndline(*p)) {
        cur.value += *p; p++;
        pos++;
    }
    if (cur.value.empty()) {
        cur.SetNull();
        return false;
    }


    return true;
}

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

bool CCLoadSourceFile(const fs::path &path, std::string &buffer) {
    FILE *file = fsbridge::fopen(path, "rb");
    if (!file) return false;
    while (feof(file) != EOF) {
        buffer += (char) fgetc(file);
    }
    fclose(file);
    return true;
}

static std::list<fs::path> files;

bool CCCompileBuffer(const std::string &file, const std::string &code) {
    CLexer lex(file, code);
    if (lex.Eof()) return false;

    CTok tok;
    while (lex.ReadToken(tok)) {
        if (tok.type == CTok::EndOfline) LogPrintf("Tok End of line\n");
        else LogPrintf("Tok %s\n", tok.value.c_str());
    }
    LogPrintf("Finished\n");

    return lex.Eof();
}

CVMValue *CCCompileSource(const std::string &code) {
    if (CCCompileBuffer("none", code)) {
        return 0;
    }
    error("%s : Failed to compile source\n", __func__);
    return nullptr;
}

CVMValue *CCCompileFile(const std::string &spath) {
    std::string buffer;
    if (!CCLoadSourceFile(fs::path(spath), buffer)) {
        error("%s : Failed load source code from file %s\n", __func__, spath.c_str());
        return nullptr;
    }
    if (CCCompileBuffer(spath, buffer)) {
        return 0;
    }
    error("%s : Failed to compile file %s\n", __func__, spath.c_str());
    return nullptr;
}