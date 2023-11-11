//
//  SyncNSStringUtilities.h
//  Synclavier³
//
//  Created by Cameron Jones on 12/8/14.
//  Copyright (c) 2014 Synclavier Digital. All rights reserved.
//

#ifdef __OBJC__
    // Misc string utilities for logging
    @interface SyncNSStringUtilities : NSObject
        + (NSString* _Nonnull)                  getLogDateAndTime;
        + (NSString* _Nonnull)                  getAccurateLogDateAndTime;
        + (NSMutableAttributedString* _Nonnull) attributedLogDateAndTime;
    @end

    // Sorted array of objects using localizedCaseInsensitiveCompare. Objects
    // typically descend from SyncNSSortedString using its sortKey.

    // Objects can be sorted using other fields by using the "forKey" methods.

    @interface SyncNSSortedString : NSObject {
    @public
        NSString*   sortKey;
    }

        - (NSUInteger)                   insertionIndex:   (NSMutableArray* _Nonnull)array;
        + (NSUInteger)                   insertionIndex:   (NSMutableArray* _Nonnull)array forSortKey: (NSString*_Nonnull)sortKey;
        - (NSUInteger)                   insertionIndex:   (NSMutableArray* _Nonnull)array forKey:     (NSString*_Nonnull)key;
        
        - (void)                         insertionXNew:    (NSMutableArray* _Nonnull)array;

        - (nullable SyncNSSortedString*) matchByName:      (NSMutableArray* _Nonnull)array returnIndex:(nullable NSUInteger*)index;
        + (nullable SyncNSSortedString*) matchByName:      (NSMutableArray* _Nonnull)array returnIndex:(nullable NSUInteger*)index forSortKey:(NSString* _Nonnull)sortKey;
        - (nullable SyncNSSortedString*) matchByName:      (NSMutableArray* _Nonnull)array returnIndex:(nullable NSUInteger*)index forKey:    (NSString* _Nonnull)key;
        + (nullable SyncNSSortedString*) matchByName:      (NSMutableArray* _Nonnull)array returnIndex:(nullable NSUInteger*)index forKey:    (NSString* _Nonnull)key keyValue:(NSString* _Nonnull)keyValue;

        - (nullable SyncNSSortedString*) matchXByName:     (NSMutableArray* _Nonnull)array returnIndex:(nullable NSUInteger*)index;
        + (nullable SyncNSSortedString*) matchXByName:     (NSMutableArray* _Nonnull)array returnIndex:(nullable NSUInteger*)index forSortKey:(NSString* _Nonnull)sortKey;

        + (NSString* _Nonnull)           standardizeString:(NSString* _Nonnull) inString;
    @end

#endif

void        SyncNSStringNSLog               (const char* _Nullable msg);

// Deprecated:
//      CFStringRef SyncNSStringGetLogDateAndTime   ();
