/*  scsilib.c                                                               */

/*  Created:                                                                */
/*      06/18/96        C. Jones                                            */

/*  Contents:                                                               */
/*      Routines to issue individual SCSI commands                          */

#include    "SCSILib.h"
#include    "Utility.h"
#include    "ScsiChip.h"
#include    "XPL.h"
#include    "XPLRuntime.h"
#include    "catrtns.h"
#include    "SynclavierFileReference.h"

#include	<StdIO.h>

#define		PRINT_ACTIVITY		g_scsi_print_basic_opt
#define		PRINT_ALL_ACTIVITY	g_scsi_print_all_opt


/*------------------------------------------------------------------------- */
/*Segmentizer Utilities:                                                    */
/*------------------------------------------------------------------------- */

scsi_segment* scsi_segment_for_range(scsi_segmentizer& izer, uint32 blockStart, uint32 blockLength)
{
    int i = 0;
    
    while (i<MAX_SCSI_SEGMENTS) {
        scsi_segment& seg = izer.fSegList[i];
        
        // List must be in order by location on the device.
        // No URLRef means end of list
        if (seg.fSegURLRef == NULL)
            break;
        
        if ((blockStart               >= seg.fSegStartOnDevice)
        &&  (blockStart + blockLength <= seg.fSegStartOnDevice + seg.fSegMaxBlocks)) {
            // File is in memory. Implementation requires entire file to be in memory.
            if (seg.fSegStash)
                return &seg;
            
            if (!seg.fSegFileRef) {
                // Note - this transfers ownership of the CFURLRef to the
                // file reference.
                seg.fSegFileRef = new CSynclavierFileReference(seg.fSegURLRef);
                
                // Failed...
                if (!seg.fSegFileRef)
                    break;
                
                // Retain the URLRef since fSegFileRef now owns the one we had
                CFRetain(seg.fSegURLRef);
            }
            
            // If not open, open it
            if (seg.fSegFileRef->GetFile() == 0)
            {
                seg.fSegFileRef->Resolve();
                
                seg.fSegFileRef->Open(O_RDWR);
                
                // Try for read only
                if (seg.fSegFileRef->GetFile() == 0)
                    seg.fSegFileRef->Open(O_RDONLY);
                
                if (seg.fSegFileRef->GetFile() == 0) {
                    // Allow segment to be returned if allows creation or synthesizes zeroes; otherwise failure; segmented file not available
                    if (!seg.fSegAllowsCreate && !seg.fSegSynthsZeroes)
                        break;
                }
            }
            
            return &seg;
        }
        
        i++;
    }
    
    return NULL;
}

/*------------------------------------------------------------------------- */
/*Global Variables:                                                         */
/*------------------------------------------------------------------------- */

boolean     g_unit_attention       = false; /* unit attention received      */
boolean		g_scsi_print_basic_opt = false; /* nonzero to print output      */
boolean		g_scsi_print_all_opt   = false; /* nonzero to print output      */
boolean		g_recognize_disks      = false; /* recognize able disks always  */

SyncMutex   SyncScsiSegmentizerMutex;

#if SYNC_USE_ASYNC_IO
    void (*SCSILib_yielder) () = NULL;
    long   SCSILib_yield_count = 0;
#endif


/**------------------------------------------------------------------------ */
/* Basic SCSI Commands: issue_inquiry                                       */
/*------------------------------------------------------------------------- */

/* Issue inquiry command to scsi device */

scsi_error_code issue_inquiry(scsi_device *the_device)
{
	scsi_command    the_command;
	chip_error_code chip_status;
	scsi_error_code scsi_status;
	

	/* issue inquiry command:       */

	while (1)   /* loop for retry   */
	{
		the_command.fWCommand[0] = INQUIRY << 8;
		the_command.fWCommand[1] = 0;
		the_command.fWCommand[2] = (sizeof(standard_inquiry_data)) << 8;
	
		the_command.fNumCommandBytes = 6;
		the_command.fData            = (byte *) &the_device->fStandardInquiryData;
		the_command.fNumDataBytes    = sizeof(the_device->fStandardInquiryData);
		the_command.fDataDirection   = DATA_IN_DIRECTION;
		
		chip_status = issue_scsi_command(*the_device, the_command);
	

		/* check status returned by the chip driver: */                    
		
		if (chip_status == CHIP_NO_RESPONSE)
			return (NO_RESPONSE);
			
		if (chip_status == CHIP_BUSY_STATUS)
			return (DEVICE_BUSY_OCCURED);
				
		if (chip_status)            				/* better be a good status from driver  */
		{                           				/* else print.  Basically, we skip the  */
			if (PRINT_ACTIVITY)   					/* print in the NO_RESPONSE case...     */
				printf("SCSI: Inquiry failed for SCSI Id %d (%d)", the_device->fTargetId, chip_status);
			
			return (BAD_STATUS);
		}
		
		
		/* status is good; now check fScsiStatus                            */
		
		if (the_command.fScsiStatus == RESERVATION_CONFL)
		{
			if (PRINT_ACTIVITY)
				printf("SCSI: Inquiry says device is reserved: SCSI Id %d\n", the_device->fTargetId);
			
			return (DEVICE_RESERVED);
		}
		
		if (the_command.fScsiStatus == CHECK_CONDITION) /* check condition  */
		{
			scsi_status = issue_request_sense(the_device);
			
			if (scsi_status == RETRY_COMMAND)           /* allow retry      */
				continue;
			
			if ((scsi_status != (scsi_error_code) GOOD_SCSI_STATUS)       /* if not good      */
			&&  (scsi_status != NOT_AVAILABLE   ))      /* and not un-ready */
			{
				if (PRINT_ACTIVITY)
					printf("SCSI: Inquiry error: bad sense: SCSI Id %d %d\n", the_device->fTargetId, scsi_status);
				
				return (scsi_status);
			}
			
			scsi_status = (scsi_error_code) GOOD_SCSI_STATUS;

			/*==> else fall through with good scsi_status and good fScsiStatus!! */
		}
		
		if (the_command.fScsiStatus || (the_command.fNumBytesXferd < 2))
		{
			if (PRINT_ACTIVITY)
				printf("SCSI: Inquiry error: failed: SCSI Id %d (%d %d)\n", the_device->fTargetId, the_command.fScsiStatus, the_command.fNumBytesXferd);
			
			return (BAD_STATUS);
		}
		
		if (PRINT_ACTIVITY)
		{
			standard_inquiry_data *inq_data = &the_device->fStandardInquiryData;
			
			printf("\nSCSI: Inquiry data: SCSI Id %d\n", the_device->fTargetId);
			
			inq_data->terminator[0] = 0;
			
			printf("      vendor, product, revision    : %s\n", (char *)&inq_data->vendor  [0]);

			if (PRINT_ALL_ACTIVITY)
			{
				printf("      pqualifier: %d\n", inq_data->pqualifier );
				printf("      ptype     : %d\n", inq_data->ptype      );
				printf("      rmb       : %d\n", inq_data->rmb        );
				printf("      modifier  : %d\n", inq_data->modifier   );
				printf("      isovers   : %d\n", inq_data->isovers    );
				printf("      ecmavers  : %d\n", inq_data->ecmavers   );
				printf("      ansivers  : %d\n", inq_data->ansivers   );
				printf("      response  : %d\n", inq_data->response   );
				printf("      length    : %d\n", inq_data->length     );
				printf("      reladr    : %d\n", inq_data->reladr     );
				printf("      wbus32    : %d\n", inq_data->wbus32     );
				printf("      wbus16    : %d\n", inq_data->wbus16     );
				printf("      sync      : %d\n", inq_data->sync       );
				printf("      linked    : %d\n", inq_data->linked     );
				printf("      queing    : %d\n", inq_data->queing     );
				printf("      softreset : %d\n", inq_data->softreset  );
			}
		}
			
		return (GOOD_STATUS);
	}
}


