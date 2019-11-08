// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UINT256_H
#define BITCOIN_UINT256_H

#include <assert.h>
#include <crypto/common.h>
#include <cstring>
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <vector>


class blob_error : public std::runtime_error
{
public:
    explicit blob_error(const std::string& str) : std::runtime_error(str) {}
};

/** Template base class for fixed-sized opaque blobs. */
template <unsigned int BITS>
class base_blob
{
protected:
    static constexpr int WIDTH = BITS / 8;
    static constexpr int WIDTH32 = BITS / 32;
    union {
        struct {
            uint8_t data[WIDTH];
        };
        struct {
            uint32_t pn[WIDTH32];
        };
    };

public:
    const uint32_t* GetDataPtr() const
    {
        return (const uint32_t*)data;
    }

    base_blob()
    {
        for (int i = 0; i < WIDTH32; i++)
            pn[i] = 0;
    }

    base_blob(const base_blob& b)
    {
        for (int i = 0; i < WIDTH32; i++)
            pn[i] = b.pn[i];
    }


    bool IsNull() const
    {
        for (int i = 0; i < WIDTH32; i++)
            if (pn[i] != 0)
                return false;
        return true;
    }

    void SetNull()
    {
        memset(pn, 0, sizeof(pn));
    }


    base_blob& operator=(const base_blob& b)
    {
        for (int i = 0; i < WIDTH32; i++)
            pn[i] = b.pn[i];
        return *this;
    }

    base_blob(uint64_t b)
    {
        pn[0] = (unsigned int)b;
        pn[1] = (unsigned int)(b >> 32);
        for (int i = 2; i < WIDTH32; i++)
            pn[i] = 0;
    }

    explicit base_blob(const std::string& str);
    explicit base_blob(const std::vector<unsigned char>& vch);

    bool operator!() const
    {
        for (int i = 0; i < WIDTH32; i++)
            if (pn[i] != 0)
                return false;
        return true;
    }


    const base_blob operator~() const
    {
        base_blob ret;
        for (int i = 0; i < WIDTH32; i++)
            ret.pn[i] = ~pn[i];
        return ret;
    }

    const base_blob operator-() const
    {
        base_blob ret;
        for (int i = 0; i < WIDTH32; i++)
            ret.pn[i] = ~pn[i];
        ret++;
        return ret;
    }

    double getdouble() const;

    base_blob& operator=(uint64_t b)
    {
        pn[0] = (unsigned int)b;
        pn[1] = (unsigned int)(b >> 32);
        for (int i = 2; i < WIDTH32; i++)
            pn[i] = 0;
        return *this;
    }

    base_blob& operator^=(const base_blob& b)
    {
        for (int i = 0; i < WIDTH32; i++)
            pn[i] ^= b.pn[i];
        return *this;
    }

    base_blob& operator&=(const base_blob& b)
    {
        for (int i = 0; i < WIDTH32; i++)
            pn[i] &= b.pn[i];
        return *this;
    }

    base_blob& operator|=(const base_blob& b)
    {
        for (int i = 0; i < WIDTH32; i++)
            pn[i] |= b.pn[i];
        return *this;
    }

    base_blob& operator^=(uint64_t b)
    {
        pn[0] ^= (unsigned int)b;
        pn[1] ^= (unsigned int)(b >> 32);
        return *this;
    }

    base_blob& operator|=(uint64_t b)
    {
        pn[0] |= (unsigned int)b;
        pn[1] |= (unsigned int)(b >> 32);
        return *this;
    }

    base_blob& operator<<=(unsigned int shift);
    base_blob& operator>>=(unsigned int shift);

    base_blob& operator+=(const base_blob& b)
    {
        uint64_t carry = 0;
        for (int i = 0; i < WIDTH32; i++) {
            uint64_t n = carry + pn[i] + b.pn[i];
            pn[i] = n & 0xffffffff;
            carry = n >> 32;
        }
        return *this;
    }

    base_blob& operator-=(const base_blob& b)
    {
        *this += -b;
        return *this;
    }

    base_blob& operator+=(uint64_t b64)
    {
        base_blob b;
        b = b64;
        *this += b;
        return *this;
    }

    base_blob& operator-=(uint64_t b64)
    {
        base_blob b;
        b = b64;
        *this += -b;
        return *this;
    }

    base_blob& operator*=(uint32_t b32);
    base_blob& operator*=(const base_blob& b);
    base_blob& operator/=(const base_blob& b);

