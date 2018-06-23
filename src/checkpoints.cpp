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
         ( 0,       uint256S("0x00000076b947553b6888ca82875e04a4db21fd904aae46589e1d183b63327468") )
         ( 95,      uint256S("0x00000068c53527393133ecff50618a5f148b84ba445a022575194286b51b46a5") )
         ( 200,     uint256S("0x0000000a1269edde5420ab128c39d392721e1ea8efae8749d83e6fc6162fb2b0") )
         ( 19000,   uint256S("0x000000000129c4ea27ef6cdb884d84dd6533e711157590d4d92b896f07e5c572") )
         ( 44413,   uint256S("0x00000000000923b0f3b55d59c982cafca653cc44ca3217edef74adc6e8fd131a") )
         ( 61300,   uint256S("0x00000000000005b40b79df9c151ccc9dbcad70cb5305873e448d1aeef0769684") )
         ( 61330,   uint256S("0x0000000000000390028c01e6764244a152b9dcccd7c6c46f25e0f4a292b3077a") )
         ( 61332,   uint256S("0x0000000000c399296696942b4de87bbbc318eb864cd50be4bdb011fa4173d5d4") )
         ( 61333,   uint256S("0x000000000121e2c83189c0c03e6a826290c328a09024c31be58ceb17aba6c6ec") )
         ( 61400,   uint256S("0x0000000000001b39ed27b2f5bc22698bcc82b4415eb68a7fb9b583fceea735cb") )
         ( 61482,   uint256S("0x5d45c5ce20ea1d077f5023f61ba608f964015ee48fadeaa13d05f55b015f6773") )
    ;
    static MapCheckpoints mapTestCheckpoints =
        boost::assign::map_list_of
         ( 0,    uint256S("0x00000076b947553b6888ca82875e04a4db21fd904aae46589e1d183b63327468") )
    ;


    bool CheckHardened(int nHeight, const uint256& hash)
    {
        MapCheckpoints& checkpoints = TestNet() ? mapTestCheckpoints : mapCheckpoints;

        MapCheckpoints::const_iterator i = checkpoints.find(nHeight);
        if (i == checkpoints.end()) return true;
        return hash == i->second;
    }

    int GetTotalBlocksEstimate()
    {
        MapCheckpoints& checkpoints = TestNet() ? mapTestCheckpoints : mapCheckpoints;

        if (checkpoints.empty())
            return 0;
        return checkpoints.rbegin()->first;
    }

    CBlockIndex* GetLastCheckpoint(const std::map<uint256, CBlockIndex*>& mapBlockIndex)
    {
        MapCheckpoints& checkpoints = TestNet() ? mapTestCheckpoints : mapCheckpoints;

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

        const int nSync = std::max(0, pindexSync->nHeight - (int)(GetArg("-maxreorganize", int64_t(100))));
        if (nHeight < nSync)
            return false;
        return true;
    }
}

