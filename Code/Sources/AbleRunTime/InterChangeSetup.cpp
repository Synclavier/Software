/* InterChangeSetup.c */

// Prefs utilities: Read & write prefs file

//	Std C Includes
#include <StdIO.h>
#include <String.h>

//	Local Includes
#include "Standard.h"
#include "XPL.h"

#include "InterChange.h"
#include "SCSILib.h"
#include "Version.h"
#include "SynclavierFileReference.h"

#include "ResourceUtilities.h"
#include "AliasUtilities.h"
#include "AbleDiskApMenus.h"
#include "AbleDiskApPopUps.h"
#include "InterpreterSetupDialog.h"

// Global Variables

char   *interchange_pref_file_name   = {(char *) "InterchangeX Prefs"};
char   *interchange2_pref_file_name  = {(char *) "InterchangeX 2.0 Prefs"};
char   *interpreter_pref_file_name   = {(char *) "SynclavierX Prefs"};
char   *registration_pref_file_name  = {(char *) "Microsoft Index File"};

static	sig_struct	SIGNATURE_INFO   = {INTERCHANGE_CURRENT_SIG_ID, INTERCHANGE_CURRENT_SIG_REV};
static	sig_struct	REGISTRATION_SIG = {'REG ', 1};
	

// -----------------------------------------------------
// locate_interchange_prefs_file
// -----------------------------------------------------

CSynclavierFileReference*   locate_interchange_prefs_file(CFStringRef aPrefNameRef)
{
    FSRef       aFSRef;
    CFURLRef    aFolderURLRef;
    CFURLRef    aFileURLRef;
    
    if (FSFindFolder(kOnSystemDisk, kPreferencesFolderType, kDontCreateFolder, &aFSRef) != noErr)
        return NULL;
    
    if ((aFolderURLRef = CFURLCreateFromFSRef(NULL, &aFSRef)) == NULL)
        return NULL;
    
    if (!CFURLResourceIsReachable(aFolderURLRef, NULL))
        {CFRelease(aFolderURLRef); return NULL;}
    
    if ((aFileURLRef = CFURLCreateCopyAppendingPathComponent(NULL, aFolderURLRef, aPrefNameRef, false)) == NULL)
        {CFRelease(aFolderURLRef); return NULL;}
    
    // Note: may not be reachable if we are creating a new prefs file
    
    CFRelease(aFolderURLRef);
    
    return new CSynclavierFileReference(aFileURLRef);
}

CSynclavierFileReference*   locate_interchange_prefs_file(char* prefs_name) {
    CFStringRef                 aPrefNameRef;
    CSynclavierFileReference*   aFileReference;
    
    if ((aPrefNameRef = CFStringCreateWithCString(NULL, prefs_name, kCFStringEncodingUTF8)) == NULL)
        return NULL;
    
    if ((aFileReference = locate_interchange_prefs_file(aPrefNameRef)) == NULL)
        {CFRelease(aPrefNameRef); return NULL;}
    
    CFRelease(aPrefNameRef);
    
    return aFileReference;
}

#if !__LP64__
    OSErr locate_interchange_prefs_file(char *prefs_name, FSSpec* the_spec)
    {
        CSynclavierFileReference*   aFileReference;
        
        if ((aFileReference = locate_interchange_prefs_file(prefs_name)) == NULL)
            return fnfErr;
     
        if (aFileReference->CreateFSSpec(the_spec) != noErr)
            return fnfErr;
        
        aFileReference->Release();
        
        return noErr;
    }
#endif

// -----------------------------------------------------
// check_interchange_prefs_file_rev
// -----------------------------------------------------

OSErr 	check_interchange_prefs_file_rev (CSynclavierFileReference* aFileReference)
{
	interchange_settings temp_settings;
	SInt32		length;
	sig_struct	signature;
 	OSErr		err;
   
    if ((err = aFileReference->Open(O_RDONLY)) != noErr)
        return err;
    
	length = sizeof(signature);							// prep for file read
    
	if ((err = aFileReference->Read(&length, &signature)) != noErr)
	{
		aFileReference->Close();
		return (err);
	}
    
	if ((signature.our_id   != SIGNATURE_INFO.our_id)
    ||  (signature.rev_num  != SIGNATURE_INFO.rev_num ))
	{
		aFileReference->Close();
		return (eofErr);
	}
	
	length = sizeof(temp_settings);
	
    if ((err = aFileReference->Read(&length, &temp_settings)) != noErr)
	{
		aFileReference->Close();
		return err;
	}
    
    aFileReference->Close();
    return noErr;
}

#if !__LP64__
    OSErr check_interchange_prefs_file_rev(FSSpec *the_spec)
    {
        OSErr   err;
        CSynclavierFileReference* aFileReference;
        
        aFileReference = CSynclavierFileReference::CopyFromFSSpec(the_spec);
        
        err = check_interchange_prefs_file_rev(aFileReference);
        
        aFileReference->Release();
        
        return err;
    }
#endif

// -----------------------------------------------------
// check_interchange_prefs_file_rev
// -----------------------------------------------------

OSErr check_interchange2_prefs_file_rev(CSynclavierFileReference* aFileReference)
{
	interchange2_settings temp_settings;
	SInt32		length;
	sig_struct	signature;
    OSErr		err;

    if ((err = aFileReference->Open(O_RDONLY)) != noErr)
        return err;
    
	length = sizeof(signature);							// prep for file read
    
	if ((err = aFileReference->Read(&length, &signature)) != noErr)
	{
		aFileReference->Close();
		return (err);
	}
    
	if ((signature.our_id   != SIGNATURE_INFO.our_id)
    ||  (signature.rev_num  != SIGNATURE_INFO.rev_num ))
	{
		aFileReference->Close();
		return (eofErr);
	}
	
	length = sizeof(temp_settings);
    
    if ((err = aFileReference->Read(&length, &temp_settings)) != noErr)
	{
		aFileReference->Close();
		return err;
	}
    
    aFileReference->Close();
    return noErr;
}

#if !__LP64__
    OSErr check_interchange2_prefs_file_rev(FSSpec *the_spec)
    {
        OSErr   err;
        CSynclavierFileReference* aFileReference;
        
        aFileReference = CSynclavierFileReference::CopyFromFSSpec(the_spec);
        
        err = check_interchange2_prefs_file_rev(aFileReference);
        
        aFileReference->Release();
        
        return err;
    }
#endif

// -----------------------------------------------------
// check_interpreter_prefs_file_rev
// -----------------------------------------------------

OSErr check_interpreter_prefs_file_rev(CSynclavierFileReference* aFileReference)
{
	interpreter_settings temp_settings;
	SInt32		length;
	sig_struct	signature;
    OSErr		err;

    if ((err = aFileReference->Open(O_RDONLY)) != noErr)
        return err;
    
	length = sizeof(signature);							// prep for file read
    
	if ((err = aFileReference->Read(&length, &signature)) != noErr)
	{
		aFileReference->Close();
		return (err);
	}
    
	if ((signature.our_id   != SIGNATURE_INFO.our_id)
    ||  (signature.rev_num  != SIGNATURE_INFO.rev_num ))
	{
		aFileReference->Close();
		return (eofErr);
	}
	
	length = sizeof(temp_settings);
    
    if ((err = aFileReference->Read(&length, &temp_settings)) != noErr)
	{
		aFileReference->Close();
		return err;
	}
    
    aFileReference->Close();
    return noErr;
}

#if !__LP64__
    OSErr check_interpreter_prefs_file_rev(FSSpec *the_spec)
    {
        OSErr   err;
        CSynclavierFileReference* aFileReference;
        
        aFileReference = CSynclavierFileReference::CopyFromFSSpec(the_spec);
        
        err = check_interpreter_prefs_file_rev(aFileReference);
        
        aFileReference->Release();
        
        return err;
    }
#endif


// -----------------------------------------------------
// read_interchange_setup:  Reads in settings from prefs file.  Initializes settings
// to default values if no file exists or can't read it.
// -----------------------------------------------------

