// =================================================================================
//	AbleOptLib
// =================================================================================

// Library for accessing Able Optical Format media

// 2/8/00 C. Jones

// Std C
#include <StdIO.h>
#include <String.h>

#include "LThread.h"
#include "CInterChangeApp.h"
#include "CSharedOpticalDataBaseCP.h"
#include "AbleDiskLib.h"
#include "AbleOptLib.h"

// Local includes
#include "XPL.h"
#include "XPLRuntime.h"
#include "Syslits.h"
#include "Samplits.h"
#include "Devutil.h"
#include "SCSILib.h"
#include "SCSIlits.h"
#include "OPTLits.h"


// =================================================================================
//		¥ Global Variables
// =================================================================================

fixed	s_sensekey;														// likely will be moved from here...


// =================================================================================
//		¥ Additional Devides
// =================================================================================

#define	quantum_controller	0											// non ld1200 (e.g. M/O, hard drive, image file)
#define	ld1200_controller	1											// ld1200
#define	magicnumber			((fixed) 0x8000)							// header revision number
#define	formatpattern		((fixed) 0x6363)							// format pattern written to magneto media
#define	defectmode			0											// auto-rewrite mode


// =================================================================================
//		¥ Static variables for caching
// =================================================================================

static	boolean	dev10set;
static	boolean	dev11set;
static	fixed	dev10ctrlr;
static	fixed	dev11ctrlr;
static	fixed	dev10blksiz;
static	fixed	dev11blksiz;
static	long	dev10atncnt;
static	long	dev11atncnt;

static  LFastMutexSemaphore gOpticalMutex;

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

static	fixed	o_watch_shown;


// =================================================================================
//		¥ Handy class for managing o_cur_bufptr
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
//		¥ AbleOptLib_finddevadr, AbleOptLib_lookup_controller_type, AbleOptLib_lookup_controller_block_size
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
	scsi_device* itsDevice = access_scsi_device(level);

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
	fixed		 devptr;
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

	devptr = find_device (level);										// get device table pointer handy in case is 10 or 11; may not be one if disk image file
	
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
		if (devptr)
		{
			set_able_core(devptr + s_seccyl, 144);						// provide simulated 1 gig quantum media
			set_able_core(devptr + s_totcyl, ((fixed) 6815));
		}
		
		return (quantum_controller);
	}
	
	devtyp = itsDevice->fStandardInquiryData.ptype;
	
	// Direct access devices and image files
	if (devtyp == 0)													// direct access - find size
	{
		if (devptr)
			set_able_core(devptr + s_devtyp, able_core(devptr + s_devtyp) | shl(1,10));	// allow formcopy to select this for format even if no media present
		
		if ((itsDevice->fNumBlocks == 0)								// if could not get block size
		||  (itsDevice->fBlockSize == 0)
		||  (itsDevice->fTotCyl    == 0)
		||  (itsDevice->fTotSec    == 0))
		{
			if (devptr)
			{
				set_able_core(devptr + s_seccyl, 144);					// provide simulated 1 gig quantum media
				set_able_core(devptr + s_totcyl, ((fixed) 6815));
			}
			
			return (quantum_controller);
		}
					
		controller = quantum_controller;								// is quantum; size is set
	}

	// LD1200 WORMS
	else if (devtyp == 4)
	{
		if (devptr)
			set_able_core(devptr + s_devtyp, able_core(devptr + s_devtyp) & (~(shl(1,10))));
		
		controller = ld1200_controller;
	}
	
	// Unknown device type code (CDROM perhaps)
	else 
	{
		set_able_core(devptr + s_seccyl, 144);							// provide simulated 1 gig quantum media
		set_able_core(devptr + s_totcyl, ((fixed) 6815));
		
		return (quantum_controller);
	}
	
	// Update config size (upper and lower memory)
	if (devptr)
		update_scsi_device_size(level);
	
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
//		¥ AbleOptLib_map_error_code
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
//		¥ AbleOptLib_check_buf
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
//		¥ AbleOptLib_transferoptical
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
//		¥ AbleOptLib_modeselectoptical
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
			buf[10] = 0x04;                                             // 1024-byte block size
			buf[11] = 0x00;
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
			if (!o_watch_shown && LThread::InMainThread())
			{
				UCursor::SetWatch();
				o_watch_shown = true;
			}

			LThread::Yield();
			
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
//		¥ AbleOptLib_readheader
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
//		¥ AbleOptLib_readheader
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
//		¥ AbleOptLib_sectoentry, AbleOptLib_entrytosec
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
//		¥ AbleOptLib_readentry, AbleOptLib_writeentry
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
//		¥ AbleOptLib_readentries, AbleOptLib_writeentries
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
//		¥ AbleOptLib_findentrycount
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
//		¥ AbleOptLib_searchempty1024kblocks
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
//		¥ AbleOptLib_searchemptyblocks
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
//		¥ AbleOptLib_get_statistics
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
	
	_write_5(ls_free);													// calculate the no. bad sectors
	_write_4(ms_free);
	_write_7(1000);														// assume .1% of these free sectors will be bad
	i = _read_5;
	
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
//		¥ AbleOptLib_init_optical_controller
// =================================================================================

// Marginally useful routine to pre-cache controller type and
// block size information for levels 10 and 11
static	void	AbleOptLib_init_optical_controller()					// set optical controller type based on system configuration
{
	static	fixed	i;
	
	dev10set = false;													// force re-polling in all cases if we are called
	dev11set = false;
	
	AbleOptLib_lookup_controller_type(10);								// set both types if configured & running
	AbleOptLib_lookup_controller_type(11);
}
	
	
// =================================================================================
//		¥ AbleOptLib_InitializeDeviceSetup()
// =================================================================================

void	AbleOptLib_InitializeDeviceSetup()
{
	dev10set = false;
	dev11set = false;
}


// =================================================================================
//		¥ AbleOptLib_ReadOpticalDirectory()
// =================================================================================

int	AbleOptLib_ReadOpticalDirectory(InterChangeItemDirectory &itsData, const char *the_tree_name)
{
	stCatSemaphore	sem;							// Protect catalog      global variables
	StFastMutex		mutex(gOpticalMutex);			// Protect optical disk global variables
	char   			er_mess[MESSAGE_BUF_SIZE];
	fixed			entry_buffer[512];
	stOptBuf		optBuf(&entry_buffer[0]);
	fixed			stat_buf[stat_rec_length] = {0};// statistics array
	
	if ((itsData.root.OpticalItem.file_system      != InterChange_AbleOpticalFileSystemCode)
	||  (itsData.root.OpticalItem.file_type        != InterChange_AbleOpticalDirectory     )
	||  (itsData.root.OpticalItem.file_device_code == 0                                    ))
	{
		printf("InterChangeª: Could not get information on optical directory '%s'\n", the_tree_name);
		return (-1);
	}

	// Discard any cached directory information
	o_entrycount  = -1;													// optical disk not mounted yet
	o_watch_shown = false;												// no watch cursor yet
	
	// Present watch cursor if main thread
	if (!o_watch_shown && LThread::InMainThread())
	{
		UCursor::SetWatch();
		o_watch_shown = true;
	}

	// Read basic header
	if ((!AbleOptLib_readheader    (itsData.root.OpticalItem.file_device_code          ))
	||  (!AbleOptLib_get_statistics(itsData.root.OpticalItem.file_device_code, stat_buf)))
	{
		if (o_watch_shown)
		{
			UCursor::SetArrow();
			o_watch_shown = false;
		}

		if (c_status)
		{
			get_cat_code_message(c_status, er_mess);
			printf("InterChangeª: Could not read directory for '%s'\n   Catalog error Report:\n", the_tree_name);
			printf("   %s\n", er_mess);
			return (c_status);
		}
		
		if (s_sensekey)
		{
			get_sense_code_message(s_sensekey, er_mess);
			printf("InterChangeª: Could not read directory for '%s'\n   SCSI error Report:\n", the_tree_name);
			printf("   %s\n", er_mess);
			return (256+s_sensekey);
		}
	}

	// Publish information about the directory
	InterChangeOpticalData&	optData = itsData.optical_data;
	
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
	
	if (o_watch_shown)
	{
		UCursor::SetArrow();
		o_watch_shown = false;
	}

	return (noErr);
}


// =================================================================================
//		¥ AbleOptLib_LocateCategory()
// =================================================================================

int	AbleOptLib_LocateCategory(InterChangeItemUnion& topUnion, LCStr255& topName, LCStr255& treeName, InterChangeItemUnion& outUnion, interchange_settings& theSettings)
{
	int						 status;
	CSharedOpticalDataBase*  opticalData = NULL;
	SOpticalCategory		 catData;
					
	// Init
	memset(&catData, 0, sizeof(catData));
	
	// Make sure is optical
	if (topUnion.Item.file_system != InterChange_AbleOpticalFileSystemCode)
		return (-1);
		
	// Find by device code
	if (topUnion.OpticalItem.file_device_code)
		opticalData = CSharedOpticalDataBase::FindOpticalDataByLegacyCode(topUnion.OpticalItem.file_device_code);
		
	// Find by file spec
	if (!opticalData)
	{
		FSSpec fileSpec;
		
		AbleDiskLib_FetchReleventFSSpec(topUnion, fileSpec, theSettings);

		if (fileSpec.name[0])
			opticalData = CSharedOpticalDataBase::FindOpticalDataByFSSPec(fileSpec);
	}
	
	if (!opticalData)
	{
		sprintf(AbleDiskLib_RecentErrorMessage, "Could not find shared optical data");
		printf("InterChangeª: Could not read directory for '%s'\n   %s\n", (char *) treeName, AbleDiskLib_RecentErrorMessage);
		return (-1);
	}
	
	// Make sure no one is writing the index
	status = opticalData->mSemaphore.Wait(semaphore_NoWait);					// access optical data structures without waiting

	if (status == errSemaphoreTimedOut)											// if data sturctures not available
	{
		opticalData->ReleaseOpticalData();

		sprintf(AbleDiskLib_RecentErrorMessage, "Shared ndex is busy being updated");
		printf("InterChangeª: Could not read directory for '%s'\n   %s\n", (char *) treeName, AbleDiskLib_RecentErrorMessage);
		return (-1);
	}
	
	opticalData->mReaders++;
	opticalData->mSemaphore.Signal();
	
	// Find category
	LCStr255	noColonName(topName);
	LCStr255	catName;
	
	if (noColonName.BeginsWith(':'))
		noColonName.Remove(1,1);

	strcpy(catData.Name, (char *) noColonName);
	
	// Find category
	ArrayIndexT    where = opticalData->mSortedCategoryList.FetchIndexOfKey(&catData);
	
	// Look up category number
	if (where)
		where = opticalData->mSortedCategoryList[where];
	
	// If exact match
	if (where)
	{
		// Extract entity name
		unsigned char priorColon = 0, nextColon;
		
		nextColon = noColonName.Find(':', priorColon+1);						// find next :
		
		while (nextColon)
		{
			priorColon = nextColon;
			nextColon = noColonName.Find(':', priorColon+1);					// find next :
		}

		if (priorColon == 0)
			catName = noColonName;												// no colon: must be entire name
		else
			catName.Assign((void *) (&noColonName[priorColon+1]), noColonName.Length() - priorColon);
		
		// Limit to what will fit
		if (catName.Length() > 32)
			catName[0] = 32;
		
		// Construct output union
		outUnion = topUnion;
		outUnion.OpticalItem.file_size_bytes  = 0;								// Don't know size
		outUnion.OpticalItem.file_block_start = where;							// category code

		strcpy(outUnion.OpticalItem.file_name, (char *) catName);				// store category name
		
		status = noErr;
	}
	
	// Else is missing
	else
	{
		sprintf(AbleDiskLib_RecentErrorMessage, "Intermediate category is missing");
		printf("InterChangeª: Could not read directory for '%s'\n   %s\n", (char *) treeName, AbleDiskLib_RecentErrorMessage);
		status = (-1);
	}
	
	// Done		
	opticalData->mReaders--;
	opticalData->ReleaseOpticalData();
	
	return (status);
}	


