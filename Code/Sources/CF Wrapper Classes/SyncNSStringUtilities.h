//
//  SyncNSStringUtilities.h
//  Synclavier³
//
//  Created by Cameron Jones on 12/8/14.
//  Copyright (c) 2014 Synclavier Digital. All rights reserved.
//

#ifdef __OBJC__
    @interface SyncNSStringUtilities : NSObject

    + (NSString*)                  getLogDateAndTime;
    + (NSString*)                  getAccurateLogDateAndTime;
    + (NSMutableAttributedString*) attributedLogDateAndTime;

    @end
#endif

CFStringRef SyncNSStringGetLogDateAndTime   ();
void        SyncNSStringNSLog               (const char* msg);

