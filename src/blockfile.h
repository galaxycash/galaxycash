// Copyright (c) 2017-2018 The GalaxyCash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef GALAXYCASH_BLOCKFILE_H
#define GALAXYCASH_BLOCKFILE_H

#include <istream>  // not iostream, since we don't need cin/cout
#include <ostream>
#include "zlib.h"
#include <fstream>
#include "serialize.h"

class CBlockFile
{
public:
    enum {
        BAD = 0,
        OPEN,
        APPEND
    };

    std::fstream    file;
    std::string     path;
    int             mode;
    short           state;
    short           exceptmask;
    int             nType;
    int             nVersion;

    CBlockFile();
    CBlockFile(const int mode, const std::string &path, int nTypeIn = SER_DISK, int nVersionIn = CLIENT_VERSION);
    ~CBlockFile();

    //
    // Stream subset
    //
    void setstate(short bits, const char* psz)
    {
        state |= bits;
        if (state & exceptmask)
            throw std::ios_base::failure(psz);
    }

    bool fail() const            { return state & (std::ios::badbit | std::ios::failbit); }
    bool good() const            { return state == 0; }
    void clear(short n = 0)      { state = n; }
    short exceptions()           { return exceptmask; }
    short exceptions(short mask) { short prev = exceptmask; exceptmask = mask; setstate(0, "CBlockFile"); return prev; }

    void SetType(int n)          { nType = n; }
    int GetType()                { return nType; }
    void SetVersion(int n)       { nVersion = n; }
    int GetVersion()             { return nVersion; }
    void ReadVersion()           { *this >> nVersion; }
    void WriteVersion()          { *this << nVersion; }

    long size();
    long tell();
    long seek(long pos);
    long skip(long count);
    long writebytes(const void *pch, long size);
    long readbytes(void *pch, long size);

    CBlockFile& read(char* pch, size_t nSize)
    {
        if (readbytes((void *) pch,  (long) nSize) != (long) nSize)
            setstate(std::ios::failbit, "CBlockFile::read : readbytes failed");
        return (*this);
    }

    CBlockFile& write(const char* pch, size_t nSize)
    {
        if (writebytes((const void *) pch, (long) nSize) != (long) nSize)
            setstate(std::ios::failbit, "CBlockFile::write : writebytes failed");
        return (*this);
    }

    template<typename T>
    unsigned int GetSerializeSize(const T& obj)
    {
        // Tells the size of the object if serialized to this stream
        return ::GetSerializeSize(obj, nType, nVersion);
    }

    template<typename T>
    CBlockFile& operator<<(const T& obj)
    {
        ::Serialize(*this, obj, nType, nVersion);
        return (*this);
    }

    template<typename T>
    CBlockFile& operator>>(T& obj)
    {
        ::Unserialize(*this, obj, nType, nVersion);
        return (*this);
    }
};

CBlockFile *OpenBlockFile(unsigned int nFile, unsigned int nBlockPos, int nMode);
CBlockFile *AppendBlockFile(unsigned int& nFileRet);

#endif
