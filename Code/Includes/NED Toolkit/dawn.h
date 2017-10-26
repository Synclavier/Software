/*	NED Toolkit - SYNCnet Event Handling
	
	Copyright � 1990-91 by New England Digital Corp.  All rights reserved.
	
	This library handles getting events from DAWN.  The function
	get_dawn_event() sets the passed event to the waiting NED_EVENT
	and returns TRUE if there is a DAWN event available.  The
	function dawn_handle() handles the resulting event.
	
	Modification History:
		09/13/90	MAC		added DRVR support
		08/06/90	MAC		Created this file
*/

#ifndef NED__DAWN
#define NED__DAWN

#ifndef NED__C
#include "c.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

bool	get_dawn_event			(EventRecord *event);	// one for packet style interface (EditView, AutoConform

bool	syncnet_get_dawn_event	(EventRecord *event);	// one for syncnet style interface (TransferMation)
bool	syncnet_dawn_handle   	(EventRecord *event);	// provide protocol service

bool	dawn_alert(const unsigned char *error, const unsigned char *fix, bool cont, bool cncl);	/* error dialog */
bool	dawn_timeout(long start);			/* watch for timeout */

void	dawn_open (void);					/* initialize SYNCnet connection */
void	dawn_close(void);					/* close down SYNCnet */

/*	packets level interface */
void	startup_dawn (void);
void	shutdown_dawn(void);

void	dawn_task(void);					/* provide extra SYNCnet processing time */
void	dawn_tickle_host_for_answer(void);	/* try to prod answer quickly			 */
long	dawn_check_packet_backlog(void);	// return how many packets have been queued but not received yet

#ifdef __cplusplus		
}
#endif

#endif	/* NED__DAWN */
