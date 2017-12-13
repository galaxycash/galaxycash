// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/assign/list_of.hpp> // for 'map_list_of()'
#include <boost/foreach.hpp>

#include "checkpoints.h"

#include "txdb.h"
#include "main.h"
#include "uint256.h"


static const int nCheckpointSpan = 0;

namespace Checkpoints
{
    typedef std::map<int, uint256> MapCheckpoints;

    //
    // What makes a good checkpoint block?
    // + Is surrounded by blocks with reasonable timestamps
    //   (no blocks before with a timestamp after, none after with
    //    timestamp before)
    // + Contains no strange transactions
    //
    static MapCheckpoints mapCheckpoints =
        boost::assign::map_list_of
         ( 0,    uint256S("0x0000023a0c024e3de098954961530f0491c1b78f1c4fba9e9601673d7bd85b7c") )
         ( 1000, uint256S("0x0000000008b8fffdcd3d1fb1f4ab2d22edc116409c4b4672d709f63921926c27") )
         ( 2000, uint256S("0x00000000366097da778b461e053bba7217a76abe46f259f59806eb718e20c9ff") )
         ( 3000, uint256S("0x0000000002125717e0eaf9bb240876e9f8e10d963a5218dc714f5fc87aec285d") )
         ( 4298, uint256S("0x00000000143a622ada8e4e973e6e8d8d7f7ad0fd0c3e13489f8311abff964dd4") )
         ( 4975, uint256S("0x0000000015a2d023f1c94d9a4d88fd2fd71c9f9a8104d419de5818b667f24236") )
         ( 5770, uint256S("0x000000267c58309f7d50dd3c6e589e7f0df9ed26753421cb39988dca09fc4c59") )
         ( 5771, uint256S("0x00000003fc145a2846c6c5d1d0599a80ffa2e857c79cf399a2724fd64d39bdb8") )
         ( 5780, uint256S("0x00000003fbcc1fc6a76daee3cbffc3edc59c052b3252a4ee2a399ea4d195d589") )
         ( 5800, uint256S("0x00000000da516bc0b9f6bcf5f5dfabf44c45457e8e07867eb9c56909ab7162e3") )
         ( 5850, uint256S("0x0000003074dd6626c33b920ff6017f244a08258265a346128b217d180d3f6e6d") )
         ( 5933, uint256S("0x00000000e4f992a1e61f89d493d361324922331e4edf6c07cd6577e5c864e4b4") )
         ( 5942, uint256S("0x00000000389e10ce9fd153f304638effdc0836999cd526d65b24be40404dd5c0") )
         ( 6000, uint256S("0x000000a073b20534bd7c728d1fa4c59a1036f78c22d71a988973a721d14b8ef3") )
         ( 7000, uint256S("0x000000ac4dd27055d37eb0cf39f2310f14f7bd6ff03570fa7794bd7205ed5ce1") )
         ( 7500, uint256S("0x0000000045963a8c7a2e9a22d0d9d1f692761e679713542d610d66df86518ae2") )
         ( 8000, uint256S("0x00000000283348147207ab6ce336507e8f1d9be037a293f666ed0351002dc45b") )
         ( 8450, uint256S("0x00000000197472eaf65f4760d43d1757eb1fb4a4afcb65bb5a2c2102677f16e6"))
    ;


    // TestNet has no checkpoints
    static MapCheckpoints mapCheckpointsTestnet =
            boost::assign::map_list_of
             ( 0,    uint256S("0x000805c5066f3fadac43ddb33819a07e360529d6345a5e60bdc14564cebca5ee") )
        ;



    bool CheckHardened(int nHeight, const uint256& hash)
    {
        MapCheckpoints& checkpoints = (TestNet() ? mapCheckpointsTestnet : mapCheckpoints);

        MapCheckpoints::const_iterator i = checkpoints.find(nHeight);
        if (i == checkpoints.end()) return true;
        return hash == i->second;
    }

    int GetTotalBlocksEstimate()
    {
        MapCheckpoints& checkpoints = (TestNet() ? mapCheckpointsTestnet : mapCheckpoints);

        if (checkpoints.empty())
            return 0;
        return checkpoints.rbegin()->first;
    }

    CBlockIndex* GetLastCheckpoint(const std::map<uint256, CBlockIndex*>& mapBlockIndex)
    {
        MapCheckpoints& checkpoints = (TestNet() ? mapCheckpointsTestnet : mapCheckpoints);

        BOOST_REVERSE_FOREACH(const MapCheckpoints::value_type& i, checkpoints)
        {
            const uint256& hash = i.second;
            std::map<uint256, CBlockIndex*>::const_iterator t = mapBlockIndex.find(hash);
            if (t != mapBlockIndex.end())
                return t->second;
        }
        return NULL;
    }

    // Automatically select a suitable sync-checkpoint
    const CBlockIndex* AutoSelectSyncCheckpoint()
    {
        const CBlockIndex *pindex = pindexBest;
        // Search backward for a block within max span and maturity window
        while (pindex->pprev && pindex->nHeight + nCheckpointSpan > pindexBest->nHeight)
            pindex = pindex->pprev;
        return pindex;
    }

    // Check against synchronized checkpoint
    bool CheckSync(int nHeight)
    {
        const CBlockIndex* pindexSync = AutoSelectSyncCheckpoint();

        if (nHeight <= pindexSync->nHeight)
            return false;
        return true;
    }
}
