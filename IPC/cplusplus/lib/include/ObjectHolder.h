
#ifndef __OBJECTHOLDER_H__
#define __OBJECTHOLDER_H__

#include "AXP/cplusplus/xplatform/include/Parcel.h"
#include "AXP/cplusplus/xplatform/include/astring.h"

namespace IPC
{
    class CObjectHolder : public AXP::CObject
    {
    public:

        VIRTUAL AXP::Sp<AXP::CParcel> OnTransact(
            IN CONST AXP::Sp<AXP::CParcel> & bundle,
            IN CONST AXP::Sp<AXP::String> & uri) = 0;

    public:

        AXP::Int32 AddRemoteRef()
        {
            return AXP::CInterlocked::Increment(&mRemoteRefCount);
        }

        AXP::Int32 DecreaseRemoteRef()
        {
            return AXP::CInterlocked::Decrement(&mRemoteRefCount);
        }

    public:

        CObjectHolder()
        {
            mRemoteRefCount = 0;
        }

    private:

        VOLATILE AXP::Int32 mRemoteRefCount;

    public:

        AXP::Sp<AXP::List<AXP::String> > mUriList = new AXP::List<AXP::String>();
    };
}

#endif // __OBJECTHOLDER_H__
