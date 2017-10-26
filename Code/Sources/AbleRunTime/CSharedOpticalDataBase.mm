// =================================================================================
//	CSharedOpticalDataBase.cp
// =================================================================================

// Manages a common shared data base of Optical Disk index files

#include "CSharedOpticalDataBaseMM.h"
#include "InterChange.h"
#include "CHFSImageFile.h"
#include "SynclavierFileReference.h"

// ---------------------------------------------------------------------------
//	¥ Global static class variables
// ---------------------------------------------------------------------------
CSynclavierMutableArray 	CSharedOpticalDataBase::SharedOpticalDataList(0);


// ---------------------------------------------------------------------------
//	¥ Construction
// ---------------------------------------------------------------------------

CSharedOpticalDataBase::CSharedOpticalDataBase( const InterChangeOpticalData& inOpticalData, const interchange_settings& inSettings ) :
	mNameComparator(this),	mTypeComparator(this), mSizeComparator(this), mCategoryComparator(this), mSettings(inSettings)
{
	mOpticalData = inOpticalData;
	mUserCount   = 0;
	mReaders     = 0;
	mIsUpToDate  = false;
	mLegacyCode  = 0;
	
	memset(&mFileSpec, 0, sizeof(mFileSpec));
	
	mSortedFileList.SetComparator    (&mNameComparator    );
	mSortedCategoryList.SetComparator(&mCategoryComparator);

    SharedOpticalDataList.Reserve(100);
	SharedOpticalDataList.PushLast(this);
}

CSharedOpticalDataBase::~CSharedOpticalDataBase()
{
	SharedOpticalDataList.Remove(this);
}


// ---------------------------------------------------------------------------
//	¥ AccessOpticalDataByHeader
// ---------------------------------------------------------------------------

// Access a shared optical data base.  Create the necessary object if
// not available.

CSharedOpticalDataBase*
CSharedOpticalDataBase::AccessOpticalDataByHeader( const InterChangeOpticalData& inOpticalData, InterChangeItemUnion& inUnion,
                                                   const interchange_settings& itsSettings )
{
	CSynclavierMutableArrayIterator         iterator(SharedOpticalDataList);
	CSharedOpticalDataBase*                 theDataBase = NULL;
	SyncFSSpec								aSpec;

	// Make sure device code is up to date
	if (inUnion.OpticalItem.file_hfs_refnum)
		inUnion.OpticalItem.file_device_code = CHFSImageFile::GetDevCode(inUnion.OpticalItem.file_hfs_refnum);

	// See if we have a data base for this device code
	theDataBase = FindOpticalDataByLegacyCode(inUnion.OpticalItem.file_device_code);
	
	// See if we have a data base for this FSSpec
	if (!theDataBase)
	{
		AbleDiskLib_FetchReleventFSSpec(inUnion, aSpec, itsSettings);
	
		if (aSpec.file_name[0])
			theDataBase = FindOpticalDataByFSSPec(aSpec);
	}
	
	// See if we have a data base by Volume Name
	if (!theDataBase)
	{
		CSharedOpticalDataBase* aDataBase = NULL;
		while (iterator.Next(aDataBase))
		{
			if (aDataBase->UsesSpecifier(inOpticalData))
			{
				theDataBase = aDataBase;
				theDataBase->mUserCount++;
				break;
			}
		}
	}
	
	// Not there: try to create one
	if (!theDataBase)
	{
		try {								// Don't throw out of a constructor
			theDataBase = new CSharedOpticalDataBase(inOpticalData, itsSettings);
			
			if (theDataBase)
				theDataBase->mUserCount++;
		}
		
		catch (...) {
			if (theDataBase)
				delete theDataBase;

			theDataBase = NULL;
		}
	}
	
	// Link to legacy code or HFS to find data base by code or by file spec
	if (theDataBase)
	{
		if ((inUnion.OpticalItem.file_device_code <  ABLE_HFS_READDATA_CODE)
		&&  (inUnion.OpticalItem.file_device_code != 0                     ))
		{
			ReleaseLegacyCode( inUnion.OpticalItem.file_device_code );
			theDataBase->mLegacyCode = inUnion.OpticalItem.file_device_code;
		}
		
		AbleDiskLib_FetchReleventFSSpec(inUnion, theDataBase->mFileSpec, itsSettings);
		
		// See if up to date.  E.G. Detect new media inserted or changes made byRTP
		if (!theDataBase->UsesSpecifier(inOpticalData))
		{
			theDataBase->mOpticalData = inOpticalData;
			theDataBase->mIsUpToDate  = false;
		}

		else if (!theDataBase->IsUpToDate(inOpticalData))
		{
			theDataBase->mOpticalData = inOpticalData;
			theDataBase->mIsUpToDate  = false;
		}
	}
	
	return (theDataBase);
}


