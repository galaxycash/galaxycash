// Copyright (c) 2017-2019 The GalaxyCash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef GALAXYCASH_EXT_SCRIPT_H
#define GALAXYCASH_EXT_SCRIPT_H

#include "hash.h"
#include "random.h"
#include "util.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstring>
#include <map>
#include <unordered_map>
#include <vector>

// GalaxyCash Scripting language

static const std::string CScriptKeywords[] = {
    "break",
    "case",
    "catch",
    "const",
    "continue",
    "debugger",
    "default",
    "delete",
    "do",
    "else",
    "finally",
    "for",
    "function"
    "if",
    "in",
    "instanceof",
    "new",
    "return",
    "switch",
    "throw",
    "try",
    "typeof",
    "var",
    "void",
    "while",
    "with"
    "false",
    "null",
    "true",
    "abstract",
    "boolean",
    "byte",
    "char",
    "class",
    "double",
    "enum",
    "export",
    "extends",
    "final",
    "float",
    "goto",
    "implements",
    "import",
    "int",
    "interface",
    "let",
    "long",
    "native",
    "package",
    "private",
    "protected",
    "public",
    "short",
    "static",
    "super",
    "synchronized",
    "this",
    "throws",
    "transient",
    "volatile",
    "yield"};

static const std::string CScriptKeywordOperator[] = {
    "++",
    "--",
    "+",
    "-",
    "!",
    "~",
    "&",
    "|",
    "^",
    "*",
    "/",
    "%",
    ">>",
    "<<",
    ">>>",
    "<",
    ">",
    "<=",
    ">=",
    "==",
    "===",
    "!=",
    "!==",
    "?",
    "=",
    "+=",
    "-=",
    "/=",
    "*=",
    "%=",
    ">>=",
    "<<=",
    ">>>=",
    "|=",
    "^=",
    "&=",
    "&&",
    "||"};

enum class CScriptAstType {
    /* value_expr */
    undefined_expr,
    null_expr,
    new_expr,
    delete_expr,
    number_expr,
    integer_expr,
    float_expr,
    string_expr,
    variable_stm,
    import_stm,
    export_stm,
    object_expr,
    array_expr,
    /* Op */
    unary_op_stm,
    binary_op_expr,
    /* Code */
    identifier_stm,
    declaration_stm,
    call_expr,
    prototype_stm,
    function_stm,
    block_stm,
    /* Control flow */
    if_else_stm,
    for_stm,
    while_stm,
    do_while_stm,
    return_stm,
    break_stm,
    continue_stm,
};

static std::map<CScriptAstType, std::string> scriptAST{
    {CScriptAstType::undefined_expr, "undefined"},
    {CScriptAstType::null_expr, "null"},
    {CScriptAstType::new_expr, "new"},
    {CScriptAstType::delete_expr, "delete"},
    {CScriptAstType::integer_expr, "integer"},
    {CScriptAstType::float_expr, "float"},
    {CScriptAstType::number_expr, "number"},
    {CScriptAstType::string_expr, "string"},
    {CScriptAstType::object_expr, "object"},
    {CScriptAstType::array_expr, "array"},
    {CScriptAstType::variable_stm, "variable"},
    {CScriptAstType::import_stm, "import"},
    {CScriptAstType::export_stm, "export"},
    {CScriptAstType::identifier_stm, "identifier"},
    {CScriptAstType::declaration_stm, "declaration"},
    {CScriptAstType::unary_op_stm, "unary_op"},
    {CScriptAstType::binary_op_expr, "binary_op"},
    {CScriptAstType::call_expr, "call"},
    {CScriptAstType::prototype_stm, "prototype"},
    {CScriptAstType::function_stm, "function"},
    {CScriptAstType::return_stm, "return"},
    {CScriptAstType::if_else_stm, "if_else"},
    {CScriptAstType::for_stm, "for"},
    {CScriptAstType::while_stm, "while"},
    {CScriptAstType::do_while_stm, "do_while"},
    {CScriptAstType::break_stm, "break"},
    {CScriptAstType::continue_stm, "continue"},
    {CScriptAstType::block_stm, "block"},
};

class CScriptAST : public std::enable_shared_from_this<CScriptAST>
{
public:
    CScriptAstType type;
    std::string file;
    size_t line;

    CScriptAST() : line(0), file(""), type(CScriptAstType::undefined_expr) {}
    CScriptAST(CScriptAstType type) : line(0), file(""), type(type) {}
    virtual ~CScriptAST() {}

    std::string ToString()
    {
        return std::string("{\n") + scriptAST[type] + "\n}";
    }

    std::string getName() { return scriptAST[type]; }
};
typedef std::shared_ptr<CScriptAST> CScriptAstRef;

class CScriptExpressionAST : public CScriptAST
{
public:
    CScriptExpressionAST() : CScriptAST() {}
    CScriptExpressionAST(CScriptAstType type) : CScriptAST(type) {}
    virtual ~CScriptExpressionAST() {}
};
typedef std::shared_ptr<CScriptExpressionAST> CScriptExpr;

class CScriptStatementAST : public CScriptAST
{
public:
    CScriptStatementAST() : CScriptAST() {}
    CScriptStatementAST(CScriptAstType type) : CScriptAST(type) {}
    virtual ~CScriptStatementAST() {}
};
typedef std::shared_ptr<CScriptStatementAST> CScriptStm;


inline bool isUndefined(CScriptExpr expr) { return expr->type == CScriptAstType::undefined_expr; }
inline bool isNull(CScriptExpr expr) { return expr->type == CScriptAstType::null_expr; }
inline bool isVariable(CScriptStm stm) { return stm->type == CScriptAstType::variable_stm; }
inline bool isImport(CScriptStm stm) { return stm->type == CScriptAstType::import_stm; }
inline bool isExport(CScriptStm stm) { return stm->type == CScriptAstType::export_stm; }
inline bool isInteger(CScriptExpr expr) { return expr->type == CScriptAstType::integer_expr; }
inline bool isFloat(CScriptExpr expr) { return expr->type == CScriptAstType::float_expr; }
inline bool isNumber(CScriptExpr expr) { return isInteger(expr) || isFloat(expr); }
inline bool isString(CScriptExpr expr) { return expr->type == CScriptAstType::string_expr; }
inline bool isIdentifier(CScriptStm stm) { return stm->type == CScriptAstType::identifier_stm; }
inline bool isUnaryOp(CScriptStm stm) { return stm->type == CScriptAstType::unary_op_stm; }
inline bool isBinaryOp(CScriptExpr expr) { return expr->type == CScriptAstType::binary_op_expr; }
inline bool isCall(CScriptExpr expr) { return expr->type == CScriptAstType::call_expr; }
inline bool isPrototype(CScriptStm stm) { return stm->type == CScriptAstType::prototype_stm; }
inline bool isFunction(CScriptStm stm) { return stm->type == CScriptAstType::function_stm; }
inline bool isReturn(CScriptStm stm) { return stm->type == CScriptAstType::return_stm; }
inline bool isIf(CScriptStm stm) { return stm->type == CScriptAstType::if_else_stm; }
inline bool isFor(CScriptStm stm) { return stm->type == CScriptAstType::for_stm; }
inline bool isWhile(CScriptStm stm) { return stm->type == CScriptAstType::while_stm; }
inline bool isDoWhile(CScriptStm stm) { return stm->type == CScriptAstType::do_while_stm; }
inline bool isBreak(CScriptStm stm) { return stm->type == CScriptAstType::break_stm; }
inline bool isContinue(CScriptStm stm) { return stm->type == CScriptAstType::continue_stm; }
inline bool isBlock(CScriptStm stm) { return stm->type == CScriptAstType::block_stm; }

