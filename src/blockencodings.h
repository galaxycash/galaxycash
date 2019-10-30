// Copyright (c) 2016-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_BLOCKENCODINGS_H
#define BITCOIN_BLOCKENCODINGS_H

#include <primitives/block.h>

#include <memory>

class CTxMemPool;

// Dumb helper to handle CTransaction compression at serialize-time
struct TransactionCompressor {
private:
    CTransactionRef& tx;

public:
    explicit TransactionCompressor(CTransactionRef& txIn) : tx(txIn) {}

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(tx); //TODO: Compress tx encoding
    }
};

class BlockTransactionsRequest
{
public:
    // A BlockTransactionsRequest message
    uint256 blockhash;
    std::vector<uint16_t> indexes;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(blockhash);
        uint64_t indexes_size = (uint64_t)indexes.size();
        READWRITE(COMPACTSIZE(indexes_size));
        if (ser_action.ForRead()) {
            size_t i = 0;
            while (indexes.size() < indexes_size) {
                indexes.resize(std::min((uint64_t)(1000 + indexes.size()), indexes_size));
                for (; i < indexes.size(); i++) {
                    uint64_t index = 0;
                    READWRITE(COMPACTSIZE(index));
                    if (index > std::numeric_limits<uint16_t>::max())
                        throw std::ios_base::failure("index overflowed 16 bits");
                    indexes[i] = index;
                }
            }

            uint16_t offset = 0;
            for (size_t j = 0; j < indexes.size(); j++) {
                if (uint64_t(indexes[j]) + uint64_t(offset) > std::numeric_limits<uint16_t>::max())
                    throw std::ios_base::failure("indexes overflowed 16 bits");
                indexes[j] = indexes[j] + offset;
                offset = indexes[j] + 1;
            }
        } else {
            for (size_t i = 0; i < indexes.size(); i++) {
                uint64_t index = indexes[i] - (i == 0 ? 0 : (indexes[i - 1] + 1));
                READWRITE(COMPACTSIZE(index));
            }
        }
    }
};

class BlockTransactions
{
public:
    // A BlockTransactions message
    uint256 blockhash;
    std::vector<CTransactionRef> txn;

    BlockTransactions() {}
    explicit BlockTransactions(const BlockTransactionsRequest& req) : blockhash(req.blockhash), txn(req.indexes.size()) {}

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(blockhash);
        uint64_t txn_size = (uint64_t)txn.size();
        READWRITE(COMPACTSIZE(txn_size));
        if (ser_action.ForRead()) {
            size_t i = 0;
            while (txn.size() < txn_size) {
                txn.resize(std::min((uint64_t)(1000 + txn.size()), txn_size));
                for (; i < txn.size(); i++)
                    READWRITE(REF(TransactionCompressor(txn[i])));
            }
        } else {
            for (size_t i = 0; i < txn.size(); i++)
                READWRITE(REF(TransactionCompressor(txn[i])));
        }
    }
};

// Dumb serialization/storage-helper for CBlockHeaderAndShortTxIDs and PartiallyDownloadedBlock
struct PrefilledTransaction {
    // Used as an offset since last prefilled tx in CBlockHeaderAndShortTxIDs,
    // as a proper transaction-in-block-index in PartiallyDownloadedBlock
    uint16_t index;
    CTransactionRef tx;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        uint64_t idx = index;
        READWRITE(COMPACTSIZE(idx));
        if (idx > std::numeric_limits<uint16_t>::max())
            throw std::ios_base::failure("index overflowed 16-bits");
        index = idx;
        READWRITE(REF(TransactionCompressor(tx)));
    }
};

typedef enum ReadStatus_t {
    READ_STATUS_OK,
    READ_STATUS_INVALID,           // Invalid object, peer is sending bogus crap
    READ_STATUS_FAILED,            // Failed to process object
    READ_STATUS_CHECKBLOCK_FAILED, // Used only by FillBlock to indicate a
                                   // failure in CheckBlock.
} ReadStatus;

ReadStatus FillBlock(CBlock& block, const std::vector<CTransactionRef>& vtx_missing);

#endif // BITCOIN_BLOCKENCODINGS_H
