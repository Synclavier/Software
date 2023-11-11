// =================================================================================
//	AbleInterpreterCore.h
// =================================================================================

// Header file for core interpreter functions

#pragma once

#include "XPL.h"

#include "PCI-1KernelDefs.h"
#include "PCI-1SharedMemory.h"
#include "SynclavierPCILib.h"
#include "AbleInterpreterStructs.h"

// Struct for passing memory pointers to core interpreter
typedef	struct	core_memory_bases										// pass task-specific memory pointer
{
	fixed*	_able_memory_;												// context-specific (e.g. kernel context or user context) pointer to internal memory
	int		_able_memory_S;												// size in bytes
	fixed*	d60;														// context-specific (e.g. kernel context or user context) pointer to external memory
	int		d60S;														// size in bytes
	fixed*	mac_polymemory_base;										// context-specific (e.g. kernel context or user context) pointer to mac-simulated poly memory
	int		mac_polymemory_baseS;										// size in bytes
	fixed*	scsi_blank_memory;											// context-specific (e.g. kernel context or user context) pointer to temporary memory area for simulating blannk optical media
	int		scsi_blank_memoryS;											// size in bytes
	fixed*	scsi_pmem_memory;											// context-specific (e.g. kernel context or user context) pointer to real poly memory disk copy buffer
	int		scsi_pmem_memoryS;											// size in bytes
	
}	core_memory_bases;

// Struct for passing hardware address pointers to core interpreter
typedef	struct	core_memory_hardware_addresses							// pass task-specific address pointer
{
    // PCI hardware pointers for original PCI-1
	volatile unsigned int*   my_able_fifo;
	volatile unsigned int*   my_able_passthru_tlimreg;
    
    // PCI hardware pointers for BTB-1
    volatile unsigned int*   mmio_in;               // Kernel address    int  read of all data
    volatile unsigned char*  mmio_sync_rev_b_in;    // Kernel address    byte read of sync input and rev code
    
    volatile unsigned int*   mmio_out;              // Kernel address    int  write of all data
    volatile unsigned char*  mmio_ctrl_b_out;       // Kernel address    byte write of cable control signals (WCR, WADR, CREAD, CWRITE, enables)
	
}	core_memory_hardware_addresses;

// Constants for SCSI simulation
#define	BLANK_MEMORY_SIZE_BYTES	(1024*100)			/* 100 block chunksize in optform       */
#define	PMEM_MEMORY_SIZE_BYTES	(1024*256)			/* 256 k copy buffer for poly load		*/

#define	interp_set_scsi_id_no_info		0			// Not sepecified yet
#define	interp_set_scsi_id_poll_host	1			// About to poll host for scsi selection
#define	interp_set_scsi_id_access_dtd	2			// About to access DTD over SCSI
#define	interp_set_scsi_id_device		3			// About to access a configurable storage device

typedef enum
{
	SCSI_IS_IDLE             = 0x00,     		/* SCSI simulation is idle				*/
	SCSI_REQUESING_IDENTIFY  = 0x01,     		/* SCSI simulation is requesting ident	*/
	SCSI_REQUESING_COMMAND   = 0x02,     		/* SCSI simulation is requesting cmd	*/
	SCSI_STATUS_AVAILABLE	 = 0x03,			/* Command complete; status avail		*/
	SCSI_STATUS_CMD_CMPL	 = 0x04,			/* Status read; cmd cmpl msg avail		*/
	SCSI_DATAIN_AVAILABLE    = 0x05,			/* locally stored data is available		*/
	SCSI_DATAIN_ACKING		 = 0x06,			/* able has asserted ack				*/
	SCSI_DATAOUT_NEEDED		 = 0x07,			/* pre-fetch of data-out data needed	*/
	SCSI_DATAOUT_ACKING		 = 0x08				/* acking last byte of a block			*/

}   scsi_state;

typedef enum
{
	SCSI_NO_MEMORY            = 0x00,     		/* Memory pointer is not set			*/
	SCSI_INTERNAL_MEMORY      = 0x01,     		/* Internal memory selected				*/
	SCSI_EXTERNAL_MEMORY      = 0x02,     		/* External memory selected				*/
	SCSI_SIMULATED_PMEM		  = 0x03,			/* Poly memory (mac simulation)			*/
	SCSI_POLY_MEMORY	      = 0x04,			/* Poly memory selected	(real mem)		*/
	SCSI_SYNTHESIZE_BLANKS	  = 0x05			/* Blank pattern for optical media		*/

}   scsi_memory_id;

// Public variables
extern int			timer_need_deferred_time;			// True if time base advances to the point where deferred task processing is needed

// Public functions
extern void			prep_interpreter_core				(SynclavierSharedStruct&         sharedStruct,
                                                         core_memory_bases&              bases,
                                                         core_memory_hardware_addresses& addresses,
                                                         SynclavierMIDICallback          midiCallback,
                                                         SynclavierHostCallback          hostCallback,
                                                         void*                           midiCallbackArg);

extern void			unprep_interpreter_core				();
extern void         abort_interpreter_core              ();
extern void			initialize_timer_simuilation		();
extern void         check_timer_simulation              ();
extern void			update_tlim_settings				();
extern void			update_poly_buf_memory				(fixed * newPtr, int newSize);
extern void			update_blank_buf_memory				(fixed * newPtr, int newSize);
extern void			update_session_memory				(fixed * newPtr, int newSize, int whichSubMemory);
extern void			core_feed_poly_memory				(int num_words);
extern void			core_goose_me						();
extern	void		interpret_able_instructions			(int doTimeBase);
extern	void		fatal_error							(const char *message, int value);
extern	void		do_response_histogram				(char *which);