class CScriptIntegerAST : public CScriptExpressionAST
{
public:
    typedef long long value_type;
    value_type value;

    CScriptIntegerAST(value_type value) : CScriptExpressionAST(CScriptAstType::integer_expr), value(value) {}
};

class CScriptFloatAST : public CScriptExpressionAST
{
public:
    typedef double value_type;
    value_type value;

    CScriptFloatAST(value_type value) : CScriptExpressionAST(CScriptAstType::float_expr), value(value) {}
};

class CScriptStringAST : public CScriptExpressionAST
{
public:
    typedef std::string value_type;
    value_type value;

    CScriptStringAST(value_type value) : CScriptExpressionAST(CScriptAstType::string_expr), value(value) {}
};

class CScriptReturnStm : public CScriptStatementAST
{
public:
    CScriptExpr value;

    CScriptReturnStm() : CScriptStatementAST(CScriptAstType::return_stm), value(nullptr) {}
    CScriptReturnStm(CScriptExpr value) : CScriptStatementAST(CScriptAstType::return_stm), value(value) {}
};

class CScriptBreakAST : public CScriptStatementAST
{
public:
    CScriptBreakAST() : CScriptStatementAST(CScriptAstType::break_stm) {}
};
typedef std::shared_ptr<CScriptBreakAST> CScriptBreakStm;

class CScriptContinueAST : public CScriptStatementAST
{
public:
    CScriptContinueAST() : CScriptStatementAST(CScriptAstType::continue_stm) {}
};
typedef std::shared_ptr<CScriptContinueAST> CScriptContinueStm;

class CScriptBlockAST : public CScriptStatementAST
{
public:
    std::vector<CScriptStm> statement;

    CScriptBlockAST() : CScriptStatementAST(CScriptAstType::block_stm) {}
    CScriptBlockAST(const CScriptStm& stm) : CScriptStatementAST(CScriptAstType::block_stm) { statement.push_back(stm); }
    CScriptBlockAST(const std::vector<CScriptStm>& stm) : CScriptStatementAST(CScriptAstType::block_stm), statement(stm) {}
};
typedef std::shared_ptr<CScriptBlockAST> CScriptBlockStm;

class CScriptIfAST : public CScriptStatementAST
{
public:
    CScriptExpr cond;
    CScriptBlockStm ifBlock, elseBlock;
    CScriptExpr elseIf;


    CScriptIfAST(CScriptExpr cond) : CScriptStatementAST(CScriptAstType::if_else_stm), cond(cond) {}
    CScriptIfAST(CScriptExpr cond, CScriptBlockStm ifBlock) : CScriptStatementAST(CScriptAstType::if_else_stm), cond(cond), ifBlock(ifBlock) {}
    CScriptIfAST(CScriptExpr cond, CScriptBlockStm ifBlock, CScriptBlockStm elseBlock) : CScriptStatementAST(CScriptAstType::if_else_stm), cond(cond), ifBlock(ifBlock), elseBlock(elseBlock) {}
    CScriptIfAST(CScriptExpr cond, CScriptBlockStm ifBlock, CScriptExpr elseIf) : CScriptStatementAST(CScriptAstType::if_else_stm), cond(cond), ifBlock(ifBlock), elseIf(elseIf) {}
};
typedef std::shared_ptr<CScriptIfAST> CScriptIfStm;

enum CScriptUnaryOp {

};

class CScriptUnaryAST : public CScriptStatementAST
{
public:
    CScriptUnaryOp op;
    CScriptExpr expr;

    CScriptUnaryAST(const CScriptUnaryOp op, CScriptExpr expr) : CScriptStatementAST(CScriptAstType::unary_op_stm), op(op), expr(expr) {}
};
typedef std::shared_ptr<CScriptUnaryAST> CScriptUnaryStm;

enum CScriptBinaryOp {
    BINARYOP_ASSIGN = 0,
    BINARYOP_ASSIGN_PLUS,
    BINARYOP_ASSIGN_MINUS,
    BINARYOP_ASSIGN_MULTIPLY,
    BINARYOP_ASSIGN_DIVIDE,
    BINARYOP_EQUAL,
    BINARYOP_NQUAL,
    BINARYOP_GREATER,
    BINARYOP_GEQUAL,
    BINARYOP_LESS,
    BINARYOP_LEQUAL,
    BINARYOP_PLUS,
    BINARYOP_MINUS,
    BINARYOP_MULTIPLY,
    BINARYOP_DIVIDE,
    BINARYOP_AND,
    BINARYOP_XOR,
    BINARYOP_OR,
    BINARYOP_MOD,
    BINARYOP_BIT_AND,
    BINARYOP_BIT_OR,
    BINARYOP_DOMMA,
    BINARYOP_LSHIFT,
    BINARYOP_RSHIFT,
    BINARYOP_ANDAND,
};

class CScriptBinaryAST : public CScriptAST
{
public:
    CScriptBinaryOp op;
    CScriptExpr lhs, rhs;

    CScriptBinaryAST(const CScriptBinaryOp op, CScriptExpr lhs, CScriptExpr rhs) : CScriptAST(CScriptAstType::binary_op_expr), op(op), lhs(lhs), rhs(rhs) {}
};
typedef std::shared_ptr<CScriptBinaryAST> CScriptBinaryExpr;

class CScriptIdentifierAST : public CScriptAST
{
public:
    std::string name;

    CScriptIdentifierAST(const std::string& name) : CScriptAST(CScriptAstType::identifier_stm), name(name) {}
};
typedef std::shared_ptr<CScriptIdentifierAST> CScriptIdentifierStm;

class CScriptVariableAST : public CScriptAST
{
public:
    CScriptIdentifierStm identifier;
    CScriptExpr initializer;

    CScriptVariableAST(CScriptIdentifierStm identifier, CScriptExpr initializer) : CScriptAST(CScriptAstType::variable_stm), identifier(identifier), initializer(initializer) {}
};
typedef std::shared_ptr<CScriptVariableAST> CScriptVariableStm;


class CScriptImportAST : public CScriptAST
{
public:
    std::vector<CScriptIdentifierStm> imports;

    CScriptImportAST(const std::vector<CScriptIdentifierStm>& imports) : CScriptAST(CScriptAstType::import_stm), imports(imports) {}
};
typedef std::shared_ptr<CScriptImportAST> CScriptImportStm;

class CScriptExportAST : public CScriptAST
{
public:
    std::vector<CScriptIdentifierStm> exports;

    CScriptExportAST(const std::vector<CScriptIdentifierStm>& exports) : CScriptAST(CScriptAstType::export_stm), exports(exports) {}
};
typedef std::shared_ptr<CScriptExportAST> CScriptExportStm;

class CScriptPrototypeAST : public CScriptAST
{
public:
    CScriptIdentifierStm identifier;
    std::vector<CScriptExpr> arguments;

    CScriptPrototypeAST(CScriptIdentifierStm identifier, std::vector<CScriptExpr> arguments) : CScriptAST(CScriptAstType::prototype_stm), identifier(identifier), arguments(arguments) {}
};
typedef std::shared_ptr<CScriptPrototypeAST> CScriptPrototypeStm;

class CScriptCallAST : public CScriptAST
{
public:
    CScriptIdentifierStm identifier;
    std::vector<CScriptExpr> arguments;

