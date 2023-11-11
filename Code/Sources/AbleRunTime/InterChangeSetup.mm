/* InterChangeSetup.c */

// Prefs utilities: Read & write prefs file

//	Std C Includes
#include <stdio.h>
#include <string.h>

//	Local Includes
#include "Standard.h"
#include "XPL.h"

#include "InterChange.h"
#include "ScsiLib.h"
#include "Version.h"
#include "SynclavierFileReference.h"

#include "ResourceUtilities.h"
#include "AliasUtilities.h"
#include "AbleDiskApMenus.h"
#include "AbleDiskApPopUps.h"
#include "InterpreterSetupDialog.h"
#include "DialogUtilities.h"

#import "SynclavierPreferenceManager.h"
#import "Synclavier3Constants.h"

// Global Variables

char   *interchange_pref_file_name   = {(char *) ""};
char   *interchange2_pref_file_name  = {(char *) ""};
char   *interpreter_pref_file_name   = {(char *) ""};
char   *registration_pref_file_name  = {(char *) ""};

static	sig_struct	SIGNATURE_INFO   = {INTERCHANGE_CURRENT_SIG_ID, INTERCHANGE_CURRENT_SIG_REV};


// -----------------------------------------------------
// locate_interchange_prefs_file
// -----------------------------------------------------

CSynclavierFileReference*   locate_interchange_prefs_file(CFStringRef aPrefNameRef)
{
    return NULL;
}

CSynclavierFileReference*   locate_interchange_prefs_file(char* prefs_name) {
    return NULL;
}


// -----------------------------------------------------
// check_interchange_prefs_file_rev
// -----------------------------------------------------

OSErr 	check_interchange_prefs_file_rev (CSynclavierFileReference* aFileReference)
{
    return noErr;
}


// -----------------------------------------------------
// check_interchange2_prefs_file_rev
// -----------------------------------------------------

OSErr check_interchange2_prefs_file_rev(CSynclavierFileReference* aFileReference)
{
    return noErr;
}


// -----------------------------------------------------
// check_interpreter_prefs_file_rev
// -----------------------------------------------------

OSErr check_interpreter_prefs_file_rev(CSynclavierFileReference* aFileReference)
{
    return noErr;
}


// -----------------------------------------------------
// read_interchange_setup:  Reads in settings from prefs file.  Initializes settings
// to default values if no file exists or can't read it.
// -----------------------------------------------------