    base_blob& operator++()
    {
        // prefix operator
        int i = 0;
        while (++pn[i] == 0 && i < WIDTH32 - 1)
            i++;
        return *this;
    }

    const base_blob operator++(int)
    {
        // postfix operator
        const base_blob ret = *this;
        ++(*this);
        return ret;
    }

    base_blob& operator--()
    {
        // prefix operator
        int i = 0;
        while (--pn[i] == (uint32_t)-1 && i < WIDTH32 - 1)
            i++;
        return *this;
    }

    const base_blob operator--(int)
    {
        // postfix operator
        const base_blob ret = *this;
        --(*this);
        return ret;
    }

    int CompareTo(const base_blob& b) const;
    inline int Compare(const base_blob& b) const { return CompareTo(b); }
    bool EqualTo(uint64_t b) const;

    friend inline const base_blob operator+(const base_blob& a, const base_blob& b) { return base_blob(a) += b; }
    friend inline const base_blob operator-(const base_blob& a, const base_blob& b) { return base_blob(a) -= b; }
    friend inline const base_blob operator*(const base_blob& a, const base_blob& b) { return base_blob(a) *= b; }
    friend inline const base_blob operator/(const base_blob& a, const base_blob& b) { return base_blob(a) /= b; }
    friend inline const base_blob operator|(const base_blob& a, const base_blob& b) { return base_blob(a) |= b; }
    friend inline const base_blob operator&(const base_blob& a, const base_blob& b) { return base_blob(a) &= b; }
    friend inline const base_blob operator^(const base_blob& a, const base_blob& b) { return base_blob(a) ^= b; }
    friend inline const base_blob operator>>(const base_blob& a, int shift) { return base_blob(a) >>= shift; }
    friend inline const base_blob operator<<(const base_blob& a, int shift) { return base_blob(a) <<= shift; }
    friend inline const base_blob operator*(const base_blob& a, uint32_t b) { return base_blob(a) *= b; }
    friend inline bool operator==(const base_blob& a, const base_blob& b) { return memcmp(a.pn, b.pn, sizeof(a.pn)) == 0; }
    friend inline bool operator!=(const base_blob& a, const base_blob& b) { return memcmp(a.pn, b.pn, sizeof(a.pn)) != 0; }
    friend inline bool operator>(const base_blob& a, const base_blob& b) { return a.CompareTo(b) > 0; }
    friend inline bool operator<(const base_blob& a, const base_blob& b) { return a.CompareTo(b) < 0; }
    friend inline bool operator>=(const base_blob& a, const base_blob& b) { return a.CompareTo(b) >= 0; }
    friend inline bool operator<=(const base_blob& a, const base_blob& b) { return a.CompareTo(b) <= 0; }
    friend inline bool operator==(const base_blob& a, uint64_t b) { return a.EqualTo(b); }
    friend inline bool operator!=(const base_blob& a, uint64_t b) { return !a.EqualTo(b); }

    std::string GetHex() const;
    void SetHex(const char* psz);
    void SetHex(const std::string& str);
    std::string ToString() const;
    std::string ToStringReverseEndian() const;

    unsigned char* begin()
    {
        return &data[0];
    }

    unsigned char* end()
    {
        return &data[WIDTH];
    }

    const unsigned char* begin() const
    {
        return &data[0];
    }

    const unsigned char* end() const
    {
        return &data[WIDTH];
    }

    unsigned int size() const
    {
        return sizeof(data);
    }

    void SetUInt8(uint8_t value) {
        data[0] = value;
        for (size_t i = 1; i < WIDTH; i++)
            data[i] = 0;        
    }

    uint8_t GetUInt8() const {
        return data[0];
    }

    void SetUInt16(uint16_t value) {
        data[0] = (uint8_t)(value);
        data[1] = (uint8_t)(value >> 8);
        for (size_t i = 2; i < WIDTH; i++)
            data[i] = 0;
    }

    uint16_t GetUInt16() const {
        return (((uint16_t)data[0]) | ((uint16_t)data[1]) << 8);
    }

    void SetUInt32(uint32_t value) {
        pn[0] = value;
        for (size_t i = 1; i < WIDTH32; i++)
            pn[i] = 0;
    }

    uint32_t GetUInt32() const {
        return pn[0];
    }

    void SetUInt64(uint64_t value) {
        pn[0] = (uint32_t)(value);
        pn[1] = (uint32_t)(value >> 32);
        for (size_t i = 1; i < WIDTH32; i++)
            pn[i] = 0;
    }