/**------------------------------------------------------------------------ */
/* Basic SCSI Commands: issue_request_sense                                 */
/*------------------------------------------------------------------------- */

/* Issue request sense command to scsi device                   */
/* note: issue_request_sense uses several variables             */
/* in the device object itself.  In particular, the command     */
/* bytes, the command byte length, and the scsi status field    */
/* are replaced by the request sense command.                   */
/* The number of bytes transfered is preserved in case the      */
/* sense key indicates 'recovered' error and the calling        */
/* routine wishes to proceed.                                   */

scsi_error_code issue_request_sense(scsi_device *the_device)
{
	scsi_command    the_command;
	chip_error_code chip_status;
	byte            sense_code;
	
	zero_mem((byte *)&the_device->fRequestSenseData,
			 sizeof(the_device->fRequestSenseData));    /* init to zeros    */
	
	
	/* issue request sense */
	
	the_command.fWCommand[0] = REQUEST_SENSE << 8;        /* set up request sense command */
	the_command.fWCommand[1] = 0;
	the_command.fWCommand[2] = (sizeof(request_sense_data)) << 8;

	the_command.fNumCommandBytes = 6;
	the_command.fData            = (byte *) &the_device->fRequestSenseData;
	the_command.fNumDataBytes    = sizeof(the_device->fRequestSenseData);
	the_command.fDataDirection   = DATA_IN_DIRECTION;
	
	chip_status = issue_scsi_command(*the_device, the_command);
								  
	if (chip_status)               /* better be a good status from SCSI    */
	{
		if (PRINT_ACTIVITY)
			printf("\nSCSI: Request Sense failed for SCSI Id %d (%d)\n", the_device->fTargetId, chip_status);
		
		if (chip_status == CHIP_BUSY_STATUS)
			return (DEVICE_BUSY_OCCURED);
				
		return (BAD_STATUS);
	}
	
	if (( the_command.fNumBytesXferd < 3)
	||  ((the_command.fScsiStatus != GOOD_SCSI_STATUS)
	&&   (the_command.fScsiStatus != CHECK_CONDITION )))
	{
		if (PRINT_ACTIVITY)
			printf("\nSCSI: Request Sense error: failed: SCSI Id %d (%d %d)\n", the_device->fTargetId, the_command.fScsiStatus, the_command.fNumBytesXferd);
		
		return (BAD_STATUS);
	}

	the_command.fScsiStatus = GOOD_SCSI_STATUS; 
	

	/* examine SCSI sense code: */
	
	sense_code = the_device->fRequestSenseData.sense_key;   /* get key      */
		
	if (sense_code != NO_SENSE)     /* print out any sensing done for now   */
	{
		request_sense_data *sense_data = &the_device->fRequestSenseData;
		
		if (PRINT_ALL_ACTIVITY)
		{
			printf("\nSCSI: RequestSense SCSI Id %d\n", the_device->fTargetId);
			
			printf("      valid              : 0x%02X\n", sense_data->valid              );
			printf("      error_code         : 0x%02X\n", sense_data->error_code         );
			printf("      segment_number     : 0x%02X\n", sense_data->segment_number     );
			printf("      filemark           : 0x%02X\n", sense_data->filemark           );
			printf("      eom                : 0x%02X\n", sense_data->eom                );
			printf("      ili                : 0x%02X\n", sense_data->ili                );
			printf("      sense_key          : 0x%02X\n", sense_data->sense_key          );
			printf("      additional_length  : 0x%02X\n", sense_data->additional_length  );
			printf("      additional_code    : 0x%02X\n", sense_data->additional_code    );
			printf("      additional_quifier : 0x%02X\n", sense_data->additional_quifier );
			printf("      scsi_id            : 0x%02X\n", sense_data->scsi_id            );
			printf("      lun                : 0x%02X\n", sense_data->lun                );
			printf("      cdb_opcode         : 0x%02X\n", sense_data->cdb_opcode         );
            
            fflush(stdout);
		}
	}
	
	
	/* handle sense codes here: */
	
	if (sense_code == NO_SENSE)             /* no sense is good sense       */
		return (GOOD_STATUS);
	
	if (sense_code == RECOVERRED_ERROR)     /* device recovered error       */
		return (GOOD_STATUS);
	
	if (sense_code == NOT_READY)            /* device not ready             */
		return (NOT_AVAILABLE);
	
	if (sense_code == UNIT_ATTENTION)       /* unit attention               */
	{
		g_unit_attention = TRUE;            /* log it for now               */
		the_device->fUnitAttention = TRUE;
		the_device->fUnitAttnCount++;

		return (RETRY_COMMAND);             /* and allow a retry            */
	}
	
	if (sense_code == MEDIUM_ERROR)
		return (MEDIUM_ERROR_OCCURED);
		
	if (sense_code == HARDWARE_ERROR)
		return (HARDWARE_ERROR_OCCURED);
		
	if (sense_code == BLANK_CHECK)
		return (BLANK_CHECK_OCCURED);
		
	return (BAD_STATUS);                    /* cannot handle other status   */
}