    CScriptCallAST(CScriptIdentifierStm identifier, std::vector<CScriptExpr> arguments) : CScriptAST(CScriptAstType::call_expr), identifier(identifier), arguments(arguments) {}
};
typedef std::shared_ptr<CScriptCallAST> CScriptCallExpr;

class CScriptFunctionAST : public CScriptAST
{
public:
    CScriptPrototypeStm prototype;
    CScriptBlockStm body;

    CScriptFunctionAST(CScriptPrototypeStm prototype, CScriptBlockStm body) : CScriptAST(CScriptAstType::function_stm), prototype(prototype), body(body) {}
};
typedef std::shared_ptr<CScriptFunctionAST> CScriptFunctionStm;

class CScriptForAST : public CScriptAST
{
public:
    CScriptExpr cond;
    CScriptBlockStm body;

    CScriptForAST(CScriptExpr cond, CScriptBlockStm body) : CScriptAST(CScriptAstType::for_stm), cond(cond), body(body) {}
};
typedef std::shared_ptr<CScriptForAST> CScriptForExpr;

class CScriptWhileAST : public CScriptAST
{
public:
    CScriptExpr cond;
    CScriptBlockStm body;

    CScriptWhileAST(CScriptExpr cond, CScriptBlockStm body) : CScriptAST(CScriptAstType::while_stm), cond(cond), body(body) {}
};
typedef std::shared_ptr<CScriptWhileAST> CScriptWhileExpr;

class CScriptDoWhileAST : public CScriptAST
{
public:
    CScriptBlockStm body;
    CScriptExpr cond;

    CScriptDoWhileAST(CScriptExpr cond, CScriptBlockStm body) : CScriptAST(CScriptAstType::do_while_stm), cond(cond), body(body) {}
};
typedef std::shared_ptr<CScriptDoWhileAST> CScriptDoWhileExpr;


template <typename Type>
std::shared_ptr<Type> MakeAST(const CScriptAstType type)
{
    return std::make_shared(new Type(type));
}

template <typename Type, typename Value>
std::shared_ptr<Type> MakeAST(const Value& value)
{
    return std::make_shared(new Type(value));
}

using std::cin;
using std::cout;
using std::endl;

enum class CScriptTokenType : char {
    tok_none,
    tok_eof,
    tok_buildin,

    tok_prototype,

    // commands
    tok_function,
    // primary
    tok_identifier,
    // value
    tok_null,
    tok_undefined,
    tok_array,
    tok_object,
    tok_class,
    tok_number,
    tok_integer,
    tok_float,
    tok_string,
    tok_single_char,
    tok_op,
    tok_return,
    tok_break,
    tok_continue,

    tok_declaration, // var let const
    tok_if,
    tok_for,
    tok_while,
    tok_do_while,
};

static std::map<CScriptTokenType, std::string> CScriptTokenName{
    {CScriptTokenType::tok_none, "tok_none"},
    {CScriptTokenType::tok_eof, "tok_eof"},
    {CScriptTokenType::tok_prototype, "tok_prototype"},
    {CScriptTokenType::tok_function, "tok_function"},
    {CScriptTokenType::tok_identifier, "tok_identifier"},
    {CScriptTokenType::tok_number, "tok_number"},
    {CScriptTokenType::tok_integer, "tok_integer"},
    {CScriptTokenType::tok_float, "tok_float"},
    {CScriptTokenType::tok_string, "tok_string"},
    {CScriptTokenType::tok_single_char, "tok_single_char"},
    {CScriptTokenType::tok_op, "tok_op"},
    {CScriptTokenType::tok_return, "tok_return"},
    {CScriptTokenType::tok_if, "tok_if"},
    {CScriptTokenType::tok_while, "tok_while"},
    {CScriptTokenType::tok_for, "tok_for"},
    {CScriptTokenType::tok_do_while, "tok_do_while"},
    {CScriptTokenType::tok_declaration, "tok_declaration"},
};

static std::map<std::string, std::pair<CScriptTokenType, CScriptTokenType>> CScriptKeyword{
    {"array", std::make_pair(CScriptTokenType::tok_array, CScriptTokenType::tok_none)},
    {"object", std::make_pair(CScriptTokenType::tok_object, CScriptTokenType::tok_none)},
    {"class", std::make_pair(CScriptTokenType::tok_class, CScriptTokenType::tok_none)},
    {"string", std::make_pair(CScriptTokenType::tok_string, CScriptTokenType::tok_none)},
    {"number", std::make_pair(CScriptTokenType::tok_number, CScriptTokenType::tok_none)},
    {"float", std::make_pair(CScriptTokenType::tok_float, CScriptTokenType::tok_number)},
    {"integer", std::make_pair(CScriptTokenType::tok_integer, CScriptTokenType::tok_number)},
    {"null", std::make_pair(CScriptTokenType::tok_null, CScriptTokenType::tok_none)},
    {"undefined", std::make_pair(CScriptTokenType::tok_undefined, CScriptTokenType::tok_none)},
    {"function", std::make_pair(CScriptTokenType::tok_prototype, CScriptTokenType::tok_function)},
    {"if", std::make_pair(CScriptTokenType::tok_if, CScriptTokenType::tok_none)},
    {"for", std::make_pair(CScriptTokenType::tok_for, CScriptTokenType::tok_none)},
    {"while", std::make_pair(CScriptTokenType::tok_while, CScriptTokenType::tok_none)},
    {"do", std::make_pair(CScriptTokenType::tok_do_while, CScriptTokenType::tok_none)},
    {"var", std::make_pair(CScriptTokenType::tok_declaration, CScriptTokenType::tok_none)},
    {"let", std::make_pair(CScriptTokenType::tok_declaration, CScriptTokenType::tok_none)},
    {"return", std::make_pair(CScriptTokenType::tok_return, CScriptTokenType::tok_none)},
    {"break", std::make_pair(CScriptTokenType::tok_break, CScriptTokenType::tok_none)},
    {"continue", std::make_pair(CScriptTokenType::tok_continue, CScriptTokenType::tok_none)},
    {"var", std::make_pair(CScriptTokenType::tok_declaration, CScriptTokenType::tok_none)},
    {"let", std::make_pair(CScriptTokenType::tok_declaration, CScriptTokenType::tok_none)},
    {"const", std::make_pair(CScriptTokenType::tok_declaration, CScriptTokenType::tok_none)},
    {"export", std::make_pair(CScriptTokenType::tok_declaration, CScriptTokenType::tok_none)},
    {"class", std::make_pair(CScriptTokenType::tok_prototype, CScriptTokenType::tok_class)},
};

class CScriptToken
{
public:
    typedef std::pair<CScriptTokenType, CScriptTokenType> Type;
    Type type;
    std::string value;


    CScriptToken() : type(std::make_pair(CScriptTokenType::tok_none, CScriptTokenType::tok_none)), value("") {}
    CScriptToken(const Type type, const std::string& value) : type(type), value(value) {}
    virtual ~CScriptToken() {}
};

class CScriptLexer
{
    using IntegerType = unsigned long long;

private:
    std::string str;
    std::string buf;
    const char* ptr;
    char last, prev;

public:
    CScriptToken token;
    IntegerType line;

public:
    CScriptLexer() : CScriptLexer("") {}
    CScriptLexer(const std::string& str) { Setup(str); }
    ~CScriptLexer() {}


    virtual void Reset()
    {
        str = "";
        buf = "";
        ptr = nullptr;
        last = prev = ' ';
        token = CScriptToken();
        line = 1;
    }

    virtual void Setup(const std::string& code)
    {
        Reset();
        buf = code;
        ptr = buf.c_str();
        last = prev = ptr ? *ptr : 0;
    }

