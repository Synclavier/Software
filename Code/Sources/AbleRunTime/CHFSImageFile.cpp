// =================================================================================
//	CHFSImageFile.cp
// =================================================================================

// Manages access to image files stored on the mac.

// Std C
#include <String.h>

// PowerPlant Addons
#include "CHFSImageFile.h"

#if !__LP64__
    #include "LSharedFileStream.h"
    #include "CInterChangeApp.h"
    #include "LFastMutexSemaphore.h"
#endif

// Local
#include "XPL.h"
#include "XPLRuntime.h"


// ---------------------------------------------------------------------------
//	¥ Global static class variables
// ---------------------------------------------------------------------------
CSynclavierMutableArray 	    CHFSImageFile::HFSImageFileList(SHFSIMAGE_HASH_SIZE);
int                             CHFSImageFile::HFSImageFileListFree;
SyncFSSpec                      CHFSImageFile::recent_1_spec;
class LSharedFileStream*		CHFSImageFile::recent_1_stream;
short                           CHFSImageFile::recent_1_ref;
short                           CHFSImageFile::recent_1_code;
SyncFSSpec                      CHFSImageFile::recent_2_spec;
class LSharedFileStream*		CHFSImageFile::recent_2_stream;
short                           CHFSImageFile::recent_2_ref;
short                           CHFSImageFile::recent_2_code;
int                             CHFSImageFile::HFSImageHash[SHFSIMAGE_HASH_SIZE];


// ---------------------------------------------------------------------------
//	¥ (Yet Another) EqualFSSpec
// ---------------------------------------------------------------------------

Boolean	CHFSImageFile::EqualFSSpec(SyncFSSpec &inSpec1, SyncFSSpec &inSpec2)
{
    #if __LP64__
        if (!inSpec1.file_ref || !inSpec2.file_ref)
            return (FALSE);
        
        if (inSpec1.file_ref == inSpec2.file_ref)
            return TRUE;
        
        // Compare using absolute posix paths
        CFStringRef spec1Path = inSpec1.file_ref->GetPath();
        CFStringRef spec2Path = inSpec2.file_ref->GetPath();
        
        if (!spec1Path || !spec2Path)
            return false;
        
        return (int) (CFStringCompare(spec1Path, spec2Path, 0) == kCFCompareEqualTo);
    #else
        int j;
        
        if (!&inSpec1 || !&inSpec2)
            return (false);
            
        if (inSpec1.name[0] == 0 || inSpec2.name[0] == 0)
            return (false);
        
        if (inSpec1.vRefNum != inSpec2.vRefNum)
            return (false);
            
        if (inSpec1.parID   != inSpec2.parID)
            return (false);
            
        if (inSpec1.name[0] != inSpec2.name[0])
            return (false);
            
        for (j=1; j<=inSpec1.name[0]; j++)
        {
            if (inSpec1.name[j] != inSpec2.name[j])
            return (false);
        }
    
        return (true);
    #endif
}

// ---------------------------------------------------------------------------
//	¥ GetRefNum
// ---------------------------------------------------------------------------

// Returns ref num for this FSSpec

short	CHFSImageFile::GetRefNum(SyncFSSpec &what)
{
	// Check for bogus spec
	if (what.name[0] == 0)									// Invalid FSSpec
		return 0;

	// Check for currently cached item(s)
	if (recent_1_spec.name[0] && EqualFSSpec(what, recent_1_spec))
		return (recent_1_ref);
		
	if (recent_2_spec.name[0] && EqualFSSpec(what, recent_2_spec))
		return (recent_2_ref);

	// Look for fsspec		
	int hash  = (what.parID & (SHFSIMAGE_HASH_SIZE - 1));	// Hash by par ID
	int which = HFSImageHash[hash];
	
	while (which)
	{
		SHFSImageFileStruct& it = HFSImageFileListItemAt(which);
		
		if (EqualFSSpec(what, it.hfs_image_spec))
			return (which);
		
		which = it.hfs_image_link;
	}
	
	return (0);
}


// ---------------------------------------------------------------------------
//	¥ Add
// ---------------------------------------------------------------------------

