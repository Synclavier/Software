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

// Deprecated:
//  CFStringRef SyncNSStringGetLogDateAndTime() {
//      return (CFStringRef) CFBridgingRetain([SyncNSStringUtilities getLogDateAndTime]);
//  }

void SyncNSStringNSLog(const char* msg) {
    NSLog(@"%s", msg);
}

@implementation SyncNSSortedString

// Index for NSBinarySearchingInsertionIndex
- (NSUInteger) insertionIndex:(NSMutableArray*)array {
    NSUInteger where;
    
    where = [array indexOfObject:self
                   inSortedRange:(NSRange){0, [array count]}
                         options:NSBinarySearchingInsertionIndex
                 usingComparator:^NSComparisonResult(id a, id b) {
                     SyncNSSortedString* aAsset = (SyncNSSortedString*)a;
                     SyncNSSortedString* bAsset = (SyncNSSortedString*)b;
                     
                     return [aAsset->sortKey localizedCaseInsensitiveCompare:bAsset->sortKey];
                 }];
    
    return where;
}

// Index for NSBinarySearchingInsertionIndex
+ (NSUInteger) insertionIndex:(NSMutableArray*)array forSortKey:(NSString*)sortKey {
    NSUInteger where;
    
    where = [array indexOfObject:sortKey
                   inSortedRange:(NSRange){0, [array count]}
                         options:NSBinarySearchingInsertionIndex
                 usingComparator:^NSComparisonResult(id a, id b) {
                     NSString*           aString = nil;
                     NSString*           bString = nil;
                     SyncNSSortedString* aAsset  = DYNAMIC_CAST(a, SyncNSSortedString);
                     SyncNSSortedString* bAsset  = DYNAMIC_CAST(b, SyncNSSortedString);
        
                     if (aAsset) aString = aAsset->sortKey;
                     else        aString = DYNAMIC_CAST(a, NSString);
                     
                     if (bAsset) bString = bAsset->sortKey;
                     else        bString = DYNAMIC_CAST(b, NSString);
        
                     if (aString == nil || bString == nil)
                         return NSOrderedSame;
                     
                     return [aString localizedCaseInsensitiveCompare:bString];
                 }];
    
    return where;
}

// Index for NSBinarySearchingInsertionIndex
- (NSUInteger) insertionIndex:(NSMutableArray*)array forKey:(NSString*)key {
    NSUInteger where;
    
    where = [array indexOfObject:self
                   inSortedRange:(NSRange){0, [array count]}
                         options:NSBinarySearchingInsertionIndex
                 usingComparator:^NSComparisonResult(id a, id b) {
                     SyncNSSortedString* aAsset = (SyncNSSortedString*)a;
                     SyncNSSortedString* bAsset = (SyncNSSortedString*)b;
                     
                     return [[aAsset valueForKey:key] localizedCaseInsensitiveCompare:[bAsset valueForKey:key]];
                 }];
    
    return where;
}

// Exact match
- (void) insertionXNew:(NSMutableArray*)array {
    NSUInteger where;
    
    where = [array indexOfObject:self
                   inSortedRange:(NSRange){0, [array count]}
                         options:NSBinarySearchingInsertionIndex
                 usingComparator:^NSComparisonResult(id a, id b) {
                     SyncNSSortedString* aAsset = (SyncNSSortedString*)a;
                     SyncNSSortedString* bAsset = (SyncNSSortedString*)b;
                     
                     return [aAsset->sortKey compare:bAsset->sortKey];
                 }];
    
    if (where != NSNotFound)
        [array insertObject:self atIndex:where];
}



// Object with matching name
- (SyncNSSortedString*)matchByName:(NSMutableArray*)array returnIndex:(NSUInteger*)index {
    NSUInteger i = [array indexOfObject:self
                          inSortedRange:(NSRange){0, [array count]}
                                options:0
                        usingComparator:^NSComparisonResult(id a, id b) {
                            SyncNSSortedString* aAsset = (SyncNSSortedString*)a;
                            SyncNSSortedString* bAsset = (SyncNSSortedString*)b;
                            
                            return [aAsset->sortKey localizedCaseInsensitiveCompare:bAsset->sortKey];
                        }];
    
    if (index)
        *index = i;
    
    if (i == NSNotFound)
        return nil;
    
    else
        return [array objectAtIndex:i];
}

// Object with matching name
+ (SyncNSSortedString*)matchByName:(NSMutableArray*)array returnIndex:(nullable NSUInteger*)index forSortKey:(NSString*)sortKey {
    NSUInteger i = [array indexOfObject:sortKey
                          inSortedRange:(NSRange){0, [array count]}
                                options:0
                        usingComparator:^NSComparisonResult(id a, id b) {
                            NSString*           aString = nil;
                            NSString*           bString = nil;
                            SyncNSSortedString* aAsset  = DYNAMIC_CAST(a, SyncNSSortedString);
                            SyncNSSortedString* bAsset  = DYNAMIC_CAST(b, SyncNSSortedString);

                            if (aAsset) aString = aAsset->sortKey;
                            else        aString = DYNAMIC_CAST(a, NSString);

                            if (bAsset) bString = bAsset->sortKey;
                            else        bString = DYNAMIC_CAST(b, NSString);

                            if (aString == nil || bString == nil)
                             return NSOrderedSame;

                            return [aString localizedCaseInsensitiveCompare:bString];
                        }];
    
    if (index)
        *index = i;
    
    if (i == NSNotFound)
        return nil;
    
    else
        return [array objectAtIndex:i];
}

