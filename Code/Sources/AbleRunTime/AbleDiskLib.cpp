// =================================================================================
//	AbleDiskLib
// =================================================================================

// Library for importing/exporting from Synclavier¨ Disks

// 3/5/99 C. Jones

// Std C
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Mac OS
#if !__LP64__
    #include <NameRegistry.h>
    #include <Events.h>
    #include <Files.h>
    #include <ctype.h>

    #include "CInterChangeDeviceButton.h"
    #include "CInterChangeApp.h"
    #include "LFastMutexSemaphore.h"
    #include "CInterChangeWindow.h"
#endif

// PowerPlant
#if !__LP64__
    #include <LThread.h>
#endif

// Local includes
#include "Standard.h"
#include "XPL.h"
#include "XPLRuntime.h"
#include "Catrtns.h"
#include "ScsiLib.h"
#include "Utility.h"
#include "AbleDiskLib.h"
#include "MacHFSLib.h"
#include "AbleOptLib.h"
#include "Messages.h"
#include "D24Sim.h"
#include "AliasUtilities.h"
#include "Interchange.h"
#include "CHFSImageFile.h"
#include "MacSCSIOSx.h"

#if (SYNC_USE_ASYNC_IO)
    #include "TimeShare.h"
#endif

// =================================================================================
//		¥ Class stThrowOnDIskError
// =================================================================================

// Handy class to set/reset g_throw_on_disk_error
class	stThrowOnDIskError
{
	public:
		stThrowOnDIskError ();
		~stThrowOnDIskError();
		
	boolean	prior_throw;
};

stThrowOnDIskError::stThrowOnDIskError()
{
	prior_throw = g_throw_on_disk_error;
	g_throw_on_disk_error = true;
}

stThrowOnDIskError::~stThrowOnDIskError()
{
	g_throw_on_disk_error = prior_throw;
}


// =================================================================================
//		¥ Class stCatSemaphore
// =================================================================================

LThread*	stCatSemaphore::owner;
int			stCatSemaphore::owner_count;

// Description: Yield waiting for access to the catalog routine variables.
// Special handling if we are the main thread, some other thread is using
// the catalog routines, and a thread is waiting for the D24 port.  We must
// wait next event in that case so the D24 get's freed up.
stCatSemaphore::stCatSemaphore()
{
	if (owner == NULL)											// No current owner: latch ourselves
	{															// with a count of 1
		owner       = LThread::GetCurrentThread();
		owner_count = 1;
	}
	
	else if (owner == LThread::GetCurrentThread())				// Second latch for same owner
		owner_count++;											// do so...
		
	else														// Else is new owner.  Current owner must have Yielded...
	{
		Boolean timeShareNeeded = (LThread::InMainThread() && D24Sim_AnyNonMainThreadsWaiting());
		
		if (timeShareNeeded)
			TimeShare_Initialize();
			
		while (owner)										// Wait for current owner to finish
		{
            #if (SYNC_USE_ASYNC_IO)
                if (timeShareNeeded)
                    TimeShare_TimeShare(mDownMask | mUpMask | keyDownMask | keyUpMask | autoKeyMask);
            #endif
            
			LThread::Yield();
		}
		
		if (timeShareNeeded)
			TimeShare_CleanUp();
		
		owner       = LThread::GetCurrentThread();				// And latch it ourselves
		owner_count = 1;
	}
}

stCatSemaphore::~stCatSemaphore()
{
	if (--owner_count == 0)
		owner = NULL;
}


// =================================================================================
//		¥ Class stCatBuf
// =================================================================================

// Handy class to save catalog variables on stack.  Used by DoCheck task to temporarily
// cache catalog informatio
class	stCatBuf
{
	public:
		stCatBuf ();
		~stCatBuf();
		
		static	fixed	cached_buf_ptr;
		static	fixed	cached_buf_med;
		static	fixed	cached_c_ms_sector;
		static	fixed	cached_c_ls_sector;
		static	fixed	cached_c_ms_length;
		static	fixed	cached_c_ls_length;
		static	fixed	cached_c_dir_siz;
		
		fixed	saved_buf_ptr;
		fixed	saved_buf_med;
		fixed	saved_c_ms_sector;
		fixed	saved_c_ls_sector;
		fixed	saved_c_ms_length;
		fixed	saved_c_ls_length;
		fixed	saved_c_dir_siz;
};

fixed	stCatBuf::cached_buf_ptr;
fixed	stCatBuf::cached_buf_med;
fixed	stCatBuf::cached_c_ms_sector;
fixed	stCatBuf::cached_c_ls_sector;
fixed	stCatBuf::cached_c_ms_length;
fixed	stCatBuf::cached_c_ls_length;
fixed	stCatBuf::cached_c_dir_siz;

stCatBuf::stCatBuf()
{
	if (!cached_buf_ptr)
	{
		cached_buf_ptr = _allocate_able_heap(c_dir_max);
		cached_buf_med = 0;
		cached_c_ms_sector = -1;
		cached_c_ls_sector = -1;
		cached_c_ms_length = 0;
		cached_c_ls_length = 0;
		cached_c_dir_siz   = 0;
	}
	
	saved_buf_ptr     = c_bufptr;
	saved_buf_med     = c_bufmed;
	saved_c_ms_sector = c_ms_sector;
	saved_c_ls_sector = c_ls_sector;
	saved_c_ms_length = c_ms_length;
	saved_c_ls_length = c_ls_length;
	saved_c_dir_siz   = c_dir_size;
	
	c_bufptr    = cached_buf_ptr;
	c_bufmed    = cached_buf_med;
	c_ms_sector = cached_c_ms_sector;
	c_ls_sector = cached_c_ls_sector;
	c_ms_length = cached_c_ms_length;
	c_ls_length = cached_c_ls_length;
	c_dir_size  = cached_c_dir_siz;
}

stCatBuf::~stCatBuf()
{
	cached_c_ms_sector = c_ms_sector;
	cached_c_ls_sector = c_ls_sector;
	cached_c_ms_length = c_ms_length;
	cached_c_ls_length = c_ls_length;
	cached_c_dir_siz   = c_dir_size;

	c_bufptr    = saved_buf_ptr;
	c_bufmed    = saved_buf_med;
	c_ms_sector = saved_c_ms_sector;
	c_ls_sector = saved_c_ls_sector;
	c_ms_length = saved_c_ms_length;
	c_ls_length = saved_c_ls_length;
	c_dir_size  = saved_c_dir_siz;
}


// =================================================================================
//		¥ Global Variables
// =================================================================================

char	AbleDiskLib_RecentErrorMessage[MESSAGE_BUF_SIZE];


// =================================================================================
//		¥ Constants
// =================================================================================

#define COPY_BUFFER_SIZE	(1024*1024)  		// Must be multiple of 1024 bytes to provide for access to optical disk [1024-byte sectors]
#define D24_BUFFER_SIZE     ( 128*1024)         // Size to use when using D24 to deal with attrociously slow rates

// =================================================================================
//		¥ SortInterChangeItems
// =================================================================================

// Builds outList list of entry pointers.  Limited to 256 entries.  Requires
// all objects in sort list to be of same file system type.

// File type sort order: folders first (0), then sequences (1), timbres (2), sounds (3), indexes (4), text (5), data (6)
int	AbleDiskLib_SortKey[AbleDiskLibTypeSize] = {5, 6, 6, 6, 1, 3, 0, 0, 6, 6, 4, 2, 6, 6, 6, 6,			// Basic Able file types
                                                0, 0, 3, 3,	3,											// InterChange_HFSFolderTypeCode_fold, InterChange_HFSFolderTypeCode_disk, InterChange_HFSFolderTypeCode_AIFF, InterChange_HFSFolderTypeCode_Sd2f, InterChange_HFSFolderTypeCode_WAVE
												0, 3, 3, 6};                                            // InterChange_AbleOpticalDirectory, InterChange_HFSFolderTypeCode_CAF, InterChange_HFSFolderTypeCode_MP3, InterChange_HFSFolderTypeCode_unk
												
void	SortInterChangeItems(void* inList, int inNumEntries, void* outList[], int theSortCode)
{
	Boolean used[AbleDiskLibSortSize];
	int		i;
	int		done_sorted = 0;
	
	// Limit
	while (inNumEntries > AbleDiskLibSortSize)
		DebugStr("\pSortInterChangeItems Fatal Error\n");
			
	if (inNumEntries == 0)
		return;
		
	// Init output array and already used list	
	for (i=0; i<inNumEntries; i++)
	{
		outList[i] = NULL;
		used   [i] = false;
	}

	// Look up info about the type of object we are sorting
	char*   whereList         = (char *) inList;
	short	theFileSystemCode = ((InterChangeItem *) inList)->file_system;
	short	whereName         = 0;
	short	itemSize          = 0;
	
	switch (theFileSystemCode)
	{
		case InterChange_MacHFSFIleSystemCode:
			itemSize  = sizeof(MacHFSItem);
			whereName = (char *) &(((MacHFSItem *) inList)->file_spec.name[1]) - (char *) inList;
			break;

		case InterChange_AbleWDFileSystemCode:
			itemSize  = sizeof(AbleWDItem);
			whereName = (char *) &(((AbleWDItem *) inList)->file_name[0]) - (char *) inList;
			break;
			
		case InterChange_AbleOpticalFileSystemCode:
			itemSize  = sizeof(AbleOpticalItem);
			whereName = (char *) &(((AbleOpticalItem *) inList)->file_name[0]) - (char *) inList;
			break;
			
		default:
			return;
	}		

	// Iterate finding smallest unused entry
	int	first = 0;
	int last  = inNumEntries;
	
	while (done_sorted < inNumEntries)
	{
		// Find first unused.  Trim end as well
		while (first < last && used[first])
			first++;
	
		while (last-1 > first && used[last-1])
			last--;
		
		// Find any smaller item
		int index = first;
		
		if (index < last)
		{
			int	next = index+1;
			
			while (next < last)
			{
				if (!used[next])
				{
					char *whereIndex = whereList + (index*itemSize);
					char *whereNext  = whereList + (next *itemSize);
					char *chosen     = whereIndex + whereName;
					char *considered = whereNext  + whereName;
					InterChangeItem* indexItem = (InterChangeItem *) whereIndex;
					InterChangeItem* nextItem  = (InterChangeItem *) whereNext;
							
					switch (theSortCode)
					{
						// Note: See identical logic in CAbleDirectoryDropFlagTable::AddNewItem...
						case InterChange_sort_by_name_ascending:								// name only
							while (*chosen && toupper(*chosen) == toupper(*considered))
								{chosen++; considered++;}
								
							if (toupper(*considered) < toupper(*chosen))
								index = next;
							break;
						
						case InterChange_sort_by_type_ascending:								// type then name
							if (AbleDiskLib_SortKey[nextItem->file_type] < AbleDiskLib_SortKey[indexItem->file_type])
								index = next;
							
							else if (AbleDiskLib_SortKey[nextItem->file_type] == AbleDiskLib_SortKey[indexItem->file_type])
							{
								while (*chosen && toupper(*chosen) == toupper(*considered))
									{chosen++; considered++;}
									
								if (toupper(*considered) < toupper(*chosen))
									index = next;
							}
							break;
						
						case InterChange_sort_by_size_ascending:								// size then name
							if (nextItem->file_size_bytes < indexItem->file_size_bytes)
								index = next;
							
							else if (nextItem->file_size_bytes == indexItem->file_size_bytes)
							{
								while (*chosen && toupper(*chosen) == toupper(*considered))
									{chosen++; considered++;}
									
								if (toupper(*considered) < toupper(*chosen))
									index = next;
							}
							break;

						case InterChange_sort_by_name_descending:								// name only
							while (*chosen && toupper(*chosen) == toupper(*considered))
								{chosen++; considered++;}
								
							if (toupper(*considered) > toupper(*chosen))
								index = next;
							break;
						
						case InterChange_sort_by_type_descending:								// type then name
							if (AbleDiskLib_SortKey[nextItem->file_type] > AbleDiskLib_SortKey[indexItem->file_type])
								index = next;
							
							else if (AbleDiskLib_SortKey[nextItem->file_type] == AbleDiskLib_SortKey[indexItem->file_type])
							{
								while (*chosen && toupper(*chosen) == toupper(*considered))
									{chosen++; considered++;}
									
								if (toupper(*considered) > toupper(*chosen))
									index = next;
							}
							break;
						
						case InterChange_sort_by_size_descending:								// size then name
							if (nextItem->file_size_bytes > indexItem->file_size_bytes)
								index = next;
							
							else if (nextItem->file_size_bytes == indexItem->file_size_bytes)
							{
								while (*chosen && toupper(*chosen) == toupper(*considered))
									{chosen++; considered++;}
									
								if (toupper(*considered) > toupper(*chosen))
									index = next;
							}
							break;
					}
				}
				
				next++;
			}
			
			outList[done_sorted++] = whereList + (index*itemSize);
			used[index] = true;
		}	
	}
}


