//
//  SyncCFStringUtilities.h
//  TermulatorX-XCode4-10.8
//
//  Created by Cameron Jones on 8/31/14.
//
//

#ifndef __TermulatorX_XCode4_10_8__SyncCFStringUtilities__
#define __TermulatorX_XCode4_10_8__SyncCFStringUtilities__

// Convenience interface from CFStringRef to C Strings
const char* SyncCFStringUtilitiesTempString(CFStringRef cfString);

void        SyncCFCStringCopy   (char * cString,          CFStringRef cfString, int max);
void        SyncCFPStringCopy   (unsigned char * pString, CFStringRef cfString, int max);
char        SyncCFStringHexByte1(char it);
char        SyncCFStringHexByte2(char it);
CFStringRef SyncHashForString   (CFStringRef str);
int         SyncIntFromHexString(CFStringRef str);

#endif /* defined(__TermulatorX_XCode4_10_8__SyncCFStringUtilities__) */
