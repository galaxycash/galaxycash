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

        nPoolMaxTransactions = 3;
        strAnonsendPoolDummyAddress = "GMY5kZMSgtJs22VDiQKtYMYcapy4FKaBVu";

        assert(hashGenesisBlock == uint256S("0x00000076b947553b6888ca82875e04a4db21fd904aae46589e1d183b63327468"));
        assert(genesis.hashMerkleRoot == uint256S("0xa3df636e1166133b477fad35d677e81ab93f9c9d242bcdd0e9955c9982615915"));

        vSeeds.clear();
        vSeeds.push_back(CDNSSeedData("galaxycash.node1","195.133.201.213:7604"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node3","195.133.147.242:9992"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node4","45.77.154.137:7604"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node5","212.237.11.136:7604"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node6","45.77.31.169:7604"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node7","91.92.136.58:7604"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node8","59.18.162.74:7604"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node9","81.169.150.193:7688"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node10","212.47.227.141:7688"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node11","144.76.173.249:60027"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node12","91.206.32.84:7604"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node13","188.187.188.194:7604"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node14","139.59.91.27:7604"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node15","159.65.165.97:7604"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node16","185.239.237.167:7604"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node17","185.5.107.48:7604"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node18","212.86.115.36:7604"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node19","195.133.201.213:7604"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node20","35.231.244.218:7604"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node21","185.235.130.31:7604"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node22","139.59.91.27:7606"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node23","195.201.20.101:7604"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node24","159.65.172.65:0"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node25","95.24.49.1:7604"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node26","194.67.193.47:7604"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node27","185.217.241.65:7604"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node28","108.16.35.40:7604"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node29","78.107.6.148:7604"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node30","77.57.73.31:7616"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node31","86.57.179.254:7604"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node32","185.223.28.72:7604"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node33","198.13.34.110:7604"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node34","192.158.231.104:7604"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node35","8.6.8.226:7604"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node36","93.76.10.56:7604"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node37","134.255.233.54:7604"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node38","103.84.143.107:7604"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node39","185.213.211.60:7604"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node40","95.179.133.119:7604"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node41","191.184.202.19:7604"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node42","159.224.5.79:7604"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node43","220.120.3.61:7605"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node44","115.178.253.3:7604"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node45","94.251.76.178:7777"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node46","134.255.219.135:7604"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node47","185.249.198.208:7604"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node48","134.255.252.192:7604"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node49","178.143.23.148:7604"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node50","185.250.205.213:7604"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node51","189.73.206.145:7604"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node52","107.191.39.190:7604"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node53","188.134.86.234:3333"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node54","103.27.237.206:7604"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node55","14.1.201.105:7604"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node56","149.28.131.17:7604"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node57","77.57.73.31:7604"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node58","80.211.25.21:7604"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node59","192.3.166.133:7604"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node60","59.18.162.74:7605"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node61","59.18.162.74:7606"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node62","220.120.3.61:7604"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node63","220.120.3.61:7606"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node64","220.120.3.61:7607"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node65","220.120.3.61:7608"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node66","220.120.3.61:7609"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node67","220.120.3.61:7610"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node68","220.120.3.61:7611"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node69","220.120.3.61:7612"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node70","220.120.3.61:7613"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node71","220.120.3.61:7614"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node72","220.120.3.61:7615"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node73","220.120.3.61:7616"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node74","220.120.3.61:7617"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node75","220.120.3.61:7618"));
        vSeeds.push_back(CDNSSeedData("galaxycash.node76","220.120.3.61:7619"));


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