// =================================================================================
//		¥ SpinCursor()
// =================================================================================

static	void	SpinCursor()
{
	LThread::Yield();
}


// =================================================================================
//		¥ AbleDiskLib_Initialize()
// =================================================================================

int		AbleDiskLib_Initialize()
{
	g_disallow_run_host_exit++;										// disallow exit from XPL run time environment

	g_scsi_print_basic_opt   = false;
	g_scsi_print_all_opt     = false;
	
	if (initialize_run_time_environment (64))						// minimal 64 sectors external memory for our purposes
		return (-1);
	
	initialize_able_catalog_routines();								// init translated catalog routines
    
    host_yielder = &SpinCursor;
	
	return (0);
}


// =================================================================================
//		¥ AbleDiskLib_Cleanup()
// =================================================================================

void	AbleDiskLib_Cleanup()
{
	cleanup_run_time_environment();

	if (g_disallow_run_host_exit)
		g_disallow_run_host_exit--;
}


// =================================================================================
//	Able String Utilities - AbleFileName2CStr
// =================================================================================

// Converts able file name stored in 4 fixeds into a C string

void AbleFileName2CStr(const uint16 *file_name, char *c_string)
{
	int i;
	
	for (i=0; i<4; i++)
	{
		c_string[(i << 1)    ] = file_name[i] & 0xFF;
		c_string[(i << 1) + 1] = file_name[i] >> 8;
	}
	
	c_string[8] = 0;							// provide trailing 0 in case is 8 char name
}


// =================================================================================
//	Able String Utilities - CheckAbleFileName
// =================================================================================

// Look for ancient cannonical file names and map to correct type

fixed CheckAbleFileName(const uint16 *file_name)
{
	char	c_name[16];
	
	AbleFileName2CStr(file_name, c_name);

	if (strcmp(c_name, ".WORK"   ) == 0)		// .work file
		return t_data;
		
	if (strcmp(c_name, ".BNKDATA") == 0)		// old bankdata format
		return t_data;
		
	if (strcmp(c_name, ".PATDATA") == 0)		// old patchdata format
		return t_data;
		
	if (strcmp(c_name, ".NEWDATA") == 0)		// common bank data format
		return t_timbre;
		
	if (strcmp(c_name, ".SQ0DATA") == 0)		// sequence
		return t_sync;

	if (strcmp(c_name, ".SQ1DATA") == 0)		// sequence
		return t_sync;

	if (strcmp(c_name, ".SQ2DATA") == 0)		// sequence
		return t_sync;

	if (strcmp(c_name, ".SQ3DATA") == 0)		// sequence
		return t_sync;

	if (strcmp(c_name, ".SQ4DATA") == 0)		// sequence
		return t_sync;

	if (strcmp(c_name, ".SQ5DATA") == 0)		// sequence
		return t_sync;

	if (strcmp(c_name, ".SQ6DATA") == 0)		// sequence
		return t_sync;

	if (strcmp(c_name, ".SQ7DATA") == 0)		// sequence
		return t_sync;

	if (strcmp(c_name, ".UNDOSEQ") == 0)		// sequence
		return t_sync;

	return (0);		
}


// =================================================================================
//	Able Utilities - GetAbleFileType
// =================================================================================

// Use .txt for text files (convert to Mac format)
// http://www.fileinfo.net/extension/sprg	t_exec
// http://www.fileinfo.net/extension/srel	t_reloc
// http://www.fileinfo.net/extension/sdat	t_data
// http://www.fileinfo.net/extension/sseq	t_sync
// http://www.fileinfo.net/extension/ssnd	t_sound
// http://www.fileinfo.net/extension/simg	t_lsubc (t_subc not stored as image file)
// http://www.fileinfo.net/extension/sdmp	t_dump
// http://www.fileinfo.net/extension/sspe	t_spect
// http://www.fileinfo.net/extension/sind	t_index
// http://www.fileinfo.net/extension/stmb	t_timbre

// http://www.fileinfo.net/extension/sopt	(not used; envisioned to be optical disk image file; implemented as subcatalog type; determine by extension)

fixed GetAbleFileTypeFromExtension(const struct FSSpec& itsSpec)
{
	int	  name_len = itsSpec.name[0];

	if (name_len >= 6 && itsSpec.name[name_len-4] == '.')
	{
		LCStr255 itsName = itsSpec.name;
		
		if (itsName.EndsWith("\p.sprg"))
			return t_exec;

		if (itsName.EndsWith("\p.srel"))
			return t_reloc;

		if (itsName.EndsWith("\p.sdat"))
			return t_data;

		if (itsName.EndsWith("\p.sseq"))
			return t_sync;

		if (itsName.EndsWith("\p.ssnd"))
			return t_sound;

		if (itsName.EndsWith("\p.simg"))
			return t_lsubc;

		if (itsName.EndsWith("\p.sdmp"))
			return t_dump;

		if (itsName.EndsWith("\p.sspe"))
			return t_spect;

		if (itsName.EndsWith("\p.sind"))
			return t_index;

		if (itsName.EndsWith("\p.stmb"))
			return t_timbre;
	}
	
	return (-1);
}

fixed GetAbleFileType(OSType mac_type, OSType mac_creator, const struct FSSpec& itsSpec)
{
	fixed export_type = GetAbleFileTypeFromExtension(itsSpec);
	
	// Give priority to extension
	if (export_type >= t_text)
		return export_type;
	
	if (mac_creator == 'SNCL' && mac_type =='TEXT')
		export_type = t_text;
		
	else if (mac_creator == 'CWIE' && mac_type =='TEXT')
		export_type = t_text;
		
	else if (mac_creator == 'MPS ' && mac_type =='TEXT')
		export_type = t_text;
		
	else if (mac_creator == 'SNCL' && mac_type == 'EXEC')
		export_type = t_exec;
		
	else if (mac_creator == 'SNCL' && mac_type == 'RLOC')
		export_type = t_reloc;
		
	else if (mac_creator == 'SNCL' && mac_type == 'DATA')
		export_type = t_data;
		
	else if (mac_creator == 'SNCL' && mac_type == 'SQNC')
		export_type = t_sync;
		
	else if (mac_creator == 'SNCL' && mac_type == 'SNDF')
		export_type = t_sound;
		
	else if (mac_creator == 'SNCL' && mac_type == 'DUMP')
		export_type = t_dump;
		
	else if (mac_creator == 'SNCL' && mac_type == 'SPEC')
		export_type = t_spect;
		
	else if (mac_creator == 'SNCL' && mac_type == 'INDX')
		export_type = t_index;
		
	else if (mac_creator == 'SNCL' && mac_type == 'TIMB')
		export_type = t_timbre;

	else if (mac_creator == 'SNCL' && mac_type == 'SUBC')
		export_type = t_lsubc;

	else if (mac_creator == 'MACS' && mac_type == 'fold')
		export_type = InterChange_HFSFolderTypeCode_fold;
	
	else if (mac_creator == 'MACS' && mac_type == 'disk')
		export_type = InterChange_HFSFolderTypeCode_disk;
	
	else if (mac_type == 'AIFF')
		export_type = InterChange_HFSFolderTypeCode_AIFF;
	
	else if (mac_type == 'Sd2f')
		export_type = InterChange_HFSFolderTypeCode_Sd2f;
		
	else if (mac_type == 'WAVE')
		export_type = InterChange_HFSFolderTypeCode_WAVE;
	
	else
		return (-1);
		
	return (export_type);
}


// =================================================================================
//	Able Utilities - GetMacFileType
// =================================================================================

void GetMacFileType(fixed file_type, OSType& out_mac_type, OSType& out_mac_creator)
{
	if (file_type == t_text)
		{out_mac_creator = 'SNCL'; out_mac_type = 'TEXT';}
	
	else if (file_type == t_exec)
		{out_mac_creator = 'SNCL'; out_mac_type = 'EXEC';}
	
	else if (file_type == t_reloc)
		{out_mac_creator = 'SNCL'; out_mac_type = 'RLOC';}
	
	else if (file_type == t_data)
		{out_mac_creator = 'SNCL'; out_mac_type = 'DATA';}
	
	else if (file_type == t_sync)
		{out_mac_creator = 'SNCL'; out_mac_type = 'SQNC';}
	
	else if (file_type == t_sound)
		{out_mac_creator = 'SNCL'; out_mac_type = 'SNDF';}	

	else if (file_type == t_dump)
		{out_mac_creator = 'SNCL'; out_mac_type = 'DUMP';}
	
	else if (file_type == t_spect)
		{out_mac_creator = 'SNCL'; out_mac_type = 'SPEC';}
	
	else if (file_type == t_index)
		{out_mac_creator = 'SNCL'; out_mac_type = 'INDX';}
	
	else if (file_type == t_timbre)
		{out_mac_creator = 'SNCL'; out_mac_type = 'TIMB';}
	
	else if (file_type == t_lsubc)
		{out_mac_creator = 'SNCL'; out_mac_type = 'SUBC';}
	
	else if (file_type == InterChange_HFSFolderTypeCode_fold)
		{out_mac_creator = 'MACS'; out_mac_type = 'fold';}
	
	else if (file_type == InterChange_HFSFolderTypeCode_disk)
		{out_mac_creator = 'MACS'; out_mac_type = 'disk';}

	else if (file_type == InterChange_HFSFolderTypeCode_AIFF)
		{out_mac_creator = 'SNCL'; out_mac_type = 'AIFF';}
	
	else if (file_type == InterChange_HFSFolderTypeCode_Sd2f)
		{out_mac_creator = 'SNCL'; out_mac_type = 'Sd2f';}
		
	else if (file_type == InterChange_HFSFolderTypeCode_WAVE)
		{out_mac_creator = 'SNCL'; out_mac_type = 'WAVE';}
	
	else
		{out_mac_creator = '????'; out_mac_type = '????';}
}


// =================================================================================
//	InterChangeItemUnion utilities - AbleDiskLib_FillUnionFromFSSpec
// =================================================================================

void 	AbleDiskLib_FillUnionFromFSSpec(const struct FSSpec& inSpec, short inType, SInt64 inLength, InterChangeItemUnion& outUnion)
{
	memset(&outUnion, 0, sizeof(outUnion));
	
	outUnion.HFSItem.file_system     = InterChange_MacHFSFIleSystemCode;
	outUnion.HFSItem.file_type       = inType;
	outUnion.HFSItem.file_size_bytes = inLength;
	
	outUnion.HFSItem.file_spec       = inSpec;
}


// ---------------------------------------------------------------------------
//	¥ AbleDiskLib_GetSpecForUnion
// ---------------------------------------------------------------------------

// Looks at an InterChangeItemUnion and extracts certain information about it
// if the union is or resides in a mac HFS item

