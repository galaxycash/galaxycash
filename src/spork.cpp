// Copyright (c) 2009-2012 The Dash developers
// Copyright (c) 2017-2018 The GalaxyCash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "spork.h"
#include "base58.h"
#include "chainparams.h"
#include "key.h"
#include "masternode.h"
#include "net.h"
#include "net_processing.h"
#include "netmessagemaker.h"
#include "protocol.h"
#include "pubkey.h"
#include "sync.h"
#include "util.h"
#include "validation.h"
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

using namespace std;
using namespace boost;

class CSporkMessage;
class CSporkManager;

CSporkManager sporkManager;

std::map<uint256, CSporkMessage> mapSporks;
std::map<int, CSporkMessage> mapSporksActive;


void ProcessSpork(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{
    if (strCommand == "spork") {
        //LogPrintf("ProcessSpork::spork\n");
        CDataStream vMsg(vRecv);
        CSporkMessage spork;
        vRecv >> spork;

        if (chainActive.Tip() == NULL) return;

        uint256 hash = spork.GetHash();
        if (mapSporksActive.count(spork.nSporkID)) {
            if (mapSporksActive[spork.nSporkID].nTimeSigned >= spork.nTimeSigned) {
                LogPrint(BCLog::SPORK, "spork - seen %s block %d \n", hash.ToString(), chainActive.Tip()->nHeight);
                return;
            } else {
                LogPrint(BCLog::SPORK, "spork - got updated spork %s block %d \n", hash.ToString(), chainActive.Tip()->nHeight);
            }
        }

        LogPrintf("spork - new %s ID %d Time %d bestHeight %d\n", hash.ToString(), spork.nSporkID, spork.nValue, chainActive.Tip()->nHeight);

        if (!sporkManager.CheckSignature(spork)) {
            LogPrintf("spork - invalid signature\n");
            Misbehaving(pfrom->GetId(), 100);
            return;
        }

        mapSporks[hash] = spork;
        mapSporksActive[spork.nSporkID] = spork;
        sporkManager.Relay(spork);

        //does a task if needed
        ExecuteSpork(spork.nSporkID, spork.nValue);
    }
    if (strCommand == "sporks") {
        //LogPrintf("ProcessSpork::spork\n");
        CDataStream vMsg(vRecv);
        std::vector<CSporkMessage> sporks;
        vRecv >> sporks;

        if (chainActive.Tip() == NULL) return;

        for (int i = 0; i < sporks.size(); i++) {
            CSporkMessage spork = sporks[i];

            uint256 hash = spork.GetHash();
            if (mapSporksActive.count(spork.nSporkID)) {
                if (mapSporksActive[spork.nSporkID].nTimeSigned >= spork.nTimeSigned) {
                    LogPrint(BCLog::SPORK, "spork - seen %s block %d \n", hash.ToString(), chainActive.Tip()->nHeight);
                    return;
                } else {
                    LogPrint(BCLog::SPORK, "spork - got updated spork %s block %d \n", hash.ToString(), chainActive.Tip()->nHeight);
                }
            }

            LogPrintf("spork - new %s ID %d Time %d bestHeight %d\n", hash.ToString(), spork.nSporkID, spork.nValue, chainActive.Tip()->nHeight);

            if (!sporkManager.CheckSignature(spork)) {
                LogPrintf("spork - invalid signature\n");
                Misbehaving(pfrom->GetId(), 100);
                return;
            }

            mapSporks[hash] = spork;
            mapSporksActive[spork.nSporkID] = spork;
            sporkManager.Relay(spork);

            //does a task if needed
            ExecuteSpork(spork.nSporkID, spork.nValue);
        }
    }
    if (strCommand == "getsporks") {
        std::vector<CSporkMessage> sporks;
        for (std::map<int, CSporkMessage>::iterator it = mapSporksActive.begin(); it != mapSporksActive.end(); it++) {
            sporks.push_back((*it).second);
        }

        g_connman->PushMessage(pfrom, CNetMsgMaker(pfrom->GetRecvVersion()).Make(NetMsgType::SPORKS, sporks));
    }
}

// grab the spork, otherwise say it's off
bool IsSporkActive(int nSporkID)
{
    int64_t r = -1;

    if (mapSporksActive.count(nSporkID)) {
        r = mapSporksActive[nSporkID].nValue;
    } else {
        if (nSporkID == SPORK_0_GALAXYCASH_TRANSACTIONS_ACCEPT)
            r = SPORK_DEFAULT_0_GALAXYCASH_TRANSACTIONS_ACCEPT;
        else if (nSporkID == SPORK_1_MASTERNODE_REWARD_REJECTION)
            r = SPORK_DEFAULT_1_MASTERNODE_REWARD_REJECTION;

        if (r == -1) LogPrintf("GetSpork::Unknown Spork %d\n", nSporkID);
    }
    if (r == -1) r = 0; //return 2099-1-1 by default

    return r < GetTime();
}

// grab the value of the spork on the network, or the default
int64_t GetSporkValue(int nSporkID)
{
    int64_t r = -1;

    if (mapSporksActive.count(nSporkID)) {
        r = mapSporksActive[nSporkID].nValue;
    } else {
        if (nSporkID == SPORK_0_GALAXYCASH_TRANSACTIONS_ACCEPT) r = SPORK_DEFAULT_0_GALAXYCASH_TRANSACTIONS_ACCEPT;
        if (nSporkID == SPORK_1_MASTERNODE_REWARD_REJECTION) r = SPORK_DEFAULT_1_MASTERNODE_REWARD_REJECTION;

        if (r == -1) LogPrintf("GetSpork::Unknown Spork %d\n", nSporkID);
    }

    return r;
}

void ExecuteSpork(int nSporkID, int nValue)
{
}

void ReprocessBlocks(int nBlocks)
{
}


void CSporkManager::Init()
{
    fs::path path(GetDataDir() / "sporks.dat");
    FILE* file = fopen(path.string().c_str(), "rb");
    CAutoFile filein(file, SER_DISK, CLIENT_VERSION);
    if (filein.IsNull()) {
        error("%s : Failed to open file %s", __func__, path.string());
        return;
    }

    filein >> mapSporks;
    filein >> mapSporksActive;
    filein.fclose();
}

void CSporkManager::Dump()
{
    fs::path path(GetDataDir() / "sporks.dat");
    FILE* file = fopen(path.string().c_str(), "wb");
    CAutoFile fileout(file, SER_DISK, CLIENT_VERSION);
    if (fileout.IsNull()) {
        error("%s : Failed to open file %s", __func__, path.string());
        return;
    }

    fileout << mapSporks;
    fileout << mapSporksActive;
    fileout.fclose();
}

bool CSporkManager::CheckSignature(CSporkMessage& spork)
{
    const CPubKey& pubkey = Params().DevPubKey();
    return pubkey.VerifyCompact(spork.GetHash(), spork.vchSig);
}

bool CSporkManager::Sign(CSporkMessage& spork)
{
    const CKey& key = Params().DevKey();
    return key.SignCompact(spork.GetHash(), spork.vchSig);
}

bool CSporkManager::UpdateSpork(int nSporkID, int64_t nValue)
{
    CSporkMessage msg;
    msg.nSporkID = nSporkID;
    msg.nValue = nValue;
    msg.nTimeSigned = GetTime();

    if (Sign(msg)) {
        Relay(msg);
        mapSporks[msg.GetHash()] = msg;
        mapSporksActive[nSporkID] = msg;
        return true;
    }

    return false;
}

void CSporkManager::Relay(CSporkMessage& msg)
{
    CInv inv(MSG_SPORK, msg.GetHash());
    RelayInv(inv);
}


int CSporkManager::GetSporkIDByName(std::string strName)
{
    if (strName == "SPORK_0_GALAXYCASH_TRANSACTIONS_ACCEPT") return SPORK_0_GALAXYCASH_TRANSACTIONS_ACCEPT;
    if (strName == "SPORK_1_MASTERNODE_REWARD_REJECTION") return SPORK_1_MASTERNODE_REWARD_REJECTION;

    return -1;
}

std::string CSporkManager::GetSporkNameByID(int id)
{
    if (id == SPORK_0_GALAXYCASH_TRANSACTIONS_ACCEPT) return "SPORK_0_GALAXYCASH_TRANSACTIONS_ACCEPT";
    if (id == SPORK_1_MASTERNODE_REWARD_REJECTION) return "SPORK_1_MASTERNODE_REWARD_REJECTION";

    return "Unknown";
}

void ThreadSporks()
{
    // Make this thread recognisable as the wallet flushing thread
    RenameThread("galaxycash-sporks");

    int64_t lastRequest = 0;

    while (true) {
        MilliSleep(1000);

        int64_t time = GetTime();
        if (lastRequest < time) {
            LOCK(cs_main);
            
            g_connman->ForEachNode([=](CNode* pnode) {
                g_connman->PushMessage(pnode, CNetMsgMaker(pnode->GetRecvVersion()).Make(NetMsgType::GETSPORKS));
            });

            lastRequest = time + GETSPORKS_TIME;
        }
    }
}