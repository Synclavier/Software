/*	PPCLibraryX.h

	History:
		The OS 9 implementation of Synclavier PowerPC used the Mac's Program-to-Program Toolbox (a.k.a. "program linking",
		a.k.a. "Program to Progam Communication"). The PPC Toolbox was used to exchange both traditional 'syncnet' messages
		between applications and new 'termulator-syncnet' messages.
		
		PPC Toolbox allowed communication to applications on other computers.
		
		The OS X implementation is a limited re-implimentation of the OS 9 version. The Synclavier Interpreter shared memory contains
		a data area that can be used to feed packets into the RTP, and retreive packets from the RTP. The interpreter itself manages
		the flow to/from the RTP. The actual interpreter application can contain sections of code that can send messages too or from
		the RTP.
		
		This allows migration of other application (editview, midinet) into the actual Synclavier Interpreter application.
		
		While other applications could theorettically talk to Synclavier Interpreter using the shared memory, the PPCLibrary
		does not yet support this use.
*/

#ifndef PPCLibrary_h
#define PPCLibrary_h

#include "PPCLibraryKernel.h"

#ifdef __cplusplus
extern "C" {
#endif

// Completion callback hooks
extern	void		(*PPC_Library_read_hook [PPC_SESSION_NUM_TYPES])(PPC_SessionPtr);						// read  callback hooks
extern	void		(*PPC_Library_write_hook[PPC_SESSION_NUM_TYPES])(PPC_SessionPtr);						// write callback hooks

// External routines:
PPC_Status			PPC_InitializePPCSubsystem(struct SynclavierSharedStruct* it, int num_sessions);		// Basic initialization; pass shared memory area and no. of simult sessions supported
PPC_Status			PPC_PublishInform		  (const char *type_name, int  visible,   int should_inform);
PPC_SessionPtr		PPC_LinkToLocalApplication(const char* our_service, char *to_service, int our_type,	// Link to app on local machine if available
											   int his_type, int block_size);

PPC_Status			PPC_CloseOffTheWorld	(bool hardwareFailed);					// Call to clean up before program terminates
PPC_SessionPtr		PPC_BrowseForSynclav	(int our_type, int his_type);			// Browse for a Synclavier¨
void	 			PPC_GetBrowsedLocation(char *here);								// Get location name that was selected
void	 			PPC_GetSessionLocation(PPC_SessionPtr session, char *here);		// Get location name of session
int	 				PPC_RecentConnectionAvailability(void);							// See if reconnect available
PPC_SessionPtr	 	PPC_Reconnect           (int our_type, int his_type);			// Reconnect to prior session
void				PPC_AbortSession		(PPC_SessionPtr session);				// Abort an open session


PPC_SessionPtr		PPC_FindSessionToClose	(int type);								// Find a dead session to close
void 				PPC_CloseSession		(PPC_SessionPtr session);				// Close a session

PPC_SessionPtr		PPC_FindNewSession		(int type);								// Find a new session, if any
PPC_SessionPtr		PPC_AckNewSession		(PPC_SessionPtr session);				// Ack the new session event
								    		 
void*				PPC_RetrievePriorOutgoingPacket(PPC_SessionPtr session,			// Retrieve prior write buffer, if available
													int* in_use);
void				PPC_ReStampRetrievedPacket(PPC_SessionPtr session, int pktlen);// Re-stamp a retrieved packet

void				PPC_PostFilledPacketsFromTask	(PPC_SessionPtr session);		// Post a write

struct SynclavierSharedStruct* PPC_FetchSharedStruct ();

// Routines that process all active sessions
void				PPC_PostAllFilledPackets ( int type );							// Post filled packets for all active sessions

#ifdef __cplusplus
}
#endif

#endif