Boolean	AbleDiskLib_GetSpecForUnion( InterChangeItemUnion &theUnion, FSSpec& theSpec, long long& theBlock, long long& theLength, short& theCode )
{
	// If we are hfs...
	if (theUnion.Item.file_system == InterChange_MacHFSFIleSystemCode)
		{theSpec = theUnion.HFSItem.file_spec; theBlock = 0; theLength = 0; theCode = 0;}
		
	// Else we may be in an WD image file
	else if ((theUnion.Item.file_system == InterChange_AbleWDFileSystemCode)
	&&       (theUnion.WDItem.file_hfs_refnum != 0))
	{
		theSpec   = CHFSImageFile::GetFSSpec(theUnion.WDItem.file_hfs_refnum);
		theBlock  = theUnion.WDItem.file_block_start;
		theLength = theUnion.WDItem.file_size_bytes >> 9;
		theCode   = theUnion.WDItem.file_device_code;
	}

	// Else we may be in an Optical image file
	else if ((theUnion.Item.file_system == InterChange_AbleOpticalFileSystemCode)
	&&       (theUnion.OpticalItem.file_hfs_refnum != 0))
	{
		theSpec   = CHFSImageFile::GetFSSpec(theUnion.OpticalItem.file_hfs_refnum);
		theBlock  = theUnion.OpticalItem.file_block_start;
		theLength = theUnion.OpticalItem.file_size_bytes >> 9;
		theCode   = theUnion.OpticalItem.file_device_code;
	}
		
	// Else may be an added root WD device
	else if ((theUnion.Item.file_system == InterChange_AbleWDFileSystemCode)
	&&       (theUnion.WDItem.file_device_code >= ABLE_HFS_READDATA_CODE)
	&&       (find_hfs_scsi_spec(theUnion.WDItem.file_device_code)))
	{
		theSpec   = *find_hfs_scsi_spec(theUnion.WDItem.file_device_code);
		theBlock  = theUnion.WDItem.file_block_start;
		theLength = theUnion.WDItem.file_size_bytes >> 9;
		theCode   = theUnion.WDItem.file_device_code;
	}
		
	// Else may be an added root Optical device
	else if ((theUnion.Item.file_system == InterChange_AbleOpticalFileSystemCode)
	&&       (theUnion.OpticalItem.file_device_code >= ABLE_HFS_READDATA_CODE)
	&&       (find_hfs_scsi_spec(theUnion.OpticalItem.file_device_code)))
	{
		theSpec   = *find_hfs_scsi_spec(theUnion.OpticalItem.file_device_code);
		theBlock  = theUnion.OpticalItem.file_block_start;
		theLength = theUnion.OpticalItem.file_size_bytes >> 9;
		theCode   = theUnion.OpticalItem.file_device_code;
	}
		
	// Else may be a legacy WD image file
	else if ((theUnion.Item.file_system == InterChange_AbleWDFileSystemCode)
	&&       (theUnion.WDItem.file_device_code < ABLE_HFS_READDATA_CODE)
	&&       (access_scsi_device(theUnion.WDItem.file_device_code))
	&&       (access_scsi_device(theUnion.WDItem.file_device_code)->fFRefNum))
	{
		theSpec   = access_scsi_device(theUnion.WDItem.file_device_code)->fFSSpec;
		theBlock  = theUnion.WDItem.file_block_start;
		theLength = theUnion.WDItem.file_size_bytes >> 9;
		theCode   = theUnion.WDItem.file_device_code;
	}

	// Else may be a legacy Optical image file
	else if ((theUnion.Item.file_system == InterChange_AbleOpticalFileSystemCode)
	&&       (theUnion.OpticalItem.file_device_code < ABLE_HFS_READDATA_CODE)
	&&       (access_scsi_device(theUnion.OpticalItem.file_device_code))
	&&       (access_scsi_device(theUnion.OpticalItem.file_device_code)->fFRefNum))
	{
		theSpec  = access_scsi_device(theUnion.OpticalItem.file_device_code)->fFSSpec;
		theBlock  = theUnion.OpticalItem.file_block_start;
		theLength = theUnion.OpticalItem.file_size_bytes >> 9;
		theCode   = theUnion.OpticalItem.file_device_code;
	}
		
	// Else not found
	else
		return (false);
		
	return (true);
}


// ---------------------------------------------------------------------------
//	¥ AbleDiskLib_UnionsAreEquivalent
// ---------------------------------------------------------------------------

// Examines two InterChangeItemUnions and returns true if they are in fact
// the same item.  Handles cases where different paths point to the same
// image file.

Boolean
AbleDiskLib_UnionsAreEquivalent( InterChangeItemUnion &unionOne, InterChangeItemUnion &unionTwo)
{
	FSSpec      one;
	FSSpec      two;
	long long   oneStart = 0, oneLength = 0;
	short       oneCode  = 0;
	long long   twoStart = 0, twoLength = 0;
	short       twoCode  = 0;
	
	// Check for legacy device items first
	if ((unionOne.Item.file_system       == InterChange_AbleWDFileSystemCode)
	&&  (unionOne.WDItem.file_hfs_refnum == 0))
	{
		oneCode  = unionOne.WDItem.file_device_code;
		oneStart = unionOne.WDItem.file_block_start;
	}

	if ((unionOne.Item.file_system       == InterChange_AbleOpticalFileSystemCode)
	&&  (unionOne.OpticalItem.file_hfs_refnum == 0))
	{
		oneCode  = unionOne.OpticalItem.file_device_code;
		oneStart = unionOne.OpticalItem.file_block_start;
	}
	
	if ((unionTwo.Item.file_system       == InterChange_AbleWDFileSystemCode)
	&&  (unionTwo.WDItem.file_hfs_refnum == 0))
	{
		twoCode  = unionTwo.WDItem.file_device_code;
		twoStart = unionTwo.WDItem.file_block_start;
	}

	if ((unionTwo.Item.file_system       == InterChange_AbleOpticalFileSystemCode)
	&&  (unionTwo.OpticalItem.file_hfs_refnum == 0))
	{
		twoCode  = unionTwo.OpticalItem.file_device_code;
		twoStart = unionTwo.OpticalItem.file_block_start;
	}
	
	// Equivalent item if same device and same block start
	if (oneCode && oneCode == twoCode && oneStart == twoStart)
		return (true);

	// Note: may still be same item even if device codes differ if both
	// point to same HFS item...
	
	// Find one FSSpec.  Not equivalent if not same device and no hfs to compare
	if (!AbleDiskLib_GetSpecForUnion(unionOne, one, oneStart, oneLength, oneCode))
		return (false);
		
	// Find two FSSPec.  Not equivalent if not same device and no hfs to compare
	if (!AbleDiskLib_GetSpecForUnion(unionTwo, two, twoStart, twoLength, twoCode))
		return (false);
	
	// Different start: not the same item
	if (oneStart != twoStart)
		return (false);

	// Else is same item if same file and same start
	return (InterChangeLibEqualFSSpecs(&one, &two));
}


// ---------------------------------------------------------------------------
//	¥ AbleDiskLib_UnionDescendsFrom
// ---------------------------------------------------------------------------

// Examines two InterChangeItemUnions and returns true the first union
// is the parent of the second union.  Returns false if unions are
// equivalent (should be handled elsewhere)

// Note: numLevels not detected for Able cases...

Boolean
AbleDiskLib_UnionDescendsFrom( InterChangeItemUnion &parentUnion, InterChangeItemUnion &childUnion, int numLevels )
{
	FSSpec      parent;
	FSSpec      child;
	long long   parentStart = 0, parentLength = 0;
	short       parentCode  = 0;
	long long   childStart  = 0, childLength = 0;
	short       childCode   = 0;
	
	// Check for legacy device items first
	if ((parentUnion.Item.file_system       == InterChange_AbleWDFileSystemCode)
	&&  (parentUnion.WDItem.file_hfs_refnum == 0))
	{
		parentCode  = parentUnion.WDItem.file_device_code;
		parentStart = parentUnion.WDItem.file_block_start;
	}

	if ((parentUnion.Item.file_system       == InterChange_AbleOpticalFileSystemCode)
	&&  (parentUnion.OpticalItem.file_hfs_refnum == 0))
	{
		parentCode  = parentUnion.OpticalItem.file_device_code;
		parentStart = parentUnion.OpticalItem.file_block_start;
	}
	
	if ((childUnion.Item.file_system       == InterChange_AbleWDFileSystemCode)
	&&  (childUnion.WDItem.file_hfs_refnum == 0))
	{
		childCode  = childUnion.WDItem.file_device_code;
		childStart = childUnion.WDItem.file_block_start;
	}

	if ((childUnion.Item.file_system       == InterChange_AbleOpticalFileSystemCode)
	&&  (childUnion.OpticalItem.file_hfs_refnum == 0))
	{
		childCode  = childUnion.OpticalItem.file_device_code;
		childStart = childUnion.OpticalItem.file_block_start;
	}
	
	// Look for parent device containing child.  If same legacy device, trust file name match to determine correct answer
	if (parentCode && parentCode >= ABLE_HFS_READDATA_CODE && parentCode == childCode && parentStart == 0 && childStart != 0)
		return (true);

	// Look for parent subcat containing child.  If same legacy device, trust file name match to determine correct answer
	if (parentCode && parentCode >= ABLE_HFS_READDATA_CODE && parentCode == childCode && childStart > parentStart && childStart < parentStart + parentLength)
		return (true);
	
	// Note: may in fact be parent if device codes differ if legacy device is also
	// in HFS hierarchy...
	
	// Find parent FSSpec.  Not parent if not same device and no hfs to compare
	if (!AbleDiskLib_GetSpecForUnion(parentUnion, parent, parentStart, parentLength, parentCode))
		return (false);
		
	// Find child FSSPec.  Not parent if not same device and no hfs to compare
	if (!AbleDiskLib_GetSpecForUnion(childUnion, child, childStart, childLength, childCode))
		return (false);
	
	// If equal, must look further.
	if (InterChangeLibEqualFSSpecs(&parent, &child))
	{
		// If same legacy device, trust file name match to determine correct answer
		if (parentCode && parentCode == childCode && parentCode < ABLE_HFS_READDATA_CODE)
			return (false);

		// If we are subcat on same device...
		if (parentStart == 0 && childStart != 0)
			return (true);
		
		// If we are in his subcat
		if (childStart > parentStart && childStart < parentStart+parentLength)
			return (true);
		
		return (false);
	}
		
	// Track lineage
	return (AliasUtilities_DescendsFrom(parent, child, numLevels));
}


// =================================================================================
//		¥ AbleDiskLib_IsolateTopDirectory
// =================================================================================

// This routine takes a root device (identified by its InterChangeItemUnion) and a complete treename
// that is on that device.  It extracts a top union and a top name that can be used
// to actually find the item in question.  Its purpose is to access an image file in
// a mac tree name.

