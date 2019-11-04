// Copyright (c) 2017-2019 The GalaxyCash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef GALAXYCASH_EXT_H
#define GALAXYCASH_EXT_H

// GalaxyCash Extended functionality

#include <chain.h>
#include <coins.h>
#include <dbwrapper.h>
#include <key.h>
#include <net.h>
#include <pubkey.h>

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <base58.h>

#include <primitives/transaction.h>

#include <serialize.h>

#include <galaxyscript.h>

struct CGalaxyCashOperand {
    uint8_t type;
    std::vector<unsigned char> vch;
    enum {
        OPERAND_NULL = 0,
        OPERAND_INT8,
        OPERAND_INT16,
        OPERAND_INT32,
        OPERAND_INT64,
        OPERAND_UINT8,
        OPERAND_UINT16,
        OPERAND_UINT32,
        OPERAND_UINT64,
        OPERAND_BOOLEAN,
        OPERAND_FLOAT,
        OPERAND_DOUBLE,
        OPERAND_STRING,
        OPERAND_TOKEN,
        OPERAND_CONSENSUS,
        OPERAND_HEADER,
        OPERAND_HASH,
        OPERAND_VCH
    };

    CGalaxyCashOperand() : type(OPERAND_NULL) {}

    CGalaxyCashOperand(const uint8_t type) : type(type) {}

    CGalaxyCashOperand(const uint8_t type, const std::vector<unsigned char>& vch)
        : type(type), vch(vch) {}

    CGalaxyCashOperand& operator=(const CGalaxyCashOperand& rhs)
    {
        type = rhs.type;
        vch = rhs.vch;
        return *this;
    }

    CGalaxyCashOperand& SetNull()
    {
        type = OPERAND_NULL;
        vch.clear();
        return *this;
    }

    bool IsNull() const { return (type == OPERAND_NULL); }

    bool IsInteger() const
    {
        return (type >= OPERAND_INT8 && type <= OPERAND_UINT64);
    }

    bool IsInt8() const { return (type == OPERAND_INT8); }

    bool IsInt16() const { return (type == OPERAND_INT16); }

    bool IsInt32() const { return (type == OPERAND_INT32); }

    bool IsInt64() const { return (type == OPERAND_INT64); }

    bool IsUInt8() const { return (type == OPERAND_UINT8); }

    bool IsUInt16() const { return (type == OPERAND_UINT16); }

    bool IsUInt32() const { return (type == OPERAND_UINT32); }

    bool IsUInt64() const { return (type == OPERAND_UINT64); }

    bool IsBoolean() const { return (type == OPERAND_BOOLEAN); }

    bool IsDouble() const { return (type == OPERAND_DOUBLE); }

    bool IsFloat() const { return (type == OPERAND_FLOAT); }

    bool IsString() const { return (type == OPERAND_STRING); }

    bool IsToken() const { return (type == OPERAND_TOKEN); }

    bool IsSignature() const { return (type == OPERAND_VCH); }

    bool IsConsensus() const { return (type == OPERAND_CONSENSUS); }

    bool IsHash() const { return (type == OPERAND_HASH); }

    bool IsVch() const { return (type == OPERAND_VCH); }

    bool IsSigned() const
    {
        return (type >= OPERAND_INT8 && type <= OPERAND_INT64);
    }

    bool IsUnsigned() const
    {
        return (type >= OPERAND_UINT8 && type <= OPERAND_UINT64);
    }

    int Size() const { return vch.size(); }

    std::string AsString() const
    {
        CDataStream s(vch, SER_NETWORK, PROTOCOL_VERSION);

        std::string res;
        s >> res;

        return res;
    }

    int8_t AsInt8() const
    {
        assert((type == OPERAND_INT8 || type == OPERAND_UINT8 || Size() >= 1));
        CDataStream s(vch, SER_NETWORK, PROTOCOL_VERSION);

        int8_t res;
        s >> res;

        return res;
    }

