// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CHECKPOINTS_H
#define BITCOIN_CHECKPOINTS_H

#include <uint256.h>

#include <map>

class CBlockIndex;
struct CCheckpointData;

/**
 * Block-chain checkpoints are compiled-in sanity checks.
 * They are updated every release or three.
 */
namespace Checkpoints
{

//! Returns last CBlockIndex* in mapBlockIndex that is a checkpoint
CBlockIndex* GetLastCheckpoint(const CCheckpointData& data);
uint256 GetLatestHardenedCheckpoint();
int GetLatestHardenedCheckpointHeight();
bool HasCheckpoint(const uint256 &hash);

//! Returns true if block passes checkpoint checks
bool CheckBlock(int nHeight, const uint256& hash, bool fMatchesCheckpoint = false);

//! Return conservative estimate of total number of blocks, 0 if unknown
int GetTotalBlocksEstimate();

} //namespace Checkpoints

#endif // BITCOIN_CHECKPOINTS_H
