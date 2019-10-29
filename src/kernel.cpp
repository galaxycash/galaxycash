// Copyright (c) 2012-2019 The GalaxyCash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bignum.h>
#include <chainparams.h>
#include <consensus/validation.h>
#include <kernel.h>
#include <random.h>
#include <script/interpreter.h>
#include <streams.h>
#include <timedata.h>
#include <txdb.h>
#include <util.h>
#include <utilstrencodings.h>
#include <validation.h>

#include <boost/assign/list_of.hpp>
#include <boost/foreach.hpp>

using namespace std;

// Hard checkpoints of stake modifiers to ensure they are deterministic
static std::map<int, unsigned int> mapStakeModifierCheckpoints;
static std::map<int, unsigned int> mapStakeModifierTestnetCheckpoints;


// Get time weight
int64_t GetWeight(int64_t nIntervalBeginning, int64_t nIntervalEnd)
{
    // Kernel hash weight starts from 0 at the min age
    // this change increases active coins participating the hash and helps
    // to secure the network when proof-of-stake difficulty is low

    return nIntervalEnd - nIntervalBeginning - Params().GetConsensus().nStakeMinAge;
}


// Get selection interval section (in seconds)
static int64_t GetStakeModifierSelectionIntervalSection(int nSection)
{
    assert(nSection >= 0 && nSection < 64);
    return (Params().GetConsensus().nModifierInterval * 63 / (63 + ((63 - nSection) * (MODIFIER_INTERVAL_RATIO - 1))));
}

// Get stake modifier selection interval (in seconds)
static int64_t GetStakeModifierSelectionInterval()
{
    int64_t nSelectionInterval = 0;
    for (int nSection = 0; nSection < 64; nSection++)
        nSelectionInterval += GetStakeModifierSelectionIntervalSection(nSection);
    return nSelectionInterval;
}


uint256 ComputeStakeModifier(const CBlockIndex* pindexPrev, const uint256& kernel)
{
    if (!pindexPrev)
        return 0; // genesis block's modifier is 0

    CDataStream ss(SER_GETHASH, 0);
    ss << kernel << pindexPrev->bnStakeModifier;
    return HashX12(ss.begin(), ss.end());
}

bool CheckStakeKernelHash(const CBlockIndex* pindexPrev, unsigned int nBits, const CBlockHeader& blockFrom, const CTransaction& tx, const CTransaction& txPrev, const COutPoint& prevout, uint256& hashProofOfStake, bool fPrintProofOfStake)
{
    if (tx.nTime < txPrev.nTime) // Transaction timestamp violation
        return error("CheckStakeKernelHash() : nTime violation");

    // Base target
    arith_uint256 bnTarget;
    bnTarget.SetCompact(nBits);

    // Weighted target
    int64_t nValueIn = txPrev.vout[prevout.n].nValue;
    arith_uint256 bnWeight = arith_uint256(nValueIn);
    bnTarget *= bnWeight;

    // Calculate hash
    CDataStream ss(SER_GETHASH, 0);
    ss << pindexPrev->bnStakeModifier;
    ss << txPrev.nTime << prevout.hash << prevout.n << tx.nTime;
    hashProofOfStake = HashX12(ss.begin(), ss.end());

    // Now check if proof-of-stake hash meets target protocol
    if (arith_uint256(hashProofOfStake) > bnTarget)
        return false;

    return true;
}

// Check kernel hash target and coinstake signature
bool CheckProofOfStake(const CBlockIndex* pindexPrev, unsigned int nBits, const CTransaction& tx, uint256& hashProofOfStake)
{
    if (!tx.IsCoinStake())
        return true;

    // Kernel (input 0) must match the stake hash target per coin age (nBits)
    const CTxIn& txin = tx.vin[0];

    // Transaction index is required to get to block header
    if (!fTxIndex)
        return error("CheckProofOfStake() : transaction index not available");

    // Get transaction index for the previous transaction
    CDiskTxPos postx;
    if (!pblocktree->ReadTxIndex(txin.prevout.hash, postx))
        return error("CheckProofOfStake() : tx index not found"); // tx index not found

    // Read txPrev and header of its block
    CBlockHeader header;
    CTransactionRef txPrev;
    {
        CAutoFile file(OpenBlockFile(postx, true), SER_DISK, CLIENT_VERSION);
        try {
            file >> header;
            fseek(file.Get(), postx.nTxOffset, SEEK_CUR);
            file >> txPrev;
        } catch (std::exception& e) {
            return error("%s() : deserialize or I/O error in CheckProofOfStake()", __PRETTY_FUNCTION__);
        }
        if (txPrev->GetHash() != txin.prevout.hash)
            return error("%s() : txid mismatch in CheckProofOfStake()", __PRETTY_FUNCTION__);
    }


    int nDepth = 0;
    CBlockIndex* ptxPrevBlock = mapBlockIndex.find(header.GetHash()) != mapBlockIndex.end() ? mapBlockIndex[header.GetHash()] : NULL;
    if (ptxPrevBlock && pindexPrev)
        nDepth = (pindexPrev->nHeight + 1) - ptxPrevBlock->nHeight;

    if (nDepth < Params().GetConsensus().nStakeMinConfirmations - 1)
        return error("CheckProofOfStake() : tried to stake at depth %d", nDepth + 1);

    // Verify signature
    {
        int nIn = 0;
        const CTxOut& prevOut = txPrev->vout[tx.vin[nIn].prevout.n];
        TransactionSignatureChecker checker(&tx, nIn, prevOut.nValue, PrecomputedTransactionData(tx));

        if (!VerifyScript(tx.vin[nIn].scriptSig, prevOut.scriptPubKey, SCRIPT_VERIFY_P2SH, checker, nullptr))
            return error("%s: VerifyScript failed on coinstake %s", __func__, tx.GetHash().ToString());
    }

    if (!CheckStakeKernelHash(pindexPrev, nBits, header, tx, *txPrev, txin.prevout, hashProofOfStake, gArgs.GetBoolArg("-debug", false)))
        return error("CheckProofOfStake() : INFO: check kernel failed on coinstake %s, hashProof=%s", tx.GetHash().ToString(), hashProofOfStake.ToString()); // may occur during initial download or if behind on block chain sync

    return true;
}