    uint64_t GetUInt64() const {
        return (((uint64_t)pn[0]) | ((uint64_t)pn[1]) << 32);
    }

    uint64_t GetUint64(int pos) const
    {
        const uint8_t* ptr = data + pos * 8;
        return ((uint64_t)ptr[0]) |
               ((uint64_t)ptr[1]) << 8 |
               ((uint64_t)ptr[2]) << 16 |
               ((uint64_t)ptr[3]) << 24 |
               ((uint64_t)ptr[4]) << 32 |
               ((uint64_t)ptr[5]) << 40 |
               ((uint64_t)ptr[6]) << 48 |
               ((uint64_t)ptr[7]) << 56;
    }

    uint64_t Get64(int n = 0) const
    {
        return ((uint32_t*)data)[2 * n] | (uint64_t)((uint32_t*)data)[2 * n + 1] << 32;
    }

    uint32_t Get32(int n = 0) const
    {
        return ((uint32_t*)data)[2 * n];
    }
    /**
     * Returns the position of the highest bit set plus one, or zero if the
     * value is zero.
     */
    unsigned int bits() const;

    uint64_t GetLow64() const
    {
        assert(WIDTH >= 2);
        return ((uint32_t*)data)[0] | (uint64_t)((uint32_t*)data)[1] << 32;
    }

    template <typename Stream>
    void Serialize(Stream& s) const
    {
        s.write((char*)data, sizeof(data));
    }

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        s.read((char*)data, sizeof(data));
    }
};

/** 160-bit opaque blob.
 * @note This type is called uint160 for historical reasons only. It is an opaque
 * blob of 160 bits and has no integer operations.
 */
class uint160 : public base_blob<160>
{
public:
    uint160() {}
    explicit uint160(const std::vector<unsigned char>& vch) : base_blob<160>(vch) {}
};

/** 256-bit opaque blob.
 * @note This type is called uint256 for historical reasons only. It is an
 * opaque blob of 256 bits and has no integer operations. Use arith_uint256 if
 * those are required.
 */
class uint256 : public base_blob<256>
{
public:
    uint256() {}
    uint256(const base_blob<256>& b) : base_blob<256>(b) {}
    uint256(uint64_t b) : base_blob<256>(b) {}
    explicit uint256(const std::string& str) : base_blob<256>(str) {}
    explicit uint256(const std::vector<unsigned char>& vch) : base_blob<256>(vch) {}


    /** A cheap hash function that just returns 64 bits from the result, it can be
     * used when the contents are considered uniformly random. It is not appropriate
     * when the value can easily be influenced from outside as e.g. a network adversary could
     * provide values to trigger worst-case behavior.
     */
    uint64_t GetCheapHash() const
    {
        return ReadLE64(data);
    }

    uint256& SetCompact(uint32_t nCompact, bool* pfNegative = NULL, bool* pfOverflow = NULL);
    uint32_t GetCompact(bool fNegative = false) const;
    uint64_t GetHash(const uint256& salt) const;
};

/* uint256 from const char *.
 * This is a separate function because the constructor uint256(const char*) can result
 * in dangerously catching uint256(0).
 */
inline uint256 uint256S(const char* str)
{
    uint256 rv;
    rv.SetHex(str);
    return rv;
}
/* uint256 from std::string.
 * This is a separate function because the constructor uint256(const std::string &str) can result
 * in dangerously catching uint256(0) via std::string(const char*).
 */
inline uint256 uint256S(const std::string& str)
{
    uint256 rv;
    rv.SetHex(str);
    return rv;
}

class uint512 : public base_blob<512>
{
public:
    uint512() {}
    explicit uint512(const std::vector<unsigned char>& vch) : base_blob<512>(vch) {}

    /** A cheap hash function that just returns 64 bits from the result, it can be
     * used when the contents are considered uniformly random. It is not appropriate
     * when the value can easily be influenced from outside as e.g. a network adversary could
     * provide values to trigger worst-case behavior.
     */
    uint64_t GetCheapHash() const
    {
        return ReadLE64(data);
    }
    uint256 trim256() const
    {
        uint256 trim;
        memcpy((void*)&trim, data, sizeof(uint256));
        return trim;
    }
};

inline uint512 uint512S(const char* str)
{
    uint512 rv;
    rv.SetHex(str);
    return rv;
}
inline uint512 uint512S(const std::string& str)
{
    uint512 rv;
    rv.SetHex(str);
    return rv;
}
#endif // BITCOIN_UINT256_H
