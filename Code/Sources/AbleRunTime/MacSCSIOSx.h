/*  macscsiosx.h                                                            */

/*  Created:                                                                */
/*      06/18/96        C. Jones                                            */

/*  Contents:                                                               */
/*      Header for use of MAC SCSI Simulation Environment					*/


#ifndef mac_scsi_osx_h                			/* include ourselves only once!	*/
#define mac_scsi_osx_h

#include    "SCSILib.h"

// Handy struct to encapsulate information about a block storage device connected to mac.
// Implemented for RATOC FR1x

typedef struct	BSD_Block_Device
{  
	int         entry_valid;					// True if valid info; e.g. device exsists
	int         bsd_file_id;					// file id for read/write/lseek
	int         unt_id;							// simulated unit id
	int         num_blocks;						// number of blocks on device
	int         block_size;						// size of block
	char		bsd_path[32];					// bsd path for open
	char		device_info[128];				// derived from object name - presented to user
												
}   BSD_Block_Device;

extern	BSD_Block_Device	bsd_simulated_disks[7];

// Entry for mac scsi port access
chip_error_code issue_mac_scsi_port_scsi_command  	(scsi_device& the_device, scsi_command& the_command);
chip_error_code	initialize_mac_scsi_port_scsi_chip	();
chip_error_code	finalize_mac_scsi_port_scsi_chip	();

#endif