void read_interchange_setup(char *prefs_name, interchange_settings *the_settings, int resolve_all_alii, int& t0_setting, scsi_settings& t0_settings)
{
    // Init for possible failure
    memset(the_settings, 0, sizeof(*the_settings));
    
    // Create filerefs for app bundle and resources folder, and our dedicated W0 image file
    CSynclavierFileReference* appFileRef = CSynclavierFileReference::CopyAppFileRef     ();
    CSynclavierFileReference* resFileRef = CSynclavierFileReference::CopyAppResourcesRef();
    CSynclavierFileReference* w0FileRef  = NULL;
   
    if (!appFileRef || !appFileRef->GetURL() || !appFileRef->GetParent())
        return;
    
	// Look up local W0 disk image file in all cases
    CFURLRef    w0URL        = NULL;
	OSErr       w0_avail_err = fnfErr;
    
    // For carbon builds, the W0 image file was in the same folder as the app.
    // For cocoa builds the W0 image is hidden in the resources folder
    w0URL = resFileRef->CreateChild(CFSTR("W0 Disk Image.simg"), false);                            // Create a file URL
    
    if (w0URL) {
        w0FileRef = new CSynclavierFileReference(w0URL); // Ownership passes to FileRef
        
        if (w0FileRef) {
            w0FileRef->Resolve();
            
            if (w0FileRef->Reachable())
                w0_avail_err = noErr;
            
            // Get rid if the file ref if the file is not reachable
            if (w0_avail_err != noErr) {
                w0FileRef->Release();
                w0FileRef = NULL;
            }
        }
        
        else
            CFRelease(w0URL);
        
        w0URL = NULL;
    }
    
    appFileRef->Release();
    appFileRef = NULL;
    
    resFileRef->Release();
    resFileRef = NULL;
    
    // Start with default settings
    the_settings->w0.bus_id    = SCSI_BUS_MENU_NONE;
    the_settings->w0.device_id = SCSI_ID_MENU_ID_5;
    the_settings->w0.scsi_id   = ABLE_W0_DEFAULT_SCSI_ID;
    the_settings->w0.sim_id    = ABLE_W0_DEFAULT_SCSI_ID;
    
    the_settings->w1.bus_id    = SCSI_BUS_MENU_NONE;
    the_settings->w1.device_id = SCSI_ID_MENU_ID_4;
    the_settings->w1.scsi_id   = ABLE_W1_DEFAULT_SCSI_ID;
    the_settings->w1.sim_id    = ABLE_W1_DEFAULT_SCSI_ID;

    the_settings->o0.bus_id    = SCSI_BUS_MENU_NONE;
    the_settings->o0.device_id = SCSI_ID_MENU_ID_1;
    the_settings->o0.scsi_id   = ABLE_O0_DEFAULT_SCSI_ID;
    the_settings->o0.sim_id    = ABLE_O0_DEFAULT_SCSI_ID;

    the_settings->o1.bus_id    = SCSI_BUS_MENU_NONE;
    the_settings->o1.device_id = SCSI_ID_MENU_ID_2;
    the_settings->o1.scsi_id   = ABLE_O1_DEFAULT_SCSI_ID;
    the_settings->o1.sim_id    = ABLE_O1_DEFAULT_SCSI_ID;
    
    t0_setting                 = SCSI_BUS_MENU_NONE;
    t0_settings.bus_id         = SCSI_BUS_MENU_NONE;
    t0_settings.device_id      = SCSI_ID_MENU_ID_0;
    t0_settings.scsi_id        = ABLE_T0_DEFAULT_SCSI_ID;
    t0_settings.sim_id         = ABLE_T0_DEFAULT_SCSI_ID;
    
    // Done if can't find default W0
    if (!w0FileRef)
        return;
        
    // Set up W0
    int w0_type = [SynclavierPreferenceManager integerForKey:SYNC_PREF_W0_DEVICE_TYPE];
    
    // Look for custom W0 disk image file
    if (w0_type == SCSI_BUS_MENU_DISK_IMAGE) {
        id        w0_book     = [SynclavierPreferenceManager objectForKey: SYNC_PREF_W0_FILE_BOOKMARK];
        NSData*   w0_bookData = DYNAMIC_CAST(w0_book, NSData);
        
        if (w0_bookData) {
            CFDataRef                 w0_bookmark = (__bridge_retained CFDataRef) w0_bookData;
            CSynclavierFileReference* w0_ref      = new CSynclavierFileReference(w0_bookmark); // Bookmark ownership passes to w0_ref
            
            if (w0_ref) {
                w0_ref->Resolve();
                
                // Use custom image file
                if (w0_ref->Reachable()) {
                    w0FileRef->Release();
                    w0FileRef = NULL;
                    
                    w0FileRef = w0_ref;
                }

                // Else is not available
                else {
                    DU_ReportErr("Your Custom W0 Disk Image File is not available at this time.");

                    w0_ref->Release();
                    w0_ref = NULL;
                }
            }
        }
    }

    the_settings->w0.bus_id      = SCSI_BUS_MENU_DISK_IMAGE;
    the_settings->w0.device_id   = SCSI_ID_MENU_ID_5;
    the_settings->w0.scsi_id     = ABLE_W0_DEFAULT_SCSI_ID;
    the_settings->w0.sim_id      = ABLE_W0_DEFAULT_SCSI_ID;
    the_settings->w0.disk_type   = 0;
    the_settings->w0.image_file  = w0FileRef->CopyBookmark();
    
    w0FileRef->CreateFSSpec(&the_settings->w0.image_spec);
    w0FileRef->Path(the_settings->w0.image_pathname, sizeof(the_settings->w0.image_pathname));
    
    // Look for W1 setting
    int w1_type = [SynclavierPreferenceManager integerForKey:SYNC_PREF_W1_DEVICE_TYPE];

    if (w1_type == SCSI_BUS_MENU_D24) {
        int w1_id = [SynclavierPreferenceManager integerForKey:SYNC_PREF_W1_SCSI_ID];
        
        if (w1_id == ABLE_W0_DEFAULT_SCSI_ID || w1_id == ABLE_W1_DEFAULT_SCSI_ID) {
            the_settings->w1.bus_id      = SCSI_BUS_MENU_D24;
            the_settings->w1.device_id   = SCSI_ID_MENU_ID_5;
            the_settings->w1.scsi_id     = w1_id;
            the_settings->w1.sim_id      = ABLE_W1_DEFAULT_SCSI_ID;
            the_settings->w1.disk_type   = 0;
            the_settings->w1.image_file  = NULL;
        }
    }
    
    else if (w1_type == SCSI_BUS_MENU_DISK_IMAGE) {
        NSData*   w1_bookData = DYNAMIC_CAST([SynclavierPreferenceManager objectForKey:SYNC_PREF_W1_FILE_BOOKMARK], NSData);

        if (w1_bookData) {
            CFDataRef                 w1_bookmark = (__bridge_retained CFDataRef) w1_bookData;
            CSynclavierFileReference* w1_ref      = new CSynclavierFileReference(w1_bookmark); // Bookmark ownership passes to w1_ref
            
            if (w1_ref) {
                w1_ref->Resolve();
                
                if (w1_ref->Reachable()) {
                    the_settings->w1.bus_id      = SCSI_BUS_MENU_DISK_IMAGE;
                    the_settings->w1.device_id   = SCSI_ID_MENU_ID_4;
                    the_settings->w1.scsi_id     = ABLE_W1_DEFAULT_SCSI_ID;
                    the_settings->w1.sim_id      = ABLE_W1_DEFAULT_SCSI_ID;
                    the_settings->w1.disk_type   = 0;
                    the_settings->w1.image_file  = w1_ref->CopyBookmark();;

                    w1_ref->CreateFSSpec(&the_settings->w1.image_spec);
                    w1_ref->Path(the_settings->w1.image_pathname, sizeof(the_settings->w1.image_pathname));
                }
                
                w1_ref->Release();
                w1_ref = NULL;
            }
        }
    }
    
    // Look for O0 setting
    int o0_type = [SynclavierPreferenceManager integerForKey:SYNC_PREF_O0_DEVICE_TYPE];
    
    if (o0_type == SCSI_BUS_MENU_D24) {
        int o0_id = [SynclavierPreferenceManager integerForKey:SYNC_PREF_O0_SCSI_ID];
        
        if (o0_id == ABLE_O0_DEFAULT_SCSI_ID || o0_id == ABLE_O1_DEFAULT_SCSI_ID) {
            the_settings->o0.bus_id      = SCSI_BUS_MENU_D24;
            the_settings->o0.device_id   = SCSI_ID_MENU_ID_5;
            the_settings->o0.scsi_id     = o0_id;
            the_settings->o0.sim_id      = ABLE_O0_DEFAULT_SCSI_ID;
            the_settings->o0.disk_type   = 0;
            the_settings->o0.image_file  = NULL;
        }
    }
    
    else if (o0_type == SCSI_BUS_MENU_DISK_IMAGE) {
        NSData*   o0_bookData = DYNAMIC_CAST([SynclavierPreferenceManager objectForKey:SYNC_PREF_O0_FILE_BOOKMARK], NSData);
        
        if (o0_bookData) {
            CFDataRef                 o0_bookmark = (__bridge_retained CFDataRef) o0_bookData;
            CSynclavierFileReference* o0_ref      = new CSynclavierFileReference(o0_bookmark); // Bookmark ownership passes to o0_ref
            
            if (o0_ref) {
                o0_ref->Resolve();
                
                if (o0_ref->Reachable()) {
                    the_settings->o0.bus_id      = SCSI_BUS_MENU_DISK_IMAGE;
                    the_settings->o0.device_id   = SCSI_ID_MENU_ID_4;
                    the_settings->o0.scsi_id     = ABLE_O0_DEFAULT_SCSI_ID;
                    the_settings->o0.sim_id      = ABLE_O0_DEFAULT_SCSI_ID;
                    the_settings->o0.disk_type   = 0;
                    the_settings->o0.image_file  = o0_ref->CopyBookmark();;
                    
                    o0_ref->CreateFSSpec(&the_settings->o0.image_spec);
                    o0_ref->Path(the_settings->o0.image_pathname, sizeof(the_settings->o0.image_pathname));
                }
                
                o0_ref->Release();
                o0_ref = NULL;
            }
        }
    }
    
    // Look for O1 setting
    int o1_type = [SynclavierPreferenceManager integerForKey:SYNC_PREF_O1_DEVICE_TYPE];
    
    if (o1_type == SCSI_BUS_MENU_D24) {
        int o1_id = [SynclavierPreferenceManager integerForKey:SYNC_PREF_O1_SCSI_ID];
        
        if (o1_id == ABLE_O0_DEFAULT_SCSI_ID || o1_id == ABLE_O1_DEFAULT_SCSI_ID) {
            the_settings->o1.bus_id      = SCSI_BUS_MENU_D24;
            the_settings->o1.device_id   = SCSI_ID_MENU_ID_5;
            the_settings->o1.scsi_id     = o1_id;
            the_settings->o1.sim_id      = ABLE_O1_DEFAULT_SCSI_ID;
            the_settings->o1.disk_type   = 0;
            the_settings->o1.image_file  = NULL;
        }
    }
    
    else if (o1_type == SCSI_BUS_MENU_DISK_IMAGE) {
        NSData*   o1_bookData = DYNAMIC_CAST([SynclavierPreferenceManager objectForKey:SYNC_PREF_O1_FILE_BOOKMARK], NSData);
        
        if (o1_bookData) {
            CFDataRef                 o1_bookmark = (__bridge_retained CFDataRef) o1_bookData;
            CSynclavierFileReference* o1_ref      = new CSynclavierFileReference(o1_bookmark); // Bookmark ownership passes to o1_ref
            
            if (o1_ref) {
                o1_ref->Resolve();
                
                if (o1_ref->Reachable()) {
                    the_settings->o1.bus_id      = SCSI_BUS_MENU_DISK_IMAGE;
                    the_settings->o1.device_id   = SCSI_ID_MENU_ID_4;
                    the_settings->o1.scsi_id     = ABLE_O1_DEFAULT_SCSI_ID;
                    the_settings->o1.sim_id      = ABLE_O1_DEFAULT_SCSI_ID;
                    the_settings->o1.disk_type   = 0;
                    the_settings->o1.image_file  = o1_ref->CopyBookmark();;
                    
                    o1_ref->CreateFSSpec(&the_settings->o1.image_spec);
                    o1_ref->Path(the_settings->o1.image_pathname, sizeof(the_settings->o1.image_pathname));
                }
                
                o1_ref->Release();
                o1_ref = NULL;
            }
        }
    }
    
    // Done with the W0 file ref; although it probably has been used & retained...
    if (w0FileRef) {
        w0FileRef->Release();
        w0FileRef = NULL;
    }
    
    // Grab T0 Pref
    int t0_type = [SynclavierPreferenceManager integerForKey:SYNC_PREF_T0_DEVICE_TYPE];
    
    if (t0_type == SCSI_BUS_MENU_D24) {
        t0_setting             = SCSI_BUS_MENU_D24;
        t0_settings.bus_id     = SCSI_BUS_MENU_D24;
        t0_settings.device_id  = SCSI_ID_MENU_ID_0;
        t0_settings.scsi_id    = ABLE_T0_DEFAULT_SCSI_ID;
        t0_settings.sim_id     = ABLE_T0_DEFAULT_SCSI_ID;
        t0_settings.disk_type  = 0;
        t0_settings.image_file = NULL;
    }
    
    else if (t0_type == SCSI_BUS_MENU_D30)
        t0_setting = SCSI_BUS_MENU_D30;
}


