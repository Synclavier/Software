/* Messages - routines for SCSI and Catalog Routine error messages */

/*	Translated to C:   	03/25/97 at 08:48	*/
/*	Translator Version:	0.000				*/

#include	"XPL.h"
#include	"messages.h"

#include	"scsilits.h"
#include	"catrtns.h"
#include	"optlits.h"

void			copy_error_message(
	char *	sou, 
	char *	des)	_recursive
{
	while (*sou)
		*des++ = *sou++;

	*des = 0;
}
	
void			add_error_message(
	char *	sou, 
	char *	des)	_recursive
{
	fixed i = 0;
	
	while (*des)
		{i++; des++;}
	
	while (*sou)
		if (i < MESSAGE_BUF_SIZE-1)
			{*des++ = *sou++; i++;}
		else
			sou++;
			
	*des = 0;
}

void			get_sense_code_message(
	fixed	code, 
	char *	message)	_recursive
	
{
	message[0] = 0;
	
	if (code > 0 && shr(code,8) != 0)
	{
		code = shr(code, 8);
		
		if 		(code == s_checkcondition)	copy_error_message((char *) "Check Condition",     message);
		else if (code == s_conditionmet  )	copy_error_message((char *) "Condition Met",       message);
		else if (code == s_busy          )	copy_error_message((char *) "Device is Busy",      message);
		else if (code == s_reserved      )	copy_error_message((char *) "Device is Reserved",  message);
		else	 							copy_error_message((char *) "Unknown SCSI status", message);
	}
		
	else 
	{
		if 		(code == s_recoverederror  )	copy_error_message((char *) "Recovered Error" ,                                    message);
		else if (code == s_notready        )	copy_error_message((char *) "Drive is Not Ready; Check Media if Removable",        message);
		else if (code == s_mediumerror     )	copy_error_message((char *) "Medium Error; The Drive cannot be read",              message);
		else if (code == s_hardwareerror   )	copy_error_message((char *) "Hardware Error; The Drive reports it is broken",      message);
		else if (code == s_illegalrequest  )	copy_error_message((char *) "Illegal Request",                                     message);
		else if (code == s_unitattention   )	copy_error_message((char *) "Unit Attention",                                      message);
		else if (code == s_dataprotect     )	copy_error_message((char *) "Write Protected; The Media is write-protected",       message);
		else if (code == s_blankcheck      )	copy_error_message((char *) "Media is Blank",                                      message);
		else if (code == s_vendorunique    )	copy_error_message((char *) "Vendor-Specific",                                     message);
		else if (code == s_copyaborted     )	copy_error_message((char *) "Copy Aborted",                                        message);
		else if (code == s_volumeoverflow  )	copy_error_message((char *) "Volume Overflow; The Media is full",                  message);
		else if (code == s_arbfailed       )	copy_error_message((char *) "Arbitration Failed; SCSI termination is faulty",      message);
		else if (code == s_selfailed       )	copy_error_message((char *) "Selection Failed; The Drive is off or not connected", message);
		else if (code == s_identfailed     )	copy_error_message((char *) "Ident Failed",                                        message);
		else if (code == s_d24notthere     )	copy_error_message((char *) "No D24 Hardware",                                     message);
		else if (code == s_badbusstate     )	copy_error_message((char *) "Bad SCSI Bus State; SCSI termination is faulty",      message);
		else if (code == s_badinitiator    )	copy_error_message((char *) "Bad Host ID",                                         message);
		else if (code == s_baddevice       )	copy_error_message((char *) "Device is not available at this time",                message);
		else if (code == s_goodconnect     )	copy_error_message((char *) "Good SCSI Status",                                    message);
		else 									copy_error_message((char *) "Unknown SCSI status",                                 message);
	}
}
	
void			get_cat_code_message(
	fixed	code, 
	char *	message)	_recursive
	
