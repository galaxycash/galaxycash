// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Copyright (c) 2017 The GalaxyCash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef GALAXYCASH_HASH_H
#define GALAXYCASH_HASH_H

#include "uint256.h"
#include "serialize.h"

#include "crypto/sph_blake.h"
#include "crypto/sph_bmw.h"
#include "crypto/sph_groestl.h"
#include "crypto/sph_jh.h"
#include "crypto/sph_keccak.h"
#include "crypto/sph_skein.h"
#include "crypto/sph_luffa.h"
#include "crypto/sph_cubehash.h"
#include "crypto/sph_shavite.h"
#include "crypto/sph_simd.h"
#include "crypto/sph_echo.h"
#include "crypto/sph_sha0.h"
#include "crypto/sph_sha1.h"
#include "crypto/sph_sha2.h"
#include "crypto/sph_hamsi.h"
#include "crypto/sph_fugue.h"
#include "crypto/sph_panama.h"
#include "crypto/sph_ripemd.h"
#include "crypto/blake2.h"


/* ----------- Sha0 Hash ------------------------------------------------- */
/** A hasher class for SHA0 hash. */
class CSha0
{
public:
    sph_sha0_context ctx;

    static const size_t OUTPUT_SIZE = 20;

    CSha0() {
        sph_sha0_init(&ctx);
    }

    void Finalize(unsigned char hash[OUTPUT_SIZE]) {
        sph_sha0_close(&ctx, hash);
    }

    CSha0& Write(const unsigned char *data, size_t len) {
        sph_sha0(&ctx, data, len);
        return *this;
    }

    CSha0& Reset() {
        sph_sha0_init(&ctx);
        return *this;
    }
};

/* ----------- Sha1 Hash ------------------------------------------------- */
/** A hasher class for SHA1 hash. */
class CSha1
{
public:
    sph_sha1_context ctx;

    static const size_t OUTPUT_SIZE = 20;

    CSha1() {
        sph_sha1_init(&ctx);
    }

    void Finalize(unsigned char hash[OUTPUT_SIZE]) {
        sph_sha1_close(&ctx, hash);
    }

    CSha1& Write(const unsigned char *data, size_t len) {
        sph_sha1(&ctx, data, len);
        return *this;
    }

    CSha1& Reset() {
        sph_sha1_init(&ctx);
        return *this;
    }
};

/* ----------- Sha256 Hash ------------------------------------------------- */
/** A hasher class for Bitcoin's 256-bit hash. */
class CSha256
{
public:
    sph_sha256_context ctx;

    static const size_t OUTPUT_SIZE = sizeof(uint256);

    CSha256() {
        sph_sha256_init(&ctx);
    }

    void Finalize(unsigned char hash[OUTPUT_SIZE]) {
        sph_sha256_close(&ctx, hash);
    }

    CSha256& Write(const unsigned char *data, size_t len) {
        sph_sha256(&ctx, data, len);
        return *this;
    }

    CSha256& Reset() {
        sph_sha256_init(&ctx);
        return *this;
    }
};

/* ----------- Sha512 Hash ------------------------------------------------- */
/** A hasher class for Sha 512-bit hash. */
class CSha512
{
public:
    sph_sha512_context ctx;

    static const size_t OUTPUT_SIZE = sizeof(uint512);

    CSha512() {
        sph_sha512_init(&ctx);
    }

    void Finalize(unsigned char hash[OUTPUT_SIZE]) {
        sph_sha512_close(&ctx, hash);
    }

    CSha512& Write(const unsigned char *data, size_t len) {
        sph_sha512(&ctx, data, len);
        return *this;
    }

    CSha512& Reset() {
        sph_sha512_init(&ctx);
        return *this;
    }
};


/* ----------- Ripemd160 Hash ------------------------------------------------- */
/** A hasher class for Ripemd 160-bit hash. */
class CRipemd160
{
public:
    sph_ripemd160_context ctx;

    static const size_t OUTPUT_SIZE = sizeof(uint160);

    CRipemd160() {
        sph_ripemd160_init(&ctx);
    }

    void Finalize(unsigned char hash[OUTPUT_SIZE]) {
        sph_ripemd160_close(&ctx, hash);
    }