// -----------------------------------------------------
// Check setup: look for possibly moved files upon switch from background
// -----------------------------------------------------

int check_interchange_image_files(interchange_settings *the_settings)
{
	int status = 0;
	
	return (status);
}

int check_interchange_selected_files(interchange_settings *the_settings)
{
	int status = 0;
	
	return (status);
}


// -----------------------------------------------------
// Toss config: just toss stuff
// -----------------------------------------------------

void toss_interchange_setup(interchange_settings *the_settings)
{	
	if (the_settings->w0.image_file)	CFRelease(the_settings->w0.image_file);
	if (the_settings->w1.image_file)	CFRelease(the_settings->w1.image_file);
	if (the_settings->o0.image_file)	CFRelease(the_settings->o0.image_file);
	if (the_settings->o1.image_file)	CFRelease(the_settings->o1.image_file);
	if (the_settings->export_fldr)		CFRelease(the_settings->export_fldr  );
	if (the_settings->export_imge)		CFRelease(the_settings->export_imge  );
	if (the_settings->export_file)		CFRelease(the_settings->export_file  );
	if (the_settings->export_sysf)		CFRelease(the_settings->export_sysf  );
	if (the_settings->export_sysi)		CFRelease(the_settings->export_sysi  );
	if (the_settings->import_fldr)		CFRelease(the_settings->import_fldr  );
	if (the_settings->info       )		CFRelease(the_settings->info         );

    the_settings->w0.image_file = NULL;
	the_settings->w1.image_file = NULL;
	the_settings->o0.image_file = NULL;
	the_settings->o1.image_file = NULL;
	the_settings->export_fldr   = NULL;
	the_settings->export_imge   = NULL;
	the_settings->export_file   = NULL;
	the_settings->export_sysf   = NULL;
	the_settings->export_sysi   = NULL;
	the_settings->import_fldr   = NULL;
	the_settings->info          = NULL;
}


