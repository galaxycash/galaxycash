// Copyright (c) 2017-2018 The GalaxyCash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/assign/list_of.hpp>

#include "main.h"
#include "chainparams.h"
#include "checkpoints.h"
#include "db.h"
#include "init.h"
#include "net.h"
#include "txdb.h"
#include "txmempool.h"
#include "blockfile.h"
#include <zlib.h>
#include <sys/fcntl.h>

using namespace std;
using namespace boost;


static filesystem::path BlockFilePath(unsigned int nFile)
{
    string strBlockFn = strprintf("blocks/blk%04u.dat", nFile);
    return GetDataDir() / strBlockFn;
}

CBlockFile::CBlockFile() :
    mode(0)
{
    nType = 0;
    nVersion = 0;
    state = 0;
    exceptmask = std::ios::badbit | std::ios::failbit;
}

CBlockFile::CBlockFile(const int mode, const std::string &path, int nTypeIn, int nVersionIn) :
    mode(mode),
    path(path),
    nType(nTypeIn),
    nVersion(nVersionIn)
{
    {
        if (mode == APPEND)
        {
            try {
                file.open(path, std::ios_base::binary | std::ios_base::out | std::ios_base::app | std::ios_base::ate);
            }
            catch (std::exception &e) {
                error("CBlockFile::CBlockFile() : Open block file to append failed %s", path.c_str());
            }

        }
        else if (mode == OPEN)
        {
            try {
                file.open(path, std::ios_base::binary | std::ios_base::in);
            }
            catch (std::exception &e) {
                error("CBlockFile::CBlockFile() : Open block file to read failed %s", path.c_str());
            }
        }
    }
    nType = nTypeIn;
    nVersion = nVersionIn;
    state = 0;
    exceptmask = std::ios::badbit | std::ios::failbit;
}

CBlockFile::~CBlockFile()
{ 
    if (file.is_open())
    {
        if (mode == APPEND) file.flush();
        file.close();
    }
}

long CBlockFile::seek(long pos)
{
    return file.seekp (streamsize(pos), file.beg).bad();
}

long CBlockFile::skip(long count)
{
    return file.seekp (streamsize(count), file.cur).bad();
}

long CBlockFile::readbytes(void *pch, long size)
{
    if (file.read(reinterpret_cast<char*>(pch), streamsize(size)).bad())
        return 0;
    return size;
}

long CBlockFile::writebytes(const void *pch, long size)
{
    if (file.write(reinterpret_cast<const char*>(pch), streamsize(size)).bad())
        return 0;
    return size;
}

long CBlockFile::tell()
{
    return file.tellp();
}

static unsigned int nCurrentBlockFile = 1;

CBlockFile *OpenBlockFile(unsigned int nFile, unsigned int nBlockPos, int nMode)
{
    if ((nFile < 1) || (nFile == (unsigned int) -1))
        return NULL;

    CBlockFile *file = new CBlockFile(nMode, BlockFilePath(nFile).string().c_str());
    if (nBlockPos != 0)
    {
        if (file->seek(nBlockPos) != 0 && file->mode != CBlockFile::APPEND)
        {
            delete file;
            return NULL;
        }
    }
    return file;
}

CBlockFile *AppendBlockFile(unsigned int& nFileRet)
{
    nFileRet = 0;
    while (true)
    {
        CBlockFile* file = OpenBlockFile(nCurrentBlockFile, 0, CBlockFile::APPEND);
        if (!file)
            return NULL;

        // FAT32 file size max 4GB, fseek and ftell max 2GB, so we must stay under 2GB
        if (file->tell() < (long)(0x7F000000 - MAX_SIZE))
        {
            nFileRet = nCurrentBlockFile;
            return file;
        }
        delete file;
        nCurrentBlockFile++;
    }
}
