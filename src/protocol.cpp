// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <protocol.h>

#include <util.h>
#include <utilstrencodings.h>

#ifndef WIN32
#include <arpa/inet.h>
#endif

namespace NetMsgType
{
const char* VERSION = "version";
const char* VERACK = "verack";
const char* ADDR = "addr";
const char* INV = "inv";
const char* GETDATA = "getdata";
const char* MERKLEBLOCK = "merkleblock";
const char* GETBLOCKS = "getblocks";
const char* GETHEADERS = "getheaders";
const char* TX = "tx";
const char* HEADERS = "headers";
const char* BLOCK = "block";
const char* GETSPORKS = "getsporks";
const char* GETSPORK = "getspork";
const char* SPORK = "spork";
const char* SPORKS = "sporks";
const char* MASTERNODE_WINNER = "mnw";
const char* MASTERNODE_SCANNING_ERROR = "mnse";
const char* BUDGET_VOTE = "mn-budget-vote";
const char* BUDGET_PROPOSAL = "mn-budget-proposal";
const char* BUDGET_FINALIZED = "mn-budget-finalized";
const char* BUDGET_FINALIZED_VOTE = "mn-budget-finalized";
const char* MASTERNODE_QUORUM = "mn-quorum";
const char* MASTERNODE_ANNOUNCE = "mn-announce";
const char* MASTERNODE_PING = "mn-ping";
const char* GETADDR = "getaddr";
const char* MEMPOOL = "mempool";
const char* PING = "ping";
const char* PONG = "pong";
const char* ALERT = "alert";
const char* NOTFOUND = "notfound";
const char* FILTERLOAD = "filterload";
const char* FILTERADD = "filteradd";
const char* FILTERCLEAR = "filterclear";
const char* REJECT = "reject";
const char* SENDHEADERS = "sendheaders";
const char* FEEFILTER = "feefilter";
const char* CHECKPOINT = "checkpoint";
const char* DSEE = "dsee";
const char* MNWINNER = "masternode winner";
const char* FILTEREDBLOCK = "filtered block";
const char* GETBLOCKTXN = "getblocktxn";
const char* BLOCKTXN = "blocktxn";
} // namespace NetMsgType

/** All known message types. Keep this in the same order as the list of
 * messages above and in protocol.h.
 */
const static std::string allNetMessageTypes[] = {
    NetMsgType::VERSION,
    NetMsgType::VERACK,
    NetMsgType::ADDR,
    NetMsgType::INV,
    NetMsgType::GETDATA,
    NetMsgType::MERKLEBLOCK,
    NetMsgType::GETBLOCKS,
    NetMsgType::GETHEADERS,
    NetMsgType::GETSPORKS,
    NetMsgType::TX,
    NetMsgType::HEADERS,
    NetMsgType::BLOCK,
    NetMsgType::SPORK,
    NetMsgType::SPORKS,
    NetMsgType::MASTERNODE_WINNER,
    NetMsgType::MASTERNODE_SCANNING_ERROR,
    NetMsgType::BUDGET_VOTE,
    NetMsgType::BUDGET_PROPOSAL,
    NetMsgType::BUDGET_FINALIZED,
    NetMsgType::BUDGET_FINALIZED_VOTE,
    NetMsgType::MASTERNODE_QUORUM,
    NetMsgType::MASTERNODE_QUORUM,
    NetMsgType::MASTERNODE_ANNOUNCE,
    NetMsgType::MASTERNODE_PING,
    NetMsgType::GETADDR,
    NetMsgType::MEMPOOL,
    NetMsgType::PING,
    NetMsgType::PONG,
    NetMsgType::ALERT,
    NetMsgType::NOTFOUND,
    NetMsgType::FILTERLOAD,
    NetMsgType::FILTERADD,
    NetMsgType::FILTERCLEAR,
    NetMsgType::REJECT,
    NetMsgType::SENDHEADERS,
    NetMsgType::FEEFILTER,
    NetMsgType::CHECKPOINT,
    NetMsgType::FILTEREDBLOCK,
    NetMsgType::BLOCKTXN,
    NetMsgType::GETBLOCKTXN};

const static std::vector<std::string> allNetMessageTypesVec(allNetMessageTypes, allNetMessageTypes + ARRAYLEN(allNetMessageTypes));