{
	message[0] = 0;
	
	if      (code == e_none       )	copy_error_message((char *) "Good Catalog Status" ,                                   message);
	else if (code == e_os         )	copy_error_message((char *) "System Catalog Error with Magic Number",                 message);
	else if (code == e_buffer     )	copy_error_message((char *) "No Catalog Buffer",                                      message);
	else if (code == e_no_dir     )	copy_error_message((char *) "Missing Directory",                                      message);
	else if (code == e_no_config  )	copy_error_message((char *) "Drive not Configured",                                   message);
	else if (code == e_no_floppy  )	copy_error_message((char *) "No Floppy in Drive",                                     message);
	else if (code == e_fcb        )	copy_error_message((char *) "System Error with FCB Number",                           message);
	else if (code == e_level      )	copy_error_message((char *) "System Eror with Device Level Specifier",                message);
	else if (code == e_storage    )	copy_error_message((char *) "No Storage Available",                                   message);
	else if (code == e_cstorage   )	copy_error_message((char *) "No Contiguous Storage Available",                        message);
	else if (code == e_dir_full   )	copy_error_message((char *) "Directory Full",                                         message);
	else if (code == e_invalid    )	copy_error_message((char *) "Invalid Directory",                                      message);
	else if (code == e_name       )	copy_error_message((char *) "Bad File Name",                                          message);
	else if (code == e_duplicate  )	copy_error_message((char *) "Duplicate File Name",                                    message);
	else if (code == e_no_file    )	copy_error_message((char *) "File Not Found",                                         message);
	else if (code == e_not_cat    )	copy_error_message((char *) "Not a Catalog",                                          message);
	else if (code == e_treename   )	copy_error_message((char *) "Bad Treename",                                           message);
	else if (code == e_no_path    )	copy_error_message((char *) "Missing Path Directory",                                 message);
	else if (code == e_type       )	copy_error_message((char *) "File Type Mismatch",                                     message);
	else if (code == e_protect    )	copy_error_message((char *) "Write Protected Media",                                  message);
	else if (code == e_too_large  )	copy_error_message((char *) "File Too Large",                                         message);
	else if (code == e_truncate   )	copy_error_message((char *) "Truncation Error",                                       message);
	else if (code == e_diskerror  )	copy_error_message((char *) "Disk Error: The Disk cannot be read",                    message);
		
	else if (code == e_bad_volume     )	copy_error_message((char *) "Bad Volume Header - Not an N.E.D. Optical Disk",     message);
	else if (code == e_bad_index   	  )	copy_error_message((char *) "Corrupt .INDEX File",                                message);
	else if (code == e_no_index       )	copy_error_message((char *) "No .INDEX file for this Volume",                     message);
	else if (code == e_volume_changed )	copy_error_message((char *) "Volume Change Error",                                message);
	else if (code == e_not_uptodate   )	copy_error_message((char *) ".INDEX File is Out-of-Date",                         message);
	else if (code == e_formatted      )	copy_error_message((char *) "Cannot Format - Volume is not blank",                message);
	else if (code == e_record_full    )	copy_error_message((char *) "Disk is Full",                                       message);
	else if (code == e_stack_full     )	copy_error_message((char *) "Out of Memory during Update",                        message);
	else if (code == e_not_initialized)	copy_error_message((char *) "Media is not N.E.D. Format; FORMAT before use",      message);
		
	else 								copy_error_message((char *) "Unknown Catalog status",                             message);
}
	
void			add_optical_sense_code_message(
	fixed	code, 
	char *	message)	_recursive
	
