/* Alias Utilities.h */

#ifndef	ALIAS_UTILITIES_H

#include "SynclavierFileReference.h"

CSynclavierFileReference*   AliasUtilities_Get_Application();   // Main bundle; release when through

int     AliasUtilities_DescendsFrom			  (SyncFSSpec& parent, SyncFSSpec& possibleChild, int numLevels);
void	AliasUtilities_GetFullPath            (SyncFSSpec* mySpec, char* the_name, int max_len);

#if !__LP64__
    OSErr	AliasUtilities_Extract_Full_Name      (AliasHandle *the_alias, SyncFSSpec *the_spec, char *full_path_name, int max_len);
    OSErr	AliasUtilities_Get_Application_FSSpec (FSSpec *the_spec);
    OSErr	AliasUtilities_GetParentSpec		  (FSSpec *the_spec, FSSpec *parent_spec);
#endif

#endif