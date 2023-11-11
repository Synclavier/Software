/* Able Disk Library */

/* Created - 7/14/96	C. Jones */

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

#define		MAX_TEXT_SIZE 			(256*1024)
#define		MAC_OS_FILE_EOLCHAR		(13)

// =================================================================================
//	Able String Utilities - AbleFileName2CStr
// =================================================================================

// Converts able file name stored in 4 fixeds into a C string
// --- watch for identical routine in able disk lib
static void AbleFileName2CStr(const uint16 *file_name, char *c_string)
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

static fixed CheckAbleFileName(const uint16 *file_name)
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

static fixed GetAbleFileType(OSType mac_type, OSType mac_creator, const FSSpec* its_spec)
{
	fixed export_type;
	
	// Give priority to file name extension
	int i = its_spec->name[0];
	
    if (i > 4)
    {
        const char* nameEnd = (const char *) & its_spec->name[i-4+1];
        
        if (MatchExtension(nameEnd, ".txt")) return t_text;
    }
    
	if (i > 5)
	{
		const char* nameEnd = (const char *) & its_spec->name[i-5+1];
		
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


// =================================================================================
//	Able Utilities - GetMacFileType
// =================================================================================

static void GetMacFileType(fixed file_type, OSType& out_mac_type, OSType& out_mac_creator)
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
	
	else
		{out_mac_creator = '\?\?\?\?'; out_mac_type = '\?\?\?\?';}
}


// =================================================================================
//	Able Utilities - Tweak Mac File Name
// =================================================================================

static	const char* Volumes = {"/Volumes/"};

static void TweakMacFileName(char * inName, char *volName)
{
	int	i = strlen(inName);
	int j = strlen(Volumes);
	int k = 0;
	
	// Remove /Volumes/ from front of file name if long enough. eg /Volumes/CJ Data/xyz to CJ Data:xyz
	if (i > j)
	{
		while (k < j && inName[k] == Volumes[k])
			k++;
	
		if (k == j)
		{
			while (j < i)
			{
				inName[j-k] = inName[j];
				j++;
			}
			
			inName[j-k] = 0;
			i = strlen(inName);
		}
	}
	
	// Substitute leading / with default volume name. eg /xyz to CJ Intel 10.4:xyz

	if (inName[0] == '/')
	{
		j = strlen(volName);
		k = i;
		
		while (k >= 0)
		{
			inName[k+j] = inName[k];
			k--;
		}
		
		k = j-1;
		while (k >= 0)
		{
			inName[k] = volName[k];
			k--;
		}
		
		i = strlen(inName);
	}
	
	// Change / to :
	k = 0;
	
	while (k < i)
	{
		if (inName[k] == '/')
			inName[k] = ':';

		k++;
	}
}


/*--------------------------------------------------------------------------*/
/*	Global Variables														*/
/*--------------------------------------------------------------------------*/

static	uint16	SCSI_id                    = 99; 
static	uint16	bus_id					   = MAC_SCSI_PORT;

static	boolean	use_sim_file			   = false;
static	boolean	use_ic2_setup			   = false;
static	boolean do_inquiry                 = false;
static	boolean do_test_unit	           = false;
static	boolean	import_scan           	   = false;
static	boolean	import_file           	   = false;
static	boolean	read_directory         	   = false;
static	boolean	export_file                = false;
static	boolean catalog      	           = false;
static	boolean dumpcontents 	           = false;
static	boolean recurs       	           = false;
static	boolean onlyaccess   	           = false;
static	boolean textonly		           = false;
static	boolean skipdots		           = false;
static	boolean quickerase                 = false;
static	boolean do_eject		           = false;
static  boolean progress_desired           = false;
static  boolean full_progress_desired      = false;
static	boolean truncate_names             = false;
static	boolean doctor_names               = false;
static	boolean don_t_over_write           = false;
static	boolean merge_but_warn             = false;
static	boolean merge_and_replace          = false;
static	boolean	merge_folder_contents      = false;
static	uint32  create_extra_space         = 0;
static  boolean stop_on_any_error          = false;
static  boolean verify_only                = false;
static  boolean ignore__size__             = false;
static	boolean	import_subcats_as_images   = false;
static	boolean import_toplevel_as_image   = false;
static	boolean	squeeze_images_on_transfer = false;
static	boolean	create_new_disk_file       = false;
static	boolean	zero_new_disk_file         = false;
static	boolean	allow_erase				   = false;
static	boolean	recognize_disks			   = false;
static	boolean	systemonly				   = false;
static	boolean	delete_after_xport		   = false;
static	boolean any_files_were_skipped     = false;
static	boolean replace_all_chosen         = false;
static	boolean report_unrecognized_files  = false;
static	boolean import_as_aiff             = false;
static	boolean import_as_sd2f             = false;
static	boolean import_as_wave             = false;
static	boolean	coerce_rates               = false;
static	boolean	do_rewrite                 = false;
static  boolean little_endian              = false;

static	char	export_mac_entity_name	[ 512] = {""};
static	char	export_able_name	    [ 512] = {""};
static	fixed	export_type;

static	char	import_able_entity_name	[ 512] = {""};
static	char	import_mac_name         [ 512] = {""};
static	fixed	import_type;

static	char    only_name  				[ 512] = {""};
static	char	level_one_name			[ 512] = {""};
static	char	log_file_name			[ 512] = {""};

static	ufixed	zero_buf [1024];

static	char   	er_mess[MESSAGE_BUF_SIZE];

static	short	AbleSimFileRefNum    = 0;
static	FSSpec	AbleSimFSSpec;
static	short	LogFileRefNum        = 0;
static	short	LogVolRefNum		 = 0;
static	FSSpec	LogFSSpec;
static	short	LogNeedsHeader		 = 0;

long	AbleDiskToolNormalTermination = false;
long	AbleDiskToolAllowPointers     = false;
void*	AbleDiskToolOutputData        = NULL;
long	AbleDiskToolOutputDataSize    = 0;
long	AbleDiskToolOutputDataEntries = 0;

static	struct PCI1AccessorStruct* gAbleAccessor;

static	interpreter_settings	InterpreterInterpreterSettings;
static	interchange_settings	InterchangeSettings;
static	interchange2_settings	Interchange2Settings;


/*--------------------------------------------------------------------------*/
/*	Reinitialize global data												*/
/*--------------------------------------------------------------------------*/

static void initialize_static_data()
{
	SCSI_id         	  		= 99;
	bus_id					    = MAC_SCSI_PORT;

	use_sim_file				= false;
	use_ic2_setup				= false;
	do_inquiry            		= false;
	do_test_unit	     	 	= false;
	import_scan   	      		= false;
	import_file   	      		= false;
	read_directory              = false;
	export_file   	      		= false;
	catalog      	      		= false;
	dumpcontents 	      		= false;
	recurs       	      		= false;
	onlyaccess   	      		= false;
	textonly		     	 	= false;
	skipdots		     	 	= false;
	quickerase           	 	= false;
	do_eject		     	 	= false;
	progress_desired     		= false;
	full_progress_desired		= false;
	truncate_names        		= false;
	doctor_names          		= false;
	don_t_over_write      		= false;
	merge_but_warn      		= false;
	merge_and_replace   		= false;
	merge_folder_contents		= false;
	create_extra_space   	 	= 0;
	stop_on_any_error     		= false;
	verify_only           		= false;
	ignore__size__		  		= false;
	import_subcats_as_images 	= false;
	import_toplevel_as_image 	= false;
	squeeze_images_on_transfer	= false;
	create_new_disk_file        = false;
	zero_new_disk_file			= false;
	allow_erase    				= false;
	recognize_disks				= false;
	systemonly					= false;
	delete_after_xport			= false;
	any_files_were_skipped      = false;
	replace_all_chosen          = false;
	report_unrecognized_files   = true;
	import_as_aiff              = false;
	import_as_sd2f              = false;
	import_as_wave              = false;
	coerce_rates                = false;
	do_rewrite					= false;
	
	export_mac_entity_name  [0] = 0;
	export_able_name        [0] = 0;
	export_type			        = 0;

	import_able_entity_name [0] = 0;
	import_mac_name         [0] = 0;
	import_type                 = 0;

	only_name  			    [0] = 0;
	level_one_name			[0] = 0;
	log_file_name			[0] = 0;

	g_scsi_print_basic_opt   = false;
	g_scsi_print_all_opt     = false;
}


/*--------------------------------------------------------------------------*/
/*	is_able_system_entity													*/
/*--------------------------------------------------------------------------*/

static boolean is_able_system_entity(const char *this_able_name)
{
	const char *system_files[] = {
		"W0:MONITOR",
		"W0:PROFILE",
		"W0:.SYSTEM",
		"W0:SYN-",
		"W1:MONITOR",
		"W1:PROFILE",
		"W1:.SYSTEM",
		"W1:SYN-",
		NULL
	};
	
	int i = 0;
	
	while (system_files[i])
	{
		if (strstr(this_able_name, system_files[i]) == this_able_name)
			return (true);
	
		i++;
	}
	
	return (false);
}


/*--------------------------------------------------------------------------*/
/*	DumpAbleCatalog															*/
/*--------------------------------------------------------------------------*/

/* Utility routine to dump an Able directory to standard output				*/

static void DumpAbleCatalog(const uint16 *buffer, int num_words, const char *cat_name_to_display)
{
	int 	i,j;
	char 	*bytes;
	uint32	start_block;
	uint32	block_len;
	uint32	end_block;
	uint32	largest_end_block = 0;
	
	printf("\nDump of: '%s'\n", cat_name_to_display);
	
	i = 0;
	while (i < num_words)
	{
		printf("   %4d: ",i);							/* print index		*/
		
		for (j=0; j<8; j++)								/* print 8 words	*/
			printf(" %04x", (uint32) (uint16) buffer[i+j]);
		
		printf(" ");									/* space			*/
		
		bytes = (char *) &buffer[i];					/* point to bytes	*/
		
		for (j=0; j<16; j++)							/* convert to ascii	*/
		{
			char theChar;
			
			#if __BIG_ENDIAN__
				if (j&1)								/* second byte is upper	*/
					theChar = bytes[j-1] & 0x7F;
				else
					theChar = bytes[j+1] & 0x7F;		/* first byte is lower	*/
			#else
				theChar = bytes[j] & 0x7F;
			#endif
			
			if ((theChar < ' ') || (theChar > 'z'))
				printf(".");
			else
				printf("%c", theChar);
		}
		
		printf(" ");									/* space			*/
		
		bytes = (char *) &buffer[i];					/* point to bytes	*/
		
		for (j=0; j<16; j++)							/* convert to ascii	*/
		{
			char theChar;
			
			theChar = bytes[j] & 0x7F;
				
			if ((theChar < ' ') || (theChar > 'z'))
				printf(".");
			else
				printf("%c", theChar);
		}
		
		printf("\n");
		
		start_block = ((uint32) buffer[i+4]) + ((((uint32) buffer[i+7]) & 0xFF00) <<  8);
		block_len   = ((uint32) buffer[i+5]) + ((((uint32) buffer[i+7]) & 0x00F0) << 12);
		end_block   = start_block + block_len;
		
		if (end_block > largest_end_block) largest_end_block = end_block;
		
		i += 8;
		
		if ((i&63) == 0)
		{
			run_host_environment_250();
			
			if (g_break_received)
				return;
		}
	}
	
	printf("   last block used: %d (%x)\n", largest_end_block, largest_end_block);
}


/*--------------------------------------------------------------------------*/
/*	IsVolumeName     														*/
/*--------------------------------------------------------------------------*/

// Sees if a macintosh folder name (e.g. ending in :) is actually a
// Mac volume.

static int IsVolumeName(const char *the_name, short *VRefnum)
{
	OSErr	FSstatus;
	FSSpec  temp_spec;
	char	pas_name[512];

	strncpy(pas_name, the_name, sizeof(pas_name));
	c2pstr(pas_name);

	FSstatus = FSMakeFSSpec (0, 0, (uint8 *) pas_name,  &temp_spec);

	if (FSstatus == noErr && temp_spec.parID == fsRtParID)
	{
		*VRefnum = temp_spec.vRefNum;
		return (true);
	}
	
	else
		return (false);
}


/*--------------------------------------------------------------------------*/
/*	Create__size__File														*/
/*--------------------------------------------------------------------------*/

/* Create__size__File creates a file in a newly created folder in			*/
/* which some information from the original able catalog structure is		*/
/* saved.  In particular, the original size of subcatalogs is stored		*/
/* therein.																	*/

static	char	SubcatalogInfoFileName [] = "__SIZE__";
static	char	SubcatalogInfoFileName1[] = "__size__";

static	void	Create__size__File(uint32 cat_blocks, short vref, long parid, uint32 block_len)
{
	OSErr	FSstatus;
	short	our_ref_num;
	
	char 	pas_name[64];
	char	info[64];
	
	uint32	file_size;
	uint32	count;
	
	if (block_len == 0)							// skip if no length available
		return;		

	if (cat_blocks != 1 && cat_blocks != 4)
		return;
		
	strncpy(pas_name, SubcatalogInfoFileName, sizeof(pas_name));
	c2pstr (pas_name);
		
	if ((FSstatus = HCreate(vref, parid, (uint8 *) pas_name, 'ttxt', 'TEXT')) != 0)
		return;
		
	if ((FSstatus = HOpen  (vref, parid, (uint8 *) pas_name, fsRdWrShPerm, &our_ref_num)) != 0)
		return;
	
	if (cat_blocks == 1)
		sprintf(info, "small, %d%c%c", block_len, MAC_OS_FILE_EOLCHAR, 0);

	else
		sprintf(info, "large, %d%c%c", block_len, MAC_OS_FILE_EOLCHAR, 0);

	file_size = strlen(info);					// size to allocate

	if ((FSstatus = SetFPos(our_ref_num, fsFromStart, 0)) != 0)
	{
		FSClose(our_ref_num);
		HDelete(vref, parid, (uint8 *) pas_name);
		return;
	}

	count = file_size;
	
	// No swap needed - writing a text file here.
	if (((FSstatus = FSWrite(our_ref_num, (SInt32 *) &count, (void *)info)) != 0)
	||  (count != file_size))
	{
		FSClose(our_ref_num);
		HDelete(vref, parid, (uint8 *) pas_name);
		return;
	}
	
	if ((FSstatus = SetEOF(our_ref_num, file_size))	!= 0)	/* set length 		*/
	{
		FSClose(our_ref_num);
		HDelete(vref, parid, (uint8 *) pas_name);
		return;
	}

	FSClose(our_ref_num);
}
		

/*--------------------------------------------------------------------------*/
/*	CreateTopImportFolder													*/
/*--------------------------------------------------------------------------*/

/* Create a Mac Folder into which imported files will be stored...			*/
/* CreateTopImportFolderder fills out a partial FSSpec for a hypothetical   */
/* file	 in the new folder (e.g. vref & id for the created directory are	*/
/* returned in it).															*/

static int CreateTopImportFolder(short VRefnum, long ParDirID, const char *FolderName, FSSpec *the_spec, uint32 /* orig_size*/,
								 const char *full_mac_name)
{
	OSErr	FSstatus;
	char	new_name[512];
	char	pas_name[512];
	FSSpec  temp_spec;
	SInt32	created_id;

	
	// Doctor up the folder name.  If it contains a :, then it must be a full path
	// name (e.g. "volume:folder name:folder name").  If it has no colon, then it must
	// be a partial path name for the specified volref/parid

	if (strrchr(FolderName, ':') == 0)
	{
		strncpy(new_name,":", sizeof(new_name));
		strncat(new_name, FolderName, sizeof(new_name) - strlen(new_name) - 1);
	}
	else
		strncpy(new_name, FolderName, sizeof(new_name));
			
	strncpy(pas_name, new_name, sizeof(pas_name));
	c2pstr(pas_name);

	
	// Make an FSSpec for the directory
		
	FSstatus = FSMakeFSSpec (VRefnum, ParDirID, (uint8 *)  pas_name,  &temp_spec);

	if (FSstatus != noErr && FSstatus != fnfErr)
		{printf("CreateTopImportFolder: Could not get information to create %s (%d)\n", new_name, FSstatus); return (-1);}


	// If directory does not exist, create it

	if (FSstatus == fnfErr)								
	{
		FSstatus = FSpDirCreate (&temp_spec,  (ScriptCode) smSystemScript, &created_id);
		
		if (FSstatus != noErr)
			{printf("AbleDiskTool: Could not create new folder %s (%d)\n", new_name, FSstatus); return (-1);}

		if (progress_desired || full_progress_desired)
			printf("AbleDiskTool: Created folder \"%s\"\n", full_mac_name);
	}
		

	// The directory (now) exists. Make an FSSpec for a hypothetical file therein
	// as a quick way to get the id of this directory
	
	strncat(new_name,":", sizeof(new_name) - strlen(new_name) - 1);
	strncat(new_name," ", sizeof(new_name) - strlen(new_name) - 1);

	strncpy(pas_name, new_name, sizeof(pas_name));
	c2pstr(pas_name);

	FSstatus = FSMakeFSSpec(VRefnum, ParDirID, (uint8 *) pas_name, the_spec);
	
	if (FSstatus != fnfErr)
		{printf("CreateTopImportFolder: Could not get FSSpec for %s (%d)\n", new_name, FSstatus); return (-1);}
	
	return (0);
}


/*--------------------------------------------------------------------------*/
/*	OpenMacFile																*/
/*--------------------------------------------------------------------------*/

typedef struct						// handy struct for accessing a mac file
{
	short		MacFRefNum;			// file reference number
	short		MacRRefNum;			// resource fork file ref num (-1 if not open; opened for Sd2f files only)
	SInt32		MacFileLen;
	OSType		MacFileTyp;
	OSType		MacFileCre;
	uint32		blocks_allocated;

}   mac_file_info;

static int OpenMacFile(const FSSpec *the_spec, mac_file_info *the_info, const char *error_name)
{
	OSErr	FSstatus;
	FInfo	info;
	
	// Init
	the_info->MacFRefNum = 0;
	the_info->MacRRefNum = -1;
	the_info->MacFileLen = 0;
	the_info->MacFileTyp = 0;
	the_info->MacFileCre = 0;
	the_info->blocks_allocated = 0;
	
	// Make an FSSpec for the file
	if ((FSstatus = FSpOpenDF    (the_spec, fsRdPerm, &the_info->MacFRefNum)) != 0)
		{printf("OpenMacFile: Could not open \"%s\" (%d)\n", error_name, FSstatus); return (-1);}
	
	if ((FSstatus = GetEOF       (the_info->MacFRefNum, &the_info->MacFileLen)) != 0)
		{printf("OpenMacFile: Could not get length of \"%s\" (%d)\n", error_name, FSstatus); FSClose(the_info->MacFRefNum); the_info->MacFRefNum = 0; return (-1);}

	if ((FSstatus = FSpGetFInfo  (the_spec, &info)) != 0)
		{printf("OpenMacFile: Could not get file type info for \"%s\" (%d)\n", error_name, FSstatus); FSClose(the_info->MacFRefNum); the_info->MacFRefNum = 0; return (-1);}

	the_info->MacFileTyp = info.fdType;
	the_info->MacFileCre = info.fdCreator;
	
	// Open res fork for Sd2 files
	if (the_info->MacFileTyp == SD2FileType)
		the_info->MacRRefNum = FSpOpenResFile (the_spec, fsRdPerm);

	return (0);
}

static	void	CloseMacFile(mac_file_info& the_info)
{
	if (the_info.MacFRefNum)
		{FSClose(the_info.MacFRefNum); the_info.MacFRefNum = 0;}
		
	if (the_info.MacRRefNum != 0 && the_info.MacRRefNum != -1)
		{CloseResFile(the_info.MacRRefNum); the_info.MacRRefNum = 0;}
}


/*--------------------------------------------------------------------------*/
/*	PromptUserForTweaks														*/
/*--------------------------------------------------------------------------*/

long (*AbleDiskToolAskForRename) (short dialog_id, const char *mac_entity, char *able_path, char *able_file, char *new_file);

enum
{
	Prompt_proceed  = 0,		// proceed with possible new name; replace file if it exists; local_level_name is valid at this point
	Prompt_skip     = 1,		// skip this file; that is return with no error
	Prompt_abort    = 2};		// abort the operation and all operation
	
static	int	PromptUserForTweaks(const char *mac_entity_name, char *local_level_name, fixed level, int do_replace_check_condition)
{
	// Ask for legit name if we need to
	char	extracted_path_name[512] = {""};
	char	extracted_file_name[512] = {""};
	char	local_path_name    [512] = {""};
	char	new_file_name      [512] = {""};
	int		i = strlen(local_level_name);
	char*	j = strrchr(local_level_name, ':');
	int		allow_replace = false;
	
	zero_mem((byte *) &extracted_path_name, sizeof(extracted_path_name));
	zero_mem((byte *) &extracted_file_name, sizeof(extracted_file_name));
	
	// path name specified isolate it.
	if (j)
	{
 		strcpy(extracted_path_name, local_level_name);		// dup level name
 		extracted_path_name[j - local_level_name + 1] = 0;	// terminate path after :
		strcpy(extracted_file_name, j+1);					// extract file name
	}
	else
		strcpy(extracted_file_name, local_level_name);

	while (1)
	{
		int		k   = strlen(extracted_file_name);
		int		bad = false;
		
		for (i=0; i<k; i++)
			if (!valid_filechar(extracted_file_name[i]))
				bad = true;
				
		if (k > 8)
			bad = true;
			
		if (k == 0)
			bad = true;
			
		if (bad)
		{
			zero_mem((byte *) &local_path_name, sizeof(new_file_name));
			zero_mem((byte *) &new_file_name,   sizeof(new_file_name));
			
			if (level == 1)
				strcpy(local_path_name,level_one_name);
			else
				strcpy(local_path_name,extracted_path_name);
				
			k = AbleDiskToolAskForRename(ASK_FOR_FILE_NAME_DIALOG_ID, mac_entity_name, local_path_name, extracted_file_name, new_file_name);
		
			if (k < 0)									// couldn't open dialog or other error - proceed to bomb out with bad name & report it
				break;

			if (k == 0)									// new name chosen
			{
				strcpy(local_level_name,    extracted_path_name);
				strcat(local_level_name,    new_file_name);
				strcpy(extracted_file_name, new_file_name);

				allow_replace = false;					// in case replacement name was also invalid; ask again
				
				continue;				
			}
			
			if (k == 1)									// skip
				return (Prompt_skip);

			if (k == 2)									// aborting
				return (Prompt_abort);
				
			// undefined behavior for undefined return values...
		}
		
		// See if name exists
		if (do_replace_check_condition && (!allow_replace) && (!replace_all_chosen) && (locate(local_level_name, level, true)))
		{
			zero_mem((byte *) &local_path_name, sizeof(new_file_name));
			zero_mem((byte *) &new_file_name,   sizeof(new_file_name));
			
			if (level == 1)
				strcpy(local_path_name,level_one_name);
			else
				strcpy(local_path_name,extracted_path_name);
				
			k = AbleDiskToolAskForRename(DUP_FILE_NAME_DIALOG_ID, mac_entity_name, local_path_name, extracted_file_name, new_file_name);
		
			if (k < 0)									// couldn't open dialog or other error - proceed to bomb out with bad name & report it
				break;

			if (k == 0)									// new name chosen; make sure it's valid and also not saved
			{
				strcpy(local_level_name,    extracted_path_name);
				strcat(local_level_name,    new_file_name);
				strcpy(extracted_file_name, new_file_name);

				continue;				
			}

			if (k == 1)									// skip
				return (Prompt_skip);

			if (k == 2)									// aborting
				return (Prompt_abort);

			if (k == 3)									// replace just proceed
			{
				strcpy(local_level_name,    extracted_path_name);
				strcat(local_level_name,    new_file_name);
				strcpy(extracted_file_name, new_file_name);
				
				allow_replace = true;

				continue;				
			}
			
			if (k == 4)									// replace all
			{
				strcpy(local_level_name,    extracted_path_name);
				strcat(local_level_name,    new_file_name);
				strcpy(extracted_file_name, new_file_name);
				
				replace_all_chosen = true;
				allow_replace      = true;

				continue;				
			}
			// undefined behavior for undefined return values...
		}

		break;
	}
	
	return (Prompt_proceed);							// proceed with new name
}


/*--------------------------------------------------------------------------*/
/*	ExportMacFile															*/
/*--------------------------------------------------------------------------*/

static uint32 	ComputeWordLengthOfMacTextFile	(uint32 byte_len, short our_ref_num, const char *error_name);
static uint32 	ComputeSqueezedSubcatSize 		(uint32 byte_len, short our_ref_num, const char *error_name, fixed type);
static uint32 	ComputeSoundFileSize	 		(short our_ref_num, short res_ref_num, const char *error_name, uint32 type);
static int 		ExportAbleTextFile				(uint16 scsi_id, uint32 block_num, uint32 byte_len,  short our_ref_num, uint32 word_len);
static int 		ExportAbleDataFile				(uint16 scsi_id, uint32 block_num, uint32 num_words, short our_ref_num);
static int 		ExportSoundFileFile				(uint16 scsi_id, uint32 block_num, short our_ref_num, short res_ref_num, uint32 type);

static int ExportMacFile(const FSSpec *the_spec, mac_file_info *the_info, const char *mac_entity_name,
                         const char *error_name, char *level_name, fixed level)
{
	fixed   export_type;
	uint32 	byte_len;
	uint32  word_len;
	uint32  sec_len;
	char	local_level_name[512] = {""};
		
	fixed   ms_len, ls_len, words;
	
	// Open it
	if (OpenMacFile(the_spec, the_info, mac_entity_name))
		return (-1);
				
	byte_len = the_info->MacFileLen;
	word_len = the_info->MacFileLen >> 1;

	// Get able file type	
	export_type = GetAbleFileType(the_info->MacFileTyp, the_info->MacFileCre, the_spec);
	
	if (export_type == -1)
	{
		printf("AbleDiskTool: Unrecognized file type for file '%s'\n", mac_entity_name);
		CloseMacFile(*the_info);
		return (-1);
	}
		
	// Compute word length if text file
	if (export_type == t_text)
	{
		word_len = ComputeWordLengthOfMacTextFile(byte_len, the_info->MacFRefNum, mac_entity_name);
	
		if (word_len == (-1))
		{
			CloseMacFile(*the_info);
			return (-1);
		}
	}
	
	// Compute work length if AIFF, Sd2F or WAVE
	else if (export_type == t_sound && (the_info->MacFileTyp == AIFFID || the_info->MacFileTyp == SD2FileType || the_info->MacFileTyp == WAVEID))
	{
		word_len = ComputeSoundFileSize(the_info->MacFRefNum, the_info->MacRRefNum, mac_entity_name, the_info->MacFileTyp);
		
		if (word_len == (-1))
		{
			CloseMacFile(*the_info);
			return (-1);
		}
	}

	sec_len  = (word_len + 255) >> 8;		/* get length in sectors		*/
	
	if (export_type == t_subc || export_type == t_lsubc)			// for subcatalogs, check for sequeeze of expand in export
	{
		if (squeeze_images_on_transfer)
		{
			uint32 word_len = ComputeSqueezedSubcatSize(byte_len, the_info->MacFRefNum, mac_entity_name, export_type);
			
			if (word_len != (-1))
				sec_len = (word_len + 255) >> 8;					// length of sequeezed subcat
		}
		
		if (create_extra_space)
		{
			sec_len += create_extra_space*((sec_len + 99)/100);
			
			if (sec_len >= (1 << 20))
				sec_len = ((1 << 20) - 1);
		}
	}

	ms_len = (fixed) (sec_len >> 16);
	ls_len = (fixed) (sec_len  & 0xFFFF);
	words  = (fixed) (word_len & 0xFFFF);
	
	the_info->blocks_allocated = sec_len;
	
	// Check for valid file name; allow correction if not
	strcpy(local_level_name, level_name);
	
	if (AbleDiskToolAskForRename)
	{
		int action = PromptUserForTweaks(mac_entity_name, local_level_name, level, don_t_over_write);
		
		if (action == Prompt_skip)							// skip
		{
			CloseMacFile(*the_info);						// close file
			
			if (full_progress_desired)
				printf("Skipping '%s'\n", mac_entity_name);
				
			any_files_were_skipped = true;

			return (0);										// return with no error
		}

		if (action == Prompt_abort)							// abort
		{
			CloseMacFile(*the_info);						// close file
			g_break_received = true;

			return (-1);									// return with error
		}

		// else proceed with possible new name and replace file if it exists			
	}
	
	// Else see if name exists
	else if (!replace_all_chosen)
	{
		if (don_t_over_write || merge_but_warn)
		{			
			if (locate(local_level_name, level, true))
			{
				printf("AbleDiskTool: Can't export '%s'; it already exists\n", error_name);
				CloseMacFile(*the_info);
				return (-1);
			}
		}
	}
	
	// Check invalid merge in all cases
	if ((merge_but_warn    && export_type == t_subc)
	||  (merge_and_replace && export_type == t_subc))
	{			
		if (locate(local_level_name, level, true))
		{
			printf("AbleDiskTool: Can't merge a Disk Image File into a subcatalog ('%s')\n", error_name);
			CloseMacFile(*the_info);
			return (-1);
		}
	}
	
	// Create the able directory entry
	AbleCatRtns_AllowTypeReplace = true;

	if (!replace(local_level_name, export_type, ms_len, ls_len, words, level, true))
	{
		AbleCatRtns_AllowTypeReplace = false;
		get_cat_code_message(c_status, er_mess);
		printf("AbleDiskTool: Could not export '%s' for the following reason:\n", error_name);
		printf("   %s\n", er_mess);

		CloseMacFile(*the_info);

		return (-1);
	}
	
	AbleCatRtns_AllowTypeReplace = false;	

	// Write the file to the SCSI disk
	if (full_progress_desired)
		printf("Exporting '%s'\n", mac_entity_name);
	
	// Export text file
	if (export_type == t_text)
	{
		if (ExportAbleTextFile(SCSI_id, (((uint32) (uint16) (f_ms_sector & 0xFF)) << 16) | ((uint32) (uint16) f_ls_sector),
		                       byte_len, the_info->MacFRefNum, word_len))
		{
			CloseMacFile(*the_info);
			XPLdelete(local_level_name, level, true);
			return (-1);
		}
	}

	// Export AIFF, Sd2F or WAVE file
	else if (export_type == t_sound && (the_info->MacFileTyp == AIFFID || the_info->MacFileTyp == SD2FileType || the_info->MacFileTyp == WAVEID))
	{
		if (ExportSoundFileFile(SCSI_id, (((uint32) (uint16) (f_ms_sector & 0xFF)) << 16) | ((uint32) (uint16) f_ls_sector),
		                        the_info->MacFRefNum, the_info->MacRRefNum, the_info->MacFileTyp))
		{
			CloseMacFile(*the_info);
			XPLdelete(local_level_name, level, true);
			return (-1);
		}		
	}

	// Export other data files
	else
	{
		if (ExportAbleDataFile(SCSI_id, (((uint32) (uint16) (f_ms_sector & 0xFF)) << 16) | ((uint32) (uint16) f_ls_sector),
	    	               word_len, the_info->MacFRefNum))
		{
			CloseMacFile(*the_info);
			XPLdelete(local_level_name, level, true);
			return (-1);
		}		
	}

	// Clean up and return	
	CloseMacFile(*the_info);

	return (0);
}


/*--------------------------------------------------------------------------*/
/*	ExportDiskImageToDrive													*/
/*--------------------------------------------------------------------------*/

/* Writes a data SUBC disk image file directly to top level of drive */

static int ExportDiskImageToDrive(const FSSpec *the_spec, mac_file_info *the_info, const char *mac_entity_name,
                                  const char *error_name, char *level_name)
{
	fixed   export_type;
	uint32 	byte_len;
	uint32  word_len;
	uint32  sec_len;
	uint32	device_size;
	uint32	temp;
	
	
	// Get size of destination drive
	if ((strcmp(level_name, "W0:") == 0)
	||  (strcmp(level_name, "w0:") == 0))
	{
		get_device_size(ABLE_W0_READDATA_CODE);
		to_able_string("W0", f_name);
		strcpy(import_able_entity_name, "W0");
	}

	else
	{
		get_device_size(ABLE_W1_READDATA_CODE);
		to_able_string("W1", f_name);
		strcpy(import_able_entity_name, "W1");
	}
	
	device_size = (((uint32) (uint16) (c_ms_length & 0xFF)) << 16) | ((uint32) (uint16) c_ls_length); // block len

	// Open it
	if (OpenMacFile(the_spec, the_info, mac_entity_name))
		return (-1);
				
	byte_len = the_info->MacFileLen;
	word_len = the_info->MacFileLen >> 1;

	// Get able file type	
	export_type = GetAbleFileType(the_info->MacFileTyp, the_info->MacFileCre, the_spec);
	
	if (export_type != t_lsubc)
	{
		printf("AbleDiskTool: Disk Image '%s' has wrong type for export-as-drive\n", mac_entity_name);
		CloseMacFile(*the_info);
		return (-1);
	}
		
	temp = ComputeSqueezedSubcatSize(byte_len, the_info->MacFRefNum, mac_entity_name, export_type);
		
	if (temp != (-1))										// if could get valid length
	{										
		if (temp < word_len)								// if squeezed is shorter, use it
			word_len = temp;
	}

	sec_len = (word_len + 255) >> 8;						// get sector length from mac-info

	the_info->blocks_allocated = sec_len;
	

	// Make sure it fits
	if (sec_len > device_size)
	{
		printf("AbleDiskTool: Can't export '%s' as drive; the drive is too small\n", error_name);
		CloseMacFile(*the_info);
		return (-1);
	}
		
	// Write the file to the SCSI disk
	if (full_progress_desired)
		printf("Exporting '%s' as drive\n", mac_entity_name);
	
	if (ExportAbleDataFile(SCSI_id, 0, word_len, the_info->MacFRefNum))
	{
		CloseMacFile(*the_info);
		return (-1);
	}		

	// Clean up and return	
	CloseMacFile(*the_info);
	
	return (0);
}


/*--------------------------------------------------------------------------*/
/*	CreateAndOpenAbleSimFile												*/
/*--------------------------------------------------------------------------*/

/* Open or Create a file on the current volume called  "*Able Disk Image"	*/
/* for simulating an Able disk on the host system							*/

ulong		AbleSimFileSize      = 32*1024*1024;
char		AbleSimFileName[512] = "*Able Disk Image";
CFStringRef AbleSimFileCFString  = NULL;

static int CreateAndOpenAbleSimFile(short VRefNum)
{
	OSErr		FSstatus;
	FSSpec  	temp_spec;
	ulong		file_size;
	ulong		count;
	ulong		position;
	ulong		length;
	char		pas_name[512];
	uint16		*in_buf = NULL;
	handle		in_buf_handle = NULL;
	ulong		progress;
	ulong		interval;
	
	strncpy(pas_name, AbleSimFileName, sizeof(pas_name));
	c2pstr(pas_name);
	
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

	// Delete prior image file
	if ((FSstatus = FSMakeFSSpec (VRefNum, 0, (uint8 *) pas_name,  &temp_spec)) == 0)
	{
		if ((FSstatus = FSpDelete(&temp_spec)) != 0)
		{
			printf("AbleDiskTool: Could not replace prior disk image file '%s' (%d)\n", AbleSimFileName, FSstatus);
			if (FSstatus == fBsyErr)
			{
				printf("   Looks like the disk image file is being used by another application\n");
				printf("   You must Quit SynclavierX before replacing '%s'\n", AbleSimFileName);
			}
			return (-1);
		}
	}

	// Create the new one
	
	if ((FSstatus = FSpCreate(&temp_spec, 'SNCL', 'SUBC', smSystemScript)) != 0)
	{
		printf("AbleDiskTool: Could not create disk image file '%s' (%d)\n", AbleSimFileName, FSstatus);
		
		if (FSstatus == dupFNErr)
			printf("   Looks like the file already exists or the path name is incorrect\n");
		
		return (-1);
	}
	
	if ((FSstatus = FSpOpenDF(&temp_spec, fsRdWrShPerm, &AbleSimFileRefNum)) != 0)
	{
		printf("AbleDiskTool: Could not open the new disk image file '%s' (%d)\n", AbleSimFileName, FSstatus);
		FSpDelete(&temp_spec);
		return (-1);
	}
	
	AbleSimFSSpec = temp_spec;
			
	file_size = AbleSimFileSize;

	if ((FSstatus = Allocate(AbleSimFileRefNum, (SInt32 *) &file_size)) != 0)
	{
		printf("AbleDiskTool: Could not allocate storage for the new disk image file '%s' (%d)\n", AbleSimFileName, FSstatus);
		if (FSstatus == dskFulErr)
		printf("   Looks like the disk is full\n");
		FSClose(AbleSimFileRefNum);
		FSpDelete(&temp_spec);
		return (-1);
	}

	file_size = AbleSimFileSize;

	if ((FSstatus = SetEOF(AbleSimFileRefNum, file_size))	!= 0)						/* set length 		*/
	{
		printf("AbleDiskTool: Could not set end-of-file for the new disk image file '%s' (%d)\n", AbleSimFileName, FSstatus);
		FSClose(AbleSimFileRefNum);
		FSpDelete(&temp_spec);
		return (-1);
	}

	if ((FSstatus = SetFPos(AbleSimFileRefNum, fsFromStart, 0)) != 0)					/* seek to start	*/
	{
		printf("AbleDiskTool: Could not reset the new disk image file '%s' (%d)\n", AbleSimFileName, FSstatus);
		FSClose(AbleSimFileRefNum);
		FSpDelete(&temp_spec);
		return (-1);
	}

	count = sizeof(zero_buf);
	
	// No swap needed - writing zeroes
	if (((FSstatus = FSWrite(AbleSimFileRefNum, (SInt32 *) &count, (void *) zero_buf)) != 0)
	||  (count != sizeof(zero_buf)))
	{
		printf("AbleDiskTool: Could not initialize the new disk image file '%s' (%d)\n", AbleSimFileName, FSstatus);
		FSClose(AbleSimFileRefNum);
		FSpDelete(&temp_spec);
		return (-1);
	}
			
	if (progress_desired || full_progress_desired)
		printf("AbleDiskTool: Synclavier Disk Image File '%s' created successfully\n", AbleSimFileName);

	if (zero_new_disk_file)
	{
		in_buf_handle = get_big_memory(MAX_TEXT_SIZE);
		
		if (in_buf_handle)
			in_buf = (uint16 *) *in_buf_handle;

		if (!in_buf_handle || !in_buf)
		{
			printf("AbleDiskTool: Could not get memory for zeroing\n");
			return (-1);
		}

		zero_mem((byte *) in_buf, MAX_TEXT_SIZE);
		
		if ((FSstatus = SetFPos(AbleSimFileRefNum, fsFromStart, 0)) != 0)
		{
			printf("AbleDiskTool: Could not reset the new disk image file for zeroing'%s' (%d)\n", AbleSimFileName, FSstatus);
			free_big_memory(in_buf_handle);
			return (-1);
		}
		
		if (progress_desired || full_progress_desired)
			printf("AbleDiskTool: Zeroing out disk image file\n");

		file_size = AbleSimFileSize;
		position  = 0;
		interval  = 10 * 1024 * 1024;
		progress  = interval;
				
		while (position < file_size)
		{
			length = file_size - position;

			if (length > MAX_TEXT_SIZE) length = MAX_TEXT_SIZE;
			
			count = length;
			
			// No swap needed - writing Zeroes
			if (((FSstatus = FSWrite(AbleSimFileRefNum, (SInt32 *) &count, (void *) in_buf)) != 0)
			||  (count != length))
			{
				printf("AbleDiskTool: Failed trying to zero out new disk image file '%s' (%d)\n", AbleSimFileName, FSstatus);
				free_big_memory(in_buf_handle);
				return (-1);
			}
			
			run_host_environment_250();
			
			if (g_break_received)
			{
				printf("AbleDiskTool: Halting...\n");
				printf("AbleDiskTool: Cleaning up...\n");
				FSClose(AbleSimFileRefNum);
				FSpDelete(&temp_spec);
				return (-1);
			}

			position += length;
			
			if (full_progress_desired)
			{
				if (position >= progress)
				{
					printf("   %d megabytes\n", position/1024/1024);
					progress += interval;
				}
			}
		}
		
		free_big_memory(in_buf_handle);
	}	
	
	return (0);
}

static int OpenAbleSimFile(short VRefNum)
{
	OSErr	FSstatus;
	FSSpec  temp_spec;
    FSRef   temp_ref;
    
	if (AbleSimFileRefNum)						/* make sure not opened twice!		*/
		{printf("OpenAbleSimFile: Double Call!!\n"); return (-1);}
	
    // We use CFURLs here to handle unicode file paths.
    // The FSSpec need to be replaced with SyncFSSpec and CSynclavierFileReference objects
    CFURLRef url = CFURLCreateWithFileSystemPath(NULL, AbleSimFileCFString, kCFURLPOSIXPathStyle, false);
    
    if (!url)
        {printf("OpenAbleSimFile: failed CFURLCreateWithFileSystemPath (%s)\n", AbleSimFileName); return (-1);}
    
    if (!CFURLResourceIsReachable(url, NULL))
        {printf("OpenAbleSimFile: failed CFURLResourceIsReachable (%s)\n", AbleSimFileName); return (-1);}
    
    if (!CFURLGetFSRef(url, &temp_ref))
        {printf("OpenAbleSimFile: failed CFURLGetFSRef (%s)\n", AbleSimFileName); return (-1);}
    
    if (FSGetCatalogInfo (&temp_ref, kFSCatInfoNone, NULL, NULL, &temp_spec, NULL) != noErr)
        {printf("OpenAbleSimFile: failed FSGetCatalogInfo (%s)\n", AbleSimFileName); return (-1);}
    
    // Open it
	if ((FSstatus = FSpOpenDF(&temp_spec, fsRdWrShPerm, &AbleSimFileRefNum)) != 0)
	{
		printf("AbleDiskTool: Could not open the new disk image file '%s' (%d)\n", AbleSimFileName, FSstatus);

		if (FSstatus == opWrErr || FSstatus == permErr)
		{
			printf("   Looks like the disk image file is being used by another application\n");
			printf("   You must Quit SynclavierX before you may access '%s' with AbleDiskTool\n", AbleSimFileName);
		}

		return (-1);
	}
	
	AbleSimFSSpec = temp_spec;
    
    if (url)
        CFRelease(url);
			
	return (0);
}


/*--------------------------------------------------------------------------*/
/*	CreateAndOpenLogFile													*/
/*--------------------------------------------------------------------------*/

static int CreateAndOpenLogFile(short VRefNum)
{
	OSErr		FSstatus;
	FSSpec  	temp_spec;
	ulong		file_size;
	ulong		count;
	ulong		position;
	ulong		length;
	char		pas_name[512];
	uint16		*in_buf = NULL;
	handle		in_buf_handle = NULL;
	ulong		progress;
	ulong		interval;
	
	strncpy(pas_name, log_file_name, sizeof(pas_name));
	c2pstr(pas_name);
	
	// Delete prior log file
	if ((FSstatus = FSMakeFSSpec (VRefNum, 0, (uint8 *) pas_name,  &temp_spec)) == 0)
	{
		if ((FSstatus = FSpDelete(&temp_spec)) != 0)
		{
			printf("AbleDiskTool: Could not replace prior log file '%s' (%d)\n", log_file_name, FSstatus);
			if (FSstatus == fBsyErr)
			{
				printf("   Looks like the log file is being used by another application\n");
				printf("   Is '%s' perhaps open in a text editor?\n", log_file_name);
			}
			return (-1);
		}
	}

	// Create the new one
	if ((FSstatus = FSpCreate(&temp_spec, 'ttxt', 'TEXT', smSystemScript)) != 0)
	{
		printf("AbleDiskTool: Could not create log file '%s' (%d)\n", log_file_name, FSstatus);
		
		if (FSstatus == dupFNErr)
			printf("   Looks like the log file already exists or the path name is incorrect\n");
		
		return (-1);
	}
	
	if ((FSstatus = FSpOpenDF(&temp_spec, fsRdWrShPerm, &LogFileRefNum)) != 0)
	{
		printf("AbleDiskTool: Could not open the new log file '%s' (%d)\n", log_file_name, FSstatus);
		FSpDelete(&temp_spec);
		return (-1);
	}
	
	LogVolRefNum   = temp_spec.vRefNum;				// publish volume reference number for flushing
	LogFSSpec      = temp_spec;
	LogNeedsHeader = true;
				
	return (0);
}


/*--------------------------------------------------------------------------*/
/*	ReadAbleDisk															*/
/*--------------------------------------------------------------------------*/

/* Reads an Able SCSI disk.  Pass in SCSI id */

static int ReadAbleDisk(uint16 scsi_id, uint16 *buffer, uint32 block_num, uint32 block_len)
{
	scsi_device		*our_device;
	scsi_error_code status;
	
	if (scsi_id >= 7)
		{printf("ReadAbleDisk: Bad SCSI Id configuration (%d)\n", scsi_id); return (-1);}
	
	our_device = &g_scsi_device_data_base[scsi_id];
	
	if ((status = issue_read_extended(our_device, (byte *) buffer, block_num, block_len)) != 0)
		{printf("ReadAbleDisk: Failed issue_read_extended (%d)\n", status);return (-1);}
	
	// Rewrite disk if requested to refresh aging media and data
	if (do_rewrite)
		issue_write_extended(our_device, (byte *) buffer, block_num, block_len);
    
    // Major hack - if little endian, ** undo ** the byte swiizzle performe by issue_read_extended
	if (little_endian) {
		ufixed *     dataPtr = (ufixed *) buffer;
		unsigned int i       = block_len*256;
		
		while (i--) {
			*dataPtr = CFSwapInt16BigToHost(*dataPtr);
            dataPtr++;
        }
	}

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
/*	DumpAbleTextFile														*/
/*--------------------------------------------------------------------------*/

/* Utility routine to dump an Able text file from memory to standard output	*/

static int DumpAbleTextFile(uint16 scsi_id, uint32 block_num, uint32 num_words)
{
	uint32	 	i,j;
	char		it;
	uint16		*in_buf = NULL;
	uint16 		*buffer = NULL;
	handle		in_buf_handle = NULL;
	
	if (num_words > (MAX_TEXT_SIZE >> 1))
		{printf("DumpAbleTextFile: File too big %d\n", num_words); return (-1);}

	in_buf_handle = get_big_memory(MAX_TEXT_SIZE);	/* get storage				*/
	
	if (in_buf_handle)
		in_buf = (uint16 *) *in_buf_handle;

	if (!in_buf_handle || !in_buf)
		{printf("DumpAbleTextFile: out of memory for text file buffer\n"); return (-1);}
		
	if (ReadAbleDisk(scsi_id, in_buf, block_num, (num_words + 255) >> 8))
		{free_big_memory(in_buf_handle); return (-1);}
	
	buffer = (uint16 *) in_buf;
	
	i = 0;
	while (i < num_words)
	{
		buffer++;							/* skip line number				*/
		i++;								/* count word processed			*/
		
		j = 0;								/* initialize byte pointer		*/
		
		while (1)
		{
			if (j&1) it = *buffer >> 8;		/* second byte is upper			*/
			else     it = *buffer & 0xFF;	/* first byte is lower			*/
			
			if (it)							/* process until end of line	*/
				printf("%c", it);
			else
				break;
				
			if (j&1) {buffer++; i++;}		/* move to next byte			*/
			j++;
		}
		
		buffer++; i++;						/* skip word with eol in it		*/
		printf("\n");						/* eol output character			*/
	
		run_host_environment_250();

		if (g_break_received)
		{
			free_big_memory(in_buf_handle);
			return(-1);
		}
	}
	
	free_big_memory(in_buf_handle);
	
	in_buf_handle = NULL; in_buf  = NULL;
	
	return (0);
}


/*--------------------------------------------------------------------------*/
/*	ImportAbleTextFile														*/
/*--------------------------------------------------------------------------*/

/* Routine to convert an Able text file (sitting in memory) to a Macintosh	*/
/* text file.																*/

static int ImportAbleTextFile(uint16 scsi_id, uint32 block_num, uint32 num_words,
                              const char *file_name, short vol_ref_num, long dir_id, const char* tree_name)
{
	uint32	 	i,j,k;
	char		it;
	uint16		*in_buf  = NULL;
	char		*out_buf = NULL;
	handle		in_buf_handle  = NULL;
	handle		out_buf_handle = NULL;
	OSErr		FSstatus;
	long		file_size;
	long		count;
	short		our_ref_num;
	char		pas_name[64];
	
	strcpy(pas_name, file_name);				// get working copy of file name
	
	if (pas_name[0] == '.')						// map leading . to * to avoid
		pas_name[0] = '*';						// mac os conflict with devices

	c2pstr(pas_name);							// get in pascal format

	if (num_words > (MAX_TEXT_SIZE >> 1))
		{printf("ImportAbleTextFile: File too big %d\n", num_words); return (-1);}

	in_buf_handle  = get_big_memory(MAX_TEXT_SIZE);
	out_buf_handle = get_big_memory(MAX_TEXT_SIZE);
	
	if (in_buf_handle)
		in_buf = (uint16 *) *in_buf_handle;
		
	if (out_buf_handle)
		out_buf = (char *) *out_buf_handle;

	if (!in_buf_handle || !in_buf || !out_buf_handle || !out_buf)
		{printf("ImportAbleTextFile: out of mac memory for text file buffer\n"); free_big_memory(in_buf_handle); free_big_memory(out_buf_handle); return (-1);}
		
	if (ReadAbleDisk(scsi_id, in_buf, block_num, (num_words + 255) >> 8))
		{free_big_memory(in_buf_handle); free_big_memory(out_buf_handle); return (-1);}
	
	if (full_progress_desired)
		printf("AbleDiskTool: Importing TEXT file \"%s\"\n", tree_name);

	i = 0; k = 0;							/* initialize in & out pointers		*/
	
	while (i < num_words)
	{
		i++;								/* skip first line number			*/
		
		j = 0;								/* initialize byte pointer			*/
		
		while (i < num_words)				/* break in case non-text...		*/
		{
			if ((i + 100) >= MAX_TEXT_SIZE)
				{printf("ImportAbleTextFile: input file too long for conversion (1) %d %d \n", i, MAX_TEXT_SIZE); free_big_memory(in_buf_handle); free_big_memory(out_buf_handle); return (-1);}
			
			if (j&1) it = in_buf[i] >> 8;	/* second byte is upper			*/
			else     it = in_buf[i] & 0xFF;	/* first byte is lower			*/
			
			if (it)							/* process until end of line	*/
			{
				if ((k + 100) >= MAX_TEXT_SIZE)
					{printf("ImportAbleTextFile: file too long for conversion (1) %d %d \n", k, MAX_TEXT_SIZE); free_big_memory(in_buf_handle); free_big_memory(out_buf_handle); return (-1);}
				
				out_buf[k++] = it;
			}
			else
				break;
				
			if (j&1) i++;					/* move to next word			*/
			j++;							/* toggle byte counter			*/
		}
		
		i++;								/* skip word with eol in it		*/
		
		if ((k + 100) >= MAX_TEXT_SIZE)
			{printf("ImportAbleTextFile: file too long for conversion (2) %d %d \n", k, MAX_TEXT_SIZE); free_big_memory(in_buf_handle); free_big_memory(out_buf_handle); return (-1);}
		
		out_buf[k++] = MAC_OS_FILE_EOLCHAR;	/* eol output character			*/
		
		run_host_environment_250();

		if (g_break_received)
		{
			free_big_memory(in_buf_handle);
			free_big_memory(out_buf_handle);
			return(-1);
		}
	}
	
	file_size = k;							/* size to allocate				*/


	/* Write output file to disk: */
		
	if ((FSstatus = HCreate(vol_ref_num, dir_id, (uint8 *) pas_name, 'ttxt', 'TEXT')) != 0)
	{
		if (FSstatus == dupFNErr)
			{printf("ImportAbleTextFile: Can't import %s; it already exists\n", tree_name); free_big_memory(in_buf_handle); free_big_memory(out_buf_handle); return (-1);}
		else
			{printf("ImportAbleTextFile: Failed HCreate (%d)\n", FSstatus); free_big_memory(in_buf_handle); free_big_memory(out_buf_handle); return (-1);}
	}
		
	if ((FSstatus = HOpen(vol_ref_num, dir_id, (uint8 *) pas_name, fsRdWrPerm, &our_ref_num)) != 0)
		{printf("ImportAbleTextFile: Failed HOpen (%d)\n", FSstatus); HDelete(vol_ref_num, dir_id, (uint8 *) pas_name); free_big_memory(in_buf_handle); free_big_memory(out_buf_handle); return (-1);}
	
	count = file_size;
	
	if ((FSstatus = Allocate(our_ref_num, (SInt32 *) &count)) != 0)
	{
		printf("ImportAbleTextFile: Failed Allocate (%d)\n", FSstatus);
		printf("   Your hard drive may be out of storage or may be read-only\n");
		FSClose(our_ref_num);
		HDelete(vol_ref_num, dir_id, (uint8 *) pas_name);
		free_big_memory(in_buf_handle);
		free_big_memory(out_buf_handle);
		return (-1);
	}

	if ((FSstatus = SetFPos(our_ref_num, fsFromStart, 0)) != 0)
		{printf("ImportAbleTextFile: Failed SetFPos (%d)\n", FSstatus); FSClose(our_ref_num); HDelete(vol_ref_num, dir_id, (uint8 *) pas_name); free_big_memory(in_buf_handle); free_big_memory(out_buf_handle); return (-1);}

	count = file_size;
	
	// No swap needed since we are writing bytes (e.g. text file)
	if (((FSstatus = FSWrite(our_ref_num, (SInt32 *) &count, (void *)out_buf)) != 0)
	||  (count != file_size))
		{printf("ImportAbleTextFile: Failed FSWrite (%d)\n", FSstatus); FSClose(our_ref_num); HDelete(vol_ref_num, dir_id, (uint8 *) pas_name); free_big_memory(in_buf_handle); free_big_memory(out_buf_handle); return (-1);}
	
	if ((FSstatus = SetEOF(our_ref_num, file_size))	!= 0)	/* set length 		*/
		{printf("ImportAbleTextFile: Failed SetEOF (%d)\n", FSstatus); FSClose(our_ref_num); HDelete(vol_ref_num, dir_id, (uint8 *) pas_name); free_big_memory(in_buf_handle); free_big_memory(out_buf_handle); return (-1);}

	if ((FSstatus = FSClose(our_ref_num)) != 0)
		{printf("ImportAbleTextFile: Failed FSClose (%d)\n", FSstatus); free_big_memory(in_buf_handle); free_big_memory(out_buf_handle); return (-1);}

	free_big_memory(in_buf_handle );
	free_big_memory(out_buf_handle);
	
	in_buf_handle  = NULL; in_buf  = 0;
	out_buf_handle = NULL; out_buf = 0;
	
	return (0);
}


/*--------------------------------------------------------------------------*/
/*	ImportAbleSubcatalog													*/
/*--------------------------------------------------------------------------*/

/* This routine imports an able catalog or subcatalog as a Mac disk image	*/
/* of an able subcatalog													*/

static int ImportAbleSubcatalog(uint16 scsi_id, uint32 block_num, uint32 num_words, OSType file_type,
                                const char *file_name, short vol_ref_num, long dir_id, const char *tree_name, fixed type,
                                const char *able_name)
{
	uint32	 	i,j,k;
	uint16		*in_buf = NULL;
	handle		in_buf_handle = NULL;
	OSErr		FSstatus;
	int			count;
	short		our_ref_num;
	uint32		chunk_blocks = MAX_TEXT_SIZE/512;
	char		pas_name[64];

	uint32		start_block;
	uint32		block_len;
	uint32		end_block;
	uint32		largest_end_block = 0;
	uint32		dir_len;
	uint32		expanded_size_bytes;
	uint32		copy_words = num_words;			// assume must copy everything

	strcpy(pas_name, file_name);				// get working copy of file name
	
	if (pas_name[0] == '.')						// map leading . to * to avoid
		pas_name[0] = '*';						// mac os conflict with devices
		
	strcat(pas_name, ".simg");					// Append file extension

	c2pstr(pas_name);							// get in pascal format

	in_buf_handle = get_big_memory(MAX_TEXT_SIZE);
	
	if (in_buf_handle)
		in_buf = (uint16 *) *in_buf_handle;
		
	if (!in_buf_handle || !in_buf)
		{printf("ImportAbleSubcatalog: out of memory for data file buffer\n"); free_big_memory(in_buf_handle); return (-1);}
	
	
	// Read in directory
	
	if (type == t_subc)
		dir_len = 256;
	else
		dir_len = 1024;
	
	if (num_words < dir_len)
		{printf("ImportAbleSubcatalog: file '%s' can't be imported due to a Synclavier catalog error\n", able_name); free_big_memory(in_buf_handle); return (-1);}
	
	zero_mem((byte *) in_buf, 1024 << 1);				// zero out in case expanding directory
	
	if (ReadAbleDisk(scsi_id, in_buf, block_num, dir_len/256))
		{free_big_memory(in_buf_handle); return (-1);}


	// Determine regions spanned by the device or subcatalog.  Only copy
	// that much data
	
	i = 0;
	while (i < dir_len)
	{
		start_block = ((uint32) in_buf[i+4]) + ((((uint32) in_buf[i+7]) & 0xFF00) <<  8);
		block_len   = ((uint32) in_buf[i+5]) + ((((uint32) in_buf[i+7]) & 0x00F0) << 12);
		end_block   = start_block + block_len;
		if (end_block > largest_end_block) largest_end_block = end_block;
		
		i += 8;
	}
	
	if (largest_end_block < (dir_len >> 8))
		largest_end_block = (dir_len >> 8);
	
	if ((largest_end_block << 8) < copy_words)			// if no catalog error
		copy_words = largest_end_block << 8;			// this is number of words to use
	
	if (squeeze_images_on_transfer)						// if squeeze desired, figure out spanned region
	{
		if (copy_words < num_words)
			num_words = copy_words;
	}
	
	
	// Expand subc to lsubc
	
	if (type == t_subc)
	{
		if (num_words > (((1 << 20) - 1) - 3) << 8)		// make sure can add 3 sectors
		{
			printf("ImportAbleSubcatalog: subcatalog '%s' must be converted to a large subcatalog\n", able_name);
			printf("  before it may be imported as a disk image.  Small subcatalogs that are 512 megabytes\n");
			printf("  in size can't be imported as disk images\n");
				
			free_big_memory(in_buf_handle);
			return (-1);
		}
		
		i = 0;
		while (i < dir_len)
		{
			start_block = ((uint32) in_buf[i+4]) + ((((uint32) in_buf[i+7]) & 0xFF00) <<  8);

			start_block = start_block + 3;				// move start block to reflect 3 additional directory sectors
			
			in_buf[i+4] = (fixed) (start_block & 0xFFFF);
			in_buf[i+7] = (in_buf[i+7] & 0xFF) | (fixed) ((start_block & 0xFF0000) >> 8);
			
			i += 8;
		}
	
		num_words  += 3*256;
		copy_words += 3*256;
	}
	

	// Create the output file
	
	if ((FSstatus = HCreate(vol_ref_num, dir_id, (uint8 *) pas_name, 'SNCL', file_type)) != 0)
	{
		if (FSstatus == dupFNErr)
			{printf("ImportAbleSubcatalog: Can't import '%s'; it already exists\n", tree_name); free_big_memory(in_buf_handle); return (-1);}
		else
			{printf("ImportAbleSubcatalog: Failed HCreate (%d)\n", FSstatus); free_big_memory(in_buf_handle); return (-1);}
	}
		
	if ((FSstatus = HOpen(vol_ref_num, dir_id, (uint8 *) pas_name, fsRdWrPerm, &our_ref_num)) != 0)
		{printf("ImportAbleSubcatalog: Failed HOpen (%d)\n", FSstatus); free_big_memory(in_buf_handle); return (-1);}
	

	// Expand the disk image on import if desired

	expanded_size_bytes = num_words << 1;

	if (create_extra_space)												// if expansion desired
	{
		uint32 entity_size = expanded_size_bytes >> 9;					// get entity size in sectors
		
		entity_size += create_extra_space*((entity_size + 99)/100);		// expand it
		
		if (entity_size >= (1 << 20))									// limit it
			entity_size = ((1 << 20) - 1);
			
		expanded_size_bytes = entity_size << 9;							// publish new computation
	}

	count = expanded_size_bytes;
	long lcount = count;
	
	if ((FSstatus = Allocate(our_ref_num, (SInt32 *) &lcount)) != 0)
	{
		printf("ImportAbleSubcatalog: Failed Allocate (%d)\n", FSstatus);
		printf("   Your hard drive may be out of storage or may be read-only\n");
		FSClose(our_ref_num);
		HDelete(vol_ref_num, dir_id, (uint8 *) pas_name);
		free_big_memory(in_buf_handle);
		return (-1);
	}
	
	count=lcount;

	if (full_progress_desired)
		printf("AbleDiskTool: Importing disk image \"%s\"\n", tree_name);


	// Write the directory
	
	if ((FSstatus = SetFPos(our_ref_num, fsFromStart, 0)) != 0)
		{printf("ImportAbleSubcatalog: Failed SetFPos (%d)\n", FSstatus); FSClose(our_ref_num); HDelete(vol_ref_num, dir_id, (uint8 *) pas_name); free_big_memory(in_buf_handle); return (-1);}
		
	count = 4 * 512;								// write 4 sectors
	k     = count;
	
	if (((FSstatus = XPLRunTime_FSWrite(our_ref_num, &count, (void *)in_buf)) != 0)
	||  (count != k))
		{printf("ImportAbleSubcatalog: Failed FSWrite (%d)\n", FSstatus); FSClose(our_ref_num); HDelete(vol_ref_num, dir_id, (uint8 *) pas_name); free_big_memory(in_buf_handle); return (-1);}

	
	// now write out the directory contents

	i = 0;											// init number of words left to write
	
	while (i < copy_words-1024)
	{
		j = (copy_words - 1024 - i + 255) >> 8;		// get number of blocks

		if (j > chunk_blocks)						// limit to buffer size
			j = chunk_blocks;
		
		if (ReadAbleDisk(scsi_id, in_buf, block_num + ((dir_len + i) >> 8), j))
			{FSClose(our_ref_num); HDelete(vol_ref_num, dir_id, (uint8 *) pas_name); free_big_memory(in_buf_handle); return (-1);}
	
		if ((FSstatus = SetFPos(our_ref_num, fsFromStart, (i + 1024) << 1)) != 0)
			{printf("ImportAbleSubcatalog: Failed SetFPos (%d)\n", FSstatus); FSClose(our_ref_num); HDelete(vol_ref_num, dir_id, (uint8 *) pas_name); free_big_memory(in_buf_handle); return (-1);}
		
		count = j << 9;						/* number of  bytes to write	*/
		k     = count;
		
		if (((FSstatus = XPLRunTime_FSWrite(our_ref_num, &count, (void *)in_buf)) != 0)
		||  (count != k))
			{printf("ImportAbleSubcatalog: Failed FSWrite (%d)\n", FSstatus); FSClose(our_ref_num); HDelete(vol_ref_num, dir_id, (uint8 *) pas_name); free_big_memory(in_buf_handle); return (-1);}
	
		i += (j << 8);						/* compute new word position	*/
		
		run_host_environment_250();

		if (g_break_received)
		{
			FSClose(our_ref_num);
			HDelete(vol_ref_num, dir_id, (uint8 *) pas_name);
			free_big_memory(in_buf_handle);
			free_big_memory(in_buf_handle);
			return(-1);
		}
	}
	
	if ((FSstatus = SetEOF(our_ref_num, expanded_size_bytes)) != 0)		/* set length 		*/
		{printf("ImportAbleSubcatalog: Failed SetEOF (%d)\n", FSstatus); FSClose(our_ref_num); HDelete(vol_ref_num, dir_id, (uint8 *) pas_name); free_big_memory(in_buf_handle); return (-1);}

	if ((FSstatus = FSClose(our_ref_num)) != 0)
		{printf("ImportAbleSubcatalog: Failed FSClose (%d)\n", FSstatus); free_big_memory(in_buf_handle); return (-1);}
	
	free_big_memory(in_buf_handle);
	
	in_buf_handle = NULL; in_buf = NULL;
	
	return (0);
}


/*--------------------------------------------------------------------------*/
/*	ImportAbleDataFile														*/
/*--------------------------------------------------------------------------*/

static int ImportAbleDataFile(uint16 scsi_id, uint32 block_num, uint32 num_words, OSType file_type,
                              char *file_name, short vol_ref_num, long dir_id, char *tree_name)
{
	uint32	 	i,j,k;
	uint16		*in_buf = NULL;
	handle		in_buf_handle = NULL;
	OSErr		FSstatus;
	int			count;
	short		our_ref_num;
	uint32		chunk_blocks = MAX_TEXT_SIZE/512;
	char		pas_name[64];
 
	strcpy(pas_name, file_name);				// get working copy of file name
	
	if (pas_name[0] == '.')						// map leading . to * to avoid
		pas_name[0] = '*';						// mac os conflict with devices

	if (file_type == 'EXEC') strcat(pas_name, ".sprg");
	if (file_type == 'RLOC') strcat(pas_name, ".srel");
	if (file_type == 'DATA') strcat(pas_name, ".sdat");
	if (file_type == 'SQNC') strcat(pas_name, ".sseq");
	if (file_type == 'SNDF') strcat(pas_name, ".ssnd");
	if (file_type == 'SUBC') strcat(pas_name, ".simg");
	if (file_type == 'DUMP') strcat(pas_name, ".sdmp");
	if (file_type == 'SPEC') strcat(pas_name, ".sspe");
	if (file_type == 'INDX') strcat(pas_name, ".sind");
	if (file_type == 'TIMB') strcat(pas_name, ".stmb");
	
	c2pstr(pas_name);							// get in pascal format

	in_buf_handle = get_big_memory(MAX_TEXT_SIZE);
	
	if (in_buf_handle)
		in_buf = (uint16 *) *in_buf_handle;
		
	if (!in_buf_handle || !in_buf)
		{printf("ImportAbleDataFile: out of memory for data file buffer\n"); free_big_memory(in_buf_handle); return (-1);}
		
	/* Create output file: */
	
	if ((FSstatus = HCreate(vol_ref_num, dir_id, (uint8 *) pas_name, 'SNCL', file_type)) != 0)
	{
		if (FSstatus == dupFNErr)
			{printf("ImportAbleDataFile: Can't import %s; it already exists\n", tree_name); free_big_memory(in_buf_handle); return (-1);}
		else
			{printf("ImportAbleDataFile: Failed HCreate (%d)\n", FSstatus); free_big_memory(in_buf_handle); return (-1);}
	}
		
	if ((FSstatus = HOpen(vol_ref_num, dir_id, (uint8 *) pas_name, fsRdWrPerm, &our_ref_num)) != 0)
		{printf("ImportAbleDataFile: Failed HOpen (%d)\n", FSstatus); HDelete(vol_ref_num, dir_id, (uint8 *) pas_name); free_big_memory(in_buf_handle); return (-1);}
	
	count = num_words << 1;
	long lcount = count;
	
	if ((FSstatus = Allocate(our_ref_num, (SInt32 *) &lcount)) != 0)
	{
		printf("ImportAbleDataFile: Failed Allocate (%d)\n", FSstatus);
		printf("   Your hard drive may be out of storage or may be read-only\n");
		FSClose(our_ref_num);
		HDelete(vol_ref_num, dir_id, (uint8 *) pas_name);
		free_big_memory(in_buf_handle);
		return (-1);
	}

	count=lcount;

	if (full_progress_desired)
		printf("AbleDiskTool: Importing file \"%s\"\n", tree_name);

	i = 0;									/* init word position			*/
	
	while (i < num_words)
	{
		j = (num_words - i + 255) >> 8;		/* get number of blocks			*/

		if (j > chunk_blocks)				/* limit to buffer size			*/
			j = chunk_blocks;
		
		if (ReadAbleDisk(scsi_id, in_buf, block_num + (i >> 8), j))
			{FSClose(our_ref_num); HDelete(vol_ref_num, dir_id, (uint8 *) pas_name); free_big_memory(in_buf_handle); return (-1);}
	
		if ((FSstatus = SetFPos(our_ref_num, fsFromStart, i << 1)) != 0)
			{printf("ImportAbleDataFile: Failed SetFPos (%d)\n", FSstatus); FSClose(our_ref_num); HDelete(vol_ref_num, dir_id, (uint8 *) pas_name); free_big_memory(in_buf_handle); return (-1);}
		
		count = j << 9;						/* number of  bytes to write	*/
		k     = count;
		
		if (((FSstatus = XPLRunTime_FSWrite(our_ref_num, &count, (void *)in_buf)) != 0)
		||  (count != k))
			{printf("ImportAbleDataFile: Failed FSWrite (%d)\n", FSstatus); FSClose(our_ref_num); HDelete(vol_ref_num, dir_id, (uint8 *) pas_name); free_big_memory(in_buf_handle); return (-1);}
	
		i += (j << 8);						/* compute new word position	*/
		
		run_host_environment_250();
	
		if (g_break_received)
		{
			FSClose(our_ref_num);
			HDelete(vol_ref_num, dir_id, (uint8 *) pas_name);
			free_big_memory(in_buf_handle);
			return(-1);
		}
	}
	
	if ((FSstatus = SetEOF(our_ref_num, num_words << 1)) != 0)		/* set length 		*/
		{printf("ImportAbleDataFile: Failed SetEOF (%d)\n", FSstatus); FSClose(our_ref_num); HDelete(vol_ref_num, dir_id, (uint8 *) pas_name); free_big_memory(in_buf_handle); return (-1);}

	if ((FSstatus = FSClose(our_ref_num)) != 0)
		{printf("ImportAbleDataFile: Failed FSClose (%d)\n", FSstatus); free_big_memory(in_buf_handle); return (-1);}
	
	free_big_memory(in_buf_handle);
	
	in_buf_handle = NULL; in_buf = NULL;
	
	return (0);
}


/*--------------------------------------------------------------------------*/
/*	ImportAbleSoundFile														*/
/*--------------------------------------------------------------------------*/

static int ImportAbleSoundFile(uint16 scsi_id, uint32 block_num, uint32 num_words,
                              char *file_name, short vol_ref_num, long dir_id, char *tree_name)
{
	uint32	 			i,j,k;
	uint16				*in_buf = NULL;
	handle				in_buf_handle = NULL;
	OSErr				FSstatus;
	int					count;
	long				file_length;
	short				our_ref_num = 0;
	short				our_res_num = (-1);
	uint32				chunk_blocks = MAX_TEXT_SIZE/512;
	char				pas_name[64];
	SynclSFHeader		sf_header;
	AudioDataDescriptor	descriptor;
	 
	strcpy(pas_name, file_name);				// get working copy of file name
	
	if (pas_name[0] == '.')						// map leading . to * to avoid
		pas_name[0] = '*';						// mac os conflict with devices
		
	if (!import_as_aiff && !import_as_sd2f && !import_as_wave) strcat(pas_name, ".ssnd");

	c2pstr(pas_name);							// get in pascal format

	in_buf_handle = get_big_memory(MAX_TEXT_SIZE);
	
	if (in_buf_handle)
		in_buf = (uint16 *) *in_buf_handle;
		
	if (!in_buf_handle || !in_buf)
		{printf("ImportAbleSoundFile: out of memory for data file buffer\n"); free_big_memory(in_buf_handle); return (-1);}
		
	/* Create output file: */
	if      (import_as_aiff) FSstatus = HCreate(vol_ref_num, dir_id, (uint8 *) pas_name, 'TVOD', 'AIFF');
	else if (import_as_sd2f) FSstatus = HCreate(vol_ref_num, dir_id, (uint8 *) pas_name, 'TVOD', 'Sd2f');
	else if (import_as_wave) FSstatus = HCreate(vol_ref_num, dir_id, (uint8 *) pas_name, 'TVOD', 'WAVE');
	else                     FSstatus = HCreate(vol_ref_num, dir_id, (uint8 *) pas_name, 'SNCL', 'SNDF');
	
	if (FSstatus)
	{
		if (FSstatus == dupFNErr)
			{printf("ImportAbleSoundFile: Can't import %s; it already exists\n", tree_name); free_big_memory(in_buf_handle); return (-1);}
		else
			{printf("ImportAbleSoundFile: Failed HCreate (%d)\n", FSstatus); free_big_memory(in_buf_handle); return (-1);}
	}

	if (import_as_sd2f)
		HCreateResFile(vol_ref_num, dir_id, (uint8 *) pas_name);

	if ((FSstatus = HOpen(vol_ref_num, dir_id, (uint8 *) pas_name, fsRdWrPerm, &our_ref_num)) != 0)
		{printf("ImportAbleSoundFile: Failed HOpen (%d)\n", FSstatus); HDelete(vol_ref_num, dir_id, (uint8 *) pas_name); free_big_memory(in_buf_handle); return (-1);}
	
	if (import_as_sd2f)
		our_res_num = HOpenResFile(vol_ref_num, dir_id, (uint8 *) pas_name, fsRdWrPerm);
	
	if (num_words <= 3*256)
	{
		printf("ImportAbleSoundFile: Can't import %s; it is too short to be a sound file\n", tree_name);
		FSClose(our_ref_num);
		if (our_res_num != (-1)) CloseResFile(our_res_num);
		HDelete(vol_ref_num, dir_id, (uint8 *) pas_name);
		free_big_memory(in_buf_handle);
		return (-1);
	}
		
	// Read in sound file header
	if (ReadAbleDisk(scsi_id, in_buf, block_num, sizeof(SynclSFHeader) / 512))
		{FSClose(our_ref_num); if (our_res_num != (-1)) CloseResFile(our_res_num); HDelete(vol_ref_num, dir_id, (uint8 *) pas_name); free_big_memory(in_buf_handle); return (-1);}
	
	memcpy(&sf_header, in_buf, 3*256*2);					// copy sound file header all bytes
	zero_mem((byte *) &descriptor, sizeof(descriptor));		// init desriptor

	// The byte swizzling performed in ReadAbleDisk messed up the sound file header on Intel macs
	#if __LITTLE_ENDIAN__
	{
		Byte it;
		char me;
		
		sf_header.valid_data.sector   = ((sf_header.valid_data.sector   & 0x0000FFFF) << 16) | ((sf_header.valid_data.sector   & 0xFFFF0000) >> 16);
		sf_header.total_data.sector   = ((sf_header.total_data.sector   & 0x0000FFFF) << 16) | ((sf_header.total_data.sector   & 0xFFFF0000) >> 16);
		sf_header.total_length.sector = ((sf_header.total_length.sector & 0x0000FFFF) << 16) | ((sf_header.total_length.sector & 0xFFFF0000) >> 16);
		sf_header.loop_length.sector  = ((sf_header.loop_length.sector  & 0x0000FFFF) << 16) | ((sf_header.loop_length.sector  & 0xFFFF0000) >> 16);
		
		it = sf_header.smpte_seconds; sf_header.smpte_seconds = sf_header.smpte_frames; sf_header.smpte_frames  = it;
		it = sf_header.smpte_hours;   sf_header.smpte_hours   = sf_header.smpte_minutes;sf_header.smpte_minutes = it;
		
		for (i=0; i<256; i += 2)
		{
			me = sf_header.id_field[i]; sf_header.id_field[i] = sf_header.id_field[i+1]; sf_header.id_field[i+1] = me;
		}
		
		for (i=0; i<64; i++)
		{
            // I suspect this was the wrong thing to do. CJ 1/5/2015. But that's the way the files were written.
			for (j=0; j<8; j += 2)
			{
				me = sf_header.symbols[i].name[j]; sf_header.symbols[i].name[j] = sf_header.symbols[i].name[j+1]; sf_header.symbols[i].name[j+1] = me;
			}
		}
	}
	#endif
	
    // Store file handle in header
    char* fileHandle = (char *)&sf_header.file_handle[0];
    
    // Erase possible file handle that may-or-may-not match the current file name
    memset(fileHandle, 0, sf_file_handle_bl);
    strcpy(fileHandle, file_name);
    
    if (*fileHandle == '*')
        *fileHandle = '.';
    
	// Handle old SF headers as per ps.load
	if (sf_header.compatibility < 0)
		sf_header.compatibility = 0;

	if (sf_header.stereo != 0 && sf_header.stereo != 1)
	{
		printf("ImportAbleSoundFile: Can't import %s; it contains invalid data\n", tree_name);
		FSClose(our_ref_num);
		if (our_res_num != (-1)) CloseResFile(our_res_num);
		HDelete(vol_ref_num, dir_id, (uint8 *) pas_name);
		free_big_memory(in_buf_handle);
		return (-1);
	}
	
	if (sf_header.sample_rate == 0)
	{
		if (sf_header.compatibility < 1)
			sf_header.period_index = 600;
		
		else if (sf_header.period_index == 0)
			sf_header.period_index = 600;

		else if (sf_header.period_index < 300)
			sf_header.period_index = 300;
	}	
	
	// Import all valid data
	if (1)
	{
		descriptor.byte_len_in_file = (sf_header.valid_data.sector << 9) + (sf_header.valid_data.word_offset << 1);
		
		if (descriptor.byte_len_in_file > ((num_words - 3*256) << 1))
			descriptor.byte_len_in_file = ((num_words - 3*256) << 1);
			
		if (0)
		{
			// Could also extract mark start to mark end
		}
	}

	// Else could just use length of file; but may contain bogus data at end	
	else
		descriptor.byte_len_in_file  = (num_words - 3*256) << 1;
	
	descriptor.start_pos_in_file = 0;						// init start pos to 0; will be set correctly by appropriate Compute....SoundFileSize routine below

	if (import_as_wave)
		descriptor.bytes_need_swizzling = true;
	else
		descriptor.bytes_need_swizzling = false;
	
	descriptor.bits_per_sample   = 16;
	descriptor.samples_per_frame = sf_header.stereo+1;
	descriptor.frames_per_file   = descriptor.byte_len_in_file / descriptor.samples_per_frame / 2;
	
	if (sf_header.sample_rate == 0 && sf_header.period_index != 0)
		descriptor.frames_per_second = 30000000.0 / ((double) sf_header.period_index);
	else
		descriptor.frames_per_second = ((double) sf_header.sample_rate)*100.0;

	if (coerce_rates)
	{
		if (descriptor.frames_per_second > 48000.00)
			descriptor.frames_per_second = 48000.00;
	}
	
	if      (import_as_aiff) file_length = ComputeAIFFSoundFileSize(sf_header, descriptor);
	else if (import_as_sd2f) file_length = ComputeSd2fSoundFileSize(sf_header, descriptor);
	else if (import_as_wave) file_length = ComputeWAVESoundFileSize(sf_header, descriptor);
	else file_length = num_words << 1;

	count = file_length;
	long lcount = count;
	
	if ((FSstatus = Allocate(our_ref_num, (SInt32 *) &lcount)) != 0)
	{
		printf("ImportAbleSoundFile: Failed Allocate (%d)\n", FSstatus);
		printf("   Your hard drive may be out of storage or may be read-only\n");
		FSClose(our_ref_num);
		if (our_res_num != (-1)) CloseResFile(our_res_num); 
		HDelete(vol_ref_num, dir_id, (uint8 *) pas_name);
		free_big_memory(in_buf_handle);
		return (-1);
	}

	count=lcount;

	if (full_progress_desired)
		printf("AbleDiskTool: Importing file \"%s\"\n", tree_name);

	if ((FSstatus = SetFPos(our_ref_num, fsFromStart, 0)) != 0)
		{printf("ImportAbleSoundFile: Failed SetFPos (%d)\n", FSstatus); FSClose(our_ref_num); if (our_res_num != (-1)) CloseResFile(our_res_num); HDelete(vol_ref_num, dir_id, (uint8 *) pas_name); free_big_memory(in_buf_handle); return (-1);}
		
	// Write info
	int	xfer_limit;

	if      (import_as_aiff)
	{
		if (CreateAIFFSoundFile(our_ref_num, our_res_num, descriptor, sf_header))
			{FSClose(our_ref_num); if (our_res_num != (-1)) CloseResFile(our_res_num); HDelete(vol_ref_num, dir_id, (uint8 *) pas_name); free_big_memory(in_buf_handle); return (-1);}

		i = 3*256;							// audio data starts here
		xfer_limit = i + (descriptor.byte_len_in_file >> 1);
	}
	
	else if (import_as_sd2f)
	{
		if (CreateSd2fSoundFile(our_ref_num, our_res_num, descriptor, sf_header))
			{FSClose(our_ref_num); if (our_res_num != (-1)) CloseResFile(our_res_num); HDelete(vol_ref_num, dir_id, (uint8 *) pas_name); free_big_memory(in_buf_handle); return (-1);}

		i = 3*256;							// copy rest of data
		xfer_limit = i + (descriptor.byte_len_in_file >> 1);
	}
	
	else if (import_as_wave)
	{
		if (CreateWAVESoundFile(our_ref_num, our_res_num, descriptor, sf_header))
			{FSClose(our_ref_num); if (our_res_num != (-1)) CloseResFile(our_res_num); HDelete(vol_ref_num, dir_id, (uint8 *) pas_name); free_big_memory(in_buf_handle); return (-1);}

		i = 3*256;							// copy rest of data
		xfer_limit = i + (descriptor.byte_len_in_file >> 1);
	}
	
	else
	{
		i = 0;								// else copy entire sound file using loop below; pray swizzle == 0!
		xfer_limit = num_words;
	}
	
	while (i < xfer_limit)					// loop over words to copy
	{
		j = (xfer_limit - i + 255) >> 8;	// number of blocks remaining in file

		if (j > chunk_blocks)				// limit to size of buffer
			j = chunk_blocks;
		
		if (ReadAbleDisk(scsi_id, in_buf, block_num + (i >> 8), j))
			{FSClose(our_ref_num); if (our_res_num != (-1)) CloseResFile(our_res_num); HDelete(vol_ref_num, dir_id, (uint8 *) pas_name); free_big_memory(in_buf_handle); return (-1);}
	
		// Swizzle bytes if we need to
		if (descriptor.bytes_need_swizzling)
		{
			long	loop  = j * 256;
			fixed*  where = (fixed*) &in_buf[0];
			
			while (loop)
                {*where = Endian16_Swap(*where); where++; loop--;}
		}
		
		count = j << 9;							// get total number of bytes read in
		
		if (count > ((xfer_limit - i) << 1))	// if more than needed to fill out the file
			count = (xfer_limit - i) << 1;		// limit to valid data or mark end computed above
			
		k = count;

		if (((FSstatus = XPLRunTime_FSWrite(our_ref_num, &count, (void *)in_buf)) != 0)
		||  (count != k))
			{printf("ImportAbleSoundFile: Failed FSWrite (%d)\n", FSstatus); FSClose(our_ref_num); if (our_res_num != (-1)) CloseResFile(our_res_num); HDelete(vol_ref_num, dir_id, (uint8 *) pas_name); free_big_memory(in_buf_handle); return (-1);}
		
		i += (count >> 1);
		
		run_host_environment_250();
	
		if (g_break_received)
		{
			FSClose(our_ref_num);
			if (our_res_num != (-1)) CloseResFile(our_res_num);
			HDelete(vol_ref_num, dir_id, (uint8 *) pas_name);
			free_big_memory(in_buf_handle);
			return(-1);
		}
	}
	
	if ((FSstatus = SetEOF(our_ref_num, file_length)) != 0)		/* set length 		*/
		{printf("ImportAbleSoundFile: Failed SetEOF (%d)\n", FSstatus); if (our_res_num != (-1)) CloseResFile(our_res_num); FSClose(our_ref_num); HDelete(vol_ref_num, dir_id, (uint8 *) pas_name); free_big_memory(in_buf_handle); return (-1);}

	if (our_res_num != (-1)) CloseResFile(our_res_num);
	
	if ((FSstatus = FSClose(our_ref_num)) != 0)
		{printf("ImportAbleSoundFile: Failed FSClose (%d)\n", FSstatus); free_big_memory(in_buf_handle); return (-1);}
	
	// Create log file entry
	if (LogFileRefNum)
	{
		char* logEntry = (char*) in_buf;
		
		*logEntry = 0;
			
		if (LogNeedsHeader)
		{
			strcat(logEntry, "FileName");
			strcat(logEntry, "\t");
			strcat(logEntry, "FilePath");
			strcat(logEntry, "\t");
			strcat(logEntry, "Caption");
			strcat(logEntry, "\t");
			strcat(logEntry, "Category1");
			strcat(logEntry, "\t");
			strcat(logEntry, "Category2");
			strcat(logEntry, "\t");
			strcat(logEntry, "Category3");
			strcat(logEntry, "\t");
			strcat(logEntry, "Category4");
			strcat(logEntry, "\t");
			strcat(logEntry, "Category5");
			strcat(logEntry, "\n");
		
			LogNeedsHeader = false;
		}
		
		// SF Header caption byte order seems wrong on both platforms
		for (i=0; i<256; i += 2)
		{
			char me;

			me = sf_header.id_field[i]; sf_header.id_field[i] = sf_header.id_field[i+1]; sf_header.id_field[i+1] = me;
		}

		strcat(logEntry, file_name);
		strcat(logEntry, "\t");
		strcat(logEntry, "/Volumes/");
		
		i = strlen(logEntry);
		
		strcat(logEntry, tree_name);
		
		j = strlen(logEntry);
		
		while (i<j)
		{
			if (logEntry[i] == ':')
				logEntry[i] = '/';
			i++;
		}
				
		strcat(logEntry, "\t");
		logEntry[strlen(logEntry)+sf_header.id_field_bytes] = 0;
		strncpy(logEntry+strlen(logEntry), sf_header.id_field, sf_header.id_field_bytes);
		
		// Optical categories
		if (sf_header.index_base && sf_header.index_base >= 128 && sf_header.index_base < 256)
		{
			int start = 256 - 2*(256 - sf_header.index_base);
			
			while (start < 256)
			{
				int length = sf_header.id_field[start];
				
				strcat(logEntry, "\t");
				logEntry[strlen(logEntry)+length] = 0;
				strncpy(logEntry+strlen(logEntry), sf_header.id_field+start+2, length);
				
				start = start + length + (length&1) + 2;
			}
		}
		
		strcat(logEntry, "\n");
		
		count = strlen(logEntry);
		
		if (((FSstatus = FSWrite(LogFileRefNum, (SInt32 *) &count, (void *)logEntry)) != 0)
		||  (count != strlen(logEntry)))
			{printf("ImportAbleSoundFile: Failed write of log file (%d)\n", FSstatus); free_big_memory(in_buf_handle); return (-1);}
	}
	
	// Done
	free_big_memory(in_buf_handle);
	
	in_buf_handle = NULL; in_buf = NULL;
	
	return (0);
}


/*--------------------------------------------------------------------------*/
/*	ExportAbleDataFile														*/
/*--------------------------------------------------------------------------*/

static int ExportAbleDataFile(uint16 scsi_id, uint32 block_num, uint32 num_words, short our_ref_num)
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
		
		if ((FSstatus = SetFPos(our_ref_num, fsFromStart, i << 1)) != 0)
			{printf("ExportAbleDataFile: Failed SetFPos (%d)\n", FSstatus); free_big_memory(in_buf_handle); return (-1);}
		
		count = count << 1;					/* number of  bytes to read	*/
		k     = count;
		
		if (((FSstatus = XPLRunTime_FSRead(our_ref_num, &count, (void *)in_buf)) != 0)
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

static void Scan__SIZE__File(uint32 byte_len, short our_ref_num, const char *error_name,
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
	
	if ((FSstatus = SetFPos(our_ref_num, fsFromStart, 0)) != 0)
		{printf("Scan__SIZE__File: Failed SetFPos for \"%s\" (%d)\n", error_name, FSstatus); return;}
	
	count = byte_len;
	
	// No swap needed since we are reading a text file here
	if (((FSstatus = FSRead(our_ref_num, (SInt32 *) &count, (void *)line)) != 0)
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

static uint32 ComputeWordLengthOfMacTextFile(uint32 byte_len, short our_ref_num, const char *error_name)
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
		
	if (byte_len > MAX_TEXT_SIZE)
		{printf("ComputeWordLengthOfMacTextFile: File \"%s\" is too big to export\n", error_name); return (uint32) (-1);}
	
	in_buf_handle = get_big_memory(MAX_TEXT_SIZE);
	
	if (in_buf_handle)
		in_buf = (char *) *in_buf_handle;
		
	if (!in_buf_handle || !in_buf)
		{printf("ComputeWordLengthOfMacTextFile: out of memory for file length computation\n"); free_big_memory(in_buf_handle); return (uint32) (-1);}
		
	if ((FSstatus = SetFPos(our_ref_num, fsFromStart, 0)) != 0)
		{printf("ComputeWordLengthOfMacTextFile: Failed SetFPos for \"%s\" (%d)\n", error_name, FSstatus); free_big_memory(in_buf_handle); return (uint32) (-1);}
	
	count = byte_len;
	long lcount = count;
	
	// No byte swap here - reaading text file and processing bytes
	if (((FSstatus = FSRead(our_ref_num, (SInt32 *) &lcount, (void *)in_buf)) != 0)
	||  (lcount != byte_len))
		{printf("ComputeWordLengthOfMacTextFile: Failed FSRead for \"%s\" (%d)\n", error_name, FSstatus); free_big_memory(in_buf_handle); return (uint32) (-1);}

	count=lcount;

	out_words = 0;
	need_sol  = true;
	need_eol  = false;
	saved     = false;
	
	for (i = 0; i < byte_len; i++)			/* process export bytes			*/
	{
		j = in_buf[i] & 0xFF;				/* get the character			*/
		
		if (need_sol)						/* emit start-of-line if req'd	*/
		{
			out_words++;					/* line number space			*/
			need_sol = false;
		}
		
		if (j == MAC_OS_FILE_EOLCHAR)		/* character is new_line		*/
		{
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
/*	ComputeSqueezedSubcatSize												*/
/*--------------------------------------------------------------------------*/

/* returns Able word length of a disk image after squeezing */

static uint32 ComputeSqueezedSubcatSize (uint32 byte_len, short our_ref_num, const char *error_name, fixed type)
{
	uint32		i, dir_len;
	uint32		start_block;
	uint32		block_len;
	uint32		end_block;
	uint32		largest_end_block = 0;
	uint32		num_words = byte_len >> 1;
	int			count;
	OSErr		FSstatus;
	fixed		the_buf[1024];
		
	if (type == t_subc)
		dir_len = 256;
	else
		dir_len = 1024;
	
	if ((byte_len >> 1) < dir_len)
		{printf("ComputeSqueezedSubcatSize: file '%s' can't be expport because it has a catalog error in it\n", error_name); return (uint32) (-1);}

	if ((FSstatus = SetFPos(our_ref_num, fsFromStart, 0)) != 0)
		{printf("ComputeSqueezedSubcatSize: Failed SetFPos for \"%s\" (%d)\n", error_name, FSstatus); return (uint32) (-1);}
	
	count = dir_len << 1;
	
	if (((FSstatus = XPLRunTime_FSRead(our_ref_num, &count, (void *)the_buf)) != 0)
	||  (count != dir_len << 1))
		{printf("ComputeSqueezedSubcatSize: Failed FSRead for \"%s\" (%d)\n", error_name, FSstatus); return (uint32) (-1);}

	i = 0;
	while (i < dir_len)
	{
		start_block = ((uint32) (uint16) the_buf[i+4]) + ((((uint32) (uint16) the_buf[i+7]) & 0xFF00) <<  8);
		block_len   = ((uint32) (uint16) the_buf[i+5]) + ((((uint32) (uint16) the_buf[i+7]) & 0x00F0) << 12);
		end_block   = start_block + block_len;
		if (end_block > largest_end_block) largest_end_block = end_block;
		
		i += 8;
	}
	
	if (largest_end_block < (dir_len >> 8))
		largest_end_block = (dir_len >> 8);
	
	if ((largest_end_block << 8) < num_words)		// if no catalog error
		num_words = largest_end_block << 8;			// this is number of words to use

	return (num_words);
}


/*--------------------------------------------------------------------------*/
/*	ComputeSoundFileSize											        */
/*--------------------------------------------------------------------------*/

/* returns Able word length of a Mac sound file */

static uint32 ComputeSoundFileSize(short our_ref_num, short res_ref_num, const char *error_name, uint32 type)
{
	SynclSFHeader		header;
	AudioDataDescriptor	descriptor;

	if (type == AIFFID)
	{
		if (ParseAIFFSoundFile(our_ref_num, res_ref_num, descriptor, header))
			{printf("ComputeSoundFileSize: File \"%s\" cannot be analyzed\n", error_name); return (uint32) (-1);}
	}
	
	else if (type == SD2FileType)
	{
		if (ParseSd2fSoundFile(our_ref_num, res_ref_num, descriptor, header))
			{printf("ComputeSoundFileSize: File \"%s\" cannot be analyzed\n", error_name); return (uint32) (-1);}
	}

	else if (type == WAVEID)
	{
		if (ParseWAVESoundFile(our_ref_num, res_ref_num, descriptor, header))
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

static int ExportSoundFileFile(uint16 scsi_id, uint32 block_num, short our_ref_num, short res_ref_num, uint32 type)
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
		if (ParseAIFFSoundFile(our_ref_num, res_ref_num, descriptor, header))
			return (-1);
	}
	
	else if (type == SD2FileType)
	{
		if (ParseSd2fSoundFile(our_ref_num, res_ref_num, descriptor, header))
			return (-1);
	}

	else if (type == WAVEID)
	{
		if (ParseWAVESoundFile(our_ref_num, res_ref_num, descriptor, header))
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
		
	if ((FSstatus = SetFPos(our_ref_num, fsFromStart, descriptor.start_pos_in_file)) != 0)
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
						if (((FSstatus = XPLRunTime_FSRead(our_ref_num, &count, (void *)&out_buf[write_where])) != 0)
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
						
						if (((FSstatus = XPLRunTime_FSRead(our_ref_num, &count, (void *)in_buf)) != 0)
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

static int ExportAbleTextFile(uint16 scsi_id, uint32 block_num, uint32 byte_len, short our_ref_num, uint32 word_len)
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
		
	if ((FSstatus = SetFPos(our_ref_num, fsFromStart, 0)) != 0)
		{printf("ExportAbleTextFile: Failed SetFPos (%d)\n", FSstatus); free_big_memory(in_buf_handle); free_big_memory(out_buf_handle); return (-1);}
	
	count = byte_len;
	
	// No byte swap needed - reading tex file here
	if (((FSstatus = FSRead(our_ref_num, (SInt32 *) &count, (void *)in_buf)) != 0)
	||  (count != byte_len))
		{printf("ExportAbleTextFile: Failed FSRead (%d)\n", FSstatus); free_big_memory(in_buf_handle); free_big_memory(out_buf_handle); return (-1);}

	out_words = 0;
	need_sol  = true;
	need_eol  = false;
	saved     = false;
	the_word  = 0;
		
	for (i = 0; i < byte_len; i++)			/* process export bytes			*/
	{
		j = in_buf[i] & 0xFF;				/* get the character			*/
		
		if (need_sol)						/* emit start-of-line if req'd	*/
		{
			out_buf[out_words++] = line_no++;
			need_sol = false;
		}
		
		if (j == MAC_OS_FILE_EOLCHAR)		/* character is new_line		*/
		{
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
/*	FetchAndRecurseAbleCatalog												*/
/*--------------------------------------------------------------------------*/

/* This routine allocates block of memory and reads in an Able disk			*/
/* directory.  It then recurses through sub-directories.					*/

static int FetchAndRecurseAbleCatalog(char *name, uint16 scsi_id, uint32 block_num, uint32 num_blocks,
                                      short vol_ref_num, long cat_dir_id,
									  boolean	force_in, char *host_name)
{
	uint32 	byte_len  = num_blocks * 512;
	int		num_words = num_blocks * 256;
	uint16 	*cat_buf  = 0;
	int 	i,j;
	uint32	start_block;
	uint32	block_len;
	uint32	end_block;
	uint32	word_len;
	uint32	file_type;
	uint32	largest_end_block = 0;
	uint32  num_entries       = 0;
	SInt32	sub_dir_id        = 0;
	OSErr	FSstatus;
	
	char	file_name   [16];
	char	tweaked_name[16];
	static	long our_level = 0;
	char	sub_name[512] = {""};
	char	mac_name[512] = {""};
	
	cat_buf = (uint16 *) malloc(byte_len);					/* get storage					*/

	if (!cat_buf)
		{printf("FetchAndRecurseAbleCatalog: Failed malloc\n"); return (-1);}

	if (ReadAbleDisk(scsi_id, cat_buf, block_num, num_blocks))
		{free(cat_buf); return (-1);}

	/* Perform a 3 pass algorithm:															*/
	/*	  first dump out catalog in hex														*/
	/*	  then print out catalog contents													*/
	/*	  then recurse into nested subcatalogs...											*/


	/* 1: dump contents: */

	if (dumpcontents
	&& (!onlyaccess || strcmp(name, only_name) == 0 || force_in))
		DumpAbleCatalog(cat_buf, num_words, name);
		
	if (g_break_received)
		{free(cat_buf); return(-1);}
	
	
	/* 2: import/catalog contents: */
	
	if (catalog || import_file || read_directory || (dumpcontents && onlyaccess))
	{
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
	
			if (end_block > largest_end_block) largest_end_block = end_block;
			
			if (cat_buf[i])										/* if name exists			*/
			{
				AbleFileName2CStr(cat_buf+i, file_name);
				
				if (CheckAbleFileName(cat_buf+i))				// get correct type of file for old non-typed special files
					file_type = CheckAbleFileName(cat_buf+i);

				if (strlen(name) < 512)
				{
					strncpy(sub_name, name,      sizeof(sub_name));
					strncat(sub_name,":",        sizeof(sub_name) - strlen(sub_name) - 1);
					strncat(sub_name,file_name,  sizeof(sub_name) - strlen(sub_name) - 1);
				}
				else sub_name[0] = 0;
				
				strcpy(tweaked_name, file_name);
				
				if (tweaked_name[0] == '.')
					tweaked_name[0] = '*';
				
				if (strlen(name) < 512)
				{
					strncpy(mac_name, host_name,   sizeof(mac_name));
					strncat(mac_name,":",          sizeof(mac_name) - strlen(mac_name) - 1);
					strncat(mac_name,tweaked_name, sizeof(mac_name) - strlen(mac_name) - 1);
				}
				else mac_name[0] = 0;
				
				if (read_directory)
					num_entries++;
				
				if (catalog
				&& (!onlyaccess || strcmp(sub_name, only_name) == 0 || force_in)
				&& (!skipdots   || file_name[0] != '.'))
				{
					printf("   %s", sub_name);
					
					j = strlen(sub_name);
					
					while (j < 48)
					{
						printf(" ");
						j++;
					}
					
					printf("%8d %8d %8d    ", start_block, block_len, word_len);
				}
				
				switch (file_type)
				{
					case t_text:
						if (catalog
						&& (!onlyaccess || strcmp(sub_name, only_name) == 0 || force_in)
						&& (!systemonly || is_able_system_entity(sub_name))
						&& (!skipdots   || file_name[0] != '.'))
							printf("TEXT\n");
						
						if (import_file
						&& (!onlyaccess || strcmp(sub_name, only_name) == 0 || force_in)
						&& (!systemonly || is_able_system_entity(sub_name))
						&& (!skipdots   || file_name[0] != '.'))
						{
							if (ImportAbleTextFile(scsi_id, block_num+start_block, word_len,
											       file_name, vol_ref_num, cat_dir_id, mac_name))
								if (stop_on_any_error || g_break_received)
									{free(cat_buf); return (-1);}
						}
								
						if (dumpcontents
						&& (!onlyaccess || strcmp(sub_name, only_name) == 0 || force_in)
						&& (!systemonly || is_able_system_entity(sub_name))
						&& (!skipdots   || file_name[0] != '.'))
						{
							if (DumpAbleTextFile(scsi_id, block_num+start_block, word_len))
								if (stop_on_any_error || g_break_received)
									{free(cat_buf); return (-1);}
						}
						
						break;
						
					case t_exec:
						if (catalog
						&& (!onlyaccess || strcmp(sub_name, only_name) == 0 || force_in)
						&& (!systemonly || is_able_system_entity(sub_name))
						&& (!skipdots   || file_name[0] != '.'))
							printf("EXECUTABLE\n");
						
						if (import_file
						&& (!onlyaccess || strcmp(sub_name, only_name) == 0 || force_in)
						&& (!systemonly || is_able_system_entity(sub_name))
						&& (!skipdots   || file_name[0] != '.')
						&& (!textonly))
						{
							if (ImportAbleDataFile(scsi_id, block_num+start_block, word_len, 'EXEC',
											       file_name, vol_ref_num, cat_dir_id, mac_name))
								if (stop_on_any_error || g_break_received)
									{free(cat_buf); return (-1);}
						}
						
						break;
						
					case t_reloc:
						if (catalog
						&& (!onlyaccess || strcmp(sub_name, only_name) == 0 || force_in)
						&& (!systemonly || is_able_system_entity(sub_name))
						&& (!skipdots   || file_name[0] != '.'))
							printf("RELOCATABLE\n");
						
						if (import_file
						&& (!onlyaccess || strcmp(sub_name, only_name) == 0 || force_in)
						&& (!systemonly || is_able_system_entity(sub_name))
						&& (!skipdots   || file_name[0] != '.')
						&& (!textonly))
						{
							if (ImportAbleDataFile(scsi_id, block_num+start_block, word_len, 'RLOC',
											       file_name, vol_ref_num, cat_dir_id, mac_name))
								if (stop_on_any_error || g_break_received)
									{free(cat_buf); return (-1);}
						}
						
						break;
						
					case t_data:
						if (catalog
						&& (!onlyaccess || strcmp(sub_name, only_name) == 0 || force_in)
						&& (!systemonly || is_able_system_entity(sub_name))
						&& (!skipdots   || file_name[0] != '.'))
							printf("DATA\n");
						
						if (import_file
						&& (!onlyaccess || strcmp(sub_name, only_name) == 0 || force_in)
						&& (!systemonly || is_able_system_entity(sub_name))
						&& (!skipdots   || file_name[0] != '.')
						&& (!textonly))
						{
							if (ImportAbleDataFile(scsi_id, block_num+start_block, word_len, 'DATA',
											       file_name, vol_ref_num, cat_dir_id, mac_name))
								if (stop_on_any_error || g_break_received)
									{free(cat_buf); return (-1);}
						}
						
						break;
						
					case t_sync:
						if (catalog
						&& (!onlyaccess || strcmp(sub_name, only_name) == 0 || force_in)
						&& (!systemonly || is_able_system_entity(sub_name))
						&& (!skipdots   || file_name[0] != '.'))
							printf("SEQUENCE\n");
						
						if (import_file
						&& (!onlyaccess || strcmp(sub_name, only_name) == 0 || force_in)
						&& (!systemonly || is_able_system_entity(sub_name))
						&& (!skipdots   || file_name[0] != '.')
						&& (!textonly))
						{
							if (ImportAbleDataFile(scsi_id, block_num+start_block, word_len, 'SQNC',
											       file_name, vol_ref_num, cat_dir_id, mac_name))
								if (stop_on_any_error || g_break_received)
									{free(cat_buf); return (-1);}
						}
						
						break;
						
					case t_sound:
						if (catalog
						&& (!onlyaccess || strcmp(sub_name, only_name) == 0 || force_in)
						&& (!systemonly || is_able_system_entity(sub_name))
						&& (!skipdots   || file_name[0] != '.'))
							printf("SOUND\n");
						
						if (import_file
						&& (!onlyaccess || strcmp(sub_name, only_name) == 0 || force_in)
						&& (!systemonly || is_able_system_entity(sub_name))
						&& (!skipdots   || file_name[0] != '.')
						&& (!textonly))
						{
							if (ImportAbleSoundFile(scsi_id, block_num+start_block, word_len,
											       file_name, vol_ref_num, cat_dir_id, mac_name))
								if (stop_on_any_error || g_break_received)
									{free(cat_buf); return (-1);}
						}
						
						break;
						
					case t_subc:
						if (catalog
						&& (!onlyaccess || strcmp(sub_name, only_name) == 0 || force_in)
						&& (!systemonly || is_able_system_entity(sub_name))
						&& (!skipdots   || file_name[0] != '.'))
							printf("SUBCATALOG\n");
							
						if (import_file
						&& (!onlyaccess || strcmp(sub_name, only_name) == 0 || force_in)
						&& (!systemonly || is_able_system_entity(sub_name))
						&& (!skipdots   || file_name[0] != '.')
						&& (!textonly)
						&& (import_subcats_as_images))
						{
							if (ImportAbleSubcatalog(scsi_id, block_num+start_block, word_len, 'SUBC',
											        file_name, vol_ref_num, cat_dir_id, mac_name, t_subc, sub_name))
								if (stop_on_any_error || g_break_received)
									{free(cat_buf); return (-1);}
						}
						
						break;
		
					case t_lsubc:
						if (catalog
						&& (!onlyaccess || strcmp(sub_name, only_name) == 0 || force_in)
						&& (!systemonly || is_able_system_entity(sub_name))
						&& (!skipdots   || file_name[0] != '.'))
							printf("LARGE SUBCATALOG\n");
						
						if (import_file
						&& (!onlyaccess || strcmp(sub_name, only_name) == 0 || force_in)
						&& (!systemonly || is_able_system_entity(sub_name))
						&& (!skipdots   || file_name[0] != '.')
						&& (!textonly)
						&& (import_subcats_as_images))
						{
							if (ImportAbleSubcatalog(scsi_id, block_num+start_block, word_len, 'SUBC',
											        file_name, vol_ref_num, cat_dir_id, mac_name, t_lsubc, sub_name))
								if (stop_on_any_error || g_break_received)
									{free(cat_buf); return (-1);}
						}
						
						break;
		
					case t_dump:
						if (catalog
						&& (!onlyaccess || strcmp(sub_name, only_name) == 0 || force_in)
						&& (!systemonly || is_able_system_entity(sub_name))
						&& (!skipdots   || file_name[0] != '.'))
							printf("DUMP\n");
						
						if (import_file
						&& (!onlyaccess || strcmp(sub_name, only_name) == 0 || force_in)
						&& (!systemonly || is_able_system_entity(sub_name))
						&& (!skipdots   || file_name[0] != '.')
						&& (!textonly))
						{
							if (ImportAbleDataFile(scsi_id, block_num+start_block, word_len, 'DUMP',
											       file_name, vol_ref_num, cat_dir_id, mac_name))
								if (stop_on_any_error || g_break_received)
									{free(cat_buf); return (-1);}
						}
						
						break;
		
					case t_spect:
						if (catalog
						&& (!onlyaccess || strcmp(sub_name, only_name) == 0 || force_in)
						&& (!systemonly || is_able_system_entity(sub_name))
						&& (!skipdots   || file_name[0] != '.'))
							printf("SPECTRAL\n");
						
						if (import_file
						&& (!onlyaccess || strcmp(sub_name, only_name) == 0 || force_in)
						&& (!systemonly || is_able_system_entity(sub_name))
						&& (!skipdots   || file_name[0] != '.')
						&& (!textonly))
						{
							if (ImportAbleDataFile(scsi_id, block_num+start_block, word_len, 'SPEC',
											       file_name, vol_ref_num, cat_dir_id, mac_name))
								if (stop_on_any_error || g_break_received)
									{free(cat_buf); return (-1);}
						}
						
						break;
		
					case t_index:
						if (catalog
						&& (!onlyaccess || strcmp(sub_name, only_name) == 0 || force_in)
						&& (!systemonly || is_able_system_entity(sub_name))
						&& (!skipdots   || file_name[0] != '.'))
							printf("INDEX\n");
						
						if (import_file
						&& (!onlyaccess || strcmp(sub_name, only_name) == 0 || force_in)
						&& (!systemonly || is_able_system_entity(sub_name))
						&& (!skipdots   || file_name[0] != '.')
						&& (!textonly))
						{
							if (ImportAbleDataFile(scsi_id, block_num+start_block, word_len, 'INDX',
											       file_name, vol_ref_num, cat_dir_id, mac_name))
								if (stop_on_any_error || g_break_received)
									{free(cat_buf); return (-1);}
						}
											 
						break;
		
					case t_timbre:
						if (catalog
						&& (!onlyaccess || strcmp(sub_name, only_name) == 0 || force_in)
						&& (!systemonly || is_able_system_entity(sub_name))
						&& (!skipdots   || file_name[0] != '.'))
							printf("TIMBRE\n");
						
						if (import_file
						&& (!onlyaccess || strcmp(sub_name, only_name) == 0 || force_in)
						&& (!systemonly || is_able_system_entity(sub_name))
						&& (!skipdots   || file_name[0] != '.')
						&& (!textonly))
						{
							if (ImportAbleDataFile(scsi_id, block_num+start_block, word_len, 'TIMB',
											       file_name, vol_ref_num, cat_dir_id, mac_name))
								if (stop_on_any_error || g_break_received)
									{free(cat_buf); return (-1);}
						}
								
						break;
				}
			}
			
			i += 8;
			
			run_host_environment_250();
			
			if (g_break_received)
				{free(cat_buf); return(-1);}
		}
	}
	
	
	/* 3: now recurse through subcatalogs, or read directory: */
	
	if (read_directory || (recurs && !import_subcats_as_images))
	{
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
	
			if (end_block > largest_end_block) largest_end_block = end_block;
			
			if (cat_buf[i])										/* if name exists			*/
			{
				AbleFileName2CStr(cat_buf+i, file_name);
				
				if (CheckAbleFileName(cat_buf+i))				// get correct type of file for old non-typed special files
					file_type = CheckAbleFileName(cat_buf+i);

				if (strlen(name) < 512)
				{
					strncpy(sub_name, name,      sizeof(sub_name));
					strncat(sub_name,":",        sizeof(sub_name) - strlen(sub_name) - 1);
					strncat(sub_name,file_name,  sizeof(sub_name) - strlen(sub_name) - 1);
				}
				else sub_name[0] = 0;
				
				strcpy(tweaked_name, file_name);
				
				if (tweaked_name[0] == '.')
					tweaked_name[0] = '*';
				
				if (strlen(name) < 512)
				{
					strncpy(mac_name, host_name,   sizeof(mac_name));
					strncat(mac_name,":",          sizeof(mac_name) - strlen(mac_name) - 1);
					strncat(mac_name,tweaked_name, sizeof(mac_name) - strlen(mac_name) - 1);
				}
				else mac_name[0] = 0;
				
				if (recurs && !import_subcats_as_images)
				switch (file_type)
				{
					case t_text:
					case t_exec:
					case t_reloc:
					case t_sync:
						break;
						
					case t_subc:
						if (import_file
						&& (!onlyaccess || strcmp(sub_name, only_name) == 0 || force_in)
						&& (!systemonly || is_able_system_entity(sub_name))
						&& (!skipdots   || file_name[0] != '.'))
						{
							if (file_name[0] == '.')				/* map leading . to # to avoid	*/
								file_name[0] = '*';					/* mac os conflict with devices	*/
							
							c2pstr(file_name);			/* get in pascal format			*/
		
							if ((FSstatus = DirCreate(vol_ref_num, cat_dir_id, (uint8*) file_name, &sub_dir_id)) != 0)
							{
								printf("FetchAndRecurseAbleCatalog: Failed DirCreate (%d)\n", FSstatus);
								
								if (stop_on_any_error || g_break_received)
									{free(cat_buf); return (-1);}
							}
							
							p2cstr((uint8*) file_name);			/* get back in c format			*/

							if (!ignore__size__)
								Create__size__File(1, vol_ref_num, sub_dir_id, block_len);

							if (progress_desired || full_progress_desired)
								printf("AbleDiskTool: Created folder \"%s\"\n", mac_name);
						}
						else							/* else import selectively into top level folder */
							sub_dir_id = cat_dir_id;

						our_level += 4;
						
						if (FetchAndRecurseAbleCatalog(sub_name, scsi_id, block_num+start_block, 1, vol_ref_num, sub_dir_id,
						                               force_in || (onlyaccess && strcmp(sub_name, only_name) == 0), mac_name))
							if (stop_on_any_error || g_break_received)
								{free(cat_buf); return (-1);}
							
						our_level -= 4;
						
						break;
		
					case t_lsubc:
						if (import_file
						&& (!onlyaccess || strcmp(sub_name, only_name) == 0 || force_in)
						&& (!systemonly || is_able_system_entity(sub_name))
						&& (!skipdots   || file_name[0] != '.'))
						{
							if (file_name[0] == '.')				/* map leading . to # to avoid	*/
								file_name[0] = '*';					/* mac os conflict with devices	*/
							
							c2pstr(file_name);			/* get in pascal format			*/
		
							if ((FSstatus = DirCreate(vol_ref_num, cat_dir_id, (uint8*) file_name, &sub_dir_id)) != 0)
							{
								printf("FetchAndRecurseAbleCatalog: Failed DirCreate (%d)\n", FSstatus);
								
								if (stop_on_any_error || g_break_received)
									{free(cat_buf); return (-1);}
							}
							
							p2cstr((uint8*) file_name);			/* get back in c format			*/

							if (!ignore__size__)
								Create__size__File(4, vol_ref_num, sub_dir_id, block_len);
							
							if (progress_desired || full_progress_desired)
								printf("AbleDiskTool: Created folder \"%s\"\n", mac_name);
						}
						else							/* else import selectively into top level folder */
							sub_dir_id = cat_dir_id;

						our_level += 4;
						
						if (FetchAndRecurseAbleCatalog(sub_name, scsi_id, block_num+start_block, 4, vol_ref_num, sub_dir_id,
						                               force_in || (onlyaccess && strcmp(sub_name, only_name) == 0), mac_name))
							if (stop_on_any_error || g_break_received)
								{free(cat_buf); return (-1);}

						our_level -= 4;
						
						break;
		
					case t_dump:
					case t_spect:
					case t_index:
						break;
				}
			}
			
			i += 8;

			run_host_environment_250();
			
			if (g_break_received)
				{free(cat_buf); return(-1);}
		}
	}
	
	free(cat_buf); cat_buf = 0;
	
	return (0);
}


/*--------------------------------------------------------------------------*/
/*	main() for AbleDiskTool													*/
/*--------------------------------------------------------------------------*/

mac_file_info	CurrentImportFile;
mac_file_info	CurrentExportFile;
short			ImportVolRefNum;

#define	MAX_LEVELS		20									// max # of folder levels
#define MAX_FOLDERS		5000   								// max # of folders

static	int		this_index[MAX_LEVELS];						// current index for this level
static	uint32	this_size [MAX_LEVELS];						// accumulated size of this level
static	uint32	spec_size [MAX_LEVELS];						// special size information available for this
static	uint32	spec_style[MAX_LEVELS];						// subcat style (large VS small) specified
static	long	this_dir  [MAX_LEVELS];						// current directory id being scanned
static	long	fldr_id   [MAX_LEVELS];						// which folder currently being scaned at this level
static	long	entries   [MAX_LEVELS];						// number of able entries on this level
static	FSSpec	lev_spec  [MAX_LEVELS];						// holds fsspec for folder being scanned

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
	
	if (AbleSimFileRefNum)
	{
		FSClose(AbleSimFileRefNum);
		AbleSimFileRefNum = 0;
	}

	if (LogFileRefNum)
	{
		FSClose(LogFileRefNum);
		LogFileRefNum = 0;
		FlushVol(NULL, LogVolRefNum);
		LogVolRefNum = 0;
	}

	CloseMacFile(CurrentImportFile);
	CloseMacFile(CurrentExportFile);
	
	if (ImportVolRefNum)
	{
		FlushVol(NULL, ImportVolRefNum);
		ImportVolRefNum = 0;
	}
	
	// Release Synclav
	if (gAbleAccessor)
	{
		gAbleAccessor = NULL;

		D24Sim_CleanUp();
	}

	cleanup_run_time_environment();
    
    if (map_set) {		// Close image files
		XPLRunTime_CloseupSCSIMap();
        map_set = false;
    }

    toss_interchange_setup(&InterchangeSettings);
	initialize_static_data();

	if (g_disallow_run_host_exit)						// allow host exits if caller allows them
		g_disallow_run_host_exit--;
}

static void print_help()
{
	printf("AbleDiskTool: Version 2.2 dated 10/1/2017 (%lu)\n", sizeof(long));
	printf("Usage:  AbleDiskTool <command> <options>\n"); 
	printf("\n");
	printf("Note: use * instead of • (bullet) from Terimal!!!\n");
	printf("\n");
	printf("commands:\n");
	printf("        import <drive> \"/ToMacFolderName\"      import entire drive or image file to Macintosh folder converting sound files to AIFF\n");
	printf("\n");
	printf("drives:\n");
	printf("        file \"FileName\"                        specify a Macintosh-resident Synclavier Disk Image File - use quotes if needed\n"); 
	printf("        scsi 6                                 specify legacy SCSI drive connected via Ratoc FR1SX FireWire-SCSI Converter\n");
	printf("        W0                                     specify source is current InterChange™ setting for W0:\n");
	printf("        W1                                     specify source is current InterChange™ setting for W1:\n");
	printf("\n");
	printf("Examples:\n");
	printf("        AbleDiskTool import file \"/ImageFile\" \"/ToMacFolderName\"\n");
	printf("        AbleDiskTool import scsi 6 \"/ToMacFolderName\"\n");
	printf("        AbleDiskTool import W0 \"/ToMacFolderName\"\n");
	printf("        AbleDiskTool import W0 \"~/Desktop/MacFolderName\"\n");
	printf("        AbleDiskTool -ic2 4 -e \"~/Desktop/*NEWDATA.stmb\" W1:\n");
	printf("\n");
	printf("Other options not formally supported:\n");
	printf("        -s scsi_id                             specify SCSI id to use\n");
	printf("        -pci                                   use Synclavier D24 bus\n");
    printf("        -ic2 5                                 use InterChange2 setup scsi ID (5==W0; 4==W1)\n");
    printf("        -inq                                   perform Inquiry before operation\n");
	printf("        -t                                     perform Test Unit Ready before operation\n");
	printf("        -d                                     dump out catalog sectors in hex\n");
	printf("        -e mac_entity    syncl_name            export Macintosh entity to Synclavier Drive or Disk Image File\n");
	printf("        -i syncl_entity  mac_name              import Synclavier file or subcatalog to the Macintosh\n");
	printf("        -p                                     emit folder-level progress info\n");
	printf("        -fp                                    emit progress info for every file\n");
	printf("        -n1                                    truncate long Macintosh file names on export; default is to stop\n");
	printf("        -n2                                    fix illegal Macintosh file name characters on export; default is to stop\n");
	printf("        -w1                                    warn instead of overwriting file or folder on export; default is to replace files as needed\n");
	printf("        -w2                                    stop  on any error during folder export; default is to continue with next file\n");
	printf("        -m1                                    merge files into existing subcats; stop if file exists\n");
	printf("        -m2                                    merge files into existing subcats; replace files as needed\n");
	printf("        -m3                                    merge Macintosh folder contents into destination subcatalog\n");
	printf("        -v                                     verify export sizes & filenames; do not actually write to disk\n");
	printf("        -ignore                                ignore __SIZE__ file information on export\n");
	printf("        -nosize                                do not create __SIZE__ files when importing\n");
	printf("        -image                                 import entity as a disk image file\n");
	printf("        -subimages                             import nested subcatalogs as disk images\n");
	printf("        -squeeze                               squeeze disk images to minimum size on import\n");
	printf("        -create name size_bytes                create a new Disk Image File\n");
	printf("        -zero                                  zero out new disk Image file\n");
	printf("        -x percentage                          make each newly created subcatalog x %% bigger than minimum needed\n");
	printf("        -c                                     dump raw catalog contents of Synclavier SCSI drive\n");
	printf("        -r                                     recurse down Synclavier catalog structure\n");
	printf("        -text                                  import only text files\n");
	printf("        -skip.                                 skip import of files that start with '.'\n");
	printf("        -system                                only import or export Synclavier system files\n");
	printf("        -delete                                delete the Macintosh file after exporting it\n");
	printf("        -o path:name                           only access this particular Synclavier file/catalog\n");
	printf("        -q ZQX3                                quick erase hard drive (e.g. initialize) before export\n");
	printf("        -j                                     eject media after operation\n");
	printf("        -allow                                 allow erase of Macintosh hard drives\n");
	printf("        -recognize                             recognize Synclavier disks in all cases\n");
	printf("        -aiff                                  import soundfiles as AIFF files\n");
	printf("        -sd2f                                  import soundfiles as Sd2f files\n");
	printf("        -wave                                  import soundfiles as WAVE files\n");
	printf("        -rates48                               limit sample rates of AIFF, Sd2f and Wave files to 48.0 Khz\n");
	printf("        -log logfile                           create a tab-delimited log file of imported AIFF file with comments\n");
	printf("        -le                                    indicates image file source is little-endian\n");
	printf("\n");
	printf("For -e (export):\n");
	printf("  mac_entity   is the name of a file or a folder on the mac\n");
	printf("  syncl_name   if syncl_name ends in : then syncl_name is name of subcatalog to create; mac_entity will be \n");
	printf("               stored therein; else syncl_name will be name of mac_entity when transferred\n");
	printf("\n");
	printf("For -i (import):\n");
	printf("  syncl_entity is the name of a file or a subcatalog on the Synclavier disk\n");
	printf("  mac_name     if mac_name ends in : mac_name is name of folder to create or use; syncl_entity will be\n");
	printf("               stored therein; else mac_name will be name of syncl_entity on the mac\n");
}

int AbleDiskTool( int argc, const char *argv[])
{
	int				argcount = 0;		
	OSErr			FSstatus;
	SInt32			DirID;
	char			VolName[64];
	char			RootVolName[64];
	char   			folder_name            [512] = {""};
	char   			destination_entity_name[512] = {""};
	char   			import_full_mac_name   [512] = {""};
	scsi_device 	*our_device = NULL;
		
	short			VRefNum   = 0;
	FSSpec			TopImportFolderFSSpec;		
	FSSpec			temp_spec;
	
	Boolean 		targetIsFolder, wasAliased;

	
	g_disallow_run_host_exit++;								// disallow exits on break
	
	initialize_static_data();								// re-init static data on second execution
	
	AbleDiskToolNormalTermination = false;					// allow caller to detect abnormal termination
	AbleDiskToolOutputData        = NULL;					// initialize possible output data
	AbleDiskToolOutputDataSize    = 0;
	AbleDiskToolOutputDataEntries = 0;
	
    // Static variable initialization
    SyncMutex::InitAll();

	if (initialize_run_time_environment (128*1024/256))		// single m128K card for our purposes
	{
		clean_up();
		return (-1);
	}
	
	initialize_able_catalog_routines();						// init translated catalog routines
	
	read_interpreter_setup (interpreter_pref_file_name,  &InterpreterInterpreterSettings, NULL);
	read_interchange_setup (interchange_pref_file_name,  &InterchangeSettings, false);
	read_interchange2_setup(interchange2_pref_file_name, &Interchange2Settings);
	
	if ((FSstatus = HGetVol((StringPtr) VolName, &VRefNum, &DirID)) != 0)
	{
		printf("AbleDiskTool: Failed HGetVol (%d)\n", FSstatus);
		clean_up();
		return (-1);
	}
	
	p2cstr((StringPtr) VolName);

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
		
		if (strcmp(the_arg, "-pci") == 0)
		{
			bus_id = D24_SCSI_PORT;
			argcount += 1;
			continue;
		}
		
		if ((strcmp(the_arg, "-file") == 0) && (argcount+1 < argc))
		{
			use_sim_file = true;
			
			strncpy(AbleSimFileName, argv[argcount+1], sizeof(AbleSimFileName));
			
            AbleSimFileCFString = CFStringCreateWithCString(NULL, AbleSimFileName, kCFStringEncodingUTF8);

            TweakMacFileName(AbleSimFileName, VolName);

			if (SCSI_id == 99)
				SCSI_id = 5;
				
			argcount += 2;
			continue;
		}
		
		if ((strcmp(the_arg, "-ic2") == 0) && (argcount+1 < argc))
		{
			use_ic2_setup = true;

			SCSI_id = (short) atol (argv[argcount+1]);
				
			argcount += 2;
			continue;
		}
		
		if (strcmp(the_arg, "-inq") == 0)
		{
			do_inquiry = true;
			
			argcount += 1;
			continue;
		}
	
		if (strcmp(the_arg, "-t") == 0)
		{
			do_test_unit = true;
			
			argcount += 1;
			continue;
		}
	
		if ((strcmp(the_arg, "-o") == 0) && (argcount+1 < argc))
		{
			strncpy(only_name, argv[argcount+1], sizeof(only_name));
			argcount += 2;
			
			onlyaccess = true;
			
			continue;
		}
		
		if ((strcmp(the_arg, "-q") == 0) && (argcount+1 < argc) && (strcmp(argv[argcount+1], "ZQX3") == 0))
		{
			argcount += 2;
			
			quickerase = true;
			
			continue;
		}
		
		if ((strcmp(the_arg, "-q") == 0) && (argcount+1 < argc) && (strcmp(argv[argcount+1], "zqx3") == 0))
		{
			argcount += 2;
			
			quickerase = true;
			
			continue;
		}
		
		if (strcmp(the_arg, "-j") == 0)
		{
			do_eject = true;
			
			argcount += 1;
			continue;
		}
			
		if (strcmp(the_arg, "-p") == 0)
		{
			progress_desired = true;
			
			argcount += 1;
			continue;
		}
			
		if (strcmp(the_arg, "-fp") == 0)
		{
			full_progress_desired = true;
			
			argcount += 1;
			continue;
		}
			
		if (strcmp(the_arg, "-n1") == 0)
		{
			truncate_names = true;
			
			argcount += 1;
			continue;
		}
			
		if (strcmp(the_arg, "-n2") == 0)
		{
			doctor_names = true;
			
			argcount += 1;
			continue;
		}
		
		if (strcmp(the_arg, "-w1") == 0)
		{
			don_t_over_write = true;
			
			argcount += 1;
			continue;
		}

		if (strcmp(the_arg, "-w2") == 0)
		{
			stop_on_any_error = true;
			
			argcount += 1;
			continue;
		}

		if (strcmp(the_arg, "-m1") == 0)
		{
			merge_but_warn = true;
			
			argcount += 1;
			continue;
		}

		if (strcmp(the_arg, "-m2") == 0)
		{
			merge_and_replace = true;
			
			argcount += 1;
			continue;
		}

		if (strcmp(the_arg, "-m3") == 0)
		{
			merge_folder_contents = true;
			
			argcount += 1;
			continue;
		}

		if (strcmp(the_arg, "-v") == 0)
		{
			verify_only = true;
			
			argcount += 1;
			continue;
		}

		if (strcmp(the_arg, "-ignore") == 0)
		{
			ignore__size__ = true;
			
			argcount += 1;
			continue;
		}

		if (strcmp(the_arg, "-nosize") == 0)
		{
			ignore__size__ = true;
			
			argcount += 1;
			continue;
		}

		if (strcmp(the_arg, "-subimages") == 0)
		{
			import_subcats_as_images = true;
			
			argcount += 1;
			continue;
		}
		
		if (strcmp(the_arg, "-image") == 0)
		{
			import_toplevel_as_image = true;
			
			argcount += 1;
			continue;
		}

		if (strcmp(the_arg, "-squeeze") == 0)
		{
			squeeze_images_on_transfer = true;
			
			argcount += 1;
			continue;
		}

		if ((strcmp(the_arg, "-create") == 0) && (argcount+2 < argc))
		{
			create_new_disk_file = true;

			strncpy(AbleSimFileName, argv[argcount+1], sizeof(AbleSimFileName));

            AbleSimFileCFString = CFStringCreateWithCString(NULL, AbleSimFileName, kCFStringEncodingUTF8);

            AbleSimFileSize = (ulong) atol (argv[argcount+2]);
			
			TweakMacFileName(AbleSimFileName, VolName);

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

		if (strcmp(the_arg, "-allow") == 0)
		{
			allow_erase = true;
			
			argcount += 1;
			continue;
		}

		if (strcmp(the_arg, "-recognize") == 0)
		{
			recognize_disks = true;
			
			argcount += 1;
			continue;
		}

		if (strcmp(the_arg, "-system") == 0)
		{
			systemonly = true;
			
			argcount += 1;
			continue;
		}

		if (strcmp(the_arg, "-delete") == 0)
		{
			delete_after_xport = true;
			
			argcount += 1;
			continue;
		}
		
		if (strcmp(the_arg, "-aiff") == 0 || strcmp(the_arg, "-AIFF") == 0)
		{
			import_as_aiff = true;
			import_as_sd2f = false;
			import_as_wave = false;
			
			argcount += 1;
			continue;
		}

		if (strcmp(the_arg, "-rewrite") == 0)
		{
			do_rewrite = true;
			
			argcount += 1;
			continue;
		}



		if (strcmp(the_arg, "-sd2f") == 0 || strcmp(the_arg, "-SD2F") == 0 || strcmp(the_arg, "-Sd2f") == 0)
		{
			import_as_aiff = false;
			import_as_sd2f = true;
			import_as_wave = false;
			
			argcount += 1;
			continue;
		}

		if (strcmp(the_arg, "-wave") == 0 || strcmp(the_arg, "-WAVE") == 0)
		{
			import_as_aiff = false;
			import_as_sd2f = false;
			import_as_wave = true;
			
			argcount += 1;
			continue;
		}

		if (strcmp(the_arg, "-rates48") == 0)
		{
			coerce_rates = true;
			
			argcount += 1;
			continue;
		}

		if ((strcmp(the_arg, "-x") == 0) && (argcount+1 < argc))
		{
			create_extra_space = (uint32) atol (argv[argcount+1]);
			argcount += 2;
			continue;
		}

		if (strcmp(the_arg, "-import and recursivley scan") == 0)
		{
			import_scan = true;
			
			argcount += 1;
			continue;
		}
		
		if ((strcmp(the_arg, "import") == 0) && (argcount+2 < argc))
		{
			import_file = true;
			
			// file "FileName"                        specify a Macintosh-resident Synclavier Disk Image File - use quotes if needed
			// scsi 6                                 specify legacy SCSI drive connected via Ratoc FR1SX FireWire-SCSI Converter
			// W0                                     specify source is current InterChange? setting for W0:
			// W1                                     specify source is current InterChange? setting for W1:

			if ((strcmp(argv[argcount+1], "file") == 0) && (argcount+3 < argc))
			{
				use_sim_file = true;
				
				strncpy(AbleSimFileName, argv[argcount+2], sizeof(AbleSimFileName));
                
                AbleSimFileCFString = CFStringCreateWithCString(NULL, AbleSimFileName, kCFStringEncodingUTF8);
				
				TweakMacFileName(AbleSimFileName, VolName);

				if (SCSI_id == 99)
					SCSI_id = 5;
					
				argcount += 3;
			}
			
			else if ((strcmp(argv[argcount+1], "scsi") == 0) && (argcount+3 < argc))
			{
				SCSI_id = (short) atol (argv[argcount+2]);

				argcount += 3;
			}
			
			else if (((strcmp(argv[argcount+1], "W0") == 0))
			||		 ((strcmp(argv[argcount+1], "w0") == 0)))
			{
				use_ic2_setup = true;
				SCSI_id       = 5;

				argcount += 2;
			}
			
			else if (((strcmp(argv[argcount+1], "W1") == 0))
			||		 ((strcmp(argv[argcount+1], "w1") == 0)))
			{
				use_ic2_setup = true;
				SCSI_id       = 4;

				argcount += 2;
			}
			
			else
			{
				print_help();
				clean_up();
				return (-1);
			}
			
			strncpy(import_able_entity_name, "W0:", sizeof(import_able_entity_name));
			strncpy(import_mac_name,         argv[argcount], sizeof(import_mac_name        ));
			argcount += 1;
			
			ignore__size__ = true;
			
			if (!import_as_sd2f && !import_as_wave)
				import_as_aiff = true;
					
			TweakMacFileName(import_mac_name, VolName);

			continue;
		}

		if (((strcmp(the_arg, "-i") == 0) && (argcount+2 < argc))
		||	((strcmp(the_arg, "i" ) == 0) && (argcount+2 < argc)))
		{
			import_file = true;
			
			strncpy(import_able_entity_name, argv[argcount+1], sizeof(import_able_entity_name));
			strncpy(import_mac_name,         argv[argcount+2], sizeof(import_mac_name        ));
			argcount += 3;
			
			if (SCSI_id == 99)
				SCSI_id = 5;
				
			TweakMacFileName(import_mac_name, VolName);
						
			if (import_able_entity_name[0] == 0)
			{
				printf("Must specify able_entity to import\n");
				clean_up();
				return (-1);
			}
			
			continue;
		}

		if ((AbleDiskToolAllowPointers && strcmp(the_arg, "-read_directory") == 0) && (argcount+1 < argc))
		{
			read_directory = true;
			
			strncpy(import_able_entity_name, argv[argcount+1], sizeof(import_able_entity_name));
			import_mac_name[0] = 0;

			argcount += 2;
			
			if (import_able_entity_name[0] == 0)
			{
				printf("Must specify able_entity to import\n");
				clean_up();
				return (-1);
			}
			
			continue;
		}

		if (((strcmp(the_arg, "-e") == 0) && (argcount+2 < argc))
		||	((strcmp(the_arg, "e" ) == 0) && (argcount+2 < argc)))
		{
			export_file = true;
			
			strncpy(export_mac_entity_name, argv[argcount+1], sizeof(export_mac_entity_name));
			strncpy(export_able_name,       argv[argcount+2], sizeof(export_able_name      ));
			argcount += 3;
			
			if (SCSI_id == 99)
				SCSI_id = 5;
				
			TweakMacFileName(export_mac_entity_name, VolName);
			
			if (export_mac_entity_name[0] == 0)
			{
				printf("Must specify mac_entity to export\n");
				clean_up();
				return (-1);
			}
			
			continue;
		}
		
		if (strcmp(the_arg, "-c") == 0)
		{
			catalog = true;
			
			argcount += 1;
			continue;
		}
		
		if (strcmp(the_arg, "-d") == 0)
		{
			dumpcontents = true;
			
			argcount += 1;
			continue;
		}
		
		if (strcmp(the_arg, "-r") == 0)
		{
			recurs = true;
			
			argcount += 1;
			continue;
		}
		
		if (strcmp(the_arg, "-text") == 0)
		{
			textonly = true;
			
			argcount += 1;
			continue;
		}
		
		if (strcmp(the_arg, "-skip.") == 0)
		{
			skipdots = true;
			
			argcount += 1;
			continue;
		}
		
		if ((strcmp(the_arg, "-log") == 0) && (argcount+1 < argc))
		{
			strncpy(log_file_name, argv[argcount+1], sizeof(log_file_name));
			
			TweakMacFileName(log_file_name, VolName);

			argcount += 2;
			continue;
		}
		
		if (strcmp(the_arg, "-le") == 0)
		{
			little_endian = true;
			
			argcount += 1;
			continue;
		}
        
		printf("Unrecognized command line argument '%s'\n", the_arg);
		print_help();
		clean_up();
		return (-1);
	}

	// Check activation
	//if (Interchange2Settings.registered_key == 0 || (Interchange2Settings.registered_bits & REG_CODE_CONSTANT) == 0)
	//{
	//	printf("AbleDiskTool: requires a valid InterChange™ License Code. Purchase one at www.synclavier.com.\n");
	//	clean_up();
	//	return (-1);
	//}

	// Create sim file if needed

	if (create_new_disk_file)
	{
		if (CreateAndOpenAbleSimFile(VRefNum))
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
		our_device->fVRefNum		 = VRefNum;
		our_device->fFRefNum		 = AbleSimFileRefNum;
		our_device->fFSSpec			 = AbleSimFSSpec;
	}
	
	// Use sim file	if specified
	else if (use_sim_file)
	{
		if (OpenAbleSimFile(VRefNum))
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
		our_device->fVRefNum		 = VRefNum;
		our_device->fFRefNum		 = AbleSimFileRefNum;
		our_device->fFSSpec			 = AbleSimFSSpec;
        our_device->fAllowGrow       = true;
	}
	
	// Check for bad ID
	else if (SCSI_id >= 7)
	{
		printf("AbleDiskTool: Bad SCSI Id specified (%d)\n", SCSI_id);
		clean_up();
		return (-1);
	}
	
	// Set up for d24 scsi port hardware access
	else if (bus_id == D24_SCSI_PORT)
	{
		int status = NOT_SUCCESSFUL;
		
		our_device = &g_scsi_device_data_base[SCSI_id];
		
		zero_mem((byte *) our_device, sizeof(*our_device));

		our_device->fDevicePort      = D24_SCSI_PORT;
		our_device->fIdentifyMessage = IDENTIFY_NO_DISC;
		our_device->fTargetId        = SCSI_id;
	
		if (((status  = SynclavierPCILib_InitializePCI1("InterChange1", kPCI1ServiceGeneric, InterpreterInterpreterSettings.cable_setting, InterpreterInterpreterSettings.bus_setting)) == noErr)
		&&  ((gAbleAccessor = SynclavierPCILib_FetchPCIAccessor( PCI1_ACCESSOR_STRUCT_VERSION )) != NULL)
		&&  (SynclavierPCILib_FetchDevReadCode( 024 ) != 0))
			D24Sim_Initialize(gAbleAccessor, NULL, false);
		
		if (status == DEVICE_IN_USE)
		{
			printf("AbleDiskTool: The PCI-1 hardware is being used by another application at this time.\n");
			printf("              You must quit that application before you can access this disk with AbleDiskTool.\n");
			clean_up();
			return (-1);
		}
		
		if (status != SUCCESSFUL)
		{
			printf("AbleDiskTool: The PCI-1 hardware needed to support the Synclavier SCSI Bus is not available at this time.\n");
			printf("              Check to make sure the PCI-1 interface card is installed and that the Synclavier is powered up.\n");
			clean_up();
			return (-1);
		}
	}
	
	// Use IC2 Setup
	else if (use_ic2_setup)
	{
		int status   = NOT_SUCCESSFUL;
		int d24Avail = false;
		
		// Access PCI card if needed
		if ((InterchangeSettings.w0.bus_id == SCSI_BUS_MENU_D24)
		||  (InterchangeSettings.w1.bus_id == SCSI_BUS_MENU_D24)
		||  (InterchangeSettings.o0.bus_id == SCSI_BUS_MENU_D24)
		||  (InterchangeSettings.o1.bus_id == SCSI_BUS_MENU_D24))
		{
			if (((status  = SynclavierPCILib_InitializePCI1("InterChange1", kPCI1ServiceGeneric, InterpreterInterpreterSettings.cable_setting, InterpreterInterpreterSettings.bus_setting)) == noErr)
			&&  ((gAbleAccessor = SynclavierPCILib_FetchPCIAccessor( PCI1_ACCESSOR_STRUCT_VERSION )) != NULL)
			&&  (SynclavierPCILib_FetchDevReadCode( 024 ) != 0))
			{
				D24Sim_Initialize(gAbleAccessor, NULL, false);
				d24Avail = true;
			}
			
			if (status == DEVICE_IN_USE)
			{
				printf("AbleDiskTool: The PCI-1 hardware is being used by another application at this time.\n");
				printf("              You must quit that application before you can access this disk with AbleDiskTool.\n");
				clean_up();
				return (-1);
			}
			
			if (status != SUCCESSFUL)
			{
				printf("AbleDiskTool: The PCI-1 hardware needed to support the Synclavier SCSI Bus is not available at this time.\n");
				printf("              Check to make sure the PCI-1 interface card is installed and that the Synclavier is powered up.\n");
				clean_up();
				return (-1);
			}
		}
		
		XPLRunTime_SetupSCSIMap(&InterchangeSettings, d24Avail);
		XPLRunTime_ConfigureSCSIMap(false, false);
		our_device = &g_scsi_device_data_base[SCSI_id];
		
		XPLRunTime_ExamineDevice(0, SCSI_id);
        
        map_set = true;
	}
	
	// Else set up for mac scsi port hardware access
	else
	{
		our_device = &g_scsi_device_data_base[SCSI_id];
		
		zero_mem((byte *)our_device, sizeof(*our_device));

		our_device->fDevicePort      = MAC_SCSI_PORT;
		our_device->fIdentifyMessage = IDENTIFY_NO_DISC;
		our_device->fTargetId        = SCSI_id;
	}
	
	if (!our_device)
	{
		clean_up();
		return(-1);
	}
	
	
	// Now process individual commands
	
	// Inquiry
	if (do_inquiry)
	{
		scsi_error_code	status;
		
		g_scsi_print_basic_opt = progress_desired | full_progress_desired;
		status = issue_inquiry(our_device);
		g_scsi_print_basic_opt = FALSE;
		
		if (status)
		{
			if (status == NO_RESPONSE)
				printf("\nSCSI: No Inquiry response received; device is not available\n");
			else
				printf("\nSCSI: SCSI Inquiry failed for SCSI Id %d\n", SCSI_id);
			
			clean_up();
			return (-1);
		}
	}
		
	if (do_test_unit)
	{
		scsi_error_code	status;
		
		g_scsi_print_basic_opt = TRUE;
		g_scsi_print_all_opt   = TRUE;
		
		status = issue_test_ready(our_device);
		
		g_scsi_print_basic_opt = FALSE;
		g_scsi_print_all_opt   = FALSE;
	
		printf("AbleDiskTool: Test Unit Ready status %d\n", status);
	}
	
	// Set up device if using it for any data access
	if (do_inquiry    || do_test_unit   || import_scan
	|| import_file    || export_file    || catalog
	|| dumpcontents   || quickerase     || do_eject
	|| read_directory)
	{
		if (our_device->fTargetId < MAX_NUM_DEVICES)
			g_indexed_device[our_device->fTargetId] = our_device;

		g_recognize_disks = recognize_disks;
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
		
		if (our_device->fDeviceType == DEVICE_CD_ROM_OF_SOME_KIND)
		{
			if (do_inquiry || do_test_unit)
				printf("\nSCSI: SCSI %d is a CD-ROM drive of some kind\n", SCSI_id);
			
			if (import_scan
			||  import_file  || export_file  || catalog
			||  dumpcontents || quickerase)
			{
				clean_up();
				return (-1);
			}
		}
		
		if ((quickerase           )
		&&  ((our_device->fDeviceType  == DEVICE_UNINITIALIZED_DISK)
		||   (our_device->fDeviceType  == DEVICE_AUDIO_DISK        )
		||   (our_device->fDeviceType  == DEVICE_MACINTOSH_DISK    )
		||   (our_device->fDeviceType  == DEVICE_ABLE_DISK         )
		||   (our_device->fDeviceType  == DEVICE_BLANK_ABLE_OPTICAL)
		||   (our_device->fDeviceType  == DEVICE_ABLE_OPTICAL      )))
		{
			if ((our_device->fDeviceType != DEVICE_ABLE_DISK         )	// if not an able disk, check...
			&&  (our_device->fDeviceType != DEVICE_BLANK_ABLE_OPTICAL)
			&&  (our_device->fDeviceType != DEVICE_ABLE_OPTICAL      )
			&&  (!allow_erase))
			{
				printf("\nAbleDiskTool: Warning: The disk you are trying to erase is not a Synclavier hard drive.\n");
				printf("   Check the 'Allow Erase of Macintosh Disks' option under the AbleDiskTool Menu if you\n");
				printf("   wish to erase this hard drive.\n");
				clean_up();
				return (-1);
			}
			
			our_device->fDeviceType = DEVICE_ABLE_DISK;
			
			if (!export_file)											// erase disk here if not exporting.  If
			{															// we are exporting, we erase after we
				if (WriteAbleDisk(SCSI_id, zero_buf, 0, 4))				// have performed all error checking
				{
					clean_up();
					return (-1);
				}

				if (progress_desired || full_progress_desired)
				{
					if (our_device->fFRefNum)				
						printf("AbleDiskTool: Disk Image File erased...\n");
					else
						printf("AbleDiskTool: SCSI Id %d erased...\n", our_device->fTargetId);
				}
			}
		}
		

		// Set up run time environment to access this able disk
		// Set up device as both W0 and W1 for our purposes since user is only accessing
		// that one device.  E.G. either W0: or W1: will access this device.

		if (!use_ic2_setup)
			configure_able_hard_drives (our_device->fTargetId, our_device->fTotCyl, our_device->fTotSec,
										our_device->fTargetId, our_device->fTotCyl, our_device->fTotSec);
									
		if ((our_device->fDeviceType != DEVICE_ABLE_DISK         )	// if not an able disk, check...
		&&  (our_device->fDeviceType != DEVICE_BLANK_ABLE_OPTICAL)
		&&  (our_device->fDeviceType != DEVICE_ABLE_OPTICAL      ))
		{
			if (do_inquiry && !export_file && !import_scan && !import_file && !catalog && !dumpcontents && !read_directory)
			{
				if (our_device->fDeviceType == DEVICE_MACINTOSH_DISK)
				{
					printf("      SCSI %d is a Macintosh disk\n", SCSI_id);
					clean_up();
					return (0);
				}
									
				else if (our_device->fDeviceType == DEVICE_UNINITIALIZED_DISK)
				{
					printf("      SCSI %d is a blank or unknown format disk\n", SCSI_id);
					clean_up();
					return (0);
				}
					
				else
				{
					printf("      Warning: SCSI %d is not a Synclavier disk\n", SCSI_id);
					clean_up();
					return (0);
				}
			}
			else
				printf("AbleDiskTool: Warning: SCSI %d is not a Synclavier disk\n", SCSI_id);
			
			if (export_file)
			{
				printf("AbleDiskTool: Can't export to non-Synclavier disk\n");
				printf("\n");
				printf("              If you are sure that this disk is a Synclavier disk\n");
				printf("              you may check the 'Recognize Synclavier Disks\n");
				printf("              in All Cases' option under the AbleDiskTool menu\n");
				printf("              and try again.\n");
				clean_up();
				return (-1);
			}
			
			else if (import_file)
			{
				printf("AbleDiskTool: Can't import from non-Synclavier disk\n");
				printf("\n");
				printf("              If you are sure that this disk is a Synclavier disk\n");
				printf("              you may check the 'Recognize Synclavier Disks\n");
				printf("              in All Cases' option under the AbleDiskTool menu\n");
				printf("              and try again.\n");
				clean_up();
				return (-1);
			}
			
			else if (read_directory)
			{
				printf("AbleDiskTool: Can't read directory from non-Synclavier disk\n");
				printf("\n");
				printf("              If you are sure that this disk is a Synclavier disk\n");
				printf("              you may check the 'Recognize Synclavier Disks\n");
				printf("              in All Cases' option under the AbleDiskTool menu\n");
				printf("              and try again.\n");
				clean_up();
				return (-1);
			}
			
			
			else if (do_inquiry || do_test_unit)
			{
				clean_up();
				return (-1);
			}
		}
		
		else if (our_device->fDeviceType == DEVICE_BLANK_ABLE_OPTICAL)
		{
			if (export_file)
			{
				printf("AbleDiskTool: This version of InterChange can't export to Optical media\n");
				clean_up();
				return (-1);
			}
			
			else if (import_file)
			{
				printf("AbleDiskTool: The optical media is blank; there are no files to import\n");
				clean_up();
				return (-1);
			}
			
			else if (read_directory)
			{
				printf("AbleDiskTool: The optical media is blank; there are no files on this media\n");
				clean_up();
				return (-1);
			}
			
			else if (do_inquiry || do_test_unit)
			{
				if (our_device->fFRefNum)				
					printf("AbleDiskTool: The Disk Image File appears to be a blank Synclavier optical media\n");
				else
					printf("AbleDiskTool: SCSI ID %d appears to be a blank Synclavier optical disk\n", SCSI_id);
			}
		}
		else if (our_device->fDeviceType == DEVICE_ABLE_OPTICAL)
		{
			if (export_file)
			{
				printf("AbleDiskTool: This version of InterChange can't export to Optical media\n");
				clean_up();
				return (-1);
			}
			
			else if (import_file)
			{
				printf("AbleDiskTool: This version of InterChange can't import from Optical media\n");
				clean_up();
				return (-1);
			}
			
			else if (read_directory)
			{
				printf("AbleDiskTool: This version of InterChange can't read the directory of Optical media\n");
				clean_up();
				return (-1);
			}
			
			else if (do_inquiry || do_test_unit)
			{
				if (our_device->fFRefNum)				
					printf("AbleDiskTool: The Disk Image File appears to be a Synclavier optical media\n");
				else
					printf("AbleDiskTool: SCSI ID %d appears to be a Synclavier optical disk\n", SCSI_id);
			}
		}
		
		// Else is an Able disk
		else if (do_inquiry || do_test_unit)
		{
			if (our_device->fFRefNum)				
				printf("AbleDiskTool: The Disk Image File appears to be a Synclavier disk image that is ready for access\n");
			else
				printf("AbleDiskTool: SCSI ID %d appears to be a Synclavier disk that is ready for access\n", SCSI_id);
		}
	}
	

	// Handle import of file or directory
	
	if (import_file || read_directory)
	{
		uint32 block_num = 0;
		uint32 block_len = 0;
		uint32 word_len  = 0;
		int	   i;
		SInt32 sub_dir_id = 0;
		char   tweaked_name[16];
				

		// Find what the user wants to import.
				
		// Check for import of W0: or W1:
		if ((strcmp(import_able_entity_name, "W0:") == 0)		// Import of w0: or w1:
		||  (strcmp(import_able_entity_name, "w0:") == 0)		// set up for scan of whole disk
		||  (strcmp(import_able_entity_name, "W1:") == 0)
		||  (strcmp(import_able_entity_name, "w1:") == 0))
		{
			if ((strcmp(import_able_entity_name, "W0:") == 0)
			||  (strcmp(import_able_entity_name, "w0:") == 0))
			{
				get_device_size(ABLE_W0_READDATA_CODE);
				to_able_string("W0", f_name);
				strcpy(import_able_entity_name, "W0");
			}

			else
			{
				get_device_size(ABLE_W1_READDATA_CODE);
				to_able_string("W1", f_name);
				strcpy(import_able_entity_name, "W1");
			}
			
			block_num = 0;
			block_len = (((uint32) (uint16) (c_ms_length & 0xFF)) << 16) | ((uint32) (uint16) c_ls_length); // block len
			
			f_type = t_lsubc;
		}

		// Locate the entity on the able				
		else if (!locate(import_able_entity_name, 0, true))
		{
			get_cat_code_message(c_status, er_mess);
			printf("AbleDiskTool: Could not find file or directory '%s' for import\nhere's why:\n", import_able_entity_name);
			printf("   %s\n", er_mess);
			clean_up();
			return (-1);
		}

		// If we could locate it, look further		
		else
		{
			if (0)
			{
                print("name:%1p   f_ms_sector %d   f_ls_sector %d  f_ms_length %d \n   f_ls_length %d  f_words %d  f_type %d\n",
				f_name, f_ms_sector, f_ls_sector, f_ms_length, f_ls_length, f_words, f_type);
			}

			block_num = (((uint32) (uint16) (f_ms_sector & 0xFF)) << 16) | ((uint32) (uint16) f_ls_sector); // block num on W0:
			block_len = (((uint32) (uint16) (f_ms_length & 0xFF)) << 16) | ((uint32) (uint16) f_ls_length); // block len
			word_len  = block_len << 8;																		// word len of full sectors

			if ((f_words & 255) != 0)
				word_len -= (256 - (f_words & 255));
			
			if (CheckAbleFileName((ufixed *) f_name))								// get correct type of file for old non-typed special files
				f_type = CheckAbleFileName((ufixed *) f_name);
		}
		

		// Parse the mac name to see if item is being renamed.  Check for volume names
		// and create the import folder as needed.  Compute what the name of the
		// able entity will be on the mac

		to_c_string(f_name, tweaked_name);								// get able name in C string form

		if (tweaked_name[0] == '.')										// map leading . to # to avoid
			tweaked_name[0] = '*';										// mac os conflict with devices
		
		i = strlen(import_mac_name);									// length of destination name on mac
		
		if (i == 0)														// if no dest name specified, use top level dir
		{
			TopImportFolderFSSpec.vRefNum = VRefNum;					// use default ref num
			TopImportFolderFSSpec.parID   = 0;							// at the top level
			strcpy(destination_entity_name, tweaked_name);				// preserve able name
			strcpy(import_full_mac_name, VolName);
			strcat(import_full_mac_name, ":");
			strcat(import_full_mac_name, tweaked_name);
		}
		
		else if (import_mac_name[i-1] == ':')							// if ends in :, it is name of mac volume or folder
		{
			if (i == 1)													// name of :
			{
				TopImportFolderFSSpec.vRefNum = VRefNum;				// use default volume
				TopImportFolderFSSpec.parID   = 0;						// at the top level
				strcpy(destination_entity_name, tweaked_name);			// preserve able name
				strcpy(import_full_mac_name, VolName);
				strcat(import_full_mac_name, ":");
				strcat(import_full_mac_name, tweaked_name);
			}
	
			else														// dest name ends in : see if is a volume
			{
				if (IsVolumeName(import_mac_name, &TopImportFolderFSSpec.vRefNum))
				{
					TopImportFolderFSSpec.parID = 0;					// if is volume, use root level of that volume
					strcpy(destination_entity_name, tweaked_name);		// and preserve able name
					strcpy(import_full_mac_name, import_mac_name);
					strcat(import_full_mac_name, tweaked_name);
				}

				else															// ends in colon but is not the name of a volume
				{																// so use or create the import folder
					strncpy(folder_name, import_mac_name, sizeof(folder_name));	// get mac folder name
					folder_name[i-1] = 0;										// remove colon; that's our mac folder name
					
					strcpy(destination_entity_name, tweaked_name);				// and preserve able name
					
					if (import_mac_name[0] == ':')								// starts with colon, prepend vol name
					{
						strcpy(import_full_mac_name, VolName);
						strcat(import_full_mac_name, import_mac_name);
					}
					else
						strcpy(import_full_mac_name, import_mac_name);
					
			 		if (CreateTopImportFolder(VRefNum, 0, folder_name, &TopImportFolderFSSpec, block_len, import_full_mac_name))
					{
						clean_up();
						return (-1);
					}
					
					strcat(import_full_mac_name, tweaked_name);
				}
			}
		}
		
		else															// else if no colon at end, user is renaming the entity upon import
		{
			if (strrchr(import_mac_name, ':') == 0)						// if no colons at all
			{
				TopImportFolderFSSpec.vRefNum = VRefNum;				// then import to top level with new name
				TopImportFolderFSSpec.parID   = 0;
				strncpy(destination_entity_name, import_mac_name, sizeof(destination_entity_name));	// that's what to call it
				strcpy(import_full_mac_name, VolName);
				strcat(import_full_mac_name, ":");
				strcat(import_full_mac_name, import_mac_name);
			}											
			
			else														// name has some colons, but not at end
			{
				while (import_mac_name[i-1] != ':')						// find last :
					i--;
					
				strncpy(destination_entity_name, &import_mac_name[i], sizeof(destination_entity_name));	// extract new name
				strncpy(folder_name,              import_mac_name,    sizeof(folder_name));				// and mac volume or folder name
				
				folder_name[i] = 0;										// remove new name leaving colon
		 		
				if (IsVolumeName(folder_name, &TopImportFolderFSSpec.vRefNum))
				{
					TopImportFolderFSSpec.parID = fsRtDirID;			// if is volume, use it
					strcpy(import_full_mac_name, folder_name);
					strcat(import_full_mac_name, destination_entity_name);
				}
		 		
		 		else													// else is not volume
		 		{
					if (folder_name[0] == ':')							// starts with colon, prepend vol name
					{
						strcpy(import_full_mac_name, VolName);
						strcat(import_full_mac_name, folder_name);
					}
					else
						strcpy(import_full_mac_name, folder_name);
					
					folder_name[i-1] = 0;								// remove trailing colong

			 		if (CreateTopImportFolder(VRefNum, 0, folder_name, &TopImportFolderFSSpec, block_len, import_full_mac_name))
					{
						clean_up();
						return (-1);
					}
					
					strcat(import_full_mac_name, destination_entity_name);
				}
			}
		}
		
		ImportVolRefNum = TopImportFolderFSSpec.vRefNum;		// publish vref num for flush
		
		// Prepare to create a log file
		if (log_file_name[0])
			CreateAndOpenLogFile(VRefNum);

		// Read a directory
		
		if (read_directory)
		{
			int dir_block_len;

			if (f_type == t_subc)
				dir_block_len = 1;
			
			else if (f_type == t_lsubc)
				dir_block_len = 4;
								
			else
				{printf("AbleDiskTool: Can't read directory for %s; it is not a subcatalog\n", import_able_entity_name); clean_up(); return (-1);}
		
			if (FetchAndRecurseAbleCatalog(import_able_entity_name, SCSI_id, block_num, dir_block_len, 0,
			    0, false, import_full_mac_name))
			{
				clean_up();
				return (-1);
			}
		}

		// Import a subcatalog:
		
		else if (f_type == t_subc)								// import of subcatalog
		{
			if (import_toplevel_as_image)						// disk image desired
			{
				progress_desired |= full_progress_desired;		// show progress for this file
				
				if (ImportAbleSubcatalog(SCSI_id, block_num, block_len << 8, 'SUBC',
								        destination_entity_name, TopImportFolderFSSpec.vRefNum, TopImportFolderFSSpec.parID,
								        import_full_mac_name, t_subc, import_able_entity_name))
				{
					clean_up();
					return (-1);
				}
			}
			
			else
			{
				c2pstr(destination_entity_name);				// get new name of subcatalog in pascal form

				if ((FSstatus = DirCreate(TopImportFolderFSSpec.vRefNum, TopImportFolderFSSpec.parID, (uint8 *) destination_entity_name, &sub_dir_id)) != 0)
				{
					p2cstr((uint8 *) destination_entity_name);	// get back in C format
					
					if (FSstatus == dupFNErr)
						{printf("AbleDiskTool: Can't import %s; it already exists\n", import_full_mac_name); clean_up(); return (-1);}
					else
						{printf("AbleDiskTool: Failed DirCreate (%d)\n", FSstatus); clean_up(); return (-1);}
				}
				
				if (!ignore__size__)
					Create__size__File(1, TopImportFolderFSSpec.vRefNum, sub_dir_id, block_len);

				p2cstr((uint8 *) destination_entity_name);		// get back in C format
				
				if (progress_desired || full_progress_desired)
					printf("AbleDiskTool: Created folder \"%s\"\n", import_full_mac_name);
				
				recurs = true;									// implies recursion
				
				if (FetchAndRecurseAbleCatalog(import_able_entity_name, SCSI_id, block_num, 1, TopImportFolderFSSpec.vRefNum,
				    sub_dir_id, false, import_full_mac_name))
				{
					clean_up();
					return (-1);
				}
			}
		}
		
		
		// Import a large subcatalog:

		else if (f_type == t_lsubc)								// import of large subcatalog
		{
			if (import_toplevel_as_image)						// disk image desired
			{
				progress_desired |= full_progress_desired;		// show progress for this file

				if (ImportAbleSubcatalog(SCSI_id, block_num, block_len << 8, 'SUBC',
								        destination_entity_name, TopImportFolderFSSpec.vRefNum, TopImportFolderFSSpec.parID,
								        import_full_mac_name, t_lsubc, import_able_entity_name))
				{
					clean_up();
					return (-1);
				}
			}
			
			else
			{
				c2pstr(destination_entity_name);				// get new name of subcatalog in pascal form

				if ((FSstatus = DirCreate(TopImportFolderFSSpec.vRefNum, TopImportFolderFSSpec.parID, (uint8 *) destination_entity_name, &sub_dir_id)) != 0)
				{
					p2cstr((uint8 *) destination_entity_name);	// get back in C format
					
					if (FSstatus == dupFNErr)
						{printf("AbleDiskTool: Can't import %s; it already exists\n", import_full_mac_name); clean_up(); return (-1);}
					else
						{printf("AbleDiskTool: Failed DirCreate (%d)\n", FSstatus); clean_up(); return (-1);}
				}
				
				p2cstr((uint8 *) destination_entity_name);		// get back in C format
				
				if (!ignore__size__)				
					Create__size__File(4, TopImportFolderFSSpec.vRefNum, sub_dir_id, block_len);

				if (progress_desired || full_progress_desired)
					printf("AbleDiskTool: Created folder \"%s\"\n", import_full_mac_name);
				
				recurs = true;									// implies recursion
				
				if (FetchAndRecurseAbleCatalog(import_able_entity_name, SCSI_id, block_num, 4, TopImportFolderFSSpec.vRefNum,
				    sub_dir_id, false, import_full_mac_name))
				{
					clean_up();
					return (-1);
				}
			}
		}


		// Import a text file		

		else if (f_type == t_text)						// import text file
		{
			progress_desired |= full_progress_desired;	// show progress for this file
			
			if (ImportAbleTextFile(SCSI_id, block_num, word_len,
							       destination_entity_name, TopImportFolderFSSpec.vRefNum,
							       TopImportFolderFSSpec.parID, import_full_mac_name))
			{
				clean_up();
				return (-1);
			}
		}
			
		// Import a sound file
		else if (f_type == t_sound)						// import sound file
		{
			progress_desired |= full_progress_desired;	// show progress for this file
			
			if (ImportAbleSoundFile(SCSI_id, block_num, word_len,
							        destination_entity_name, TopImportFolderFSSpec.vRefNum, TopImportFolderFSSpec.parID, import_full_mac_name))
			{
				clean_up();
				return (-1);
			}
		}
			
		// Import a non-text file
		
		else											// import of data file
		{
			OSType	mac_type;
			
			if (f_type == t_exec)
				mac_type = 'EXEC';
				
			else if (f_type == t_reloc)
				mac_type = 'RLOC';

			else if (f_type == t_data)
				mac_type = 'DATA';

			else if (f_type == t_sync)
				mac_type = 'SQNC';

			else if (f_type == t_dump)
				mac_type = 'DUMP';

			else if (f_type == t_spect)
				mac_type = 'SPEC';

			else if (f_type == t_index)
				mac_type = 'INDX';

			else if (f_type == t_timbre)
				mac_type = 'TIMB';

			else
				mac_type = 'DATA';
			
			progress_desired |= full_progress_desired;		// show progress for this file
			
			if (ImportAbleDataFile(SCSI_id, block_num, word_len, mac_type,
							       destination_entity_name, TopImportFolderFSSpec.vRefNum, TopImportFolderFSSpec.parID, import_full_mac_name))
			{
				clean_up();
				return (-1);
			}
		}
	}

	
	// Handle export of file or folder
	
	else if (export_file)
	{
		uint32 		block_num;
		uint32 		block_len;
		uint32 		word_len;
		uint32 		sec_len;
		uint32		cat_style;
		fixed		cat_type;
				
		fixed  		ms_len, ls_len, words;
		
		CInfoPBRec  info_rec;
		int			i;
		char 		temp_name[512]     = {""};
		boolean		exporting_to_drive = false;
		
		
		// Make an FSSpec for the entity being exported so we can get its
		// raw name. Also so we can find out what it is (e.g. folder or file)
		
		zero_mem((byte *) &info_rec,  sizeof(info_rec ));
		zero_mem((byte *) &temp_name, sizeof(temp_name));

		strncpy(temp_name, export_mac_entity_name, sizeof(temp_name));
		c2pstr(temp_name);
		
        int xyz = temp_name[0];
        
		if ((FSstatus = FSMakeFSSpec (VRefNum, 0, (uint8 *) temp_name,  &temp_spec)) != 0)
		{
            // Try resolving alias
            FSRef ref;
            Boolean isDirectory;
            
            strcpy(temp_name, "/Volumes/");
            strcat(temp_name, export_mac_entity_name);
            
            for (int i=0; temp_name[i] != 0; i++)
                if (temp_name[i] == ':')
                    temp_name[i] = '/';
            
            FSstatus = FSPathMakeRef ((UInt8 *) temp_name, &ref, &isDirectory);
            
            if (FSstatus == noErr)
                FSstatus = FSGetCatalogInfo (&ref, kFSCatInfoNone, NULL, NULL, &temp_spec, NULL);
            
            if (FSstatus != noErr) {
                printf("AbleDiskTool: Could not get specific information on '%s'\n", export_mac_entity_name);
                clean_up();
                return (-1);
            }
		}
		
		// Get information about the entity being exported
		
		info_rec.hFileInfo.ioCompletion = NULL;		
		info_rec.hFileInfo.ioNamePtr    = temp_spec.name;		
		info_rec.hFileInfo.ioVRefNum    = temp_spec.vRefNum;		
		info_rec.hFileInfo.ioFDirIndex  = 0;		
		info_rec.hFileInfo.ioDirID      = temp_spec.parID;		

		if ((FSstatus = PBGetCatInfoSync(&info_rec)) != 0)
		{
			printf("AbleDiskTool: Could not get information on '%s'\n", export_mac_entity_name);
			clean_up();
			return (-1);
		}


		// Parse the specified names to see if item is being renamed.

		i = strlen(export_able_name);									// length of destination name on able
		
		if (i == 0)														// if no dest name specified, use top level dir
		{
			if (merge_folder_contents || systemonly)					// if merging contents or exporting system files but no name specified
			{															// merge into W0: (e.g. export folder contents to top level)
				strncpy(destination_entity_name, "W0:", sizeof(destination_entity_name));
				exporting_to_drive = true;
			}
			
			else														// else use entity name
			{
				p2cstr(temp_spec.name);									// get C equivalent of entity name
				strncpy(destination_entity_name, (char *) temp_spec.name, sizeof(destination_entity_name));	// that's what it will be called at the top level of w0:
				c2pstr((char *) temp_spec.name);

				if (destination_entity_name[0] == (char)165)            // map leading • back to . for able's use
					destination_entity_name[0] = '.';
							
				if (destination_entity_name[0] == '*')					// map leading * back to . for able's use
					destination_entity_name[0] = '.';
				
				WipeNameExtension(destination_entity_name);		
							 
				if (truncate_names)										// limit to 8 chars if should do so
					destination_entity_name[8] = 0;

				if (doctor_names)										// fix name if should do so
					for (i=0; i<strlen(destination_entity_name); i++)
						if (!valid_filechar(destination_entity_name[i]))
							destination_entity_name[i] = '_';
			}
		}
		
		else if (export_able_name[i-1] == ':')							// if ends in :, it is name of an able device or folder
		{
			// Substitute "W0:" for ":"
			if (strcmp(export_able_name, ":") == 0)
				strncpy(destination_entity_name, "W0:", sizeof(destination_entity_name));
			else
				strncpy(destination_entity_name, export_able_name, sizeof(destination_entity_name));

			// Check for special case of export to w0: while erasing disk.  Means dump
			// entire subcatalog as hard drive
			
			if ((((info_rec.hFileInfo.ioFlAttrib & 0x10) != 0)			// if exporting a folder or a disk image or a folder alias
			||   (info_rec.hFileInfo.ioFlFndrInfo.fdType == 'SUBC' && info_rec.hFileInfo.ioFlFndrInfo.fdCreator == 'SNCL')
			||   (info_rec.hFileInfo.ioFlFndrInfo.fdType == 'fdrp'))
			&&  ((strcmp(destination_entity_name, "W0:") == 0)			// to w0:
			||   (strcmp(destination_entity_name, "w0:") == 0)			// and erasing or merging folder contents
			||   (strcmp(destination_entity_name, "W1:") == 0)
			||   (strcmp(destination_entity_name, "w1:") == 0))
			&&  (quickerase || merge_folder_contents || systemonly))
				exporting_to_drive = true;								// then export to entire drive w0:
				
			else if ((((info_rec.hFileInfo.ioFlAttrib & 0x10) != 0)		// else if exporting a folder
			||        (info_rec.hFileInfo.ioFlFndrInfo.fdType == 'fdrp'))
			&&       (merge_folder_contents || systemonly))				// and merging it into top level
				destination_entity_name[i-1] = 0;						// remove trailing :; this is name of dest top level directory

			else														// if not merging contents, create name
			{			
				p2cstr(temp_spec.name);									// get C equivalent of entity name
				strncpy(temp_name, (char *) temp_spec.name, sizeof(temp_name));
				c2pstr((char *) temp_spec.name);

				if (temp_name[0] == (char)165)                          // map leading • back to . for able's use
					temp_name[0] = '.';
				
				if (temp_name[0] == '*')								// map leading * back to . for able's use
					temp_name[0] = '.';

				WipeNameExtension(temp_name);
				
				if (truncate_names)										// limit to 8 chars if should do so
					temp_name[8] = 0;

				if (doctor_names)										// fix name if should do so
					for (i=0; i<strlen(temp_name); i++)
						if (!valid_filechar(temp_name[i]))
							temp_name[i] = '_';

				strncat(destination_entity_name, temp_name, sizeof(destination_entity_name) - strlen(destination_entity_name) - 1);	// append that to subcatalog name
			}
		}
		
		else															// else if no colon at end, user is renaming the entity upon export
			strncpy(destination_entity_name, export_able_name, sizeof(destination_entity_name));
		
		
		// If is a file, then export just that one file
		
		if (((info_rec.hFileInfo.ioFlAttrib & 0x10) == 0)
		&&  (info_rec.hFileInfo.ioFlFndrInfo.fdType != 'fdrp'))
		{		
			progress_desired |= full_progress_desired;				// show progress for this file
			
			if (quickerase)											// if erasing, do so now for single-file export
			{
				if (WriteAbleDisk(SCSI_id, zero_buf, 0, 4))
				{
					clean_up();
					return (-1);
				}
				
				if (progress_desired || full_progress_desired)
				{
					if (our_device->fFRefNum)				
						printf("AbleDiskTool: Disk Image File erased...\n");
					else
						printf("AbleDiskTool: SCSI Id %d erased...\n", SCSI_id);
				}
			}

			if (quickerase && exporting_to_drive)					// if exporting disk image to drive
			{
				if (ExportDiskImageToDrive(&temp_spec, &CurrentExportFile, export_mac_entity_name,
		    				               destination_entity_name, destination_entity_name))
				{
					clean_up();
					return (-1);
				}
			}

			else if (ExportMacFile(&temp_spec, &CurrentExportFile, export_mac_entity_name,
			                  destination_entity_name, destination_entity_name, 0))
			{
				clean_up();
				return (-1);
			}
			
			if (delete_after_xport)
				FSpDelete(&temp_spec);
		}


		// Handle export of directory

		else
		{
			char	entry_name     [ 64];
			char	mac_name       [ 64];
			fixed	able_entry	   [ 32];
			char	mac_path_name  [512];
			char	able_path_name [512];
			char	this_mac_name  [512];
			char	this_able_name [512];
						
			short	the_vref = temp_spec.vRefNum;
			
			int		level, do_copy;
			uint32  fldr_count;
			
			
			// Make sure destination path name if valid

			if (quickerase && !exporting_to_drive)				// if erasing, do so here unless writing entire drive
			{
				if (WriteAbleDisk(SCSI_id, zero_buf, 0, 4))
				{
					clean_up();
					return (-1);
				}
			
				if (progress_desired || full_progress_desired)
				{
					if (our_device->fFRefNum)				
						printf("AbleDiskTool: Disk Image File erased...\n");
					else
						printf("AbleDiskTool: SCSI Id %d erased...\n", SCSI_id);
				}
			}
			
			if (exporting_to_drive)								// no need to locate dest directory if exporting to drive
				;
			
			// Prompt user for fixes if we should
			else if (AbleDiskToolAskForRename)
			{
				int action = PromptUserForTweaks(export_mac_entity_name, destination_entity_name, 0, don_t_over_write && !merge_but_warn && !merge_and_replace && !merge_folder_contents);
		
				if (action == Prompt_skip)						// skip
				{
					if (full_progress_desired)
						printf("Skipping export of '%s' as '%s'\n", export_mac_entity_name, destination_entity_name);

					clean_up();
					return (-1);
				}

				if (action == Prompt_abort)						// abort
				{
					g_break_received = true;

					clean_up();
					return (-1);
				}
				// else proceed with possible new name and replace file if it exists			
			}
	
			else if (!locate(destination_entity_name, 0, true))		// see if destination directory exists
			{
				if (c_status != e_no_file)						// if doesn't exist but not because file-not-found
				{	
					get_cat_code_message(c_status, er_mess);
					printf("AbleDiskTool: Can't export '%s' as '%s'\nhere's why:\n", export_mac_entity_name, destination_entity_name);
					printf("   %s\n", er_mess);
					
					clean_up();
					return (-1);
				}
			}

			else if (don_t_over_write && !merge_but_warn && !merge_and_replace && !merge_folder_contents)
			{
				printf("AbleDiskTool: Can't export '%s'; it already exists\n", destination_entity_name);
				clean_up();
				return (-1);
			}
			
			
			// Save FSSpec of directory we are exporting (for later delete).
			// Do so before we resolve any possible file aliases
			
			lev_spec[0] = temp_spec;						// save spec of what's being exported

			
			// Resolve the folder alias if we need to

			if (((info_rec.hFileInfo.ioFlAttrib & 0x10) == 0)
			&&  (info_rec.hFileInfo.ioFlFndrInfo.fdType == 'fdrp'))
			{
				if ((FSstatus = ResolveAliasFile(&temp_spec, true, &targetIsFolder, &wasAliased)) != 0)
				{
					printf("AbleDiskTool: Could not resolve Alias for '%s' (%d)\n", export_mac_entity_name, FSstatus);
				
					if (stop_on_any_error)
					{
						clean_up();
						return (-1);
					}
				}
				
				zero_mem((byte *) &info_rec,  sizeof(info_rec ));
				zero_mem((byte *) &temp_name, sizeof(temp_name));
				info_rec.hFileInfo.ioCompletion = NULL;		
				info_rec.hFileInfo.ioNamePtr    = temp_spec.name;		
				info_rec.hFileInfo.ioVRefNum    = temp_spec.vRefNum;		
				info_rec.hFileInfo.ioFDirIndex  = 0;		
				info_rec.hFileInfo.ioDirID      = temp_spec.parID;		

				if ((FSstatus = PBGetCatInfoSync(&info_rec)) != 0)
				{
					printf("AbleDiskTool: Could not get Alias information on '%s'\n", export_mac_entity_name);
					clean_up();
					return (-1);
				}
				
				if ((info_rec.hFileInfo.ioFlAttrib & 0x10) == 0)
				{
					printf("AbleDiskTool: Resolved Alias for '%s' was not a folder; it is now a file\n", export_mac_entity_name);
					printf("              The Alias '%s' is defective and should be deleted\n", export_mac_entity_name);
					clean_up();
					return (-1);
				}
			}
			
			
			// Loop over folder to export twice; once to compute the sizes of things, then
			// again to do the export...

			this_dir[0] = info_rec.dirInfo.ioDrDirID;		// latch away the directory id
			
			for (do_copy = 0; do_copy < 2; do_copy++)
			{
				// Resolve folder alias if needed
				
				// Initialize for recursive scan
				level         = 0;
				fldr_count    = 0;
				
				this_index[0] = 1;											// mac wants index of 1
				fldr_id   [0] = fldr_count;									// fldr 0 will be size of outer level
				this_size [0] = 0;											// compute size of contents only for now
				spec_size [0] = 0;
				spec_style[0] = 0;
				entries   [0] = 0;

				// Initialize full tree names we maintain
				strncpy(mac_path_name,  export_mac_entity_name , sizeof(mac_path_name ));	// starting name
				strncpy(able_path_name, destination_entity_name, sizeof(able_path_name));
				
				if (mac_path_name[strlen(mac_path_name)-1] != ':')			// add colon to path name if none there
					strncat(mac_path_name, ":", sizeof(mac_path_name) - strlen(mac_path_name) - 1);

				if (!exporting_to_drive)									// add colon unless name is already w0:
					strncat(able_path_name, ":", sizeof(able_path_name) - strlen(able_path_name) - 1);
				
				// Set up for computation of folder sizes if pass 1
				if (!do_copy)
				{
					if (progress_desired || full_progress_desired)
						printf("AbleDiskTool: Calculating subcatalog sizes for \"%s\"\n", able_path_name);
				}
				
				// Create the destination top level directory if need be if pass 2
				else														// else is pass 2
				{
				
					// pass2: if quick-erasing, do so now
					if (quickerase && exporting_to_drive)					// quick erase drive for folder export at start of pass 2
					{
						if (WriteAbleDisk(SCSI_id, zero_buf, 0, 4))
						{
							clean_up();
							return (-1);
						}
						
						if (progress_desired || full_progress_desired)
						{
							if (our_device->fFRefNum)				
								printf("AbleDiskTool: Disk Image File erased...\n");
							else
								printf("AbleDiskTool: SCSI Id %d erased...\n", SCSI_id);
						}
					}
					
					// pass2: else if not quick-erasing, create the top level export directory
					else if (!exporting_to_drive)							// else create destination top level subcatalog if we need to
					{
						sec_len   = fldr_size [fldr_count];					// look up size for this folder
						cat_style = fldr_style[fldr_count];
						
						if (cat_style != 1 && cat_style != 4)				// better by 1 or 4; use lsubc if error
							cat_style = 4;
						
						if (sec_len < cat_style)							// emergency recovery from system errors
							sec_len = cat_style;
									
						word_len = sec_len << 8;
						
						ms_len = (fixed) (sec_len >> 16);
						ls_len = (fixed) (sec_len  & 0xFFFF);
						words  = (fixed) (word_len & 0xFFFF);
						
						if (cat_style == 1)
							cat_type = t_subc;
						else
							cat_type = t_lsubc;
						
						// pass2: if merge into top level directory is desired, do so
						if (merge_but_warn || merge_and_replace || merge_folder_contents)	// if merge desired: use existing subcat
						{
							if (locate(destination_entity_name, 0, true))					// if exists: check type
							{
								if (f_type == t_subc || f_type == t_lsubc)					// if is subcat or cat, ok
								{
									block_num = (((uint32) (uint16) (f_ms_sector & 0xFF)) << 16) | ((uint32) (uint16) f_ls_sector); // block num on W0:
									block_len = (((uint32) (uint16) (f_ms_length & 0xFF)) << 16) | ((uint32) (uint16) f_ls_length); // block len
									
									if (progress_desired || full_progress_desired)
										printf("AbleDiskTool: Merging into subcatalog \"%s\"\n", able_path_name);
								}
								else
								{
									printf("AbleDiskTool: Could not merge into '%s' because it is not a subcatalog; it is a file\n", destination_entity_name);
									clean_up();
									return (-1);
								}
							}
							
							else if (c_status != e_no_file)									// if doesn't exist but not because file-not-found
							{	
								get_cat_code_message(c_status, er_mess);
								printf("AbleDiskTool: Could not access subcatalog '%s' for the following reason:\n", destination_entity_name);
								printf("   %s\n", er_mess);
								clean_up();
								return (-1);
							}
							
							else if (!replace(destination_entity_name, cat_type, ms_len, ls_len, words, 0, true))
							{
								get_cat_code_message(c_status, er_mess);
								printf("AbleDiskTool: Could not create subcatalog '%s' for the following reason:\n", destination_entity_name);
								printf("   %s\n", er_mess);
								clean_up();
								return (-1);
							}
							
							else			// else did create it; zero out cat sectors
							{
								block_num = (((uint32) (uint16) (f_ms_sector & 0xFF)) << 16) | ((uint32) (uint16) f_ls_sector); // block num on W0:
								block_len = (((uint32) (uint16) (f_ms_length & 0xFF)) << 16) | ((uint32) (uint16) f_ls_length); // block len
								
								if (WriteAbleDisk(SCSI_id, zero_buf, block_num, cat_style))
								{
									clean_up();
									return (-1);
								}
								
								if (progress_desired || full_progress_desired)
									printf("AbleDiskTool: Creating subcatalog \"%s\"\n", able_path_name);
							}
						}

						// pass2: else if a merge is not desired, just create the destination folder
						else if (!replace(destination_entity_name, cat_type, ms_len, ls_len, words, 0, true))
						{
							get_cat_code_message(c_status, er_mess);
							printf("AbleDiskTool: Could not create subcatalog '%s' for the following reason:\n", destination_entity_name);
							printf("   %s\n", er_mess);
							clean_up();
							return (-1);
						}
						
						else
						{
							block_num = (((uint32) (uint16) (f_ms_sector & 0xFF)) << 16) | ((uint32) (uint16) f_ls_sector); // block num on W0:
							block_len = (((uint32) (uint16) (f_ms_length & 0xFF)) << 16) | ((uint32) (uint16) f_ls_length); // block len
							
							if (WriteAbleDisk(SCSI_id, zero_buf, block_num, cat_style))
							{
								clean_up();
								return (-1);
							}
							
							if (progress_desired || full_progress_desired)
								printf("AbleDiskTool: Creating subcatalog \"%s\"\n", able_path_name);
						}
					}
					
					if (progress_desired || full_progress_desired)
						printf("AbleDiskTool: Beginning export...\n");
					
					// In export pass 2: begin caching
					cache_treename(true);									// allow tree name caching
					cache_id = cache(c_bufptr, c_bufmed);					// enable cacheing
					caching  = true;
					
					if (!enter_catalog(destination_entity_name, 0, true))	// set up so level 1 accesses that catalog
					{
						get_cat_code_message(c_status, er_mess);
						printf("AbleDiskTool: Could not enter subcatalog '%s' for the following reason:\n", destination_entity_name);
						printf("   %s\n", er_mess);
						clean_up();
						return (-1);
					}
					
					strcpy(level_one_name, destination_entity_name);
				}
				
				fldr_count++;
								
				
				// Loop through this directory

				while (1)													// loop through entries in this directory
				{
					zero_mem((byte *) &info_rec,   sizeof(info_rec));		// clean up
					zero_mem((byte *) &entry_name, sizeof(entry_name));
					
					info_rec.hFileInfo.ioCompletion = NULL;		
					info_rec.hFileInfo.ioNamePtr    = (unsigned char *) entry_name;
					info_rec.hFileInfo.ioVRefNum    = the_vref;
					info_rec.hFileInfo.ioFDirIndex  = this_index[level];		
					info_rec.hFileInfo.ioDirID      = this_dir  [level];		
					
					
					// If end of directory, bump up a level, or done with this iteration
					
					if ((FSstatus = PBGetCatInfoSync(&info_rec)) != 0)		// if end of this directory
					{
						if (FSstatus != fnfErr)
						{
							printf("AbleDiskTool: Could not get information on '%s' %d\n", mac_path_name, FSstatus);
						
							if (stop_on_any_error)
							{
								clean_up();
								return (-1);
							}
						}
						
						// Reserve room for subcatalog itself
						if (spec_style[level] == 1)					// if small subc called for
							this_size[level] += 1;					// account for it
						else										// else leave room for large subcat
							this_size[level] += 4;
						
						// Reserve room for .WORK & .INDEX
						if (systemonly && (level == 0))
							this_size[level] += ( 4000				// 4000  sector .work
							                    +10000				// 10000 sector .index
							                    + 8192				// 8 .sqxdata files
							                    +  392);			// .NEWDATA
											
						// Use user-specified size if available
						if (spec_size[level] > this_size[level])
							this_size[level] = spec_size[level];
							
						// Reserve extra space if desired
						if (create_extra_space)
						{
							this_size[level] += create_extra_space*((this_size[level] + 99)/100);
						
							if (this_size[level] >= (1 << 20))
								this_size[level] = ((1 << 20) - 1);
						}

						// Check for phase error on subcat size computation during pass 2
						if (do_copy)
						{
							if ((!any_files_were_skipped) && (fldr_size [fldr_id [level] ] != this_size[level]))
							{
								printf("AbleDiskTool: Phase error occured; wrong size for '%s' (%d %d)\n", mac_path_name, fldr_size [fldr_id [level] ], this_size[level]);
								
								if (stop_on_any_error)
								{
									clean_up();
									return (-1);
								}
							}
							
							// Create .work & .index at end of pass2
							if (level == 0 && systemonly)
							{
								uint32 	device_size;
								int		i;
							
								// Create .WORK
								if (!locate((void *) ".WORK", 1, true))
								{
									uint32 work_size = 4000;
									
									device_size = (((uint32) (uint16) (c_ms_length & 0xFF)) << 16) | ((uint32) (uint16) c_ls_length); // block len

									if (work_size > (device_size/50))
										work_size = device_size/50;
									
									work_size &= 0xFFFFFFF8;			// make multiple of 8 per XPL compiler
									
									if (work_size < 256)
										work_size = 256;
										
									if (!replace((void *) ".WORK", t_data, 0, work_size, work_size << 8, 1, true))
									{
										get_cat_code_message(c_status, er_mess);
										printf("AbleDiskTool: Could not create .WORK for the following reason:\n");
										printf("   %s\n", er_mess);
										clean_up();
										return (-1);
									}
									
									if (progress_desired || full_progress_desired)
										printf("AbleDiskTool: Creating .WORK\n");
								}
								
								// Create .INDEX
								if (!locate((void *) ".INDEX", 1, true))
								{
									uint32 index_size = 10000;
									
									device_size = (((uint32) (uint16) (c_ms_length & 0xFF)) << 16) | ((uint32) (uint16) c_ls_length); // block len

									if (index_size > (device_size/50))
										index_size = device_size/50;
										
									if (index_size < 256)
										index_size = 256;
										
									if (!replace((void *) ".INDEX", t_lsubc, 0, index_size, index_size << 8, 1, true))
									{
										get_cat_code_message(c_status, er_mess);
										printf("AbleDiskTool: Could not create .INDEX for the following reason:\n");
										printf("   %s\n", er_mess);
										clean_up();
										return (-1);
									}
									
									block_num = (((uint32) (uint16) (f_ms_sector & 0xFF)) << 16) | ((uint32) (uint16) f_ls_sector); // block num on W0:
									block_len = (((uint32) (uint16) (f_ms_length & 0xFF)) << 16) | ((uint32) (uint16) f_ls_length); // block len
									
									if (WriteAbleDisk(SCSI_id, zero_buf, block_num, 4))
									{
										clean_up();
										return (-1);
									}
									
									if (progress_desired || full_progress_desired)
										printf("AbleDiskTool: Creating .INDEX\n");
								}
								
								// Create .SQxDATA
								for (i=0; i<8; i++)
								{
									char *names[] = {
										(char *) ".SQ0DATA",
										(char *) ".SQ1DATA",
										(char *) ".SQ2DATA",
										(char *) ".SQ3DATA",
										(char *) ".SQ4DATA",
										(char *) ".SQ5DATA",
										(char *) ".SQ6DATA",
										(char *) ".SQ7DATA"
									};
									
									if (!locate(names[i], 1, true))
									{
										uint32 sq0_size = 1024;
										
										device_size = (((uint32) (uint16) (c_ms_length & 0xFF)) << 16) | ((uint32) (uint16) c_ls_length); // block len

										if (sq0_size > (device_size/100))
											sq0_size = device_size/100;
											
										if (sq0_size < 128)
											sq0_size = 128;
											
										if (!replace(names[i], t_sync, 0, sq0_size, sq0_size << 8, 1, true))
										{
											get_cat_code_message(c_status, er_mess);
											printf("AbleDiskTool: Could not create %s for the following reason:\n", names[i]);
											printf("   %s\n", er_mess);
											clean_up();
											return (-1);
										}
										
										block_num = (((uint32) (uint16) (f_ms_sector & 0xFF)) << 16) | ((uint32) (uint16) f_ls_sector); // block num on W0:
										block_len = (((uint32) (uint16) (f_ms_length & 0xFF)) << 16) | ((uint32) (uint16) f_ls_length); // block len
										
										if (WriteAbleDisk(SCSI_id, zero_buf, block_num, 4))
										{
											clean_up();
											return (-1);
										}
										
										if (progress_desired || full_progress_desired)
											printf("AbleDiskTool: Creating %s\n", names[i]);
									}
								}
								
								// Create .NEWDATA
								if (!locate((void *) ".NEWDATA", 1, true))
								{
									uint32 new_size = 392;
									int	   where = 0;
									
									device_size = (((uint32) (uint16) (c_ms_length & 0xFF)) << 16) | ((uint32) (uint16) c_ls_length); // block len

									if (new_size > (device_size/50))
										new_size = device_size/50;
										
									if (new_size < 192)
										new_size = 192;
										
									device_size = (((uint32) (uint16) (c_ms_length & 0xFF)) << 16) | ((uint32) (uint16) c_ls_length); // block len
									
									if (!replace((void *) ".NEWDATA", t_timbre, 0, new_size, new_size << 8, 1, true))
									{
										get_cat_code_message(c_status, er_mess);
										printf("AbleDiskTool: Could not create .NEWDATA for the following reason:\n");
										printf("   %s\n", er_mess);
										clean_up();
										return (-1);
									}
									
									block_num = (((uint32) (uint16) (f_ms_sector & 0xFF)) << 16) | ((uint32) (uint16) f_ls_sector); // block num on W0:
									block_len = (((uint32) (uint16) (f_ms_length & 0xFF)) << 16) | ((uint32) (uint16) f_ls_length); // block len
									
									while (where < block_len)
									{
										uint32 chunk = block_len - where;
										
										if (chunk > 4)
											chunk = 4;
											
										if (WriteAbleDisk(SCSI_id, zero_buf, block_num + where, chunk))
										{
											clean_up();
											return (-1);
										}
									
										where += chunk;
									}
									
									if (progress_desired || full_progress_desired)
										printf("AbleDiskTool: Creating .NEWDATA\n");
								}
							}
							
							flush_cache(cache_id);							// flush cache at end of mac directory during pass 2
						}
						
						// Else at end of directory during pass 1, just record computed size
						else
						{
							if (this_size[level] >= (1 << 20))				// check for too big for subcatalog
							{
								printf("AbleDiskTool: Subcatalog \"%s\" is larger than 512 Mb and can't be created\n", able_path_name);
								
								if (stop_on_any_error)
								{
									clean_up();
									return (-1);
								}
							}
							
							fldr_size [fldr_id [level] ] = this_size [level];
							fldr_style[fldr_id [level] ] = spec_style[level];

							if (full_progress_desired)
								printf("AbleDiskTool: Size needed for \"%s\" is %d sectors\n", able_path_name, fldr_size [fldr_id [level] ]);
						}
						
						if (level)											// if in sub folder, continue
						{
							i = strlen(mac_path_name);						// current length of path name
							
							if (i != 0)										// if any
								mac_path_name[--i] = 0;						// remove trailing colon
								
							while (i > 0 && mac_path_name[i] != ':')		// remove directory name from end of path
								mac_path_name[i--] = 0;
								
							i = strlen(able_path_name);						// current length of path name
							
							if (i != 0)										// if any
								able_path_name[--i] = 0;					// remove trailing colon
								
							while (i > 0 && able_path_name[i] != ':')		// remove directory name from end of path
								able_path_name[i--] = 0;
							
							if (level == 1)									// if going back to top level
							{
								if (do_copy && !enter_catalog(destination_entity_name, 0, true))	// set up for level 1 access to this subcatalog
								{
									get_cat_code_message(c_status, er_mess);
									printf("AbleDiskTool: Could not enter subcatalog '%s' for the following reason:\n", destination_entity_name);
									printf("   %s\n", er_mess);
									clean_up();
									return (-1);
								}
								strcpy(level_one_name, destination_entity_name);
							}
							else											// else tweak path name for enter
							{
								able_path_name[i] = 0;						// remove colon for a moment
								
								if (do_copy && !enter_catalog(able_path_name, 0, true))		// set up for level 1 access to this subcatalog
								{
									get_cat_code_message(c_status, er_mess);
									printf("AbleDiskTool: Could not enter subcatalog '%s' for the following reason:\n", able_path_name);
									printf("   %s\n", er_mess);
									clean_up();
									return (-1);
								}
								
								strcpy(level_one_name, able_path_name);
								
								able_path_name[i] = ':';					// restore trailing colon
							}
							
							// Delete folder during pass 2 if should
							if (do_copy && delete_after_xport)				// if should delete the folder during export
							{
								if (FSpDelete(&lev_spec[level]) != noErr)	// delete the folder
									this_index[level-1]++;					// and advance to next file if got error
							}
							
							this_size[level-1] += this_size[level];			// accumulate space for this folder in upper level
							level--;
							continue;										// continue with prior level
						}					
						
						else												// else we are done!
						{
							if (do_copy)									// else if pass 2, clean up from cacheing
							{
								disable_cache(cache_id);					// disable further cacheing
								cache_treename(false);						// disallow further tree name caching
								caching = false;
								
								if (delete_after_xport)						// delete entity being exported
									FSpDelete(&lev_spec[0]);				// if all done
							}
							break;
						}
					}
					

					// If not end of the directory, look at the entry.  Begin by computing
					// the full path name
					
					p2cstr((uint8 *) entry_name);
                    
                    // Ignore Mac OS .DS_Store files on folder export
                    bool is_DS_Store = (strcmp((const char *) entry_name, ".DS_Store") == 0);

					strncpy(this_mac_name, mac_path_name, sizeof(this_mac_name));
					strncat(this_mac_name, entry_name,    sizeof(this_mac_name) - strlen(this_mac_name) - 1);

					strncpy(temp_name, entry_name, sizeof(temp_name));
					strcpy (mac_name,  entry_name);
					
					if (temp_name[0] == (char)165)                          // map leading • back to . for able's use
						temp_name[0] = '.';

					if (temp_name[0] == '*')								// map leading * back to . for able's use
						temp_name[0] = '.';
					
					WipeNameExtension(temp_name);
					
					if (truncate_names)										// limit to 8 chars if should do so
						temp_name[8] = 0;

					if (doctor_names)										// fix name if should do so
						for (i=0; i<strlen(temp_name); i++)
							if (!valid_filechar(temp_name[i]))
								temp_name[i] = '_';
					
					to_able_string (temp_name, able_entry);					// get as able string

					strncpy(this_able_name, able_path_name, sizeof(this_able_name));
					strncat(this_able_name, temp_name, sizeof(this_able_name) - strlen(this_able_name) - 1);
					
					c2pstr(entry_name);
					
					// Make an FS Spec for the file
					if ((FSstatus = FSMakeFSSpec (the_vref, this_dir [level], (uint8 *) entry_name,  &temp_spec)) != 0)
					{
						printf("AbleDiskTool: Could not get information on '%s' (%d)\n", this_mac_name, FSstatus);
						clean_up();
						return (-1);
					}
					
					// Skip invisible files
					if (info_rec.hFileInfo.ioFlFndrInfo.fdFlags & kIsInvisible)
						;

                    // Skip .DS_Store. Puzzling that is not identified as "Invisible"...
                    else if (is_DS_Store)
                        ;
                    
					// Skip the file or directory if exporting system files only
					// and this is not a system entity
					
					else if (systemonly										// if only accessing system files
					&&  (is_able_system_entity(this_able_name) == false))	// not a system entity
						;													// just skip it
					
					// Check for invalid file name if we don't have a name validator present
					
					else if (!AbleDiskToolAskForRename && !valid_filename(able_entry))
					{
						printf("AbleDiskTool: Invalid file name '%s'\n", this_able_name);
						
						if (stop_on_any_error)
						{
							clean_up();
							return (-1);
						}
					}
					
					// If this entry is a file, compute how much storage it will
					// require
					
					else if (((info_rec.hFileInfo.ioFlAttrib & 0x10) == 0)			// if is file
					&&       (info_rec.hFileInfo.ioFlFndrInfo.fdType != 'fdrp'))	// and not an alias file to a folder
					{
                        if ((strcmp(mac_name, SubcatalogInfoFileName ) == 0)        // Force text for __size__ files created by unix shell scripts (e.g. echo)
                        ||  (strcmp(mac_name, SubcatalogInfoFileName1) == 0))
                            export_type = t_text;
                        
                        else
                            export_type = GetAbleFileType(info_rec.hFileInfo.ioFlFndrInfo.fdType, info_rec.hFileInfo.ioFlFndrInfo.fdCreator, &temp_spec);
			
						if (export_type == -1)
						{
							if (!report_unrecognized_files)
								;
								
							else
							{
								printf("AbleDiskTool: Unrecognized file type for file '%s'\n", this_able_name);
								
								if (stop_on_any_error)
								{
									clean_up();
									return (-1);
								}
							}
						}

						// For text, AIFF, Sd2f and WAVE files, handle specially to compute length
						else if (( export_type == t_text  )							// if text file
						||		 ((export_type == t_sound)
						&&        (info_rec.hFileInfo.ioFlFndrInfo.fdType == AIFFID
						||         info_rec.hFileInfo.ioFlFndrInfo.fdType == SD2FileType
						||         info_rec.hFileInfo.ioFlFndrInfo.fdType == WAVEID     )))
						{
							// Resolve possible alias
							if ((FSstatus = ResolveAliasFile(&temp_spec, true, &targetIsFolder, &wasAliased)) != 0)
							{
								printf("AbleDiskTool: Could not resolve Alias for '%s' (%d)\n", this_mac_name, FSstatus);
							
								if (stop_on_any_error)
								{
									clean_up();
									return (-1);
								}
							}

							// Scan subcat data if that's what the file is
							else if ((export_type == t_text)
							&&       ((strcmp(mac_name, SubcatalogInfoFileName ) == 0)
							||        (strcmp(mac_name, SubcatalogInfoFileName1) == 0)))
							{
								if (ignore__size__)
									;

								else if ((FSstatus = OpenMacFile(&temp_spec, &CurrentExportFile, this_mac_name)) != 0)
									;

								else
								{
									uint32	scanned_size;
									uint32	scanned_style;
									
									Scan__SIZE__File(CurrentExportFile.MacFileLen,
												     CurrentExportFile.MacFRefNum,
												     this_mac_name, &scanned_size, &scanned_style);
												     
									if (scanned_size && (scanned_style == 1 || scanned_style == 4))
									{
									   spec_size [level] = scanned_size;
									   spec_style[level] = scanned_style;
								
										if (spec_size[level] >= (1 << 20))
										{
											spec_size[level] = ((1 << 20) - 1);
											
											printf("AbleDiskTool: __SIZE__ specification is too large in file '%s' (limit is %d)\n", this_mac_name, ((1 << 20) - 1));
											
											if (stop_on_any_error)
											{
												clean_up();
												return (-1);
											}
										}
									}
									
									else
									{
										printf("AbleDiskTool: __SIZE__ specification can't be read from '%s'\n", this_mac_name);
										
										if (stop_on_any_error)
										{
											clean_up();
											return (-1);
										}
									}
									
									CloseMacFile(CurrentExportFile);
								}
							}

							// Else if pass 2, write the text or sound file out
							else if (do_copy)								// if pass 2, do the export
							{
								entries[level]++;							// count 1 more able entry this level
					
								if (ExportMacFile(&temp_spec, &CurrentExportFile, this_mac_name, this_able_name, temp_name, 1))
								{
									if (stop_on_any_error || g_break_received)
									{
										clean_up();
										return (-1);
									}
								}
								this_size[level] += CurrentExportFile.blocks_allocated;
							}

							// Else if pass 1, open the file so we can compute its length
							else if ((FSstatus = OpenMacFile(&temp_spec, &CurrentExportFile, this_mac_name)) != 0)
							{
								printf("AbleDiskTool: Could not open '%s' (%d)\n", this_mac_name, FSstatus);
								
								if (stop_on_any_error)
								{
									clean_up();
									return (-1);
								}
							}
							
							// and compute its length
							else
							{
								if (export_type == t_text)					
									word_len = ComputeWordLengthOfMacTextFile(CurrentExportFile.MacFileLen,
									                                          CurrentExportFile.MacFRefNum,
									                                          this_mac_name);
								else
									word_len = ComputeSoundFileSize(CurrentExportFile.MacFRefNum,
									                                CurrentExportFile.MacRRefNum, this_mac_name, CurrentExportFile.MacFileTyp);
								
								if (word_len == (-1))
								{
									if (stop_on_any_error)
									{
										clean_up();
										return (-1);
									}
								}
								
								else if (entries[level] >= 128)										// to many entries
								{
									printf("AbleDiskTool: Out of room (1) in subcatalog '%s' for file '%s'\n", able_path_name, temp_name);
									
									if (stop_on_any_error)
									{
										clean_up();
										return (-1);
									}
								}

								else
								{
									entries[level]++;												// count 1 more able entrythis level
						
									this_size[level] += (word_len + 255) >> 8;						// length of text or sound file
									
									CloseMacFile(CurrentExportFile);
								}
							}
						}

						// Else if data file, make sure we have room
						else if (entries[level] >= 128)												// to many entries
						{
							printf("AbleDiskTool: Out of room (2) in subcatalog '%s' for file '%s'\n", able_path_name, temp_name);
							
							if (stop_on_any_error)
							{
								clean_up();
								return (-1);
							}
						}
						
						// Else if not a text file, must be a data file
						// and length computation is easier
						
						else
						{
							uint32 entity_size = ((info_rec.hFileInfo.ioFlLgLen >> 1) + 255) >> 8;	// get sector size handy
							
							entries[level]++;														// count 1 more able entry this level

							// Export of data file, pass 1: compute what size will be
							if (!do_copy)
							{							
								// Squeeze or Expand disk images on export
								if (export_type == t_subc || export_type == t_lsubc)
								{
									// Export of disk image, pass 1: open the file and sequeeze it if desired
									if (squeeze_images_on_transfer)
									{
									// Resolve possible alias
										if ((FSstatus = ResolveAliasFile(&temp_spec, true, &targetIsFolder, &wasAliased)) != 0)
										{
											printf("AbleDiskTool: Could not resolve Alias for '%s' (%d)\n", this_mac_name, FSstatus);
										
											if (stop_on_any_error)
											{
												clean_up();
												return (-1);
											}
										}

										// Else if pass 1, open the file so we can compute its squeezed size
										else if ((FSstatus = OpenMacFile(&temp_spec, &CurrentExportFile, this_mac_name)) != 0)
										{
											printf("AbleDiskTool: Could not open '%s' (%d)\n", this_mac_name, FSstatus);
											
											if (stop_on_any_error)
											{
												clean_up();
												return (-1);
											}
										}
										
										// and compute its squeezed size
										else
										{						
											word_len = ComputeSqueezedSubcatSize(CurrentExportFile.MacFileLen,
											                                     CurrentExportFile.MacFRefNum,
											                                     this_mac_name, export_type);
											if (word_len == (-1))
											{
												if (stop_on_any_error)
												{
													clean_up();
													return (-1);
												}
											}
											
											else
											{
												entity_size = (word_len + 255) >> 8;					// length of sequeezed subcat
												CloseMacFile(CurrentExportFile);
											}
										}
									}
									
									if (create_extra_space)
									{
										entity_size += create_extra_space*((entity_size + 99)/100);		// note duplicate computation
																										// in exportmacfile!!
										if (entity_size >= (1 << 20))
											entity_size = ((1 << 20) - 1);
									}
								}
								
								this_size [level] += entity_size;										// length of data file
							}
							
							// else if pass 2, copy the data file out
							else
							{
								// Resolve possible alias
								if ((FSstatus = ResolveAliasFile(&temp_spec, true, &targetIsFolder, &wasAliased)) != 0)
								{
									printf("AbleDiskTool: Could not resolve Alias for '%s' (%d)\n", this_mac_name, FSstatus);
								
									if (stop_on_any_error)
									{
										clean_up();
										return (-1);
									}
								}

								else if (ExportMacFile(&temp_spec, &CurrentExportFile, this_mac_name, this_able_name, temp_name, 1))
								{
									if (stop_on_any_error || g_break_received)
									{
										clean_up();
										return (-1);
									}
								}
								
								this_size [level] += CurrentExportFile.blocks_allocated;
							}
						}
					}
					
					
					// Else if item is a folder, get ready to recurs
					
					else if (fldr_count >= MAX_FOLDERS-1)
					{
						printf("AbleDiskTool: Out of folder memory for '%s'\n", this_mac_name);
						
						if (stop_on_any_error)
						{
							clean_up();
							return (-1);
						}
					}
						
					else if (level >= MAX_LEVELS-1)
					{
						printf("AbleDiskTool: Out of level memory for '%s'\n", this_mac_name);
						
						if (stop_on_any_error)
						{
							clean_up();
							return (-1);
						}
					}
											
					else													// is directory & we can process it
					{
						entries[level]++;									// count 1 more able entrythis level
							
						strncpy(mac_path_name, this_mac_name, sizeof(mac_path_name));		// get its full name
						strncat(mac_path_name, ":",           sizeof(mac_path_name) - strlen(mac_path_name) - 1);
						
						strncpy(able_path_name, this_able_name, sizeof(able_path_name));	// get its full name
						
						if (!do_copy || !delete_after_xport)				// if in first pass, or we are not deleteing the files
							this_index[level]++;							// advance over this level for when we return to this file
																			// else leave index at one since we will delete the folder entry!!						
						
						lev_spec[level+1] = temp_spec;						// save FSSpec away for the directory we are about to process
						
						// Resolve the folder alias

						if (((info_rec.hFileInfo.ioFlAttrib & 0x10) == 0)
						&&  (info_rec.hFileInfo.ioFlFndrInfo.fdType == 'fdrp'))
						{
							if ((FSstatus = ResolveAliasFile(&temp_spec, true, &targetIsFolder, &wasAliased)) != 0)
							{
								printf("AbleDiskTool: Could not resolve Alias for '%s' (%d)\n", this_mac_name, FSstatus);
							
								if (stop_on_any_error)
								{
									clean_up();
									return (-1);
								}
							}
							
							zero_mem((byte *) &info_rec,  sizeof(info_rec ));
							info_rec.hFileInfo.ioCompletion = NULL;		
							info_rec.hFileInfo.ioNamePtr    = temp_spec.name;		
							info_rec.hFileInfo.ioVRefNum    = temp_spec.vRefNum;		
							info_rec.hFileInfo.ioFDirIndex  = 0;		
							info_rec.hFileInfo.ioDirID      = temp_spec.parID;		

							if ((FSstatus = PBGetCatInfoSync(&info_rec)) != 0)
							{
								printf("AbleDiskTool: Could not get Alias information on '%s'\n", this_mac_name);
								clean_up();
								return (-1);
							}
							
							if ((info_rec.hFileInfo.ioFlAttrib & 0x10) == 0)
							{
								printf("AbleDiskTool: Resolved Alias for '%s' was not a folder; it is now a file\n", this_mac_name);
								printf("              The Alias '%s' is defective and should be deleted\n", this_mac_name);
								clean_up();
								return (-1);
							}
						}
						
						// Lock
						#if (0)												// supposed to be able to lock directories!!!
							FSstatus = HSetFLock(the_vref, info_rec.dirInfo.ioDrDirID, NULL);
							printf("Lock   status: %d\n", FSstatus);
							FSstatus = HRstFLock(the_vref, info_rec.dirInfo.ioDrDirID, NULL);
							printf("Unlock status: %d\n", FSstatus);
						#endif
						
						// Çreate the subdirectory
						if (do_copy)
						{
							sec_len   = fldr_size [fldr_count];				// look up size for this folder
							cat_style = fldr_style[fldr_count];
							
							
							// Attempt to recover from phase errors as best
							// we can.  Look for the folder size that goes with this
							// directory id.
							
							if (fldr_ids [fldr_count] != info_rec.dirInfo.ioDrDirID)
							{
								for (i=0; i<fldr_count; i++)
								{
									if (fldr_ids [i] == info_rec.dirInfo.ioDrDirID)
									{
										sec_len   = fldr_size [i];
										cat_style = fldr_style[fldr_count];
									}
								}
							}
							
							if (cat_style != 1 && cat_style != 4)			// better by 1 or 4; use lsubc if error
								cat_style = 4;
							
							if (sec_len < cat_style)						// emergency recovery from system errors
								sec_len = cat_style;
										
							word_len = sec_len << 8;
							
							ms_len = (fixed) (sec_len >> 16);
							ls_len = (fixed) (sec_len  & 0xFFFF);
							words  = (fixed) (word_len & 0xFFFF);
							
							if (cat_style == 1)
								cat_type = t_subc;
							else
								cat_type = t_lsubc;
							
							if (merge_but_warn || merge_and_replace)			// if merge desired: use existing subcat
							{
								if (locate(temp_name, 1, true))					// if exists: check type
								{
									if (f_type == t_subc || f_type == t_lsubc)	// if is subcat or cat, ok
									{
										block_num = (((uint32) (uint16) (f_ms_sector & 0xFF)) << 16) | ((uint32) (uint16) f_ls_sector); // block num on W0:
										block_len = (((uint32) (uint16) (f_ms_length & 0xFF)) << 16) | ((uint32) (uint16) f_ls_length); // block len
										
										if (progress_desired || full_progress_desired)
											printf("AbleDiskTool: Merging into subcatalog \"%s\"\n", able_path_name);
									}
									else
									{
										printf("AbleDiskTool: Could not merge into '%s' because it not a subcatalog\n", able_path_name);
										clean_up();
										return (-1);
									}
								}
								
								else if (!replace(temp_name, cat_type, ms_len, ls_len, words, 1, true))
								{
									get_cat_code_message(c_status, er_mess);
									printf("AbleDiskTool: Could not create subcatalog '%s' for the following reason:\n", able_path_name);
									printf("   %s\n", er_mess);
									clean_up();
									return (-1);
								}
								
								else
								{
									block_num = (((uint32) (uint16) (f_ms_sector & 0xFF)) << 16) | ((uint32) (uint16) f_ls_sector); // block num on W0:
									block_len = (((uint32) (uint16) (f_ms_length & 0xFF)) << 16) | ((uint32) (uint16) f_ls_length); // block len
									
									if (WriteAbleDisk(SCSI_id, zero_buf, block_num, cat_style))
									{
										clean_up();
										return (-1);
									}
									
									if (progress_desired || full_progress_desired)
										printf("AbleDiskTool: Creating subcatalog \"%s\"\n", able_path_name);
								}
							}

							else if (!replace(temp_name, cat_type, ms_len, ls_len, words, 1, true))
							{
								get_cat_code_message(c_status, er_mess);
								printf("AbleDiskTool: Could not create subcatalog '%s' for the following reason:\n", able_path_name);
								printf("   %s\n", er_mess);
								clean_up();
								return (-1);
							}
							
							else
							{
								block_num = (((uint32) (uint16) (f_ms_sector & 0xFF)) << 16) | ((uint32) (uint16) f_ls_sector); // block num on W0:
								block_len = (((uint32) (uint16) (f_ms_length & 0xFF)) << 16) | ((uint32) (uint16) f_ls_length); // block len
								
								if (WriteAbleDisk(SCSI_id, zero_buf, block_num, cat_style))
								{
									if (stop_on_any_error)
									{
										clean_up();
										return (-1);
									}
								}
								
								if (progress_desired || full_progress_desired)
									printf("AbleDiskTool: Creating subcatalog \"%s\"\n", able_path_name);
							}
							
							flush_cache(cache_id);							// flush cache before entering
							
							if (!enter_catalog(able_path_name, 0, true))	// set up for level 1 access to this subcatalog
							{
								get_cat_code_message(c_status, er_mess);
								printf("AbleDiskTool: Could not enter subcatalog '%s' for the following reason:\n", able_path_name);
								printf("   %s\n", er_mess);
								clean_up();
								return (-1);
							}
							
							strcpy(level_one_name, able_path_name);
						}
					
						// Else if during pass 1, just emit progress info if desired
							
						else if (full_progress_desired)
							printf("AbleDiskTool: Calculating subcatalog sizes for \"%s\"\n", able_path_name);
						
						strncat(able_path_name, ":", sizeof(able_path_name) - strlen(able_path_name) - 1);	// finish constructing able path name
						
						level++;											// bump level
						
						this_index[level] = 1;								// mac wants index of 1
						this_size [level] = 0;								// total up contents only for now
						spec_size [level] = 0;
						spec_style[level] = 0;
						this_dir  [level] = info_rec.dirInfo.ioDrDirID;		// directoy id of what we are exporting
						fldr_id   [level] = fldr_count;						// save folder id for this level
						entries   [level] = 0;								// prep to count entries on this level
						
						fldr_ids [fldr_count] = info_rec.dirInfo.ioDrDirID;	// to verify in pass 2

						fldr_count++;
						
						continue;											// and start scan of that level
					}
					
					if (!do_copy || !delete_after_xport)					// if in first pass, or we are not deleteing the files
						this_index[level]++;								// advance to next file
					else if (FSpDelete(&temp_spec) != noErr)				// else delete the file
						this_index[level]++;								// and advance to next file if got error

					run_host_environment_250();
					
					if (g_break_received)
					{
						clean_up();
						return (-1);
					}
				}
				
				if (verify_only)											// skip second pass if verify only
					break;
			}
		}
	}

	// Handle recursive import, catalog, or dump
	
	else if (catalog || dumpcontents)
	{	
		if (FetchAndRecurseAbleCatalog((char *) "", SCSI_id, 0, 4, VRefNum, 0, false, (char *) ""))
		{
			clean_up();
			return (-1);
		}
	}
	
	
	// Eject media after operation if desired

	if (do_eject && SCSI_id < 7)
		issue_start_unit(our_device, 1, 1, 0);

	
	// Done
	
	if (g_break_received)			// if we intercepted a break
	{
		clean_up();
		return (-1);
	}
	
	clean_up();						// else done
	return (0);
}
