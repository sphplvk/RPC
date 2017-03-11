
#ifndef __STUB_MANAGER_H__
#define __STUB_MANAGER_H__

#include "AXP/cplusplus/xplatform/include/stl/hashTable.h"
#include "Stub.h"
#include "Tracker.h"

namespace IPC
{
    class ServiceManager : public AXP::CObject
    {
    public:

        STATIC AXP::Sp<AXP::String> sServerAddress;

        STATIC AXP::Sp<AXP::HashTable<AXP::PCWStr, AXP::String> > sProxyAddr;

        STATIC AXP::Boolean RegisterProxyAddr(
            IN CONST AXP::PCWStr className,
            IN CONST AXP::Sp<AXP::String> & addr);

        STATIC AXP::Sp<AXP::String> GetProxyAddr(
            IN CONST AXP::PCWStr className);

        STATIC AXP::ARESULT RegisterService(
            IN AXP::PCWStr className,
            IN CONST AXP::Sp<IStub> & stub);

        STATIC AXP::Sp<IStub> GetService(
            IN CONST AXP::Sp<AXP::String> & uri);

        STATIC AXP::Sp<AXP::List<IStub> > GetAllServices();
    };
}

#endif // __STUB_MANAGER_H__
