// Copyright (c) 2017-2018 The GalaxyCash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "masternodeman.h"
#include "main.h"
#include "init.h"
#include "util.h"
#include "base58.h"
#include "coins.h"

CCoinMan coinman;


CCoinMan::CCoinMan()
{}

CCoinMan::~CCoinMan()
{}

bool CCoinMan::GetCoins(const CGalaxyCashAddress &holder, std::vector <CCoin> &vCoins)
{
    return false;
}