CMessageHeader::CMessageHeader(const MessageStartChars& pchMessageStartIn)
{
    memcpy(pchMessageStart, pchMessageStartIn, MESSAGE_START_SIZE);
    memset(pchCommand, 0, sizeof(pchCommand));
    nMessageSize = -1;
    memset(pchChecksum, 0, CHECKSUM_SIZE);
}

CMessageHeader::CMessageHeader(const MessageStartChars& pchMessageStartIn, const char* pszCommand, unsigned int nMessageSizeIn)
{
    memcpy(pchMessageStart, pchMessageStartIn, MESSAGE_START_SIZE);
    memset(pchCommand, 0, sizeof(pchCommand));
    strncpy(pchCommand, pszCommand, COMMAND_SIZE);
    nMessageSize = nMessageSizeIn;
    memset(pchChecksum, 0, CHECKSUM_SIZE);
}

std::string CMessageHeader::GetCommand() const
{
    return std::string(pchCommand, pchCommand + strnlen(pchCommand, COMMAND_SIZE));
}

bool CMessageHeader::IsValid(const MessageStartChars& pchMessageStartIn) const
{
    // Check start string
    if (memcmp(pchMessageStart, pchMessageStartIn, MESSAGE_START_SIZE) != 0)
        return false;

    // Check the command string for errors
    for (const char* p1 = pchCommand; p1 < pchCommand + COMMAND_SIZE; p1++) {
        if (*p1 == 0) {
            // Must be all zeros after the first zero
            for (; p1 < pchCommand + COMMAND_SIZE; p1++)
                if (*p1 != 0)
                    return false;
        } else if (*p1 < ' ' || *p1 > 0x7E)
            return false;
    }

    // Message size
    if (nMessageSize > MAX_SIZE) {
        LogPrintf("CMessageHeader::IsValid(): (%s, %u bytes) nMessageSize > MAX_SIZE\n", GetCommand(), nMessageSize);
        return false;
    }

    return true;
}


CAddress::CAddress() : CService()
{
    Init();
}

CAddress::CAddress(CService ipIn, ServiceFlags nServicesIn) : CService(ipIn)
{
    Init();
    nServices = nServicesIn;
}

void CAddress::Init()
{
    nServices = NODE_NONE;
    nTime = 100000000;
}

CInv::CInv()
{
    type = 0;
    hash.SetNull();
}

CInv::CInv(int typeIn, const uint256& hashIn) : type(typeIn), hash(hashIn) {}

bool operator<(const CInv& a, const CInv& b)
{
    return (a.type < b.type || (a.type == b.type && a.hash < b.hash));
}

std::string CInv::GetCommand() const
{
    switch (type) {
    case MSG_TX:
        return NetMsgType::TX;
        break;
    case MSG_BLOCK:
        return NetMsgType::BLOCK;
        break;
    case MSG_FILTERED_BLOCK:
        return NetMsgType::MERKLEBLOCK;
        break;
    case MSG_SPORK:
        return NetMsgType::SPORK;
        break;
    case MSG_SPORKS:
        return NetMsgType::SPORKS;
        break;
    case MSG_MASTERNODE_WINNER:
        return NetMsgType::MASTERNODE_WINNER;
        break;
    case MSG_MASTERNODE_SCANNING_ERROR:
        return NetMsgType::MASTERNODE_SCANNING_ERROR;
        break;
    case MSG_MASTERNODE_QUORUM:
        return NetMsgType::MASTERNODE_QUORUM;
        break;
    case MSG_MASTERNODE_ANNOUNCE:
        return NetMsgType::MASTERNODE_ANNOUNCE;
        break;
    case MSG_MASTERNODE_PING:
        return NetMsgType::MASTERNODE_PING;
        break;
    }
}

std::string CInv::ToString() const
{
    try {
        return strprintf("%s %s", GetCommand(), hash.ToString());
    } catch (const std::out_of_range&) {
        return strprintf("0x%08x %s", type, hash.ToString());
    }
}

const std::vector<std::string>& getAllNetMessageTypes()
{
    return allNetMessageTypesVec;
}

const unsigned int POW_HEADER_COOLING = 70;

