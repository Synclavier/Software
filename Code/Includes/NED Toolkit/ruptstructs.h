/*	NED Toolkit - Packet "Interrupt" Mechanism
	
	Copyright © 1991 by New England Digital Corp.  All rights reserved.
	
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

#ifndef NED__RUPTSTRUCTS
#define NED__RUPTSTRUCTS

#include	"c.h"
#include	"packetstructs.h"
#include	"answerstructs.h"

#define RUPT_MAGIC	0x52555054						/* magic tag */

typedef void pktHandler(void *that, PACKET *pkt);	/* our client */

struct rupt;									/* pseudo-interrupt */

typedef struct rupt	*ruptlist;					/* chain of handlers */

typedef	struct	rupt
{
	long				magic;					/* magic cookie tag */
	ProcessSerialNumber pid;					/* id of process */
	ruptlist			link;					/* link in list of calls */
	void				*that;					/* object context */
	pktHandler			*call;					/* object method */
	long				glob;					/* global context */
	ruptlist			*svc;					/* service queue */
	union { Meter_Control meter; }	parm;		/* rupt parameters */
}	rupt;

#endif
