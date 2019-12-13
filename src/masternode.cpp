// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2015-2019 The GalaxyCash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include <addrman.h>
#include <alert.h>
#include <arith_uint256.h>
#include <base58.h>
#include <blockencodings.h>
#include <chainparams.h>
#include <consensus/validation.h>
#include <hash.h>
#include <init.h>
#include <merkleblock.h>
#include <net_processing.h>
#include <netaddress.h>
#include <netbase.h>
#include <netmessagemaker.h>
#include <policy/policy.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <random.h>
#include <reverse_iterator.h>
#include <scheduler.h>
#include <tinyformat.h>
#include <txmempool.h>
#include <ui_interface.h>
#include <util.h>
#include <utilmoneystr.h>
#include <utilstrencodings.h>
#include <validation.h>
#include <wallet/wallet.h>


#include <memory>


#include "chain.h"
#include "coins.h"
#include "masternode.h"
#include "protocol.h"
#include "txdb.h"

#include <boost/filesystem.hpp>


bool fMasterNode = false;
std::string strMasterNodePrivKey = "";
std::string strMasterNodeAddr = "";
int64_t enforceMasternodePaymentsTime = 4085657524;
std::string strVoteMode = "";
CActiveMasternode activeMasternode;

int64_t GetMasternodePayment(int nHeight, int64_t blockValue, int nMasternodeCount)
{
    return blockValue * 0.95;
}

static bool GetTransaction2(const uint256& hash, CTransactionRef& tx, uint256 *h = nullptr)
{
    uint256 hashBlock;
    bool fOk = ::GetTransaction(hash, tx, Params().GetConsensus(), hashBlock, true, NULL);
    if (h) *h = hashBlock;
    return fOk;
}


static bool IsVinAssociatedWithPubkey(CTxIn& vin, CPubKey& pubkey)
{
    CScript payee2 = GetScriptForDestination(pubkey.GetID());

    CTransactionRef txVin;
    if (GetTransaction2(vin.prevout.hash, txVin)) {
        for (CTxOut out : txVin->vout) {
            if (out.nValue == MASTERNODE_COLLATERAL) {
                if (out.scriptPubKey == payee2) return true;
            }
        }
    }

    return false;
}
/// Set the private/public key values, returns true if successful
static bool GetKeysFromSecret(const std::string& strSecret, CKey& keyRet, CPubKey& pubkeyRet)
{
    CBitcoinSecret vchSecret;

    if(!vchSecret.SetString(strSecret)) return false;

    keyRet = vchSecret.GetKey();
    pubkeyRet = keyRet.GetPubKey();

    return true;
}


/// Set the private/public key values, returns true if successful
static bool SetKey(std::string strSecret, std::string& errorMessage, CKey& key, CPubKey& pubkey)
{
    CBitcoinSecret vchSecret;
    bool fGood = vchSecret.SetString(strSecret);

    if (!fGood) {
        errorMessage = _("Invalid private key.");
        return false;
    }

    key = vchSecret.GetKey();
    pubkey = key.GetPubKey();

    return true;
}

bool MasternodeSetKey(std::string strSecret, std::string& errorMessage, CKey& key, CPubKey& pubkey)
{
    return ::SetKey(strSecret, errorMessage, key, pubkey);
}

/// Sign the message, returns true if successful
static bool SignMessage(std::string strMessage, std::string& errorMessage, std::vector<unsigned char>& vchSig, CKey key)
{
    CHashWriter ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << strMessage;

    if (!key.SignCompact(ss.GetHash(), vchSig)) {
        errorMessage = _("Signing failed.");
        return false;
    }

    return true;
}
/// Verify the message, returns true if succcessful
static bool VerifyMessage(CPubKey pubkey, std::vector<unsigned char>& vchSig, std::string strMessage, std::string& errorMessage)
{
    CHashWriter ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << strMessage;

    CPubKey pubkey2;
    if (!pubkey2.RecoverCompact(ss.GetHash(), vchSig)) {
        errorMessage = _("Error recovering public key.");
        return false;
    }

    if (pubkey2.GetID() != pubkey.GetID())
        LogPrintf("CObfuScationSigner::VerifyMessage -- keys don't match: %s %s\n", pubkey2.GetID().ToString(), pubkey.GetID().ToString());

    return (pubkey2.GetID() == pubkey.GetID());
}


static bool GetVinAndKeysFromOutput(COutput out, CTxIn& txinRet, CPubKey& pubKeyRet, CKey& keyRet)
{
    // wait for reindex and/or import to finish
    if (fImporting || fReindex) return false;

    CTxDestination dest;
    CInputCoin coin(out.tx, out.i);
    txinRet = CTxIn(coin.outpoint.hash, coin.outpoint.n);
    ExtractDestination(coin.txout.scriptPubKey, dest);


    bool fOk = false;
    for (CWalletRef pwallet : vpwallets) {
        CKeyID id = GetKeyForDestination(*pwallet, dest);
        if (pwallet->HaveKey(id)) {
            pwallet->GetKey(id, keyRet);
            fOk = true;
        }
    }

    if (!fOk) {
        LogPrintf("CWallet::GetVinAndKeysFromOutput -- Private key for address is not known\n");
        return false;
    }

    pubKeyRet = keyRet.GetPubKey();
    return true;
}

static bool GetMasternodeVinAndKeys(CTxIn& txinRet, CPubKey& pubKeyRet, CKey& keyRet, std::string strTxHash, std::string strOutputIndex)
{
    // wait for reindex and/or import to finish
    if (fImporting || fReindex) return false;

    int64_t collateral = MASTERNODE_COLLATERAL;
    for (CWalletRef pwallet : vpwallets) {
        // Find possible candidates
        std::vector<COutput> vPossibleCoins;
        pwallet->AvailableCoins(vPossibleCoins, true, NULL, collateral, collateral);
        if (vPossibleCoins.empty()) {
            LogPrintf("CWallet::GetMasternodeVinAndKeys -- Could not locate any valid masternode vin\n");
            continue;
        }

        if (strTxHash.empty())
            return GetVinAndKeysFromOutput(vPossibleCoins[0], txinRet, pubKeyRet, keyRet);
        else {
            // Find specific vin
            uint256 txHash = uint256S(strTxHash);

            int nOutputIndex;
            try {
                nOutputIndex = std::stoi(strOutputIndex.c_str());
            } catch (const std::exception& e) {
                LogPrintf("%s: %s on strOutputIndex\n", __func__, e.what());
            }

            for (COutput& out : vPossibleCoins)
                if (out.tx->GetHash() == txHash && out.i == nOutputIndex) // found it!
                    return GetVinAndKeysFromOutput(out, txinRet, pubKeyRet, keyRet);

            LogPrintf("CWallet::GetMasternodeVinAndKeys -- Could not locate specified masternode vin\n");
        }
    }
}


CMasternodeConfig masternodeConfig;

CMasternodeConfig::CMasternodeEntry* CMasternodeConfig::add(std::string alias, std::string ip, std::string privKey, std::string txHash, std::string outputIndex)
{
    CMasternodeEntry cme(alias, ip, privKey, txHash, outputIndex);
    entries.push_back(cme);
    return &(entries[entries.size() - 1]);
}

void CMasternodeConfig::remove(std::string alias)
{
    int pos = -1;
    for (int i = 0; i < ((int)entries.size()); ++i) {
        CMasternodeEntry e = entries[i];
        if (e.getAlias() == alias) {
            pos = i;
            break;
        }
    }
    entries.erase(entries.begin() + pos);
}

bool CMasternodeConfig::read(std::string& strErr)
{
    int linenumber = 1;
    boost::filesystem::path pathMasternodeConfigFile = GetMasternodeConfigFile();
    boost::filesystem::ifstream streamConfig(pathMasternodeConfigFile);

    if (!streamConfig.good()) {
        FILE* configFile = fopen(pathMasternodeConfigFile.string().c_str(), "a");
        if (configFile != NULL) {
            std::string strHeader = "# Masternode config file\n"
                                    "# Format: alias IP:port masternodeprivkey collateral_output_txid collateral_output_index\n"
                                    "# Example: mn1 127.0.0.2:7604 93HaYBVUCYjEMeeH1Y4sBGLALQZE1Yc1K64xiqgX37tGBDQL8Xg 2bcd3c84c84f87eaa86e4e56834c92927a07f9e18718810b92e0d0324456a67c 0"
                                    "#\n";
            fwrite(strHeader.c_str(), std::strlen(strHeader.c_str()), 1, configFile);
            fclose(configFile);
        }
        return true; // Nothing to read, so just return
    }

    for (std::string line; std::getline(streamConfig, line); linenumber++) {
        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string comment, alias, ip, privKey, txHash, outputIndex;

        if (iss >> comment) {
            if (comment.at(0) == '#') continue;
            iss.str(line);
            iss.clear();
        }

        if (!(iss >> alias >> ip >> privKey >> txHash >> outputIndex)) {
            iss.str(line);
            iss.clear();
            if (!(iss >> alias >> ip >> privKey >> txHash >> outputIndex)) {
                strErr = _("Could not parse masternode.conf") + "\n" +
                         strprintf(_("Line: %d"), linenumber) + "\n\"" + line + "\"";
                streamConfig.close();
                return false;
            }
        }

        int port = 0;
        std::string hostname = "";
        SplitHostPort(ip, port, hostname);
        if (port == 0 || hostname == "") {
            strErr = _("Failed to parse host:port string") + "\n" +
                     strprintf(_("Line: %d"), linenumber) + "\n\"" + line + "\"";
            streamConfig.close();
            return false;
        }

        if (Params().NetworkIDString() == CBaseChainParams::MAIN) {
            if (port != Params().GetDefaultPort()) {
                strErr = _("Invalid port detected in masternode.conf") + "\n" +
                         strprintf(_("Line: %d"), linenumber) + "\n\"" + line + "\"" + "\n" +
                         _("(must be 7604 for mainnet)");
                streamConfig.close();
                return false;
            }
        } else if (port == Params().GetDefaultPort()) {
            strErr = _("Invalid port detected in masternode.conf") + "\n" +
                     strprintf(_("Line: %d"), linenumber) + "\n\"" + line + "\"" + "\n" +
                     _("(7604 could be used only on mainnet)");
            streamConfig.close();
            return false;
        }


        add(alias, ip, privKey, txHash, outputIndex);
    }

    streamConfig.close();
    return true;
}

bool CMasternodeConfig::CMasternodeEntry::castOutputIndex(int& n)
{
    try {
        n = std::stoi(outputIndex);
    } catch (const std::exception& e) {
        LogPrintf("%s: %s on getOutputIndex\n", __func__, e.what());
        return false;
    }

    return true;
}


// keep track of the scanning errors I've seen
std::map<uint256, int> mapSeenMasternodeScanningErrors;
// cache block hashes as we calculate them
std::map<int64_t, uint256> mapCacheBlockHashes;

//Get the last hash that matches the modulus given. Processed in reverse order
bool GetBlockHash(uint256& hash, int nBlockHeight)
{
    if (chainActive.Tip() == NULL) return false;

    if (nBlockHeight == 0)
        nBlockHeight = chainActive.Tip()->nHeight;

    if (mapCacheBlockHashes.count(nBlockHeight)) {
        hash = mapCacheBlockHashes[nBlockHeight];
        return true;
    }

    const CBlockIndex* BlockLastSolved = chainActive.Tip();
    const CBlockIndex* BlockReading = chainActive.Tip();

    if (BlockLastSolved == NULL || BlockLastSolved->nHeight == 0 || chainActive.Tip()->nHeight + 1 < nBlockHeight) return false;

    int nBlocksAgo = 0;
    if (nBlockHeight > 0) nBlocksAgo = (chainActive.Tip()->nHeight + 1) - nBlockHeight;
    assert(nBlocksAgo >= 0);

    int n = 0;
    for (unsigned int i = 1; BlockReading && BlockReading->nHeight > 0; i++) {
        if (n >= nBlocksAgo) {
            hash = BlockReading->GetBlockHash();
            mapCacheBlockHashes[nBlockHeight] = hash;
            return true;
        }
        n++;

        if (BlockReading->pprev == NULL) {
            assert(BlockReading);
            break;
        }
        BlockReading = BlockReading->pprev;
    }

    return false;
}

CMasternode::CMasternode()
{
    LOCK(cs);
    vin = CTxIn();
    addr = CService();
    pubKeyCollateralAddress = CPubKey();
    pubKeyMasternode = CPubKey();
    sig = std::vector<unsigned char>();
    activeState = MASTERNODE_ENABLED;
    sigTime = GetAdjustedTime();
    lastPing = CMasternodePing();
    cacheInputAge = 0;
    cacheInputAgeBlock = 0;
    unitTest = false;
    allowFreeTx = true;
    nActiveState = MASTERNODE_ENABLED,
    protocolVersion = PROTOCOL_VERSION;
    nLastDsq = 0;
    nScanningErrorCount = 0;
    nLastScanningErrorBlockHeight = 0;
    lastTimeChecked = 0;
    nLastDsee = 0;  // temporary, do not save. Remove after migration to v12
    nLastDseep = 0; // temporary, do not save. Remove after migration to v12
}

CMasternode::CMasternode(const CMasternode& other)
{
    LOCK(cs);
    vin = other.vin;
    addr = other.addr;
    pubKeyCollateralAddress = other.pubKeyCollateralAddress;
    pubKeyMasternode = other.pubKeyMasternode;
    sig = other.sig;
    activeState = other.activeState;
    sigTime = other.sigTime;
    lastPing = other.lastPing;
    cacheInputAge = other.cacheInputAge;
    cacheInputAgeBlock = other.cacheInputAgeBlock;
    unitTest = other.unitTest;
    allowFreeTx = other.allowFreeTx;
    nActiveState = MASTERNODE_ENABLED,
    protocolVersion = other.protocolVersion;
    nLastDsq = other.nLastDsq;
    nScanningErrorCount = other.nScanningErrorCount;
    nLastScanningErrorBlockHeight = other.nLastScanningErrorBlockHeight;
    lastTimeChecked = 0;
    nLastDsee = other.nLastDsee;   // temporary, do not save. Remove after migration to v12
    nLastDseep = other.nLastDseep; // temporary, do not save. Remove after migration to v12
}

CMasternode::CMasternode(const CMasternodeBroadcast& mnb)
{
    LOCK(cs);
    vin = mnb.vin;
    addr = mnb.addr;
    pubKeyCollateralAddress = mnb.pubKeyCollateralAddress;
    pubKeyMasternode = mnb.pubKeyMasternode;
    sig = mnb.sig;
    activeState = MASTERNODE_ENABLED;
    sigTime = mnb.sigTime;
    lastPing = mnb.lastPing;
    cacheInputAge = 0;
    cacheInputAgeBlock = 0;
    unitTest = false;
    allowFreeTx = true;
    nActiveState = MASTERNODE_ENABLED,
    protocolVersion = mnb.protocolVersion;
    nLastDsq = mnb.nLastDsq;
    nScanningErrorCount = 0;
    nLastScanningErrorBlockHeight = 0;
    lastTimeChecked = 0;
    nLastDsee = 0;  // temporary, do not save. Remove after migration to v12
    nLastDseep = 0; // temporary, do not save. Remove after migration to v12
}

//
// When a new masternode broadcast is sent, update our information
//
bool CMasternode::UpdateFromNewBroadcast(CMasternodeBroadcast& mnb)
{
    if (mnb.sigTime > sigTime) {
        pubKeyMasternode = mnb.pubKeyMasternode;
        pubKeyCollateralAddress = mnb.pubKeyCollateralAddress;
        sigTime = mnb.sigTime;
        sig = mnb.sig;
        protocolVersion = mnb.protocolVersion;
        addr = mnb.addr;
        lastTimeChecked = 0;
        int nDoS = 0;
        if (mnb.lastPing == CMasternodePing() || (mnb.lastPing != CMasternodePing() && mnb.lastPing.CheckAndUpdate(nDoS, false))) {
            lastPing = mnb.lastPing;
            mnodeman.mapSeenMasternodePing.insert(std::make_pair(lastPing.GetHash(), lastPing));
        }
        return true;
    }
    return false;
}

//
// Deterministically calculate a given "score" for a Masternode depending on how close it's hash is to
// the proof of work for that block. The further away they are the better, the furthest will win the election
// and get paid this block
//
uint256 CMasternode::CalculateScore(int mod, int64_t nBlockHeight)
{
    if (chainActive.Tip() == NULL) return 0;

    uint256 hash = 0;
    uint256 aux = vin.prevout.hash + vin.prevout.n;

    if (!GetBlockHash(hash, nBlockHeight)) {
        LogPrint(BCLog::MASTERNODE, "CalculateScore ERROR - nHeight %d - Returned 0\n", nBlockHeight);
        return 0;
    }

    CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
    ss << hash;
    uint256 hash2 = ss.GetHash();

    CHashWriter ss2(SER_GETHASH, PROTOCOL_VERSION);
    ss2 << hash;
    ss2 << aux;
    uint256 hash3 = ss2.GetHash();

    uint256 r = (hash3 > hash2 ? hash3 - hash2 : hash2 - hash3);

    return r;
}

void CMasternode::Check(bool forceCheck)
{
    if (ShutdownRequested()) return;

    if (!forceCheck && (GetTime() - lastTimeChecked < MASTERNODE_CHECK_SECONDS)) return;
    lastTimeChecked = GetTime();


    //once spent, stop doing the checks
    if (activeState == MASTERNODE_VIN_SPENT) return;


    if (!IsPingedWithin(MASTERNODE_REMOVAL_SECONDS)) {
        activeState = MASTERNODE_REMOVE;
        return;
    }

    if (!IsPingedWithin(MASTERNODE_EXPIRATION_SECONDS)) {
        activeState = MASTERNODE_EXPIRED;
        return;
    }

    if (lastPing.sigTime - sigTime < MASTERNODE_MIN_MNP_SECONDS) {
        activeState = MASTERNODE_PRE_ENABLED;
        return;
    }

    if (!unitTest) {
        CValidationState state;
        CMutableTransaction tx = CMutableTransaction();
        std::string devAddr = "GL83ZiVZ26z3stMtrF91WJ5f77q6EnKXnC";
        CBitcoinAddress gdevAddr;
        gdevAddr.SetString(devAddr);
        CTxOut vout = CTxOut(MASTERNODE_COLLATERAL - 1, GetScriptForDestination(gdevAddr.Get()));
        tx.vin.push_back(vin);
        tx.vout.push_back(vout);

        {
            TRY_LOCK(cs_main, lockMain);
            if (!lockMain) return;

            if (!AcceptableInputs(mempool, state, CTransaction(tx), false, NULL)) {
                activeState = MASTERNODE_VIN_SPENT;
                return;
            }
        }
    }

    activeState = MASTERNODE_ENABLED; // OK
}

int64_t CMasternode::SecondsSincePayment()
{
    CScript pubkeyScript;
    pubkeyScript = GetScriptForDestination(pubKeyCollateralAddress.GetID());

    int64_t sec = (GetAdjustedTime() - GetLastPaid());
    int64_t month = 60 * 60 * 24 * 30;
    if (sec < month) return sec; //if it's less than 30 days, give seconds

    CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
    ss << vin;
    ss << sigTime;
    uint256 hash = ss.GetHash();

    // return some deterministic value for unknown/unpaid but force it to be more than 30 days old
    return month + hash.GetCompact(false);
}