// ---------------------------------------------------------------------------
//	¥ ReleaseLegacyCode
// ---------------------------------------------------------------------------

void
CSharedOpticalDataBase::ReleaseLegacyCode( long legacyCode )
{
	CSynclavierMutableArrayIterator         iterator(SharedOpticalDataList);
	CSharedOpticalDataBase*                 theDataBase = NULL;

	if (legacyCode == 0 || legacyCode >= ABLE_HFS_READDATA_CODE)
		return;

	while (iterator.Next(theDataBase))
	{
		if (theDataBase->mLegacyCode == legacyCode)
			theDataBase->mLegacyCode = 0;
	}
}


// ---------------------------------------------------------------------------
//	¥ FindOpticalDataByLegacyCode
// ---------------------------------------------------------------------------

CSharedOpticalDataBase*
CSharedOpticalDataBase::FindOpticalDataByLegacyCode( long legacyCode )
{
	CSynclavierMutableArrayIterator         iterator(SharedOpticalDataList);
	CSharedOpticalDataBase*                 theDataBase = NULL;

	if (legacyCode == 0 || legacyCode >= ABLE_HFS_READDATA_CODE)
		return NULL;

	while (iterator.Next(theDataBase))
	{
		if (legacyCode && theDataBase->mLegacyCode == legacyCode)
			return (theDataBase);
	}
	
	return NULL;
}


// ---------------------------------------------------------------------------
//	¥ FindOpticalDataByFSSPec
// ---------------------------------------------------------------------------

CSharedOpticalDataBase*
CSharedOpticalDataBase::FindOpticalDataByFSSPec( SyncFSSpec& fileSpec   )
{
	CSynclavierMutableArrayIterator         iterator(SharedOpticalDataList);
	CSharedOpticalDataBase*                 theDataBase = NULL;

	while (iterator.Next(theDataBase))
	{
		if (InterChangeLibEqualFSSpecs(&fileSpec, &(theDataBase->mFileSpec)))
			return (theDataBase);
	}
	
	return NULL;
}


// ---------------------------------------------------------------------------
//	¥ CloseSharedFile
// ---------------------------------------------------------------------------

void CSharedOpticalDataBase::ReleaseOpticalData( )
{
	if (mUserCount)
		mUserCount--;
		
	if (mUserCount)
		return;
	
	// Could perform garbage collection in some way...  But for now, leave the data base around...
}


// ---------------------------------------------------------------------------
//	¥ UsesSpecifier
// ---------------------------------------------------------------------------

Boolean CSharedOpticalDataBase::UsesSpecifier( const InterChangeOpticalData& inOpticalData )
{
	int i = 0;
	
	if (!mOpticalData.volume_name[0])									// No volume name - not legit name
		return (false);
		
	while (mOpticalData.volume_name[i] && mOpticalData.volume_name[i] == inOpticalData.volume_name[i])
		i++;
		
	// Equal name implies all characters match and both end with a null (duhhh....)
	if (mOpticalData.volume_name[i] || inOpticalData.volume_name[i])	// Name doesn't match
		return (false);
	
	if ((mOpticalData.volume_time != inOpticalData.volume_time)
	||  (mOpticalData.dir_start   != inOpticalData.dir_start  )
	||  (mOpticalData.dir_len     != inOpticalData.dir_len    )
	||  (mOpticalData.data_start  != inOpticalData.data_start )
	||  (mOpticalData.data_len    != inOpticalData.data_len   ))
		return (false);

	return (true);
}


// ---------------------------------------------------------------------------
//	¥ IsUpToDate
// ---------------------------------------------------------------------------

