// ===========================================================================
//	LComparator.cp				PowerPlant 2.2.2	©1995-2005 Metrowerks Inc.
// ===========================================================================
//
//	Comparators are objects that know how to compare to other objects
//	or structures.  Subclasses will need to implement the Compare()
//	method. The compare Compare() result should be one of the following:
//
//		< 0		object/data item 1 is less than object/data item 2
//		0		object/data item 1 is equal to object/data item 2
//		> 0		object/data item 1 is greater than object/data item 2

#include "LComparator.h"


// ---------------------------------------------------------------------------
//	¥ BlocksAreEqual
// ---------------------------------------------------------------------------
//	Blocks are equal if the first n bytes pointed to by s1 have the same
//	values as the first n bytes pointed to by s2. Note that this always
//	returns true if n is zero.

Boolean
BlocksAreEqual(
               const void	*s1,
               const void	*s2,
               UInt32		n)
{
    const unsigned char	*ucp1 = (const unsigned char *) s1;
    const unsigned char	*ucp2 = (const unsigned char *) s2;
    
    while (n > 0) {
        if (*ucp1++ != *ucp2++) {
            return false;
        }
        n--;
    }
    
    return true;
}


// ---------------------------------------------------------------------------
//	¥ BlockCompare(const void*, const void*, UInt32)
// ---------------------------------------------------------------------------
//	Compare two equal-length blocks of memory
//
//	Return values:
//
//		< 0		block 1 is less than block 2
//		0		block 1 is equal to block 2
//		> 0		block 1 is greater than block 2
//
// Note that this always returns 0 (equal) if n <= 0.

SInt32
BlockCompare(
             const void	*s1,
             const void	*s2,
             UInt32		n)
{
    const unsigned char	*ucp1 = (const unsigned char *) s1;
    const unsigned char	*ucp2 = (const unsigned char *) s2;
    
    while (n > 0) {
        if (*ucp1 != *ucp2) {
            return ((SInt32)(*ucp1 - *ucp2));
        }
        
        ucp1++;
        ucp2++;
        n--;
    }
    
    return 0;
}


// ---------------------------------------------------------------------------
//	¥ BlockCompare(const void*, const void*, UInt32, Unit32)
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

SInt32
BlockCompare(
             const void	*inPtr1,
             const void	*inPtr2,
             UInt32		inLen1,
             UInt32		inLen2)
{
    const unsigned char	*ucp1 = (const unsigned char *) inPtr1;
    const unsigned char	*ucp2 = (const unsigned char *) inPtr2;
    
    UInt32	len = inLen1;				// len is the shorter block length
    if (inLen2 < inLen1) {
        len = inLen2;
    }
    
    while (len > 0) {					// Compare byte by byte
        if (*ucp1 != *ucp2) {
            return ((SInt32)(*ucp1 - *ucp2));
        }
        
        ucp1++;
        ucp2++;
        len--;
    }
    // All bytes the same so far
    return (SInt32)(inLen1 - inLen2);	// Longer block is "bigger"
}


// ===========================================================================
//	¥ LComparator
// ===========================================================================
//	Compares items byte by byte

LComparator*	LComparator::sComparator = nil;		// static class variable

LComparator::LComparator()
{
}


LComparator::~LComparator()
{
	if (sComparator == this) {
		sComparator = nil;
	}
}


SInt32
LComparator::Compare(
	const void*		inItemOne,
	const void*		inItemTwo,
	UInt32			inSizeOne,
	UInt32			inSizeTwo) const
{
	return BlockCompare(inItemOne, inItemTwo, inSizeOne, inSizeTwo);
}


Boolean
LComparator::IsEqualTo(
	const void*		inItemOne,
	const void*		inItemTwo,
	UInt32			inSizeOne,
	UInt32			inSizeTwo) const
{
	return (Compare(inItemOne, inItemTwo, inSizeOne, inSizeTwo) == 0);
}


SInt32
LComparator::CompareToKey(
	const void*		/* inItem */,
	UInt32			/* inSize */,
	const void*		/* inKey */) const
{
	return 1;
}


Boolean
LComparator::IsEqualToKey(
	const void*		inItem,
	UInt32			inSize,
	const void*		inKey) const
{
	return (CompareToKey(inItem, inSize, inKey) == 0);
}


LComparator*
LComparator::GetComparator()
{
	if (sComparator == nil) {
		sComparator = new LComparator;
	}

	return sComparator;
}


LComparator*
LComparator::Clone()
{
	return new LComparator;
}

#pragma mark -

// ===========================================================================
//	¥ LLongComparator
// ===========================================================================
//	Compares items as long integer values

LLongComparator*	LLongComparator::sLongComparator = nil;

LLongComparator::LLongComparator()
{
}


LLongComparator::~LLongComparator()
{
	if (sLongComparator == this) {
		sLongComparator = nil;
	}
}


SInt32
LLongComparator::Compare(
	const void*		inItemOne,
	const void*		inItemTwo,
	UInt32			/* inSizeOne */,
	UInt32			/* inSizeTwo */) const
{
    if ((*(long*) inItemOne) > (*(long*) inItemTwo))
        return 1;
    
    if ((*(long*) inItemOne) == (*(long*) inItemTwo))
        return 0;
    
    return -1;

    //return ( (*(long*) inItemOne) - (*(long*) inItemTwo) );
}


Boolean
LLongComparator::IsEqualTo(
	const void*		inItemOne,
	const void*		inItemTwo,
	UInt32			/* inSizeOne */,
	UInt32			/* inSizeTwo */) const
{
	return ( (*(long*) inItemOne) == (*(long*) inItemTwo) );
}


LLongComparator*
LLongComparator::GetComparator()
{
	if (sLongComparator == nil) {
		sLongComparator = new LLongComparator;
	}

	return sLongComparator;
}


LComparator*
LLongComparator::Clone()
{
	return new LLongComparator;
}