    int16_t AsInt16() const
    {
        assert((type == OPERAND_INT16 || type == OPERAND_UINT16 || Size() >= 2));

        CDataStream s(vch, SER_NETWORK, PROTOCOL_VERSION);

        int16_t res;
        s >> res;

        return res;
    }

    int32_t AsInt32() const
    {
        assert((type == OPERAND_INT32 || type == OPERAND_UINT32 || Size() >= 4));

        CDataStream s(vch, SER_NETWORK, PROTOCOL_VERSION);

        int32_t res;
        s >> res;

        return res;
    }

    int64_t AsInt64() const
    {
        assert((type == OPERAND_INT64 || type == OPERAND_UINT64 ||
                type == OPERAND_DOUBLE || Size() >= 8));

        CDataStream s(vch, SER_NETWORK, PROTOCOL_VERSION);

        int64_t res;
        s >> res;

        return res;
    }

    uint8_t AsUInt8() const
    {
        assert((type == OPERAND_INT8 || type == OPERAND_UINT8 || Size() >= 1));
        CDataStream s(vch, SER_NETWORK, PROTOCOL_VERSION);

        uint8_t res;
        s >> res;

        return res;
    }

    uint16_t AsUInt16() const
    {
        assert((type == OPERAND_INT16 || type == OPERAND_UINT16 || Size() >= 2));

        CDataStream s(vch, SER_NETWORK, PROTOCOL_VERSION);

        uint16_t res;
        s >> res;

        return res;
    }

    uint32_t AsUInt32() const
    {
        assert((type == OPERAND_INT32 || type == OPERAND_UINT32 || Size() >= 4));

        CDataStream s(vch, SER_NETWORK, PROTOCOL_VERSION);

        uint32_t res;
        s >> res;

        return res;
    }

    uint64_t AsUInt64() const
    {
        assert((type == OPERAND_INT64 || type == OPERAND_UINT64 ||
                type == OPERAND_DOUBLE || Size() >= 8));

        CDataStream s(vch, SER_NETWORK, PROTOCOL_VERSION);

        uint64_t res;
        s >> res;

        return res;
    }

    float AsFloat() const
    {
        assert((type == OPERAND_INT32 || type == OPERAND_UINT32 ||
                type == OPERAND_FLOAT || Size() >= 4));

        CDataStream s(vch, SER_NETWORK, PROTOCOL_VERSION);

        float res;
        s >> res;

        return res;
    }

    double AsDouble() const
    {
        assert((type == OPERAND_INT64 || type == OPERAND_UINT64 ||
                type == OPERAND_DOUBLE || Size() >= 8));

        CDataStream s(vch, SER_NETWORK, PROTOCOL_VERSION);

        double res;
        s >> res;

        return res;
    }

    bool AsBoolean() const
    {
        assert((type == OPERAND_INT8 || type == OPERAND_UINT8 ||
                type == OPERAND_BOOLEAN || Size() >= 1));

        CDataStream s(vch, SER_NETWORK, PROTOCOL_VERSION);

        uint8_t res;
        s >> res;

        return res > 0 ? true : false;
    }

    uint256 AsHash() const
    {
        CDataStream s(vch, SER_NETWORK, PROTOCOL_VERSION);

        uint256 res;
        s >> res;

        return res;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(type);
        READWRITE(vch);
    }
};

inline CGalaxyCashOperand HashAsOperand(const uint256& hash)
{
    CGalaxyCashOperand ret(CGalaxyCashOperand::OPERAND_STRING);
    CDataStream s(SER_NETWORK, PROTOCOL_VERSION);
    s << hash;
    ret.vch = std::vector<unsigned char>(s.uptr(), s.uptr() + s.size());
    return ret;
}

