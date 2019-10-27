// Copyright (c) 2012-2019 The GalaxyCash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef GALAXYCASH_QT_MINTINGFILTERPROXY_H
#define GALAXYCASH_QT_MINTINGFILTERPROXY_H

#include <QSortFilterProxyModel>

class MintingFilterProxy : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit MintingFilterProxy(QObject *parent = 0);
};

#endif // GALAXYCASH_QT_MINTINGFILTERPROXY_H