void	AbleDiskLib_IsolateTopDirectory ( InterChangeItemUnion &rootUnion, LCStr255 &rootName, InterChangeItemUnion &topUnion, LCStr255 &topName)
{
	int nextColon;

	// Special case on Mac
	if (rootUnion.Item.file_system == InterChange_MacHFSFIleSystemCode)				// Mac: descend tree looking for image files
	{
		if ((rootUnion.HFSItem.file_type == InterChange_HFSFolderTypeCode_fold)		// Folder or volume
		||  (rootUnion.HFSItem.file_type == InterChange_HFSFolderTypeCode_disk))
			MacHFSLib_IsolateTopDirectory ( rootUnion, rootName, topUnion, topName );
			
		else																		// Else must be HFS image file
		{
			// Note: if directory is an hfs image file it is already registered with
			// xpl runtime (e.g. is an added root device).
			ufixed	     itsDeviceCode = XPLRunTime_LookForFSSpecInUse(&rootUnion.HFSItem.file_spec);
			scsi_device* itsDevice     = find_hfs_scsi_device(itsDeviceCode);

			if (itsDevice && ((itsDevice->fDeviceType == DEVICE_ABLE_OPTICAL) || (itsDevice->fDeviceType == DEVICE_BLANK_ABLE_OPTICAL)))
			{
				topUnion.OpticalItem.file_system = InterChange_AbleOpticalFileSystemCode;
				topUnion.OpticalItem.file_type   = InterChange_AbleOpticalDirectory;

				topUnion.OpticalItem.file_size_bytes   = rootUnion.HFSItem.file_size_bytes;
				topUnion.OpticalItem.file_device_code  = itsDeviceCode;
				topUnion.OpticalItem.file_hfs_refnum   = 0;

				topUnion.OpticalItem.file_block_start  = 0;
				topUnion.OpticalItem.file_name[0]      = 0;
			}

			else
			{
				topUnion.WDItem.file_system = InterChange_AbleWDFileSystemCode;
				topUnion.WDItem.file_type   = t_lsubc;
			
				topUnion.WDItem.file_size_bytes   = rootUnion.HFSItem.file_size_bytes;
				topUnion.WDItem.file_device_code  = itsDeviceCode;
				topUnion.WDItem.file_hfs_refnum   = 0;

				topUnion.WDItem.file_block_start  = 0;
				topUnion.WDItem.file_name[0]      = 0;
			}
			
			nextColon = rootName.Find(':', 1);										// find first :

			if (nextColon == 0 || nextColon == rootName.Length())					// no colon? or looking for xyz:
				topName = "\p";														// return null string for name (e.g. reference to directory itself)
			else																	// else file name starts with colon on selected device (e.g. :cat:cat:cat:file
				topName.Assign((void *) (&rootName[nextColon]), (UInt8) (rootName.Length() + 1 - nextColon));
		}

		return;
	}
	
	// Else see if able WD
	else if (rootUnion.Item.file_system == InterChange_AbleWDFileSystemCode)		// Able device
	{
		topUnion = rootUnion;														// Start with union for directory

		// Look up code for possible ref num, although we don't latch it here
		if (topUnion.WDItem.file_hfs_refnum)
			topUnion.WDItem.file_device_code = CHFSImageFile::GetDevCode(topUnion.WDItem.file_hfs_refnum);

		nextColon = rootName.Find(':', 1);											// find first :

		if (nextColon == 0 || nextColon == rootName.Length())						// no colon? or looking for xyz:
			topName = "\p";															// return null string for name (e.g. reference to directory itself)
		else																		// else file name starts with colon on selected device (e.g. :cat:cat:cat:file
			topName.Assign((void *) (&rootName[nextColon]), (UInt8) (rootName.Length() + 1 - nextColon));

		return;
	}

	// Else see if able optical
	else if (rootUnion.Item.file_system == InterChange_AbleOpticalFileSystemCode)	// Able optical
	{
		topUnion = rootUnion;														// Start with union for directory

		// Look up code for possible ref num, although we don't latch it here
		if (topUnion.OpticalItem.file_hfs_refnum)
			topUnion.OpticalItem.file_device_code = CHFSImageFile::GetDevCode(topUnion.OpticalItem.file_hfs_refnum);

		nextColon = rootName.Find(':', 1);											// find first :

		if (nextColon == 0 || nextColon == rootName.Length())						// no colon? or looking for xyz:
			topName = "\p";															// return null string for name (e.g. reference to directory itself)
		else																		// else file name starts with colon on selected device (e.g. :cat:cat:cat:file
			topName.Assign((void *) (&rootName[nextColon]), (UInt8) (rootName.Length() + 1 - nextColon));

		return;
	}

	// Else don't know...
	else
	{
		topUnion = rootUnion;
		topName  = rootName;

		return;
	}
}


// =================================================================================
//		¥ AbleDiskLib_CheckForSoundFIleChange(), AbleDiskLib_ResetSoundFIleChange
// =================================================================================

Boolean	AbleDiskLib_CheckForSoundFIleChange()
{
	return (AbleCatRtns_SoundFileChanged);
}

void	AbleDiskLib_ResetSoundFIleChange()
{
	AbleCatRtns_SoundFileChanged = false;
}


// =================================================================================
//		¥ AbleDiskLib_ReadAbleDisk()
// =================================================================================

int AbleDiskLib_ReadAbleDisk(scsi_device* the_device, uint16 *buffer, uint32 block_num, uint32 block_len)
{
	scsi_error_code status;
	
	if (the_device->fBlockSize == 512)
		status = issue_read_extended(the_device, (byte *) buffer, block_num, block_len);
	
	else if (the_device->fBlockSize == 1024)
	{
		if (block_num & 1) printf("InterChangeX: Odd sector read error from optical disk\n");
		
		status = issue_read_extended(the_device, (byte *) buffer, block_num>>1, (block_len+1)>>1);
	}
	
	else
		status = BAD_STATUS;
		
	if (status)
		printf("InterChangeX: Disk Read Error Occured (%d)\n", status);
	
	return (status);
}


// =================================================================================
//		¥ WriteAbleDisk()
// =================================================================================

int AbleDiskLib_WriteAbleDisk(scsi_device* the_device, uint16 *buffer, uint32 block_num, uint32 block_len)
{
	scsi_error_code status;
	
	if (the_device->fDeviceType == DEVICE_MACINTOSH_DISK)
	{
		printf("InterChangeX: Serious Disk Write Error Occured\n");
		return (-1);
	}
	
	if (the_device->fBlockSize == 512)
		status = issue_write_extended(the_device, (byte *) buffer, block_num, block_len);
	
	else if (the_device->fBlockSize == 1024)
	{
		if (block_num & 1) printf("InterChangeX: Odd sector write error from optical disk\n");

		status = issue_write_extended(the_device, (byte *) buffer, block_num>>1, (block_len+1)>>1);
	}
	
	else
		status = BAD_STATUS;
		
	if (status)
		printf("InterChangeX: Disk Write Error Occured (%d)\n", status);

	return (status);
}


// =================================================================================
//		¥ CopyAbleFile()
// =================================================================================

static int AbleDiskLib_CopyAbleFile(scsi_device* source_device, uint32 source_block,
					    			scsi_device* dest_device,   uint32 dest_block,
									uint32 block_len, Boolean &abortFlag, AbleDiskLibProgressProc progressProc, void* itsObject)
{
	uint16		*the_buf       = NULL;
	handle		the_buf_handle = NULL;
	uint32		where_read     = 0;
    uint32      copy_chunks    = COPY_BUFFER_SIZE;
    
    if (source_device->fDevicePort == D24_SCSI_PORT || dest_device->fDevicePort == D24_SCSI_PORT)
        copy_chunks = D24_BUFFER_SIZE;

	the_buf_handle = get_big_memory(copy_chunks);
	
	if (the_buf_handle)
		the_buf = (uint16 *) *the_buf_handle;
		
	if (!the_buf_handle || !the_buf)
		{printf("InterChangeX: No memory available for buffer\n"); free_big_memory(the_buf_handle); return (-1);}

	while (where_read < block_len)
	{
		uint32	chunk_blocks = block_len - where_read;
		
		if (chunk_blocks > (copy_chunks >> 9))
			chunk_blocks = (copy_chunks >> 9);
		
		if (AbleDiskLib_ReadAbleDisk(source_device, the_buf, source_block + where_read, chunk_blocks))
			{free_big_memory(the_buf_handle); return (-1);}
	
		run_host_environment_250();
					
		if (AbleDiskLib_WriteAbleDisk(dest_device, the_buf, dest_block + where_read, chunk_blocks))
			{free_big_memory(the_buf_handle); return (-1);}
	
		where_read += chunk_blocks;
		
		if (progressProc) progressProc(itsObject, where_read);

		run_host_environment_250();

		if (abortFlag)
			{free_big_memory(the_buf_handle); return (0);}
	}
	
	free_big_memory(the_buf_handle );
	return (0);
}


// =================================================================================
//		¥ AbleDiskLib_FillAbleFile()
// =================================================================================

static int AbleDiskLib_FillAbleFile(scsi_device *the_device, uint32 block_len,  Boolean &abortFlag,
                                    AbleDiskLibProgressProc progressProc, void* itsObject, char fillByte)
{
	uint16		*the_buf       = NULL;
	handle		the_buf_handle = NULL;
	uint32		where_to_write = 0;

    uint32      copy_chunks    = COPY_BUFFER_SIZE;
    
    if (the_device->fDevicePort == D24_SCSI_PORT)
        copy_chunks = D24_BUFFER_SIZE;
    
	the_buf_handle = get_big_memory(copy_chunks);
	
	if (the_buf_handle)
		the_buf = (uint16 *) *the_buf_handle;
		
	if (!the_buf_handle || !the_buf)
		{printf("InterChangeX: No memory available for buffer\n"); free_big_memory(the_buf_handle); return (-1);}

	memset(the_buf, fillByte, copy_chunks);
	
	while (where_to_write < block_len)
	{
		uint32	chunk_blocks = block_len - where_to_write;
		
		if (chunk_blocks > (copy_chunks >> 9))
			chunk_blocks = (copy_chunks >> 9);
		
		run_host_environment_250();
					
		if (AbleDiskLib_WriteAbleDisk(the_device, the_buf, where_to_write, chunk_blocks))
			{free_big_memory(the_buf_handle); return (-1);}
	
		where_to_write += chunk_blocks;
		
		if (progressProc) progressProc(itsObject, where_to_write);

		run_host_environment_250();

		if (abortFlag)
			{free_big_memory(the_buf_handle); return (0);}
	}
	
	free_big_memory(the_buf_handle );
	return (0);
}


// =================================================================================
//		¥ AbleDiskLib_ReadAbleWDDirectory()
// =================================================================================

static int	AbleDiskLib_ReadAbleWDDirectory(InterChangeItemDirectory &itsData, char *the_tree_name)
{
	fixed	dir_word_length = itsData.root.WDItem.file_type == t_subc ? 256 : 1024;
	stCatSemaphore	sem;

	{
		stThrowOnDIskError	error;
		try
		{
			readcat ((fixed) (itsData.root.WDItem.file_block_start >> 16) | (itsData.root.WDItem.file_device_code << 8),
			         (fixed) (itsData.root.WDItem.file_block_start & 0x0000FFFF),
			         (fixed) (dir_word_length),
			         (fixed) ((itsData.root.WDItem.file_size_bytes >> (9+16))),
			         (fixed) ((itsData.root.WDItem.file_size_bytes >> (9   )) & 0x0000FFFF));
		}
		
		catch (...)
		{
			c_status = e_diskerror;
		}
	}
	
	if (c_status != e_none)
	{
		char   	er_mess[MESSAGE_BUF_SIZE];
		get_cat_code_message(c_status, er_mess);
		printf("InterChangeX: Could not read directory for '%s'\n   Error Report:\n", the_tree_name);
		printf("   %s\n", er_mess);
		return (c_status);
	}
	
	itsData.blocks_total = (((UInt32) (UInt16) c_ms_length) << 16) | ((UInt32) (UInt16) c_ls_length);
	itsData.blocks_used  = dir_word_length / 256;
	itsData.num_items    = 0;

	// Digest the entries
	
	uint16 	*cat_buf  = (unsigned short *) &ABLE_CONTEXT._able_memory_[c_bufptr];
	int		num_words = dir_word_length;
	int 	i;
	uint32	start_block;
	uint32	block_len;
	uint32	end_block;
	uint32	word_len;
	uint32	file_type;
	char	file_name[16];
	uint32  num_entries = 0;
	
	// First: count them
	
	i = 0;
	while (i < num_words)
	{
		start_block = ((uint32) cat_buf[i+4]) + ((((uint32) cat_buf[i+7]) & 0xFF00) <<  8);
		block_len   = ((uint32) cat_buf[i+5]) + ((((uint32) cat_buf[i+7]) & 0x00F0) << 12);
		word_len    = ((uint32) cat_buf[i+6]);
		end_block   = start_block + block_len;
		file_type   = (((uint32) cat_buf[i+7]) & 0x000F);
		
		if (block_len >= 256)		/* if more than 256 sectors long, compute actual	*/
		{							/* word length										*/
			if (word_len & 255)
				word_len = (block_len << 8) - (256 - (word_len & 0xFF));
			else
				word_len = (block_len << 8);
		}

		if (cat_buf[i])										/* if name exists			*/
		{
			AbleFileName2CStr(cat_buf+i, file_name);

			// get correct type of file for old non-typed special files
			fixed	temp_file_type	= CheckAbleFileName(cat_buf+i);
			if (temp_file_type)	file_type = temp_file_type;

			num_entries++;
		}
		
		i += 8;
	}
	
	// Then: process them

	if (num_entries)										// allocate output storage
	{
		itsData.void_items = malloc(num_entries * sizeof(AbleWDItem));

		if (itsData.void_items == 0)
			{printf("InterChangeX: Ran out of memory reading directory\n"); return (-1);}

		itsData.item_size = sizeof(AbleWDItem);
	}
	
	i = 0;
	while (i < num_words)
	{
		start_block = ((uint32) cat_buf[i+4]) + ((((uint32) cat_buf[i+7]) & 0xFF00) <<  8);
		block_len   = ((uint32) cat_buf[i+5]) + ((((uint32) cat_buf[i+7]) & 0x00F0) << 12);
		word_len    = ((uint32) cat_buf[i+6]);
		end_block   = start_block + block_len;
		file_type   = (((uint32) cat_buf[i+7]) & 0x000F);
		
		long long itsLength = word_len;
		
		if (block_len >= 256)		/* if more than 256 sectors long, compute actual	*/
		{							/* word length										*/
			if (word_len & 255)
				itsLength = (((long long) block_len) << 8) - (256 - (((long long) word_len) & 0xFF));
			else
				itsLength = (((long long) block_len) << 8);
		}

		if (cat_buf[i])										/* if name exists			*/
		{
			itsData.blocks_used += block_len;
			
			AbleFileName2CStr(cat_buf+i, file_name);
			
			// get correct type of file for old non-typed special files
			fixed	temp_file_type	= CheckAbleFileName(cat_buf+i);
			if (temp_file_type)	file_type = temp_file_type;
				
			if (itsData.wd_items && itsData.num_items < num_entries)
			{
				AbleWDItem &the_entry = itsData.wd_items[itsData.num_items++];						// point to entry
				
				the_entry.file_system       = itsData.root.WDItem.file_system;						// file system code				
				the_entry.file_type         = file_type;											// file type
				the_entry.file_size_bytes   = itsLength << 1;										// file size bytes
				the_entry.file_device_code  = itsData.root.WDItem.file_device_code;					// read data code to use
				the_entry.file_hfs_refnum   = itsData.root.WDItem.file_hfs_refnum;					// ref num therein if on the mac
				the_entry.file_block_start  = itsData.root.WDItem.file_block_start + start_block;	// where on the device
				
				strncpy(the_entry.file_name, file_name, sizeof(the_entry.file_name));				// store file name
			}
		}

		i += 8;
	}
	
	return (c_status);
}	//	end of AbleDiskLib_ReadAbleWDDirectory()


