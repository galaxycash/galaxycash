// Copyright (c) 2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "base58.h"

#include "hash.h"
#include "uint256.h"
#include "chainparams.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <vector>
#include <string>
#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/static_visitor.hpp>

/** base58-encoded Bitcoin addresses.
 * Public-key-hash-addresses have version 0 (or 111 testnet).
 * The data vector contains RIPEMD160(SHA256(pubkey)), where pubkey is the serialized public key.
 * Script-hash-addresses have version 5 (or 196 testnet).
 * The data vector contains RIPEMD160(SHA256(cscript)), where cscript is the serialized redemption script.
 */
CChainParams::Base58Type pubkey_address = (CChainParams::Base58Type)0;
CChainParams::Base58Type script_address = (CChainParams::Base58Type)5;
bool CBitcoinAddress::Set(const CKeyID &id) {
    SetData(Params().Base58Prefix(pubkey_address), &id, 20);
    return true;
}

bool CBitcoinAddress::Set(const CScriptID &id) {
    SetData(Params().Base58Prefix(script_address), &id, 20);
    return true;
}

bool CBitcoinAddress::Set(const CTxDestination &dest) {
    return boost::apply_visitor(CBitcoinAddressVisitor(this), dest);
}

bool CBitcoinAddress::IsValid() const {
    bool fCorrectSize = vchData.size() == 20;
    bool fKnownVersion = vchVersion == Params().Base58Prefix(pubkey_address) ||
                         vchVersion == Params().Base58Prefix(script_address);
    return fCorrectSize && fKnownVersion;
}

CTxDestination CBitcoinAddress::Get() const {
    if (!IsValid())
        return CNoDestination();
    uint160 id;
    memcpy(&id, &vchData[0], 20);
    if (vchVersion == Params().Base58Prefix(pubkey_address))
        return CKeyID(id);
    else if (vchVersion == Params().Base58Prefix(script_address))
        return CScriptID(id);
    else
        return CNoDestination();
}

bool CBitcoinAddress::GetKeyID(CKeyID &keyID) const {
    if (!IsValid() || vchVersion != Params().Base58Prefix(pubkey_address))
        return false;
    uint160 id;
    memcpy(&id, &vchData[0], 20);
    keyID = CKeyID(id);
    return true;
}

bool CBitcoinAddress::IsScript() const {
    return IsValid() && vchVersion == Params().Base58Prefix(script_address);
}