// Check whether the coinstake timestamp meets protocol
bool CheckCoinStakeTimestamp(int64_t nTimeBlock, int64_t nTimeTx)
{
    if (true)
        return (nTimeBlock == nTimeTx) && ((nTimeTx & STAKE_TIMESTAMP_MASK) == 0);
    else
        return (nTimeBlock == nTimeTx);
}

// Get stake modifier checksum
unsigned int GetStakeModifierChecksum(const CBlockIndex* pindex)
{
    assert(pindex->pprev || pindex->GetBlockHash() == Params().GetConsensus().hashGenesisBlock);
    // Hash previous checksum with flags, hashProofOfStake and nStakeModifier
    CDataStream ss(SER_GETHASH, 0);
    if (pindex->pprev)
        ss << pindex->pprev->nStakeModifierChecksum;
    ss << pindex->nFlags << pindex->hashProofOfStake << pindex->bnStakeModifier;
    arith_uint256 hashChecksum = UintToArith256(HashX12(ss.begin(), ss.end()));
    hashChecksum >>= (256 - 32);
    return hashChecksum.GetLow64();
}

// Check stake modifier hard checkpoints
bool CheckStakeModifierCheckpoints(int nHeight, unsigned int nStakeModifierChecksum)
{
    if (nHeight = 0)
        return true;

    bool fTestNet = Params().NetworkIDString() == CBaseChainParams::TESTNET;
    if (fTestNet && mapStakeModifierTestnetCheckpoints.count(nHeight))
        return nStakeModifierChecksum == mapStakeModifierTestnetCheckpoints[nHeight];

    if (!fTestNet && mapStakeModifierCheckpoints.count(nHeight))
        return nStakeModifierChecksum == mapStakeModifierCheckpoints[nHeight];

    return true;
}

bool IsSuperMajority(int minVersion, const CBlockIndex* pstart, unsigned int nRequired, unsigned int nToCheck)
{
    unsigned int nFound = 0;
    for (unsigned int i = 0; i < nToCheck && nFound < nRequired && pstart != NULL; pstart = pstart->pprev) {
        if (!pstart->IsProofOfStake())
            continue;

        if (pstart->nVersion >= minVersion)
            ++nFound;

        i++;
    }
    return (nFound >= nRequired);
}

// galaxycash: entropy bit for stake modifier if chosen by modifier
unsigned int GetStakeEntropyBit(const CBlockHeader& block)
{
    unsigned int nEntropyBit = (block.GetHash().GetLow64() & 1llu); // last bit of block hash
    if (gArgs.GetBoolArg("-printstakemodifier", false))
        LogPrintf("GetStakeEntropyBit(v0.4+): nTime=%u hashBlock=%s entropybit=%d\n", block.nTime, block.GetHash().ToString(), nEntropyBit);
    return nEntropyBit;
}


bool CheckKernel(CBlockIndex* pindexPrev, unsigned int nBits, const CTransaction& tx, uint256* hashProof)
{
    uint256 hashProofOfStake;

    if (!tx.IsCoinStake())
        return error("CheckKernel() : called on non-coinstake %s", tx.GetHash().ToString());

    // Kernel (input 0) must match the stake hash target per coin age (nBits)
    const CTxIn& txin = tx.vin[0];

    // Transaction index is required to get to block header
    if (!fTxIndex)
        return error("CheckKernel() : transaction index not available");

    // Get transaction index for the previous transaction
    CDiskTxPos postx;
    if (!pblocktree->ReadTxIndex(txin.prevout.hash, postx))
        return error("CheckProofOfStake() : tx index not found"); // tx index not found

    // Read txPrev and header of its block
    CBlockHeader header;
    CTransactionRef txPrev;
    {
        CAutoFile file(OpenBlockFile(postx, true), SER_DISK, CLIENT_VERSION);
        try {
            file >> header;
            fseek(file.Get(), postx.nTxOffset, SEEK_CUR);
            file >> txPrev;
        } catch (std::exception& e) {
            return error("%s() : deserialize or I/O error in CheckProofOfStake()", __PRETTY_FUNCTION__);
        }
        if (txPrev->GetHash() != txin.prevout.hash)
            return error("%s() : txid mismatch in CheckProofOfStake()", __PRETTY_FUNCTION__);
    }


    int nDepth = 0;
    CBlockIndex* ptxPrevBlock = mapBlockIndex.find(header.GetHash()) != mapBlockIndex.end() ? mapBlockIndex[header.GetHash()] : NULL;
    if (ptxPrevBlock && pindexPrev)
        nDepth = (pindexPrev->nHeight + 1) - ptxPrevBlock->nHeight;

    if (nDepth < Params().GetConsensus().nStakeMinConfirmations - 1)
        return error("CheckProofOfStake() : tried to stake at depth %d", nDepth + 1);

    bool fOk = CheckStakeKernelHash(pindexPrev, nBits, header, tx, *txPrev, txin.prevout, hashProofOfStake, gArgs.GetBoolArg("-debug", false));
    if (hashProof)
        *hashProof = hashProofOfStake;
    return fOk;
}
