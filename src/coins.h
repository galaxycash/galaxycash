// Copyright (c) 2017-2018 The GalaxyCash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef GALAXYCASH_COINS_PARAMS_H
#define GALAXYCASH_COINS_PARAMS_H

#include "base58.h"

class CCoin
{
public:
    unsigned int            nHeight;
    unsigned int            nOutput;
    bool                    fActive, fCoinbase;
    CGalaxyCashAddress      holder;
    uint256                 hash;

    inline CCoin() :
        nHeight(0),
        nOutput(0),
        fActive(false),
        fCoinbase(false)
    {}
    inline CCoin(const CCoin &coin) :
        nHeight(coin.nHeight),
        nOutput(coin.nOutput),
        fActive(coin.fActive),
        fCoinbase(coin.fCoinbase),
        holder(coin.holder),
        hash(coin.hash)
    {}

    unsigned int            NumConfirmations() const {
        return (nBestHeight - nHeight);
    }

    inline bool             IsActive() const {
        return fActive;
    }

    inline bool             IsCoinbase() const {
        return fCoinbase;
    }

};

class CCoinMan
{
private:
    // critical section to protect the inner data structures
    mutable CCriticalSection    cs;
public:
    CCoinMan();
    ~CCoinMan();

    bool                    IsBlockchainSynced() const;

    void                    GetActiveCoins(const CGalaxyCashAddress &holder, std::vector <CCoin> &vCoins);

    void                    ProcessInput(CTxIn &in);
    void                    ProcessOutput(CTxOut &out);

    void                    ProcessBlock(const CBlockIndex *pindex, CBlock *pblock);
};

extern CCoinMan             coinman;

void                        ThreadCoins();

#endif