// Adds FSSpec to list
short	CHFSImageFile::Add(SyncFSSpec &what, char* name)
{
    SHFSImageFileStruct* it  = NULL;
	short                ref = 0;
    
    // Re-use free space
	if (HFSImageFileListFree)
	{
		ref = HFSImageFileListFree;
        it  = &HFSImageFileListItemAt(ref);
		
        HFSImageFileListFree = it->hfs_image_link;
	}
	
	else {
        it = new SHFSImageFileStruct;
        
        if (!it)
            throw(0);
        
        ref = HFSImageFileListAdd(*it);
    }
    
	int  hash  = (what.parID & (SHFSIMAGE_HASH_SIZE - 1));	// Hash by par ID
	int  which = HFSImageHash[hash];
	
    memset(it, 0, sizeof(*it));
    
	it->hfs_image_spec = what;
	it->hfs_image_link = which;
	strcpy(it->hfs_image_name, name);
    
    SyncFSSpecRetain(&it->hfs_image_spec);
	
	HFSImageHash[hash] = ref;
	
	return (ref);
}


// ---------------------------------------------------------------------------
//	¥ GetFSSpec
// ---------------------------------------------------------------------------

// Look up FSSpec for a refnum without accessing it
SyncFSSpec	CHFSImageFile::GetFSSpec(short  refNum)
{
	SyncFSSpec          retVal;
	SyncUint32			index = refNum;

	memset(&retVal, 0, sizeof(retVal));
	
    if (index < 1 || index > HFSImageFileList.Count())
		return (retVal);
		
	return (HFSImageFileListItemAt(index).hfs_image_spec);
}


// ---------------------------------------------------------------------------
//	¥ GetDevCode
// ---------------------------------------------------------------------------

// Set up to access a device
short	CHFSImageFile::GetDevCode(short  refNum)
{
	// See if cached:
	if (recent_1_ref == refNum)
		return recent_1_code;
		
	if (recent_2_ref == refNum)
		return recent_2_code;
	
	// Set up to use
	SHFSImageFileStruct& it = HFSImageFileListItemAt(refNum);
	StFastMutex	mutex(gXPLMutex);
	
	// Close out cache2; swap; use cache1
    SyncFSSpecRelease(&recent_2_spec);

	if (recent_2_stream)
	{
		if (recent_2_code)
			XPLRunTime_FreeHFSReaddataCode(recent_2_code);
		
		memset(&recent_2_spec, 0, sizeof(recent_2_spec));

		recent_2_stream->CloseSharedFile();
		recent_2_stream    = NULL;
		recent_2_ref       = 0;
		recent_2_code      = 0;
	}

	recent_2_spec   = recent_1_spec;
	recent_2_stream = recent_1_stream;
	recent_2_ref    = recent_1_ref;
	recent_2_code   = recent_1_code;

	memset(&recent_1_spec, 0, sizeof(recent_1_spec));
	recent_1_stream  = NULL;
	recent_1_ref     = 0;
	recent_1_code    = 0;
	
	recent_1_stream = LSharedFileStream::OpenSharedFile(it.hfs_image_spec);
		
	if (recent_1_stream)
	{
		recent_1_spec = it.hfs_image_spec;
		recent_1_ref  = refNum;
		recent_1_code = XPLRunTime_AssignHFSReaddataCode(recent_1_stream->GetDataForkRefNum(), &recent_1_spec);

        SyncFSSpecRetain(&recent_1_spec);
    }

	return recent_1_code;
}


// ---------------------------------------------------------------------------
//	¥ LatchDevCode, ReleaseDevCode
// ---------------------------------------------------------------------------

// Latch on to a HFS image file by incrementing its device code count
short	CHFSImageFile::LatchDevCode(short  refNum)
{
	if (!refNum)									// Bogus
		return (0);
		
	GetDevCode(refNum);								// Cache data
	
	if (recent_1_stream && recent_1_ref == refNum)
	{
		recent_1_stream->AddUser();
		
		if (recent_1_code)
			XPLRunTime_LatchHFSReaddataCode(recent_1_code);
			
		return recent_1_code;
	}
	
	if (recent_2_stream && recent_2_ref == refNum)
	{
		recent_2_stream->AddUser();
		
		if (recent_2_code)
			XPLRunTime_LatchHFSReaddataCode(recent_2_code);
			
		return recent_2_code;
	}
	
	return (0);
}