    CRipemd160& Write(const unsigned char *data, size_t len) {
        sph_ripemd160(&ctx, data, len);
        return *this;
    }

    CRipemd160& Reset() {
        sph_ripemd160_init(&ctx);
        return *this;
    }
};

/* ----------- Bitcoin Hash ------------------------------------------------- */
/** A hasher class for Bitcoin's 256-bit hash (double SHA-256). */
class CHash256 {
public:
    CSha256 sha;

    static const size_t OUTPUT_SIZE = CSha256::OUTPUT_SIZE;

    void Finalize(unsigned char hash[OUTPUT_SIZE]) {
        unsigned char buf[sha.OUTPUT_SIZE];
        sha.Finalize(buf);
        sha.Reset().Write(buf, sha.OUTPUT_SIZE).Finalize(hash);
    }

    CHash256& Write(const unsigned char *data, size_t len) {
        sha.Write(data, len);
        return *this;
    }

    CHash256& Reset() {
        sha.Reset();
        return *this;
    }
};

/** A hasher class for Bitcoin's 160-bit hash (SHA-256 + RIPEMD-160). */
class CHash160 {
public:
    CSha256 sha;

    static const size_t OUTPUT_SIZE = CRipemd160::OUTPUT_SIZE;

    void Finalize(unsigned char hash[OUTPUT_SIZE]) {
        unsigned char buf[sha.OUTPUT_SIZE];
        sha.Finalize(buf);
        CRipemd160().Write(buf, sha.OUTPUT_SIZE).Finalize(hash);
    }

    CHash160& Write(const unsigned char *data, size_t len) {
        sha.Write(data, len);
        return *this;
    }

    CHash160& Reset() {
        sha.Reset();
        return *this;
    }
};

inline uint160 ripemd160_hash(const void *input, size_t inputLen)
{
    sph_ripemd160_context ctx;
    uint160 hash;
    sph_ripemd160_init(&ctx);
    sph_ripemd160 (&ctx, input, inputLen);
    sph_ripemd160_close(&ctx, static_cast<void*>(&hash));
    return hash;
}

inline uint256 sha256_hash(const void *input, size_t inputLen)
{
    sph_sha256_context ctx;
    uint256 hash;
    sph_sha256_init(&ctx);
    sph_sha256 (&ctx, input, inputLen);
    sph_sha256_close(&ctx, static_cast<void*>(&hash));
    return hash;
}

inline uint256 sha256d_hash(const void *input, size_t inputLen)
{
    sph_sha256_context ctx;
    uint256 hash;
    sph_sha256_init(&ctx);
    sph_sha256 (&ctx, input, inputLen);
    sph_sha256_close(&ctx, static_cast<void*>(&hash));

    return sha256_hash(&hash, sizeof(uint256));
}

inline uint512 sha512_hash(const void *input, size_t inputLen)
{
    sph_sha512_context ctx;
    uint512 hash;
    sph_sha512_init(&ctx);
    sph_sha512 (&ctx, input, inputLen);
    sph_sha512_close(&ctx, static_cast<void*>(&hash));
    return hash;
}

inline uint256 sha512trim_hash(const void *input, size_t inputLen)
{
    sph_sha512_context ctx;
    uint512 hash;
    sph_sha512_init(&ctx);
    sph_sha512 (&ctx, input, inputLen);
    sph_sha512_close(&ctx, static_cast<void*>(&hash));
    return hash.trim256();
}


/* ----------- Blake2s ------------------------------------------------ */
template<typename T1>
inline uint256 HashBlake2s(const T1 pbegin, const T1 pend)
{
    static unsigned char pblank[1];
    uint256 hash1;
    blake2s_state S[1];
    blake2s_init( S, BLAKE2S_OUTBYTES );
    blake2s_update( S, (pbegin == pend ? pblank : (unsigned char*)&pbegin[0]), (pend - pbegin) * sizeof(pbegin[0]) );
    blake2s_final( S, (unsigned char*)&hash1, BLAKE2S_OUTBYTES );
    return hash1;
}

/* ----------- X11 ------------------------------------------------ */
template<typename T1>
inline uint256 HashX11(const T1 pbegin, const T1 pend)

