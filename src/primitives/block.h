// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PRIMITIVES_BLOCK_H
#define BITCOIN_PRIMITIVES_BLOCK_H

#include <primitives/transaction.h>
#include <serialize.h>
#include <uint256.h>
#include <tinyformat.h>
#include <utilstrencodings.h>

/** Nodes collect new transactions into a block, hash them into a hash tree,
 * and scan through nonce values to make the block's hash satisfy proof-of-work
 * requirements.  When they solve the proof-of-work, they broadcast the block
 * to everyone and the block is added to the block chain.  The first transaction
 * in the block is a special one that creates a new coin owned by the creator
 * of the block.
 */
class CBlockHeader
{
public:
    // header
    int32_t nVersion;
    uint256 hashPrevBlock;
    uint256 hashMerkleRoot;
    uint32_t nTime;
    uint32_t nBits;
    uint32_t nNonce;

    // galaxycash: A copy from CBlockIndex.nFlags from other clients. We need this information because we are using headers-first syncronization.
    mutable int32_t nFlags;
    mutable bool fFlags;
    // galaxycash: Used in CheckProofOfStake().
    static const int32_t NORMAL_SERIALIZE_SIZE = 80;
    static const int32_t MINIMAL_VERSION = 9;
    static const int32_t CURRENT_VERSION = 9;
    static const int32_t X11_VERSION = 10;
    static const int32_t X12_VERSION = 9;
    static const int32_t X13_VERSION = 11;
    static const int32_t SHA256D_VERSION = 12;
    static const int32_t BLAKE2S_VERSION = 13;
    static const int32_t LAST_VERSION = BLAKE2S_VERSION;

    enum {
        ALGO_X12 = 0,
        ALGO_X11,
        ALGO_X13,
        ALGO_SHA256D,
        ALGO_BLAKE2S
    };

    CBlockHeader()
    {
        SetNull();
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(this->nVersion);
        READWRITE(hashPrevBlock);
        READWRITE(hashMerkleRoot);
        READWRITE(nTime);
        READWRITE(nBits);
        READWRITE(nNonce);

        // galaxycash: do not serialize nFlags when computing hash
        if (!(s.GetType() & SER_GETHASH) && (s.GetType() & SER_GALAXYCASH)) {
            READWRITE(nFlags);
            READWRITE(fFlags);
        }
    }

    void SetNull()
    {
        nVersion = CURRENT_VERSION;
        hashPrevBlock.SetNull();
        hashMerkleRoot.SetNull();
        nTime = 0;
        nBits = 0;
        nNonce = 0;
        nFlags = 0;
        fFlags = false;
    }

    bool IsNull() const
    {
        return (nBits == 0);
    }

    void SetAlgorithm(const int32_t nAlgorithm)
    {
        switch (nAlgorithm) {
        case ALGO_X11:
            nVersion = X11_VERSION;
        case ALGO_X12:
            nVersion = X12_VERSION;    
        case ALGO_X13:
            nVersion = X13_VERSION;
        case ALGO_SHA256D:
            nVersion = SHA256D_VERSION;
        case ALGO_BLAKE2S:
            nVersion = BLAKE2S_VERSION;
        default:
            nVersion = X12_VERSION;
        }
    }

    int32_t GetAlgorithm() const
    {
        if (nFlags & (1 << 0)) return ALGO_X12;
        switch (nVersion) {
        case X11_VERSION:
            return ALGO_X11;
        case X12_VERSION:
            return ALGO_X12;
        case X13_VERSION:
            return ALGO_X13;
        case SHA256D_VERSION:
            return ALGO_SHA256D;
        case BLAKE2S_VERSION:
            return ALGO_BLAKE2S;
        default:
            return ALGO_X12;
        }
    }

    unsigned int GetStakeEntropyBit() const; // galaxycash: entropy bit for stake modifier if chosen by modifier

    uint256 GetHash() const;
    uint256 GetPoWHash() const
    {
        return GetHash();
    }

    int64_t GetBlockTime() const
    {
        return (int64_t)nTime;
    }



    std::string ToString() const
    {
        std::stringstream s;
        s << strprintf("CBlockHeader(hash=%s, ver=0x%08x, hashPrevBlock=%s, hashMerkleRoot=%s, nTime=%u, nBits=%08x, nNonce=%u, nFlags=%08x)\n",
            GetHash().GetHex(),
            nVersion,
            hashPrevBlock.ToString(),
            hashMerkleRoot.ToString(),
            nTime, nBits, nNonce,
            nFlags);
        return s.str();
    }    
};


class CBlock : public CBlockHeader
{
public:
    // network and disk
    std::vector<CTransactionRef> vtx;