// =================================================================================
//		¥ AbleOptLib_UpdateSFHeaderFromEntry()
// =================================================================================

// Updates the sound file header of a sound file from an optical disk directory entry.
// Called after copying from an optical device
int		AbleOptLib_UpdateSFHeaderFromEntry  (InterChangeItemUnion& theSource, scsi_device* source_device, scsi_device* dest_device, unsigned long dest_block,
											 interchange_settings& theSettings)
{
	int						 status;
	CSharedOpticalDataBase*  opticalData = NULL;
	ArrayIndexT				 sortedWhere = 0;
	ArrayIndexT				 listWhere   = 0;
	unsigned short			 entryNum    = 0;
	SOpticalFileData		 fileData;
	uint16					 gBuf[512];
	uint16					 dBuf[512];
	int i;

	// Find by device code
	if (theSource.OpticalItem.file_device_code)
		opticalData = CSharedOpticalDataBase::FindOpticalDataByLegacyCode(theSource.OpticalItem.file_device_code);
		
	// Find by file spec
	if (!opticalData)
	{
		FSSpec fileSpec;
		
		AbleDiskLib_FetchReleventFSSpec(theSource, fileSpec, theSettings);

		if (fileSpec.name[0])
			opticalData = CSharedOpticalDataBase::FindOpticalDataByFSSPec(fileSpec);
	}
	
	if (!opticalData)
	{
		SysBeep(10);
		printf("InterChangeª: Missing optical index (no device code)\n");
		return (-1);
	}

	// Set up SOpticalFileData to find the file
	memset(&fileData, 0, sizeof(fileData));
	
	if (strlen(theSource.OpticalItem.file_name) > 8)
	{
		SysBeep(10);
		printf("InterChangeª: Missing optical index (bad file name)\n");
		return (-1);
	}
	
	strcpy(fileData.Name, theSource.OpticalItem.file_name);

	sortedWhere = opticalData->mSortedFileList.FetchIndexOfKey(&fileData);

	// Look up file number
	if (sortedWhere)
		listWhere = opticalData->mSortedFileList[sortedWhere];

	if (!opticalData->mFileList.ValidIndex(listWhere))
	{
		SysBeep(10);
		printf("InterChangeª: Missing sound file header (invalid index)\n");
		return (-1);
	}
	
	entryNum = opticalData->mFileList[listWhere].DirEntry;
	
	if (entryNum+1 != listWhere)
	{
		SysBeep(10);
		printf("InterChangeª: Missing sound file header (phase error)\n");
		return (-1);
	}
	
	status = AbleDiskLib_ReadAbleDisk(source_device, gBuf, opticalData->mOpticalData.dir_start + (entryNum<<1), 2);

	if (status)
	{
		SysBeep(10);
		printf("InterChangeª: Could not read sound file header\n");
		return (-1);
	}

	status = AbleDiskLib_ReadAbleDisk(dest_device, dBuf, dest_block, 2);

	if (status)
	{
		SysBeep(10);
		printf("InterChangeª: Could not update sound file header\n");
		return (-1);
	}
	
	for (i=0; i<256; i++)
		dBuf[i] = gBuf[256+i];
		
	status = AbleDiskLib_WriteAbleDisk(dest_device, dBuf, dest_block, 2);

	if (status)
	{
		SysBeep(10);
		printf("InterChangeª: Could not update sound file header\n");
		return (-1);
	}
	
	return (noErr);
}


// =================================================================================
//		¥ AbleOptLib_CreateVolumeHeader()
// =================================================================================

int	AbleOptLib_CreateVolumeHeader(scsi_device *the_device, unsigned long block_len, char* itsName)
{
	long   header_start;
	long   header_size;													// size of header
	long   header_gap;													// size of header gap
	long   dir_start;
	long   dir_size;														// size of directory area
	long   dir_gap;														// size of directory gap
	long   data_start;
	long   data_size;
	long   data_gap;														// size of data gap
	ufixed pbuf[h_rec_length];
	int    i;
	char   its_zname[10] = {0,0,0,0,0,0,0,0,0,0};
	
	DateTimeRec	timeRec;

	for (i = 0; i < h_rec_length; i++) {								// clear the buffer
		pbuf[i] = 0;
	}
		
	if (strlen(itsName) > 8)											// Disallowed for now
		return (-1);

	strcpy(its_zname, itsName);											// stuff name into zero-filled array
	
	for (i=0; i<4; i++)													// store file name
	{
		pbuf[h_name+i] = (((uint16) (uint8) its_zname[i*2 + 0])     )
					   | (((uint16) (uint8) its_zname[i*2 + 1]) << 8);
	}

	// Compute info using 1024-byte sectors
	block_len >>= 1;
	
	// Allocate Header Area
	header_start = 0;
	header_size  = 1;													// header takes up 1 sector
	header_gap   = 100;													// allow room for more headers
	
	// Allocate directory area											// 1%
	dir_start = header_start + header_size + header_gap;
	
	if (block_len > 6553400)
		dir_size = 65534;
	else
		dir_size = (block_len / 100);
		
	dir_gap = dir_size / 500;
	
	if (dir_gap < 10) dir_gap = 10;
	
	// Allocate data area
	data_start = dir_start + dir_size + dir_gap;
	data_size  = block_len - data_start;
	data_gap   = data_size / 1000;
	
	if (data_gap < 10) data_gap = 10;
	
	data_size -= data_gap;
	
	/* Set up buffer with header information */
	pbuf[h_magic] = magicnumber;

	GetTime(&timeRec);

	// Date:
	// 	Lower 5 bits day
	// 	Mid   4 bits month
	// 	Upper 7 bits year

	// Time:
	//	Lower 5 bits seconds/2
	//  Mid   6 bits minutes
	//  Mid   4 bits hours
	//  Upper 1 bit  am/pm

	pbuf[h_date] = timeRec.day + ((timeRec.month-1) << 5) + (((timeRec.year - 1900) % 100) << 9);
	pbuf[h_time] = (timeRec.second >> 1) + (timeRec.minute << 5) + ((timeRec.hour % 12) << 11) + ((timeRec.hour/12) << 15);

	pbuf[h_ls_dirstart] = (dir_start << 1) & 0xFFFF;
	pbuf[h_ms_dirstart] = (dir_start << 1) >> 16;
	
	pbuf[h_ls_dirlen]   = (dir_size << 1) & 0xFFFF;
	pbuf[h_ms_dirlen]   = (dir_size << 1) >> 16;
	
	pbuf[h_ls_datastart] = (data_start << 1) & 0xFFFF;
	pbuf[h_ms_datastart] = (data_start << 1) >> 16;
	
	pbuf[h_ls_datalen]   = (data_size << 1) & 0xFFFF;
	pbuf[h_ms_datalen]   = (data_size << 1) >> 16;
	
	return (AbleDiskLib_WriteAbleDisk(the_device, pbuf, 0, 2));
}


// =================================================================================
//		¥ AbleOptLib_RenameFile()
// =================================================================================