int64_t CMasternode::GetLastPaid()
{
    CBlockIndex* pindexPrev = chainActive.Tip();
    if (pindexPrev == NULL) return false;

    CScript mnpayee;
    mnpayee = GetScriptForDestination(pubKeyCollateralAddress.GetID());

    CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
    ss << vin;
    ss << sigTime;
    uint256 hash = ss.GetHash();

    // use a deterministic offset to break a tie -- 2.5 minutes
    int64_t nOffset = hash.GetCompact(false) % 150;

    if (chainActive.Tip() == NULL) return false;

    const CBlockIndex* BlockReading = chainActive.Tip();

    int nMnCount = mnodeman.CountEnabled() * 1.25;
    int n = 0;
    for (unsigned int i = 1; BlockReading && BlockReading->nHeight > 0; i++) {
        if (n >= nMnCount) {
            return 0;
        }
        n++;

        if (masternodePayments.mapMasternodeBlocks.count(BlockReading->nHeight)) {
            /*
                Search for this payee, with at least 2 votes. This will aid in consensus allowing the network
                to converge on the same payees quickly, then keep the same schedule.
            */
            if (masternodePayments.mapMasternodeBlocks[BlockReading->nHeight].HasPayeeWithVotes(mnpayee, 2)) {
                return BlockReading->nTime + nOffset;
            }
        }

        if (BlockReading->pprev == NULL) {
            assert(BlockReading);
            break;
        }
        BlockReading = BlockReading->pprev;
    }

    return 0;
}

std::string CMasternode::GetStatus()
{
    switch (nActiveState) {
    case CMasternode::MASTERNODE_PRE_ENABLED:
        return "PRE_ENABLED";
    case CMasternode::MASTERNODE_ENABLED:
        return "ENABLED";
    case CMasternode::MASTERNODE_EXPIRED:
        return "EXPIRED";
    case CMasternode::MASTERNODE_OUTPOINT_SPENT:
        return "OUTPOINT_SPENT";
    case CMasternode::MASTERNODE_REMOVE:
        return "REMOVE";
    case CMasternode::MASTERNODE_WATCHDOG_EXPIRED:
        return "WATCHDOG_EXPIRED";
    case CMasternode::MASTERNODE_POSE_BAN:
        return "POSE_BAN";
    case CMasternode::MASTERNODE_MISSING:
        return "MISSING";
    default:
        return "UNKNOWN";
    }
}

bool CMasternode::IsValidNetAddr()
{
    // TODO: regtest is fine with any addresses for now,
    // should probably be a bit smarter if one day we start to implement tests for this
    return Params().NetworkIDString() == CBaseChainParams::REGTEST ||
           (IsReachable(addr) && addr.IsRoutable());
}

CMasternodeBroadcast::CMasternodeBroadcast()
{
    vin = CTxIn();
    addr = CService();
    pubKeyCollateralAddress = CPubKey();
    pubKeyMasternode1 = CPubKey();
    sig = std::vector<unsigned char>();
    activeState = MASTERNODE_ENABLED;
    sigTime = GetAdjustedTime();
    lastPing = CMasternodePing();
    cacheInputAge = 0;
    cacheInputAgeBlock = 0;
    unitTest = false;
    allowFreeTx = true;
    protocolVersion = PROTOCOL_VERSION;
    nLastDsq = 0;
    nScanningErrorCount = 0;
    nLastScanningErrorBlockHeight = 0;
}

CMasternodeBroadcast::CMasternodeBroadcast(CService newAddr, CTxIn newVin, CPubKey pubKeyCollateralAddressNew, CPubKey pubKeyMasternodeNew, int protocolVersionIn)
{
    vin = newVin;
    addr = newAddr;
    pubKeyCollateralAddress = pubKeyCollateralAddressNew;
    pubKeyMasternode = pubKeyMasternodeNew;
    sig = std::vector<unsigned char>();
    activeState = MASTERNODE_ENABLED;
    sigTime = GetAdjustedTime();
    lastPing = CMasternodePing();
    cacheInputAge = 0;
    cacheInputAgeBlock = 0;
    unitTest = false;
    allowFreeTx = true;
    protocolVersion = protocolVersionIn;
    nLastDsq = 0;
    nScanningErrorCount = 0;
    nLastScanningErrorBlockHeight = 0;
}

CMasternodeBroadcast::CMasternodeBroadcast(const CMasternode& mn)
{
    vin = mn.vin;
    addr = mn.addr;
    pubKeyCollateralAddress = mn.pubKeyCollateralAddress;
    pubKeyMasternode = mn.pubKeyMasternode;
    sig = mn.sig;
    activeState = mn.activeState;
    sigTime = mn.sigTime;
    lastPing = mn.lastPing;
    cacheInputAge = mn.cacheInputAge;
    cacheInputAgeBlock = mn.cacheInputAgeBlock;
    unitTest = mn.unitTest;
    allowFreeTx = mn.allowFreeTx;
    protocolVersion = mn.protocolVersion;
    nLastDsq = mn.nLastDsq;
    nScanningErrorCount = mn.nScanningErrorCount;
    nLastScanningErrorBlockHeight = mn.nLastScanningErrorBlockHeight;
}

bool CMasternodeBroadcast::Create(std::string strService, std::string strKeyMasternode, std::string strTxHash, std::string strOutputIndex, std::string& strErrorRet, CMasternodeBroadcast& mnbRet, bool fOffline)
{
    CTxIn txin;
    CPubKey pubKeyCollateralAddressNew;
    CKey keyCollateralAddressNew;
    CPubKey pubKeyMasternodeNew;
    CKey keyMasternodeNew;

    //need correct blocks to send ping
    if (!fOffline && !masternodeSync.IsBlockchainSynced()) {
        strErrorRet = "Sync in progress. Must wait until sync is complete to start Masternode";
        LogPrint(BCLog::MASTERNODE, "CMasternodeBroadcast::Create -- %s\n", strErrorRet);
        return false;
    }

    if (!GetKeysFromSecret(strKeyMasternode, keyMasternodeNew, pubKeyMasternodeNew)) {
        strErrorRet = strprintf("Invalid masternode key %s", strKeyMasternode);
        LogPrint(BCLog::MASTERNODE, "CMasternodeBroadcast::Create -- %s\n", strErrorRet);
        return false;
    }

    if (!GetMasternodeVinAndKeys(txin, pubKeyCollateralAddressNew, keyCollateralAddressNew, strTxHash, strOutputIndex)) {
        strErrorRet = strprintf("Could not allocate txin %s:%s for masternode %s", strTxHash, strOutputIndex, strService);
        LogPrint(BCLog::MASTERNODE, "CMasternodeBroadcast::Create -- %s\n", strErrorRet);
        return false;
    }

    // The service needs the correct default port to work properly
    if (!CheckDefaultPort(strService, strErrorRet, "CMasternodeBroadcast::Create"))
        return false;

    return Create(txin, CService(strService), keyCollateralAddressNew, pubKeyCollateralAddressNew, keyMasternodeNew, pubKeyMasternodeNew, strErrorRet, mnbRet);
}

bool CMasternodeBroadcast::Create(CTxIn txin, CService service, CKey keyCollateralAddressNew, CPubKey pubKeyCollateralAddressNew, CKey keyMasternodeNew, CPubKey pubKeyMasternodeNew, std::string& strErrorRet, CMasternodeBroadcast& mnbRet)
{
    // wait for reindex and/or import to finish
    if (fImporting || fReindex) return false;

    LogPrint(BCLog::MASTERNODE, "CMasternodeBroadcast::Create -- pubKeyCollateralAddressNew = %s, pubKeyMasternodeNew.GetID() = %s\n",
        CBitcoinAddress(pubKeyCollateralAddressNew.GetID()).ToString(),
        pubKeyMasternodeNew.GetID().ToString());

    CMasternodePing mnp(txin);
    if (!mnp.Sign(keyMasternodeNew, pubKeyMasternodeNew)) {
        strErrorRet = strprintf("Failed to sign ping, masternode=%s", txin.prevout.hash.ToString());
        LogPrint(BCLog::MASTERNODE, "CMasternodeBroadcast::Create -- %s\n", strErrorRet);
        mnbRet = CMasternodeBroadcast();
        return false;
    }

    mnbRet = CMasternodeBroadcast(service, txin, pubKeyCollateralAddressNew, pubKeyMasternodeNew, PROTOCOL_VERSION);

    if (!mnbRet.IsValidNetAddr()) {
        strErrorRet = strprintf("Invalid IP address %s, masternode=%s", mnbRet.addr.ToStringIP(), txin.prevout.hash.ToString());
        LogPrint(BCLog::MASTERNODE, "CMasternodeBroadcast::Create -- %s\n", strErrorRet);
        mnbRet = CMasternodeBroadcast();
        return false;
    }

    mnbRet.lastPing = mnp;
    if (!mnbRet.Sign(keyCollateralAddressNew)) {
        strErrorRet = strprintf("Failed to sign broadcast, masternode=%s", txin.prevout.hash.ToString());
        LogPrint(BCLog::MASTERNODE, "CMasternodeBroadcast::Create -- %s\n", strErrorRet);
        mnbRet = CMasternodeBroadcast();
        return false;
    }

    return true;
}

bool CMasternodeBroadcast::CheckDefaultPort(std::string strService, std::string& strErrorRet, std::string strContext)
{
    CService service = CService(strService);
    int nDefaultPort = Params().GetDefaultPort();

    if (service.GetPort() != nDefaultPort) {
        strErrorRet = strprintf("Invalid port %u for masternode %s, only %d is supported on %s-net.",
            service.GetPort(), strService, nDefaultPort, Params().NetworkIDString());
        LogPrint(BCLog::MASTERNODE, "%s - %s\n", strContext, strErrorRet);
        return false;
    }

    return true;
}

bool CMasternodeBroadcast::CheckAndUpdate(int& nDos)
{
    // make sure signature isn't in the future (past is OK)
    if (sigTime > GetAdjustedTime() + 60 * 60) {
        LogPrint(BCLog::MASTERNODE, "mnb - Signature rejected, too far into the future %s\n", vin.prevout.hash.ToString());
        nDos = 1;
        return false;
    }

    // incorrect ping or its sigTime
    if (lastPing == CMasternodePing() || !lastPing.CheckAndUpdate(nDos, false, true))
        return false;

    if (protocolVersion < masternodePayments.GetMinMasternodePaymentsProto()) {
        LogPrint(BCLog::MASTERNODE, "mnb - ignoring outdated Masternode %s protocol version %d\n", vin.prevout.hash.ToString(), protocolVersion);
        return false;
    }

    CScript pubkeyScript;
    pubkeyScript = GetScriptForDestination(pubKeyCollateralAddress.GetID());

    if (pubkeyScript.size() != 25) {
        LogPrint(BCLog::MASTERNODE, "mnb - pubkey the wrong size\n");
        nDos = 100;
        return false;
    }

    CScript pubkeyScript2;
    pubkeyScript2 = GetScriptForDestination(pubKeyMasternode.GetID());

    if (pubkeyScript2.size() != 25) {
        LogPrint(BCLog::MASTERNODE, "mnb - pubkey2 the wrong size\n");
        nDos = 100;
        return false;
    }

    if (!vin.scriptSig.empty()) {
        LogPrint(BCLog::MASTERNODE, "mnb - Ignore Not Empty ScriptSig %s\n", vin.prevout.hash.ToString());
        return false;
    }

    std::string errorMessage = "";
    if (!VerifyMessage(pubKeyCollateralAddress, sig, GetStrMessage(), errorMessage)) {
        // don't ban for old masternodes, their sigs could be broken because of the bug
        nDos = protocolVersion < MIN_PEER_MNANNOUNCE ? 0 : 100;
        return error("CMasternodeBroadcast::CheckAndUpdate - Got bad Masternode address signature : %s", errorMessage);
    }

    if (Params().NetworkIDString() == CBaseChainParams::MAIN) {
        if (addr.GetPort() != Params().GetDefaultPort()) return false;
    } else if (addr.GetPort() == Params().GetDefaultPort())
        return false;

    //search existing Masternode list, this is where we update existing Masternodes with new mnb broadcasts
    CMasternode* pmn = mnodeman.Find(vin);

    // no such masternode, nothing to update
    if (pmn == NULL) return true;

    // this broadcast is older or equal than the one that we already have - it's bad and should never happen
    // unless someone is doing something fishy
    // (mapSeenMasternodeBroadcast in CMasternodeMan::ProcessMessage should filter legit duplicates)
    if (pmn->sigTime >= sigTime) {
        return error("CMasternodeBroadcast::CheckAndUpdate - Bad sigTime %d for Masternode %20s %105s (existing broadcast is at %d)",
            sigTime, addr.ToString(), vin.ToString(), pmn->sigTime);
    }

    // masternode is not enabled yet/already, nothing to update
    if (!pmn->IsEnabled()) return true;

    // mn.pubkey = pubkey, IsVinAssociatedWithPubkey is validated once below,
    //   after that they just need to match
    if (pmn->pubKeyCollateralAddress == pubKeyCollateralAddress && !pmn->IsBroadcastedWithin(MASTERNODE_MIN_MNB_SECONDS)) {
        //take the newest entry
        LogPrint(BCLog::MASTERNODE, "mnb - Got updated entry for %s\n", vin.prevout.hash.ToString());
        if (pmn->UpdateFromNewBroadcast((*this))) {
            pmn->Check();
            if (pmn->IsEnabled()) Relay();
        }
        masternodeSync.AddedMasternodeList(GetHash());
    }

    return true;
}

bool CMasternodeBroadcast::CheckInputsAndAdd(int& nDoS)
{
    // we are a masternode with the same vin (i.e. already activated) and this mnb is ours (matches our Masternode privkey)
    // so nothing to do here for us
    if (fMasterNode && vin.prevout == activeMasternode.vin.prevout && pubKeyMasternode == activeMasternode.pubKeyMasternode)
        return true;

    // incorrect ping or its sigTime
    if (lastPing == CMasternodePing() || !lastPing.CheckAndUpdate(nDoS, false, true)) return false;

    // search existing Masternode list
    CMasternode* pmn = mnodeman.Find(vin);

    if (pmn != NULL) {
        // nothing to do here if we already know about this masternode and it's enabled
        if (pmn->IsEnabled()) return true;
        // if it's not enabled, remove old MN first and continue
        else
            mnodeman.Remove(pmn->vin);
    }

    CValidationState state;
    CMutableTransaction tx = CMutableTransaction();
    std::string devAddr = "GL83ZiVZ26z3stMtrF91WJ5f77q6EnKXnC";
    CBitcoinAddress gdevAddr;
    gdevAddr.SetString(devAddr);
    CTxOut vout = CTxOut(MASTERNODE_COLLATERAL - 1, GetScriptForDestination(gdevAddr.Get()));
    tx.vin.push_back(vin);
    tx.vout.push_back(vout);

    {
        TRY_LOCK(cs_main, lockMain);
        if (!lockMain) {
            // not mnb fault, let it to be checked again later
            mnodeman.mapSeenMasternodeBroadcast.erase(GetHash());
            masternodeSync.mapSeenSyncMNB.erase(GetHash());
            return false;
        }

        if (!AcceptableInputs(mempool, state, CTransaction(tx), false, NULL)) {
            //set nDos
            state.IsInvalid(nDoS);
            return false;
        }
    }

    LogPrint(BCLog::MASTERNODE, "mnb - Accepted Masternode entry\n");

    if (GetInputAge(vin) < MASTERNODE_MIN_CONFIRMATIONS) {
        LogPrint(BCLog::MASTERNODE, "mnb - Input must have at least %d confirmations\n", MASTERNODE_MIN_CONFIRMATIONS);
        // maybe we miss few blocks, let this mnb to be checked again later
        mnodeman.mapSeenMasternodeBroadcast.erase(GetHash());
        masternodeSync.mapSeenSyncMNB.erase(GetHash());
        return false;
    }

    // verify that sig time is legit in past
    // should be at least not earlier than block when 1000 PIV tx got MASTERNODE_MIN_CONFIRMATIONS
    uint256 hashBlock = 0;
    CTransactionRef tx2;
    GetTransaction2(vin.prevout.hash, tx2, &hashBlock);
    BlockMap::iterator mi = mapBlockIndex.find(hashBlock);
    if (mi != mapBlockIndex.end() && (*mi).second) {
        CBlockIndex* pMNIndex = (*mi).second;                                                        // block for 1000 GalaxyCash tx -> 1 confirmation
        CBlockIndex* pConfIndex = chainActive[pMNIndex->nHeight + MASTERNODE_MIN_CONFIRMATIONS - 1]; // block where tx got MASTERNODE_MIN_CONFIRMATIONS
        if (pConfIndex->GetBlockTime() > sigTime) {
            LogPrint(BCLog::MASTERNODE, "mnb - Bad sigTime %d for Masternode %s (%i conf block is at %d)\n",
                sigTime, vin.prevout.hash.ToString(), MASTERNODE_MIN_CONFIRMATIONS, pConfIndex->GetBlockTime());
            return false;
        }
    }

    LogPrint(BCLog::MASTERNODE, "mnb - Got NEW Masternode entry - %s - %lli \n", vin.prevout.hash.ToString(), sigTime);
    CMasternode mn(*this);
    mnodeman.Add(mn);

    // if it matches our Masternode privkey, then we've been remotely activated
    if (pubKeyMasternode == activeMasternode.pubKeyMasternode && protocolVersion == PROTOCOL_VERSION) {
        activeMasternode.EnableHotColdMasterNode(vin, addr);
    }

    bool isLocal = addr.IsRFC1918() || addr.IsLocal();
    if (Params().NetworkIDString() == CBaseChainParams::REGTEST) isLocal = false;

    if (!isLocal) Relay();

    return true;
}

void CMasternodeBroadcast::Relay()
{
    CInv inv(MSG_MASTERNODE_ANNOUNCE, GetHash());
    RelayInv(inv);
}

bool CMasternodeBroadcast::Sign(CKey& keyCollateralAddress)
{
    std::string errorMessage;
    sigTime = GetAdjustedTime();

    std::string strMessage = GetStrMessage();

    if (!SignMessage(strMessage, errorMessage, sig, keyCollateralAddress))
        return error("CMasternodeBroadcast::Sign() - Error: %s", errorMessage);

    if (!VerifyMessage(pubKeyCollateralAddress, sig, strMessage, errorMessage))
        return error("CMasternodeBroadcast::Sign() - Error: %s", errorMessage);

    return true;
}


bool CMasternodeBroadcast::VerifySignature()
{
    std::string errorMessage;

    if (!VerifyMessage(pubKeyCollateralAddress, sig, GetStrMessage(), errorMessage))
        return error("CMasternodeBroadcast::VerifySignature() - Error: %s", errorMessage);

    return true;
}

