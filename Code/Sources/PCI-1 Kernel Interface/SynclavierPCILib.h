// =================================================================================
//	SynclavierPCILib.h
// =================================================================================

// Header file for SynclavierPCILib.c

#pragma once

#include "PCI-1KernelDefs.h"
#include "PCI-1SharedMemory.h"
#include "XPL.h"
#include "AbleInterpreterStructs.h"
#include "PPCLibraryStructs.h"

// Enumerations for Synclavier PowerPC message codes. These messages are typically sent from InterChange
// to Synclavier PowerPC when files are changed (so Synclavier PowerPC can update it's cache of sound files for example).

// Code is in lower 8 bits; option bits above that.
enum	SynclavierPowerPCMessageCode
{
	Synclavier_eject_media = 1,									// eject media. data is ejected_device_code (readdata code)
	Synclavier_sound_file_changed,								// sound file has moved around
	Synclavier_stop_audition,									// stop audition in process
	Synclavier_audition_sound_file,								// audition a sound file
	Synclavier_callup_sound_file,								// call up a sound file
	Synclavier_callup_sequence,									// call up a sequence file
    Synclavier_callup_timbre_file,								// call up a timbre file
    Synclavier_callup_workspace,								// call up a sequence workspace
    Synclavier_save_sequence,									// save a sequence file
	Synclavier_audition_started,								// audition has started
	Synclavier_audition_stopped,								// audition has stopped
	Synclavier_catalog_written,									// a catalog sector was written
	Synclavier_file_unsaved,									// a file was unsaved
	Synclavier_file_renamed,									// a file was renamed
	Synclavier_setup_changed,									// synclavier setup was changed
	Synclavier_registration                                     // registration code entered - data is bits
};

// Enumerations for Synclavier MIDI device codes
enum SynclavierPowerPCMIDIDeviceCode
{
	SYNCLAVIER_KEYBOARD = 0x0100,
	SYNCLAVIER_TRACKS	= 0x0200,
	SYNCLAVIER_MTC		= 0x0400,
	SYNCLAVIER_MIDICLK  = 0x0800,
	SYNCLAVIER_VPORT    = 0x1000
};

// Enumerations for Synclavier MIDI Cable codes - USB Midi Event identified source/destination by using these 'cable' codes
enum SynclavierPowerPCMIDIDeviceCableCode
{
	SYNCLAVIER_KEYBOARD_CABLE       = 0,                        // Cable 0: keyboard midi input and output
	SYNCLAVIER_MTC_CABLE            = 1,                        // Cable 1: MIDI Time Code
	SYNCLAVIER_MIDICLK_CABLE        = 2,                        // Cable 2: MIDI Clocking
	SYNCLAVIER_TRACKS_CABLE         = 3,                        // Cable 3 - 15: 13 cables to cover 200 sequencer tracks
	SYNCLAVIER_VPORTS_CABLE         = 16                        // Cable 16 - 79: 64 cables for 64 virtual ports
};

// Enumerations for Real Time Preferences
enum SynclavierRealTimePreferences
{
    SYNCLAVIER_PREF_POLL_DTD        = 0x0001,                   // Real time code should poll for DTD
    SYNCLAVIER_PREF_POLL_DD70MIDI   = 0x0002,                   // Real time code should poll D70 MIDI hardware
    SYNCLAVIER_PREF_POLL_D34GPI     = 0x0004,                   // Real time code should poll D34 GPI
    SYNCLAVIER_PREF_BOOT_MONITOR    = 0x0008,                   // Real time code should boot to MONITOR
    SYNCLAVIER_PREF_BOOT_LOGGER     = 0x0010,                   // Real time code should boot to LOGGER
    SYNCLAVIER_PREF_PEDAL2_MAX      = 0x0020,                   // Real time code should initialize pedal 2 to max
    SYNCLAVIER_PREF_CREATE_CLOCK    = 0x0040,                   // Real time code should generate ext 50 hz or beat clock
    SYNCLAVIER_PREF_CREATE_CACHE    = 0x0080,                   // Real time code should create B-Screen sound file cache
    SYNCLAVIER_PREF_NO_OPT_DTD      = 0x0100,                   // Disallow Optical-to-DTD Transfers (no data path)
    SYNCLAVIER_PREF_RECORD_SUSTAIN  = 0x0200,                   // Real time code should record sustain pedal as MIDI controller
    SYNCLAVIER_PREF_XPOS_MIDI       = 0x0400,                   // Transpose MIDI Output
    SYNCLAVIER_PREF_XPOS_MIDI_MASK  = 0x0C00,                   // Transpose MIDI Output
    SYNCLAVIER_PREF_POLL_TRACE      = 0x8000,                   // Real time code should trace

    // Available to kernel only (for now)
    SYNCLAVIER_PREF_USE_MTC         = 0x00010000,               // Real time code should trace
};

// Enumerations for Real Time Preferences
enum SynclavierTimbreInfoCodes
{
    timbre_info_code_bank_read        = 1,
    timbre_info_code_bank_written     = 2,
    timbre_info_code_timbre_read      = 3,
    timbre_info_code_timbre_written   = 4
};

