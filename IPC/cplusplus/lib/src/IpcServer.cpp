//
//  HttpServer.cpp
//  TestWebServer
//
//  Created by Liu Alex on 14-7-2.
//  Copyright (c) 2014年 Liu Alex. All rights reserved.
//

#ifdef PLATFORM_WIN32
#include <winsock.h>
typedef int socklen_t;
#define CloseSocket closesocket
#pragma comment(lib, "Ws2_32.lib")
#elif defined(PLATFORM_IOS) || defined(PLATFORM_LINUX)
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
typedef int SOCKET;
#define CloseSocket close
#endif // PLATFORM_WIN32

#include "../include/ServiceManager.h"
#include "../include/IpcException.h"
#include "../include/IpcServer.h"

#define MAX_THREAD_COUNT (30)
#define CUR_THREAD_COUNT (10)
#define MAX_LISTEN_COUNT (10)

using namespace AXP;
using namespace IPC;

typedef struct _PARAM {
    Int32 sock;
    IpcServer * pThis;
} Param;

STATIC Byte sHeader[2] = { 0x7E, 0x7D };

Sp<IpcServer> IpcServer::Create(
    IN CONST Sp<String> & localAddr,
    IN CONST Sp<HashTable<PCWStr, String> > & proxyAddr)
{
    if ((!localAddr) || (!proxyAddr))
        return NULL;

    Int32 a, b, c, d;
    Int32 port;
#ifdef PLATFORM_WIN32
    if (swscanf_s(*localAddr, L"http://%d.%d.%d.%d:%d/", &a, &b, &c, &d, &port) < 0)
        return NULL;
#elif defined(PLATFORM_IOS) || defined(PLATFORM_LINUX)
    if (swscanf(*localAddr, L"http://%d.%d.%d.%d:%d/", &a, &b, &c, &d, &port) < 0)
        return NULL;
#endif
    ServiceManager::sServerAddress = localAddr;
    for (Sp<HashTable<PCWStr, String>::Iterator> iter = proxyAddr->GetIterator(); iter->MoveNext();) {
        PCWStr key = iter->GetKey();
        Sp<String> value = iter->GetValue();
        if ((!key) || (!value))
            return NULL;

        if (value->Equals(L"local")) {
        }
        else {
            if (!ServiceManager::RegisterProxyAddr(key, value))
                return NULL;
        }
    }

    Sp<IpcServer> obj = new IpcServer();
    if (obj == NULL)
        return NULL;

    if (AFAILED(obj->Initialize(port)))
        return NULL;

    return obj;
}

ARESULT IpcServer::Initialize(
    IN Int16 port)
{
    mThreadPool = ThreadPool::Create(MAX_THREAD_COUNT, CUR_THREAD_COUNT);
    if (mThreadPool == NULL)
        return AE_OUTOFMEMORY;

    mIpAddress = INADDR_ANY;
    mPort = port;
    return AS_OK;
}

ARESULT STDCALL IpcServer::Discard()
{
    ::CloseSocket(mSock);


    return CBaseWorker::Discard();
}

