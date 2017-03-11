
#ifndef __SERVER_CONNECTION_H__
#define __SERVER_CONNECTION_H__

#include "AXP/cplusplus/xplatform/include/uri.h"
#include "AXP/cplusplus/xplatform/include/Parcel.h"
#include "Stub.h"

namespace IPC
{
    class CServerConnection : public IPC::IStub
    {
    public:

        STATIC AXP::Sp<IPC::IStub> Create(
            IN CONST AXP::Sp<AXP::String> & description);

    protected:

        CServerConnection();

    public:

        AXP::ARESULT Initialize(
            IN CONST AXP::Sp<AXP::Uri> & uri);

        VIRTUAL AXP::Sp<AXP::CParcel> Transact(       
            IN CONST AXP::Sp<AXP::CParcel> & bundle);

        VIRTUAL AXP::Void OnDeath(
            IN CONST AXP::Sp<AXP::String> & uri);

        VIRTUAL AXP::Void AddRemoteRef(
            IN CONST AXP::Sp<AXP::String> & uri,
            IN AXP::Int64 objRef);        

        VIRTUAL ~CServerConnection();

    private:

        AXP::Sp<AXP::String> mUri;
        AXP::Char mIpAddress[16];
        AXP::Int32 mPort;
        AXP::Boolean mRunFlag;
    };
}

#endif // __SERVER_CONNECTION_H__