    char GetNextChar() { return last; }

    char Peek() const
    {
        if (ptr && *ptr != 0)
            return *(ptr + 1);
        return 0;
    }

    char Get()
    {
        char sym = *ptr;
        if (ptr && *ptr != 0) ptr++;
        return sym;
    }

    bool Eof() const
    {
        return last == 0;
    }

    CScriptToken GetNextToken()
    {
        // Skip space
        while (isspace(last)) {
            if (last == '\n')
                line++;

            last = Get();
            if (Eof()) // End of file
                return token = CScriptToken(std::make_pair(CScriptTokenType::tok_eof, CScriptTokenType::tok_eof), "");
        }

        // Identifier
        // ([a-zA-Z_][a-zA-Z0-9_]*)
        if (isalpha(last) || last == '_') {
            str = last;

            while (isalnum(last = Get()) || last == '_')
                str += last;

            // Is keyword?
            if (CScriptKeyword.find(str) != CScriptKeyword.end())
                return token = CScriptToken(CScriptKeyword[str], str);

            return token = CScriptToken(std::make_pair(CScriptTokenType::tok_identifier, CScriptTokenType::tok_none), str);
        }

        // Number
        // (-?[0-9]*.[0-9]*)
        // If previous token is not a number, maybe minus
        if (isdigit(last)) {
            str = last;
            while (isdigit(last = Get()))
                str += last;

            if (last != '.')
                return token = CScriptToken(std::make_pair(CScriptTokenType::tok_integer, CScriptTokenType::tok_number), str);

            str += last;
            while (isdigit(last = Get()))
                str += last;

            return token = CScriptToken(std::make_pair(CScriptTokenType::tok_float, CScriptTokenType::tok_number), str);
        }

        // String
        // ((".*?")|('.*?'))
        if (last == '\'' || last == '\"') {
            char end_char = last;
            str = "";
            while (true) {
                if (Peek() == '\\')
                    str += GetSpecialChar();
                else
                    str += Get();
                if (Peek() == end_char)
                    break;
            }
            Get();        // eat end_char
            last = Get(); // pre-read a char
            return token = CScriptToken(std::make_pair(CScriptTokenType::tok_string, CScriptTokenType::tok_none), str);
        }

        // Comment
        // (//.*?\n)
        if (last == '/' && Peek() == '/') {
            while (Peek() != '\n' && Peek() != '\r')
                Get();
            while (Peek() == '\n' || Peek() == '\r') {
                line++;
                Get();
            }
            last = Get();
            return GetNextToken();
        }

        // Operator
        str = last;
        switch (last) {
        case '+':
        case '-':
        case '*':
        case '/':
        case '>':
        case '<':
        case '=':
        case '!':
            DoubleCharOp('>');
            DoubleCharOp('<');
            DoubleCharOp('=');
            break;

        case '&':
            DoubleCharOp('&');
            break;
        case '|':
            DoubleCharOp('|');
            break;

        default:
            token = CScriptToken(std::make_pair(CScriptTokenType::tok_single_char, CScriptTokenType::tok_number), str);
            break;
        }

        last = Get(); // pre-read a char

        // Single char
        return token;
    }

    void DoubleCharOp(char c)
    {
        if (Peek() == c)
            str += Get();
        token = CScriptToken(std::make_pair(CScriptTokenType::tok_op, CScriptTokenType::tok_none), str);
    }

    char GetSpecialChar()
    {
        Get(); // eat '\\'
        char c;
        switch ((c = Get())) {
        case 'n':
            return '\n';
        case 't':
            return '\t';
        case 'r':
            return '\r';
        default:
            return c;
        }
    }

    std::string PrintToken(const CScriptToken& t)
    {
        return std::string("Token {\n  type: ") + CScriptTokenName[t.type.second] + "\n  string: " + t.value + "\n}\n";
    }
};

typedef std::vector<CScriptStm> CScriptAstTree;

class CScriptParser : public CScriptLexer
{
public:
    CScriptAstTree ast;
    std::unordered_map<CScriptBinaryOp, int> binOpPrecedence;
    int getBinaryOpPrecedence(const CScriptBinaryOp op) { return binOpPrecedence.find(op) != binOpPrecedence.end() ? binOpPrecedence[op] : -1; }

    CScriptParser() : CScriptLexer()
    {
    }
    CScriptParser(const std::string& code) : CScriptLexer(code)
    {
        Setup(code);
    }

    void SetBinaryOp(const CScriptBinaryOp op, int level)
    {
        binOpPrecedence[op] = level;
    }

    void DelBinaryOp(const CScriptBinaryOp op)
    {
        binOpPrecedence.erase(op);
    }

    void Init()
    {
        if (binOpPrecedence.empty()) {
            binOpPrecedence[CScriptBinaryOp::BINARYOP_ASSIGN] = 30;
            binOpPrecedence[CScriptBinaryOp::BINARYOP_AND] = 40;
            binOpPrecedence[CScriptBinaryOp::BINARYOP_OR] = 40;
            binOpPrecedence[CScriptBinaryOp::BINARYOP_LSHIFT] = 40;
            binOpPrecedence[CScriptBinaryOp::BINARYOP_RSHIFT] = 40;
            binOpPrecedence[CScriptBinaryOp::BINARYOP_GREATER] = 60;
            binOpPrecedence[CScriptBinaryOp::BINARYOP_LESS] = 60;
            binOpPrecedence[CScriptBinaryOp::BINARYOP_GEQUAL] = 60;
            binOpPrecedence[CScriptBinaryOp::BINARYOP_LEQUAL] = 60;
            binOpPrecedence[CScriptBinaryOp::BINARYOP_EQUAL] = 60;
            binOpPrecedence[CScriptBinaryOp::BINARYOP_NQUAL] = 60;
            binOpPrecedence[CScriptBinaryOp::BINARYOP_BIT_AND] = 80;
            binOpPrecedence[CScriptBinaryOp::BINARYOP_BIT_OR] = 80;
            binOpPrecedence[CScriptBinaryOp::BINARYOP_XOR] = 80;
            binOpPrecedence[CScriptBinaryOp::BINARYOP_PLUS] = 90;
            binOpPrecedence[CScriptBinaryOp::BINARYOP_MINUS] = 90;
            binOpPrecedence[CScriptBinaryOp::BINARYOP_MULTIPLY] = 100;
            binOpPrecedence[CScriptBinaryOp::BINARYOP_DIVIDE] = 100;
            binOpPrecedence[CScriptBinaryOp::BINARYOP_MOD] = 100;
        }
        ast.clear();
        SetBinaryOp(CScriptBinaryOp::BINARYOP_DOMMA, 1); // for domma expression
    }

    void Reset()
    {
        Init();
        CScriptLexer::Reset();
    }

    void Setup(const std::string& code)
    {
        Reset();
        CScriptLexer::Setup(code);
        Init();
    }


