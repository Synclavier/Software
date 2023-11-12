/* Able Disk Library */

/* Created - 7/14/96	C. Jones */

// June 2020. AbleDiskEngine.cpp was gutted to only provide the following services to support building Synclavier3.
// The hardware import/export functions have not been ported to 64-bit. The import functionality is now in Synclavier3.
// ./Deprecated/AbleDiskEngine.cpp has the original


// -create "/Volumes/CJ_Cache/XPL_Build_Products/Synclavier3/W0 Disk Image.simg" 7340032 -zero
// -w2 -s 5 -file "/Volumes/CJ_Cache/XPL_Build_Products/Synclavier3/W0 Disk Image.simg" "-e" "/Volumes/CJ_Cache/XPL_Build_Products/W0/MONITOR.sprg" "W0:MONITOR"
// -w2 -s 5 -file "/Volumes/CJ_Cache/XPL_Build_Products/Synclavier3/W0 Disk Image.simg" "-e" "/Volumes/CJ_Cache/XPL_Build_Products/W0/*SYSTEM/" "W0:.SYSTEM"
// -w2 -s 5 -file "/Volumes/CJ_Cache/XPL_Build_Products/Synclavier3/W0 Disk Image.simg" "-e" "/Volumes/CJ_Cache/XPL_Build_Products/W0/PROFILE-5.3.txt" "W0:PROFILE"
// -w2 -s 5 -file "/Volumes/CJ_Cache/XPL_Build_Products/Synclavier3/W0 Disk Image.simg" "-e" "/Volumes/CJ_Cache/XPL_Build_Products/W0/SYN-5.3.sprg" "W0:SYN-5.3"
// -w2 -s 5 -file "/Volumes/CJ_Cache/XPL_Build_Products/Synclavier3/W0 Disk Image.simg" "-e" "/Volumes/CJ_Cache/XPL_Build_Products/W0/SYN-5.3G.sprg" "W0:SYN-5.3G"
// -w2 -s 5 -file "/Volumes/CJ_Cache/XPL_Build_Products/Synclavier3/W0 Disk Image.simg" "-e" "/Volumes/CJ_Cache/XPL_Build_Products/W0/SYN-5.3M.sprg" "W0:SYN-5.3M"
// -w2 -s 5 -file "/Volumes/CJ_Cache/XPL_Build_Products/Synclavier3/W0 Disk Image.simg" "-e" "/Volumes/CJ_Cache/XPL_Build_Products/W0 XPL Files/*asm-7.sprg" "W0:.system:.asm-7"
// -w2 -s 5 -file "/Volumes/CJ_Cache/XPL_Build_Products/Synclavier3/W0 Disk Image.simg" "-e" "/Volumes/CJ_Cache/XPL_Build_Products/W0 XPL Files/*p1-7.sprg" "W0:.system:.p1-7"
// -w2 -s 5 -file "/Volumes/CJ_Cache/XPL_Build_Products/Synclavier3/W0 Disk Image.simg" "-e" "/Volumes/CJ_Cache/XPL_Build_Products/W0 XPL Files/*p2-7.sprg" "W0:.system:.p2-7"
// -w2 -s 5 -file "/Volumes/CJ_Cache/XPL_Build_Products/Synclavier3/W0 Disk Image.simg" "-e" "/Volumes/CJ_Cache/XPL_Build_Products/W0 XPL Files/*p3-7.sprg" "W0:.system:.p3-7"
// -w2 -s 5 -file "/Volumes/CJ_Cache/XPL_Build_Products/Synclavier3/W0 Disk Image.simg" "-e" "/Volumes/CJ_Cache/XPL_Build_Products/W0 XPL Files/DUMP.sprg" "W0:.system:DUMP"
// -w2 -s 5 -file "/Volumes/CJ_Cache/XPL_Build_Products/Synclavier3/W0 Disk Image.simg" "-e" "/Volumes/CJ_Cache/XPL_Build_Products/W0 XPL Files/*RTC-7.sdat" "W0:.system:.RTC-7"
// -w2 -s 5 -file "/Volumes/CJ_Cache/XPL_Build_Products/Synclavier3/W0 Disk Image.simg" "-e" "/Volumes/CJ_Cache/XPL_Build_Products/W0 XPL Files/*RTD-7.sdat" "W0:.system:.RTD-7"
// -w2 -s 5 -file "/Volumes/CJ_Cache/XPL_Build_Products/Synclavier3/W0 Disk Image.simg" "-e" "/Volumes/CJ_Cache/XPL_Build_Products/W0 XPL Files/*ST-7.sdat" "W0:.system:.ST-7"
// -w2 -s 5 -file "/Volumes/CJ_Cache/XPL_Build_Products/Synclavier3/W0 Disk Image.simg" "-e" "/Volumes/CJ_Cache/XPL_Build_Products/W0 MPLT Files/*SAUX-7.sdat" "W0:.system:.SAUX-7"
// -w2 -s 5 -file "/Volumes/CJ_Cache/XPL_Build_Products/Synclavier3/W0 Disk Image.simg" "-e" "/Volumes/CJ_Cache/XPL_Build_Products/W0 MPLT Files/*SCWT-7.sdat" "W0:.system:.SCWT-7"
// -w2 -s 5 -file "/Volumes/CJ_Cache/XPL_Build_Products/Synclavier3/W0 Disk Image.simg" "-e" "/Volumes/CJ_Cache/XPL_Build_Products/W0 MPLT Files/*SFRM-7.sdat" "W0:.system:.SFRM-7"
// -w2 -s 5 -file "/Volumes/CJ_Cache/XPL_Build_Products/Synclavier3/W0 Disk Image.simg" "-e" "/Volumes/CJ_Cache/XPL_Build_Products/W0 MPLT Files/*SLIB-7.sdat" "W0:.system:.SLIB-7"
// -w2 -s 5 -file "/Volumes/CJ_Cache/XPL_Build_Products/Synclavier3/W0 Disk Image.simg" "-e" "/Volumes/CJ_Cache/XPL_Build_Products/W0 MPLT Files/*SPRO-7.txt" "W0:.system:.SPRO-7"
// -w2 -s 5 -file "/Volumes/CJ_Cache/XPL_Build_Products/Synclavier3/W0 Disk Image.simg" "-e" "/Volumes/CJ_Cache/XPL_Build_Products/W0 MPLT Files/*SPLT-7.sprg" "W0:.system:.SPLT-7"
// -w2 -s 5 -file "/Volumes/CJ_Cache/XPL_Build_Products/Synclavier3/W0 Disk Image.simg" "-e" "/Volumes/CJ Data/Dropbox/Synclavier Digital Repository/Projects/AbleBrowser/../../Able/W0 Demo Files/CLARA3.ssnd" "W0:"
// -w2 -s 5 -file "/Volumes/CJ_Cache/XPL_Build_Products/Synclavier3/W0 Disk Image.simg" "-e" "/Volumes/CJ Data/Dropbox/Synclavier Digital Repository/Projects/AbleBrowser/../../Able/W0 Demo Files/CLARA4.ssnd" "W0:"
// -w2 -s 5 -file "/Volumes/CJ_Cache/XPL_Build_Products/Synclavier3/W0 Disk Image.simg" "-e" "/Volumes/CJ_Cache/XPL_Build_Products/DIAGBINS/" "W0:DIAGBINS"


/* Provides code to access Able winchester disks connected to host 			*/
/* computer SCSI port...													*/

// Std C includes
#include	<stdlib.h>
#include	<stdio.h>
#include 	<string.h>

// Able includes
#include	"Standard.h"
#include	"XPL.h"
#include	"XPLRuntime.h"
#include	"Catrtns.h"

#include	"ScsiLib.h"
#include	"Messages.h"
#include	"D24Sim.h"

#include	"Utility.h"
#include	"MacSCSIOSx.h"
#include	"SynclavierPCILib.h"
#include	"samplits.h"
#include    "SyncMutex.h"

// Interchange
#include	"InterChange.h"
#include	"AbleDiskEngine.h"
#include	"AbleDiskAp.r"

// Soundfile Translator
#include 	"CSynclavierSoundFileHeader.h"
#include 	"SoundfileTranslators.h"
#include	"SoundDesigner.h"
#include	"Wave.h"

// Wrapper classes
#include    "SynclavierFileReference.h"
#include    "CSynclavierMutableArray.h"
#include    "SyncCFStringUtilities.h"

#define		MAX_TEXT_SIZE 			(256*1024)
#define		MAC_OS_FILE_RETURN		(13)
#define     MAC_OS_FILE_NEWLINE     (10)


// =================================================================================
//	Able String Utilities - GetAbleFileType
// =================================================================================

static int MatchExtension(const char *nameEnd, const  char *extension)
{
	int i = 0;

	for (i=0; i<strlen(extension); i++)
		if (nameEnd[i] != extension[i])
			return false;
			
	return true;
}

static void WipeExtension4(char *nameEnd)
{
    nameEnd[0] = nameEnd[1] = nameEnd[2] = nameEnd[3] = nameEnd[4] = 0;
}

static void WipeExtension5(char *nameEnd)
{
	nameEnd[0] = nameEnd[1] = nameEnd[2] = nameEnd[3] = nameEnd[4] = nameEnd[5] = 0;
}

