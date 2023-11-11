/*  ScsiLib.h                                                               */

/*  Created:                                                                */
/*      06/18/96        C. Jones                                            */

/*  Contents:                                                               */
/*      Header for use of SCSI subsystem; interface to scsilib.c            */


#ifndef scsilib_h               /* include ourselves only once!             */
#define scsilib_h

// Mac OS
#include <CoreServices/CoreServices.h>

// Local
#include "XPL.h"
#include "XPLRuntime.h"
#include "ScsiDefs.h"
#include "SynclavierFileReference.h"
#include "SyncMutex.h"


/*------------------------------------------------------------------------- */
/* Basic Port Types:                                                        */
/*------------------------------------------------------------------------- */

typedef enum                                /* disk type enumeration        */
{
	NO_SCSI_PORT   = 0x00,                  /* disk is on the no port       */
	MAC_SCSI_PORT  = 0x01,                 	/* disk is on the mac SCSI port */
	D24_SCSI_PORT  = 0x02,                 	/* disk is on the d24 SCSI port */
	MAC_IDE_PORT   = 0x03,                  /* disk is on the IDE port      */
	SIMULATED_PORT = 0x04					/* host resident (simulated)	*/

}   device_port;


/*------------------------------------------------------------------------- */
/* Device states:                                                           */
/*------------------------------------------------------------------------- */

typedef enum                                /* device type enumeration      */
{
	DEVICE_NOT_EXAMINED         = 0x00,     /* not been examined yet        */
	DEVICE_DOES_NOT_EXIST       = 0x01,     /* no response to selection     */
	DEVICE_NOT_TALKING          = 0x02,     /* inquiry failed               */
	
	DEVICE_UNCOOPERATIVE_DISK   = 0x03,     /* disk, but won't ready        */
	DEVICE_RESERVED_DISK        = 0x04,     /* disk, but reserved           */
	DEVICE_UNINITIALIZED_DISK   = 0x05,     /* disk, not initialized        */
	DEVICE_AUDIO_DISK           = 0x06,     /* audio disk                   */
	DEVICE_MACINTOSH_DISK       = 0x07,     /* mac disk                     */
	DEVICE_ABLE_DISK			= 0x08,		/* able disk					*/
	DEVICE_BLANK_ABLE_OPTICAL	= 0x09,		/* blank ned optical (0x6363)	*/
	DEVICE_ABLE_OPTICAL			= 0x0A,		/* able optical format			*/
	DEVICE_CD_ROM_OF_SOME_KIND  = 0x0B,		/* CD rom; don't know more		*/
	
	DEVICE_UNKNOWN_DEVICE       = 0xFF      /* unknown device type          */

}   device_type;


/*------------------------------------------------------------------------- */
/* Software Error Codes:                                                    */
/*------------------------------------------------------------------------- */

typedef enum                            /* error code enumeration           */
{
	GOOD_STATUS             = 0x00,     /* General purpose good status      */
	BAD_STATUS              = 0x01,     /* General purpose bad status       */
	NO_RESPONSE             = 0x02,     /* No response to selection         */
	NOT_AVAILABLE           = 0x03,     /* Not available 'cause not ready   */
	CANNOT_INITIALIZE       = 0x04,     /* initialize failed                */
	DEVICE_RESERVED         = 0x05,     /* reservation conflict             */
	RETRY_COMMAND           = 0x06,     /* retry command                    */
	MEDIUM_ERROR_OCCURED    = 0x07,     /* medium error occured				*/
	HARDWARE_ERROR_OCCURED  = 0x08,		/* hardware error occured			*/
	BLANK_CHECK_OCCURED		= 0x09,		/* blank check						*/
	DEVICE_BUSY_OCCURED		= 0x0A,		/* device busy						*/
	DEVICE_COND_MET_OCCURED = 0x0B		/* condition met					*/

}   scsi_error_code;


/*------------------------------------------------------------------------- */
/* SCSI Data Direction Codes:                                               */
/*------------------------------------------------------------------------- */

typedef enum                            /* data direction enumeration       */
{
	DATA_IN_DIRECTION       = 0x00,     /* data is coming in                */
	DATA_OUT_DIRECTION      = 0x01      /* data is going  out               */

}   scsi_direction;


/*------------------------------------------------------------------------- */
/* Audio Disk Format:                                                       */
/*------------------------------------------------------------------------- */

