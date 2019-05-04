// Copyright (c) 2017-2019 The GalaxyCash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef GALAXYCASH_EXT_H
#define GALAXYCASH_EXT_H

// GalaxyCash Extended functionality

#include "base58.h"
#include "leveldbwrapper.h"

struct CExtOperand {
    uint8_t         type;
    std::vector <unsigned char>
                    vch;
    enum {
        OPERAND_NULL = 0,
        OPERAND_INT16,
        OPERAND_INT32,
        OPERAND_INT64,
        OPERAND_UINT16,
        OPERAND_UINT32,
        OPERAND_UINT64,
        OPERAND_BOOLEAN,
        OPERAND_DOUBLE,
        OPERAND_STRING,
        OPERAND_VCH
    };


    CExtOperand() : type(OPERAND_NULL) {
    }

    CExtOperand(const uint8_t type) : type(type) {}

    CExtOperand &operator = (const CExtOperand &rhs) {
        type = rhs.type;
        vch = rhs.vch;
        return *this;
    }

    CExtOperand &SetNull() {
        type = OPERAND_NULL;
        vch.clear();
        return *this;
    }

    bool IsNull() const {
        return (type == OPERAND_NULL);
    }

    bool IsInteger() const {
        return (type >= OPERAND_INT16 && type <= OPERAND_UINT64);
    }

    bool IsInt16() const {
        return (type == OPERAND_INT16);
    }

    bool IsInt32() const {
        return (type == OPERAND_INT32);
    }

    bool IsInt64() const {
        return (type == OPERAND_INT64);
    }

    bool IsUInt16() const {
        return (type == OPERAND_UINT16);
    }

    bool IsUInt32() const {
        return (type == OPERAND_UINT32);
    }

    bool IsUInt64() const {
        return (type == OPERAND_UINT64);
    }

    bool IsBoolean() const {
        return (type == OPERAND_BOOLEAN);
    }

    bool IsString() const {
        return (type == OPERAND_STRING);
    }

    bool IsVch() const {
        return (type == OPERAND_VCH);
    }

    bool IsSigned() const {
        return (type >= OPERAND_INT16 && type <= OPERAND_INT64);
    }

    bool IsUnsigned() const {
        return (type >= OPERAND_UINT16 && type <= OPERAND_UINT64);
    }

    int Size() const {
        return vch.size();
    }

    std::string AsString() const {
        return (const char *) vch.data();
    }

    IMPLEMENT_SERIALIZE
    (
        READWRITE(type);
        READWRITE(vch);
    )
};

inline CExtOperand StringAsOperand(const std::string &str) {
    CExtOperand ret(CExtOperand::OPERAND_STRING);
    ret.vch = std::vector <unsigned char>((const unsigned char*)(str.c_str()), (const unsigned char*)(str.c_str() + str.size()));
    return ret;
}

struct CExtOpcode {
    uint8_t         type;
    std::vector <CExtOperand>
                    operands;

    enum {
        OPCODE_NULL = 0,
        OPCODE_BURN,
        OPCODE_TOKEN,
        OPCODE_TRANSFER,
        OPCODE_REWARD,
    };


    CExtOpcode() : type(OPCODE_NULL) {
    }

    CExtOpcode &operator = (const CExtOpcode &rhs) {
        type = rhs.type;
        operands = rhs.operands;
        return *this;
    }

    CExtOpcode &SetNull() {
        type = OPCODE_NULL;
        operands.clear();
        return *this;
    }

    bool IsNull() const {
        return (type == OPCODE_NULL);
    }

    IMPLEMENT_SERIALIZE
    (
        READWRITE(type);
        READWRITE(operands);
    )
};

class CToken {
public:
    std::string         name;
    std::string         ticker;
    int64_t             supply;
    uint160             owner;

    IMPLEMENT_SERIALIZE
    (
        READWRITE(name);
        READWRITE(ticker);
        READWRITE(supply);
        READWRITE(owner);
    )

    CToken() {}
    CToken(const CToken &token) : name(token.name), ticker(token.ticker), supply(token.supply), owner(token.owner) {}

    CToken &operator = (const CToken &token) {
        name = token.name;
        ticker = token.ticker;
        supply = token.supply;
        owner = token.owner;
        return *this;
    }

    uint256 GetHash() {
        return SerializeHash(*this);
    }
};

class CExtDB {
protected:
    CLevelDBWrapper db;
public:
    CExtDB(size_t nCacheSize, bool fMemory = false, bool fWipe = false);
    ~CExtDB();

    void AddToken(const CToken &token);
    void GetTokens(std::vector <CToken> &tokens);

    void SetBlockReward(const int64_t rw);
    bool GetBlockReward(int64_t &rw);
};

extern CExtDB *pextDb;

#endif