    CScriptExpr ParserExperssion() { return CScriptExpr(); }
    CScriptStm ParserUnaryStm() { return CScriptStm(); }
    CScriptExpr ParserBinaryExpr(int precedence, CScriptExpr rhs) { return CScriptExpr(); }
    CScriptExpr ParserPrimary() { return CScriptExpr(); }
    CScriptExpr ParserValue() { return CScriptExpr(); }
    CScriptExpr ParserObject() { return CScriptExpr(); }
    CScriptExpr ParserArray() { return CScriptExpr(); }
    CScriptStm ParserIdentifier() { return CScriptStm(); }
    CScriptStm ParserArguments() { return CScriptStm(); }
    CScriptFunctionStm ParserFunction() { return CScriptFunctionStm(); }
    CScriptPrototypeStm ParserPrototype() { return CScriptPrototypeStm(); }
    CScriptStm ParserReturn() { return CScriptStm(); }
    CScriptStm ParserBreak() { return CScriptStm(); }
    CScriptStm ParserContinue() { return CScriptStm(); }
    CScriptStm ParserVariable() { return CScriptStm(); }
    CScriptStm ParserIf() { return CScriptStm(); }
    CScriptStm ParserWhile() { return CScriptStm(); }
    CScriptStm ParserDoWhile() { return CScriptStm(); }
    CScriptStm ParserFor() { return CScriptStm(); }
    CScriptStm ParserDeclaration() { return CScriptStm(); }
    CScriptBlockStm ParserBlock() { return CScriptBlockStm(); }


    CScriptStm ParserStatement()
    {
        CScriptStm ret;
        switch (token.type.first) {
        case CScriptTokenType::tok_declaration:
            ret = ParserDeclaration();
            break;
        case CScriptTokenType::tok_eof:
        default:
            return nullptr;
        }

        while (token.value == ";")
            GetNextToken(); // eat ";"

        return ret;
    }

    CScriptAstTree& Parse()
    {
        GetNextToken();
        while (!Eof()) {
            // cout << "ready> " << "in line: " << LineNumber << endl;
            if (token.type.first == CScriptTokenType::tok_eof) // End of file
                break;

            CScriptStm stm = ParserStatement();
            stm->line = line;
            ast.push_back(stm);
        }
        return ast;
    }

    void ParserError(const std::string& info)
    {
        error("Parser error %s, line %i, token %s", info, line, PrintToken(token));
    }
};


enum CScriptSymbolType {
    SYMBOL_VALUE = 0,
    SYMBOL_VARIABLE,
    SYMBOL_PROTOTYPE
};

std::string SymbolTypeNames[] = {
    "value",
    "variable",
    "prototype"};

enum CScriptPrototypeType {
    PROTOTYPE_FUNCTION = 0,
    PROTOTYPE_OBJECT,
    PROTOTYPE_CLASS
};

std::string PrototypeTypeNames[] = {
    "function",
    "object",
    "class"};

enum CScriptFunctionype {
    FUNCTION_DEFAULT = 0,
    FUNCTION_CONSTRUCTOR,
    FUNCTION_DESTRUCTOR,
    FUNCTION_METHOD,
    FUNCTION_OPERATOR,
    FUNCTION_SETTER,
    FUNCTION_GETTER
};

std::string FunctionTypeNames[] = {
    "function",
    "constructor",
    "destructor",
    "method",
    "operator",
    "setter",
    "getter"};


enum CScriptValueType {
    VALUE_UNDEFINED = 0,
    VALUE_NULL,
    VALUE_VOID,
    VALUE_STRING,
    VALUE_NUMBER,
    VALUE_OBJECT,
    VALUE_ARRAY,
    VALUE_POINTER,
    VALUE_CALLABLE,
    VALUE_PUBKEY,
    VALUE_PRIVKEY,
    VALUE_WIFKEY,
    VALUE_ADDRESS,
    VALUE_TRANSACTION,
    VALUE_BLOCK,
    VALUE_CHAIN,
    VALUE_TOKEN,
    VALUE_MODULE
};


std::string ValueTypeNames[] = {
    "null",
    "void",
    "string",
    "number",
    "object",
    "array",
    "pointer",
    "callable",
    "bignum",
    "pubkey",
    "privkey",
    "wifkey",
    "address",
    "transaction",
    "block",
    "chain",
    "token",
    "module"};

enum CScriptNumberType {
    NUMBER_INTEGER = 0,
    NUMBER_FLOAT,
    NUMBER_BOOLEAN,
    NUMBER_BIGNUM
};

std::string NumberTypeNames[] = {
    "integer",
    "float",
    "boolean",
    "bignum"};

enum CScriptValueFlags {
    VALUE_CONSTANT = (1 << 0),
    VALUE_STATIC = (1 << 1),
    VALUE_NATIVE = (1 << 2),
    VALUE_BUILDIN = (1 << 3),
    VALUE_IMPORT = (1 << 4),
    VALUE_EXPORT = (1 << 5),
    VALUE_PRIVATE = (1 << 6),
    VALUE_UNSIGNED = (1 << 7),
    VALUE_ARGUMENTS = (1 << 8),
    VALUE_THIS = (1 << 9),
    VALUE_SUPER = (1 << 10),
    VALUE_READ = (1 << 11),
    VALUE_WRITE = (1 << 12),
    VALUE_CONFIGURABLE = (1 << 13),
    VALUE_ENUMERABLE = (1 << 14)
};


class CScriptModule;
typedef std::shared_ptr<CScriptModule> CScriptModuleRef;

class CScriptValue;
typedef std::shared_ptr<CScriptValue> CScriptValueRef;

class CScriptString;
typedef std::shared_ptr<CScriptString> CScriptStringRef;

class CScriptNumber;
typedef std::shared_ptr<CScriptNumber> CScriptNumberRef;

class CScriptPointer;
typedef std::shared_ptr<CScriptPointer> CScriptPointerRef;

class CScriptVariable;
typedef std::shared_ptr<CScriptVariable> CScriptVariableRef;

class CScriptProperty;
typedef std::shared_ptr<CScriptProperty> CScriptPropertyRef;

class CScriptPrototype;
typedef std::shared_ptr<CScriptPrototype> CScriptPrototypeRef;

class CScriptCallable;
typedef std::shared_ptr<CScriptCallable> CScriptCallableRef;

class CScriptFunction;
typedef std::shared_ptr<CScriptFunction> CScriptFunctionRef;

class CScriptObject;
typedef std::shared_ptr<CScriptObject> CScriptObjectRef;

class CScriptClass;
typedef std::shared_ptr<CScriptClass> CScriptClasstRef;

class CScriptArray;
typedef std::shared_ptr<CScriptArray> CScriptArrayRef;

class CScriptContext
{
public:
    std::unordered_map<uint256, CScriptModuleRef> modules;
    std::unordered_map<uint256, CScriptValueRef> values;
    std::unordered_map<std::string, uint256> names;

    CScriptContext();
    virtual ~CScriptContext();


    inline uint256 combine(const uint256& a, const uint256& b)
    {
        uint256 buf;
        CSHA256().Write(a.begin(), sizeof(a)).Write(b.begin(), sizeof(b)).Finalize(buf.begin());
        return buf;
    }

    virtual uint256 UniqueID(const std::string& name);
    virtual std::string Name(const uint256& uuid);
    virtual CScriptValueRef Value(const uint256& uuid);
    virtual CScriptModuleRef Module(const uint256& uuid);
    virtual CScriptModuleRef Import(const std::string& name);
    virtual CScriptModuleRef Scope() const;
    virtual CScriptValueRef Void() const;
    virtual CScriptValueRef Null() const;
    virtual CScriptValueRef Zero() const;
    virtual CScriptValueRef One() const;
};
typedef std::shared_ptr<CScriptContext> CScriptContextRef;

class CLinkedValue
{
public:
    typedef std::pair<uint256, uint256> CValueLinkage;

    CScriptContext context;
    CValueLinkage linkage;
    mutable CScriptValueRef value;