int	AbleOptLib_RenameFile(InterChangeItemUnion& topUnion, LCStr255& topName, LCStr255& treeName, LCStr255& theNewName, interchange_settings& theSettings)
{
	int						 status;
	CSharedOpticalDataBase*  opticalData   = NULL;
	ArrayIndexT 			 sortedWhere   = 0, listWhere = 0, newWhere = 0;
	scsi_device*			 source_device = NULL;
	uint16				     gBuf[512];
	int						 i;
	
	// Check file name
	if (theNewName.Length() == 0 || theNewName.Length() > 8)
	{
		sprintf(AbleDiskLib_RecentErrorMessage, "Illegal file name");
		printf("InterChangeª: Could not rename '%s' to '%s'\n   %s\n", (char *) treeName, (char *) theNewName, AbleDiskLib_RecentErrorMessage);
		return (-1);
	}
	
	for (i=1; i<= theNewName.Length(); i++)
	{
		if (valid_filechar(theNewName[i]) == false)
		{
			sprintf(AbleDiskLib_RecentErrorMessage, "Illegal file name character");
			printf("InterChangeª: Could not rename '%s' to '%s'\n   %s\n", (char *) treeName, (char *) theNewName, AbleDiskLib_RecentErrorMessage);
			return (-1);
		}
	}
	
	// Make sure is optical and can access it
	source_device = access_scsi_device(topUnion.OpticalItem.file_device_code);

	if ((topUnion.Item.file_system != InterChange_AbleOpticalFileSystemCode)
	||  (source_device == NULL))
		return (-1);
		
	// Find by device code
	if (topUnion.OpticalItem.file_device_code)
		opticalData = CSharedOpticalDataBase::FindOpticalDataByLegacyCode(topUnion.OpticalItem.file_device_code);
		
	// Find by file spec
	if (!opticalData)
	{
		FSSpec fileSpec;
		
		AbleDiskLib_FetchReleventFSSpec(topUnion, fileSpec, theSettings);

		if (fileSpec.name[0])
			opticalData = CSharedOpticalDataBase::FindOpticalDataByFSSPec(fileSpec);
	}
	
	if (!opticalData)
	{
		sprintf(AbleDiskLib_RecentErrorMessage, "Could not find shared optical data");
		printf("InterChangeª: Could not rename '%s' to '%s'\n   %s\n", (char *) treeName, (char *) theNewName, AbleDiskLib_RecentErrorMessage);
		return (-1);
	}
	
	// Make sure no one is writing the index
	status = opticalData->mSemaphore.Wait(semaphore_NoWait);					// access optical data structures without waiting

	if (status == errSemaphoreTimedOut)											// if data sturctures not available
	{
		opticalData->ReleaseOpticalData();

		sprintf(AbleDiskLib_RecentErrorMessage, "Shared ndex is busy being updated");
		printf("InterChangeª: Could not rename '%s' to '%s'\n   %s\n", (char *) treeName, (char *) theNewName, AbleDiskLib_RecentErrorMessage);
		return (-1);
	}
	
	// Make sure no one is reading the index
	if (opticalData->mReaders)
	{
		opticalData->mSemaphore.Signal();
		opticalData->ReleaseOpticalData();

		sprintf(AbleDiskLib_RecentErrorMessage, "Index is busy");
		printf("InterChangeª: Could not rename '%s' to '%s'\n   %s\n", (char *) treeName, (char *) theNewName, AbleDiskLib_RecentErrorMessage);
		return (-1);
	}
	
	// Make sure in-memory data base is up to date
	InterChangeItemDirectory theDir;
	
	memset(&theDir, 0, sizeof(theDir));
	
	theDir.root = topUnion;
	
	if ((AbleOptLib_ReadOpticalDirectory(theDir, treeName))
	||  (!opticalData->IsUpToDate(theDir.optical_data)))
	{
		opticalData->mSemaphore.Signal();
		opticalData->ReleaseOpticalData();

		sprintf(AbleDiskLib_RecentErrorMessage, "Index is not up-to-date");
		printf("InterChangeª: Could not rename '%s' to '%s'\n   %s\n", (char *) treeName, (char *) theNewName, AbleDiskLib_RecentErrorMessage);
		return (-1);
	}

	// Isolate file name
	LCStr255			fileName;
	unsigned char 		priorColon = 0, nextColon = 0;
	unsigned short		entryNum;
	SOpticalFileData	fileData, newData;
	char				checkName[64];
	unsigned short      checkTime;
	
	memset(&fileData, 0, sizeof(fileData));
	memset(&newData,  0, sizeof(newData ));

	nextColon = topName.Find(':', priorColon+1);						// find next :
	
	while (nextColon)
	{
		priorColon = nextColon;
		nextColon = topName.Find(':', priorColon+1);					// find next :
	}

	if (priorColon == 0)												// rename of file
		fileName = topName;
	else
		fileName.Assign((void *) (&topName[priorColon+1]), topName.Length() - priorColon);

	if (fileName  [0] > 8) fileName  [0] = 8;
	if (theNewName[0] > 8) theNewName[0] = 8;
	
	strcpy(fileData.Name, (char *) fileName  );
	strcpy(newData.Name,  (char *) theNewName);
	
	// Find file entry
	sortedWhere = opticalData->mSortedFileList.FetchIndexOfKey(&fileData);
	newWhere    = opticalData->mSortedFileList.FetchIndexOfKey(&newData );

	// Look up file number
	if (sortedWhere)
		listWhere = opticalData->mSortedFileList[sortedWhere];

	if (!opticalData->mFileList.ValidIndex(listWhere))
	{
		opticalData->mSemaphore.Signal();
		opticalData->ReleaseOpticalData();

		sprintf(AbleDiskLib_RecentErrorMessage, "File not found");
		printf("InterChangeª: Could not rename '%s' to '%s'\n   %s\n", (char *) treeName, (char *) theNewName, AbleDiskLib_RecentErrorMessage);
		return (-1);
	}
	
	entryNum = opticalData->mFileList[listWhere].DirEntry;
	
	// Make sure new name does not already exist
	if (opticalData->mSortedFileList.ValidIndex(newWhere))
	{
		opticalData->mSemaphore.Signal();
		opticalData->ReleaseOpticalData();

		sprintf(AbleDiskLib_RecentErrorMessage, "File already exists by that name");
		printf("InterChangeª: Could not rename '%s' to '%s'\n   %s\n", (char *) treeName, (char *) theNewName, AbleDiskLib_RecentErrorMessage);
		return (-1);
	}
	
	// See if a file by the new name used to exist on the media.  If so, we will place an update record at the end
	// of the media.  Otherwise, we can modify the existing record.
	Boolean canReWrite = true;
	
	for (i=1; i<=opticalData->mFileList.GetCount(); i++)
	{
		SOpticalFileData&	theData = opticalData->mFileList[i];
		
		if (strcmp(theData.Name, newData.Name) == 0)
		{
			canReWrite = false;
			break;
		}
	}
	
	if (!canReWrite)
	{
		if (opticalData->mOpticalData.dir_free == 0)
		{
			opticalData->mSemaphore.Signal();
			opticalData->ReleaseOpticalData();

			sprintf(AbleDiskLib_RecentErrorMessage, "Directory space exhausted on media");
			printf("InterChangeª: Could not rename '%s' to '%s'\n   %s\n", (char *) treeName, (char *) theNewName, AbleDiskLib_RecentErrorMessage);
			return (-1);
		}
	}
	
	// Read prior entry.  Rewrite changed entry if we can, otherwise append a new entry
	status = AbleDiskLib_ReadAbleDisk(source_device, gBuf, opticalData->mOpticalData.dir_start + (entryNum<<1), 2);

	if (status)
	{
		opticalData->mSemaphore.Signal();
		opticalData->ReleaseOpticalData();

		sprintf(AbleDiskLib_RecentErrorMessage, "Could not read optical media (Entry)");
		printf("InterChangeª: Could not rename '%s' to '%s'\n   %s\n", (char *) treeName, (char *) theNewName, AbleDiskLib_RecentErrorMessage);
		return (-1);
	}
	
	for (i=0; i<4; i++)														// get file name from entry
	{
		checkName[i*2    ] = gBuf[e__name+i] & 0xFF;
		checkName[i*2 + 1] = gBuf[e__name+i] >> 8;
		checkName[i*2 + 2] = 0;
	}

	for (i=0; checkName[i] && checkName[i] == fileData.Name[i]; i++)		// make sure file name matches
		;
	
	if (checkName[i] || fileData.Name[i])
	{
		opticalData->mSemaphore.Signal();
		opticalData->ReleaseOpticalData();

		sprintf(AbleDiskLib_RecentErrorMessage, "Directory entry mis-match");
		printf("InterChangeª: Could not rename '%s' to '%s'\n   %s\n", (char *) treeName, (char *) theNewName, AbleDiskLib_RecentErrorMessage);
		return (-1);
	}
	
	for (i=theNewName.Length()+1; i<=8; i++)								// zero fill
		theNewName[i] = 0;

	// Create a rename entry if can't rewrite media
	if (!canReWrite)
	{
		// Check for phase error
		if (opticalData->mFileList.GetCount() != opticalData->mOpticalData.dir_next)
		{
			opticalData->mSemaphore.Signal();
			opticalData->ReleaseOpticalData();

			sprintf(AbleDiskLib_RecentErrorMessage, "Directory phase error");
			printf("InterChangeª: Could not rename '%s' to '%s'\n   %s\n", (char *) treeName, (char *) theNewName, AbleDiskLib_RecentErrorMessage);
			return (-1);
		}

		for (i=0; i<4; i++)													// set up old name
			gBuf[e_oldname+i] = gBuf[e__name+i];
		
		for (i=0; i<4; i++)													// store file name
		{
			gBuf[e__name+i] = (((uint16) (uint8) theNewName[i*2 + 1])     )
						    | (((uint16) (uint8) theNewName[i*2 + 2]) << 8);
		}
	
		gBuf[e__type     ] = e_rename_entry;
		gBuf[e_prev_entry] = entryNum;

		status = AbleDiskLib_WriteAbleDisk(source_device, gBuf, opticalData->mOpticalData.dir_start + (opticalData->mOpticalData.dir_next<<1), 2);
		
		if (status)
		{
			opticalData->mSemaphore.Signal();
			opticalData->ReleaseOpticalData();

			sprintf(AbleDiskLib_RecentErrorMessage, "Could not write optical media (Entry)");
			printf("InterChangeª: Could not rename '%s' to '%s'\n   %s\n", (char *) treeName, (char *) theNewName, AbleDiskLib_RecentErrorMessage);
			return (-1);
		}
	
		// Update name and entry num of this record
		fileData = opticalData->mFileList[listWhere];
		memset(&fileData.Name, 0, sizeof(fileData.Name));
		strcpy(fileData.Name, (char *) theNewName);
		
		fileData.DirEntry = opticalData->mOpticalData.dir_next;
		
		try
		{
			listWhere = opticalData->mFileList.AddItem(fileData);
		}

		catch (ExceptionCode theError)
		{
			SysBeep(10);
			if (theError == memFullErr)
				printf("Ran out of memory in AbleOptLib_RenameFile\n");
			else
				printf("Failed AbleOptLib_RenameFile (%d)\n", theError);
		}

		catch (...)
		{
			SysBeep(10);
			printf("Failed AbleOptLib_RenameFile\n");
		}
		
		// Update index size
		opticalData->mOpticalData.dir_used++;
		opticalData->mOpticalData.dir_free--;
		opticalData->mOpticalData.dir_next++;
	}
	
	// Else update prior entry and change volume time
	else
	{	
		for (i=0; i<4; i++)													// store file name
		{
			gBuf[e__name+i] = (((uint16) (uint8) theNewName[i*2 + 1])     )
						    | (((uint16) (uint8) theNewName[i*2 + 2]) << 8);
		}

		status = AbleDiskLib_WriteAbleDisk(source_device, gBuf, opticalData->mOpticalData.dir_start + (entryNum<<1), 2);
	
		if (status)
		{
			opticalData->mSemaphore.Signal();
			opticalData->ReleaseOpticalData();

			sprintf(AbleDiskLib_RecentErrorMessage, "Could not write optical media (Entry)");
			printf("InterChangeª: Could not rename '%s' to '%s'\n   %s\n", (char *) treeName, (char *) theNewName, AbleDiskLib_RecentErrorMessage);
			return (-1);
		}
		
		// Extract record and change name
		fileData = opticalData->mFileList[listWhere];
		memset(&fileData.Name, 0, sizeof(fileData.Name));
		strcpy(fileData.Name, (char *) theNewName);
		opticalData->mFileList[listWhere] = fileData;
	}
	
	// Read header and update time field
	status = AbleDiskLib_ReadAbleDisk(source_device, gBuf, opticalData->mOpticalData.header, 2);

	if (status)
	{
		opticalData->mSemaphore.Signal();
		opticalData->ReleaseOpticalData();

		sprintf(AbleDiskLib_RecentErrorMessage, "Could not read optical media (header)");
		printf("InterChangeª: Could not rename '%s' to '%s'\n   %s\n", (char *) treeName, (char *) theNewName, AbleDiskLib_RecentErrorMessage);
		return (-1);
	}

	for (i=0; i<h_name_max; i++)
	{
		checkName[i*2    ] = gBuf[h_name+i] & 0xFF;
		checkName[i*2 + 1] = gBuf[h_name+i] >> 8;
		checkName[i*2 + 2] = 0;
	}

	checkTime = (unsigned short) gBuf[h_time];
	
	for (i=0; checkName[i] && checkName[i] == opticalData->mOpticalData.volume_name[i]; i++)
		;
	
	if (checkName[i] || opticalData->mOpticalData.volume_name[i] || (checkTime != opticalData->mOpticalData.volume_time))
	{
		opticalData->mSemaphore.Signal();
		opticalData->ReleaseOpticalData();

		sprintf(AbleDiskLib_RecentErrorMessage, "Header mis-match");
		printf("InterChangeª: Could not rename '%s' to '%s'\n   %s\n", (char *) treeName, (char *) theNewName, AbleDiskLib_RecentErrorMessage);
		return (-1);
	}
	
	gBuf[h_time]++;
	
	status = AbleDiskLib_WriteAbleDisk(source_device, gBuf, opticalData->mOpticalData.header, 2);

	if (status)
	{
		opticalData->mSemaphore.Signal();
		opticalData->ReleaseOpticalData();

		sprintf(AbleDiskLib_RecentErrorMessage, "Could not write optical media (Header)");
		printf("InterChangeª: Could not rename '%s' to '%s'\n   %s\n", (char *) treeName, (char *) theNewName, AbleDiskLib_RecentErrorMessage);
		return (-1);
	}

	opticalData->mOpticalData.volume_time = (unsigned short) (gBuf[h_time]);
		
	// Remove from sorted list and re-insert
	opticalData->mSortedFileList.RemoveItemsAt(1, sortedWhere);
	newWhere = opticalData->mSortedFileList.InsertItemsAt(1, 0, listWhere);
	
	// Update category ranges
	for (i=0; i<8; i++)
	{
		ArrayIndexT tempIndex;

		if ((fileData.Categories[i])
		&&  (opticalData->mCategoryList.ValidIndex(tempIndex = fileData.Categories[i])))
		{
			SOpticalCategory&	catData = opticalData->mCategoryList[fileData.Categories[i]];

			// Detect adding first file to a category
			if (catData.first_file == 0 && catData.last_file == 0)
			{
				catData.first_file = fileData.DirEntry+1;								// this is first file in this category
				catData.last_file  = fileData.DirEntry+1;
			}
				
			// Detect adding a second file to a category
			else if ((opticalData->mFileList.ValidIndex(tempIndex = catData.first_file))
			&&       (opticalData->mFileList.ValidIndex(tempIndex = catData.last_file )))
			{
				ArrayIndexT	sortedFirst = opticalData->mSortedFileList.FetchIndexOf(catData.first_file);
				ArrayIndexT	sortedLast  = opticalData->mSortedFileList.FetchIndexOf(catData.last_file );

				// If catData.first_file has been deleted, point to the whole file list
				if (sortedFirst == 0) sortedFirst = 1;
				if (sortedLast  == 0) sortedLast  = opticalData->mSortedFileList.GetCount();
				
				// If new entry is lower (alphabetically) than prior, extend range
				// of category accordingly.  Also, handle rename records intelligently.
				if (newWhere <= sortedFirst)
					catData.first_file = fileData.DirEntry+1;

				// If renaming what had been first entry, point to prior one
				else if ((catData.first_file == (entryNum + 1))
				&&       (sortedFirst > 1))
					catData.first_file = opticalData->mSortedFileList[sortedFirst-1];

				if (newWhere > sortedLast)
					catData.last_file  = fileData.DirEntry+1;

				// If renaming what had been last entry, point to next one
				else if ((catData.last_file == (entryNum + 1))
				&&       (sortedLast < opticalData->mSortedFileList.GetCount()))
					catData.last_file = opticalData->mSortedFileList[sortedLast+1];

			}
		}
	}
	
	// Done		
	opticalData->mSemaphore.Signal();
	opticalData->ReleaseOpticalData();
	
	return (status);
}