{
    sph_blake512_context     ctx_blake;
    sph_bmw512_context       ctx_bmw;
    sph_groestl512_context   ctx_groestl;
    sph_jh512_context        ctx_jh;
    sph_keccak512_context    ctx_keccak;
    sph_skein512_context     ctx_skein;
    sph_luffa512_context     ctx_luffa;
    sph_cubehash512_context  ctx_cubehash;
    sph_shavite512_context   ctx_shavite;
    sph_simd512_context      ctx_simd;
    sph_echo512_context      ctx_echo;
    static unsigned char     pblank[1];

    uint512 hash[11];

    sph_blake512_init(&ctx_blake);
    sph_blake512 (&ctx_blake, (pbegin == pend ? pblank : static_cast<const void*>(&pbegin[0])), (pend - pbegin) * sizeof(pbegin[0]));
    sph_blake512_close(&ctx_blake, static_cast<void*>(&hash[0]));

    sph_bmw512_init(&ctx_bmw);
    sph_bmw512 (&ctx_bmw, static_cast<const void*>(&hash[0]), 64);
    sph_bmw512_close(&ctx_bmw, static_cast<void*>(&hash[1]));

    sph_groestl512_init(&ctx_groestl);
    sph_groestl512 (&ctx_groestl, static_cast<const void*>(&hash[1]), 64);
    sph_groestl512_close(&ctx_groestl, static_cast<void*>(&hash[2]));

    sph_skein512_init(&ctx_skein);
    sph_skein512 (&ctx_skein, static_cast<const void*>(&hash[2]), 64);
    sph_skein512_close(&ctx_skein, static_cast<void*>(&hash[3]));

    sph_jh512_init(&ctx_jh);
    sph_jh512 (&ctx_jh, static_cast<const void*>(&hash[3]), 64);
    sph_jh512_close(&ctx_jh, static_cast<void*>(&hash[4]));

    sph_keccak512_init(&ctx_keccak);
    sph_keccak512 (&ctx_keccak, static_cast<const void*>(&hash[4]), 64);
    sph_keccak512_close(&ctx_keccak, static_cast<void*>(&hash[5]));

    sph_luffa512_init(&ctx_luffa);
    sph_luffa512 (&ctx_luffa, static_cast<void*>(&hash[5]), 64);
    sph_luffa512_close(&ctx_luffa, static_cast<void*>(&hash[6]));

    sph_cubehash512_init(&ctx_cubehash);
    sph_cubehash512 (&ctx_cubehash, static_cast<const void*>(&hash[6]), 64);
    sph_cubehash512_close(&ctx_cubehash, static_cast<void*>(&hash[7]));

    sph_shavite512_init(&ctx_shavite);
    sph_shavite512(&ctx_shavite, static_cast<const void*>(&hash[7]), 64);
    sph_shavite512_close(&ctx_shavite, static_cast<void*>(&hash[8]));

    sph_simd512_init(&ctx_simd);
    sph_simd512 (&ctx_simd, static_cast<const void*>(&hash[8]), 64);
    sph_simd512_close(&ctx_simd, static_cast<void*>(&hash[9]));

    sph_echo512_init(&ctx_echo);
    sph_echo512 (&ctx_echo, static_cast<const void*>(&hash[9]), 64);
    sph_echo512_close(&ctx_echo, static_cast<void*>(&hash[10]));

    return hash[10].trim256();
}

/* ----------- X12 ------------------------------------------------ */
template<typename T1>
inline uint256 HashX12(const T1 pbegin, const T1 pend)