static void WipeNameExtension(char *itsName)
{
	int i = strlen(itsName);
    
    if (i < 5)
        return;
    
    char* nameEnd = itsName + i - 4;

    if (MatchExtension(nameEnd, ".txt")) {WipeExtension4(nameEnd); return;}

	if (i < 6)
		return;
	
	nameEnd = itsName + i - 5;

    if (MatchExtension(nameEnd, ".text")) {WipeExtension5(nameEnd); return;}
    if (MatchExtension(nameEnd, ".sprg")) {WipeExtension5(nameEnd); return;}
	if (MatchExtension(nameEnd, ".srel")) {WipeExtension5(nameEnd); return;}
	if (MatchExtension(nameEnd, ".sdat")) {WipeExtension5(nameEnd); return;}
	if (MatchExtension(nameEnd, ".sseq")) {WipeExtension5(nameEnd); return;}
	if (MatchExtension(nameEnd, ".ssnd")) {WipeExtension5(nameEnd); return;}
	if (MatchExtension(nameEnd, ".simg")) {WipeExtension5(nameEnd); return;}
	if (MatchExtension(nameEnd, ".sdmp")) {WipeExtension5(nameEnd); return;}
	if (MatchExtension(nameEnd, ".sspe")) {WipeExtension5(nameEnd); return;}
	if (MatchExtension(nameEnd, ".sind")) {WipeExtension5(nameEnd); return;}
	if (MatchExtension(nameEnd, ".stmb")) {WipeExtension5(nameEnd); return;}
}