// =================================================================================
//		¥ AbleOptLib_UnsaveFile()
// =================================================================================

int	AbleOptLib_UnsaveFile(InterChangeItemUnion& topUnion, LCStr255& topName, LCStr255& treeName, interchange_settings& theSettings)
{
	int						 status;
	CSharedOpticalDataBase*  opticalData   = NULL;
	ArrayIndexT 			 sortedWhere   = 0, listWhere = 0;
	scsi_device*			 source_device = NULL;
	uint16				     gBuf[512];
	int						 i;
	
	// Make sure is optical and can access it
	source_device = access_scsi_device(topUnion.OpticalItem.file_device_code);

	if ((topUnion.Item.file_system != InterChange_AbleOpticalFileSystemCode)
	||  (source_device == NULL))
		return (-1);
		
	// Find by device code
	if (topUnion.OpticalItem.file_device_code)
		opticalData = CSharedOpticalDataBase::FindOpticalDataByLegacyCode(topUnion.OpticalItem.file_device_code);
		
	// Find by file spec
	if (!opticalData)
	{
		FSSpec fileSpec;
		
		AbleDiskLib_FetchReleventFSSpec(topUnion, fileSpec, theSettings);

		if (fileSpec.name[0])
			opticalData = CSharedOpticalDataBase::FindOpticalDataByFSSPec(fileSpec);
	}
	
	if (!opticalData)
	{
		sprintf(AbleDiskLib_RecentErrorMessage, "Could not find shared optical data");
		printf("InterChangeª: Could not unsave '%s'\n   %s\n", (char *) treeName, AbleDiskLib_RecentErrorMessage);
		return (-1);
	}
	
	// Make sure no one is writing the index
	status = opticalData->mSemaphore.Wait(semaphore_NoWait);					// access optical data structures without waiting

	if (status == errSemaphoreTimedOut)											// if data sturctures not available
	{
		opticalData->ReleaseOpticalData();

		sprintf(AbleDiskLib_RecentErrorMessage, "Shared ndex is busy being updated");
		printf("InterChangeª: Could not unsave '%s'\n   %s\n", (char *) treeName, AbleDiskLib_RecentErrorMessage);
		return (-1);
	}
	
	// Make sure no one is reading the index
	if (opticalData->mReaders)
	{
		opticalData->mSemaphore.Signal();
		opticalData->ReleaseOpticalData();

		sprintf(AbleDiskLib_RecentErrorMessage, "Index is busy");
		printf("InterChangeª: Could not unsave '%s'\n   %s\n", (char *) treeName, AbleDiskLib_RecentErrorMessage);
		return (-1);
	}
	
	// Make sure in-memory data base is up to date
	InterChangeItemDirectory theDir;
	
	memset(&theDir, 0, sizeof(theDir));
	
	theDir.root = topUnion;
	
	if ((AbleOptLib_ReadOpticalDirectory(theDir, treeName))
	||  (!opticalData->IsUpToDate(theDir.optical_data)))
	{
		opticalData->mSemaphore.Signal();
		opticalData->ReleaseOpticalData();

		sprintf(AbleDiskLib_RecentErrorMessage, "Index is not up-to-date");
		printf("InterChangeª: Could not unsave '%s'\n   %s\n", (char *) treeName, AbleDiskLib_RecentErrorMessage);
		return (-1);
	}

	// Isolate file name
	LCStr255			fileName;
	unsigned char 		priorColon = 0, nextColon = 0;
	unsigned short		entryNum;
	SOpticalFileData	fileData;
	char				checkName[64];
	unsigned short      checkTime;
	
	memset(&fileData, 0, sizeof(fileData));

	nextColon = topName.Find(':', priorColon+1);						// find next :
	
	while (nextColon)
	{
		priorColon = nextColon;
		nextColon = topName.Find(':', priorColon+1);					// find next :
	}

	if (priorColon == 0)												// unsave of file
		fileName = topName;
	else
		fileName.Assign((void *) (&topName[priorColon+1]), topName.Length() - priorColon);

	if (fileName  [0] > 8) fileName  [0] = 8;
	
	strcpy(fileData.Name, (char *) fileName  );
	
	// Find file entry
	sortedWhere = opticalData->mSortedFileList.FetchIndexOfKey(&fileData);

	// Look up file number
	if (sortedWhere)
		listWhere = opticalData->mSortedFileList[sortedWhere];

	if (!opticalData->mFileList.ValidIndex(listWhere))
	{
		opticalData->mSemaphore.Signal();
		opticalData->ReleaseOpticalData();

		sprintf(AbleDiskLib_RecentErrorMessage, "File not found");
		printf("InterChangeª: Could not unsave '%s'\n   %s\n", (char *) treeName, AbleDiskLib_RecentErrorMessage);
		return (-1);
	}
	
	entryNum = opticalData->mFileList[listWhere].DirEntry;
	
	// Read prior entry.  Change a rename entry to a delete entry; just overwrite a directory entry...
	status = AbleDiskLib_ReadAbleDisk(source_device, gBuf, opticalData->mOpticalData.dir_start + (entryNum<<1), 2);

	if (status)
	{
		opticalData->mSemaphore.Signal();
		opticalData->ReleaseOpticalData();

		sprintf(AbleDiskLib_RecentErrorMessage, "Could not read optical media (Entry)");
		printf("InterChangeª: Could not unsave '%s'\n   %s\n", (char *) treeName, AbleDiskLib_RecentErrorMessage);
		return (-1);
	}
	
	for (i=0; i<4; i++)														// get file name from entry
	{
		checkName[i*2    ] = gBuf[e__name+i] & 0xFF;
		checkName[i*2 + 1] = gBuf[e__name+i] >> 8;
		checkName[i*2 + 2] = 0;
	}

	for (i=0; checkName[i] && checkName[i] == fileData.Name[i]; i++)		// make sure file name matches
		;
	
	if (checkName[i] || fileData.Name[i])
	{
		opticalData->mSemaphore.Signal();
		opticalData->ReleaseOpticalData();

		sprintf(AbleDiskLib_RecentErrorMessage, "Directory entry mis-match");
		printf("InterChangeª: Could not unsave '%s'\n   %s\n", (char *) treeName, AbleDiskLib_RecentErrorMessage);
		return (-1);
	}
	
	// See if we can re-write this directory entry.  We can't if this is the last valid directory
	// entry or else GetStatistics messes up finding the last valid block
	Boolean canReWrite = true;

	if (entryNum == opticalData->mOpticalData.dir_last)
		canReWrite = false;
	
	// Make sure room
	if (!canReWrite)
	{
		if (opticalData->mOpticalData.dir_free == 0)
		{
			opticalData->mSemaphore.Signal();
			opticalData->ReleaseOpticalData();

			sprintf(AbleDiskLib_RecentErrorMessage, "Directory space exhausted on media");
			printf("InterChangeª: Could not unsave '%s'\n   %s\n", (char *) treeName, AbleDiskLib_RecentErrorMessage);
			return (-1);
		}

		// Check for phase error
		if (opticalData->mFileList.GetCount() != opticalData->mOpticalData.dir_next)
		{
			opticalData->mSemaphore.Signal();
			opticalData->ReleaseOpticalData();

			sprintf(AbleDiskLib_RecentErrorMessage, "Directory phase error");
			printf("InterChangeª: Could not unsave '%s'\n   %s\n", (char *) treeName, AbleDiskLib_RecentErrorMessage);
			return (-1);
		}

		gBuf[e__type     ] = e_delete_entry;
		gBuf[e_prev_entry] = entryNum;

		for (i=0; i<4; i++)													// set up old name
			gBuf[e_oldname+i] = gBuf[e__name+i];
		
		for (i=0; i<512; i++)												// clean up rename entry to look like a legit delete entry
		{
			if ((i != e__type)
			&&  (i != e_prev_entry)
			&&  (i < e_oldname)
			&&  (i > e_oldname+3))
				gBuf[i] = 0;
		}
	
		status = AbleDiskLib_WriteAbleDisk(source_device, gBuf, opticalData->mOpticalData.dir_start + (opticalData->mOpticalData.dir_next<<1), 2);
		
		if (status)
		{
			opticalData->mSemaphore.Signal();
			opticalData->ReleaseOpticalData();

			sprintf(AbleDiskLib_RecentErrorMessage, "Could not write optical media (Entry)");
			printf("InterChangeª: Could not unsave '%s'\n   %s\n", (char *) treeName, AbleDiskLib_RecentErrorMessage);
			return (-1);
		}
	
		// Update name and entry num of this record
		fileData = opticalData->mFileList[listWhere];
		memset(&fileData.Name, 0, sizeof(fileData.Name));
		
		fileData.DirEntry = opticalData->mOpticalData.dir_next;
		
		try
		{
			listWhere = opticalData->mFileList.AddItem(fileData);
		}

		catch (ExceptionCode theError)
		{
			SysBeep(10);
			if (theError == memFullErr)
				printf("Ran out of memory in AbleOptLib_UnsaveFile\n");
			else
				printf("Failed AbleOptLib_UnsaveFile (%d)\n", theError);
		}

		catch (...)
		{
			SysBeep(10);
			printf("Failed AbleOptLib_UnsaveFile\n");
		}
		
		// Update index size
		opticalData->mOpticalData.dir_used++;
		opticalData->mOpticalData.dir_free--;
		opticalData->mOpticalData.dir_next++;
	}
	
	// Change a rename entry to a delete entry (e.g. delete what had been renamed or udpated)
	else
	{
		if (gBuf[e__type] == e_rename_entry)
		{
			gBuf[e__type] = e_delete_entry;
		
			for (i=0; i<512; i++)												// clean up rename entry to look like a legit delete entry
			{
				if ((i != e__type)
				&&  (i != e_prev_entry)
				&&  (i < e_oldname)
				&&  (i > e_oldname+3))
					gBuf[i] = 0;
			}
		}
		
		// Else change a directory entry to zippo
		else
		{
			gBuf[e__type     ] = e_delete_entry;
			gBuf[e_prev_entry] = entryNum;
		
			for (i=0; i<4; i++)													// set old file name
				gBuf[e_oldname + i] = gBuf[e__name + i];

			for (i=0; i<512; i++)												// clean up entry to look like a legit delete entry
			{
				if ((i != e__type)
				&&  (i != e_prev_entry)
				&&  (i < e_oldname)
				&&  (i > e_oldname+3))
					gBuf[i] = 0;
			}
		}
	
		status = AbleDiskLib_WriteAbleDisk(source_device, gBuf, opticalData->mOpticalData.dir_start + (entryNum<<1), 2);

		if (status)
		{
			opticalData->mSemaphore.Signal();
			opticalData->ReleaseOpticalData();

			sprintf(AbleDiskLib_RecentErrorMessage, "Could not write optical media (Entry)");
			printf("InterChangeª: Could not unsave '%s'\n   %s\n", (char *) treeName, AbleDiskLib_RecentErrorMessage);
			return (-1);
		}
	}

	// Read header and update time field
	status = AbleDiskLib_ReadAbleDisk(source_device, gBuf, opticalData->mOpticalData.header, 2);

	if (status)
	{
		opticalData->mSemaphore.Signal();
		opticalData->ReleaseOpticalData();

		sprintf(AbleDiskLib_RecentErrorMessage, "Could not read optical media (header)");
		printf("InterChangeª: Could not unsave '%s'\n   %s\n", (char *) treeName, AbleDiskLib_RecentErrorMessage);
		return (-1);
	}

	for (i=0; i<h_name_max; i++)
	{
		checkName[i*2    ] = gBuf[h_name+i] & 0xFF;
		checkName[i*2 + 1] = gBuf[h_name+i] >> 8;
		checkName[i*2 + 2] = 0;
	}

	checkTime = (unsigned short) gBuf[h_time];
	
	for (i=0; checkName[i] && checkName[i] == opticalData->mOpticalData.volume_name[i]; i++)
		;
	
	if (checkName[i] || opticalData->mOpticalData.volume_name[i] || (checkTime != opticalData->mOpticalData.volume_time))
	{
		opticalData->mSemaphore.Signal();
		opticalData->ReleaseOpticalData();

		sprintf(AbleDiskLib_RecentErrorMessage, "Header mis-match");
		printf("InterChangeª: Could not unsave '%s'\n   %s\n", (char *) treeName, AbleDiskLib_RecentErrorMessage);
		return (-1);
	}
	
	gBuf[h_time]++;
	
	status = AbleDiskLib_WriteAbleDisk(source_device, gBuf, opticalData->mOpticalData.header, 2);

	if (status)
	{
		opticalData->mSemaphore.Signal();
		opticalData->ReleaseOpticalData();

		sprintf(AbleDiskLib_RecentErrorMessage, "Could not write optical media (Header)");
		printf("InterChangeª: Could not unsave '%s'\n   %s\n", (char *) treeName, AbleDiskLib_RecentErrorMessage);
		return (-1);
	}

	opticalData->mOpticalData.volume_time = (unsigned short) (gBuf[h_time]);
		
	// Remove from sorted list
	opticalData->mSortedFileList.RemoveItemsAt(1, sortedWhere);
	
	// Update category ranges
	fileData = opticalData->mFileList[listWhere];				// Look up complete filelist record for this entry

	for (i=0; i<8; i++)
	{
		ArrayIndexT tempIndex;

		if ((fileData.Categories[i])
		&&  (opticalData->mCategoryList.ValidIndex(tempIndex = fileData.Categories[i])))
		{
			SOpticalCategory&	catData = opticalData->mCategoryList[fileData.Categories[i]];

			// See if deleting what had been first file or last file in this category
			if ((opticalData->mFileList.ValidIndex(tempIndex = catData.first_file))
			&&  (opticalData->mFileList.ValidIndex(tempIndex = catData.last_file )))
			{
				ArrayIndexT	sortedFirst = opticalData->mSortedFileList.FetchIndexOf(catData.first_file);
				ArrayIndexT	sortedLast  = opticalData->mSortedFileList.FetchIndexOf(catData.last_file );

				// If catData.first_file has been deleted, point to the whole file list
				if (sortedFirst == 0) sortedFirst = 1;
				if (sortedLast  == 0) sortedLast  = opticalData->mSortedFileList.GetCount();
				
				// If deleting what had been first entry, point to prior one
				if ((catData.first_file == (entryNum + 1))
				&&  (sortedFirst > 1))
					catData.first_file = opticalData->mSortedFileList[sortedFirst-1];

				// If deleting what had been last entry, point to next one
				if ((catData.last_file == (entryNum + 1))
				&&  (sortedLast < opticalData->mSortedFileList.GetCount()))
					catData.last_file = opticalData->mSortedFileList[sortedLast+1];
			}
		}
	}
	
	// Done		
	opticalData->mSemaphore.Signal();
	opticalData->ReleaseOpticalData();
	
	return (status);
}


