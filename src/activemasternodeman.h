
// Copyright (c) 2017-2018 The GalaxyCash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef GALAXYCASH_ACTIVEMASTERNODEMAN_H
#define GALAXYCASH_ACTIVEMASTERNODEMAN_H

#include "arith_uint256.h"
#include "sync.h"
#include "net.h"
#include "key.h"
#include "core.h"
#include "util.h"
#include "script.h"
#include "base58.h"
#include "main.h"
#include "activemasternode.h"


using namespace std;

class CActiveMasternodeMan;

extern CActiveMasternodeMan activemnodeman;


class CActiveMasternodeMan
{
private:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;

    // map to hold all MNs
    std::vector<CTxIn> vMasternodes;

public:
    CActiveMasternodeMan();
    ~CActiveMasternodeMan();

    int Register(CTxIn &vin);
    int Unregister(CTxIn &vin);
    int ManageStatus();
    int NumNodes() const;
    CTxIn GetVin(int i);
    CMasternode *GetMasternode(int i);
};

#endif
