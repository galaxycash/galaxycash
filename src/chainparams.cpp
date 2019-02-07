// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Copyright (c) 2017 The GalaxyCash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "assert.h"

#include "chainparams.h"
#include "main.h"
#include "util.h"
#include "base58.h"

#include <boost/assign/list_of.hpp>

using namespace boost::assign;

struct SeedSpec6 {
    uint8_t addr[16];
    uint16_t port;
};

#include "chainparamsseeds.h"

//
// Main network
//

// Convert the pnSeeds6 array into usable address objects.
static void convertSeed6(std::vector<CAddress> &vSeedsOut, const SeedSpec6 *data, unsigned int count)
{
    // It'll only connect to one or two seed nodes because once it connects,
    // it'll get a pile of addresses with newer timestamps.
    // Seed nodes are given a random 'last seen time' of between one and two
    // weeks ago.
    const int64_t nOneWeek = 7*24*60*60;
    for (unsigned int i = 0; i < count; i++)
    {
        struct in6_addr ip;
        memcpy(&ip, data[i].addr, sizeof(ip));
        CAddress addr(CService(ip, data[i].port));
        addr.nTime = GetTime() - GetRand(nOneWeek) - nOneWeek;
        vSeedsOut.push_back(addr);
    }
}

class CMainParams : public CChainParams {
public:
    CMainParams() {

        // The message start string is designed to be unlikely to occur in normal data.
        // The characters are rarely used upper ASCII, not valid as UTF-8, and produce
        // a large 4-byte int at any alignment.
        pchMessageStart[0] = 0x4e;
        pchMessageStart[1] = 0xe6;
        pchMessageStart[2] = 0xe6;
        pchMessageStart[3] = 0x4e;

        // Subsidy halvings
        nSubsidyHalvingInterval = 210000;

        // POS
        stakeLimit = uint256S("00000fffff000000000000000000000000000000000000000000000000000000");
        nPOSFirstBlock = 61300;

        // Merge
        nMergeFirstBlock = nCoinbaseMaturity + 2;
        nMergeLastBlock = 95;

        // Last PoW block
        nLastPowBlock = 130000;

        // Ports
        nDefaultPort = 7604;
        nRPCPort = 4604;

        // POW params
        powLimit = uint256S("00000fffff000000000000000000000000000000000000000000000000000000");
        nPowTargetSpacing = 3 * 60; // 3 minutes
        nPowTargetSpacing2 = 10 * 60; // 10 minutes
        nPowTargetSpacingPOS = 2 * 60; // 2 minutes
        nPowTargetTimespan = 6 * 60 * 60; // 6 hours
        nPowTargetTimespan2 = 24 * 60 * 60; // 24 hours
        nPowTargetTimespanPOS = 6 * 60 * 60; // 6 hours
        fPOWNoRetargeting = false;

        // Build the genesis block. Note that the output of the genesis coinbase cannot
        // be spent as it did not originally exist in the database.
        //
        const char* pszTimestamp = "15/october/2017 The development of Galaxy Cash started.";

        std::vector<CTxIn> vin;
        vin.resize(1);
        vin[0].scriptSig = CScript() << 0 << CScriptNum(42) << vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));

        std::vector<CTxOut> vout;
        vout.resize(1);
        vout[0].SetEmpty();

        CTransaction txNew(1, 1515086697, vin, vout, 0);
        genesis.vtx.push_back(txNew);
        genesis.hashPrevBlock = 0;
        genesis.hashMerkleRoot = genesis.BuildMerkleTree();
        genesis.nVersion = 9;
        genesis.nTime    = 1515086697;
        genesis.nBits    = UintToArith256(powLimit).GetCompact();
        genesis.nNonce   = 1303736;

        hashGenesisBlock = genesis.GetHash();

        assert(hashGenesisBlock == uint256S("0x00000076b947553b6888ca82875e04a4db21fd904aae46589e1d183b63327468"));
        assert(genesis.hashMerkleRoot == uint256S("0xa3df636e1166133b477fad35d677e81ab93f9c9d242bcdd0e9955c9982615915"));

        vSeeds.clear();
        vSeeds.push_back(CDNSSeedData("187.37.74.22:7604","187.37.74.22:7604"));
        vSeeds.push_back(CDNSSeedData("68.5.50.43:7604","68.5.50.43:7604"));
        vSeeds.push_back(CDNSSeedData("165.227.118.61:7604","165.227.118.61:7604"));
        vSeeds.push_back(CDNSSeedData("147.135.130.119","147.135.130.119:7604"));
        vSeeds.push_back(CDNSSeedData("178.233.151.114","178.233.151.114:7604"));
        vSeeds.push_back(CDNSSeedData("134.255.233.54","134.255.233.54:7604"));
        vSeeds.push_back(CDNSSeedData("104.156.251.36","104.156.251.36:7604"));
        vSeeds.push_back(CDNSSeedData("45.77.27.194","45.77.27.194:7604"));
        vSeeds.push_back(CDNSSeedData("185.249.198.208","185.249.198.208:7604"));
        vSeeds.push_back(CDNSSeedData("[2604:2d80:4018:8e2a::2]","[2604:2d80:4018:8e2a::2]:7604"));
        vSeeds.push_back(CDNSSeedData("77.57.73.31","77.57.73.31:7604"));
        vSeeds.push_back(CDNSSeedData("[2001:0:9d38:6ab8:1c66:2f9b:a435:d3c4]","[2001:0:9d38:6ab8:1c66:2f9b:a435:d3c4]:7604"));
        vSeeds.push_back(CDNSSeedData("[2001:0:9d38:78cf:38df:3382:a865:5ae3]","[2001:0:9d38:78cf:38df:3382:a865:5ae3]:7604"));
        vSeeds.push_back(CDNSSeedData("[2001:41d0:303:2777::]","[2001:41d0:303:2777::]:7604"));
        vSeeds.push_back(CDNSSeedData("77.87.206.82","77.87.206.82:7604"));
        vSeeds.push_back(CDNSSeedData("[2001:0:4137:9e76:1400:39b6:85ee:f1e6]","[2001:0:4137:9e76:1400:39b6:85ee:f1e6]:7604"));
        vSeeds.push_back(CDNSSeedData("[2001:0:9d38:953c:1c83:1d32:a72b:c808]","[2001:0:9d38:953c:1c83:1d32:a72b:c808]:7604"));
        vSeeds.push_back(CDNSSeedData("[2001:0:9d38:90d7:c9a:131f:82e7:9c90]","[2001:0:9d38:90d7:c9a:131f:82e7:9c90]:7604"));
        vSeeds.push_back(CDNSSeedData("[2001:0:4137:9e76:349f:1278:ab1d:e4d3]","[2001:0:4137:9e76:349f:1278:ab1d:e4d3]:7604"));


        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,38);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,99);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,89);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x1D)(0x88)(0xB2)(0x23).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x1D)(0x88)(0x2D)(0x56).convert_to_container<std::vector<unsigned char> >();

        vFixedSeeds.clear();
        convertSeed6(vFixedSeeds, pnSeed6_main, ARRAYLEN(pnSeed6_main));
    }

    virtual const CBlock& GenesisBlock() const { return genesis; }
    virtual Network NetworkID() const { return CChainParams::MAIN; }
    virtual string NetworkIDString() const { return "main"; }

    virtual const vector<CAddress>& FixedSeeds() const {
        return vFixedSeeds;
    }
