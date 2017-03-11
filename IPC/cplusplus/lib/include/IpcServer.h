//
//  HttpServer.h
//  TestWebServer
//
//  Created by Liu Alex on 14-7-2.
//  Copyright (c) 2014年 Liu Alex. All rights reserved.
//

#ifndef __IPCSERVER_H__
#define __IPCSERVER_H__

#include "ThreadPool.h"
#include "AXP/cplusplus/xplatform/include/type.h"
#include "AXP/cplusplus/xplatform/include/refBase.h"
#include "AXP/cplusplus/xplatform/include/streambuf.h"
#include "AXP/cplusplus/xplatform/include/astring.h"
#include "AXP/cplusplus/libc/include/Common/BaseWorker.h"
#include "AXP/cplusplus/xplatform/include/Parcel.h"
#include "AXP/cplusplus/xplatform/include/vbuf.h"
#include "AXP/cplusplus/xplatform/include/stl/hashTable.h"

namespace IPC
{
    class IpcServer : public AXP::Libc::Common::CBaseWorker
    {
    public:

        STATIC AXP::Sp<IpcServer> Create(
        IN CONST AXP::Sp<AXP::String> & localAddr,
        IN CONST AXP::Sp<AXP::HashTable<AXP::PCWStr, AXP::String> > & proxyAddr);

    private:

        STATIC AXP::ULong STDCALL Request(
            IN AXP::PVoid param);

    public:

        VIRTUAL AXP::Void STDCALL Run();

        VIRTUAL AXP::ARESULT STDCALL Discard();

    protected:

        AXP::ARESULT STDCALL OnRequest(
            IN CONST AXP::Sp<AXP::CParcel> & request,
            OUT AXP::Sp<AXP::CParcel> & response);

    private:

        AXP::ARESULT AcceptRequest(
            IN AXP::Int32 sock);

        AXP::Void BadRequest(
            IN AXP::Int32 sock);

        AXP::ARESULT BadRequest(
            IN AXP::Int32 sock,
            AXP::Int32 code);

        AXP::ARESULT Initialize(
            IN AXP::Int16 port);

    private:

        IpcServer() {}

    protected:

        AXP::Int32 mIpAddress;
        AXP::Int32 mPort;

    private:

        AXP::Sp<ThreadPool> mThreadPool;
        AXP::Int32 mSock;
    };
}

#endif // __IPCSERVER_H__
