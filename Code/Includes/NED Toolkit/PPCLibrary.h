/*	PPCLibrary.h

	History:
		The OS 9 implementation of Synclavier PowerPC used the Mac's Program-to-Program Toolbox (a.k.a. "program linking",
		a.k.a. "Program to Progam Communication"). The PPC Toolbox was used to exchange both traditional 'syncnet' messages
		between applications and new 'termulator-syncnet' messages.
		
		PPC Toolbox allowed communication to applications on other computers.
*/

#ifndef PPCLibrary_h
#define PPCLibrary_h

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
	PPC_SESSION_CLOSING_SESSION

}	PPC_SessionStateCodes;

typedef enum	PPC_SessionTypes													// indicates starting session type: e.g. syncnet or serial
{
	PPC_SESSION_SYNCNET,															// session is a syncnet client (e.g. editview, autoconform)
	PPC_SESSION_TERMULATOR,															// session is a termulator serial port emulator
	PPC_SESSION_RTP																	// session is a syncnet and/or termulator host

} 	PPC_SessionTypes;

typedef enum	PPC_SessionLocales													// indicates starting session type: e.g. syncnet or serial
{
	PPC_SESSION_LOCAL,																// session is on this computer
	PPC_SESSION_REMOTE																// session is on a remote computer

} 	PPC_SessionLocales;


// Sessions are managed by an invisible struct
typedef struct PPC_Session* PPC_SessionPtr;

// Events published - probably should be moved to shared memory
extern	long		PPC_Library_needs_task_service;									// set nonzero when PPC library needs task level service
extern	long		PPC_Library_read_was_completed;									// set nonzero when a read completes (e.g. we got a message)
extern	long		PPC_Library_write_has_been_filled;								// set nonzero when a write is filled

// Routines eligible for calling from a kernel context
void				PPC_SetCoreStruct		(struct SynclavierSharedStruct* it);	// Set pointer to shared memory (for interpreter core routines only)
long				PPC_CountIncomingPacketsAvailable	(PPC_SessionPtr session);	// Count number of received but not processed packets
void*				PPC_FindNextIncomingPacket(PPC_SessionPtr session, long* len);	// Get pointer to next incoming packet (and its length) to process
void				PPC_AckIncomingPacket				(PPC_SessionPtr session);	// Ack
long				PPC_CountOutgoingPacketsWaiting(PPC_SessionPtr session);		// Return how many packets have been filled but not sent yet...
void*				PPC_FindNextOutgoingPacketToFill(PPC_SessionPtr session);		// Find next write buffer, if any
long				PPC_GetBasicPacketSize      (PPC_SessionPtr session);			// Get size available for filling packet
void				PPC_StampNextOutgoingPacket	(PPC_SessionPtr sess, long pktlen);	// Seal & stamp last filled packet

void				PPC_SetSessionRefcon	(PPC_SessionPtr sess, long refcon);		// Set a user-available reference constant for a session
long				PPC_GetSessionRefcon	(PPC_SessionPtr sess);					// Get a user-available reference constant for a session
long				PPC_GetHisSessionIndex  (PPC_SessionPtr sess);					// Get remotes session index
long				PPC_GetIndexedRefcon    (long index);							// Get the refcon for a session via its index
long				PPC_GetHisSessionType   (PPC_SessionPtr sess);					// Get type of remote session
long				PPC_GetHisSessionLocale	(PPC_SessionPtr sess);					// Get local VS remote for this session

// Completion callback hooks
extern	void		(*PPC_Library_read_hook )(PPC_SessionPtr);						// read  callback hook
extern	void		(*PPC_Library_write_hook)(PPC_SessionPtr);						// write callback hook

// External routines:
PPC_Status			PPC_InitializePPCSubsystem(int num_sessions);					// Basic initialization; pass no. of simult sessions supported
void				PPC_SetSharedStruct		(struct SynclavierSharedStruct* it);	// Set pointer to shared memory (for user space main loop level only)
PPC_Status			PPC_PublishInform		(char *app_name, char *type_name,		// Call to publish our port availability using PPCInform
							         	 	 char *nbp_name, void (*syncio)(),
							         	 	 int  visible,   int should_inform);
void				PPC_SetSyncIO(void (*syncio)());								// Set the SynchronizeIO routine that the PPC library should use

PPC_Status			PPC_CloseOffTheWorld	(void);									// Call to clean up before program terminates
PPC_SessionPtr		PPC_BrowseForSynclav	(long our_type, long his_type);			// Browse for a Synclavier¨
void	 			PPC_GetBrowsedLocation(char *here);								// Get location name that was selected
void	 			PPC_GetSessionLocation(PPC_SessionPtr session, char *here);		// Get location name of session
int	 				PPC_RecentConnectionAvailability(void);							// See if reconnect available
PPC_SessionPtr	 	PPC_Reconnect           (long our_type, long his_type);			// Reconnect to prior session
void				PPC_AbortSession		(PPC_SessionPtr session);				// Abort an open session

PPC_SessionPtr		PPC_LinkToLocalApplication(char *which, long our_type,			// Link to app on local machine if available
											   long his_type);

PPC_SessionPtr		PPC_FindSessionToClose	(void);									// Find a dead session to close
void 				PPC_CloseSession		(PPC_SessionPtr session);				// Close a session

PPC_SessionPtr		PPC_FindNewSession		(void);									// Find a new session, if any
PPC_SessionPtr		PPC_AckNewSession		(PPC_SessionPtr session,				// Ack the new session event
								    		 long numBlocks, long blockSize);
								    		 
PPC_SessionPtr		PPC_FindSessionToPrime	(void);									// Find a session whose read que needs to be reprimed
void 				PPC_PrimeSession		(PPC_SessionPtr session);				// Prime the read que to recover from a full que

void*				PPC_RetrievePriorOutgoingPacket(PPC_SessionPtr session,			// Retrieve prior write buffer, if available
													long* in_use);
void				PPC_ReStampRetrievedPacket(PPC_SessionPtr session, long pktlen);// Re-stamp a retrieved packet

void				PPC_PostFilledPacketsFromTask	(PPC_SessionPtr session);		// Post a write

// Routines that process all active sessions
void				PPC_PostAllFilledPackets (void);								// Post filled packets for all active sessions
PPC_SessionPtr		PPC_FindSessionWithMail  (void);								// Find active session with mail

#ifdef __cplusplus
}
#endif

#endif