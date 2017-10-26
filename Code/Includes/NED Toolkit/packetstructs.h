/* NED Toolkit - DAWN Protocol Packet Interface Definitions

   Copyright © 1990-1992 by New England Digital Corporation

   By:  Karim J. Chichakly on 24 May 1989
*/

#ifndef NED__PACKETSTRUCTS
#define NED__PACKETSTRUCTS

#ifndef NED__C
#include "c.h"
#endif

#define	SYNCLAVIER_TIMEOUT	(3000/CLOCK)		/* time to wait to pronounce Synclavier dead 	*/
#define	central_node()		0x00				/* node ID of central processor 				*/
#define	remote_node()		0x10				/* node ID of ourselves							*/

/* pkt_send & send_packet return codes; if code > 0 packet has not been freed 					*/

#define PKT_NO_MEMORY		-1					/* not enough memory to queue up packet 		*/
#define PKT_SENT			0					/* packet was sent 								*/
#define PKT_TOO_LONG		1					/* packet is too long 							*/
#define PKT_BAD_TYPE		2					/* packet has invalid type 						*/
#define PKT_NO_HARDWARE		3					/* no hardware exists to send packets 			*/
#define PKT_NOT_LOADED		4					/* the hardware hasn't been properly downloaded */
#define PKT_NOT_CONNECTED	5					/* no one is connected to us 					*/
#define PKT_CHANNEL_FULL	6					/* channel is full; try again later (internal) 	*/

/* key data structures */

#define PACKET_HEADER			(2*sizeof (int16))	/* length of packet header (length & node) 		*/
#define PACKET_MIN				(PACKET_HEADER + 2*sizeof (int8))	/* smallest possible packet (can't use sizeof() due to alignment) */ 
#define PACKET_DATA_MAX			68					/* maximum length of packet data (extra four bytes for network routing) */
#define PACKET_MAX				(PACKET_MIN + PACKET_DATA_MAX)	/* largest possible packet 			*/
#define PACKET_SWIZZLE_I32(a)	(( int32) ((((uint32)(a))<<16)|(((uint32)(a))>>16)))
#define PACKET_SWIZZLE_U32(a)	((uint32) ((((uint32)(a))<<16)|(((uint32)(a))>>16)))

#pragma pack(push,2)

#if __BIG_ENDIAN__
	typedef struct packet {							/* packet 										*/
		uint16	length;								/* byte length 									*/
		uint16	node;								/* source/dest node 							*/
		uint8	type;								/* type of packet 								*/
		uint8	subtype;							/* subtype of packet 							*/
		uint16	wdata [PACKET_DATA_MAX/2];			/* variable length data area 					*/
	} PACKET;
#endif

#if __LITTLE_ENDIAN__
	typedef struct packet {							/* packet 										*/
		uint16	length;								/* byte length 									*/
		uint16	node;								/* source/dest node 							*/
		uint8	subtype;							/* subtype of packet 							*/
		uint8	type;								/* type of packet 								*/
		uint16	wdata [PACKET_DATA_MAX/2];			/* variable length data area 					*/
	} PACKET;
#endif

typedef struct cur_time {						/* current time 								*/
	int32 sequencer;							/* sequencer time 								*/
	int32 disk;									/* disk time 									*/
	int32 velocity;								/* last sequencer velocity 						*/
} CUR_TIME;

#pragma pack(pop)

#endif				/* NED__PACKETSTRUCTS */