// =================================================================================
//		¥ AbleOptLib_ParseCaptionsFromSFHeader()
// =================================================================================

// Scans up to 8 caption strings from a sound file header
static	void AbleOptLib_ParseCaptionsFromSFHeader(fixed gbuf[], LCStr255 theCaps[8], long& numCaps)
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
			
			theCap[0] = len;
			for (k=0; k<len; k++)
				theCap[k+1] = byte(&gbuf[nextCatPtr], k);
			
			nextCatPtr += (1 + ((gbuf[nextCatPtr]+1)>>1));
			
			// disallow null category names (indicates serious error)
			if (theCap.Length() == 0)
				continue;
							
			// disallow leading :'s in categories; change to -'s
			if (theCap[1] == ':')
				theCap[1] = '-';
				
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
//		¥ AbleOptLib_ConstructOpticalIndex()
// =================================================================================

// Higher level routine called from a thread to construct a complete optical disk
// index (file list, category list, etc.)

// Data base must be accessed with no readers and HFS refnum latched before calling
int AbleOptLib_ConstructOpticalIndex(InterChangeItemUnion& itsUnion, CSharedOpticalDataBase& itsOpticalData, long& abortMe, long& fatalUpdateErrorOccured,
								     void (*gearTuner) (void *), void *gearData)
{
	handle			bufHandle    = NULL;										// handle to large buffer for reading entries
	byte			*entryBuf    = NULL;										// buffer for reading directory entries
	long			firstEntry   = 0;											// first and last entry to read
	long			lastEntry    = 0;
	long			chunkSize    = 128;											// 256 entries per disk chunk
	long			entrySize	 = 1024;										// 1024 bytes per entry
	scsi_device* 	itsDevice 	 = NULL;
	Boolean			errorPosted  = false;
	long			yieldedTime  = TickCount();
	long			x;

	fatalUpdateErrorOccured = false;											// init to no error
	
	firstEntry = 0;
	lastEntry  = itsOpticalData.mOpticalData.dir_used;
	
	itsOpticalData.mFileList.RemoveAllItemsAfter(0);							// remove any prior possibly out of date entries
	itsOpticalData.mCategoryList.RemoveAllItemsAfter(0);
	itsOpticalData.mSortedFileList.RemoveAllItemsAfter(0);
	itsOpticalData.mSortedCategoryList.RemoveAllItemsAfter(0);
	
	itsDevice = access_scsi_device(itsUnion.OpticalItem.file_device_code);
		
	if (!itsDevice)
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
			// Get memory for buffer
			if (!bufHandle || !entryBuf)
			{
				bufHandle = get_big_memory(chunkSize*entrySize);
				
				if (bufHandle)
					entryBuf = (byte *) *bufHandle;
					
				if (!bufHandle || !entryBuf)
				{
					printf("InterChangeª: No memory available for buffer\n");
					SysBeep(10);
									
					abortMe = true;
					continue;
				}
			}
			
			// Turn the busy pane
			if (gearTuner) gearTuner(gearData);
			
			// Read a chunk of entries
			long	chunkEntries = lastEntry - firstEntry;
			long	blockNum     = itsOpticalData.mOpticalData.dir_start + (firstEntry << 1);
			long	blockSize    = itsDevice->fBlockSize;
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

					LThread::Yield();
					yieldedTime = TickCount();

					if (abortMe)
						continue;
				
					goto retry_chunk;
				}
				
				if (!errorPosted)
				{
					printf("InterChangeª: Optical Disk catalog entry could not be read due to medium error\n");
					SysBeep(10);
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
					printf("InterChangeª: Optical Disk catalog entry could not be read\n   SCSI error Report:\n");
					printf("   %s\n", er_mess);
					SysBeep(10);
					errorPosted = true;
				}
				
				memset(entryBuf, 0, chunkEntries*entrySize);							// provide all zeroes of data we could not read
			}		

			// Process the entries into the optical data base
			fixed* gbuf = (fixed*) &entryBuf[0];		

			while (chunkEntries)
			{
				SOpticalFileData	fileData;
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
				long		numCaps = 0;
				
				AbleOptLib_ParseCaptionsFromSFHeader(&gbuf[e_header], catStrings, numCaps);
				
				// Enter categories into category list for entry and rename records only
				if ((gbuf[e__type] == e_dir_entry) || (gbuf[e__type] == e_rename_entry))
				{
					ArrayIndexT sortedWhere;
					ArrayIndexT tempIndex;
					
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
						SOpticalCategory	catData;
						char*				theCap    = &catData.Name[0];
						ArrayIndexT    		where;
						unsigned short 		catCode;
						
						strcpy(catData.Name, (char *) catStrings[i]);					// copy entire caption string into place
						catData.first_file = 0;
						catData.last_file  = 0;
						
						while (*theCap && *theCap != ':')
							theCap++;
							
						while (*theCap == ':')
						{
							*theCap = 0;												// terminate higher category (e.g turn BASSES:ACOUSTIC into BASSES
							
							where = itsOpticalData.mSortedCategoryList.FetchIndexOfKey(&catData);

							if (where == 0)
							{
								catCode = itsOpticalData.mCategoryList.AddItem(catData);
								itsOpticalData.mSortedCategoryList.InsertItemsAt(1, 0, catCode);
							}
							
							*theCap++ = ':';											// restore colong
							
							while (*theCap && *theCap != ':')
								theCap++;
						}
						
						// Insert lowest level category (e.g. that containing the file)
						where = itsOpticalData.mSortedCategoryList.FetchIndexOfKey(&catData);

						// Adding this category for this first time.  Relatively easy
						if (where == 0)
						{
							catData.first_file = firstEntry+1;									// this is first file in this category
							catData.last_file  = firstEntry+1;
							
							catCode = itsOpticalData.mCategoryList.AddItem(catData);			// add category to category list
							itsOpticalData.mSortedCategoryList.InsertItemsAt(1, 0, catCode);	// add its index to the sorted list
						}
						
						// Else updating a category.  Do some analysis to find the range of files that
						// might be included in this category.  This speeds up the category display
						// since, for example, ABS40 contains files that begin with ABS...
						else if ((itsOpticalData.mSortedCategoryList.FetchItemAt(where, catCode))
						&&       (itsOpticalData.mCategoryList.ValidIndex(tempIndex = catCode)))
						{
							SOpticalCategory&	catData = itsOpticalData.mCategoryList[catCode];
							
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
								ArrayIndexT	sortedFirst = itsOpticalData.mSortedFileList.FetchIndexOf(catData.first_file);
								ArrayIndexT	sortedLast  = itsOpticalData.mSortedFileList.FetchIndexOf(catData.last_file );

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
									catData.first_file = itsOpticalData.mSortedFileList[sortedFirst-1];
								
								if (sortedWhere > sortedLast)
									catData.last_file  = firstEntry+1;

								// If renaming what had been last entry, point to next one
								else if ((gbuf[e__type] == e_rename_entry)
								&&       (catData.last_file == (unsigned short) (gbuf[e_prev_entry] + 1))
								&&       (sortedLast < itsOpticalData.mSortedFileList.GetCount()))
									catData.last_file = itsOpticalData.mSortedFileList[sortedLast+1];
							}
						}
						
						fileData.Categories[i] = catCode;
					}
				}
				
				// Caption
				j = gbuf[e_header + sf_id_field_bytes]; 				/* get length of caption */
				
				if (j<0) j = 0;
				if (j>(l_caption_max<<1)) j = l_caption_max<<1;

				for (i=0; i<j; i++)
					fileData.Caption[i] = byte(&gbuf[e_header + sf_id_field_bytes], i);

				// Add this file record to the file list (always, so it matches disk)
				ArrayIndexT	   whereAdded = itsOpticalData.mFileList.AddItem(fileData);
				unsigned short whereIs, whereWas;
				SOpticalFileData oldFileData;
					
				// Check for phase error
				if (whereAdded != firstEntry+1)
				{
					if (!fatalUpdateErrorOccured)
					{
						SysBeep(10);
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
							SysBeep(10);
							printf("AbleOptLib_ConstructOpticalIndex: Duplicate directory entry\n");
						}
						
						itsOpticalData.mSortedFileList.RemoveItemsAt(1, whereIs);
					}

					itsOpticalData.mSortedFileList.InsertItemsAt(1, 0, (unsigned short) whereAdded);
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

					// Make sure old name was in data base and that it matches
					whereIs = itsOpticalData.mSortedFileList.FetchIndexOfKey(&oldFileData);
					
					if (whereIs && itsOpticalData.mSortedFileList[whereIs] != whereWas)
					{
						if (0)
						{
							SysBeep(10);
							printf("AbleOptLib_ConstructOpticalIndex: Mismatched rename entry\n");
						}
						
						whereWas = itsOpticalData.mSortedFileList[whereIs] != whereWas;
					}
					
					// Make sure new name is not already in there
					whereIs = itsOpticalData.mSortedFileList.FetchIndexOfKey(&fileData);
					
					// if new name is in data base, had best be at prior entry and this is an update record
					if (whereIs && itsOpticalData.mSortedFileList[whereIs] != whereWas)
					{
						if (0)
						{
							SysBeep(10);
							printf("AbleOptLib_ConstructOpticalIndex: Duplicate rename entry\n");
						}
						
						itsOpticalData.mSortedFileList.RemoveItemsAt(1, whereIs);
					}
					
					// Remove pointer to deleted file from the sorted list
					itsOpticalData.mSortedFileList.Remove(whereWas);

					// Add rename entry to sorted list
					itsOpticalData.mSortedFileList.InsertItemsAt(1, 0, (unsigned short) whereAdded);
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

					// Make sure old name was in data base and that it matches
					whereIs = itsOpticalData.mSortedFileList.FetchIndexOfKey(&oldFileData);
					
					if (whereIs && itsOpticalData.mSortedFileList[whereIs] != whereWas)
					{
						if (0)
						{
							SysBeep(10);
							printf("AbleOptLib_ConstructOpticalIndex: Mismatched delete entry\n");
						}

						whereWas = itsOpticalData.mSortedFileList[whereIs];
					}

					// Remove pointer to deleted file from the sorted list
					itsOpticalData.mSortedFileList.Remove(whereWas);
				}
				
				chunkEntries--;
				firstEntry++;
				gbuf+=512;
			}

			// If processed all entries, mark optical data as up to date
			if (!fatalUpdateErrorOccured && firstEntry == lastEntry)
				itsOpticalData.SetIsUpToDate(true);
				
			// Yield periodically
			if ((x = (TickCount() - yieldedTime)) >= 3)
			{
				LThread::Yield();
				yieldedTime = TickCount();
				continue;															// resume
			}
		}
	}
	
	catch (ExceptionCode theError)
	{
		SysBeep(10);
		if (theError == memFullErr)
			printf("Ran out of memory in AbleOptLib_ConstructOpticalIndex\n");
		else
			printf("Failed AbleOptLib_ConstructOpticalIndex (%d)\n", theError);

		abortMe                 = true;
		fatalUpdateErrorOccured = true;
	}

	catch (...)
	{
		SysBeep(10);
		printf("Failed AbleOptLib_ConstructOpticalIndex\n");
		
		abortMe                 = true;
		fatalUpdateErrorOccured = true;
	}

	if (abortMe)
		fatalUpdateErrorOccured = true;
		
	if (fatalUpdateErrorOccured)
	{
		itsOpticalData.mFileList.RemoveAllItemsAfter(0);
		itsOpticalData.mCategoryList.RemoveAllItemsAfter(0);
		itsOpticalData.mSortedFileList.RemoveAllItemsAfter(0);
		itsOpticalData.mSortedCategoryList.RemoveAllItemsAfter(0);
	}

	if (bufHandle)
		free_big_memory(bufHandle);

	bufHandle = NULL;
	entryBuf  = NULL;

	return (noErr);
}



