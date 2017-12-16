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
        pchMessageStart[0] = 0x13;
        pchMessageStart[1] = 0x2a;
        pchMessageStart[2] = 0xe6;
        pchMessageStart[3] = 0x4e;

        // Ports
        nDefaultPort = 7604;
        nRPCPort = 4604;

        // POW params
        powLimit = uint256S("00000fffff000000000000000000000000000000000000000000000000000000");
        nPowTargetSpacing = 3 * 60; // 3 minutes
        nPowTargetTimespan = 6 * 60 * 60; // 6 hours

        // Build the genesis block. Note that the output of the genesis coinbase cannot
        // be spent as it did not originally exist in the database.
        //
        const char* pszTimestamp = "15/october/2017 The development of Galaxy Cash started.";

        std::vector<CTxIn> vin;
        vin.resize(1);
        vin[0].scriptSig = CScript() << 0 << CBigNum(42) << vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));

        std::vector<CTxOut> vout;
        vout.resize(1);
        vout[0].SetEmpty();

        CTransaction txNew(1, 1511100000, vin, vout, 0);
        genesis.vtx.push_back(txNew);
        genesis.hashPrevBlock = 0;
        genesis.hashMerkleRoot = genesis.BuildMerkleTree();
        genesis.nVersion = 9;
        genesis.nTime    = 1511208000;
        genesis.nBits    = UintToArith256(powLimit).GetCompact();
        genesis.nNonce   = 1986208;

        hashGenesisBlock = genesis.GetHash();

        assert(hashGenesisBlock == uint256S("0x0000023a0c024e3de098954961530f0491c1b78f1c4fba9e9601673d7bd85b7c"));
        assert(genesis.hashMerkleRoot == uint256S("0x43580fb123f142578ca6c1e6f4ce411b551279e1968a875e6b493ec3da51bdb9"));

        vSeeds.clear();
        vSeeds.push_back(CDNSSeedData("195.133.201.213", "195.133.201.213:7604"));


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

class CTestNetParams : public CMainParams {
public:
    CTestNetParams() {
        strDataDir = "testnet";

        // The message start string is designed to be unlikely to occur in normal data.
        // The characters are rarely used upper ASCII, not valid as UTF-8, and produce
        // a large 4-byte int at any alignment.
        pchMessageStart[0] = 0x14;
        pchMessageStart[1] = 0x2a;
        pchMessageStart[2] = 0xe6;
        pchMessageStart[3] = 0x4e;

        // POW params
        powLimit = uint256S("000fffff00000000000000000000000000000000000000000000000000000000");

        // Ports
        nDefaultPort = 17604;
        nRPCPort = 14604;


        // Modify the testnet genesis block so the timestamp is valid for a later start.
        genesis.nBits  = UintToArith256(powLimit).GetCompact();
        genesis.nTime  = 1511208000;
        genesis.nNonce = 4500;

        hashGenesisBlock = genesis.GetHash();

        assert(hashGenesisBlock == uint256S("0x000805c5066f3fadac43ddb33819a07e360529d6345a5e60bdc14564cebca5ee"));

        vSeeds.clear();
        vSeeds.push_back(CDNSSeedData("galaxycash.host", "127.0.0.1"));

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,38);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,99);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,89);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x21)(0x88)(0xB2)(0x23).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x21)(0x88)(0x1D)(0x56).convert_to_container<std::vector<unsigned char> >();


        vFixedSeeds.clear();
        convertSeed6(vFixedSeeds, pnSeed6_test, ARRAYLEN(pnSeed6_test));
    }
    virtual Network NetworkID() const { return CChainParams::TESTNET; }
    virtual string NetworkIDString() const { return "main"; }
};
static CTestNetParams testNetParams;




static CChainParams *pCurrentParams = &mainParams;

const CChainParams &Params() {
    return *pCurrentParams;
}

void SelectParams(CChainParams::Network network) {
    switch (network) {
        case CChainParams::MAIN:
            pCurrentParams = &mainParams;
            break;
        case CChainParams::TESTNET:
            pCurrentParams = &testNetParams;
            break;

        default:
            assert(false && "Unimplemented network");
            return;
    }
}

bool SelectParamsFromCommandLine() {

    bool fTestNet = GetBoolArg("-testnet", false);



    if (fTestNet) {
        SelectParams(CChainParams::TESTNET);
    } else {
        SelectParams(CChainParams::MAIN);
    }
    return true;
}
