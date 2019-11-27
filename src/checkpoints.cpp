// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <checkpoints.h>

#include <chain.h>
#include <chainparams.h>
#include <reverse_iterator.h>
#include <validation.h>

#include <stdint.h>


namespace Checkpoints {

    bool CheckBlock(int nHeight, const uint256& hash, bool fMatchesCheckpoint)
    {
        if (!fCheckpointsEnabled)
            return true;

        const MapCheckpoints& checkpoints = Params().Checkpoints().mapCheckpoints;

        MapCheckpoints::const_iterator i = checkpoints.find(nHeight);
        // If looking for an exact match, then return false
        if (i == checkpoints.end()) return !fMatchesCheckpoint;
        return hash == i->second;
    }
    
    int GetTotalBlocksEstimate()
    {
        if (!fCheckpointsEnabled)
            return 0;

        const MapCheckpoints& checkpoints = Params().Checkpoints().mapCheckpoints;

        return checkpoints.rbegin()->first;
    }

    CBlockIndex* GetLastCheckpoint(const CCheckpointData& data)
    {
        const MapCheckpoints& checkpoints = data.mapCheckpoints;

        for (const MapCheckpoints::value_type& i : reverse_iterate(checkpoints))
        {
            const uint256& hash = i.second;
            BlockMap::const_iterator t = mapBlockIndex.find(hash);
            if (t != mapBlockIndex.end())
                return t->second;
        }
        return nullptr;
    }

    uint256 GetLatestHardenedCheckpoint()
    {
        const MapCheckpoints& checkpoints = Params().Checkpoints().mapCheckpoints;
        return (checkpoints.rbegin()->second);
    }

    int GetLatestHardenedCheckpointHeight()
    {
        const MapCheckpoints& checkpoints = Params().Checkpoints().mapCheckpoints;
        return (checkpoints.rbegin()->first);
    }

    bool HasCheckpoint(const uint256 &hash) {
        const MapCheckpoints& checkpoints = Params().Checkpoints().mapCheckpoints;
        for (MapCheckpoints::const_iterator it = checkpoints.begin(); it != checkpoints.end(); it++) {
            if ((*it).second == hash) return true;
        }
        return false;
    }

} // namespace Checkpoints