Boolean CSharedOpticalDataBase::IsUpToDate( const InterChangeOpticalData& inOpticalData )
{
	int i = 0;
	
	if (!mOpticalData.volume_name[0])									// No volume name - not legit name
		return (false);
		
	while (mOpticalData.volume_name[i] && mOpticalData.volume_name[i] == inOpticalData.volume_name[i])
		i++;

	// Equal name implies all characters match and both end with a null (duhhh....)
	if (mOpticalData.volume_name[i] || inOpticalData.volume_name[i])	// Name doesn't match
		return (false);
		
	if ((mOpticalData.volume_time != inOpticalData.volume_time)
	||  (mOpticalData.dir_start   != inOpticalData.dir_start  )
	||  (mOpticalData.dir_len     != inOpticalData.dir_len    )
	||  (mOpticalData.data_start  != inOpticalData.data_start )
	||  (mOpticalData.data_len    != inOpticalData.data_len   )
	||  (mOpticalData.dir_used    != inOpticalData.dir_used   )
	||  (mOpticalData.dir_free    != inOpticalData.dir_free   )
	||  (mOpticalData.dir_next    != inOpticalData.dir_next   )
	||  (mOpticalData.data_used   != inOpticalData.data_used  )
	||  (mOpticalData.data_free   != inOpticalData.data_free  )
	||  (mOpticalData.data_next   != inOpticalData.data_next  ))
		return (false);

	return (true);
}


// ---------------------------------------------------------------------------
//	¥ Comparators - COpticalFileDataNameComparator
// ---------------------------------------------------------------------------

SInt32	COpticalFileDataNameComparator::DoCompare   ( const SOpticalFileData& itemOne, const SOpticalFileData& itemTwo ) const
{
	const char* itemOneName = &itemOne.Name[0];
	const char* itemTwoName = &itemTwo.Name[0];
	
	while (*itemOneName && *itemOneName == *itemTwoName)
	{
		itemOneName++;
		itemTwoName++;
	}
	
	if (*itemOneName == *itemTwoName)									// implies we reached end of strings
		return (0);
	
	if (*itemOneName < *itemTwoName)
		return (-1);
	
	return (+1);
}

SInt32	COpticalFileDataNameComparator::CompareToKey( const void* inItemOne, UInt32 inSizeOne, const void* inKey ) const
{
	#pragma unused (inSizeOne)

	int	  indexOne = (int) *(unsigned short*) inItemOne;

    const SOpticalFileData& itemOne = * (SOpticalFileData*) mDataBase->mFileList[indexOne];
	const SOpticalFileData& itemTwo = * (SOpticalFileData*) inKey;
	
	return (DoCompare(itemOne, itemTwo));
}

// *inItemOne and *inItemTwo are (unsigned short) indexes into mFileList
SInt32	COpticalFileDataNameComparator::Compare( const void* inItemOne, const void* inItemTwo, UInt32 inSizeOne, UInt32 inSizeTwo ) const
{
	#pragma unused (inSizeOne)
	#pragma unused (inSizeTwo)

	int	  indexOne = (int) *(unsigned short*) inItemOne;
	int	  indexTwo = (int) *(unsigned short*) inItemTwo;
	
	const SOpticalFileData& itemOne = * (SOpticalFileData*) mDataBase->mFileList[indexOne];
	const SOpticalFileData& itemTwo = * (SOpticalFileData*) mDataBase->mFileList[indexTwo];
	
	return (DoCompare(itemOne, itemTwo));
}


// ---------------------------------------------------------------------------
//	¥ Comparators - COpticalFileDataTypeComparator
// ---------------------------------------------------------------------------

SInt32	COpticalFileDataTypeComparator::DoCompare   ( const SOpticalFileData& itemOne, const SOpticalFileData& itemTwo ) const
{
	int	itemOneKey = AbleDiskLib_SortKey[itemOne.FileType];
	int	itemTwoKey = AbleDiskLib_SortKey[itemTwo.FileType];

	if (itemOneKey == itemTwoKey)											// Same file type: sort by name
		return (mDataBase->mNameComparator.DoCompare(itemOne, itemTwo));
	
	if (itemOneKey < itemTwoKey)
		return (-1);
	
	return (+1);
}

SInt32	COpticalFileDataTypeComparator::CompareToKey( const void* inItemOne, UInt32 inSizeOne, const void* inKey ) const
{
	#pragma unused (inSizeOne)

	int	  indexOne = (int) *(unsigned short*) inItemOne;

	const SOpticalFileData& itemOne = * (SOpticalFileData*) mDataBase->mFileList[indexOne];
	const SOpticalFileData& itemTwo = * (SOpticalFileData*) inKey;
	
	return (DoCompare(itemOne, itemTwo));
}

