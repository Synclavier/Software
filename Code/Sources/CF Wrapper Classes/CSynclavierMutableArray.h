//
//  SyncCFWrappers.h
//
//  Created by Cameron Jones on 3/22/14.
//  Copyright (c) 2014 Cameron Jones. All rights reserved.
//

#ifndef __SynclavierCP__SyncCFWrappers__
#define __SynclavierCP__SyncCFWrappers__

#include "Synclavier3Constants.h"

// We define our own set of foundation classes. For OSX they are
// based upon Core Foundation

// The intent is to use our own classes so that future migration
// to Windows might be easier.

// Log to terminal screen
int	 SyncDebugPrintf(const char * __restrict, ...) __printflike(1, 2);


// CSynclavierMutableArray is a wrapper class that embodies basic mutable array functionality.
// It is designed to be easy to port to other platforms.

typedef struct __CFArray*   SyncBaseArrayRef;

class   CSynclavierMutableArray
{
public:
    CSynclavierMutableArray ();
    CSynclavierMutableArray (SyncUint32 startingCapacity);
    ~CSynclavierMutableArray();
    
    SyncUint32          Count   ();
    const void*         ItemAt  (SyncUint32 index) const;
    void                SetAt   (SyncUint32 index, const void *item);
    void                RemoveAt(SyncUint32 index);
    void                Remove  (const void *item);
    void                PushLast(const void *item);
    const void*         PopLast ();
    bool                Contains(const void *item);
    SyncUint32          IndexOf (const void *item);
    void                Empty   ();
    void                Reserve (SyncUint32 startingCapacity);
    
    // Type cast to CFArrayRef
    inline                      operator SyncBaseArrayRef() const   {return array;}
    inline  SyncBaseArrayRef    GetArrayRef()                       {return array;}

protected:
    SyncBaseArrayRef    array;
};

typedef CSynclavierMutableArray* CSynclavierMutableArrayRef;


// This is a version of CSynclavierMutableArray that includes
// some 1-based indexing methods. It is provided to simplify transitioning
// from PowerPlant LArray derivatives.
class   CSynclavierMutable1BasedArray : protected CSynclavierMutableArray
{
public:
    
    // These are 1-based index methods
    const void*         operator []   (SyncUint32 index) const      {if (index == 0) return NULL; else return ItemAt(index-1);}
    SyncUint32          AddItem       (const void *item)            {PushLast(item); return Count();}
    SyncUint32          GetCount      ()                            {return Count();   }
    bool                ValidIndex    (SyncUint32 index)            {return (index > 0 && index <= Count());}
    void                RemoveItemAt  (SyncUint32 index)            {RemoveAt(index-1);}
    const void*         RemoveLastItem(                )            {return PopLast(); }
};

typedef CSynclavierMutable1BasedArray* CSynclavierMutable1BasedArrayRef;


// This is a sorted version based around CFArray Binary Sorting.
// It also includes the 1-based indexing methods
class   CSynclavierSortedMutableArray : public CSynclavierMutable1BasedArray
{
public:
    CSynclavierSortedMutableArray();
    
    SyncUint32              FetchInsertIndexOfKey(const void *key );
    SyncUint32              FetchIndexOfKey      (const void *key );
    SyncUint32              FetchIndexOf         (const void *item);
    SyncUint32              InsertItem           (const void *item);
    
    void                    SetComparator        (class CSynclavierComparator* inComparator);
    
    CSynclavierComparator*  mComparator;
    Boolean                 mOwnsComparator;
    Boolean                 mIsSorted;
    const void*             mComparisonKey;
    
};

typedef CSynclavierSortedMutableArray* CSynclavierSortedMutableArrayRef;


// Iterator
class   CSynclavierMutableArrayIterator
{
public:
    CSynclavierMutableArrayIterator(CSynclavierMutableArray& array);

    Boolean			Next( void* outItem );
    
    // Innards not implemented yet

protected:
};
#endif /* defined(__SynclavierCP__SyncCFWrappers__) */