/**------------------------------------------------------------------------ */
/* Basic SCSI Commands: issue_test_ready                                    */
/*------------------------------------------------------------------------- */

/* Issue test ready command to scsi device */

scsi_error_code issue_test_ready(scsi_device *the_device)
{
	scsi_command    the_command;
	chip_error_code chip_status;
	scsi_error_code scsi_status;
	
	/* issue test unit ready */
	
	while (1)   /* loop for retry   */
	{
		the_command.fWCommand[0] = TEST_UNIT_READY << 8;      /* set up command           */
		the_command.fWCommand[1] = 0;
		the_command.fWCommand[2] = 0;

		the_command.fNumCommandBytes = 6;
		the_command.fData            = 0;
		the_command.fNumDataBytes    = 0;
		the_command.fDataDirection   = DATA_OUT_DIRECTION;
		
		chip_status = issue_scsi_command(*the_device, the_command);
									  
		/* check status returned by the chip driver: */                    
		
		if (chip_status == CHIP_NO_RESPONSE)
			return (NO_RESPONSE);
			
		if (chip_status)            /* better be a good status from driver  */
		{
			if (PRINT_ACTIVITY)
				printf("\nSCSI: Test Ready failed for SCSI Id %d (%d)\n", the_device->fTargetId, chip_status);
			
			if (chip_status == CHIP_BUSY_STATUS)
				return (DEVICE_BUSY_OCCURED);
				
			return (BAD_STATUS);
		}
		
		
		/* Now check the SCSI status itself */
			
		if (the_command.fScsiStatus == RESERVATION_CONFL)
			return (DEVICE_RESERVED);
		
		if (the_command.fScsiStatus == CHECK_CONDITION) /* check condition  */
		{
			scsi_status = issue_request_sense(the_device);
			
			if (scsi_status == RETRY_COMMAND)           /* allow retry      */
				continue;                               /* after chk cond   */
			
			if (scsi_status)                            /* most likely not  */
				return (scsi_status);                   /* available        */
			
			/*==> else fall through with good scsi_status and good fScsiStatus!! */
		}
		
		if (the_command.fScsiStatus == BUSY)
			return (DEVICE_BUSY_OCCURED);
		
		if (the_command.fScsiStatus || (the_command.fNumBytesXferd != 0))
			return (BAD_STATUS);
		
		if (PRINT_ALL_ACTIVITY)
			printf("\nSCSI: Device is ready: SCSI Id %d\n", the_device->fTargetId);
	
		return (GOOD_STATUS);
	}
}


/**------------------------------------------------------------------------ */
/* Basic SCSI Commands: issue_mode_select                                   */
/*------------------------------------------------------------------------- */

/* Issue mode select command to scsi device */

scsi_error_code issue_mode_select(scsi_device    *the_device,
								  byte           *data_pointer,
								  uint32          num_bytes)
{
	scsi_command    the_command;
	chip_error_code chip_status;
	scsi_error_code scsi_status;

	/* Access IDE drive: */

	if (the_device->fDevicePort == MAC_IDE_PORT)
		return (BAD_STATUS);

	/* Else access SCSI drive: */

	while (1)   /* loop for retry   */
	{
		the_command.fWCommand[0] = MODE_SELECT << 8;
		the_command.fWCommand[1] = 0;
		the_command.fWCommand[2] = num_bytes << 8;
			
		the_command.fNumCommandBytes = 6;
		the_command.fData            = data_pointer;
		the_command.fNumDataBytes    = num_bytes;
		the_command.fDataDirection   = DATA_OUT_DIRECTION;

		chip_status = issue_scsi_command(*the_device, the_command);
	
        if (PRINT_ACTIVITY) {
            printf("\nSCSI: Mode Select status for SCSI Id %d (%d)\n", the_device->fTargetId, chip_status);
            printf("0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
                   data_pointer[0], data_pointer[1], data_pointer[2 ], data_pointer[3 ], data_pointer[4 ], data_pointer[5 ], data_pointer[6 ], data_pointer[7 ],
                   data_pointer[8], data_pointer[9], data_pointer[10], data_pointer[11], data_pointer[12], data_pointer[13], data_pointer[14], data_pointer[15]);
        }

        /* check status returned by the chip driver: */
		
		if (chip_status == CHIP_NO_RESPONSE)
			return (NO_RESPONSE);
			
		if (chip_status == CHIP_BUSY_STATUS)
			return (DEVICE_BUSY_OCCURED);
				
		if (chip_status)            /* better be a good status from driver  */
			return (BAD_STATUS);
		
		
		/* status is good; now check fScsiStatus                            */
		
		if (the_command.fScsiStatus == RESERVATION_CONFL)
			return (DEVICE_RESERVED);
		
		if (the_command.fScsiStatus == CHECK_CONDITION) /* check condition  */
		{
			scsi_status = issue_request_sense(the_device);
			
			if (scsi_status == RETRY_COMMAND)           /* allow retry      */
				continue;
			
			if (scsi_status)                            /* other prob       */
				return (scsi_status);
			
			/*==> else fall through with good scsi_status and good fScsiStatus!! */
		}
		
		if ((the_command.fScsiStatus)                   
		||  (the_command.fNumBytesXferd != num_bytes))
			return (BAD_STATUS);
		
		return (GOOD_STATUS);
	}
 }
 

/**------------------------------------------------------------------------ */
/* Basic SCSI Commands: issue_search_empty_blocks                           */
/*------------------------------------------------------------------------- */

/* Issue search-empty-blocks */

