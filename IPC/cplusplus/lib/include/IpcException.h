
#ifndef __IPC_EXCEPTION_H__
#define __IPC_EXCEPTION_H__

#include <exception>
#include "AXP/cplusplus/xplatform/include/type.h"

namespace IPC
{
    enum Exception
    {
        NoException,
        RemoteException,
        RemoteRefException,
        ServerBusyException,
    };

    class CException : public std::exception
    {
    };

    class CRemoteException : public CException
    {
    };

    class CRemoteRefException : public CRemoteException
    {
    };

    class CServerBusyException : public CRemoteException
    {
    };

    STATIC AXP::Void ReadException(AXP::Int32 code)
    {
        switch (code) {
        case (AXP::Int32)IPC::NoException:
            break;

        case (AXP::Int32)IPC::RemoteException:
            throw IPC::CRemoteException();
            break;

        case (AXP::Int32)IPC::RemoteRefException:
            throw IPC::CRemoteRefException();
            break;

        case (AXP::Int32)IPC::ServerBusyException:
            throw IPC::CServerBusyException();
            break;

        default:
            break;
        }
    }
}

#endif // __IPC_EXCEPTION_H__
