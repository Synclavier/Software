//
//  SyncNSArraySorter.m
//  Synclavier³
//
//  Created by Cameron Jones on 12/5/14.
//  Copyright (c) 2014 Synclavier Digital. All rights reserved.
//

#import "SyncNSArraySorter.h"

@implementation SyncNSArraySorter

// Sorted object is NSString
+ (NSUInteger) insertNSStringIndex:(NSString*)string inArray:(NSMutableArray*)array {
    return  [array indexOfObject:string
                   inSortedRange:(NSRange){0, [array count]}
                         options:NSBinarySearchingInsertionIndex
                 usingComparator:^NSComparisonResult(id a, id b) {
                     NSString* aString = (NSString*)a;
                     NSString* bString = (NSString*)b;
                     
                     return [aString localizedCaseInsensitiveCompare:bString];
                 }];
}

+ (NSUInteger) matchNSStringIndex:(NSString*)string  inArray:(NSArray*)array {
    return  [array indexOfObject:string
                   inSortedRange:(NSRange){0, [array count]}
                         options:0
                 usingComparator:^NSComparisonResult(id a, id b) {
                     NSString* aString = (NSString*)a;
                     NSString* bString = (NSString*)b;
                     
                     return [aString localizedCaseInsensitiveCompare:bString];
                 }];
}

+ (void) insertNSString:(NSString*)string inArray:(NSMutableArray*)array {
    NSUInteger whereAt = [self insertNSStringIndex:string inArray:array];
    
    [array insertObject:string atIndex:whereAt];
}

// Sorted object is NSString with exact match
+ (NSUInteger) insertXNSStringIndex:(NSString*)string inArray:(NSMutableArray*)array {
    return  [array indexOfObject:string
                   inSortedRange:(NSRange){0, [array count]}
                         options:NSBinarySearchingInsertionIndex
                 usingComparator:^NSComparisonResult(id a, id b) {
                     NSString* aString = (NSString*)a;
                     NSString* bString = (NSString*)b;
                     
                     return [aString compare:bString];
                 }];
}

+ (NSUInteger) matchXNSStringIndex:(NSString*)string  inArray:(NSArray*)array {
    return  [array indexOfObject:string
                   inSortedRange:(NSRange){0, [array count]}
                         options:0
                 usingComparator:^NSComparisonResult(id a, id b) {
                     NSString* aString = (NSString*)a;
                     NSString* bString = (NSString*)b;
                     
                     return [aString compare:bString];
                 }];
}

+ (void) insertXNSString:(NSString*)string inArray:(NSMutableArray*)array {
    NSUInteger whereAt = [self insertXNSStringIndex:string inArray:array];
    
    [array insertObject:string atIndex:whereAt];
}

+ (BOOL) eitherPrefixNSString:(NSString*)string inArray:(NSMutableArray*)array {
    NSUInteger whereAt = [self insertNSStringIndex:string inArray:array];
    
    if (whereAt < [array count]) {
        NSString* aString = [array objectAtIndex:whereAt];
        
        if ([aString hasPrefix:string ]) return YES;
        if ([string  hasPrefix:aString]) return YES;
    }
    
    if (whereAt > 0) {
        NSString* aString = [array objectAtIndex:whereAt-1];
        
        if ([aString hasPrefix:string ]) return YES;
        if ([string  hasPrefix:aString]) return YES;
    }
    
    return NO;
}

+ (BOOL) hasPrefixNSString:(NSString*)string inArray:(NSMutableArray*)array {
    NSUInteger whereAt = [self insertNSStringIndex:string inArray:array];
    
    if (whereAt < [array count]) {
        NSString* aString = [array objectAtIndex:whereAt];
        
        if ([string  hasPrefix:aString]) return YES;
    }
    
    if (whereAt > 0) {
        NSString* aString = [array objectAtIndex:whereAt-1];
        
        if ([string  hasPrefix:aString]) return YES;
    }
    
    return NO;
}


// Sorted object is NSNumber unsigned int arrays
+ (NSUInteger) insertNSUINumberIndex:(NSNumber*)number inArray:(NSMutableArray*)array {
    return  [array indexOfObject:number
                   inSortedRange:(NSRange){0, [array count]}
                         options:NSBinarySearchingInsertionIndex
                 usingComparator:^NSComparisonResult(id a, id b) {
                     NSNumber* aNumber = (NSNumber*)a;
                     NSNumber* bNumber = (NSNumber*)b;
                     
                     if ( aNumber.unsignedIntValue < bNumber.unsignedIntValue ) {
                         return NSOrderedAscending;
                     } else if ( aNumber.unsignedIntValue > bNumber.unsignedIntValue ) {
                         return NSOrderedDescending;
                     } else {
                         return NSOrderedSame;
                     }
                 }];
}

