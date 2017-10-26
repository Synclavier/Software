// =================================================================================
//	AbleOptLib
// =================================================================================

// Library for accessing Able Optical Format media

// 2/8/00 C. Jones

// Std C
#include <StdIO.h>
#include <String.h>

#if !__LP64__
    #include "LThread.h"
    #include "CInterChangeApp.h"
#endif

#include "CSharedOpticalDataBaseMM.h"
#include "CSynclavierSoundFileHeader.h"
#include "SoundfileTranslators.h"

// Local includes
#include "XPL.h"
#include "XPLRuntime.h"
#include "Syslits.h"
#include "Samplits.h"
#include "Devutil.h"
#include "SCSILib.h"
#include "SCSIlits.h"
#include "OPTLits.h"
#include "AbleDiskLib.h"
#include "AbleOptLib.h"


// =================================================================================
//		• Global Variables
// =================================================================================

fixed	s_sensekey;														// likely will be moved from here...


// =================================================================================
//		• Additional Devides
// =================================================================================

#define	quantum_controller	0											// non ld1200 (e.g. M/O, hard drive, image file)
#define	ld1200_controller	1											// ld1200
#define	magicnumber			((fixed) 0x8000)							// header revision number
#define	formatpattern		((fixed) 0x6363)							// format pattern written to magneto media
#define	defectmode			0											// auto-rewrite mode


// =================================================================================
//		• Static variables for caching
// =================================================================================

static	boolean	dev10set;
static	boolean	dev11set;
static	fixed	dev10ctrlr;
static	fixed	dev11ctrlr;
static	fixed	dev10blksiz;
static	fixed	dev11blksiz;
static	long	dev10atncnt;
static	long	dev11atncnt;

static	fixed* o_cur_bufptr;											// working pointer

static	char	o_volume_name[h_name_max*2 + 1];						// volume name
static  fixed	o_volume_time;

static  fixed	o_ms_header;
static  fixed	o_ls_header;

static	fixed	o_ms_dirstart;											// sector start of directory
static	fixed	o_ls_dirstart;

static	fixed	o_ms_dirlen;											// sector length of directory
static	fixed	o_ls_dirlen;

static	fixed	o_ms_datastart;											// sector start of data area
static	fixed	o_ls_datastart;

static	fixed	o_ms_datalen;											// sector length of data area
static	fixed	o_ls_datalen;

static	fixed	o_entrycount;											// no. directory entries on optical disk


// =================================================================================
//		• Handy class for managing o_cur_bufptr
// =================================================================================

class	stOptBuf
{
	public:
		stOptBuf (fixed *bufPtr);
		~stOptBuf();
};

stOptBuf::stOptBuf(fixed *bufPtr)
{
	o_cur_bufptr = bufPtr;
}

stOptBuf::~stOptBuf()
{
	o_cur_bufptr = NULL;
}


// =================================================================================
//		• AbleOptLib_finddevadr, AbleOptLib_lookup_controller_type, AbleOptLib_lookup_controller_block_size
// =================================================================================

/* This routine must be called before anything else to set up 
      the correct controller type. */

/* lookup_controller_type: interrogates drive for controller type and capacity   */
/* caches info for speedy access..												 */

/* lookup_controller_block_size: reports the device blocksize that was cached	 */
/* by lookup_controller_type													 */

static	scsi_device*	AbleOptLib_finddevadr(							// Find device address for this drive
	fixed	level)	_recursive	_swap									// level to check
{
    // We are hard-wired to ABLE_IMPORT_SCSI_ID in this implementation. This file is only used in Synclavier³
    scsi_device* itsDevice = g_indexed_device[ABLE_IMPORT_SCSI_ID];

	// Reset cached information on unit attention
	if (dev10set && level == 10 && itsDevice && itsDevice->fUnitAttnCount != dev10atncnt)
		dev10set = false;
		
	if (dev11set && level == 11 && itsDevice && itsDevice->fUnitAttnCount != dev11atncnt)
		dev11set = false;
				
	return (itsDevice);													// return device address (used to be scsi id)
}
	
static	fixed	AbleOptLib_lookup_controller_type(						// look up controller type for this level
	fixed	level)	_recursive
{
	scsi_device* itsDevice = AbleOptLib_finddevadr(level);				// access scsi device (checking for unit attention as well)
	fixed		 devtyp;
	fixed		 controller;
	
	if ((level    == 10)												// look up cached controller type for D10
	&&  (dev10set != 0 ))
		return (dev10ctrlr);
		
	if ((level    == 11)												// look up cached controller type for D11
	&&  (dev11set != 0 ))
		return (dev11ctrlr);
	
	if (!itsDevice)														// death by chocolate...
		return (quantum_controller);

	// Lock out other change in case we interrogate the
	// device and/or change its capacity
	StFastMutex	mutex(gXPLMutex);

	// Check for unit attention
	if ((itsDevice->fDeviceType == DEVICE_NOT_EXAMINED)					// if not examined or unit attention occured
	||  (itsDevice->fUnitAttention))
	{
		interrogate_device(itsDevice);

		itsDevice->fUnitAttention = FALSE;
	}
	
	// Trust we have interrogated the device by the time we get here
	if ((itsDevice->fDeviceType == DEVICE_NOT_EXAMINED      )			// if could not interrogate...
	||  (itsDevice->fDeviceType == DEVICE_DOES_NOT_EXIST    )
	||  (itsDevice->fDeviceType == DEVICE_NOT_TALKING       )
	||  (itsDevice->fDeviceType == DEVICE_UNCOOPERATIVE_DISK)
	||  (itsDevice->fDeviceType == DEVICE_RESERVED_DISK     )
	||  (itsDevice->fDeviceType == DEVICE_NOT_TALKING       )
	||  (itsDevice->fDeviceType == DEVICE_UNKNOWN_DEVICE    ))
	{
		return (quantum_controller);
	}
	
	devtyp = itsDevice->fStandardInquiryData.ptype;
	
	// Direct access devices and image files
	if (devtyp == 0)													// direct access - find size
	{
		if ((itsDevice->fNumBlocks == 0)								// if could not get block size
		||  (itsDevice->fBlockSize == 0)
		||  (itsDevice->fTotCyl    == 0)
		||  (itsDevice->fTotSec    == 0))
		{
			return (quantum_controller);
		}
					
		controller = quantum_controller;								// is quantum; size is set
	}

	// LD1200 WORMS
	else if (devtyp == 4)
	{
		controller = ld1200_controller;
	}
	
	// Unknown device type code (CDROM perhaps)
	else 
	{
		return (quantum_controller);
	}
	
	// Cache info
	if (level == 10)
	{ 	dev10set = true; dev10ctrlr = controller; dev10blksiz = itsDevice->fBlockSize; dev10atncnt = itsDevice->fUnitAttnCount;}
		
	if (level == 11)
	{ 	dev11set = true; dev11ctrlr = controller; dev11blksiz = itsDevice->fBlockSize; dev11atncnt = itsDevice->fUnitAttnCount;}
		
	return (controller);
}
	
static	fixed	AbleOptLib_lookup_controller_block_size(				// (presumes AbleOptLib_lookup_controller_type has just been called)
	fixed	level)	_recursive
	
{
	scsi_device* itsDevice = AbleOptLib_finddevadr(level);				// access scsi device (checking for unit attention as well)

	if ((level    == 10)												// look up block size for D10
	&&  (dev10set != 0 ))
		return (dev10blksiz);
		
	if ((level    == 11)												// look up block size for D11
	&&  (dev11set != 0 ))
		return (dev11blksiz);
		
	if (!itsDevice)														// death by chocolate...
		return (0);

	return (itsDevice->fBlockSize);
}


// =================================================================================
//		• AbleOptLib_map_error_code
// =================================================================================

// Maps scsi_error_code back to scsilits.h code
fixed	AbleOptLib_map_error_code(fixed status)
{
	if (status == GOOD_STATUS           )	return (s_nosense       );
	if (status == NO_RESPONSE           )	return (s_selfailed     );
	if (status == NOT_AVAILABLE         )	return (s_notready      );
	if (status == DEVICE_RESERVED       )	return (s_devicereserved);
	if (status == MEDIUM_ERROR_OCCURED  )	return (s_mediumerror   );
	if (status == HARDWARE_ERROR_OCCURED)	return (s_hardwareerror );
	if (status == BLANK_CHECK_OCCURED   )	return (s_blankcheck    );
	if (status == DEVICE_BUSY_OCCURED   )	return (s_devicebusy    );
	
	return (s_illegalrequest);	
}


// =================================================================================
//		• AbleOptLib_check_buf
// =================================================================================

