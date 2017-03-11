
#import "AXP/objective-c/AException.h"
#include "IPC/cplusplus/lib/include/IpcException.h"

static void ReadException(int32_t code)
{
    switch (code) {
        case (int32_t)IPC::NoException:
            break;
            
        case (int32_t)IPC::RemoteException:
            [ARemoteException Raise: AXP::AE_FAIL];
            break;
            
        case (int32_t)IPC::RemoteRefException:
            [ARemoteRefException Raise: AXP::AE_FAIL];
            break;
            
        case (int32_t)IPC::ServerBusyException:
            [AServerBusyException Raise: AXP::AE_FAIL];
            break;
            
        default:
            break;
    }
}