// -----------------------------------------------------
// Release config: release file refs; mostly applicable for 64-bit builds
// -----------------------------------------------------

void 	release_interchange_setup(interchange_settings *the_settings)
{
    toss_interchange_setup(the_settings);           // Get rid of any dangling bookmarks
    
    // And release the file references
    SyncFSSpecRelease(&the_settings->w0.image_spec);
    SyncFSSpecRelease(&the_settings->w1.image_spec);
    SyncFSSpecRelease(&the_settings->o0.image_spec);
    SyncFSSpecRelease(&the_settings->o1.image_spec);
    
	SyncFSSpecRelease(&the_settings->export_fldrspec);
	SyncFSSpecRelease(&the_settings->export_imgespec);
	SyncFSSpecRelease(&the_settings->export_filespec);
	SyncFSSpecRelease(&the_settings->export_sysfspec);
	SyncFSSpecRelease(&the_settings->export_sysispec);
	SyncFSSpecRelease(&the_settings->import_fldrspec);
}


// -----------------------------------------------------
// write_interchange_basic_settings: writes basic settings
// to memory.  Aliai are not written
// -----------------------------------------------------

void write_interchange_basic_settings(char *prefs_name, interchange_settings *the_settings)
{
    if (the_settings->w1.bus_id == SCSI_BUS_MENU_D24) {
        [SynclavierPreferenceManager setIntegerForKey:SYNC_PREF_W1_DEVICE_TYPE value:SCSI_BUS_MENU_D24       ];
        [SynclavierPreferenceManager setIntegerForKey:SYNC_PREF_W1_SCSI_ID     value:the_settings->w1.scsi_id];
        // Leave bookmark in place so user can switch back to it with ease
    }
    
    else if (the_settings->w1.bus_id == SCSI_BUS_MENU_DISK_IMAGE && the_settings->w1.image_file) {
        [SynclavierPreferenceManager setIntegerForKey:SYNC_PREF_W1_DEVICE_TYPE   value:SCSI_BUS_MENU_DISK_IMAGE                      ];
        [SynclavierPreferenceManager setObjectForKey: SYNC_PREF_W1_FILE_BOOKMARK value:(__bridge NSData*) the_settings->w1.image_file];
    }
    
    else
        [SynclavierPreferenceManager setIntegerForKey:SYNC_PREF_W1_DEVICE_TYPE value:SCSI_BUS_MENU_NONE];

    if (the_settings->o0.bus_id == SCSI_BUS_MENU_D24) {
        [SynclavierPreferenceManager setIntegerForKey:SYNC_PREF_O0_DEVICE_TYPE value:SCSI_BUS_MENU_D24       ];
        [SynclavierPreferenceManager setIntegerForKey:SYNC_PREF_O0_SCSI_ID     value:the_settings->o0.scsi_id];
        // Leave bookmark in place so user can switch back to it with ease
    }
    
    else if (the_settings->o0.bus_id == SCSI_BUS_MENU_DISK_IMAGE && the_settings->o0.image_file) {
        [SynclavierPreferenceManager setIntegerForKey:SYNC_PREF_O0_DEVICE_TYPE   value:SCSI_BUS_MENU_DISK_IMAGE                      ];
        [SynclavierPreferenceManager setObjectForKey: SYNC_PREF_O0_FILE_BOOKMARK value:(__bridge NSData*) the_settings->o0.image_file];
    }
    
    else
        [SynclavierPreferenceManager setIntegerForKey:SYNC_PREF_O0_DEVICE_TYPE value:SCSI_BUS_MENU_NONE];

    if (the_settings->o1.bus_id == SCSI_BUS_MENU_D24) {
        [SynclavierPreferenceManager setIntegerForKey:SYNC_PREF_O1_DEVICE_TYPE value:SCSI_BUS_MENU_D24       ];
        [SynclavierPreferenceManager setIntegerForKey:SYNC_PREF_O1_SCSI_ID     value:the_settings->o1.scsi_id];
        // Leave bookmark in place so user can switch back to it with ease
    }
    
    else if (the_settings->o1.bus_id == SCSI_BUS_MENU_DISK_IMAGE && the_settings->o1.image_file) {
        [SynclavierPreferenceManager setIntegerForKey:SYNC_PREF_O1_DEVICE_TYPE   value:SCSI_BUS_MENU_DISK_IMAGE                      ];
        [SynclavierPreferenceManager setObjectForKey: SYNC_PREF_O1_FILE_BOOKMARK value:(__bridge NSData*) the_settings->o1.image_file];
    }
    
    else
        [SynclavierPreferenceManager setIntegerForKey:SYNC_PREF_O1_DEVICE_TYPE value:SCSI_BUS_MENU_NONE];
}