    CLinkedValue();
    CLinkedValue(const CScriptContextRef& context, const CScriptValueRef& value);
    CLinkedValue(const CScriptContextRef& context, const uint256& uuid);
    virtual ~CLinkedValue();


    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    void SerializationOp(Stream& s, Operation ser_action)
    {
        std::vector<char> buf;
        if (ser_action.ForRead()) {
            READWRITE(buf);
            SerializeValue(buf);
        } else {
            UnserializeValue(buf);
            READWRITE(buf);
        }
    }

    virtual void SerializeValue(std::vector<char>& buf);
    virtual void UnserializeValue(std::vector<char>& buf);

    virtual void Link(CScriptModuleRef module);
    virtual bool IsLinked() const;


    virtual CScriptValue& AsValue();
    virtual const CScriptValueRef& AsValue() const;
};


class CScriptValue : public std::enable_shared_from_this<CScriptValue>
{
public:
    typedef std::shared_ptr<CScriptValue> Ref;


    CScriptModuleRef module;
    CScriptValueRef scope;

    std::vector<char> data;
    uint256 uuid;
    int32_t flags;

    CScriptValue() : flags(0), uuid(GenUUID()) {}
    CScriptValue(const int32_t flags) : flags(flags), uuid(GenUUID()) {}
    CScriptValue(const Ref& value, const int32_t flags) : flags(value->flags | flags), uuid(value->GenUUID())
    {
    }
    virtual ~CScriptValue() {}

    static uint256 GenUUID()
    {
        return GetRandHash();
    }

    static std::string GenName()
    {
        return GenUUID();
    }

    const uint256& UniqueID() const
    {
        return uuid;
    }


    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(module);
        std::vector<char> buffer;
        if (ser_action.ForRead()) {
            READWRITE(buffer);
            SerializeValue(buffer);
        } else {
            UnserializeValue(buffer);
            READWRITE(buffer);
        }
        READWRITE(data);
        READWRITE(uuid);
        READWRITE(flags);
        READWRITE(scope);
    }

    virtual void SerializeValue(std::vector<char>& buffer)
    {
    }
    virtual void UnserializeValue(std::vector<char>& buffer)
    {
    }

    bool isVariable() const { return false; }
    bool isValue() const { return true; }
    bool isPrototype() const { return (SymbolType() == SYMBOL_PROTOTYPE); }
    bool isUndefined() const { return (ValueType() == VALUE_UNDEFINED); }
    bool isNull() const { return (ValueType() == VALUE_NULL); }
    bool isVoid() const { return (ValueType() == VALUE_VOID); }
    bool isBoolean() const { return isNumber() && (NumberType() == NUMBER_BOOLEAN); }
    bool isInteger() const { return isNumber() && (NumberType() == NUMBER_INTEGER); }
    bool isFloat() const { return isNumber() && (NumberType() == NUMBER_FLOAT); }
    bool isNumber() const { return (ValueType() == VALUE_NUMBER); }
    bool isString() const { return (ValueType() == VALUE_STRING); }
    bool isPointer() const { return (ValueType() == VALUE_POINTER); }
    bool isArray() const { return (ValueType() == VALUE_ARRAY); }
    bool isObject() const { return (ValueType() == VALUE_OBJECT); }
    bool isConstant() const { return (flags & VALUE_CONSTANT); }
    bool isVisible() const { return (flags & VALUE_VISIBLITY); }
    bool isNative() const { return (flags & VALUE_NATIVE); }
    bool isCallable() const { return (ValueType() == VALUE_CALLABLE); }
    bool isFunction() const { return (ValueType() == VALUE_FUNCTION); }
    bool isImport() const { return (flags & VALUE_IMPORT); }
    bool isExport() const { return (flags & VALUE_EXPORT); }
    bool isReadable() const { return (flags & VALUE_READ); }
    bool isWritable() const { return (flags & VALUE_WRITE); }
    bool isConfigurable() const { return (flags & VALUE_CONFIGURABLE); }
    bool isEnumerable() const { return (flags & VALUE_ENUMERABLE); }
    bool isModule() const { return (flags & VALUE_MODULE); }
    bool isPubkey() const { return (ValueType() == VALUE_PUBKEY); }
    bool isPrivkey() const { return (ValueType() == VALUE_PRIVKEY); }
    bool isWifkey() const { return (ValueType() == VALUE_WIFKEY); }
    bool isAddress() const { return (ValueType() == VALUE_ADDRESS); }
    bool isTransaction() const { return (ValueType() == VALUE_TRANSACTION); }
    bool isBlock() const { return (ValueType() == VALUE_BLOCK); }
    bool isChain() const { return (ValueType() == VALUE_CHAIN); }
    bool isBignum() const { return (NumberType() == NUMBER_BIGNUM); }

    virtual int SymbolType() const { return SYMBOL_UNDEFINED; }
    virtual std::string SymbolTypeName() const { return SymbolTypeNames[SymbolType()]; }

    virtual int ValueType() const { return VALUE_NULL; }
    virtual std::string ValueTypeName() { return ValueTypeNames[ValueType()]; }

    virtual int NumberType() const { return NUMBER_INTEGER; }
    virtual std::string NumberTypeName() { return NumberTypeNames[NumberType()]; }


    virtual void SetNamespace(const Ref& ns) { this->ns = ns; }
    virtual Ref GetNamespace() { return this->ns; }
    virtual bool IsLinked() const { return ns != nullptr; }

    virtual void FromValue(const Ref& value) {}
    virtual Ref AsValue() const { return shared_from_this(); }

    virtual Ref ValueByName(const std::string& id) const
    {
        return valueScope != nullptr ? valueModule->ValueByName(id) : MakeNull();
    }

    virtual Ref ValueByUniqueID(const std::string& id) const
    {
        return valueScope != nullptr ? valueModule->ValueUniqueID(id) : MakeNull();
    }

    virtual Ref Clone()
    {
        return std::make_shared<CScriptValue>(*this);
    }

    virtual void SetPointer(const void* p) {}
    virtual void* AsPointer() const { return nullptr; }

    virtual void SetBoolean(const bool value) {}
    virtual bool AsBoolean() const { return false; }

    virtual void SetInteger(const int64_t value);
    virtual int64_t AsInteger() const { return 0; }

    virtual void SetFloat(const double value) {}
    virtual double AsFloat() const { return 0.0; }

    virtual void SetString(const std::string& value) {}
    virtual std::string AsString() const { return ""; }

    virtual void SetBignum(const uint256& bignum) {}
    virtual uint256 AsBignum() const { return uint256(); }


    static Ref Parse(const Ref& value, const Ref& replacer);
    static Ref Stringify(const Ref& value, const Ref& replacer, const Ref& space);


    static Ref MakeVariable(const CScriptValueRef& name, const Ref& value, int32_t flags = 0);
    static Ref MakeVariable(const std::string& name, const Ref& value, int32_t flags = 0);
    static Ref MakeUndefined();
    static Ref MakeNull() { return std::make_shared<CScriptValue>(); }
    static Ref MakeVoid();
    static Ref MakeBoolean(const bool value, int32_t flags = 0) { return std::make_shared<CScriptValue>(); }
    static Ref MakeInteger(const int64_t value, int32_t flags = 0) { return std::make_shared<CScriptValue>(); }
    static Ref MakeFloat(const double value, int32_t flags = 0) { return std::make_shared<CScriptValue>(); }
    static Ref MakeString(const std::string& value, int32_t flags = 0);
    static Ref MakePointer(void* p) { return std::make_shared<CScriptValue>(); }
    static Ref MakeArray() { return std::make_shared<CScriptValue>(); }
    static Ref MakeObject();
    static Ref MakePubkey() { return std::make_shared<CScriptValue>(); }
    static Ref MakePrivkey() { return std::make_shared<CScriptValue>(); }
    static Ref MakeWifkey() { return std::make_shared<CScriptValue>(); }
    static Ref MakeAddress() { return std::make_shared<CScriptValue>(); }
    static Ref MakeTransaction() { return std::make_shared<CScriptValue>(); }
    static Ref MakeBlock() { return std::make_shared<CScriptValue>(); }
    static Ref MakeChain() { return std::make_shared<CScriptValue>(); }
};