#if 0
/* $title Optical Disk Management Routines */

/*
 Optical Disk Management Routines

 by Kip Olson, February 1987

 Modification history:
    24 Feb 96 - CWJ - Recombined files. Reworked init_optical_controller to interrogate drive for info
    04 Mar 91 - LSS - several fixes for magneto, broke file into two pieces
    04 Oct 90 - LSS - buffer check bug fix in TRANSFEROPTICAL
    17 Jul 90 - LSS - support for 5-inch magneto optical devices
    27 Feb 89 - CWJ - moved HeaderGapSize to optlits for Repair program
                    - added error recovery to ReadHeader,  WriteHeader.
                    - added SearchEmpty1024kBlocks
    1  Aug 88 - KJO - Made TRANSFEROPTICAL global, added serial no. to header
    8  Jun 88 - KJO - Added WRITEHEADER, support for multiple volume headers
    8  Jul 87 - KJO - Added physical sector relignment, usage of directory pointer
*/


/*	Translated to C:   	12/22/99 at 22:29	*/
/*	Translator Version:	0.000				*/

#include	"XPL.h"
#include	"Optical.h"

_module(optical)

#include "Literals.h"
#include "Syslits.h"
#include "XPLASCIILit.h"
#include "Catrtns.h"
#include "SCSICode.h"
#include "CmdRout.h"
#include "Optlits.h"
#include "Devutil.h"

// Code for :xpl:optical

/* $subtitle Buffer Manipulation */
void			pbuf(													// store a word in the buffer
	fixed	index, 														// offset index into buffer
	fixed	value)														// value to store
	
{
	if (o_bufmed == 0) {												// internal memory
		set_able_core(o_bufptr + index, value);							// store word
	}
	else {																// external memory
		_write_60(o_bufptr + shr(index,8));								// sector in external memory
		_write_61((index & 0x00FF));									// word in that sector
		_write_62(value);												// store word
	}
}

