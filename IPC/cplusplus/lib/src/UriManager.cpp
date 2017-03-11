
#ifdef PLATFORM_WIN32
#include <winsock.h>
typedef int socklen_t;
#define CloseSocket closesocket
#pragma comment(lib, "Ws2_32.lib")
#elif defined(PLATFORM_IOS) || defined(PLATFORM_LINUX)
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
typedef int SOCKET;
#define CloseSocket close
#endif // PLATFORM_WIN32

#include "AXP/cplusplus/xplatform/include/uri.h"
#include "AXP/cplusplus/xplatform/include/atime.h"
#include "../include/ServiceManager.h"
#include "../include/UriManager.h"

using namespace AXP;
using namespace IPC;

#define MAX_CLIENT_COUNT (5)
#define CUR_THREAD_COUNT (1)

Sp<ThreadPool> UriManager::sThreadPool = ThreadPool::Create(MAX_CLIENT_COUNT, CUR_THREAD_COUNT);
Sp<List<String> > UriManager::sUriList = new List<String>();
Sp<HashTable<PCWStr, UriManager::CExit> > UriManager::sExitHandlerTable = new HashTable<PCWStr, UriManager::CExit>(MAX_CLIENT_COUNT);

Boolean UriManager::RegisterOnExitHanlder(IN CONST AXP::Sp<AXP::String> & uri, OnExit hanlder)
{
    if ((uri == NULL) || (hanlder == NULL))
        return FALSE;

    Sp<CExit> obj = new CExit();
    if (obj == NULL)
        return FALSE;

    obj->mHandler = hanlder;

    return sExitHandlerTable->InsertUnique(*uri, obj);
}

ARESULT UriManager::ConnectServer(IN CONST Sp<String> & param, OUT Int32 * pSock)
{
    if ((param == NULL) || (pSock == NULL))
        return AE_FAIL;

    Int32 serverPort;
    Char ipAddress[16];
    Sp<Uri> uri = Uri::Create(param);
    if (uri == NULL)
        return AE_FAIL;

    Sp<String> domain = uri->GetDomain();
    if (domain == NULL)
        return AE_OUTOFMEMORY;

    Int32 pos = domain->IndexOf(L':');
    if (pos > 0) {
        Sp<String> port = domain->SubString(pos + 1, domain->Length() - pos - 1);
        if (port == NULL)
            return AE_OUTOFMEMORY;

        Sp<ByteArray> ba = port->GetBytes();
        if (ba == NULL)
            return AE_OUTOFMEMORY;

        domain = domain->SubString(0, pos);
        serverPort = atoi((PCStr)ba->GetPayload());
    }
    else {
        serverPort = 80;
    }

    Sp<ByteArray> ba = domain->GetBytes();
    if (ba == NULL)
        return AE_OUTOFMEMORY;

    PCStr str = (PCStr)ba->GetPayload();
    Strcpy_s(ipAddress, 16, str, NULL);

#ifdef PLATFORM_WIN32
    WSADATA wsaData;
    if (WSAStartup(0x101, &wsaData) != 0)
        return AE_FAIL;
#endif // PLATFORM_WIN32

    SOCKET sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0)
        return AE_FAIL;

    sockaddr_in clientAddr;
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_addr.s_addr = htons(INADDR_ANY);
    clientAddr.sin_port = htons(0);

    if (::bind(sock, (const struct sockaddr *)&clientAddr, sizeof(clientAddr)) == -1) {
        ::CloseSocket(sock);
        return AE_FAIL;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(ipAddress);
    serverAddr.sin_port = htons(serverPort);

    if (connect(sock, (const struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
        DebugPrint("connect %s:%d failed. error=%d!", ipAddress, serverPort, errno);
        return AE_FAIL;
    }

    *pSock = sock;

    return AS_OK;
}

ULong STDCALL UriManager::Routine(IN PVoid param)
{
    if (param == NULL)
        return AE_FAIL;

    Sp<String> uri = (String*)param;

    Int32 sock;
    if (AFAILED(ConnectServer(uri, &sock)))
        return AE_FAIL;

    Sp<CParcel> parcel = new CParcel();
    if (parcel == NULL)
        return AE_FAIL;

    if (AFAILED(parcel->WriteInt8((Int8)0x82)))
        return AE_FAIL;

    if (AFAILED(parcel->WriteString(IPC::ServiceManager::sServerAddress)))
        return AE_FAIL;

    Int32 used = (Int32)parcel->GetPosition();
    Int16 crc1 = 0, crc2 = 0;
    Byte sHeader[2] = { 0x7E, 0x7D };
    Sp<ByteArray> byteBuffer = ByteArray::Create(used + sizeof(sHeader)+sizeof(used)+sizeof(crc1)* 2);
    byteBuffer->Append(sHeader, sizeof(sHeader));
    byteBuffer->Append((PCByte)&used, sizeof(used));
    byteBuffer->Append((PCByte)&crc1, sizeof(crc1));
    byteBuffer->Append(parcel->GetPayload(), used);
    byteBuffer->Append((PCByte)&crc2, sizeof(crc2));
    Long count = send(sock, (PStr)byteBuffer->GetPayload(), byteBuffer->GetUsed(), 0);
    if (count != byteBuffer->GetUsed())
        return AE_FAIL;

    DebugPrint("connect %ls successful", (PCWStr)*uri);
    Char buf[1024];
    Long ret = recv(sock, buf, sizeof(buf), 0);
    DebugPrint("Remtoe Process socket is closed.code=%d(%ls)", ret, (PCWStr)*uri);
    if (ret > 0) {
        Int32 used = *((Int32*)(buf + 2));
        Sp<CParcel> parcel = new CParcel();
        if (parcel == NULL)
            return AE_FAIL;

        parcel->Reset((PCByte)(buf + 8), used);
        Sp<String> uri2;
        if (AFAILED(parcel->ReadString(uri2)))
            return AE_FAIL;

        Synchronized(&sUriList->mLock) {
            Sp<List<IStub> > stubList = IPC::ServiceManager::GetAllServices();
            if (stubList == NULL)
                return AE_FAIL;

            int count = 0;
            Foreach(IStub, obj, stubList) {
                obj->OnDeath(uri2);
            }

            sUriList->Detach(uri2);
        }

        Routine(uri.Get());
    }
    else {
        Sp<CExit> exitHandler = sExitHandlerTable->GetValue(*uri);
        if (exitHandler && exitHandler->mHandler)
            exitHandler->mHandler(uri);

        Synchronized(&sUriList->mLock) {
            Sp<List<IStub> > stubList = IPC::ServiceManager::GetAllServices();
            if (stubList == NULL)
                return AE_FAIL;

            int count = 0;
            Foreach(IStub, obj, stubList) {
                obj->OnDeath(uri);
            }

            sUriList->Detach(uri);
        }
    }


    return AS_OK;
}

Boolean UriManager::StartThread(IN CONST Sp<String> & uri)
{
    if (uri == NULL)
        return FALSE;

    Synchronized(&sUriList->mLock) {
        Foreach(String, obj, sUriList) {
            if (obj->Equals(uri))
                return TRUE;
        }
    }

    Sp<String> uriCopy = String::Create(uri);
    if (uriCopy == NULL)
        return FALSE;

    if (!sUriList->PushBack(uriCopy))
        return FALSE;

    ARESULT ar = sThreadPool->RunThread((PVoid)Routine, (PVoid)uriCopy.Get());
    if (AFAILED(ar))
        return FALSE;

    return TRUE;
}