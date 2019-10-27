// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/block.h>

#include <crypto/common.h>
#include <hash.h>
#include <tinyformat.h>
#include <utilstrencodings.h>

uint256 CBlockHeader::GetHash() const
{
    switch (nVersion) {
    case X11_VERSION:
        return HashX11(BEGIN(nVersion), END(nNonce));
    case X13_VERSION:
        return HashX13(BEGIN(nVersion), END(nNonce));
    case SHA256D_VERSION:
        return Hash(BEGIN(nVersion), END(nNonce));
    case BLAKE2S_VERSION:
        return HashBlake2s(BEGIN(nVersion), END(nNonce));
    default:
        return HashX12(BEGIN(nVersion), END(nNonce));
    }
}

unsigned int CBlockHeader::GetStakeEntropyBit() const
{
    // Take last bit of block hash as entropy bit
    unsigned int nEntropyBit = (GetHash().GetLow64() & 1llu);
    return nEntropyBit;
}

unsigned int CBlock::GetStakeEntropyBit() const
{
    // Take last bit of block hash as entropy bit
    unsigned int nEntropyBit = (GetHash().GetLow64() & 1llu);
    return nEntropyBit;
}

extern bool IsDeveloperBlock(const CBlock& block);

bool CBlock::IsDeveloperBlock() const
{
    return ::IsDeveloperBlock(*this);
}

std::string CBlock::ToString() const
{
    std::stringstream s;
    s << strprintf("CBlock(hash=%s, ver=0x%08x, hashPrevBlock=%s, hashMerkleRoot=%s, nTime=%u, nBits=%08x, nNonce=%u, nFlags=%08x, vtx=%u)\n",
        GetHash().ToString(),
        nVersion,
        hashPrevBlock.ToString(),
        hashMerkleRoot.ToString(),
        nTime, nBits, nNonce,
        nFlags, vtx.size());
    for (const auto& tx : vtx) {
        s << "  " << tx->ToString() << "\n";
    }
    return s.str();
}
