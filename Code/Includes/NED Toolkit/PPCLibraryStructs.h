/*	PPCLibraryStructs.h

	Interface for standardized PPC library

	This file defines structs used to simulate SyncNet in OS X
*/

#ifndef PPCLibraryStructs_h
#define PPCLibraryStructs_h

#ifndef	COMPILE_OSX_KERNEL
	#include <CoreServices/CoreServices.h>
#endif

#include "packetstructs.h"
#include "XPL.h"
#include "PPCLibraryKernel.h"

#pragma pack(push,2)

// Basic struct used to manage receiving a packet - contains a length and a pointer (e.g. offset into queDataArea) of some data
typedef	struct	PPC_ReadRequest
{
		int					pktlen;											// length of packet received
		int					dataOffset;										// offset into queDataArea

}	PPC_ReadRequest;

// Basic struct used to manage sending a packet - contains a length and a pointer (e.g. offset into queDataArea) of some data and some state information
typedef	struct	PPC_WriteRequest
{
		int					pktlen;											// length of packet to write
		int					dataOffset;										// offset into queDataArea
		int					dontsend;										// don't send even though qued for output - some other task is modifying the data

}	PPC_WriteRequest;

// Basic struct to control 1/2 of a session - that is one is required at each end of a session

#ifdef	COMPILE_OSX_KERNEL
	struct ProcessSerialNumber {											// we need psn type for kernel compilation, but not used therein
	  int				highLongOfPSN;
	  int				lowLongOfPSN;
	};
#endif

typedef	struct	PPC_Session
{
		volatile int		session_state;									// coded state info
		
		ProcessSerialNumber psn;											// psn of application we belong to
		int					informant_index;								// index into informant list of service we connected to
		int					is_originator;									// true if this side initiated the session
		
		int					refCon;											// refCon - used by interpreter to store its index into active_sessions
		int					numQueBlocks;									// number of read & write request slots available for  use (e.g. with data space allocated)
		int					queBlockStride;									// stride of each data space
		int					queBlockSize;									// size of each data space
				
		int					nextReadToRequest;								// next PPC_ReadRequest to issue read for
		int					nextReadToProcess;								// next PPC_ReadRequest that needs to be processed
		int					nextWriteToFill;								// next free PPC_WriteRequest to store data into
		int					nextWriteToPost;								// next PPC_WriteRequest to send
		
		int					readsReceived;									// count of reads received
		int					readsProcessed;									// count of reads processed
		int					writesFilled;									// count of writes filled by an interrupt routine
		int					writesPosted;									// count of writes posted to PPC toolkit
		int					writesCompleted;								// count of writes completed
		
		int					session_is_terminating;							// set upon termination desired
		int					write_has_failed;								// set if a write failed; e.g. stop writing!
		int					read_has_failed;								// set if read failed; e.g. close the session
		
		int					our_index;										// holds our index into session_data
		int					our_session_type;								// our session type
		int					his_index;										// holds his index into session_data
		int					his_session_type;								// his session type
		int					his_locale;										// indicates local vs remote

		int					just_tickled;									// true if we just woke up the process
		int					ogmsg_count;									// out-going message count
		int					icmsg_count;									// in-coming message count

		// Data storage
		PPC_ReadRequest		readList [PPC_MAX_MAX_BLOCKS];
		PPC_WriteRequest	writeList[PPC_MAX_MAX_BLOCKS];
		
		char				queDataArea[2*PPC_SUGGESTED_DATA_AREA_SIZE];	// Que data stored here
} PPC_Session;

// Struct to keep track of processes that are informing the world of their existence
typedef	struct	PPC_Informant
{
		ProcessSerialNumber	informant_process;
		char				informant_name[64];								// published name
		int					informant_visible;								// true if visible locally
		int					informant_network_visible;						// true if visible over network
		int					informant_index;								// holds our index back into informant_data
		volatile int		informant_state;								// coded state info
} PPC_Informant;

// Specific structs for sync-net style messages
// The struct so that it may be manipulated as 16-bit data.

typedef struct {													/* packet 								*/
	uint16	length;													/* byte length 							*/
	uint16	channel;												/* channel of board (e.g. 0 or 1)		*/
	
	#if __BIG_ENDIAN__
		uint8	type;												/* type of packet 						*/
		uint8	subtype;											/* subtype of packet 					*/
	#endif
	
	#if __LITTLE_ENDIAN__
		uint8	subtype;											/* subtype of packet 					*/
		uint8	type;												/* type of packet 						*/
	#endif
	
	uint16	pktdata[PACKET_DATA_MAX/2];								/* packet data							*/
} packet_struct;

#pragma pack(pop)

#endif