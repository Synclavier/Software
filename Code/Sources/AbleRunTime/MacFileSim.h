/*  macscsiosx.h                                                            */

/*  Created:                                                                */
/*      06/18/96        C. Jones                                            */

/*  Contents:                                                               */
/*      Header for use of MAC SCSI Simulation Environment					*/


#ifndef mac_scsi_file_sim_h                			/* include ourselves only once!	*/
#define mac_scsi_file_sim_h

#include    "ScsiLib.h"

// Entry for simulated file access
chip_error_code issue_mac_file_sim_scsi_command  	(scsi_device& the_device, scsi_command& the_command);
chip_error_code	initialize_mac_file_sim_scsi_chip	();

// Callback for detecting file access
typedef	void (*MacFileSimCallback)(void* objref, CFURLRef url, bool didWrite);

extern  MacFileSimCallback  mac_file_sim_sequence_file_accessed_proc;
extern  void*               mac_file_sim_sequence_file_accessed_arg;

#endif
