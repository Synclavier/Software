/*	NED Toolkit - Definitions of SYNCnet protocol answer records.
	
	Copyright © 1990-91 by New England Digital Corp.  All rights reserved.
	
	This file defines the packets and data types returned by the audio
	engine over SYNCnet.  It also includes a function get_answer() which
	returns a pointer to the data structures defined in this file.
	
	Modification History:
		10/04/90	KJC	Created this file
*/

#ifndef NED__ANSWERSTUCTS
#define NED__ANSWERSTUCTS

#include "c.h"
#include "convertstructs.h"
#include "messagetype.h"				/* Specific Packet Type Codes */

/* This function is passed a pointer to a packet and returns a pointer to the
  start of the answer data structure for that packet:
  
  void *get_answer (PACKET *p);
  
  It does not affect your ability to parse a packet with the normal message
  library get_ routines.  Note that get_msg does not need to (but can) be
  called if get_answer is called. */

#define get_answer(p)	(get_msg (p), get_struct (0))	/* get pointer to answer structure */


/********* Retrieval Types *********/

#pragma pack(push,2)

typedef struct {					/* status from audition data structure */
	int32	dir_id;					/* directory ID */
	int32	ds_id;					/* data structure ID */
	int32	length;					/* size in bytes of data structure */
	int16 	status;					/* status of audition */
} DS_Audition_Status;

typedef struct {					/* sequencer time parameters */
	int32	smpte_offset;			/* SMPTE offset */
	int32	footage_offset;			/* footage offset */
	int16	speed;					/* speed (1000: normal play speed 1.000) */
	int16	click_rate;				/* click rate (clicks/second) */
	int16	bpm;					/* beats per measure */
	int16	sample_rate;			/* DTD sample rate (500: 50.0 kHz) */

	#if __BIG_ENDIAN__
		int8	smpte_mode;			/* SMPTE mode */
		int8	footage_mode;		/* footage mode */
		int8	display_format;		/* display format */
		int8	fill;
	#endif

	#if __LITTLE_ENDIAN__
		int8	footage_mode;		/* footage mode */
		int8	smpte_mode;			/* SMPTE mode */
		int8	fill;
		int8	display_format;		/* display format */
	#endif
} Time_Parameters;

typedef struct {					/* directory information */
	int32	id;						/* directory or volume ID */
	int32	entries;				/* number of entries in directory */
	int32	revision;				/* revision of directory */
	int16 	status;					/* status of directory request */
} Directory_Info;

typedef struct {					/* directory entry */
	int32	id;						/* data structure ID */
	int32	length;					/* byte length of data structure */
	int32	revision;				/* revision of data structure */
	int16	type;					/* type of data structure */
	char	name [1];				/* name of data structure */
} Directory_Entry;

typedef struct {					/* directory entries */
	int32	id;						/* directory or volume ID */
	int32	starting_entry;			/* starting entry number in list */
	int32	entries;				/* number of entries in list */
	int16	name_length;			/* length of names */
	int16 	status;					/* status of directory read */

	Directory_Entry entry [1];		/* the directory entries themselves */
} Directory_Entries;

typedef struct {					/* status from open data structure */
	int32	dir_id;					/* directory ID */
	int32	ds_id;					/* data structure ID */
	int32	length;					/* size in bytes of data structure */
	int16	status;					/* status of open */
	int16	xfer_id;				/* transfer ID */
	int16	accesses;				/* accesses opened with */
	int16	type;					/* data structure type */
} DS_Open_Status;

typedef struct {					/* data structure transfer record */
	int16	id;						/* transfer ID */
	int16 	status;					/* status of read/unused for write */
	int32	address;				/* byte address this data belongs at in data structure */
	int8	data [1];				/* data structure contents */
} DS_Transfer;

typedef struct {					/* status from data structure write (NOT IMPLEMENTED YET?) */
	int16	id;						/* transfer ID */
	int16 	status;					/* status of write */
	int32	address;				/* byte address of data written */
	int32	length;					/* length of data written */
} DS_Write_Status;

typedef struct {					/* status from copy data structure */
	int32	dir_id;					/* directory ID */
	int32	ds_id;					/* data structure ID */
	int32	length;					/* size in bytes of data structure */
	int16 	status;					/* status of copy */
	int16	extra_status;			/* additional status information */
	int16	extra_error;			/* additional error code */
} DS_Copy_Status;

typedef struct {					/* status from activate data structure */
	int32	dir_id;					/* directory ID */
	int32	ds_id;					/* data structure ID */
	int32	length;					/* size in bytes of data structure */
	int16 	status;					/* status of activate */
} DS_Activate_Status;

typedef struct {					/* audio meter control message */
	uint32	inmap;					/* bitmap of input channels */
	uint32	outmap;					/* bitmap of output channels */
	int32	interval;				/* metering interval (msec)  (0 to disable) */
} Meter_Control;

typedef struct {					/* audio meter data */
	uint32	inmap;					/* bitmap of input channels */
	uint32	outmap;					/* bitmap of output channels */
	int16	level[24];				/* meter level (.01 dB) */
} Meter_Data;

typedef struct {					/* Describes a single mixer element */
	int16	element_type;			/* One of the above */
	int16	num_elements;			/* Number of this type of element */
} Mixer_Element;

typedef struct {					/* describes mixer configuration */
	int32	num_element_types;		/* Number of different types of elements in mixer */
	Mixer_Element element[1];		/* There are num_element_types of these */
} Mixer_Config;

#pragma pack(pop)

#endif	/* NED__ANSWERSTUCTS */
