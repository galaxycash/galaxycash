// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <chainparams.h>
#include <codecvt>
#include <fs.h>
#include <fstream>
#include <galaxycash.h>
#include <galaxyscript-compiler.h>
#include <galaxyscript.h>
#include <hash.h>
#include <locale>
#include <memory>
#include <pow.h>
#include <stack>
#include <stdint.h>
#include <string>
#include <tinyformat.h>
#include <uint256.h>
#include <util.h>
#include <utilmoneystr.h>
#include <utilstrencodings.h>

class CTok
{
public:
    enum {
        Unknown = 0,
        Number,
        String,
        Keyword,
        Operator,
        Punctuation,
        Identifier,
        EndOfline
    };

    enum {
        None = 0,
        Literal = (1 << 0),
        Float = (1 << 1),
        Unary = (1 << 2),
        Binary = (1 << 3),
        Unsigned = (1 << 4),
        Logical = (1 << 5),
        Hex = (1 << 6),
        Big = (1 << 7),
        Negative = (1 << 8),
        Assignment = (1 << 9),
        Native = (1 << 10),
        Buildin = (1 << 11)
    };

    CTok* prv;
    uint8_t type;
    uint32_t flags;
    std::string value;
    std::string file;
    size_t pos, line;

    CTok() : prv(nullptr), type(Unknown), flags(None), pos(0), line(0) {}
    CTok(const CTok& tok) : prv(nullptr), type(tok.type), flags(tok.flags), pos(tok.pos), line(tok.line)
    {
        if (tok.prv)
            this->prv = new CTok(*tok.prv);
    }
    ~CTok()
    {
        if (prv) delete prv;
    }

    CTok& operator=(const CTok& tok)
    {
        if (prv) delete prv;
        prv = nullptr;
        if (tok.prv) prv = new CTok(*tok.prv);
        type = tok.type;
        flags = tok.flags;
        value = tok.value;
        file = tok.file;
        pos = tok.pos;
        line = tok.line;
        return *this;
    }

    bool IsNull() const
    {
        return type == Unknown && flags == None;
    }

    void SetNull()
    {
        if (prv) delete prv;
        prv = nullptr;
        type = Unknown;
        flags = None;
        value.clear();
        file.clear();
        pos = 0;
        line = 0;
    }

    bool IsNumber() const
    {
        return (type == Number);
    }
    bool IsFloat() const
    {
        return (flags & Float);
    }
    bool IsUnsigned() const
    {
        return (flags & Unsigned);
    }
    bool IsHexNum() const
    {
        return (flags & Hex);
    }
    bool IsBigNum() const
    {
        return (flags & Big);
    }
    bool IsNegative() const
    {
        return (flags & Negative);
    }

    bool IsOperator() const
    {
        return (type == Operator);
    }
    bool IsUnary() const
    {
        return (flags & Unary);
    }
    bool IsBinary() const
    {
        return (flags & Binary);
    }
    bool IsLogical() const
    {
        return (flags & Logical);
    }

    bool IsNative() const
    {
        return (flags & Native);
    }
    bool IsBuildin() const
    {
        return (flags & Buildin);
    }
    bool IsLiteral() const
    {
        return (flags & Literal);
    }
    bool IsVariableDeclare() const
    {
        return (type == Keyword) && (value == "var" || value == "let" || value == "const" || value == "static");
    }
    bool IsFunctionDeclare() const
    {
        return (type == Keyword) && (value == "function");
    }
    bool IsClassDeclare() const
    {
        return (type == Keyword) && (value == "class");
    }
    bool IsConstructorDeclare() const
    {
        return (type == Keyword) && (value == "constructor");
    }
    bool IsDestructorDeclare() const
    {
        return (type == Keyword) && (value == "destructor");
    }

    bool operator<(const CTok& tok) const { return (type < tok.type) || (value < tok.value); }
    bool operator==(const CTok& tok) const { return (type == tok.type) && (value == tok.value); }
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
    "yield",
    "native",
    "buildin",
    "constructor",
    "destructor"};

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
    0,
    CTok::Native,
    CTok::Buildin,
    0,
    0};

static const std::string g_operators[] = {
    ">>>=", "<<=", ">>=", ">>>", "!==", "===", "!=", "%=", "&&",
    "&=", "*=", "*", "++", "+=", "--", "-=", "<<", "<=", "==", ">=", ">>",
    "^=", "|=", "||", "!", "%", "&", "+", "-", "=",
    ">", "<", "^", "|", "~"};

