/*  ScsiDefs.h                                                              */

/*  Created:                                                                */
/*      06/18/96        C. Jones                                            */

/*  Contents:                                                               */
/*      SCSI Command Set enumerations & structure definitions               */


#ifndef scsidefs_h              /* include ourselves only once!             */
#define scsidefs_h

#ifndef     standard_h          /* include standards file as well           */
	#include    "Standard.h"
#endif


/*------------------------------------------------------------------------- */
/* Status Codes:                                                            */
/*------------------------------------------------------------------------- */

typedef enum                            /* error code enumeration           */
{
	CHIP_GOOD_STATUS        = 0x00,     /* General purpose good status      */
	CHIP_BAD_STATUS         = 0x01,     /* General purpose bad status       */
	CHIP_NO_RESPONSE        = 0x02,     /* No response to selection         */
	CHIP_BUSY_STATUS		= 0x03		/* SCSI device reported busy		*/
    
}   chip_error_code;


/*------------------------------------------------------------------------- */
/* Useful constants:                                                        */
/*------------------------------------------------------------------------- */

#define SCSI_HOST_ID             7          /* Host ID to use               */
#define MAX_NUM_DEVICES          7          /* Max # of SCSI devices        */
#define CHIP_TIMEOUT          5000          /* Max ticks to wait for chip   */
#define DISK_TIMEOUT         30000          /* Max ticks to wait for disk   */
#define TAPE_TIMEOUT       6000000          /* Max ticks to wait for tape   */
#define RETRY_LIMIT              5          /* busy retry limit             */
#define RETRY_INTERVAL        1000          /* busy retry interval          */


/*------------------------------------------------------------------------- */
/* Enumerations for SCSI command opcodes:                                   */
/*------------------------------------------------------------------------- */

typedef enum
{   
	TEST_UNIT_READY             = 0x00,     /* Test Unit Ready              */
	REWIND                      = 0x01,     /* Rewind                       */
	REQUEST_SENSE               = 0x03,     /* Request Sense                */
	FORMAT_UNIT                 = 0x04,     /* Format Unit                  */
	READ                        = 0x08,     /* Read                         */
	WRITE                       = 0x0A,     /* Write                        */
	SEARCH_EMPTY_BLOCKS         = 0x0C,     /* Search Empty Blocks (LD1200) */
	WRITE_MARK                  = 0x10,     /* WriteMark                    */
	SPACE                       = 0x11,     /* Space                        */
	INQUIRY                     = 0x12,     /* Inquiry                      */
	MODE_SELECT                 = 0x15,     /* Mode Select                  */
	RESERVE_UNIT                = 0x16,     /* Reserve Unit                 */
	RELEASE_UNIT                = 0x17,     /* Release Unit                 */
	ERASE_TAPE                  = 0x19,     /* Erase                        */
	MODE_SENSE                  = 0x1A,     /* Mode Sense                   */
	START_UNIT                  = 0x1B,     /* Start Unit                   */
	SEND_DIAGNOSTIC             = 0x1D,     /* Send Diagnostic              */
	PREVENT_REMOVAL             = 0x1E,     /* Prevent/Allow removal        */
	READ_CAPACITY               = 0x25,     /* Read Capacity                */
	READ_EXTENDED               = 0x28,     /* Read Extended                */
	WRITE_EXTENDED              = 0x2A,     /* Write Extended               */
	LOCATE                      = 0x2B,     /* Locate                       */
	READ_POSITION               = 0x34,     /* Read current position        */
	LOG_SENSE                   = 0x4D      /* Read error log (read/write)  */

}   scsi_opcode;


/*------------------------------------------------------------------------- */
/* Byte codes for SCSI status definitions:                                  */
/*------------------------------------------------------------------------- */

typedef enum
{
	GOOD_SCSI_STATUS            = 0x00,     /* good                         */
	CHECK_CONDITION             = 0x02,     /* must request sense           */
	CONDITION_MET               = 0x04,     /* condition met/good			*/
	BUSY                        = 0x08,     /* busy                         */
	RESERVATION_CONFL           = 0x18      /* device is reserved           */

}   scsi_status_code;


/*------------------------------------------------------------------------- */
/* Byte codes for SCSI sense keys:                                          */
/*------------------------------------------------------------------------- */