// Return true if o_cur_bufptr is a blank check pattern
static	boolean	AbleOptLib_check_buf(									// check through buffer for a blank check
	fixed	seclen)														// sector length of data read into buffer
{
	long	wrdlen = seclen << 8;
	long	i;
	
	if (!o_cur_bufptr)													// no buffer: yup, must be blank!
		return (true);
	
	for (i=0; i<wrdlen; i++)
		if (o_cur_bufptr[i] != formatpattern)
			return (false);
	
	return (true);
}
	

// =================================================================================
//		• AbleOptLib_transferoptical
// =================================================================================

static	fixed	AbleOptLib_transferoptical(								// read data into the buffer
	fixed	cmd, 														// command to send
	fixed	ms_sec,
	fixed	ls_sec, 													// sector to read
	fixed	seclen, 													// no. sectors to read
	fixed	level)	_recursive	_swap									// level to read
{
	scsi_device*    itsDevice = NULL;
	fixed			controller;
	fixed			blksiz;
	fixed			status;
	uint32	        sec = (((uint32) (uint16) ms_sec) << 16) | ((uint32) (uint16) ls_sec);
	
	if (!o_cur_bufptr)													// no buffer?
		return (s_baddevice);
		
	itsDevice = AbleOptLib_finddevadr(level);							// access scsi device (checking for unit attention as well)

	if (!itsDevice)
		return (s_baddevice);											// return error
		
	controller = AbleOptLib_lookup_controller_type      (level);
	blksiz     = AbleOptLib_lookup_controller_block_size(level);
	
	if (blksiz == 0)													// if can't get block size, must be not ready
		return (s_notready);
	
	if (cmd == s_extendedread)
	{
		if (blksiz == 512)												// 512 media - do normally
			status = issue_read_extended(itsDevice, (byte *) o_cur_bufptr, sec, seclen);
		
		else if (blksiz == 1024)	
			status = issue_read_extended(itsDevice, (byte *) o_cur_bufptr, sec>>1, seclen>>1);
			
		else
			status = BAD_STATUS;
	} 

	else if (cmd == s_extendedwrite)
	{
		if (blksiz == 512)												// 512 media - do normally
			status = issue_write_extended(itsDevice, (byte *) o_cur_bufptr, sec, seclen);
		
		else if (blksiz == 1024)	
			status = issue_write_extended(itsDevice, (byte *) o_cur_bufptr, sec>>1, seclen>>1);
			
		else
			status = BAD_STATUS;
	} 

	else
		status = BAD_STATUS;

	if (status)																// if error
		return (AbleOptLib_map_error_code(status));							// return able-style error

	if ((controller == quantum_controller) && (cmd == s_extendedread)) {	// using Winchester simulation (or magneto optical)
		if (AbleOptLib_check_buf(seclen))									// look through buffer for format pattern
			status = s_blankcheck;											// if format pattern is found, force a BlankCheck
	}
	
	return (status);
}


// =================================================================================
//		• AbleOptLib_modeselectoptical
// =================================================================================

static	fixed	AbleOptLib_modeselectoptical(							// set up optical disk
	fixed	level)	_recursive	_swap									// level to set up
{
	scsi_device*    itsDevice = NULL;
	fixed			i, j;
	fixed			controller;

	itsDevice = AbleOptLib_finddevadr(level);							// access scsi device (checking for unit attention as well)

	if (!itsDevice)
		return (s_baddevice);											// return error
		
	controller = AbleOptLib_lookup_controller_type(level);
	
	if (controller == ld1200_controller) {								// optical disk
		byte			buf[16];										// mode select buffer
	
		i = issue_mode_sense(itsDevice, 0, buf, 16);					// get sense bytes

		if (i != GOOD_STATUS)
			return (AbleOptLib_map_error_code(i));
			
		if ((shr(buf[12],5) & 0x0003) != defectmode) {					// not in correct defect mode
			for (i = 0; i < 16; i++) {									// zero out buffer
				buf[i] = 0;
			}
			buf[ 3] = 8;												// block descriptor length
			buf[10] = 0x0004;											// 1024-byte block size
			buf[11] = 0x0000;
			buf[12] = shl(defectmode,5);								// use correct defect mode
			buf[13] = 0;												// use defaults
			buf[15] = 0;												// use default reselection delay
			
			i = issue_mode_select(itsDevice, buf, 16);					// select these parameters

			if (i != GOOD_STATUS)
				return (AbleOptLib_map_error_code(i));
		}
	}
		
	else {																// magneto optical
		long    now = TickCount();

		i = issue_test_ready(itsDevice);

		while (i == DEVICE_BUSY_OCCURED)								// some units report busy if media inserted???
		{
			usleep(10000);
            
			if ((j = (TickCount() - now)) > 180)
				break;
	
			i = issue_test_ready(itsDevice);
		}

		if (i != GOOD_STATUS)
			return (AbleOptLib_map_error_code(i));
	}
		
	return (s_good);													// everything's cool
}
	

// =================================================================================
//		• AbleOptLib_readheader
// =================================================================================

static	boolean			AbleOptLib_readheader(							// read volume header and set up globals for this disk
	fixed	level)	_recursive	_swap									// level to read
{
	scsi_device*    itsDevice = NULL;
	fixed			ms_sec, ls_sec;
	fixed			hmsb, hlsb;
	fixed			controller;
	fixed			blocksize;
	fixed			i;
	
	c_status   = e_none;												// assume no error
	s_sensekey = s_good;
	
	itsDevice = AbleOptLib_finddevadr(level);							// access scsi device (checking for unit attention as well)

	if (!itsDevice)
	{
		c_status = e_no_config;
		return (false);
	}

	controller = AbleOptLib_lookup_controller_type      (level);
	blocksize  = AbleOptLib_lookup_controller_block_size(level);
	
	if (controller == quantum_controller) {
		s_sensekey = AbleOptLib_modeselectoptical(level);				// make sure drive is set up correctly
		if (s_sensekey != s_good) return (false);
	}
		
	ms_sec = 0;															// start at first sector on disk
	ls_sec = 0;
	
	hmsb = (-1); hlsb = (-1);											// assume no header found
	
	s_sensekey = AbleOptLib_transferoptical(s_extendedread,ms_sec,ls_sec,2,level);	// try to read the first volume header
	
	if ((s_sensekey != s_good       )
	&&  (s_sensekey != s_mediumerror))
		return (false);													// not formatted, or some other error
		
	// This next block will catch magneto media that has not been
	// formatted or not formatted slip sector.
	
	if (controller == quantum_controller) {								// magneto optical drive
		if (s_sensekey == s_mediumerror) {								// never been formatted at all ?
			s_sensekey = s_good;										// what we really want is an optical catalog error, not bad SCSI status
			c_status = e_not_initialized;								// must format the media first with FORMCOPY
			return (false);
		}
		else if (s_sensekey == s_good) {								// could read it, check format type
			if (blocksize == 1024)										// only look further if may be M/O (vs. hard disk)
			{															// in other words, skip the LNR check for Jaz drives...
				byte	buf[16];										// mode select buffer
				i = issue_mode_sense(itsDevice, 0x002A, buf, 16);		// get sense bytes
				if (i == GOOD_STATUS) {									// if tahiti 1...  E.G. T-4 failes the above mode sense
					if (shr(buf[14],1) == 1) {							// Lnr bit is set, not slip sector
						c_status = e_not_initialized;					// must format the media with FORMCOPY
						return (false);
					}
				}
				else s_sensekey = s_good;								// just set good sense for T-4
			}
		}
	}
		
	if (s_sensekey == s_good) {											// save good sector #
		hmsb = ms_sec; hlsb = ls_sec;
	}
		
	i = 0;
	
	while (((_ILT_(i, (headergapsize-1))   ))
	&&      ((s_sensekey == s_good       )
	||       (s_sensekey == s_mediumerror))) {
		ls_sec = ls_sec + 2;											// try to read the next header
		if (_ILT_(ls_sec, 2)) ms_sec = ms_sec + 1;
			
		i = i + 1;
		
		s_sensekey = AbleOptLib_transferoptical(s_extendedread,ms_sec,ls_sec,2,level);	// try to read the next volume header
		
		if (s_sensekey == s_good) {										// we read something that was there
			hmsb = ms_sec; hlsb = ls_sec;								// save good sector number
		}
		else if ((s_sensekey != s_blankcheck )
		&&       (s_sensekey != s_mediumerror))							// something weird happened
			return (false);
	}
		
	if (hmsb == (-1))													// if we get nothing but medium errors,
		return (false);													// then cannot read.
		
	s_sensekey = AbleOptLib_transferoptical(s_extendedread,hmsb,hlsb,2,level);		// read the good one again
	
	if (s_sensekey != s_good)											// could not read it a second time?
		return (false);
		
	if ((controller == quantum_controller)								// optical type is M/O
	&& (o_cur_bufptr[h_magic] != magicnumber)							// it doesn't have a header yet
	&& (o_cur_bufptr[h_magic] != formatpattern)) {						// and can't find the format pattern either
		c_status = e_not_initialized;									// must format the media first with FORMCOPY
		return (false);
	}
		
	if ((controller == ld1200_controller)								// optical type is LMS WORM
	&& (o_cur_bufptr[h_magic] != magicnumber)) {						// header not in NED format
		c_status = e_bad_volume;										// volume in bad state
		return (false);
	}
	
	o_ms_header = hmsb;													// publish header sector for possible rewrite
	o_ls_header = hlsb;

	for (i=0; i<h_name_max; i++)
	{
		o_volume_name[i*2    ] = o_cur_bufptr[h_name+i] & 0xFF;
		o_volume_name[i*2 + 1] = o_cur_bufptr[h_name+i] >> 8;
		o_volume_name[i*2 + 2] = 0;
	}
	
	o_volume_time  = o_cur_bufptr[h_time];								// latch volume time
	
	o_ls_dirstart  = o_cur_bufptr[h_ls_dirstart ];						// store sector start of directory
	o_ms_dirstart  = o_cur_bufptr[h_ms_dirstart ];
	
	o_ls_dirlen    = o_cur_bufptr[h_ls_dirlen   ];						// store sector length of directory
	o_ms_dirlen    = o_cur_bufptr[h_ms_dirlen   ];
	
	o_ls_datastart = o_cur_bufptr[h_ls_datastart];						// store sector start of data area
	o_ms_datastart = o_cur_bufptr[h_ms_datastart];
	
	o_ls_datalen   = o_cur_bufptr[h_ls_datalen  ];						// store sector length of data area
	o_ms_datalen   = o_cur_bufptr[h_ms_datalen  ];
	
	return (true);														// header read correctly
}
	

