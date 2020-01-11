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
        Assignment = (1 << 9)
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
    "yield"};

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


static uint8_t modType = CModule::ModuleDefault;
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