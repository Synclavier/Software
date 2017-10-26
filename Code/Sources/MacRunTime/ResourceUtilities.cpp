/* Resource Utilities.c */

// Mac OS
#include <CoreServices/CoreServices.h>

//	Local Includes
#include "ResourceUtilities.h"

// Resource utilities
void	replace_1_resource		(Handle the_resource, short res_file_refnum,
								 OSType res_type, int res_id, const unsigned char * res_name)
{
	short 	cur_res_file      = CurResFile();
	Handle	existing_resource = NULL;

	UseResFile(res_file_refnum);
	
	existing_resource = Get1Resource(res_type, res_id);
	
	while (existing_resource)
	{
		RemoveResource(existing_resource);
		existing_resource = Get1Resource(res_type, res_id);
	}
	
	AddResource   (the_resource, res_type, res_id, res_name);
	UpdateResFile (res_file_refnum);
	UseResFile    (cur_res_file);
}

void	delete_1_resource		(short res_file_refnum,
								 OSType res_type, int res_id)
{
	short 	cur_res_file      = CurResFile();
	Handle	existing_resource = NULL;

	UseResFile(res_file_refnum);
	
	existing_resource = Get1Resource(res_type, res_id);
	
	while (existing_resource)
	{
		RemoveResource(existing_resource);
		existing_resource = Get1Resource(res_type, res_id);
	}

	UpdateResFile (res_file_refnum);
	UseResFile    (cur_res_file);
}
