// Copyright (c) 2017-2019 The GalaxyCash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef GALAXYCASH_EXT_SCRIPT_COMPILER_H
#define GALAXYCASH_EXT_SCRIPT_COMPILER_H

CVMValue *CCCompileFile(const std::string &path);
CVMValue *CCCompileSource(const std::string &code);
CVMValue *CCCompileModule(const std::string &dir);

#endif