// -----------------------------------------------------
// write_interchange_1_0_aliai
// -----------------------------------------------------

// write aliai associated with interchange 1.0 settings only
void write_interchange_1_0_aliai(char *prefs_name, interchange_settings *the_settings)
{	
}


// -----------------------------------------------------
// replace_interchange_1_0_alias
// -----------------------------------------------------

// write one alias associated with interchange 1.0 settings only
void replace_interchange_1_0_alias(char *prefs_name, short the_id, xpl_file_alias the_alias)
{	
}


// -----------------------------------------------------
// write_interchange_setup: writes settings to prefs file, thereby freeing up
// allocated memory.
// -----------------------------------------------------

void write_interchange_setup(char *prefs_name, interchange_settings *the_settings)
{	
	write_interchange_basic_settings(prefs_name, the_settings);
}


// -----------------------------------------------------
// read_interchange2_setup:  Reads in settings from prefs file.  Initializes settings
// to default values if no file exists or can't read it.
// -----------------------------------------------------

void read_interchange2_setup(char *prefs_name, interchange2_settings *the_settings)
{
    RGBColor color_black = {0, 0, 0};
    
    memset(the_settings, 0, sizeof(*the_settings));

    the_settings->sound_color    = color_black;
    the_settings->sequence_color = color_black;
    the_settings->timbre_color   = color_black;
    the_settings->other_color    = color_black;
	
	// Initialize new settings
	if (the_settings->information  == 0) the_settings->information  = 1;
	if (the_settings->optical_sort == 0) the_settings->optical_sort = 1;
}


