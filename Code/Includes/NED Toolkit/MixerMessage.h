/*	NED Toolkit - Definitions of Mixer protocol message structures.
	
	Copyright © 1991-1992 by New England Digital Corp.  All rights reserved.
	
	This file defines message fields for the Mixer.
	It should be shared by Mac & Baker code
	
	REVISION HISTORY
	----------------
	23-OCT-1991		Michael Geilich
			Added support for parametric eq.
	10-SEP-1991		Michael Geilich
			Added stuff for DSP23 crossbar routing.
	01-AUG-1991		Michael Geilich
			Created this module.
*/

#ifndef NED__MIXERMESSAGE
#define NED__MIXERMESSAGE

#ifndef NED__C
#include "c.h"
#endif

/* These structs are exchanged between the Macintosh Mixer screen,
 * the Baker Mixer task, and the Baker MIDI task.
 * the actual size is used instead of the enum because these are passed across machines
 */

#pragma pack(push,2)

typedef struct {
	#if __BIG_ENDIAN__
		int8 intype, innum, outtype, outnum;		/* mixer element coordinates */
	#endif

	#if __LITTLE_ENDIAN__
		int8 innum, intype, outnum, outtype;		/* mixer element coordinates */
	#endif

	int16 dial;										/* type of dial */
	int32 setting;									/* dial setting */
	}	Mixer_Dial_Set;

typedef struct {
	#if __BIG_ENDIAN__
		int8 intype, innum, outtype, outnum;		/* mixer element coordinates */
	#endif

	#if __LITTLE_ENDIAN__
		int8 innum, intype, outnum, outtype;		/* mixer element coordinates */
	#endif

	int16 swtch;									/* type of switch */
	int16 setting;									/* switch setting */
	}	Mixer_Switch_Set;

typedef struct {									/* Describes a single mixer element */
	#if __BIG_ENDIAN__
		int8	type;								/* One of the above */
		int8	num;								/* Number of this type of element */
	#endif

	#if __LITTLE_ENDIAN__
		int8	num;								/* Number of this type of element */
		int8	type;								/* One of the above */
	#endif

	} Mixer_Configuration_Element;
	
typedef struct {								/* crossbar element coordinates */
	int16 intype, innum, outtype, outnum;		/* crossbar types */
	int16 setting;								/* switch setting (0=off, 1=on) */
	} Xbar_Routing_Set;

#pragma pack(pop)

#define MIX_UNITY_COEFF	0x200000				/* unity gain */

/* DSP Mixer & Audiola Element Types */
typedef enum {									/* mixer element types */
	MIXT_UNUSED		= -1,						/* unused input/output */
	MIXT_CHAN_IN	= 1,						/* channel input */
	MIXT_RETURN_IN	= 2,						/* return input */
	MIXT_MIX_OUT	= 3,						/* strip output */
	MIXT_SEND_OUT	= 4,						/* send output */
	MIXT_PARA_EQ	= 5,						/* parametric eq */
	MIXT_MATRIX_OUT	= 6,						/* matrix mixer output */
	MIXT_AUDIOLA_OUT= 7,						/* Audiola output */
	MIXT_TYPES		= 7							/* number of types */
	}	Mixer_IO_Type;

typedef enum {									/* mixer element switches (mostly Boolean) */
	MIXS_PRE_POST	= 1,						/* pre/post fader */
	MIXS_SOLO		= 2,						/* solo button */
	MIXS_MUTE		= 3,						/* mute button */
	MIXS_ONOFF		= 4,						/* on/off button */
	MIXS_EQ_IN_OUT	= 5,						/* EQ in or out of "circuit" */
	MIXS_PEAK_SHELF	= 6,						/* EQ section set to "peak/notch" or "shelving" filter " */
	MIXS_MONO		= 7,						/* Make the monitor buss mono */
	MIXS_DUMP		= 8,						/* Dump master buss to monitor */
	MIXS_BASE_CHAN	= 9,						/* External tactile surface controller base Channel strip mapping */
	MIXS_TYPES		= 9							/* number of switches */
	}	Mixer_Switch_Type;
	
typedef enum {
	MIXD_LEVEL		= 1,						/* level control */
	MIXD_TRIM		= 2,						/* input trim control */
	MIXD_PAN		= 3,						/* pan control */
	MIXD_EQ_FREQ	= 4,						/* parametric eq frequency */
	MIXD_EQ_BW		= 5,						/* parametric eq bandwidth */
	MIXD_EQ_GAIN	= 6,						/* parametric eq gain */
	MIXD_TYPES		= 6							/* number of dials */
	}	Mixer_Dial_Type;


typedef enum {
	XBAR_UNUSED		= -1,						/* disconnected input/output */
	XBAR_LOGICAL	= 1,						/* qdio input/output */
	XBAR_DTD_CHAN	= 2,						/* between dtd and dsp2 strip input */
	XBAR_EQ_CHAN	= 3,						/* between strip input and eq block */
	XBAR_MIX_CHAN	= 4,						/* between eq block and dsp3 (mixer) */
	XBAR_EXPANSION	= 5,						/* expansion inputs */
	XBAR_RETURN		= 6,						/* return inputs */
	XBAR_SEND		= 7,						/* send outputs */
	XBAR_MONITOR	= 8,						/* monitor outputs */
	XBAR_MAIN		= 9,						/* main outputs */
	XBAR_TYPES		= 9
	} Xbar_Routing_Type;


/* DSP Option ASC bus device types used with P_ROUTING messages */
#define DEVICE_QDIO				0x00
#define DEVICE_DDIOC			0x01
#define DEVICE_SDIF2			0x02
#define DEVICE_SDIFM			0x03
#define DEVICE_DAC8				0x04
#define DEVICE_AES_EBU			0x05
#define DEVICE_UNKNOWN			-128	/* unrecognized ASC bus device found */



#endif	/* NED__MIXERMESSAGE */
