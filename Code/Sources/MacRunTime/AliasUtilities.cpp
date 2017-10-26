/* AliasUtilities.c */

//	Std C Includes
#include <String.h>

// Mac OS
#include <CoreServices/CoreServices.h>

//	Local Includes
#include "AliasUtilities.h"

// ¥ AliasUtilities_Get_Application

CSynclavierFileReference*   AliasUtilities_Get_Application()
{
    return CSynclavierFileReference::CopyAppFileRef();
}


// ¥ AliasUtilities_DescendsFrom(FSSpec& parent, FSSpec& possibleChild)

int	AliasUtilities_DescendsFrom(SyncFSSpec& parent, SyncFSSpec& possibleChild, int numLevels)
{
    #if __LP64__
        CSynclavierFileReference* parentRef = parent.file_ref;
        CSynclavierFileReference* childRef  = possibleChild.file_ref;
    
        if (!parentRef || !childRef)
            return false;
    
        // Compare using absolute posix paths
        CFStringRef        parentPath = parentRef->GetPath();
        CFStringRef        childPath  = childRef ->GetPath();
        CFMutableStringRef fullPath   = CFStringCreateMutableCopy(NULL, 0, parentPath);
    
        if (!parentPath || !childPath || !fullPath)
            return false;
    
        CFStringAppend(fullPath, CFSTR("/"));
    
        CFRange range = CFStringFind(childPath, fullPath, 0);
    
        CFRelease(fullPath);
    
        return (range.location == 0);
    #else
        FSSpec aSpec = possibleChild;

        // Different volume: no
        if (parent.vRefNum != possibleChild.vRefNum)
            return false;
            
        // Track up lineage
        if (possibleChild.parID == fsRtParID)					// if child is volume, it can't descend from anything
            return false;
                
        while (1)
        {
            // Get spec for parent of this possible child
            if (AliasUtilities_GetParentSpec(&aSpec, &aSpec))
                break;
                
            if (aSpec.parID == parent.parID)					// parent of child is same directory as passed parent
            {
                int i;
                
                if (aSpec.name[0] != parent.name[0])			// different name length; must be different
                    return false;

                for (i=1; i<= aSpec.name[0]; i++)				// name doesn't match
                    if (aSpec.name[i] != parent.name[i])
                        return false;
                        
                return true;									// yes; parent of passed child is equal to passed parrent
            }
                
            if (aSpec.parID == fsRtParID)						// done if got to root
                break;
                
            // Could limit the number of levels we look at
            if (numLevels && --numLevels == 0)
                break;
        }
    #endif

	return false;
}


// Alias utility: Compute full path name from FSSpec

void	AliasUtilities_GetFullPath(SyncFSSpec* mySpec, char* the_name, int max_len)
{
    if (max_len == 0)										// check for no length...
        return;
    
    memset(the_name, 0, max_len);
    
    #if __LP64__
        CSynclavierFileReference* itsRef = mySpec->file_ref;
    
        if (!itsRef)
            return;
    
        if (!itsRef->GetURL())
            return;
    
        if (!itsRef->GetPath())
            return;
    
        itsRef->Path(the_name, max_len);
    #else
        char		name_list[512];
        char*		name_ptrs[256];
        int			num_names = 0;
        int			name_ptr  = 0;
        FSSpec		aSpec     = *mySpec;
        
        if (aSpec.parID == fsRtParID)							// if is volume...
        {
            p2cstr(aSpec.name);									// get name in C format
            strcpy(the_name, (char *) aSpec.name);	
            c2pstr((char *) aSpec.name);						// revert to p string
            return;
        }
        
        p2cstr(aSpec.name);										// get name in C format
        
        if (strlen((char *) aSpec.name) >= sizeof(name_list))	// file name itself too long
        {
            c2pstr((char *) aSpec.name);						// revert to p string
            return;
        }
        
        strcpy(&name_list[name_ptr], (char *) aSpec.name);		// last name will be file name
        name_ptrs[num_names++] = &name_list[name_ptr];			// save poitner to it
        name_ptr += (strlen((char *) aSpec.name) + 1);			// save room
        c2pstr((char *) aSpec.name);							// revert to p string
        
        while (1)
        {
            // Get spec for parent
            if (AliasUtilities_GetParentSpec(&aSpec, &aSpec))
                return;
        
            p2cstr(aSpec.name);									// get in c string
        
            if (name_ptr + strlen((char *) aSpec.name) + 1 >= max_len)
                return;
        
            if (name_ptr + strlen((char *) aSpec.name) + 1 >= sizeof(name_list))
                return;
        
            if (num_names >= 256)								// limit to reasonable limit
                return;
                
            strcpy(&name_list[name_ptr], (char *) aSpec.name);	// store directoryname
            name_ptrs[num_names++] = &name_list[name_ptr];		// store pointer
            name_ptr += (strlen((char *) aSpec.name) + 1);		// advance storage
            
            c2pstr((char *) aSpec.name);
        
            if (aSpec.parID == fsRtParID)
                break;
        }
        
        if (name_ptr >= max_len)							// won't be room
            return;
            
        while (num_names--)									// create path name in reverse order
        {
            strcat(the_name, name_ptrs[num_names]);

            if (num_names)
                strcat(the_name,":");
        }
    #endif
}