// -----------------------------------------------------
// write_interchange2_setup
// -----------------------------------------------------

void write_interchange2_setup(char *prefs_name, interchange2_settings *the_settings)
{	
}


// -----------------------------------------------------
// store_interchange2_device_alias
// -----------------------------------------------------

void store_interchange2_device_alias(char *prefs_name, interchange2_settings *the_settings, short resID, xpl_file_alias theAlias)
{
}


// -----------------------------------------------------
// remove_interchange2_device_alias
// -----------------------------------------------------

void remove_interchange2_device_alias(char *prefs_name, short resID)
{
}


// -----------------------------------------------------
// open_interchange2_res_file
// -----------------------------------------------------

short open_interchange2_res_file(char *prefs_name)
{
	short   refNum = 0;
		
	return (refNum);
}


// -----------------------------------------------------
// read_interpreter_setup:  Reads in settings from prefs file.  Initializes settings
// to default values if no file exists or can't read it.
// -----------------------------------------------------

void read_interpreter_setup(char *prefs_name, interpreter_settings *the_settings, interpreter_midi_patching *the_patching)
{
    memset(the_settings, 0, sizeof(*the_settings));
    
    if (the_patching)
        memset(the_patching, 0, sizeof(*the_patching));
    
    the_settings->cable_setting = CABLE_LENGTH_MENU_50;				// 50 foot
    the_settings->bus_setting   = BUS_LOADING_MENU_MEDIUM;			// medium
    the_settings->net_visible   = NET_VISIBLE_MENU_NOT_VISIBLE;		// not visible
    the_settings->use_mp        = USE_MP_MENU_USE_MP;				// allow use of MP if available
    the_settings->m512k			= M512K_MENU_MODEL_10;				// provide 10 M512k's
    
    the_settings->polymem		= [SynclavierPreferenceManager integerForKey:SYNC_PREF_REAL_TIME_POLY_VALUE];
    
    // Default is 100
    if (the_settings->polymem == 0)
        the_settings->polymem = 100;

    // Grab the MIDI routings
    if (the_patching) {
        the_patching->midi_sig = SIGNATURE_INFO;
        
        for (int i=0; i<NUM_NEW_MIDI_PATCHINGS; i++) {
            NSString* to_key   = [NSString stringWithFormat:@"%@%d", SYNC_PREF_MIDI_TO  , i];
            NSString* from_key = [NSString stringWithFormat:@"%@%d", SYNC_PREF_MIDI_FROM, i];
            
            NSString* to_val   = [SynclavierPreferenceManager stringForKey:to_key  ];
            NSString* from_val = [SynclavierPreferenceManager stringForKey:from_key];
            
            if (to_val)
                [to_val   getCString:&the_patching->midi_to_synclavierx  [i][0] maxLength:MIDI_PATCHING_STRING_SIZE encoding:NSUTF8StringEncoding];
            
            if (from_val)
                [from_val getCString:&the_patching->midi_from_synclavierx[i][0] maxLength:MIDI_PATCHING_STRING_SIZE encoding:NSUTF8StringEncoding];
        }
    }
}

