
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

#include "../include/ServerConnection.h"
#include "IPC/cplusplus/lib/include/ServiceManager.h"

using namespace AXP;
using namespace IPC;

STATIC Byte sHeader[2] = { 0x7E, 0x7D };

AXP::Sp<IPC::IStub> CServerConnection::Create(
    IN CONST AXP::Sp<AXP::String> & description)
{
    if (description == NULL)
        return NULL;

    Sp<CServerConnection> obj = new CServerConnection();
    if (obj == NULL)
        return NULL;

    Sp<Uri> uri = new Uri(ServiceManager::GetProxyAddr(*description));
    if (uri == NULL)
        return NULL;

    if (AFAILED(obj->Initialize(uri)))
        return NULL;

    return obj;
}

CServerConnection::CServerConnection()
{
}

CServerConnection::~CServerConnection()
{
    
}

AXP::ARESULT CServerConnection::Initialize(
    IN CONST AXP::Sp<AXP::Uri> & uri)
{
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
        mPort = atoi((PCStr)ba->GetPayload());
    }
    else {
        mPort = 80;
    }

    Sp<ByteArray> ba = domain->GetBytes();
    if (ba == NULL)
        return AE_OUTOFMEMORY;

    PCStr str = (PCStr)ba->GetPayload();
    Strcpy_s(mIpAddress, 16, str, NULL);

    mRunFlag = TRUE;

    return AS_OK;
}

AXP::Sp<AXP::CParcel> CServerConnection::Transact(
    IN CONST AXP::Sp<AXP::CParcel> & bundle)
{
#if defined(PLATFORM_IOS) || defined(PLATFORM_LINUX)
    sigset_t signal_mask;
    sigemptyset (&signal_mask);
    sigaddset (&signal_mask, SIGPIPE);
    Int32 rc = pthread_sigmask (SIG_BLOCK, &signal_mask, NULL);
    if (rc != 0)
        return NULL;
#endif
    
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(mIpAddress);
    serverAddr.sin_port = htons(mPort);
    
#ifdef PLATFORM_WIN32
    WSADATA wsaData;
    if (WSAStartup(0x101, &wsaData) != 0)
        return NULL;
#endif // PLATFORM_WIN32

    Int32 sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) {
        DebugPrint("create socket failed");
        return NULL;
    }

    sockaddr_in clientAddr;
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_addr.s_addr = htons(INADDR_ANY);
    clientAddr.sin_port = htons(0);

    if (::bind(sock, (const struct sockaddr *)&clientAddr, sizeof(clientAddr)) == -1) {        
        ::CloseSocket(sock);
        DebugPrint("bind failed");
        return NULL;
    }

    if (::connect(sock, (const struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
        bundle->Reset();
        Int8 tag;
        if (AFAILED(bundle->ReadInt8(tag)))
            return NULL;

        Sp<String> token;
        if (AFAILED(bundle->ReadString(token)))
            return NULL;

        Sp<String> description;
        if (AFAILED(bundle->ReadString(description)))
            return NULL;

        Int32 code;
        if (AFAILED(bundle->ReadInt32(code)))
            return NULL;

        DebugPrint("connect http://%s:%d/ failed(%ls,%d)!", mIpAddress, mPort, (PCWStr)*description, code);
        return NULL;
    }

    // Header + Length + CRC1 + used + CRC2
    Int32 used = (Int32)bundle->GetLength();
    Int16 crc1 = 0;
    Int16 crc2 = 0;

    Long send = ::send(sock, (PCStr)sHeader, sizeof(sHeader), 0);
    if (send != sizeof(sHeader)) {
        DebugPrint("Send Header failed!");
        return NULL;
    }

    send = ::send(sock, (PCStr)&used, sizeof(used), 0);
    if (send != sizeof(used)) {
        DebugPrint("Send Package Length failed!");
        return NULL;
    }
          
    send = ::send(sock, (PCStr)&crc1, sizeof(crc1), 0);
    if (send != sizeof(crc1))  {
        DebugPrint("Send crc1 failed!");
        return NULL;
    }

    send = ::send(sock, (PCStr)bundle->GetPayload(), used, 0);
    if (send != used)  {
        DebugPrint("Send Package failed!");
        return NULL;
    }

    send = ::send(sock, (PCStr)&crc2, sizeof(crc2), 0);
    if (send != sizeof(crc2)) {
        DebugPrint("Send crc2 failed!");
        return NULL;
    }

    bundle->Reset();
    Int8 tag;
    if (AFAILED(bundle->ReadInt8(tag)))
        return NULL;

    if ((tag & 0x0F) == 0x04)
        return bundle;
        
    Byte header[2];
    while (mRunFlag) {
        Long recv = ::recv(sock, (PStr)&header, sizeof(header), 0);
        if ((recv != sizeof(header)) || (header[0] != sHeader[0]) || (header[1] != sHeader[1])) {
            DebugPrint("Receive Header failed!");
            return NULL;
        }

        break;
    }
                     
    while (mRunFlag) {
        Long recv = ::recv(sock, (PStr)&used, sizeof(used), 0);
        if (recv != sizeof(used)) {
            DebugPrint("Receive Package Length failed!");
            return NULL;
        }

        break;
    }

    while (mRunFlag) {
        Long recv = ::recv(sock, (PStr)&crc1, sizeof(crc1), 0);
        if (recv != sizeof(crc1)) {
            DebugPrint("Receive crc1 failed!");
            return NULL;
        }

        break;
    }

    Sp<MemoryStream<PARCEL_BUFFER_DEFAULT_LENGTH> > ms = new MemoryStream<PARCEL_BUFFER_DEFAULT_LENGTH>();
    if (ms == NULL)  {
        DebugPrint("alloc \"MemoryStreaam\" failed!");
        return NULL;
    }

    while (mRunFlag && (used > 0)) {
        Byte buffer[1024];
        Long recv;
        if (used > 1024)
            recv = ::recv(sock, (PStr)buffer, 1024, 0);
        else
            recv = ::recv(sock, (PStr)buffer, used, 0);
        
        if (recv < 0) {
            DebugPrint("Receive Package failed!");
            return NULL;
        }

        ms->Write(buffer, recv);
        used -= recv;
    }

    while (mRunFlag) {
        Long recv = ::recv(sock, (PStr)&crc2, sizeof(crc2), 0);
        if (recv != sizeof(crc2)) {
            DebugPrint("Receive crc2 failed!");
            return NULL;
        }

        break;
    }    

    ::CloseSocket(sock);

    return new CParcel(ms);
}

AXP::Void CServerConnection::OnDeath(IN CONST AXP::Sp<AXP::String> & uri)
{
}

AXP::Void CServerConnection::AddRemoteRef(
    IN CONST AXP::Sp<AXP::String> & uri,
    IN AXP::Int64 objRef)
{
}