{
	if (code > 0 && shr(code,8) != 0)
	{
		code = shr(code, 8);
		
		if      (code == s_checkcondition)		add_error_message((char *) "",                    message);
		else if (code == s_conditionmet  )		add_error_message((char *) "",                    message);
		else if (code == s_busy          )		add_error_message((char *) "",                    message);
		else if (code == s_reserved      )		add_error_message((char *) "",                    message);
		else 									add_error_message((char *) "",                    message);
	}
		
	else 
	{
		if      (code == s_recoverederror  )	add_error_message((char *) "",                    message);
		else if (code == s_notready        )	add_error_message((char *) "",                    message);
		else if (code == s_mediumerror     )	add_error_message((char *) "",                    message);
		else if (code == s_hardwareerror   )	add_error_message((char *) "",                    message);
		else if (code == s_illegalrequest  )	add_error_message((char *) "",                    message);
		else if (code == s_unitattention   )	add_error_message((char *) "",                    message);
		else if (code == s_dataprotect     )	add_error_message((char *) "",                    message);
		else if (code == s_blankcheck      )	add_error_message((char *) "",                    message);
		else if (code == s_vendorunique    )	add_error_message((char *) "",                    message);
		else if (code == s_copyaborted     )	add_error_message((char *) "",                    message);
		else if (code == s_volumeoverflow  )	add_error_message((char *) "",                    message);
		else if (code == s_arbfailed       )	add_error_message((char *) "",                    message);
		else if (code == s_selfailed       )	add_error_message((char *) "",                    message);
		else if (code == s_identfailed     )	add_error_message((char *) "",                    message);
		else if (code == s_d24notthere     )	add_error_message((char *) "",                    message);
		else if (code == s_badbusstate     )	add_error_message((char *) "",                    message);
		else if (code == s_badinitiator    )	add_error_message((char *) "",                    message);
		else if (code == s_baddevice       )	add_error_message((char *) "",                    message);
		else if (code == s_goodconnect     )	add_error_message((char *) "",                    message);
		else 									add_error_message((char *) "",                    message);
	}
}

	
void			add_optical_cat_code_message(
	fixed	code, 
	char *	message)	_recursive
	
{
	if      (code == e_none       )		add_error_message((char *) "",                                      message);
	else if (code == e_os         )		add_error_message((char *) "",                                      message);
	else if (code == e_buffer     )		add_error_message((char *) "",                                      message);
	else if (code == e_no_dir     )		add_error_message((char *) "",                                      message);
	else if (code == e_no_config  )		add_error_message((char *) "",                                      message);
	else if (code == e_no_floppy  )		add_error_message((char *) "",                                      message);
	else if (code == e_fcb        )		add_error_message((char *) "",                                      message);
	else if (code == e_level      )		add_error_message((char *) "",                                      message);
	else if (code == e_storage    )		add_error_message((char *) "",                                      message);
	else if (code == e_cstorage   )		add_error_message((char *) "",                                      message);
	else if (code == e_dir_full   )		add_error_message((char *) "",                                      message);
	else if (code == e_invalid    )		add_error_message((char *) "",                                      message);
	else if (code == e_name       )		add_error_message((char *) "",                                      message);
	else if (code == e_duplicate  )		add_error_message((char *) "",                                      message);
	else if (code == e_no_file    )		add_error_message((char *) "; must UPDATE .INDEX file before copy", message);
	else if (code == e_not_cat    )		add_error_message((char *) "",                                      message);
	else if (code == e_treename   )		add_error_message((char *) "",                                      message);
	else if (code == e_no_path    )		add_error_message((char *) "; .INDEX subcatalog is missing",        message);
	else if (code == e_type       )		add_error_message((char *) "",                                      message);
	else if (code == e_protect    )		add_error_message((char *) "",                                      message);
	else if (code == e_too_large  )		add_error_message((char *) "",                                      message);
	else if (code == e_truncate   )		add_error_message((char *) "; .INDEX subcatalog must be enlarged",  message);
	else if (code == e_diskerror  )		add_error_message((char *) "",                                      message);
		
	else if (code == e_bad_volume     )	add_error_message((char *) "",                                      message);
	else if (code == e_bad_index   	  )	add_error_message((char *) "",                                      message);
	else if (code == e_no_index       )	add_error_message((char *) "",                                      message);
	else if (code == e_volume_changed )	add_error_message((char *) "",                                      message);
	else if (code == e_not_uptodate   )	add_error_message((char *) "",                                      message);
	else if (code == e_formatted      )	add_error_message((char *) "",                                      message);
	else if (code == e_record_full    )	add_error_message((char *) "",                                      message);
	else if (code == e_stack_full     )	add_error_message((char *) "",                                      message);
	else if (code == e_not_initialized)	add_error_message((char *) "",                                      message);
		
	else 								add_error_message((char *) "",                                      message);
}