// Offsets for raw synthesized RTP packets
#define Synclavier_Packet_Len      0  		/* Length of packet (including length word) */
#define Synclavier_Packet_Source   1  		/* Packet source node           */
#define Synclavier_Packet_Type     2  		/* Basic packet type code       */
#define Synclavier_Packet_Data     3  		/* Packet data begins here      */

// Struct versions
#define	SYNCLAVIER_POWERPC_STRUCT_VERSION	07
#define	SYNCLAVIER_POWERPC_STRUCT_NAME	   "07"
#define SYNCLAVIER_POWERPC_MESSAGE_Q_SIZE	8
#define SYNCLAVIER_POWERPC_MESSAGE_BUF_SIZE 20480
#define SYNCLAVIER_POWERPC_OMS_SYNC_SIZE 	64
#define SYNCLAVIER_POWERPC_TERMBUF_SIZE		8192
#define	SYNCLAVIER_POWERPC_DEBUGBUF_SIZE	1024
#define SYNCLAVIER_POWERPC_SNARF_BUF_SIZE   (32*512)    // SCSI snarf buffer; big enough for SFM sampling

#define	SYNCLAVIER_BOARD_TARGET_AVAIL		1
#define	SYNCLAVIER_BOARD_TARGET_USESD24		2
#define	SYNCLAVIER_BOARD_TARGET_NEEDSINQ	4
#define	SYNCLAVIER_BOARD_TARGET_ALTID       8

#define		SYNCLAVIER_POWERPC_NUM_MIDI_PATCHINGS			 80		// note - see identical declaration NUM_MIDI_PATCHINGS InterChange.h; see identical declaration MU_MAX_PATCHINGS in MIDIUtilities.h; see identical PCI1_MAX_MIDI_PORTS in PCI-1SharedMemory.h
#define		SYNCLAVIER_POWERPC_MIDI_PATCHING_STRING_SIZE	256		// note - see identical declaration MIDI_PATCHING_STRING_SIZE InterChange.h; see identical declaration MU_MAX_NAME_LENGTH in MIDIUtilities.h

// Defines
#define SYNCLAVIER_MIDI_PORTS			    202
#define SYNCLAVIER_VIRTUAL_PORTS			64
#define EVENT_SET_SIZE						16				// size of event set in 16-bit words
#define	PPC_SESSIONS_SUPPORTED				5				// MidiNet, AutoConform, EditView, one for browse, 1 for inform
#define	PPC_SESSIONS_RECORDS_AVAILABLE		10				// Number of session records in shared data area
#define PCI1SessionClientSize				5				// Number of shared sync-net sessions
#define SYNCLAVIER_VIRTUAL_PORT_CODE		0x0800			// Bit set for virtual code - 7 bits of virtual port; 4 bits of MIDI channel

#pragma pack(push,8)

typedef UInt64  AUInt64 __attribute__ ((aligned (8)));

// Handy message structs
typedef struct 	SynclavierCatUpdateStruct
{
	unsigned int	device_code;
	unsigned int	base_address;

} SynclavierCatUpdateStruct;

typedef struct 	SynclavierCallupStruct
{
    #if defined(COMPILE_OSX_KERNEL)
        union {
            void*       callup_url;
            AUInt64     callup_64;
        };
    #else
        union {
            CFURLRef    callup_url;
            AUInt64     callup_64;
        };
    #endif
    
    char            callup_name[200];
    
} SynclavierCallupStruct;

typedef union 	SynclavierPowerPCMessage
{
	short						ejected_device_code;
	int							registration_bits;
	char						file_name[256];
	SynclavierCatUpdateStruct	cat_update_info;
    SynclavierCallupStruct      callup_info;

} SynclavierPowerPCMessage;

// Handy struct idential to OMSPacket
typedef struct SynclavierMIDIPacket {
	unsigned char	flags;
	unsigned char	len;				/* including 6 bytes before data 	*/
	unsigned short	srcIORefNum;		/* refNum of source node 			*/	// Temporarily holds 8-bit time-stamp info
	unsigned short	appConnRefCon;		/* app ref con for the connection 	*/
	unsigned char	data[4];

} SynclavierMIDIPacket;

#define omsContMask				0x03
#define omsNoCont				0x00
#define omsStartCont			0x01
#define omsMidCont				0x03
#define omsEndCont				0x02
#define omsPktBeatTStamped		0x80
#define omsPktSMPTETStamped		0x40

// Handy struct for processing MIDI data
typedef struct SynclavierMidiChannel
{
	short						oms_cable;						// cable number with which to construct PCI1MIDIEvent
	short						oms_unused;						// unused place holder
	short						is_active;						// Nonzero means Core MIDI is running and may have recipients for MIDI data from this object, or SynclavierX is running and is forwarding MIDI to a real hardware port
	short						std_channel;					// Holds MIDI channel number we use for sending
	SynclavierMIDIPacket		oms_packet;

} SynclavierMidiChannel;

typedef	void (*SynclavierMIDICallback) (void* obj_ref, SynclavierMidiChannel& aChannel);
typedef	void (*SynclavierHostCallback) (void* obj_ref                                 );

