
#ifndef __ISTUB_H__
#define __ISTUB_H__

#include "AXP/cplusplus/xplatform/include/Parcel.h"

namespace IPC
{
    class CommandCode
    {
    public:

        STATIC CONST AXP::Int32 COMMAND_CREATE = 0;
        STATIC CONST AXP::Int32 COMMAND_CALLBACK = 1;
        STATIC CONST AXP::Int32 COMMAND_CALL = 2;
        STATIC CONST AXP::Int32 COMMAND_RELEASE = 3;
    };
    

    class IStub : public AXP::CObject
    {
    public:

        VIRTUAL AXP::Sp<AXP::CParcel> Transact(
            IN CONST AXP::Sp<AXP::CParcel> & bundle) = 0;

        VIRTUAL AXP::Void OnDeath(
            IN CONST AXP::Sp<AXP::String> & uri) = 0;

        VIRTUAL AXP::Void AddRemoteRef(
            IN CONST AXP::Sp<AXP::String> & uri,
            IN AXP::Int64 objRef) = 0;
    };
}

#endif // __ISTUB_H__
