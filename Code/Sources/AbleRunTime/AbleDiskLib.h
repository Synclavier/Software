/* Able Disk Library */

// Header for Able Disk Library

// Mac OS
#if __LP64__
    #include "SyncMutex.h"

#else
    #include <NameRegistry.h>
    #include <Types.h>
    #include <Files.h>
#endif

// PowerPlant addons
#include "LCStr255.h"

// Local includes
#include "InterChange.h"
#include "catrtns.h"
#include "messages.h"

#pragma once

// ---------------------------------------------------------------------------
//	¥ InterChangeItem and related structures
// ---------------------------------------------------------------------------

// The following structs are used to identify InterChange items such as drives,
// files, and folders.  They are implemented as structs instead of classes
// so they can be arrayed.

// Enumerate file system and sort option codes

#define	AbleDiskLibSortSize	1024				// Maximum number of items that can be sorted
#define	AbleDiskLibTypeSize   25                // Must be same as InterChange_HFSFolderTypeCode_unk+1

typedef enum InterChangeFileSystemCodes
{
	InterChange_NoFIleSystemCode,				// Indicates item is null
	InterChange_MacHFSFIleSystemCode,			// Indicates item is on a Mac HFS volume
	InterChange_AbleWDFileSystemCode,			// Indicates item is on an Able hard drive or image file
	InterChange_AbleOpticalFileSystemCode		// Indicates item is on an Able Optical media or image file

} InterChangeFileSystemCodes;

typedef enum	InterChangeSortCode				// Sort codings; must mach menu items directly
{
	InterChange_sort_by_type_ascending  = 1,
	InterChange_sort_by_name_ascending  = 2,
	InterChange_sort_by_size_ascending  = 3,
	InterChange_sort_by_size_descending = 4,
	InterChange_sort_by_name_descending = 5,
	InterChange_sort_by_type_descending = 6
	
} InterChangeSortCode;

typedef enum	InterChangeOpticalSortCode		// Sort codings; must mach menu items directly
{
	InterChange_show_categories         = 1,
	InterChange_show_files              = 2
	
} InterChangeOpticalSortCode;

typedef enum	HFSFolderTypeCodes				// Codings for HFS items that are 'folders' or mac audio files; must be exclusive of able file types
{
	InterChange_HFSFolderTypeCode_fold  = 16,	// FSSpec points to Macintosh HFS folder
	InterChange_HFSFolderTypeCode_disk  = 17,	// FSSpec points to Macintosh volume
	InterChange_HFSFolderTypeCode_AIFF  = 18,	// FSSpec points to AIFF file
	InterChange_HFSFolderTypeCode_Sd2f  = 19,	// FSSpec points to Sd2f file
	InterChange_HFSFolderTypeCode_WAVE  = 20,	// FSSpec points to WAVE file
	InterChange_AbleOpticalDirectory	= 21,	// Block start == 0 implies optical media or image file; block start != 0 implies optical category
    InterChange_HFSFolderTypeCode_CAF   = 22,   // FSSpec points to a CAF file
    InterChange_HFSFolderTypeCode_MP3   = 23,   // FSSpec points to a MP3 file
	InterChange_HFSFolderTypeCode_unk   = 24	// When adding type, change AbleDiskLibTypeSize and look up sort table useage
	
} HFSFolderTypeCodes;

typedef enum	InterChangeLocateCode			// Codings for prefered file when locating by treename
{
	InterChangeLocateCode_none       = 0,		// No preference
	InterChangeLocateCode_directory  = 1,		// Prefer directory
	InterChangeLocateCode_file       = 2		// Prefer file
	
} InterChangeLocateCode;

// Handy class to protect catalog routine global variables in threaded environment
#if __LP64__
    extern SyncMutex gCatMutex;
    extern SyncMutex gOptMutex;

#else
    class	stCatSemaphore
    {
        public:
            stCatSemaphore ();
            ~stCatSemaphore();
            
            static	class LThread*	owner;
            static	int	    		owner_count;
    };
#endif

// InterChangeItem and derivative structs
typedef struct InterChangeItem					// Basic InterChange item.  Effectively a base class.
{
	unsigned short	file_system;				// Indicates which file system (see enums)
	unsigned short	file_type;					// Indicates file type (able file type enumerations)
	SInt64			file_size_bytes;			// File size in bytes.  Not available at all times.
} InterChangeItem;

typedef struct MacHFSItem
{
	unsigned short	file_system;				// Indicates which file system (see enums)
	unsigned short	file_type;					// Indicates file type (able file type enumerations)
	SInt64			file_size_bytes;			// File size in bytes.  Not available at all times.

	SyncFSSpec		file_spec;					// Mac HFS FSSpec the points to device (vol, par ID, name) NAME MUST BE NULL TERMINATED!!!
} MacHFSItem;

