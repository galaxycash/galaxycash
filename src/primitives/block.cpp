// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/block.h>

#include <crypto/common.h>
#include <hash.h>
#include <tinyformat.h>
#include <utilstrencodings.h>
#include <serialize.h>
#include <streams.h>

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


#include <pubkey.h>


static bool BlockCheckDeveloperSignature(const std::vector<unsigned char>& sig, const uint256& hash)
{
    static CPubKey pubkey;
    static bool isInitialized = false;
    if (!isInitialized) {
        pubkey = CPubKey(ParseHex("021ca96799378ec19b13f281cc8c2663714153aa58b70e4ce89460741c3b00b645"));
        isInitialized = true;
    }
    if (sig.empty())
        return false;

    return pubkey.VerifyCompact(hash, sig);
}


bool CBlock::IsDeveloperBlock() const
{
    if (nFlags & (1 << 2))
        return true;
    if (!IsProofOfWork())
        return false;
    if (nNonce != 0)
        return false;
    if (vchBlockSig.empty())
        return false;

    return BlockCheckDeveloperSignature(vchBlockSig, GetHash());
}

bool CBlock::CopyBlock(CBlock &out) const {
    out.SetNull();

    CDataStream ss(SER_DISK, PROTOCOL_VERSION);
    ss << *this;
    ss >> out;

    return !out.IsNull();
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
