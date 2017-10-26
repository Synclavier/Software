/*	PPCLibraryKernel.h

	History:
		The OS 9 implementation of Synclavier PowerPC used the Mac's Program-to-Program Toolbox (a.k.a. "program linking",
		a.k.a. "Program to Progam Communication"). The PPC Toolbox was used to exchange both traditional 'syncnet' messages
		between applications and new 'termulator-syncnet' messages.
		
		PPC Toolbox allowed communication to applications on other computers.
		
		The OS X implementation will first use a shared memory scheem to model the PPC toolbox. A later implementation
		can use apple events or perhaps TCP/IP to communicate to other machines
		
		The routines in this module are availabled to be called from within the kernel.

	Extracts suitable for calling from kernel
*/

#ifndef PPCLibraryKernel_h
#define PPCLibraryKernel_h

// Global Defines
#define	PPC_SUGGESTED_BLOCKS_FOR_SYNCNET		 200								// suggested packet    buffers for syncnet    connection
#define	PPC_SUGGESTED_BLOCKS_FOR_TERMULATOR		  20								// suggested character buffers for termulator connection
#define	PPC_SUGGESTED_BLOCKS_FOR_INTERCHANGE	  10								// suggested scsi      buffers for scsi       connection

#define	PPC_SUGGESTED_SIZE_FOR_SYNCNET			 (sizeof(PACKET))					// suggested packet    buffer size for syncnet    connection
#define	PPC_SUGGESTED_SIZE_FOR_TERMULATOR		1024								// suggested character buffer size for termulator connection
#define	PPC_SUGGESTED_SIZE_FOR_INTERCHANGE		   0								// suggested scsi      buffer size for scsi       connection

#define	PPC_SUGGESTED_DATA_AREA_SIZE		   20480								// size in bytes of packet data area in session struct

#define	PPC_MAX_MAX_BLOCKS	200														// max blocks that can be in a session

#ifdef __cplusplus
extern "C" {
#endif

// Enums
typedef enum	PPC_Status
{
	GOOD_PPC_STATUS = 0,
	BAD_PPC_STATUS

} 	PPC_Status;

typedef	enum	PPC_SessionStateCodes
{
	PPC_SESSION_NO_STATE = 0,
	PPC_SESSION_INFORM_POSTED,
	PPC_SESSION_ACCEPT_POSTED,
	PPC_SESSION_NEW_SESSION,
	PPC_SESSION_ACTIVE_SESSION,
	PPC_SESSION_CLOSING_SESSION,
	PPC_SESSION_CHANGING,

}	PPC_SessionStateCodes;

typedef enum	PPC_SessionTypes													// indicates starting session type: e.g. syncnet or serial
{
	PPC_SESSION_SYNCNET,															// session is a syncnet client (e.g. editview, autoconform)
	PPC_SESSION_TERMULATOR,															// session is a termulator serial port emulator
	PPC_SESSION_RTP,																// session is a syncnet and/or termulator host
	PPC_SESSION_NUM_TYPES

} 	PPC_SessionTypes;

typedef enum	PPC_SessionLocales													// indicates starting session type: e.g. syncnet or serial
{
	PPC_SESSION_LOCAL,																// session is on this computer
	PPC_SESSION_REMOTE																// session is on a remote computer

} 	PPC_SessionLocales;


// Sessions are managed by an invisible struct
typedef struct PPC_Session* PPC_SessionPtr;

// Routines eligible for calling from a kernel context
void				PPC_SetCoreStruct		(struct SynclavierSharedStruct* it);	// Set pointer to shared memory (for interpreter core routines only)
int					PPC_CountIncomingPacketsAvailable	(PPC_SessionPtr session);	// Count number of received but not processed packets
void*				PPC_FindNextIncomingPacket(PPC_SessionPtr session, int* len);	// Get pointer to next incoming packet (and its length) to process
void				PPC_AckIncomingPacket				(PPC_SessionPtr session);	// Ack
int					PPC_CountOutgoingPacketsWaiting(PPC_SessionPtr session);		// Return how many packets have been filled but not sent yet...
void*				PPC_FindNextOutgoingPacketToFill(PPC_SessionPtr session);		// Find next write buffer, if any
int					PPC_GetBasicPacketSize      (PPC_SessionPtr session);			// Get size available for filling packet
void				PPC_StampNextOutgoingPacket	(PPC_SessionPtr sess, int pktlen);	// Seal & stamp last filled packet

void				PPC_SetSessionRefcon	(PPC_SessionPtr sess, int refcon);		// Set a user-available reference constant for a session
int					PPC_GetSessionRefcon	(PPC_SessionPtr sess);					// Get a user-available reference constant for a session
int					PPC_GetHisSessionIndex  (PPC_SessionPtr sess);					// Get index into remote's session_data (used to identify within the remote which session an answer packet should be delivered to)
int					PPC_GetIndexedRefcon    (int index);							// Get the refcon for a session via its index
int					PPC_GetHisSessionType   (PPC_SessionPtr sess);					// Get type of remote session
int					PPC_GetHisSessionLocale	(PPC_SessionPtr sess);					// Get local VS remote for this session

#ifdef __cplusplus
}
#endif

#endif