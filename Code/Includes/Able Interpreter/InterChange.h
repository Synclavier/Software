/* InterChange.h */

#ifndef	INTERCHANGE_H
#define	INTERCHANGE_H

#include <CoreServices/CoreServices.h>
#include <ApplicationServices/ApplicationServices.h>

#include "Standard.h"
#include "XPL.h"
#include "SynclavierFileReference.h"

// Provide our own types for soon-to-be-deprecated types
#if __LP64__
    typedef CFDataRef       xpl_file_alias;
#else
    typedef AliasHandle     xpl_file_alias;
#endif

typedef RGBColor        interchange_color;

#pragma pack(push,4)

// Signature struct for prefs file
typedef struct sig_struct {							// signature struct
	unsigned int	our_id;
	unsigned int	rev_num;
} sig_struct;

#define	INTERCHANGE_CURRENT_SIG_ID	'ICHG'
#define	INTERCHANGE_CURRENT_SIG_REV	24
#define	INTERCHANGE_ID_TABLE_SIZE	200

// InterChange Setting Codes. Originally mapped to InterChange 1.0 bus menu
enum scsi_bus_options {
	SCSI_BUS_MENU_NONE = 1,
    SCSI_BUS_MENU_MAC_SCSI_PORT,
    SCSI_BUS_MENU_DISK_IMAGE,
	SCSI_BUS_MENU_D24,
    SCSI_BUS_MENU_END
};

// InterChange Setting ALIS Resource ID's
enum												// alias resources
{
	W0_IMAGE_FILE_ALIS_RESOURCE_ID = 1024,
	W1_IMAGE_FILE_ALIS_RESOURCE_ID,
	O0_IMAGE_FILE_ALIS_RESOURCE_ID,
	O1_IMAGE_FILE_ALIS_RESOURCE_ID,
	EXPORT_FOLDER_ALIS_RESOURCE_ID,
	EXPORT_IMAGE_ALIS_RESOURCE_ID,
	EXPORT_FILE_ALIS_RESOURCE_ID,
	EXPORT_SYSTEM_FOLDER_ALIS_RESOURCE_ID,
	EXPORT_SYSTEM_IMAGE_ALIS_RESOURCE_ID,
	IMPORT_FOLDER_ALIS_RESOURCE_ID
};

enum
{
	kInfoResourceType 	  = 'info',					// holds user settings that change frequently
	INFO_RESOURCE_ID      = 128
};

// Handy struct for a scsi setting. This struct is stored in the prefs file, so it is difficult to change.
typedef	struct scsi_settings
{
	ulong				bus_id;						// holds bus id menu item index - scsi_bus_options
	ulong				device_id;					// holds device menu item index - SCSI_ID_MENU_ID - not used any more
	ulong				scsi_id;					// holds working scsi id of device if on a mac or D24 scsi bus
	ulong				sim_id;						// used as index to g_indexed_device
	char				inq_name[32];				// inq name
	ulong				disk_type;					// type of disk
	xpl_file_alias 		image_file;					// handle to disk image file alias; meaningless except when in memory
	SyncFSSpec          image_spec;					// file spec for image file
	char				image_pathname[512];		// full path name for image file; HFS path for 32-bit builds; posix path for 64-bit builds

}	scsi_settings;