std::string CMasternodeBroadcast::GetStrMessage()
{
    return (addr.ToString() +
            std::to_string(sigTime) +
            pubKeyCollateralAddress.GetID().ToString() +
            pubKeyMasternode.GetID().ToString() +
            std::to_string(protocolVersion));
}

CMasternodePing::CMasternodePing()
{
    vin = CTxIn();
    blockHash = uint256(0);
    sigTime = 0;
    vchSig = std::vector<unsigned char>();
}

CMasternodePing::CMasternodePing(CTxIn& newVin)
{
    vin = newVin;
    blockHash = chainActive[chainActive.Height() - 12]->GetBlockHash();
    sigTime = GetAdjustedTime();
    vchSig = std::vector<unsigned char>();
}


bool CMasternodePing::Sign(CKey& keyMasternode, CPubKey& pubKeyMasternode)
{
    std::string errorMessage;
    std::string strMasterNodeSignMessage;

    sigTime = GetAdjustedTime();
    std::string strMessage = vin.ToString() + blockHash.ToString() + std::to_string(sigTime);

    if (!SignMessage(strMessage, errorMessage, vchSig, keyMasternode)) {
        LogPrint(BCLog::MASTERNODE, "CMasternodePing::Sign() - Error: %s\n", errorMessage);
        return false;
    }

    if (!VerifyMessage(pubKeyMasternode, vchSig, strMessage, errorMessage)) {
        LogPrint(BCLog::MASTERNODE, "CMasternodePing::Sign() - Error: %s\n", errorMessage);
        return false;
    }

    return true;
}

bool CMasternodePing::VerifySignature(CPubKey& pubKeyMasternode, int& nDos)
{
    std::string strMessage = vin.ToString() + blockHash.ToString() + std::to_string(sigTime);
    std::string errorMessage = "";

    if (!VerifyMessage(pubKeyMasternode, vchSig, strMessage, errorMessage)) {
        nDos = 33;
        return error("CMasternodePing::VerifySignature - Got bad Masternode ping signature %s Error: %s", vin.ToString(), errorMessage);
    }
    return true;
}

bool CMasternodePing::CheckAndUpdate(int& nDos, bool fRequireEnabled, bool fCheckSigTimeOnly)
{
    if (sigTime > GetAdjustedTime() + 60 * 60) {
        LogPrint(BCLog::MASTERNODE, "CMasternodePing::CheckAndUpdate - Signature rejected, too far into the future %s\n", vin.prevout.hash.ToString());
        nDos = 1;
        return false;
    }

    if (sigTime <= GetAdjustedTime() - 60 * 60) {
        LogPrint(BCLog::MASTERNODE, "CMasternodePing::CheckAndUpdate - Signature rejected, too far into the past %s - %d %d \n", vin.prevout.hash.ToString(), sigTime, GetAdjustedTime());
        nDos = 1;
        return false;
    }

    if (fCheckSigTimeOnly) {
        CMasternode* pmn = mnodeman.Find(vin);
        if (pmn) return VerifySignature(pmn->pubKeyMasternode, nDos);
        return true;
    }

    LogPrint(BCLog::MASTERNODE, "CMasternodePing::CheckAndUpdate - New Ping - %s - %s - %lli\n", GetHash().ToString(), blockHash.ToString(), sigTime);

    // see if we have this Masternode
    CMasternode* pmn = mnodeman.Find(vin);
    if (pmn != NULL && pmn->protocolVersion >= masternodePayments.GetMinMasternodePaymentsProto()) {
        if (fRequireEnabled && !pmn->IsEnabled()) return false;

        // LogPrint(BCLog::MASTERNODE,"mnping - Found corresponding mn for vin: %s\n", vin.ToString());
        // update only if there is no known ping for this masternode or
        // last ping was more then MASTERNODE_MIN_MNP_SECONDS-60 ago comparing to this one
        if (!pmn->IsPingedWithin(MASTERNODE_MIN_MNP_SECONDS - 60, sigTime)) {
            if (!VerifySignature(pmn->pubKeyMasternode, nDos))
                return false;

            BlockMap::iterator mi = mapBlockIndex.find(blockHash);
            if (mi != mapBlockIndex.end() && (*mi).second) {
                if ((*mi).second->nHeight < chainActive.Height() - 24) {
                    LogPrint(BCLog::MASTERNODE, "CMasternodePing::CheckAndUpdate - Masternode %s block hash %s is too old\n", vin.prevout.hash.ToString(), blockHash.ToString());
                    // Do nothing here (no Masternode update, no mnping relay)
                    // Let this node to be visible but fail to accept mnping

                    return false;
                }
            } else {
                LogPrint(BCLog::MASTERNODE, "CMasternodePing::CheckAndUpdate - Masternode %s block hash %s is unknown\n", vin.prevout.hash.ToString(), blockHash.ToString());
                // maybe we stuck so we shouldn't ban this node, just fail to accept it
                // TODO: or should we also request this block?

                return false;
            }

            pmn->lastPing = *this;

            //mnodeman.mapSeenMasternodeBroadcast.lastPing is probably outdated, so we'll update it
            CMasternodeBroadcast mnb(*pmn);
            uint256 hash = mnb.GetHash();
            if (mnodeman.mapSeenMasternodeBroadcast.count(hash)) {
                mnodeman.mapSeenMasternodeBroadcast[hash].lastPing = *this;
            }

            pmn->Check(true);
            if (!pmn->IsEnabled()) return false;

            LogPrint(BCLog::MASTERNODE, "CMasternodePing::CheckAndUpdate - Masternode ping accepted, vin: %s\n", vin.prevout.hash.ToString());

            Relay();
            return true;
        }
        LogPrint(BCLog::MASTERNODE, "CMasternodePing::CheckAndUpdate - Masternode ping arrived too early, vin: %s\n", vin.prevout.hash.ToString());
        //nDos = 1; //disable, this is happening frequently and causing banned peers
        return false;
    }
    LogPrint(BCLog::MASTERNODE, "CMasternodePing::CheckAndUpdate - Couldn't find compatible Masternode entry, vin: %s\n", vin.prevout.hash.ToString());

    return false;
}

void CMasternodePing::Relay()
{
    CInv inv(MSG_MASTERNODE_PING, GetHash());
    RelayInv(inv);
}


//
// Bootup the Masternode, look for a 10000 GalaxyCash input and register on the network
//
void CActiveMasternode::ManageStatus()
{
    std::string errorMessage;

    if (!fMasterNode) return;

    LogPrintf("CActiveMasternode::ManageStatus() - Begin\n");

    //need correct blocks to send ping
    if (Params().NetworkIDString() != CBaseChainParams::REGTEST && !masternodeSync.IsBlockchainSynced()) {
        status = ACTIVE_MASTERNODE_SYNC_IN_PROCESS;
        LogPrintf("CActiveMasternode::ManageStatus() - %s\n", GetStatus());
        return;
    }

    if (status == ACTIVE_MASTERNODE_SYNC_IN_PROCESS) status = ACTIVE_MASTERNODE_INITIAL;

    if (status == ACTIVE_MASTERNODE_INITIAL) {
        CMasternode* pmn;
        pmn = mnodeman.Find(pubKeyMasternode);
        if (pmn != NULL) {
            pmn->Check();
            if (pmn->IsEnabled() && pmn->protocolVersion == PROTOCOL_VERSION) EnableHotColdMasterNode(pmn->vin, pmn->addr);
        }
    }

    if (status != ACTIVE_MASTERNODE_STARTED) {
        // Set defaults
        status = ACTIVE_MASTERNODE_NOT_CAPABLE;
        notCapableReason = "";

        bool isLocked = false;
        bool isHot = false;

        for (CWalletRef pwallet : vpwallets) {
            if (pwallet->IsLocked())
                isLocked = true;
            if (pwallet->GetBalance() == 0)
                isHot = true;
        }
        if (isLocked) {
            notCapableReason = "Wallet is locked.";
            LogPrintf("CActiveMasternode::ManageStatus() - not capable: %s\n", notCapableReason);
            return;
        }

        if (isHot) {
            notCapableReason = "Hot node, waiting for remote activation.";
            LogPrintf("CActiveMasternode::ManageStatus() - not capable: %s\n", notCapableReason);
            return;
        }

        if (strMasterNodeAddr.empty()) {
            if (!GetLocal(service)) {
                notCapableReason = "Can't detect external address. Please use the masternodeaddr configuration option.";
                LogPrintf("CActiveMasternode::ManageStatus() - not capable: %s\n", notCapableReason);
                return;
            }
        } else {
            service = CService(strMasterNodeAddr);
        }

        // The service needs the correct default port to work properly
        if (!CMasternodeBroadcast::CheckDefaultPort(strMasterNodeAddr, errorMessage, "CActiveMasternode::ManageStatus()"))
            return;

        LogPrintf("CActiveMasternode::ManageStatus() - Checking inbound connection to '%s'\n", service.ToString());

        CNode* pnode = g_connman->ConnectNode(CAddress(service, (ServiceFlags)(NODE_NETWORK)), NULL, false);
        if (!pnode && !g_connman->FindNode(service)) {
            notCapableReason = "Could not connect to " + service.ToString();
            LogPrintf("CActiveMasternode::ManageStatus() - not capable: %s\n", notCapableReason);
            return;
        } else 
            pnode->Release();

        // Choose coins to use
        CPubKey pubKeyCollateralAddress;
        CKey keyCollateralAddress;

        if (GetMasterNodeVin(vin, pubKeyCollateralAddress, keyCollateralAddress)) {
            if (GetInputAge(vin) < MASTERNODE_MIN_CONFIRMATIONS) {
                status = ACTIVE_MASTERNODE_INPUT_TOO_NEW;
                notCapableReason = strprintf("%s - %d confirmations", GetStatus(), GetInputAge(vin));
                LogPrintf("CActiveMasternode::ManageStatus() - %s\n", notCapableReason);
                return;
            }

            for (CWalletRef pwallet : vpwallets) {
                pwallet->LockCoin(vin.prevout);
            }

            // send to all nodes
            CPubKey pubKeyMasternode;
            CKey keyMasternode;

            if (!SetKey(strMasterNodePrivKey, errorMessage, keyMasternode, pubKeyMasternode)) {
                notCapableReason = "Error upon calling SetKey: " + errorMessage;
                LogPrintf("Register::ManageStatus() - %s\n", notCapableReason);
                return;
            }

            CMasternodeBroadcast mnb;
            if (!CreateBroadcast(vin, service, keyCollateralAddress, pubKeyCollateralAddress, keyMasternode, pubKeyMasternode, errorMessage, mnb)) {
                notCapableReason = "Error on Register: " + errorMessage;
                LogPrintf("CActiveMasternode::ManageStatus() - %s\n", notCapableReason);
                return;
            }

            //send to all peers
            LogPrintf("CActiveMasternode::ManageStatus() - Relay broadcast vin = %s\n", vin.ToString());
            mnb.Relay();

            LogPrintf("CActiveMasternode::ManageStatus() - Is capable master node!\n");
            status = ACTIVE_MASTERNODE_STARTED;

            return;
        } else {
            notCapableReason = "Could not find suitable coins!";
            LogPrintf("CActiveMasternode::ManageStatus() - %s\n", notCapableReason);
            return;
        }
    }

    //send to all peers
    if (!SendMasternodePing(errorMessage)) {
        LogPrintf("CActiveMasternode::ManageStatus() - Error on Ping: %s\n", errorMessage);
    }
}

std::string CActiveMasternode::GetStatus()
{
    switch (status) {
    case ACTIVE_MASTERNODE_INITIAL:
        return "Node just started, not yet activated";
    case ACTIVE_MASTERNODE_SYNC_IN_PROCESS:
        return "Sync in progress. Must wait until sync is complete to start Masternode";
    case ACTIVE_MASTERNODE_INPUT_TOO_NEW:
        return strprintf("Masternode input must have at least %d confirmations", MASTERNODE_MIN_CONFIRMATIONS);
    case ACTIVE_MASTERNODE_NOT_CAPABLE:
        return "Not capable masternode: " + notCapableReason;
    case ACTIVE_MASTERNODE_STARTED:
        return "Masternode successfully started";
    default:
        return "unknown";
    }
}

bool CActiveMasternode::SendMasternodePing(std::string& errorMessage)
{
    if (status != ACTIVE_MASTERNODE_STARTED) {
        errorMessage = "Masternode is not in a running status";
        return false;
    }

    CPubKey pubKeyMasternode;
    CKey keyMasternode;

    if (!SetKey(strMasterNodePrivKey, errorMessage, keyMasternode, pubKeyMasternode)) {
        errorMessage = strprintf("Error upon calling SetKey: %s\n", errorMessage);
        return false;
    }

    LogPrintf("CActiveMasternode::SendMasternodePing() - Relay Masternode Ping vin = %s\n", vin.ToString());

    CMasternodePing mnp(vin);
    if (!mnp.Sign(keyMasternode, pubKeyMasternode)) {
        errorMessage = "Couldn't sign Masternode Ping";
        return false;
    }

    // Update lastPing for our masternode in Masternode list
    CMasternode* pmn = mnodeman.Find(vin);
    if (pmn != NULL) {
        if (pmn->IsPingedWithin(MASTERNODE_PING_SECONDS, mnp.sigTime)) {
            errorMessage = "Too early to send Masternode Ping";
            return false;
        }

        pmn->lastPing = mnp;
        mnodeman.mapSeenMasternodePing.insert(std::make_pair(mnp.GetHash(), mnp));

        //mnodeman.mapSeenMasternodeBroadcast.lastPing is probably outdated, so we'll update it
        CMasternodeBroadcast mnb(*pmn);
        uint256 hash = mnb.GetHash();
        if (mnodeman.mapSeenMasternodeBroadcast.count(hash)) mnodeman.mapSeenMasternodeBroadcast[hash].lastPing = mnp;

        mnp.Relay();

        /*
         * IT'S SAFE TO REMOVE THIS IN FURTHER VERSIONS
         * AFTER MIGRATION TO V12 IS DONE
         */


        // for migration purposes ping our node on old masternodes network too
        std::string retErrorMessage;
        std::vector<unsigned char> vchMasterNodeSignature;
        int64_t masterNodeSignatureTime = GetAdjustedTime();

        std::string strMessage = service.ToString() + std::to_string(masterNodeSignatureTime) + std::to_string(false);

        if (!SignMessage(strMessage, retErrorMessage, vchMasterNodeSignature, keyMasternode)) {
            errorMessage = "dseep sign message failed: " + retErrorMessage;
            return false;
        }

        if (!VerifyMessage(pubKeyMasternode, vchMasterNodeSignature, strMessage, retErrorMessage)) {
            errorMessage = "dseep verify message failed: " + retErrorMessage;
            return false;
        }

        LogPrint(BCLog::MASTERNODE, "dseep - relaying from active mn, %s \n", vin.ToString().c_str());
        CTxIn vin2 = this->vin;
        g_connman->ForEachNode([=, &vin2, &vchMasterNodeSignature, &masterNodeSignatureTime](CNode* pnode) {
            g_connman->PushMessage(pnode, CNetMsgMaker(pnode->nVersion).Make("dseep", vin2, vchMasterNodeSignature, masterNodeSignatureTime, false));
        });
        /*
         * END OF "REMOVE"
         */

        return true;
    } else {
        // Seems like we are trying to send a ping while the Masternode is not registered in the network
        errorMessage = " Masternode List doesn't include our Masternode, shutting down Masternode pinging service! " + vin.ToString();
        status = ACTIVE_MASTERNODE_NOT_CAPABLE;
        notCapableReason = errorMessage;
        return false;
    }
}

bool CActiveMasternode::CreateBroadcast(std::string strService, std::string strKeyMasternode, std::string strTxHash, std::string strOutputIndex, std::string& errorMessage, CMasternodeBroadcast& mnb, bool fOffline)
{
    CTxIn vin;
    CPubKey pubKeyCollateralAddress;
    CKey keyCollateralAddress;
    CPubKey pubKeyMasternode;
    CKey keyMasternode;

    //need correct blocks to send ping
    if (!fOffline && !masternodeSync.IsBlockchainSynced()) {
        errorMessage = "Sync in progress. Must wait until sync is complete to start Masternode";
        LogPrintf("CActiveMasternode::CreateBroadcast() - %s\n", errorMessage);
        return false;
    }

    if (!GetKeysFromSecret(strKeyMasternode, keyMasternode, pubKeyMasternode)) {
        errorMessage = strprintf("Can't find keys for masternode %s", strService);
        LogPrintf("CActiveMasternode::CreateBroadcast() - %s\n", errorMessage);
        return false;
    }

    if (!GetMasterNodeVin(vin, pubKeyCollateralAddress, keyCollateralAddress, strTxHash, strOutputIndex)) {
        errorMessage = strprintf("Could not allocate vin %s:%s for masternode %s", strTxHash, strOutputIndex, strService);
        LogPrintf("CActiveMasternode::CreateBroadcast() - %s\n", errorMessage);
        return false;
    }

    CService service = CService(strService);

    // The service needs the correct default port to work properly
    if (!CMasternodeBroadcast::CheckDefaultPort(strService, errorMessage, "CActiveMasternode::CreateBroadcast()"))
        return false;

    std::vector<CAddress> vAddr;
    vAddr.push_back(CAddress(service, (ServiceFlags)(NODE_NETWORK)));
    g_connman->AddNewAddresses(vAddr, CAddress(CService("127.0.0.1"), (ServiceFlags)(NODE_NETWORK)), 2 * 60 * 60);

    return CreateBroadcast(vin, CService(strService), keyCollateralAddress, pubKeyCollateralAddress, keyMasternode, pubKeyMasternode, errorMessage, mnb);
}