// =================================================================================
//		¥ AbleDiskLib_FillUnionFromWDGlobals()
// =================================================================================

static	void 	AbleDiskLib_FillUnionFromWDGlobals(InterChangeItemUnion& topUnion, AbleWDItem& outWDItem)
{
	outWDItem.file_system      = InterChange_AbleWDFileSystemCode;
	outWDItem.file_type        = f_type;
	outWDItem.file_size_bytes  = ((((long long) (UInt16) (f_ms_length)) << 16) | ((long long) (UInt16) f_ls_length)) << 9;
	outWDItem.file_device_code = ((UInt16) f_ms_sector) >> 8;

	if (topUnion.WDItem.file_device_code == outWDItem.file_device_code)
		outWDItem.file_hfs_refnum = topUnion.WDItem.file_hfs_refnum;
	else
		outWDItem.file_hfs_refnum = 0;
	
	outWDItem.file_block_start = ((((UInt32) (UInt16) (f_ms_sector & 0xFF)) << 16) | ((UInt32) (UInt16) f_ls_sector));

	if (f_words & 0xFF)						// compute correct byte length of file
		outWDItem.file_size_bytes -= ((256 - (f_words & 0xFF)) << 1);

	to_c_string(f_name, outWDItem.file_name);
}


// =================================================================================
//		¥ AbleDiskLib_LocateFileByTreename()
// =================================================================================

int 	AbleDiskLib_LocateFileByTreename(LCStr255 &theTreeName, InterChangeItemUnion &outUnion, int preferedCode, interchange_settings& theSettings)
{
	CInterChangeDeviceButton* itsButton = CInterChangeDeviceButton::LocateRootDevice(theTreeName);
	InterChangeItemUnion	  topUnion;
	LCStr255				  topName;
	LCStr255				  aName(theTreeName);
	int						  status;

	if (!gInterChangeWindow || !gInterChangeWindow->fInterchange2Settings->show_show_ext)
		aName.RemoveFileExtension();
	
	memset(&outUnion, 0, sizeof(outUnion));

	if (!itsButton)
	{
		printf("InterChangeX AbleDiskLib_LocateFileByTreename: Could not locate file '%s' (root device not located)\n", (char *) aName);
		return (-1);
	}
	
	// Check for locate of root device
	if (theTreeName == itsButton->GetDeviceName())
	{
		outUnion = itsButton->GetInterChangeDescriptor();
		return (noErr);
	}
	
	AbleDiskLib_IsolateTopDirectory(itsButton->GetInterChangeDescriptor(), theTreeName, topUnion, topName);

	// Handle locate of a directory itself	
	if (topName.Length() == 0)
	{
		outUnion = topUnion;
		return (0);
	}
	
	// Check on W0:, W1:, etc.
	if (topUnion.Item.file_system == InterChange_AbleWDFileSystemCode)
	{
		stCatSemaphore	    sem;
		stThrowOnDIskError	error;
	 
		AbleDiskLib_LatchHFSRefNum(topUnion);
		
		try
		{
			locate ((void *) (char *) topName, (fixed) topUnion.WDItem.file_device_code, true);
		}
		
		catch (...)
		{
			c_status = e_diskerror;
		}
		
		if (c_status == e_none)
			AbleDiskLib_FillUnionFromWDGlobals(topUnion, outUnion.WDItem);

		else
		{
			char   	er_mess[MESSAGE_BUF_SIZE];
			get_cat_code_message(c_status, er_mess);
			printf("InterChangeX: Could not locate file '%s'\n   Error Report:\n", (char *) aName);
			printf("   %s\n", er_mess);
		}

		AbleDiskLib_ReleaseHFSRefNum(topUnion);

		return (c_status);
	}
		
	// Check on Optical
	if (topUnion.Item.file_system == InterChange_AbleOpticalFileSystemCode)
	{
		AbleDiskLib_LatchHFSRefNum(topUnion);
		
		// Locate an optical category
		if (preferedCode == InterChangeLocateCode_directory)
			status = AbleOptLib_LocateCategory(topUnion, topName, theTreeName, outUnion, theSettings);
		
		else if (preferedCode == InterChangeLocateCode_file)
		{
			c_status = e_no_file;
		}
		
		else
			c_status = e_no_dir;
			
		if (c_status == e_none)
		{
		
		
		}
		
		else
		{
			char   	er_mess[MESSAGE_BUF_SIZE];
			get_cat_code_message(c_status, er_mess);
			printf("InterChangeX: Could not locate file '%s'\n   Error Report:\n", (char *) aName);
			printf("   %s\n", er_mess);
		}

		AbleDiskLib_ReleaseHFSRefNum(topUnion);

		return (c_status);
	}
		
	printf("InterChangeX: Could not locate file '%s'\n", (char *) aName);
	return (-1);
}


// =================================================================================
//		¥ AbleDiskLib_ReadDirectory()
// =================================================================================

int	AbleDiskLib_ReadDirectory(InterChangeItemUnion &inUnion, LCStr255 &theTreeName, InterChangeItemDirectory &outData)
{
	LCStr255				  aName(theTreeName);

	if (!gInterChangeWindow || !gInterChangeWindow->fInterchange2Settings->show_show_ext)
		aName.RemoveFileExtension();

	memset(&outData, 0, sizeof(outData));
	
	outData.root = inUnion;
	
	switch (inUnion.Item.file_system)
	{
		case InterChange_AbleWDFileSystemCode:
			return (AbleDiskLib_ReadAbleWDDirectory(outData, theTreeName));
		
		case InterChange_MacHFSFIleSystemCode:
			return (MacHFSLib_ReadAbleHFSDirectory (outData, theTreeName));
			
		case InterChange_AbleOpticalFileSystemCode:
			return (AbleOptLib_ReadOpticalDirectory(outData, (const char*) theTreeName));
	}
	
	printf("InterChangeX: Could not read directory '%s'\n", (char *) aName);
	return (-1);
}


// =================================================================================
//		¥ AbleDiskLib_FetchDevicePointer()
// =================================================================================

struct scsi_device*	AbleDiskLib_FetchDevicePointer(InterChangeItemUnion &theUnion)
{
	switch (theUnion.Item.file_system)
	{
		case InterChange_AbleWDFileSystemCode:
			return (access_scsi_device(theUnion.WDItem.file_device_code));
	
		case InterChange_AbleOpticalFileSystemCode:
			return (access_scsi_device(theUnion.OpticalItem.file_device_code));
			
		case InterChange_MacHFSFIleSystemCode:
			if (theUnion.HFSItem.file_type == t_lsubc || theUnion.HFSItem.file_type == InterChange_AbleOpticalDirectory)
			{
				ufixed	itsDeviceCode = XPLRunTime_LookForFSSpecInUse(&theUnion.HFSItem.file_spec);
				
				if (itsDeviceCode)
					return (access_scsi_device(itsDeviceCode));
			}
			break;
	}
	
	return (NULL);
}


// =================================================================================
//		¥ AbleDiskLib_LatchHFSRefNum()
// =================================================================================

// If theUnion is a mac-resident image file, create a device for it's use and keep it open

void	AbleDiskLib_LatchHFSRefNum(InterChangeItemUnion &theUnion)
{
	if ((theUnion.Item.file_system == InterChange_AbleWDFileSystemCode)
	&&  (theUnion.WDItem.file_hfs_refnum))
		theUnion.WDItem.file_device_code = CHFSImageFile::LatchDevCode(theUnion.WDItem.file_hfs_refnum);
		
	else if ((theUnion.Item.file_system == InterChange_AbleOpticalFileSystemCode)
	&&       (theUnion.OpticalItem.file_hfs_refnum))
		theUnion.OpticalItem.file_device_code = CHFSImageFile::LatchDevCode(theUnion.OpticalItem.file_hfs_refnum);
}


// =================================================================================
//		¥ AbleDiskLib_ReleaseHFSRefNum()
// =================================================================================

// If theUnion is a mac-resident image file, release a latch useage

void	AbleDiskLib_ReleaseHFSRefNum(InterChangeItemUnion &theUnion)
{
	if ((theUnion.WDItem.file_system     == InterChange_AbleWDFileSystemCode)
	&&  (theUnion.WDItem.file_hfs_refnum != 0								  ))
		CHFSImageFile::ReleaseDevCode(theUnion.WDItem.file_hfs_refnum);
		
	else if ((theUnion.OpticalItem.file_system     == InterChange_AbleOpticalFileSystemCode)
	&&       (theUnion.OpticalItem.file_hfs_refnum != 0								  ))
		CHFSImageFile::ReleaseDevCode(theUnion.OpticalItem.file_hfs_refnum);
}


// =================================================================================
//		¥ AbleDiskLib_FetchDeviceCode()
// =================================================================================

short	AbleDiskLib_FetchDeviceCode(const InterChangeItemUnion &theUnion)
{
	switch (theUnion.Item.file_system)
	{
		case InterChange_AbleWDFileSystemCode:
			return (theUnion.WDItem.file_device_code);
	
		case InterChange_AbleOpticalFileSystemCode:
			return (theUnion.OpticalItem.file_device_code);

		case InterChange_MacHFSFIleSystemCode:
			if (theUnion.HFSItem.file_type == t_lsubc || theUnion.HFSItem.file_type == InterChange_AbleOpticalDirectory)
			{
				ufixed	itsDeviceCode = XPLRunTime_LookForFSSpecInUse((const xpl_file_ref) &theUnion.HFSItem.file_spec);
				
				if (itsDeviceCode)
					return (itsDeviceCode);
			}
			break;
	}
	
	return (-1);
}


// =================================================================================
//		¥ AbleDiskLib_InterrogateDevice()
// =================================================================================