// Mongo struct for all InterChangeª 1.0 settings
typedef	struct interchange_settings
{			
	scsi_settings		w0;							// holds settings for W0
	scsi_settings		w1;							// holds settings for W1
	scsi_settings		o0;							// holds settings for O0
	scsi_settings		o1;							// holds settings for O1

	xpl_file_alias		export_fldr;				// alias to folder to export; meaningless except when in memory
	xpl_file_alias		export_imge;				// alias to image to export; meaningless except when in memory
	xpl_file_alias		export_file;				// alias to file to export; meaningless except when in memory
	xpl_file_alias		export_sysf;				// alias to system folder to export; meaningless except when in memory
	xpl_file_alias		export_sysi;				// alias to system file to export; meaningless except when in memory
	xpl_file_alias		import_fldr;				// alias to dest fldr into which to store imported entities; meaningless except when in memory
	Handle				info;						// info resource; meaningless except when in memory

	char				export_fldrname[512];		// full path name of export folder; HFS path for 32-bit builds; posix path for 64-bit builds
	char				export_imgename[512];		// full path name of export disk image file name; HFS path for 32-bit builds; posix path for 64-bit builds
	char				export_filename[512];		// full path name of export file name; HFS path for 32-bit builds; posix path for 64-bit builds
	char				export_sysfname[512];		// full path name of system folder; HFS path for 32-bit builds; posix path for 64-bit builds
	char				export_sysiname[512];		// full path name of export disk image file name; HFS path for 32-bit builds; posix path for 64-bit builds
	char				import_fldrname[512];		// full path name of import folder; HFS path for 32-bit builds; posix path for 64-bit builds

	SyncFSSpec          export_fldrspec;			// FSSpecs for them as well.
	SyncFSSpec          export_imgespec;
	SyncFSSpec          export_filespec;
	SyncFSSpec          export_sysfspec;
	SyncFSSpec          export_sysispec;
	SyncFSSpec          import_fldrspec;
	
	ulong				what_to_do;					// what to do menu selection
	ulong				what_to_import;				// what to import menu selection
	char				what_to_import_text [256];	// what to import text
	ulong				where_to_export;			// where to export device
	char				where_to_export_text[256];	// where to export to text
	
	ulong				logging_options;			// options settings
	ulong				error_options;
	ulong				filenames_options;
	ulong				disk_image_options;
	ulong				subcatalog_options;
	ushort				soundfile_options;
	ushort				replacing_options;
	ushort				coerce_sample_rate;
	ushort				eject_media;
	
	ulong				show_setup;					// saved screen configuration
	ulong				show_options;
	
	ulong				image_file_size;			// entered size for new image file (megabytes)

	ulong				enter_can_activate_copy;	// true if current settings allow operation with <enter>
	ulong				need_to_save_settings;		// true if user made changes to setup
	ulong				need_to_export_settings;	// true if user changed settings have not been exported
	ulong				in_forground;				// true if our application has been activated
	ulong				export_active;				// true if export modal dialog is the active window (e.g. SIOUX is not)
	ulong				we_are_working;				// true if busy processing
	ulong				any_new_setting;			// true if user settings (non-config) have changed this session
	ulong				SIOUX_window_is_detatched;	// true if user has detatched SIOUX window
	ulong				have_seen_SIOUX_yet;		// allows us to detect SIOUX window first opening
	short				current_scolled_amount;		// amount dialog has been scrolled vertically
	ulong				selected_text_field;		// currently selected text field
	short				recognize_disks;			// override unrecognized able disk warning
	short				allow_mac_erase;			// true if enabled to erase macintosh disk

}	interchange_settings;

// Local struct to save settings for some InterChangeª 1.0 screen items
typedef	struct saved_settings
{
	ulong				what_to_do;					// what to do menu selection
	ulong				what_to_import;				// what to import menu selection
	char				what_to_import_text [256];	// what to import text
	ulong				where_to_export;			// where to export device
	char				where_to_export_text[256];	// where to export text

	ulong				logging_options;			// options settings
	ulong				error_options;
	ulong				filenames_options;
	ulong				disk_image_options;
	ulong				subcatalog_options;
	ushort				soundfile_options;
	ushort				replacing_options;
	ushort				coerce_sample_rate;
	ushort				eject_media;

	ulong				show_setup;					// screen configuration
	ulong				show_options;

}	saved_settings;

// Struct for interpreter settings
typedef	struct interpreter_settings
{
	ulong				cable_setting;				// holds cable length menu item index
	ulong				bus_setting;				// holds bus_loading_setting
	ulong				net_visible;				// holds network visible item index
	ulong				use_mp;						// use MP
	uint32_ratio		metronome_calib_data;		// measured calibration data
	ulong				m512k;						// setting for m512ks to model
	ulong				dont_warn_on_file_change;	// don't warn after interchange file change
	ulong				polymem;					// setting for number of poly megs to simulate
	ulong				spare[119];
}	interpreter_settings;

