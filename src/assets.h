// Copyright (c) 2019 The GalaxyCash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef GALAXYCASH_ASSETS_H
#define GALAXYCASH_ASSETS_H

#define MAX_ASSET_SIZE 1000000 // 1 MB

class CAssetHeader {
public:
    uint32_t            nVersion;
    uint8_t             nType;
    std::string         sName;

    IMPLEMENT_SERIALIZE
    (
        READWRITE(nVersion);
        READWRITE(nType);
        READWRITE(sName);
    )

    CAssetHeader() {
        SetNull();
    }

    CAssetHeader(const CAssetHeader &h) : nVersion(h.nVersion),
        nType(h.nType),
        sName(h.sName) {

    }

    CAssetHeader &      SetNull() {
        nVersion = 1;
        nType = 0;
        sName.clear();
        return *this;
    }

    bool                IsNull() const {
        return (nType == 0);
    }
};

class CAsset {
public:
    enum {
        Undefined = 0,
        Image,
        Link,
        Text
    };

    CAssetHeader        header;
    std::vector<unsigned char>
                        vch;

    IMPLEMENT_SERIALIZE
    (
        READWRITE(header);
        READWRITE(vch);
    )

    CAsset() {
        SetNull();
    }

    CAsset(const CAsset &asset) :
        header(asset.header),
        vch(asset.vch) {
    }

    CAsset(const CAssetHeader &inHeader, const std::vector <unsigned char> &inVch) :
        header(inHeader),
        vch(inVch)
    {
    }
    ~CAsset() {
        vch.clear();
    }

    CAsset &            SetNull() {
        header.SetNull();
        vch.clear();
        return *this;
    }

    bool                IsNull() const {
        return header.IsNull();
    }

    bool                IsImage() const {
        return (header.type == Image);
    }

    bool                IsLink() const {
        return (header.type == Link);
    }

    bool                IsText() const {
        return (header.type == Text);
    }

    const char *        AsText() const {
        return (const char * ) vch.data();
    }

    bool                AddToScript(CScript &script);
};

bool ScriptAddAsset(CScriot &script, const CAsset &asset);
bool ScriptGetAsset(const CScript &script, CAsset &asset);
bool ScriptHaveAsset(const CScript &script);


#endif
