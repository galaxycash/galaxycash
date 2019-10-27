// Copyright (c) 2012-2019 The GalaxyCash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef GALAXYCASH_KERNEL_H
#define GALAXYCASH_KERNEL_H

#include <primitives/transaction.h> // CTransaction(Ref)

class CBlockIndex;
class CValidationState;
class CBlockHeader;
class CBlock;


// To decrease granularity of timestamp
// Supposed to be 2^n-1
static const int STAKE_TIMESTAMP_MASK = 15;

// MODIFIER_INTERVAL_RATIO:
// ratio of group interval length between the last group and the first group
static const int MODIFIER_INTERVAL_RATIO = 3;


// Compute the hash modifier for proof-of-stake
bool ComputeNextStakeModifier(const CBlockIndex* pindexCurrent, uint64_t& nStakeModifier, bool& fGeneratedStakeModifier);
uint256 ComputeStakeModifierV2(const CBlockIndex* pindexPrev, const uint256& kernel);

// Check whether stake kernel meets hash target
// Sets hashProofOfStake on success return
bool CheckStakeKernelHash(const CBlockIndex* pindexPrev, unsigned int nBits, const CBlockHeader& blockFrom, const CTransaction& tx, const CTransaction& txPrev, const COutPoint& prevout, uint256& hashProofOfStake, bool fPrintProofOfStake = false);
bool CheckKernel(CBlockIndex* pindexPrev, unsigned int nBits, const CTransaction& tx, uint256* hashProof = nullptr);

// Check kernel hash target and coinstake signature
// Sets hashProofOfStake on success return
bool CheckProofOfStake(const CBlockIndex* pindexPrev, unsigned int nBits, const CTransaction& tx, uint256& hashProofOfStake);

// Check whether the coinstake timestamp meets protocol
bool CheckCoinStakeTimestamp(int64_t nTimeBlock, int64_t nTimeTx);

// Get stake modifier checksum
unsigned int GetStakeModifierChecksum(const CBlockIndex* pindex);

// Check stake modifier hard checkpoints
bool CheckStakeModifierCheckpoints(int nHeight, unsigned int nStakeModifierChecksum);

bool IsSuperMajority(int minVersion, const CBlockIndex* pstart, unsigned int nRequired, unsigned int nToCheck);

// galaxycash: entropy bit for stake modifier if chosen by modifier
unsigned int GetStakeEntropyBit(const CBlockHeader& block);

// Get time weight using supplied timestamps
int64_t GetWeight(int64_t nIntervalBeginning, int64_t nIntervalEnd);

#endif // GALAXYCASH_KERNEL_H
