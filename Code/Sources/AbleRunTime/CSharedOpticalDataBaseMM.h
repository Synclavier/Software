// =================================================================================
//	CSharedOpticalDataBase.h
// =================================================================================

// Manages a common shared data base of Optical Disk index files

#pragma once

#include "XPLRuntime.h"
#include "CSynclavierComparator.h"
#include "CSynclavierMutableArray.h"
#include "AbleDiskLib.h"
#include "SynclavierFileReference.h"
#include "InterChange.h"
#include "optlits.h"

// ------------------------------------
// ¥ SOpticalFileData, SOpticalCategory
// ------------------------------------

typedef	struct	SOpticalFileData
{
	char			Name[9];                // File name
	unsigned char	FileType;               // File type
	unsigned short	DirEntry;               // Directry entry number on optical media (0-based)
	unsigned short  SampleRate;             // File sample rate, khz*10
	unsigned short  Flags;                  // Sound file flags
	unsigned long	SecStart;               // Sector start  of sound file on media (probably 512-byte sectors)
	long long		ByteLength;             // Byte length of sound file on media
	unsigned short	Categories[8];          // Indexes to up to 8 categories
	char			Caption[64];            // Partial caption
    unsigned short  SFHeader[e_header_max]; // Snarf of updated sound file header that was stored in the optical disk entry
    
} SOpticalFileData;

typedef	struct	SOpticalCategory
{
	char			Name[42];			// Category name (max 40 significant characters)
	//unsigned short  first_file;		// 1-based index to mFileList of first alphabetical file which may point to this category
	//unsigned short  last_file;		// 1-based index to mFileList of last  alphabetical file which may point to this category

} SOpticalCategory;


// ---------------------------------------------------------------------------
//	¥ Comparators - COpticalFileDataNameComparator
// ---------------------------------------------------------------------------

class	COpticalFileDataNameComparator : public CSynclavierComparator {
	public:			COpticalFileDataNameComparator ( const class CSharedOpticalDataBase* itsDataBase ) {mDataBase = itsDataBase;}
	virtual SInt32	Compare     ( const void* inItemOne, const void* inItemTwo, UInt32 inSizeOne, UInt32 inSizeTwo ) const;
	virtual SInt32	CompareToKey( const void* inItemOne, UInt32 inSizeOne,      const void* inKey                  ) const;
	virtual SInt32  DoCompare   ( const SOpticalFileData& itemOne, const SOpticalFileData& ItemTwo) const;
					
					const class CSharedOpticalDataBase*	mDataBase;
};

// ---------------------------------------------------------------------------
//	¥ Comparators - COpticalFileDataTypeComparator
// ---------------------------------------------------------------------------

class	COpticalFileDataTypeComparator : public CSynclavierComparator {
	public:			COpticalFileDataTypeComparator ( const class CSharedOpticalDataBase* itsDataBase ) {mDataBase = itsDataBase;}
	virtual SInt32	Compare		( const void* inItemOne, const void* inItemTwo, UInt32 inSizeOne, UInt32 inSizeTwo ) const;
	virtual SInt32	CompareToKey( const void* inItemOne, UInt32 inSizeOne,      const void* inKey                  ) const;
	virtual SInt32  DoCompare   ( const SOpticalFileData& itemOne, const SOpticalFileData& ItemTwo) const;
					
					const class CSharedOpticalDataBase*	mDataBase;
};


// ---------------------------------------------------------------------------
//	¥ Comparators - COpticalFileDataSizeComparator
// ---------------------------------------------------------------------------

class	COpticalFileDataSizeComparator : public CSynclavierComparator {
	public:			COpticalFileDataSizeComparator ( const class CSharedOpticalDataBase* itsDataBase ) {mDataBase = itsDataBase;}
	virtual SInt32	Compare		( const void* inItemOne, const void* inItemTwo, UInt32 inSizeOne, UInt32 inSizeTwo ) const;
	virtual SInt32	CompareToKey( const void* inItemOne, UInt32 inSizeOne,      const void* inKey                  ) const;
	virtual SInt32  DoCompare   ( const SOpticalFileData& itemOne, const SOpticalFileData& ItemTwo) const;
					