scsi_error_code issue_search_empty_blocks(scsi_device    *the_device,
								          byte           *data_pointer,
								          uint32          num_bytes)
{
	scsi_command    the_command;
	chip_error_code chip_status;
	scsi_error_code scsi_status;

	/* Access IDE drive: */

	if (the_device->fDevicePort == MAC_IDE_PORT)
		return (BAD_STATUS);

	/* Else access SCSI drive: */

	while (1)   /* loop for retry   */
	{
		the_command.fWCommand[0] = SEARCH_EMPTY_BLOCKS << 8;
		the_command.fWCommand[1] = 0;
		the_command.fWCommand[2] = 0;
			
		the_command.fNumCommandBytes = 6;
		the_command.fData            = data_pointer;
		the_command.fNumDataBytes    = num_bytes;
		the_command.fDataDirection   = DATA_OUT_DIRECTION;

		chip_status = issue_scsi_command(*the_device, the_command);
	
        if (PRINT_ACTIVITY) {
            printf("\nSCSI: Search Empty status for SCSI Id %d (%d)\n", the_device->fTargetId, chip_status);
            printf("0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x \n",
                   data_pointer[0], data_pointer[1], data_pointer[2], data_pointer[3], data_pointer[4],
                   data_pointer[5], data_pointer[6], data_pointer[7], data_pointer[8], data_pointer[9]);
        }
        
		/* check status returned by the chip driver: */
		
		if (chip_status == CHIP_NO_RESPONSE)
			return (NO_RESPONSE);
			
		if (chip_status == CHIP_BUSY_STATUS)
			return (DEVICE_BUSY_OCCURED);
				
		if (chip_status)            /* better be a good status from driver  */
			return (BAD_STATUS);
		
		/* status is good; now check fScsiStatus                            */
		
		if (the_command.fScsiStatus == RESERVATION_CONFL)
			return (DEVICE_RESERVED);
		
		if (the_command.fScsiStatus == CONDITION_MET)
			return (DEVICE_COND_MET_OCCURED);
		
		if (the_command.fScsiStatus == CHECK_CONDITION) /* check condition  */
		{
			scsi_status = issue_request_sense(the_device);
			
			if (scsi_status == RETRY_COMMAND)           /* allow retry      */
				continue;
			
			if (scsi_status)                            /* other prob       */
				return (scsi_status);
			
			/*==> else fall through with good scsi_status and good fScsiStatus!! */
		}
		
		if ((the_command.fScsiStatus)                   
		||  (the_command.fNumBytesXferd != num_bytes))
			return (BAD_STATUS);
		
		return (GOOD_STATUS);
	}
 }
 

/**------------------------------------------------------------------------ */
/* Basic SCSI Commands: issue_mode_sense                                    */
/*------------------------------------------------------------------------- */

/* Issue mode sense command to scsi device */

scsi_error_code issue_mode_sense(scsi_device    *the_device,
								 uint32		     page,
								 byte           *data_pointer,
								 uint32          num_bytes)
{
	scsi_command    the_command;
	chip_error_code chip_status;
	scsi_error_code scsi_status;

	/* Access IDE drive: */

	if (the_device->fDevicePort == MAC_IDE_PORT)
		return (BAD_STATUS);

	/* Else access SCSI drive: */

	while (1)   /* loop for retry   */
	{
		the_command.fWCommand[0] = MODE_SENSE << 8;
		the_command.fWCommand[1] = page       << 8;
		the_command.fWCommand[2] = num_bytes  << 8;
			
		the_command.fNumCommandBytes = 6;
		the_command.fData            = data_pointer;
		the_command.fNumDataBytes    = num_bytes;
		the_command.fDataDirection   = DATA_IN_DIRECTION;

		chip_status = issue_scsi_command(*the_device, the_command);
	
        if (PRINT_ACTIVITY) {
            printf("\nSCSI: Mode Sense status for SCSI Id %d (%d)\n", the_device->fTargetId, chip_status);
            printf("0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
                   data_pointer[0], data_pointer[1], data_pointer[2 ], data_pointer[3 ], data_pointer[4 ], data_pointer[5 ], data_pointer[6 ], data_pointer[7 ],
                   data_pointer[8], data_pointer[9], data_pointer[10], data_pointer[11], data_pointer[12], data_pointer[13], data_pointer[14], data_pointer[15]);
        }
        
		/* check status returned by the chip driver: */
		
		if (chip_status == CHIP_NO_RESPONSE)
			return (NO_RESPONSE);
			
		if (chip_status == CHIP_BUSY_STATUS)
			return (DEVICE_BUSY_OCCURED);
				
		if (chip_status)            /* better be a good status from driver  */
			return (BAD_STATUS);
		
		
		/* status is good; now check fScsiStatus                            */
		
		if (the_command.fScsiStatus == RESERVATION_CONFL)
			return (DEVICE_RESERVED);
		
		if (the_command.fScsiStatus == CHECK_CONDITION) /* check condition  */
		{
			scsi_status = issue_request_sense(the_device);
			
			if (scsi_status == RETRY_COMMAND)           /* allow retry      */
				continue;
			
			if (scsi_status)                            /* other prob       */
				return (scsi_status);
			
			/*==> else fall through with good scsi_status and good fScsiStatus!! */
		}
		
		if ((the_command.fScsiStatus)                   
		||  (the_command.fNumBytesXferd != num_bytes))
			return (BAD_STATUS);
		
		return (GOOD_STATUS);
	}
 }
 

/**------------------------------------------------------------------------ */
/* Basic SCSI Commands: issue_start_unit                                    */
/*------------------------------------------------------------------------- */

/* Issue start unit command to scsi device */

scsi_error_code issue_start_unit(scsi_device    *the_device,
								 boolean         immediate,
								 boolean         load,
								 boolean         state)
{
	scsi_command    the_command;
	chip_error_code chip_status;
	scsi_error_code scsi_status;
	
	
	/* issue start unit; also used for load/unload */
	
	while (1)   /* loop for retry   */
	{
		the_command.fWCommand[0] = (START_UNIT << 8) | immediate;
		the_command.fWCommand[1] = 0;
		the_command.fWCommand[2] = ((load << 1) | state) << 8;

		the_command.fNumCommandBytes = 6;
		the_command.fData            = 0;
		the_command.fNumDataBytes    = 0;
		the_command.fDataDirection   = DATA_OUT_DIRECTION;
		
		chip_status = issue_scsi_command(*the_device, the_command);
		
        if (PRINT_ACTIVITY)
            printf("\nSCSI: Start Unit status for SCSI Id %d (%d)\n", the_device->fTargetId, chip_status);
        
		/* check status returned by the chip driver: */
		
		if (chip_status == CHIP_NO_RESPONSE)
			return (NO_RESPONSE);
			
		if (chip_status == CHIP_BUSY_STATUS)
			return (DEVICE_BUSY_OCCURED);
				
		if (chip_status)            /* better be a good status from driver  */
			return (BAD_STATUS);
		
		
		/* Now check the SCSI status itself */
			
		if (the_command.fScsiStatus == RESERVATION_CONFL)
			return (DEVICE_RESERVED);
		
		if (the_command.fScsiStatus == CHECK_CONDITION) /* check condition  */
		{
			scsi_status = issue_request_sense(the_device);
			
			if (scsi_status == RETRY_COMMAND)           /* allow retry      */
				continue;                               /* after chk cond   */
			
			if (scsi_status)                            /* most likely not  */
				return (scsi_status);                   /* available        */
			
			/*==> else fall through with good scsi_status and good fScsiStatus!! */
		}
		
		if (the_command.fScsiStatus || (the_command.fNumBytesXferd != 0))
			return (BAD_STATUS);
		
		return (GOOD_STATUS);
	}
}