// This struct is shared between kernel and user space.
typedef	struct SynclavierSharedStruct
{
	unsigned int				struct_version;					// Holds SYNCLAVIER_POWERPC_STRUCT_VERSION

	// Messages to InterChange are stored here
	volatile int				to_interchange_write_ptr;
	int							to_interchange_size;
	int							to_interchange_message[SYNCLAVIER_POWERPC_MESSAGE_Q_SIZE];
	SynclavierPowerPCMessage	to_interchange_data   [SYNCLAVIER_POWERPC_MESSAGE_Q_SIZE];
	
	// Messages to Synclavier PowerPC are stored her
	volatile int				to_synclavier_write_ptr;
	int							to_synclavier_size;
	int							to_synclavier_message [SYNCLAVIER_POWERPC_MESSAGE_Q_SIZE];
	SynclavierPowerPCMessage	to_synclavier_data    [SYNCLAVIER_POWERPC_MESSAGE_Q_SIZE];
	
	// Semaphores for media management
	volatile unsigned short		media_locked_by_rtp        [16]; 	// True (indexed by readdata code) if logical device is locked by SynclavierPowerPC
	volatile unsigned short  	media_locked_by_interchange[16];	// True (indexed by readdata code) if logical device is locked by InterChange
	volatile unsigned short  	media_used_by_rtp          [16];	// True (indexed by readdata code) if logical device is being used by SynclavierPowerPC
	volatile unsigned short  	media_used_by_interchange  [16];	// True (indexed by readdata code) if logical device is being used by InterChange
	
	// InterChange Message Variables
	volatile int				audition_sound_pending;				// State
	volatile int				callup_sound_pending;				//	variables
	volatile int				callup_sequence_pending;			//		set in response
	volatile int				callup_timbre_pending;				//			to
    volatile int                callup_workspace_pending;           //				InterChange messages
    volatile int                save_sequence_pending;
	volatile int				stop_pending;

	volatile char				audition_sound_pending_file  [256];	// Able file
	volatile char				callup_sound_pending_file    [256];	//	name for
	volatile char				callup_sequence_pending_file [256];	//		RTP functions triggered
    volatile char				callup_timbre_pending_file   [256];	//			from InterChange
    volatile char				callup_workspace_pending_file[256];	//
    volatile char				save_sequence_pending_file   [256];	//

	// General state variables
	volatile int				audition_in_process;
	volatile int				sequence_is_dirty;
	volatile int				new_midi_routing;
	volatile int				midi_started;
    volatile int                midi_driver_running;        // 2 == 64-bit MIDI driver running; 1 == 32-bit MIDI driver running; 0 == no MIDI driver running
	volatile int				sequence_path_changed;
	volatile int				host_run_desired;
	volatile int				monitor_crlf_count;
    volatile int                patch_screen_active;
    volatile int                patch_screen_partial;
    volatile int                patch_screen_frame;
    volatile int                timbre_info_changed;
    volatile int                timbre_info_code;           // What happened
    volatile int                timbre_info_arg;            // Details
    volatile char               current_timbre_name[64];    // C-string by the time it gets here; should always be space-filled 16 characters
    
	// Debug buffer
	volatile int				message_write_ptr;
	volatile int				message_buf_size;

	// MIDI in/out (this is midi input/output to SynclavierX for forwarding to real hardware MIDI ports; see the PCI1 shared struct for communication with the MIDI driver)
	volatile int				midiInPending;								// True if message has been sent to wake up SynclavierX MIDI Input
	volatile int				midiInRunning;								// True if SynclavierX MIDI processing is running at this instant
	volatile int                midiInActive[PCI1_MAX_MIDI_PORTS];			// True if SynclavierX MIDI is up and running and listening to this input port (index by cable #)

	volatile int				midiInPostedPtr;							// Write pointer used to post data into midiInBuf
	
	volatile int                midiOutPending;								// True SynclavierX is getting ready to call into kext driver
	volatile int                midiOutRunning;								// True if SynclavierX is processing MIDI output at this moment
	volatile int                midiOutPostedPtr;
	
	// OMS MIDI Sync and Bulk Input buffers
	volatile int				oms_midi_sync_write_ptr;			// next write pointer into oms_midi_sync_buf
	volatile int			    oms_midi_sync_size;					// size of oms_midi_sync_buf in bytes
	volatile unsigned char      oms_midi_sync_buf[SYNCLAVIER_POWERPC_OMS_SYNC_SIZE];
	volatile unsigned int       oms_midi_sync_any_bytes;			// internal memory space pointer (word pointer) to variable to set when incoming OMS sync traffic is posted
	volatile unsigned int       oms_midi_bulk_any_bytes;			// internal memory space pointer (word pointer) to variable to set when incoming OMS bulk traffic is posted
	volatile unsigned int       oms_midi_bulk_data;					// external memory space pointer (word pointer) to where to put an OMS bulk packet in memory
	volatile int                oms_midi_bulk_sector;
	volatile int                oms_midi_bulk_toggle;
    volatile int                oms_midi_lock;                      // mutex lock
    volatile AUInt64            oms_mtc_frame_boundary;             // instant of frame boundary, mach_absolute_time()
    volatile AUInt64            oms_mtc_frame_duration;             // duration of 1 frame precisely, mach_absolute_time() units
    volatile int                oms_mtc_tracking;                   // true if tracking
    volatile int                oms_mtc_frame;                      // frame number
    volatile int                oms_mtc_second;                     // second number
    volatile int                oms_mtc_minute;                     // minute number
    volatile int                oms_mtc_hour;                       // hour number
    volatile int                oms_mtc_code;                       // bit code
	
	// Variables published from interpreter initialization routine to core interpreter
	int							timer_interval;						// Processing interval for real time task, milliseconds. Normally 1 millisecond on modern dual processor platforms.
	int							timer_interval_for_deftask;			// Processing interval for deferred task, milliseconds
	int							timer_d03_counts_per_deftask;		// D03 interrupts to post per deferred task (e.g. timer_interval_for_deftask/5)
	uint32_ratio				metronome_calib_data;				// measured calibration data
	volatile uint32				MSecCount;							// millisecond ticker is published here by interpreter core
    AUInt64                     iokit_task_interpreting_time;       // start time of iokit task interpreting
	
	// Primary core interpreter state variables
    volatile int				signal_ui;                          // Set true when main loop level processing is needed before continuing. For example: read data from a mac file. (used within interpreter core only)
    volatile int				ui_processing_needed;				// Set true when main loop level processing is needed before continuing. For example: read data from a mac file. (published to UI thread)
	volatile int				main_loop_is_interpreting;			// Set true when we are interpreting at main loop level. Implies kernel driver is not present.
	volatile int				iokit_task_is_interpreting;			// Set true when an IOKit thread is interpreting in the kernel. We do the bulk of our interpreting at this level (if hardware is present) when not playing.
	volatile int				defrd_task_is_interpreting;			// Set true when we are interpreting in the kernel at slightly above IOKit thread priority. We do the bulk of our interpreting at this level (if hardware is present) when playing.
	volatile int				timer_task_is_interpreting;			// Set true when we are interpreting in the kernel at Real Time Task priority
	volatile int				suspend_timer_task_for_floppy;		// Set to suspend timer task while accessing floppy
	volatile int				prime_timer_task_after_floppy_access;

	volatile int				interpret_at_interrupt_prior;		// Set true when the RTP (vs other programs) is running. Perform interpretation every millisecond at real time task priority.
	volatile int				interpret_at_defrd_prior;			// Set true when RTP is playing (vs not playing). In this case run the IOKit interpretation at slightly higher than IOKit thread priority instead of IOkit thread priority.

	volatile int				do_main_loop_at_def_prior;			// Set true to perform RTP's main loop from the next deferred task invokation
	volatile int				expedite_iokit_interpreting;		// Set true when to_termulator buf fills but we are'nt out of stuff to do yet, so please hurry back for more interpreting!

	volatile int				do_main_loop_stuff;					// Sampled version of do_main_loop_at_def_prior sampled at start of IOKit task
    volatile int				rtp_is_playing;						// true: means RTP is playing (e.g. sequencer running)
    volatile int				rtp_is_running;						// true: means RTP is running and ready to load sound files for example; 2 means SFM is sampling to/from disk
	volatile int				newkey_is_pending;					// true: means RTP has sensed a newkey and DTask should launch immediately. Also set when character is typed to wake up DTask
	volatile int				delay_d50_input;					// counter to delay passing d50 characters to interpreter to allow paste from mac to work
	volatile int				termulator_just_pasted;				// true: termulator just pasted so special delays are needed

	volatile int				termination_pending;				// true: termination is pending
	volatile int				termination_fatal;					// true: fatal termination occured
	volatile int				launch_ppc_subsystem;				// true: launch PPC subsystem & try to find editview/autoconform
	volatile int				cleanup_ppc_subsystem;				// true: RTP is terminating so clean up PPC subsystem
	volatile int				warn_user_about_tags;				// true: warn of incompatible application
	volatile int				goose_d40_file_output;				// true: resume d40 file output at main loop level
	volatile char   			d40_file_output_char;				// saved up d40 file output character

	volatile int				debug_output_pending;
	volatile int				scsi_processing_needed;				// Various
	volatile int				scsi_get_bmem_needed;				//	flags
	volatile int				scsi_free_bmem_needed;				//		set	
	volatile int				scsi_get_pmem_needed;				//			by
	volatile int				scsi_free_pmem_needed;				//				core
	volatile int				terminal_flush_needed;				//					interpreter
	volatile int				scsi_allow_overlap;					//						to cause
	volatile int				scsi_do_overlap_scsi;				//							main loop
	volatile int				scsi_overlap_arg;					//								actions
	volatile int				scsi_processing_arg;
    volatile int				new_scsi_overlap;
	volatile int				host_msec_ticks;
	volatile int				main_msec_ticks;
	volatile int				changed_warning_needed;
	volatile int				new_quit_menu;
	volatile int				new_SCSIMap;
	volatile int				new_allow_media;
	volatile int				eject_id;
	volatile int				eject_target;
	volatile int				media_locked_arg;
	volatile int				halt_for_sleep;
	volatile int				real_time_prefs;					// Preference bits
	volatile int				suspend_test_for_sleep;				// Set by kernel to suspend board test when system sleeps
	volatile int				testing_poly_memory;				// Set by kernel when poly memory is being accessed
	volatile int				send_oms_time_stamps;				// if true, send OMS time stamp data
    volatile int				recall_mac_file;                    // if true, need to recall a mac file
    volatile int				recall_mac_file_type;               // of this type
    volatile char               recall_mac_file_name[256];          // with this name (handle)
    volatile int				recall_mac_posted;                  // set true when data base request is posted
    volatile int				recall_mac_got_file;                // 1 + shl (fileRev, 1) if we got the file
    
	// SCSI simulation shared variables
	volatile int				current_scsi_state;
	volatile int				current_target_valid;
	volatile int				current_target_board;
	volatile int				current_target_id;
	volatile byte				scsi_command_bytes[12];
	volatile int				scsi_command_count;
	volatile int				scsi_command_max;
	volatile byte				scsi_status_byte;
	volatile byte				scsi_message_byte;
	volatile int				scsi_read_count;
	volatile int				scsi_read_max;
	volatile int				scsi_write_count;
	volatile int				scsi_write_max;
	volatile int				scsi_memory_space;
	volatile uint32				scsi_memory_offset;							// word (!!) offset into simulated memory space
	volatile int				scsi_feeding_loader;
	volatile int				d40_capture_in_progress;

	volatile unsigned short		scsi_pmem_sector;
	volatile unsigned short		scsi_pmem_pageword;

	volatile int				d24_sim_data;								// simulated d24 data
	volatile int				d25_sim_data;								// simulated d24 data

	// Working variables for interpreter core also shared with user space
	volatile int				to_termulator_write_ptr;
	volatile int				to_termulator_read_ptr;
	volatile int				from_termulator_write_ptr;
	volatile int				from_termulator_read_ptr;
	
	volatile char				termulator_quit_subap[64];
	
	// PPC Library State Variables
	volatile int				PPC_Library_needs_task_service    [PPC_SESSION_NUM_TYPES];	// set nonzero when task-level service is needed
	volatile int				PPC_Library_write_has_been_filled [PPC_SESSION_NUM_TYPES];	// set when a packet record has been filled and needs to be posted
	volatile int				PPC_Library_read_was_completed    [PPC_SESSION_NUM_TYPES];	// set nonzero when a read completes (e.g. we got a message)
	volatile int				PPC_Library_someone_is_posting    [PPC_SESSION_NUM_TYPES];  // set when interrupt routines are posting write packets to avoid collision

	// Able register set passed to startup
	able_registers				regs;
	
	// Shared accessor struct
	PCI1AccessorStruct			accessor_struct;
    
    // Timing values for synchronous operations.
    // These are the user setting in units of 160 nsec
    int                         new_user_settings;                          // Set true to tell low level to compute new working values
    
    int                         scsi_d24_read_time_setting;
    int                         scsi_d24_write_time_setting;
    int                         scsi_d25_read_time_setting;
    int                         scsi_d25_write_time_setting;
    int                         scsi_d27_write_time_setting;
    int                         scsi_end_timing_setting;
    
    int                         poly_read_time_setting;
    int                         poly_write_time_setting;
    int                         poly_end_timing_setting;
    
    int                         generic_read_time_setting;
    int                         generic_setup_time_setting;
    int                         generic_write_time_setting;
    int                         generic_end_timing_setting;
    
    int                         measured_uptime_time;                       // Measured time of call to clock_get_uptime() nsec
    int                         measured_read_time;                         // Measured time of single read  of 16-bits from DIO card nsec
    int                         measured_write_time;                        // Measured tiem of single write of 16-bits to   DIO card nsec
    
	// D115 emulation shared data
	volatile	int				active_sessions[PPC_SESSIONS_SUPPORTED];	// list of pointers to active syncnet sessions (index into session_data) for the RTP side of each syncnet session
	volatile	fixed			active_events  [PPC_SESSIONS_SUPPORTED]		// events this syncnet session has enabled for
										       [EVENT_SET_SIZE        ];
	volatile	fixed			board0_events  [EVENT_SET_SIZE        ];	// events board 0/chan0 has enabled for

	volatile	int				num_active_sessions;						// number of active syncnet sessions

	volatile	int				active_termulators[PPC_SESSIONS_SUPPORTED];	// list of indexes to active termulator sessions
	volatile	int				to_remote_read_ptr[PPC_SESSIONS_SUPPORTED];	// read pointer for sending characters to them

	volatile	int				num_active_termulators;						// number of active termulator sessions
	volatile	int				num_local_termulators;						// number of active termulators on this computer

	volatile	int				syncnet_is_running;							// true once syncnet is up and running
	volatile	int				D115_relay_id;								// identifies most likely destination for a relay packet
	
	// Packets are built into these structs
	union	outgoing_packet {
		packet_struct	hdr;
		fixed			data[PACKET_MAX];
	}	outgoing_packet;

	union	incoming_packet {
		packet_struct	hdr;
		fixed			data[PACKET_MAX];
	}	incoming_packet;

	union	hereis_packet {
		packet_struct	hdr;
		fixed			data[PACKET_MAX];
	}	hereis_packet;

	// Debug (safe-printf) output
	char						debug_buf[SYNCLAVIER_POWERPC_DEBUGBUF_SIZE];
	volatile int				debug_buf_wr_ptr;
	
	// Put large stuff at the end to simplify addressing
	char						to_termulator_buf  [SYNCLAVIER_POWERPC_TERMBUF_SIZE    ];
	char						from_termulator_buf[SYNCLAVIER_POWERPC_TERMBUF_SIZE    ];
	char						message_buf		   [SYNCLAVIER_POWERPC_MESSAGE_BUF_SIZE];
	
	PCI1MIDIEvent				midiInBuf   [PCI1_MIDI_IN_BUF_SIZE ];		// Holds input    PCI1MIDIEvents
	PCI1MIDIEvent				midiOutBuf  [PCI1_MIDI_OUT_BUF_SIZE];		// Holds outgoing PCI1MIDIEvents

	AUInt64						midiInStamp [PCI1_MIDI_IN_BUF_SIZE ];		// Holds time stamp of incoming midi event
	AUInt64						midiOutStamp[PCI1_MIDI_IN_BUF_SIZE ];		// Holds time stamp of outgoing midi event (going out to Synclavier interpreter)

	volatile char				board_target_list [NUM_BOARDS_IN_BOARD_LIST][NUM_TARGETS_IN_TARGET_LIST];
	volatile int				target_block_size [NUM_BOARDS_IN_BOARD_LIST][NUM_TARGETS_IN_TARGET_LIST];
	volatile char				target_device_code[NUM_BOARDS_IN_BOARD_LIST][NUM_TARGETS_IN_TARGET_LIST];
	
	byte						snarfed_scsi_data[SYNCLAVIER_POWERPC_SNARF_BUF_SIZE];

	// MIDI accessors
	SynclavierMidiChannel		midi_channels[SYNCLAVIER_MIDI_PORTS];
	SynclavierMidiChannel		mtc_channel;
	SynclavierMidiChannel		mclk_channel;
	SynclavierMidiChannel		midi_vports[SYNCLAVIER_VIRTUAL_PORTS];
	
	// Virtual port routing. Not yet implemented. This table would provide the ability to route MIDI data
    // coming into V1 through V64 to be forwarded to a Synclavier track.
	UInt16						midi_vport_input_routing[SYNCLAVIER_VIRTUAL_PORTS];				// Holds which Synclavier track number a virtual port is routed to (SYNCLAVIER_VPORT + Synclavier track number 0 - 201); 0 means nowhere	

	// MIDI patching
	char						midi_to_synclavierx  [SYNCLAVIER_POWERPC_NUM_MIDI_PATCHINGS][SYNCLAVIER_POWERPC_MIDI_PATCHING_STRING_SIZE];	// 80 strings for midi input to SynclavierX
	char						midi_from_synclavierx[SYNCLAVIER_POWERPC_NUM_MIDI_PATCHINGS][SYNCLAVIER_POWERPC_MIDI_PATCHING_STRING_SIZE];	// 80 strings for output from SynclavierX

	// SyncNet Session Buffers (2 buffers required for each session - one for sender, one for receiver)
	PPC_Session					session_data  [PPC_SESSIONS_RECORDS_AVAILABLE];
	PPC_Informant				informant_data[PPC_SESSIONS_SUPPORTED        ];
	
}	SynclavierSharedStruct;

