// Copyright (c) 2017-2019 The GalaxyCash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef GALAXYCASH_EXT_SCRIPT_COMPILER_H
#define GALAXYCASH_EXT_SCRIPT_COMPILER_H

std::string CCGenName();

class CJSAst;
class CJSStatement;
class CJSExpression;
class CJSBlockStatement;
class CJSScopeStatement;


class CJSAst
{
public:
    enum {
        None = 0,
        Statement,
        Expression
    };

    virtual ~CJSAst() {}

    virtual uint8_t GetType() const { return None; }
    virtual CJSValue* CodeGen() { return nullptr; }
};

class CExpression : public CJSAst
{
public:
    enum {
        Binary = 1,
        Unary,
        Call,
        SysCall,
        Await,
        New,
        Identifier
    };

    virtual uint8_t GetType() const { return Expression; }
    virtual uint8_t GetExprType() const { return None; }
    virtual CJSValue* CodeGen() { return nullptr; }
};

class CStatement : public CJSAst
{
public:
    enum {
        List = 1,
        Block,
        Scope,
        Module,
        Variable,
        Argument,
        Function,
        Class,
        IfElse,
        For,
        While,
        DoWhile,
        TryCatch,
        Unary
    };

    virtual uint8_t GetType() const { return Statement; }
    virtual uint8_t GetStmType() const { return None; }
    virtual CJSValue* CodeGen() { return nullptr; }
};

class CStatementList : public CStatement
{
public:
    std::vector<CStatement*> stms;

    CStatementList();
    CStatementList(std::vector<CStatement*> stms);
    virtual ~CStatementList();

    virtual uint8_t GetStmType() const { return List; }
    virtual CJSValue* CodeGen();
};

class CScopeStatement : public CStatementList
{
public:
    CScopeStatement* root;

    CScopeStatement();
    CScopeStatement(CScopeStatement* root);
    virtual ~CScopeStatement();

    virtual uint8_t GetStmType() const { return Scope; }
    virtual CJSValue* CodeGen();
};

class CBlockStatement : public CStatement
{
public:
    CScopeStatement* scope;

    CBlockStatement();
    CBlockStatement(CScopeStatement* root);
    virtual ~CBlockStatement();

    virtual uint8_t GetStmType() const { return Block; }
    virtual CJSValue* CodeGen();
};

class CModuleStatement : public CStatement
{
public:
    CScopeStatement* scope;

    CModuleStatement();
    virtual ~CModuleStatement();

    virtual uint8_t GetStmType() const { return Module; }
    virtual CJSValue* CodeGen();
};

class CVariableStatement : public CStatement
{
public:
    enum {
        VAR = 0,
        LET,
        CONST,
        STATIC
    };

    uint8_t type;
    std::string name;
    CExpression* initializer;

    CVariableStatement();
    CVariableStatement(const uint8_t type, const std::string& name, CExpression* expr = nullptr);
    virtual ~CVariableStatement();

    virtual uint8_t GetStmType() const { return Variable; }
    virtual CJSValue* CodeGen();
};

class CFunctionArgument
{
public:
    bool inout;
    int8_t index;
    std::string name;
    CExpression* initializer;

    inline CFunctionArgument() : inout(true), index(0), name(CCGenName()), initializer(nullptr) {}
    inline CFunctionArgument(const uint8_t index, const std::string& name, const bool inout = true, CExpression* expr = nullptr) : inout(inout), index(index), name(name), initializer(expr) {}
    inline ~CFunctionArgument()
    {
        if (initializer) delete initializer;
    }
};

class CFunctionStatement : public CStatement
{
public:
    enum {
        Default = 0,
        Arrow,
        Setter,
        Getter,
        Constructor,
        Destructor
    };
    uint8_t type;
    bool async;
    std::string name;
    std::vector<CFunctionArgument> args;
    CBlockStatement* body;

    CFunctionStatement();
    CFunctionStatement(const uint8_t type, const bool async, const std::string& name, CBlockStatement* body = nullptr, std::vector<CFunctionArgument> args = std::vector<CFunctionArgument>());
    virtual ~CFunctionStatement();

    virtual uint8_t GetStmType() const { return Function; }
    virtual CJSValue* CodeGen();
};

class CClassStatement : public CStatement
{
public:
    std::string name, super;
    CFunctionStatement *constructor, *destructor;
    std::vector<CFunctionStatement*> methods;
    std::vector<CFunctionStatement*> setters, getters;

    CClassStatement();
    CClassStatement(const std::string& name, const std::string& super);
    virtual ~CClassStatement();

    virtual uint8_t GetStmType() const { return Class; }
    virtual CJSValue* CodeGen();
};

class CJSCompiler
{
public:
    virtual ~CJSCompiler() {}

    virtual CJSModule* Compile(const std::string& file, const std::string& source, std::vector<CError>* log = nullptr) = 0;
};

CJSCompiler* GetJSCompiler();

void CCInitModule();
void CCEmitInt8(int8_t val);
void CCEmitInt16(int16_t val);
void CCEmitInt32(int32_t val);
void CCEmitInt64(int64_t val);
void CCEmitUInt8(uint8_t val);
void CCEmitUInt16(uint16_t val);
void CCEmitUInt32(uint32_t val);
void CCEmitAddr(size_t val);
void CCEmitUInt64(uint64_t val);
void CCEmitFloat(float val);
void CCEmitDouble(double val);
void CCEmitCompactSize(uint64_t nSize);
void CCEmitBytes(std::vector<char>& bytes);
void CCEmitString(const std::string& val);
#endif