/**------------------------------------------------------------------------ */
/* Basic SCSI Commands: issue_prevent_allow                                 */
/*------------------------------------------------------------------------- */

/* Issue prevent media removal command */

scsi_error_code issue_prevent_allow(scsi_device    *the_device,
						    		boolean         prevent)
{
	scsi_command    the_command;
	chip_error_code chip_status;
	scsi_error_code scsi_status;
	
	
	while (1)   /* loop for retry   */
	{
		the_command.fWCommand[0] = (PREVENT_REMOVAL << 8);
		the_command.fWCommand[1] = 0;
		the_command.fWCommand[2] = prevent << 8;

		the_command.fNumCommandBytes = 6;
		the_command.fData            = 0;
		the_command.fNumDataBytes    = 0;
		the_command.fDataDirection   = DATA_OUT_DIRECTION;
		
		chip_status = issue_scsi_command(*the_device, the_command);
		
        if (PRINT_ACTIVITY)
            printf("\nSCSI: Prevent Allow status for SCSI Id %d (%d)\n", the_device->fTargetId, chip_status);
    
		/* check status returned by the chip driver: */                    
		
		if (chip_status == CHIP_NO_RESPONSE)
			return (NO_RESPONSE);
			
		if (chip_status == CHIP_BUSY_STATUS)
			return (DEVICE_BUSY_OCCURED);
				
		if (chip_status)            /* better be a good status from driver  */
			return (BAD_STATUS);
		
		
		/* Now check the SCSI status itself */
			
		if (the_command.fScsiStatus == RESERVATION_CONFL)
			return (DEVICE_RESERVED);
		
		if (the_command.fScsiStatus == CHECK_CONDITION) /* check condition  */
		{
			scsi_status = issue_request_sense(the_device);
			
			if (scsi_status == RETRY_COMMAND)           /* allow retry      */
				continue;                               /* after chk cond   */
			
			if (scsi_status)                            /* most likely not  */
				return (scsi_status);                   /* available        */
			
			/*==> else fall through with good scsi_status and good fScsiStatus!! */
		}
		
		if (the_command.fScsiStatus || (the_command.fNumBytesXferd != 0))
			return (BAD_STATUS);
		
		return (GOOD_STATUS);
	}
}


/**------------------------------------------------------------------------ */
/* Basic SCSI Commands: issue_read_cap                                      */
/*------------------------------------------------------------------------- */

/* Issue read capacity command to scsi device */

void    compute_secs_cyls(uint32 size, uint32& cyls, uint32& secs)
{
	uint32	rem;
    
    if (size >= 256*65536)					/* limit to 8 gigs... 			*/
        size = 256*65536;
    
    secs = ((size >> 16) << 1) + 2;			/* match able code				*/
    cyls = size/secs; rem = size%secs;		/* find 16-bit total cyl count	*/
                                            /* and sectors/cyl count		*/
    while ((cyls > 30000) || (rem != 0))	/* for use by able...			*/
    {
        secs += 1;
        cyls = size/secs; rem = size%secs;
        
        if (secs >= cyls)
        {
            if (PRINT_ACTIVITY)
                printf("\nSCSI: Could not find suitable integer cylinder size\n");
            
            if ((size >> 16) <= 65)
                secs = 144;
            else
                secs = 4000;
            
            cyls = size/secs;
            
            break;
        }
    }
}

scsi_error_code issue_read_cap(scsi_device *the_device)
{
	scsi_command    the_command;
	chip_error_code chip_status;
	scsi_error_code scsi_status;
	uint32	size, cyls, secs;

	/* issue read capacity command: */

	while (1)   /* loop for retry   */
	{
		the_device->fBlockStart   = 0;      /* init in case error on retry  */
		the_device->fNumBlocks    = 0;
		the_device->fBlockSize    = 0;
		
		the_command.fWCommand[0] = READ_CAPACITY << 8;    /* set up command               */
		the_command.fWCommand[1] = 0;
		the_command.fWCommand[2] = 0;
		the_command.fWCommand[3] = 0;
		the_command.fWCommand[4] = 0;
	
		the_command.fNumCommandBytes = 10;
		the_command.fData            = (byte *) &the_device->fReadCapacityData;
		the_command.fNumDataBytes    = sizeof(the_device->fReadCapacityData);
		the_command.fDataDirection   = DATA_IN_DIRECTION;
		
		chip_status = issue_scsi_command(*the_device, the_command);
	
        if (PRINT_ACTIVITY) {
            printf("\nSCSI: Read Capacity status for SCSI Id %d (%d)\n", the_device->fTargetId, chip_status);
            printf("As read: 0x%08x, 0x%08x\n", the_device->fReadCapacityData.num_blocks, the_device->fReadCapacityData.block_size);
            printf("Swapped: 0x%08x, 0x%08x\n", CFSwapInt32BigToHost(the_device->fReadCapacityData.num_blocks), CFSwapInt32BigToHost(the_device->fReadCapacityData.block_size));
        }
        
		/* check status returned by the chip driver: */
		
		if (chip_status == CHIP_NO_RESPONSE)
			return (NO_RESPONSE);
			
		if (chip_status == CHIP_BUSY_STATUS)
			return (DEVICE_BUSY_OCCURED);
				
		if (chip_status)            /* better be a good status from driver  */
			return (BAD_STATUS);
		
		
		/* status is good; now check fScsiStatus                            */
		
		if (the_command.fScsiStatus == RESERVATION_CONFL)
			return (DEVICE_RESERVED);
		
		if (the_command.fScsiStatus == CHECK_CONDITION) /* check condition  */
		{
			scsi_status = issue_request_sense(the_device);
			
			if (scsi_status == RETRY_COMMAND)           /* allow retry      */
				continue;
			
			if (scsi_status)                            /* other prob       */
				return (scsi_status);
			
			/*==> else fall through with good scsi_status and good fScsiStatus!! */
		}
		
		if ((the_command.fScsiStatus)                   
		||  (the_command.fNumBytesXferd != sizeof(read_capacity_data)))
			return (BAD_STATUS);
		
        // For D24 devices have to swap bytes
        if (the_device->fDevicePort == D24_SCSI_PORT) {
            the_device->fReadCapacityData.num_blocks = CFSwapInt32BigToHost(the_device->fReadCapacityData.num_blocks);
            the_device->fReadCapacityData.block_size = CFSwapInt32BigToHost(the_device->fReadCapacityData.block_size);
        }
        
		the_device->fNumBlocks = the_device->fReadCapacityData.num_blocks + 1;
		the_device->fBlockSize = the_device->fReadCapacityData.block_size;
		
		if  ((the_device->fBlockSize !=  512)   /* must fit into our buffer 	*/
		&&   (the_device->fBlockSize != 1024)
		&&   (the_device->fBlockSize != 2048))
			return (BAD_STATUS);
		
		size = the_device->fNumBlocks;
		
		if (the_device->fBlockSize == 1024)		/* compute size based upon		*/
			size <<= 1;							/* 512 byte sectors...			*/
			
		if (the_device->fBlockSize == 2048)		/* compute size based upon		*/
			size <<= 2;							/* 512 byte sectors...			*/
        
        // The growable devices are typically a disk image that is a subcatalog
        // within virtual W0. As in W0:USER. So we limit the size of a growable
        // image file to be that which fits in a subcatalog (20 bits of sectors).
        if ((the_device->fAllowGrow)            /* growable device - set max    */
        &&  (size < 0xFFFFF))
            size = 0xFFFFF;
        
        compute_secs_cyls(size, cyls, secs);
        
		the_device->fTotCyl = cyls;
		the_device->fTotSec = secs;
		
		if (PRINT_ACTIVITY)
		{
			printf("\nSCSI: Capacity data: SCSI Id %d\n", the_device->fTargetId);
			
			printf("      fBlockStart  : %d\n", the_device->fBlockStart );
			printf("      fNumBlocks   : %d\n", the_device->fNumBlocks  );
			printf("      fBlockSize   : %d\n", the_device->fBlockSize  );
			printf("      fTotCyl      : %d\n", the_device->fTotCyl     );
			printf("      fTotSec      : %d\n", the_device->fTotSec     );
		}
			
		return (GOOD_STATUS);
	}
}