typedef enum
{   NO_SENSE                    = 0x00,     /* no sense information         */
	RECOVERRED_ERROR            = 0x01,     /* error recovery performed     */
	NOT_READY                   = 0x02,     /* device is not ready          */
	MEDIUM_ERROR                = 0x03,     /* unrecoverable medium error   */
	HARDWARE_ERROR              = 0x04,     /* unrecoverable hardware error */
	ILLEGAL_REQUEST             = 0x05,     /* bad command                  */
	UNIT_ATTENTION              = 0x06,     /* unit attention               */
	DATA_PROTECT                = 0x07,     /* write protected              */
	BLANK_CHECK                 = 0x08,     /* blank                        */
	ABORTED_COMMAND             = 0x0B,     /* aborted command (parity err) */
	VOLUME_OVERFLOW             = 0x0D      /* volume overflow              */

}   scsi_sense_key;


/**------------------------------------------------------------------------ */
/* Byte codes for SCSI status Message definitions:                          */
/*------------------------------------------------------------------------- */

typedef enum
{   CMD_COMPLETE_MESSAGE        = 0x00,     /* sent at end of scsi command  */
	EXTENDED_MSG_MESSAGE        = 0x01,     /* preamble for negotiate sync  */
	SAVE_DATA_POINTER_MESSAGE   = 0x02,
	RESTORE_POINTERS_MESSAGE    = 0x03,
	TARG_DISCONNECT_MESSAGE     = 0x04,
	INIT_DETECTED_ERR_MESSAGE   = 0x05,
	ABORT_MESSAGE               = 0x06,
	MSG_REJECT_MESSAGE          = 0x07,
	NO_OP_MESSAGE               = 0x08,
	MSG_PARITY_ERROR_MESSAGE    = 0x09,
	LINKED_CMD_COMPLETE_MESSAGE = 0x0A,
	LINKED_CMD_COMPL_F_MESSAGE  = 0x0B,
	BUS_DEVICE_RESET_MESSAGE    = 0x0C,
	IDENTIFY_MESSAGE            = 0xC0,
	IDENTIFY_NO_DISC            = 0x80,
	
	/* Extended messages                                                    */

	SYNC_DATA_XFER_REQ          = 0x01      /* Synchronous data xfer req    */

}   scsi_message_code;


/*------------------------------------------------------------------------- */
/* Byte codes for Request Sense additional sense codes:                     */
/*------------------------------------------------------------------------- */

typedef enum
{   
	UNRECOVERRED_READ_ERROR     = 0x11,     /* Wangdat bad EOD block        */
	NO_MEDIUM_PRESENT           = 0x3A,     /* Wangdat, Tahiti              */
	OVER_TEMPERATURE            = 0xAD      /* Tahiti                       */

}   scsi_additional_code;


/*------------------------------------------------------------------------- */
/* Scsi data record structures: Inquiry                                     */
/*------------------------------------------------------------------------- */

typedef enum
{   DISK_TYPE                   = 0x00,     /* Inquiry peripheral types     */
	TAPE_TYPE                   = 0x01,
	WORM_TYPE					= 0x04,
	CDROM_TYPE					= 0x05,
	OPTICAL_TYPE                = 0x07

}   periph_type;

// Struct is defined such that, when filled out by software assigns, it appears
// correct to Able software.

// In big endian machines this allows the structs to be read directly from the device.

// Little endian implementations may need further tweaking when reading real hardware

#pragma pack(push,2)