void	AbleDiskLib_InterrogateDevice(InterChangeItemUnion &theUnion)
{
	StFastMutex	mutex(gXPLMutex);

	switch (theUnion.Item.file_system)
	{
		case InterChange_AbleWDFileSystemCode:
		{
			scsi_device *itsDevice = access_scsi_device(theUnion.WDItem.file_device_code);
			
			if (itsDevice)
			{
				memset(&itsDevice->fStandardInquiryData, 0, sizeof(itsDevice->fStandardInquiryData));
				
				interrogate_device(itsDevice);
		
				itsDevice->fUnitAttention = FALSE;
				
				update_scsi_device_size(theUnion.WDItem.file_device_code);
			}
			
			break;
		}

		case InterChange_AbleOpticalFileSystemCode:
		{
			scsi_device *itsDevice = access_scsi_device(theUnion.OpticalItem.file_device_code);
			
			if (itsDevice)
			{
				memset(&itsDevice->fStandardInquiryData, 0, sizeof(itsDevice->fStandardInquiryData));

				interrogate_device(itsDevice);
		
				itsDevice->fUnitAttention = FALSE;
				
				update_scsi_device_size(theUnion.OpticalItem.file_device_code);
			}
			
			break;
		}
	}
}


// =================================================================================
//		¥ AbleDiskLib_RenameFileByTreename()
// =================================================================================

int 	AbleDiskLib_RenameFileByTreename(LCStr255 &theTreeName, LCStr255 &theNewName, interchange_settings& theSettings)
{
	CInterChangeDeviceButton* itsButton = CInterChangeDeviceButton::LocateRootDevice(theTreeName);
	InterChangeItemUnion	  topUnion;
	LCStr255				  topName;
	LCStr255				  aName(theTreeName);

	if (!gInterChangeWindow || !gInterChangeWindow->fInterchange2Settings->show_show_ext)
		aName.RemoveFileExtension();
	
	if (!itsButton)
	{
		printf("InterChangeX AbleDiskLib_RenameFileByTreename: Could not locate file '%s' (root device not located)\n", (char *) aName);
		return (-1);
	}
	
	AbleDiskLib_IsolateTopDirectory(itsButton->GetInterChangeDescriptor(), theTreeName, topUnion, topName);

	// Can't rename a directory itself with this routine
	if (topName.Length() == 0)
	{
		printf("InterChangeX: Could not rename null file '%s'\n", (char *) aName);
		return (-1);
	}
	
	// Check on W0:, W1:, etc.
	if (topUnion.Item.file_system == InterChange_AbleWDFileSystemCode)
	{
		stCatSemaphore	    sem;
		stThrowOnDIskError	error;

		AbleDiskLib_LatchHFSRefNum(topUnion);

		try
		{
			rename_able_file ((void *) (char *) topName, (void *) (char *) theNewName, (fixed) topUnion.WDItem.file_device_code, true);
		}
		
		catch (...)
		{
			c_status = e_diskerror;
		}
	
		if (c_status != e_none)
		{
			get_cat_code_message(c_status, AbleDiskLib_RecentErrorMessage);
			printf("InterChangeX: Could not rename file '%s'\n   Error Report:\n", (char *) aName);
			printf("   %s\n", AbleDiskLib_RecentErrorMessage);
		}
		else
			AbleDiskLib_RecentErrorMessage[0] = 0;

		AbleDiskLib_ReleaseHFSRefNum(topUnion);

		return (c_status);
	}
	
	// Check on Optical
	if (topUnion.Item.file_system == InterChange_AbleOpticalFileSystemCode)
	{
		int status;
		
		AbleDiskLib_LatchHFSRefNum(topUnion);
		
		status = AbleOptLib_RenameFile(topUnion, topName, theTreeName, theNewName, theSettings);
		
		AbleDiskLib_ReleaseHFSRefNum(topUnion);

		return (status);
	}

	printf("InterChangeX: Could not rename file '%s'\n", (char *) aName);
	AbleDiskLib_RecentErrorMessage[0] = 0;

	return (-1);
}


// =================================================================================
//		¥ AbleDiskLib_FindLargestHole()
// =================================================================================

int 	AbleDiskLib_FindLargestHole(LCStr255 &theTreeName, uint32 &theHole)
{
	CInterChangeDeviceButton* itsButton = CInterChangeDeviceButton::LocateRootDevice(theTreeName);
	InterChangeItemUnion	  topUnion;
	LCStr255				  topName;
	LCStr255				  aName(theTreeName);

	if (!gInterChangeWindow || !gInterChangeWindow->fInterchange2Settings->show_show_ext)
		aName.RemoveFileExtension();

	theHole = 0;
	
	if (!itsButton)
	{
		printf("InterChangeX AbleDiskLib_FindLargestHole: Could not locate file '%s' (root device not located)\n", (char *) aName);
		return (-1);
	}
	
	AbleDiskLib_IsolateTopDirectory(itsButton->GetInterChangeDescriptor(), theTreeName, topUnion, topName);

	// Check on W0:, W1:, etc.
	if (topUnion.Item.file_system == InterChange_AbleWDFileSystemCode)
	{
		stCatSemaphore	    sem;
		stThrowOnDIskError	error;

		AbleDiskLib_LatchHFSRefNum(topUnion);

		try
		{
			lookmax ((void *) (char *) topName, (fixed) topUnion.WDItem.file_device_code, true);
		}
		
		catch (...)
		{
			c_status = e_diskerror;
		}

		if (c_status != e_none)
		{
			get_cat_code_message(c_status, AbleDiskLib_RecentErrorMessage);
			printf("InterChangeX: Could not create a subcatalog in '%s'\n   Error Report:\n", (char *) aName);
			printf("   %s\n", AbleDiskLib_RecentErrorMessage);
		}
		else
		{
			theHole = (((UInt32) (UInt16) (f_ms_length)) << 16) | ((UInt32) (UInt16) f_ls_length);
			AbleDiskLib_RecentErrorMessage[0] = 0;
		}

		AbleDiskLib_ReleaseHFSRefNum(topUnion);
		
		return (c_status);
	}
		
	printf("InterChangeX: Could not create subcatalog in '%s'\n", (char *) aName);
	AbleDiskLib_RecentErrorMessage[0] = 0;

	return (-1);
}


// =================================================================================
//		¥ AbleDiskLib_UnsaveFileByTreename()
// =================================================================================

int 	AbleDiskLib_UnsaveFileByTreename(LCStr255 &theTreeName, interchange_settings& theSettings)
{
	CInterChangeDeviceButton* itsButton = CInterChangeDeviceButton::LocateRootDevice(theTreeName);
	InterChangeItemUnion	  topUnion;
	LCStr255				  topName;
	LCStr255				  aName(theTreeName);

	if (!gInterChangeWindow || !gInterChangeWindow->fInterchange2Settings->show_show_ext)
		aName.RemoveFileExtension();
	
	if (!itsButton)
	{
		printf("InterChangeX AbleDiskLib_UnsaveFileByTreename: Could not locate file '%s' (root device not located)\n", (char *) aName);
		return (-1);
	}
	
	AbleDiskLib_IsolateTopDirectory(itsButton->GetInterChangeDescriptor(), theTreeName, topUnion, topName);

	// Can't unsave a directory itself with this routine
	if (topName.Length() == 0)
	{
		printf("InterChangeX: Could not unsave null file '%s'\n", (char *) aName);
		return (-1);
	}
	
	// Check on W0:, W1:, etc.
	if (topUnion.Item.file_system == InterChange_AbleWDFileSystemCode)
	{
		stCatSemaphore	    sem;
		stThrowOnDIskError	error;

		AbleDiskLib_LatchHFSRefNum(topUnion);

		try
		{
			XPLdelete ((void *) (char *) topName, (fixed) topUnion.WDItem.file_device_code, true);
		}
		
		catch (...)
		{
			c_status = e_diskerror;
		}
	
		if (c_status != e_none)
		{
			char   	er_mess[MESSAGE_BUF_SIZE];
			get_cat_code_message(c_status, er_mess);
			printf("InterChangeX: Could not unsave file '%s'\n   Error Report:\n", (char *) aName);
			printf("   %s\n", er_mess);
		}

		AbleDiskLib_ReleaseHFSRefNum(topUnion);

		return (c_status);
	}
	
	// Check on Optical
	if (topUnion.Item.file_system == InterChange_AbleOpticalFileSystemCode)
	{
		int status;
		
		AbleDiskLib_LatchHFSRefNum(topUnion);
		
		status = AbleOptLib_UnsaveFile(topUnion, topName, theTreeName, theSettings);
		
		AbleDiskLib_ReleaseHFSRefNum(topUnion);

		return (status);
	}

	printf("InterChangeX: Could not unsave file '%s'\n", (char *) aName);
	return (-1);
}


// =================================================================================
//		¥ AbleDiskLib_CheckDuplicateByTreename()
// =================================================================================

int 	AbleDiskLib_CheckDuplicateByTreename(LCStr255 &theTreeName)
{
	CInterChangeDeviceButton* itsButton = CInterChangeDeviceButton::LocateRootDevice(theTreeName);
	InterChangeItemUnion	  topUnion;
	LCStr255				  topName;
	LCStr255				  aName(theTreeName);

	if (!gInterChangeWindow || !gInterChangeWindow->fInterchange2Settings->show_show_ext)
		aName.RemoveFileExtension();
	
	if (!itsButton)
	{
		printf("InterChangeX AbleDiskLib_CheckDuplicateByTreename: Could not locate file '%s' (root device not located)\n", (char *) aName);
		return (-1);
	}
	
	AbleDiskLib_IsolateTopDirectory(itsButton->GetInterChangeDescriptor(), theTreeName, topUnion, topName);

	// Check for locate of root device
	if (theTreeName == itsButton->GetDeviceName())
		return (noErr);
	
	// Top device exists
	if (topName.Length() == 0)
		return (noErr);
	
	// Check on W0:, W1:, etc.
	if (topUnion.Item.file_system == InterChange_AbleWDFileSystemCode)
	{
		stCatSemaphore	    sem;
		stThrowOnDIskError	error;

		AbleDiskLib_LatchHFSRefNum(topUnion);

		try
		{
			locate ((void *) (char *) topName, (fixed) topUnion.WDItem.file_device_code, true);
		}
		
		catch (...)
		{
			c_status = e_diskerror;
		}
		
		if (c_status != e_none && c_status != e_no_file)
		{
			char   	er_mess[MESSAGE_BUF_SIZE];
			get_cat_code_message(c_status, er_mess);
			printf("InterChangeX: Could not search for file '%s'\n   Error Report:\n", (char *) aName);
			printf("   %s\n", er_mess);
		}

		AbleDiskLib_ReleaseHFSRefNum(topUnion);

		return (c_status);
	}
	
	printf("InterChangeX: Could not search for file '%s'\n", (char *) aName);
	return (-1);
}


// =================================================================================
//		¥ AbleDiskLib_ReadDropDirectory(), AbleDiskLib_CacheDropDirectory()
// =================================================================================

// Read a directory into memory temporarily so we can see if there is room for a list
// of files
int 	AbleDiskLib_ReadDropDirectory(LCStr255 &theTreeName, int& outBlockStart)
{
	CInterChangeDeviceButton* itsButton = CInterChangeDeviceButton::LocateRootDevice(theTreeName);
	InterChangeItemUnion	  topUnion;
	LCStr255				  topName;
	LCStr255				  aName(theTreeName);

	if (!gInterChangeWindow || !gInterChangeWindow->fInterchange2Settings->show_show_ext)
		aName.RemoveFileExtension();

	outBlockStart = 0;
	
	if (!itsButton)
	{
		printf("InterChangeX AbleDiskLib_ReadDropDirectory: Could not locate file '%s' (root device not located)\n", (char *) aName);
		return (-1);
	}
	
	AbleDiskLib_IsolateTopDirectory(itsButton->GetInterChangeDescriptor(), theTreeName, topUnion, topName);

	// Check on W0:, W1:, etc.
	if (topUnion.Item.file_system == InterChange_AbleWDFileSystemCode)
	{
		stCatSemaphore	    sem;
		stThrowOnDIskError	error;

		AbleDiskLib_LatchHFSRefNum(topUnion);

		try
		{
			AbleCatRtns_GetCatalog((void *) (char *) topName, (fixed) topUnion.WDItem.file_device_code, true);
		}
		
		catch (...)
		{
			c_status = e_diskerror;
		}

		if (c_status != e_none)
		{
			char   	er_mess[MESSAGE_BUF_SIZE];
			get_cat_code_message(c_status, er_mess);
			printf("InterChangeX: Could not read directory '%s'\n   Error Report:\n", (char *) aName);
			printf("   %s\n", er_mess);
		}

		else
			outBlockStart = ((((UInt32) (UInt16) (c_ms_sector & 0xFF)) << 16) | ((UInt32) (UInt16) c_ls_sector));
		
		AbleDiskLib_ReleaseHFSRefNum(topUnion);

		return (c_status);
	}

	printf("InterChangeX: Could not read directory '%s'\n", (char *) aName);
	return (-1);
}

