// ===========================================================================
//	CSynclavierComparator.cp
// ===========================================================================
//
//	Comparators are objects that know how to compare to other objects
//	or structures.  Subclasses will need to implement the Compare()
//	method. The compare Compare() result should be one of the following:
//
//		< 0		object/data item 1 is less than object/data item 2
//		0		object/data item 1 is equal to object/data item 2
//		> 0		object/data item 1 is greater than object/data item 2

#include "CSynclavierComparator.h"

// ---------------------------------------------------------------------------
//	• BlockCompare(const void*, const void*, SyncUint32, SyncUint32)
// ---------------------------------------------------------------------------
//	Compare two blocks of memory. Blocks may have different lengths.
//
//	Return values:
//
//		< 0		block 1 is less than block 2
//		0		block 1 is equal to block 2
//		> 0		block 1 is greater than block 2
//
// Note that this always returns 0 (equal) if n <= 0.

static SyncSint32
BlockCompare(
             const void	*inPtr1,
             const void	*inPtr2,
             SyncUint32 inLen1,
             SyncUint32 inLen2)
{
    const unsigned char	*ucp1 = (const unsigned char *) inPtr1;
    const unsigned char	*ucp2 = (const unsigned char *) inPtr2;
    
    SyncUint32	len = inLen1;           // len is the shorter block length
    if (inLen2 < inLen1) {
        len = inLen2;
    }
    
    while (len > 0) {					// Compare byte by byte
        if (*ucp1 != *ucp2) {
            return ((SyncSint32)(*ucp1 - *ucp2));
        }
        
        ucp1++;
        ucp2++;
        len--;
    }
    // All bytes the same so far
    return (SyncSint32)(inLen1 - inLen2);	// Longer block is "bigger"
}


// ===========================================================================
//	• CSynclavierComparator
// ===========================================================================
//	Compares items byte by byte

CSynclavierComparator*	CSynclavierComparator::sComparator = nullptr;		// static class variable

CSynclavierComparator::CSynclavierComparator()
{
}


CSynclavierComparator::~CSynclavierComparator()
{
	if (sComparator == this) {
		sComparator = nullptr;
	}
}


SyncSint32
CSynclavierComparator::Compare(
	const void*		inItemOne,
	const void*		inItemTwo,
	SyncUint32      inSizeOne,
	SyncUint32      inSizeTwo) const
{
	return BlockCompare(inItemOne, inItemTwo, inSizeOne, inSizeTwo);
}


SyncBool
CSynclavierComparator::IsEqualTo(
	const void*		inItemOne,
	const void*		inItemTwo,
	SyncUint32      inSizeOne,
	SyncUint32      inSizeTwo) const
{
	return (Compare(inItemOne, inItemTwo, inSizeOne, inSizeTwo) == 0);
}


SyncSint32
CSynclavierComparator::CompareToKey(
	const void*		/* inItem */,
	SyncUint32      /* inSize */,
	const void*		/* inKey */) const
{
	return 1;
}


SyncBool
CSynclavierComparator::IsEqualToKey(
	const void*		inItem,
	SyncUint32      inSize,
	const void*		inKey) const
{
	return (CompareToKey(inItem, inSize, inKey) == 0);
}


CSynclavierComparator*
CSynclavierComparator::GetComparator()
{
	if (sComparator == nullptr) {
		sComparator = new CSynclavierComparator;
	}

	return sComparator;
}


CSynclavierComparator*
CSynclavierComparator::Clone()
{
	return new CSynclavierComparator;
}


// ===========================================================================
//	• CSynclavierLongComparator
// ===========================================================================
//	Compares items as long integer values

CSynclavierLongComparator*	CSynclavierLongComparator::sLongComparator = nullptr;

CSynclavierLongComparator::CSynclavierLongComparator()
{
}


CSynclavierLongComparator::~CSynclavierLongComparator()
{
	if (sLongComparator == this) {
		sLongComparator = nullptr;
	}
}


SyncSint32
CSynclavierLongComparator::Compare(
	const void*		inItemOne,
	const void*		inItemTwo,
	SyncUint32      /* inSizeOne */,
	SyncUint32      /* inSizeTwo */) const
{
    if ((*(long*) inItemOne) > (*(long*) inItemTwo))
        return 1;
    
    if ((*(long*) inItemOne) == (*(long*) inItemTwo))
        return 0;
    
    return -1;

    //return ( (*(long*) inItemOne) - (*(long*) inItemTwo) );
}


SyncBool
CSynclavierLongComparator::IsEqualTo(
	const void*		inItemOne,
	const void*		inItemTwo,
	SyncUint32      /* inSizeOne */,
	SyncUint32      /* inSizeTwo */) const
{
	return ( (*(long*) inItemOne) == (*(long*) inItemTwo) );
}


CSynclavierLongComparator*
CSynclavierLongComparator::GetComparator()
{
	if (sLongComparator == nullptr) {
		sLongComparator = new CSynclavierLongComparator;
	}

	return sLongComparator;
}


CSynclavierComparator*
CSynclavierLongComparator::Clone()
{
	return new CSynclavierLongComparator;
}
