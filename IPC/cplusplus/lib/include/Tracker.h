
#ifndef __IPC_TRACKER_H__
#define __IPC_TRACKER_H__

#include "AXP/cplusplus/libc/include/Common/Tracker.h"

#define DEBUG 1
#ifdef DEBUG
#ifdef PLATFORM_WIN32
#define DebugPrint(format, ...) \
    do {       \
        AXP::Char buffer[1024] = "";  \
        if (sprintf_s(buffer, "%s(%d): " format "\n", __FILE__, __LINE__, ##__VA_ARGS__) < 0) \
            break; \
        printf("%s", buffer); \
        AXP::Libc::Common::CTracker::d(buffer); \
    } while(0)
#elif defined(PLATFORM_IOS) || defined(PLATFORM_LINUX)
#define DebugPrint(format, ...) \
    do {       \
        AXP::Char buffer[1024] = "";  \
        if (sprintf(buffer, "%s(%d): " format "\n", __FILE__, __LINE__, ##__VA_ARGS__) < 0) \
            break; \
        printf("%s", buffer); \
        AXP::Libc::Common::CTracker::d(buffer); \
    } while(0)
#endif // PLATFORM_WIN32
#else
#define DebugPrint(...)
#endif  // DEBUG


#endif // __IPC_TRACKER_H__