#define     VALID_VOLUME   'sncl'   	/* indicates valid audio volume     */
#define     VOLUME_VERSION 0            /* current version number           */

#pragma pack(push,2)

typedef struct	audio_volume_record
{           
	uint32  audio_sig;              	/* holds valid_volume code          */
	uint32  version;                    /* holds volume format version      */

}   audio_volume_record;    

#pragma pack(pop)


/*------------------------------------------------------------------------- */
/* SCSI Segmentizer = manages pieces of a simulated SCSI file               */
/*------------------------------------------------------------------------- */

// We save pointers to specific funcitonal segments so we can go in there
// and change the file it refers to. We maintain an array of pointers
// to segments that are for certain purposes.
#define MAX_SCSI_SEGMENTS           200
#define SCSI_SEGMENT_FOR_SEQ0         0     // Segment for .SQ0DATA through .SQ7DATA (8 sequences on the button panel)
#define SCSI_SEGMENT_FOR_TIMB         8     // Segment for .NEWDATA
#define SCSI_SEGMENT_FOR_SEQ          9     // Segment for .SEQDATA
#define SCSI_SEGMENT_FOR_SOUND       10     // Segment for .SNDDATA
#define SCSI_SEGMENT_FOR_BUFS        11     // 100 segments for mono sampling attack buffers
#define MAX_SEGMENTS_FOR_BUFS       100
#define MAX_SPECIAL_SCSI_SEGMENTS    (SCSI_SEGMENT_FOR_BUFS+MAX_SEGMENTS_FOR_BUFS)
#define SCSI_SEGMENT_UNUSED          -1

typedef struct scsi_segment                     /* struct for segments      */
{
    CFURLRef                        fSegURLRef;         // URL to the underlying file
    CSynclavierFileReference*       fSegFileRef;        // CSynclavierFileReference with which to read it
    struct SynclavierAudioStash*    fSegStash;          // Points to file data in memory, not on disk. Entire file is in memory.
    
    uint32                          fSegStartOnDevice;  // Block number in able catalog where this file starts
    uint32                          fSegBlocksOnDevice; // Block extent in able catalog
    uint32                          fSegMaxBlocks;      // Max block extent. This much is reserved when the entry is created; allows shorter file
    long long                       fSegMaxBytes;       // Max byte length of file on disk
    boolean                         fSegAllowsCreate;   // File can be created on write
    boolean                         fSegSynthsZeroes;   // File synthesises zeroes if able reads when file does not exist on mac
    OSType                          fSegCreatedType;    // File type to use if file is created
    char                            fSegAbleName[16];   // Name of file in able catalog (c string; for updating the length)
    fixed                           fSegAbleType;       // Able file type (for updating the length; we could change the type too)
    
}   scsi_segment;

typedef struct scsi_segmentizer                     /* struct for segment list  */
{
    fixed                           fCatBuf[1024];  /* catalog buffer, wrds */
    fixed                           c_ms_sector;	/* device and MS starting sector of catalog		*/
    fixed                           c_ls_sector;    /* Ls starting sector of catalog				*/
    fixed                           c_ms_length;    /* Ms sector length of catalog (including directory)	*/
    fixed                           c_ls_length;	/* Ls sector length of catalog (including directory)	*/
    fixed                           c_dir_size;		/* size of catalog directory (in words)			*/

    uint32                          fNumSegments;
    uint32                          fBlockStartOnDevice;
    uint32                          fNumBlocksOnDevice;
    scsi_segment*                   fSpecialSegments[MAX_SPECIAL_SCSI_SEGMENTS];
    
    scsi_segment                    fSegList[MAX_SCSI_SEGMENTS];
    
}   scsi_segmentizer;

// One mutex is provided to deskew (system-wide) scsi_segmentizer structs
extern SyncMutex SyncScsiSegmentizerMutex;

/*------------------------------------------------------------------------- */
/* Your Basic SCSI Device Object:                                           */
/*------------------------------------------------------------------------- */

