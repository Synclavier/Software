//
//  SyncNSArraySorter.h
//  Synclavier³
//
//  Created by Cameron Jones on 12/5/14.
//  Copyright (c) 2014 Synclavier Digital. All rights reserved.
//

// Can sort any object that responds to 'row'
@protocol SyncNSArraySorterByRow
    - (NSUInteger)row;
@end

typedef NSObject<SyncNSArraySorterByRow> SyncSortableObject;

// Maintains sorted arrays -
//  Array of NSString
//  Array of NSNumber
//  Array of NSObjects sorted by 'row'

// See SyncNSSortedString for more comprehensive solution
// for general array of objects.

@interface SyncNSArraySorter : NSObject
    // NSString arrays. Localized insensitive compare:
    + (NSUInteger) insertNSStringIndex:   (NSString*)string inArray:(NSMutableArray*)array;
    + (NSUInteger) matchNSStringIndex:    (NSString*)string inArray:(NSArray       *)array;
    + (void      ) insertNSString:        (NSString*)string inArray:(NSMutableArray*)array;

    // NSString arrays. Precise comparison.
    + (NSUInteger) insertXNSStringIndex:  (NSString*)string inArray:(NSMutableArray*)array;
    + (NSUInteger) matchXNSStringIndex:   (NSString*)string inArray:(NSArray       *)array;
    + (void      ) insertXNSString:       (NSString*)string inArray:(NSMutableArray*)array;

    // Special cases used somewhere for some unknown purpose
    + (BOOL      ) eitherPrefixNSString:  (NSString*)string inArray:(NSMutableArray*)array;
    + (BOOL      ) hasPrefixNSString:     (NSString*)string inArray:(NSMutableArray*)array;

    // NSNumber unsigned int arrays
    + (NSUInteger) insertNSUINumberIndex: (NSNumber*)number inArray:(NSMutableArray*)array;
    + (NSUInteger) matchNSUINumberIndex:  (NSNumber*)number inArray:(NSMutableArray*)array;
    + (void      ) insertNSUINumber:      (NSNumber*)number inArray:(NSMutableArray*)array;
    + (void      ) removeNSUINumber:      (NSNumber*)number inArray:(NSMutableArray*)array;

    // SyncSortableObject arrays
    + (NSUInteger) insertByRowIndex:      (SyncSortableObject*)object inArray:(NSMutableArray*)array;
    + (NSUInteger) matchByRowIndex:       (SyncSortableObject*)object inArray:(NSMutableArray*)array;
    + (void      ) insertByRow:           (SyncSortableObject*)object inArray:(NSMutableArray*)array;
    + (void      ) removeByRow:           (SyncSortableObject*)object inArray:(NSMutableArray*)array;
@end
