// Copyright (c) 2019 The GalaxyCash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef GALAXYCASH_LIGHTWALLET_H
#define GALAXYCASH_LIGHTWALLET_H

#include "hash.h"

// Lightweight wallet implementation

class CLightCoin {
public:
    int64_t                     amount;
    uint256                     hash;
    uint32_t                    index;
    uint32_t                    height;
};


class CLightAddress {
public:
    uint160                     pubkey;
    std::vector <CLightCoin>    coins;
};


class CLightWallet {
public:
    int64_t                     balance;
    CLightAddress               primary, secondary;

    inline uint256              GetWalletHash() const {
        CHashWriter h(SER_GETHASH, PROTOCOL_VERSION);
        h << primary.pubkey;
        h << secondary.pubkey;
        return h.GetHash();
    }
};


class CLightWalletManager {
public:
    CLightWalletManager();
    ~CLightWalletManager();
};

//extern CLightWalletManager      lightWalletMan;

#endif
