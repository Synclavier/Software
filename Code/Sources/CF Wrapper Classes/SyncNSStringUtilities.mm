//
//  SyncNSStringUtilities.m
//  Synclavier³
//
//  Created by Cameron Jones on 12/8/14.
//  Copyright (c) 2014 Synclavier Digital. All rights reserved.
//

#import "SyncNSStringUtilities.h"

@implementation SyncNSStringUtilities

+ (NSString*) getLogDateAndTime {
    NSDateFormatter *dateFormatter = [[NSDateFormatter alloc] init];
    [dateFormatter setDateStyle:NSDateFormatterMediumStyle];
    [dateFormatter setTimeStyle:NSDateFormatterMediumStyle];
    
    NSString *formattedDateString = [dateFormatter stringFromDate:[NSDate date]];
    
    return formattedDateString;
}

+ (NSString*) getAccurateLogDateAndTime {
    NSDateFormatter *dateFormatter = [[NSDateFormatter alloc] init];
    [dateFormatter setDateStyle:NSDateFormatterMediumStyle];
    [dateFormatter setTimeStyle:NSDateFormatterFullStyle];
    
    NSString *formattedDateString = [dateFormatter stringFromDate:[NSDate date]];
    
    return formattedDateString;
}

+ (NSMutableAttributedString*) attributedLogDateAndTime {
    return [[NSMutableAttributedString alloc] initWithString:[self getLogDateAndTime] attributes:nil];
}

@end

CFStringRef SyncNSStringGetLogDateAndTime() {
    return (CFStringRef) CFBridgingRetain([SyncNSStringUtilities getLogDateAndTime]);
}

void        SyncNSStringNSLog(const char* msg) {
    NSLog(@"%s", msg);
}


