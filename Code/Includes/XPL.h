/*	xpl.h																		*/

/*	Contents: Macros & run-time interface for XPL translations					*/

/*	Created:	08/11/96 - C. Jones												*/


#ifndef XPL_h
#define XPL_h


// This file seves two very different purposes. Firstly, it is designed to be included in any C code
// that has been translated from XPL. That is, the XPL-to-C translater emits translated code that requires
// some of these macros to work.

// This file also serves as the header file for the complex able simulation run time environment that
// is implemented in the Synclavier PowerPC run time implementation


/*	The following definitions preserve some xpl-specific information that will	*/
/* 	be used by the C-Compatible XPL compiler to reconstruct the ABLE object		*/
/*	code from the C translated file.  They null out all information for			*/
/* 	the C compiler.																*/

#define _recursive					/* null out recursive attribute				*/
#define _swap						/* null out swap      attribute				*/
#define _swapcode					/* null out swapcode  attribute				*/
#define _was_nested(a,b)			/* preserve XPL nested proc definitions		*/
#define _pdl(a)						/* preserve xpl PDL statement				*/
#define _configuration(a)			/* preserve xpl CONFIGURATION statement		*/
#define _ram(a)						/* preserve xpl RAM statement				*/
#define _library(a)					/* preserve XPL LIBRARY statement			*/
#define _label(a)					/* preserve XPL label declaration			*/
#define _module(a)					/* preserve XPL MODULE statement			*/


/*	Typedefs:																	*/

typedef short			fixed;		/* xpl "fixed" are 16-bit, signed			*/
typedef unsigned short	ufixed;		/* but many operations unsigned				*/
typedef fixed*			pointer;	/* xpl pointer types						*/
typedef fixed			array  [];	/* xpl array type							*/
typedef fixed			data   [];	/* xpl data type							*/
typedef char			cstring[];	/* xpl cstring type (heh, heh!)				*/
typedef void**			handle;		/* legit handle on host system				*/
typedef unsigned int	ulong;		/* handy unsigned long  typedef - 32 bits	*/
typedef unsigned short	ushort;		/* handy unsigned short typedef - 16 bits	*/
typedef unsigned char	uchar;		/* handy unsigned char  typedef -  8 bits	*/

#ifndef RATIO_TYPES
    #define	RATIO_TYPES

    #pragma pack(push,2)

    typedef	struct uint32_ratio			// packed ratio of two unsigned 32-bit numbers
    {
        unsigned int  num;
        unsigned int  den;
        
    } uint32_ratio;

    #pragma pack(pop)
#endif

#ifndef	standard_h
	typedef	short		boolean;	/* handle xpl boolean types					*/
#endif


/* Struct to model Able Hardware Mul/Div unit:									*/

#pragma pack(push,2)

typedef	union                           /* define struct for mul/div use			*/
{                                       /* hardware-specific union to simplify		*/
	struct                              /* modeling of Able's d4567					*/
	{
		unsigned	volatile	int	d4_5;
	}	both;
	
	struct
	{
		#if __BIG_ENDIAN__
			unsigned	volatile	short	d4;
			unsigned	volatile	short	d5;
		#else
			unsigned	volatile	short	d5;
			unsigned	volatile	short	d4;
		#endif
		
	}	each;

}	_mul_div_union;

#pragma pack(pop)

/*	Macros:																		*/

/*	Certain XPL functions are implemented in C as macros.						*/

/*	Notes:	fmul	was implemented as signed a*b in XPL						*/
/*			fdiv 	was implemented as signed (a*65536)/b in XPL				*/
/*			*    	was implemented as signed a*b in XPL						*/
/*			/    	was implemented as signed a/b in XPL						*/
/*			mod  	was implemented as signed a%b in XPL						*/
/*			a*b/c	was implemented as signed (a*b)/c with 32-bit intermediate	*/

/*			d4567 itself operated unsigned:										*/
/*				write to d5 cleared d4											*/
/*				write to d4 loaded  d4											*/
/*				write to d6 computed d5*d6+d4									*/
/*				write to d7 computed (d4:d5) / d7; q to d5, r to d4				*/

/* C-only macros.  These macros do not access any global variables.  They		*/
/* implement certain xpl language features in c.								*/

#define		fmul(a,b)				((fixed) ((((int) (a)) * ((int) (b))) >> 16))
#define		fdiv(a,b)				((fixed) ((((int) (a)) << 16) / ((int) (b))))
#define		_scale_(a,b,c)			((fixed) ((((int) (a)) * ((int) (b))) / ((int) (c))))

#define		shr(a,b)				((fixed) (((ufixed) (a)) >> ((ufixed) (b))))
#define		shl(a,b)				((fixed) (((ufixed) (a)) << ((ufixed) (b))))
#define		rot(a,b)				(shl((a),(b)) | shr((a),16-(b)))

#define		_IGE_(a,b)				(((ufixed) (a)) >= ((ufixed) (b)))
#define		_IGT_(a,b)				(((ufixed) (a)) >  ((ufixed) (b)))
#define		_ILT_(a,b)				(((ufixed) (a)) <  ((ufixed) (b)))
#define		_ILE_(a,b)				(((ufixed) (a)) <= ((ufixed) (b)))

#define		_location_(a)			((pointer) (a))
#define		_cbyte(a,b)				((fixed) (((uchar *)(a))[b]))

#define		_to32(a,b)				((((int) ((ufixed) (a))) << 16) | ((int) ((ufixed) (b))))
#define		_to32a(a)				(_to32(a[0], a[1]))

