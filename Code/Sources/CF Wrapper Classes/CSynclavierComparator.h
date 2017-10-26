// ===========================================================================
//	CSynclavierComparator.h
// ===========================================================================
//
//	CSynclavierComparator
//		Compares two items of arbitrary length
//
//	CSynclavierLongComparator
//		Compares two items which are sizeof(long)

#ifndef _H_CSynclavierComparator
#define _H_CSynclavierComparator

#include "SynclavierTypes.h"

typedef const void* CompareKeyT;

// ===========================================================================
//	• CSynclavierComparator •
// ===========================================================================

class	CSynclavierComparator {
public:
						CSynclavierComparator();
						
	virtual				~CSynclavierComparator();

	virtual SyncSint32  Compare(
								const void*			inItemOne,
								const void* 		inItemTwo,
								SyncUint32          inSizeOne,
								SyncUint32          inSizeTwo) const;

	virtual SyncBool    IsEqualTo(
								const void*			inItemOne,
								const void* 		inItemTwo,
								SyncUint32          inSizeOne,
								SyncUint32          inSizeTwo) const;

	virtual	SyncSint32  CompareToKey(
								const void*			inItem,
								SyncUint32          inSize,
								const void*			inKey) const;

	virtual	SyncBool    IsEqualToKey(
								const void*			inItem,
								SyncUint32          inSize,
								const void*			inKey) const;

	virtual CSynclavierComparator*	Clone();

	static CSynclavierComparator*	GetComparator();

protected:
	static	CSynclavierComparator*	sComparator;
};


// ===========================================================================
//	• CSynclavierLongComparator •
// ===========================================================================

class	CSynclavierLongComparator : public CSynclavierComparator {
public:
						CSynclavierLongComparator();
						
	virtual				~CSynclavierLongComparator();

	virtual SyncSint32  Compare(
								const void*			inItemOne,
								const void* 		inItemTwo,
								SyncUint32          inSizeOne,
								SyncUint32          inSizeTwo) const;

	virtual SyncBool    IsEqualTo(
								const void*			inItemOne,
								const void* 		inItemTwo,
								SyncUint32          inSizeOne,
								SyncUint32          inSizeTwo) const;

	virtual CSynclavierComparator*	Clone();

	static CSynclavierLongComparator*		GetComparator();

protected:
	static CSynclavierLongComparator*	sLongComparator;
};

#endif