/* $subtitle Operations on Directory Parameter */

static	void	cleanname(												// make sure name checks out
	fixed	oldname[], 													// name to check
	fixed	newname[], 													// space to put cleaned name
	fixed	max)	_recursive	_swap									// max no. words in name
{
    fixed			_upper0;
	fixed			i, ch;												// word length of name
	
	for (_upper0 = shl(max,1) - 1, i = 0; i <= _upper0; i++) {			// loop over name
		if (_ILT_(i, oldname[0])) {										// process character
			ch = byte(oldname,i);										// get character
			if ((ch >= l_a) && (ch <= l_z)) ch = ch - 0x0020;			// make upper case
		}
		else ch = 0;													// pad with nulls
		pbyte(_location_(&(newname[0]) - 1),i,ch);
	}
}
	
/* $subtitle Mode Select */

boolean			writeheader(											// write volume header
	fixed	header[], 													// header to write
	fixed	level)	_recursive	_swap									// level to write
{
	fixed			ms_sec, ls_sec;
	fixed			i;
	
	c_status = e_none;													// assume no error
	
	ms_sec = 0;															// start at first sector on disk
	ls_sec = 0;
	
	s_sensekey = AbleOptLib_transferoptical(s_extendedread,ms_sec,ls_sec,2,level);	// try to read the first volume header
	
	if ((s_sensekey != s_good       )
	&& (s_sensekey != s_mediumerror)
	&& (s_sensekey != s_blankcheck ))
		return (false);													// not formatted, or some other error
		
	i = 0;
	
	while ((_ILT_(i,          (headergapsize-2)))						// -2 for error recovery below
	&&       (s_sensekey !=  s_blankcheck    )) {						// read headers until a blank check is found
		i = i + 1;
		ls_sec = ls_sec + 2;											// try to read the next header
		if (_ILT_(ls_sec, 2)) ms_sec = ms_sec + 1;
		s_sensekey = AbleOptLib_transferoptical(s_extendedread,ms_sec,ls_sec,2,level);	// try to read the next volume header
		
		if ((s_sensekey != s_good       )
		&& (s_sensekey != s_mediumerror)
		&& (s_sensekey != s_blankcheck ))
			return (false);												// not formatted, or some other error
	}
		
	if (s_sensekey != s_blankcheck) {									// no more blank header entries
		c_status = e_dir_full;											// set catalog error status
		return (false);
	}
		
	for (i = 0; i < h_rec_length; i++) {								// copy header info into buffer
		pbuf(i,header[i]);
	}
		
	s_sensekey = AbleOptLib_transferoptical(s_extendedwrite,ms_sec,ls_sec,2,level);	// try to write the new volume header
	
	/* Must use complicated error recovery since block is reported as */
	/* blank,  but we cannot write ...  The one time I tried this,    */
	/* I was able to read the sector back from either logical sector  */
	/* number!!.                                                      */
	
	if (s_sensekey == s_blankcheck) {
		ls_sec = ls_sec + 2;											// try to read the next header
		if (_ILT_(ls_sec, 2)) ms_sec = ms_sec + 1;
		s_sensekey = AbleOptLib_transferoptical(s_extendedwrite,ms_sec,ls_sec,2,level);	// try to write the new volume header
	}
		
	if (s_sensekey != s_good) return (false);
		
	return (true);														// header saved successfully
}
	
/* $subtitle Format Disk */

boolean			optical_format(											// format a new disk
	fixed	name[], 													// volume name
	fixed	caption[], 													// volume caption
	fixed	date, 														// date stamp for volume
	fixed	time, 														// time stamp for volume
	fixed	serial, 													// serial number for volume
	fixed	level)	_recursive	_swap									// level to write volume on
{
    fixed			_upper0;
	fixed			devadr;												// Scsi device address
	fixed			ms_total, ls_total;									// total no. sectors on volume
	fixed			header_size;										// size of header
	fixed			header_gap;											// size of header gap
	fixed			dir_size;											// size of directory area
	fixed			dir_gap;											// size of directory gap
	fixed			ms_data, ls_data;									// size of data area
	fixed			data_gap;											// size of data gap
	fixed			databuf[5];											// general data buffer
	fixed			headername[h_name_max];								// Header name
	fixed			msw, lsw;
	fixed			controller;
	fixed			i, j;
	
	c_status = e_none;													// assume no errors
	s_sensekey = s_good;
	
	if (! (valid_filename(name) & 1)) {									// not a valid file name
		c_status = e_treename;
		return (false);													// die if bad name
	}
		
	cleanname(name,headername,h_name_max);								// clean and fill name
	
	devadr = AbleOptLib_finddevadr(level);								// find device address for this level and check for unit attention
	
	controller = AbleOptLib_lookup_controller_type(level);
	
	if (controller == quantum_controller) {
		s_sensekey = AbleOptLib_modeselectoptical(level);				// make sure drive is set up correctly
		if (s_sensekey != s_good) return (false);
	}
		
	s_sensekey = AbleOptLib_transferoptical(s_extendedread,0,0,2,level);			// try to read the first sector of the disk
	
	if (s_sensekey == s_good) {											// if it has already been written
		c_status = e_formatted;											// disk is already formatted
		return (false);													// die
	}
	else if (s_sensekey != s_blankcheck) return (false);				// some other error
		
	s_sensekey = readcapacity(devadr,databuf);							// get capacity information
	if (s_sensekey != s_good) return (false);							// an error occurred
		
	i = (shl(byte(databuf,6),8) | byte(databuf,7));						// get LSW of block size
	if (controller == ld1200_controller && i != 1024) {					// block size must be 1024 bytes for WORMS
		s_sensekey = s_baddevice;
		return (false);													// die
	}
		
	if (i != 512 && i != 1024)											// must be 512-byte or 1024-byte media at this point
	{
		s_sensekey = s_baddevice;
		return (false);													// die
	}
		
	ms_total = (shl(byte(databuf,0),8) | byte(databuf,1));				// add one to last logical block to get total blocks
	ls_total = (shl(byte(databuf,2),8) | byte(databuf,3)) + 1;
	if (ls_total == 0) ms_total = ms_total + 1;
		
	if (i == 512)														// if device is 512 media, compute number of 1024
	{																	// byte blocks it maps to for the following size
		ls_total = shr(ls_total, 1) | shl(ms_total, 15);				// computations...
		ms_total = shr(ms_total, 1);
	}
		
	// Limit to 8-gigabytes for use with readdata, writedata:
	
	if (_IGE_(ms_total, 128))											// note: limit is in 1024-byte sectors at this point!
	{
		ms_total = 0x007F;
		ls_total = ((fixed) 0xFFFF);
	}
		
	// Allocate Header Area
	header_size = 1;													// header takes up 1 sector
	header_gap = 100;													// allow room for more headers
	
	/* Allocate Directory Area */
	if ((_IGT_(ms_total, 0x0063))
	|| ((ms_total  ==  0x0063) && (_IGT_(ls_total, ((fixed) 0xFF38)))))	// 1% of the disk would hold more than 65534 entries
		dir_size = ((fixed) 65534);										// so limit to 65534 entries
	else {
		_write_5(ls_total);												// calculate size of directory area
		_write_4(ms_total);
		_write_7(100);													// directory size is 1% of the total space available
		dir_size = _read_5;												// size of directory in 1024-byte blocks
	}
		
	_write_5(dir_size);													// calculate size of directory gap
	_write_7(500);														// assume 0.2% of the sectors in the directory area will be bad
	dir_gap = _read_5;													// size of directory gap in 1024-byte sectors
	
	/* Allocate Data Area */
	msw = 0;
	lsw = header_size + header_gap + dir_gap;							// calculate size taken up by header and directory area
	lsw = lsw + dir_size;
	if (_ILT_(lsw, dir_size)) msw = msw + 1;
		
	ms_data = ms_total - msw;											// calculate size of data area
	if (_ILT_(ls_total, lsw)) ms_data = ms_data - 1;
	ls_data = ls_total - lsw;											// subtract size of header and directory areas
	
	_write_5(ls_data);													// calculate size of data area gap
	_write_4(ms_data);
	_write_7(1000);														// assume .1% of the data sectors will be bad
	data_gap = _read_5;													// size of data area gap
	
	if (_ILT_(ls_data, data_gap)) ms_data = ms_data - 1;				// subtract data gap from data area
	ls_data = ls_data - data_gap;
	
	for (i = 0; i < h_rec_length; i++) {								// clear the buffer
		pbuf(i,0);
	}
		
	/* Set up buffer with header information */
	
	pbuf(h_magic, magicnumber);											// magic number
	
	for (i = 0; i < h_name_max; i++) {									// volume name
		pbuf(h_name + i,headername[i]);									// store name characters
	}
		
	pbuf(h_date,date);													// store date
	pbuf(h_time,time);													// store time
	
	msw = 0; lsw = 0;													// start at beginning of disk
	
	lsw = lsw + (header_size + header_gap);								// calculate start of directory area
	pbuf(h_ls_dirstart,shl(lsw,1));										// 512-byte sector start of directory
	pbuf(h_ms_dirstart,shr(lsw,15));
	
	pbuf(h_ls_dirlen,shl(dir_size,1));									// 512-byte sector length of directory
	pbuf(h_ms_dirlen,shr(dir_size,15));
	
	lsw = lsw + dir_size;												// calculate start of data area
	if (_ILT_(lsw, dir_size)) msw = msw + 1;							// add on size of directory
	lsw = lsw + dir_gap;
	if (_ILT_(lsw, dir_gap)) msw = msw + 1;								// add on size of directory gap
		
	pbuf(h_ls_datastart,shl(lsw,1));									// 512-byte start of data area
	pbuf(h_ms_datastart,shl(msw,1) | shr(lsw,15));
	
	pbuf(h_ls_datalen,shl(ls_data,1));									// 512-byte length of data area
	pbuf(h_ms_datalen,shl(ms_data,1) | shr(ls_data,15));				// the no. sectors left is the length of the data area
	
	pbuf(h_serial,serial);												// store serial number
	
	j = caption[0];														// get length of caption
	if (_IGT_(j, shl(h_caption_max,1))) j = shl(h_caption_max,1);		// limit size of caption
		
	pbuf(h_caption,j);													// store length of caption
	for (_upper0 = shr(j + 1,1), i = 1; i <= _upper0; i++) {			// volume caption
		pbuf(h_caption + i,caption[i]);									// store caption characters
	}
		
	s_sensekey = AbleOptLib_transferoptical(s_extendedwrite,0,0,2,level);			// write out the buffer
	return (s_sensekey == s_good);										// format complete
}
	
/* $subtitle Replace File */

