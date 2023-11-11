//
//  SyncCFStringUtilities.cpp
//  TermulatorX-XCode4-10.8
//
//  Created by Cameron Jones on 8/31/14.
//
//

#include <atomic>

#include "SyncCFStringUtilities.h"
#include "sha1.h"

// Temporary string buffer for debug printouts. Allows up to 4 SyncCFSTR uses in one SyncPrintf...
static  std::atomic_int32_t SyncCFStringUtilitiesWhichTempStringBuffer;
static  char                SyncCFStringUtilitiesTempStringBuffer[4][256];

const char* SyncCFStringUtilitiesTempString(CFStringRef cfString)
{
    int32_t which_buffer = SyncCFStringUtilitiesWhichTempStringBuffer.fetch_add(1);

    char* whichString = SyncCFStringUtilitiesTempStringBuffer[which_buffer&3];
    
    if (cfString == NULL)
        memset(whichString, 0, sizeof(SyncCFStringUtilitiesTempStringBuffer[0]));
    
    else
        CFStringGetCString(cfString, whichString,  sizeof(SyncCFStringUtilitiesTempStringBuffer[0]), kCFStringEncodingUTF8);
    
    return whichString;
}

// Copies from a CFStringRef into a C string
void    SyncCFCStringCopy(char * cString, CFStringRef cfString, int max)
{
    if (cfString && CFStringGetCString(cfString, cString, (CFIndex) max, kCFStringEncodingUTF8))
        return;
    
    memset(cString, 0, (size_t) max);
}

// Copies from a CFStringRef into a P string
void    SyncCFPStringCopy(unsigned char * pString, CFStringRef cfString, int max) {
    if (cfString && CFStringGetPascalString(cfString, pString, (CFIndex) max, kCFStringEncodingUTF8))
        return;
    
    memset(pString, 0, (size_t) max);
}

// Convenience routines
char    SyncCFStringHexByte1(char it)
{
    return SyncCFStringHexByte2(it >> 4);
}

char    SyncCFStringHexByte2(char it)
{
    return ((it&0xf) < 10) ? '0'+(it&0xf) : 'A'-10+(it&0xf);
}

CFStringRef SyncHashForString(CFStringRef str)
{
    SHA1_INFO   sha1_info;
    char        utf8Info[512] = {0};
    char        code    [512] = {0};
    
    CFMutableStringRef mutableString = CFStringCreateMutableCopy(NULL, 0, str);
    CFLocaleRef        localeRef     = CFLocaleCreate(NULL, CFSTR("C"));

    CFStringUppercase(mutableString, localeRef);
    
    // Get string in UTF-8
    Boolean result = CFStringGetCString(mutableString, utf8Info, sizeof(utf8Info), kCFStringEncodingUTF8);

    CFRelease(localeRef    ); localeRef     = NULL;
    CFRelease(mutableString); mutableString = NULL;

    if (!result)
        return NULL;
    
    unsigned char hash[20];
    
    sha1_init  (&sha1_info);
    sha1_update(&sha1_info, (const uint8_t*) utf8Info, (int) strlen(utf8Info));
    sha1_final (&sha1_info, hash);
    
    for (int i=0; i<4; i++) {
        code[(i*5)+0] = SyncCFStringHexByte1(hash[i*2+0]);
        code[(i*5)+1] = SyncCFStringHexByte2(hash[i*2+0]);
        code[(i*5)+2] = SyncCFStringHexByte1(hash[i*2+1]);
        code[(i*5)+3] = SyncCFStringHexByte2(hash[i*2+1]);
        
        if (i<3)
            code[(i*5)+4] = '-';    // Insert dash
        else
            code[(i*5)+4] = 0;      // Append the trailing C-string null
    }
    
    return CFStringCreateWithCString(NULL, code, kCFStringEncodingUTF8);
}

int SyncIntFromHexString(CFStringRef str)
{
    char utf8Info[512] = {0};
    
    // Get string in UTF-8
    if (!CFStringGetCString(str, utf8Info, sizeof(utf8Info), kCFStringEncodingUTF8))
        return 0;
    
    if (CFStringGetLength(str) == 0)
        return 0;
    
    int value = 0;
    
    for (int i=0; i<CFStringGetLength(str); i++) {
        char ch = toupper(utf8Info[i]);

        if (ch >= '0' && ch <= '9')
            value = (value << 4) | (ch - '0');
            
        else if (ch >= 'A' && ch <= 'F')
            value = (value << 4) | (10 + (ch - 'A'));
        
        else
           return 0;
    }
    
    return value;
}