inline CGalaxyCashOperand StringAsOperand(const std::string& str)
{
    CGalaxyCashOperand ret(CGalaxyCashOperand::OPERAND_STRING);
    CDataStream s(SER_NETWORK, PROTOCOL_VERSION);
    s << str;
    ret.vch = std::vector<unsigned char>(s.uptr(), s.uptr() + s.size());
    return ret;
}

inline CGalaxyCashOperand BooleanAsOperand(const bool value)
{
    CGalaxyCashOperand ret(CGalaxyCashOperand::OPERAND_BOOLEAN);
    CDataStream s(SER_NETWORK, PROTOCOL_VERSION);
    s << (value ? 1 : 0);
    ret.vch = std::vector<unsigned char>(s.uptr(), s.uptr() + s.size());
    return ret;
}

inline CGalaxyCashOperand DoubleAsOperand(const double value)
{
    CGalaxyCashOperand ret(CGalaxyCashOperand::OPERAND_BOOLEAN);
    CDataStream s(SER_NETWORK, PROTOCOL_VERSION);
    s << value;
    ret.vch = std::vector<unsigned char>(s.uptr(), s.uptr() + s.size());
    return ret;
}

inline CGalaxyCashOperand FloatAsOperand(const float value)
{
    CGalaxyCashOperand ret(CGalaxyCashOperand::OPERAND_BOOLEAN);
    CDataStream s(SER_NETWORK, PROTOCOL_VERSION);
    s << value;
    ret.vch = std::vector<unsigned char>(s.uptr(), s.uptr() + s.size());
    return ret;
}

inline CGalaxyCashOperand Int8AsOperand(const int8_t value)
{
    CGalaxyCashOperand ret(CGalaxyCashOperand::OPERAND_BOOLEAN);
    CDataStream s(SER_NETWORK, PROTOCOL_VERSION);
    s << value;
    ret.vch = std::vector<unsigned char>(s.uptr(), s.uptr() + s.size());
    return ret;
}

inline CGalaxyCashOperand Int16AsOperand(const int16_t value)
{
    CGalaxyCashOperand ret(CGalaxyCashOperand::OPERAND_BOOLEAN);
    CDataStream s(SER_NETWORK, PROTOCOL_VERSION);
    s << value;
    ret.vch = std::vector<unsigned char>(s.uptr(), s.uptr() + s.size());
    return ret;
}

inline CGalaxyCashOperand Int32AsOperand(const int32_t value)
{
    CGalaxyCashOperand ret(CGalaxyCashOperand::OPERAND_BOOLEAN);
    CDataStream s(SER_NETWORK, PROTOCOL_VERSION);
    s << value;
    ret.vch = std::vector<unsigned char>(s.uptr(), s.uptr() + s.size());
    return ret;
}

inline CGalaxyCashOperand Int64AsOperand(const int64_t value)
{
    CGalaxyCashOperand ret(CGalaxyCashOperand::OPERAND_BOOLEAN);
    CDataStream s(SER_NETWORK, PROTOCOL_VERSION);
    s << value;
    ret.vch = std::vector<unsigned char>(s.uptr(), s.uptr() + s.size());
    return ret;
}

inline CGalaxyCashOperand UInt8AsOperand(const uint8_t value)
{
    CGalaxyCashOperand ret(CGalaxyCashOperand::OPERAND_BOOLEAN);
    CDataStream s(SER_NETWORK, PROTOCOL_VERSION);
    s << value;
    ret.vch = std::vector<unsigned char>(s.uptr(), s.uptr() + s.size());
    return ret;
}

inline CGalaxyCashOperand UInt16AsOperand(const uint16_t value)
{
    CGalaxyCashOperand ret(CGalaxyCashOperand::OPERAND_BOOLEAN);
    CDataStream s(SER_NETWORK, PROTOCOL_VERSION);
    s << value;
    ret.vch = std::vector<unsigned char>(s.uptr(), s.uptr() + s.size());
    return ret;
}