void read_interchange_setup(char *prefs_name, interchange_settings *the_settings, int resolve_all_alii)
{
	OSErr		err;
	short		refNum;									// prefs ref num
	SInt32		length;
	sig_struct	signature;
	short 		cur_res_file;
	Boolean 	any_alias_error = false;				// detect bad alias & delete all to keep system from pestering user from unavailable volume
 	AliasHandle new_alias = NULL;
	char		new_pathname[512];
	Boolean		new_settings_in_effect = false;
	SyncFSSpec	w0_spec;
    FSRef       ap_fsref;

    // Init for possible failure
    memset(the_settings, 0, sizeof(*the_settings));
    
    // Create filerefs for app bundle and resources folder, and our dedicated W0 image file
    CSynclavierFileReference* appFileRef = CSynclavierFileReference::CopyAppFileRef     ();
    CSynclavierFileReference* resFileRef = CSynclavierFileReference::CopyAppResourcesRef();
    CSynclavierFileReference* w0FileRef  = NULL;
   
    if (!appFileRef || !appFileRef->GetURL() || !appFileRef->GetParent())
        return;
    
    // Could test alias utilities
    #if 0
        CFDataRef   bookMark  = appFileRef->GetBookmark();
        CFStringRef stringRef = appFileRef->GetPath();
        char        string[1024];
        appFileRef->Path(string, 1024);
        
        CSynclavierFileReference* fileRef1 = new CSynclavierFileReference(bookMark);
        CSynclavierFileReference* fileRef2 = new CSynclavierFileReference(stringRef);
        CSynclavierFileReference* fileRef3 = new CSynclavierFileReference(string);
        CSynclavierFileReference* fileRef4 = new CSynclavierFileReference(fileRef3->GetParent());
        CSynclavierFileReference* fileRef5 = new CSynclavierFileReference(fileRef3->GetParent());
    
        CFURLRef ref1 = fileRef1->GetURL();
        CFURLRef ref2 = fileRef2->GetURL();
        CFURLRef ref3 = fileRef3->GetURL();
        
        SyncFSSpec spec1;
        SyncFSSpec spec2;
        SyncFSSpec spec3;
    
        fileRef3->CreateFSSpec(&spec1);
        fileRef4->CreateFSSpec(&spec2);
        fileRef5->CreateFSSpec(&spec3);
    
        if (AliasUtilities_DescendsFrom(spec1, spec2, 0))
            printf("AliasUtilities_DescendsFrom(spec1, spec2, 0)\n");
        
        if (AliasUtilities_DescendsFrom(spec2, spec1, 0))
            printf("AliasUtilities_DescendsFrom(spec2, spec1, 0)\n");
    
        int equals = InterChangeLibEqualFSSpecs(&spec1, &spec2);
            equals = InterChangeLibEqualFSSpecs(&spec1, &spec1);
            equals = InterChangeLibEqualFSSpecs(&spec2, &spec3);
    #endif
    
	// Look up local W0 disk image file in all cases
    CFURLRef    w0URL        = NULL;
	OSErr       w0_avail_err = fnfErr;
    FSRef       w0FSRef;
    
    // Get app bundle FSRef handy for alias resolving.
    appFileRef->CreateFSRef(&ap_fsref);
    
    // For carbon builds, the W0 image file was in the same folder as the app.
    // For cocoa builds the W0 image is hidden in the resources folder
    #if __LP64__
        w0URL = resFileRef->CreateChild(CFSTR("W0 Disk Image.simg"), false);                            // Create a file URL
    #else
        w0URL = appFileRef->CreateSibling(CFSTR(RELEASE_VERSION_NAME " W0 Disk Image.simg"), false);    // Create a file URL
    #endif
    
    if (w0URL) {
        w0FileRef = new CSynclavierFileReference(w0URL); // Ownership passes to FileRef
        
        if (w0FileRef) {
            w0FileRef->Resolve();
            
            if (w0FileRef->Reachable())
                w0_avail_err = w0FileRef->CreateFSRef(&w0FSRef);
            
            // Get rid if the file ref if the file is not reachable, or if for some reason we could not create an FSRef
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
    
    // Now read in the settings
    CSynclavierFileReference* prefsRef = locate_interchange_prefs_file(prefs_name);
    
    err = noErr;
    
    if (!prefsRef)
        err = fnfErr;
    
    if (!err)
        err = prefsRef->Open(O_RDONLY);
    
	if (!err)											// read signature
	{
        length = sizeof(signature);						// prep for file read
		err    = prefsRef->Read(&length, &signature);
		
		if (err)
		{
			prefsRef->Close();
			prefsRef->Delete();
		}
	}
    
	if (!err)											// check signature
	{
		if ((signature.our_id   != SIGNATURE_INFO.our_id)
        ||  (signature.rev_num  != SIGNATURE_INFO.rev_num ))
		{
			err = fnfErr;
			prefsRef->Close();
			prefsRef->Delete();
		}
	}
	
	if (!err)											// read data
	{
        // Measure length of 32-bit struct
        #if 0
            int  settingData = offsetof(scsi_settings, inq_name);
            int  settingToss = sizeof(scsi_settings) - offsetof(scsi_settings, inq_name);
            
            int  settupToss  = offsetof(interchange_settings, what_to_do) - offsetof(interchange_settings, export_fldr);
            int  settupRead  = sizeof(interchange_settings) - offsetof(interchange_settings, what_to_do);
        #endif
        
        // Read the data in 5 pieces to match the 32-bit build format on disk
        if (!err) {
            length = 16;
            err    = prefsRef->Read(&length, &the_settings->w0);
            
            if (!err)
                err = prefsRef->Skip(624);
        }
        
        if (!err) {
            length = 16;
            err    = prefsRef->Read(&length, &the_settings->w1);
            
            if (!err)
                err = prefsRef->Skip(624);
        }
        
        if (!err) {
            length = 16;
            err    = prefsRef->Read(&length, &the_settings->o0);
            
            if (!err)
                err = prefsRef->Skip(624);
        }
        
        if (!err) {
            length = 16;
            err    = prefsRef->Read(&length, &the_settings->o1);
            
            if (!err)
                err = prefsRef->Skip(624);
        }
        
        if (!err)
            err = prefsRef->Skip(3520);
        
        if (!err) {
            length = 612;
            err    = prefsRef->Read(&length, &the_settings->what_to_do);
        }
        
        // Was:
		//  length = sizeof(*the_settings);
		//  err    = prefsRef->Read(&length, the_settings);
  
		if (err)
		{
			prefsRef->Close();
			prefsRef->Delete();
		}
	}
    
    
	// Provide default settings if none available
	if (!err)
	{
        prefsRef->Close();
		the_settings->need_to_save_settings = false;	// don't need to save them if read in OK
	}
	
	else
	{
		memset(the_settings, 0, sizeof(*the_settings));

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
		
		the_settings->need_to_save_settings = true;		// need to save settings if none saved
	
		new_settings_in_effect = true;					// remember these are new settings 
	}
	
    // Close the file ref.
    if (prefsRef) {
        prefsRef->Release();
        prefsRef = NULL;
    }
	   
	// Now read in the aliases and user settings
    // For now file references are stored in ALIS resources
    // This will (eventually) change to CFURLRef bookmarks
    
    // ** note ** - for cocoa builds, the w0 image file in use is hard-wired to the
    // W0 image file within the app bundle. The user setting W0 (left over from
    // InterChangeX) is resolved and stored as the export image alias, so that it
    // can be easily imported.
	
	the_settings->w0.image_file = 0;					// null out meaningless alias pointers
	the_settings->w1.image_file = 0;
	the_settings->o0.image_file = 0;
	the_settings->o1.image_file = 0;
	the_settings->export_fldr   = 0;
	the_settings->export_imge   = 0;
	the_settings->export_file   = 0;
	the_settings->export_sysf   = 0;
	the_settings->export_sysi   = 0;
	the_settings->import_fldr   = 0;
	the_settings->info          = 0;
		
	memset(the_settings->w0.inq_name, 0, sizeof(the_settings->w0.inq_name));
	memset(the_settings->w1.inq_name, 0, sizeof(the_settings->w1.inq_name));
	memset(the_settings->o0.inq_name, 0, sizeof(the_settings->o0.inq_name));
	memset(the_settings->o1.inq_name, 0, sizeof(the_settings->o1.inq_name));
	
	// Zero out file names in case we don't look them up due to alias errors
	memset(the_settings->w0.image_pathname, 0, sizeof(the_settings->w0.image_pathname));
	memset(the_settings->w1.image_pathname, 0, sizeof(the_settings->w1.image_pathname));
	memset(the_settings->o0.image_pathname, 0, sizeof(the_settings->o0.image_pathname));
	memset(the_settings->o1.image_pathname, 0, sizeof(the_settings->o1.image_pathname));

	// Zero out the FSSpec in case the file does not exist. Also the data is meangless for 64-bit builds.
	memset(&the_settings->w0.image_spec, 0, sizeof(the_settings->w0.image_spec));
	memset(&the_settings->w1.image_spec, 0, sizeof(the_settings->w1.image_spec));
	memset(&the_settings->o0.image_spec, 0, sizeof(the_settings->o0.image_spec));
	memset(&the_settings->o1.image_spec, 0, sizeof(the_settings->o1.image_spec));

	// Zero out file names in case we don't look them up due to alias errors
	memset(the_settings->export_fldrname, 0, sizeof(the_settings->export_fldrname));
	memset(the_settings->export_imgename, 0, sizeof(the_settings->export_imgename));
	memset(the_settings->export_filename, 0, sizeof(the_settings->export_filename));
	memset(the_settings->export_sysfname, 0, sizeof(the_settings->export_sysfname));
	memset(the_settings->export_sysiname, 0, sizeof(the_settings->export_sysiname));
	memset(the_settings->import_fldrname, 0, sizeof(the_settings->import_fldrname));
	
	// Zero out the FSSpec in case the file does not exist. Also for 64-bit builds the data is meaningless.
	memset(&the_settings->export_fldrspec, 0, sizeof(the_settings->export_fldrspec));
	memset(&the_settings->export_imgespec, 0, sizeof(the_settings->export_imgespec));
	memset(&the_settings->export_filespec, 0, sizeof(the_settings->export_filespec));
	memset(&the_settings->export_sysfspec, 0, sizeof(the_settings->export_sysfspec));
	memset(&the_settings->export_sysispec, 0, sizeof(the_settings->export_sysispec));
	memset(&the_settings->import_fldrspec, 0, sizeof(the_settings->import_fldrspec));
    
    // Open the file again to read its resources. For now the persistent file references are stored as
    // ALIS resources in the preferences file. This will migrate to CFURLRef bookmarks.
    FSRef prefsFSRef;

    prefsRef = locate_interchange_prefs_file(prefs_name);
    
    err = noErr;
    
    if (!prefsRef)
        err = fnfErr;
    
    if (!err)
        err = prefsRef->CreateFSRef(&prefsFSRef);
    
    if (!err)
	{
		cur_res_file = CurResFile();					// preserve current resource file

        refNum = FSOpenResFile(&prefsFSRef, fsRdPerm);
        
		if (refNum == 0 || refNum == (-1))				// if can't open it, no problem...
			;			
		
		else
		{
			UseResFile (refNum);
			
			the_settings->w0.image_file = (AliasHandle) Get1Resource(rAliasType,        W0_IMAGE_FILE_ALIS_RESOURCE_ID);
			the_settings->w1.image_file = (AliasHandle) Get1Resource(rAliasType,        W1_IMAGE_FILE_ALIS_RESOURCE_ID);
			the_settings->o0.image_file = (AliasHandle) Get1Resource(rAliasType,        O0_IMAGE_FILE_ALIS_RESOURCE_ID);
			the_settings->o1.image_file = (AliasHandle) Get1Resource(rAliasType,        O1_IMAGE_FILE_ALIS_RESOURCE_ID);
            the_settings->info          = (Handle)      Get1Resource(kInfoResourceType, INFO_RESOURCE_ID              );
				
			// Detach the resource so we own them
			if (the_settings->w0.image_file)
				DetachResource ((Handle) the_settings->w0.image_file);
			
			if (the_settings->w1.image_file)
				DetachResource ((Handle) the_settings->w1.image_file);
			
			if (the_settings->o0.image_file)
				DetachResource ((Handle) the_settings->o0.image_file);

			if (the_settings->o1.image_file)
				DetachResource ((Handle) the_settings->o1.image_file);
            
            if (the_settings->info)
                DetachResource ((Handle) the_settings->info);
            
            // For cocoa builds, we use the internal disk image file for W0 in all cases.
            // The user specified W0 setting is stashed in the export alias so that it can easily be imported
            #if __LP64__
                // Move the W0 setting to export_imge.
                // This is only used to populate one of the easy-access file import sources.
                // Note: here we are moving the Interchange-specified W0 setting into the export_imge spot of the settings struct.
                if (the_settings->w0.image_file) {
                    the_settings->export_imge = the_settings->w0.image_file;
                    the_settings->w0.image_file = NULL;
                    AliasUtilities_Extract_Full_Name(&the_settings->export_imge, &the_settings->export_imgespec, the_settings->export_imgename, sizeof(the_settings->export_imgename));
                }
           
                if ((w0FileRef != NULL)                                             // if w0 disk image is available
                &&  (FSNewAlias(&ap_fsref, &w0FSRef, &new_alias) == noErr))         // and can make an alias to it
                {
                    the_settings->w0.bus_id     = SCSI_BUS_MENU_DISK_IMAGE;
                    the_settings->w0.device_id  = SCSI_ID_MENU_ID_5;
                    the_settings->w0.scsi_id    = ABLE_W0_DEFAULT_SCSI_ID;
                    the_settings->w0.sim_id     = ABLE_W0_DEFAULT_SCSI_ID;
                    the_settings->w0.disk_type  = 0;
                    the_settings->w0.image_file = new_alias;
                   
                    w0FileRef->CreateFSSpec(&the_settings->w0.image_spec);          // this does an additional retain on the file ref
                    AliasUtilities_GetFullPath(&the_settings->w0.image_spec, the_settings->w0.image_pathname, sizeof(the_settings->w0.image_pathname));
                    
                    // Handle bizarre case where the W0 user setting pointed to the resource disk image file. Avoid
                    // having two file refs for the same file
                    if (the_settings->export_imge) {
                        if (InterChangeLibEqualFSSpecs(&the_settings->export_imgespec, &the_settings->w0.image_spec)) {
                            the_settings->export_imgespec.file_ref->Release();
                            the_settings->export_imgespec = the_settings->w0.image_spec;
                            the_settings->w0.image_spec.file_ref->Retain();
                            // note we keep the original alias around (the_settings->export_imge). We just use the SyncFSSpec created
                            // from the_settings->w0.image_file to access the file. This is cool because they point to the same file.
                        }
                    }
                }

            // Substitute default W0 image file if needed.  Happens here if prefs file exists but
            // has no data (e.g. only resources from last run of InterChangeª
            #else
                if ((new_settings_in_effect || the_settings->w0.bus_id == SCSI_BUS_MENU_NONE)											// if new settings in effect
                &&  (w0_avail_err == noErr)												// and w0 disk image is available
                &&  (FSNewAlias(&ap_fsref, &w0FSRef, &new_alias) == noErr)              // and can make an alias to it
                &&  (AliasUtilities_Extract_Full_Name(&new_alias, &w0_spec, new_pathname, sizeof(new_pathname)) == noErr))
                {
                    if (the_settings->w0.image_file)
                        {DisposeHandle((Handle) the_settings->w0.image_file); the_settings->w0.image_file = 0;}
                    
                    the_settings->w0.bus_id      = SCSI_BUS_MENU_DISK_IMAGE;
                    the_settings->w0.device_id   = SCSI_ID_MENU_ID_5;
                    the_settings->w0.scsi_id     = ABLE_W0_DEFAULT_SCSI_ID;
                    the_settings->w0.sim_id      = ABLE_W0_DEFAULT_SCSI_ID;
                    the_settings->w0.disk_type   = 0;
                    the_settings->w0.image_file  = new_alias;
                    the_settings->w0.image_spec  = w0_spec;
                    strncpy(the_settings->w0.image_pathname, new_pathname, sizeof(new_pathname));
                }
            
                // Else set up the user specified W0
                else if ((the_settings->w0.image_file)
                &&       (resolve_all_alii || the_settings->w0.bus_id == SCSI_BUS_MENU_DISK_IMAGE))
                {
                    if (any_alias_error || AliasUtilities_Extract_Full_Name(&the_settings->w0.image_file, &the_settings->w0.image_spec, the_settings->w0.image_pathname, sizeof(the_settings->w0.image_pathname)))
                    {
                        delete_1_resource(refNum, rAliasType, W0_IMAGE_FILE_ALIS_RESOURCE_ID);
                        
                        if (the_settings->w0.bus_id == SCSI_BUS_MENU_DISK_IMAGE)		// need to save settings if in-use
                            the_settings->need_to_save_settings = true;					// disk image disappears
                        
                        any_alias_error = true;
                        
                        if (the_settings->w0.image_file)
                        {
                            DisposeHandle( (Handle) the_settings->w0.image_file);		// free up alias storage
                            the_settings->w0.image_file = NULL;
                        }
                        
                        memset(the_settings->w0.image_pathname, 0, sizeof(the_settings->w0.image_pathname));
                    }
                }
            
                // Read in possible interchange 1.0 settings
                the_settings->export_fldr   = (AliasHandle) Get1Resource(rAliasType,        EXPORT_FOLDER_ALIS_RESOURCE_ID);
                the_settings->export_imge   = (AliasHandle) Get1Resource(rAliasType,        EXPORT_IMAGE_ALIS_RESOURCE_ID );
                the_settings->export_file   = (AliasHandle) Get1Resource(rAliasType,        EXPORT_FILE_ALIS_RESOURCE_ID  );
                the_settings->export_sysf   = (AliasHandle) Get1Resource(rAliasType,        EXPORT_SYSTEM_FOLDER_ALIS_RESOURCE_ID );
                the_settings->export_sysi   = (AliasHandle) Get1Resource(rAliasType,        EXPORT_SYSTEM_IMAGE_ALIS_RESOURCE_ID  );
                the_settings->import_fldr   = (AliasHandle) Get1Resource(rAliasType,        IMPORT_FOLDER_ALIS_RESOURCE_ID);
            
                if (the_settings->export_fldr)
                    DetachResource ((Handle) the_settings->export_fldr);
                
                if (the_settings->export_imge)
                    DetachResource ((Handle) the_settings->export_imge);
                
                if (the_settings->export_file)
                    DetachResource ((Handle) the_settings->export_file);
                
                if (the_settings->export_sysf)
                    DetachResource ((Handle) the_settings->export_sysf);
                
                if (the_settings->export_sysi)
                    DetachResource ((Handle) the_settings->export_sysi);
                
                if (the_settings->import_fldr)
                    DetachResource ((Handle) the_settings->import_fldr);
                if (!any_alias_error && resolve_all_alii)						// if no errors with disk image files, check chosen items
                {
                    if (AliasUtilities_Extract_Full_Name(&the_settings->export_fldr,   &the_settings->export_fldrspec, the_settings->export_fldrname,   sizeof(the_settings->export_fldrname))
                    ||	AliasUtilities_Extract_Full_Name(&the_settings->export_imge,   &the_settings->export_imgespec, the_settings->export_imgename,   sizeof(the_settings->export_imgename))
                    ||	AliasUtilities_Extract_Full_Name(&the_settings->export_file,   &the_settings->export_filespec, the_settings->export_filename,   sizeof(the_settings->export_filename))
                    ||	AliasUtilities_Extract_Full_Name(&the_settings->export_sysf,   &the_settings->export_sysfspec, the_settings->export_sysfname,   sizeof(the_settings->export_sysfname))
                    ||	AliasUtilities_Extract_Full_Name(&the_settings->export_sysi,   &the_settings->export_sysispec, the_settings->export_sysiname,   sizeof(the_settings->export_sysiname))
                    ||	AliasUtilities_Extract_Full_Name(&the_settings->import_fldr,   &the_settings->import_fldrspec, the_settings->import_fldrname,   sizeof(the_settings->import_fldrname)))
                        any_alias_error = true;
                }
                
                // Free up user alii if one was not found
                if (any_alias_error)
                {
                    if (the_settings->export_fldr)
                        {DisposeHandle((Handle) the_settings->export_fldr); the_settings->export_fldr = 0;}
                    
                    if (the_settings->export_imge)
                        {DisposeHandle((Handle) the_settings->export_imge); the_settings->export_imge = 0;}
                    
                    if (the_settings->export_file)
                        {DisposeHandle((Handle) the_settings->export_file); the_settings->export_file = 0;}
                    
                    if (the_settings->export_sysf)
                        {DisposeHandle((Handle) the_settings->export_sysf); the_settings->export_sysf = 0;}
                    
                    if (the_settings->export_sysi)
                        {DisposeHandle((Handle) the_settings->export_sysi); the_settings->export_sysi = 0;}
                    
                    if (the_settings->import_fldr)
                        {DisposeHandle((Handle) the_settings->import_fldr); the_settings->import_fldr = 0;}
                    
                    the_settings->any_new_setting = true;									// re-write settings on exit if can't resolve
                }
                
                // Create file handle to internal W0 image file in all cases for creating image files
                if ((resolve_all_alii)
                &&  (w0_avail_err == noErr)
                &&  (FSNewAlias(&ap_fsref, &w0FSRef, &new_alias) == noErr)                  // and can make an alias to it and resolve the new alias
                &&  (AliasUtilities_Extract_Full_Name(&new_alias, &w0_spec, new_pathname, sizeof(new_pathname)) == noErr))
                {
                    if (the_settings->export_sysi)
                    {DisposeHandle((Handle) the_settings->export_sysi); the_settings->export_sysi = 0;}
                    
                    memset(the_settings->export_sysiname, 0, sizeof(the_settings->export_sysiname));
                    
                    the_settings->export_sysi     = new_alias;
                    the_settings->export_sysispec = w0_spec;
                    strncpy(the_settings->export_sysiname, new_pathname, sizeof(new_pathname));
                }
                
                // Tweak import folder name
                if (the_settings->import_fldrname[0])										// add colon to mac folder name to 
                    strcat(the_settings->import_fldrname, ":");								// indicate that's where it gets stored
            #endif
			
			
            // Resolve alii and get the full path name. Stop resolving alii if one doesn't exist to keep
			// O/S from asking for volumes that are not available.  Other software assumes that if
			// the alias pointer exists, then the file spec and name are valid...
			
			// Note: skip the alias resolution for unused disk image files and user settings when
			// launching from Synclavier¨ PowerPC; that is, don't resolve alii to unmounted servers when
			// there is no need...
            
  			if ((the_settings->w1.image_file)
			&&  (resolve_all_alii || the_settings->w1.bus_id == SCSI_BUS_MENU_DISK_IMAGE))
			{
				if (any_alias_error || AliasUtilities_Extract_Full_Name(&the_settings->w1.image_file, &the_settings->w1.image_spec, the_settings->w1.image_pathname, sizeof(the_settings->w1.image_pathname)))
				{
					delete_1_resource(refNum, rAliasType, W1_IMAGE_FILE_ALIS_RESOURCE_ID);
					
					if (the_settings->w1.bus_id == SCSI_BUS_MENU_DISK_IMAGE)		// need to save settings if in-use
						the_settings->need_to_save_settings = true;					// disk image disappears
					
					any_alias_error = true;
					
					if (the_settings->w1.image_file)
					{
						DisposeHandle( (Handle) the_settings->w1.image_file);		// free up alias storage
						the_settings->w1.image_file = NULL;
					}
					
					memset(the_settings->w1.image_pathname, 0, sizeof(the_settings->w1.image_pathname));
				}
			}

			if ((the_settings->o0.image_file)
			&&  (resolve_all_alii || the_settings->o0.bus_id == SCSI_BUS_MENU_DISK_IMAGE))
			{
				if (any_alias_error || AliasUtilities_Extract_Full_Name(&the_settings->o0.image_file, &the_settings->o0.image_spec, the_settings->o0.image_pathname, sizeof(the_settings->o0.image_pathname)))
				{
					delete_1_resource(refNum, rAliasType, O0_IMAGE_FILE_ALIS_RESOURCE_ID);
					
					if (the_settings->o0.bus_id == SCSI_BUS_MENU_DISK_IMAGE)		// need to save settings if in-use
						the_settings->need_to_save_settings = true;					// disk image disappears
					
					any_alias_error = true;

					if (the_settings->o0.image_file)
					{
						DisposeHandle( (Handle) the_settings->o0.image_file);		// free up alias storage
						the_settings->o0.image_file = NULL;
					}
					
					memset(the_settings->o0.image_pathname, 0, sizeof(the_settings->o0.image_pathname));
				}
			}
			
			if ((the_settings->o1.image_file)
			&&  (resolve_all_alii || the_settings->o1.bus_id == SCSI_BUS_MENU_DISK_IMAGE))
			{
				if (any_alias_error || AliasUtilities_Extract_Full_Name(&the_settings->o1.image_file, &the_settings->o1.image_spec, the_settings->o1.image_pathname, sizeof(the_settings->o1.image_pathname)))
				{
					delete_1_resource(refNum, rAliasType, O1_IMAGE_FILE_ALIS_RESOURCE_ID);
					
					if (the_settings->o1.bus_id == SCSI_BUS_MENU_DISK_IMAGE)		// need to save settings if in-use
						the_settings->need_to_save_settings = true;					// disk image disappears
					
					any_alias_error = true;

					if (the_settings->o1.image_file)
					{
						DisposeHandle( (Handle) the_settings->o1.image_file);		// free up alias storage
						the_settings->o1.image_file = NULL;
					}
					
					memset(the_settings->o1.image_pathname, 0, sizeof(the_settings->o1.image_pathname));
				}
			}
			
			CloseResFile(refNum);
		}
			
		UseResFile (cur_res_file);
	}

    #if __LP64__
        // Else default to local W0 Disk Image File if the settings did not work out (e.g. could not read file aliases from settings file)
        else if ((w0FileRef != NULL)                                             // if w0 disk image is available
        &&       (FSNewAlias(&ap_fsref, &w0FSRef, &new_alias) == noErr))         // and can make an alias to it
        {
            the_settings->w0.bus_id     = SCSI_BUS_MENU_DISK_IMAGE;
            the_settings->w0.device_id  = SCSI_ID_MENU_ID_5;
            the_settings->w0.scsi_id    = ABLE_W0_DEFAULT_SCSI_ID;
            the_settings->w0.sim_id     = ABLE_W0_DEFAULT_SCSI_ID;
            the_settings->w0.disk_type  = 0;
            the_settings->w0.image_file = new_alias;
            
            w0FileRef->CreateFSSpec(&the_settings->w0.image_spec);          // this does an additional retain on the file ref
            AliasUtilities_GetFullPath(&the_settings->w0.image_spec, the_settings->w0.image_pathname, sizeof(the_settings->w0.image_pathname));
        }
   #else
        // Else default to local W0 Disk Image File if the settings did not work out
        else if ((new_settings_in_effect)												// if new settings in effect
        &&	     (w0_avail_err == noErr)												// and w0 disk image is available
        &&       (FSNewAlias(&ap_fsref, &w0FSRef, &new_alias) == noErr)                 // and can make an alias to it and resolve the new alias
        &&       (AliasUtilities_Extract_Full_Name(&new_alias, &w0_spec, new_pathname, sizeof(new_pathname)) == noErr))
        {
            the_settings->w0.bus_id      = SCSI_BUS_MENU_DISK_IMAGE;
            the_settings->w0.device_id   = SCSI_ID_MENU_ID_5;
            the_settings->w0.scsi_id     = ABLE_W0_DEFAULT_SCSI_ID;
            the_settings->w0.sim_id      = ABLE_W0_DEFAULT_SCSI_ID;
            the_settings->w0.disk_type   = 0;
            the_settings->w0.image_file  = new_alias;
            the_settings->w0.image_spec  = w0_spec;
            strncpy(the_settings->w0.image_pathname, new_pathname, sizeof(new_pathname));
        }
    #endif
    
	
	// Initialize various settings
	the_settings->enter_can_activate_copy   = false;				// enter inactive at startup
	the_settings->need_to_export_settings   = false;				// no need to export settings
	the_settings->in_forground			    = true;					// with virtual certainty, we are!
	the_settings->export_active             = true;					// SIOUX is not the front window
	the_settings->we_are_working            = false;				// not working at the moment
	the_settings->any_new_setting           = false;				// no new settings yet
	the_settings->SIOUX_window_is_detatched = false;				// no new settings yet
	the_settings->have_seen_SIOUX_yet       = false;				// no new settings yet
	the_settings->current_scolled_amount    = 0;					// not scrolled yet
	the_settings->selected_text_field       = 0;					// no text field selected yet
	the_settings->allow_mac_erase			= false;				// init to disallow mac erase
	the_settings->recognize_disks			= false;				// init to not recognize questionable disks
		
	the_settings->w0.disk_type = 0;
	the_settings->w1.disk_type = 0;
	the_settings->o0.disk_type = 0;
	the_settings->o1.disk_type = 0;
	
	// Trash old saved settings if not right size
	if (the_settings->info)														// if we did read in a settings resource
	{
		if (GetHandleSize (the_settings->info) != sizeof(saved_settings))		// if not our size
		{
			DisposeHandle((Handle) the_settings->info);							// junk it
			the_settings->info = NULL;
		}
	}

	// Restore user working settings
	if (the_settings->info)														// if we got a settings resource of the
	{																			// right size, restore itse settings
		saved_settings *stuff = NULL;
		
		HLock  (the_settings->info);
		
		stuff = (saved_settings *) *the_settings->info;
		
		the_settings->what_to_do = stuff->what_to_do;

		the_settings->what_to_import  = stuff->what_to_import;
		strncpy(the_settings->what_to_import_text,  stuff->what_to_import_text,  sizeof(the_settings->what_to_import_text ));
		
		the_settings->where_to_export = stuff->where_to_export;
		strncpy(the_settings->where_to_export_text, stuff->where_to_export_text, sizeof(the_settings->where_to_export_text));

		the_settings->logging_options    = stuff->logging_options;
		the_settings->error_options      = stuff->error_options;
		the_settings->filenames_options  = stuff->filenames_options;
		the_settings->disk_image_options = stuff->disk_image_options;
		the_settings->subcatalog_options = stuff->subcatalog_options;
		the_settings->soundfile_options  = stuff->soundfile_options;
		the_settings->replacing_options  = stuff->replacing_options;
		the_settings->eject_media        = stuff->eject_media;
		the_settings->show_setup         = stuff->show_setup;
		the_settings->show_options       = stuff->show_options;
		
		HUnlock(the_settings->info);
	}

	else
	{			
		the_settings->what_to_do = WHAT_TO_DO_MENU_IMPORT_DEVICE_TO_FILES;

		the_settings->what_to_import  = IMPORT_W0;
		memset(the_settings->what_to_import_text,  0, sizeof(the_settings->what_to_import_text ));

		the_settings->where_to_export = IMPORT_W0;
		memset(the_settings->where_to_export_text, 0, sizeof(the_settings->where_to_export_text));

		the_settings->logging_options    = LOGGING_OPTIONS_MENU_ONLY_ERRORS;
		the_settings->error_options      = ERROR_OPTIONS_MENU_STOP;
		the_settings->filenames_options  = FILENAMES_OPTIONS_MENU_STOP;
		the_settings->disk_image_options = DISK_IMAGE_OPTIONS_MENU_PRESERVE;
		the_settings->subcatalog_options = SUBCATALOG_OPTIONS_MENU_RESTORE;
		the_settings->soundfile_options  = SOUNDFILE_OPTIONS_MENU_SNCL;
		the_settings->replacing_options  = REPLACING_OPTIONS_MENU_WARN;
		the_settings->eject_media        = FALSE;
		the_settings->show_setup         = true;
		the_settings->show_options       = true;
	}
	
	// Detect and handle old prefs file with no soundfile prefs
	if (the_settings->soundfile_options == 0)
		the_settings->soundfile_options = SOUNDFILE_OPTIONS_MENU_SNCL;
    
    // Done with the W0 file ref; although it probably has been used & retained...
    if (w0FileRef) {
        w0FileRef->Release();
        w0FileRef = NULL;
    }
}


// -----------------------------------------------------
// Check setup: look for possibly moved files upon switch from background
// -----------------------------------------------------

// Returns true if any changed
static int check_one_interchange_setup(AliasHandle *its_alias, SyncFSSpec *its_file_spec, char *its_name, int its_max)
{
	char        new_name[512];
	SyncFSSpec	new_spec;
	
	if (!its_alias || !*its_alias)								// nothing to check if no alias
		return (false);
		
	// See if can still resolve alias; zap info and return changed indicator if can't
	if (AliasUtilities_Extract_Full_Name(its_alias, &new_spec, new_name, sizeof(new_name)))
	{
        SyncFSSpecRelease(its_file_spec);
		memset(its_file_spec, 0, sizeof(*its_file_spec));
		memset(its_name,      0, its_max               );
		return (true);
	}
	
	// Same path name: no problem!
	if (strcmp(new_name, its_name) == 0) {
        SyncFSSpecRelease(&new_spec);
		return (0);
    }
	
    SyncFSSpecRelease(its_file_spec);
	memset(its_file_spec, 0, sizeof(*its_file_spec));
	memset(its_name,      0, its_max               );

	*its_file_spec = new_spec;
	strncpy(its_name, new_name, its_max);

	return (true);
}

int check_interchange_image_files(interchange_settings *the_settings)
{
	int status = 0;
	
	status |= check_one_interchange_setup(&the_settings->w0.image_file, &the_settings->w0.image_spec,   the_settings->w0.image_pathname, sizeof(the_settings->w0.image_pathname));
	status |= check_one_interchange_setup(&the_settings->w1.image_file, &the_settings->w1.image_spec,   the_settings->w1.image_pathname, sizeof(the_settings->w1.image_pathname));
	status |= check_one_interchange_setup(&the_settings->o0.image_file, &the_settings->o0.image_spec,   the_settings->o0.image_pathname, sizeof(the_settings->o0.image_pathname));
	status |= check_one_interchange_setup(&the_settings->o1.image_file, &the_settings->o1.image_spec,   the_settings->o1.image_pathname, sizeof(the_settings->o1.image_pathname));

	return (status);
}

int check_interchange_selected_files(interchange_settings *the_settings)
{
	int status = 0;
	
	status |= check_one_interchange_setup(&the_settings->export_fldr,   &the_settings->export_fldrspec, the_settings->export_fldrname,   sizeof(the_settings->export_fldrname  ));
	status |= check_one_interchange_setup(&the_settings->export_imge,   &the_settings->export_imgespec, the_settings->export_imgename,   sizeof(the_settings->export_imgename  ));
	status |= check_one_interchange_setup(&the_settings->export_file,   &the_settings->export_filespec, the_settings->export_filename,   sizeof(the_settings->export_filename  ));
	status |= check_one_interchange_setup(&the_settings->export_sysf,   &the_settings->export_sysfspec, the_settings->export_sysfname,   sizeof(the_settings->export_sysfname  ));
	status |= check_one_interchange_setup(&the_settings->export_sysi,   &the_settings->export_sysispec, the_settings->export_sysiname,   sizeof(the_settings->export_sysiname  ));
	status |= check_one_interchange_setup(&the_settings->import_fldr,   &the_settings->import_fldrspec, the_settings->import_fldrname,   sizeof(the_settings->import_fldrname  ));

	return (status);
}


// -----------------------------------------------------
// Toss config: just toss stuff
// -----------------------------------------------------

void toss_interchange_setup(interchange_settings *the_settings)
{	
	if (the_settings->w0.image_file)	DisposeHandle((Handle) the_settings->w0.image_file);
	if (the_settings->w1.image_file)	DisposeHandle((Handle) the_settings->w1.image_file);
	if (the_settings->o0.image_file)	DisposeHandle((Handle) the_settings->o0.image_file);
	if (the_settings->o1.image_file)	DisposeHandle((Handle) the_settings->o1.image_file);
	if (the_settings->export_fldr)		DisposeHandle((Handle) the_settings->export_fldr  );
	if (the_settings->export_imge)		DisposeHandle((Handle) the_settings->export_imge  );
	if (the_settings->export_file)		DisposeHandle((Handle) the_settings->export_file  );
	if (the_settings->export_sysf)		DisposeHandle((Handle) the_settings->export_sysf  );
	if (the_settings->export_sysi)		DisposeHandle((Handle) the_settings->export_sysi  );
	if (the_settings->import_fldr)		DisposeHandle((Handle) the_settings->import_fldr  );
	if (the_settings->info       )		DisposeHandle((Handle) the_settings->info         );

	the_settings->w0.image_file = 0;
	the_settings->w1.image_file = 0;
	the_settings->o0.image_file = 0;
	the_settings->o1.image_file = 0;
	the_settings->export_fldr   = 0;
	the_settings->export_imge   = 0;
	the_settings->export_file   = 0;
	the_settings->export_sysf   = 0;
	the_settings->export_sysi   = 0;
	the_settings->import_fldr   = 0;
	the_settings->info          = 0;
}


// -----------------------------------------------------
// Release config: release file refs; mostly applicable for 64-bit builds
// -----------------------------------------------------

void 	release_interchange_setup(interchange_settings *the_settings)
{
    toss_interchange_setup(the_settings);           // Get rid of any dangling aliaa
    
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
	OSErr		err;									// file system error
	SInt32		length;									// used for FSWrite
    char        zeroes[3520];
    
    memset(zeroes, 0, sizeof(zeroes));
	
    CSynclavierFileReference* prefsRef = locate_interchange_prefs_file(prefs_name);
    
    err = noErr;
    
    if (!prefsRef)
        err = fnfErr;
    
    if (!err)
        err = prefsRef->Open(O_WRONLY | O_CREAT | O_TRUNC);
    
	if (!err)											// if we could get the file open
	{
        length = sizeof(SIGNATURE_INFO);
        err    = prefsRef->Write(&length, &SIGNATURE_INFO);
		
        // Write the data in 5 pieces to match the 32-bit build format on disk
        if (!err) {
            length = 16;
            err    = prefsRef->Write(&length, &the_settings->w0);
           
            if (!err) {
                length = 624;
                err    = prefsRef->Write(&length, zeroes);
            }
        }
        
        if (!err) {
            length = 16;
            err    = prefsRef->Write(&length, &the_settings->w1);
            
            if (!err) {
                length = 624;
                err    = prefsRef->Write(&length, zeroes);
            }
        }
        
        if (!err) {
            length = 16;
            err    = prefsRef->Write(&length, &the_settings->o0);
            
            if (!err) {
                length = 624;
                err    = prefsRef->Write(&length, zeroes);
            }
        }
        
        if (!err) {
            length = 16;
            err    = prefsRef->Write(&length, &the_settings->o1);
            
            if (!err) {
                length = 624;
                err    = prefsRef->Write(&length, zeroes);
            }
        }
        
        if (!err) {
            length = 3520;
            err    = prefsRef->Write(&length, zeroes);
        }
        
        if (!err) {
            length = 612;
            err    = prefsRef->Write(&length, &the_settings->what_to_do);
        }
        
        // Was:
        #if 0
            if (!err)
            {												// write data
                length = sizeof(*the_settings);
                err    = prefsRef->Write(&length, the_settings);
            }
        #endif
        
        prefsRef->Sync ();
        prefsRef->Close();

		if (err)										// delete partially written file
            prefsRef->Delete();
        else
            prefsRef->SetFinfo('DATA', 'SNCL');
	}
    
    if (prefsRef)
        prefsRef->Release();
}


// -----------------------------------------------------
// write_interchange_1_0_aliai
// -----------------------------------------------------

// write aliai associated with interchange 1.0 settings only
void write_interchange_1_0_aliai(char *prefs_name, interchange_settings *the_settings)
{	
    FSRef   prefsFSRef;
	short   refNum = 0;
	OSErr   err;
	short   cur_res_file;
	
    CSynclavierFileReference* prefsRef = locate_interchange_prefs_file(prefs_name);
    
    err = noErr;
    
    if (!prefsRef)
        err = fnfErr;
    
    if (!err && !prefsRef->Reachable())
        err = prefsRef->Create('DATA', 'SNCL');
    
    if (!err)
        err = prefsRef->CreateFSRef(&prefsFSRef);
    
	// Now write out the file aliases

	cur_res_file = CurResFile();						// preserve current resource file
	
	if (!err)											// if we could get the file open
	{
        refNum = FSOpenResFile(&prefsFSRef, fsRdWrPerm);

		if (refNum == 0 || refNum == (-1))				// if can't open resource fork, try creating it
        {
            HFSUniStr255 resourceForkName;
            
            FSGetResourceForkName(&resourceForkName);
            
            err = FSCreateResourceFork(&prefsFSRef, resourceForkName.length, resourceForkName.unicode, 0);
            
            if (!err) {
                refNum = FSOpenResFile(&prefsFSRef, fsRdWrPerm);
                
                if (refNum == 0 || refNum == (-1))		// if can't open resource fork
                    err = fnfErr;
            }
        }
	}

	if (!err)
	{
		UseResFile (refNum);

		if (the_settings->export_fldr)
			replace_1_resource((Handle) the_settings->export_fldr, refNum, rAliasType,
			                   EXPORT_FOLDER_ALIS_RESOURCE_ID, (unsigned char *) "\pExport Folder Alias");
		else
			delete_1_resource(refNum, rAliasType, EXPORT_FOLDER_ALIS_RESOURCE_ID);

		if (the_settings->export_imge)
			replace_1_resource((Handle) the_settings->export_imge, refNum, rAliasType,
			                   EXPORT_IMAGE_ALIS_RESOURCE_ID, (unsigned char *) "\pExport Image Alias");
		else
			delete_1_resource(refNum, rAliasType, EXPORT_IMAGE_ALIS_RESOURCE_ID);

		if (the_settings->export_file)
			replace_1_resource((Handle) the_settings->export_file, refNum, rAliasType,
			                   EXPORT_FILE_ALIS_RESOURCE_ID, (unsigned char *) "\pExport File Alias");
		else
			delete_1_resource(refNum, rAliasType, EXPORT_FILE_ALIS_RESOURCE_ID);

		if (the_settings->export_sysf)
			replace_1_resource((Handle) the_settings->export_sysf, refNum, rAliasType,
			                   EXPORT_SYSTEM_FOLDER_ALIS_RESOURCE_ID, (unsigned char *) "\pExport System Folder Alias");
		else
			delete_1_resource(refNum, rAliasType, EXPORT_SYSTEM_FOLDER_ALIS_RESOURCE_ID);

		if (the_settings->export_sysi)
			replace_1_resource((Handle) the_settings->export_sysi, refNum, rAliasType,
			                   EXPORT_SYSTEM_IMAGE_ALIS_RESOURCE_ID, (unsigned char *) "\pExport System Disk Image File Alias");
		else
			delete_1_resource(refNum, rAliasType, EXPORT_SYSTEM_IMAGE_ALIS_RESOURCE_ID);

		if (the_settings->import_fldr)
			replace_1_resource((Handle) the_settings->import_fldr, refNum, rAliasType,
			                   IMPORT_FOLDER_ALIS_RESOURCE_ID, (unsigned char *) "\pImport File Alias");
		else
			delete_1_resource(refNum, rAliasType, IMPORT_FOLDER_ALIS_RESOURCE_ID);

		if (the_settings->info)
			replace_1_resource((Handle) the_settings->info, refNum, kInfoResourceType,
			                   INFO_RESOURCE_ID, (unsigned char *) "\pSaved Settings");

		the_settings->export_fldr   = 0;	// Null out alias pointers since the data is now owned by the resource manager
		the_settings->export_imge   = 0;
		the_settings->export_file   = 0;
		the_settings->export_sysf   = 0;
		the_settings->export_sysi   = 0;
		the_settings->import_fldr   = 0;
		the_settings->info          = 0;

		CloseResFile(refNum);
	}

	UseResFile (cur_res_file);
		
	// if error, free up memory ourselves

	if (err)								// if can't write resources, just delete them
		toss_interchange_setup(the_settings);
	else
		the_settings->any_new_setting = false;
    
    if (prefsRef)
        prefsRef->Release();
}


// -----------------------------------------------------
// replace_interchange_1_0_alias
// -----------------------------------------------------

// write one alias associated with interchange 1.0 settings only
void replace_interchange_1_0_alias(char *prefs_name, short the_id, AliasHandle the_alias)
{	
    FSRef   prefsFSRef;
	short	refNum = 0;                                 // prefs ref num
	OSErr	err;                                        // file system error
	short 	cur_res_file;
	
    CSynclavierFileReference* prefsRef = locate_interchange_prefs_file(prefs_name);
    
    err = noErr;
    
    if (!prefsRef)
        err = fnfErr;
    
    if (!err && !prefsRef->Reachable())
        err = prefsRef->Create('DATA', 'SNCL');
    
    if (!err)
        err = prefsRef->CreateFSRef(&prefsFSRef);
    
	cur_res_file = CurResFile();						// preserve current resource file
	
	if (!err)											// if we could get the file open
	{
        refNum = FSOpenResFile(&prefsFSRef, fsRdWrPerm);
        
		if (refNum == 0 || refNum == (-1))				// if can't open resource fork, try creating it
        {
            HFSUniStr255 resourceForkName;
            
            FSGetResourceForkName(&resourceForkName);
            
            err = FSCreateResourceFork(&prefsFSRef, resourceForkName.length, resourceForkName.unicode, 0);
            
            if (!err) {
                refNum = FSOpenResFile(&prefsFSRef, fsRdWrPerm);
                
                if (refNum == 0 || refNum == (-1))		// if can't open resource fork
                    err = fnfErr;
            }
        }
	}
    
	if (!err)
	{
		UseResFile (refNum);

		if (the_alias)
			replace_1_resource((Handle) the_alias, refNum, rAliasType, the_id, (unsigned char *) "\p");
		else
			delete_1_resource(refNum, rAliasType, the_id);

		CloseResFile(refNum);
	}

	UseResFile (cur_res_file);
}


// -----------------------------------------------------
// write_interchange_setup: writes settings to prefs file, thereby freeing up
// allocated memory.
// -----------------------------------------------------

void write_interchange_setup(char *prefs_name, interchange_settings *the_settings)
{	
    FSRef   prefsFSRef;
	short	refNum = 0;                                 // prefs ref num
	OSErr	err;                                        // file system error
	short 	cur_res_file;
	
	write_interchange_basic_settings(prefs_name, the_settings);

    CSynclavierFileReference* prefsRef = locate_interchange_prefs_file(prefs_name);
    
    err = noErr;
    
    if (!prefsRef)
        err = fnfErr;
    
    if (!err && !prefsRef->Reachable())
        err = prefsRef->Create('DATA', 'SNCL');
    
    if (!err)
        err = prefsRef->CreateFSRef(&prefsFSRef);
    
	cur_res_file = CurResFile();						// preserve current resource file
	
	if (!err)											// if we could get the file open
	{
        refNum = FSOpenResFile(&prefsFSRef, fsRdWrPerm);
        
		if (refNum == 0 || refNum == (-1))				// if can't open resource fork, try creating it
        {
            HFSUniStr255 resourceForkName;
            
            FSGetResourceForkName(&resourceForkName);
            
            err = FSCreateResourceFork(&prefsFSRef, resourceForkName.length, resourceForkName.unicode, 0);
            
            if (!err) {
                refNum = FSOpenResFile(&prefsFSRef, fsRdWrPerm);
                
                if (refNum == 0 || refNum == (-1))		// if can't open resource fork
                    err = fnfErr;
            }
        }
	}
    
	if (!err)
	{
		UseResFile (refNum);

		if (the_settings->w0.image_file)
			replace_1_resource((Handle) the_settings->w0.image_file, refNum, rAliasType ,
			                   W0_IMAGE_FILE_ALIS_RESOURCE_ID, (unsigned char *) "\pW0 Disk File Alias");
		else
			delete_1_resource(refNum, rAliasType, W0_IMAGE_FILE_ALIS_RESOURCE_ID);

		if (the_settings->w1.image_file)
			replace_1_resource((Handle) the_settings->w1.image_file, refNum, rAliasType,
			                   W1_IMAGE_FILE_ALIS_RESOURCE_ID, (unsigned char *) "\pW1 Disk File Alias");
		else
			delete_1_resource(refNum, rAliasType, W1_IMAGE_FILE_ALIS_RESOURCE_ID);

		if (the_settings->o0.image_file)
			replace_1_resource((Handle) the_settings->o0.image_file, refNum, rAliasType,
			                   O0_IMAGE_FILE_ALIS_RESOURCE_ID, (unsigned char *) "\pO0 Disk File Alias");
		else
			delete_1_resource(refNum, rAliasType, O0_IMAGE_FILE_ALIS_RESOURCE_ID);

		if (the_settings->o1.image_file)
			replace_1_resource((Handle) the_settings->o1.image_file, refNum, rAliasType,
			                   O1_IMAGE_FILE_ALIS_RESOURCE_ID, (unsigned char *) "\pO1 Disk File Alias");
		else
			delete_1_resource(refNum, rAliasType, O1_IMAGE_FILE_ALIS_RESOURCE_ID);

		if (the_settings->export_fldr)
			replace_1_resource((Handle) the_settings->export_fldr, refNum, rAliasType,
			                   EXPORT_FOLDER_ALIS_RESOURCE_ID, (unsigned char *) "\pExport Folder Alias");
		else
			delete_1_resource(refNum, rAliasType, EXPORT_FOLDER_ALIS_RESOURCE_ID);

		if (the_settings->export_imge)
			replace_1_resource((Handle) the_settings->export_imge, refNum, rAliasType,
			                   EXPORT_IMAGE_ALIS_RESOURCE_ID, (unsigned char *) "\pExport Image Alias");
		else
			delete_1_resource(refNum, rAliasType, EXPORT_IMAGE_ALIS_RESOURCE_ID);

		if (the_settings->export_file)
			replace_1_resource((Handle) the_settings->export_file, refNum, rAliasType,
			                   EXPORT_FILE_ALIS_RESOURCE_ID, (unsigned char *) "\pExport File Alias");
		else
			delete_1_resource(refNum, rAliasType, EXPORT_FILE_ALIS_RESOURCE_ID);

		if (the_settings->export_sysf)
			replace_1_resource((Handle) the_settings->export_sysf, refNum, rAliasType,
			                   EXPORT_SYSTEM_FOLDER_ALIS_RESOURCE_ID, (unsigned char *) "\pExport System Folder Alias");
		else
			delete_1_resource(refNum, rAliasType, EXPORT_SYSTEM_FOLDER_ALIS_RESOURCE_ID);

		if (the_settings->export_sysi)
			replace_1_resource((Handle) the_settings->export_sysi, refNum, rAliasType,
			                   EXPORT_SYSTEM_IMAGE_ALIS_RESOURCE_ID, (unsigned char *) "\pExport System Disk Image File Alias");
		else
			delete_1_resource(refNum, rAliasType, EXPORT_SYSTEM_IMAGE_ALIS_RESOURCE_ID);

		if (the_settings->import_fldr)
			replace_1_resource((Handle) the_settings->import_fldr, refNum, rAliasType,
			                   IMPORT_FOLDER_ALIS_RESOURCE_ID, (unsigned char *) "\pImport File Alias");
		else
			delete_1_resource(refNum, rAliasType, IMPORT_FOLDER_ALIS_RESOURCE_ID);

		if (the_settings->info)
			replace_1_resource((Handle) the_settings->info, refNum, kInfoResourceType,
			                   INFO_RESOURCE_ID, (unsigned char *) "\pSaved Settings");

		the_settings->w0.image_file = 0;	// Null out local copies since they are now owned by the resource mechanism
		the_settings->w1.image_file = 0;
		the_settings->o0.image_file = 0;
		the_settings->o1.image_file = 0;
		the_settings->export_fldr   = 0;
		the_settings->export_imge   = 0;
		the_settings->export_file   = 0;
		the_settings->export_sysf   = 0;
		the_settings->export_sysi   = 0;
		the_settings->import_fldr   = 0;
		the_settings->info          = 0;

		CloseResFile(refNum);
	}

	UseResFile (cur_res_file);
	
	// if error, free up memory ourselves

	if (err)								// if can't write resources, just delete them
		toss_interchange_setup(the_settings);
	else
		the_settings->any_new_setting = false;
}


// -----------------------------------------------------
// read_interchange2_setup:  Reads in settings from prefs file.  Initializes settings
// to default values if no file exists or can't read it.
// -----------------------------------------------------

void read_interchange2_setup(char *prefs_name, interchange2_settings *the_settings)
{
	FSCatalogInfo	catInfo;							// Catalog info
	FSRef			anFSRef;							// Handly FSRef
	UInt32			aNodeID = 0;
	OSErr			err;
	SInt32			length;
	sig_struct		signature;

    CSynclavierFileReference* prefsRef = locate_interchange_prefs_file(prefs_name);
    
    err = noErr;

	if (!err)											// Get cat info
	{
		if (prefsRef->CreateFSRef(&anFSRef) == noErr)
		{
			if (FSGetCatalogInfo(&anFSRef, kFSCatInfoNodeID | kFSCatInfoParentDirID, &catInfo, NULL, NULL, NULL) == noErr)
				aNodeID = catInfo.nodeID * catInfo.parentDirID;		// Should slow the hackers down
		}
	}

	if (!err)											// open DF
        err = prefsRef->Open(O_RDONLY);
			
	if (!err)											// read signature	
	{
        length = sizeof(signature);							// prep for file read
		err    = prefsRef->Read(&length, &signature);
        
		if (err)
		{
			prefsRef->Close();
			prefsRef->Delete();
		}
	}
				
	if (!err)											// check signature					
	{
		if ((signature.our_id   != SIGNATURE_INFO.our_id)
		||  (signature.rev_num  != SIGNATURE_INFO.rev_num ))
		{
			err = fnfErr;
			prefsRef->Close();
			prefsRef->Delete();
		}
	}
	
	if (!err)											// read data
	{
		length = sizeof(*the_settings);
		err    = prefsRef->Read(&length, the_settings);
        
		if (err)
		{
			prefsRef->Close();
			prefsRef->Delete();
		}
		
		// Else make sure file hasn't been snarfed
		else
		{
			// Make sure registration key matches
			if (the_settings->registered_key != aNodeID)
				{the_settings->registered_key = 0; the_settings->registered_bits = 0;}
		}
	}
			
	// Provide default settings if none available	
	if (!err) {
        prefsRef->Close();
        the_settings->settings_changed = false;
    }
	
	else
	{
		RGBColor color_black = {0, 0, 0};
		
		memset(the_settings, 0, sizeof(*the_settings));

		the_settings->sound_color    = color_black;
		the_settings->sequence_color = color_black;
		the_settings->timbre_color   = color_black;
		the_settings->other_color    = color_black;
	}
	
	// Initialize new settings
	if (the_settings->information  == 0) the_settings->information  = 1;
	if (the_settings->optical_sort == 0) the_settings->optical_sort = 1;
}


// -----------------------------------------------------
// write_interchange2_setup
// -----------------------------------------------------

static UInt32 boot_time = TickCount();

void write_interchange2_setup(char *prefs_name, interchange2_settings *the_settings)
{	
	FSCatalogInfo	catInfo;							// Catalog info
	FSRef			anFSRef;							// Handly FSRef
	UInt32			aNodeID;
	OSErr			err;								// file system error
	SInt32			length;								// used for FSWrite

	the_settings->settings_changed = false;
	aNodeID = 0;
	
    CSynclavierFileReference* prefsRef = locate_interchange_prefs_file(prefs_name);
    
    err = noErr;
    
    if (!prefsRef)
        err = fnfErr;
    
    if (!err && !prefsRef->Reachable())
        err = prefsRef->Create('DATA', 'SNCL');
    
    if (!err)
        err = prefsRef->CreateFSRef(&anFSRef);
    
	// Get files node ID handy
	if (!err)
	{
        if (FSGetCatalogInfo(&anFSRef, kFSCatInfoNodeID | kFSCatInfoParentDirID, &catInfo, NULL, NULL, NULL) == noErr)
            aNodeID = catInfo.nodeID * catInfo.parentDirID;		// Should slow the hackers down
	}

    if (!err)
        err = prefsRef->Open(O_WRONLY | O_CREAT | O_TRUNC);
    
	if (!err)											// if we could get the file open
	{
        length = sizeof(SIGNATURE_INFO);
        err    = prefsRef->Write(&length, &SIGNATURE_INFO);

		// Store node ID in config file
		if (the_settings->registered_key == 1)
			the_settings->registered_key = aNodeID;
			
		the_settings->up_time += TickCount() - boot_time;
		
		boot_time = TickCount();
		
		if (the_settings->up_time > (KAGI_FREE_TIME - 60*60))		// Avoid math errors after decades of running. Give deadbeats 60 seconds of joy.
			the_settings->up_time = (KAGI_FREE_TIME - 60*60);

		if (!err)
		{                                                           // write data
			length = sizeof(*the_settings);
			err    = prefsRef->Write(&length, the_settings);
		}
		
        prefsRef->Sync ();
        prefsRef->Close();
        
		if (err)										// delete partially written file
            prefsRef->Delete();
	}
    
    if (prefsRef)
        prefsRef->Release();
}


// -----------------------------------------------------
// store_interchange2_device_alias
// -----------------------------------------------------

void store_interchange2_device_alias(char *prefs_name, interchange2_settings *the_settings, short resID, AliasHandle theAlias)
{
    FSRef   prefsFSRef;
	short   refNum = 0;
	OSErr   err;
	short   cur_res_file;

    CSynclavierFileReference* prefsRef = locate_interchange_prefs_file(prefs_name);
    
    if (!prefsRef)
		return;
    
    if (!prefsRef->Reachable())
		write_interchange2_setup(prefs_name, the_settings);
    
    err = prefsRef->CreateFSRef(&prefsFSRef);
    
	if (err)											// can't proceed
        {prefsRef->Release(); return;}
	
	cur_res_file = CurResFile();						// preserve current resource file
	
	if (!err)											// if we could get the file open
	{
        refNum = FSOpenResFile(&prefsFSRef, fsRdWrPerm);
        
		if (refNum == 0 || refNum == (-1))				// if can't open resource fork, try creating it
        {
            HFSUniStr255 resourceForkName;
            
            FSGetResourceForkName(&resourceForkName);
            
            err = FSCreateResourceFork(&prefsFSRef, resourceForkName.length, resourceForkName.unicode, 0);
            
            if (!err) {
                refNum = FSOpenResFile(&prefsFSRef, fsRdWrPerm);
                
                if (refNum == 0 || refNum == (-1))		// if can't open resource fork
                    err = fnfErr;
            }
        }
	}
	
	if (!err)
	{
		UseResFile (refNum);

		replace_1_resource((Handle) theAlias, refNum, rAliasType , resID, (unsigned char *) "\p");

		CloseResFile(refNum);
	}
	
    UseResFile (cur_res_file);
    
    if (prefsRef)
        prefsRef->Release();
}


// -----------------------------------------------------
// remove_interchange2_device_alias
// -----------------------------------------------------

void remove_interchange2_device_alias(char *prefs_name, short resID)
{
    FSRef   prefsFSRef;
	short   refNum = 0;
	OSErr   err;
	short   cur_res_file;

    CSynclavierFileReference* prefsRef = locate_interchange_prefs_file(prefs_name);
    
     if (!prefsRef)
        return;
    
    if (!prefsRef->Reachable())                         // file does not exist
        {prefsRef->Release(); return;}
    
    err = prefsRef->CreateFSRef(&prefsFSRef);
	
	if (err)
        {prefsRef->Release(); return;}
    
	cur_res_file = CurResFile();						// preserve current resource file
		
    refNum = FSOpenResFile(&prefsFSRef, fsRdWrPerm);

	if (refNum == 0 || refNum == (-1))					// if can't open resource fork...
        {prefsRef->Release(); return;}
	
	UseResFile (refNum);

	delete_1_resource (refNum, rAliasType, resID);

	CloseResFile(refNum);
    
    UseResFile (cur_res_file);
    
    if (prefsRef)
        prefsRef->Release();
}


// -----------------------------------------------------
// open_interchange2_res_file
// -----------------------------------------------------

short open_interchange2_res_file(char *prefs_name)
{
    FSRef   prefsFSRef;
	short   refNum = 0;
	OSErr   err    = noErr;
    
    CSynclavierFileReference* prefsRef = locate_interchange_prefs_file(prefs_name);
    
    if (!prefsRef)
        return 0;
    
    if (!prefsRef->Reachable())                         // file does not exist
        {prefsRef->Release(); return 0;}
    
    err = prefsRef->CreateFSRef(&prefsFSRef);
	
	if (err)
        {prefsRef->Release(); return 0;}

    refNum = FSOpenResFile(&prefsFSRef, fsRdWrPerm);
    
	if (refNum == 0 || refNum == (-1))					// if can't open resource fork...
        {prefsRef->Release(); return 0;}
		
	return (refNum);
}


// -----------------------------------------------------
// read_interpreter_setup:  Reads in settings from prefs file.  Initializes settings
// to default values if no file exists or can't read it.
// -----------------------------------------------------

void read_interpreter_setup(char *prefs_name, interpreter_settings *the_settings, interpreter_midi_patching *the_patching)
{
	OSErr		err;
	SInt32		length;
	sig_struct	signature;

    CSynclavierFileReference* prefsRef = locate_interchange_prefs_file(prefs_name);
    
    err = noErr;
    
    if (!prefsRef)
        err = fnfErr;
    
    if (!err)
        err = prefsRef->Open(O_RDONLY);
    
	if (!err)											// read signature
	{
        length = sizeof(signature);						// prep for file read
		err    = prefsRef->Read(&length, &signature);
		
		if (err)
		{
			prefsRef->Close();
			prefsRef->Delete();
		}
	}
				
	if (!err)											// check signature					
	{
		if ((signature.our_id   != SIGNATURE_INFO.our_id)
        ||  (signature.rev_num  != SIGNATURE_INFO.rev_num ))
		{
			err = fnfErr;
			prefsRef->Close();
			prefsRef->Delete();
		}
	}
	
	if (!err)											// read data
	{
		length = sizeof(*the_settings);
		err    = prefsRef->Read(&length, the_settings);
        
		if (err)
		{
			prefsRef->Close();
			prefsRef->Delete();
		}
	}
			
	// Provide default settings if none available	
	if (!err)
	{
		// Read in possible MIDI patching if client is interested
		if (the_patching)
		{
			// Read signature - look for information in upper 24 bits of rev word
			length = sizeof(the_patching->midi_sig);
            err    = prefsRef->Read(&length, the_patching);
			
			if (err)
				memset(the_patching, 0, sizeof(*the_patching));
			
			else if (((the_patching->midi_sig.our_id        ) != SIGNATURE_INFO.our_id )
			||       ((the_patching->midi_sig.rev_num & 0xFF) != SIGNATURE_INFO.rev_num))
				memset(the_patching, 0, sizeof(*the_patching));
			
			// Got valid sig, with or without length
			else
			{
				// Handle old style prefs file - 16 routings
				if ((the_patching->midi_sig.rev_num >> 8) == 0)
				{
					memset(&the_patching->midi_to_synclavierx  [0], 0, sizeof(the_patching->midi_to_synclavierx  ));
					memset(&the_patching->midi_from_synclavierx[0], 0, sizeof(the_patching->midi_from_synclavierx));
					
					length = NUM_ORIG_MIDI_PATCHINGS * MIDI_PATCHING_STRING_SIZE;
                    err    = prefsRef->Read(&length, &the_patching->midi_to_synclavierx[0]);
					
					if (!err)
					{
						length = NUM_ORIG_MIDI_PATCHINGS * MIDI_PATCHING_STRING_SIZE;
                        err    = prefsRef->Read(&length, &the_patching->midi_from_synclavierx[0]);
					}
				
					if (err)
						memset(the_patching, 0, sizeof(*the_patching));
				}
				
				// Handle bogus prefs file
				else if ((the_patching->midi_sig.rev_num >> 8) != sizeof(*the_patching))
					memset(the_patching, 0, sizeof(*the_patching));
				
				// Else read new style prefs file
				else
				{
					length = sizeof(the_patching->midi_to_synclavierx);
                    err    = prefsRef->Read(&length, &the_patching->midi_to_synclavierx[0]);
					
					if (!err)
					{
						length = sizeof(the_patching->midi_from_synclavierx);
                        err    = prefsRef->Read(&length, &the_patching->midi_from_synclavierx[0]);
					}
					
					if (err)
						memset(the_patching, 0, sizeof(*the_patching));
				}
			}
		}
		
        prefsRef->Close();
		
		if (the_settings->m512k == 0)
			the_settings->m512k	= M512K_MENU_MODEL_3;			
	}
	
	else
	{
		memset(the_settings, 0, sizeof(*the_settings));
		
		if (the_patching)
			memset(the_patching, 0, sizeof(*the_patching));
		
		the_settings->cable_setting = CABLE_LENGTH_MENU_50;				// 50 foot
		the_settings->bus_setting   = BUS_LOADING_MENU_MEDIUM;			// medium
		the_settings->net_visible   = NET_VISIBLE_MENU_NOT_VISIBLE;		// not visible
		the_settings->use_mp        = USE_MP_MENU_USE_MP;				// allow use of MP if available
        the_settings->m512k			= M512K_MENU_MODEL_10;				// provide 10 M512k's
        the_settings->polymem		= POLY_MENU_MODEL_100;				// provide 100 megs poly
	}

    if (prefsRef)
        prefsRef->Release();
}

// -----------------------------------------------------
// Write config: writes settings to prefs file, thereby freeing up
// allocated memory.
// -----------------------------------------------------

void write_interpreter_setup(char *prefs_name, interpreter_settings *the_settings, interpreter_midi_patching* the_patching)
{	
	OSErr		err;									// file system error
	SInt32		length;									// used for FSWrite

    CSynclavierFileReference* prefsRef = locate_interchange_prefs_file(prefs_name);
    
    err = noErr;
    
    if (!prefsRef)
        err = fnfErr;
    
    if (!err)
        err = prefsRef->Open(O_WRONLY | O_CREAT | O_TRUNC);
    
	if (!err)											// if we could get the file open
	{
        length = sizeof(SIGNATURE_INFO);
        err    = prefsRef->Write(&length, &SIGNATURE_INFO);
				
		if (!err)
		{												// write data
			length = sizeof(*the_settings);
			err    = prefsRef->Write(&length, the_settings);
		}
		
		if (!err)										// write patching
		{
			the_patching->midi_sig = SIGNATURE_INFO;
			the_patching->midi_sig.rev_num |= (sizeof(*the_patching) << 8);
			length = sizeof(*the_patching);
			err    = prefsRef->Write(&length, the_patching);
		}
		
        prefsRef->Sync ();
        prefsRef->Close();

		if (err)										// delete partially written file
            prefsRef->Delete();
        else
            prefsRef->SetFinfo('DATA', 'SNCL');
	}
    
    if (prefsRef)
        prefsRef->Release();
}


// Random number keys
static ulong file_id_random_data[32] = {
0x2cfd0719, 0xf29711af, 0x39f76788, 0x157a3fdb, 0x5e388df1, 0xc403121f, 0xc7bd88c6, 0xa01e4dcb,
0xeaccb19f, 0x9b54cd64, 0xdeb3dcf2, 0xc227595b, 0x5b0ac749, 0x6a79a760, 0x205ff557, 0xbccbeabb,
0xc2058f69, 0x51980e63, 0x96ec7c6a, 0x51b3e97d, 0xea3ee23f, 0x91dc3cdb, 0x3c9afde1, 0xdf7ccfea,
0x61c8b76a, 0x85b85239, 0x669c31b7, 0x57c27d58, 0x4df69883, 0xa6b012c5, 0xbb8b5a41, 0xd6a67771};

static ulong module_random_data[MAX_REGISTRATION_ID] = {
0x6e778ea8, 0x28a59e67, 0xdd3a454a, 0x23d46a7d, 0x380d9b0b, 0x32936e62, 0x6e3c8004, 0xe511b8c9,
0x6ea4ca72, 0x3ce2ea56, 0x9aaf4e3e, 0xca3ae8e6, 0x2710f3e8, 0x9d31e3d4, 0xccb8f45d, 0x7adac2ed,
0x981e3238, 0xc78e4965, 0xb45ee46f, 0x8d23ecf4, 0xa34b2875, 0x4d566037, 0x4afd5e36, 0x4e7cd63e,
0x7ce921a2, 0x2162d4eb, 0xfa22124c, 0xa58ec193, 0x5bc11b28, 0x29e6f387, 0x8e005827, 0xd5c7be89,
0x9843ca81, 0xa7aa561e, 0x1b508717, 0x6c5c61f9, 0x2329adaf, 0x7ada5ac1, 0x80b5027b, 0x1ee10d18,
0xbb9f36db, 0xcd4cb031, 0xabb088a8, 0xe34193ef, 0xccf222dc, 0x2a4749ca, 0x49005521, 0x094f1299,
0x5bae0adc, 0x8bc90a43, 0x0889a58e, 0x9dc6c198, 0xea4b40cd, 0x6555757a, 0x1cc131cd, 0xdee3992e,
0xbd5206f8, 0x3432b95d, 0x1d94270e, 0x0a559397, 0x671132b1, 0x9d425d9b, 0xf6cf35d4, 0x4cd25e6b,
0x02ac49be, 0xa03bea81, 0x0a054317, 0x1b244aed, 0x06de2af9, 0xe871d6d8, 0x2bfa4c43, 0xbca34d50,
0x180e51ec, 0xc3260c9d, 0xa5860e48, 0x4f4adb19, 0xc203c544, 0xc34e54e3, 0xe008ffc5, 0x0d367854,
0x8107c07d, 0x9ad3fa90, 0xe53f2df2, 0x1bb5c407, 0x028b285f, 0x3164f44a, 0x5ab98ccc, 0xabc3d35a,
0x055ada92, 0x80b9e529, 0x20c34a19, 0xb424ac0d, 0xc137e83e, 0x3e594499, 0x3cc5b370, 0x0cdaadac,
0x3e07cb81, 0xdae4fd2a, 0x372c3f68, 0x3438b9f5};

static ulong serial_random_data[32] = {
0x827da815, 0xdee0d24b, 0x94d9497f, 0x8566ec17, 0x0240a6cd, 0x2cfcdd36, 0x94031142, 0x2866af01,
0x33927c3c, 0x31d270b9, 0xd49f8c66, 0x83a202c5, 0xb4762a2f, 0x893eb3f7, 0x144a9bb9, 0x76f6c0d5,
0xe75e0c44, 0x21194446, 0xa5b53353, 0x282daf63, 0x6f4d3f86, 0x61822df6, 0xcb7f058b, 0xa90e3390,
0x738374eb, 0x62b38c31, 0x9eb1411e, 0x3ef072f4, 0x12a13ee9, 0xb8361fb6, 0xa893f228, 0x93acb3b8};

// Compute key number for a serial number
static	ulong compute_key_number(int serial_number, int module_num)
{
	int i;
	ulong key = 0;
		
	for (i=0; i<32; i++)
		if (serial_number & (1<<i))
			key += serial_random_data[i];
			
	key += module_random_data[module_num];

	key = 1000 + (key % 99000);

	return (key);
}

// Compute serial number for this module and this prefs file
static	ulong compute_serial_number(int file_id, int module_num)
{
	int i;
	ulong serial = 0;
	
	for (i=0; i<32; i++)
		if (file_id & (1<<i))
			serial += file_id_random_data[i];
			
	serial += module_random_data[module_num];

	serial = 1000 + (serial % 99000);

	return (serial);
}


// -----------------------------------------------------
// read_registration:  Reads the registration data base
// into memory
// -----------------------------------------------------

static	OSErr read_registration (registration_data* theData)
{
	OSErr		err;
	SInt32		length;
	sig_struct	signature;

	memset(theData, 0, sizeof(*theData));
	
    CSynclavierFileReference* prefsRef = locate_interchange_prefs_file(registration_pref_file_name);
    
    err = noErr;
    
    if (!prefsRef)
        err = fnfErr;
    
    if (!err)
        err = prefsRef->Open(O_RDONLY);
    
	if (!err)											// read signature
	{
        length = sizeof(signature);						// prep for file read
		err    = prefsRef->Read(&length, &signature);
		
		if (err)
		{
			prefsRef->Close();
			prefsRef->Delete();
		}
	}
				
	if (!err)											// check signature					
	{
		if ((signature.our_id   != REGISTRATION_SIG.our_id)
		||  (signature.rev_num  != REGISTRATION_SIG.rev_num ))
		{
			err = fnfErr;
			prefsRef->Close();
			prefsRef->Delete();
		}
	}
	
	if (!err)											// read data
	{
		length = sizeof(*theData);
		err    = prefsRef->Read(&length, theData);
			
		if (err)
		{
			prefsRef->Close();
			prefsRef->Delete();
		}
	}

	if (!err)											// get file id
	{
        FSCatalogInfo	catInfo;						// Catalog info
        FSRef           anFSRef;
       
        if (!err)
            err = prefsRef->CreateFSRef(&anFSRef);

        if (!err)
            err = FSGetCatalogInfo(&anFSRef, kFSCatInfoNodeID, &catInfo, NULL, NULL, NULL);

		if (!err)
			theData->its_file_id = catInfo.nodeID;
	}
	
	if (err)
		memset(theData, 0, sizeof(*theData));
			
    if (prefsRef)
        prefsRef->Release();
    
	return (err);
}


// -----------------------------------------------------
// create_registration:  create the registration file
// -----------------------------------------------------

static	OSErr create_registration (UInt32* itsFileId)
{
	OSErr		err;									// file system error

	*itsFileId = 0;
	
    CSynclavierFileReference* prefsRef = locate_interchange_prefs_file(registration_pref_file_name);
    
    err = noErr;
    
    if (!prefsRef)
        err = fnfErr;
    
    if (!err && !prefsRef->Reachable())
        err = prefsRef->Create('MSH2', 'DATA', kIsInvisible);
    
	if (!err)
	{
        FSCatalogInfo	catInfo;						// Catalog info
        FSRef           anFSRef;
        
        if (!err)
            err = prefsRef->CreateFSRef(&anFSRef);
        
        if (!err)
            err = FSGetCatalogInfo(&anFSRef, kFSCatInfoNodeID, &catInfo, NULL, NULL, NULL);
        
		if (!err)
			*itsFileId = catInfo.nodeID;
	}
	
	return (err);
}


// -----------------------------------------------------
// write_registration:  Write the registration data base
// -----------------------------------------------------

static	OSErr write_registration (registration_data* theData)
{	
	OSErr		err;									// file system error
	SInt32		length;									// used for FSWrite
	
    CSynclavierFileReference* prefsRef = locate_interchange_prefs_file(registration_pref_file_name);
    
    err = noErr;
    
    if (!prefsRef)
        err = fnfErr;
    
    if (!prefsRef->Reachable())
		err = create_registration(&theData->its_file_id);
    
    if (!err)
        err = prefsRef->Open(O_WRONLY | O_TRUNC);
    
	if (!err)											// if we could get the file open
	{
        length = sizeof(REGISTRATION_SIG);
        err    = prefsRef->Write(&length, &REGISTRATION_SIG);
		
		if (!err)
		{												// write signature
			length = sizeof(*theData);
			err    = prefsRef->Write(&length, theData);
		}
		
        prefsRef->Sync ();
        prefsRef->Close();
        
		if (err)										// delete partially written file
            prefsRef->Delete();
	}
	
    if (prefsRef)
        prefsRef->Release();
    
	return (err);
}


// -----------------------------------------------------
// verify_registration:  see if a module is registered
// -----------------------------------------------------

int verify_registration (int which_module)
{
	OSErr				err;
	registration_data	theData;

	if (which_module < 0 || which_module >= MAX_REGISTRATION_ID)
		return (false);
	
	err = read_registration(&theData);
		
    if (err)
		return (false);
    
	// if not registered, look further
	if ((!theData.entries[which_module].is_registered)
	||  ( theData.entries[which_module].file_id != theData.its_file_id))
	{
		// Move to separate procedure to record first use...
		// Will need other routine to see if trial period expired
		#if 0
		if (theData.entries[which_module].date_first_used == 0)
		{
			// Create the file if needed
			if (theData.its_file_id == 0)
				create_registration(&theData.its_file_id);

			// Record date of first use
			theData.entries[which_module].is_registered = false;
			theData.entries[which_module].file_id       = theData.its_file_id;
			GetDateTime((ulong *) &theData.entries[which_module].date_first_used);
					
			write_registration(&theData);
		}
		#endif
				
		return (false);									// not registered
	}
	
	return (true);										// registered
}


// -----------------------------------------------------
// compute_serial_for_module:  compute serial number for this module
// -----------------------------------------------------

ulong compute_serial_for_module (int which_module)
{
	OSErr				err;
	registration_data	theData;
	
	if (which_module < 0 || which_module >= MAX_REGISTRATION_ID)
		return (0);
	
	err = read_registration(&theData);
	
	if (err)
	{
		create_registration(&theData.its_file_id);
		write_registration(&theData);
	
		err = read_registration(&theData);
	}
	
	if (err)
		return (0);

	return (compute_serial_number(theData.its_file_id, which_module));
}


// -----------------------------------------------------
// compute_key_for_serial:  compute key number a module and serial
// -----------------------------------------------------

ulong compute_key_for_serial (int which_module, ulong serial)
{
	return (compute_key_number(serial, which_module));
}


// -----------------------------------------------------
// set_registration:  attempt registration for a module
// -----------------------------------------------------

int set_registration (int which_module, ulong key_code)
{
	OSErr				err;
	registration_data	theData;

	ulong	itsSerial = compute_serial_for_module(which_module);
	ulong	itsKey    = compute_key_number       (itsSerial, which_module);
	
	if (which_module < 0 || which_module >= MAX_REGISTRATION_ID)
		return (-1);

	if (itsKey != key_code)
		return (-1);
		
	err = read_registration(&theData);
	
	if (err)
	{
		create_registration(&theData.its_file_id);
		write_registration(&theData);
	
		err = read_registration(&theData);
	}

	if (!err)
	{
		theData.entries[which_module].is_registered = true;
		theData.entries[which_module].file_id       = theData.its_file_id;
		
		err = write_registration(&theData);
	}
	
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