// =================================================================================
//		• AbleOptLib_readheader
// =================================================================================

static	boolean	AbleOptLib_findnextunused(								// find next unused block in the given section
	fixed	ms_start,
	fixed	ls_start, 													// starting sector of region to search
	fixed	ms_len,
	fixed	ls_len, 													// sector length of region to search
	fixed	level)	_recursive	_swap									// level to search
{
	fixed			ms_first, ls_first;									// first sector of region to search
	fixed			ms_end, ls_end;										// end of directory
	fixed			msw, lsw;
	fixed			i;
	
	s_sensekey = s_good;												// assume no errors will occur
	
	ls_start = shl(ms_start,15) | shr(ls_start,1);						// convert to 1024-byte starting sector
	ms_start = shr(ms_start,1);
	
	ls_len = shl(ms_len,15) | shr(ls_len,1);							// convert to 1024-byte sector length
	ms_len = shr(ms_len,1);
	
	ms_end = ms_start + ms_len;											// calculate the end of the area
	ls_end = ls_start + ls_len;
	if (_ILT_(ls_end, ls_start)) ms_end = ms_end + 1;
		
	ms_first = ms_start;												// Start at beginning of area
	ls_first = ls_start;
	
	while (! ( (ms_len == 0) && (ls_len == 0) )) {						// Repeat until entire area has been searched
	
		msw = ms_first + shr(ms_len,1);									// get midpoint
		lsw = ls_first + (shl(ms_len,15) | shr(ls_len,1));
		if (_ILT_(lsw, ls_first)) msw = msw + 1;
			
		i = AbleOptLib_transferoptical(s_extendedread,shl(msw,1) | shr(lsw,15),shl(lsw,1),2,level);	// try to read this sector
		
		if (i == s_good) {												// this sector has data in it
			msw = ms_first + shr(ms_len,1);								// skip half the region
			lsw = ls_first + (shl(ms_len,15) | shr(ls_len,1));
			if (_ILT_(lsw, ls_first)) msw = msw + 1;
				
			ms_first = msw;												// add one to new starting sector
			ls_first = lsw + 1;
			if (_ILT_(ls_first, 1)) ms_first = ms_first + 1;
				
			if (ls_len == 0) ms_len = ms_len - 1;						// subtract one from length
			ls_len = ls_len - 1;
		}
		else if ((i == s_blankcheck) || (i == s_mediumerror)) {			// could not read this sector
																		// search at a lower address
		}
		else {															// die if some other error occurred
			s_sensekey = i;												// store error status
			return (false);
		}
			
		ls_len = shl(ms_len,15) | shr(ls_len,1);						// divide no. sectors to search by 2
		ms_len = shr(ms_len,1);
	}
		
	f_ms_sector = shl(ms_first,1) | shr(ls_first,15);					// convert back to 512-byte sectors
	f_ls_sector = shl(ls_first,1);
	
	if ((  _IGT_(ms_first, ms_end) )
	|| ( (ms_first  ==  ms_end) && (_IGE_(ls_first, ls_end)) )) {		// no unsed sector found
		return (false);													// no more room in directory
	}
		
	return (true);														// it's been found
}
	

// =================================================================================
//		• AbleOptLib_sectoentry, AbleOptLib_entrytosec
// =================================================================================

static	fixed			AbleOptLib_sectoentry(							// convert sector no. to directory entry no.
	fixed	msw,
	fixed	lsw)	_recursive	_swap									// 32-bit sector number
	
{
	msw = msw - o_ms_dirstart;											// subtract starting sector of directory
	if (_ILT_(lsw, o_ls_dirstart)) msw = msw - 1;
	lsw = lsw - o_ls_dirstart;
	
	return (shl(msw,15) | shr(lsw,1));									// divide by two to get entry no.
}
	
static	void			AbleOptLib_entrytosec(							// convert entry no. to a sector no.
	fixed	entry)	_recursive	_swap									// entry no. to convert
	
{
	f_ms_sector = shr(entry,15);										// turn entry number into a sector offset
	f_ls_sector = shl(entry,1);
	
	f_ms_sector = o_ms_dirstart + f_ms_sector;							// add sector offset to beginning of directory area
	f_ls_sector = o_ls_dirstart + f_ls_sector;
	if (_ILT_(f_ls_sector, o_ls_dirstart)) f_ms_sector = f_ms_sector + 1;
}
	

// =================================================================================
//		• AbleOptLib_readentry, AbleOptLib_writeentry
// =================================================================================

static	fixed		AbleOptLib_readentry(								// read a directory entry
	fixed	entry, 														// entry to read
	fixed	level)	_recursive	_swap									// level to read from
{
	fixed			i;
	
	AbleOptLib_entrytosec(entry);										// convert entry no. to it's 32-bit sector number
	
	i = AbleOptLib_transferoptical(s_extendedread,f_ms_sector,f_ls_sector,2,level);	// try to read this sector
	return (i);																		// return status of read
}
	
static	fixed		AbleOptLib_writeentry(								// write a directory entry
	fixed	entry, 														// entry to read
	fixed	level)	_recursive	_swap									// level to read from
{
	fixed			i;
	
	AbleOptLib_entrytosec(entry);										// convert entry no. to it's 32-bit sector number
	
	i = AbleOptLib_transferoptical(s_extendedwrite,f_ms_sector,f_ls_sector,2,level);	// try to write this sector
	return (i);															// return status of write
}


// =================================================================================
//		• AbleOptLib_readentries, AbleOptLib_writeentries
// =================================================================================

static	fixed		AbleOptLib_readentries(								// read directory entries
	fixed	entry, 														// first entry to read
	fixed	entrycount, 												// no. entries to read
	fixed	level)	_recursive	_swap									// level to read from
{
	fixed			i;
	
	AbleOptLib_entrytosec(entry);										// convert entry no. to it's 32-bit sector number
	
	i = AbleOptLib_transferoptical(s_extendedread,f_ms_sector,f_ls_sector,shl(entrycount,1),level);	// try to read these sectors
	return (i);															// return status of read
}
	
static	fixed		AbleOptLib_writeentries(							// write directory entries
	fixed	entry, 														// first entry to write
	fixed	entrycount, 												// no. entries to write
	fixed	level)	_recursive	_swap									// level to write to
{
	fixed			i;
	
	AbleOptLib_entrytosec(entry);										// convert entry no. to it's 32-bit sector number
	
	i = AbleOptLib_transferoptical(s_extendedwrite,f_ms_sector,f_ls_sector,shl(entrycount,1),level);	// try to write these sectors
	return (i);															// return status of write
}
	

// =================================================================================
//		• AbleOptLib_findentrycount
// =================================================================================

