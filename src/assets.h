// Copyright (c) 2019 The GalaxyCash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef GALAXYCASH_ASSETS_H
#define GALAXYCASH_ASSETS_H

#define MAX_ASSET_SIZE 1000000 // 1 MB

class CAssetHeader {
public:
    uint8_t             type;
    uint16_t            length;
    std::string         name;
    std::string         description;
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

    CAsset();
    ~CAsset();
};

inline int64_t CalculateAssetFee(const uint8_t type, const uint32_t length) {
    switch (type)
    {
        case CAsset::Link:
        {
            int64_t fee = ((int64_t) (COIN * 0.10) * (int64_t) length) / 1000;

            if (fee < COIN)
                return COIN;

            return fee;
        }
        break;
        case CAsset::Text:
        {
            int64_t fee = ((int64_t) (COIN * 0.15) * (int64_t) length) / 1000;

            if (fee < COIN)
                return COIN;

            return fee;
        }
        break;
        case CAsset::Image:
        {
            int64_t fee = ((int64_t) (COIN * 0.5) * (int64_t) length) / 1000;

            if (fee < COIN)
                return COIN;

            return fee;
        }
        break;
    };
    return 0;
}

#endif