void	CHFSImageFile::ReleaseDevCode(short  refNum)
{
	if (!refNum)									// Bogus
		return;
		
	GetDevCode(refNum);								// Cache data
	
	if (recent_1_stream && recent_1_ref == refNum)
	{
		if (recent_1_stream->GetUsers() > 1)		// Most likely the case
		{
			recent_1_stream->RemoveUser();

			if (recent_1_code)
				XPLRunTime_FreeHFSReaddataCode(recent_1_code);
		}
		
        else
		{
			recent_1_stream->CloseSharedFile();
			
			if (recent_1_code)
				XPLRunTime_FreeHFSReaddataCode(recent_1_code);

            SyncFSSpecRelease(&recent_1_spec);

			memset(&recent_1_spec, 0, sizeof(recent_1_spec));
			
			recent_1_stream->CloseSharedFile();
			recent_1_stream    = NULL;
			recent_1_ref       = 0;
			recent_1_code      = 0;
		}
	}

	if (recent_2_stream && recent_2_ref == refNum)
	{
		if (recent_2_stream->GetUsers() > 1)		// Most likely the case
		{
			recent_2_stream->RemoveUser();

			if (recent_2_code)
				XPLRunTime_FreeHFSReaddataCode(recent_2_code);
		}
		else
		{
			recent_2_stream->CloseSharedFile();
			
			if (recent_2_code)
				XPLRunTime_FreeHFSReaddataCode(recent_2_code);

            SyncFSSpecRelease(&recent_2_spec);
            
			memset(&recent_2_spec, 0, sizeof(recent_2_spec));
			
			recent_2_stream->CloseSharedFile();
			recent_2_stream    = NULL;
			recent_2_ref       = 0;
			recent_2_code      = 0;
		}
	}
}


// ---------------------------------------------------------------------------
//	¥ GetCachedLength
// ---------------------------------------------------------------------------

// Look up the length for an active file
long long	CHFSImageFile::GetCachedLength(short  refNum)
{
	// See if cached:
	if (recent_1_stream && recent_1_ref == refNum)
		return recent_1_stream->GetForkLength();

	if (recent_2_stream && recent_2_ref == refNum)
		return recent_2_stream->GetForkLength();

	return (0);
}


// ---------------------------------------------------------------------------
//	¥ CloseCaches
// ---------------------------------------------------------------------------

// Close XPL access to any hfs image recently opened
void	CHFSImageFile::CloseCaches()
{
	if (recent_1_stream)									// Close shared file if any
	{
		if (recent_1_code)
			XPLRunTime_FreeHFSReaddataCode(recent_1_code);
		
        SyncFSSpecRelease(&recent_1_spec);
        
		memset(&recent_1_spec, 0, sizeof(recent_1_spec));
		
		recent_1_stream->CloseSharedFile();
		recent_1_stream    = NULL;
		recent_1_ref       = 0;
		recent_1_code      = 0;
	}

	if (recent_2_stream)									// Close shared file if any
	{
		if (recent_2_code)
			XPLRunTime_FreeHFSReaddataCode(recent_2_code);
		
        SyncFSSpecRelease(&recent_2_spec);
        
		memset(&recent_2_spec, 0, sizeof(recent_2_spec));

		recent_2_stream->CloseSharedFile();
		recent_2_stream  = NULL;
		recent_2_ref     = 0;
		recent_2_code    = 0;
	}
}


// ---------------------------------------------------------------------------
//	¥ Prune
// ---------------------------------------------------------------------------

// Remove FSSpecs from list when owning image file is removed from device or header

void	CHFSImageFile::Prune(LCStr255 &what)
{
	long i;
	
	CloseCaches();											// Make sure things closed

	for (i=0; i < HFSImageFileList.Count(); i++)
	{
		SHFSImageFileStruct& it = HFSImageFileListItemAt(i+1);
		LCStr255			 itsName (it.hfs_image_name);
		
		if (itsName[0] && itsName.BeginsWith(what))
		{
            SyncFSSpecRelease(&it.hfs_image_spec);
            
			memset(&it.hfs_image_spec, 0, sizeof(it.hfs_image_spec));
			memset(&it.hfs_image_name, 0, sizeof(it.hfs_image_name));
			it.hfs_image_link = HFSImageFileListFree;
			HFSImageFileListFree = i+1;
		}
	}
	
	// Reconstruct hash list
	memset(HFSImageHash, 0, sizeof(HFSImageHash));
	
	for (i=0; i < HFSImageFileList.Count(); i++)
	{
		SHFSImageFileStruct& it = HFSImageFileListItemAt(i+1);
		
		if (it.hfs_image_spec.name[0])
		{
			long hash  = (it.hfs_image_spec.parID & (SHFSIMAGE_HASH_SIZE - 1));	// Hash by par ID
			
			it.hfs_image_link  = HFSImageHash[hash];
			HFSImageHash[hash] = i+1;
		}
	}
}
