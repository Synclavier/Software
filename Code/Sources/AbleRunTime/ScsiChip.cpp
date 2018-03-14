//  ScsiChip.cpp

/*  Contents:                                                               */
/*      Routine provides simulation of SCSI hard disk using file system		*/

// Our inclues
#include    "ScsiChip.h"
#include    "ScsiLib.h"
#include    "Utility.h"
#include    "XPL.h"
#include    "MacSCSIOSx.h"
#include    "D24Sim.h"
#include    "MacFileSim.h"

// Mac inculdes
#include 	<string.h>
#include	<stdlib.h>
#include	<stdio.h>

// Branch on simulated VS real scsi port
chip_error_code initialize_scsi_chip()
{
	initialize_mac_scsi_port_scsi_chip();
	initialize_mac_file_sim_scsi_chip ();
	initialize_d24_scsi_port_scsi_chip();
	
	return (CHIP_GOOD_STATUS);  
}

chip_error_code issue_scsi_command  (scsi_device& the_device, scsi_command& the_command)
{
	if (the_device.fDevicePort == SIMULATED_PORT && the_device.fFRefNum != 0)
		return (issue_mac_file_sim_scsi_command  (the_device, the_command));
	
	else if (the_device.fDevicePort == MAC_SCSI_PORT && the_device.fFRefNum == 0)
		return (issue_mac_scsi_port_scsi_command (the_device, the_command));

	else if (the_device.fDevicePort == D24_SCSI_PORT && the_device.fFRefNum == 0)
		return (issue_d24_scsi_port_scsi_command (the_device, the_command));

	else
		return (CHIP_BAD_STATUS);
}