static	fixed			AbleOptLib_findentrycount(						// find no. directory entries
	fixed	level)	_recursive	_swap									// level to look at
{
	boolean			found;
	
	s_sensekey = s_good;												// assume no errors
	c_status   = e_none;
	
	if (o_entrycount == -1) {											// optical disk not mounted yet
		found = AbleOptLib_findnextunused(o_ms_dirstart,o_ls_dirstart,o_ms_dirlen,o_ls_dirlen,level);	// could not find free entry
		
		if ((found & 1) || ((! (found & 1)) && (s_sensekey == s_good))) {	// directory area searched without any SCSI errors
			return (AbleOptLib_sectoentry(f_ms_sector,f_ls_sector));		// convert sector to directory entry and return
		}
	}
	else {																// optical disk must already be mounted
		
		s_sensekey = AbleOptLib_readentry(o_entrycount,level);			// try to read this entry
		
		if ((s_sensekey == s_blankcheck) || (s_sensekey == s_mediumerror)) {	// could not read it, so continue
			if (o_entrycount == 0)												// there could not be an entry before this one
				s_sensekey = s_good;
			else s_sensekey = AbleOptLib_readentry(o_entrycount - 1,level);		// try to read entry before this
				
			if (s_sensekey == s_good) {									// entry could be read, so this must be the end of directory
				return (o_entrycount);									// return no. directory entries
			}
			else if (s_sensekey == s_blankcheck) {						// could not read one before it
				s_sensekey = s_good;									// clear sense key
				c_status = e_volume_changed;							// volume must be mounted again
			}
		}
		else if (s_sensekey == s_good) {								// sector already written
			c_status = e_volume_changed;								// volume must be mounted again
		}
	}
		
	return (-1);														// signal error condition
}


// =================================================================================
//		• AbleOptLib_searchempty1024kblocks
// =================================================================================

static	boolean			AbleOptLib_searchempty1024kblocks(				// search for empty data blocks
	fixed	ms_start,
	fixed	ls_start, 													// sector to start search at
	fixed	scanlen, 													// no. BLOCKS    to scan
	fixed	emptylen, 													// no. empty BLOCKS to search for
	fixed	level)	_recursive	_swap									// level to read
{
	scsi_device*    itsDevice = NULL;
	fixed			controller;
	fixed			dataBuf[5];											// 10 bytes of search-emtpy-blocks data
	int				status;

	s_sensekey = s_good;												// assume no errors
	
	itsDevice = AbleOptLib_finddevadr(level);							// access scsi device (checking for unit attention as well)

	if (!itsDevice)
	{
		s_sensekey = s_baddevice;										// bad device
		return (false);
	}
		
	controller = AbleOptLib_lookup_controller_type(level);
	
	if (controller == quantum_controller) {								// winchester simulation
		f_ms_sector = ms_start;
		f_ls_sector = ls_start + 2;
		if (_ILT_(f_ls_sector, 2)) f_ms_sector = f_ms_sector + 1;
		return (true);
	}
	
	dataBuf[0] = shr(ms_start,1);										// Starting 1024-byte block (MSW)
	dataBuf[1] = (shl(ms_start,15) | shr(ls_start,1));					// Starting 1024-byte block (LSW)
	dataBuf[2] = 0;														// No. 1024-byte blocks to scan (MSW)
	dataBuf[3] = scanlen;												// No. 1024-byte blocks to scan (LSW)
	dataBuf[4] = emptylen;												// No. 1024-byte empty blocks to scan for (LSW)
    
    // Original Able code was:
    // write(ScsiWord) = shr(MS_Start,1);                       /* Starting 1024-byte block (MSW) */
    // write(ScsiWord) = (shl(MS_Start,15) or shr(LS_Start,1)); /* Starting 1024-byte block (LSW) */
    // write(ScsiWord) = 0;                                     /* No. 1024-byte blocks to scan (MSW) */
    // write(ScsiWord) = ScanLen;                               /* No. 1024-byte blocks to scan (LSW) */
    // write(ScsiWord) = EmptyLen;                              /* No. 1024-byte empty blocks to scan for (LSW) */
    
    // Swap bytes for D24
    if (itsDevice->fDevicePort == D24_SCSI_PORT) {
        dataBuf[0] = CFSwapInt16HostToBig(dataBuf[0]);
        dataBuf[1] = CFSwapInt16HostToBig(dataBuf[1]);
        dataBuf[2] = CFSwapInt16HostToBig(dataBuf[2]);
        dataBuf[3] = CFSwapInt16HostToBig(dataBuf[3]);
        dataBuf[4] = CFSwapInt16HostToBig(dataBuf[4]);
    }

	status = issue_search_empty_blocks(itsDevice, (byte *) &dataBuf[0], 10);

	if (status == DEVICE_COND_MET_OCCURED) {							// found empty blocks
		status = issue_request_sense(itsDevice);

		if (status == GOOD_STATUS) {									// got sense data
			f_ms_sector = ( shl(itsDevice->fRequestSenseData.information[0],8) | itsDevice->fRequestSenseData.information[1] );		// store in buffer
			f_ls_sector = ( shl(itsDevice->fRequestSenseData.information[2],8) | itsDevice->fRequestSenseData.information[3] );
			f_ms_sector = ( shl(f_ms_sector,1) | shr(f_ls_sector,15) );	// convert back to 512-byte sectors
			f_ls_sector = ( shl(f_ls_sector,1) );
			return (true);												// success
		}
	}
	
	s_sensekey = AbleOptLib_map_error_code(status);						// map to able error codes
	
	return (false);														// failure
}
	

// =================================================================================
//		• AbleOptLib_searchemptyblocks
// =================================================================================

static	boolean		AbleOptLib_searchemptyblocks(						// search for empty data blocks
	fixed	ms_start,
	fixed	ls_start, 													// sector to start search at
	fixed	scanlen, 													// no. sectors to scan
	fixed	emptylen, 													// no. empty sectors to search for
	fixed	level)	_recursive	_swap									// level to read
	
{
	return (AbleOptLib_searchempty1024kblocks(ms_start, ls_start, shr(scanlen,1),  shr(emptylen,1), level));
}

	
// =================================================================================
//		• AbleOptLib_get_statistics
// =================================================================================

static	boolean		AbleOptLib_get_statistics(							// get disk statistics
	fixed	level, 														// level to get statistics from
	fixed	buf[])	_recursive	_swap									// buffer to return data in
{
	fixed			ms_start, ls_start;									// first physical sector of free data area
	fixed			ms_used, ls_used;									// no. sectors used on this disk
	fixed			ms_free, ls_free;									// no. sectors free on this disk
	fixed			ms_end, ls_end;										// end of data area
	fixed			freeentry;											// next free entry in directory
	fixed			lastentry;											// last directory entry in directory
	fixed			lastfound = (-1);
	boolean			found;
	fixed			i;
	
	if (!o_cur_bufptr)													// no buffer?
		return (false);

	freeentry = AbleOptLib_findentrycount(level);						// find the no. entries in directory
	if (freeentry == -1) return (false);								// an error occurred
		
	lastentry = freeentry;												// now search for the last directory entry
	found = false;														// have not found entry yet
	while ((lastentry != 0) && (! (found & 1))) {						// loop over directory entries
	
		lastentry = lastentry - 1;										// back up one entry
		
		s_sensekey = AbleOptLib_readentry(lastentry,level);
		if (s_sensekey != s_good) return (false);						// error reading entry
			
		if (o_cur_bufptr[e__type] == e_dir_entry)
		{
			found = true;												// this is a directory entry, so we've found what we're looking for
			lastfound = lastentry;
		}
	}																	// looping over directory entries
		
	if (found & 1) {													// directory entry was found
		ms_start = o_cur_bufptr[e_ms_secstart] + o_cur_bufptr[e_ms_seclen];	// calculate start of file
		ls_start = o_cur_bufptr[e_ls_secstart] + o_cur_bufptr[e_ls_seclen];
		if (_ILT_(ls_start, o_cur_bufptr[e_ls_secstart])) ms_start = ms_start + 1;
			
		if ((ls_start & 0x0001) != 0) {									// cannot have odd starting sectors
			ls_start = ls_start + 1;									// add one to get even starting sectors
			if (ls_start == 0) ms_start = ms_start + 1;
		}
			
		if (_ILT_(ls_start, 2)) ms_start = ms_start - 1;				// calculate last sector of previous file
		ls_start = ls_start - 2;
		
		ms_end = o_ms_datastart + o_ms_datalen;							// compute end of data area
		ls_end = o_ls_datastart + o_ls_datalen;
		if (_ILT_(ls_end, o_ls_datastart)) ms_end = ms_end + 1;
			
		if (AbleOptLib_searchemptyblocks(ms_start,ls_start,0,8,level) & 1) {	// find next physical block
			if ((_IGT_(f_ms_sector, ms_end))									// empty blocks found beyond end of data area
			|| ((f_ms_sector  ==  ms_end) && (_IGE_(f_ls_sector, ls_end)))) {
				ms_start = ms_end;												// start at end of data area
				ls_start = ls_end;
			}
			else {
				ms_start = f_ms_sector;									// store physical starting sector
				ls_start = f_ls_sector;
			}
		}
		else if (s_sensekey == s_good) {								// empty blocks not found
			ms_start = ms_end;											// start at end of data area
			ls_start = ls_end;
		}
		else return (false);											// Scsi error occurred
	}
	else {																// entry was not found, so start at beginning of data area
		ms_start = o_ms_datastart;
		ls_start = o_ls_datastart;
	}
		
	ms_used = ms_start - o_ms_datastart;								// calculate the no. sectors used
	if (_ILT_(ls_start, o_ls_datastart)) ms_used = ms_used - 1;
	ls_used = ls_start - o_ls_datastart;
	
	ms_free = o_ms_datalen - ms_used;									// calculate no. free sectors
	if (_ILT_(o_ls_datalen, ls_used)) ms_free = ms_free - 1;
	ls_free = o_ls_datalen - ls_used;
	
    i = (fixed)(((((uint32) (uint16) ms_free) << 16) | ((uint32) (uint16) ls_free)) / 1000);

    // _write_5(ls_free);                                               // calculate the no. bad sectors
	// _write_4(ms_free);
	// _write_7(1000);                                                  // assume .1% of these free sectors will be bad
	// i = _read_5;
	
	if (_ILT_(ls_free, i)) ms_free = ms_free - 1;						// subtract the no. bad sectors
	ls_free = ls_free - i;												// this is the true no. sectors free
	
	/* store these values */
	
	buf[stat_dir_used ] = freeentry;									// no. entries used
	buf[stat_dir_free ] = (shl(o_ms_dirlen,15) | shr(o_ls_dirlen,1)) - freeentry;
	buf[stat_dir_start] = freeentry;									// first free entry
	
	buf[stat_ms_used  ] = ms_used;										// no. data sectors used
	buf[stat_ls_used  ] = ls_used;
	buf[stat_ms_free  ] = ms_free;										// no. data sectors free
	buf[stat_ls_free  ] = ls_free;
	buf[stat_ms_start ] = ms_start;										// first free sector in data area
	buf[stat_ls_start ] = ls_start;
	buf[stat_dir_last ] = lastfound;

	return (true);														// got statistics
}
	