// -----------------------------------------------------
// Write config: writes settings to prefs file, thereby freeing up
// allocated memory.
// -----------------------------------------------------

void write_interpreter_setup(char *prefs_name, interpreter_settings *the_settings, interpreter_midi_patching* the_patching)
{	
    // Write the MIDI routings
    if (the_patching) {
        for (int i=0; i<NUM_NEW_MIDI_PATCHINGS; i++) {
            NSString* to_key   = [NSString stringWithFormat:@"%@%d", SYNC_PREF_MIDI_TO  , i];
            NSString* from_key = [NSString stringWithFormat:@"%@%d", SYNC_PREF_MIDI_FROM, i];
            
            if (the_patching->midi_to_synclavierx[i][0])
                [SynclavierPreferenceManager setStringForKey:to_key value:[NSString stringWithUTF8String:&the_patching->midi_to_synclavierx[i][0]]];
            else
                [SynclavierPreferenceManager removeKey:to_key];
            
            if (the_patching->midi_from_synclavierx[i][0])
                [SynclavierPreferenceManager setStringForKey:from_key value:[NSString stringWithUTF8String:&the_patching->midi_from_synclavierx[i][0]]];
            else
                [SynclavierPreferenceManager removeKey:from_key];
        }
    }
}


// -----------------------------------------------------
// verify_registration:  see if a module is registered
// -----------------------------------------------------

int verify_registration (int which_module)
{
	return (true);										// registered
}


// -----------------------------------------------------
// compute_serial_for_module:  compute serial number for this module
// -----------------------------------------------------

ulong compute_serial_for_module (int which_module)
{
	return (0);
}


// -----------------------------------------------------
// compute_key_for_serial:  compute key number a module and serial
// -----------------------------------------------------

ulong compute_key_for_serial (int which_module, ulong serial)
{
    return (0);
}


