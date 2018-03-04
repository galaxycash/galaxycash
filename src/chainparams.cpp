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
        nPOSFirstBlock = 100000;

        // Merge
        nMergeFirstBlock = nCoinbaseMaturity + 2;
        nMergeLastBlock = 95;

        // Ports
        nDefaultPort = 7604;
        nRPCPort = 4604;

        // POW params
        powLimit = uint256S("00000fffff000000000000000000000000000000000000000000000000000000");
        nPowTargetSpacing = 3 * 60; // 3 minutes
        nPowTargetSpacing2 = 10 * 60; // 10 minutes
        nPowTargetTimespan = 6 * 60 * 60; // 6 hours
        nPowTargetTimespan2 = 24 * 60 * 60; // 24 hours
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

        nPoolMaxTransactions = 3;
        strAnonsendPoolDummyAddress = "GMY5kZMSgtJs22VDiQKtYMYcapy4FKaBVu";

        assert(hashGenesisBlock == uint256S("0x00000076b947553b6888ca82875e04a4db21fd904aae46589e1d183b63327468"));
        assert(genesis.hashMerkleRoot == uint256S("0xa3df636e1166133b477fad35d677e81ab93f9c9d242bcdd0e9955c9982615915"));

        vSeeds.clear();
        vSeeds.push_back(CDNSSeedData("galaxycash.main", "195.133.201.213"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node1", "195.133.201.213:7688"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node2", "128.69.4.143"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node3", "210.211.124.162"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node4", "37.21.134.25"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node5", "73.213.209.183"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node6", "74.103.154.124"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node7", "45.77.31.169"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node8", "104.155.55.140"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node9", "35.229.115.102"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node10", "35.199.170.116"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node11", "35.224.227.214"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node12", "73.140.224.25"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node13", "179.214.188.108"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node14", "159.224.5.79"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node15", "188.220.237.254"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node16", "68.235.38.177"));

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