// =================================================================================
//		• AbleOptLib_init_optical_controller
// =================================================================================

// Marginally useful routine to pre-cache controller type and
// block size information for levels 10 and 11
static	void	AbleOptLib_init_optical_controller()					// set optical controller type based on system configuration
{
	dev10set = false;													// force re-polling in all cases if we are called
	dev11set = false;
	
	AbleOptLib_lookup_controller_type(10);								// set both types if configured & running
	AbleOptLib_lookup_controller_type(11);
}
	
	
// =================================================================================
//		• AbleOptLib_InitializeDeviceSetup()
// =================================================================================

void	AbleOptLib_InitializeDeviceSetup()
{
	dev10set = false;
	dev11set = false;
}


// =================================================================================
//		• AbleOptLib_ReadOpticalDirectory()
// =================================================================================

int	AbleOptLib_ReadOpticalDirectory(InterChangeItemDirectory &itsData, const char *the_tree_name)
{
	StFastMutex	    semex(gCatMutex);           // Protect catalog      global variables
	StFastMutex		mutex(gOptMutex);			// Protect optical disk global variables
	char   			er_mess[MESSAGE_BUF_SIZE];
	fixed			entry_buffer[512];
	stOptBuf		optBuf(&entry_buffer[0]);
	fixed			stat_buf[stat_rec_length] = {0};// statistics array
	
    memset(&itsData, 0, sizeof(itsData));

    itsData.root.OpticalItem.file_system      = InterChange_AbleOpticalFileSystemCode;
    itsData.root.OpticalItem.file_type        = InterChange_AbleOpticalDirectory;
    itsData.root.OpticalItem.file_device_code = 10;
    
	// Discard any cached directory information
	o_entrycount  = -1;													// optical disk not mounted yet
	
	// Read basic header
	if ((!AbleOptLib_readheader    (itsData.root.OpticalItem.file_device_code          ))
	||  (!AbleOptLib_get_statistics(itsData.root.OpticalItem.file_device_code, stat_buf)))
	{
		if (c_status)
		{
			get_cat_code_message(c_status, er_mess);
			printf("Synclavier³: Could not read directory for '%s'\n   Catalog error Report:\n", the_tree_name);
			printf("   %s\n", er_mess);
			return (c_status);
		}
		
		if (s_sensekey)
		{
			get_sense_code_message(s_sensekey, er_mess);
			printf("Synclavier³: Could not read directory for '%s'\n   SCSI error Report:\n", the_tree_name);
			printf("   %s\n", er_mess);
			return (256+s_sensekey);
		}
	}

	// Publish information about the directory
	InterChangeOpticalData&	optData = itsData.optical_data;
    
    // Remove trailing spaces and bogus characters from volume name
    int i = (int) strlen(o_volume_name);
	
    while (i > 0 && (o_volume_name[i-1] <= ' ' || o_volume_name[i-1] > 'z'))
        o_volume_name[i-1] = 0;
    
    if (strlen(o_volume_name) == 0)
        strcpy(optData.volume_name, "OPTICAL_VOLUME");
    else
        strcpy(optData.volume_name, o_volume_name);
	
	optData.volume_time = (unsigned short) o_volume_time;
	
	optData.header     = (((uint32) (uint16) o_ms_header   ) << 16) | ((uint32) (uint16) o_ls_header   );
	optData.dir_start  = (((uint32) (uint16) o_ms_dirstart ) << 16) | ((uint32) (uint16) o_ls_dirstart );
	optData.dir_len	   = (((uint32) (uint16) o_ms_dirlen   ) << 16) | ((uint32) (uint16) o_ls_dirlen   );
	optData.data_start = (((uint32) (uint16) o_ms_datastart) << 16) | ((uint32) (uint16) o_ls_datastart);
	optData.data_len   = (((uint32) (uint16) o_ms_datalen  ) << 16) | ((uint32) (uint16) o_ls_datalen  );
	
	optData.dir_used   =   (uint32) (uint16) stat_buf[stat_dir_used ];
	optData.dir_free   =   (uint32) (uint16) stat_buf[stat_dir_free ];
	optData.dir_next   =   (uint32) (uint16) stat_buf[stat_dir_start];
	optData.data_used  = (((uint32) (uint16) stat_buf[stat_ms_used  ]) << 16) | ((uint32) (uint16) stat_buf[stat_ls_used ]);
	optData.data_free  = (((uint32) (uint16) stat_buf[stat_ms_free  ]) << 16) | ((uint32) (uint16) stat_buf[stat_ls_free ]);
	optData.data_next  = (((uint32) (uint16) stat_buf[stat_ms_start ]) << 16) | ((uint32) (uint16) stat_buf[stat_ls_start]);
	optData.dir_last   =   (uint32) ( int16) stat_buf[stat_dir_last ];
	
	itsData.blocks_total = optData.dir_len + optData.data_len;
	itsData.blocks_used  = (optData.dir_used << 1) + optData.data_used;  // Entries used * 2 blocks/entry + data used

	itsData.optical_data_valid = true;
	
	return (noErr);
}


// =================================================================================
//		• AbleOptLib_ParseCaptionsFromSFHeader()
// =================================================================================

// Scans up to 8 caption strings from a sound file header
static	void AbleOptLib_ParseCaptionsFromSFHeader(fixed gbuf[], LCStr255 theCaps[8], int& numCaps)
{
	fixed	nextCatPtr = gbuf[sf_index_base];;
	int 	i, k, l;
	
	numCaps = 0;
	
	if (nextCatPtr == 0) nextCatPtr = 256;

	for (i=0; i<8; i++)
	{
		if (nextCatPtr > 0 && nextCatPtr < 256)
		{
			LCStr255& theCap = theCaps[numCaps];
			int	 	  len    = gbuf[nextCatPtr];
			
			if (len > 40) len = 40;
			
			for (k=0; k<len; k++)
				theCap[k] = byte(&gbuf[nextCatPtr], k);
			
			nextCatPtr += (1 + ((gbuf[nextCatPtr]+1)>>1));
			
			// disallow null category names (indicates serious error)
			if (theCap.Length() == 0)
				continue;
            
            // Ignore cataegories that begin with :; that is apparently a reference to the sub-category
            // which is duplicated
            if (theCap[0] == ':')
                continue;
            
			// Could disallow leading :'s in categories
            // if (theCap[0] == ':')
            //     for (l=0; theCap[l]; l++)
            //         theCap[l] = theCap[l+1];
				
			// disallow repeated :'s in category name
			char *itsString = (char *) theCap;
			
			for (k=0; itsString[k]; k++)
			{
				while ((itsString[k  ] == ':')
				&&     (itsString[k+1] == ':'))
				{
					for (l=k; itsString[l]; l++)
						itsString[l] = itsString[l+1];
				}
			}
			
			// remove trailing colons
			while (itsString[k-1] == ':')
				itsString[--k] = 0;
				
			// got one caption
			numCaps++;
		}
		
		else
			break;
	}
}