/**------------------------------------------------------------------------ */
/* Basic SCSI Commands: issue_read_extended                                 */
/*------------------------------------------------------------------------- */

/* Issue read extended command to scsi device */

scsi_error_code issue_read_extended(scsi_device *the_device,
									byte        *data_pointer,
									uint32       block_num,
									uint32       num_blocks)
{
	scsi_command    the_command;
	chip_error_code chip_status;
	scsi_error_code scsi_status;

	if (the_device->fDevicePort == MAC_IDE_PORT)
		return (BAD_STATUS);
	
	if (the_device->fBlockSize == 0)
		return (BAD_STATUS);

	/* issue read extended command: */

	while (1)   /* loop for retry   */
	{
		the_command.fWCommand[0] = READ_EXTENDED << 8;
		
		the_command.fWCommand[1] = (uint16) (block_num  >> 16);
		the_command.fWCommand[2] = (uint16) (block_num       );
		
		the_command.fWCommand[3] = (uint16) (num_blocks >>  8);
		the_command.fWCommand[4] = (uint16) (num_blocks << 8 );
		
		the_command.fNumCommandBytes = 10;
		the_command.fData            = data_pointer;
		the_command.fNumDataBytes    = num_blocks*the_device->fBlockSize;
		the_command.fDataDirection   = DATA_IN_DIRECTION;
		
		chip_status = issue_scsi_command(*the_device, the_command);
	
		/* check status returned by the chip driver: */                    
		
		if (chip_status == CHIP_NO_RESPONSE)
			return (NO_RESPONSE);   /* is device not there.                 */
			
		if (chip_status == CHIP_BUSY_STATUS)
			return (DEVICE_BUSY_OCCURED);
				
		if (chip_status)            /* better be a good status from driver  */
		{
			if (PRINT_ACTIVITY)
				printf("SCSI: ReadExtended: Bad chip status: %d", chip_status);
			
			return (BAD_STATUS);
		}
		
		
		/* status is good; now check fScsiStatus                            */
		if (the_command.fScsiStatus == RESERVATION_CONFL)
			return (DEVICE_RESERVED);
		
		if (the_command.fScsiStatus == CHECK_CONDITION) /* check condition  */
		{
			scsi_status = issue_request_sense(the_device);
			
			if (scsi_status == RETRY_COMMAND)           /* allow retry      */
				continue;
			
			if (scsi_status)                            /* other prob (including blank check)      */
				return (scsi_status);
			
			/*==> else fall through with good scsi_status and good fScsiStatus!! */
		}
		
		if ((the_command.fScsiStatus)                   
		||  (the_command.fNumBytesXferd != num_blocks*the_device->fBlockSize))
		{
			if (PRINT_ACTIVITY)
			{
				if (the_command.fScsiStatus)                   
					printf("SCSI: ReadExtend: Bad SCSI status: %d", the_command.fScsiStatus);
				if (the_command.fNumBytesXferd != num_blocks*the_device->fBlockSize)
					printf("SCSI: ReadExtend: Bad Num Xferd status: %d %d", the_command.fNumBytesXferd, num_blocks*the_device->fBlockSize);
			}
			
			return (BAD_STATUS);
		}
		
		return (GOOD_STATUS);
	}
}


/**------------------------------------------------------------------------ */
/* Basic SCSI Commands: issue_write_extended                                */
/*------------------------------------------------------------------------- */

/* Issue write extended command to scsi device */