bool CActiveMasternode::CreateBroadcast(CTxIn vin, CService service, CKey keyCollateralAddress, CPubKey pubKeyCollateralAddress, CKey keyMasternode, CPubKey pubKeyMasternode, std::string& errorMessage, CMasternodeBroadcast& mnb)
{
    // wait for reindex and/or import to finish
    if (fImporting || fReindex) return false;

    CMasternodePing mnp(vin);
    if (!mnp.Sign(keyMasternode, pubKeyMasternode)) {
        errorMessage = strprintf("Failed to sign ping, vin: %s", vin.ToString());
        LogPrintf("CActiveMasternode::CreateBroadcast() -  %s\n", errorMessage);
        mnb = CMasternodeBroadcast();
        return false;
    }

    mnb = CMasternodeBroadcast(service, vin, pubKeyCollateralAddress, pubKeyMasternode, PROTOCOL_VERSION);
    mnb.lastPing = mnp;
    if (!mnb.Sign(keyCollateralAddress)) {
        errorMessage = strprintf("Failed to sign broadcast, vin: %s", vin.ToString());
        LogPrintf("CActiveMasternode::CreateBroadcast() - %s\n", errorMessage);
        mnb = CMasternodeBroadcast();
        return false;
    }

    /*
     * IT'S SAFE TO REMOVE THIS IN FURTHER VERSIONS
     * AFTER MIGRATION TO V12 IS DONE
     */


    // for migration purposes inject our node in old masternodes' list too
    std::string retErrorMessage;
    std::vector<unsigned char> vchMasterNodeSignature;
    int64_t masterNodeSignatureTime = GetAdjustedTime();
    std::string donationAddress = "";
    int donationPercantage = 0;

    std::string vchPubKey(pubKeyCollateralAddress.begin(), pubKeyCollateralAddress.end());
    std::string vchPubKey2(pubKeyMasternode.begin(), pubKeyMasternode.end());

    std::string strMessage = service.ToString() + std::to_string(masterNodeSignatureTime) + vchPubKey + vchPubKey2 + std::to_string(PROTOCOL_VERSION) + donationAddress + std::to_string(donationPercantage);

    if (!SignMessage(strMessage, retErrorMessage, vchMasterNodeSignature, keyCollateralAddress)) {
        errorMessage = "dsee sign message failed: " + retErrorMessage;
        LogPrintf("CActiveMasternode::Register() - Error: %s\n", errorMessage.c_str());
        return false;
    }

    if (!VerifyMessage(pubKeyCollateralAddress, vchMasterNodeSignature, strMessage, retErrorMessage)) {
        errorMessage = "dsee verify message failed: " + retErrorMessage;
        LogPrintf("CActiveMasternode::Register() - Error: %s\n", errorMessage.c_str());
        return false;
    }

    g_connman->ForEachNode([=, &vin, &service, &vchMasterNodeSignature, &masterNodeSignatureTime, &pubKeyCollateralAddress, &pubKeyMasternode, &donationAddress, &donationPercantage](CNode* pnode) {
        g_connman->PushMessage(pnode, CNetMsgMaker(pnode->nVersion).Make("dsee", vin, service, vchMasterNodeSignature, masterNodeSignatureTime, pubKeyCollateralAddress, pubKeyMasternode, -1, -1, masterNodeSignatureTime, PROTOCOL_VERSION, donationAddress, donationPercantage));
    });
    /*
     * END OF "REMOVE"
     */

    return true;
}

bool CActiveMasternode::GetMasterNodeVin(CTxIn& vin, CPubKey& pubkey, CKey& secretKey)
{
    return GetMasterNodeVin(vin, pubkey, secretKey, "", "");
}

bool CActiveMasternode::GetMasterNodeVin(CTxIn& vin, CPubKey& pubkey, CKey& secretKey, std::string strTxHash, std::string strOutputIndex)
{
    // wait for reindex and/or import to finish
    if (fImporting || fReindex) return false;

    std::vector<COutput> possibleCoins = SelectCoinsMasternode();
    COutput* selectedOutput;

    // Find the vin
    if (!strTxHash.empty()) {
        // Let's find it
        uint256 txHash(strTxHash);
        int outputIndex;
        try {
            outputIndex = std::stoi(strOutputIndex.c_str());
        } catch (const std::exception& e) {
            LogPrintf("%s: %s on strOutputIndex\n", __func__, e.what());
            return false;
        }

        bool found = false;
        for (COutput& out : possibleCoins) {
            if (out.tx->GetHash() == txHash && out.i == outputIndex) {
                selectedOutput = &out;
                found = true;
                break;
            }
        }
        if (!found) {
            LogPrintf("CActiveMasternode::GetMasterNodeVin - Could not locate valid vin\n");
            return false;
        }
    } else {
        // No output specified,  Select the first one
        if (possibleCoins.size() > 0) {
            selectedOutput = &possibleCoins[0];
        } else {
            LogPrintf("CActiveMasternode::GetMasterNodeVin - Could not locate specified vin from possible list\n");
            return false;
        }
    }

    // At this point we have a selected output, retrieve the associated info
    return GetVinFromOutput(*selectedOutput, vin, pubkey, secretKey);
}


// Extract Masternode vin information from output
bool CActiveMasternode::GetVinFromOutput(COutput out, CTxIn& vin, CPubKey& pubkey, CKey& secretKey)
{
    CScript pubScript;

    CInputCoin coin(out.tx, out.i);

	vin = CTxIn(coin.outpoint.hash,coin.outpoint.n);

	CTxDestination address1;
    ExtractDestination(coin.txout.scriptPubKey, address1);
    CBitcoinAddress address2(address1);

    CKeyID keyID;
    if (!address2.GetKeyID(keyID)) {
        LogPrintf("CActiveMasternode::GetMasterNodeVin - Address does not refer to a key\n");
        return false;
    }

    if (vpwallets.size() >= 1 && !vpwallets[0]->GetKey(keyID, secretKey)) {
        LogPrintf ("CActiveMasternode::GetMasterNodeVin - Private key for address is not known\n");
        return false;
    }

    pubkey = secretKey.GetPubKey();
    return true;
}

// get all possible outputs for running Masternode
std::vector<COutput> CActiveMasternode::SelectCoinsMasternode()
{
    CWalletRef pwallet = vpwallets.size() >= 1 ? vpwallets[0] : nullptr;
    if (!pwallet) return std::vector<COutput>();

    std::vector<COutput> vCoins;
    std::vector<COutput> filteredCoins;
    std::vector<COutPoint> confLockedCoins;

    // Temporary unlock MN coins from masternode.conf
    if (gArgs.GetBoolArg("-mnconflock", true)) {
        uint256 mnTxHash;
        for (CMasternodeConfig::CMasternodeEntry mne : masternodeConfig.getEntries()) {
            mnTxHash.SetHex(mne.getTxHash());

            int nIndex;
            if (!mne.castOutputIndex(nIndex))
                continue;

            COutPoint outpoint = COutPoint(mnTxHash, nIndex);
            confLockedCoins.push_back(outpoint);
            pwallet->UnlockCoin(outpoint);
        }
    }

    {
        // Retrieve all possible outputs
        pwallet->AvailableCoins(vCoins);

        // Lock MN coins from masternode.conf back if they where temporary unlocked
        if (!confLockedCoins.empty()) {
            for (COutPoint outpoint : confLockedCoins)
                pwallet->LockCoin(outpoint);
        }

        // Filter
        for (const COutput& out : vCoins) {
            if (out.tx->tx->vout[out.i].nValue == MASTERNODE_COLLATERAL) { //exactly
                filteredCoins.push_back(out);
            }
        }
    }
    return filteredCoins;
}

// when starting a Masternode, this can enable to run as a hot wallet with no funds
bool CActiveMasternode::EnableHotColdMasterNode(CTxIn& newVin, CService& newService)
{
    if (!fMasterNode) return false;

    status = ACTIVE_MASTERNODE_STARTED;

    //The values below are needed for signing mnping messages going forward
    vin = newVin;
    service = newService;

    LogPrintf("CActiveMasternode::EnableHotColdMasterNode() - Enabled! You may shut down the cold daemon.\n");

    return true;
}


#define MN_WINNER_MINIMUM_AGE 8000 // Age in seconds. This should be > MASTERNODE_REMOVAL_SECONDS to avoid misconfigured new nodes in the list.

/** Masternode manager */
CMasternodeMan mnodeman;

struct CompareLastPaid {
    bool operator()(const std::pair<int64_t, CTxIn>& t1,
        const std::pair<int64_t, CTxIn>& t2) const
    {
        return t1.first < t2.first;
    }
};

struct CompareScoreTxIn {
    bool operator()(const std::pair<int64_t, CTxIn>& t1,
        const std::pair<int64_t, CTxIn>& t2) const
    {
        return t1.first < t2.first;
    }
};

struct CompareScoreMN {
    bool operator()(const std::pair<int64_t, CMasternode>& t1,
        const std::pair<int64_t, CMasternode>& t2) const
    {
        return t1.first < t2.first;
    }
};

//
// CMasternodeDB
//

CMasternodeDB::CMasternodeDB()
{
    pathMN = GetDataDir() / "mncache.dat";
    strMagicMessage = "MasternodeCache";
}

bool CMasternodeDB::Write(const CMasternodeMan& mnodemanToSave)
{
    int64_t nStart = GetTimeMillis();

    // serialize, checksum data up to that point, then append checksum
    CDataStream ssMasternodes(SER_DISK, PROTOCOL_VERSION);
    ssMasternodes << strMagicMessage;                   // masternode cache file specific magic message
    ssMasternodes << FLATDATA(Params().MessageStart()); // network specific magic number
    ssMasternodes << mnodemanToSave;
    uint256 hash = Hash(ssMasternodes.begin(), ssMasternodes.end());
    ssMasternodes << hash;

    // open output file, and associate with CAutoFile
    FILE* file = fopen(pathMN.string().c_str(), "wb");
    CAutoFile fileout(file, SER_DISK, PROTOCOL_VERSION);
    if (fileout.IsNull())
        return error("%s : Failed to open file %s", __func__, pathMN.string());

    // Write and commit header, data
    try {
        fileout << ssMasternodes;
    } catch (const std::exception& e) {
        return error("%s : Serialize or I/O error - %s", __func__, e.what());
    }
    //    FileCommit(fileout);
    fileout.fclose();

    LogPrint(BCLog::MASTERNODE, "Written info to mncache.dat  %dms\n", GetTimeMillis() - nStart);
    LogPrint(BCLog::MASTERNODE, "  %s\n", mnodemanToSave.ToString());

    return true;
}

CMasternodeDB::ReadResult CMasternodeDB::Read(CMasternodeMan& mnodemanToLoad, bool fDryRun)
{
    int64_t nStart = GetTimeMillis();
    // open input file, and associate with CAutoFile
    FILE* file = fopen(pathMN.string().c_str(), "rb");
    CAutoFile filein(file, SER_DISK, PROTOCOL_VERSION);
    if (filein.IsNull()) {
        error("%s : Failed to open file %s", __func__, pathMN.string());
        return FileError;
    }

    // use file size to size memory buffer
    int fileSize = boost::filesystem::file_size(pathMN);
    int dataSize = fileSize - sizeof(uint256);
    // Don't try to resize to a negative number if file is small
    if (dataSize < 0)
        dataSize = 0;
    std::vector<unsigned char> vchData;
    vchData.resize(dataSize);
    uint256 hashIn;

    // read data and checksum from file
    try {
        filein.read((char*)&vchData[0], dataSize);
        filein >> hashIn;
    } catch (const std::exception& e) {
        error("%s : Deserialize or I/O error - %s", __func__, e.what());
        return HashReadError;
    }
    filein.fclose();

    CDataStream ssMasternodes(vchData, SER_DISK, PROTOCOL_VERSION);

    // verify stored checksum matches input data
    uint256 hashTmp = Hash(ssMasternodes.begin(), ssMasternodes.end());
    if (hashIn != hashTmp) {
        error("%s : Checksum mismatch, data corrupted", __func__);
        return IncorrectHash;
    }

    unsigned char pchMsgTmp[4];
    std::string strMagicMessageTmp;
    try {
        // de-serialize file header (masternode cache file specific magic message) and ..

        ssMasternodes >> strMagicMessageTmp;

        // ... verify the message matches predefined one
        if (strMagicMessage != strMagicMessageTmp) {
            error("%s : Invalid masternode cache magic message", __func__);
            return IncorrectMagicMessage;
        }

        // de-serialize file header (network specific magic number) and ..
        ssMasternodes >> FLATDATA(pchMsgTmp);

        // ... verify the network matches ours
        if (memcmp(pchMsgTmp, Params().MessageStart(), sizeof(pchMsgTmp))) {
            error("%s : Invalid network magic number", __func__);
            return IncorrectMagicNumber;
        }
        // de-serialize data into CMasternodeMan object
        ssMasternodes >> mnodemanToLoad;
    } catch (const std::exception& e) {
        mnodemanToLoad.Clear();
        error("%s : Deserialize or I/O error - %s", __func__, e.what());
        return IncorrectFormat;
    }

    LogPrint(BCLog::MASTERNODE, "Loaded info from mncache.dat  %dms\n", GetTimeMillis() - nStart);
    LogPrint(BCLog::MASTERNODE, "  %s\n", mnodemanToLoad.ToString());
    if (!fDryRun) {
        LogPrint(BCLog::MASTERNODE, "Masternode manager - cleaning....\n");
        mnodemanToLoad.CheckAndRemove(true);
        LogPrint(BCLog::MASTERNODE, "Masternode manager - result:\n");
        LogPrint(BCLog::MASTERNODE, "  %s\n", mnodemanToLoad.ToString());
    }

    return Ok;
}

void DumpMasternodes()
{
    int64_t nStart = GetTimeMillis();

    CMasternodeDB mndb;
    CMasternodeMan tempMnodeman;

    LogPrint(BCLog::MASTERNODE, "Verifying mncache.dat format...\n");
    CMasternodeDB::ReadResult readResult = mndb.Read(tempMnodeman, true);
    // there was an error and it was not an error on file opening => do not proceed
    if (readResult == CMasternodeDB::FileError)
        LogPrint(BCLog::MASTERNODE, "Missing masternode cache file - mncache.dat, will try to recreate\n");
    else if (readResult != CMasternodeDB::Ok) {
        LogPrint(BCLog::MASTERNODE, "Error reading mncache.dat: ");
        if (readResult == CMasternodeDB::IncorrectFormat)
            LogPrint(BCLog::MASTERNODE, "magic is ok but data has invalid format, will try to recreate\n");
        else {
            LogPrint(BCLog::MASTERNODE, "file format is unknown or invalid, please fix it manually\n");
            return;
        }
    }
    LogPrint(BCLog::MASTERNODE, "Writting info to mncache.dat...\n");
    mndb.Write(mnodeman);

    LogPrint(BCLog::MASTERNODE, "Masternode dump finished  %dms\n", GetTimeMillis() - nStart);
}


/** Object for who's going to get paid on which blocks */
CMasternodePayments masternodePayments;

CCriticalSection cs_vecPayments;
CCriticalSection cs_mapMasternodeBlocks;
CCriticalSection cs_mapMasternodePayeeVotes;

//
// CMasternodePaymentDB
//

CMasternodePaymentDB::CMasternodePaymentDB()
{
    pathDB = GetDataDir() / "mnpayments.dat";
    strMagicMessage = "MasternodePayments";
}

bool CMasternodePaymentDB::Write(const CMasternodePayments& objToSave)
{
    int64_t nStart = GetTimeMillis();

    // serialize, checksum data up to that point, then append checksum
    CDataStream ssObj(SER_DISK, PROTOCOL_VERSION);
    ssObj << strMagicMessage;                   // masternode cache file specific magic message
    ssObj << FLATDATA(Params().MessageStart()); // network specific magic number
    ssObj << objToSave;
    uint256 hash = Hash(ssObj.begin(), ssObj.end());
    ssObj << hash;

    // open output file, and associate with CAutoFile
    FILE* file = fopen(pathDB.string().c_str(), "wb");
    CAutoFile fileout(file, SER_DISK, PROTOCOL_VERSION);
    if (fileout.IsNull())
        return error("%s : Failed to open file %s", __func__, pathDB.string());

    // Write and commit header, data
    try {
        fileout << ssObj;
    } catch (const std::exception& e) {
        return error("%s : Serialize or I/O error - %s", __func__, e.what());
    }
    fileout.fclose();

    LogPrint(BCLog::MASTERNODE, "Written info to mnpayments.dat  %dms\n", GetTimeMillis() - nStart);

    return true;
}

CMasternodePaymentDB::ReadResult CMasternodePaymentDB::Read(CMasternodePayments& objToLoad, bool fDryRun)
{
    int64_t nStart = GetTimeMillis();
    // open input file, and associate with CAutoFile
    FILE* file = fopen(pathDB.string().c_str(), "rb");
    CAutoFile filein(file, SER_DISK, PROTOCOL_VERSION);
    if (filein.IsNull()) {
        error("%s : Failed to open file %s", __func__, pathDB.string());
        return FileError;
    }

    // use file size to size memory buffer
    int fileSize = boost::filesystem::file_size(pathDB);
    int dataSize = fileSize - sizeof(uint256);
    // Don't try to resize to a negative number if file is small
    if (dataSize < 0)
        dataSize = 0;
    std::vector<unsigned char> vchData;
    vchData.resize(dataSize);
    uint256 hashIn;

    // read data and checksum from file
    try {
        filein.read((char*)&vchData[0], dataSize);
        filein >> hashIn;
    } catch (const std::exception& e) {
        error("%s : Deserialize or I/O error - %s", __func__, e.what());
        return HashReadError;
    }
    filein.fclose();

    CDataStream ssObj(vchData, SER_DISK, PROTOCOL_VERSION);

    // verify stored checksum matches input data
    uint256 hashTmp = Hash(ssObj.begin(), ssObj.end());
    if (hashIn != hashTmp) {
        error("%s : Checksum mismatch, data corrupted", __func__);
        return IncorrectHash;
    }

    unsigned char pchMsgTmp[4];
    std::string strMagicMessageTmp;
    try {
        // de-serialize file header (masternode cache file specific magic message) and ..
        ssObj >> strMagicMessageTmp;

        // ... verify the message matches predefined one
        if (strMagicMessage != strMagicMessageTmp) {
            error("%s : Invalid masternode payement cache magic message", __func__);
            return IncorrectMagicMessage;
        }


        // de-serialize file header (network specific magic number) and ..
        ssObj >> FLATDATA(pchMsgTmp);

        // ... verify the network matches ours
        if (memcmp(pchMsgTmp, Params().MessageStart(), sizeof(pchMsgTmp))) {
            error("%s : Invalid network magic number", __func__);
            return IncorrectMagicNumber;
        }

        // de-serialize data into CMasternodePayments object
        ssObj >> objToLoad;
    } catch (const std::exception& e) {
        objToLoad.Clear();
        error("%s : Deserialize or I/O error - %s", __func__, e.what());
        return IncorrectFormat;
    }

    LogPrint(BCLog::MASTERNODE, "Loaded info from mnpayments.dat  %dms\n", GetTimeMillis() - nStart);
    LogPrint(BCLog::MASTERNODE, "  %s\n", objToLoad.ToString());
    if (!fDryRun) {
        LogPrint(BCLog::MASTERNODE, "Masternode payments manager - cleaning....\n");
        objToLoad.CleanPaymentList();
        LogPrint(BCLog::MASTERNODE, "Masternode payments manager - result:\n");
        LogPrint(BCLog::MASTERNODE, "  %s\n", objToLoad.ToString());
    }

    return Ok;
}

void DumpMasternodePayments()
{
    int64_t nStart = GetTimeMillis();

    CMasternodePaymentDB paymentdb;
    CMasternodePayments tempPayments;

    LogPrint(BCLog::MASTERNODE, "Verifying mnpayments.dat format...\n");
    CMasternodePaymentDB::ReadResult readResult = paymentdb.Read(tempPayments, true);
    // there was an error and it was not an error on file opening => do not proceed
    if (readResult == CMasternodePaymentDB::FileError)
        LogPrint(BCLog::MASTERNODE, "Missing budgets file - mnpayments.dat, will try to recreate\n");
    else if (readResult != CMasternodePaymentDB::Ok) {
        LogPrint(BCLog::MASTERNODE, "Error reading mnpayments.dat: ");
        if (readResult == CMasternodePaymentDB::IncorrectFormat)
            LogPrint(BCLog::MASTERNODE, "magic is ok but data has invalid format, will try to recreate\n");
        else {
            LogPrint(BCLog::MASTERNODE, "file format is unknown or invalid, please fix it manually\n");
            return;
        }
    }
    LogPrint(BCLog::MASTERNODE, "Writting info to mnpayments.dat...\n");
    paymentdb.Write(masternodePayments);

    LogPrint(BCLog::MASTERNODE, "Budget dump finished  %dms\n", GetTimeMillis() - nStart);
}

