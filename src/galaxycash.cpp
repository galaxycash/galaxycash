// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <galaxycash.h>

#include <chainparams.h>
#include <hash.h>
#include <memory>
#include <pow.h>
#include <uint256.h>
#include <util.h>

#include <stdint.h>


CGalaxyCashStateRef g_galaxycash;


CGalaxyCashDB::CGalaxyCashDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "galaxycash" / "database", nCacheSize, fMemory, fWipe)
{
}


class CGalaxyCashVM
{
public:
    CGalaxyCashVM();
    ~CGalaxyCashVM();
};

CGalaxyCashVM::CGalaxyCashVM()
{
}

CGalaxyCashState::CGalaxyCashState() : pdb(new CGalaxyCashDB((gArgs.GetArg("-gchdbcache", 128) << 20), false, gArgs.GetBoolArg("-reindex", false)))
{
}
CGalaxyCashState::~CGalaxyCashState()
{
    delete pdb;
}

void CGalaxyCashState::EvalScript()
{
}

bool CGalaxyCashConsensus::CheckSignature() const
{
    return true;
}