// Alias utility: Resolve an alias to a full path name, or delete the alias.
// We fill in an FSSpec with the result of the resove.
// For the 64-bit builds we create a new CSynclavierFileReference with which to access the file.

#if !__LP64__
    static  CFURLRef applicationURLRef = NULL;
    static  FSRef    applicationFSRef;

    OSErr	AliasUtilities_Extract_Full_Name(AliasHandle *the_alias, SyncFSSpec *the_spec, char *full_path_name, int max_len)
    {
        AliasHandle it;
        Boolean		wasChanged;
        Boolean 	wasAliased;
        Boolean 	isFolder;
        OSErr		status;
        FSRef       aFSRef;

        // Get main app fsref for alias resolving first time through
        if (applicationURLRef == NULL) {
            CFBundleRef apBundle = CFBundleGetMainBundle();             // Does not need to be release
        
            applicationURLRef = CFBundleCopyBundleURL(apBundle);        // Does need to be released; but we keep it
        
            if (applicationURLRef)
                CFURLGetFSRef(applicationURLRef, &applicationFSRef);    // Get app fsref handy for alias resolving
        }
        
        memset(full_path_name, 0, max_len);						// assume no name available
        
        if (!the_alias)											// dead wrong; supposed to pass pointer
            return (noErr);

        if ((it = *the_alias) == NULL)							// if no alias, done
            return (noErr);

        if ((status = FSResolveAlias(&applicationFSRef, it, &aFSRef, &wasChanged)) != noErr)
        {
            DisposeHandle( (Handle) it);						// free up alias storage
            *the_alias = 0;										// null out users pointer
            return (status);									// done
        }
        
        if ((status = FSResolveAliasFile(&aFSRef, true, &isFolder, &wasAliased)) != noErr)
        {
            DisposeHandle( (Handle) it);						// free up alias storage
            *the_alias = 0;										// null out users pointer
            return (status);									// done
        }
        
        // For 64-bit builds fabricate the SyncFSSpec struct
        #if __LP64__
            CSynclavierFileReference* newRef = new CSynclavierFileReference(&aFSRef);
        
            newRef->CreateFSSpec(the_spec);
        
            newRef->Release();
        #else
            if (FSGetCatalogInfo(&aFSRef, 0, NULL, NULL, the_spec, NULL) != noErr)
                return fnfErr;
        #endif
        
        AliasUtilities_GetFullPath(the_spec, full_path_name, max_len);
        
        return (noErr);
    }
        

    // =================================================================================
    //		¥ AliasUtilities_GetParentID()
    //		¥ AliasUtilities_Get_Application_FSSpec()
    // =================================================================================

    // Gets directory ID of parent.
    // Obsolete; not used in 64-bit builds
    OSErr	AliasUtilities_GetParentSpec(FSSpec *the_spec, FSSpec *parent_spec)
    {
        FSRefParam	myPB;
        FSRef		aRef;
        FSRef		pRef;
        OSErr		status;
        
        memset(&myPB, 0, sizeof(myPB));
        memset(&aRef, 0, sizeof(aRef));
        memset(&pRef, 0, sizeof(pRef));
        
        if (!the_spec)
            return paramErr;
        
        // If file does not exist use '.' to get information on directory itself
        if ((status = FSpMakeFSRef(the_spec, &aRef)) != 0)
        {
            FSSpec aSpec;
            
            aSpec.vRefNum = the_spec->vRefNum;
            aSpec.parID   = the_spec->parID;
            aSpec.name[0] = 1;
            aSpec.name[1] = '.';
            
            if ((status = FSpMakeFSRef(&aSpec, &pRef)) != 0)
                return status;
        }
        
        // Else if file does exist, get FSRef to its parent
        else
        {
            myPB.ref		  = &aRef;
            myPB.parentRef    = &pRef;

            if ((status = PBGetCatalogInfoSync(&myPB)) != 0)
                return status;
        }

        // Zero out output spec in here in case it is the same as the input spec
        if (parent_spec)
            memset(parent_spec, 0, sizeof(*parent_spec));
            
        memset(&myPB, 0, sizeof(myPB));
        myPB.ref		  = &pRef;
        myPB.spec		  = parent_spec;
        
        if ((status = PBGetCatalogInfoSync(&myPB)) != 0)
            return status;
                
        return noErr;
    }

    // Obsolete; not used in 64-bit builds
    // USE CFBundleRef
    OSErr	AliasUtilities_Get_Application_FSSpec(FSSpec *the_spec)
    {
        ProcessInfoRec 	info;
        OSErr			status;
        
        memset(&info, 0, sizeof(info));
        
        info.processInfoLength = sizeof(info);
        
        if ((status = GetCurrentProcess(&info.processNumber)) != 0)
            return (status);
        
        info.processAppSpec = the_spec;
        
        if ((status = GetProcessInformation(&info.processNumber, &info)) != 0)
            return (status);
        
        // Bump up to bundle level from Contents. Ugh.
        status = AliasUtilities_GetParentSpec(the_spec, the_spec);
        
        if (!status)
            status = AliasUtilities_GetParentSpec(the_spec, the_spec);
        
        if (!status)
            status = AliasUtilities_GetParentSpec(the_spec, the_spec);
        
        return (status);
    }
#endif
