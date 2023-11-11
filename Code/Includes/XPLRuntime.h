/*	XPLRuntime.h																		*/

/*	Contents: Macros & run-time interface for XPL translations					*/

/*	Created:	08/11/96 - C. Jones												*/


#ifndef XPL_Runtime_h
#define XPL_Runtime_h

#include "XPL.h"
#include "SynclavierFileReference.h"
#include "SyncMutex.h"

// Routines provided by the XPL Runtime Environment

#if __LP64__
    typedef int                             xpl_file_ref_num;
    typedef int                             xpl_result;
    typedef SyncFSSpec*                     xpl_file_ref;
#else
    typedef short                           xpl_file_ref_num;
    typedef short                           xpl_result;
    typedef struct FSSpec*                  xpl_file_ref;
#endif

/*	External Declarations: */

/*	The following variables and functions are declared in xpl_run_time.c (or	*/
/*	directly in an MPW library in the ase of printf and exit).	These functions	*/
/*	provide the capabilities of the XPL Run Time Package						*/
/*	functions																	*/

extern	void			print        (const char __restrict *format, ...) __printflike(1, 2);       // Print output with %a support
extern	void			(*printChar) (char c);                                                      // Alternative character output
extern  void            (*XPLPrinter)(const char *format, va_list);                                 // Alternate intercept routine

extern	void			linput (array);

extern	void			XPLexport    (fixed mam, fixed mal, array where, fixed words);
extern	void			XPLimport    (fixed mam, fixed mal, array where, fixed words);
extern	void			XPLextset	  (fixed mam, fixed mal, fixed words, fixed value);

extern	void			blockset  (array where, fixed num, fixed val);
extern	void			blockmove (array source, array dest, fixed length);

extern	fixed			able_core     (fixed  );				/* accessor			*/
extern	fixed			host_core     (pointer);
extern	void			set_able_core (fixed,   fixed);
extern	void			set_host_core (pointer, fixed);
extern	void			_write_60 (fixed address);
extern	void			_write_61 (fixed address);
extern	void			_write_62 (fixed data   );
extern	void			_write_63 (fixed data   );
extern	fixed			_read_60  ();							/* return xmem		*/
extern	fixed			_read_61  ();							/* pointers			*/
extern  fixed			_read_62  ();
extern  fixed			_read_63  ();

extern	void			set_run_time_situation(int it);		/* set XPL_RUN_TIME_USE_BIG_MEMORY or XPL_RUN_TIME_USE_KERNEL_MEMORY */

extern	fixed			initialize_run_time_environment (ulong ext_mem_size_sectors);
extern	void			cleanup_run_time_environment();

extern	fixed			_allocate_able_heap (fixed n);

extern	void * 			open_able_file            (char *file_name);
extern	void *			open_able_file_for_output (char *file_name, int type, int creator);

extern  void            (*host_yielder)          ();        /* builds using async IO call this thread yielder   */
extern  ulong			host_milliseconds        ();		/* millisecond timer								*/
extern	void			run_host_environment     ();		/* run host											*/
extern	void			run_host_environment_250 ();		/* run host every 250 msecs							*/

extern	int				g_disallow_run_host_exit;			/* nonzero to disallow exit by run_host_environmen 	*/
extern	int				g_disallow_atexit;					/* disallow atexit setup							*/
extern  boolean			g_break_received;					/* set by sigint (or run_host...) if break desired	*/
extern	boolean			g_throw_on_disk_error;				/* set to throw on disk error from read/writedata	*/
extern	void			(*able_exit_callout)();				/* called during atexit								*/

extern	handle          get_big_memory  (int size);
extern	void			free_big_memory (handle it);


extern	void            configure_able_hard_drives (int w0_scsi_id, int w0_cyls, int w0_secs, 	/* add hard drive to config area 	*/
                                                    int w1_scsi_id, int w1_cyls, int w1_secs);

extern	void            readdata  (fixed ms_sector, fixed ls_sector, fixed *_bufptr, fixed dir_size);
extern	void            writedata (fixed ms_sector, fixed ls_sector, fixed *_bufptr, fixed dir_size);

extern	struct  scsi_device*	access_scsi_device (fixed readdata_code);
extern	struct  scsi_settings*	access_scsi_setting(fixed readdata_code);

extern	void					update_scsi_device_size(fixed readdata_code);


/* for extread, write:	info[0] = sector of external memory	*/
/*                      info[1] = word   of external memory	*/
/*                      info[2] = sectors to transfer		*/
/*                      info[3] = words   to transfer		*/

extern	void			extread     (fixed ms_sector, fixed ls_sector, fixed *info);
extern	void			extwrite    (fixed ms_sector, fixed ls_sector, fixed *info);
extern	void			polyread    (fixed ms_sector, fixed ls_sector, fixed *info, fixed page_maybe);
extern	void			polywrite   (fixed ms_sector, fixed ls_sector, fixed *info, fixed page_maybe);
extern	fixed			set_curdev  (fixed device);		/* set current device in configuration			*/
extern	fixed			disk_check  (fixed);			/* check floppy disk							*/

extern	ufixed                  find_device 		 ( fixed device);		// find config table entry for a legacy device
extern	xpl_file_ref_num        find_hfs_device      (ufixed device);		// look up Mac File Ref Num for an active added device
extern	struct scsi_device*		find_hfs_scsi_device (ufixed device);		// look up scsi_device object for an active added device
extern	xpl_file_ref			find_hfs_scsi_spec   (ufixed device);		// look up the FSSpec for an active added device

extern	void		XPL_enable_interrupts ();						// XPL enable  statement
extern	void		XPL_disable_interrupts();						// XPL disable statement