// -----------------------------------------------------
// set_registration:  attempt registration for a module
// -----------------------------------------------------

int set_registration (int which_module, ulong key_code)
{
	OSErr				err = noErr;
	
	return (err);
}


// ---------------------------------------------------------------------
// ¥ InterChangeLib
// ---------------------------------------------------------------------
static	int codeLookup[4] = {ABLE_W0_READDATA_CODE,   ABLE_W1_READDATA_CODE,   ABLE_O0_READDATA_CODE,   ABLE_O1_READDATA_CODE  };
static	int idLookup  [4] = {ABLE_W0_DEFAULT_SCSI_ID, ABLE_W1_DEFAULT_SCSI_ID, ABLE_O0_DEFAULT_SCSI_ID, ABLE_O1_DEFAULT_SCSI_ID};

#pragma unused(idLookup)

int	InterChangeLibGetIndexedReaddataCode(int index)				// Look up readdata code from index; used by iterators
{
	return (codeLookup[index]);
}

scsi_settings*  InterChangeLibGetSettingForCode(const interchange_settings *the_settings, int code)
{
	if (code == ABLE_W0_READDATA_CODE)
		return ((scsi_settings*) &the_settings->w0);

	if (code == ABLE_W1_READDATA_CODE)
		return ((scsi_settings*) &the_settings->w1);

	if (code == ABLE_O0_READDATA_CODE)
		return ((scsi_settings*) &the_settings->o0);
	
	if (code == ABLE_O1_READDATA_CODE)
		return ((scsi_settings*) &the_settings->o1);
	
	return (NULL);
}

int	InterChangeLibGetSCSIIDForCode(int code )
{
	if (code == ABLE_W0_READDATA_CODE)
		return (ABLE_W0_DEFAULT_SCSI_ID);

	if (code == ABLE_W1_READDATA_CODE)
		return (ABLE_W1_DEFAULT_SCSI_ID);

	if (code == ABLE_O0_READDATA_CODE)
		return (ABLE_O0_DEFAULT_SCSI_ID);
	
	if (code == ABLE_O1_READDATA_CODE)
		return (ABLE_O1_DEFAULT_SCSI_ID);
	
	return (0);
}

int	InterChangeLibGetResourceIDForCode(int code )
{
	if (code == ABLE_W0_READDATA_CODE)
		return (W0_IMAGE_FILE_ALIS_RESOURCE_ID);

	if (code == ABLE_W1_READDATA_CODE)
		return (W1_IMAGE_FILE_ALIS_RESOURCE_ID);

	if (code == ABLE_O0_READDATA_CODE)
		return (O0_IMAGE_FILE_ALIS_RESOURCE_ID);
	
	if (code == ABLE_O1_READDATA_CODE)
		return (O1_IMAGE_FILE_ALIS_RESOURCE_ID);
	
	return (0);
}

int	InterChangeLibEqualFSSpecs(SyncFSSpec *inSpec1, SyncFSSpec *inSpec2)
{
	if (!inSpec1 || !inSpec2)
		return (FALSE);
    
    #if __LP64__
        if (!inSpec1->file_ref || !inSpec2->file_ref)
            return (FALSE);
    
        if (inSpec1->file_ref == inSpec2->file_ref)
            return TRUE;
    
        // Compare using absolute posix paths
        CFStringRef spec1Path = inSpec1->file_ref->GetPath();
        CFStringRef spec2Path = inSpec2->file_ref->GetPath();
 
        if (!spec1Path || !spec2Path)
            return false;
        
        return (int) (CFStringCompare(spec1Path, spec2Path, 0) == kCFCompareEqualTo);
    #else
        int j;
	
        if (inSpec1->name[0] == 0 || inSpec2->name[0] == 0)
            return (FALSE);
        
        if (inSpec1->vRefNum != inSpec2->vRefNum)
            return (FALSE);
            
        if (inSpec1->parID   != inSpec2->parID)
            return (FALSE);
            
        if (inSpec1->name[0] != inSpec2->name[0])
            return (FALSE);
            
        for (j=1; j<=inSpec1->name[0]; j++)
        {
            if (inSpec1->name[j] != inSpec2->name[j])
            return (FALSE);
        }
    #endif

	return (TRUE);
}