Void STDCALL IpcServer::Run()
{
#if defined(PLATFORM_IOS) || defined(PLATFORM_LINUX)
    sigset_t signal_mask;
    sigemptyset (&signal_mask);
    sigaddset (&signal_mask, SIGPIPE);
    Int32 rc = pthread_sigmask (SIG_BLOCK, &signal_mask, NULL);
    if (rc != 0) {
        NotifyToStop(AE_FAIL);
        return;
    }
#endif

    DebugPrint("Run");
    Synchronized(&mLock) {
        if (AXP::AFAILED(Reset())) {
            NotifyToStop(AE_FAIL);
            return;
        }
    }

#ifdef PLATFORM_WIN32
    WSADATA wsaData;
    if (WSAStartup(0x101, &wsaData) != 0) {
        NotifyToStop(AE_FAIL);
        return;
    }
#endif // PLATFORM_WIN32

    mSock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (mSock < 0) {
        NotifyToStop(AE_FAIL);
        DebugPrint("create socket failed");
        return;
    }

    Int32 yes = 1;
    if (::setsockopt(mSock, SOL_SOCKET, SO_REUSEADDR, (PCStr)&yes, sizeof(yes)) == -1) {
        ::CloseSocket(mSock);
        NotifyToStop(AE_FAIL);
        DebugPrint("setsocketopt failed");
        return;
    }

    sockaddr_in servAddr;
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = mIpAddress;
    servAddr.sin_port = htons(mPort);

    if (::bind(mSock, (const struct sockaddr *)&servAddr, sizeof(servAddr)) == -1) {
        ::CloseSocket(mSock);
        NotifyToStop(AE_FAIL);
        DebugPrint("bind faiiled");
        return;
    }

    if (::listen(mSock, MAX_LISTEN_COUNT) == -1) {
        ::CloseSocket(mSock);
#ifdef PLATFORM_WIN32
        WSACleanup();
#endif // PLATFORM_WIN32
        NotifyToStop(AXP::AE_FAIL);
        DebugPrint("listen failed");
        return;
    }

    while (!IsTerminated()) {
        sockaddr_in clientAddr;
        socklen_t sockSize = sizeof(clientAddr);
        Int32 clientSock = ::accept(
            mSock, (struct sockaddr *)&clientAddr, &sockSize);

        if (clientSock == -1) {
            DebugPrint("accept failed");
            continue;
        }

        Param * param = (Param*)malloc(sizeof(Param));
        if (!param) {
            ::CloseSocket(clientSock);
            ::CloseSocket(mSock);
#ifdef PLATFORM_WIN32
            WSACleanup();
#endif // PLATFORM_WIN32
            NotifyToStop(AXP::AE_FAIL);
            return;
        }

        param->pThis = this;
        param->sock = clientSock;

        ARESULT ar = mThreadPool->RunThread((AXP::PVoid)Request, param);
        if (AFAILED(ar)) {
            if (ar == AE_BUSY) {
                ar = BadRequest(clientSock, IPC::ServerBusyException);
                ::CloseSocket(clientSock);
                if (AFAILED(ar)) {
                    ::CloseSocket(mSock);
#ifdef PLATFORM_WIN32
                    WSACleanup();
#endif // PLATFORM_WIN32
                    NotifyToStop(AXP::AE_FAIL);
                    return;
                }
            }
            else {
                ::CloseSocket(clientSock);
                ::CloseSocket(mSock);
#ifdef PLATFORM_WIN32
                WSACleanup();
#endif // PLATFORM_WIN32
                NotifyToStop(AXP::AE_FAIL);
                return;
            }
        }
    }

    ::CloseSocket(mSock);

#ifdef PLATFORM_WIN32
    WSACleanup();
#endif // PLATFORM_WIN32

    NotifyToStop(AXP::AS_OK);
}

ARESULT STDCALL IpcServer::OnRequest(
    IN CONST Sp<CParcel> & request,
    OUT Sp<CParcel> & response)
{
    Sp<String> token;
    if (AFAILED(request->ReadString(token)))
        return AE_FAIL;

    Sp<String> className;
    if (AFAILED(request->ReadString(className)))
        return AE_FAIL;

    AXP::Sp<IPC::IStub> stub = IPC::ServiceManager::GetService(className);
    if (stub == NULL)
        return AE_FAIL;

    Int64 len = request->GetLength();
    Int64 pos = request->GetPosition();
    request->Seek(len);
    if (AFAILED(request->WriteString(token)))
        return AE_FAIL;

    request->Seek(pos);
    response = stub->Transact(request);

    return AS_OK;
}