{
    sph_blake512_context     ctx_blake;
    sph_bmw512_context       ctx_bmw;
    sph_groestl512_context   ctx_groestl;
    sph_jh512_context        ctx_jh;
    sph_keccak512_context    ctx_keccak;
    sph_skein512_context     ctx_skein;
    sph_luffa512_context     ctx_luffa;
    sph_cubehash512_context  ctx_cubehash;
    sph_shavite512_context   ctx_shavite;
    sph_simd512_context      ctx_simd;
    sph_echo512_context      ctx_echo;
    sph_hamsi512_context     ctx_hamsi;
    static unsigned char     pblank[1];

    uint512 hash[12];

    sph_blake512_init(&ctx_blake);
    sph_blake512 (&ctx_blake, (pbegin == pend ? pblank : static_cast<const void*>(&pbegin[0])), (pend - pbegin) * sizeof(pbegin[0]));
    sph_blake512_close(&ctx_blake, static_cast<void*>(&hash[0]));

    sph_bmw512_init(&ctx_bmw);
    sph_bmw512 (&ctx_bmw, static_cast<const void*>(&hash[0]), 64);
    sph_bmw512_close(&ctx_bmw, static_cast<void*>(&hash[1]));

    sph_luffa512_init(&ctx_luffa);
    sph_luffa512 (&ctx_luffa, static_cast<void*>(&hash[1]), 64);
    sph_luffa512_close(&ctx_luffa, static_cast<void*>(&hash[2]));

    sph_cubehash512_init(&ctx_cubehash);
    sph_cubehash512 (&ctx_cubehash, static_cast<const void*>(&hash[2]), 64);
    sph_cubehash512_close(&ctx_cubehash, static_cast<void*>(&hash[3]));

    sph_shavite512_init(&ctx_shavite);
    sph_shavite512(&ctx_shavite, static_cast<const void*>(&hash[3]), 64);
    sph_shavite512_close(&ctx_shavite, static_cast<void*>(&hash[4]));

    sph_simd512_init(&ctx_simd);
    sph_simd512 (&ctx_simd, static_cast<const void*>(&hash[4]), 64);
    sph_simd512_close(&ctx_simd, static_cast<void*>(&hash[5]));

    sph_echo512_init(&ctx_echo);
    sph_echo512 (&ctx_echo, static_cast<const void*>(&hash[5]), 64);
    sph_echo512_close(&ctx_echo, static_cast<void*>(&hash[6]));

    sph_groestl512_init(&ctx_groestl);
    sph_groestl512 (&ctx_groestl, static_cast<const void*>(&hash[6]), 64);
    sph_groestl512_close(&ctx_groestl, static_cast<void*>(&hash[7]));

    sph_skein512_init(&ctx_skein);
    sph_skein512 (&ctx_skein, static_cast<const void*>(&hash[7]), 64);
    sph_skein512_close(&ctx_skein, static_cast<void*>(&hash[8]));

    sph_jh512_init(&ctx_jh);
    sph_jh512 (&ctx_jh, static_cast<const void*>(&hash[8]), 64);
    sph_jh512_close(&ctx_jh, static_cast<void*>(&hash[9]));

    sph_keccak512_init(&ctx_keccak);
    sph_keccak512 (&ctx_keccak, static_cast<const void*>(&hash[9]), 64);
    sph_keccak512_close(&ctx_keccak, static_cast<void*>(&hash[10]));

    sph_hamsi512_init(&ctx_hamsi);
    sph_hamsi512 (&ctx_hamsi, static_cast<const void*>(&hash[10]), 64);
    sph_hamsi512_close(&ctx_hamsi, static_cast<void*>(&hash[11]));

    return hash[11].trim256();
}

/* ----------- X13 ------------------------------------------------ */
template<typename T1>
inline uint256 HashX13(const T1 pbegin, const T1 pend)