typedef struct AbleWDItem
{
	unsigned short	file_system;				// Indicates which file system (see enums)
	unsigned short	file_type;					// Indicates file type (able file type enumerations)
	SInt64			file_size_bytes;			// File size in bytes.  Not available at all times.

	unsigned short	file_device_code;			// Able device code (e.g. AbleReaddataCode)
	unsigned short	file_hfs_refnum;			// Mac HFS refnm of file resides on the Mac
	unsigned int	file_block_start;			// Block start on device
	char			file_name[10];				// File name, c-string
} AbleWDItem;

typedef struct AbleOpticalItem
{
	unsigned short	file_system;				// Indicates which file system (see enums)
	unsigned short	file_type;					// Indicates file type (able file type enumerations)
	SInt64			file_size_bytes;			// File size in bytes.  Not available at all times.

	unsigned short	file_device_code;			// Able device code (e.g. AbleReaddataCode)
	unsigned short	file_hfs_refnum;			// Mac HFS refnm of file resides on the Mac
	unsigned int	file_block_start;			// Block start on device
	char			file_name[34];				// File name, c-string
} AbleOpticalItem;

// Handy union that can hold any one of them
typedef union InterChangeItemUnion
{
	InterChangeItem	Item;
	MacHFSItem		HFSItem;
	AbleWDItem		WDItem;
	AbleOpticalItem OpticalItem;
	
} InterChangeItemUnion;

// Even handier default structs for root directories
const AbleWDItem		AbleWDItemW0       = {InterChange_AbleWDFileSystemCode,      t_lsubc, 0, ABLE_W0_READDATA_CODE, 0, 0, "W0:" };
const AbleWDItem		AbleWDItemW1       = {InterChange_AbleWDFileSystemCode,      t_lsubc, 0, ABLE_W1_READDATA_CODE, 0, 0, "W1:" };
const AbleOpticalItem	AbleOpticalItemOp0 = {InterChange_AbleOpticalFileSystemCode, InterChange_AbleOpticalDirectory, 0, ABLE_O0_READDATA_CODE, 0, 0, "Op0:"};
const AbleOpticalItem	AbleOpticalItemOp1 = {InterChange_AbleOpticalFileSystemCode, InterChange_AbleOpticalDirectory, 0, ABLE_O1_READDATA_CODE, 0, 0, "Op1:"};

// Awesomely handy struct for storing information about an optical media

typedef	struct	InterChangeOpticalData
{
	char			volume_name[34];			// Optical volume name
	unsigned short  volume_time;

	unsigned int	header;						// Sector number of header			(512-byte sectors)
	unsigned int	dir_start;					// Sector number of directory start	(512-byte sectors)
	unsigned int	dir_len;					// Sector length of directory		(512-byte sectors)
	unsigned int	data_start;					// Sector of data start				(512-byte sectors)
	unsigned int	data_len;					// Sector length of data area		(512-byte sectors)
	
	unsigned int	dir_used;					// Number of used entries			(No. of entries)
	unsigned int	dir_free;					// Number of free entries			(No. of free entries)
	unsigned int	dir_next;					// Next free entry number			(0-based entry number of next free entry)
	unsigned int	data_used;					// Sectors of data area used		(512-byte sectors)
	unsigned int	data_free;					// Sectors of data free				(512-byte sectors)
	unsigned int    data_next;					// Next free sector in data area	(512-byte sectors)
	unsigned int	dir_last;					// Entry no. (0-based) of last e_dir_entry
	
} InterChangeOpticalData;

// Ultra-handy struct for storing a complete directory
typedef	struct	InterChangeItemDirectory
{
	InterChangeItemUnion	root;				// Descriptor for which directory this data is from
	union
	{
		void*				void_items;			// Pointer to list of entries
		MacHFSItem*			mac_items;			// Handy type-casted equivalents
		AbleWDItem*			wd_items;
		AbleOpticalItem*	op_items;
	};

	int                     item_size;			// Size (bytes) of each entry item
	int                     num_items;			// Number of entries
	int                     blocks_total;		// Total blocks available for all files, including directory header
	int                     blocks_used;		// Total blocks used for all files, including directory header
	
	int                     optical_data_valid;
	
    InterChangeOpticalData	optical_data;		// Data from optical device
	
} InterChangeItemDirectory;

// File type sort look-up table
extern	int		AbleDiskLib_SortKey[AbleDiskLibTypeSize];

// Quasi-useful error message
extern	char	AbleDiskLib_RecentErrorMessage[MESSAGE_BUF_SIZE];

// ProcPtr callback type to show transaction progress
typedef	void (*AbleDiskLibProgressProc)(void *theObject, SInt32 theValue);

// InterChangeItem support routines
void			SortInterChangeItems(void* inList, int inNumEntries, void* outList[], int theSortCode);