#define		NUM_ORIG_MIDI_PATCHINGS		16
#define		NUM_NEW_MIDI_PATCHINGS		80
#define		MIDI_PATCHING_STRING_SIZE	256

typedef	struct interpreter_midi_patching
{
	sig_struct			midi_sig;
	char				midi_to_synclavierx  [NUM_NEW_MIDI_PATCHINGS][MIDI_PATCHING_STRING_SIZE];	// 80 strings for midi input to SynclavierX
	char				midi_from_synclavierx[NUM_NEW_MIDI_PATCHINGS][MIDI_PATCHING_STRING_SIZE];	// 80 strings for output from SynclavierX
	
}	interpreter_midi_patching;

// Various defaults
enum {
	default_size_sectors, default_size_kb, default_size_mb
};

enum {
	default_to_disk_image, default_to_optical_image
};

typedef	struct interchange2_settings
{
	int                 settings_available;			// true if settings are valid
	interchange_color   sound_color;				// color for sound file text
	interchange_color	sequence_color;				// color for sequence file text
	interchange_color	timbre_color;				// color for timbre file text
	interchange_color	other_color;				// color for any other text
	
	int                 default_sort;				// default sort order for new window
	
	int                 show_treename;				// settings for window contents
	int                 show_filename;
	int                 show_icon;
	int                 show_filetype;
	int                 show_size_sec;
	int                 show_sizekb;
	int                 show_sizemb;
	
	int                 skip_unsave_warning;		// bypass unsave warning dialog
	int                 skip_copy_warning;			// bypass copy warning dialog
	int                 normally_move;				// normally move items instead of copying them (not implemented)
	int                 default_size_entry;			// default subcat size entry format
	int                 audition_upon_recall;

	int                 information;				// default information

	int                 settings_changed;			// set to force update of settings on quit
	
	int                 header_set;					// true if header height & width set
	int                 header_height;				// header area height
	int                 header_width;				// header area width
	
	int                 window_set;					// true if window position set
	int                 window_top;					// window position and size on screen
	int                 window_left;
	int                 window_bottom;
	int                 window_right;
	
	int                 layout_set;					// true if panel layout is set
	int                 setting_value;
	int                 option_value;
	int                 log_value;
	
	int                 legacy_dev_pos_set;			// true if cell position of legacy devices is set
	short				legacy_dev_row[4];			// cell row of legacy device		
	short				legacy_dev_col[4];			// cell col of legacy device
	
	int                 hfs_devices_available;		// true if devices are available

	short				hfs_device_id   [INTERCHANGE_ID_TABLE_SIZE];		// holds alis resource id of alias identifying added devices
	unsigned char		hfs_device_row  [INTERCHANGE_ID_TABLE_SIZE];		// holds row of header table to display in
	unsigned char		hfs_device_col  [INTERCHANGE_ID_TABLE_SIZE];		// holds col of header table to display in
	unsigned char		hfs_device_width[INTERCHANGE_ID_TABLE_SIZE];		// holds width of area for this name in header table
	unsigned char		hfs_device_type [INTERCHANGE_ID_TABLE_SIZE];		// holds coded type of file (SUBC, fldr, disk, etc.)

	int                 default_image_size;			// default size of created image file
	int                 default_image_style;		// default style of created image file (disk, optical)
	int                 default_image_fill;			// default fill of created image file (no fill, zero fill)
	int                 default_contig;				// default require contiguous storage allocation

	int                 optical_sort;

	int                 show_show_ext;				// True to show filename extension
	
	int                 up_time;					// total unregistered uptime
	int                 registered_bits;			// indicates bits we have registered for
	int                 registered_key;				// nonzero means registered OK

	int                 subcat_size;				// default make subcatalog size
	
	int                 spare[694];

}	interchange2_settings;

// Registration codes
typedef enum
{
	registration_interchange_2_0

} registration_id;