    // galaxycash: block signature - signed by coin base txout[0]'s owner
    std::vector<unsigned char> vchBlockSig;

    // memory only
    mutable bool fChecked;

    CBlock()
    {
        SetNull();
    }

    CBlock(const CBlockHeader& header)
    {
        SetNull();
        *((CBlockHeader*)this) = header;
    }

    bool IsDeveloperBlock() const;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(*(CBlockHeader*)this);
        READWRITE(vtx);
        READWRITE(vchBlockSig);

        if (!fFlags) MakeFlags();
    }

    bool CopyBlock(CBlock &out) const;

    void SetNull()
    {
        CBlockHeader::SetNull();
        vtx.clear();
        fChecked = false;
        vchBlockSig.clear();
    }

    void MakeFlags() const {
        if (fFlags) return;
        nFlags = 0;
        if (IsDeveloperBlock()) { nFlags |= (1 << 2); }
        else if (IsProofOfStake()) { nFlags |= (1 << 0); };

        uint32_t nEntropyBit = GetStakeEntropyBit();
        nFlags |= ((nEntropyBit && nEntropyBit <= 1) ? (1 << 1) : 0);
        fFlags = true;
    }

    CBlockHeader GetBlockHeader() const
    {
        CBlockHeader block;
        block.nVersion = nVersion;
        block.hashPrevBlock = hashPrevBlock;
        block.hashMerkleRoot = hashMerkleRoot;
        block.nTime = nTime;
        block.nBits = nBits;
        block.nNonce = nNonce;
        block.nFlags = nFlags;
        block.fFlags = fFlags;
        return block;
    }

    void SetAlgorithm(const int32_t algo)
    {
        switch (algo) {
        case CBlockHeader::ALGO_X11:
            nVersion = CBlockHeader::X11_VERSION;
        case CBlockHeader::ALGO_X13:
            nVersion = CBlockHeader::X13_VERSION;
        case CBlockHeader::ALGO_SHA256D:
            nVersion = CBlockHeader::SHA256D_VERSION;
        case CBlockHeader::ALGO_BLAKE2S:
            nVersion = CBlockHeader::BLAKE2S_VERSION;
        default:
            nVersion = CBlockHeader::X12_VERSION;
        }
    }

    int32_t GetAlgorithm() const
    {
        if (IsProofOfStake())
            return CBlockHeader::X12_VERSION;

        switch (nVersion) {
        case CBlockHeader::X11_VERSION:
            return CBlockHeader::ALGO_X11;
        case CBlockHeader::X13_VERSION:
            return CBlockHeader::ALGO_X13;
        case CBlockHeader::SHA256D_VERSION:
            return CBlockHeader::ALGO_SHA256D;
        case CBlockHeader::BLAKE2S_VERSION:
            return CBlockHeader::ALGO_BLAKE2S;
        default:
            return CBlockHeader::ALGO_X12;
        }
        return CBlockHeader::ALGO_X12;
    }

    // galaxycash: two types of block: proof-of-work or proof-of-stake
    bool IsProofOfStake() const
    {
        return (vtx.size() > 1 && vtx[1]->IsCoinStake());
    }

    bool IsProofOfWork() const
    {
        return !IsProofOfStake();
    }

    std::pair<COutPoint, unsigned int> GetProofOfStake() const
    {
        return IsProofOfStake() ? std::make_pair(vtx[1]->vin[0].prevout, vtx[1]->nTime) : std::make_pair(COutPoint(), (unsigned int)0);
    }

    // galaxycash: get max transaction timestamp
    int64_t GetMaxTransactionTime() const
    {
        int64_t maxTransactionTime = 0;
        for (const auto& tx : vtx)
            maxTransactionTime = std::max(maxTransactionTime, (int64_t)tx->nTime);
        return maxTransactionTime;
    }

    unsigned int GetStakeEntropyBit() const; // galaxycash: entropy bit for stake modifier if chosen by modifier

    std::string ToString() const;
};

/** Describes a place in the block chain to another node such that if the
 * other node doesn't have the same branch, it can find a recent common trunk.
 * The further back it is, the further before the fork it may be.
 */
struct CBlockLocator {
    std::vector<uint256> vHave;

    CBlockLocator() {}

    explicit CBlockLocator(const std::vector<uint256>& vHaveIn) : vHave(vHaveIn) {}

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        int nVersion = s.GetVersion();
        if (!(s.GetType() & SER_GETHASH))
            READWRITE(nVersion);
        READWRITE(vHave);
    }

    void SetNull()
    {
        vHave.clear();
    }

    bool IsNull() const
    {
        return vHave.empty();
    }
};

#endif // BITCOIN_PRIMITIVES_BLOCK_H