int 	AbleDiskLib_CacheDropDirectory(LCStr255 &theTreeName, int& outBlockStart)
{
	stCatSemaphore  sem;
	stCatBuf 		saver;
	
	return (AbleDiskLib_ReadDropDirectory(theTreeName, outBlockStart));
}


// =================================================================================
//		¥ AbleDiskLib_AddEntityToDirectory(), AbleDiskLib_AddEntityToCachedDirectory()
// =================================================================================

// Add an item to a chached directory.  Mostly done to see if it will fit.
int 	AbleDiskLib_AddEntityToDirectory(LCStr255 &theEntityName, unsigned short file_type, long long file_size_bytes)
{
	stCatSemaphore      	  sem;
	stThrowOnDIskError		  error;
	unsigned int              file_size_blocks = ((file_size_bytes) + 511) >> 9;
	fixed	            	  changed = AbleCatRtns_SoundFileChanged;
	
	f_type      = file_type;
	f_ms_length = file_size_blocks >> 16;
	f_ls_length = file_size_blocks & 0x0000FFFF;
	f_words     = ((file_size_bytes + 1) >> 1) & 0x0000FFFF;
	
	AbleCatRtns_AllowTypeReplace = true;
	
	try
	{
		AbleCatRtns_AddFile((void *) (char *) theEntityName, f_type, f_ms_length, f_ls_length, f_words, true);
	}
	
	catch (...)
	{
		c_status = e_diskerror;
	}
	
	AbleCatRtns_AllowTypeReplace = false;
	AbleCatRtns_SoundFileChanged = changed;

	// Let higher level code handle these errors explicitly
	if (c_status == e_storage)
		return (e_storage);
		
	if (c_status == e_cstorage)
		return (e_cstorage);
		
	if (c_status == e_dir_full)
		return (e_dir_full);
		
	if (c_status != e_none)
	{
		char   	er_mess[MESSAGE_BUF_SIZE];
		get_cat_code_message(c_status, er_mess);
		printf("InterChangeX: Could not copy '%s'\n   Error Report:\n", (char *) theEntityName);
		printf("   %s\n", er_mess);
		return (c_status);
	}
	
	return (c_status);
}

int 	AbleDiskLib_AddEntityToCachedDirectory(LCStr255 &theEntityName, unsigned short file_type, long long file_size_bytes)
{
	stCatSemaphore  sem;
	stCatBuf 		saver;
	
	return (AbleDiskLib_AddEntityToDirectory(theEntityName, file_type, file_size_bytes));
}

// =================================================================================
//		¥ AbleDiskLib_FindFileInCachedDirectory()
// =================================================================================

// Sees if a name is used in a cached directory
int 	AbleDiskLib_FindFileInCachedDirectory(LCStr255 &theEntityName)
{
	stCatSemaphore  		  sem;
	stCatBuf 				  saver;

	try
	{
		AbleCatRtns_FindFile((void *) (char *) theEntityName, true);
	}
	
	catch (...)
	{
		c_status = e_diskerror;
	}

	// Let higher level code handle these errors explicitly
	if (c_status == e_no_file)
		return (e_no_file);
		
	if (c_status != e_none)
	{
		char   	er_mess[MESSAGE_BUF_SIZE];
		get_cat_code_message(c_status, er_mess);
		printf("InterChangeX: Could not copy '%s'\n   Error Report:\n", (char *) theEntityName);
		printf("   %s\n", er_mess);
		return (c_status);
	}
	
	return (c_status);
}


// =================================================================================
//		¥ AbleDiskLib_CloneFile()
// =================================================================================

// Clones a file from a source and creates a copy in a destination

int		AbleDiskLib_CloneFile (InterChangeItemUnion &theSource, LCStr255 &theDestinationTree, InterChangeItemUnion &theResult, Boolean &abortFlag,
      						   AbleDiskLibProgressProc progressProc, void* itsObject, UInt32 resizeSectors,  interchange_settings& theSettings )
{
	CInterChangeDeviceButton* itsButton = CInterChangeDeviceButton::LocateRootDevice(theDestinationTree);
	InterChangeItemUnion	  topUnion;
	LCStr255				  topName;
	
	// Init return value
	memset(&theResult, 0, sizeof(theResult));

	if (!itsButton)
	{
		printf("InterChangeX AbleDiskLib_CloneFile: Could not locate file '%s' (root device not located)\n", (char *) theDestinationTree);
		return (-1);
	}
	
	AbleDiskLib_IsolateTopDirectory(itsButton->GetInterChangeDescriptor(), theDestinationTree, topUnion, topName);

	// Can't clone a directory itself with this routine
	if (topName.Length() == 0)
	{
		printf("InterChangeX: Could not clone null file '%s'\n", (char *) theDestinationTree);
		return (-1);
	}
	
	// Copy able-to-able (any file on WD; Sound files only on Opticals; e.g. don't handle optical categories at this level
	if (((topUnion.Item.file_system  == InterChange_AbleWDFileSystemCode))
	&&  ((theSource.Item.file_system == InterChange_AbleWDFileSystemCode)
	||   ((theSource.Item.file_system == InterChange_AbleOpticalFileSystemCode) && (theSource.Item.file_type == t_sound))))
	{
		unsigned int    	file_size_blocks = 0;
		unsigned short   	file_type        = 0;
		fixed				word_length      = 0;
		uint32 				source_block     = 0;

		if (theSource.Item.file_system  == InterChange_AbleWDFileSystemCode)
		{
			file_size_blocks = ((theSource.WDItem.file_size_bytes) + 511) >> 9;
			file_type        = theSource.WDItem.file_type;
			word_length      = (((resizeSectors << 9) + theSource.WDItem.file_size_bytes + 1) >> 1) & 0x0000FFFF;
			source_block     = theSource.WDItem.file_block_start;
		}
		else
		{
			file_size_blocks = ((theSource.OpticalItem.file_size_bytes) + 511) >> 9;
			file_type        = theSource.OpticalItem.file_type;
			word_length      = (((resizeSectors << 9) + theSource.OpticalItem.file_size_bytes + 1) >> 1) & 0x0000FFFF;
			source_block     = theSource.OpticalItem.file_block_start;
		}
		
		scsi_device*		source_device    = NULL;
		scsi_device*		dest_device      = NULL;
		fixed				msb_length       = (file_size_blocks + resizeSectors) >> 16;
		fixed				lsb_length       = (file_size_blocks + resizeSectors) & 0x0000FFFF;
		uint32 				avail_len 		 = 0;
		uint32 				dest_block       = 0;
		
		// Latch possible device codes
		AbleDiskLib_LatchHFSRefNum(topUnion);
		AbleDiskLib_LatchHFSRefNum(theSource);

		// Make sure the source file is available before we create the catalog entry.
		if (theSource.Item.file_system  == InterChange_AbleWDFileSystemCode)
			source_device = access_scsi_device(theSource.WDItem.file_device_code);
		else
			source_device = access_scsi_device(theSource.OpticalItem.file_device_code);
		
		if (!source_device)
		{
			printf("InterChangeX: Could not create file '%s'\n   Error Report:\n", (char *) theDestinationTree);
			printf("   Original FIle Could Not Be Found\n");
			AbleDiskLib_ReleaseHFSRefNum(theSource);
			AbleDiskLib_ReleaseHFSRefNum(topUnion);
			return (-1);											// device should have been configured before we got here...
		}
		
		if (abortFlag)
		{
			AbleDiskLib_ReleaseHFSRefNum(theSource);
			AbleDiskLib_ReleaseHFSRefNum(topUnion);
			return(0);
		}

		// Create the destination file on disk		
		{
			stCatSemaphore	    sem;
			stThrowOnDIskError	error;
			
			AbleCatRtns_AllowTypeReplace = true;
			
			try
			{
				replace((void *) (char *) topName, file_type, msb_length, lsb_length, word_length, (fixed) topUnion.WDItem.file_device_code, true);
			}
		
			catch (...)
			{
				c_status = e_diskerror;
			}

			AbleCatRtns_AllowTypeReplace = false;

			if (c_status != e_none)
			{
				char   	er_mess[MESSAGE_BUF_SIZE];
				get_cat_code_message(c_status, er_mess);
				printf("InterChangeX: Could not create file '%s'\n   Error Report:\n", (char *) theDestinationTree);
				printf("   %s\n", er_mess);
				AbleDiskLib_ReleaseHFSRefNum(theSource);
				AbleDiskLib_ReleaseHFSRefNum(topUnion);
				return (c_status);
			}

			dest_device = access_scsi_device(f_ms_sector >> 8);
			
			// Consistency check
			avail_len 	 = (((UInt32) (UInt16) (f_ms_length)) << 16) | ((UInt32) (UInt16) f_ls_length);
			dest_block   = (((UInt32) (UInt16) (f_ms_sector & 0xFF)) << 16) | ((UInt32) (UInt16) f_ls_sector);
			
			AbleDiskLib_FillUnionFromWDGlobals(topUnion, theResult.WDItem);

			if ((!source_device)
			||  (!dest_device  )
			||  (avail_len < file_size_blocks)
			||  (f_type != file_type)
			||  (dest_block < 4))
			{
				try { XPLdelete((void *) (char *) topName, (fixed) topUnion.WDItem.file_device_code, true); }
				catch (...) {}

				printf("InterChangeX: Could not create file '%s'\n   Error Report:\n", (char *) theDestinationTree);
				printf("   System Catalog Error\n");
				AbleDiskLib_ReleaseHFSRefNum(theSource);
				AbleDiskLib_ReleaseHFSRefNum(topUnion);
				return (-1);											// device should have been configured before we got here...
			}
		}
		
		if (AbleDiskLib_CopyAbleFile(source_device, source_block, dest_device, dest_block, file_size_blocks, abortFlag, progressProc, itsObject))
		{
			stCatSemaphore	    sem;
			stThrowOnDIskError	error;
			try { XPLdelete((void *) (char *) topName, (fixed) topUnion.WDItem.file_device_code, true); }
			catch (...) {}
			AbleDiskLib_ReleaseHFSRefNum(theSource);
			AbleDiskLib_ReleaseHFSRefNum(topUnion);
			return (-1);
		}

		if (abortFlag)
		{
			stCatSemaphore	    sem;
			stThrowOnDIskError	error;
			try { XPLdelete((void *) (char *) topName, (fixed) topUnion.WDItem.file_device_code, true); }
			catch (...) {}
			AbleDiskLib_ReleaseHFSRefNum(theSource);
			AbleDiskLib_ReleaseHFSRefNum(topUnion);
			return (-1);
		}

		// Copy optical header from latest entry
		if ((theSource.Item.file_system  == InterChange_AbleOpticalFileSystemCode)
		&&  (theSource.Item.file_type    == t_sound)
		&&  (file_size_blocks            >= 2      ))
			AbleOptLib_UpdateSFHeaderFromEntry(theSource, source_device, dest_device, dest_block, theSettings);
		
		AbleDiskLib_ReleaseHFSRefNum(theSource);
		AbleDiskLib_ReleaseHFSRefNum(topUnion);
			
		return (0);
	}
	
	printf("InterChangeX: Could not create file '%s'\n", (char *) theDestinationTree);
	return (-1);
}


// =================================================================================
//		¥ AbleDiskLib_CreateSubcat()
// =================================================================================

// Creates a subcatalog

