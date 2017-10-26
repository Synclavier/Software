///  Created by Cameron Jones on 8/26/12.
//

// SynclavierTypes.h

#ifndef __SynclavierTypes__
#define __SynclavierTypes__

#ifndef COMPILE_FOR_MAC
    #if defined(_WIN32)
        #define COMPILE_FOR_MAC 0
        #define COMPILE_FOR_WIN 1
    #else
        #define COMPILE_FOR_MAC 1
        #define COMPILE_FOR_WIN 0
    #endif
#endif

#if COMPILE_FOR_MAC
    typedef unsigned char       SyncUint8;
    typedef unsigned short      SyncUint16;
    typedef unsigned int        SyncUint32;
    typedef unsigned long long  SyncUint64;

    typedef char                SyncSint8;
    typedef short               SyncSint16;
    typedef int                 SyncSint32;
    typedef long long           SyncSint64;

    typedef double              SyncTimeInterval;

    #ifdef __OBJC__
        typedef BOOL            SyncBool;
        typedef id              SyncUIObjectRef;
        typedef unsigned int    SyncUnichar;
    #else
        typedef bool            SyncBool;
        typedef void*           SyncUIObjectRef;
		typedef unsigned int    SyncUnichar;
    #endif

    #define OutputDebugStringA  printf
    #define MemoryBarrier       OSMemoryBarrier
#endif

#if COMPILE_FOR_WIN
    typedef bool                SyncBool;

    typedef unsigned char       SyncUint8;
    typedef unsigned short      SyncUint16;
    typedef unsigned int        SyncUint32;
    typedef unsigned long long  SyncUint64;

    typedef char                SyncSint8;
    typedef short               SyncSint16;
    typedef int                 SyncSint32;
    typedef long long           SyncSint64;

    typedef double              SyncTimeInterval;

	typedef wchar_t             SyncUnichar;
	typedef struct unknown*     SyncUIObjectRef;
#endif

// STL Types
#ifndef	COMPILE_OSX_KERNEL
    #include <string>

    typedef std::string         SyncString;
    typedef SyncString*         SyncStringRef;
    typedef char                SyncStringCharType;
#endif

#endif