// Enumerations for PCI1SetParameterCode codes. These messages are sent from user space applications to the PCI1 kernel driver
enum PCI1SetParameterCode
{
	PCI1InitializePCI1,
	PCI1AllocateInternalMem,
	PCI1AllocateExternalMem,
	PCI1AllocatePolySimMem,
	PCI1AllocateScsiBlankMem,
	PCI1AllocatePolyBufMem,
	PCI1AllocateSessionMem,
	PCI1PrepCore,
	PCI1UnPrepCore,
	PCI1Calibrate,
	PCI1UpdateRatio,
	PCI1UpdateTlim,
	PCI1FeedPoly,
	PCI1GooseMe,
	PCI1MainFinished,
	PCI1Suspend,
	PCI1TestTimingRegister,
	PCI1XPLRead,
	PCI1XPLWrite,
	PCI1JamTlim,
	PCI1ReadSector,
	PCI1WriteSector,
	PCI1WritePoly,
	PCI1ReadPoly,
	PCI1SetMidi,
	PCI1GooseMidi,
	PCI1SetXMidi,
	PCI1GooseXMidi,
	PCI1SetMIDIChans,
	PCI1DoBroadcast,							// Broadcast situation change event - called only from UserClent kernel code
	PCI1SetMidiMSB,
	PCI1SetXMidiMSB,
    PCI1SetXMOutput,
    PCI1SetXMOutputMSB,
    PCI1SetWakeMainLoop,
    PCI1RemoveClient,
    PCI1TestBTB1

};