scsi_error_code issue_write_extended(scsi_device    *the_device,
									 byte           *data_pointer,
									 uint32          block_num,
									 uint32          num_blocks)
{
	scsi_command    the_command;
	chip_error_code chip_status;
	scsi_error_code scsi_status;
	
	
	/* Access IDE drive: */

	if (the_device->fDevicePort == MAC_IDE_PORT)
		return (BAD_STATUS);

	if (the_device->fBlockSize == 0)
		return (BAD_STATUS);

	/* Else access SCSI drive: */

	while (1)   /* loop for retry   */
	{
		the_command.fWCommand[0] = WRITE_EXTENDED << 8;
		
		the_command.fWCommand[1] = (uint16) (block_num  >> 16);
		the_command.fWCommand[2] = (uint16) (block_num       );
		
		the_command.fWCommand[3] = (uint16) (num_blocks >>  8);
		the_command.fWCommand[4] = (uint16) (num_blocks <<  8);
			
		the_command.fNumCommandBytes = 10;
		the_command.fData            = data_pointer;
		the_command.fNumDataBytes    = num_blocks*the_device->fBlockSize;
		the_command.fDataDirection   = DATA_OUT_DIRECTION;
		
		chip_status = issue_scsi_command(*the_device, the_command);
	
	
		/* check status returned by the chip driver: */                    
		
		if (chip_status == CHIP_NO_RESPONSE)
			return (NO_RESPONSE);
			
		if (chip_status == CHIP_BUSY_STATUS)
			return (DEVICE_BUSY_OCCURED);
				
		if (chip_status)            /* better be a good status from driver  */
			return (BAD_STATUS);
		
		
		/* status is good; now check fScsiStatus                            */
		
		if (the_command.fScsiStatus == RESERVATION_CONFL)
			return (DEVICE_RESERVED);
		
		if (the_command.fScsiStatus == CHECK_CONDITION) /* check condition  */
		{
			scsi_status = issue_request_sense(the_device);
			
			if (scsi_status == RETRY_COMMAND)           /* allow retry      */
				continue;
			
			if (scsi_status)                            /* other prob       */
				return (scsi_status);
			
			/*==> else fall through with good scsi_status and good fScsiStatus!! */
		}
		
		if ((the_command.fScsiStatus)                   
		||  (the_command.fNumBytesXferd != num_blocks*the_device->fBlockSize))
			return (BAD_STATUS);
		
		return (GOOD_STATUS);
	}
}


/**------------------------------------------------------------------------ */
/* Higher level SCSI Commands: interrogate_device                           */
/*------------------------------------------------------------------------- */

/* Issue Inquiry and handle as needed */

