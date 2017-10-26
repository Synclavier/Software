/* MC68K OS:

  This library provides basic functionality for an MC68K running at the end of a
  DAWN protocol line.  This includes packet handling to support the basic requirements
  of DAWN, an event handling mechanism, and a regular clock interrupt.
  
  This library also provides a main function that calls your initialization code
  (which must be called appl_init ()) and then enters an infinite loop that handles
  basic DAWN housekeeping and calls your main event code (which must be called
  appl_main ()).  appl_main () must return after every event.
  
  If you need to intercept high priority packets, set the variable appl_packet
  to point to your packet handler.  This is best done in appl_init ().  The
  packet handler is passed a pointer to the packet and returns TRUE if it
  handled the packet and FALSE if it did not.  If the packet handler returns
  TRUE, it must free the packet (by calling free_packet () or release_packet ())
  BEFORE returning.  All packets for which the packet handler returns FALSE
  are passed onto the spare-time code via a RECEIVED_PACKET event.  The
  following packets are already intercepted by this library: the current
  time, any type of event, and crash requests.
  
  If you need to perform some periodic processing, set the variable appl_timer
  to point to your periodic processing function.  This is best done in appl_init ().
*/

#ifndef NED__OS
#define NED__OS

#include "c.h"
#include "packetstructs.h"


/* clock rate */

#define	CLOCK			5				/* ms between timer interrupts */


/* types of events (message value shown in parens; slash used to divide MS/LS) */

#define	NULL_EVENT			0				/* no event */
#define	RECEIVED_PACKET		1				/* packet received event (ptr to packet) */
#define	BUTTON_PRESS		2				/* button press event (logical button) */
#define	BUTTON_RELEASE		3				/* button release event (logical button) */
#define	KNOB_MOTION			4				/* knob motion event (relative motion/logical knob) */
#define	FADER_MOTION		5				/* fader motion event (absolute position/logical fader) */
#define	SYNCLAVIER_PRESENT	10				/* Synclavier present (TRUE if it's back, FALSE if it's left) */
#define	PACKETS_LOST		11				/* Rx packets have been lost (number of packets lost) */
#define PROTOCOL_EVENT		12				/* protocol event message received (ptr to event SET) */


/* special timeout values for get_event () */

#define	NO_WAIT				0L				/* don't wait for an event to occur */
#define	WAIT_FOREVER		-1L				/* don't return until an event occurs */


/* key data structures */

typedef struct event {						/* event structure */
	int16 what;								/* type of event */
	int32 message;							/* data particular to the event */
	int32 when;								/* when it happened */
} EVENT;


/* globals */

extern int32 the_time;							/* time since bootload in milliseconds */
extern int   synclavier_watchdog;				/* need to hear from them once in a while */
extern void (*appl_timer) (void);				/* ptr to application timer function */
extern bool (*appl_packet) (PACKET *packet);	/* ptr to application packet handler (TRUE if packet's been handled & freed) */


/* functions */

#ifdef __cplusplus
extern "C" {
#endif

EVENT get_event (int32 timeout);				/* get next event */
void  post_event (int what, int32 message);		/* post an event */
void  exit (int status);						/* exit back to OS with status (0: good, 1: bad) */

#ifdef __cplusplus
}
#endif

#endif	/* NED__OS */