{
    sph_blake512_context     ctx_blake;
    sph_bmw512_context       ctx_bmw;
    sph_groestl512_context   ctx_groestl;
    sph_jh512_context        ctx_jh;
    sph_keccak512_context    ctx_keccak;
    sph_skein512_context     ctx_skein;
    sph_luffa512_context     ctx_luffa;
    sph_cubehash512_context  ctx_cubehash;
    sph_shavite512_context   ctx_shavite;
    sph_simd512_context      ctx_simd;
    sph_echo512_context      ctx_echo;
    sph_hamsi512_context     ctx_hamsi;
    sph_fugue512_context     ctx_fugue;
    static unsigned char     pblank[1];

    uint512 hash[13];

    sph_blake512_init(&ctx_blake);
    sph_blake512 (&ctx_blake, (pbegin == pend ? pblank : static_cast<const void*>(&pbegin[0])), (pend - pbegin) * sizeof(pbegin[0]));
    sph_blake512_close(&ctx_blake, static_cast<void*>(&hash[0]));

    sph_bmw512_init(&ctx_bmw);
    sph_bmw512 (&ctx_bmw, static_cast<const void*>(&hash[0]), 64);
    sph_bmw512_close(&ctx_bmw, static_cast<void*>(&hash[1]));

    sph_groestl512_init(&ctx_groestl);
    sph_groestl512 (&ctx_groestl, static_cast<const void*>(&hash[1]), 64);
    sph_groestl512_close(&ctx_groestl, static_cast<void*>(&hash[2]));

    sph_skein512_init(&ctx_skein);
    sph_skein512 (&ctx_skein, static_cast<const void*>(&hash[2]), 64);
    sph_skein512_close(&ctx_skein, static_cast<void*>(&hash[3]));

    sph_jh512_init(&ctx_jh);
    sph_jh512 (&ctx_jh, static_cast<const void*>(&hash[3]), 64);
    sph_jh512_close(&ctx_jh, static_cast<void*>(&hash[4]));

    sph_keccak512_init(&ctx_keccak);
    sph_keccak512 (&ctx_keccak, static_cast<const void*>(&hash[4]), 64);
    sph_keccak512_close(&ctx_keccak, static_cast<void*>(&hash[5]));

    sph_luffa512_init(&ctx_luffa);
    sph_luffa512 (&ctx_luffa, static_cast<void*>(&hash[5]), 64);
    sph_luffa512_close(&ctx_luffa, static_cast<void*>(&hash[6]));

    sph_cubehash512_init(&ctx_cubehash);
    sph_cubehash512 (&ctx_cubehash, static_cast<const void*>(&hash[6]), 64);
    sph_cubehash512_close(&ctx_cubehash, static_cast<void*>(&hash[7]));

    sph_shavite512_init(&ctx_shavite);
    sph_shavite512(&ctx_shavite, static_cast<const void*>(&hash[7]), 64);
    sph_shavite512_close(&ctx_shavite, static_cast<void*>(&hash[8]));

    sph_simd512_init(&ctx_simd);
    sph_simd512 (&ctx_simd, static_cast<const void*>(&hash[8]), 64);
    sph_simd512_close(&ctx_simd, static_cast<void*>(&hash[9]));

    sph_echo512_init(&ctx_echo);
    sph_echo512 (&ctx_echo, static_cast<const void*>(&hash[9]), 64);
    sph_echo512_close(&ctx_echo, static_cast<void*>(&hash[10]));

    sph_hamsi512_init(&ctx_hamsi);
    sph_hamsi512 (&ctx_hamsi, static_cast<const void*>(&hash[10]), 64);
    sph_hamsi512_close(&ctx_hamsi, static_cast<void*>(&hash[11]));

    sph_fugue512_init(&ctx_fugue);
    sph_fugue512 (&ctx_fugue, static_cast<const void*>(&hash[11]), 64);
    sph_fugue512_close(&ctx_fugue, static_cast<void*>(&hash[12]));

    return hash[12].trim256();
}

template<typename T1>
inline uint160 HashRipemd160(const T1 pbegin, const T1 pend)
{
    sph_ripemd160_context ctx;
    static unsigned char pblank[1];
    uint160 hash;

    sph_ripemd160_init(&ctx);
    sph_ripemd160 (&ctx, (pbegin == pend ? pblank : static_cast<const void*>(&pbegin[0])), (pend - pbegin) * sizeof(pbegin[0]));
    sph_ripemd160_close(&ctx, static_cast<void*>(&hash));

    return hash;
}

inline void Sha0Hash(const void *data, const size_t length, void *output)
{
    sph_sha0_context ctx;
    sph_sha0_init(&ctx);
    sph_sha0(&ctx, data, length);
    sph_sha0_close(&ctx, output);
}

inline void Sha1Hash(const void *data, const size_t length, void *output)
{
    sph_sha1_context ctx;
    sph_sha1_init(&ctx);
    sph_sha1(&ctx, data, length);
    sph_sha1_close(&ctx, output);
}

inline void Ripemd160Hash(const void *data, const size_t length, void *output)
{
    sph_ripemd160_context ctx;
    sph_ripemd160_init(&ctx);
    sph_ripemd160(&ctx, data, length);
    sph_ripemd160_close(&ctx, output);
}