typedef struct scsi_device                      /* struct for device object */
{
	device_port             fDevicePort;        /* Mac SCSI, D24, or IDE	*/
	device_type             fDeviceType;        /* indicates type of device */

	boolean                 fLogPolling;        /* printout level desired   */
	boolean                 fIsReserved;        /* is reserved              */
	boolean                 fMediaIsLocked;     /* media locked in place    */
	boolean                 fDeviceIsStartable; /* is startable             */
	boolean                 fEjectOnSpindown;   /* is ejectable             */
	boolean                 fAllowGrow;         /* allow disk image to grow */
	boolean					fUnitAttention;		/* set if unit attn occured */
	
	uint16                  fIdentifyMessage;   /* identify message to use  */
	uint16                  fTargetId;          /* target id                */
	uint16                  fSyncXferCode;      /* sync xfer setting        */
	
	uint32                  fBlockStart;        /* starting block number    */
	uint32                  fNumBlocks;         /* capacity                 */
	uint32                  fBlockSize;         /* block size               */
	uint32                  fTimeout;           /* timeout timer            */

	standard_inquiry_data   fStandardInquiryData;   /* inquiry data         */
	request_sense_data      fRequestSenseData;      /* request sense data   */
	read_capacity_data      fReadCapacityData;      /* capacity data        */
	
	uint32					fTotCyl;			/* total cylinders (ABLE)	*/
	uint32					fTotSec;			/* total secs/cyls (ABLE)	*/
	
	uint32					fUnitAttnCount;		/* count of un. attentions  */
    
    scsi_segmentizer*       fSegManager;        /* segments stored on Mac   */
    SyncFSSpec              fFSSpec;            /* fsspec to image file		*/
    xpl_file_ref_num        fFRefNum;           /* sim file fref num        */
    
    #if !__LP64__
        short               fVRefNum;           /* sim file vref num        */
    #endif

}   scsi_device;


/*------------------------------------------------------------------------- */
/* Your Basic SCSI Comand Object:                                           */
/*------------------------------------------------------------------------- */

typedef struct scsi_command                     /* struct for command object */
{
	uint16                  fWCommand[6];       /* holds scsi command bytes */
	int                     fNumCommandBytes;   /* holds num of cmnd bytes  */
	
	byte                    *fData;             /* points to data bytes     */
	uint32                  fNumDataBytes;      /* holds numf of data bytes */
	scsi_direction          fDataDirection;     /* holds direction of data  */
	
	uint32                  fNumBytesXferd;     /* number of bytes xferd    */
	uint16                  fScsiStatus;        /* status byte              */
	
}   scsi_command;


/*------------------------------------------------------------------------- */
/* External Declarations:                                                   */
/*------------------------------------------------------------------------- */

/* global variables:                    */
extern  boolean      g_unit_attention;          /* unit attention received  */
extern	boolean		 g_scsi_print_basic_opt;	/* nonzero to print output  */
extern	boolean		 g_scsi_print_all_opt;		/* nonzero to print output  */
extern	boolean		 g_recognize_disks;			/* recognize questionable	*/

/* segmentizer functions                */
scsi_segment* scsi_segment_for_range(scsi_segmentizer& izer, uint32 blockStart, uint32 blockLength);

/* basic scsi command routines:         */
scsi_error_code issue_inquiry       (scsi_device    *the_device);
scsi_error_code issue_request_sense (scsi_device    *the_device);
scsi_error_code issue_test_ready    (scsi_device    *the_device);
scsi_error_code issue_mode_select	(scsi_device    *the_device,
								     byte           *data_pointer,
								     uint32          num_bytes);

scsi_error_code issue_search_empty_blocks(scsi_device    *the_device,
								          byte           *data_pointer,
								          uint32          num_bytes);

scsi_error_code issue_mode_sense	(scsi_device    *the_device,
									 uint32		     page,
									 byte           *data_pointer,
									 uint32          num_bytes);

void            compute_secs_cyls   (uint32 size, uint32& cyls, uint32& secs);

scsi_error_code issue_read_cap      (scsi_device    *the_device);

scsi_error_code issue_prevent_allow (scsi_device    *the_device,
						    		 boolean         prevent);

scsi_error_code issue_start_unit    (scsi_device    *the_device,
									 boolean         immediate,
									 boolean         load,
									 boolean         state);

scsi_error_code issue_read_extended (scsi_device    *the_device,
									 byte           *data_pointer,
									 uint32          block_num,
									 uint32          num_blocks);

scsi_error_code issue_write_extended(scsi_device    *the_device,
									 byte           *data_pointer,
									 uint32          block_num,
									 uint32          num_blocks);


/* higher level interrogation routines: */
scsi_error_code interrogate_device  (scsi_device    *the_device);

/* asynchronous operation */
#if SYNC_USE_ASYNC_IO
    extern void (*SCSILib_yielder) ();
    extern long   SCSILib_yield_count;
#endif

#endif