enum PCI1TestBTB1Code
{
    PCI1TestBTB1SetAddr,
    PCI1TestBTB1SetRead,
    PCI1TestBTB1GrabSync,
    PCI1TestBTB1GrabData,
    PCI1TestBTB1SetData,
    PCI1TestBTB1SetWrite,
    PCI1TestBTB1CleanUp,
    PCI1TestBTB1SetSync,
    PCI1TestBTB1ClearSync,
    PCI1TestBTB1YooHoo,
    PCI1TestBTB1ClearPDS,
    PCI1TestBTB1SetPDS
    
};

enum PCI1UserClientSetParameterCode
{
	PCI1UCSetInterpreterChange,					// Set async callback to invoke when interpreter situation changes
	PCI1UCBroadcastChange,						// Broadcast a situation change callback to all *other* user clients that are so registered
	PCI1UCReleaseMemoryRefs,					// Release memory mappings (internal and external memories only UFN)
	PCI1UCSetInterpreterChangeMSB,				// Set async callback MSB pointers for 64-bit aps
    PCI1UCRemoveUserClient                      // This user client is finished
};

// Kernel versions
#if defined(COMPILE_OSX_KERNEL)
	extern	void								SynclavierPCILIB_initialize				(struct PCI1KernelAccessorStruct* accessor, struct SynclavierSharedStruct* sharedStruct);
	extern	struct PCI1KernelAccessorStruct*	SynclavierPCILib_FetchPCIAccessor		( int required_version_id );
	extern	void								SynclavierPCILib_InitializePCI1			( int cable_load, int bus_load );
	extern	void								SynclavierPCILIB_Finalize				();
	extern	void								SynclavierPCILib_DoDebugOutput			( const char *m );
	