// ABLE file name utilities
void 			AbleFileName2CStr(const uint16 *file_name, char *c_string);
fixed 			CheckAbleFileName(const uint16 *file_name);
fixed           GetAbleFileTypeFromExtension(LCStr255& itsName);
fixed			GetAbleFileTypeFromExtension(const SyncFSSpec& itsSpec);
fixed			GetAbleFileTypeFromExtension(const LSItemInfoRecord& lsRecord);
fixed 			GetAbleFileType(OSType mac_type, OSType mac_creator, const SyncFSSpec& itsSpec);
void 			GetMacFileType (fixed file_type, OSType& out_mac_type, OSType& out_mac_creator);

void		    AbleDiskLib_FillUnionFromFSSpec	( const SyncFSSpec& inSpec, short inType, SInt64 inLength, InterChangeItemUnion& outUnion );
Boolean			AbleDiskLib_GetSpecForUnion  	( InterChangeItemUnion &theUnion, SyncFSSpec& theSpec, long long& theBlock, long long& theLength, short& theCode );
Boolean			AbleDiskLib_UnionsAreEquivalent	( InterChangeItemUnion &unionOne, InterChangeItemUnion &unionTwo );
Boolean			AbleDiskLib_UnionDescendsFrom	( InterChangeItemUnion &parentUnion, InterChangeItemUnion &childUnion, int numLevels );
void			AbleDiskLib_IsolateTopDirectory ( InterChangeItemUnion &rootUnion, LCStr255 &rootName, InterChangeItemUnion &topUnion, LCStr255 &topName);

// AbleDiskLib Routines
int				AbleDiskLib_Initialize();
void			AbleDiskLib_Cleanup();
Boolean			AbleDiskLib_CheckForSoundFIleChange();
void			AbleDiskLib_ResetSoundFIleChange   ();
int				AbleDiskLib_ReadAbleDisk		     (struct scsi_device* the_device, uint16 *buffer, uint32 block_num, uint32 block_len);
int 			AbleDiskLib_WriteAbleDisk			 (struct scsi_device* the_device, uint16 *buffer, uint32 block_num, uint32 block_len);

struct scsi_device*	AbleDiskLib_FetchDevicePointer    (InterChangeItemUnion &theUnion);
void			AbleDiskLib_LatchHFSRefNum			  (InterChangeItemUnion &theUnion);
void			AbleDiskLib_ReleaseHFSRefNum		  (InterChangeItemUnion &theUnion);

short			AbleDiskLib_FetchDeviceCode			  (const InterChangeItemUnion &theUnion);
void			AbleDiskLib_InterrogateDevice         (InterChangeItemUnion &theUnion);
int 			AbleDiskLib_ReadDirectory        	  (InterChangeItemUnion &theUnion, LCStr255 &theTreeName, InterChangeItemDirectory &itsData);
int 			AbleDiskLib_FindLargestHole			  (LCStr255 &theTreeName, uint32 &theHole);
int 			AbleDiskLib_LocateFileByTreename 	  (LCStr255 &theTreeName, InterChangeItemUnion &theUnion, int fileType, interchange_settings &theSettings);
int 			AbleDiskLib_RenameFileByTreename	  (LCStr255 &theTreeName, LCStr255 &theNewName, interchange_settings& theSettings);
int				AbleDiskLib_UnsaveFileByTreename 	  (LCStr255 &theTreeName, interchange_settings& theSettings);
int				AbleDiskLib_CheckDuplicateByTreename  (LCStr255 &theTreeName);
int 			AbleDiskLib_ReadDropDirectory         (LCStr255 &theTreeName, int& outBlockStart);
int				AbleDiskLib_AddEntityToDirectory      (LCStr255 &theEntityName, unsigned short file_type, long long file_size_bytes);
int 			AbleDiskLib_CacheDropDirectory        (LCStr255 &theTreeName, int& outBlockStart);
int				AbleDiskLib_AddEntityToCachedDirectory(LCStr255 &theEntityName, unsigned short file_type, long long file_size_bytes);
int 			AbleDiskLib_FindFileInCachedDirectory (LCStr255 &theEntityName);
int				AbleDiskLib_CloneFile		  		  (InterChangeItemUnion &theSource, LCStr255 &theDestinationTree, InterChangeItemUnion &theResult,
													   Boolean &abortFlag, AbleDiskLibProgressProc progressProc, void* itsObject, UInt32 resizeSectors,
													   interchange_settings& theSettings);
int				AbleDiskLib_CreateSubcat 			  (LCStr255 &theSubcatTreeName, InterChangeItemUnion &theResult, uint32 sectorSize);
int				AbleDiskLib_ConstructDeviceDescription(InterChangeItemUnion &ofWhat, LCStr255& outDesc, struct interchange_settings& settings);
int				AbleDiskLib_FetchReleventFSSpec		  (const InterChangeItemUnion &ofWhat, SyncFSSpec& outSpec, const interchange_settings& settings);
int				AbleDiskLib_FillFile 				  (InterChangeItemUnion &theFile, LCStr255 &theFileTree, LCStr255& volName, Boolean &abortFlag,
      						   						   AbleDiskLibProgressProc progressProc, void* itsObject, char fillByte);