int		AbleDiskLib_CreateSubcat (LCStr255 &theSubcatTreeName, InterChangeItemUnion &theResult, uint32 sectorSize)
{
	CInterChangeDeviceButton* itsButton = CInterChangeDeviceButton::LocateRootDevice(theSubcatTreeName);
	InterChangeItemUnion	  topUnion;
	LCStr255				  topName;
	uint16					  buf[1024];
	
	// Init return value
	memset(buf,        0, 2048);
	memset(&theResult, 0, sizeof(theResult));

	if (!itsButton)
	{
		printf("InterChangeX AbleDiskLib_CreateSubcat: Could not locate file '%s' (root device not located)\n", (char *) theSubcatTreeName);
		return (-1);
	}
	
	AbleDiskLib_IsolateTopDirectory(itsButton->GetInterChangeDescriptor(), theSubcatTreeName, topUnion, topName);

	// Can't rename a directory itself with this routine
	if (topName.Length() == 0)
	{
		printf("InterChangeX: Could not create null subcatalog '%s'\n", (char *) theSubcatTreeName);
		return (-1);
	}
	
	// Check on W0:, W1:, etc.
	if (topUnion.Item.file_system == InterChange_AbleWDFileSystemCode)
	{
		stCatSemaphore	    sem;
		stThrowOnDIskError	error;

		sectorSize += 4;
		
		AbleDiskLib_LatchHFSRefNum(topUnion);

		AbleCatRtns_AllowTypeReplace = true;
		
		try
		{
			replace((void *) (char *) topName, t_lsubc, (uint16) (sectorSize >> 16), (uint16) (sectorSize & 0xFFFF), (uint16) (sectorSize << 8), (fixed) topUnion.WDItem.file_device_code, true);
		}
	
		catch (...)
		{
			c_status = e_diskerror;
		}

		AbleCatRtns_AllowTypeReplace = false;

		if (c_status != e_none)
		{
			get_cat_code_message(c_status, AbleDiskLib_RecentErrorMessage);
			printf("InterChangeX: Could not create subcatalog '%s'\n   Error Report:\n", (char *) theSubcatTreeName);
			printf("   %s\n", AbleDiskLib_RecentErrorMessage);
			AbleDiskLib_ReleaseHFSRefNum(topUnion);
			return (c_status);
		}
		
		// Zero out cat buf
		scsi_device* dest_device = access_scsi_device(f_ms_sector >> 8);
		
		// Consistency check
		try
		{
			uint32 avail_len  = (((UInt32) (UInt16) (f_ms_length)) << 16) | ((UInt32) (UInt16) f_ls_length);
			uint32 dest_block = (((UInt32) (UInt16) (f_ms_sector & 0xFF)) << 16) | ((UInt32) (UInt16) f_ls_sector);
			
			if ((!dest_device  )
			||  (avail_len < 4)
			||  (f_type != t_lsubc)
			||  (dest_block < 4))
			{
				XPLdelete((void *) (char *) topName, (fixed) topUnion.WDItem.file_device_code, true);
				printf("InterChangeX: Could not create subcatalog '%s'\n   Error Report:\n", (char *) theSubcatTreeName);
				printf("   System Catalog Error\n");
				AbleDiskLib_RecentErrorMessage[0] = 0;
				AbleDiskLib_ReleaseHFSRefNum(topUnion);
				return (-1);											// device should have been configured before we got here...
			}
			
			if (AbleDiskLib_WriteAbleDisk(dest_device, buf, dest_block, 4))
			{
				XPLdelete((void *) (char *) topName, (fixed) topUnion.WDItem.file_device_code, true);
				printf("InterChangeX: Could not initialize subcatalog '%s'\n   Error Report:\n", (char *) theSubcatTreeName);
				printf("   System Catalog Error\n");
				AbleDiskLib_RecentErrorMessage[0] = 0;
				AbleDiskLib_ReleaseHFSRefNum(topUnion);
				return (-1);											// device should have been configured before we got here...
			}

			// Fill out destination info if subcat create succeeded
			AbleDiskLib_FillUnionFromWDGlobals(topUnion, theResult.WDItem);
			AbleDiskLib_RecentErrorMessage[0] = 0;
			AbleDiskLib_ReleaseHFSRefNum(topUnion);
			return (0);
		}
		
		catch (...)		// most likely would catch here if XPLdelete failed...
			{}

		AbleDiskLib_ReleaseHFSRefNum(topUnion);
	}
	
	printf("InterChangeX: Could not create subcatalog '%s'\n", (char *) theSubcatTreeName);
	AbleDiskLib_RecentErrorMessage[0] = 0;

	return (-1);
}


// =================================================================================
//		¥ AbleDiskLib_ConstructDeviceDescription(LCStr255 outDesc)
// =================================================================================

// Construct a description of the device for the info area.  Returns type of mac
// file if is an HFS item. Returns name including possible file name extension.

int	AbleDiskLib_ConstructDeviceDescription(InterChangeItemUnion &ofWhat, LCStr255& outDesc, interchange_settings& settings)
{
	short			theFileSystemCode = ofWhat.Item.file_system;
	char			itsName[512]      = {""};
	short			itsCode           = 0;
	scsi_settings*  itsSetting        = NULL;
	
	switch (theFileSystemCode)
	{
		case InterChange_MacHFSFIleSystemCode:
			AliasUtilities_GetFullPath(&ofWhat.HFSItem.file_spec, itsName, 512);
			outDesc = itsName;
			return (ofWhat.HFSItem.file_type);

		case InterChange_AbleWDFileSystemCode:
		case InterChange_AbleOpticalFileSystemCode:
			itsCode = AbleDiskLib_FetchDeviceCode(ofWhat);
			
			if (itsCode < ABLE_HFS_READDATA_CODE)
				itsSetting = InterChangeLibGetSettingForCode(&settings, itsCode);
			else
				itsSetting = access_scsi_setting(itsCode);			// Note: Setting will not be available for 'none' devices or syncl scsi devices if no PCI1

			if (!itsSetting)										// no setting; is likely an image file
			{
				FSSpec *itsSpec = NULL;
				
				// See if is added i mage file
				if ((itsSpec = find_hfs_scsi_spec(itsCode)) != NULL)
				{
					AliasUtilities_GetFullPath(itsSpec, itsName, 512);
					outDesc = itsName;
					return (t_lsubc);
				}
				
				outDesc = "";
				return (0);
			}

			switch (itsSetting->bus_id)
			{
				case	SCSI_BUS_MENU_MAC_SCSI_PORT:
				{
					BSD_Block_Device& theDisk = bsd_simulated_disks[itsSetting->scsi_id];
					
					if (theDisk.entry_valid)
					{
						LCStr255	itsName((const char *) theDisk.device_info);
						
						if (itsName.EndsWith("\p Media"))
							itsName.Remove(itsName.Length() - 5, 6);
						
						outDesc = "Macintosh SCSI Bus: ";
						outDesc += itsName;
					}
					
					else
					{
						outDesc = "Macintosh SCSI Bus, ID ";
						outDesc += (SInt32) itsSetting->scsi_id;
					}
					
					return (0);
				}
					
				case	SCSI_BUS_MENU_DISK_IMAGE:
				{
					outDesc = itsSetting->image_pathname;
					return (t_lsubc);
				}
					
				case	SCSI_BUS_MENU_D24:
				{
					outDesc = "Synclavier¨ SCSI Bus, ID ";
					outDesc += (SInt32) itsSetting->scsi_id;
					return (0);
				}
					
				case	SCSI_BUS_MENU_NONE:
				default:
				{
					outDesc = "None";
					return (0);
				}
			}
			return (0);
			
		default:
			outDesc = "";
			return (0);
	}		
}


// =================================================================================
//		¥ AbleDiskLib_FetchReleventFSSpec
// =================================================================================

int	AbleDiskLib_FetchReleventFSSpec(const InterChangeItemUnion &ofWhat, FSSpec& outSpec, const interchange_settings& settings)
{
	short			theFileSystemCode = ofWhat.Item.file_system;
	char			itsName[512]      = {""};
	short			itsCode           = 0;
	scsi_settings*  itsSetting        = NULL;
	
	memset(&outSpec, 0, sizeof(outSpec));
	
	switch (theFileSystemCode)
	{
		case InterChange_MacHFSFIleSystemCode:
			outSpec = ofWhat.HFSItem.file_spec;
			return (ofWhat.HFSItem.file_type);

		case InterChange_AbleWDFileSystemCode:
		case InterChange_AbleOpticalFileSystemCode:
			itsCode = AbleDiskLib_FetchDeviceCode(ofWhat);
			
			if (itsCode < ABLE_HFS_READDATA_CODE)
				itsSetting = InterChangeLibGetSettingForCode(&settings, itsCode);
			else
				itsSetting = access_scsi_setting(itsCode);			// Note: Setting will not be available for 'none' devices or syncl scsi devices if no PCI1

			if (!itsSetting)										// no setting; is likely an image file
			{
				FSSpec *itsSpec = NULL;
				
				// See if is added image file
				if ((itsSpec = find_hfs_scsi_spec(itsCode)) != NULL)
				{
					outSpec = *itsSpec;
					return (t_lsubc);
				}

				return (0);
			}

			if (itsSetting->bus_id == SCSI_BUS_MENU_DISK_IMAGE)
			{
				outSpec = itsSetting->image_spec;
				return (t_lsubc);
			}
	}
	
	return (0);	
}


// =================================================================================
//		¥ AbleDiskLib_FillFile()
// =================================================================================

// Fills a file (for formating optical images or devices or zeroing image files)

int		AbleDiskLib_FillFile (InterChangeItemUnion &theFile, LCStr255 &theFileTree, LCStr255& volName, Boolean &abortFlag,
      						  AbleDiskLibProgressProc progressProc, void* itsObject, char fillByte)
{
	scsi_device*	the_device       = NULL;
	unsigned int    file_size_blocks = ((theFile.Item.file_size_bytes) + 511) >> 9;

	// Fill an able device
	if (theFile.Item.file_system == InterChange_AbleWDFileSystemCode)
	{
		// Look up device code for possible ref num
		if (theFile.WDItem.file_hfs_refnum)
			theFile.WDItem.file_device_code = CHFSImageFile::GetDevCode(theFile.WDItem.file_hfs_refnum);

		AbleDiskLib_LatchHFSRefNum(theFile);

		// Make sure the source file is available before we create the catalog entry.
		the_device = access_scsi_device(theFile.WDItem.file_device_code);
	}
	
	else if (theFile.Item.file_system == InterChange_AbleOpticalFileSystemCode)
	{
		// Look up device code for possible ref num
		if (theFile.OpticalItem.file_hfs_refnum)
			theFile.OpticalItem.file_device_code = CHFSImageFile::GetDevCode(theFile.OpticalItem.file_hfs_refnum);

		AbleDiskLib_LatchHFSRefNum(theFile);

		// Make sure the source file is available before we create the catalog entry.
		the_device = access_scsi_device(theFile.OpticalItem.file_device_code);
	}
	
	if (!the_device)
	{
		printf("InterChangeX: Could not initialize file '%s'\n   Error Report:\n", (char *) theFileTree);
		printf("   Device Manager Could Not Be Found\n");
		AbleDiskLib_ReleaseHFSRefNum(theFile);
		return (-1);											// device should have been configured before we got here...
	}
	
	if (abortFlag)
	{
		AbleDiskLib_ReleaseHFSRefNum(theFile);
		return(0);
	}

	if (AbleDiskLib_FillAbleFile(the_device, file_size_blocks, abortFlag, progressProc, itsObject, fillByte))
	{
		AbleDiskLib_ReleaseHFSRefNum(theFile);
		return (-1);
	}

	if (abortFlag)
	{
		AbleDiskLib_ReleaseHFSRefNum(theFile);
		return (-1);
	}
	
	// Create optical volume header
	if (fillByte == 0x63 && volName.Length() != 0)
	{
		if (AbleOptLib_CreateVolumeHeader(the_device, file_size_blocks, (char*) volName))
		{
			printf("InterChangeX: Could not initialize file '%s'\n   Error Report:\n", (char *) theFileTree);
			printf("   Could not write optical volume header\n");
			AbleDiskLib_ReleaseHFSRefNum(theFile);
			return (-1);
		}
	}
	
	AbleDiskLib_ReleaseHFSRefNum(theFile);
	return (0);
}