					const class CSharedOpticalDataBase*	mDataBase;
};


// ---------------------------------------------------------------------------
//	¥ Comparators - COpticalCategoryComparator
// ---------------------------------------------------------------------------

class	COpticalCategoryComparator : public CSynclavierComparator {
	public:			COpticalCategoryComparator ( const class CSharedOpticalDataBase* itsDataBase ) {mDataBase = itsDataBase;}
	virtual SInt32	Compare		( const void* inItemOne, const void* inItemTwo, UInt32 inSizeOne, UInt32 inSizeTwo ) const;
	virtual SInt32	CompareToKey( const void* inItemOne, UInt32 inSizeOne,      const void* inKey                  ) const;
	virtual SInt32  DoCompare   ( const SOpticalCategory& itemOne, const SOpticalCategory& ItemTwo) const;
					
					const class CSharedOpticalDataBase*	mDataBase;
};


// ------------------------
// CSharedOpticalDataBase
// ------------------------

class	CSharedOpticalDataBase {

public:
						CSharedOpticalDataBase (  const InterChangeOpticalData& inOpticalData, const struct interchange_settings& itsSettings );
						~CSharedOpticalDataBase();

	static	CSharedOpticalDataBase*		AccessOpticalDataByHeader   ( const         InterChangeOpticalData& inOpticalData, InterChangeItemUnion& inUnion,
																      const         struct interchange_settings& itsSettings );
	static	CSharedOpticalDataBase*		FindOpticalDataByLegacyCode ( long          deviceCode );
	static	CSharedOpticalDataBase*		FindOpticalDataByFSSPec     ( SyncFSSpec&   fileSpec   );
	static	void						ReleaseLegacyCode  		    ( long          deviceCode );
	
			void						ReleaseOpticalData ();

			Boolean						UsesSpecifier   ( const InterChangeOpticalData& inOpticalData );
			Boolean						IsUpToDate      ( const InterChangeOpticalData& inOpticalData );
			long						GetUsers		() {return mUserCount;}
			void						AddUser			() {mUserCount++;}
			void						RemoveUser		() {if (mUserCount) mUserCount--;}

			long						GetIsUpToDate	( ) {return (mIsUpToDate);}
			void						SetIsUpToDate	( long isUpToDate) {mIsUpToDate = isUpToDate;}

	static  CSynclavierMutableArray		SharedOpticalDataList;              // List of all CSharedOpticalDataBase objects created
	
	LFastMutexSemaphore				mSemaphore;								// Fast mutual exclusion sempahore

	InterChangeOpticalData			mOpticalData;							// Holds header information scanned from media
	long							mUserCount;								// Count of users with pointers to this object
	long							mReaders;								// Count of users reading the data base (when !=0, don't change it!)
	long							mIsUpToDate;							// True if is up to date
	long							mLegacyCode;							// Legacy device code (e.g. 10, 11) for this platter
	SyncFSSpec						mFileSpec;								// File spec for image file containing this media
	const interchange_settings& 	mSettings;
	
	CSynclavierMutable1BasedArray   mFileList;								// Unsorted - matches entries on media                (SOpticalFileData)
	CSynclavierMutable1BasedArray 	mCategoryList;							// Unsorted - order derived during processing entries (SOpticalCategory)
	CSynclavierSortedMutableArray   mSortedFileList;						// Sorted (by name) (unsigned short)
	CSynclavierSortedMutableArray   mSortedCategoryList;					// Softed (by name) (unsigned short)
					
	COpticalFileDataNameComparator 	mNameComparator;
	COpticalFileDataTypeComparator 	mTypeComparator;
	COpticalFileDataSizeComparator	mSizeComparator;
	COpticalCategoryComparator 		mCategoryComparator;
};
