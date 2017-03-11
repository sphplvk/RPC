
#ifndef __URI_MANAGER_H__
#define __URI_MANAGER_H__

#include "AXP/cplusplus/xplatform/include/stl/hashTable.h"
#include "ObjectHolder.h"
#include "ThreadPool.h"

namespace IPC
{
    class UriManager
    {
        typedef void (*OnExit)(IN CONST AXP::Sp<AXP::String> & uri);
        
    public:

        STATIC AXP::Boolean StartThread(
            IN CONST AXP::Sp<AXP::String> & uri);
        
        STATIC AXP::Boolean RegisterOnExitHanlder(IN CONST AXP::Sp<AXP::String> & uri, OnExit hanlder);

    private:

        STATIC AXP::ARESULT ConnectServer(
            IN CONST AXP::Sp<AXP::String> & param,
            OUT AXP::Int32 * pSock);

        STATIC AXP::ULong STDCALL Routine(
            IN AXP::PVoid param);

    private:
        
        class CExit : public AXP::CObject
        {
        public:
            
            OnExit mHandler;
        };
        
    private:

        STATIC AXP::Sp<ThreadPool> sThreadPool;

        STATIC AXP::Sp<AXP::List<AXP::String> > sUriList;
        STATIC AXP::Sp<AXP::HashTable<AXP::PCWStr, CExit> > sExitHandlerTable;
    };
}

#endif // __URI_MANAGER_H__