inline CGalaxyCashOperand UInt32AsOperand(const uint32_t value)
{
    CGalaxyCashOperand ret(CGalaxyCashOperand::OPERAND_BOOLEAN);
    CDataStream s(SER_NETWORK, PROTOCOL_VERSION);
    s << value;
    ret.vch = std::vector<unsigned char>(s.uptr(), s.uptr() + s.size());
    return ret;
}

inline CGalaxyCashOperand UInt64AsOperand(const uint64_t value)
{
    CGalaxyCashOperand ret(CGalaxyCashOperand::OPERAND_BOOLEAN);
    CDataStream s(SER_NETWORK, PROTOCOL_VERSION);
    s << value;
    ret.vch = std::vector<unsigned char>(s.uptr(), s.uptr() + s.size());
    return ret;
}

inline CGalaxyCashOperand PubKeyAsOperand(const CPubKey& value)
{
    CGalaxyCashOperand ret(CGalaxyCashOperand::OPERAND_BOOLEAN);
    CDataStream s(SER_NETWORK, PROTOCOL_VERSION);
    s << value;
    ret.vch = std::vector<unsigned char>(s.uptr(), s.uptr() + s.size());
    return ret;
}

struct CGalaxyCashOpcode {
    uint8_t type;
    std::vector<CGalaxyCashOperand> operands;

    enum {
        OPCODE_NULL = 0,
        OPCODE_CONSENSUS,
        OPCODE_COINBASE,
        OPCODE_BURN,
        OPCODE_TOKEN,
        OPCODE_TRANSFER,
        OPCODE_REWARD,
        OPCODE_COUNT,
        OPCODE_LAST = (OPCODE_COUNT - 1)
    };

    CGalaxyCashOpcode() : type(OPCODE_NULL) {}

    CGalaxyCashOpcode& operator=(const CGalaxyCashOpcode& rhs)
    {
        type = rhs.type;
        operands = rhs.operands;
        return *this;
    }

    CGalaxyCashOpcode& SetNull()
    {
        type = OPCODE_NULL;
        operands.clear();
        return *this;
    }

    bool IsNull() const { return (type == OPCODE_NULL); }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(type);
        READWRITE(operands);
    }
};

struct CGalaxyCashToken {
    std::string name;
    std::string symbol;
    int64_t supply, fee;
    CPubKey owner;
    bool minable;
    COutPoint proof;


    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(name);
        READWRITE(symbol);
        READWRITE(supply);
        READWRITE(fee);
        READWRITE(owner);
        READWRITE(minable);
        READWRITE(proof);
    }

    CGalaxyCashToken() { SetNull(); }
    CGalaxyCashToken(const CGalaxyCashToken& token)
        : name(token.name), symbol(token.symbol), supply(token.supply),
          owner(token.owner), minable(token.minable), proof(token.proof) {}

    CGalaxyCashToken& operator=(const CGalaxyCashToken& token)
    {
        name = token.name;
        symbol = token.symbol;
        supply = token.supply;
        owner = token.owner;
        minable = token.minable;
        proof = token.proof;
        return *this;
    }

    void SetNull()
    {
        name.clear();
        symbol.clear();
        supply = 0;
        owner = CPubKey();
        minable = false;
        proof.SetNull();
    }

    bool IsNull() const
    {
        return name.empty() || symbol.empty() || owner == CPubKey();
    }

    uint256 GetHash() { return SerializeHash(*this); }
};

typedef std::shared_ptr<CGalaxyCashToken> CGalaxyCashTokenRef;
static inline CGalaxyCashTokenRef MakeGalaxyCashTokenRef() { return std::make_shared<CGalaxyCashToken>(); }
template <typename GalaxyCashToken>
static inline CGalaxyCashTokenRef MakeGalaxyCashTokenRef(GalaxyCashToken&& in)
{
    return std::make_shared<CGalaxyCashToken>(std::forward<GalaxyCashToken>(in));
}

