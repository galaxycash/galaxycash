// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pow.h>

#include <arith_uint256.h>
#include <chain.h>
#include <primitives/block.h>
#include <uint256.h>

#include <bignum.h>
#include <chainparams.h>

// galaxycash: find last block index up to pindex
const CBlockIndex* GetLastBlockIndex(const CBlockIndex* pindex, int algo, bool fProofOfStake)
{
    while (pindex && pindex->pprev && (pindex->GetBlockAlgorithm() != algo) && pindex->IsProofOfStake() != fProofOfStake)
        pindex = pindex->pprev;
    return pindex;
}

const CBlockIndex* GetLastBlockIndex(const CBlockIndex* pindex, bool fProofOfStake)
{
    while (pindex && pindex->pprev && pindex->IsProofOfStake() != fProofOfStake)
        pindex = pindex->pprev;
    return pindex;
}

const CBlockIndex* GetLastBlockIndex(const CBlockIndex* pindex)
{
    while (pindex && pindex->pprev)
        pindex = pindex->pprev;
    return pindex;
}


unsigned int DarkGravityWave(const CBlockIndex* pindexLast, const int32_t nAlgo, const bool fProofOfStake, const Consensus::Params& params)
{
    /* current difficulty formula, dash - AnonGravity v3, written by Evan Duffield - evan@dash.org */
    const CBlockIndex* BlockLastSolved = pindexLast;
    const CBlockIndex* BlockReading = pindexLast;
    int64_t nActualTimespan = 0;
    int64_t LastBlockTime = 0;
    int64_t PastBlocksMin = 24;
    int64_t PastBlocksMax = 24;
    int64_t CountBlocks = 0;
    arith_uint256 PowLimit = UintToArith256(Params().GetConsensus().ProofOfWorkLimit());
    arith_uint256 PastDifficultyAverage;
    arith_uint256 PastDifficultyAveragePrev;

    if (!pindexLast || params.IsMergeBlock(pindexLast->nHeight + 1))
        return PowLimit.GetCompact();

    if (BlockLastSolved == NULL ||
        BlockLastSolved->nHeight == 0 ||
        BlockLastSolved->nHeight < PastBlocksMin)
        return PowLimit.GetCompact();

    while (BlockReading && BlockReading->nHeight > 0) {
        if (PastBlocksMax > 0 && CountBlocks >= PastBlocksMax)
            break;

        // we only consider proof-of-work blocks for the configured mining algo here
        if (BlockReading->GetBlockAlgorithm() != nAlgo || BlockReading->IsProofOfStake() != fProofOfStake) {
            BlockReading = BlockReading->pprev;
            continue;
        }

        CountBlocks++;

        if (CountBlocks <= PastBlocksMin) {
            if (CountBlocks == 1)
                PastDifficultyAverage.SetCompact(BlockReading->nBits);
            else
                PastDifficultyAverage =
                    ((PastDifficultyAveragePrev * CountBlocks) + (arith_uint256().SetCompact(BlockReading->nBits))) / (CountBlocks + 1);

            PastDifficultyAveragePrev = PastDifficultyAverage;
        }

        if (LastBlockTime > 0) {
            int64_t Diff = (LastBlockTime - BlockReading->GetBlockTime());
            nActualTimespan += Diff;
        }
        LastBlockTime = BlockReading->GetBlockTime();

        if (BlockReading->pprev == NULL) {
            assert(BlockReading);
            break;
        }
        BlockReading = BlockReading->pprev;
    }

    if (!CountBlocks)
        return PowLimit.GetCompact();

    arith_uint256 bnNew(PastDifficultyAverage);

    int64_t _nTargetTimespan = CountBlocks * Params().GetConsensus().TargetSpacing(pindexLast->nHeight);

    if (nActualTimespan < _nTargetTimespan / 3)
        nActualTimespan = _nTargetTimespan / 3;
    if (nActualTimespan > _nTargetTimespan * 3)
        nActualTimespan = _nTargetTimespan * 3;

    // Retarget
    bnNew *= nActualTimespan;
    bnNew /= _nTargetTimespan;

    if (bnNew > PowLimit) {
        bnNew = PowLimit;
    }

    return bnNew.GetCompact();
}

unsigned int GetNextTargetRequired(const CBlockIndex* pindexLast, int algo, bool fProofOfStake, const Consensus::Params& params)
{
    return DarkGravityWave(pindexLast, algo, fProofOfStake, params);
}

bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& params)
{
    arith_uint256 bnTarget;
    bnTarget.SetCompact(nBits);

    // Check range
    if (bnTarget <= 0 || bnTarget > UintToArith256(params.ProofOfWorkLimit()))
        return false;
    // Check proof of work matches claimed amount
    if (hash > ArithToUint256(bnTarget))
        return false;

    return true;
}
