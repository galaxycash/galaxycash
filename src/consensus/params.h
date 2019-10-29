// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_PARAMS_H
#define BITCOIN_CONSENSUS_PARAMS_H

#include <limits>
#include <map>
#include <string>
#include <uint256.h>


namespace Consensus
{
enum DeploymentPos {
    DEPLOYMENT_TESTDUMMY,
    DEPLOYMENT_CSV, // Deployment of BIP68, BIP112, and BIP113.
    MAX_VERSION_BITS_DEPLOYMENTS
};

/**
 * Parameters that influence chain consensus.
 */
struct Params {
    uint256 hashGenesisBlock;
    int nLastPoW;
    /** Block height at which BIP16 becomes active */
    int BIP16Height;
    /** Block height and hash at which BIP34 becomes active */
    int BIP34Height;
    uint256 BIP34Hash;
    uint32_t nRuleChangeActivationThreshold;
    uint32_t nMinerConfirmationWindow;
    /** Proof of work parameters */

    bool fPowAllowMinDifficultyBlocks;
    bool fPowNoRetargeting;
    uint256 nMinimumChainWork;
    uint256 defaultAssumeValid;

    /** galaxycash stuff */
    uint256 bnInitialHashTarget;
    int64_t nTargetSpacingWorkMax;
    int64_t nStakeMinAge;
    int64_t nStakeMaxAge;
    int64_t nModifierInterval;
    int nCoinbaseMaturity; // Coinbase transaction outputs can only be spent after this number of new blocks (network rule)

    int nStakeMinConfirmations;
    int64_t nPOSFirstBlock, nMergeFirstBlock, nMergeLastBlock;
    int64_t nTargetTimespan, nStakeTargetTimespan, nTargetTimespan2;
    int64_t nTargetSpacing, nStakeTargetSpacing, nTargetSpacing2;
    int32_t nLastPowBlock;
    bool fPOWNoRetargeting;
    uint256 stakeLimit;
    uint256 powLimit;
    int nSubsidyHalvingInterval;
    int nHeightV2;

    const uint256& ProofOfWorkLimit() const { return powLimit; }
    const uint256& ProofOfStakeLimit() const { return stakeLimit; }

    bool IsProtocolV1(int32_t nHeight) const { return nHeight <= nHeightV2; }
    bool IsProtocolV2(int32_t nHeight) const { return nHeight > nHeightV2; }
    bool IsProtocolV3(int32_t nHeight) const { return nHeight > nLastPoW; }

    int64_t TargetTimespan(int32_t nHeight) const
    {
        if (IsProtocolV3(nHeight)) return nStakeTargetTimespan;
        return IsProtocolV2(nHeight) ? nTargetTimespan2 : nTargetTimespan;
    }
    int64_t TargetSpacing(int32_t nHeight) const
    {
        if (IsProtocolV3(nHeight)) return nStakeTargetSpacing;
        return IsProtocolV2(nHeight) ? nTargetSpacing2 : nTargetSpacing;
    }

    int64_t StakeTargetTimespan(int32_t nHeight) const
    {
        return nStakeTargetTimespan;
    }
    int64_t StakeTargetSpacing(int32_t nHeight) const
    {
        return nStakeTargetSpacing;
    }

    int64_t DifficultyAdjustmentInterval(int32_t nHeight) const
    {
        return IsProtocolV2(nHeight) ? (nTargetTimespan2 / nTargetSpacing2) : (nTargetTimespan / nTargetSpacing);
    }
    int64_t StakeDifficultyAdjustmentInterval(int32_t nHeight) const { return (nStakeTargetTimespan / nStakeTargetSpacing); }
    int64_t POSStart() const { return nPOSFirstBlock; }
    int64_t MergeStart() const { return nMergeFirstBlock; }
    int64_t MergeEnd() const { return nMergeLastBlock; }
    int32_t LastPowBlock() const { return nLastPoW; }
    int SubsidyHalvingInterval() const { return nSubsidyHalvingInterval; }
    int StakeMinConfirmations() const { return nStakeMinConfirmations; }

    bool IsMergeBlock(int32_t nHeight) const
    {
        if (nHeight >= 1)
            return nHeight >= MergeStart() && nHeight <= MergeEnd();
        else
            return false;
    }
};
} // namespace Consensus

#endif // BITCOIN_CONSENSUS_PARAMS_H