boolean			optical_replace(										// replace on optical
	fixed	name[], 													// name of file
	fixed	type, 														// type of file
	fixed	ms_len,
	fixed	ls_len, 													// size of file in sectors
	fixed	wordlen, 													// size of file in words (mod 64K)
	fixed	header[], 													// sound file header
	fixed	entry, 														// previous directory entry
	fixed	level)	_recursive	_swap									// level to replace on
{
	fixed			entryname[e_name_max];								// entry name
	fixed			buf[stat_rec_length];								// statistics array
	fixed			databuf[2];											// buffer for data information
	fixed			i;
	
	c_status = e_none;													// assume no error
	s_sensekey = s_good;
	
	if (! (valid_filename(name) & 1)) {									// not a valid file name
		c_status = e_treename;
		return (false);													// die if bad name
	}
	cleanname(name,entryname,e_name_max);								// clean and fill name
	
	if (! (AbleOptLib_readheader(level) & 1)) return (false);			// header could not be read
		
	if (entry != -1) {													// there is a previous entry
		s_sensekey = AbleOptLib_readentry(entry,level);					// get previous entry
		if (s_sensekey != s_good) return (false);						// an error occurred
			
		for (i = 0; i < e_name_max; i++) {								// loop over words of name
			if (entryname[i] != o_cur_bufptr[e__name + i]) {			// names do not match
				c_status = e_no_file;									// file to replace was not found
				return (false);											// die
			}
		}
	}
		
	if (! (AbleOptLib_get_statistics(level,buf) & 1)) return (false);	// could not get statistics
		
	if (buf[stat_dir_free] == 0) {										// no more directory entries
		c_status = e_dir_full;											// no more room
		return (false);													// die
	}
		
	if ((_IGT_(ms_len, buf[stat_ms_free]))
	|| ((ms_len  ==  buf[stat_ms_free]) && (_IGT_(ls_len, buf[stat_ls_free])))) {	// not enough room
		c_status = e_storage;											// no more room
		return (false);													// die
	}
		
	for (i = 0; i < e_rec_length; i++) {								// clear the buffer
		pbuf(i,0);
	}
		
	/* Set up buffer with entry information */
	
	pbuf(e__type,e_dir_entry);											// this is a directory entry
	
	for (i = 0; i < e_name_max; i++) {									// entry name
		pbuf(e__name + i,entryname[i]);									// store name characters
	}
		
	getdatetime(databuf);												// get date and time from clock, if it's there
	pbuf(e_date,databuf[0]);											// store date
	pbuf(e_time,databuf[1]);											// store time
	
	pbuf(e_ls_secstart,buf[stat_ls_start]);
	pbuf(e_ms_secstart,buf[stat_ms_start]);								// store sector start
	
	pbuf(e_ls_seclen,ls_len);
	pbuf(e_ms_seclen,ms_len);											// store sector length
	
	pbuf(e_ls_wordlen,wordlen);
	pbuf(e_ms_wordlen,0);												// store word length
	
	pbuf(e_file_type,type);												// store file type
	
	pbuf(e_prev_entry,entry);											// store previous entry
	
	for (i = 0; i < e_header_max; i++) {								// store sound file header
		pbuf(e_header + i,header[i]);
	}
		
	s_sensekey = AbleOptLib_writeentry(buf[stat_dir_start],level);		// try to write this directory entry at first free entry
	if (s_sensekey != s_good) return (false);
		
	f_ms_sector = shl(level,8) | buf[stat_ms_start];					// set up file variables
	f_ls_sector = buf[stat_ls_start];
	f_ms_length = ms_len;
	f_ls_length = ls_len;
	f_words = wordlen;
	f_type = type;
	
	if (o_entrycount != -1)												// volume is mounted
		o_entrycount = o_entrycount + 1;								// another directory entry inserted
		
	return (true);														// directory entry now in place
}
	
/* $subtitle Delete File */

boolean			optical_delete(											// delete file
	fixed	name[], 													// name of file to delete
	fixed	entry, 														// latest directory entry for this file
	fixed	level)	_recursive	_swap									// level to delete file from
{
	fixed			entryname[e_name_max];								// entry name
	fixed			freeentry;											// next free entry in directory
	fixed			i;
	
	c_status = e_none;													// assume no error
	s_sensekey = s_good;
	
	if (! (AbleOptLib_readheader(level) & 1)) return (false);			// header could not be read
		
	s_sensekey = AbleOptLib_readentry(entry,level);						// get entry
	if (s_sensekey != s_good) return (false);							// an error occurred
		
	cleanname(name,entryname,e_name_max);								// clean and fill name
	for (i = 0; i < e_name_max; i++) {									// loop over words of name
		if (entryname[i] != o_cur_bufptr[e__name + i]) {				// names do not match
			c_status = e_no_file;										// file to delete was not found
			return (false);												// die
		}
	}
		
	freeentry = AbleOptLib_findentrycount(level);						// find no. entries in directory
	if (freeentry == -1) return (false);								// could not find free entry
	if (_IGE_(freeentry, (shl(o_ms_dirlen,15) | shr(o_ls_dirlen,1)))) {	// all directory entries are used
		c_status = e_dir_full;											// signal error
		return (false);													// die
	}
		
	for (i = 0; i < e_rec_length; i++) {								// clear the buffer
		pbuf(i,0);
	}
		
	/* Set up buffer with entry information */
	
	pbuf(e__type,e_delete_entry);										// this is a delete entry
	
	for (i = 0; i < e_name_max; i++) {									// entry name
		pbuf(e_oldname + i,entryname[i]);								// store name characters in old name field
	}
		
	pbuf(e_prev_entry,entry);											// store previous entry
	
	s_sensekey = AbleOptLib_writeentry(freeentry,level);				// try to write this directory entry
	if (s_sensekey != s_good) return (false);
		
	if (o_entrycount != -1)												// volume is mounted
		o_entrycount = o_entrycount + 1;								// another directory entry inserted
		
	return (true);														// directory entry now in place
}
	
/* $subtitle Update File */

boolean			optical_update(											// update file
	fixed	oldname[], 													// old name of file
	fixed	newname[], 													// new name of file
	fixed	header[], 													// new sound file header for file
	fixed	entry, 														// latest directory entry for file to update
	fixed	level)	_recursive	_swap									// level to update file on
{
	fixed			entryname[e_name_max];								// entry name
	fixed			freeentry;											// next free entry in directory
	fixed			i, c;
	
	c_status = e_none;													// assume no error
	s_sensekey = s_good;
	
	if (! (valid_filename(newname) & 1)) {								// not a valid file name
		c_status = e_treename;
		return (false);													// die if bad name
	}
		
	if (! (AbleOptLib_readheader(level) & 1)) return (false);			// header could not be read
		
	freeentry = AbleOptLib_findentrycount(level);						// find no. entries in directory
	if (freeentry == -1) return (false);								// could not find free entry
	if (_IGE_(freeentry, (shl(o_ms_dirlen,15) | shr(o_ls_dirlen,1)))) {	// all directory entries are used
		c_status = e_dir_full;											// signal error
		return (false);													// die
	}
		
	s_sensekey = AbleOptLib_readentry(entry,level);						// get entry
	if (s_sensekey != s_good) return (false);							// an error occurred
		
	cleanname(oldname,entryname,e_name_max);							// clean and fill name
	for (i = 0; i < e_name_max; i++) {									// loop over words of name
		if (entryname[i] != o_cur_bufptr[e__name + i]) {				// names do not match
			c_status = e_no_file;										// file to update was not found
			return (false);												// die
		}
	}
		
	/* Set up buffer with entry information */
	
	pbuf(e__type,e_rename_entry);										// this is a rename entry
	
	cleanname(newname,entryname,e_name_max);							// clean and fill name
	for (i = 0; i < e_name_max; i++) {									// entry name
		c = o_cur_bufptr[e__name + i];									// get word of current name
		pbuf(e_oldname + i,c);											// store old name
		pbuf(e__name + i,entryname[i]);									// store new name
	}
		
	pbuf(e_prev_entry,entry);											// store previous entry
	
	for (i = 0; i < e_header_max; i++) {								// store sound file header
		pbuf(e_header + i,header[i]);
	}
		
	s_sensekey = AbleOptLib_writeentry(freeentry,level);				// try to write this directory entry
	if (s_sensekey != s_good) return (false);
		
	if (o_entrycount != -1)												// volume is mounted
		o_entrycount = o_entrycount + 1;								// another directory entry inserted
		
	return (true);														// directory entry now in place
}
	
/* $subtitle Locate File */

boolean			optical_locate(											// find file
	fixed	name[], 													// name of file
	fixed	level)	_recursive	_swap									// level to search on
{
	fixed			entry;												// directory entry
	fixed			numentries;											// total no. entries in directory
	boolean			filefound;											// True if file found
	fixed			fileentry;											// file entry
	fixed			entryname[e_name_max];								// entry name
	fixed			i;													// word length of string
	
	c_status = e_none;													// assume no errors
	s_sensekey = s_good;
	
	cleanname(name,entryname,e_name_max);								// clean file name
	
	if (! (AbleOptLib_readheader(level) & 1)) return (false);			// try to read header
		
	numentries = shl(o_ms_dirlen,15) | shr(o_ls_dirlen,1);				// get total no. directory entries
	
	filefound = false;													// assume file not found
	s_sensekey = s_good;
	entry = 0;															// start at first entry
	while ((_ILT_(entry, numentries)) && (s_sensekey == s_good)) {		// loop over entries until an error occurs
		s_sensekey = AbleOptLib_readentry(entry,level);					// get entry
		
		if (s_sensekey == s_good) {										// got an entry
			if (o_cur_bufptr[e__type] == e_dir_entry) {					// this is a directory entry
				
				i = 0;													// start at first character
				while ((i < e_name_max) && (entryname[i] == o_cur_bufptr[e__name + i])) {	// loop while words are the same
					i = i + 1;
				}
					
				if (i == e_name_max) {									// same name
					filefound = true;									// file found
					fileentry = entry;									// save entry
				}
			}															// directory entry
		}																// entry found
			
		entry = entry + 1;												// go to next entry
	}																	// looping over directory entries
		
	if ((_ILT_(entry, numentries)) && (s_sensekey != s_blankcheck)) return (false);	// error reading directory
		
	if (filefound & 1) {												// matching file name was found
		s_sensekey = AbleOptLib_readentry(fileentry,level);				// read in entry
		if (s_sensekey != s_good) return (false);
			
		f_ms_sector = shl(level,8) | o_cur_bufptr[e_ms_secstart];		// set up globals
		f_ls_sector = o_cur_bufptr[e_ls_secstart];
		f_ms_length = o_cur_bufptr[e_ms_seclen  ];
		f_ls_length = o_cur_bufptr[e_ls_seclen  ];
		f_words     = o_cur_bufptr[e_ls_wordlen ];
		f_type      = o_cur_bufptr[e_file_type  ];
		
		return (true);													// file found
	}
		
	c_status = e_no_file;
	return (false);														// file not found
}
#endif
