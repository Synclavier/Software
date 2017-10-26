/* NED Toolkit - Time Conversion Library

   Copyright © 1990 by New England Digital Corporation

   In general, only the millisecond-to-string and string-to-millisecond
   routines are needed.  Most of the other routines, as well as the
   structures they use (such as S_TIME), are internal routines that are
   useful only in very specific circumstances (such as scanning input).
   
   Note:	SMPTE offsets are in smpte bits (sbits), NOT milliseconds.
  		Footage offsets are in footage bits (fbits), NOT milliseconds.
		SPEED is the sequencer speed times 1000, so 1250 is a speed of 1.250.
		BPM is beats per measure.
		CLICK is the click rate in clicks/second.
		Sample RATE is the sample rate (in kHz) times ten, so 500 is 50.0 kHz.
*/


#ifndef NED__CONVERTSTRUCTS
#define NED__CONVERTSTRUCTS

#include "c.h"

#pragma pack(push,2)

typedef struct {					/* 64-bit number */
	volatile uint16 integer;
	volatile uint16 frac [3];
} LARGE_FLOAT;

#if __BIG_ENDIAN__
	typedef struct {					/* block floating point number */
		volatile uint32 integer;
		volatile uint16 frac;
	} FLOAT;
#endif

#if __LITTLE_ENDIAN__
	typedef struct {					/* block floating point number */
		volatile uint16 frac;
		volatile uint32 integer;
	} FLOAT;
#endif

#pragma pack(pop)

#define SBITS_PER_FRAME	80			/* each smpte frame divides into 80 parts */
#define FBITS_PER_FRAME	80			/* assume each frame divides into 80 parts */

typedef enum {						/* possible time formats */
	F_SEC = 0,					/* seconds */
	F_BEAT,						/* beats */
	F_MEAS,						/* measures and beats */
	F_SMPTE,						/* SMPTE */
	F_FEET,						/* feet and frames */
	F_MIN						/* minutes and seconds */
} T_FORMAT;

typedef enum {						/* format of time value */
	CF_TIME,						/* a point in time */
	CF_DUR						/* a duration */
} C_FORMAT;

typedef enum {						/* SMPTE mode */
	SM_DROP,						/* 30 fps drop frame */
	SM_NONDROP,					/* 30 fps non-drop frame */
	SM_25FPS,						/* 25 fps */
	SM_24FPS,						/* 24 fps */
	SM_PULLDOWN					/* 30 fps non-drop at 29.97 (drop frame) rate */
} S_MODE;

typedef enum {						/* film footage mode */
	FM_30FPS_35MM,					/* 30 fps, 35 mm, 16 fpf */
	FM_25FPS_35MM,					/* 25 fps, 35 mm, 16 fpf */
	FM_24FPS_35MM,					/* 24 fps, 35 mm, 16 fpf */
	FM_30FPS_16MM,					/* 30 fps, 16 mm, 40 fpf */
	FM_25FPS_16MM,					/* 25 fps, 16 mm, 40 fpf */
	FM_24FPS_16MM					/* 24 fps, 16 mm, 40 fpf */
} F_MODE;

typedef struct {					/* beats time	*/
	bool sign;
	int	beat;
	int	frac;
} B_TIME;

typedef struct {					/* SMPTE time */
	bool sign;
	int	hrs;				
	int	min;
	int	sec;
	int	fra;
	int	bit;
} S_TIME;

typedef struct {					/* FEET:FRAME time */
	bool sign;
	int	feet;				
	int	fra;
	int	bit;
} F_TIME;

typedef struct {					/* seconds time */
	bool sign;
	int	sec;
	int	msec;
} M_TIME;

#endif				/* NED__CONVERTSTRUCTS */
