// Copyright (c) 2016-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blockencodings.h>
#include <chainparams.h>
#include <consensus/consensus.h>
#include <consensus/validation.h>
#include <hash.h>
#include <random.h>
#include <streams.h>
#include <txmempool.h>
#include <util.h>
#include <validation.h>


#include <unordered_map>

ReadStatus FillBlock(CBlock& block, const std::vector<CTransactionRef>& txs)
{
    block.vtx.resize(txs.size());

    for (size_t i = 0; i < txs.size(); i++) {
        block.vtx[i] = txs[i];
    }

    CValidationState state;
    if (!CheckBlock(block, state, Params().GetConsensus())) {
        // TODO: We really want to just check merkle tree manually here,
        // but that is expensive, and CheckBlock caches a block's
        // "checked-status" (in the CBlock?). CBlock should be able to
        // check its own merkle root and cache that check.
        if (state.CorruptionPossible())
            return READ_STATUS_FAILED; // Possible Short ID collision
        return READ_STATUS_CHECKBLOCK_FAILED;
    }

    return READ_STATUS_OK;
}
