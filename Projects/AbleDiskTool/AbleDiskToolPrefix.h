#define COMPILE_FOR_MAC 1
#define COMPILE_FOR_OSX 1
#define COMPILE_FOR_IOS 0
#define COMPILE_FOR_WIN 0

#define SYNCLAVIER_APPLICATION_NAME "Synclavier³"

#ifdef __OBJC__
    #define DYNAMIC_CAST(theObject,theClass) ([theObject isKindOfClass:[theClass class]] ? (theClass *) theObject : nil)
    #define SYNC_RETAIN(it)
    #define SYNC_RELEASE(it)
    #define SYNC_SUPER_DEALLOC

    #import <Foundation/Foundation.h>
#endif

#ifdef __cplusplus
    #include <stdio.h>
    #include <CoreFoundation/CoreFoundation.h>
#endif

#define SyncLog(xyz, ...)    {}
#define SyncPrintf(xyz, ...) {}
#define SyncTrap(xyz)        {}
#define SyncCFSTR(xyz)       ""

#define ALLOW_THROW 				false
#define INCLUDE_open_able_file		true
#define KAGI_FREE_TIME				15*60*60
#define REG_CODE_CONSTANT			2
#define COMPILE_OSX					1

// General Mac OS Includes
#include <ApplicationServices/ApplicationServices.h>
#include <CoreServices/CoreServices.h>

// Carbon still required until interchange prefs gets reworked
#include <Carbon/Carbon.h>

// Synclavier Interpreter configuration flags
#define SYNC_USE_ASYNC_IO       0
#define SYNC_USE_KERNEL         0