- (SyncNSSortedString*)matchByName:(NSMutableArray*)array returnIndex:(NSUInteger*)index forKey:(NSString*)key {
    NSUInteger i = [array indexOfObject:self
                          inSortedRange:(NSRange){0, [array count]}
                                options:0
                        usingComparator:^NSComparisonResult(id a, id b) {
                            SyncNSSortedString* aAsset = (SyncNSSortedString*)a;
                            SyncNSSortedString* bAsset = (SyncNSSortedString*)b;
                            
                            return [[aAsset valueForKey:key] localizedCaseInsensitiveCompare:[bAsset valueForKey:key]];
                        }];
    
    if (index)
        *index = i;
    
    if (i == NSNotFound)
        return nil;
    
    else
        return [array objectAtIndex:i];
}

// Object with matching name
+ (SyncNSSortedString*)matchByName:(NSMutableArray*)array returnIndex:(nullable NSUInteger*)index forKey:(NSString*)key keyValue:(NSString*)keyValue {
    NSUInteger i = [array indexOfObject:keyValue
                          inSortedRange:(NSRange){0, [array count]}
                                options:0
                        usingComparator:^NSComparisonResult(id a, id b) {
                            NSString*           aString = nil;
                            NSString*           bString = nil;
                            SyncNSSortedString* aAsset  = DYNAMIC_CAST(a, SyncNSSortedString);
                            SyncNSSortedString* bAsset  = DYNAMIC_CAST(b, SyncNSSortedString);

                            if (aAsset) aString = [aAsset valueForKey:key];
                            else        aString = DYNAMIC_CAST(a, NSString);

                            if (bAsset) bString = [bAsset valueForKey:key];
                            else        bString = DYNAMIC_CAST(b, NSString);

                            if (aString == nil || bString == nil)
                             return NSOrderedSame;

                            return [aString localizedCaseInsensitiveCompare:bString];
                        }];
    
    if (index)
        *index = i;
    
    if (i == NSNotFound)
        return nil;
    
    else
        return [array objectAtIndex:i];
}

- (SyncNSSortedString*)matchXByName:(NSMutableArray*)array returnIndex:(NSUInteger*)index {
    NSUInteger i = [array indexOfObject:self
                          inSortedRange:(NSRange){0, [array count]}
                                options:0
                        usingComparator:^NSComparisonResult(id a, id b) {
                            SyncNSSortedString* aAsset = (SyncNSSortedString*)a;
                            SyncNSSortedString* bAsset = (SyncNSSortedString*)b;
                            
                            return [aAsset->sortKey compare:bAsset->sortKey];
                        }];
    
    if (index)
        *index = i;
    
    if (i == NSNotFound)
        return nil;
    
    else
        return [array objectAtIndex:i];
}

// Object with matching name
+ (SyncNSSortedString*)matchXByName:(NSMutableArray*)array returnIndex:(nullable NSUInteger*)index forSortKey:(NSString*)sortKey {
    NSUInteger i = [array indexOfObject:sortKey
                          inSortedRange:(NSRange){0, [array count]}
                                options:0
                        usingComparator:^NSComparisonResult(id a, id b) {
                            NSString*           aString = nil;
                            NSString*           bString = nil;
                            SyncNSSortedString* aAsset  = DYNAMIC_CAST(a, SyncNSSortedString);
                            SyncNSSortedString* bAsset  = DYNAMIC_CAST(b, SyncNSSortedString);

                            if (aAsset) aString = aAsset->sortKey;
                            else        aString = DYNAMIC_CAST(a, NSString);

                            if (bAsset) bString = bAsset->sortKey;
                            else        bString = DYNAMIC_CAST(b, NSString);

                            if (aString == nil || bString == nil)
                             return NSOrderedSame;

                            return [aString compare:bString];
                        }];
    
    if (index)
        *index = i;
    
    if (i == NSNotFound)
        return nil;
    
    else
        return [array objectAtIndex:i];
}

+ (NSString*) standardizeString:(NSString*) inString {
    if (inString == nil)
        return nil;
    
    NSMutableString*    buffer    = [inString mutableCopy];
    CFMutableStringRef  bufferRef = (__bridge_retained CFMutableStringRef)buffer;
    
    if (CFStringTransform(bufferRef, NULL, CFSTR("Any-Latin; Latin-ASCII; Any-Upper"), false))
        return (__bridge_transfer NSString*) bufferRef;
    
    else {
        CFRelease(bufferRef);
        return [inString uppercaseStringWithLocale:nil];
    }
}

@end