ARESULT IpcServer::AcceptRequest(
    IN Int32 sock)
{
    // Header + Length + CRC1 + used + CRC2
    Int32 used;
    Int16 crc1 = 0;
    Int16 crc2 = 0;

    Byte header[2];
    while (true) {
        Long recv = ::recv(sock, (PStr)&header, sizeof(header), 0);
        if ((recv != sizeof(header)) || (header[0] != sHeader[0]) || (header[1] != sHeader[1])) {
            BadRequest(sock);
            return AE_FAIL;
        }

        if (!mRunFlag)
            return AS_OK;

        break;
    }

    while (true) {
        Long recv = ::recv(sock, (PStr)&used, sizeof(used), 0);
        if (recv != sizeof(used)) {
            BadRequest(sock);
            return AE_FAIL;
        }

        if (!mRunFlag)
            return AS_OK;

        break;
    }

    while (mRunFlag) {
        Long recv = ::recv(sock, (PStr)&crc1, sizeof(crc1), 0);
        if (recv != sizeof(crc1)) {
            BadRequest(sock);
            return AE_FAIL;
        }

        if (!mRunFlag)
            return AS_OK;

        break;
    }

    Sp<MemoryStream<PARCEL_BUFFER_DEFAULT_LENGTH> > ms = new MemoryStream<PARCEL_BUFFER_DEFAULT_LENGTH>();
    if (ms == NULL) {
        BadRequest(sock);
        return AE_OUTOFMEMORY;
    }

    while (mRunFlag && (used > 0)) {
        Byte buffer[1024];
        Long recv;
        if (used > 1024)
            recv = ::recv(sock, (PStr)buffer, 1024, 0);
        else
            recv = ::recv(sock, (PStr)buffer, used, 0);

        if (recv < 0) {
            BadRequest(sock);
            return AE_FAIL;
        }

        ms->Write(buffer, recv);
        used -= recv;
    }

    while (mRunFlag) {
        Long recv = ::recv(sock, (PStr)&crc2, sizeof(crc2), 0);
        if (recv != sizeof(crc2)) {
            BadRequest(sock);
            return AE_FAIL;
        }

        break;
    }

    Sp<CParcel> request = new CParcel(ms);
    if (request == NULL) {
        BadRequest(sock);
        return AE_OUTOFMEMORY;
    }

    Int8 tag;
    if (AFAILED(request->ReadInt8(tag)))
        return AE_FAIL;

    Sp<CParcel> response;
    if ((tag & 0x0F) == 0x02) {
        Char buf[16];
        Long count = ::recv(sock, buf, sizeof(buf), 0);
        Long error;
#ifdef PLATFORM_WIN32
        error = WSAGetLastError();
#elif defined(PLATFORM_IOS) || defined(PLATFORM_LINUX)
        error = errno;
#endif
        Sp<String> uri;
        if (AFAILED(request->ReadString(uri)))
            return AE_FAIL;

        DebugPrint("callback monitor return code = %ld, reason: %ld.(%ls)\n", count, error, (PCWStr)*uri);
        return AS_OK;
    }
    else if ((tag & 0x0F) == 0x03) {
        Sp<CParcel> response;
        ARESULT result = OnRequest(request, response);
        if (AFAILED(result) || (response == NULL)) {
            BadRequest(sock);
            return AXP::AS_OK;
        }

        used = (Int32)response->GetLength();

        Long send = ::send(sock, (PCStr)sHeader, sizeof(sHeader), 0);
        if (send != sizeof(sHeader)) {
            BadRequest(sock);
            return AE_FAIL;
        }

        send = ::send(sock, (PCStr)&used, sizeof(used), 0);
        if (send != sizeof(used)) {
            BadRequest(sock);
            return AE_FAIL;
        }

        send = ::send(sock, (PCStr)&crc1, sizeof(crc1), 0);
        if (send != sizeof(crc1)) {
            BadRequest(sock);
            return AE_FAIL;
        }

        send = ::send(sock, (PCStr)response->GetPayload(), used, 0);
        if (send != used) {
            BadRequest(sock);
            return AE_FAIL;
        }

        send = ::send(sock, (PCStr)&crc2, sizeof(crc2), 0);
        if (send != sizeof(crc2)) {
            BadRequest(sock);
            return AE_FAIL;
        }
    }
    else if ((tag & 0x0F) == 0x04) {
        Sp<CParcel> response;
        ARESULT result = OnRequest(request, response);
        if (AFAILED(result)) {
            BadRequest(sock);
            return AXP::AS_OK;
        }
    }

    return AXP::AS_OK;
}

Void IpcServer::BadRequest(
    IN Int32 sock)
{
    BadRequest(sock, IPC::RemoteException);
}

ARESULT IpcServer::BadRequest(
    IN Int32 sock,
    IN Int32 code)
{
    Sp<CParcel> request = new CParcel();
    if (request == NULL)
        return AE_OUTOFMEMORY;

    if (AFAILED(request->WriteInt32(code)))
        return AE_OUTOFMEMORY;

    // Header + Length + CRC1 + used + CRC2
    Int32 used = (Int32)request->GetLength();
    Int16 crc1 = 0;
    Int16 crc2 = 0;

    Long send = ::send(sock, (PCStr)sHeader, sizeof(sHeader), 0);
    if (send != sizeof(sHeader))
        return AE_FAIL;

    send = ::send(sock, (PCStr)&used, sizeof(used), 0);
    if (send != sizeof(used))
        return AE_FAIL;

    send = ::send(sock, (PCStr)&crc1, sizeof(crc1), 0);
    if (send != sizeof(crc1))
        return AE_FAIL;

    send = ::send(sock, (PCStr)request->GetPayload(), used, 0);
    if (send != used)
        return AE_FAIL;

    send = ::send(sock, (PCStr)&crc2, sizeof(crc2), 0);
    if (send != sizeof(crc2))
        return AE_FAIL;

    return AS_OK;
}

ULong STDCALL IpcServer::Request(
    IN PVoid param)
{
    if (!param)
        return 0;

    Param * execParam = (Param*)param;
    execParam->pThis->AcceptRequest(execParam->sock);

    ::CloseSocket(execParam->sock);
    ::free(execParam);

    return 0;
}