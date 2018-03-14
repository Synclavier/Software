/*  file:   ScsiChip.h  */
/*  author: C. Jones    */

/*  date:   03/26/96    */

/*  Modification History:               */
/*  06/18/96 - C. Jones - Rev 0.000     */

// This file defines constants and prototypes around emulating SCSI commands
// using the mac file system, a real D24 card, or a Macintosh SCSI card


#ifndef 	scsichip_h          /* include ourselves only once!             */
#define 	scsichip_h

#include    "Standard.h"
#include    "ScsiLib.h"

// Generic Entry
chip_error_code issue_scsi_command  				(scsi_device& the_device, scsi_command& the_command);
chip_error_code	initialize_scsi_chip				();

#endif