static fixed GetAbleFileType(OSType mac_type, OSType mac_creator, const SyncFSSpec* its_spec)
{
	fixed export_type;
	
	// Give priority to file name extension
	int i = strlen(its_spec->file_name);
	
    if (i > 4)
    {
        const char* nameEnd = (const char *) & its_spec->file_name[i-4];
        
        if (MatchExtension(nameEnd, ".txt")) return t_text;
    }
    
	if (i > 5)
	{
		const char* nameEnd = (const char *) & its_spec->file_name[i-5];
		
        if (MatchExtension(nameEnd, ".text")) return t_text;
		if (MatchExtension(nameEnd, ".sprg")) return t_exec;
		if (MatchExtension(nameEnd, ".srel")) return t_reloc;
		if (MatchExtension(nameEnd, ".sdat")) return t_data;
		if (MatchExtension(nameEnd, ".sseq")) return t_sync;
		if (MatchExtension(nameEnd, ".ssnd")) return t_sound;
		if (MatchExtension(nameEnd, ".simg")) return t_lsubc;
		if (MatchExtension(nameEnd, ".sdmp")) return t_dump;
		if (MatchExtension(nameEnd, ".sspe")) return t_spect;
		if (MatchExtension(nameEnd, ".sind")) return t_index;
		if (MatchExtension(nameEnd, ".stmb")) return t_timbre;
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

	else if (mac_type == AIFFID)
		export_type = t_sound;
		
	else if (mac_type == SD2FileType)
		export_type = t_sound;
	
	else if (mac_type == WAVEID)
		export_type = t_sound;

	else
		return (-1);
		
	return (export_type);
}


/*--------------------------------------------------------------------------*/
/*	Global Variables														*/
/*--------------------------------------------------------------------------*/

static	uint16	SCSI_id                    = 99; 
static	uint16	bus_id					   = MAC_SCSI_PORT;

static	boolean	use_sim_file			   = false;
static	boolean	export_file                = false;
static	boolean	create_new_disk_file       = false;
static	boolean	zero_new_disk_file         = false;

static	char	export_mac_entity_name	[ 512] = {""};
static	char	export_able_name	    [ 512] = {""};
static	fixed	export_type;

static	ufixed	zero_buf [1024];

static	char   	er_mess[MESSAGE_BUF_SIZE];

static	SyncFSSpec	AbleSimFSSpec;

long	AbleDiskToolNormalTermination = false;


/*--------------------------------------------------------------------------*/
/*	Reinitialize global data												*/
/*--------------------------------------------------------------------------*/

static void initialize_static_data()
{
	SCSI_id         	  		= 99;
	bus_id					    = MAC_SCSI_PORT;

	use_sim_file				= false;
	export_file   	      		= false;
	create_new_disk_file        = false;
	zero_new_disk_file			= false;
	
	export_mac_entity_name  [0] = 0;
	export_able_name        [0] = 0;
	export_type			        = 0;

	g_scsi_print_basic_opt   = false;
	g_scsi_print_all_opt     = false;
}


/*--------------------------------------------------------------------------*/
/*	OpenMacFile																*/
/*--------------------------------------------------------------------------*/

typedef struct						// handy struct for accessing a mac file
{
    CSynclavierFileReference* MacFileRef;   // Not retained
	SInt32		MacFileLen;
	OSType		MacFileTyp;
	OSType		MacFileCre;
	uint32		blocks_allocated;

}   mac_file_info;

// Open a file for reading
static int OpenMacFile(const SyncFSSpec *the_spec, mac_file_info *the_info, const char *error_name)
{
	FInfo	info;
	
	// Init
    the_info->MacFileRef = nullptr;
	the_info->MacFileLen = 0;
	the_info->MacFileTyp = 0;
	the_info->MacFileCre = 0;
	the_info->blocks_allocated = 0;
	
	// Make an FSSpec for the file
	if (the_spec->file_ref == nullptr || !the_spec->file_ref->Reachable())
		{printf("OpenMacFile: Could not open \"%s\"\n", error_name); return (-1);}
    
    the_spec->file_ref->Open(O_RDONLY);
    
    the_info->MacFileRef = the_spec->file_ref;
    the_info->MacFileLen = the_spec->file_ref->Size();
	
    return (0);
}

static	void	CloseMacFile(mac_file_info& the_info)
{
    if (the_info.MacFileRef) {
        the_info.MacFileRef->Close();
        the_info.MacFileRef = nullptr;
    }
}


/*--------------------------------------------------------------------------*/
/*	ExportMacFile															*/
/*--------------------------------------------------------------------------*/

static uint32 	ComputeWordLengthOfMacTextFile	(uint32 byte_len, CSynclavierFileReference& our_ref, const char *error_name);
static uint32 	ComputeSoundFileSize	 		(CSynclavierFileReference& our_ref, const char *error_name, uint32 type);
static int 		ExportAbleTextFile				(uint16 scsi_id, uint32 block_num, uint32 byte_len,  CSynclavierFileReference& our_ref, uint32 word_len);
static int 		ExportAbleDataFile				(uint16 scsi_id, uint32 block_num, uint32 num_words, CSynclavierFileReference& our_ref);
static int 		ExportSoundFileFile				(uint16 scsi_id, uint32 block_num, CSynclavierFileReference& our_ref, short res_ref_num, uint32 type);

static int ExportMacFile(const SyncFSSpec *the_spec, const char *mac_entity_name,
                         const char *error_name, char *level_name, fixed level)
{
	fixed           export_type;
	uint32 	        byte_len;
	uint32          word_len;
	uint32          sec_len;
	char	        local_level_name[512] = {""};
    mac_file_info   the_info;
	fixed           ms_len, ls_len, words;
	
	// Open it
	if (OpenMacFile(the_spec, &the_info, mac_entity_name))
		return (-1);
				
	byte_len = the_info.MacFileLen;
	word_len = the_info.MacFileLen >> 1;

	// Get able file type	
	export_type = GetAbleFileType(the_info.MacFileTyp, the_info.MacFileCre, the_spec);
	
	if (export_type == -1)
	{
		printf("AbleDiskTool: Unrecognized file type for file '%s'\n", mac_entity_name);
		CloseMacFile(the_info);
		return (-1);
	}
		
	// Compute word length if text file
	if (export_type == t_text)
	{
		word_len = ComputeWordLengthOfMacTextFile(byte_len, *the_info.MacFileRef, mac_entity_name);
	
		if (word_len == (-1))
		{
			CloseMacFile(the_info);
			return (-1);
		}
	}
	
	// Compute work length if AIFF, Sd2F or WAVE
	else if (export_type == t_sound && (the_info.MacFileTyp == AIFFID || the_info.MacFileTyp == SD2FileType || the_info.MacFileTyp == WAVEID))
	{
		word_len = ComputeSoundFileSize(*the_info.MacFileRef, mac_entity_name, the_info.MacFileTyp);
		
		if (word_len == (-1))
		{
			CloseMacFile(the_info);
			return (-1);
		}
	}

	sec_len  = (word_len + 255) >> 8;		/* get length in sectors		*/
	
	ms_len = (fixed) (sec_len >> 16);
	ls_len = (fixed) (sec_len  & 0xFFFF);
	words  = (fixed) (word_len & 0xFFFF);
	
	the_info.blocks_allocated = sec_len;
	
	// Check for valid file name; allow correction if not
	strcpy(local_level_name, level_name);
	
    for (int i=0; i<strlen(local_level_name); i++)
        if (local_level_name[i] == '*')     // map leading * back to . for able's use
            local_level_name[i] = '.';
    
    // Append file name
    int i = strlen(local_level_name);
    
    if (i > 0 && local_level_name[i-1] == ':')
        strncat(local_level_name, the_spec->file_name, sizeof(local_level_name)-1);
        
    WipeNameExtension(local_level_name);

	// Create the able directory entry
	AbleCatRtns_AllowTypeReplace = true;

	if (!replace(local_level_name, export_type, ms_len, ls_len, words, level, true))
	{
		AbleCatRtns_AllowTypeReplace = false;
		get_cat_code_message(c_status, er_mess);
		printf("AbleDiskTool: Could not export '%s' for the following reason:\n", error_name);
		printf("   %s\n", er_mess);
		CloseMacFile(the_info);
		return (-1);
	}
	
	AbleCatRtns_AllowTypeReplace = false;	

	// Write the file to the SCSI disk
	
	// Export text file
	if (export_type == t_text)
	{
		if (ExportAbleTextFile(SCSI_id, (((uint32) (uint16) (f_ms_sector & 0xFF)) << 16) | ((uint32) (uint16) f_ls_sector),
		                       byte_len,*the_info.MacFileRef, word_len))
		{
			CloseMacFile(the_info);
			XPLdelete(local_level_name, level, true);
			return (-1);
		}
	}

	// Export AIFF, Sd2F or WAVE file
	else if (export_type == t_sound && (the_info.MacFileTyp == AIFFID || the_info.MacFileTyp == SD2FileType || the_info.MacFileTyp == WAVEID))
	{
		if (ExportSoundFileFile(SCSI_id, (((uint32) (uint16) (f_ms_sector & 0xFF)) << 16) | ((uint32) (uint16) f_ls_sector),
		                        *the_info.MacFileRef, 0, the_info.MacFileTyp))
		{
			CloseMacFile(the_info);
			XPLdelete(local_level_name, level, true);
			return (-1);
		}		
	}

	// Export other data files
	else
	{
		if (ExportAbleDataFile(SCSI_id, (((uint32) (uint16) (f_ms_sector & 0xFF)) << 16) | ((uint32) (uint16) f_ls_sector),
	    	               word_len, *the_info.MacFileRef))
		{
			CloseMacFile(the_info);
			XPLdelete(local_level_name, level, true);
			return (-1);
		}		
	}

	// Clean up and return	
	CloseMacFile(the_info);

	return (0);
}


/*--------------------------------------------------------------------------*/
/*    ExportMacFolder                                                       */
/*--------------------------------------------------------------------------*/

static CSynclavierMutableArray* GetListing(CFOptionFlags flags, CSynclavierFileReference& dirRef);
static void Scan__SIZE__File(uint32 byte_len, CSynclavierFileReference& our_ref, const char *error_name, uint32 *scanned_size, uint32 *scanned_style);
static int WriteAbleDisk(uint16 scsi_id, uint16 *buffer, uint32 block_num, uint32 block_len);

static int ExportMacFolder(const SyncFSSpec *the_spec, const char *mac_entity_name,
                           const char *error_name, char *level_name, fixed level)
{
    OSErr   FSstatus;
    
    Boolean targetIsFolder, wasAliased;

    uint32  block_num;
    uint32  block_len;
    uint32  word_len;
    uint32  sec_len;
    uint32  cat_style;
    fixed   cat_type;
            
    fixed   ms_len, ls_len, words;

    char    mac_path_name  [512] = {""};
    char    able_path_name [512] = {""};
    char    level_one_name [512] = {""};

    uint32  scanned_size  = 0;
    uint32  scanned_style = 0;

    CSynclavierFileReference& expRef = *the_spec->file_ref;
    
    CFURLRef sizeFileURL = expRef.CreateChild(CFSTR("__SIZE__"), false);
    
    CSynclavierFileReference sizeFileRef(sizeFileURL);
    
    // Scan size file
    if (sizeFileRef.Reachable()) {
    
        if ((FSstatus = sizeFileRef.Open(O_RDONLY)) != 0) {
            printf("AbleDiskTool: Could not open __SIZE__ file for '%s' (%d)\n", error_name, FSstatus);
            return (-1);
        }

        Scan__SIZE__File(sizeFileRef.Size(), sizeFileRef, error_name, &scanned_size, &scanned_style);

        if (scanned_size == 0 || (scanned_style != 1 && scanned_style != 4)) {
            printf("AbleDiskTool: __SIZE__ specification can't be read from '%s'\n", error_name);
            return (-1);
        }
    }
    
    // Else estimate
    else {
        scanned_size  = expRef.Size() / 512;   // In sectors
        scanned_style = 4;
    }
        
    // Initialize full tree names we maintain
    strncpy(mac_path_name,  mac_entity_name, sizeof(mac_path_name ));    // starting name
    strncpy(able_path_name, level_name,      sizeof(able_path_name));

    for (int i=0; i<strlen(able_path_name); i++)
        if (able_path_name[i] == '*')         // map leading * back to . for able's use
            able_path_name[i] = '.';

    WipeNameExtension(able_path_name);
    
    // create the top level export directory
    sec_len   = scanned_size;                   // look up size for this folder
    cat_style = scanned_style;

    if (cat_style != 1 && cat_style != 4)                // better by 1 or 4; use lsubc if error
        cat_style = 4;

    if (sec_len < cat_style)                            // emergency recovery from system errors
        sec_len = cat_style;

    word_len = sec_len << 8;

    ms_len = (fixed) (sec_len >> 16);
    ls_len = (fixed) (sec_len  & 0xFFFF);
    words  = (fixed) (word_len & 0xFFFF);

    if (cat_style == 1)
        cat_type = t_subc;
    else
        cat_type = t_lsubc;

    // pass2: else if a merge is not desired, just create the destination folder
    if (!replace(able_path_name, cat_type, ms_len, ls_len, words, 0, true))
    {
        get_cat_code_message(c_status, er_mess);
        printf("AbleDiskTool: Could not create subcatalog '%s' for the following reason:\n", error_name);
        printf("   %s\n", er_mess);
        return (-1);
    }

    block_num = (((uint32) (uint16) (f_ms_sector & 0xFF)) << 16) | ((uint32) (uint16) f_ls_sector); // block num on W0:
    block_len = (((uint32) (uint16) (f_ms_length & 0xFF)) << 16) | ((uint32) (uint16) f_ls_length); // block len

    if (WriteAbleDisk(SCSI_id, zero_buf, block_num, cat_style))
        return (-1);

    strcpy(level_one_name, able_path_name);
    
    if (!enter_catalog(level_one_name, 0, true))    // set up so level 1 accesses that catalog
    {
        get_cat_code_message(c_status, er_mess);
        printf("AbleDiskTool: Could not enter subcatalog '%s' for the following reason:\n", able_path_name);
        printf("   %s\n", er_mess);
        return (-1);
    }

    // Get mac OS file listing
    CSynclavierMutableArray* listing = GetListing(kCFURLEnumeratorSkipInvisibles, *the_spec->file_ref);
    
    if (mac_path_name[strlen(mac_path_name)-1] != '/')            // add colon to path name if none there
         strncat(mac_path_name, "/", sizeof(mac_path_name) - strlen(mac_path_name) - 1);

    strncat(able_path_name, ":", sizeof(able_path_name) - strlen(able_path_name) - 1);

    for (int i=0; i < listing->Count(); i++) {
        CFURLRef    childURL = (CFURLRef) listing->ItemAt(i);
        SyncFSSpec  temp_spec;
        char        this_mac_name  [512] = {""};
        char        this_able_name [512] = {""};

        // Skip size file
        CFStringRef name = NULL;
        CFURLCopyResourcePropertyForKey(childURL, kCFURLNameKey, &name, NULL);
        
        if (CFEqual(name, CFSTR("__SIZE__"))) {
            CFRelease(name);
            continue;
        }
        
        // Skip plist files
        if (CFStringHasSuffix(name, CFSTR(".plist")))
            continue;
        
        CFRelease(name);

        // Create ref for child entry
        CSynclavierFileReference expRef = CSynclavierFileReference(childURL);
        
        expRef.Resolve();
        
        if (!expRef.Reachable()) {
            printf("AbleDiskTool: Could not get specific information on '%s'\n", error_name);
            return (-1);
        }

        expRef.CreateFSSpec(&temp_spec);    // Note we don't release it since expRef is a local variable
        
        // Get file name for export
        strncpy(this_mac_name,  mac_path_name,  sizeof(this_mac_name ));    // starting name
        strncpy(this_able_name, able_path_name, sizeof(this_able_name));

        strncat(this_mac_name,  temp_spec.file_name, sizeof(this_mac_name)  - strlen(this_mac_name) - 1);
        strncat(this_able_name, temp_spec.file_name, sizeof(this_able_name) - strlen(this_able_name) - 1);

        WipeNameExtension(this_able_name);

        // If is a file, then export just that one file
        if (!expRef.IsDirectory())
        {
            if (ExportMacFile(&temp_spec, this_mac_name,
                              this_able_name, this_able_name, 1))
            {
                SyncFSSpecRelease(&temp_spec);
                return (-1);
            }
        }

        // Handle export of mac folder
        else
        {
            // Make sure destination path name if valid
            if (!locate(export_able_name, 0, true))        // see if destination directory exists
            {
                if (c_status != e_no_file)                // if doesn't exist but not because file-not-found
                {
                    get_cat_code_message(c_status, er_mess);
                    printf("AbleDiskTool: Can't export '%s' as '%s'\nhere's why:\n", export_mac_entity_name, export_able_name);
                    printf("   %s\n", er_mess);
                    
                    return (-1);
                }
            }
            
            if (ExportMacFolder(&temp_spec, this_mac_name,
                                this_able_name, this_able_name, 1))
            {
                SyncFSSpecRelease(&temp_spec);
                return (-1);
            }
            
            // Enter back to this level
            if (!enter_catalog(level_one_name, 0, true))    // set up so level 1 accesses that catalog
            {
                get_cat_code_message(c_status, er_mess);
                printf("AbleDiskTool: Could not enter subcatalog '%s' for the following reason:\n", able_path_name);
                printf("   %s\n", er_mess);
                return (-1);
            }
        }
        
        SyncFSSpecRelease(&temp_spec);
    }

    delete listing;
    
	return (0);
}

        
/*--------------------------------------------------------------------------*/
/*	CreateAndOpenAbleSimFile, OpenAbleSimFile   							*/
/*--------------------------------------------------------------------------*/

/* Open or Create a file on the current volume called  "*Able Disk Image"	*/
/* for simulating an Able disk on the host system							*/

ulong		AbleSimFileSize      = 32*1024*1024;
char		AbleSimFileName[512] = "*Able Disk Image";
CFStringRef AbleSimFileCFString  = NULL;

static int CreateAndOpenAbleSimFile()
{
	OSErr		FSstatus = noErr;
	long long   file_size;
	ulong		count;
	ulong		position;
	ulong		length;
	uint16		*in_buf = NULL;
	handle		in_buf_handle = NULL;
	
	if (AbleSimFileSize == 0)
	{
		printf("AbleDiskTool: Cannot specify size of zero for image file '%s'\n", AbleSimFileName);
		return (-1);
	}
	
	if (AbleSimFileSize > 2000*1024*1024)
	{
		printf("AbleDiskTool: Specified image file size is too large; limit is 2000 Megabytes\n");
		return (-1);
	}

	if (AbleSimFileSize < 2048)
	{
		printf("AbleDiskTool: Specified image file size is too small; limit is 8 sectors\n");
		return (-1);
	}
    
    CSynclavierFileReference* simRef = new CSynclavierFileReference(AbleSimFileCFString);
    
    // Ownership is now in the simRef
    AbleSimFileCFString = nullptr;

	// Delete prior image file
    if (simRef->Reachable())
        simRef->Delete();
    
    if (simRef->Reachable()) {
        printf("AbleDiskTool: Could not replace prior disk image file '%s' (%d)\n", AbleSimFileName, FSstatus);
        simRef->Release();
        return (-1);
	}

    file_size = AbleSimFileSize;

    if (((FSstatus = simRef->MkPath()) != 0)
    ||  ((FSstatus = simRef->Create('SUBC', 'SNCL', 0, file_size)) != 0)
    ||  ((FSstatus = simRef->Open(O_RDWR)) != 0))
	{
		printf("AbleDiskTool: Could not open the new disk image file '%s' (%d)\n", AbleSimFileName, FSstatus);
        simRef->Delete();
        simRef->Release();
		return (-1);
	}
	
	count = sizeof(zero_buf);
	    
	// No swap needed - writing zeroes
	if (((FSstatus = simRef->Write((SInt32 *) &count, (void *) zero_buf)) != 0)
	||  (count != sizeof(zero_buf)))
	{
		printf("AbleDiskTool: Could not initialize the new disk image file '%s' (%d)\n", AbleSimFileName, FSstatus);
        simRef->Close();
        simRef->Delete();
        simRef->Release();
		return (-1);
	}
			
	if (zero_new_disk_file)
	{
		in_buf_handle = get_big_memory(MAX_TEXT_SIZE);
		
		if (in_buf_handle)
			in_buf = (uint16 *) *in_buf_handle;

		if (!in_buf_handle || !in_buf)
		{
			printf("AbleDiskTool: Could not get memory for zeroing\n");
            simRef->Close();
            simRef->Delete();
            simRef->Release();
			return (-1);
		}

		zero_mem((byte *) in_buf, MAX_TEXT_SIZE);
		
		if ((FSstatus = simRef->Seek(0)) != 0)
		{
			printf("AbleDiskTool: Could not reset the new disk image file for zeroing'%s' (%d)\n", AbleSimFileName, FSstatus);
			free_big_memory(in_buf_handle);
            simRef->Close();
            simRef->Delete();
            simRef->Release();
			return (-1);
		}
		
		file_size = AbleSimFileSize;
		position  = 0;
				
		while (position < file_size)
		{
			length = file_size - position;

			if (length > MAX_TEXT_SIZE) length = MAX_TEXT_SIZE;
			
			count = length;
			
			// No swap needed - writing Zeroes
			if (((FSstatus = simRef->Write((SInt32 *) &count, (void *) in_buf)) != 0)
			||  (count != length))
			{
				printf("AbleDiskTool: Failed trying to zero out new disk image file '%s' (%d)\n", AbleSimFileName, FSstatus);
				free_big_memory(in_buf_handle);
                simRef->Close();
                simRef->Delete();
                simRef->Release();
				return (-1);
			}
			
			position += length;
		}
		
		free_big_memory(in_buf_handle);
	}	
	
    simRef->CreateFSSpec(&AbleSimFSSpec);
    simRef->Release();
    
	return (0);
}

static int OpenAbleSimFile()
{
    if (AbleSimFSSpec.file_ref)                        /* make sure not opened twice!        */
        {printf("OpenAbleSimFile: Double Call!!\n"); return (-1);}

	OSErr	FSstatus;

    CSynclavierFileReference* simRef = new CSynclavierFileReference(AbleSimFileCFString);
    
    // Ownership is now in the simRef
    AbleSimFileCFString = nullptr;

    simRef->Resolve();

    if ((FSstatus = simRef->Open(O_RDWR)) != 0)
	{
		printf("AbleDiskTool: Could not open the new disk image file '%s' (%d)\n", AbleSimFileName, FSstatus);

		if (FSstatus == opWrErr || FSstatus == permErr)
		{
			printf("   Looks like the disk image file is being used by another application\n");
			printf("   You must Quit SynclavierX before you may access '%s' with AbleDiskTool\n", AbleSimFileName);
		}

        simRef->Release();
        
		return (-1);
	}

    simRef->CreateFSSpec(&AbleSimFSSpec);
    simRef->Release();
    
	return (0);
}


/*--------------------------------------------------------------------------*/
/*	WriteAbleDisk															*/
/*--------------------------------------------------------------------------*/

/* Writes an Able SCSI disk */

static int WriteAbleDisk(uint16 scsi_id, uint16 *buffer, uint32 block_num, uint32 block_len)
{
	scsi_device		*our_device;
	scsi_error_code status;
	
	if (scsi_id >= 7)
		{printf("WriteAbleDisk: Bad SCSI Id configuration (%d)\n", scsi_id); return (-1);}
	
	our_device = &g_scsi_device_data_base[scsi_id];
	
	if ((scsi_id != our_device->fTargetId)
	||  (our_device->fDeviceType == DEVICE_MACINTOSH_DISK))
	{
		printf("WriteAbleDisk: Not a Synclavier disk: %2d\n", scsi_id);
		return (-1);
	}
	
	if ((status = issue_write_extended(our_device, (byte *) buffer, block_num, block_len)) != 0)
		{printf("WriteAbleDisk: Failed issue_write_extended (%d)\n", status); return (-1);}

	return (0);
}


/*--------------------------------------------------------------------------*/
/*	ExportAbleDataFile														*/
/*--------------------------------------------------------------------------*/

static int ExportAbleDataFile(uint16 scsi_id, uint32 block_num, uint32 num_words, CSynclavierFileReference& our_ref)
{
	uint32	 	i,j,k;
	handle		in_buf_handle;
	uint16		*in_buf = NULL;
	OSErr		FSstatus;
	int			count;
	uint32		chunk_blocks = MAX_TEXT_SIZE/512;
	
	block_num = block_num & 0x00FFFFFF;
	
	in_buf_handle = get_big_memory(MAX_TEXT_SIZE);
	
	if (in_buf_handle)
		in_buf = (uint16 *) *in_buf_handle;
		
	if (!in_buf_handle || !in_buf)
		{printf("ExportAbleDataFile: out of memory for data file buffer\n"); free_big_memory(in_buf_handle); return (-1);}
		
	i = 0;									/* init word position			*/
	
	while (i < num_words)
	{
		count = num_words - i;				/* number of  bytes to read		*/
		j = (count + 255) >> 8;				/* get number of blocks			*/

		if (j > chunk_blocks)				/* limit to buffer size			*/
		{
			j = chunk_blocks;
			count = j << 8;
		}
		
		if ((FSstatus = our_ref.Seek(i << 1)) != 0)
			{printf("ExportAbleDataFile: Failed SetFPos (%d)\n", FSstatus); free_big_memory(in_buf_handle); return (-1);}
		
		count = count << 1;					/* number of  bytes to read	*/
		k     = count;
		
		if (((FSstatus = XPLRunTime_FSRead(our_ref.GetFile(), &count, (void *)in_buf)) != 0)
		||  (count != k))
			{printf("ExportAbleDataFile: Failed FSRead (%d)\n", FSstatus); free_big_memory(in_buf_handle); return (-1);}
	
		if (WriteAbleDisk(scsi_id, in_buf, block_num + (i >> 8), j))
			{printf("ExportAbleDataFile: Failed WriteAbleDisk\n"); free_big_memory(in_buf_handle); return (-1);}
	
		i += (j << 8);						/* compute new word position	*/
		
		run_host_environment_250();
	}
	
	free_big_memory(in_buf_handle);
	
	in_buf_handle = NULL; in_buf = NULL;
	
	return (0);
}


/*--------------------------------------------------------------------------*/
/*	Scan__SIZE__File														*/
/*--------------------------------------------------------------------------*/

/* Scans the subdirectory size information from a __SIZE__ file				*/

static void Scan__SIZE__File(uint32 byte_len, CSynclavierFileReference& our_ref, const char *error_name,
							 uint32 *scanned_size, uint32 *scanned_style)
{
	char	line		[512] = {""};
	char	subcat_size	[512] = {""};
	OSErr	FSstatus;
	long	count;
	int		i;
	
	*scanned_size  = 0;
	*scanned_style = 0;
	
	if (byte_len > sizeof(line))
		{printf("Scan__SIZE__File: File \"%s\" is too long\n", error_name); return;}
	
	if ((FSstatus = our_ref.Seek(0)) != 0)
		{printf("Scan__SIZE__File: Failed SetFPos for \"%s\" (%d)\n", error_name, FSstatus); return;}
	
	count = byte_len;
	
	// No swap needed since we are reading a text file here
	if (((FSstatus = our_ref.Read((SInt32 *) &count, (void *)line)) != 0)
	||  (count != byte_len))
		{printf("Scan__SIZE__File: Failed FSRead for \"%s\" (%d)\n", error_name, FSstatus); return;}

	line[sizeof(line)-1] = 0;
	
	sscanf(line, "%s %i", (char *) &subcat_size, scanned_size);

	for (i=0; i<strlen(subcat_size); i++)
		subcat_size[i] = toupper(subcat_size[i]);
		
	if ((strcmp(subcat_size, "LARGE" ) == 0)
	||  (strcmp(subcat_size, "LARGE,") == 0))
		*scanned_style = 4;

	else if ((strcmp(subcat_size, "SMALL" ) == 0)
	||       (strcmp(subcat_size, "SMALL,") == 0))
		*scanned_style = 1;
}


/*--------------------------------------------------------------------------*/
/*	ComputeWordLengthOfMacTextFile											*/
/*--------------------------------------------------------------------------*/

/* returns Able word length of a Mac text file */

static uint32 ComputeWordLengthOfMacTextFile(uint32 byte_len, CSynclavierFileReference& our_ref, const char *error_name)
{
	uint32	 	i,j;
	uint32		out_words;
	char		*in_buf = NULL;
	handle		in_buf_handle = NULL;
	int			count;
	OSErr		FSstatus;
	boolean		saved;
	boolean		need_eol;
	boolean		need_sol;
    int         last_ch;
		
	if (byte_len > MAX_TEXT_SIZE)
		{printf("ComputeWordLengthOfMacTextFile: File \"%s\" is too big to export\n", error_name); return (uint32) (-1);}
	
	in_buf_handle = get_big_memory(MAX_TEXT_SIZE);
	
	if (in_buf_handle)
		in_buf = (char *) *in_buf_handle;
		
	if (!in_buf_handle || !in_buf)
		{printf("ComputeWordLengthOfMacTextFile: out of memory for file length computation\n"); free_big_memory(in_buf_handle); return (uint32) (-1);}
		
	if ((FSstatus = our_ref.Seek(0)) != 0)
		{printf("ComputeWordLengthOfMacTextFile: Failed SetFPos for \"%s\" (%d)\n", error_name, FSstatus); free_big_memory(in_buf_handle); return (uint32) (-1);}
	
	count = byte_len;
	long lcount = count;
	
	// No byte swap here - reaading text file and processing bytes
	if (((FSstatus = our_ref.Read((SInt32 *) &lcount, (void *)in_buf)) != 0)
	||  (lcount != byte_len))
		{printf("ComputeWordLengthOfMacTextFile: Failed FSRead for \"%s\" (%d)\n", error_name, FSstatus); free_big_memory(in_buf_handle); return (uint32) (-1);}

	out_words = 0;
    saved     = false;
	need_sol  = true;
	need_eol  = false;
    last_ch   = 0;

	for (i = 0; i < byte_len; i++, last_ch = j) /* process export bytes	    */
	{
		j = in_buf[i] & 0xFF;				/* get the character			*/
		
		if (need_sol)						/* emit start-of-line if req'd	*/
		{
			out_words++;					/* line number space			*/
			need_sol = false;
		}
		
        if (j == MAC_OS_FILE_RETURN         /* character is new_line        */
        ||  j == MAC_OS_FILE_NEWLINE)
		{
            // Ignore linefeed after carriage return
            if (last_ch == MAC_OS_FILE_RETURN && j == MAC_OS_FILE_NEWLINE)
                continue;
            
			out_words++;					/* saved byte or all nulls..	*/
			
			saved    = false;				/* no byte saved up				*/
			need_eol = false;
			need_sol = true;				/* start of line for next char	*/
		}
		else
		{
			need_eol = true;				/* will need eol if missing		*/
			
			if (!saved)						/* save up first byte			*/
				saved = true;
			else
			{
				out_words++;
				saved = false;
			}
		}
	}

	if (saved)								/* emit last byte				*/
		out_words++;
	else if (need_eol)
		out_words++;						/* or end of line if none provided */
		
	free_big_memory(in_buf_handle);
	
	in_buf_handle = NULL; in_buf = NULL;
	
	return (out_words);
}


/*--------------------------------------------------------------------------*/
/*	ComputeSoundFileSize											        */
/*--------------------------------------------------------------------------*/

/* returns Able word length of a Mac sound file */

static uint32 ComputeSoundFileSize(CSynclavierFileReference& our_ref, const char *error_name, uint32 type)
{
	SynclSFHeader		header;
	AudioDataDescriptor	descriptor;

	if (type == AIFFID)
	{
		if (ParseAIFFSoundFile(our_ref, descriptor, header))
			{printf("ComputeSoundFileSize: File \"%s\" cannot be analyzed\n", error_name); return (uint32) (-1);}
	}
	
	else if (type == WAVEID)
	{
		if (ParseWAVESoundFile(our_ref, descriptor, header))
			{printf("ComputeSoundFileSize: File \"%s\" cannot be analyzed\n", error_name); return (uint32) (-1);}
	}
	
	else
		{printf("ComputeSoundFileSize: File \"%s\" cannot be converted\n", error_name); return (uint32) (-1);}

	uint32 out_words = (header.total_data.sector << 8) + (3*256);

	return (out_words);
}


/*--------------------------------------------------------------------------*/
/*	ExportSoundFileFile											        */
/*--------------------------------------------------------------------------*/

static int ExportSoundFileFile(uint16 scsi_id, uint32 block_num, CSynclavierFileReference& our_ref, short res_ref_num, uint32 type)
{
	SynclSFHeader		header;
	AudioDataDescriptor	descriptor;
	char				*in_buf  = NULL;
	uint16				*out_buf = NULL;
	handle				in_buf_handle  = NULL;
	handle				out_buf_handle = NULL;
	OSErr				FSstatus;

	if (type == AIFFID)
	{
		if (ParseAIFFSoundFile(our_ref, descriptor, header))
			return (-1);
	}
	
	else if (type == WAVEID)
	{
		if (ParseWAVESoundFile(our_ref, descriptor, header))
			return (-1);
	}
	
	else
		{printf("ExportSoundFileFile: Missing translator\n"); return (-1);}

	uint32 out_words = (header.total_data.sector << 8) + (3*256);

	block_num = block_num & 0x00FFFFFF;
	
	in_buf_handle  = get_big_memory(MAX_TEXT_SIZE);
	out_buf_handle = get_big_memory(MAX_TEXT_SIZE);
	
	if (in_buf_handle)
		in_buf = (char *) *in_buf_handle;
		
	if (out_buf_handle)
		out_buf = (uint16 *) *out_buf_handle;
		
	if (!in_buf_handle || !in_buf || !out_buf_handle || !out_buf)
		{printf("ExportSoundFileFile: out of memory for sound file export\n"); free_big_memory(in_buf_handle); free_big_memory(out_buf_handle); return (-1);}
		
	if ((FSstatus = our_ref.Seek(descriptor.start_pos_in_file)) != 0)
		{printf("ExportSoundFileFile: Failed SetFPos (%d)\n", FSstatus); free_big_memory(in_buf_handle); free_big_memory(out_buf_handle); return (-1);}
	
	// Copy and synthesize file as needed
	long	write_ptr = 0;				// word pointer to write file
	long	read_ptr  = 0;				// byte pointer to read file (data area only)
	long	samples_to_copy  = descriptor.frames_per_file * descriptor.samples_per_frame;
	long    bytes_to_copy    = descriptor.byte_len_in_file;
	long    bytes_per_sample = descriptor.bits_per_sample / 8;

	// Write entire file
	while (write_ptr < out_words)
	{
		long	chunk_words = out_words - write_ptr;			// assume rest of file will fit in this buffer
		long	write_where = 0;								// init word pointer into output bufer
		long	chunk_left;

		if (chunk_words > (MAX_TEXT_SIZE>>1))					// limit to buffer size
			chunk_words = MAX_TEXT_SIZE>>1;
		
		chunk_left = chunk_words;								// number of samples needed to fill out this chunk

		// Stuff sound file header into output buffer.  Here is the 1 place we advance read pointer without advancing write pointer
		if ((read_ptr == 0) && ((write_ptr + write_where) == 0))
		{
			memcpy(out_buf, &header, 3*256*2);					// copy sound file header all bytes
			
			write_where += (3*256);								// advance word pointer to output buffer; samples will go here
			chunk_left  -= (3*256);								// reduce samples needed to fill buffer
		}
		
		// Get and expand audio data
		while (chunk_left)										// fill out chunk's worth of data (to end of buffer or end of file)
		{
			// Fetch audio data if any
			if (samples_to_copy != 0)							// get more samples if any.  Note we (gracefully) stop if we run out of data in the file.
			{
				long	samples_to_read = chunk_left;						// assume all data needed in output buffer will fit in read buffer
				long	bytes_of_data   = samples_to_read * bytes_per_sample;
				
				if (bytes_of_data > MAX_TEXT_SIZE)							// limit to what will fit in read buffer
				{
					samples_to_read = MAX_TEXT_SIZE   / bytes_per_sample;	// watch for round-down due to 3 bytes per sample
					bytes_of_data   = samples_to_read * bytes_per_sample;
				}

				if (samples_to_read > samples_to_copy)						// also limit to samples left to copy
				{
					samples_to_read = samples_to_copy;						// assume 16-bit source data and all will fit
					bytes_of_data   = samples_to_read * bytes_per_sample;
				}
				
				if (bytes_of_data > bytes_to_copy)							// also limit to valid
				{
					samples_to_read = bytes_to_copy   / bytes_per_sample;	// watch for round-down due to 3 bytes per sample
					bytes_of_data   = samples_to_read * bytes_per_sample;
				}
					
				if (samples_to_read == 0)									// detect weird case of not enough bytes to finish last sample
				{
					samples_to_copy = 0;
					bytes_to_copy   = 0;
					continue;
				}
				
				else
				{
					int count = bytes_of_data;
					
					// Read 16-bit big-endian data directly into output buffer to avoid copy
					if (bytes_per_sample == 2 && !descriptor.bytes_need_swizzling)
					{
						if (((FSstatus = XPLRunTime_FSRead(our_ref.GetFile(), &count, (void *)&out_buf[write_where])) != 0)
						||  (count != bytes_of_data))
							{printf("ExportSoundFileFile: Failed FSRead (%d)\n", FSstatus); free_big_memory(in_buf_handle); free_big_memory(out_buf_handle); return (-1);}
					}
					
					// Else read and tweak the bytes
					else
					{
						long  loop;
						char  *source_chars;
						fixed *source_fixeds;
						long  *source_longs;
						fixed *dest_data;
						
						if (((FSstatus = XPLRunTime_FSRead(our_ref.GetFile(), &count, (void *)in_buf)) != 0)
						||  (count != bytes_of_data))
							{printf("ExportSoundFileFile: Failed FSRead (%d)\n", FSstatus); free_big_memory(in_buf_handle); free_big_memory(out_buf_handle); return (-1);}
					
						if (bytes_per_sample == 1 && !descriptor.bytes_need_swizzling)			// 8-bit Sd2f & AIFF
						{
							source_chars = (char  *) in_buf;
							dest_data    = (fixed *) &out_buf[write_where];
							loop = bytes_of_data;

							while (loop)
								{*dest_data++ = ((fixed) (*source_chars++)) << 8; loop--;}
						}
						
						else if (bytes_per_sample == 1 && !descriptor.bytes_need_swizzling)		// 8-bit WAVE
						{
							source_chars = (char  *) in_buf;
							dest_data    = (fixed *) &out_buf[write_where];
							loop = bytes_of_data;

							while (loop)
								{*dest_data++ = ((fixed) (*source_chars++ ^ 128)) << 8; loop--;}
						}
						
						else if (bytes_per_sample == 2)								// 16-bit swizzled (non_swizzled handled above)
						{
							source_fixeds = (fixed *) in_buf;
							dest_data     = (fixed *) &out_buf[write_where];
							loop = bytes_of_data / 2;

							while (loop)
								{*dest_data++ = Endian16_Swap(*source_fixeds); source_fixeds++; loop--;}
						}
						
						else if (bytes_per_sample == 3 && !descriptor.bytes_need_swizzling)				// 24-bit data
						{
							source_fixeds = (fixed *) in_buf;
							dest_data     = (fixed *) &out_buf[write_where];
							loop = bytes_of_data / 3;

							while (loop)
								{*dest_data++ = *source_fixeds; source_fixeds = (fixed *) (((char *) source_fixeds) + 3); loop--;}
						}
						else if (bytes_per_sample == 3 &&  descriptor.bytes_need_swizzling)
						{
							source_fixeds = (fixed *) (in_buf + 1);
							dest_data     = (fixed *) &out_buf[write_where];
							loop = bytes_of_data / 3;

							while (loop)
								{*dest_data++ = Endian16_Swap(*source_fixeds); source_fixeds = (fixed *) (((char *) source_fixeds) + 3); loop--;}
						}
						else if (bytes_per_sample == 4 && !descriptor.bytes_need_swizzling)
						{
							source_longs = (long  *) in_buf;
							dest_data    = (fixed *) &out_buf[write_where];
							loop = bytes_of_data / 4;

							while (loop)
								{*dest_data++ = *source_longs++ >> 16; loop--;}
						}
						else if (bytes_per_sample == 4 &&  descriptor.bytes_need_swizzling)
						{
							source_fixeds = (fixed *) (in_buf + 2);
							dest_data     = (fixed *) &out_buf[write_where];
							loop = bytes_of_data / 4;

							while (loop)
								{*dest_data++ = Endian16_Swap(*source_fixeds); source_fixeds = (fixed *) (((char *) source_fixeds) + 4); loop--;}
						}
					}
										
					samples_to_copy -= samples_to_read;				// few samples to copy from source file
					bytes_to_copy   -= bytes_of_data;				// few bytes of data available in source file
					read_ptr        += bytes_of_data;				// advance (unused) read_ptr position in source file
					write_where     += samples_to_read;				// advance where we write in output buffer
					chunk_left      -= samples_to_read;				// reduce samples left needed to fill output buffer
					
					continue;
				}
			}
			
			// Else synth zeroes to fill out last sector
			zero_mem((byte *) &out_buf[write_where], chunk_left << 1);
			write_where += chunk_left;
			chunk_left = 0;
		}
		
		if (WriteAbleDisk(scsi_id, out_buf, block_num + (write_ptr >> 8), chunk_words >> 8))
			{printf("ExportSoundFileFile: failed WriteAbleDisk\n"); free_big_memory(in_buf_handle); free_big_memory(out_buf_handle); return (-1);}
		
		write_ptr += chunk_words;

		run_host_environment_250();
	}

	free_big_memory(in_buf_handle );
	free_big_memory(out_buf_handle);
	in_buf_handle  = NULL; in_buf  = 0;
	out_buf_handle = NULL; out_buf = 0;
	
	return (0);
}


/*--------------------------------------------------------------------------*/
/*	ExportAbleTextFile														*/
/*--------------------------------------------------------------------------*/

static int ExportAbleTextFile(uint16 scsi_id, uint32 block_num, uint32 byte_len, CSynclavierFileReference& our_ref, uint32 word_len)
{
	uint32	 	i,j;
	uint32		out_words;
	char		*in_buf  = NULL;
	uint16		*out_buf = NULL;
	handle		in_buf_handle  = NULL;
	handle		out_buf_handle = NULL;
	long		count;
	OSErr		FSstatus;
	boolean		saved;
	fixed		the_word;
	boolean		need_eol;
	boolean		need_sol;
    int         last_ch;
	fixed		line_no = 1;
	
	block_num = block_num & 0x00FFFFFF;
	
	if (byte_len > MAX_TEXT_SIZE)
		{printf("ExportAbleTextFile: File too big to export\n"); return (-1);}
	
	in_buf_handle  = get_big_memory(MAX_TEXT_SIZE);
	out_buf_handle = get_big_memory(MAX_TEXT_SIZE);
	
	if (in_buf_handle)
		in_buf = (char *) *in_buf_handle;
		
	if (out_buf_handle)
		out_buf = (uint16 *) *out_buf_handle;
		
	if (!in_buf_handle || !in_buf || !out_buf_handle || !out_buf)
		{printf("ExportAbleTextFile: out of memory for text file export\n"); free_big_memory(in_buf_handle); free_big_memory(out_buf_handle); return (-1);}
		
	if ((FSstatus = our_ref.Seek(0)) != 0)
		{printf("ExportAbleTextFile: Failed SetFPos (%d)\n", FSstatus); free_big_memory(in_buf_handle); free_big_memory(out_buf_handle); return (-1);}
	
	count = byte_len;
	
	// No byte swap needed - reading tex file here
	if (((FSstatus = our_ref.Read((SInt32 *) &count, (void *)in_buf)) != 0)
	||  (count != byte_len))
		{printf("ExportAbleTextFile: Failed FSRead (%d)\n", FSstatus); free_big_memory(in_buf_handle); free_big_memory(out_buf_handle); return (-1);}

	out_words = 0;
    saved     = false;
	need_sol  = true;
    need_eol  = false;
    last_ch   = 0;
	the_word  = 0;
		
	for (i = 0; i < byte_len; i++, last_ch = j) /* process export bytes	    */
	{
		j = in_buf[i] & 0xFF;				/* get the character			*/
		
		if (need_sol)						/* emit start-of-line if req'd	*/
		{
			out_buf[out_words++] = line_no++;
			need_sol = false;
		}
		
        if (j == MAC_OS_FILE_RETURN         /* character is new_line        */
        ||  j == MAC_OS_FILE_NEWLINE)
		{
            // Ignore linefeed after carriage return
            if (last_ch == MAC_OS_FILE_RETURN && j == MAC_OS_FILE_NEWLINE)
                continue;

            if (saved)						/* saved up byte: emit it!		*/
				out_buf[out_words++] = the_word;
			else							/* else emit null word			*/
				out_buf[out_words++] = 0;
			
			saved    = false;				/* no byte saved up				*/
			need_eol = false;
			need_sol = true;				/* start of line for next char	*/
		}
		else
		{
			need_eol = true;				/* will need eol if missing		*/
			
			if (!saved)						/* save up first byte			*/
				{saved = true; the_word = (fixed) j;}
			else
			{
				out_buf[out_words++] = the_word | shl(j,8);
				saved = false;
			}
		}
		
		if ((out_words << 1) >= MAX_TEXT_SIZE-100)
			{printf("ExportAbleTextFile: File too big to export\n"); free_big_memory(in_buf_handle); free_big_memory(out_buf_handle); return (-1);}
	}
	
	if (saved)								/* emit last byte				*/
		out_buf[out_words++] = the_word;
	else if (need_eol)
		out_buf[out_words++] = 0;
	
	if (out_words != word_len)
		{printf("ExportAbleTextFile: Phase error with text file length\n"); free_big_memory(in_buf_handle); free_big_memory(out_buf_handle); return (-1);}

	if (WriteAbleDisk(scsi_id, out_buf, block_num, (out_words + 255) >> 8))
		{printf("ExportAbleTextFile: failed WriteAbleDisk\n"); free_big_memory(in_buf_handle); free_big_memory(out_buf_handle); return (-1);}

	free_big_memory(in_buf_handle );
	free_big_memory(out_buf_handle);
	in_buf_handle  = NULL; in_buf  = 0;
	out_buf_handle = NULL; out_buf = 0;
	
	return (0);
}


/*--------------------------------------------------------------------------*/
/*    main() for AbleDiskTool                                                    */
/*--------------------------------------------------------------------------*/

static  CSynclavierMutableArray& gListingProperties() {
    static CSynclavierMutableArray* listingArray;
    
    if (listingArray == NULL)
        listingArray = new CSynclavierMutableArray(10);
    
    return (*listingArray);
}

#if MAC_OS_X_VERSION_MIN_REQUIRED==MAC_OS_X_VERSION_10_6
    #define kCFURLEnumeratorDirectoryPostOrderSuccess 4
#endif

static CSynclavierMutableArray* GetListing(CFOptionFlags flags, CSynclavierFileReference& dirRef) {
    CSynclavierMutableArray& listing = * new CSynclavierMutableArray(100);
    
    // Grab mutex once to check things. Then mutex is not needed for directory construction.
    {
        CSynclavierMutableArray& listingProperties = gListingProperties();
        
        dirRef.Resolve();
    
        if (!dirRef.Reachable() || !dirRef.IsDirectory())
            return &listing;
    
        // Build - once - list of properties to prefetch
        if (listingProperties.Count() == 0) {
            listingProperties.PushLast(kCFURLNameKey);
            listingProperties.PushLast(kCFURLLocalizedNameKey);
            listingProperties.PushLast(kCFURLCreationDateKey);
            listingProperties.PushLast(kCFURLContentModificationDateKey);
            listingProperties.PushLast(kCFURLTypeIdentifierKey);
            listingProperties.PushLast(kCFURLLocalizedTypeDescriptionKey);
            listingProperties.PushLast(kCFURLFileSizeKey);
            listingProperties.PushLast(kCFURLIsDirectoryKey);
            listingProperties.PushLast(kCFURLIsAliasFileKey);
        }
    }
    
    CFURLEnumeratorRef enumerator = CFURLEnumeratorCreateForDirectoryURL(NULL, dirRef.GetURL(), flags, gListingProperties());
    
    if (enumerator) {
        CFURLRef              childURL         = NULL;
        CFURLEnumeratorResult enumeratorResult = kCFURLEnumeratorEnd;
        
        enumeratorResult = CFURLEnumeratorGetNextURL(enumerator, (CFURLRef *)&childURL, NULL);

        while (enumeratorResult == kCFURLEnumeratorSuccess || enumeratorResult == kCFURLEnumeratorDirectoryPostOrderSuccess) {
            #if 0   // Could print
            {
                CFStringRef name = NULL;
                CFStringRef type = NULL;
                CFStringRef ltyp = NULL;
                CFNumberRef size = NULL;
                CFBooleanRef isDirectory = NULL;
                CFBooleanRef isAlias     = NULL;
                
                long long   flen = 0;
                
                CFURLCopyResourcePropertyForKey(childURL, kCFURLNameKey, &name, NULL);
                CFURLCopyResourcePropertyForKey(childURL, kCFURLTypeIdentifierKey, &type, NULL);
                CFURLCopyResourcePropertyForKey(childURL, kCFURLLocalizedTypeDescriptionKey, &ltyp, NULL);
                CFURLCopyResourcePropertyForKey(childURL, kCFURLFileSizeKey, &size, NULL);
                CFURLCopyResourcePropertyForKey(childURL, kCFURLIsDirectoryKey, &isDirectory, NULL);
                CFURLCopyResourcePropertyForKey(childURL, kCFURLIsAliasFileKey, &isAlias, NULL);
                
                if (size)
                    CFNumberGetValue (size, kCFNumberSInt64Type, &flen);
                
                printf("Listing: Name %s Type %s Ltyp %s Blok %lld\n", SyncCFStringUtilitiesTempString(name), SyncCFStringUtilitiesTempString(type), SyncCFStringUtilitiesTempString(ltyp), flen);
                
                if (name) CFRelease(name);
                if (type) CFRelease(type);
                if (ltyp) CFRelease(ltyp);
                if (size) CFRelease(size);
                if (size) CFRelease(isDirectory);
                if (size) CFRelease(isAlias);
            }
            #endif
            
            CFRetain(childURL);
            
            listing.PushLast(childURL);

            enumeratorResult = CFURLEnumeratorGetNextURL(enumerator, (CFURLRef *)&childURL, NULL);
        }
    
        CFRelease(enumerator);
    }
    
    return &listing;
}


/*--------------------------------------------------------------------------*/
/*	main() for AbleDiskTool													*/
/*--------------------------------------------------------------------------*/

#define	MAX_LEVELS		20									// max # of folder levels
#define MAX_FOLDERS		5000   								// max # of folders

static	int		this_index[MAX_LEVELS];						// current index for this level
static	uint32	this_size [MAX_LEVELS];						// accumulated size of this level
static	uint32	spec_size [MAX_LEVELS];						// special size information available for this
static	uint32	spec_style[MAX_LEVELS];						// subcat style (large VS small) specified
static	long	this_dir  [MAX_LEVELS];						// current directory id being scanned
static	long	fldr_id   [MAX_LEVELS];						// which folder currently being scaned at this level
static	long	entries   [MAX_LEVELS];						// number of able entries on this level

static	uint32  fldr_size [MAX_FOLDERS];					// size needed for this subcatalog
static	char	fldr_style[MAX_FOLDERS];					// style to use for this subcatalog
static	long    fldr_ids  [MAX_FOLDERS];					// dir id for verification

static	fixed	cache_id = 0;
static	boolean	caching  = false;
static	boolean	map_set  = false;

static void clean_up()
{
	g_scsi_print_basic_opt = false;
	g_scsi_print_all_opt   = false;
		
	if (caching)
	{
		flush_cache(cache_id);							// flush cache before bumping back or exiting
		disable_cache(cache_id);						// disable further cacheing
		cache_treename(false);							// disallow further tree name caching
		caching = false;
	}
		
	AbleDiskToolNormalTermination = true;
    
    SyncFSSpecRelease(&AbleSimFSSpec);
        
	cleanup_run_time_environment();
    
    if (map_set) {		// Close image files
		XPLRunTime_CloseupSCSIMap();
        map_set = false;
    }

	initialize_static_data();

	if (g_disallow_run_host_exit)						// allow host exits if caller allows them
		g_disallow_run_host_exit--;
}

static void print_help()
{
	printf("AbleDiskTool: Version 3.0 dated 6/1/2020 (%lu)\n", sizeof(long));
	printf("Usage:  AbleDiskTool <command> <options>\n"); 
	printf("\n");
	printf("        -e mac_entity    syncl_name            export Macintosh entity to Synclavier Drive or Disk Image File\n");
	printf("        -w2                                    stop  on any error during folder export; default is to continue with next file\n");
	printf("        -create name size_bytes                create a new Disk Image File\n");
	printf("        -zero                                  zero out new disk Image file\n");
}

int AbleDiskTool( int argc, const char *argv[])
{
	int				argcount    = 0;
	scsi_device 	*our_device = NULL;
		
	g_disallow_run_host_exit++;								// disallow exits on break
	
	initialize_static_data();								// re-init static data on second execution
	
	AbleDiskToolNormalTermination = false;					// allow caller to detect abnormal termination
	
    // Static variable initialization
    SyncMutex::InitAll();

	if (initialize_run_time_environment (128*1024/256))		// single m128K card for our purposes
	{
		clean_up();
		return (-1);
	}
	
	initialize_able_catalog_routines();						// init translated catalog routines
	
	if (argc<=1)											// no args?
	{
		print_help();
		clean_up();
		return (-1);
	}

	for (argcount = 1; argcount < argc; )					// process args
	{
		const char *the_arg = argv[argcount];
		
		if (!the_arg)										// big time
		{
			clean_up();
			return (-1);
		}

		if (strcmp(the_arg, "") == 0)
		{
			argcount += 1;
			continue;
		}
		
		if ((strcmp(the_arg, "-s") == 0) && (argcount+1 < argc))
		{
			SCSI_id = (short) atol (argv[argcount+1]);
			argcount += 2;
			continue;
		}
		
		if ((strcmp(the_arg, "-file") == 0) && (argcount+1 < argc))
		{
			use_sim_file = true;
			
			strncpy(AbleSimFileName, argv[argcount+1], sizeof(AbleSimFileName));
			
            AbleSimFileCFString = CFStringCreateWithCString(NULL, AbleSimFileName, kCFStringEncodingUTF8);

			if (SCSI_id == 99)
				SCSI_id = 5;
				
			argcount += 2;
			continue;
		}
		
		if (strcmp(the_arg, "-w2") == 0)
		{
			argcount += 1;
			continue;
		}

		if ((strcmp(the_arg, "-create") == 0) && (argcount+2 < argc))
		{
			create_new_disk_file = true;

			strncpy(AbleSimFileName, argv[argcount+1], sizeof(AbleSimFileName));

            AbleSimFileCFString = CFStringCreateWithCString(NULL, AbleSimFileName, kCFStringEncodingUTF8);

            AbleSimFileSize = (ulong) atol (argv[argcount+2]);
			
			argcount += 3;
			
			if (SCSI_id == 99)
				SCSI_id = 5;
			
			continue;
		}

		if (strcmp(the_arg, "-zero") == 0)
		{
			zero_new_disk_file = true;
			
			argcount += 1;
			continue;
		}
		if ((strcmp(the_arg, "-e") == 0) && (argcount+2 < argc))
		{
			export_file = true;
			
			strncpy(export_mac_entity_name, argv[argcount+1], sizeof(export_mac_entity_name));
			strncpy(export_able_name,       argv[argcount+2], sizeof(export_able_name      ));
			argcount += 3;
			
			if (SCSI_id == 99)
				SCSI_id = 5;
				
			if (export_mac_entity_name[0] == 0 || export_able_name[0] == 0)
			{
				printf("Must specify mac_entity and able_name to export\n");
				clean_up();
				return (-1);
			}
			
			continue;
		}
		
		printf("Unrecognized command line argument '%s'\n", the_arg);
		print_help();
		clean_up();
		return (-1);
	}

	// Create sim file if needed

	if (create_new_disk_file)
	{
		if (CreateAndOpenAbleSimFile())
		{
			clean_up();
			return (-1);
		}

		if (SCSI_id >= 7)
		{
			printf("AbleDiskTool: Bad SCSI Id specified (%d)\n", SCSI_id);
			clean_up();
			return (-1);
		}
		
		our_device = &g_scsi_device_data_base[SCSI_id];
		zero_mem((byte *)our_device, sizeof(*our_device));

		our_device->fDevicePort      = SIMULATED_PORT;
		our_device->fIdentifyMessage = IDENTIFY_NO_DISC;
		our_device->fTargetId        = SCSI_id;
		our_device->fFRefNum		 = AbleSimFSSpec.file_ref->GetFile();
		our_device->fFSSpec			 = AbleSimFSSpec;
	}
	
	// Use sim file	if specified
	else if (use_sim_file)
	{
		if (OpenAbleSimFile())
		{
			clean_up();
			return (-1);
		}

		if (SCSI_id >= 7)
		{
			printf("AbleDiskTool: Bad SCSI Id specified (%d)\n", SCSI_id);
			clean_up();
			return (-1);
		}

		our_device = &g_scsi_device_data_base[SCSI_id];
		zero_mem((byte *)our_device, sizeof(*our_device));

		our_device->fDevicePort      = SIMULATED_PORT;
		our_device->fIdentifyMessage = IDENTIFY_NO_DISC;
		our_device->fTargetId        = SCSI_id;
		our_device->fFRefNum		 = AbleSimFSSpec.file_ref->GetFile();
		our_device->fFSSpec			 = AbleSimFSSpec;
        our_device->fAllowGrow       = true;
	}
	
	// Check for bad ID
	else
	{
		printf("AbleDiskTool: Bad SCSI Id specified (%d)\n", SCSI_id);
		clean_up();
		return (-1);
	}

    if (!our_device)
	{
		clean_up();
		return(-1);
	}
	
	
	// Now process individual commands
	
	// Set up device if using it for any data access
	if (export_file)
	{
		if (our_device->fTargetId < MAX_NUM_DEVICES)
			g_indexed_device[our_device->fTargetId] = our_device;

		g_recognize_disks = true;
		interrogate_device(our_device);
		g_recognize_disks = false;
				
		if (our_device->fDeviceType == DEVICE_DOES_NOT_EXIST)
		{
			printf("\nSCSI: No response from SCSI device %d; it is  not available\n", our_device->fTargetId);
			clean_up();
			return (-1);
		}
		
		if (our_device->fDeviceType == DEVICE_NOT_TALKING)
		{
			printf("\nSCSI: Unable to communicate with device\n");
			clean_up();
			return (-1);
		}
		
		if (our_device->fDeviceType == DEVICE_UNCOOPERATIVE_DISK)
		{
			printf("\nSCSI: Device is not ready for access\n");
			clean_up();
			return (-1);
		}
		
		if (our_device->fDeviceType == DEVICE_RESERVED_DISK)
		{
			printf("\nSCSI: Device is reserved for use by another computer at this time\n");
			clean_up();
			return (-1);
		}
        
        // Set up run time environment to access this able disk
        // Set up device as both W0 and W1 for our purposes since user is only accessing
        // that one device.  E.G. either W0: or W1: will access this device.

        configure_able_hard_drives (our_device->fTargetId, our_device->fTotCyl, our_device->fTotSec,
                                    our_device->fTargetId, our_device->fTotCyl, our_device->fTotSec);
    }
	
    if (export_file)
	{
        SyncFSSpec  temp_spec;
        
        // Open the export file
        CFStringRef               export_entity = CFStringCreateWithCString(NULL, export_mac_entity_name, kCFStringEncodingUTF8);
        CSynclavierFileReference* expRef        = new CSynclavierFileReference(export_entity);
        
        expRef->Resolve();
        
        if (!expRef->Reachable()) {
            printf("AbleDiskTool: Could not get specific information on '%s'\n", export_mac_entity_name);
            SyncFSSpecRelease(&temp_spec);
            clean_up();
            return (-1);
        }
        
        expRef->CreateFSSpec(&temp_spec);
        expRef->Release();
        
		// If is a file, then export just that one file
		if (!expRef->IsDirectory())
		{		
			if (ExportMacFile(&temp_spec, export_mac_entity_name,
			                  export_able_name, export_able_name, 0))
			{
                SyncFSSpecRelease(&temp_spec);
				clean_up();
				return (-1);
			}
		}

		// Handle export of mac folder
		else
		{
			// Make sure destination path name if valid
		    if (!locate(export_able_name, 0, true))		// see if destination directory exists
			{
				if (c_status != e_no_file)			    // if doesn't exist but not because file-not-found
				{	
					get_cat_code_message(c_status, er_mess);
					printf("AbleDiskTool: Can't export '%s' as '%s'\nhere's why:\n", export_mac_entity_name, export_able_name);
					printf("   %s\n", er_mess);
					
					clean_up();
					return (-1);
				}
			}
            
            if (ExportMacFolder(&temp_spec, export_mac_entity_name,
                                export_able_name, export_able_name, 0))
            {
                SyncFSSpecRelease(&temp_spec);
                clean_up();
                return (-1);
            }
		}
        
        SyncFSSpecRelease(&temp_spec);
	}
	
	// Done
	if (g_break_received)			// if we intercepted a break
	{
		clean_up();
		return (-1);
	}
	
	clean_up();						// else done
	return (0);
}