bool IsBlockValueValid(const CBlock& block, CAmount nExpectedValue, CAmount nMinted)
{
    CBlockIndex* pindexPrev = chainActive.Tip();
    if (pindexPrev == NULL) return true;

    int nHeight = 0;
    if (pindexPrev->GetBlockHash() == block.hashPrevBlock) {
        nHeight = pindexPrev->nHeight + 1;
    } else { //out of order
        BlockMap::iterator mi = mapBlockIndex.find(block.hashPrevBlock);
        if (mi != mapBlockIndex.end() && (*mi).second)
            nHeight = (*mi).second->nHeight + 1;
    }

    if (nHeight == 0) {
        LogPrint(BCLog::MASTERNODE, "IsBlockValueValid() : WARNING: Couldn't find previous block\n");
    }

    //LogPrintf("XX69----------> IsBlockValueValid(): nMinted: %d, nExpectedValue: %d\n", FormatMoney(nMinted), FormatMoney(nExpectedValue));

    if (!masternodeSync.IsSynced()) { //there is no budget data to use to check anything
        //super blocks will always be on these blocks, max 100 per budgeting
        if (nHeight % BUDGET_CYCLE_BLOCKS < 100) {
            return true;
        } else {
            if (nMinted > nExpectedValue) {
                return false;
            }
        }
    } else { // we're synced and have data so check the budget schedule

        if (nMinted > nExpectedValue) {
            return false;
        }
    }

    return true;
}

bool IsBlockPayeeValid(const CBlock& block, int nBlockHeight)
{
    TrxValidationStatus transactionStatus = TrxValidationStatus::InValid;

    if (!masternodeSync.IsSynced()) { //there is no budget data to use to check anything -- find the longest chain
        LogPrint(BCLog::PAYMENTS, "Client not synced, skipping block payee checks\n");
        return true;
    }

    const CTransactionRef& txNew = (nBlockHeight >= Params().GetConsensus().FirstMNPaymentRejectionBlock() ? block.vtx[1] : block.vtx[0]);

    //check for masternode payee
    if (masternodePayments.IsTransactionValid(*txNew, nBlockHeight))
        return true;
    LogPrint(BCLog::MASTERNODE, "Invalid mn payment detected %s\n", txNew->ToString().c_str());

    return false;
}


void FillBlockPayee(CMutableTransaction& txNew, CAmount nFees, bool fProofOfStake)
{
    CBlockIndex* pindexPrev = chainActive.Tip();
    if (!pindexPrev) return;

    masternodePayments.FillBlockPayee(txNew, nFees, fProofOfStake);
}

std::string GetRequiredPaymentsString(int nBlockHeight)
{
    return masternodePayments.GetRequiredPaymentsString(nBlockHeight);
}

void CMasternodePayments::FillBlockPayee(CMutableTransaction& txNew, int64_t nFees, bool fProofOfStake)
{
    CBlockIndex* pindexPrev = chainActive.Tip();
    if (!pindexPrev) return;

    bool hasPayment = true;
    CScript payee;

    //spork
    if (!masternodePayments.GetBlockPayee(pindexPrev->nHeight + 1, payee)) {
        //no masternode detected
        CMasternode* winningNode = mnodeman.GetCurrentMasterNode(1);
        if (winningNode) {
            payee = GetScriptForDestination(winningNode->pubKeyCollateralAddress.GetID());
        } else {
            LogPrint(BCLog::MASTERNODE, "CreateNewBlock: Failed to detect masternode to pay\n");
            hasPayment = false;
        }
    }

    CAmount blockValue = fProofOfStake ? GetProofOfStakeReward(nFees, pindexPrev->nHeight) : GetProofOfWorkReward(nFees, pindexPrev->nHeight);
    CAmount masternodePayment = GetMasternodePayment(pindexPrev->nHeight, blockValue, 0);

    if (hasPayment) {
        if (fProofOfStake) {
            /**For Proof Of Stake vout[0] must be null
             * Stake reward can be split into many different outputs, so we must
             * use vout.size() to align with several different cases.
             * An additional output is appended as the masternode payment
             */
            unsigned int i = txNew.vout.size();
            txNew.vout.resize(i + 1);
            txNew.vout[i].scriptPubKey = payee;
            txNew.vout[i].nValue = masternodePayment;

            //subtract mn payment from the stake reward
            {
                if (i == 2) {
                    // Majority of cases; do it quick and move on
                    txNew.vout[i - 1].nValue -= masternodePayment;
                } else if (i > 2) {
                    // special case, stake is split between (i-1) outputs
                    unsigned int outputs = i - 1;
                    CAmount mnPaymentSplit = masternodePayment / outputs;
                    CAmount mnPaymentRemainder = masternodePayment - (mnPaymentSplit * outputs);
                    for (unsigned int j = 1; j <= outputs; j++) {
                        txNew.vout[j].nValue -= mnPaymentSplit;
                    }
                    // in case it's not an even division, take the last bit of dust from the last one
                    txNew.vout[outputs].nValue -= mnPaymentRemainder;
                }
            }
        } else {
            txNew.vout.resize(2);
            txNew.vout[1].scriptPubKey = payee;
            txNew.vout[1].nValue = masternodePayment;
            txNew.vout[0].nValue = blockValue - masternodePayment;
        }

        CTxDestination address1;
        ExtractDestination(payee, address1);
        CBitcoinAddress address2(address1);

        LogPrint(BCLog::MASTERNODE, "Masternode payment of %s to %s\n", FormatMoney(masternodePayment).c_str(), address2.ToString().c_str());
    }
}

int CMasternodePayments::GetMinMasternodePaymentsProto()
{
    return MIN_MASTERNODE_VERSION; // Allow only updated peers
}

void CMasternodePayments::ProcessMessageMasternodePayments(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv)
{
    if (!masternodeSync.IsBlockchainSynced()) return;


    if (strCommand == "mnget") { //Masternode Payments Request Sync

        int nCountNeeded;
        vRecv >> nCountNeeded;

        if (Params().NetworkIDString() == CBaseChainParams::MAIN) {
            if (pfrom->HasFulfilledRequest("mnget")) {
                LogPrintf("CMasternodePayments::ProcessMessageMasternodePayments() : mnget - peer already asked me for the list\n");
                return;
            }
        }

        pfrom->FulfilledRequest("mnget");
        masternodePayments.Sync(pfrom, nCountNeeded);
        LogPrint(BCLog::PAYMENTS, "mnget - Sent Masternode winners to peer %i\n", pfrom->GetId());
    } else if (strCommand == "mnw") { //Masternode Payments Declare Winner
        //this is required in litemodef
        CMasternodePaymentWinner winner;
        vRecv >> winner;

        if (pfrom->nVersion < MIN_MASTERNODE_VERSION) return;

        int nHeight;
        {
            TRY_LOCK(cs_main, locked);
            if (!locked || chainActive.Tip() == NULL) return;
            nHeight = chainActive.Tip()->nHeight;
        }

        if (masternodePayments.mapMasternodePayeeVotes.count(winner.GetHash())) {
            LogPrint(BCLog::PAYMENTS, "mnw - Already seen - %s bestHeight %d\n", winner.GetHash().ToString().c_str(), nHeight);
            masternodeSync.AddedMasternodeWinner(winner.GetHash());
            return;
        }

        int nFirstBlock = nHeight - (mnodeman.CountEnabled() * 1.25);
        if (winner.nBlockHeight < nFirstBlock || winner.nBlockHeight > nHeight + 20) {
            LogPrint(BCLog::PAYMENTS, "mnw - winner out of range - FirstBlock %d Height %d bestHeight %d\n", nFirstBlock, winner.nBlockHeight, nHeight);
            return;
        }

        std::string strError = "";
        if (!winner.IsValid(pfrom, strError)) {
            // if(strError != "") LogPrint(BCLog::MASTERNODE,"mnw - invalid message - %s\n", strError);
            return;
        }

        if (!masternodePayments.CanVote(winner.vinMasternode.prevout, winner.nBlockHeight)) {
            //  LogPrint(BCLog::MASTERNODE,"mnw - masternode already voted - %s\n", winner.vinMasternode.prevout.ToStringShort());
            return;
        }

        if (!winner.SignatureValid()) {
            if (masternodeSync.IsSynced()) {
                LogPrintf("CMasternodePayments::ProcessMessageMasternodePayments() : mnw - invalid signature\n");
                Misbehaving(pfrom->GetId(), 20);
            }
            // it could just be a non-synced masternode
            mnodeman.AskForMN(pfrom, winner.vinMasternode);
            return;
        }

        CTxDestination address1;
        ExtractDestination(winner.payee, address1);
        CBitcoinAddress address2(address1);

        //   LogPrint(BCLog::PAYMENTS, "mnw - winning vote - Addr %s Height %d bestHeight %d - %s\n", address2.ToString().c_str(), winner.nBlockHeight, nHeight, winner.vinMasternode.prevout.ToStringShort());

        if (masternodePayments.AddWinningMasternode(winner)) {
            winner.Relay();
            masternodeSync.AddedMasternodeWinner(winner.GetHash());
        }
    }
}

bool CMasternodePaymentWinner::Sign(CKey& keyMasternode, CPubKey& pubKeyMasternode)
{
    std::string errorMessage;
    std::string strMasterNodeSignMessage;

    std::string strMessage = vinMasternode.prevout.ToStringShort() + std::to_string(nBlockHeight) + payee.ToString();

    if (!SignMessage(strMessage, errorMessage, vchSig, keyMasternode)) {
        LogPrint(BCLog::MASTERNODE, "CMasternodePing::Sign() - Error: %s\n", errorMessage.c_str());
        return false;
    }

    if (!VerifyMessage(pubKeyMasternode, vchSig, strMessage, errorMessage)) {
        LogPrint(BCLog::MASTERNODE, "CMasternodePing::Sign() - Error: %s\n", errorMessage.c_str());
        return false;
    }

    return true;
}

bool CMasternodePayments::GetBlockPayee(int nBlockHeight, CScript& payee)
{
    if (mapMasternodeBlocks.count(nBlockHeight)) {
        return mapMasternodeBlocks[nBlockHeight].GetPayee(payee);
    }

    return false;
}

// Is this masternode scheduled to get paid soon?
// -- Only look ahead up to 8 blocks to allow for propagation of the latest 2 winners
bool CMasternodePayments::IsScheduled(CMasternode& mn, int nNotBlockHeight)
{
    LOCK(cs_mapMasternodeBlocks);

    int nHeight;
    {
        TRY_LOCK(cs_main, locked);
        if (!locked || chainActive.Tip() == NULL) return false;
        nHeight = chainActive.Tip()->nHeight;
    }

    CScript mnpayee;
    mnpayee = GetScriptForDestination(mn.pubKeyCollateralAddress.GetID());

    CScript payee;
    for (int64_t h = nHeight; h <= nHeight + 8; h++) {
        if (h == nNotBlockHeight) continue;
        if (mapMasternodeBlocks.count(h)) {
            if (mapMasternodeBlocks[h].GetPayee(payee)) {
                if (mnpayee == payee) {
                    return true;
                }
            }
        }
    }

    return false;
}

bool CMasternodePayments::AddWinningMasternode(CMasternodePaymentWinner& winnerIn)
{
    uint256 blockHash = 0;
    if (!GetBlockHash(blockHash, winnerIn.nBlockHeight - 100)) {
        return false;
    }

    {
        LOCK2(cs_mapMasternodePayeeVotes, cs_mapMasternodeBlocks);

        if (mapMasternodePayeeVotes.count(winnerIn.GetHash())) {
            return false;
        }

        mapMasternodePayeeVotes[winnerIn.GetHash()] = winnerIn;

        if (!mapMasternodeBlocks.count(winnerIn.nBlockHeight)) {
            CMasternodeBlockPayees blockPayees(winnerIn.nBlockHeight);
            mapMasternodeBlocks[winnerIn.nBlockHeight] = blockPayees;
        }
    }

    mapMasternodeBlocks[winnerIn.nBlockHeight].AddPayee(winnerIn.payee, 1);

    return true;
}

bool CMasternodeBlockPayees::IsTransactionValid(const CTransaction& txNew)
{
    LOCK(cs_vecPayments);

    int nMaxSignatures = 0;
    int nMasternode_Drift_Count = 0;

    std::string strPayeesPossible = "";


    CAmount nReward = nBlockHeight > Params().GetConsensus().LastPowBlock() ? GetProofOfStakeReward(0, nBlockHeight) : GetProofOfWorkReward(0, nBlockHeight);

    nMasternode_Drift_Count = mnodeman.size() + MASTERNODE_COUNT_DRIFT;


    CAmount requiredMasternodePayment = GetMasternodePayment(nBlockHeight, nReward, nMasternode_Drift_Count);

    //require at least 6 signatures
    for (CMasternodePayee& payee : vecPayments)
        if (payee.nVotes >= nMaxSignatures && payee.nVotes >= MNPAYMENTS_SIGNATURES_REQUIRED)
            nMaxSignatures = payee.nVotes;

    // if we don't have at least 6 signatures on a payee, approve whichever is the longest chain
    if (nMaxSignatures < MNPAYMENTS_SIGNATURES_REQUIRED) return true;

    for (CMasternodePayee& payee : vecPayments) {
        bool found = false;
        for (CTxOut out : txNew.vout) {
            if (payee.scriptPubKey == out.scriptPubKey) {
                if (out.nValue >= requiredMasternodePayment)
                    found = true;
                else
                    LogPrint(BCLog::MASTERNODE, "Masternode payment is out of drift range. Paid=%s Min=%s\n", FormatMoney(out.nValue).c_str(), FormatMoney(requiredMasternodePayment).c_str());
            }
        }

        if (payee.nVotes >= MNPAYMENTS_SIGNATURES_REQUIRED) {
            if (found) return true;

            CTxDestination address1;
            ExtractDestination(payee.scriptPubKey, address1);
            CBitcoinAddress address2(address1);

            if (strPayeesPossible == "") {
                strPayeesPossible += address2.ToString();
            } else {
                strPayeesPossible += "," + address2.ToString();
            }
        }
    }

    LogPrint(BCLog::MASTERNODE, "CMasternodePayments::IsTransactionValid - Missing required payment of %s to %s\n", FormatMoney(requiredMasternodePayment).c_str(), strPayeesPossible.c_str());
    return false;
}

std::string CMasternodeBlockPayees::GetRequiredPaymentsString()
{
    LOCK(cs_vecPayments);

    std::string ret = "Unknown";

    for (CMasternodePayee& payee : vecPayments) {
        CTxDestination address1;
        ExtractDestination(payee.scriptPubKey, address1);
        CBitcoinAddress address2(address1);

        if (ret != "Unknown") {
            ret += ", " + address2.ToString() + ":" + std::to_string(payee.nVotes);
        } else {
            ret = address2.ToString() + ":" + std::to_string(payee.nVotes);
        }
    }

    return ret;
}

std::string CMasternodePayments::GetRequiredPaymentsString(int nBlockHeight)
{
    LOCK(cs_mapMasternodeBlocks);

    if (mapMasternodeBlocks.count(nBlockHeight)) {
        return mapMasternodeBlocks[nBlockHeight].GetRequiredPaymentsString();
    }

    return "Unknown";
}

bool CMasternodePayments::IsTransactionValid(const CTransaction& txNew, int nBlockHeight)
{
    LOCK(cs_mapMasternodeBlocks);

    if (mapMasternodeBlocks.count(nBlockHeight)) {
        return mapMasternodeBlocks[nBlockHeight].IsTransactionValid(txNew);
    }

    return true;
}

void CMasternodePayments::CleanPaymentList()
{
    LOCK2(cs_mapMasternodePayeeVotes, cs_mapMasternodeBlocks);

    int nHeight;
    {
        TRY_LOCK(cs_main, locked);
        if (!locked || chainActive.Tip() == NULL) return;
        nHeight = chainActive.Tip()->nHeight;
    }

    //keep up to five cycles for historical sake
    int nLimit = std::max(int(mnodeman.size() * 1.25), 1000);

    std::map<uint256, CMasternodePaymentWinner>::iterator it = mapMasternodePayeeVotes.begin();
    while (it != mapMasternodePayeeVotes.end()) {
        CMasternodePaymentWinner winner = (*it).second;

        if (nHeight - winner.nBlockHeight > nLimit) {
            LogPrint(BCLog::PAYMENTS, "CMasternodePayments::CleanPaymentList - Removing old Masternode payment - block %d\n", winner.nBlockHeight);
            masternodeSync.mapSeenSyncMNW.erase((*it).first);
            mapMasternodePayeeVotes.erase(it++);
            mapMasternodeBlocks.erase(winner.nBlockHeight);
        } else {
            ++it;
        }
    }
}

bool CMasternodePaymentWinner::IsValid(CNode* pnode, std::string& strError)
{
    CMasternode* pmn = mnodeman.Find(vinMasternode);

    if (!pmn) {
        strError = strprintf("Unknown Masternode %s", vinMasternode.prevout.hash.ToString());
        LogPrint(BCLog::MASTERNODE, "CMasternodePaymentWinner::IsValid - %s\n", strError);
        mnodeman.AskForMN(pnode, vinMasternode);
        return false;
    }

    if (pmn->protocolVersion < MIN_MASTERNODE_VERSION) {
        strError = strprintf("Masternode protocol too old %d - req %d", pmn->protocolVersion, MIN_MASTERNODE_VERSION);
        LogPrint(BCLog::MASTERNODE, "CMasternodePaymentWinner::IsValid - %s\n", strError);
        return false;
    }

    int n = mnodeman.GetMasternodeRank(vinMasternode, nBlockHeight - 100, MIN_MASTERNODE_VERSION);

    if (n > MNPAYMENTS_SIGNATURES_TOTAL) {
        //It's common to have masternodes mistakenly think they are in the top 10
        // We don't want to print all of these messages, or punish them unless they're way off
        if (n > MNPAYMENTS_SIGNATURES_TOTAL * 2) {
            strError = strprintf("Masternode not in the top %d (%d)", MNPAYMENTS_SIGNATURES_TOTAL * 2, n);
            LogPrint(BCLog::MASTERNODE, "CMasternodePaymentWinner::IsValid - %s\n", strError);
            //if (masternodeSync.IsSynced()) Misbehaving(pnode->GetId(), 20);
        }
        return false;
    }

    return true;
}