template<typename T1>
inline uint256 HashSha256(const T1 pbegin, const T1 pend)
{
    sph_sha256_context ctx;
    static unsigned char pblank[1];
    uint256 hash;

    sph_sha256_init(&ctx);
    sph_sha256 (&ctx, (pbegin == pend ? pblank : static_cast<const void*>(&pbegin[0])), (pend - pbegin) * sizeof(pbegin[0]));
    sph_sha256_close(&ctx, static_cast<void*>(&hash));

    return hash;
}

inline void Sha256Hash(const void *data, const size_t length, void *output)
{
    sph_sha256_context ctx;
    sph_sha256_init(&ctx);
    sph_sha256(&ctx, data, length);
    sph_sha256_close(&ctx, output);
}

template<typename T1>
inline uint256 HashSha256D(const T1 pbegin, const T1 pend)
{
    sph_sha256_context ctx;
    static unsigned char pblank[1];
    uint256 hashA, hashB;

    sph_sha256_init(&ctx);
    sph_sha256 (&ctx, (pbegin == pend ? pblank : static_cast<const void*>(&pbegin[0])), (pend - pbegin) * sizeof(pbegin[0]));
    sph_sha256_close(&ctx, static_cast<void*>(&hashA));

    sph_sha256_init(&ctx);
    sph_sha256 (&ctx, static_cast<const void*>(&hashA), sizeof(uint256));
    sph_sha256_close(&ctx, static_cast<void*>(&hashB));

    return hashB;
}

inline void Sha256DHash(const void *data, const size_t length, void *output)
{
    uint256 hash = sha256_hash(data, length);
    sph_sha256_context ctx;
    sph_sha256_init(&ctx);
    sph_sha256(&ctx, &hash, sizeof(uint256));
    sph_sha256_close(&ctx, output);
}

template<typename T1>
inline uint512 HashSha512(const T1 pbegin, const T1 pend)
{
    sph_sha512_context ctx;
    static unsigned char pblank[1];
    uint512 hash;

    sph_sha512_init(&ctx);
    sph_sha512 (&ctx, (pbegin == pend ? pblank : static_cast<const void*>(&pbegin[0])), (pend - pbegin) * sizeof(pbegin[0]));
    sph_sha512_close(&ctx, static_cast<void*>(&hash));

    return hash;
}

inline void Sha512Hash(const void *data, const size_t length, void *output)
{
    sph_sha512_context ctx;
    sph_sha512_init(&ctx);
    sph_sha512(&ctx, data, length);
    sph_sha512_close(&ctx, output);
}

template<typename T1>
inline uint256 HashSha512Trim(const T1 pbegin, const T1 pend)
{
    sph_sha512_context ctx;
    static unsigned char pblank[1];
    uint512 hash;

    sph_sha512_init(&ctx);
    sph_sha512 (&ctx, (pbegin == pend ? pblank : static_cast<const void*>(&pbegin[0])), (pend - pbegin) * sizeof(pbegin[0]));
    sph_sha512_close(&ctx, static_cast<void*>(&hash));

    return hash.trim256();
}


/** Compute the 256-bit hash of an object. */
template<typename T1>
inline uint256 Hash(const T1 pbegin, const T1 pend)
{
    static const unsigned char pblank[1] = {};
    uint256 result;
    CHash256().Write(pbegin == pend ? pblank : (const unsigned char*)&pbegin[0], (pend - pbegin) * sizeof(pbegin[0]))
              .Finalize((unsigned char*)&result);
    return result;
}

/** Compute the 256-bit hash of the concatenation of two objects. */
template<typename T1, typename T2>
inline uint256 Hash(const T1 p1begin, const T1 p1end,
                    const T2 p2begin, const T2 p2end) {
    static const unsigned char pblank[1] = {};
    uint256 result;
    CHash256().Write(p1begin == p1end ? pblank : (const unsigned char*)&p1begin[0], (p1end - p1begin) * sizeof(p1begin[0]))
              .Write(p2begin == p2end ? pblank : (const unsigned char*)&p2begin[0], (p2end - p2begin) * sizeof(p2begin[0]))
              .Finalize((unsigned char*)&result);
    return result;
}

