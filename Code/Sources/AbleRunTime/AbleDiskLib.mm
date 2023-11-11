// =================================================================================
//	AbleDiskLib
// =================================================================================

// Library for importing/exporting from Synclavier¨ Disks

// 3/5/99 C. Jones

// Std C
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

SyncMutex   gCatMutex;
SyncMutex   gOptMutex;


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
			whereName = (char *) &(((MacHFSItem *) inList)->file_spec.file_name) - (char *) inList;
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
		return t_timbre;
		
	if (strcmp(c_name, ".PATDATA") == 0)		// old patchdata format
		return t_timbre;
		
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

    // Not a special name
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

fixed GetAbleFileTypeFromExtension(LCStr255& itsName)
{
    if (itsName.EndsWith(".sprg"))
        return t_exec;
    
    if (itsName.EndsWith(".srel"))
        return t_reloc;
    
    if (itsName.EndsWith(".sdat"))
        return t_data;
    
    if (itsName.EndsWith(".sseq"))
        return t_sync;
    
    if (itsName.EndsWith(".ssnd"))
        return t_sound;
    
    if (itsName.EndsWith(".simg"))
        return t_lsubc;
    
    if (itsName.EndsWith(".sdmp"))
        return t_dump;
    
    if (itsName.EndsWith(".sspe"))
        return t_spect;
    
    if (itsName.EndsWith(".sind"))
        return t_index;
    
    if (itsName.EndsWith(".stmb"))
        return t_timbre;
	
    if (itsName.BeginsWith("*WORK"))
        return t_data;
    
    if (itsName.BeginsWith("*BNKDATA"))
        return t_timbre;
    
    if (itsName.BeginsWith("*PATDATA"))
        return t_timbre;
    
    if (itsName.BeginsWith("*NEWDATA"))
        return t_timbre;
    
    if (itsName.BeginsWith("*SQ0DATA"))
        return t_sync;
    
    if (itsName.BeginsWith("*SQ1DATA"))
        return t_sync;
    
    if (itsName.BeginsWith("*SQ2DATA"))
        return t_sync;
    
    if (itsName.BeginsWith("*SQ3DATA"))
        return t_sync;
    
    if (itsName.BeginsWith("*SQ4DATA"))
        return t_sync;
    
    if (itsName.BeginsWith("*SQ5DATA"))
        return t_sync;
    
    if (itsName.BeginsWith("*SQ6DATA"))
        return t_sync;
    
    if (itsName.BeginsWith("*SQ7DATA"))
        return t_sync;
    
    if (itsName.BeginsWith("*UNDOSEQ"))
        return t_sync;
    
	return (-1);
}

fixed GetAbleFileTypeFromExtension(const SyncFSSpec& itsSpec)
{
    LCStr255 itsName(itsSpec.file_name);
    
    return GetAbleFileTypeFromExtension(itsName);
}

// Note - this is only called if the extension did not exist or was not recognized. The extension field of the lsRecord is not populated.
fixed GetAbleFileTypeFromExtension(const LSItemInfoRecord& lsRecord)
{
    fixed  export_type = (-1);
    OSType mac_type    = lsRecord.filetype;
    OSType mac_creator = lsRecord.creator;
    
    // Note - some early mac files were created with swapped type & extension. So check.
    if (mac_type == 'SNCL') {
        mac_type    = lsRecord.creator;
        mac_creator = lsRecord.filetype;
    }

    if (mac_type =='TEXT')
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

fixed GetAbleFileType(OSType mac_type, OSType mac_creator, const SyncFSSpec& itsSpec)
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
		{out_mac_creator = '\?\?\?\?'; out_mac_type = '\?\?\?\?';}
}


// =================================================================================
//	InterChangeItemUnion utilities - AbleDiskLib_FillUnionFromFSSpec
// =================================================================================

void 	AbleDiskLib_FillUnionFromFSSpec(const SyncFSSpec& inSpec, short inType, SInt64 inLength, InterChangeItemUnion& outUnion)
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

Boolean	AbleDiskLib_GetSpecForUnion( InterChangeItemUnion &theUnion, SyncFSSpec& theSpec, long long& theBlock, long long& theLength, short& theCode )
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
	SyncFSSpec	one;
	SyncFSSpec	two;
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
	SyncFSSpec	parent;
	SyncFSSpec	child;
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
			
			nextColon = rootName.Find('/', 0);										// find first :

			if (nextColon == -1 || nextColon == rootName.Length()-1)				// no colon? or looking for xyz:
				topName = "";														// return null string for name (e.g. reference to directory itself)
			else																	// else file name starts with colon on selected device (e.g. :cat:cat:cat:file
				topName.Assign(&((char *)rootName)[nextColon], (int) (rootName.Length() - nextColon));
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

		nextColon = rootName.Find('/', 0);											// find first :

		if (nextColon == -1 || nextColon == rootName.Length()-1)					// no colon? or looking for xyz:
			topName = "";															// return null string for name (e.g. reference to directory itself)
		else																		// else file name starts with colon on selected device (e.g. :cat:cat:cat:file
			topName.Assign(&((char*)rootName)[nextColon], (int) (rootName.Length() - nextColon));

		return;
	}

	// Else see if able optical
	else if (rootUnion.Item.file_system == InterChange_AbleOpticalFileSystemCode)	// Able optical
	{
		topUnion = rootUnion;														// Start with union for directory

		// Look up code for possible ref num, although we don't latch it here
		if (topUnion.OpticalItem.file_hfs_refnum)
			topUnion.OpticalItem.file_device_code = CHFSImageFile::GetDevCode(topUnion.OpticalItem.file_hfs_refnum);

		nextColon = rootName.Find('/', 0);											// find first :

		if (nextColon == -1 || nextColon == rootName.Length()-1)						// no colon? or looking for xyz:
			topName = "";															// return null string for name (e.g. reference to directory itself)
		else																		// else file name starts with colon on selected device (e.g. :cat:cat:cat:file
			topName.Assign((&((char*)rootName)[nextColon]), (UInt8) (rootName.Length() - nextColon));

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
		if (block_num & 1) printf(SYNCLAVIER_APPLICATION_NAME ": Odd sector read error from optical disk\n");
		
		status = issue_read_extended(the_device, (byte *) buffer, block_num>>1, (block_len+1)>>1);
	}
	
	else
		status = BAD_STATUS;
		
	if (status)
		printf(SYNCLAVIER_APPLICATION_NAME ": Disk Read Error Occured (%d)\n", status);
	
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
		printf(SYNCLAVIER_APPLICATION_NAME ": Serious Disk Write Error Occured\n");
		return (-1);
	}
	
	if (the_device->fBlockSize == 512)
		status = issue_write_extended(the_device, (byte *) buffer, block_num, block_len);
	
	else if (the_device->fBlockSize == 1024)
	{
		if (block_num & 1) printf(SYNCLAVIER_APPLICATION_NAME ": Odd sector write error from optical disk\n");

		status = issue_write_extended(the_device, (byte *) buffer, block_num>>1, (block_len+1)>>1);
	}
	
	else
		status = BAD_STATUS;
		
	if (status)
		printf(SYNCLAVIER_APPLICATION_NAME ": Disk Write Error Occured (%d)\n", status);

	return (status);
}
