//
//  SyncCFWrappers.cpp
//
//  Created by Cameron Jones on 3/22/14.
//  Copyright (c) 2014 Cameron Jones. All rights reserved.
//

#include "CSynclavierMutableArray.h"
#include "CSynclavierComparator.h"

// --------------------------------
// CSynclavierMutableArray
// --------------------------------

// A simple mutable array classed based upon CFMutableArray

CSynclavierMutableArray::CSynclavierMutableArray()
{
    array = CFArrayCreateMutable(NULL, NULL, NULL);
};

CSynclavierMutableArray::CSynclavierMutableArray(SyncUint32 startingCapacity)
{
    if (startingCapacity > 0)
        array = CFArrayCreateMutable(NULL, (CFIndex) startingCapacity, NULL);
    else
        array = NULL;
};

CSynclavierMutableArray::~CSynclavierMutableArray()
{
    if (array)
        CFRelease(array);
};

SyncUint32  CSynclavierMutableArray::Count()
{
    return (SyncUint32) (array ? CFArrayGetCount(array) : 0);
};

const void* CSynclavierMutableArray::ItemAt(SyncUint32 index) const
{
    return (array ? CFArrayGetValueAtIndex(array, (CFIndex) index) : NULL);
};

void        CSynclavierMutableArray::SetAt(SyncUint32 index, const void *item)
{
    if (array) CFArraySetValueAtIndex(array, (CFIndex) index, item);
}

void        CSynclavierMutableArray::RemoveAt(SyncUint32 index)
{
    if (array) CFArrayRemoveValueAtIndex(array, (CFIndex) index);
}

void        CSynclavierMutableArray::Remove  (const void *item)
{
    for (SyncUint32 i = 0; i < Count(); i++) {
        if (ItemAt(i) == item) {
            RemoveAt(i);
            return;
        }
    }
}

void        CSynclavierMutableArray::PushLast(const void* item)
{
    if (!array) Reserve(1000);
    
    CFArrayAppendValue(array, item);
};

const void* CSynclavierMutableArray::PopLast()
{
    if (!array) return NULL;
    
    CFIndex     lastIndex = CFArrayGetCount(array);
    const void* item      = NULL;
    
    if (lastIndex) {
        item = CFArrayGetValueAtIndex(array, lastIndex-1);
        CFArrayRemoveValueAtIndex(array, lastIndex-1);
    }
    
    return item;
};

bool        CSynclavierMutableArray::Contains(const void* item)
{
    if (!array) return false;
    
    CFRange range = CFRangeMake(0, CFArrayGetCount(array));
    
    return CFArrayContainsValue(array, range, item);
};

SyncUint32  CSynclavierMutableArray::IndexOf(const void *item)
{
    for (SyncUint32 i = 0; i < Count(); i++)
        if (ItemAt(i) == item)
            return i;
    
    return 0;
}

void        CSynclavierMutableArray::Empty()
{
    if (array) CFArrayRemoveAllValues(array);
}

void        CSynclavierMutableArray::Reserve(SyncUint32 startingCapacity)
{
    if (array == NULL)
        array = CFArrayCreateMutable(NULL, (CFIndex) startingCapacity, NULL);
}


// --------------------------------
// CSynclavierSortedMutableArray
// --------------------------------

// A sorted version of CSynclavierMutableArray
// These methods use a 1-based index

// The return value is one of the following:
// The index of a value that matched, if the target value matches one or more in the range.
// Greater than or equal to the end point of the range, if the value is greater than all the values in the range.
// The index of the value greater than the target value, if the value lies between two of (or less than all of)
// the values in the range.

//		< 0		block 1 is less than block 2
//		0		block 1 is equal to block 2
//		> 0		block 1 is greater than block 2

// Also - key is passed as either argument...

static  CFComparisonResult  CSynclavierSortedMutableArrayComparitorByKey(const void *val1, const void *val2, void *context)
{
    CSynclavierSortedMutableArray& us = * (CSynclavierSortedMutableArray*) context;
    
    if (val2 == us.mComparisonKey)
        return (CFComparisonResult) us.mComparator->CompareToKey(val1, 0, val2);
    
    else
        return (CFComparisonResult) (0 - (int) us.mComparator->CompareToKey(val2, 0, val1));
}

static  CFComparisonResult  CSynclavierSortedMutableArrayComparitor(const void *val1, const void *val2, void *context)
{
    CSynclavierSortedMutableArray& us = * (CSynclavierSortedMutableArray*) context;

    return (CFComparisonResult) us.mComparator->Compare(val1, val2, 0, 0);
}

CSynclavierSortedMutableArray::CSynclavierSortedMutableArray()
{
    mComparator     = NULL;
    mOwnsComparator = false;
    mIsSorted       = true;
}

// One-based index support
SyncUint32  CSynclavierSortedMutableArray::FetchInsertIndexOfKey(const void *key )
{
    if (Count() == 0)
        return 1;
    
    #if 0
        for (SyncUint32 index = 0; index < Count(); index++) {
            SInt32 compare = mComparator->CompareToKey(ItemAt(index), 0, key);
            
            if (compare < 0)
                continue;
            
            return index + 1;
        }
        
        return Count() + 1;
    #endif
    
    CFRange range = {0, (CFIndex) Count()};
    
    mComparisonKey = key;
    
    SyncUint32 index = (SyncUint32) CFArrayBSearchValues(array, range, key, CSynclavierSortedMutableArrayComparitorByKey, this);
    
    if (index >= Count()) // Implies not found
        return Count()+1;
    
    return index+1;
}

SyncUint32  CSynclavierSortedMutableArray::FetchIndexOfKey(const void *key)
{
    if (Count() == 0)
        return 0;
    
    #if 0
        for (SyncUint32 index = 0; index < Count(); index++) {
            SInt32 compare = mComparator->CompareToKey(ItemAt(index), 0, key);
            
            if (compare < 0)
                continue;
            
            if (compare == 0)
                return index + 1;
            
            return 0;
        }
        
        return 0;
    #endif
    
    CFRange range = {0, (CFIndex) Count()};
    
    mComparisonKey = key;
    
    SyncUint32 index = (SyncUint32) CFArrayBSearchValues(array, range, key, CSynclavierSortedMutableArrayComparitorByKey, this);
    
    if (index >= Count()) // Implies not found
        return 0;
        
    if (mComparator->IsEqualToKey(ItemAt(index), 0, key))
        return index+1;
    
    return 0;
}

SyncUint32  CSynclavierSortedMutableArray::FetchIndexOf(const void *item) {
    CFRange range = {0, (CFIndex) Count()};
    
    if (Count() == 0)
        return 0;
    
    SyncUint32 index = (SyncUint32) CFArrayBSearchValues(array, range, item, CSynclavierSortedMutableArrayComparitor, this);
    
    if (index >= Count()) // Implies not found
        return 0;
    
    if (mComparator->IsEqualTo(ItemAt(index), item, 0, 0))
        return index+1;
    
    return 0;
}

SyncUint32  CSynclavierSortedMutableArray::InsertItem(const void *item) {
    CFRange    range = {0, (CFIndex) Count()};
    SyncUint32 where = 0;
    
    if (Count() > 0) {
        where = (SyncUint32) CFArrayBSearchValues(array, range, item, CSynclavierSortedMutableArrayComparitor, this);
    
        if (where > Count())
            where = Count();
    }
    
    CFArrayInsertValueAtIndex(array, where, item);
    
    return where+1;
}

void    CSynclavierSortedMutableArray::SetComparator(CSynclavierComparator* inComparator)
{
    mComparator = inComparator;
}