typedef struct
{
	#if __BIG_ENDIAN__
		byte        pqualifier:3,       /* peripheral qualifier                 */
					ptype:5;            /* peripheral device type               */
		byte        rmb:1,              /* removeable media bit                 */
					modifier:7;         /* device-type modifier                 */
		byte        isovers:2,          /* ISO version                          */
					ecmavers:3,         /* ECMA version                         */
					ansivers:3;         /* ANSI version                         */
		byte        :4,                 /* reserved                             */
					response:4;         /* response format                      */
		byte        length;             /* additional length                    */
		byte        :8;                 /* reserved                             */
		byte        :8;                 /* reserved                             */
		byte        reladr:1,           /* relative addressing available        */
					wbus32:1,           /* 32 bit bus available                 */
					wbus16:1,           /* 16 bit bus available                 */
					sync:1,             /* synchronous transfer available       */
					linked:1,           /* linked commands supported            */
					:1,                 /* reserved                             */
					queing:1,           /* command queing supported             */
					softreset:1;        /* reset alternative                    */
				
		byte        vendor    [ 8];      /* vendor identification                */
		byte        product   [16];      /* product identification               */
		byte        revision  [ 4];      /* product revision level               */
		byte        serial    [ 8];      /* drive serial number (sometimes)      */
		short       terminator[ 1];      /* handy word to stop string output     */
	#else
		// Swap bit definitions: done in reverse order in little endian compilations. BYTE ORDER WILL ALSO NEED TO BE SWAPPED: THIS IS TAKEN CARE OF AT A HIGHER LEVEL
		byte        ptype:5,            /* peripheral device type               */
					pqualifier:3;       /* peripheral qualifier                 */
		byte        modifier:7,         /* device-type modifier                 */
					rmb:1;              /* removeable media bit                 */
		byte        ansivers:3,         /* ANSI version                         */
					ecmavers:3,         /* ECMA version                         */
					isovers:2;          /* ISO version                          */
		byte        response:4,         /* response format                      */
					:4;                 /* reserved                             */
		byte        length;             /* additional length                    */
		byte        :8;                 /* reserved                             */
		byte        :8;                 /* reserved                             */
		byte        softreset:1,        /* reset alternative                    */
					queing:1,           /* command queing supported             */
					:1,                 /* reserved                             */
					linked:1,           /* linked commands supported            */
					sync:1,             /* synchronous transfer available       */
					wbus16:1,           /* 16 bit bus available                 */
					wbus32:1,           /* 32 bit bus available                 */
					reladr:1;           /* relative addressing available        */
		byte        vendor    [ 8];     /* vendor identification                */
		byte        product   [16];     /* product identification               */
		byte        revision  [ 4];     /* product revision level               */
		byte        serial    [ 8];     /* drive serial number (sometimes)      */
		short       terminator[ 1];     /* handy word to stop string output     */
	#endif

}   standard_inquiry_data;


/**------------------------------------------------------------------------ */
/* Scsi data record structures: Request Sense                               */
/*------------------------------------------------------------------------- */

typedef struct
{   
	#if __BIG_ENDIAN__
		byte        valid:1,            /* valid data bit                       */
					error_code:7;       /* error code                           */
		byte        segment_number;     /* segment number                       */
		byte        filemark:1,         /* file mark detected                   */
					eom:1,              /* end of media detected                */
					ili:1,              /* illegal length indicator             */
					:1,                 /* reserved                             */
					sense_key:4;        /* sense key                            */
		byte        information[4];     /* information                          */
		byte        additional_length;  /* additional length                    */
		byte        command_info[4];    /* command-inherent information         */
		byte        additional_code;    /* additional sense code                */
		byte        additional_quifier; /* additional sense code qualifier      */
		byte        field_replaceable;  /* field replaceable unit code          */
		byte        inherent[3];        /* sense key inherent information       */
		byte        :1,                 /* reserved                             */
					scsi_id:3,          /* scsi id                              */
					lun:4;              /* logical unit number                  */
		byte        cdb_opcode;         /* cdb opcode                           */
		byte        more_info[3];       /* extended information                 */
		byte        remaining_space[3]; /* remaining space                      */
	#else
		byte        error_code:7,       /* error code                           */
					valid:1;            /* valid data bit                       */
		byte        segment_number;     /* segment number                       */
		byte        sense_key:4,        /* sense key                            */
					:1,                 /* reserved                             */
					ili:1,              /* illegal length indicator             */
					eom:1,              /* end of media detected                */
					filemark:1;         /* file mark detected                   */
		byte        information[4];     /* information                          */
		byte        additional_length;  /* additional length                    */
		byte        command_info[4];    /* command-inherent information         */
		byte        additional_code;    /* additional sense code                */
		byte        additional_quifier; /* additional sense code qualifier      */
		byte        field_replaceable;  /* field replaceable unit code          */
		byte        inherent[3];        /* sense key inherent information       */
		byte        lun:4,              /* logical unit number                  */
					scsi_id:3,          /* scsi id                              */
					:1;                 /* reserved                             */
		byte        cdb_opcode;         /* cdb opcode                           */
		byte        more_info[3];       /* extended information                 */
		byte        remaining_space[3]; /* remaining space                      */
	#endif

}   request_sense_data;


/*------------------------------------------------------------------------- */
/* Scsi data record structures: Read Capacity                               */
/*------------------------------------------------------------------------- */

typedef struct
{   uint32      num_blocks;         /* device capacity in blocks            */
	uint32      block_size;         /* block size in bytes                  */

}   read_capacity_data;

#pragma pack(pop)

#endif