class CScriptValueUndefined : public CScriptValue
{
public:
    virtual int SymbolType() const { return SYMBOL_UNDEFINED; }
    virtual int ValueType() const { return VALUE_NULL; }


    static CScriptValueRef Make() { return std::make_shared<CScriptValueUndefined>(); }
};

CScriptValueRef CScriptValue::MakeUndefined() { return CScriptValueUndefined::Make(); }

class CScriptValueVoid : public CScriptValue
{
public:
    virtual int ValueType() const { return VALUE_VOID; }
    static CScriptValueRef Make() { return std::make_shared<CScriptValueVoid>(); }
};
CScriptValueRef CScriptValue::MakeVoid() { return CScriptValueVoid::Make(); }

class CScriptString : public CScriptValue
{
public:
    std::string value;

    CScriptString(const std::string& value, int32_t flags) : CScriptValue(flags), value(value) {}
    virtual int ValueType() const { return VALUE_VOID; }
    static CScriptValueRef Make(const std::string& value, int32_t flags = 0) { return std::make_shared<CScriptString>(value, flags); }
};
CScriptValueRef CScriptValue::MakeString(const std::string& value, int32_t flags) { return CScriptString::Make(value, flags); }

class CScriptVariable : public CScriptValue
{
public:
    typedef std::shared_ptr<CScriptVariable> Ref;

    std::string name, valueUUID;
    CScriptValueRef value;

    CScriptVariable() : CScriptValue(), name(GenName()), value(CScriptValue::MakeNull()) {}
    CScriptVariable(const CScriptVariableRef variable, int32_t flags = 0) : name(variable->name), CScriptValue(variable, flags), value(variable->value == nullptr ? CScriptValue::MakeNull() : variable->value)
    {
    }
    CScriptVariable(const std::string& name, const CScriptValueRef& value, int32_t flags = 0) : CScriptValue(flags), name(name.empty() ? GenName() : name), value(value) {}
    virtual ~CScriptVariable() {}

    virtual void AssignVariable(const uint256& uuid)
    {
        if (valueScope) {
            valueUUID = uuid;
            value = valueScope->valueByUUID(uuid);
        }
    }

    virtual void SerializeValue(std::vector<char>& buffer)
    {
        CDataStream ss(buffer.begin(), buffer.end(), SER_DISK, PROTOCOL_VERSION);
        ss >> valueUUID;
        ss >> name;

        AssignVariable(valueUUID, value);
    }

    virtual void UnserializeValue(std::vector<char>& buffer)
    {
        CDataStream ss(SER_DISK, PROTOCOL_VERSION);
        ss << value != nullptr ? value->UniqueID() : "nullnullnullnull";
        ss << name;
    }

    virtual int Type() const { return SYMBOL_VARIABLE; }

    virtual int ValueType() const { return (value != nullptr) ? value->ValueType() : VALUE_NULL; }
    virtual std::string ValueTypeName() const { return ValueTypeNames[ValueType()]; }

    virtual void FromValue(const CScriptValueRef& value)
    {
        value->FromValue(value);
    }
    virtual CScriptValueRef AsValue() const { return value != nullptr ? value->AsValue() : CScriptValue::MakeNull(); }

    virtual Ref Clone()
    {
        return std::make_shared<CScriptVariable>(shared_from_this(), 0);
    }


    virtual bool isVariable() const { return true; }

    static CScriptValueRef Make(const std::string& name, const CScriptValueRef& value, int32_t flags = 0)
    {
        if (name.length() == 0) return CScriptValue::MakeNull();
        return std::make_shared<CScriptVariable>(name, value, flags);
    }
    static CScriptValueRef Make(const CScriptValueRef& name, const CScriptValueRef& value, int32_t flags = 0)
    {
        if (name == nullptr) return CScriptValue::MakeNull();
        std::string str = name->AsString();
        if (str.length() == 0) return CScriptValue::MakeNull();
        return std::make_shared<CScriptVariable>(str, value, flags);
    }
};

CScriptValueRef CScriptValue::MakeVariable(const CScriptValueRef& name, const CScriptValueRef& value, int32_t flags) { return CScriptVariable::Make(name, value, flags); }
CScriptValueRef CScriptValue::MakeVariable(const std::string& name, const CScriptValueRef& value, int32_t flags) { return CScriptVariable::Make(name, value, flags); }

class CScriptLitteral : public CScriptValue
{
public:
    uint8_t type;

    CScriptLitteral() : CScriptValue() {}
    CScriptLitteral(const uint8_t type, const int32_t flags) : type(type), CScriptValue(flags) {}
};

class CScriptSetter : public CScriptCallable
{
public:
    CScriptValueRef object;
    CScriptValueRef property;
    CScriptValueRef callable;

    CScriptSetter() : CScriptCallable(VALUE_SETTER) {}

    virtual int ValueType() const { return VALUE_CALLABLE; }

    virtual bool isSetter() const { return true; }
};

class CScriptProperty : public CScriptValue
{
public:
    typedef std::shared_ptr<CScriptProperty> Ref;

    CScriptValueRef name;
    CScriptValueRef value;
    CScriptValueRef setter, getter;

    CScriptProperty() : name(GenName()), value(MakeNull()), CScriptValue()
    {
    }
};

class CScriptPrototype : public CScriptValue
{
public:
    typedef std::shared_ptr<CScriptPrototype> Ref;
    typedef std::vector<CScriptValueRef> Elements;
    typedef std::map<std::string, CScriptPropertyRef> KeyValues;

    KeyValues keys;
    Elements elements;

    CScriptVariable() : CScriptValue(), name(GenName()), value(CScriptValue::MakeNull()) {}
    CScriptVariable(const CScriptVariableRef variable, int32_t flags = 0) : name(variable->name), CScriptValue(variable, flags), value(variable->value == nullptr ? CScriptValue::MakeNull() : variable->value)
    {
    }
    CScriptVariable(const std::string& name, const CScriptValueRef& value, int32_t flags = 0) : CScriptValue(flags), name(name.empty() ? GenName() : name), value(value) {}
    virtual ~CScriptVariable() {}

    void defineGetter(const std::string& name, const CScriptValueRef& value);
    inline void defineGetter(const CScriptValueRef& name, const CScriptValueRef& value) { defineGetter(name.AsString(), value); }
    void defineSetter(const std::string& name, const CScriptValueRef& value);
    inline void defineSetter(const CScriptValueRef& name, const CScriptValueRef& value) { defineSetter(name.AsString(), value); }
};

class CScriptArray : public CScriptValue
{
public:
    typedef std::shared_ptr<CScriptArray> Ref;
    typedef std::vector<CScriptValueRef> Elements;
    CScriptPrototypeRef prototype;
    CScriptValueRef length;
    Elements elements;

    CScriptArray() : CScriptValue()
    {
    }
    CScriptArray(const Elements& elements, int32_t flags = 0) : CScriptValueRef(flags)
    {
        for (Elements::iterator it = elements.begin(); it != elements.end(); it++) {
            this->elements.push_back(CScriptVariable::Make("", (*it)->AsValue()))
        }
    }
    virtual ~CScriptArray()
    {
    }
    virtual int ValueType() const { return VALUE_ARRAY; }

