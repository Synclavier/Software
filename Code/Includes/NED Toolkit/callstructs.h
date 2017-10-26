/*	NED Toolkit - General Purpose Callout Mechanism
	
	Copyright © 1990 by New England Digital Corp.  All rights reserved.
	
	This library implements a general-purpose callout mechanism.  A callout
	can be created with callfor() by providing a Handler and a pointer to
	be passed as a parameter to that handler when the callout is made.
	
	Callouts are queued across applications (or across DRVR/application)
	with callnq() and are served by the application with callq().
	
	A call queue is global to an application. The process Manager is used to
	wakeup the application on queueing.
	
	Modification History:
		03/25/92	mac		added process manager calls
		10/31/90	MAC		rewired for call queueing
		09/13/90	MAC		added support for DRVR
		07/25/90	MAC		Created this file

*/

#ifndef NED__CALLSTRUCTS
#define NED__CALLSTRUCTS

#include	"handler.h"

#define MAGIC ((long)'Call')				/* mark for safety 				*/

typedef struct callout		callout;		/* a service call 				*/
typedef struct callqueue	callqueue;		/* application service queue 	*/

struct callout {
	long		magic;						/* magic number 				*/
	void		*refcon;					/* client object identifier 	*/
	Handler		*out;						/* client callout proc 			*/
	callqueue 	*queue;						/* que we ar linked on			*/
	callout		*link;						/* service queue thread 		*/
};						

struct callqueue {
	callout		*tail;						/* tail pointer 				*/
	int			count;						/* entry counter 				*/
	ProcessSerialNumber pid;				/* id of process 				*/
};

#endif	/* NED__CALLSTRUCTS */