bool CMasternodePayments::ProcessBlock(int nBlockHeight)
{
    if (!fMasterNode) return false;

    //reference node - hybrid mode

    int n = mnodeman.GetMasternodeRank(activeMasternode.vin, nBlockHeight - 100, MIN_MASTERNODE_VERSION);

    if (n == -1) {
        LogPrint(BCLog::PAYMENTS, "CMasternodePayments::ProcessBlock - Unknown Masternode\n");
        return false;
    }

    if (n > MNPAYMENTS_SIGNATURES_TOTAL) {
        LogPrint(BCLog::PAYMENTS, "CMasternodePayments::ProcessBlock - Masternode not in the top %d (%d)\n", MNPAYMENTS_SIGNATURES_TOTAL, n);
        return false;
    }

    if (nBlockHeight <= nLastBlockHeight) return false;

    CMasternodePaymentWinner newWinner(activeMasternode.vin);

    {
        LogPrint(BCLog::MASTERNODE, "CMasternodePayments::ProcessBlock() Start nHeight %d - vin %s. \n", nBlockHeight, activeMasternode.vin.prevout.hash.ToString());

        // pay to the oldest MN that still had no payment but its input is old enough and it was active long enough
        int nCount = 0;
        CMasternode* pmn = mnodeman.GetNextMasternodeInQueueForPayment(nBlockHeight, true, nCount);

        if (pmn != NULL) {
            LogPrint(BCLog::MASTERNODE, "CMasternodePayments::ProcessBlock() Found by FindOldestNotInVec \n");

            newWinner.nBlockHeight = nBlockHeight;

            CScript payee = GetScriptForDestination(pmn->pubKeyCollateralAddress.GetID());
            newWinner.AddPayee(payee);

            CTxDestination address1;
            ExtractDestination(payee, address1);
            CBitcoinAddress address2(address1);

            LogPrint(BCLog::MASTERNODE, "CMasternodePayments::ProcessBlock() Winner payee %s nHeight %d. \n", address2.ToString().c_str(), newWinner.nBlockHeight);
        } else {
            LogPrint(BCLog::MASTERNODE, "CMasternodePayments::ProcessBlock() Failed to find masternode to pay\n");
        }
    }

    std::string errorMessage;
    CPubKey pubKeyMasternode;
    CKey keyMasternode;

    if (!SetKey(strMasterNodePrivKey, errorMessage, keyMasternode, pubKeyMasternode)) {
        LogPrint(BCLog::MASTERNODE, "CMasternodePayments::ProcessBlock() - Error upon calling SetKey: %s\n", errorMessage.c_str());
        return false;
    }

    LogPrint(BCLog::MASTERNODE, "CMasternodePayments::ProcessBlock() - Signing Winner\n");
    if (newWinner.Sign(keyMasternode, pubKeyMasternode)) {
        LogPrint(BCLog::MASTERNODE, "CMasternodePayments::ProcessBlock() - AddWinningMasternode\n");

        if (AddWinningMasternode(newWinner)) {
            newWinner.Relay();
            nLastBlockHeight = nBlockHeight;
            return true;
        }
    }

    return false;
}

void CMasternodePaymentWinner::Relay()
{
    CInv inv(MSG_MASTERNODE_WINNER, GetHash());
    RelayInv(inv);
}

bool CMasternodePaymentWinner::SignatureValid()
{
    CMasternode* pmn = mnodeman.Find(vinMasternode);

    if (pmn != NULL) {
        std::string strMessage = vinMasternode.prevout.ToStringShort() + std::to_string(nBlockHeight) + payee.ToString();

        std::string errorMessage = "";
        if (!VerifyMessage(pmn->pubKeyMasternode, vchSig, strMessage, errorMessage)) {
            return error("CMasternodePaymentWinner::SignatureValid() - Got bad Masternode address signature %s\n", vinMasternode.prevout.hash.ToString());
        }

        return true;
    }

    return false;
}

void CMasternodePayments::Sync(CNode* node, int nCountNeeded)
{
    LOCK(cs_mapMasternodePayeeVotes);

    int nHeight;
    {
        TRY_LOCK(cs_main, locked);
        if (!locked || chainActive.Tip() == NULL) return;
        nHeight = chainActive.Tip()->nHeight;
    }

    int nCount = (mnodeman.CountEnabled() * 1.25);
    if (nCountNeeded > nCount) nCountNeeded = nCount;

    int nInvCount = 0;
    std::map<uint256, CMasternodePaymentWinner>::iterator it = mapMasternodePayeeVotes.begin();
    while (it != mapMasternodePayeeVotes.end()) {
        CMasternodePaymentWinner winner = (*it).second;
        if (winner.nBlockHeight >= nHeight - nCountNeeded && winner.nBlockHeight <= nHeight + 20) {
            node->PushInventory(CInv(MSG_MASTERNODE_WINNER, winner.GetHash()));
            nInvCount++;
        }
        ++it;
    }
    g_connman->PushMessage(node, CNetMsgMaker(node->nVersion).Make("ssc", MASTERNODE_SYNC_MNW, nInvCount));
}

std::string CMasternodePayments::ToString() const
{
    std::ostringstream info;

    info << "Votes: " << (int)mapMasternodePayeeVotes.size() << ", Blocks: " << (int)mapMasternodeBlocks.size();

    return info.str();
}


int CMasternodePayments::GetOldestBlock()
{
    LOCK(cs_mapMasternodeBlocks);

    int nOldestBlock = std::numeric_limits<int>::max();

    std::map<int, CMasternodeBlockPayees>::iterator it = mapMasternodeBlocks.begin();
    while (it != mapMasternodeBlocks.end()) {
        if ((*it).first < nOldestBlock) {
            nOldestBlock = (*it).first;
        }
        it++;
    }

    return nOldestBlock;
}


int CMasternodePayments::GetNewestBlock()
{
    LOCK(cs_mapMasternodeBlocks);

    int nNewestBlock = 0;

    std::map<int, CMasternodeBlockPayees>::iterator it = mapMasternodeBlocks.begin();
    while (it != mapMasternodeBlocks.end()) {
        if ((*it).first > nNewestBlock) {
            nNewestBlock = (*it).first;
        }
        it++;
    }

    return nNewestBlock;
}


CMasternodeMan::CMasternodeMan()
{
    nDsqCount = 0;
}

bool CMasternodeMan::Add(CMasternode& mn)
{
    LOCK(cs);

    if (!mn.IsEnabled())
        return false;

    CMasternode* pmn = Find(mn.vin);
    if (pmn == NULL) {
        LogPrint(BCLog::MASTERNODE, "CMasternodeMan: Adding new Masternode %s - %i now\n", mn.vin.prevout.hash.ToString(), size() + 1);
        vMasternodes.push_back(mn);
        return true;
    }

    return false;
}

void CMasternodeMan::AskForMN(CNode* pnode, CTxIn& vin)
{
    std::map<COutPoint, int64_t>::iterator i = mWeAskedForMasternodeListEntry.find(vin.prevout);
    if (i != mWeAskedForMasternodeListEntry.end()) {
        int64_t t = (*i).second;
        if (GetTime() < t) return; // we've asked recently
    }

    // ask for the mnb info once from the node that sent mnp

    LogPrint(BCLog::MASTERNODE, "CMasternodeMan::AskForMN - Asking node for missing entry, vin: %s\n", vin.prevout.hash.ToString());
    g_connman->PushMessage(pnode, CNetMsgMaker(pnode->GetRecvVersion()).Make("dseg", vin));
    int64_t askAgain = GetTime() + MASTERNODE_MIN_MNP_SECONDS;
    mWeAskedForMasternodeListEntry[vin.prevout] = askAgain;
}

void CMasternodeMan::Check()
{
    LOCK(cs);

    for (CMasternode& mn : vMasternodes) {
        mn.Check();
    }
}

void CMasternodeMan::CheckAndRemove(bool forceExpiredRemoval)
{
    Check();

    LOCK(cs);

    //remove inactive and outdated
    std::vector<CMasternode>::iterator it = vMasternodes.begin();
    while (it != vMasternodes.end()) {
        if ((*it).activeState == CMasternode::MASTERNODE_REMOVE ||
            (*it).activeState == CMasternode::MASTERNODE_VIN_SPENT ||
            (forceExpiredRemoval && (*it).activeState == CMasternode::MASTERNODE_EXPIRED) ||
            (*it).protocolVersion < masternodePayments.GetMinMasternodePaymentsProto()) {
            LogPrint(BCLog::MASTERNODE, "CMasternodeMan: Removing inactive Masternode %s - %i now\n", (*it).vin.prevout.hash.ToString(), size() - 1);

            //erase all of the broadcasts we've seen from this vin
            // -- if we missed a few pings and the node was removed, this will allow is to get it back without them
            //    sending a brand new mnb
            std::map<uint256, CMasternodeBroadcast>::iterator it3 = mapSeenMasternodeBroadcast.begin();
            while (it3 != mapSeenMasternodeBroadcast.end()) {
                if ((*it3).second.vin == (*it).vin) {
                    masternodeSync.mapSeenSyncMNB.erase((*it3).first);
                    mapSeenMasternodeBroadcast.erase(it3++);
                } else {
                    ++it3;
                }
            }

            // allow us to ask for this masternode again if we see another ping
            std::map<COutPoint, int64_t>::iterator it2 = mWeAskedForMasternodeListEntry.begin();
            while (it2 != mWeAskedForMasternodeListEntry.end()) {
                if ((*it2).first == (*it).vin.prevout) {
                    mWeAskedForMasternodeListEntry.erase(it2++);
                } else {
                    ++it2;
                }
            }

            it = vMasternodes.erase(it);
        } else {
            ++it;
        }
    }

    // check who's asked for the Masternode list
    std::map<CNetAddr, int64_t>::iterator it1 = mAskedUsForMasternodeList.begin();
    while (it1 != mAskedUsForMasternodeList.end()) {
        if ((*it1).second < GetTime()) {
            mAskedUsForMasternodeList.erase(it1++);
        } else {
            ++it1;
        }
    }

    // check who we asked for the Masternode list
    it1 = mWeAskedForMasternodeList.begin();
    while (it1 != mWeAskedForMasternodeList.end()) {
        if ((*it1).second < GetTime()) {
            mWeAskedForMasternodeList.erase(it1++);
        } else {
            ++it1;
        }
    }

    // check which Masternodes we've asked for
    std::map<COutPoint, int64_t>::iterator it2 = mWeAskedForMasternodeListEntry.begin();
    while (it2 != mWeAskedForMasternodeListEntry.end()) {
        if ((*it2).second < GetTime()) {
            mWeAskedForMasternodeListEntry.erase(it2++);
        } else {
            ++it2;
        }
    }

    // remove expired mapSeenMasternodeBroadcast
    std::map<uint256, CMasternodeBroadcast>::iterator it3 = mapSeenMasternodeBroadcast.begin();
    while (it3 != mapSeenMasternodeBroadcast.end()) {
        if ((*it3).second.lastPing.sigTime < GetTime() - (MASTERNODE_REMOVAL_SECONDS * 2)) {
            mapSeenMasternodeBroadcast.erase(it3++);
            masternodeSync.mapSeenSyncMNB.erase((*it3).second.GetHash());
        } else {
            ++it3;
        }
    }

    // remove expired mapSeenMasternodePing
    std::map<uint256, CMasternodePing>::iterator it4 = mapSeenMasternodePing.begin();
    while (it4 != mapSeenMasternodePing.end()) {
        if ((*it4).second.sigTime < GetTime() - (MASTERNODE_REMOVAL_SECONDS * 2)) {
            mapSeenMasternodePing.erase(it4++);
        } else {
            ++it4;
        }
    }
}

void CMasternodeMan::Clear()
{
    LOCK(cs);
    vMasternodes.clear();
    mAskedUsForMasternodeList.clear();
    mWeAskedForMasternodeList.clear();
    mWeAskedForMasternodeListEntry.clear();
    mapSeenMasternodeBroadcast.clear();
    mapSeenMasternodePing.clear();
    nDsqCount = 0;
}

int CMasternodeMan::stable_size()
{
    int nStable_size = 0;
    int nMinProtocol = MIN_MASTERNODE_VERSION;
    int64_t nMasternode_Min_Age = MN_WINNER_MINIMUM_AGE;
    int64_t nMasternode_Age = 0;

    for (CMasternode& mn : vMasternodes) {
        if (mn.protocolVersion < nMinProtocol) {
            continue; // Skip obsolete versions
        }
        nMasternode_Age = GetAdjustedTime() - mn.sigTime;
        if ((nMasternode_Age) < nMasternode_Min_Age) {
            continue; // Skip masternodes younger than (default) 8000 sec (MUST be > MASTERNODE_REMOVAL_SECONDS)
        }
        mn.Check();
        if (!mn.IsEnabled())
            continue; // Skip not-enabled masternodes

        nStable_size++;
    }

    return nStable_size;
}

int CMasternodeMan::CountEnabled(int protocolVersion)
{
    int i = 0;
    protocolVersion = protocolVersion == -1 ? masternodePayments.GetMinMasternodePaymentsProto() : protocolVersion;

    for (CMasternode& mn : vMasternodes) {
        mn.Check();
        if (mn.protocolVersion < protocolVersion || !mn.IsEnabled()) continue;
        i++;
    }

    return i;
}

void CMasternodeMan::CountNetworks(int protocolVersion, int& ipv4, int& ipv6, int& onion)
{
    protocolVersion = protocolVersion == -1 ? masternodePayments.GetMinMasternodePaymentsProto() : protocolVersion;

    for (CMasternode& mn : vMasternodes) {
        mn.Check();
        std::string strHost;
        int port;
        SplitHostPort(mn.addr.ToString(), port, strHost);
        CNetAddr node = CNetAddr(strHost, false);
        int nNetwork = node.GetNetwork();
        switch (nNetwork) {
        case 1:
            ipv4++;
            break;
        case 2:
            ipv6++;
            break;
        case 3:
            onion++;
            break;
        }
    }
}

void CMasternodeMan::DsegUpdate(CNode* pnode)
{
    LOCK(cs);

    if (Params().NetworkIDString() == CBaseChainParams::MAIN) {
        if (!(pnode->addr.IsRFC1918() || pnode->addr.IsLocal())) {
            std::map<CNetAddr, int64_t>::iterator it = mWeAskedForMasternodeList.find(pnode->addr);
            if (it != mWeAskedForMasternodeList.end()) {
                if (GetTime() < (*it).second) {
                    LogPrint(BCLog::MASTERNODE, "dseg - we already asked peer %i for the list; skipping...\n", pnode->GetId());
                    return;
                }
            }
        }
    }

    g_connman->PushMessage(pnode, CNetMsgMaker(pnode->nVersion).Make("dseg", CTxIn()));
    int64_t askAgain = GetTime() + MASTERNODES_DSEG_SECONDS;
    mWeAskedForMasternodeList[pnode->addr] = askAgain;
}

CMasternode* CMasternodeMan::Find(const CScript& payee)
{
    LOCK(cs);
    CScript payee2;

    for (CMasternode& mn : vMasternodes) {
        payee2 = GetScriptForDestination(mn.pubKeyCollateralAddress.GetID());
        if (payee2 == payee)
            return &mn;
    }
    return NULL;
}

CMasternode* CMasternodeMan::Find(const CTxIn& vin)
{
    LOCK(cs);

    for (CMasternode& mn : vMasternodes) {
        if (mn.vin.prevout == vin.prevout)
            return &mn;
    }
    return NULL;
}


CMasternode* CMasternodeMan::Find(const CPubKey& pubKeyMasternode)
{
    LOCK(cs);

    for (CMasternode& mn : vMasternodes) {
        if (mn.pubKeyMasternode == pubKeyMasternode)
            return &mn;
    }
    return NULL;
}

//
// Deterministically select the oldest/best masternode to pay on the network
//
CMasternode* CMasternodeMan::GetNextMasternodeInQueueForPayment(int nBlockHeight, bool fFilterSigTime, int& nCount)
{
    LOCK(cs);

    CMasternode* pBestMasternode = NULL;
    std::vector<std::pair<int64_t, CTxIn>> vecMasternodeLastPaid;

    /*
        Make a vector with all of the last paid times
    */

    int nMnCount = CountEnabled();
    for (CMasternode& mn : vMasternodes) {
        mn.Check();
        if (!mn.IsEnabled()) continue;

        // //check protocol version
        if (mn.protocolVersion < masternodePayments.GetMinMasternodePaymentsProto()) continue;

        //it's in the list (up to 8 entries ahead of current block to allow propagation) -- so let's skip it
        if (masternodePayments.IsScheduled(mn, nBlockHeight)) continue;

        //it's too new, wait for a cycle
        if (fFilterSigTime && mn.sigTime + (nMnCount * 2.6 * 60) > GetAdjustedTime()) continue;

        //make sure it has as many confirmations as there are masternodes
        if (mn.GetMasternodeInputAge() < nMnCount) continue;

        vecMasternodeLastPaid.push_back(std::make_pair(mn.SecondsSincePayment(), mn.vin));
    }

    nCount = (int)vecMasternodeLastPaid.size();

    //when the network is in the process of upgrading, don't penalize nodes that recently restarted
    if (fFilterSigTime && nCount < nMnCount / 3) return GetNextMasternodeInQueueForPayment(nBlockHeight, false, nCount);

    // Sort them high to low
    sort(vecMasternodeLastPaid.rbegin(), vecMasternodeLastPaid.rend(), CompareLastPaid());

    // Look at 1/10 of the oldest nodes (by last payment), calculate their scores and pay the best one
    //  -- This doesn't look at who is being paid in the +8-10 blocks, allowing for double payments very rarely
    //  -- 1/100 payments should be a double payment on mainnet - (1/(3000/10))*2
    //  -- (chance per block * chances before IsScheduled will fire)
    int nTenthNetwork = CountEnabled() / 10;
    int nCountTenth = 0;
    uint256 nHigh = 0;
    for (PAIRTYPE(int64_t, CTxIn) & s : vecMasternodeLastPaid) {
        CMasternode* pmn = Find(s.second);
        if (!pmn) break;

        uint256 n = pmn->CalculateScore(1, nBlockHeight - 100);
        if (n > nHigh) {
            nHigh = n;
            pBestMasternode = pmn;
        }
        nCountTenth++;
        if (nCountTenth >= nTenthNetwork) break;
    }
    return pBestMasternode;
}

CMasternode* CMasternodeMan::FindRandomNotInVec(std::vector<CTxIn>& vecToExclude, int protocolVersion)
{
    LOCK(cs);

    protocolVersion = protocolVersion == -1 ? masternodePayments.GetMinMasternodePaymentsProto() : protocolVersion;

    int nCountEnabled = CountEnabled(protocolVersion);
    LogPrint(BCLog::MASTERNODE, "CMasternodeMan::FindRandomNotInVec - nCountEnabled - vecToExclude.size() %d\n", nCountEnabled - vecToExclude.size());
    if (nCountEnabled - vecToExclude.size() < 1) return NULL;

    int rand = GetRandInt(nCountEnabled - vecToExclude.size());
    LogPrint(BCLog::MASTERNODE, "CMasternodeMan::FindRandomNotInVec - rand %d\n", rand);
    bool found;

    for (CMasternode& mn : vMasternodes) {
        if (mn.protocolVersion < protocolVersion || !mn.IsEnabled()) continue;
        found = false;
        for (CTxIn& usedVin : vecToExclude) {
            if (mn.vin.prevout == usedVin.prevout) {
                found = true;
                break;
            }
        }
        if (found) continue;
        if (--rand < 1) {
            return &mn;
        }
    }

    return NULL;
}

