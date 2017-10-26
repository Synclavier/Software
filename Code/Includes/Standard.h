/*  file:   Standard.h  				*/
/*  author: C. Jones    				*/


/*  Modification History:               */
/*  06/18/96 - C. Jones - Created       */

#ifndef standard_h
#define standard_h

// File includes C type definitions for standard integer types as well as macros.
// Originaly developed for early XPL translator

/*------------------------------------------------------------------------- */
/*  Standard Data Types:                                                    */
/*------------------------------------------------------------------------- */

/*  To provide compatibility with as many C compilers as possible, the      */
/*  following typedefs will be used for all declarations.                   */

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
	typedef unsigned int		uint32;	/* unsigned 32-bit integer */
	#endif
	typedef unsigned long long 	uint64;	/* unsigned 64-bit integer */
	
	#if __LP64__
        typedef unsigned long long	ptrtyp; /* type used to do pointer math */
	#else
        typedef unsigned long		ptrtyp; /* type used to do pointer math */
	#endif
#endif

#ifndef RATIO_TYPES
	#define	RATIO_TYPES
	
	#pragma pack(push,2)

	typedef	struct uint32_ratio			// packed ratio of two unsigned 32-bit numbers
	{
		uint32	num;
		uint32  den;

	} uint32_ratio;

	#pragma pack(pop)
#endif

#ifndef	XPL_h
	#ifndef __RPCNDR_H__
		typedef short    	boolean;
	#endif
#endif

/* Symbol names:                                                            */
/*  use underscores for separator                                           */
/*  use g_ preceding global data storage                                    */
/*  use CAPITAL letters for ENUMS and DEFINES                               */

/*  examples:   local_variable                                              */
/*              g_global_variable                                           */
/*              #define DEFINE_VARIABLE ...                                 */
/*              enum    ENUMERATION {...}                                   */


/* booleans:                                                                */

#ifndef TRUE
	#define TRUE    1
#endif

#ifndef FALSE
	#define FALSE   0
#endif

#ifndef NULL
	#define NULL    0
#endif

#endif