// User-land versions
#else
	typedef struct SynclavierPCILib_MIDIByteStash							// Handy stash of MIDI bytes used to communicate with parsers
	{
		int				numBytes;
		unsigned char 	byte1;
		unsigned char 	byte2;
		unsigned char	byte3;
		
	} SynclavierPCILib_MIDIByteStash;

	typedef void (*SynclavierPCILib_Callback         )( void* objRef, int which );
	typedef void (*SynclavierPCILib_TerminateCallback)( void* objRef, int which );
    typedef void (*SynclavierPCILib_WakeupCallback   )(                         );

	extern	void								SynclavierPCILib_WriteMIDIOutput		( struct SynclavierMIDIPacket* packet, void* inAccessor, int whichOutput, unsigned short* internal_memory, unsigned short* external_memory);
	extern	void								SynclavierPCILib_SetCallbacks			( SynclavierPCILib_Callback publishCallback, SynclavierPCILib_TerminateCallback terminationCallback, void* objRef );
    extern  void                                SynclavierPCILib_SetWakeupCallback      ( SynclavierPCILib_WakeupCallback wakeupCallback );
    extern  void                                SynclavierPCILib_PeekAtHardware         (int& numVirtuals, int& numPCI1s, int& numPCIes );
	extern	int									SynclavierPCILib_InitializePCI1			( const char *hostAp, PCI1UserClientServiceTypes serviceType, int cable_load, int bus_load );
	extern	void								SynclavierPCILIB_Finalize				( bool doUnprep );
	extern  class PCI1AudioDeviceManager*		SynclavierPCILib_FetchDeviceManager		( );
	extern	unsigned int						SynclavierPCILib_FetchServiceConnection ( );
	extern  struct PCI1SharedStruct*			SynclavierPCILib_FetchPCI1SharedStruct	( int required_version_id );
	extern	struct PCI1AccessorStruct*			SynclavierPCILib_FetchPCIAccessor		( int required_version_id );
	extern	fixed*								SynclavierPCILib_FetchInternalMemory	( int required_size_bytes );
	extern	fixed*								SynclavierPCILib_FetchExternalMemory	( int required_size_bytes );
	extern	fixed*								SynclavierPCILib_FetchPolySimMemory		( int required_size_bytes );
	extern	fixed*								SynclavierPCILib_FetchBlankMemory		( int required_size_bytes );
	extern	fixed*								SynclavierPCILib_FetchPolyBufMemory		( int required_size_bytes );
	extern	fixed*								SynclavierPCILib_FetchSessionMemory		( int required_size_bytes, int whichSubMemory );
    extern  void                                SynclavierPCILib_ReleaseInternalMemory  ();
    extern  void                                SynclavierPCILib_ReleaseExternalMemory  ();
	extern	void								SynclavierPCILib_BroadcastChange		();
	extern	void								SynclavierPCILib_PrepInterpreterCore	();
	extern	void								SynclavierPCILib_UnPrepInterpreterCore	();
	extern	uint32_ratio						SynclavierPCILib_PerformCalibration		();
	extern	void								SynclavierPCILib_ReleaseMemories		();
	extern	void								SynclavierPCILib_UpdateRatios			(uint32_ratio newRatio);
	extern	void								SynclavierPCILib_FeedPolyMemory			(int	num_words     );
	extern	void								SynclavierPCILib_GooseKernel			(int   do_interpretation);
	extern	void								SynclavierPCILib_MainFinished			();
	extern	void								SynclavierPCILib_Suspend				();
	extern	void								SynclavierPCILib_TestTimingRegister		();
	extern	int									SynclavierPCILib_XPL_Read				(int address);
	extern	void								SynclavierPCILib_XPL_Write				(int address, int data);
	extern	void								SynclavierPCILib_JamTLIMRegister		(int value);
	extern	void								SynclavierPCILib_ReadSector				();
	extern	void								SynclavierPCILib_WriteSector			();
	extern	void								SynclavierPCILib_WritePoly				(int	num_words     );
	extern	void								SynclavierPCILib_ReadPoly				(int	num_words     );
	extern	void								SynclavierPCILib_GooseMIDIOutput		();
	extern	void								SynclavierPCILib_RegisterMIDIClient		(void *, void *);	// Register callback info for MIDI driver
	extern	void								SynclavierPCILib_GooseXMIDIOutput		();
	extern	void								SynclavierPCILib_RegisterXMIDIClient	(void *, void *);	// Register callback info for SynclavierX
	extern	void								SynclavierPCILib_SetMIDIChans			();                 // Enable/Disable MIDI changes from interpreter
    extern	void								SynclavierPCILib_WakeMainLoop           ();                 // Wake main loop (after releasing D24 for example)
	extern	int									SynclavierPCILib_ParseUSBMIDIEvent		(struct PCI1MIDIEvent& inEvent, unsigned char* outData, bool* isSysex, bool* isSysexEnd);
	extern	int									SynclavierPCILib_ConstructUSBMidiEvent	(int cableNum, SynclavierPCILib_MIDIByteStash& inBytes, struct PCI1MIDIEvent& outEvent);
	extern	void								SynclavierPCILib_StuffMIDIMIDI			( SynclavierSharedStruct& sharedStruct,  PCI1SharedStruct& PCI_shared, int& readPtr, unsigned short* internalMemory, unsigned short* externalMemory);
	extern	void								SynclavierPCILib_StuffSyncXMIDI			( SynclavierSharedStruct& shared,  int& readPtr, unsigned short* internalMemory, unsigned short* externalMemory);
    extern	void								SynclavierPCILib_TestBTB1				(int which, int data);

    // IOConnectMethodScalarIScalarO calls:
    //	PCI1InitializePCI1,		  cable_load, bus_load					SynclavierPCILib_InitializePCI1
    //	PCI1AllocateInternalMem,  required_size_bytes					Allocate internal memory chunk
    //	PCI1AllocateExternalMem,  required_size_bytes					Allocate external memory chunk
    //	PCI1AllocatePolySimMem,   required_size_bytes					Allocate polysim  memory chunk
    //	PCI1AllocateScsiBlankMem, required_size_bytes					Allocate blank    memory chunk
    //	PCI1AllocatePolyBufMem,   required_size_bytes					Allocate polybuf  memory chunk
    //	PCI1AllocateSessionMem,   required_size_bytes, whichSubMemory	Allocate session  memory chunk
    //	PCI1PrepCore, 0													prep_interpreter_core
    //	PCI1UnPrepCore, 0												unprep_interpreter_core
    //	PCI1Calibrate, 0												TU_MeasureUPTimeUsingD03Timer or TU_MeasureUPTimeUsingD16Timer
    //	PCI1UpdateRatio, 0												TU_PublishNewRatios
    //	PCI1UpdateTlim, cable load, bus load							SynclavierPCILib_UpdateTLim     (int cable_load, int bus_load)
    //	PCI1FeedPoly, 0													core_feed_poly_memory			(int	num_words)
    //	PCI1GooseMe, 0													core_goose_me

    // Other routines that are sometimes available
    extern  int                                     sync_prefs_boot_to_monitor;
    extern  int                                     sync_prefs_boot_to_logger;
    extern  int                                     sync_prefs_grab_one_pref(const char* which);
    extern  void                                    sync_prefs_grab_real_time_prefs(SynclavierSharedStruct& sharedStruct);
    extern  void                                    sync_send_notification(CFStringRef aNotification);
#endif

// Implemented in Kernel in C; Implemented in user land via system call.
extern	struct SynclavierSharedStruct*			SynclavierPCILib_FetchSharedStruct		( int required_version_id  );
extern  bool                                    SynclavierPCILib_FetchFailed            ( int& required_version_id );

extern	unsigned int							SynclavierPCILib_FetchDevReadCode		( int which );
extern	unsigned int							SynclavierPCILib_FetchDevWriteCode		( int which );
extern	void									SynclavierPCILib_UpdateTLim				( int cable_load, int bus_load );

// Handy accessors to post messages
extern 	SynclavierPowerPCMessage*				SynclavierPCILib_GetNextInterChangeDataPointer();
extern	void									SynclavierPCILib_PostNextInterChangeMessage   (int message_code);
extern	SynclavierPowerPCMessage*				SynclavierPCILib_GetNextSynclavierDataPointer ();
extern	void									SynclavierPCILib_PostNextSynclavierMessage    (int message_code);

#pragma pack(pop)