#if __BIG_ENDIAN__
	#define		byte(a,b)			((fixed) (((uchar *)(a))[((b) + 2) ^ 1]))
	#define		pbyte(a,b,c)		((((uchar *)(a))[((b) + 2) ^ 1]) = (c))
	#define		Host2BigByte(a,b)	(((uchar*)(a))[b])
#else
	#define		byte(a,b)			((fixed) (((uchar *)(a))[((b) + 2)]))
	#define		pbyte(a,b,c)		((((uchar *)(a))[((b) + 2)]) = (c))
	#define		Host2BigByte(a,b)	(((uchar*)(a))[(b)^1])
#endif

/* Misc constants: */

#define		d_protect				0x0002			/* set in value returned by DISK_CHECK if disk is write-protected	*/
#define		_ABLE_MEM_WORD_SIZE  	(64*1024)		/* size in words	*/


/* Macros for accessing simulated Able hardware							*/

/* The following macros provide shorthand equivalents for accessing		*/
/* various able structures that model specific Able hardware.  They		*/
/* all incorporate a reference to a macro called ABLE_CONTECT which		*/
/* is used to associate these macros with a spcific simulated			*/
/* run-time context.   This context must be set before these macros		*/
/* can be used.  The purpose of this mechanism is to be able to			*/
/* support implementations where all access are computed at 			*/
/* compile time (for cases where speed is extremely important) plus		*/
/* to support implementations where the data is part of a struct.		*/
/* This latter case may be used to simulate multiple able computers		*/
/* by one program.														*/

/* Note: these macros are used both by code that has been translated	*/
/* into C, and code that is emulating able object code.					*/

typedef	struct _able_context					/* define a struct that can hold an entire able context	*/
{
	fixed 	*_able_memory_;						/* pointer to simulated main able memory				*/
	fixed	_able_heap_pointer;					/* use for able core heap manager						*/
	ulong	_able_memory_size_bytes;			/* size of allocated space for able memory				*/

	_mul_div_union	 d4567;						/* holds data for d4567 emulation						*/

	fixed	*d60;								/* points to memory allocated for d60 emulation			*/
	ulong	_able_xmem_addr;					/* working pointer for extermal memory					*/
	fixed	_able_xmem_size;					/* holds size in sectors of allocated d60  memory		*/
	ulong	_able_xmem_size_bytes;				/* size of allocated space for able external memory		*/

	char 	able_cur_dir_name    [256];			/* able current dir										*/
	char 	able_master_dir_name [256];			/* able master dir										*/
	char 	host_output_dir_name [256];			/* overriding hostoutput dir							*/
	char 	opened_file_name     [256];			/* returned when file opened							*/

}	_able_context;

extern	_able_context   *g_able_context;		/* points to context being used at this time			*/
#define ABLE_CONTEXT	(*g_able_context)		/* can be undef'd for evil purposes!!!					*/
#define ABLE_D4567      (ABLE_CONTEXT.d4567)	/* can be undef'd for halloween!!!						*/

#define		_write_4(a)		(ABLE_D4567.each.d4   = (         (ushort) (a)))
#define		_write_5(a)		(ABLE_D4567.both.d4_5 = ((ulong ) (ushort) (a)))

#define		_write_6(a)		(ABLE_D4567.both.d4_5 = ( ( ((ulong) ABLE_D4567.each.d5)      \
 												      * ((ulong) (ushort) (a)))           \
 											        + (  (ulong) ABLE_D4567.each.d4)))

#define		_write_7(a)		(ABLE_D4567.both.d4_5 = ( ( (ABLE_D4567.both.d4_5)             \
                                                      / ((ulong) (ushort) (a)))		       \
                                                    + ( (ABLE_D4567.both.d4_5 % (ulong) (ushort) (a)) << 16)))

#define		_read_4			((fixed) ABLE_D4567.each.d4)
#define		_read_5			((fixed) ABLE_D4567.each.d5)

/* Accessing able devices:

.    -2: Path catalog
.    -1: Alternate catalog
.     0: System catalog
.     1: User catalog
.     2: F0 (Leftmost floppy)
.     3: F1 (Rightmost floppy)
.     4: R0 (Remote floppy)
.     5: R1 (Remote floppy)
.     6: W0 (Winchester disk)
.     7: W1 (Winchester disk)
.     8: T0 (Cartridge tape)
.     9: T1 (Cartridge tape)
.    10: O0 (Optical Disk
.    11: O1 (Optical Disk)
.    16 & above: Image files on mac */

typedef enum                            /* readdata codes for able disk	*/
{
	ABLE_W0_READDATA_CODE  =  6,
	ABLE_W1_READDATA_CODE  =  7,
	
	ABLE_O0_READDATA_CODE  = 10,
	ABLE_O1_READDATA_CODE  = 11,

	ABLE_HFS_READDATA_CODE = 16

}	able_readdata_code;

typedef enum
{
	ABLE_W0_DEFAULT_SCSI_ID = 5,
	ABLE_W1_DEFAULT_SCSI_ID = 4,
	ABLE_O0_DEFAULT_SCSI_ID = 1,
	ABLE_O1_DEFAULT_SCSI_ID = 2,
    ABLE_IMPORT_SCSI_ID     = 3         // Use for Synclavier3 import window; device is not owned by the interpreter; was DTD in vitro
	
}	able_default_scsi_ids;


#define	XPL_RUN_TIME_USE_BIG_MEMORY		1
#define	XPL_RUN_TIME_USE_KERNEL_MEMORY	2

#define	NUM_TARGETS_IN_TARGET_LIST		8
#define	NUM_BOARDS_IN_BOARD_LIST		4

extern	void			to_c_string    (const fixed *a_string, char  *c_string);
extern	void			to_able_string (const char *c_string,  fixed *a_string);

#endif

