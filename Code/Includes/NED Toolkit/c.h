/* NED Toolkit - C Language Enhancement Definitions

   Copyright © 1990-1992 by New England Digital Corporation

   By:  Karim J. Chichakly on 6/1/87
*/

#ifndef NED__C
#define NED__C

// Provide compatible flags for CodeWarrior
#if		(!defined(__BIG_ENDIAN__)&&!defined(__LITTLE_ENDIAN__))
	#define __BIG_ENDIAN__		1
	#define __LITTLE_ENDIAN__	0
#endif

// File includes C type definitions for standard integer types as well as macros.
// Originaly developed for NED Toolkit software framework

#if !defined(__GNUC__)
	#define not		!
	#define and		&&
	#define or		||
#endif

#ifndef FALSE
	#define	FALSE	0
#endif

#ifndef TRUE
	#define TRUE	!FALSE
#endif

#ifndef NULL
	#define NULL	0
#endif

#ifndef NIL
	#define NIL	0
#endif

#ifndef Nil
	#define Nil	0
#endif

#ifndef INT_TYPES
	#define	INT_TYPES
	typedef char				int8;	/* 8-bit integer  */
	typedef short				int16;	/* 16-bit integer */
	typedef	int					int32;	/* 32-bit integer */
	typedef long long			int64;	/* 64-bit integer */

	typedef	unsigned char  		uint8;	/* unsigned 8-bit integer */
	typedef unsigned char  		byte;	/* unsigned 8-bit integer */
	typedef unsigned short 		uint16;	/* unsigned 16-bit integer */
	#ifndef _UINT32
	typedef unsigned int  		uint32;	/* unsigned 32-bit integer */
	#endif
	typedef unsigned long long 	uint64;	/* unsigned 64-bit integer */
	
	#if __LP64__
        typedef unsigned long long	ptrtyp; /* type used to do pointer math */
	#else
        typedef unsigned long		ptrtyp; /* type used to do pointer math */
	#endif
#endif

#define MAX_INT8	0x7F				/* largest positive 8-bit integer */
#define MAX_INT16	0x7FFF				/* largest positive 16-bit integer */
#define MAX_INT32	0x7FFFFFFF			/* largest positive 32-bit integer */

#define MAX_UINT8	0xFFu				/* largest unsigned 8-bit integer */
#define MAX_UINT16	0xFFFFu				/* largest unsigned 16-bit integer */
#define MAX_UINT32	0xFFFFFFFFu			/* largest unsigned 32-bit integer */

#if !defined(__GNUC__)
	#if (!__option (bool))
		#ifndef	BOOL_TYPE
			typedef unsigned char bool;		/* boolean value */
		#endif
	#endif
#endif

/* manipulation of 16-bit values in 32-bit words */

#define	make_long(msw,lsw) ((((uint32)(uint16)(msw))<<16)+((uint32)(uint16)(lsw)))
// #define hi_word(x)			((unsigned)(x) >> 16)
// #define lo_word(x)			((uint16)(x))

// #define make_word(msb, lsb)	(((uint8) (msb) << 8) + (uint8) (lsb))
// #define hi_byte(x)			((uint16)(x) >> 8)
// #define lo_byte(x)			((uint8)(x))


/* standard maximum and minimum macros */
#ifndef max
	//#define max(x, y)	((x) < (y) ? y : x)
#endif

#ifndef min
	//#define min(x, y)	((x) < (y) ? x : y)
#endif

/* object existence check */
#define exists(x)	((x) != NULL)		/* TRUE if object/pointer exists */

#endif /* NED__C */