static const uint32_t g_operators_flags[] = {
    CTok::Unary, CTok::Unary, CTok::Unary, CTok::Binary, CTok::Binary | CTok::Logical, CTok::Binary | CTok::Logical, CTok::Binary | CTok::Logical, CTok::Unary, CTok::Binary | CTok::Logical,
    CTok::Unary, CTok::Unary, CTok::Binary, CTok::Unary, CTok::Unary, CTok::Unary, CTok::Unary, CTok::Binary, CTok::Binary | CTok::Logical, CTok::Binary | CTok::Logical, CTok::Binary | CTok::Logical, CTok::Binary,
    CTok::Unary, CTok::Unary, CTok::Binary | CTok::Logical, CTok::Unary, CTok::Binary, CTok::Binary, CTok::Binary, CTok::Binary, CTok::Unary,
    CTok::Binary | CTok::Logical, CTok::Binary | CTok::Logical, CTok::Binary, CTok::Binary, CTok::Binary};


static const std::string g_punctuations[] = {
    ".", ",", ";", ":", "[", "]", "{", "}", "(", ")"};

static const uint32_t g_punctuations_flags[] = {
    0, 0, 0, 0, 0, 0, 0, 0};


class CLexer
{
public:
    CTok cur;
    std::string buffer, file;
    size_t pos, line;
    std::vector<std::string> errs;

    CLexer();
    CLexer(const std::string& file, const std::string& code);
    virtual ~CLexer();

    bool Eof() const;

    char PrevChar();
    char LastChar();
    char NextChar();
    char Advance();
    void SkipWhitespace();


    bool ReadToken(CTok& tok);
    bool ReadString(std::string& str);
    bool ReadNumber(std::string& str, uint32_t& flags);
    void UnreadToken(const CTok& tok);
    bool MatchToken(const std::string& val);
    bool MatchType(const uint8_t type);
    bool CheckToken(const std::string& val);
    bool CheckType(const uint8_t type);
    void SkipToken();
};

CLexer::CLexer() : pos(0), line(0)
{
}
CLexer::CLexer(const std::string& file, const std::string& code) : buffer(code), file(file), pos(0), line(0)
{
}
CLexer::~CLexer()
{
}

bool CLexer::Eof() const
{
    if (buffer.empty()) return true;
    return pos >= buffer.length();
}

char CLexer::LastChar()
{
    if (pos < buffer.length()) {
        return *(buffer.c_str() + pos);
    }
    return 0;
}

char CLexer::PrevChar()
{
    if (buffer.length() > 0 && pos >= 0) {
        if (pos == 0) return *buffer.c_str();
        return *(buffer.c_str() + (pos - 1));
    }
    return 0;
}

char CLexer::NextChar()
{
    if (!Eof()) {
        if (buffer.length() <= (pos + 1)) return 0;
        return *(buffer.c_str() + (pos + 1));
    }
    return 0;
}

char CLexer::Advance()
{
    if (!Eof()) {
        pos++;
        if (!Eof())
            return *(buffer.c_str() + pos);
        return 0;
    }
    return 0;
}

bool IsWhitespace(int c)
{
    if (c == ' ')
        return true;
    else if (c == '\t')
        return true;
    return false;
}

bool IsEndline(int c)
{
    if (c == '\n')
        return true;
    else if (c == '\r')
        return true;
    else if (c == ';')
        return true;
    return false;
}

void CLexer::SkipWhitespace()
{
    while (!Eof() && IsWhitespace(LastChar())) {
        pos++;
    }
    cur.pos = pos;

    if (LastChar() == '/' && NextChar() == '/') {
        pos += 2;

        while (LastChar() != '\n' && LastChar() != '\r' && !Eof())
            pos++;

        cur.pos = pos;
    }

    if (LastChar() == '/' && NextChar() == '*') {
        pos += 2;

        while (LastChar() != '*' && NextChar() != '/' && !Eof())
            pos++;

        cur.pos = pos;
    }
}

bool CLexer::ReadString(std::string& tok)
{
    if (Eof()) return false;
    const char* p = buffer.c_str() + pos;
    if (*p == '"') {
        tok.clear();
        p++;
        pos++;
        while (!Eof() && *p != '"') {
            tok += *p;
            p++;
            pos++;
        }
        if (!Eof() && *p == '"') {
            pos++;
        }
        return !tok.empty();
    }
    if (*p == '\'') {
        tok.clear();
        p++;
        pos++;
        while (!Eof() && *p != '\'') {
            tok += *p;
            p++;
            pos++;
        }
        if (!Eof() && *p == '\'') {
            pos++;
        }
        return !tok.empty();
    }
    return false;
}