// =================================================================================
//		• AbleOptLib_ConstructOpticalIndex()
// =================================================================================

// Higher level routine called from a thread to construct a complete optical disk
// index (file list, category list, etc.)

// Data base must be accessed with no readers and HFS refnum latched before calling
int     AbleOptLib_ConstructOpticalIndex(scsi_device* itsDevice, CSharedOpticalDataBase& itsOpticalData, long& abortMe, long& fatalUpdateErrorOccured)
{
    int             chunkSize    = 128;											// 128 entries per disk chunk
    int             entrySize	 = 1024;										// 1024 bytes per entry
    SyncStackBuffer ssb(chunkSize*entrySize);                                   // Create a self-releasing buffer
	byte			*entryBuf    = ssb.Bytes();									// buffer for reading directory entries
	int             firstEntry   = 0;											// first and last entry to read
	int             lastEntry    = 0;
	Boolean			errorPosted  = false;

    InterChangeItemDirectory itsData;
    const char*              treeName = "";
    
    if (AbleOptLib_ReadOpticalDirectory(itsData, treeName))
        return -1;

    // Move the optical data to the optical data base
    itsOpticalData.mOpticalData = itsData.optical_data;
    
	fatalUpdateErrorOccured = false;											// init to no error
	
	firstEntry = 0;
	lastEntry  = itsOpticalData.mOpticalData.dir_used;
	
    AbleOptLib_ReleaseOpticalIndex(itsOpticalData);
    
	if (!itsDevice || !entryBuf)
	{
		fatalUpdateErrorOccured = true;
		return (-1);
	}

	// Handle running out of memory
	try
	{
		// If we need to update the optical disk, read through all the directory entries
		// and construct the optical data base
		
		while (firstEntry < lastEntry && !abortMe)
		{
			// Read a chunk of entries
			int     chunkEntries = lastEntry - firstEntry;
			int     blockNum     = itsOpticalData.mOpticalData.dir_start + (firstEntry << 1);
			int     blockSize    = itsDevice->fBlockSize;
			short	status;
			
			if (chunkEntries > chunkSize)
				chunkEntries = chunkSize;
			
			retry_chunk:;
			
			if (blockSize == 512)													// 512 media
				status = issue_read_extended(itsDevice, entryBuf, blockNum, chunkEntries<<1);

			else if (blockSize == 1024)												// 1024 media
				status = issue_read_extended(itsDevice, entryBuf, blockNum>>1, chunkEntries);
				
			else
				status = BAD_STATUS;
			
			// Check for abort
			if (abortMe)
				continue;

			// If medium error, try reading smaller chunk
			if (status == MEDIUM_ERROR_OCCURED)
			{
				if (chunkEntries > 1)
				{
					chunkEntries >>= 1;												// try smaller chunk to read

					if (abortMe)
						continue;
				
					goto retry_chunk;
				}
				
				if (!errorPosted)
				{
					printf("Synclavier³: Optical Disk catalog entry could not be read due to medium error\n");
					errorPosted = true;
				}
			}
			
			// Could not read
			if (status)
			{
				if (!errorPosted)
				{
					char er_mess[MESSAGE_BUF_SIZE];
				
					get_sense_code_message(AbleOptLib_map_error_code(status), er_mess);
					printf("Synclavier³: Optical Disk catalog entry could not be read\n   SCSI error Report:\n");
					printf("   %s\n", er_mess);
					errorPosted = true;
				}
				
				memset(entryBuf, 0, chunkEntries*entrySize);							// provide all zeroes of data we could not read
			}		

			// Process the entries into the optical data base
			fixed* gbuf = (fixed*) &entryBuf[0];		

			while (chunkEntries)
			{
                SOpticalFileData* newData = new SOpticalFileData;
                
                if (!newData) {
                    chunkEntries--;
                    firstEntry++;
                    gbuf+=512;
                    continue;
                }
                
                SOpticalFileData&	fileData = *newData;
				int					i,j;
				fixed				msb, lsb, rate;
				uint32				wordLen, blockLen;

				// Init
				memset(&fileData, 0, sizeof(fileData));
				
				// Name
				for (i=0; i<4; i++)														// get file name
				{
					fileData.Name[i*2    ] = gbuf[e__name+i] & 0xFF;
					fileData.Name[i*2 + 1] = gbuf[e__name+i] >> 8;
					fileData.Name[i*2 + 2] = 0;
				}

                if (fileData.Name[0] == '.')
                    fileData.Name[0] = '*';

				// Type, DirEntry
				fileData.FileType = gbuf[e_file_type];
				fileData.DirEntry = firstEntry;

				// Sample Rate
				rate = gbuf[e_header + sf_sample_rate];
				
				if (rate == 0)						/* no sample rate - must calculate it from period index */
				{
					rate = gbuf[e_header + sf_period_index];
					
					if (rate == 0)
						rate = 500;
					else
						rate = (fixed) ((((long) 300000) + (((long) rate) >> 1)) / ((long) rate));
				}
				
				fileData.SampleRate = rate;

				// Flags
	     		fileData.Flags = gbuf[e_header + sf_file_data_type] | shl(gbuf[e_header + sf_stereo],10);

				// SecStart
				lsb = gbuf[e_ls_secstart];
				msb = gbuf[e_ms_secstart];

				fileData.SecStart = (((uint32) (uint16) msb ) << 16) | ((uint32) (uint16) lsb );

				// SecLen
				lsb = gbuf[e_ls_seclen];
				msb = gbuf[e_ms_seclen];

				blockLen = (((uint32) (uint16) msb ) << 16) | ((uint32) (uint16) lsb );
				
				wordLen  = (uint32) (uint16) gbuf[e_ls_wordlen];
				
				if (blockLen >= 256)										// Compute word length completely if big file
				{
					if (wordLen & 255)
						fileData.ByteLength = (((long long) blockLen) << 8) - (256 - (((long long) wordLen) & 0xFF));
					else
						fileData.ByteLength = (((long long) blockLen) << 8);
						
					fileData.ByteLength <<= 1;								// Byte length!
				}
				
				else
					fileData.ByteLength = ((long long) wordLen) << 1;
				
				// Parse categories
				LCStr255	catStrings[8];
				int  		numCaps = 0;
				
				AbleOptLib_ParseCaptionsFromSFHeader(&gbuf[e_header], catStrings, numCaps);
				
				// Enter categories into category list for entry and rename records only
				if ((gbuf[e__type] == e_dir_entry) || (gbuf[e__type] == e_rename_entry))
				{
					SyncUint32 sortedWhere;
					
					// Look up where this file will go in the sorted list for speed reasons
					if (itsOpticalData.mSortedFileList.GetCount() == 0)
						sortedWhere = 1;
					else
						sortedWhere = itsOpticalData.mSortedFileList.FetchInsertIndexOfKey(&fileData);			
				
					// Parse all category strings.  E.G. if category is Basses:Acoustic:ABS40, create 
					// categories for BASSES,  BASSES:ACOUSTIC, and BASSES:ACOUSTIC:ABS40.
					// Begin by entering higher level categories (e.g. those that only contain subcategories)
					for (i=0; i<numCaps; i++)
					{
                        SOpticalCategory* newCat = new SOpticalCategory;
                        
                        if (!newCat)
                            continue;
						
                        SOpticalCategory&	catData = *newCat;
                        SyncUint32    		where   = 0;
                        SyncUint16  		catCode = 0;

                        memset(&catData, 0, sizeof(catData));
						strcpy(catData.Name, (char *) catStrings[i]);					// copy entire caption string into place
						
                        #if 0
                            catData.first_file = 0;
                            catData.last_file  = 0;
                        
                            // This code created category entries for all the intermediate files.
                            // No longer used. Sync3 indexes the category fields in real time.
                            char* theCap  = &catData.Name[0];
                        
                            while (*theCap && *theCap != ':')
                                theCap++;
                                
                            while (*theCap == ':')
                            {
                                *theCap = 0;												// terminate higher category (e.g turn BASSES:ACOUSTIC into BASSES
                                
                                where = itsOpticalData.mSortedCategoryList.FetchIndexOfKey(&catData);

                                if (where == 0)
                                {
                                    catCode = itsOpticalData.mCategoryList.AddItem(&catData);
                                    
                                    SyncUint16 &catCodeItem = *new SyncUint16;
                                    
                                    if (&catCodeItem) {
                                        catCodeItem = catCode;
                                        itsOpticalData.mSortedCategoryList.InsertItem(&catCodeItem);
                                    }
                                }
                                
                                *theCap++ = ':';											// restore colong
                                
                                while (*theCap && *theCap != ':')
                                    theCap++;
                            }
                        #endif  
                        
						// See if this category is in the file
						where = itsOpticalData.mSortedCategoryList.FetchIndexOfKey(&catData);

						// Adding this category for this first time.  Relatively easy
						if (where == 0)
						{
							//catData.first_file = firstEntry+1;                                          // this is first file in this category
							//catData.last_file  = firstEntry+1;
							
							catCode = itsOpticalData.mCategoryList.AddItem(&catData);                   // add category to category list
                            
                            SyncUint16* catCodeItem = new SyncUint16;
                            
                            if (catCodeItem) {
                                *catCodeItem = catCode;
                                itsOpticalData.mSortedCategoryList.InsertItem(catCodeItem);
                            }
						}
						
                        // Else retrieve the pointer to the unsorted list of categories
                        // since that's what get's stored in the file record
                        else
                            catCode = * (SyncUint16 *) itsOpticalData.mSortedCategoryList[where];
                        
						// Else updating a category.  Do some analysis to find the range of files that
						// might be included in this category.  This speeds up the category display
						// since, for example, ABS40 contains files that begin with ABS...
                        #if 0
                            else if (itsOpticalData.mSortedCategoryList.ValidIndex(where)) {
                                SyncUint32 tempIndex;
                                
                                catCode = * (SyncUint16 *) itsOpticalData.mSortedCategoryList[where];
                                
                                if (itsOpticalData.mCategoryList.ValidIndex(tempIndex = catCode))
                                {
                                    SOpticalCategory&	catData = * (SOpticalCategory*) itsOpticalData.mCategoryList[catCode];
                                    
                                    // Detect adding first file to a category
                                    if (catData.first_file == 0 && catData.last_file == 0)
                                    {
                                        catData.first_file = firstEntry+1;								// this is first file in this category
                                        catData.last_file  = firstEntry+1;
                                    }
                                    
                                    // Detect adding a second file to a category
                                    else if ((itsOpticalData.mFileList.ValidIndex(tempIndex = catData.first_file))
                                    &&       (itsOpticalData.mFileList.ValidIndex(tempIndex = catData.last_file )))
                                    {
                                        SyncUint32	sortedFirst = itsOpticalData.mSortedFileList.FetchIndexOf(&catData.first_file);
                                        SyncUint32	sortedLast  = itsOpticalData.mSortedFileList.FetchIndexOf(&catData.last_file );

                                        // If catData.first_file has been deleted, point to the start of the file list
                                        if (sortedFirst == 0) sortedFirst = 1;
                                        if (sortedLast  == 0) sortedLast  = itsOpticalData.mSortedFileList.GetCount();
                                        
                                        // If new entry is lower (alphabetically) than prior, extend range
                                        // of category accordingly.  Also, handle rename records intelligently.
                                        if (sortedWhere <= sortedFirst)
                                            catData.first_file = firstEntry+1;

                                        // If renaming what had been first entry, point to prior one
                                        else if ((gbuf[e__type] == e_rename_entry)
                                        &&       (catData.first_file == (unsigned short) (gbuf[e_prev_entry] + 1))
                                        &&       (sortedFirst > 1))
                                            catData.first_file = * (SyncUint16 *) itsOpticalData.mSortedFileList[sortedFirst-1];
                                        
                                        if (sortedWhere > sortedLast)
                                            catData.last_file  = firstEntry+1;

                                        // If renaming what had been last entry, point to next one
                                        else if ((gbuf[e__type] == e_rename_entry)
                                        &&       (catData.last_file == (unsigned short) (gbuf[e_prev_entry] + 1))
                                        &&       (sortedLast < itsOpticalData.mSortedFileList.GetCount()))
                                            catData.last_file = * (SyncUint16 *) itsOpticalData.mSortedFileList[sortedLast+1];
                                    }
                                }
                            }
                        #endif
                        
						fileData.Categories[i] = catCode;
					}
				}
				
				// Caption
				j = gbuf[e_header + sf_id_field_bytes]; 				/* get length of caption */
				
				if (j<0) j = 0;
				if (j>(l_caption_max<<1)) j = l_caption_max<<1;

				for (i=0; i<j; i++)
					fileData.Caption[i] = byte(&gbuf[e_header + sf_id_field_bytes], i);

                // Store the whole sound file header for copying to disk.
                for (i=0; i<e_header_max; i++)
                    fileData.SFHeader[i] = gbuf[e_header + i];
               
				// Add this file record to the file list (always, so it matches disk)
				SyncUint32	   whereAdded = itsOpticalData.mFileList.AddItem(&fileData);
				SyncUint16     whereIs, whereWas;
				SOpticalFileData oldFileData;
					
				// Check for phase error
				if (whereAdded != firstEntry+1)
				{
					if (!fatalUpdateErrorOccured)
					{
						printf("Phase error in AbleOptLib_ConstructOpticalIndex\n");
					}
					
					abortMe                 = true;
					fatalUpdateErrorOccured = true;
					break;
				}
					
				// If new record, put in sorted list (Note: InsertItemsAt does its own index sort)
				if (gbuf[e__type] == e_dir_entry && fileData.Name[0] != 0)
				{
					whereIs = itsOpticalData.mSortedFileList.FetchIndexOfKey(&fileData);
					
					// See if name already in data base
					if (whereIs)
					{
						if (0)
						{
							printf("OpticalIndex: Duplicate index entry for %s\n", fileData.Name);
						}
						
                        delete (SyncUint16*) itsOpticalData.mSortedFileList[whereIs];
                        
                        itsOpticalData.mSortedFileList.RemoveItemAt(whereIs);
					}

                    SyncUint16* whereAddedItem = new SyncUint16;
                    
                    if (whereAddedItem) {
                        *whereAddedItem = whereAdded;
                        itsOpticalData.mSortedFileList.InsertItem(whereAddedItem);
                    }
				}
					
				// If rename entry, null out prior entry
				else if (gbuf[e__type] == e_rename_entry && fileData.Name[0] != 0)
				{
					whereWas = gbuf[e_prev_entry] + 1;							// Get previous entry (1-based)
					
					for (i=0; i<4; i++)											// get old file name
					{
						oldFileData.Name[i*2    ] = gbuf[e_oldname+i] & 0xFF;
						oldFileData.Name[i*2 + 1] = gbuf[e_oldname+i] >> 8;
						oldFileData.Name[i*2 + 2] = 0;
					}

                    if (oldFileData.Name[0] == '.')
                        oldFileData.Name[0] = '*';

                    // Make sure old name was in data base and that it matches
					whereIs = itsOpticalData.mSortedFileList.FetchIndexOfKey(&oldFileData);
					
					if (whereIs && (* (SyncUint16 *) itsOpticalData.mSortedFileList[whereIs]) != whereWas)
					{
						if (1)
						{
							printf("OpticalIndex: Mismatched rename entry for %s\n", oldFileData.Name);
						}
						
						whereWas = * (SyncUint16 *) itsOpticalData.mSortedFileList[whereIs];
					}
					
                    if (whereIs) {
                        delete (SyncUint16*) itsOpticalData.mSortedFileList[whereIs];
                    
                        itsOpticalData.mSortedFileList.RemoveItemAt(whereIs);
                    }
                    
					// Make sure new name is not already in there
					whereIs = itsOpticalData.mSortedFileList.FetchIndexOfKey(&fileData);
					
					// if new name is in data base, had best be at prior entry and this is an update record
					if (whereIs && (* (SyncUint16 *) itsOpticalData.mSortedFileList[whereIs]) != whereWas)
					{
						if (1)
						{
                            printf("OpticalIndex: Duplicate rename entry for %s\n", fileData.Name);
						}
					}
					
					// Remove pointer to renamed file from the sorted list
                    if (whereIs) {
                        delete (SyncUint16*) itsOpticalData.mSortedFileList[whereIs];
                        
                        itsOpticalData.mSortedFileList.RemoveItemAt(whereIs);
                    }
                    
					// Add rename entry to sorted list
                    SyncUint16* whereAddedItem = new SyncUint16;
                    
                    if (whereAddedItem) {
                        *whereAddedItem = whereAdded;
                        itsOpticalData.mSortedFileList.InsertItem(whereAddedItem);
                    }
				}
					
				// If delete entry, null out prior entry
				else if (gbuf[e__type] == e_delete_entry)
				{
					whereWas = gbuf[e_prev_entry] + 1;
				
					for (i=0; i<4; i++)											// get old file name
					{
						oldFileData.Name[i*2    ] = gbuf[e_oldname+i] & 0xFF;
						oldFileData.Name[i*2 + 1] = gbuf[e_oldname+i] >> 8;
						oldFileData.Name[i*2 + 2] = 0;
					}

                    if (oldFileData.Name[0] == '.')
                        oldFileData.Name[0] = '*';
                    
					// Make sure old name was in data base and that it matches
					whereIs = itsOpticalData.mSortedFileList.FetchIndexOfKey(&oldFileData);
					
					if (whereIs && (* (SyncUint16 *)itsOpticalData.mSortedFileList[whereIs]) != whereWas)
					{
						if (1)
						{
							printf("OpticalIndex: Mismatched delete entry for %s\n", oldFileData.Name);
						}
					}

					// Remove pointer to deleted file from the sorted list
                    if (whereIs) {
                        delete (SyncUint16*) itsOpticalData.mSortedFileList[whereIs];
                        
                        itsOpticalData.mSortedFileList.RemoveItemAt(whereIs);
                    }
				}
				
				chunkEntries--;
				firstEntry++;
				gbuf+=512;
			}

			// If processed all entries, mark optical data as up to date
			if (!fatalUpdateErrorOccured && firstEntry == lastEntry)
				itsOpticalData.SetIsUpToDate(true);
		}
	}
	
	catch (long theError)
	{
		if (theError == memFullErr)
			printf("Ran out of memory in AbleOptLib_ConstructOpticalIndex\n");
		else
			printf("Failed AbleOptLib_ConstructOpticalIndex (%d)\n", (int) theError);

		abortMe                 = true;
		fatalUpdateErrorOccured = true;
	}

	catch (...)
	{
		printf("Failed AbleOptLib_ConstructOpticalIndex\n");
		
		abortMe                 = true;
		fatalUpdateErrorOccured = true;
	}

	if (abortMe)
		fatalUpdateErrorOccured = true;
		
	if (fatalUpdateErrorOccured)
        AbleOptLib_ReleaseOpticalIndex(itsOpticalData);

    // Print results. Careful - we are on a background thread here...
    #if 0
        NSLog(@"mFileList");
        for (int i=1; i<=itsOpticalData.mFileList.GetCount(); i++)
            NSLog(@"%5d %s", i, ((SOpticalFileData*) itsOpticalData.mFileList[i])->Name);
        
        NSLog(@"mSortedFileList");
        for (int i=1; i<=itsOpticalData.mSortedFileList.GetCount(); i++) {
            SyncUint16        which = * (SyncUint16*      ) itsOpticalData.mSortedFileList[i    ];
            SOpticalFileData& data  = * (SOpticalFileData*) itsOpticalData.mFileList      [which];
            NSLog(@"%5d %s", i, ((SOpticalFileData*) itsOpticalData.mFileList[which])->Name);
            for (int i=0; i<8; i++)
                if (data.Categories[i])
                    NSLog(@"    %5d %s", i, ((SOpticalCategory*) itsOpticalData.mCategoryList[data.Categories[i]])->Name);
        }

        NSLog(@"mCategoryList");
        for (int i=1; i<=itsOpticalData.mCategoryList.GetCount(); i++)
            NSLog(@"%5d %s", i, ((SOpticalCategory*) itsOpticalData.mCategoryList[i])->Name);
        
        NSLog(@"mSortedCategoryList");
        for (int i=1; i<=itsOpticalData.mSortedCategoryList.GetCount(); i++) {
            SyncUint16 which = (* (SyncUint16*) itsOpticalData.mSortedCategoryList[i]);
            NSLog(@"%5d %s", i, ((SOpticalCategory*) itsOpticalData.mCategoryList[which])->Name);
        }
    #endif
    
	entryBuf = NULL;

	return (noErr);
}

