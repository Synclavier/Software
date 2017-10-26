//
//  SyncNotification.cpp
//  Synclavier³
//
//  Created by Cameron Jones on 11/24/14.
//  Copyright (c) 2014 Synclavier Digital. All rights reserved.
//

#include "SyncNotification.h"

void    SyncNotificationPostNotification(CFStringRef notification, void* objRef) {
    [[NSNotificationCenter defaultCenter] postNotificationName:(__bridge NSString*)notification object:(__bridge id)objRef];
}