CMasternode* CMasternodeMan::GetCurrentMasterNode(int mod, int64_t nBlockHeight, int minProtocol)
{
    int64_t score = 0;
    CMasternode* winner = NULL;

    // scan for winner
    for (CMasternode& mn : vMasternodes) {
        mn.Check();
        if (mn.protocolVersion < minProtocol || !mn.IsEnabled()) continue;

        // calculate the score for each Masternode
        uint256 n = mn.CalculateScore(mod, nBlockHeight);
        int64_t n2 = n.GetCompact(false);

        // determine the winner
        if (n2 > score) {
            score = n2;
            winner = &mn;
        }
    }

    return winner;
}

int CMasternodeMan::GetMasternodeRank(const CTxIn& vin, int64_t nBlockHeight, int minProtocol, bool fOnlyActive)
{
    std::vector<std::pair<int64_t, CTxIn>> vecMasternodeScores;
    int64_t nMasternode_Min_Age = MN_WINNER_MINIMUM_AGE;
    int64_t nMasternode_Age = 0;

    //make sure we know about this block
    uint256 hash = 0;
    if (!GetBlockHash(hash, nBlockHeight)) return -1;

    // scan for winner
    for (CMasternode& mn : vMasternodes) {
        if (mn.protocolVersion < minProtocol) {
            LogPrint(BCLog::MASTERNODE, "Skipping Masternode with obsolete version %d\n", mn.protocolVersion);
            continue; // Skip obsolete versions
        }


        nMasternode_Age = GetAdjustedTime() - mn.sigTime;
        if ((nMasternode_Age) < nMasternode_Min_Age) {
            LogPrint(BCLog::MASTERNODE, "Skipping just activated Masternode. Age: %ld\n", nMasternode_Age);
            continue; // Skip masternodes younger than (default) 1 hour
        }

        if (fOnlyActive) {
            mn.Check();
            if (!mn.IsEnabled()) continue;
        }
        uint256 n = mn.CalculateScore(1, nBlockHeight);
        int64_t n2 = n.GetCompact(false);

        vecMasternodeScores.push_back(std::make_pair(n2, mn.vin));
    }

    sort(vecMasternodeScores.rbegin(), vecMasternodeScores.rend(), CompareScoreTxIn());

    int rank = 0;
    for (PAIRTYPE(int64_t, CTxIn) & s : vecMasternodeScores) {
        rank++;
        if (s.second.prevout == vin.prevout) {
            return rank;
        }
    }

    return -1;
}

std::vector<std::pair<int, CMasternode>> CMasternodeMan::GetMasternodeRanks(int64_t nBlockHeight, int minProtocol)
{
    std::vector<std::pair<int64_t, CMasternode>> vecMasternodeScores;
    std::vector<std::pair<int, CMasternode>> vecMasternodeRanks;

    //make sure we know about this block
    uint256 hash = 0;
    if (!GetBlockHash(hash, nBlockHeight)) return vecMasternodeRanks;

    // scan for winner
    for (CMasternode& mn : vMasternodes) {
        mn.Check();

        if (mn.protocolVersion < minProtocol) continue;

        if (!mn.IsEnabled()) {
            vecMasternodeScores.push_back(std::make_pair(9999, mn));
            continue;
        }

        uint256 n = mn.CalculateScore(1, nBlockHeight);
        int64_t n2 = n.GetCompact(false);

        vecMasternodeScores.push_back(std::make_pair(n2, mn));
    }

    sort(vecMasternodeScores.rbegin(), vecMasternodeScores.rend(), CompareScoreMN());

    int rank = 0;
    for (PAIRTYPE(int64_t, CMasternode) & s : vecMasternodeScores) {
        rank++;
        vecMasternodeRanks.push_back(std::make_pair(rank, s.second));
    }

    return vecMasternodeRanks;
}

CMasternode* CMasternodeMan::GetMasternodeByRank(int nRank, int64_t nBlockHeight, int minProtocol, bool fOnlyActive)
{
    std::vector<std::pair<int64_t, CTxIn>> vecMasternodeScores;

    // scan for winner
    for (CMasternode& mn : vMasternodes) {
        if (mn.protocolVersion < minProtocol) continue;
        if (fOnlyActive) {
            mn.Check();
            if (!mn.IsEnabled()) continue;
        }

        uint256 n = mn.CalculateScore(1, nBlockHeight);
        int64_t n2 = n.GetCompact(false);

        vecMasternodeScores.push_back(std::make_pair(n2, mn.vin));
    }

    sort(vecMasternodeScores.rbegin(), vecMasternodeScores.rend(), CompareScoreTxIn());

    int rank = 0;
    for (PAIRTYPE(int64_t, CTxIn) & s : vecMasternodeScores) {
        rank++;
        if (rank == nRank) {
            return Find(s.second);
        }
    }

    return NULL;
}

void CMasternodeMan::ProcessMasternodeConnections()
{
    //we don't care about this for regtest
    if (Params().NetworkIDString() == CBaseChainParams::REGTEST) return;

    /*g_connman->ForEachNode([=](CNode* pnode) {
        if (pnode->fMasternode) {
            LogPrint(BCLog::MASTERNODE, "Closing Masternode connection peer=%i \n", pnode->GetId());
            pnode->fMasternode = false;
            pnode->Release();
        }
    })*/
}

void CMasternodeMan::ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv)
{
    if (!masternodeSync.IsBlockchainSynced()) return;

    LOCK(cs_process_message);

    if (strCommand == "mnb") { //Masternode Broadcast
        CMasternodeBroadcast mnb;
        vRecv >> mnb;

        if (mapSeenMasternodeBroadcast.count(mnb.GetHash())) { //seen
            masternodeSync.AddedMasternodeList(mnb.GetHash());
            return;
        }
        mapSeenMasternodeBroadcast.insert(std::make_pair(mnb.GetHash(), mnb));

        int nDoS = 0;
        if (!mnb.CheckAndUpdate(nDoS)) {
            if (nDoS > 0)
                Misbehaving(pfrom->GetId(), nDoS);

            //failed
            return;
        }

        // make sure the vout that was signed is related to the transaction that spawned the Masternode
        //  - this is expensive, so it's only done once per Masternode
        if (!IsVinAssociatedWithPubkey(mnb.vin, mnb.pubKeyCollateralAddress)) {
            LogPrintf("CMasternodeMan::ProcessMessage() : mnb - Got mismatched pubkey and vin\n");
            Misbehaving(pfrom->GetId(), 33);
            return;
        }

        // make sure it's still unspent
        //  - this is checked later by .check() in many places and by ThreadCheckPool()
        if (mnb.CheckInputsAndAdd(nDoS)) {
            // use this as a peer
            std::vector<CAddress> vAddr;
            vAddr.push_back(CAddress(mnb.addr, (ServiceFlags)(NODE_NETWORK | NODE_MASTERNODE)));
            g_connman->AddNewAddresses(vAddr, pfrom->addr, 2 * 60 * 60);
            masternodeSync.AddedMasternodeList(mnb.GetHash());
        } else {
            LogPrint(BCLog::MASTERNODE, "mnb - Rejected Masternode entry %s\n", mnb.vin.prevout.hash.ToString());

            if (nDoS > 0)
                Misbehaving(pfrom->GetId(), nDoS);
        }
    }

    else if (strCommand == "mnp") { //Masternode Ping
        CMasternodePing mnp;
        vRecv >> mnp;

        LogPrint(BCLog::MASTERNODE, "mnp - Masternode ping, vin: %s\n", mnp.vin.prevout.hash.ToString());

        if (mapSeenMasternodePing.count(mnp.GetHash())) return; //seen
        mapSeenMasternodePing.insert(std::make_pair(mnp.GetHash(), mnp));

        int nDoS = 0;
        if (mnp.CheckAndUpdate(nDoS)) return;

        if (nDoS > 0) {
            // if anything significant failed, mark that node
            Misbehaving(pfrom->GetId(), nDoS);
        } else {
            // if nothing significant failed, search existing Masternode list
            CMasternode* pmn = Find(mnp.vin);
            // if it's known, don't ask for the mnb, just return
            if (pmn != NULL) return;
        }

        // something significant is broken or mn is unknown,
        // we might have to ask for a masternode entry once
        AskForMN(pfrom, mnp.vin);

    } else if (strCommand == "dseg") { //Get Masternode list or specific entry

        CTxIn vin;
        vRecv >> vin;

        if (vin == CTxIn()) { //only should ask for this once
            //local network
            bool isLocal = (pfrom->addr.IsRFC1918() || pfrom->addr.IsLocal());

            if (!isLocal && Params().NetworkIDString() == CBaseChainParams::MAIN) {
                std::map<CNetAddr, int64_t>::iterator i = mAskedUsForMasternodeList.find(pfrom->addr);
                if (i != mAskedUsForMasternodeList.end()) {
                    int64_t t = (*i).second;
                    if (GetTime() < t) {
                        LogPrintf("CMasternodeMan::ProcessMessage() : dseg - peer already asked me for the list\n");
                        return;
                    }
                }
                int64_t askAgain = GetTime() + MASTERNODES_DSEG_SECONDS;
                mAskedUsForMasternodeList[pfrom->addr] = askAgain;
            }
        } //else, asking for a specific node which is ok


        int nInvCount = 0;

        for (CMasternode& mn : vMasternodes) {
            if (mn.addr.IsRFC1918()) continue; //local network

            if (mn.IsEnabled()) {
                LogPrint(BCLog::MASTERNODE, "dseg - Sending Masternode entry - %s \n", mn.vin.prevout.hash.ToString());
                if (vin == CTxIn() || vin == mn.vin) {
                    CMasternodeBroadcast mnb = CMasternodeBroadcast(mn);
                    uint256 hash = mnb.GetHash();
                    pfrom->PushInventory(CInv(MSG_MASTERNODE_ANNOUNCE, hash));
                    nInvCount++;

                    if (!mapSeenMasternodeBroadcast.count(hash)) mapSeenMasternodeBroadcast.insert(std::make_pair(hash, mnb));

                    if (vin == mn.vin) {
                        LogPrint(BCLog::MASTERNODE, "dseg - Sent 1 Masternode entry to peer %i\n", pfrom->GetId());
                        return;
                    }
                }
            }
        }

        if (vin == CTxIn()) {
            g_connman->PushMessage(pfrom, CNetMsgMaker(pfrom->nVersion).Make("ssc", MASTERNODE_SYNC_LIST, nInvCount));
            LogPrint(BCLog::MASTERNODE, "dseg - Sent %d Masternode entries to peer %i\n", nInvCount, pfrom->GetId());
        }
    }
    /*
     * IT'S SAFE TO REMOVE THIS IN FURTHER VERSIONS
     * AFTER MIGRATION TO V12 IS DONE
     */

    // Light version for OLD MASSTERNODES - fake pings, no self-activation
    else if (strCommand == "dsee") { // Election Entry

        CTxIn vin;
        CService addr;
        CPubKey pubkey;
        CPubKey pubkey2;
        std::vector<unsigned char> vchSig;
        int64_t sigTime;
        int count;
        int current;
        int64_t lastUpdated;
        int protocolVersion;
        CScript donationAddress;
        int donationPercentage;
        std::string strMessage;

        vRecv >> vin >> addr >> vchSig >> sigTime >> pubkey >> pubkey2 >> count >> current >> lastUpdated >> protocolVersion >> donationAddress >> donationPercentage;

        // make sure signature isn't in the future (past is OK)
        if (sigTime > GetAdjustedTime() + 60 * 60) {
            LogPrintf("CMasternodeMan::ProcessMessage() : dsee - Signature rejected, too far into the future %s\n", vin.prevout.hash.ToString());
            Misbehaving(pfrom->GetId(), 1);
            return;
        }

        std::string vchPubKey(pubkey.begin(), pubkey.end());
        std::string vchPubKey2(pubkey2.begin(), pubkey2.end());

        strMessage = addr.ToString() + std::to_string(sigTime) + vchPubKey + vchPubKey2 + std::to_string(protocolVersion) + donationAddress.ToString() + std::to_string(donationPercentage);

        if (protocolVersion < masternodePayments.GetMinMasternodePaymentsProto()) {
            LogPrintf("CMasternodeMan::ProcessMessage() : dsee - ignoring outdated Masternode %s protocol version %d < %d\n", vin.prevout.hash.ToString(), protocolVersion, masternodePayments.GetMinMasternodePaymentsProto());
            return;
        }

        CScript pubkeyScript;
        pubkeyScript = GetScriptForDestination(pubkey.GetID());

        if (pubkeyScript.size() != 25) {
            LogPrintf("CMasternodeMan::ProcessMessage() : dsee - pubkey the wrong size\n");
            Misbehaving(pfrom->GetId(), 100);
            return;
        }

        CScript pubkeyScript2;
        pubkeyScript2 = GetScriptForDestination(pubkey2.GetID());

        if (pubkeyScript2.size() != 25) {
            LogPrintf("CMasternodeMan::ProcessMessage() : dsee - pubkey2 the wrong size\n");
            Misbehaving(pfrom->GetId(), 100);
            return;
        }

        if (!vin.scriptSig.empty()) {
            LogPrintf("CMasternodeMan::ProcessMessage() : dsee - Ignore Not Empty ScriptSig %s\n", vin.prevout.hash.ToString());
            Misbehaving(pfrom->GetId(), 100);
            return;
        }

        std::string errorMessage = "";
        if (!VerifyMessage(pubkey, vchSig, strMessage, errorMessage)) {
            LogPrintf("CMasternodeMan::ProcessMessage() : dsee - Got bad Masternode address signature\n");
            Misbehaving(pfrom->GetId(), 100);
            return;
        }

        if (Params().NetworkIDString() == CBaseChainParams::MAIN) {
            if (addr.GetPort() != Params().GetDefaultPort()) return;
        } else if (addr.GetPort() == Params().GetDefaultPort())
            return;

        //search existing Masternode list, this is where we update existing Masternodes with new dsee broadcasts
        CMasternode* pmn = this->Find(vin);
        if (pmn != NULL) {
            // count == -1 when it's a new entry
            //   e.g. We don't want the entry relayed/time updated when we're syncing the list
            // mn.pubkey = pubkey, IsVinAssociatedWithPubkey is validated once below,
            //   after that they just need to match
            if (count == -1 && pmn->pubKeyCollateralAddress == pubkey && (GetAdjustedTime() - pmn->nLastDsee > MASTERNODE_MIN_MNB_SECONDS)) {
                if (pmn->protocolVersion >= NEW_VERSION && sigTime - pmn->lastPing.sigTime < MASTERNODE_MIN_MNB_SECONDS) return;
                if (pmn->nLastDsee < sigTime) { //take the newest entry
                    LogPrint(BCLog::MASTERNODE, "dsee - Got updated entry for %s\n", vin.prevout.hash.ToString());
                    if (pmn->protocolVersion <= OLD_VERSION) {
                        pmn->pubKeyMasternode = pubkey2;
                        pmn->sigTime = sigTime;
                        pmn->sig = vchSig;
                        pmn->protocolVersion = protocolVersion;
                        pmn->addr = addr;
                        //fake ping
                        pmn->lastPing = CMasternodePing(vin);
                    }
                    pmn->nLastDsee = sigTime;
                    pmn->Check();
                    if (pmn->IsEnabled()) {
                        g_connman->ForEachNode([=, &vin, &addr, &vchSig, &sigTime, &pubkey, &pubkey2, &count, &current, &lastUpdated, &protocolVersion, &donationAddress, &donationPercentage](CNode* pnode) {
                            if (pnode->nVersion >= masternodePayments.GetMinMasternodePaymentsProto())
                                g_connman->PushMessage(pnode, CNetMsgMaker(pnode->nVersion).Make("dsee", vin, addr, vchSig, sigTime, pubkey, pubkey2, count, current, lastUpdated, protocolVersion, donationAddress, donationPercentage));
                        });
                    }
                }
            }

            return;
        }

        static std::map<COutPoint, CPubKey> mapSeenDsee;
        if (mapSeenDsee.count(vin.prevout) && mapSeenDsee[vin.prevout] == pubkey) {
            LogPrint(BCLog::MASTERNODE, "dsee - already seen this vin %s\n", vin.prevout.ToString());
            return;
        }
        mapSeenDsee.insert(std::make_pair(vin.prevout, pubkey));
        // make sure the vout that was signed is related to the transaction that spawned the Masternode
        //  - this is expensive, so it's only done once per Masternode
        if (!IsVinAssociatedWithPubkey(vin, pubkey)) {
            LogPrintf("CMasternodeMan::ProcessMessage() : dsee - Got mismatched pubkey and vin\n");
            Misbehaving(pfrom->GetId(), 100);
            return;
        }


        LogPrint(BCLog::MASTERNODE, "dsee - Got NEW OLD Masternode entry %s\n", vin.prevout.hash.ToString());

        // make sure it's still unspent
        //  - this is checked later by .check() in many places and by ThreadCheckPool()

        CValidationState state;
        CMutableTransaction tx = CMutableTransaction();
        std::string devAddr = "GL83ZiVZ26z3stMtrF91WJ5f77q6EnKXnC";
        CBitcoinAddress gdevAddr;
        gdevAddr.SetString(devAddr);
        CTxOut vout = CTxOut((MASTERNODE_COLLATERAL_AMOUNT - 1) * COIN, GetScriptForDestination(gdevAddr.Get()));
        tx.vin.push_back(vin);
        tx.vout.push_back(vout);

        bool fAcceptable = false;
        {
            TRY_LOCK(cs_main, lockMain);
            if (!lockMain) return;
            fAcceptable = AcceptableInputs(mempool, state, CTransaction(tx), false, NULL);
        }

        if (fAcceptable) {
            if (GetInputAge(vin) < MASTERNODE_MIN_CONFIRMATIONS) {
                LogPrintf("CMasternodeMan::ProcessMessage() : dsee - Input must have least %d confirmations\n", MASTERNODE_MIN_CONFIRMATIONS);
                Misbehaving(pfrom->GetId(), 1);
                return;
            }

            // verify that sig time is legit in past
            // should be at least not earlier than block when 1000 GalaxyCash tx got MASTERNODE_MIN_CONFIRMATIONS
            uint256 hashBlock = 0;
            CTransactionRef tx2;
            GetTransaction2(vin.prevout.hash, tx2, &hashBlock);
            BlockMap::iterator mi = mapBlockIndex.find(hashBlock);
            if (mi != mapBlockIndex.end() && (*mi).second) {
                CBlockIndex* pMNIndex = (*mi).second;                                                        // block for 10000 PIV tx -> 1 confirmation
                CBlockIndex* pConfIndex = chainActive[pMNIndex->nHeight + MASTERNODE_MIN_CONFIRMATIONS - 1]; // block where tx got MASTERNODE_MIN_CONFIRMATIONS
                if (pConfIndex->GetBlockTime() > sigTime) {
                    LogPrint(BCLog::MASTERNODE, "mnb - Bad sigTime %d for Masternode %s (%i conf block is at %d)\n",
                        sigTime, vin.prevout.hash.ToString(), MASTERNODE_MIN_CONFIRMATIONS, pConfIndex->GetBlockTime());
                    return;
                }
            }

            // use this as a peer
            std::vector<CAddress> vAddr;
            vAddr.push_back(CAddress(addr, (ServiceFlags)(NODE_NETWORK | NODE_MASTERNODE)));
            g_connman->AddNewAddresses(vAddr, pfrom->addr, 2 * 60 * 60);

            // add Masternode
            CMasternode mn = CMasternode();
            mn.addr = addr;
            mn.vin = vin;
            mn.pubKeyCollateralAddress = pubkey;
            mn.sig = vchSig;
            mn.sigTime = sigTime;
            mn.pubKeyMasternode = pubkey2;
            mn.protocolVersion = protocolVersion;
            // fake ping
            mn.lastPing = CMasternodePing(vin);
            mn.Check(true);
            // add v11 masternodes, v12 should be added by mnb only
            if (protocolVersion <= OLD_VERSION) {
                LogPrint(BCLog::MASTERNODE, "dsee - Accepted OLD Masternode entry %i %i\n", count, current);
                Add(mn);
            }
            if (mn.IsEnabled()) {
                g_connman->ForEachNode([=, &vin, &addr, &vchSig, &sigTime, &pubkey, &pubkey2, &count, &current, &lastUpdated, &protocolVersion, &donationAddress, &donationPercentage](CNode* pnode) {
                    if (pnode->nVersion >= masternodePayments.GetMinMasternodePaymentsProto())
                        g_connman->PushMessage(pnode, CNetMsgMaker(pnode->nVersion).Make("dsee", vin, addr, vchSig, sigTime, pubkey, pubkey2, count, current, lastUpdated, protocolVersion, donationAddress, donationPercentage));
                });
            }
        } else {
            LogPrint(BCLog::MASTERNODE, "dsee - Rejected Masternode entry %s\n", vin.prevout.hash.ToString());

            int nDoS = 0;
            if (state.IsInvalid(nDoS)) {
                LogPrint(BCLog::MASTERNODE, "dsee - %s from %i %s was not accepted into the memory pool\n", tx.GetHash().ToString().c_str(),
                    pfrom->GetId(), pfrom->cleanSubVer.c_str());
                if (nDoS > 0)
                    Misbehaving(pfrom->GetId(), nDoS);
            }
        }
    }

    else if (strCommand == "dseep") { // Election Entry Ping

        CTxIn vin;
        std::vector<unsigned char> vchSig;
        int64_t sigTime;
        bool stop;
        vRecv >> vin >> vchSig >> sigTime >> stop;

        //LogPrint(BCLog::MASTERNODE,"dseep - Received: vin: %s sigTime: %lld stop: %s\n", vin.ToString().c_str(), sigTime, stop ? "true" : "false");

        if (sigTime > GetAdjustedTime() + 60 * 60) {
            LogPrintf("CMasternodeMan::ProcessMessage() : dseep - Signature rejected, too far into the future %s\n", vin.prevout.hash.ToString());
            return;
        }

        if (sigTime <= GetAdjustedTime() - 60 * 60) {
            LogPrintf("CMasternodeMan::ProcessMessage() : dseep - Signature rejected, too far into the past %s - %d %d \n", vin.prevout.hash.ToString(), sigTime, GetAdjustedTime());
            return;
        }

        std::map<COutPoint, int64_t>::iterator i = mWeAskedForMasternodeListEntry.find(vin.prevout);
        if (i != mWeAskedForMasternodeListEntry.end()) {
            int64_t t = (*i).second;
            if (GetTime() < t) return; // we've asked recently
        }

        // see if we have this Masternode
        CMasternode* pmn = this->Find(vin);
        if (pmn != NULL && pmn->protocolVersion >= masternodePayments.GetMinMasternodePaymentsProto()) {
            // LogPrint(BCLog::MASTERNODE,"dseep - Found corresponding mn for vin: %s\n", vin.ToString().c_str());
            // take this only if it's newer
            if (sigTime - pmn->nLastDseep > MASTERNODE_MIN_MNP_SECONDS) {
                std::string strMessage = pmn->addr.ToString() + std::to_string(sigTime) + std::to_string(stop);

                std::string errorMessage = "";
                if (!VerifyMessage(pmn->pubKeyMasternode, vchSig, strMessage, errorMessage)) {
                    LogPrint(BCLog::MASTERNODE, "dseep - Got bad Masternode address signature %s \n", vin.prevout.hash.ToString());
                    //Misbehaving(pfrom->GetId(), 100);
                    return;
                }

                // fake ping for v11 masternodes, ignore for v12
                if (pmn->protocolVersion <= OLD_VERSION) pmn->lastPing = CMasternodePing(vin);
                pmn->nLastDseep = sigTime;
                pmn->Check();
                if (pmn->IsEnabled()) {
                    LogPrint(BCLog::MASTERNODE, "dseep - relaying %s \n", vin.prevout.hash.ToString());
                    g_connman->ForEachNode([=, &vin, &vchSig, &sigTime, &stop](CNode* pnode) {
                        if (pnode->nVersion >= masternodePayments.GetMinMasternodePaymentsProto())
                            g_connman->PushMessage(pnode, CNetMsgMaker(pnode->nVersion).Make("dseep", vin, vchSig, sigTime, stop));
                    });
                }
            }
            return;
        }

        LogPrint(BCLog::MASTERNODE, "dseep - Couldn't find Masternode entry %s peer=%i\n", vin.prevout.hash.ToString(), pfrom->GetId());

        AskForMN(pfrom, vin);
    }

    /*
     * END OF "REMOVE"
     */
}