extern	ufixed		XPL_read (ufixed address);						// XPL read    statement
extern	void		XPL_write(ufixed address, fixed data);			// XPL write   statement

extern	fixed		XPL_abs  (fixed);								// XPL abs     operator


// Struct used to store mapping of Able SCSI board & ID to Mac SCSI bus & id

// This class associates 8 SCSI id's on up to 4 D24 boards to physical devices selected
// in the Interchange setup selection.  This allows software to reference Able devices
// by their W0: and W1: codes and access the selected device or image file

typedef struct	MAC_SCSI_Target						// Struct to identify specific Mac SCSI target
{  
    short					entry_avail;			// nonzero if this entry has been entered
    xpl_file_ref_num		ref_num;				// ref num if file is opened (image file only)
    int						use_d24;				// nonzero indicates use real hardware
    struct scsi_device*		device_manager;			// pointer to device manager struct
    struct scsi_settings*	device_setup;			// pointer to InterChangeª setup descriptor
        
}   MAC_SCSI_Target;

typedef struct	SCSI_bus_list						// Handly list of 8 targets on a bus
{  
    MAC_SCSI_Target			target_list[NUM_TARGETS_IN_TARGET_LIST];
    
}   SCSI_bus_list;

typedef struct	SCSI_board_list						// List of bus targets for 4 SCSI Boards
{  
    SCSI_bus_list			board_list[NUM_BOARDS_IN_BOARD_LIST];
    
}   SCSI_board_list;

extern	SCSI_board_list         XPL_scsi_code_map;
extern	struct scsi_device*     g_scsi_device_data_base;
extern	struct scsi_device*     g_indexed_device [8];
extern	struct scsi_settings*   g_indexed_setting[8];

// Setup up global scsi board list from interchange setttings:
extern	void					XPLRunTime_SetupSCSIMap		   (struct interchange_settings* interchangeSettings, int d24Avail, int t0Config, scsi_settings* t0Settings);
extern	void					XPLRunTime_ConfigureSCSIMap    (fixed floppy_available, fixed d66_available, int t0Config);
extern	void					XPLRunTime_UpdateHiMemConfig   ();
extern	MAC_SCSI_Target*		XPLRunTime_InterrogateDevice   (struct MAC_SCSI_Target *the_target);
extern	MAC_SCSI_Target*		XPLRunTime_ExamineDevice	   (int theBoard, int theScsiID);
extern	struct scsi_device*		XPLRunTime_LookUpDevicePointer (int theBoard, int theScsiID);
extern	struct scsi_settings*	XPLRunTime_LookUpDeviceSettings(int theBoard, int theScsiID);
extern	void                    XPLRunTime_RemoveDevice        (struct MAC_SCSI_Target *the_target);
extern	void 					XPLRunTime_CloseupSCSIMap	   ();

// Access Mac HFS image files
extern	ufixed					XPLRunTime_AssignHFSReaddataCode (xpl_file_ref_num  inFileRefNum, xpl_file_ref inFileSpec);
extern	void					XPLRunTime_FreeHFSReaddataCode   (ufixed inReadDataCode);
extern	void					XPLRunTime_LatchHFSReaddataCode  (ufixed inReadDataCode);
extern	ufixed					XPLRunTime_LookForFSSpecInUse    (const xpl_file_ref inFileSpec);

extern	xpl_result              (*XPLRunTime_FileOpener)		 (xpl_file_ref inFileSpec, char permission, xpl_file_ref_num* refNum);
extern	xpl_result              (*XPLRunTime_FileCloser)		 (xpl_file_ref_num refNum);

// Host file I/O with byte swap
extern xpl_result		XPLRunTime_FSRead (xpl_file_ref_num   refNum, int*  count, void*       buffPtr);
extern xpl_result		XPLRunTime_FSWrite(xpl_file_ref_num   refNum, int*  count, const void* buffPtr);

extern xpl_result		XPLRunTime_FSReadFork (xpl_file_ref_num forkRefNum, unsigned short positionMode, long long positionOffset, unsigned int requestCount, void*       buffer, unsigned int* actualCount);
extern xpl_result		XPLRunTime_FSWriteFork(xpl_file_ref_num forkRefNum, unsigned short positionMode, long long positionOffset, unsigned int requestCount, const void* buffer, unsigned int* actualCount);

// Class to provide a mutex-protected global byte-swizzle buffer
class SyncSwizzleBuffer {
public:
	
	// constructors / destructors
    SyncSwizzleBuffer ();
	~SyncSwizzleBuffer();
    
    ufixed*             Buffer()    {return buffer;   }
    int                 Size  ()    {return 1024*1024;}
    
private:
    SyncMutexWaiter  waiter;
    
    static SyncMutex mutex;
    static ufixed*   buffer;
};

// Class to provide a stack-based self-releasing buffer
class SyncStackBuffer {
public:
        
    // constructors / destructors
    SyncStackBuffer (int size);
    ~SyncStackBuffer();
    
    ufixed*          Buffer()    {return                  itsBuffer;}
    char*            Chars ()    {return (char*)          itsBuffer;}
    unsigned char*   Bytes ()    {return (unsigned char*) itsBuffer;}
    int              Size  ()    {return                  itsSize;  }

    void             Resize(int newSize);

private:
    ufixed* itsBuffer;
    int     itsSize;
};


#if __LP64__
    typedef SyncMutex       LFastMutexSemaphore;
    typedef SyncMutexWaiter StFastMutex;

    extern  SyncMutex       gXPLMutex;
#endif
#endif