inline CGalaxyCashTokenRef OperandAsToken(const CGalaxyCashOperand& value)
{
    CGalaxyCashTokenRef ret = MakeGalaxyCashTokenRef();
    CDataStream s((const char*)*value.vch.begin(), (const char*)*value.vch.end(), SER_NETWORK, PROTOCOL_VERSION);
    s >> *ret;
    return ret;
}

inline CGalaxyCashOperand TokenAsOperand(const CGalaxyCashTokenRef& value)
{
    CGalaxyCashOperand ret(CGalaxyCashOperand::OPERAND_TOKEN);
    CDataStream s(SER_NETWORK, PROTOCOL_VERSION);
    s << (*value);
    ret.vch = std::vector<unsigned char>(s.uptr(), s.uptr() + s.size());
    return ret;
}

#include "pubkey.h"

class CGalaxyCashTransaction
{
public:
    enum {
        CURRENT_VERSION = 1,
        MINIMAL_VERSION = CURRENT_VERSION
    };

    uint32_t version;
    uint256 token;
    CKeyID address;
    CPubKey pubKey;
    std::vector<CGalaxyCashOpcode>
        script;
    std::vector<unsigned char>
        signature;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(version);
        READWRITE(token);
        READWRITE(address);
        GetPubKey(address, pubKey);
        READWRITE(script);
        if (!(s.GetType() & SER_GETHASH)) READWRITE(signature);
    }

    CGalaxyCashTransaction()
    {
        SetNull();
    }


    void SetNull()
    {
        version = CURRENT_VERSION;
        address = CKeyID();
        pubKey = CPubKey();
        script.clear();
        signature.clear();
    }

    void GetPubKey(const CKeyID& addr, CPubKey& pub);

    uint256 GetHash() const
    {
        return SerializeHash(*this);
    }

    bool CheckSignature() const
    {
        if (!pubKey.IsValid())
            return false;
        return pubKey.Verify(GetHash(), signature);
    }

    bool CheckTransaction() const;

    bool Sign();

    void AddCoinbase(const CGalaxyCashOpcode& op)
    {
        for (std::vector<CGalaxyCashOpcode>::iterator it = script.begin(); it != script.end(); it++) {
            if ((*it).type == CGalaxyCashOpcode::OPCODE_COINBASE)
                return;
        }
        script.push_back(op);
    }

    bool GetCoinbase(CGalaxyCashOpcode& op)
    {
        for (std::vector<CGalaxyCashOpcode>::iterator it = script.begin(); it != script.end(); it++) {
            if ((*it).type == CGalaxyCashOpcode::OPCODE_COINBASE) {
                op = (*it);
                return true;
            }
        }
        return false;
    }

    void AddTransfer(const CPubKey& address, CAmount amount)
    {
        CGalaxyCashOpcode op;
        op.type = CGalaxyCashOpcode::OPCODE_TRANSFER;
        op.operands.push_back(PubKeyAsOperand(address));
        op.operands.push_back(Int64AsOperand(amount));
        script.push_back(op);
    }

    bool GetTransfers(std::vector<CGalaxyCashOpcode>& transfers) const
    {
        for (std::vector<CGalaxyCashOpcode>::const_iterator it = script.begin(); it != script.end(); it++) {
            if ((*it).type == CGalaxyCashOpcode::OPCODE_TRANSFER) {
                transfers.push_back((*it));
                return true;
            }
        }
        return false;
    }

    void Coinbase(CAmount amount)
    {
        CGalaxyCashOpcode op;
        op.type = CGalaxyCashOpcode::OPCODE_COINBASE;
        op.operands.push_back(Int64AsOperand(amount));
        script.push_back(op);
    }

    bool IsCoinbase() const
    {
        for (std::vector<CGalaxyCashOpcode>::const_iterator it = script.begin(); it != script.end(); it++) {
            if ((*it).type == CGalaxyCashOpcode::OPCODE_COINBASE) {
                return true;
            }
        }
        return false;
    }
};