protected:
    CBlock genesis;
    vector<CAddress> vFixedSeeds;
};
static CMainParams mainParams;


//
// Testnet
//

class CTestParams : public CMainParams {
public:
    CTestParams() {
        strDataDir = "test";

        // The message start string is designed to be unlikely to occur in normal data.
        // The characters are rarely used upper ASCII, not valid as UTF-8, and produce
        // a large 4-byte int at any alignment.
        pchMessageStart[0] = 0xe6;
        pchMessageStart[1] = 0x2a;
        pchMessageStart[2] = 0xe6;
        pchMessageStart[3] = 0x4e;

        // POW
        fPOWNoRetargeting = GetBoolArg("-pownoretargeting", true);

        // Merge params
        nMergeFirstBlock = nMergeLastBlock = 0;

        nSubsidyHalvingInterval = 100;

        // POS params
        nPOSFirstBlock = 0;

        // Ports
        nDefaultPort = 17604;
        nRPCPort = 14604;

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,41);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,51);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,61);
    }
    virtual Network NetworkID() const { return CChainParams::TEST; }
    virtual string NetworkIDString() const { return "test"; }
};
static CTestParams testParams;

//
// Testnet
//

class CEasyParams : public CMainParams {
public:
    CEasyParams() {
        strDataDir = "easy";

        // The message start string is designed to be unlikely to occur in normal data.
        // The characters are rarely used upper ASCII, not valid as UTF-8, and produce
        // a large 4-byte int at any alignment.
        pchMessageStart[0] = 0x2a;
        pchMessageStart[1] = 0x2a;
        pchMessageStart[2] = 0xe6;
        pchMessageStart[3] = 0x4e;

        // Merge params
        nMergeFirstBlock = nMergeLastBlock = 0;

        // Subsidy halvings
        nSubsidyHalvingInterval = 380;

        // POS params
        nPOSFirstBlock = 0;

        // POW params
        fPOWNoRetargeting = true;

        // Ports
        nDefaultPort = 18604;
        nRPCPort = 15604;

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,40);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,50);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,60);
    }
    virtual Network NetworkID() const { return CChainParams::EASY; }
    virtual string NetworkIDString() const { return "easy"; }
};
static CEasyParams easyParams;


static CChainParams *pCurrentParams = &mainParams;

const CChainParams &Params() {
    return *pCurrentParams;
}

void SelectParams(CChainParams::Network network) {
    switch (network) {
        case CChainParams::MAIN:
            pCurrentParams = &mainParams;
            break;
        case CChainParams::EASY:
            pCurrentParams = &easyParams;
        break;
        case CChainParams::TEST:
            pCurrentParams = &testParams;
            break;

        default:
            assert(false && "Unimplemented network");
            return;
    }
}

bool SelectParamsFromCommandLine() {

    bool fTestNet = GetBoolArg("-testnet", false);
    bool fEasyNet = GetBoolArg("-easynet", false);


    if (fTestNet)
        SelectParams(CChainParams::TEST);
    else if (fEasyNet)
        SelectParams(CChainParams::EASY);
    else
        SelectParams(CChainParams::MAIN);

    return true;
}

