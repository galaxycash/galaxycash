
// Copyright (c) 2009-2012 The Dash developers
// Copyright (c) 2017-2018 The GalaxyCash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef SPORK_H
#define SPORK_H

#include "base58.h"
#include "key.h"
#include "net.h"
#include "sync.h"
#include "util.h"
#include "validation.h"

#include "masternode.h"
#include "protocol.h"
#include <boost/lexical_cast.hpp>

using namespace std;
using namespace boost;

#define GETSPORKS_TIME 100000

#define SPORK_0_GALAXYCASH_TRANSACTIONS_ACCEPT 0
#define SPORK_DEFAULT_0_GALAXYCASH_TRANSACTIONS_ACCEPT 0
#define SPORK_1_MASTERNODE_REWARD_REJECTION 1
#define SPORK_DEFAULT_1_MASTERNODE_REWARD_REJECTION 650000

class CSporkMessage;
class CSporkManager;

extern std::map<uint256, CSporkMessage> mapSporks;
extern std::map<int, CSporkMessage> mapSporksActive;
extern CSporkManager sporkManager;
void DumpSporks();

void ProcessSpork(CNode* pfrom, std::string& strCommand, CDataStream& vRecv);
int64_t GetSporkValue(int nSporkID);
bool IsSporkActive(int nSporkID);
void ExecuteSpork(int nSporkID, int nValue);
void ReprocessBlocks(int nBlocks);

//
// Spork Class
// Keeps track of all of the network spork settings
//

class CSporkMessage
{
public:
    std::vector<unsigned char> vchSig;
    int nSporkID;
    int64_t nValue;
    int64_t nTimeSigned;

    uint256 GetHash()
    {
        uint256 n = HashX12(BEGIN(nSporkID), END(nTimeSigned));
        return n;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(nSporkID);
        READWRITE(nValue);
        READWRITE(nTimeSigned);
        READWRITE(vchSig);
    }
};


class CSporkManager
{
private:
    std::vector<unsigned char> vchSig;
public:
    CSporkManager()
    {
    }

    void Init();

    std::string GetSporkNameByID(int id);
    int GetSporkIDByName(std::string strName);
    bool UpdateSpork(int nSporkID, int64_t nValue);
    bool CheckSignature(CSporkMessage& spork);
    bool Sign(CSporkMessage& spork);
    void Relay(CSporkMessage& msg);
};

void ThreadSporks();

#endif