/** Compute the 256-bit hash of the concatenation of three objects. */
template<typename T1, typename T2, typename T3>
inline uint256 Hash(const T1 p1begin, const T1 p1end,
                    const T2 p2begin, const T2 p2end,
                    const T3 p3begin, const T3 p3end) {
    static const unsigned char pblank[1] = {};
    uint256 result;
    CHash256().Write(p1begin == p1end ? pblank : (const unsigned char*)&p1begin[0], (p1end - p1begin) * sizeof(p1begin[0]))
              .Write(p2begin == p2end ? pblank : (const unsigned char*)&p2begin[0], (p2end - p2begin) * sizeof(p2begin[0]))
              .Write(p3begin == p3end ? pblank : (const unsigned char*)&p3begin[0], (p3end - p3begin) * sizeof(p3begin[0]))
              .Finalize((unsigned char*)&result);
    return result;
}

/** Compute the 256-bit hash of the concatenation of three objects. */
template<typename T1, typename T2, typename T3, typename T4>
inline uint256 Hash(const T1 p1begin, const T1 p1end,
                    const T2 p2begin, const T2 p2end,
                    const T3 p3begin, const T3 p3end,
                    const T4 p4begin, const T4 p4end) {
    static const unsigned char pblank[1] = {};
    uint256 result;
    CHash256().Write(p1begin == p1end ? pblank : (const unsigned char*)&p1begin[0], (p1end - p1begin) * sizeof(p1begin[0]))
              .Write(p2begin == p2end ? pblank : (const unsigned char*)&p2begin[0], (p2end - p2begin) * sizeof(p2begin[0]))
              .Write(p3begin == p3end ? pblank : (const unsigned char*)&p3begin[0], (p3end - p3begin) * sizeof(p3begin[0]))
              .Write(p4begin == p4end ? pblank : (const unsigned char*)&p4begin[0], (p4end - p4begin) * sizeof(p4begin[0]))
              .Finalize((unsigned char*)&result);
    return result;
}

/** Compute the 256-bit hash of the concatenation of three objects. */
template<typename T1, typename T2, typename T3, typename T4, typename T5>
inline uint256 Hash(const T1 p1begin, const T1 p1end,
                    const T2 p2begin, const T2 p2end,
                    const T3 p3begin, const T3 p3end,
                    const T4 p4begin, const T4 p4end,
                    const T5 p5begin, const T5 p5end) {
    static const unsigned char pblank[1] = {};
    uint256 result;
    CHash256().Write(p1begin == p1end ? pblank : (const unsigned char*)&p1begin[0], (p1end - p1begin) * sizeof(p1begin[0]))
              .Write(p2begin == p2end ? pblank : (const unsigned char*)&p2begin[0], (p2end - p2begin) * sizeof(p2begin[0]))
              .Write(p3begin == p3end ? pblank : (const unsigned char*)&p3begin[0], (p3end - p3begin) * sizeof(p3begin[0]))
              .Write(p4begin == p4end ? pblank : (const unsigned char*)&p4begin[0], (p4end - p4begin) * sizeof(p4begin[0]))
              .Write(p5begin == p5end ? pblank : (const unsigned char*)&p5begin[0], (p5end - p5begin) * sizeof(p5begin[0]))
              .Finalize((unsigned char*)&result);
    return result;
}

/** Compute the 256-bit hash of the concatenation of three objects. */
template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
inline uint256 Hash(const T1 p1begin, const T1 p1end,
                    const T2 p2begin, const T2 p2end,
                    const T3 p3begin, const T3 p3end,
                    const T4 p4begin, const T4 p4end,
                    const T5 p5begin, const T5 p5end,
                    const T6 p6begin, const T6 p6end) {
    static const unsigned char pblank[1] = {};
    uint256 result;
    CHash256().Write(p1begin == p1end ? pblank : (const unsigned char*)&p1begin[0], (p1end - p1begin) * sizeof(p1begin[0]))
              .Write(p2begin == p2end ? pblank : (const unsigned char*)&p2begin[0], (p2end - p2begin) * sizeof(p2begin[0]))
              .Write(p3begin == p3end ? pblank : (const unsigned char*)&p3begin[0], (p3end - p3begin) * sizeof(p3begin[0]))
              .Write(p4begin == p4end ? pblank : (const unsigned char*)&p4begin[0], (p4end - p4begin) * sizeof(p4begin[0]))
              .Write(p5begin == p5end ? pblank : (const unsigned char*)&p5begin[0], (p5end - p5begin) * sizeof(p5begin[0]))
              .Write(p6begin == p6end ? pblank : (const unsigned char*)&p6begin[0], (p6end - p6begin) * sizeof(p6begin[0]))
              .Finalize((unsigned char*)&result);
    return result;
}