// Struct for software registration entries
typedef struct registration_entry
{
	int		is_registered;                          // true if this module is registered
	int		file_id;                                // id of file when registered
	int		date_first_used;                        // datetime of first use (if unregistered)

} registration_entry;

// Struct for registration file
#define	MAX_REGISTRATION_ID	100

typedef	struct registration_data
{
	UInt32              its_file_id;
	registration_entry	entries[MAX_REGISTRATION_ID];
	
} registration_data;

#pragma pack(pop)

// Public interface

// These routines are provided so 32-bit builds can take advantage of CSynclavierFileReference.
// 64-bit builds would access them directly.
class   CSynclavierFileReference*   locate_interchange_prefs_file(CFStringRef aPrefNameRef);
class   CSynclavierFileReference*   locate_interchange_prefs_file(char* prefs_name);
OSErr                               check_interchange_prefs_file_rev (class CSynclavierFileReference* aFileReference);
OSErr                               check_interchange2_prefs_file_rev(class CSynclavierFileReference* aFileReference);
OSErr                               check_interpreter_prefs_file_rev (class CSynclavierFileReference* aFileReference);

#if !__LP64__
    // These routines provide temporary glue to original FSSpec-based routines.
    OSErr 	locate_interchange_prefs_file		(char *prefs_name, FSSpec *the_spec);
    OSErr 	check_interchange_prefs_file_rev	(FSSpec *the_spec);
    OSErr 	check_interchange2_prefs_file_rev	(FSSpec *the_spec);
    OSErr 	check_interpreter_prefs_file_rev	(FSSpec *the_spec);
#endif

void 	read_interchange_setup				(char *prefs_name, interchange_settings *the_settings, int resolve_all_alii);
int 	check_interchange_image_files		(interchange_settings *the_settings);
int 	check_interchange_selected_files	(interchange_settings *the_settings);
void 	toss_interchange_setup				(interchange_settings *the_settings);
void 	release_interchange_setup			(interchange_settings *the_settings);
void 	write_interchange_setup				(char *prefs_name, interchange_settings *the_settings);
void 	write_interchange_basic_settings	(char *prefs_name, interchange_settings *the_settings);
void 	write_interchange_1_0_aliai			(char *prefs_name, interchange_settings *the_settings);
void    replace_interchange_1_0_alias		(char *prefs_name, short the_id, xpl_file_alias the_alias);

void 	read_interchange2_setup				(char *prefs_name, interchange2_settings *the_settings);
void 	write_interchange2_setup			(char *prefs_name, interchange2_settings *the_settings);
void 	store_interchange2_device_alias		(char *prefs_name, interchange2_settings *the_settings, short resID, xpl_file_alias theAlias);
void 	remove_interchange2_device_alias	(char *prefs_name, short resID);
short 	open_interchange2_res_file			(char *prefs_name);

void 	read_interpreter_setup				(char *prefs_name, interpreter_settings *the_settings, interpreter_midi_patching* the_patching);
void 	write_interpreter_setup				(char *prefs_name, interpreter_settings *the_settings, interpreter_midi_patching* the_patching);

int     verify_registration					(int which_module);
ulong	compute_serial_for_module 			(int which_module);
int     set_registration					(int which_module, ulong key_code);
ulong	compute_key_for_serial 				(int which_module, ulong serial  );

extern	char *interchange_pref_file_name;			// default file name for interchange 1.0 prefs
extern	char *interchange2_pref_file_name;			// default file name for interchange 2.0 prefs
extern	char *interpreter_pref_file_name;			// default file name for interpreter prefs

//InterChangeLib
extern	int             InterChangeLibGetIndexedReaddataCode(int index);
extern  scsi_settings*  InterChangeLibGetSettingForCode     (const interchange_settings *the_settings, int code );
extern  int  			InterChangeLibGetSCSIIDForCode 		(int code );
extern  int  			InterChangeLibGetResourceIDForCode 	(int code );
extern	int             InterChangeLibEqualFSSpecs			(SyncFSSpec *inSpec1, SyncFSSpec *inSpec2);

#endif
