// =================================================================================
//	CHFSImageFile.h
// =================================================================================

// Manages access to image files stored on the mac.

#pragma once

#include "SynclavierFileReference.h"
#include "CSynclavierMutableArray.h"

// PowerPlant addons
#include "LCStr255.h"

// SHFSImageFileStruct

#define				SHFSIMAGE_HASH_SIZE	256

typedef struct SHFSImageFileStruct				// Holds information about an image file that has been found
{
	SyncFSSpec		hfs_image_spec;				// Holds the file spec
	char			hfs_image_name[256];		// Complete name of file as it is known to InterChange (c string)
	int             hfs_image_link;				// Hash index pointer

} SHFSImageFileStruct;

#if __LP64__
    typedef CSynclavierFileReference xpl_stream;
#else
    typedef class LSharedFileStream  xpl_stream;
#endif

// ------------------------
// CHFSImageFile
// ------------------------
// Note: refNums in this context refer to a one-based index into HFSImageFileList. It acts as a file-handle.

class	CHFSImageFile {

public:
	static	Boolean							EqualFSSpec 	(SyncFSSpec   &in1, SyncFSSpec &ins);

	static	short							GetRefNum   	(SyncFSSpec   &what);				// Returns ref num for this FSSpec; 0 if not there
	static	short							Add				(SyncFSSpec   &what, char* name);	// Add to list; return ref num
	static	SyncFSSpec						GetFSSpec  		(short    refNum);                  // Look up FSSpec for a refnum without accessing it
	static	short							GetDevCode  	(short    refNum);                  // Activates and sets up for accessing
	static	short							LatchDevCode  	(short    refNum);                  // Activates device and increments user count
	static	void							ReleaseDevCode 	(short    refNum);                  // Releases latched device code
	static	long long						GetCachedLength (short    refNum);                  // Activates and sets up for accessing

	static	void							CloseCaches ();                                     // Close 'most recent' caches

	static	void							Prune  		(LCStr255 &what);                       // Prune files when device removed
										
protected:
	static  CSynclavierMutableArray         HFSImageFileList;
	static	int                             HFSImageFileListFree;

    static  SHFSImageFileStruct&            HFSImageFileListItemAt(SyncUint32 which)          {return * (SHFSImageFileStruct*) HFSImageFileList.ItemAt(which-1);};
    static  SyncUint32                      HFSImageFileListAdd   (SHFSImageFileStruct& item) {HFSImageFileList.Reserve(SHFSIMAGE_HASH_SIZE); HFSImageFileList.PushLast(&item); return HFSImageFileList.Count();};
    
	static	SyncFSSpec                      recent_1_spec;
	static	xpl_stream*                     recent_1_stream;
	static	short                           recent_1_ref;
	static	short                           recent_1_code;
	static	SyncFSSpec                      recent_2_spec;
	static	xpl_stream*                     recent_2_stream;
	static	short                           recent_2_code;
	static	short                           recent_2_ref;

	static	int                             HFSImageHash[SHFSIMAGE_HASH_SIZE];                  // Hash by ParID; index to first SHFSImageFileStruct of that ParID
};
