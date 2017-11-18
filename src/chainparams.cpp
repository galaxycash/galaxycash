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

        nDefaultPort = 7604;
        nRPCPort = 4604;
        bnProofOfWorkLimit = CBigNum(~uint256(0) >> 20);

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

        CTransaction txNew(1, 1508070915, vin, vout, 0);
        genesis.vtx.push_back(txNew);
        genesis.hashPrevBlock = 0;
        genesis.hashMerkleRoot = genesis.BuildMerkleTree();
        genesis.nVersion = 1;
        genesis.nTime    = 1508070915;
        genesis.nBits    = bnProofOfWorkLimit.GetCompact();
        genesis.nNonce   = 611237;

        hashGenesisBlock = genesis.GetHash();

        // If genesis block hash does not match, then generate new genesis hash.
        if (true)
        {

            std::cout << "Searching for genesis block" << std::endl;

            uint256 target = CBigNum().SetCompact(genesis.nBits).getuint256();
            uint256 hash;
            uint32_t counter = 0;
            uint32_t last = time(NULL);
            while (true)
            {

                hash = genesis.GetHash();
                counter++;

                if (hash <= target)
                    break;


                if ((genesis.nNonce & 0xFFF) == 0)
                    cout << "Nonce: " << genesis.nNonce << " Hash: " << hash.ToString() << " Target: " << target.ToString() << " Hashrate: " << counter << endl;

                uint32_t current = time(NULL);
                if (current - last > 1)
                {
                    counter = 0;
                    last = current;
                }

                ++genesis.nNonce;

                if (genesis.nNonce == 0)
                    ++genesis.nTime;
            }
        }

        std::cout << "Hash: " << genesis.GetHash().ToString() << std::endl;
        std::cout << "Merkle: " << genesis.hashMerkleRoot.ToString() << std::endl;
        std::cout << "Nonce: " << genesis.nNonce << std::endl;

        std::cout << "Max money: " << MAX_MONEY / COIN << std::endl;

        assert(hashGenesisBlock == uint256("0x000000135201f8c8cd8f353511d9a73d463e5f9883cdd430277b962229549f92"));
        assert(genesis.hashMerkleRoot == uint256("0x3081e1468bc0b3162670cfc8635880816fd3b330827b98d9ed99fbc52c89eff7"));

        vSeeds.clear();
        vSeeds.push_back(CDNSSeedData("galaxycash.host", "193.124.0.135"));

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
        // The message start string is designed to be unlikely to occur in normal data.
        // The characters are rarely used upper ASCII, not valid as UTF-8, and produce
        // a large 4-byte int at any alignment.
        pchMessageStart[0] = 0x14;
        pchMessageStart[1] = 0x2a;
        pchMessageStart[2] = 0xe6;
        pchMessageStart[3] = 0x4e;
        bnProofOfWorkLimit = CBigNum(~uint256(0) >> 8);
        nDefaultPort = 17604;
        nRPCPort = 14604;
        strDataDir = "testnet";

        // Modify the testnet genesis block so the timestamp is valid for a later start.
        genesis.nBits  = bnProofOfWorkLimit.GetCompact();
        genesis.nTime  = 1508070915;
        genesis.nNonce = 261;


        // If genesis block hash does not match, then generate new genesis hash.
        if (true)
        {

            std::cout << "Searching for genesis block" << std::endl;

            uint256 target = CBigNum().SetCompact(genesis.nBits).getuint256();
            uint256 hash;
            uint32_t counter = 0;
            uint32_t last = time(NULL);
            while (true)
            {

                hash = genesis.GetHash();
                counter++;

                if (hash <= target)
                    break;


                if ((genesis.nNonce & 0xFFF) == 0)
                    cout << "Nonce: " << genesis.nNonce << " Hash: " << hash.ToString() << " Target: " << target.ToString() << " Hashrate: " << counter << endl;

                uint32_t current = time(NULL);
                if (current - last > 1)
                {
                    counter = 0;
                    last = current;
                }

                ++genesis.nNonce;

                if (genesis.nNonce == 0)
                    ++genesis.nTime;
            }
        }

        std::cout << "Hash: " << genesis.GetHash().ToString() << std::endl;
        std::cout << "Merkle: " << genesis.hashMerkleRoot.ToString() << std::endl;
        std::cout << "Nonce: " << genesis.nNonce << std::endl;

        hashGenesisBlock = genesis.GetHash();
        assert(hashGenesisBlock == uint256("0x0095d5d1dbedc643279dc5f51a034db3f0a000cce4693a6c4109140d50236465"));

        vSeeds.clear();
        vSeeds.push_back(CDNSSeedData("galaxycash.host", "193.124.0.135"));

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,38);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,99);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,89);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x21)(0x88)(0xB2)(0x23).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x21)(0x88)(0x1D)(0x56).convert_to_container<std::vector<unsigned char> >();

        vFixedSeeds.clear();
        convertSeed6(vFixedSeeds, pnSeed6_test, ARRAYLEN(pnSeed6_test));
    }
    virtual Network NetworkID() const { return CChainParams::TESTNET; }
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
