/*	NED Toolkit - Packet "Interrupt" Mechanism
	
	Copyright � 1991 by New England Digital Corp.  All rights reserved.
	
	This interface describes the SYNCNet DRVR packet handler mechanism
	It provides a means for applications to receive data from packets as
	they arrive.
	
	The handlers are called out of the driver as the packet is
	received.  This means that no allocation can be done.  The data
	must be processed on the spot.  The a5 globals register is provided.
	
	The handler must be followed by type-dependent parameters
	
	This is a low-level feature.
	The expected use is for real-time traffic such as metering data.
	
	Modification History:
		01/15/91	MAC		Created this file

*/

#ifndef NED__TOOLRUPT
#define NED__TOOLRUPT

#include	"c.h"
#include	"packetstructs.h"
#include	"answerstructs.h"
#include	"ruptstructs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* public user interface
 * the service is identified by a packet type & some qualifiers
 * only one service to a client.
 * the pktHandler routine can, of course, be shared
 */

void rupt_open 	 (rupt *p, void *that, pktHandler *call);	/* initialize common part 	*/
void rupt_close  (rupt *p);									/* detach 					*/
bool rupt_for 	 (rupt *p, int type, int subtype);			/* install service 			*/
bool rupt_active (rupt *p);									/* is handler installed? 	*/
bool rupt_cancel (rupt *p);									/* cancel service 			*/

/* memory management */
rupt *rupt_new(long size);									/* allocate a rupt of size 	*/
void rupt_delete(rupt *p);									/* release such 			*/

#ifdef __cplusplus
}
#endif

#endif
