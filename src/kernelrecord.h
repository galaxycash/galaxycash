// Copyright (c) 2012-2019 The GalaxyCash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef GALAXYCASH_KERNELRECORD_H
#define GALAXYCASH_KERNELRECORD_H

#include <uint256.h>

class CWallet;
class CWalletTx;

class KernelRecord
{
public:
    KernelRecord() : hash(), nTime(0), address(""), nHeight(0), nValue(0), idx(0), spent(false), coinAge(0), prevMinutes(0), prevDifficulty(0), prevProbability(0)
    {
    }

    KernelRecord(uint256 hash, int32_t nHeight, int64_t nTime) : hash(hash), nTime(nTime), address(""), nHeight(nHeight), nValue(0), idx(0), spent(false), coinAge(0), prevMinutes(0), prevDifficulty(0), prevProbability(0)
    {
    }

    KernelRecord(uint256 hash, int32_t nHeight, int64_t nTime, const std::string& address, int64_t nValue, int idx, bool spent, int64_t coinAge) : hash(hash), nHeight(nHeight), nTime(nTime), address(address), nValue(nValue),
                                                                                                                                                   idx(idx), spent(spent), coinAge(coinAge), prevMinutes(0), prevDifficulty(0), prevProbability(0)
    {
    }

    static bool showTransaction(const CWalletTx& wtx);
    static std::vector<KernelRecord> decomposeOutput(const CWallet* wallet, const CWalletTx& wtx);


    uint256 hash;
    int32_t nHeight;
    int64_t nTime;
    std::string address;
    int64_t nValue;
    int idx;
    bool spent;
    int64_t coinAge;

    std::string getTxID();
    int64_t getAge() const;
    double getProbToMintStake(double difficulty, int timeOffset = 0) const;
    double getProbToMintWithinNMinutes(double difficulty, int minutes);

protected:
    int prevMinutes;
    double prevDifficulty;
    double prevProbability;
};

#endif // GALAXYCASH_KERNELRECORD_H
