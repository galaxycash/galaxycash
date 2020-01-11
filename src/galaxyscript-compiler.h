// Copyright (c) 2017-2019 The GalaxyCash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef GALAXYCASH_EXT_SCRIPT_COMPILER_H
#define GALAXYCASH_EXT_SCRIPT_COMPILER_H

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

class CVariableStatement : public CStatement
{
public:
    enum {
        VAR = 0,
        LET,
        CONST,
        PROPERTY
    };

    uint8_t type;
    std::string name;
    CExpression* initializer;

    CVariableStatement();
    CVariableStatement(const uint8_t type, const std::string& name);
    virtual ~CVariableStatement();

    virtual uint8_t GetStmType() const { return Variable; }
    virtual CJSValue* CodeGen();
};

class CArgumentStatement : public CStatement
{
public:
    uint8_t index;
    std::string name;
    CExpression* initializer;

    CArgumentStatement();
    CArgumentStatement(const uint8_t index, const std::string& name);
    virtual ~CArgumentStatement();

    virtual uint8_t GetStmType() const { return Argument; }
    virtual CJSValue* CodeGen();
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
    std::vector<CArgumentStatement*> args;
    CBlockStatement* body;

    CFunctionStatement();
    CFunctionStatement(const uint8_t type, const bool async, const std::string& name);
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

    CClassStatement();
    CClassStatement(const std::string& name, const std::string& super);
    virtual ~CClassStatement();

    virtual uint8_t GetStmType() const { return Class; }
    virtual CJSValue* CodeGen();
};

#endif