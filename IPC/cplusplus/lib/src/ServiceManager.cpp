
#include "../include/ServiceManager.h"

using namespace AXP;
using namespace IPC;

Sp<String> ServiceManager::sServerAddress;

STATIC HashTable<PCWStr, IStub> * sServiceList;

Sp<HashTable<PCWStr, String> > ServiceManager::sProxyAddr = new HashTable<PCWStr, String>(50);

AXP::Boolean ServiceManager::RegisterProxyAddr(
    IN CONST AXP::PCWStr className,
    IN CONST AXP::Sp<AXP::String> & addr)
{
    if ((!className) || (!addr))
        return FALSE;
    
    if (sProxyAddr == NULL)
        return FALSE;

    Synchronized (&sProxyAddr->mLock) {
        return sProxyAddr->InsertUnique(className, addr);
    }

    return FALSE;
}

Sp<String> ServiceManager::GetProxyAddr(IN CONST AXP::PCWStr key)
{
    if (!key)
        return NULL;
    
    if (!sProxyAddr)
        return NULL;

    Synchronized (&sProxyAddr->mLock) {
        return sProxyAddr->GetValue(key);
    }  

    return NULL;
}

ARESULT ServiceManager::RegisterService(
    IN AXP::PCWStr className,
    IN CONST AXP::Sp<IStub> & stub)
{
    if ((className == NULL) || (stub == NULL))
        return AE_INVALIDARG;

    if (sServiceList == NULL) {
        sServiceList = new HashTable<PCWStr, IStub>(200);
        if (sServiceList == NULL)
            return AE_OUTOFMEMORY;
    }

    Synchronized(&sServiceList->mLock) {
        if (!sServiceList->InsertUnique((AXP::PWStr)className, stub))
            return AE_OUTOFMEMORY;
    }

    return AS_OK;
}

Sp<IStub> ServiceManager::GetService(
    IN CONST AXP::Sp<AXP::String> & key)
{
    if ((sServiceList == NULL) || (key == NULL))
        return NULL;

    Synchronized(&sServiceList->mLock) {
        return sServiceList->GetValue(*key);
    }

    return NULL;
}

AXP::Sp<AXP::List<IStub> > ServiceManager::GetAllServices()
{
    if (sServiceList == NULL)
        return NULL;

    Synchronized(&sServiceList->mLock) {
        return sServiceList->GetValues();
    }

    return NULL;
}