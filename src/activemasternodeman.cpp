#include "masternodeman.h"
#include "masternode.h"
#include "activemasternode.h"
#include "activemasternodeman.h"
#include "core.h"
#include "util.h"
#include "addrman.h"
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>


/** ActiveMasternode manager */
CActiveMasternodeMan activemnodeman;


CActiveMasternodeMan::CActiveMasternodeMan()
{}
CActiveMasternodeMan::~CActiveMasternodeMan()
{}


int CActiveMasternodeMan::Register(CTxIn &node)
{
    LOCK(cs);
    for (int i = 0; i < vMasternodes.size(); i++)
    {
        if (node == vMasternodes[i])
            return i;
    }
    vMasternodes.push_back(node);
    return vMasternodes.size() - 1;
}

int CActiveMasternodeMan::Unregister(CTxIn &node)
{
    LOCK(cs);
    for (int i = 0; i < vMasternodes.size(); i++)
    {
        if (node == vMasternodes[i])
        {
            vMasternodes.erase(vMasternodes.begin() + i);
            return i;
        }

    }
    return -1;
}

int CActiveMasternodeMan::NumNodes() const
{
    LOCK(cs);
    return vMasternodes.size();
}

CTxIn CActiveMasternodeMan::GetVin(int i)
{
    LOCK(cs);
    return vMasternodes[i];
}

CMasternode *CActiveMasternodeMan::GetMasternode(int i)
{
    LOCK(cs);
    return mnodeman.Find(vMasternodes[i]);
}

int CActiveMasternodeMan::ManageStatus()
{
    LOCK(cs);
    if (vMasternodes.size() > 0)
    {
        CTxIn vin = activeMasternode.vin;
        for (int i = 0; i < vMasternodes.size(); i++)
        {
            activeMasternode.vin = vMasternodes[i];
            activeMasternode.ManageStatus();
        }
        activeMasternode.vin = vin;

        return vMasternodes.size();
    }

    activeMasternode.ManageStatus();
    return 1;
}