// *inItemOne and *inItemTwo are (unsigned short) indexes into mFileList
SInt32	COpticalFileDataTypeComparator::Compare( const void* inItemOne, const void* inItemTwo, UInt32 inSizeOne, UInt32 inSizeTwo ) const
{
	#pragma unused (inSizeOne)
	#pragma unused (inSizeTwo)
	
	int	  indexOne = (int) *(unsigned short*) inItemOne;
	int	  indexTwo = (int) *(unsigned short*) inItemTwo;
	
	const SOpticalFileData& itemOne = * (SOpticalFileData*) mDataBase->mFileList[indexOne];
	const SOpticalFileData& itemTwo = * (SOpticalFileData*) mDataBase->mFileList[indexTwo];

	return (DoCompare(itemOne, itemTwo));
}


// ---------------------------------------------------------------------------
//	¥ Comparators - COpticalFileDataSizeComparator
// ---------------------------------------------------------------------------

SInt32	COpticalFileDataSizeComparator::DoCompare   ( const SOpticalFileData& itemOne, const SOpticalFileData& itemTwo ) const
{
	long long	itemOneSize = itemOne.ByteLength;
	long long	itemTwoSize = itemTwo.ByteLength;

	if (itemOneSize == itemTwoSize)											// Same length: sort by name
		return (mDataBase->mNameComparator.DoCompare(itemOne, itemTwo));
	
	if (itemOneSize < itemTwoSize)
		return (-1);
	
	return (+1);
}

SInt32	COpticalFileDataSizeComparator::CompareToKey( const void* inItemOne, UInt32 inSizeOne, const void* inKey ) const
{
	#pragma unused (inSizeOne)

	int	  indexOne = (int) *(unsigned short*) inItemOne;

	const SOpticalFileData& itemOne = * (SOpticalFileData*) mDataBase->mFileList[indexOne];
	const SOpticalFileData& itemTwo = * (SOpticalFileData*) inKey;
	
	return (DoCompare(itemOne, itemTwo));
}

// *inItemOne and *inItemTwo are (unsigned short) indexes into mFileList
SInt32	COpticalFileDataSizeComparator::Compare( const void* inItemOne, const void* inItemTwo, UInt32 inSizeOne, UInt32 inSizeTwo ) const
{
	#pragma unused (inSizeOne)
	#pragma unused (inSizeTwo)
	
	int	  indexOne = (int) *(unsigned short*) inItemOne;
	int	  indexTwo = (int) *(unsigned short*) inItemTwo;
	
	const SOpticalFileData& itemOne = * (SOpticalFileData*) mDataBase->mFileList[indexOne];
	const SOpticalFileData& itemTwo = * (SOpticalFileData*) mDataBase->mFileList[indexTwo];

	return (DoCompare(itemOne, itemTwo));
}


// ---------------------------------------------------------------------------
//	¥ Comparators - COpticalCategoryComparator
// ---------------------------------------------------------------------------

SInt32	COpticalCategoryComparator::DoCompare   ( const SOpticalCategory& itemOne, const SOpticalCategory& itemTwo ) const
{
	const char* itemOneName = &itemOne.Name[0];
	const char* itemTwoName = &itemTwo.Name[0];
	
	while (*itemOneName && *itemOneName == *itemTwoName)
	{
		itemOneName++;
		itemTwoName++;
	}
	
	if (*itemOneName == *itemTwoName)									// implies we reached end of strings
		return (0);
	
	if (*itemOneName < *itemTwoName)
		return (-1);
	
	return (+1);
}

SInt32	COpticalCategoryComparator::CompareToKey( const void* inItemOne, UInt32 inSizeOne, const void* inKey ) const
{
	#pragma unused (inSizeOne)

	int	  indexOne = (int) *(unsigned short*) inItemOne;

	const SOpticalCategory& itemOne = * (SOpticalCategory*) mDataBase->mCategoryList[indexOne];
	const SOpticalCategory& itemTwo = * (SOpticalCategory*) inKey;
	
	return (DoCompare(itemOne, itemTwo));
}

// *inItemOne and *inItemTwo are (unsigned short) indexes into mCategory
SInt32	COpticalCategoryComparator::Compare( const void* inItemOne, const void* inItemTwo, UInt32 inSizeOne, UInt32 inSizeTwo ) const
{
	#pragma unused (inSizeOne)
	#pragma unused (inSizeTwo)
	
	int	  indexOne = (int) *(unsigned short*) inItemOne;
	int	  indexTwo = (int) *(unsigned short*) inItemTwo;
	
	const SOpticalCategory& itemOne = * (SOpticalCategory*) mDataBase->mCategoryList[indexOne];
	const SOpticalCategory& itemTwo = * (SOpticalCategory*) mDataBase->mCategoryList[indexTwo];

	return (DoCompare(itemOne, itemTwo));
}