void CMasternodeMan::Remove(CTxIn vin)
{
    LOCK(cs);

    std::vector<CMasternode>::iterator it = vMasternodes.begin();
    while (it != vMasternodes.end()) {
        if ((*it).vin == vin) {
            LogPrint(BCLog::MASTERNODE, "CMasternodeMan: Removing Masternode %s - %i now\n", (*it).vin.prevout.hash.ToString(), size() - 1);
            vMasternodes.erase(it);
            break;
        }
        ++it;
    }
}

void CMasternodeMan::UpdateMasternodeList(CMasternodeBroadcast mnb)
{
    mapSeenMasternodePing.insert(std::make_pair(mnb.lastPing.GetHash(), mnb.lastPing));
    mapSeenMasternodeBroadcast.insert(std::make_pair(mnb.GetHash(), mnb));
    masternodeSync.AddedMasternodeList(mnb.GetHash());

    LogPrint(BCLog::MASTERNODE, "CMasternodeMan::UpdateMasternodeList() -- masternode=%s\n", mnb.vin.prevout.ToString());

    CMasternode* pmn = Find(mnb.vin);
    if (pmn == NULL) {
        CMasternode mn(mnb);
        Add(mn);
    } else {
        pmn->UpdateFromNewBroadcast(mnb);
    }
}

std::string CMasternodeMan::ToString() const
{
    std::ostringstream info;

    info << "Masternodes: " << (int)vMasternodes.size() << ", peers who asked us for Masternode list: " << (int)mAskedUsForMasternodeList.size() << ", peers we asked for Masternode list: " << (int)mWeAskedForMasternodeList.size() << ", entries in Masternode list we asked for: " << (int)mWeAskedForMasternodeListEntry.size() << ", nDsqCount: " << (int)nDsqCount;

    return info.str();
}


class CMasternodeSync;
CMasternodeSync masternodeSync;

CMasternodeSync::CMasternodeSync()
{
    Reset();
}

bool CMasternodeSync::IsSynced()
{
    return RequestedMasternodeAssets == MASTERNODE_SYNC_FINISHED;
}

bool CMasternodeSync::IsBlockchainSynced()
{
    static bool fBlockchainSynced = false;
    static int64_t lastProcess = GetTime();

    // if the last call to this function was more than 60 minutes ago (client was in sleep mode) reset the sync process
    if (GetTime() - lastProcess > 60 * 60) {
        Reset();
        fBlockchainSynced = false;
    }
    lastProcess = GetTime();

    if (fBlockchainSynced) return true;

    if (fImporting || fReindex) return false;

    TRY_LOCK(cs_main, lockMain);
    if (!lockMain) return false;

    CBlockIndex* pindex = chainActive.Tip();
    if (pindex == NULL) return false;


    if (pindex->nTime + 60 * 60 < GetTime())
        return false;

    fBlockchainSynced = true;

    return true;
}

void CMasternodeSync::Reset()
{
    lastMasternodeList = 0;
    lastMasternodeWinner = 0;
    lastBudgetItem = 0;
    mapSeenSyncMNB.clear();
    mapSeenSyncMNW.clear();
    mapSeenSyncBudget.clear();
    lastFailure = 0;
    nCountFailures = 0;
    sumMasternodeList = 0;
    sumMasternodeWinner = 0;
    sumBudgetItemProp = 0;
    sumBudgetItemFin = 0;
    countMasternodeList = 0;
    countMasternodeWinner = 0;
    countBudgetItemProp = 0;
    countBudgetItemFin = 0;
    RequestedMasternodeAssets = MASTERNODE_SYNC_INITIAL;
    RequestedMasternodeAttempt = 0;
    nAssetSyncStarted = GetTime();
}

void CMasternodeSync::AddedMasternodeList(uint256 hash)
{
    if (mnodeman.mapSeenMasternodeBroadcast.count(hash)) {
        if (mapSeenSyncMNB[hash] < MASTERNODE_SYNC_THRESHOLD) {
            lastMasternodeList = GetTime();
            mapSeenSyncMNB[hash]++;
        }
    } else {
        lastMasternodeList = GetTime();
        mapSeenSyncMNB.insert(std::make_pair(hash, 1));
    }
}

void CMasternodeSync::AddedMasternodeWinner(uint256 hash)
{
    if (masternodePayments.mapMasternodePayeeVotes.count(hash)) {
        if (mapSeenSyncMNW[hash] < MASTERNODE_SYNC_THRESHOLD) {
            lastMasternodeWinner = GetTime();
            mapSeenSyncMNW[hash]++;
        }
    } else {
        lastMasternodeWinner = GetTime();
        mapSeenSyncMNW.insert(std::make_pair(hash, 1));
    }
}

void CMasternodeSync::AddedBudgetItem(uint256 hash)
{
}

bool CMasternodeSync::IsBudgetPropEmpty()
{
    return sumBudgetItemProp == 0 && countBudgetItemProp > 0;
}

bool CMasternodeSync::IsBudgetFinEmpty()
{
    return sumBudgetItemFin == 0 && countBudgetItemFin > 0;
}

void CMasternodeSync::GetNextAsset()
{
    switch (RequestedMasternodeAssets) {
    case (MASTERNODE_SYNC_INITIAL):
    case (MASTERNODE_SYNC_FAILED): // should never be used here actually, use Reset() instead
        ClearFulfilledRequest();
        RequestedMasternodeAssets = MASTERNODE_SYNC_LIST;
        break;
    case (MASTERNODE_SYNC_LIST):
        RequestedMasternodeAssets = MASTERNODE_SYNC_MNW;
        break;
    case (MASTERNODE_SYNC_MNW):
        RequestedMasternodeAssets = MASTERNODE_SYNC_FINISHED;
        break;
    }
    RequestedMasternodeAttempt = 0;
    nAssetSyncStarted = GetTime();
}

std::string CMasternodeSync::GetSyncStatus()
{
    switch (masternodeSync.RequestedMasternodeAssets) {
    case MASTERNODE_SYNC_INITIAL:
        return _("Synchronization pending...");
    case MASTERNODE_SYNC_LIST:
        return _("Synchronizing masternodes...");
    case MASTERNODE_SYNC_MNW:
        return _("Synchronizing masternode winners...");
    case MASTERNODE_SYNC_FAILED:
        return _("Synchronization failed");
    case MASTERNODE_SYNC_FINISHED:
        return _("Synchronization finished");
    }
    return "";
}

void CMasternodeSync::ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv)
{
    if (strCommand == "ssc") { //Sync status count
        int nItemID;
        int nCount;
        vRecv >> nItemID >> nCount;

        if (RequestedMasternodeAssets >= MASTERNODE_SYNC_FINISHED) return;

        //this means we will receive no further communication
        switch (nItemID) {
        case (MASTERNODE_SYNC_LIST):
            if (nItemID != RequestedMasternodeAssets) return;
            sumMasternodeList += nCount;
            countMasternodeList++;
            break;
        case (MASTERNODE_SYNC_MNW):
            if (nItemID != RequestedMasternodeAssets) return;
            sumMasternodeWinner += nCount;
            countMasternodeWinner++;
            break;
        }

        LogPrint(BCLog::MASTERNODE, "CMasternodeSync:ProcessMessage - ssc - got inventory count %d %d\n", nItemID, nCount);
    }
}

void CMasternodeSync::ClearFulfilledRequest()
{
    g_connman->ForEachNode([=](CNode* pnode) {
        pnode->ClearFulfilledRequest("mnsync");
        pnode->ClearFulfilledRequest("mnwsync");
    });
}

void CMasternodeSync::Process()
{
    static int tick = 0;

    if (tick++ % MASTERNODE_SYNC_TIMEOUT != 0) return;

    if (IsSynced()) {
        /* 
            Resync if we lose all masternodes from sleep/wake or failure to sync originally
        */
        if (mnodeman.CountEnabled() == 0) {
            Reset();
        } else
            return;
    }

    //try syncing again
    if (RequestedMasternodeAssets == MASTERNODE_SYNC_FAILED && lastFailure + (1 * 60) < GetTime()) {
        Reset();
    } else if (RequestedMasternodeAssets == MASTERNODE_SYNC_FAILED) {
        return;
    }

    LogPrint(BCLog::MASTERNODE, "CMasternodeSync::Process() - tick %d RequestedMasternodeAssets %d\n", tick, RequestedMasternodeAssets);

    if (RequestedMasternodeAssets == MASTERNODE_SYNC_INITIAL) GetNextAsset();

    // sporks synced but blockchain is not, wait until we're almost at a recent block to continue
    if (Params().NetworkIDString() != CBaseChainParams::REGTEST &&
        !IsBlockchainSynced() && RequestedMasternodeAssets > MASTERNODE_SYNC_LIST) return;


    g_connman->ForEachNode([&, this](CNode* pnode) {
        if (Params().NetworkIDString() == CBaseChainParams::REGTEST) {
            if (RequestedMasternodeAttempt < 2) {
                mnodeman.DsegUpdate(pnode);
            } else if (RequestedMasternodeAttempt < 4) {
                int nMnCount = mnodeman.CountEnabled();
                g_connman->PushMessage(pnode, CNetMsgMaker(pnode->nVersion).Make("mnget", nMnCount)); //sync payees
                uint256 n = 0;
                g_connman->PushMessage(pnode, CNetMsgMaker(pnode->nVersion).Make("mnvs", n)); //sync masternode votes
            } else {
                RequestedMasternodeAssets = MASTERNODE_SYNC_FINISHED;
            }
            RequestedMasternodeAttempt++;
            return;
        }

        if (pnode->nVersion >= masternodePayments.GetMinMasternodePaymentsProto()) {
            if (RequestedMasternodeAssets == MASTERNODE_SYNC_LIST) {
                LogPrint(BCLog::MASTERNODE, "CMasternodeSync::Process() - lastMasternodeList %lld (GetTime() - MASTERNODE_SYNC_TIMEOUT) %lld\n", lastMasternodeList, GetTime() - MASTERNODE_SYNC_TIMEOUT);
                if (gArgs.GetBoolArg("-masternode", false) || lastMasternodeList > 0 && lastMasternodeList < GetTime() - MASTERNODE_SYNC_TIMEOUT * 2 && RequestedMasternodeAttempt >= MASTERNODE_SYNC_THRESHOLD) { //hasn't received a new item in the last five seconds, so we'll move to the
                    GetNextAsset();
                    return;
                }

                if (!pnode->HasFulfilledRequest("mnsync")) {
                    pnode->FulfilledRequest("mnsync");

                    // timeout
                    if (lastMasternodeList == 0 &&
                        (RequestedMasternodeAttempt >= MASTERNODE_SYNC_THRESHOLD * 3 || GetTime() - nAssetSyncStarted > MASTERNODE_SYNC_TIMEOUT * 5)) {
                        GetNextAsset();
                        return;
                    }

                    if (RequestedMasternodeAttempt >= MASTERNODE_SYNC_THRESHOLD * 3) return;

                    mnodeman.DsegUpdate(pnode);
                    RequestedMasternodeAttempt++;
                    return;
                }
            }

            if (RequestedMasternodeAssets == MASTERNODE_SYNC_MNW) {
                if (gArgs.GetBoolArg("-masternode", false) || lastMasternodeWinner > 0 && lastMasternodeWinner < GetTime() - MASTERNODE_SYNC_TIMEOUT * 2 && RequestedMasternodeAttempt >= MASTERNODE_SYNC_THRESHOLD) { //hasn't received a new item in the last five seconds, so we'll move to the
                    GetNextAsset();
                    activeMasternode.ManageStatus();
                    return;
                }

                if (!pnode->HasFulfilledRequest("mnwsync")) {
                    pnode->FulfilledRequest("mnwsync");

                    // timeout
                    if (lastMasternodeWinner == 0 &&
                        (RequestedMasternodeAttempt >= MASTERNODE_SYNC_THRESHOLD * 3 || GetTime() - nAssetSyncStarted > MASTERNODE_SYNC_TIMEOUT * 5)) {
                        GetNextAsset();
                        activeMasternode.ManageStatus();
                        return;
                    }

                    if (RequestedMasternodeAttempt >= MASTERNODE_SYNC_THRESHOLD * 3) return;

                    CBlockIndex* pindexPrev = chainActive.Tip();
                    if (pindexPrev == NULL) return;

                    int nMnCount = mnodeman.CountEnabled();
                    g_connman->PushMessage(pnode, CNetMsgMaker(pnode->GetRecvVersion()).Make("mnget", nMnCount)); //sync payees
                    uint256 n = 0;
                    g_connman->PushMessage(pnode, CNetMsgMaker(pnode->GetRecvVersion()).Make("mnvs", n)); //sync masternode votes
                    RequestedMasternodeAttempt++;

                    return;
                }
            }
        }
    });
}



void ThreadMasternode()
{
    // Make this thread recognisable as the wallet flushing thread
    RenameThread("galaxycash-masternode");

    unsigned int c = 0;
    static int RequestedMasterNodeList = 0;
    while (true) {
        MilliSleep(1000);
        //LogPrintf("ThreadCheckObfuScationPool::check timeout\n");
        if (ShutdownRequested()) return;
        
        // try to sync from all available nodes, one step at a time
        {
            //TRY_LOCK(cs_main, lockMain);
            //if (!lockMain) continue;

            masternodeSync.Process();
            if (masternodeSync.IsBlockchainSynced()) {
                c++;

                // check if we should activate or ping every few minutes,
                // start right after sync is considered to be done
                if (c % MASTERNODE_PING_SECONDS == 1) activeMasternode.ManageStatus();

                if (c % 60 == 0) {
                    mnodeman.CheckAndRemove();
                    mnodeman.ProcessMasternodeConnections();
                    masternodePayments.CleanPaymentList();
                }

                //if(c % MASTERNODES_DUMP_SECONDS == 0) DumpMasternodes();
            }
        }
    }
}