    virtual void SetElement(size_t index, const CScriptValueRef& value)
    {
        if (elements.size() > index) {
            elements[index] = value;
        } else {
            elements.resize(index + 1);
            elements[index] = value;
        }
    }

    virtual CScriptValueRef GetElement(size_t index)
    {
        if (elements.size() <= index) return CScriptValue::MakeNull();
        return elements[index];
    }

    virtual void SetArrayElement(const CScriptValueRef& index, const CScriptValueRef& value)
    {
        size_t idx = (index != nullptr) ? (size_t)index->AsInteger() : 0;

        if (idx > 0 && elements.size() <= idx) {
            elements.resize(idx + 1);
        }

        if (index != nullptr) elements[idx] = value;
    }

    virtual CScriptValueRef GetArrayElement(const CScriptValueRef& index)
    {
        size_t idx = index != nullptr ? (size_t)index->AsInteger() : 0;
        if (index == nullptr || elements.size() <= idx) return CScriptValue::MakeNull();
        return elements[idx];
    }

    virtual CScriptValueRef GetArrayLength()
    {
        return CScriptValue::MakeNull(); //MakeInteger(elements.size());
    }

    virtual CScriptValueRef Push(const CScriptValueRef& value)
    {
        return CScriptValue::MakeNull();
    }
};

class CScriptObject : public CScriptValue
{
public:
    typedef std::shared_ptr<CScriptObject> Ref;
    typedef std::map<std::string, CScriptValueRef> KeyValues;

    CScriptPrototypeRef prototype;
    KeyValues keys;

    virtual int ValueType() const { return VALUE_OBJECT; }
    virtual bool isObject() const { return true; }

    CScriptObject() : CScriptValue() {}
    CScriptObject(const int32_t flags) : CScriptValue(flags)
    {
    }
    virtual ~CScriptObject()
    {
    }

    virtual CScriptArrayRef Values()
    {
        CScriptArrayRef arr = CScriptArray::Make();
        for (KeyValues::iterator it = keys.begin(); it != keys.end(); it++) {
        }
    }


    virtual void SetProperty(const CScriptValueRef prop, const CScriptValueRef& value)
    {
        std::string name = prop->AsString();
        if (!name.empty())
            keys[name] = value;
    }

    virtual CScriptValueRef GetProperty(const CScriptValueRef prop)
    {
        std::string propertyName = prop->AsString();
        if (propertyName.empty())
            return CScriptValue::MakeNull();

        std::map<std::string, Ref>::iterator it = keys.find(propertyName);
        if (it != keys.end()) {
            return (*it).second;
        }
        return CScriptValue::MakeNull();
    }

    virtual Ref HasOwnProperty(const Ref& prop)
    {
        if (prop != nullptr && keys.find(prop->AsString()) != keys.end())
            return MakeBoolean(true);

        return MakeBoolean(false);
    }
};

CScriptValueRef CScriptValue::MakeObject()
{
    CScriptObjectRef res = std::make_shared<CScriptObject>();
    return res;
}


enum CScriptOpcode {
    SCRIPT_OP_NOP = 0,
    SCRIPT_OP_NEW_NUMBER,    // Allocate number
    SCRIPT_OP_NEW_STRING,    // Allocate string
    SCRIPT_OP_NEW_POINTER,   // Allocate pointer
    SCRIPT_OP_NEW_VARIABLE,  // Allocate variable
    SCRIPT_OP_NEW_ARRAY,     // Allocate new array
    SCRIPT_OP_NEW_CLASS,     // Allocate new class
    SCRIPT_OP_NEW_OBJECT,    // Allocate new object
    SCRIPT_OP_NEW_PROTOTYPE, // Allocate new prototype
    SCRIPT_OP_NEW_PROPERTY,  // Allocate new property
    SCRIPT_OP_NEW_FUNCTION,  // Allocate new function
    SCRIPT_OP_NEW_VARIABLE,  // Allocate new variable
    SCRIPT_OP_PRTSET,        // Prototype set
    SCRIPT_OP_PRTGET,        // Prototype get
    SCRIPT_OP_ARGSET,        // Argument set
    SCRIPT_OP_ARGSSET,       // Arguments set
    SCRIPT_OP_ARGGET,        // Argument get
    SCRIPT_OP_ARGSGET,       // Arguments get
    SCRIPT_OP_SCOPEGET,      // Scoped variable set
    SCRIPT_OP_SCOPESET,      // Scoped variable get
    SCRIPT_OP_THISGET,       // This scoped variable set
    SCRIPT_OP_THISSET,       // This scoped variable get
    SCRIPT_OP_DELETE,        // delete var
    SCRIPT_OP_PLUS,          // +
    SCRIPT_OP_MINUS,         // -
    SCRIPT_OP_MULTIPLY,      // *
    SCRIPT_OP_DIVIDE,        // /
    SCRIPT_OP_MOD,           // %
    SCRIPT_OP_CALL           // callable(this, args)
};

class CScriptCode
{
public:
    uint8_t opcode;
    std::vector<CScriptValueRef> args;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(opcode);
        READWRITE(args);
    }
};

class CScriptCallable : public CScriptValue
{
public:
    virtual bool isCallable() const { return true; }

    virtual CScriptValueRef Execute(const CScriptValueRef& thisArg, const CScriptValueRef& args, const CScriptValueRef& returnArg)
    {
        return MakeVoid();
    }
    virtual CScriptValueRef ExecuteOpcode(const CScriptOpcode& opcode)
    {
        return MakeVoid();
    }
};

class CScriptFunction : public CScriptCallable
{
public:
    CScriptValueRef caller;
    CScriptValueRef name, displayName;
    CScriptArrayRef arguments;

    virtual int ValueType() const { return VALUE_FUNCTION; }
    virtual bool isFunction() const { return true; }

    CScriptFunction() : CScriptCallable() {}
    CScriptFunction(const int32_t flags) : CScriptCallable(flags | VALUE_CALLABLE)
    {
    }
    virtual ~CScriptFunction()
    {
    }

    virtual CScriptValueRef Call(const CScriptValueRef& thisArg, const CScriptArrayRef& args) { return MakeVoid(); }
};


class CScriptModule : public CScriptObject
{
public:
    std::unordered_map<uint256, CScriptValueRef> storage;
    std::unordered_map<std::string, uint256> names;

    virtual int ValueType() const { return VALUE_OBJECT; }

    CScriptModule() : CScriptObject(VALUE_MODULE) {}
    CScriptModule(const int32_t flags) : CScriptObject(flags | VALUE_MODULE)
    {
    }
    virtual ~CScriptModule()
    {
    }

    virtual CScriptVariableRef NewVariable(const std::string& name, const CScriptValueRef& value, int32_t flags)
    {
        if (names.count(name)) return storage[names[name]];
        CScriptVariableRef variable = CScriptVariable::Make(name, value, flags);
        return storage[names[name]] = variable;
    }

    virtual void AssignName(const std::string& name, const uint256& uuid)
    {
        names[name] = uuid;
    }

    virtual void Assign(const uint256& uuid, const CScriptValueRef& value)
    {
        storage[uuid] = value;
    }

    virtual void Initialize()
    {
    }
};
typedef std::shared_ptr<CScriptModule> CScriptModuleRef;

class CScriptCodegen
{
public:
    CScriptCodegen();
    virtual ~CScriptCodegen();

    virtual CScriptModuleRef Make(CScriptParser* parser);
    virtual CScriptModuleRef MakeFile(const std::string& path);
};

#endif