void    AbleOptLib_ReleaseOpticalIndex(CSharedOpticalDataBase& itsOpticalData)
{
    while (itsOpticalData.mFileList.GetCount())
        delete (SOpticalFileData*) itsOpticalData.mFileList.RemoveLastItem();
    
    while (itsOpticalData.mCategoryList.GetCount())
        delete (SOpticalCategory*) itsOpticalData.mCategoryList.RemoveLastItem();
    
    while (itsOpticalData.mSortedFileList.GetCount())
        delete (SyncUint16*) itsOpticalData.mSortedFileList.RemoveLastItem();
    
    while (itsOpticalData.mSortedCategoryList.GetCount())
        delete (SyncUint16*) itsOpticalData.mSortedCategoryList.RemoveLastItem();
}

int     AbleOptLib_ParseSoundfileHeader(uint32 num_words,
                                        struct SynclSFHeader& header,
                                        class  LCStr255 theCaps[8],
                                        int&   numCaps,
                                        struct AudioDataDescriptor& descriptor,
                                        char   Caption[256])
{
    AbleOptLib_ParseCaptionsFromSFHeader((fixed*)&header, theCaps, numCaps);
    
    // Handle old SF headers as per ps.load. But this should have been done when the
    // file was imported. We double-check here in case we are dealing with a very old
    // InterChange 1.0 imported file.
    if (header.compatibility < 0)
        header.compatibility = 0;
    
    if (header.stereo != 0 && header.stereo != 1)
        return kUnknownType;
    
    if (header.sample_rate == 0)
    {
        if (header.compatibility < 1)
            header.period_index = 600;
        
        else if (header.period_index == 0)
            header.period_index = 600;
        
        else if (header.period_index < 300)
            header.period_index = 300;
    }
    
    // Import all valid data
    if (1)
    {
        descriptor.byte_len_in_file = (header.valid_data.sector << 9) + (header.valid_data.word_offset << 1);
        
        if (descriptor.byte_len_in_file > ((num_words - 3*256) << 1))
            descriptor.byte_len_in_file = ((num_words - 3*256) << 1);
        
        if (0)
        {
            // Could also extract mark start to mark end
        }
    }
    
    // Else could just use length of file; but may contain bogus data at end
    else
        descriptor.byte_len_in_file  = (num_words - 3*256) << 1;
    
    descriptor.start_pos_in_file = sizeof(header);
    
    descriptor.bits_per_sample   = 16;
    descriptor.samples_per_frame = header.stereo+1;
    descriptor.frames_per_file   = descriptor.byte_len_in_file / descriptor.samples_per_frame / 2;
    
    if (header.sample_rate == 0 && header.period_index != 0)
        descriptor.frames_per_second = 30000000.0 / ((double) header.period_index);
    else
        descriptor.frames_per_second = ((double) header.sample_rate)*100.0;
    
    int i,j;
    
    j = header.id_field_bytes; 				// Able string format - get its length in bytes
				
    if (j<0) j = 0;
    if (j>(l_caption_max<<1)) j = l_caption_max<<1;
    
    for (i=0; i<j; i++)
        Caption[i] = byte(&header.id_field_bytes, i); // Note we point to the word before the characters; that's where the able string starts
    
    // Headers seem to have lots of trailing spaces
    while (j>0 && (Caption[j-1] == ' '))
        Caption[--j] = 0;
    
    return noErr;
}

