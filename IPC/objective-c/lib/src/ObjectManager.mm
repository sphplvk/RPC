
#import <Foundation/Foundation.h>
#include "AXP/cplusplus/xplatform/include/mutex.h"

STATIC AXP::Mutex sInstanceListLock;
STATIC NSMutableDictionary * sInstanceList = [NSMutableDictionary dictionary];

AXP::Boolean ObjectManager_RegisterObject(
    IN AXP::Int64 key,
    IN id value)
{
    @autoreleasepool {
        Synchronized(&sInstanceListLock) {
            [sInstanceList setObject: value forKey: [NSNumber numberWithLongLong: key]];
            return TRUE;
        }

        return FALSE;
    }
}

AXP::Boolean ObjectManager_UnregisterObject(
    IN AXP::Int64 key)
{
    @autoreleasepool {
    Synchronized(&sInstanceListLock) {
        [sInstanceList removeObjectForKey: [NSNumber numberWithLongLong: key]];
     
        return TRUE;
    }
    
    return FALSE;
    }
}

id ObjectManager_GetObject(
    IN AXP::Int64 key)
{
    @autoreleasepool {
        Synchronized(&sInstanceListLock) {
            return [sInstanceList objectForKey: [NSNumber numberWithLongLong: key]];
        }
        
        return nil;
    }
}