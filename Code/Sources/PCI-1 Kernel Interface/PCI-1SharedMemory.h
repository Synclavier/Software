// =================================================================================
//	PCI-1 Shared Memory.h
// =================================================================================

// OS X:
// The Shared Library structures are used to communicate between the (kext) driver and the various user land
// applications using a User Client mechanism.

#ifndef __PCI1SHAREDMEMORY_H__
#define __PCI1SHAREDMEMORY_H__

// Mac OS Includes
#include <libkern/OSTypes.h>

// Basic configuration
#define	PCI1_SHARED_STRUCT_VERSION		5		// Current rev of shared struct
#define PCI1_MAX_MIDI_PORTS				80		// Number of virtual MIDI ports

#define PCI1_MIDI_IN_BUF_SIZE			1024	// Size of incoming MIDI event buffer
#define PCI1_MIDI_OUT_BUF_SIZE			1024	// Size of outgoing MIDI event buffer
#define PCI1_MESSAGE_BUF_SIZE			128		// OS X - Message buf not (realy) used yet

#pragma pack(push,2)

// Handy struct for Midi Events

typedef struct PCI1MIDIEvent
{
	UInt16	type;								// Type codeing
	UInt16	cable;								// Cable number
	UInt8	byte1;								// MIDI data byte 1
	UInt8	byte2;								// MIDI data byte 2
	UInt8	byte3;								// MIDI data byte 3
	UInt8	filler;

} PCI1MIDIEvent;

// Handy struct for possible card-specific preference settings
typedef struct PCI1Settings
{
	SInt32					signature;
	SInt32					version;
    
	SInt32					NotUsingAnyPreferencesAtTheMoment;
	
} PCI1Settings;


// =================================================================================
//	PCI1SharedStruct
// =================================================================================

// OS X - This structure is shared between the Kernel Extension and user space applications.
// It is primarily used to exchange MIDI data between the kernel driver and the MIDI driver

typedef	struct PCI1SharedStruct
{
	UInt32					struct_version;									// Holds PCI1_SHARED_STRUCT_VERSION

	volatile SInt32         message_write_ptr;
	volatile SInt32         message_buf_size;
	volatile SInt32         not_used_devices_available;						// Publish number of PCI1 devices connected
	
	PCI1Settings			not_used_settings[1];							// Stored settings
		
	char					message_buf[PCI1_MESSAGE_BUF_SIZE];

	// Hardware info:
	SInt32					unit_number;									// Unit number - 0 for first, 1 for second, etc.
	SInt32					product_id;										// Bus Number/Board Number
	
	// State info:
	volatile SInt32         daemon_has_memory;								// True if daemon has grabbed memory pointers
	volatile SInt32         midi_has_memory;								// True if MIDI driver has grabbed memory pointers
	volatile SInt32		    interpreter_running;							// True if interpreter running; dont bother posting MIDI output to SynclavierX if it is not running
	volatile SInt32         close_desired;									// Set by kernel when close is desired - tells MIDI driver to bail
	
	// Activity info:
	volatile SInt32         midi_in_occured;
	volatile SInt32         midi_out_occured;
		
	// MIDI in/out
	volatile SInt32			midiInPending;									// True if message has been sent to wake up MIDI driver
	volatile SInt32			midiInRunning;									// True if MIDI driver is running at this instant
	volatile SInt32         midiInActive[PCI1_MAX_MIDI_PORTS];				// True if MIDI server is up and running and listening to this input port (index by cable #)

	volatile SInt32			midiInPostedPtr;								// Write pointer used to post data into midiInBuf
	
	volatile SInt32         midiOutPending;									// True midi driver is getting ready to call into kext driver
	volatile SInt32         midiOutRunning;									// True if kext driver is processing MIDI output at this moment
	volatile SInt32         midiOutPostedPtr;
	
	PCI1MIDIEvent			midiInBuf   [PCI1_MIDI_IN_BUF_SIZE ];			// Holds input    PCI1MIDIEvent midi events
	PCI1MIDIEvent			midiOutBuf  [PCI1_MIDI_OUT_BUF_SIZE];			// Holds outgoing PCI1MIDIEvent midi events

	UInt64					midiInStamp [PCI1_MIDI_IN_BUF_SIZE ];			// Holds time stamp of incoming midi event (going  into MIDI services)
	UInt64					midiOutStamp[PCI1_MIDI_IN_BUF_SIZE ];			// Holds time stamp of outgoing midi event (coming from MIDI services to the interpreter)
    
}	PCI1SharedStruct;

#pragma pack(pop)

// Define Accessors for OS X Control Panel and daemons
#define kPCI1UniqueIDKey	"SynclavierPCI1UniqueID"						// Dictionary key for GUID published by driver

// There are 7 basic memory areas that are shared between the kernel and user space. These basic areas are used to hold
// the emulated contents of the able's internal memory, external memory, etc. etc. etc. Additionally, a shared memory space
// is allocated for each emulated sync-net session (for example: editview to RTP, AutoConform to RTP, etc. etc. etc).
typedef enum PCI1UserClientMemoryDescriptorID {								// Enums for clientMemoryForType a.k.a. IOConnectMapMemory
	kPCI1SharedStructID,												
	kSynclavierSharedStructID,
	kAbleInternalMemoryID,
	kAbleExternalMemoryID,
	kAblePolySimMemoryID,
	kAbleBlankMemoryID,
	kAblePolyBufMemoryID,
	kSessionMemoryID

} PCI1UserClientMemoryDescriptorID;

typedef enum PCI1UserClientMethodID {										// Enums for User Client method table
	kPCI1SetParam,
	kPCI1SetUserClientParam

} PCI1UserClientMethodID;

typedef enum PCI1UserClientAsyncMethodID {									// Enums for User Client async method table
	kPCI1SetAsyncRef,														// Set async callback ref for MIDI driver      (used to wake up MIDI driver when MIDI messages are posted for the MIDI driver)
	kPCI1SetXAsyncRef,														// Set async callback ref for SynclavierX MIDI (used to wake up SynclavierX when MIDI messages are posted for SynclavierX    )
	kPCI1SetUserClientMachPortRef,											// Tell user client what port to wake us up with for asynchronous callbacks
	kPCI1SetSessionReadAsyncRef,											// Set async callback to wake up ap when SyncNet message is posted for the application
    kPCI1SetOutputAvailableAsyncRef                                         // Set async callback to wake up ap when interpreter has created some character output
    
} PCI1UserClientAsyncMethodID;

typedef enum PCI1UserClientServiceTypes {									// Enums for UserClient service types.
	kPCI1ServiceMIDI,														// MIDI Driver - only 1 allowed
	KPCI1ServiceTypeSynclavier,												// Interpreter or test program - only 1 allowed
	KPCI1ServiceTestProgram,												// BTB1 Test Program - only 1 allowed.
	kPCI1ServiceGeneric														// Any other type - InterChangeX - multiple allowed
	
} PCI1UserClientServiceTypes;												// Note: Only one of each type allowed in.

#endif