bool CLexer::ReadNumber(std::string& tok, uint32_t& flags)
{
    if (Eof()) return false;
    const char* p = buffer.c_str() + pos;
    if (isdigit(*p)) {
        tok.clear();
        tok += *p;
        p++;
        pos++;
        while (!Eof() && (isdigit(*p) || *p == '.')) {
            if (*p == '.')
                flags |= CTok::Float;
            tok += *p;
            p++;
            pos++;
        }
        flags |= CTok::Literal;
        return !tok.empty();
    }
    if (*p == '.') {
        p++;
        pos++;
        if (p && isdigit(*p)) {
            tok = '.';
            flags |= CTok::Float;

            while (!Eof() && isdigit(*p)) {
                tok += *p;
                p++;
                pos++;
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
        p++;
        pos++;
        if (p && isdigit(*p)) {
            tok = '-';
            flags |= CTok::Negative;

            while (!Eof() && (isdigit(*p) || *p == '.')) {
                if (*p == '.')
                    flags |= CTok::Float;
                tok += *p;
                p++;
                pos++;
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
        p++;
        pos++;
        if (p && isdigit(*p)) {
            tok.clear();
            flags |= CTok::Unsigned;

            while (!Eof() && isdigit(*p)) {
                tok += *p;
                p++;
                pos++;
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

bool MatchStr(const char* source, const char* value)
{
    if (!source || *source == '\0' || !value || *value == '\0') return false;
    while (source && value) {
        if (*value == '\0') return true;
        if (*source == '\0') return false;
        if (*source != *value)
            return false;
        else {
            source++;
            value++;
        }
    }
    return false;
}

bool IsKeyword(const char* source, std::string* value = 0, uint32_t* flags = 0)
{
    for (size_t i = 0; i < sizeof(g_keywords) / sizeof(g_keywords[0]); i++) {
        if (MatchStr(source, g_keywords[i].c_str())) {
            if (value) *value = g_keywords[i];
            if (flags) *flags |= g_keywords_flags[i];
            return true;
        }
    }
    return false;
}

bool IsOperator(const char* source, std::string* value = 0, uint32_t* flags = 0)
{
    for (size_t i = 0; i < sizeof(g_operators) / sizeof(g_operators[0]); i++) {
        if (MatchStr(source, g_operators[i].c_str())) {
            if (value) *value = g_operators[i];
            if (flags) *flags |= g_operators_flags[i];
            return true;
        }
    }
    return false;
}

bool IsPunctuation(const char* source, std::string* value = 0, uint32_t* flags = 0)
{
    for (size_t i = 0; i < sizeof(g_punctuations) / sizeof(g_punctuations[0]); i++) {
        if (MatchStr(source, g_punctuations[i].c_str())) {
            if (value) *value = g_punctuations[i];
            if (flags) *flags |= g_punctuations_flags[i];
            return true;
        }
    }
    return false;
}

bool CLexer::ReadToken(CTok& tok)
{
    CTok lst(cur);

    cur = CTok();
    cur.prv = new CTok(lst);
    cur.pos = pos;
    cur.file = file;
    cur.line = line;

    SkipWhitespace();

    if (Eof()) return false;

    if (IsEndline(LastChar())) {
        cur.file = file;
        cur.line = line;
        line++;
        cur.pos = pos;
        pos++;
        cur.type = CTok::EndOfline;
        cur.flags = CTok::None;
        cur.value = "\n";
        tok = cur;
        return true;
    }


    if (ReadString(cur.value)) {
        cur.type = CTok::String;
        cur.flags = CTok::Literal;
        tok = cur;
        return true;
    }

    if (ReadNumber(cur.value, cur.flags)) {
        cur.type = CTok::Number;
        tok = cur;
        return true;
    }

    const char* p = buffer.c_str() + pos;

    for (size_t i = 0; i < sizeof(g_operators) / sizeof(g_operators[0]); i++) {
        if (MatchStr(p, g_operators[i].c_str())) {
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
        if (MatchStr(p, g_punctuations[i].c_str())) {
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
        if (MatchStr(p, g_keywords[i].c_str())) {
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

    cur.type = CTok::Identifier;
    cur.flags = 0;

    while (!Eof() && p && !IsWhitespace(*p) && !IsEndline(*p) && *p != '\'' && *p != '"') {
        if (IsPunctuation(p)) break;
        if (IsOperator(p)) break;
        cur.value += *p;
        p++;
        pos++;
    }

    if (!cur.value.empty()) {
        tok = cur;
        return true;
    }


    UnreadToken(lst);
    return false;
}

void CLexer::SkipToken()
{
    CTok tok;
    ReadToken(tok);
}

void CLexer::UnreadToken(const CTok& tok)
{
    pos = tok.prv ? tok.prv->pos : tok.pos;
    line = tok.prv ? tok.prv->line : tok.line;
    file = tok.prv ? tok.prv->file : tok.file;
    cur = tok.prv ? *tok.prv : tok;
}

bool CLexer::CheckToken(const std::string& val)
{
    CTok tok;
    if (!ReadToken(tok)) return false;
    if (val != tok.value) {
        UnreadToken(tok);
        return false;
    }

    UnreadToken(tok);
    return true;
}

bool CLexer::CheckType(const uint8_t type)
{
    CTok tok;
    if (!ReadToken(tok)) return false;
    if (type != tok.type) {
        UnreadToken(tok);
        return false;
    }

    UnreadToken(tok);
    return true;
}

bool CLexer::MatchToken(const std::string& val)
{
    CTok tok;
    if (!ReadToken(tok)) return false;
    if (val != tok.value) {
        UnreadToken(tok);
        return false;
    }

    return true;
}

bool CLexer::MatchType(const uint8_t type)
{
    CTok tok;
    if (!ReadToken(tok)) return false;
    if (type != tok.type) {
        UnreadToken(tok);
        return false;
    }

    return true;
}

struct CError {
    enum {
        Unknown = 0,
        NotFound,
        BadSymbol,
        BadName,
        BadLiteral,
        BadString,
        BadComment,
        Missings
    };
    int code;
    std::string file;
    size_t line;
    std::string text;

    CError() : code(-1), line(0) {}
    CError(const CError& err) : code(err.code), file(err.file), line(err.line), text(err.text) {}
    CError(const int code, const std::string& file, const size_t line, const std::string& text) : code(code), file(file), line(line), text(text) {}

    bool IsError() const { return code > 0; }
};

struct CLabel {
    std::string name;
    size_t offset;
};

static std::vector<char> code;
static std::vector<CLabel> labels;

std::string CCGenName()
{
    char name[17];
    GetRandBytes((unsigned char*)name, sizeof(name) - 1);
    name[sizeof(name) - 1] = '\0';
    return name;
}

void CCGenLabel(const std::string& name, CLabel& label)
{
    label.name = name.empty() ? CCGenName() : name;
    label.offset = code.size();
    labels.push_back(label);
}


static uint8_t modType = CJSModule::ModuleDefault;
static uint32_t modMajorVersion = 1;
static uint32_t modMinorVersion = 0;
static uint32_t modBuildVersion = 0;
static std::string modName;
static CLabel modFunctions, modVariables;

void CCInitModule()
{
    code.clear();
    CCEmitString(modName);
    CCEmitUInt8(modType);
    CCEmitUInt32(modMajorVersion);
    CCEmitUInt32(modMinorVersion);
    CCEmitUInt32(modBuildVersion);
    CCGenLabel("__functions__", modFunctions);
    CCEmitUInt32(0);
    CCGenLabel("__variables__", modVariables);
    CCEmitUInt32(0);
}

void CCEmitInt8(int8_t val)
{
    code.push_back(val);
}

void CCEmitInt16(int16_t val)
{
    int16_t c = htole16(val);
    CCEmitInt8((int8_t)c);
    CCEmitInt8((int8_t)(c >> 8));
}

void CCEmitInt32(int32_t val)
{
    int32_t c = htole32(val);
    CCEmitInt16((int16_t)c);
    CCEmitInt16((int16_t)(c >> 16));
}

void CCEmitInt64(int64_t val)
{
    int64_t c = htole64(val);
    CCEmitInt32((int32_t)c);
    CCEmitInt32((int32_t)(c >> 32));
}

void CCEmitUInt8(uint8_t val)
{
    code.push_back(*(char*)&val);
}

void CCEmitUInt16(uint16_t val)
{
    uint16_t c = htole16(val);
    CCEmitUInt8((uint8_t)c);
    CCEmitUInt8((uint8_t)(c >> 8));
}

void CCEmitUInt32(uint32_t val)
{
    uint32_t c = htole32(val);
    CCEmitUInt16((uint16_t)c);
    CCEmitUInt16((uint16_t)(c >> 16));
}

void CCEmitAddr(size_t val)
{
    uint32_t c = htole32((uint32_t)val);
    CCEmitUInt16((uint16_t)c);
    CCEmitUInt16((uint16_t)(c >> 16));
}

void CCEmitUInt64(uint64_t val)
{
    uint64_t c = htole64(val);
    CCEmitUInt32((uint32_t)c);
    CCEmitUInt32((uint32_t)(c >> 32));
}

void CCEmitFloat(float val)
{
    CCEmitUInt32(*(uint32_t*)&val);
}

void CCEmitDouble(double val)
{
    CCEmitUInt64(*(uint64_t*)&val);
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

void CCEmitBytes(std::vector<char>& bytes)
{
    CCEmitCompactSize(bytes.size());
    code.insert(code.end(), bytes.begin(), bytes.end());
}

void CCEmitString(const std::string& val)
{
    CCEmitCompactSize(val.size());
    code.insert(code.end(), val.begin(), val.end());
}


struct CJSVar {
    mutable std::string name;
    mutable CJSObject* scope;
    mutable CJSValue* variable;
    mutable size_t size;

    inline CJSVar() : scope(nullptr), variable(nullptr), size(0)
    {
    }
    inline CJSVar(const CJSVar& var) : name(var.name), scope(var.scope), variable(var.variable), size(var.size)
    {
        if (scope) scope->Grab();
        if (variable) variable->Grab();
    }
    inline CJSVar(const std::string& name, CJSValue* scopeIn = nullptr, CJSValue* variableIn = nullptr, const size_t size = 0) : name(name), scope(scopeIn), variable(variableIn), size(size)
    {
        if (variable) variable->Grab();
        if (scope) scope->Grab();
    }
    inline ~CJSVar()
    {
        Free();
    }

    inline CJSVar& operator=(const CJSVar& var)
    {
        name = var.name;
        if (scope) scope->Drop();
        scope = var.scope;
        if (scope) scope->Grab();
        if (variable) variable->Drop();
        variable = var.variable;
        if (variable) variable->Grab();
        size = var.size;
        return *this;
    }

    inline void Free()
    {
        if (variable) {
            variable->Drop();
            variable = nullptr;
        }
        if (scope) {
            scope->Drop();
            scope = nullptr;
        }
    }
};

struct CJSBlock {
    enum {
        None = 0,
        Scope,
        If,
        Else,
        For,
        While,
        Simple
    };
    mutable uint8_t type;
    mutable size_t offset;
    mutable std::string file;
    mutable size_t line;
    mutable CJSValue* value;
    mutable CJSBlock* root;
    mutable std::vector<CJSVar> declares;

    inline CJSBlock() : type(None), offset(0), line(0), value(nullptr), root(nullptr) {}
    inline CJSBlock(const uint8_t type, const size_t offset, const std::string& file, const size_t line, CJSValue* valueIn = nullptr, CJSBlock* rootIn = nullptr, const std::vector<CJSVar>& declares = std::vector<CJSVar>()) : type(type), offset(offset), value(valueIn), root(rootIn), declares(declares)
    {
        if (value) value->Grab();
    }
    inline CJSBlock(const CJSBlock& block) : type(block.type), offset(block.offset), file(block.file), line(block.line), value(block.value), root(block.root), declares(block.declares)
    {
        if (value) value->Grab();
    }
    inline ~CJSBlock()
    {
        if (value) value->Drop();
    }

    inline CJSBlock& operator=(const CJSBlock& block)
    {
        type = block.type;
        offset = block.offset;
        file = block.file;
        line = block.line;
        if (value) value->Drop();
        value = block.value;
        if (value) value->Grab();
        root = block.root;
        declares = block.declares;
        return *this;
    }

    inline void Free()
    {
        for (size_t i = 0; i < declares.size(); i++) {
            declares[i].Free();
        }
        declares.clear();
        if (value) {
            value->Drop();
            value = nullptr;
        }
    }

    inline CJSObject* GetScope()
    {
        if (value) {
            if (value->IsObject()) return (CJSObject*)value;
        } else if (root) {
            return root->GetScope();
        }
        return nullptr;
    }
};


struct CJSCState {
    mutable CJSCState* previous;

    mutable CJSModule* module;
    mutable CJSBlock* scope;

    mutable CJSFunction *executable, *onload;
    mutable std::stack<CJSCallable*> callable;
    mutable std::stack<CJSBlock> block;

    mutable CLexer lexer;
    mutable std::string name;
    mutable std::string file;
    mutable std::string source;
    mutable size_t line;
    mutable CTok tok;

    inline CJSCState() : previous(nullptr), module(nullptr), executable(nullptr), onload(nullptr), line(0) {}
    inline CJSCState(const CJSCState& o) : previous(o.previous), name(o.name), file(o.file), source(o.source), line(o.line), module(o.module), executable(o.executable), onload(o.onload), callable(o.callable), block(o.block), lexer(o.lexer), tok(o.tok)
    {
        if (module) module->Grab();
        if (executable) executable->Grab();
        if (onload) onload->Grab();
    }
    inline CJSCState(CJSCState* o) : previous(o->previous), name(o->name), file(o->file), source(o->source), line(o->line), module(o->module), executable(o->executable), onload(o->onload), callable(o->callable), block(o->block), lexer(o->lexer), tok(o->tok)
    {
        if (module) module->Grab();
        if (executable) executable->Grab();
        if (onload) onload->Grab();
    }
    inline ~CJSCState()
    {
        Free();
    }

    inline void Free()
    {
        if (executable) executable->Drop();
        if (onload) onload->Drop();
        while (!block.empty()) {
            block.top().Free();
            block.pop();
        }
        while (!callable.empty()) {
            CJSCallable* c = callable.top();
            if (c) c->Drop();
            callable.pop();
        }
        if (module) module->Drop();
    }
};

class CJSCImpl : public CJSCompiler
{
public:
    std::stack<CJSCState> state;
    std::vector<CError> errs;

    CJSCImpl();
    virtual ~CJSCImpl();

    CJSCState* NewState(const std::string& name, const std::string& filename, const std::string& source);
    CJSCState* CurrentState();

    CJSCallable* CurrentCallable();
    CJSFunction* Executable();
    CJSFunction* OnLoad();

    CJSObject* GetScope(CJSValue* value);

    bool Error(const int code, const std::string& file, const size_t line, const std::string& text);

    CJSBlock* CurrentBlock();
    CJSBlock* NewFunction(const std::string& name, const size_t argc);
    CJSBlock* NewClass(const std::string& name);
    CJSVar* NewVariable(CJSValue* value, const std::string& name, const size_t size = 0);

    virtual CJSModule* CompileFile(const std::string& name, const std::string& filename);
    virtual CJSModule* CompileSource(const std::string& name, const std::string& filename, const std::string& source);

    virtual CJSModule* Compile(const std::string& name, const std::string& filename, const std::string& source, std::vector<CError>* log = nullptr);

    virtual bool CompileStatement();
    virtual bool CompileDeclare(CJSVar* o);
    virtual bool CompileVarialbleDeclare(CJSVar* o);
    virtual bool CompileClassDeclare(CJSVar* o);
    virtual bool CompileFunctionDeclare(CJSVar* o);
    virtual bool CompileAssignment(CJSVar* o);
    virtual bool CompileIfElse();
    virtual bool CompileLoop();
    virtual bool CompileWhile();
    virtual bool CompileFor();
    virtual bool CompileSwitch();
};


CJSCImpl::CJSCImpl()
{
}

CJSCImpl::~CJSCImpl()
{
    while (!state.empty()) {
        CJSCState& s = state.top();
        s.Free();
        state.pop();
    }
}

CJSCState* CJSCImpl::NewState(const std::string& name, const std::string& filename, const std::string& source)
{
    CJSCState* prev = state.empty() ? nullptr : &state.top();

    state.push(CJSCState());
    CJSCState& s = state.top();

    s.previous = prev;

    s.name = name;
    s.file = filename;
    s.source = source;
    s.lexer = CLexer(filename, source);
    s.module = new CJSModule(name);

    return &state.top();
}

bool CJSCImpl::Error(const int code, const std::string& file, const size_t line, const std::string& text)
{
    errs.push_back(CError(code, file, line, text));
    return false;
}

CJSFunction* CJSCImpl::OnLoad()
{
    CJSCState* s = CurrentState();
    if (!s->onload) {
        s->onload = new CJSFunction();
        s->module->AddOwnProperty("__onload__", CJSPropertyDescriptor(CJSPropertyDescriptor::VALUE, s->onload));
    }
    return s->onload;
}

CJSFunction* CJSCImpl::Executable()
{
    CJSCState* s = CurrentState();
    if (!s->executable) {
        s->executable = new CJSFunction();
        s->module->AddOwnProperty("__executable__", CJSPropertyDescriptor(CJSPropertyDescriptor::VALUE, s->executable));

        std::stack<CJSCallable*> copyOfCallable(s->callable);
        s->callable.clear();
        s->callable.push(s->executable);
        while (!copyOfCallable.empty()) {
            CJSCallable* c = copyOfCallable.top();
            copyOfCallable.pop();
            s->callable.push(c);
        }
    }
    return s->executable;
}

CJSCallable* CJSCImpl::CurrentCallable()
{
    CJSCState* s = CurrentState();
    return s->callable.size() >= 1 ? s->callable.top() : Executable();
}

CJSBlock* CJSCImpl::CurrentBlock()
{
    CJSCState* s = CurrentState();
    if (s->block.empty()) {
        CJSCallable* exec = CurrentCallable();
        CJSBlock blk;
        blk.type = CJSBlock::Scope;
        blk.value = CurrentCallable();
        s->block.push(blk);
    }
    return &s->block.top();
}

CJSObject* CJSCImpl::GetScope(CJSValue* value)
{
    if (value->GetType()->IsObject())
        return (CJSObject*)value;
    else if (value->root)
        return GetScope(value->root);

    CJSCState* s = CurrentState();
    return s->module;
}

CJSBlock* CJSCImpl::NewFunction(const std::string& name, const size_t argc)
{
    CJSCState* s = CurrentState();
    CJSBlock* root = CurrentBlock();
    CJSBlock* scope = root->GetScope();
    if (!scope) scope = s->scope;
    for (size_t i = 0; i < scope->declares.size(); i++) {
        if (!name.empty() && scope->declares[i].name == name && scope->declares[i].size == argc) {
            Error(CError::BadName, s->file, s->line, format("Function with \"%s\" name and %d arguments already declared!", name, argc));
            return nullptr;
        }
    }

    CJSFunction* function = new CJSFunction();

    CJSVar* var = NewVariable(function, name, argc);

    s->block.push(CJSBlock());
    CJSBlock& blk = s->block.top();

    blk.type = CJSBlock::Scope;
    blk.root = root;
    blk.value = var->value;
    blk.file = s->file;
    blk.line = s->line;


    s->callable.push(function);

    return &s->block.top();
}

CJSBlock* CJSCImpl::NewClass(const std::string& name)
{
    CJSCState* s = CurrentState();
    CJSBlock* root = CurrentBlock();
    for (size_t i = 0; i < root->declares.size(); i++) {
        if (root->declares[i].name == name && root->declares[i].size == argc) {
            Error(CError::BadName, s->file, s->line, format("Function with \"%s\" name and %d arguments already declared!", name, argc));
            return nullptr;
        }
    }

    CJSVar* var = NewVariable(new CJSObject(), name);


    s->block.push(CJSBlock());
    CJSBlock& blk = s->block.top();

    blk.type = CJSBlock::Scope;
    blk.root = root;
    blk.value = var->value;
    blk.file = s->file;
    blk.line = s->line;

    return &block.top();
}

CJSVar* CJSCImpl::NewVariable(CJSValue* value, const std::string& name, const size_t size)
{
    CJSCState* s = CurrentState();
    CJSBlock* root = CurrentBlock();
    CJSBlock* scope = root->GetScope();
    if (!scope) scope = s->scope;
    for (size_t i = 0; i < scope->declares.size(); i++) {
        if (scope->declares[i].name == name && scope->declares[i].size == size) {
            Error(CError::BadName, s->file, s->line, format("Function with \"%s\" name and %d arguments already declared!", name, argc));
            return nullptr;
        }
    }

    CJSObject* o = scope->value->AsObject();
    o->AddOwnProperty(name, CJSPropertyDescriptor(CJSPropertyDescriptor::VALUE, value));

    CJSVar var;
    var.name = name;
    var.size = size;
    var.value = value->Grab();
    scope->declares.push_back(var);

    return &scope->declares[scope->declares.size() - 1];
}