// Copyright (c) 2012-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_VERSION_H
#define BITCOIN_VERSION_H

//
// database format versioning
//
static const int DATABASE_VERSION = 90909;

/**
 * network protocol versioning
 */

static const int PROTOCOL_VERSION = 94440;
static const int NEW_VERSION = 94000;            // galaxycash: used to communicate with clients knows about galaxycash new protocol
static const int OLD_VERSION = 90920;            // galaxycash: used to communicate with clients that don't know how to send PoS information in headers
static const int MIN_MASTERNODE_VERSION = 90920; // galaxycash: minimal masternode protocol version

//! initial proto version, to be increased after version/verack negotiation
static const int INIT_PROTO_VERSION = 90918;

//! In this version, 'getheaders' was introduced.
static const int GETHEADERS_VERSION = 31800;

//! disconnect from peers older than this proto version
static const int MIN_PEER_PROTO_VERSION = NEW_VERSION;

//! nTime field added to CAddress, starting with this version;
//! if possible, avoid requesting addresses nodes older than this
static const int CADDR_TIME_VERSION = 31402;

//! BIP 0031, pong message, is enabled for all versions AFTER this one
static const int BIP0031_VERSION = 60000;

//! "filter*" commands are disabled without NODE_BLOOM after and including this version
static const int NO_BLOOM_VERSION = 70011;

//! "sendheaders" command and announcing blocks with headers starts with this version
static const int SENDHEADERS_VERSION = 94000;

//! "feefilter" tells peers to filter invs to you by fee starts with this version
static const int FEEFILTER_VERSION = 70013;

//! short-id-based block download starts with this version
static const int SHORT_IDS_BLOCKS_VERSION = 70014;

//! not banning for invalid compact blocks starts with this version
static const int INVALID_CB_NO_BAN_VERSION = 70015;


// minimum peer version that can receive masternode payments
// V1 - Last protocol version before update
// V2 - Newest protocol version
static const int MIN_MASTERNODE_PAYMENT_PROTO_VERSION_1 = 90920;
static const int MIN_MASTERNODE_PAYMENT_PROTO_VERSION_2 = 90920;

// reject blocks with non-canonical signatures starting from this version
static const int CANONICAL_BLOCK_SIG_VERSION = 90920;
static const int CANONICAL_BLOCK_SIG_LOW_S_VERSION = 90920;

//! masternodes older than this proto version use old strMessage format for mnannounce
static const int MIN_PEER_MNANNOUNCE = 94000;

#endif // BITCOIN_VERSION_H