scsi_error_code interrogate_device  (scsi_device    *the_device)
{
	scsi_error_code scsi_status;
	device_type     new_type;

	
	/* Check for IDE port usage:                                            */
	
	if (the_device->fDevicePort == MAC_IDE_PORT)
		return (GOOD_STATUS);
	
	
	/* Handle SCSI port:                                                    */
	
	if (initialize_scsi_chip())                 /* init chip on each inqry  */
		return (BAD_STATUS);
	
	new_type = DEVICE_NOT_EXAMINED;             /* init to not examined     */
	
    #if SYNC_DEBUG_OPTICAL
        g_scsi_print_basic_opt = g_scsi_print_all_opt = (the_device->fDevicePort == D24_SCSI_PORT && (the_device->fTargetId == 1 || the_device->fTargetId == 2));
    #endif
    
	if ((scsi_status = issue_inquiry(the_device)) != 0)
	{
		if (scsi_status == NO_RESPONSE)
			new_type = DEVICE_DOES_NOT_EXIST;   /* device does not exist    */
		
		else
			new_type = DEVICE_NOT_TALKING;      /* else can't talk          */
	}
	
	else if ((the_device->fStandardInquiryData.ptype == DISK_TYPE   )
	||       (the_device->fStandardInquiryData.ptype == OPTICAL_TYPE)
	||       (the_device->fStandardInquiryData.ptype == CDROM_TYPE  ))
	{
		uint32      disk_buf[512];    			/* one disk buffer for our & ide use only!! */

		the_device->fTimeout = DISK_TIMEOUT;    /* init timeout for disks   */

		if (PRINT_ACTIVITY)
			printf("\nSCSI: Found disk: %d\n", the_device->fTargetId);
		
		scsi_status = issue_test_ready(the_device);

		if (scsi_status == DEVICE_RESERVED)     /* detect reserved disk     */
			new_type = DEVICE_RESERVED_DISK;
			
		else if (the_device->fStandardInquiryData.ptype == CDROM_TYPE)  
			new_type = DEVICE_CD_ROM_OF_SOME_KIND;
		
		else                                    /* else look deeper         */
		{
			scsi_error_code test_status, cap_status = GOOD_STATUS, read_ext_status = GOOD_STATUS;
			
			if (scsi_status == NOT_AVAILABLE)   /* start up if needed       */
				issue_start_unit(the_device, FALSE, FALSE, TRUE);
			
			zero_mem((byte *)&disk_buf[0], sizeof(disk_buf));
			
			if ((!(test_status     = issue_test_ready(the_device))) /* if unit is now available */
			&&  (!(cap_status      = issue_read_cap  (the_device))) /* and can read capacity    */
			&&  (!(read_ext_status = issue_read_extended (the_device, (byte *) disk_buf,
									   the_device->fBlockStart,
									   sizeof(disk_buf)/the_device->fBlockSize))))
			{
				ufixed *vinfoData = (ufixed *) disk_buf;
				audio_volume_record vinfo;
				
				vinfo.audio_sig = ((uint32) vinfoData[1]) | (((uint32) vinfoData[0]) << 16);
				vinfo.version   = ((uint32) vinfoData[3]) | (((uint32) vinfoData[2]) << 16);
			
				if ((vinfo.audio_sig == VALID_VOLUME)
				&&  (vinfo.version   == VOLUME_VERSION))
				{
					new_type = DEVICE_AUDIO_DISK;
				
					if (PRINT_ACTIVITY)
						printf("\nSCSI: Found Audio Disk %d\n", the_device->fTargetId);
				}
				
				else if ((vinfo.audio_sig >> 16) == 0x4552)	/* mac      */
				{
					new_type = DEVICE_MACINTOSH_DISK;
				
					if (PRINT_ACTIVITY)
						printf("\nSCSI: Found Mac Disk %d\n", the_device->fTargetId);
				}
				
				else if ((vinfo.audio_sig == 0x63636363)
				&&       (vinfo.version   == 0x63636363))
				{
					new_type = DEVICE_BLANK_ABLE_OPTICAL;
				
					if (PRINT_ACTIVITY)
						printf("\nSCSI: Found Blank NED M/O Optical Disk %d\n", the_device->fTargetId);

					// Simulate 1024-byte media for Mac HFS optical files to debug stuff
					if ((0)
					&&  (the_device->fDevicePort == SIMULATED_PORT)
					&&  (the_device->fBlockSize  == 512           ))
					{
						printf("SCSI: Simulating 1024 byte optical media for target %d\n", the_device->fTargetId);
						the_device->fBlockSize                   <<= 1;
						the_device->fReadCapacityData.block_size <<= 1;
						the_device->fReadCapacityData.num_blocks >>= 1;
					}
				}
				
				else if ((vinfo.audio_sig & 0xFFFF0000) == 0x80000000)
				{
					new_type = DEVICE_ABLE_OPTICAL;
				
					if (PRINT_ACTIVITY)
						printf("\nSCSI: Found NED Optical Disk %d\n", the_device->fTargetId);

					// Simulate 1024-byte media for Mac HFS optical files to debug stuff
					if ((0)
					&&  (the_device->fDevicePort == SIMULATED_PORT)
					&&  (the_device->fBlockSize  == 512           ))
					{
						printf("SCSI: Simulating 1024 byte optical media for target %d\n", the_device->fTargetId);
						the_device->fBlockSize                   <<= 1;
						the_device->fReadCapacityData.block_size <<= 1;
						the_device->fReadCapacityData.num_blocks >>= 1;
					}
				}
				
				else
				{
					/* Check for able disk */
					
					int	i;
					uint32 name, blockstart, blocklen, filelen, filetyp, fileblocks;
					uint16 *bytes  = (uint16 *) disk_buf;
					boolean	isable = TRUE;
					
					for (i=0; i<512; i += 8)	/* check catalog entries	*/
					{
						name       = bytes[i  ];
						blockstart = bytes[i+4];
						blocklen   = bytes[i+5];
						filelen    = bytes[i+6];
						filetyp    = bytes[i+7] & 0xF;
						
						// we want to recognize all able disks, just make sure the name doesn't contain any
						// non-printing characters (e.g. >= 128)
						
						if (g_recognize_disks)
						{
							if ((name & 0x00008080) != 0)							// if name has non-ascii characters
								isable = FALSE; 
						}

						/* if block start is specified, make sure			*/
						/* block length and file length match...			*/
						
						else if (blockstart)
						{
							/* detect an able disk by seeing of the length	*/
							/* of the file (modulo 64k) matches the			*/
							/* number of sectors allocated for the file.	*/
							/* this test is known to fail for -symtab		*/
							/* files, and for sound files...				*/
                            
                            #if __BIG_ENDIAN__
                                #error Not tested for big-endian
                            #endif
                            
                            const char* symtab  = "-SYMTAB-";
                            const char* copylog = "COPYLOG";
                            
							if ((name && name != (* (unsigned short*) symtab))		/* -SYMTAB files fail length/alloc test...	*/
							&&  (name && name != (* (unsigned short*) copylog))		/* COPYLOG files fail length/alloc test...	*/
							&&  (filetyp      != 5   ))		/* sound files contain no word length...	*/
							{
								fileblocks = (filelen + 255) >> 8;
								
								if ((fileblocks & 0xFF) != (blocklen & 0xFF))
									isable = FALSE;
							}
						}
						
						/* else if no block start, make sure entire record	*/
						/* is zeroes...										*/
						
						
						else
						{
							if (blocklen || filelen)
								isable = FALSE;
						}
					}
					
					if ((isable)
					&&  (the_device->fBlockSize == 512))
					{
						new_type = DEVICE_ABLE_DISK;
						
						if (PRINT_ACTIVITY)
							printf("\nSCSI: Found Able Disk %d\n", the_device->fTargetId);
					}
										
					else
					{
						new_type = DEVICE_UNINITIALIZED_DISK;
					
						if (PRINT_ACTIVITY)
							printf("\nSCSI: Found Uninitialized Disk %d\n", the_device->fTargetId);
					}
				}
			}
			
			else
			{
				new_type = DEVICE_UNCOOPERATIVE_DISK;
			
				if (PRINT_ACTIVITY)
				{
					printf("\nSCSI: Found Uncooperative Disk %d\n", the_device->fTargetId);

					printf("Test ready status: %d\n", test_status);
					printf("Read cap   status: %d\n", cap_status);
					printf("Read extnd status: %d\n", read_ext_status);
				}
			}
		}
	}


	// Handle LD1200
	else if (the_device->fStandardInquiryData.ptype == WORM_TYPE)
	{
		unsigned short    worm_buf[512];

		the_device->fTimeout = DISK_TIMEOUT;    /* init timeout for disks   */

		if (PRINT_ACTIVITY)
			printf("\nSCSI: Found worm disk: %d\n", the_device->fTargetId);
		
		new_type = DEVICE_BLANK_ABLE_OPTICAL;   /* optical disk; may be able optical format or not, we don't know */
			
		the_device->fBlockStart = 0;			/* inform the palaentology department of an important pre-neanderthal	*/
		the_device->fNumBlocks  = 2048000;		/* technology dicovery regarding the high level of technical			*/
		the_device->fBlockSize  = 1024;			/* advancement of precolumbian culture									*/
		the_device->fTotCyl     = 2048;			/* e.g. preset for LD1200 constants derived from Pythagorus	C. 400 B.C.	*/
		the_device->fTotSec     = 1000;

		// Synth read capacity information since we don't know how the LD1200 responds to it
		the_device->fReadCapacityData.num_blocks = 2048000;
		the_device->fReadCapacityData.block_size =    1024;

		// Try to read the first header block to see if it's blank or not
		zero_mem((byte *)&worm_buf[0], sizeof(worm_buf));

		if (!issue_read_extended (the_device, (byte *) worm_buf, the_device->fBlockStart,
                                  sizeof(worm_buf)/the_device->fBlockSize))
		{
			if (worm_buf[0] == (unsigned short) 0x8000)
				new_type = DEVICE_ABLE_OPTICAL;
		}
	}

	// Else is unknown
	else
	{
		if (PRINT_ACTIVITY)
			printf("\nSCSI: Found Unknown SCSI Device Type %d\n", the_device->fTargetId);
		
		new_type = DEVICE_UNKNOWN_DEVICE;
	}
	

	/* check for device changing type */
	
	if (the_device->fDeviceType != new_type)
	{
		the_device->fDeviceType    = new_type;
		the_device->fUnitAttention = TRUE;
		the_device->fUnitAttnCount++;

		g_unit_attention = TRUE;                /* treat as unit attention  */
	}
	
	return (GOOD_STATUS);
}