typedef std::shared_ptr<const CGalaxyCashTransaction> CGalaxyCashTransactionRef;
static inline CGalaxyCashTransactionRef MakeGalaxyCashTransactionRef() { return std::make_shared<const CGalaxyCashTransaction>(); }
template <typename Tx>
static inline CGalaxyCashTransactionRef MakeGalaxyCashTransactionRef(Tx&& txIn)
{
    return std::make_shared<const CGalaxyCashTransaction>(std::forward<Tx>(txIn));
}

class CGalaxyCashConsensus
{
public:
    uint32_t version, minVersion;
    uint32_t height;
    int64_t reward;
    int64_t minfee;
    uint32_t spacing, timespan;
    std::vector<unsigned char>
        signature;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(version);
        READWRITE(minProtocolVersion);
        READWRITE(height);
        READWRITE(reward);
        READWRITE(minfee);
        READWRITE(spacing);
        READWRITE(timespan);
        if (!(s.GetType() & SER_GETHASH)) READWRITE(signature);
    }

    uint256 GetHash() const
    {
        return SerializeHash(*this);
    }

    bool CheckSignature() const;
    bool Sign();
};

typedef std::shared_ptr<CGalaxyCashConsensus> CGalaxyCashConsensusRef;
static inline CGalaxyCashConsensusRef MakeGalaxyCashConsensusRef() { return std::make_shared<CGalaxyCashConsensus>(); }
static inline CGalaxyCashConsensusRef MakeGalaxyCashConsensusRef(CGalaxyCashConsensus* in)
{
    return CGalaxyCashConsensusRef(in);
}

class CGalaxyCashDB : public CDBWrapper
{
public:
    CGalaxyCashDB(size_t nCacheSize, bool fMemory = false, bool fWipe = false);

    void UpdateConsensus(const CGalaxyCashConsensusRef& consensus);
    bool GetConsensus(CGalaxyCashConsensusRef& consensus);

    bool AddToken(const CGalaxyCashTokenRef& token);
    bool GetTokenByHash(const uint256& hash, CGalaxyCashTokenRef& token);
    bool GetTokenBySymbol(const std::string& symbol, CGalaxyCashTokenRef& token);
    bool GetTokenByName(const std::string& name, CGalaxyCashTokenRef& token);
    bool GetTokenByOutPoint(const COutPoint& tx, CGalaxyCashTokenRef& token);
    bool IsToken(const uint256& hash);
    bool IsToken(const COutPoint& tx);
};

class CGalaxyCashState
{
private:
    class CGalaxyCashVM* pvm;

public:
    CGalaxyCashDB* pdb;

    CGalaxyCashState();
    ~CGalaxyCashState();

    bool Init();
    void Shutdown();


    void ProcessMessages(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv);

    void EvalScript();
};

void ThreadGalaxyCash();


typedef std::shared_ptr<CGalaxyCashState> CGalaxyCashStateRef;
static inline CGalaxyCashStateRef MakeGalaxyCashStateRef() { return CGalaxyCashStateRef(NULL); }
static inline CGalaxyCashStateRef MakeGalaxyCashStateRef(CGalaxyCashState* in)
{
    return CGalaxyCashStateRef(in);
}

extern CGalaxyCashStateRef g_galaxycash;

bool GalaxyCashCheckScript(CGalaxyCashStateRef& state, const CGalaxyCashOpcode* script, uint32_t count);
bool GalaxyCashRunScript(CGalaxyCashStateRef& state, const CGalaxyCashOpcode* script, uint32_t count);
bool GalaxyCashUndoScript(CGalaxyCashStateRef& state, const CGalaxyCashOpcode* script, uint32_t count);
bool GalaxyCashGetTransaction(const CGalaxyCashStateRef& state, const uint256& hash, CGalaxyCashTransactionRef& tx);


#endif