/** Compute the 160-bit hash an object. */
template<typename T1>
inline uint160 Hash160(const T1 pbegin, const T1 pend)
{
    static unsigned char pblank[1] = {};
    uint160 result;
    CHash160().Write(pbegin == pend ? pblank : (const unsigned char*)&pbegin[0], (pend - pbegin) * sizeof(pbegin[0]))
              .Finalize((unsigned char*)&result);
    return result;
}

/** Compute the 160-bit hash of a vector. */
inline uint160 Hash160(const std::vector<unsigned char>& vch)
{
    return Hash160(vch.begin(), vch.end());
}

/** A writer stream (for serialization) that computes a 256-bit hash. */
class CHashWriter
{
public:
    CHash256 ctx;
    int nType;
    int nVersion;

    CHashWriter(int nTypeIn, int nVersionIn) : nType(nTypeIn), nVersion(nVersionIn) {}

    CHashWriter& write(const char *pch, size_t size) {
        ctx.Write((const unsigned char*)pch, size);
        return (*this);
    }

    // invalidates the object
    uint256 GetHash() {
        uint256 result;
        ctx.Finalize((unsigned char*)&result);
        return result;
    }

    template<typename T>
    CHashWriter& operator<<(const T& obj) {
        // Serialize to this stream
        ::Serialize(*this, obj, nType, nVersion);
        return (*this);
    }
};

/** Compute the 256-bit hash of an object's serialization. */
template<typename T>
uint256 SerializeHash(const T& obj, int nType=SER_GETHASH, int nVersion=PROTOCOL_VERSION)
{
    CHashWriter ss(nType, nVersion);
    ss << obj;
    return ss.GetHash();
}


/** A hasher class for HMAC-SHA-256. */
class CHmacSha256
{
public:
    CSha256 outer;
    CSha256 inner;

    static const size_t OUTPUT_SIZE = 32;

    CHmacSha256(const unsigned char* key, size_t keylen);
    inline CHmacSha256(const char *key, size_t keylen) : CHmacSha256((const unsigned char *) key, keylen)
    {}

    CHmacSha256& Init(const unsigned char* key, size_t keylen);
    CHmacSha256& Write(const unsigned char* data, size_t len)
    {
        inner.Write(data, len);
        return *this;
    }
    void Finalize(unsigned char hash[OUTPUT_SIZE]);
};

/** A hasher class for HMAC-SHA-512. */
class CHmacSha512
{
public:
    CSha512 outer;
    CSha512 inner;

    static const size_t OUTPUT_SIZE = 64;

    CHmacSha512(const unsigned char* key, size_t keylen);
    inline CHmacSha512(const char *key, size_t keylen) : CHmacSha512((const unsigned char *) key, keylen)
    {}

    CHmacSha512& Init(const unsigned char* key, size_t keylen);
    CHmacSha512& Write(const unsigned char* data, size_t len)
    {
        inner.Write(data, len);
        return *this;
    }
    void Finalize(unsigned char hash[OUTPUT_SIZE]);
};

inline void BIP32Hash(const unsigned char chainCode[32], unsigned int nChild, unsigned char header, const unsigned char data[32], unsigned char output[64]) {
    unsigned char num[4];
    num[0] = (nChild >> 24) & 0xFF;
    num[1] = (nChild >> 16) & 0xFF;
    num[2] = (nChild >>  8) & 0xFF;
    num[3] = (nChild >>  0) & 0xFF;
    CHmacSha512 ctx(chainCode, 32);
    ctx.Write(&header, 1);
    ctx.Write(data, 32);
    ctx.Write(num, 4);
    ctx.Finalize(output);
}

#endif