int     AbleOptLib_FetchSoundfileHeader(class  CSynclavierFileReference& fileRef,
                                        struct SynclSFHeader& header,
                                        class  LCStr255 theCaps[8],
                                        int&   numCaps,
                                        struct AudioDataDescriptor& descriptor,
                                        char   Caption[256])
{
    uint32   num_words = (uint32) (fileRef.Size() >> 1);
    SInt32   count     = sizeof(header);
    OSStatus err;
    
    // Read the header from file
    if ((err = fileRef.XPLRead(&count, &header)) != noErr || count != sizeof(header))
        return err;
    
    // The byte swizzling performed in ReadAbleDisk messed up the sound file header on Intel macs
    #if __LITTLE_ENDIAN__
        // Sync3 wrote some files to disk with these words in the wrong order. Fix.
        if ((header.file_handle[0] == 0) || (((header.valid_data.sector & 0x0000FFFF) == 0) && ((header.valid_data.sector & 0xFFFF0000) != 0)))
        {
            Byte it;
            
            header.valid_data.sector   = ((header.valid_data.sector   & 0x0000FFFF) << 16) | ((header.valid_data.sector   & 0xFFFF0000) >> 16);
            header.total_data.sector   = ((header.total_data.sector   & 0x0000FFFF) << 16) | ((header.total_data.sector   & 0xFFFF0000) >> 16);
            header.total_length.sector = ((header.total_length.sector & 0x0000FFFF) << 16) | ((header.total_length.sector & 0xFFFF0000) >> 16);
            header.loop_length.sector  = ((header.loop_length.sector  & 0x0000FFFF) << 16) | ((header.loop_length.sector  & 0xFFFF0000) >> 16);
            
            it = header.smpte_seconds; header.smpte_seconds = header.smpte_frames; header.smpte_frames  = it;
            it = header.smpte_hours;   header.smpte_hours   = header.smpte_minutes;header.smpte_minutes = it;
        }
    #endif

    // Parse the header into various useful items
    return AbleOptLib_ParseSoundfileHeader(num_words, header, theCaps, numCaps, descriptor, Caption);
}