+ (NSUInteger) matchNSUINumberIndex:(NSNumber*)number inArray:(NSMutableArray*)array {
    return  [array indexOfObject:number
                   inSortedRange:(NSRange){0, [array count]}
                         options:0
                 usingComparator:^NSComparisonResult(id a, id b) {
                     NSNumber* aNumber = (NSNumber*)a;
                     NSNumber* bNumber = (NSNumber*)b;
                     
                     if ( aNumber.unsignedIntValue < bNumber.unsignedIntValue ) {
                         return NSOrderedAscending;
                     } else if ( aNumber.unsignedIntValue > bNumber.unsignedIntValue ) {
                         return NSOrderedDescending;
                     } else {
                         return NSOrderedSame;
                     }
                 }];
}

+ (void) insertNSUINumber:(NSNumber*)number inArray:(NSMutableArray*)array {
    NSUInteger whereAt = [self insertNSUINumberIndex:number inArray:array];
    
    [array insertObject:number atIndex:whereAt];
}

+ (void) removeNSUINumber:(NSNumber*)number inArray:(NSMutableArray*)array {
    NSUInteger whereAt = [self matchNSUINumberIndex:number inArray:array];
    
    if (whereAt != NSNotFound)
        [array removeObjectAtIndex:whereAt];
}



// SyncNSArraySorterByRow arrays
+ (NSUInteger) insertByRowIndex:(SyncSortableObject*)object inArray:(NSMutableArray*)array {
    return  [array indexOfObject:object
                   inSortedRange:(NSRange){0, [array count]}
                         options:NSBinarySearchingInsertionIndex
                 usingComparator:^NSComparisonResult(id a, id b) {
                     SyncSortableObject* aObject = (SyncSortableObject*)a;
                     SyncSortableObject* bObject = (SyncSortableObject*)b;
                     
                     if ( aObject.row < bObject.row )
                         return NSOrderedAscending;
                     
                     else if ( aObject.row > bObject.row )
                         return NSOrderedDescending;
                     
                     else
                         return NSOrderedSame;
                 }];
}

// Returns lowest match, or end of array, or next greater
+ (NSUInteger) matchByRowIndex:(SyncSortableObject*)object inArray:(NSMutableArray*)array {
    return  [array indexOfObject:object
                   inSortedRange:(NSRange){0, [array count]}
                         options:NSBinarySearchingInsertionIndex + NSBinarySearchingFirstEqual         // Must find first equal row
                 usingComparator:^NSComparisonResult(id a, id b) {
                     SyncSortableObject* aObject = (SyncSortableObject*)a;
                     SyncSortableObject* bObject = (SyncSortableObject*)b;
                     
                     if ( aObject.row < bObject.row )
                         return NSOrderedAscending;
                     
                     else if ( aObject.row > bObject.row )
                         return NSOrderedDescending;
                     
                     else
                         return NSOrderedSame;
                 }];
}

+ (void) insertByRow:(SyncSortableObject*)object inArray:(NSMutableArray*)array {
    NSUInteger whereAt = [self insertByRowIndex:object inArray:array];
    
    [array insertObject:object atIndex:whereAt];
}

+ (void) removeByRow:(SyncSortableObject*)object inArray:(NSMutableArray*)array {
    NSUInteger whereAt = [self matchByRowIndex:object inArray:array];
    
    // Handle multiple items on same row intelligently. If we found the first one, find the exact one and delete it
    if (whereAt >= [array count]) {                      // Could be a problem if no object matches
        SyncLog(@"removeByRow failed whereAt >= [array count] %p", object);
        return;
    }
    
    // Matches row number; look for equality
    while ([array objectAtIndex:whereAt] != object) {     // Hmmm... Matching row but not found...
        if (++whereAt >= [array count]) {
            SyncLog(@"removeByRow failed ++whereAt >= [array count] %p", object);
           return;
        }
    }
    
    [array removeObjectAtIndex:whereAt];                // Removed the matching object
}

@end
