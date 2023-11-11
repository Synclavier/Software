/* Catrtns  $title System Catalog Routines */

/*	Translated to C:   	11/26/96 at 17:17	*/
/*	Translator Version:	0.000				*/

/* Catrtns code */

/* These routines comprise the catalog management routines for the
.  ABLE Series computer.  They include routines to read and write
.  the catalog, look for files in the catalog, add files to the
.  catalog, remove files from the catalog, etc.
.
.  These routines support the following catalog levels as well as
.  explicitly addressed catalogs and cached catalogs:
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
.
.  These routines also support accessing files or catalogs by treename.
.  The format of a treename is:
.    <treename> := ( <pathname> ) <filename>
.    <pathname> := ( (<devname>) :) { <catname> : }
.    <devname>  := [ F0 | F1 | R0 | R1 | W0 | W1 | T0 | T1 ]
.
.  where spaces are inserted for readability (no spaces should appear in
.  a treename), () represents optional productions, {} indicate zero
.  or more repititions of the enclosed production, and [] represents
.  "either" of the following productions (separated by |).
.
.  By:  Karim J. Chichakly on 30 October 1985
.       from pieces of various other catalog routines by CWJ et al.
.
.  Modification history:
.    15 Oct 1986:  Added TREENAMEs to level-oriented routines [KJC]
.    28 May 1987:  Added TRUNCATE and SHORTENFILE [KJC]
*/

/* $subtitle Literal Declarations */

#include    <CoreServices/CoreServices.h>

#include	"XPL.h"
#include    "XPLRuntime.h"

#include	"asciilit.h"
#include	"syslits.h"
#include	"catrtns.h"

/* Catalog entry definitions */

#define	c_nm		0						/* filename (four words)						*/
#define	c_ls		4						/* Ls starting sector							*/
#define	c_ll		5						/* Ls sector length								*/
#define	c_wd		6						/* word length (modulo 64K)						*/
#define	c_ty		7						/* Ms starting sector (8 bits)/MS sector length (4 bits)/file type (4 bits)	*/
#define	c_len		8						/* number of words in a catalog entry			*/

#define	c_dir_max	1024					/* maximum directory size (words)				*/


/* Fcb definitions (used with GET_FCB and PUT_FCB) */

#define	f_nm		0						/* filename (four words)						*/
#define	f_ls		4						/* Ls starting sector							*/
#define	f_ms		5						/* Ms starting sector							*/
#define	f_ll		6						/* Ls sector length								*/
#define	f_ml		7						/* Ms sector length								*/
#define	f_wd		8						/* word length (modulo 64K)						*/
#define	f_ty		9						/* file type									*/
#define	f_len		10						/* number of words in an FCB					*/

#define	f_name_len	4						/* number of words in a filename				*/

/* $page */


/* File type definitions */

#define	t_text		0						/* text file									*/
#define	t_exec		1						/* executable binary							*/
#define	t_reloc		2						/* relocatable binary							*/
#define	t_data		3						/* data file									*/
#define	t_sync		4						/* synclavier sequence							*/
#define	t_sound		5						/* sound file									*/
#define	t_subc		6						/* subcatalog									*/
#define	t_lsubc		7						/* large subcatalog								*/
#define	t_dump		8						/* dump file									*/
#define	t_spect		9						/* spectral file								*/
#define	t_index		10						/* index file									*/
#define	t_timbre	11						/* synclavier timbre							*/
#define	t_max		11						/* maximum defined filetype						*/


/* Errors returned in C#STATUS */

#define	e_none		0						/* no error encountered							*/
#define	e_os		1						/* operating system error - magic number not set	*/
#define	e_buffer	2						/* no catalog buffer allocated					*/
#define	e_no_dir	3						/* no directory in memory						*/
#define	e_no_config	4						/* device not configured						*/
#define	e_no_floppy	5						/* no floppy in drive							*/
#define	e_fcb		6						/* Fcb number out of bounds						*/
#define	e_level		7						/* level number out of bounds					*/
#define	e_storage	8						/* not enough available storage					*/
#define	e_cstorage	9						/* not enough contiguous storage available		*/
#define	e_dir_full	10						/* no entries left in the directory				*/
#define	e_invalid	11						/* invalid directory							*/
#define	e_name		12						/* invalid filename specified for operation requested	*/
#define	e_duplicate	13						/* duplicate filename							*/
#define	e_no_file	14						/* file not found								*/
#define	e_not_cat	15						/* name specified is required to be a catalog, but isn't	*/
#define	e_treename	16						/* incorrect format for treename				*/
#define	e_no_path	17						/* intermediate catalog (in treename) not found	*/
#define	e_type		18						/* type mismatch between saved file and replaced file (ADDFILE/REPLACE only)	*/
#define	e_protect	19						/* write protected floppy						*/
#define	e_too_large	20						/* file too large (>= 2^20 sectors)				*/
#define	e_truncate	21						/* truncation error - trying to expand a file (or truncate tape file)	*/


/* Caching literals */

#define	__enabled	0						/* offset for cache enabled flag (lower eight bits)	*/
#define	__bufmed	0						/* offset for cache media (upper 8 bits)		*/
#define	__bufptr	1						/* offset for cache pointer						*/
#define	__ms_sector	2						/* offset for device/MS sector of catalog		*/
#define	__ls_sector	3						/* offset for LS sector of catalog				*/
#define	__dir_size	4						/* offset for directory length (FLUSH_CACHE needs this)	*/
#define	__vars		5						/* five variables/cache							*/
#define	__default	2						/* number of default caches						*/

#define	max_depth	8						/* number of treename levels we can cache		*/

/* $subtitle Global Variable Declarations */


/* Global catalog variables */
fixed			AbleCatRtns_AllowTypeReplace;
fixed			AbleCatRtns_SoundFileChanged;

fixed			c_status;					/* status of last catalog operation				*/
fixed			c_bufptr = -1;				/* pointer to the catalog buffer				*/
fixed*          c_bufhost;                  /* catalog buffer in host computer memory       */
fixed			c_bufmed;					/* 0 - main memory, 1 - external memory         */
fixed			c_ms_sector;				/* device and MS starting sector of catalog		*/
fixed			c_ls_sector;				/* Ls starting sector of catalog				*/
fixed			c_ms_length;				/* Ms sector length of catalog (including directory)	*/
fixed			c_ls_length;				/* Ls sector length of catalog (including directory)	*/
fixed			c_dir_size;					/* size of catalog directory (in words)			*/


/* Global file variables */

fixed			f_name[f_name_len + 1];		/* name of current subpart of scanned treename	*/

fixed			f_ms_sector;				/* device and MS starting sector of file		*/
fixed			f_ls_sector;				/* Ls starting sector of file					*/
fixed			f_ms_length;				/* Ms sector length of file						*/
fixed			f_ls_length;				/* Ls sector length of file						*/
fixed			f_words;					/* word length of file (modulo 64K)				*/
fixed			f_type;						/* type of file									*/


/* Alternate catalog variables */

fixed			a_name[f_name_len + 1];		/* name of alternate catalog					*/

fixed			a_ms_sector;				/* device and MS starting sector of catalog		*/
fixed			a_ls_sector;				/* Ls starting sector of catalog				*/
fixed			a_ms_length;				/* Ms sector length of catalog (including directory)	*/
fixed			a_ls_length;				/* Ls sector length of catalog (including directory)	*/
fixed			a_dir_size;					/* size of catalog directory (in words)			*/


/* Caching variables */

#ifndef	false
	#define		false	0
#endif

#ifndef true
	#define		true	1
#endif

static	fixed	caches;						/* pointer to cache variable array				*/
static	fixed	max_caches;					/* size of cache variable array					*/
static	fixed	catalog_cache = -1;			/* cache number of catalog buffer (if used as a cache)	*/

static	fixed	path_depth;					/* depth (number of levels) of current pathname	*/
static	boolean	path_cached = false;		/* True if the pathname is cached				*/

static	fixed	names[((fixed) (max_depth*(f_name_len + 1) ))];	/* names of each level							*/
static	fixed	name[max_depth];			/* pointer to NAMES at each level				*/
static	fixed	ms_sector[max_depth];		/* C#ms_sector at each level					*/
static	fixed	ls_sector[max_depth];		/* C#ls_sector at each level					*/
static	fixed	ms_length[max_depth];		/* C#ms_length at each level					*/
static	fixed	ls_length[max_depth];		/* C#ls_length at each level					*/
static	fixed	dir_siz[max_depth];			/* C#dir_size at each level						*/
static	fixed	topmost_level;				/* level of catalog treename starts in			*/

/* Variables set by FIND_EXACT */

static	fixed	last_free;					/* Fcb number of last free block				*/
static	fixed	ms_lastfree;				/* sector address of last free block			*/
static	fixed	ls_lastfree;
static	fixed	ms_remaining;				/* remaining sectors in catalog					*/
static	fixed	ls_remaining;


static	fixed	fcb[16];					/* global FCB (NOT preserved across calls!) [size must be max(F#LEN - 1, 15)]	*/
static	fixed	fcb2[f_len];				/* global FCB (NOT preserved across calls!) [size must be max(F#LEN - 1, C#LEN - 1)]	*/
static	fixed	entry[c_len];				/* global catalog entry (set by GET_FCB and PUT_FCB only)	*/
static	fixed	fname[f_name_len];			/* global cleaned up filename (set FINDFILE)	*/

#define	a	fcb
#define	b	fcb2							/* rename our global FCBs						*/
	

/* $subtitle Filename Processing:  Valid_FileChar, Valid_Filename */


/* This procedure returns TRUE if the specified character is valid
   .  in a filename.  Otherwise, it returns FALSE. */

boolean			valid_filechar(					/* return TRUE if specified character is valid in a filename	*/
	fixed	c)									/* character to test							*/
	
{
	if ((c <= a_exclam) || (c == a_percent) || (c == a_and) || ((c >= a_star) && (c <= a_comma)) || (c == a_slash)/*  ctrl's space ! % & * + , /					*/
	|| ((c >= a_colon) && (c <= a_at)) || (c == a_backslash) || (c == a_bar) || (c == a_star) || (c >= a_del))/*  : ; < = > ? @ \ | * Del						*/
		return (false);							/* these are invalid							*/
	else return (true);							/* valid filename character						*/
}
	
	
/* This procedure returns TRUE if the specified NAME is a valid
   .  filename.  Otherwise, it returns FALSE. */

boolean			valid_filename(					/* return TRUE if passed filename is valid		*/
	fixed	name[])	_recursive					/* filename to test								*/
{
    fixed			_upper0;
	fixed			i;
	
	if (name [0] == 0) return (false);			/* there has to be a name						*/
	if (name [0] >  8) return (false);			/* and it can't be too long						*/
		
	for (_upper0 = name [0] - 1, i = 0; i <= _upper0; i++) {	/* test the name								*/
		if (! (valid_filechar (byte(name, i)) & 1)) return (false);	/* no good										*/
	}
		
	return (true);								/* name is okay									*/
}
	
/* $subtitle Filename Processing:  Clean_Filename, Clean_FCBname */


/* This procedure converts the XPL string format NAME to an FCB format
   .  name (starts at element 0, eight characters, uppercase, padded with
   .  zeroes) and stores the result in the passed FCB_NAME. */

void			clean_filename(					/* convert XPL string format to FCB name format	*/
	fixed	name[], 							/* filename to clean							*/
	fixed	fcb_name[])	_recursive				/* store result here							*/
{
    fixed			_upper0;
	fixed			len;						/* name length									*/
	fixed			i, c;
	
	if (_ILE_(name [0], shl(f_name_len, 1))) len = name [0]; else len = shl(f_name_len, 1);	/* len = min(name (0), f#name_len*2); restrict name to F#NAME_LEN*2 chars	*/
		
	for (_upper0 = shl(f_name_len, 1) - 1, i = 0; i <= _upper0; i++) {	/* clean up the name							*/
		if (i < len) {							/* get next character from name					*/
			c = byte(name, i);					/* get next character							*/
			if (_ILE_(c - l_a, (l_z - l_a))) c = c - (l_a - a_a);	/* uppercase it									*/
		}										/* of get next character from name				*/
		else c = a_null;						/* pad with nulls								*/
			
		pbyte(_location_(&(fcb_name [0]) - 1), i, c);	/* save in FCB_NAME								*/
	}
}
	
	
/* This procedure converts the FCB format name to an XPL string format
   .  name and stores the result in the passed NAME.  The returned
   .  NAME will be padded with spaces to F#NAME_LEN*2 characters,
   .  but the length of the string (that is, NAME (0)) will be set
   .  to the actual number of characters in the name. */

void			clean_fcbname(					/* convert FCB name to XPL string format		*/
	fixed	fcb_name[], 						/* Fcb name to convert							*/
	fixed	name[])	_recursive					/* store result here							*/
{
    fixed			_upper0;
	fixed			len;						/* name length									*/
	fixed			i, c;
	
	len = 0;									/* assume zero length							*/
	
	for (_upper0 = shl(f_name_len, 1) - 1, i = 0; i <= _upper0; i++) {	/* convert the name								*/
		c = byte(_location_(&(fcb_name [0]) - 1), i);	/* get next byte from FCB_NAME					*/
		
		if (c == a_null)						/* if it's a NULL								*/
			c = a_sp;							/* convert to space								*/
		else len = len + 1;						/* found another filename character				*/
			
		pbyte(name, i, c);						/* save in NAME									*/
	}
		
	name [0] = len;								/* save length									*/
}
	
/* $subtitle Logical FCB Handling:  Get_FCB */


/* This procedure extracts FCB number FCB# (where FCBs are numbered from
   .  zero to C#DIR_SIZE/C#LEN - 1) from the catalog buffer.  If FCB# is
   .  out of bounds (or there is no catalog buffer), a boolean FALSE is
   .  returned.  Otherwise, the FCB is returned in the passed array FCB. */

boolean			get_fcb(						/* extract an FCB from the catalog buffer		*/
	fixed	fcb_, 								/* Fcb number of block to get					*/
	fixed	fcb[])								/* fcb array									*/
{
	static	fixed	i;
	
    fcb_ = shl(fcb_, 3);						/* convert the FCB number into a pointer into the catalog buffer	*/
    
    if (c_bufhost) {                            /* if the catalog's in host memory				*/
        for (i = 0; i < f_name_len; i++) {		/* get the name									*/
            fcb [f_nm + i] = c_bufhost[fcb_ + c_nm + i];
        }
        
        fcb [f_ms] = shr(c_bufhost[fcb_ + c_ty], 8);            /* get starting sector							*/
        fcb [f_ls] = c_bufhost[fcb_ + c_ls];
        fcb [f_ml] = (shr(c_bufhost[fcb_ + c_ty], 4) & 0x000F);	/* get sector length							*/
        fcb [f_ll] = c_bufhost[fcb_ + c_ll];
        fcb [f_wd] = c_bufhost[fcb_ + c_wd];                    /* get word length								*/
        fcb [f_ty] = (c_bufhost[fcb_ + c_ty] & 0x000F);         /* get file type								*/
        
        return true;                                            /* skip other error checks in this case         */
    }
    
	c_status = e_none;							/* no errors yet								*/
	
	if (c_bufptr == -1) {						/* if no buffer									*/
		c_status = e_buffer;
		return (false);
	}
		
	if ((c_ms_sector == -1) && (c_ls_sector == -1)) {	/* if no catalog								*/
		c_status = e_no_dir;
		return (false);
	}
		
	if (_IGE_(fcb_, c_dir_size)) {              /* if the FCB number's out of bounds			*/
		c_status = e_fcb;
		return (false);
	}
		
    if (c_bufmed == 0) {                        /* if the catalog's in main memory				*/
        for (i = 0; i < f_name_len; i++) {		/* get the name									*/
            fcb [f_nm + i] = able_core(c_bufptr + fcb_ + c_nm + i);
        }
        
        fcb [f_ms] = shr(able_core(c_bufptr + fcb_ + c_ty), 8);             /* get starting sector							*/
        fcb [f_ls] = able_core(c_bufptr + fcb_ + c_ls);
        fcb [f_ml] = (shr(able_core(c_bufptr + fcb_ + c_ty), 4) & 0x000F);	/* get sector length							*/
        fcb [f_ll] = able_core(c_bufptr + fcb_ + c_ll);
        fcb [f_wd] = able_core(c_bufptr + fcb_ + c_wd);                     /* get word length								*/
        fcb [f_ty] = (able_core(c_bufptr + fcb_ + c_ty) & 0x000F);          /* get file type								*/
	}											/* of catalog in main memory					*/
    
	else {										/* catalog's in external memory					*/
		XPLimport (c_bufptr, fcb_, entry, c_len);	/* get the entry from external memory       */
		
		for (i = 0; i < f_name_len; i++) {		/* get the name									*/
			fcb [f_nm + i] = entry [c_nm + i];
		}
			
		fcb [f_ms] = shr(entry [c_ty], 8);              /* get starting sector                  */
		fcb [f_ls] = entry [c_ls];
		fcb [f_ml] = (shr(entry [c_ty], 4) & 0x000F);	/* get sector length                    */
		fcb [f_ll] = entry [c_ll];
		fcb [f_wd] = entry [c_wd];				/* get word length								*/
		fcb [f_ty] = (entry [c_ty] & 0x000F);	/* get file type								*/
	}											/* of catalog in external memory				*/
		
	return (true);								/* we got the FCB								*/
}
	
/* $subtitle Logical FCB Handling:  Put_FCB */


/* This procedure puts the contents of array FCB into FCB number FCB#
   .  of the catalog buffer.  If FCB# is out of bounds (or there is no
   .  catalog buffer), a boolean FALSE is returned. */

boolean			put_fcb(						/* replace an FCB into the catalog buffer		*/
	fixed	fcb_, 								/* Fcb number of block to get					*/
	fixed	fcb[])								/* fcb array									*/
{
	static	fixed	i;
	
    fcb_ = shl(fcb_, 3);						/* convert the FCB number into a pointer into the catalog buffer	*/
    
    if (c_bufhost) {                            /* if the catalog's in host memory				*/
        for (i = 0; i < f_name_len; i++) {		/* put the name									*/
            c_bufhost[fcb_ + c_nm + i] = fcb [f_nm + i];
        }
        
        c_bufhost[fcb_ + c_ls] = fcb [f_ls];	/* put the LS starting sector					*/
        c_bufhost[fcb_ + c_ll] = fcb [f_ll];	/* put the LS sector length						*/
        c_bufhost[fcb_ + c_wd] = fcb [f_wd];	/* put the word length							*/
        c_bufhost[fcb_ + c_ty] = (shl(fcb [f_ms], 8) | shl(fcb [f_ml] & 0x000F, 4) | (fcb [f_ty] & 0x000F));	/* put MS sector/MS length/type					*/
        
        return true;                            /* skip other error checks                      */
    }
    
	if (c_bufptr == -1) {						/* if no buffer									*/
		c_status = e_buffer;
		return (false);
	}
		
	if ((c_ms_sector == -1) && (c_ls_sector == -1)) {	/* if no catalog								*/
		c_status = e_no_dir;
		return (false);
	}
		
	if (_IGE_(fcb_, c_dir_size)) {              /* if the FCB number's out of bounds			*/
		c_status = e_fcb;
		return (false);
	}
		
	if (c_bufmed == 0) {                        /* if the catalog's in main memory				*/
		for (i = 0; i < f_name_len; i++) {		/* put the name									*/
			set_able_core(c_bufptr + fcb_ + c_nm + i, fcb [f_nm + i]);
		}
			
		set_able_core(c_bufptr + fcb_ + c_ls, fcb [f_ls]);	/* put the LS starting sector					*/
		set_able_core(c_bufptr + fcb_ + c_ll, fcb [f_ll]);	/* put the LS sector length						*/
		set_able_core(c_bufptr + fcb_ + c_wd, fcb [f_wd]);	/* put the word length							*/
		set_able_core(c_bufptr + fcb_ + c_ty, (shl(fcb [f_ms], 8) | shl(fcb [f_ml] & 0x000F, 4) | (fcb [f_ty] & 0x000F)));	/* put MS sector/MS length/type					*/
	}											/* of catalog in main memory					*/
    
	else {										/* catalog's in external memory					*/
		for (i = 0; i < f_name_len; i++) {		/* put the name									*/
			entry [c_nm + i] = fcb [f_nm + i];
		}
			
		entry [c_ls] = fcb [f_ls];				/* put the LS starting sector					*/
		entry [c_ll] = fcb [f_ll];				/* put the LS sector length						*/
		entry [c_wd] = fcb [f_wd];				/* put the word length							*/
		entry [c_ty] = (shl(fcb [f_ms], 8) | shl(fcb [f_ml] & 0x000F, 4) | (fcb [f_ty] & 0x000F));	/* put MS sector/MS length/type					*/
		
		XPLexport (c_bufptr, fcb_, entry, c_len);	/* put the entry in external memory				*/
	}											/* of catalog in external memory				*/
		
	return (true);								/* we put the FCB								*/
}
	
/* $subtitle Storage Management:  Consolidate */


/* This (internal) procedure makes a pass through the catalog buffer,
   .  consolidating all contiguous blocks of free storage into one block.
   .  This procedure returns FALSE if GET_FCB fails for any reason. */

static	boolean	consolidate()	_recursive		/* consolidate contiguous blocks of free storage	*/
{
    fixed			_upper0;
	fixed			ams, als;					/* ending sector of block in A					*/
	fixed			bms, bls;					/* and in B										*/
	fixed			x, y;
	
	fixed			zero[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};	/* a zeroed out fcb								*/
	
	if ((shr(c_ms_sector, 8) != 8) && (shr(c_ms_sector, 8) != 9))/* don't consolidate the tape drive!			*/
	for (_upper0 = shr(c_dir_size, 3) - 1, x = 0; x <= _upper0; x++) {	/* look at all FCBs								*/
		if (! (get_fcb (x, a) & 1)) return (false);	/* get this one									*/
			
		if (((a [f_nm] == 0) && (a [f_ml] == 0) && (a [f_ll] == 0))/* if deleted block								*/
		|| ((a [f_ms] == 0) && (a [f_ls] == 0) && ((a [f_ml] != 0) || (a [f_ll] != 0))))
			put_fcb (x, zero);					/* no data in this section of catalog			*/
		else if ((a [f_nm] == 0) && ((a [f_ml] != 0) || (a [f_ll] != 0))) {	/* hole											*/
			for (y = 0; y < x; y++) {			/* now look through cat up to where we are		*/
				get_fcb (y, b);					/* get it										*/
				
				if ((b [f_nm] == 0) && ((b [f_ml] != 0) || (b [f_ll] != 0))) {	/* B is a hole with some sectors in it			*/
					ams = a [f_ms] + a [f_ml];	/* find where block A ends						*/
					als = a [f_ls] + a [f_ll];
					if (_ILT_(als, a [f_ll])) ams = ams + 1;
						
					bms = b [f_ms] + b [f_ml];	/* find where block B ends						*/
					bls = b [f_ls] + b [f_ll];
					if (_ILT_(bls, b [f_ll])) bms = bms + 1;
						
					if ((bms == a [f_ms]) && (bls == a [f_ls])) {		/* hole b ends at start of a					*/
						a [f_ms] = b [f_ms];							/* a hole starts here							*/
						a [f_ls] = b [f_ls];
						a [f_ml] = a [f_ml] + b [f_ml];
						a [f_ll] = a [f_ll] + b [f_ll];
						if (_ILT_(a [f_ll], b [f_ll])) a [f_ml] = a [f_ml] + 1;
							
						if (_ILT_(a [f_ml], 0x0010))					/* if combined length will fit in block			*/
						{												/* then combine them							*/
							put_fcb (y, zero);							/* zero it										*/
							put_fcb (x, a);								/* new a										*/
						}
						else 											/* else limit to 2^20-1							*/
						{
							b [f_ml] = 0x000F;							/* limit length of b to 2^20-1					*/
							b [f_ll] = ((fixed) 0xFFFF);
							
							a [f_ms] = b [f_ms] + b [f_ml];				/* compute start of a to be at end of			*/
							a [f_ls] = b [f_ls] + b [f_ll];				/* truncated b									*/
							if (_ILT_(a [f_ls], b [f_ll]))
								a [f_ms] = a [f_ms] + 1;
								
							if (_ILT_(a [f_ll], ((fixed) 0xFFFF)))		/* and compute length of a to be that			*/
								a [f_ml] = a [f_ml] - 1;				/* part above 0x FF FFFF						*/
								
							a [f_ll] = a [f_ll] - ((fixed) 0xFFFF);
							a [f_ml] = a [f_ml] - 0x000F;
							
							put_fcb (y, b);								/* new b										*/
							put_fcb (x, a);								/* new a										*/
						}
					}
					else if ((ams == b [f_ms]) && (als == b [f_ls])) {	/* hole b starts at end of a					*/
						a [f_ml] = a [f_ml] + b [f_ml];
						a [f_ll] = a [f_ll] + b [f_ll];
						if (_ILT_(a [f_ll], b [f_ll])) a [f_ml] = a [f_ml] + 1;
							
						if (_ILT_(a [f_ml], 0x0010))					/* if combined length will fit in block			*/
						{												/* then combine them							*/
							put_fcb (y, zero);							/* zero it										*/
							put_fcb (x, a);								/* new a										*/
						}
						else 											/* else limit to 2^20-1							*/
						{
							b [f_ms] = a [f_ms];
							b [f_ls] = a [f_ls];
							b [f_ml] = 0x000F;							/* limit length of b to 2^20-1					*/
							b [f_ll] = ((fixed) 0xFFFF);
							
							a [f_ms] = b [f_ms] + b [f_ml];				/* compute start of a to be at end of			*/
							a [f_ls] = b [f_ls] + b [f_ll];				/* truncated b									*/
							if (_ILT_(a [f_ls], b [f_ll]))
								a [f_ms] = a [f_ms] + 1;
								
							if (_ILT_(a [f_ll], ((fixed) 0xFFFF)))		/* and compute length of a to be that			*/
								a [f_ml] = a [f_ml] - 1;				/* part above 0x FF FFFF						*/
								
							a [f_ll] = a [f_ll] - ((fixed) 0xFFFF);
							a [f_ml] = a [f_ml] - 0x000F;
							
							put_fcb (y, b);								/* new b										*/
							put_fcb (x, a);								/* new a										*/
						}
					}
				}
			}									/* of looking through cat up to where we are	*/
		}
	}											/* of looking at all FCBs						*/
		
	return (true);								/* we consolidated it okay						*/
}
	
/* $subtitle Storage Management:  Find_Exact */

/* This (internal) procedure looks for a block exactly MS_SIZE|LS_SIZE
   .  sectors long.  If it finds one (and SEARCHING is TRUE), it returns
   .  that block.  Otherwise, it calculates the remaining sectors in the
   .  catalog (returned in MS_REMAINING|LS_REMAINING), finds the FCB number
   .  of the last free block (returned in LAST_FREE) along with its sector
   .  address (returned in MS_LASTFREE|LS_LASTFREE), and returns the FCB
   .  number of the first free block in the catalog (or -1 if there are
   .  no free blocks). */

static	fixed	find_exact(						/* find a block exactly MS_SIZE|LS_SIZE sectors long	*/
	fixed	ms_size,
	fixed	ls_size, 							/* size of block to find						*/
	boolean	searching)	_recursive				/* True if we're really searching for a match (i.e., MS_SIZE|LS_SIZE is valid)	*/
{
    fixed			_upper0;
	fixed			last_used;					/* Fcb number of last used block				*/
	fixed			ms_lastused;				/* sector address of last used block			*/
	fixed			ls_lastused;
	fixed			x, hole;
	
	hole = -1;									/* assume not found								*/
	last_free = -1; ms_lastfree = 0; ls_lastfree = 0;	/* initialize									*/
	last_used = -1; ms_lastused = 0; ls_lastused = 0;
	
	ms_remaining = c_ms_length;					/* so far the entire catalog is free			*/
	if (_ILT_(c_ls_length, shr(c_dir_size, 8))) ms_remaining = ms_remaining - 1;	/* borrow										*/
	ls_remaining = c_ls_length - shr(c_dir_size, 8);	/* except the directory							*/
	
	for (_upper0 = shr(c_dir_size, 3) - 1, x = 0; x <= _upper0; x++) {	/* look for perfect fit							*/
		if (! (get_fcb (x, fcb) & 1)) return (-1);	/* get next block								*/
			
		if (fcb [f_nm] == 0) {					/* hole											*/
			if (searching & 1 && (fcb [f_ml] == ms_size) && (fcb [f_ll] == ls_size)) return (x);	/* exact fit									*/
			if ((fcb [f_ml] == 0) && (fcb [f_ll] == 0)) {	/* if a hole									*/
				if (hole == -1) hole = x;		/* if first hole, keep it						*/
			}
			else {								/* has some storage with it						*/
				if ((_IGT_(fcb [f_ms], ms_lastfree))/* if FCB starts after last						*/
				|| ((fcb [f_ms] == ms_lastfree) && (_IGT_(fcb [f_ls], ls_lastfree)))) {
					ms_lastfree = fcb [f_ms]; ls_lastfree = fcb [f_ls]; last_free = x;
				}
			}
		}										/* of hole										*/
		else {									/* used											*/
			if ((_IGT_(fcb [f_ms], ms_lastused))/* if FCB starts after last_used				*/
			|| ((fcb [f_ms] == ms_lastused) && (_IGT_(fcb [f_ls], ls_lastused)))) {
				ms_lastused = fcb [f_ms]; ls_lastused = fcb [f_ls]; last_used = x;
			}
				
			ms_remaining = ms_remaining - fcb [f_ml];	/* subtract out used sectors					*/
			if (_ILT_(ls_remaining, fcb [f_ll])) ms_remaining = ms_remaining - 1;
			ls_remaining = ls_remaining - fcb [f_ll];
		}
	}
		
	if (hole == -1) c_status = e_dir_full;		/* if we didn't find a block, the directory is full	*/
	else if ((_ILT_(ms_lastfree, ms_lastused))	/* last thing isn't hole						*/
	|| ((ms_lastfree == ms_lastused) && (_ILT_(ls_lastfree, ls_lastused)))) {
		get_fcb (last_used, fcb2);
		ms_lastfree = ms_lastused + fcb2 [f_ml];	/* and this is where free stuff starts			*/
		ls_lastfree = ls_lastused + fcb2 [f_ll];
		if (_ILT_(ls_lastfree, ls_lastused)) ms_lastfree = ms_lastfree + 1;
		last_free = -1;							/* and there is no half filled block			*/
	}
	else if ((ms_lastfree == 0) && (ls_lastfree == 0)) {	/* nothing anywhere								*/
		ms_lastfree = 0; ls_lastfree = shr(c_dir_size, 8);	/* start saving files after directory			*/
		last_free = -1;							/* and no half filled blocks					*/
	}
		
	return (hole);								/* return FCB number of found block				*/
}
	
/* $subtitle Storage Management:  FindFree */


/* This (internal) procedure searches the catalog buffer for a free
   .  contiguous block of storage that has a size of MS_SIZE|LS_SIZE sectors.
   .  If such a block is found, the FCB number of that block is returned.
   .  Otherwise, a minus one (-1) is returned and C#STATUS contains
   .  the reason for failure.
   .
   .  The algorithm proceeds as follows:
   .     1. Consolidate free blocks.
   .     2. Look for a perfect fit - a free block that is the exact size.
   .     3. If there is no perfect fit, look for a large block and then
   .        divide the block into two halves - the one nearest the front
   .        of the disk is returned.
   .     4. If there is no large block, see if there is enough room
   .        between the last file stored and the end of the disk. */

static	fixed	findfree(						/* find contiguous free block for saving file	*/
	fixed	ms_size,
	fixed	ls_size)	_recursive				/* size of block to find						*/
{
    fixed			_upper0;
	fixed			x, blk, hole, ms, ls;
	
	if (_IGE_(ms_size, shl(1, 20 - 16))) {		/* file is larger than 2^20 sectors				*/
		c_status = e_too_large;					/* set status									*/
		return (-1);
	}
		
	if ((shr(c_ms_sector, 8) == 8) || (shr(c_ms_sector, 8) == 9))/* is this the tape drive?						*/
	{
		c_status = e_no_config;					/* set status									*/
		return (-1);
	}
		
	if (! (consolidate() & 1)) return (-1);		/* consolidate free blocks						*/
	hole = find_exact (ms_size, ls_size, true);	/* look for a block the right size				*/
	if (hole == -1) return (-1);				/* no room for it in directory					*/
		
	get_fcb (hole, a);							/* pick up FCB									*/
	if ((a [f_ml] == ms_size) && (a [f_ll] == ls_size)) return (hole);	/* exact fit									*/
		
	if ((_IGT_(ms_size, ms_remaining))			/* no room on the disk?							*/
	|| ((ms_size == ms_remaining) && (_IGT_(ls_size, ls_remaining)))) {
		c_status = e_storage;					/* no room										*/
		return (-1);
	}
		
	/* $page										*/
	
	
	/* perfect match not found:  look for a large free area on the disk	*/
	
	blk = -1;									/* have not found any							*/
	for (_upper0 = shr(c_dir_size, 3) - 1, x = 0; x <= _upper0; x++) {	/* look again									*/
		get_fcb (x, a);							/* get it										*/
		if ((a [f_nm] == 0) && ((_IGT_(a [f_ml], ms_size))/* hole found that's large enough				*/
		|| ((a [f_ml] == ms_size) && (_IGE_(a [f_ll], ls_size))))) {
			if ((blk == -1) || (_ILT_(a [f_ms], b [f_ms]))/* use one closest to front						*/
			|| ((a [f_ms] == b [f_ms]) && (_ILT_(a [f_ls], b [f_ls])))) {
				blk = x; get_fcb (blk, b);		/* get block for comparisons					*/
			}
		}
	}											/* of look again								*/
		
	if (blk != -1) {							/* one found - divide up and use it				*/
		get_fcb (hole, a);						/* get absolute hole							*/
		get_fcb (blk, b);						/* and get header with an empty block			*/
		a [f_ms] = b [f_ms]; a [f_ls] = b [f_ls];
		a [f_ml] = ms_size; a [f_ll] = ls_size;
		b [f_ms] = b [f_ms] + ms_size;
		b [f_ls] = b [f_ls] + ls_size;
		if (_ILT_(b [f_ls], ls_size)) b [f_ms] = b [f_ms] + 1;
		b [f_ml] = b [f_ml] - ms_size;
		if (_ILT_(b [f_ll], ls_size)) b [f_ml] = b [f_ml] - 1;
		b [f_ll] = b [f_ll] - ls_size;
		put_fcb (hole, a);
		put_fcb (blk, b);
		return (hole);							/* contains a block of desired size				*/
	}
		
		
	/* check for one at end of disk					*/
	
	if (last_free == -1) {						/* fill up disk									*/
		get_fcb (hole, a);
		a [f_ms] = ms_lastfree; a [f_ls] = ls_lastfree;	/* use last free block							*/
		a [f_ml] = ms_size; a [f_ll] = ls_size;	/* would you believe we just grabbed some storage	*/
		
		ms = c_ms_length - ms_lastfree;			/* calculate storage at end of disk				*/
		if (_ILT_(c_ls_length, ls_lastfree)) ms = ms - 1;
		ls = c_ls_length - ls_lastfree;
		
		if ((_IGT_(ms_size, ms))				/* will not fit									*/
		|| ((ms_size == ms) && (_IGT_(ls_size, ls)))) {
			c_status = e_cstorage;				/* not enough contiguous space on disk			*/
			return (-1);
		}
		put_fcb (hole, a);
		return (hole);
	}
		
	get_fcb (last_free, a);						/* half empty block at end of disk				*/
	a [f_ml] = ms_size; a [f_ll] = ls_size;		/* would you believe we just grabbed some storage	*/
	
	ms = c_ms_length - a [f_ms];				/* calculate storage at end of disk				*/
	if (_ILT_(c_ls_length, a [f_ls])) ms = ms - 1;
	ls = c_ls_length - a [f_ls];
	
	if ((_IGT_(ms_size, ms))					/* too big?										*/
	|| ((ms_size == ms) && (_IGT_(ls_size, ls)))) {
		c_status = e_cstorage;					/* not enough contiguous space on disk			*/
		return (-1);
	}
	put_fcb (last_free, a);
	
	return (last_free);
}
	
/* $subtitle Storage Management:  MaxFree */


/* This (internal) procedure searches the catalog buffer for the largest
   .  free block.  If such an FCB is found, the file variables are set to
   .  contain the relevant information and TRUE is returned.  Otherwise,
   .  FALSE is returned and C#STATUS contains the reason for failure. */

static	boolean	maxfree()	_recursive			/* find largest contiguous free block			*/
{
    fixed			_upper0;
	fixed			x, blk, ms, ls;
	
	if ((shr(c_ms_sector, 8) == 8) || (shr(c_ms_sector, 8) == 9)) {	/* is this the tape drive?						*/
		c_status = e_no_config;					/* set status									*/
		return (false);
	}											/* of tape drive								*/
	else {										/* not the tape drive							*/
		if (! (consolidate() & 1)) return (false);	/* consolidate free blocks						*/
		blk = find_exact (-1, -1, false);		/* see if there are any free blocks				*/
		if (blk == -1) return (false);			/* no room for it in directory					*/
			
			
		/* look in directory for largest free area		*/
		
		fcb2 [f_ml] = 0; fcb2 [f_ll] = 0;		/* zero length thus far							*/
		blk = -1;								/* have not found any							*/
		for (_upper0 = shr(c_dir_size, 3) - 1, x = 0; x <= _upper0; x++) {	/* look again									*/
			get_fcb (x, fcb);					/* get it										*/
			if ((fcb [f_nm] == 0) && ((_IGT_(fcb [f_ml], fcb2 [f_ml]))/* hole found that's large enough				*/
			|| ((fcb [f_ml] == fcb2 [f_ml]) && (_IGE_(fcb [f_ll], fcb2 [f_ll]))))) {
				blk = x; get_fcb (blk, fcb2);	/* get block for comparisons					*/
			}
		}										/* of look again								*/
			
		ms = c_ms_length - ms_lastfree;			/* calculate storage at end of disk				*/
		if (_ILT_(c_ls_length, ls_lastfree)) ms = ms - 1;
		ls = c_ls_length - ls_lastfree;
		
		if ((_IGT_(ms, fcb2 [f_ml]))			/* is the end block larger than what we found?	*/
		|| ((ms == fcb2 [f_ml]) && (_IGT_(ls, fcb2 [f_ll])))) {
			f_ms_sector = ms_lastfree; f_ls_sector = ls_lastfree;	/* save starting sector							*/
			f_ms_length = ms; f_ls_length = ls;	/* save sector length							*/
		}
		else {									/* we found the largest block					*/
			f_ms_sector = fcb2 [f_ms]; f_ls_sector = fcb2 [f_ls];	/* save starting sector							*/
			f_ms_length = fcb2 [f_ml]; f_ls_length = fcb2 [f_ll];	/* save sector length							*/
		}
			
		f_ms_sector = f_ms_sector + c_ms_sector;	/* add in catalog base							*/
		f_ls_sector = f_ls_sector + c_ls_sector;
		if (_ILT_(f_ls_sector, c_ls_sector)) f_ms_sector = f_ms_sector + 1;
	}											/* of not a tape drive							*/
		
	f_words = shl(f_ls_length, 8);				/* set number of words							*/
	
	return (true);
}
	
/* $subtitle Catalog Utilities:  Set_Size */


/* This (internal) procedure calculates the size (in sectors) of the
   .  catalog in the catalog buffer (which must be from the top level
   .  of some device).  It sets C#MS_LENGTH and C#LS_LENGTH based on this
   .  value.  If the size cannot be determined by this routine, then
   .  C#MS_LENGTH and C#LS_LENGTH are set to the passed MS_LENGTH and LS_LENGTH. */

static	void	set_size(						/* calculate device size (in sectors)			*/
	fixed	ms_length, 							/* Ms sector length of catalog estimate			*/
	fixed	ls_length)	_recursive				/* Ls sector length of catalog estimate			*/
{
	fixed               _upper0;
	fixed               device;                 /* device number of catalog in memory			*/
	fixed               config_ptr;             /* pointer to this device's stored configuration	*/
	fixed               type;                   /* the device's internally coded type			*/
	fixed               ms_sectors;             /* Ms sectors on this drive						*/
	fixed               ls_sectors;             /* Ls sectors on this drive						*/
	xpl_file_ref_num	ref_num;
	
	device = shr(c_ms_sector, 8);				/* pick up device								*/
	
	if (((c_ms_sector & 0x00FF) != 0) || (c_ls_sector != 0)) {	/* if not at top level of device				*/
		c_ms_length = ms_length;				/* we don't know the size						*/
		c_ls_length = ls_length;				/* so use user's estimate						*/
	}											/* of not at top level of device				*/
	else if ((device == 8) || (device == 9)) {	/* tape drive?									*/
		c_ms_length = -1;						/* the tape drive holds a lot					*/
		c_ls_length = -1;
	}
	else if ((ref_num = find_hfs_device(device)) != 0)
	{
        SInt64	its_length_bytes	= 0;

        #if __LP64__
            its_length_bytes = (long long) lseek(ref_num, 0, SEEK_END);
        
            if (its_length_bytes == -1)
                its_length_bytes = 0;
        #else
            FSGetForkSize(ref_num, &its_length_bytes);
        #endif

		c_ms_length = (fixed) (its_length_bytes >> (16+8+1));
		c_ls_length = (fixed) (its_length_bytes >> (   8+1));
	}

	else {										/* search the configuration						*/
		c_ms_length = 0; c_ls_length = 0;		/* initialize device size						*/
		config_ptr = find_device (device);		/* look up the device in the lowcore configuration	*/

		if (config_ptr != 0) {					/* if the device is there, compute how large it is	*/
			if ((c_status == e_none)			/* if no catalog errors							*/
			&&  (((able_core(config_ptr + s_devtyp) & 0x040F) == 0x0400)
			||   ((able_core(config_ptr + s_devtyp) & 0x040F) == 0x0402)))
			{									/* on a super floppy drive -- compute C#LENGTH from catalog	*/
				
				/* Since the floppy being used may be an old double density
				               .  disk, you need to count the number of sectors accounted for
				               .  in the catalog.  This assumes every sector on the disk will
				               .  be accounted for in the catalog. */
				
				for (_upper0 = c_dir_size - 1, type = 0; type <= _upper0; type += 8) {	/* add up sectors accounted for					*/
					if (c_bufmed == 0)			/* if the catalog buffer's in main memory		*/
						c_ls_length = c_ls_length + able_core(c_bufptr + type + c_ll);
					else {												/* it's in external memory						*/
						XPLimport (c_bufptr, type, fcb2, c_len);		/* read in the catalog entry					*/
						c_ls_length = c_ls_length + fcb2 [c_ll];
					}
				}								/* of adding up sectors accounted for			*/
				c_ls_length = c_ls_length + shr(c_dir_size, 8);	/* add in catalog								*/
			}
			else {								/* not a super floppy -- use config info		*/
				type = (able_core(config_ptr + s_devtyp) & 0x00FF);	/* get device type								*/
				
				while ((able_core(config_ptr + s_devtyp) & 0x00FF) == type) {	/* make sure we look at all entries for this device	*/
					if (d4567_present & 1) {							/* if they have a D4567, do it the easy way		*/
						_write_5(able_core(config_ptr + s_seccyl));		/* multiply sectors/cylinder					*/
						_write_6(able_core(config_ptr + s_totcyl));		/* by total cylinders							*/
						ms_sectors = _read_4; ls_sectors = _read_5;		/* to get total sectors							*/
					}
					else {												/* do it the hard way [this only works if both multiplicands are < 32768]	*/
						ms_sectors = fmul(able_core(config_ptr + s_seccyl),able_core(config_ptr + s_totcyl));	/* get MS sectors								*/
						ls_sectors = able_core(config_ptr + s_seccyl)*able_core(config_ptr + s_totcyl);	/* and LS sectors								*/
					}
						
					c_ms_length = c_ms_length + ms_sectors;	/* add in number of sectors on this drive		*/
					c_ls_length = c_ls_length + ls_sectors;
					if (_ILT_(c_ls_length, ls_sectors)) c_ms_length = c_ms_length + 1;
					config_ptr = config_ptr + s_blklen;	/* look at next entry							*/
				}
			}
		}										/* of computing size of device					*/
	}											/* of searching the configuration				*/   
}
	
/* $subtitle Catalog Utilities:  Check_Polycache */


/* This (internal) procedure checks to see if the catalog operation
   .  being performed invalidates the poly cache.  Basically, if a
   .  sound file or subcatalog is being operated on, the poly cache
   .  is considered invalid.  The file TYPE of the file being affected
   .  is passed. */

static	void	check_polycache(				/* trash poly cache if necessary				*/
	fixed	type)	_recursive					/* type of file being operated upon				*/
	
{
	if ((type == t_sound) || (type == t_lsubc) || (type == t_subc))		/* if sound file or subcatalog					*/
	{
		AbleCatRtns_SoundFileChanged = true;
	
		if (able_core(loc_emsize) != 0)			/* no poly cache if no external memory			*/
			XPLextset (able_core(loc_emarea), em_polycache, 1, false);	/* invalidate the cache							*/
	}											/* of have external memory						*/
}
	
/* $subtitle Catalog Caching:  Find_Cache */


/* This (internal) procedure searches the caches available for the
   .  catalog at MS_SECTOR|LS_SECTOR.  If it is found, the pointer to
   .  the cache variables is returned.  Otherwise, -1 is returned. */

static	fixed	find_cache(						/* search for catalog's cache					*/
	fixed	ms_sector, 							/* device/MS sector of catalog to read			*/
	fixed	ls_sector)	_recursive				/* Ls sector of catalog to read					*/
{
	boolean			found;						/* True if we find a cache						*/
	fixed			i, j;
	
	i     = caches;
	j     = 0;
	found = false;

	while ((_ILT_(j, max_caches)) && (! (found & 1))) {	/* thumb through caches							*/
		if ((able_core(i + __enabled) & 1)
		&&  (able_core(i + __ms_sector) == ms_sector)
		&&  (able_core(i + __ls_sector) == ls_sector))
			found = true;						/* we found the cache for this catalog			*/
		else {									/* keep looking									*/
			i = i + __vars;						/* look at next cache							*/
			j = j + 1;
		}
	}											/* of thumbing through caches					*/
		
	if (found & 1)								/* if we found it								*/
		return (i);								/* return an absolute pointer to it				*/
	
	else return (-1);							/* didn't find it								*/
}
	
/* $subtitle Catalog Caching:  Read_Cache, Write_Cache */


/* This (internal) procedure searches the caches available for the
   .  catalog at MS_SECTOR|LS_SECTOR.  If one is found, the catalog is
   .  read in from the cache and a boolean TRUE is returned.  Otherwise,
   .  a boolean FALSE is returned and the catalog must be read in from
   .  the disk. */

static	boolean	read_cache(						/* search for and read in cached catalog		*/
	fixed	ms_sector, 							/* device/MS sector of catalog to read			*/
	fixed	ls_sector, 							/* Ls sector of catalog to read					*/
	fixed	dir_size)	_recursive				/* word length of catalog directory				*/
{
    fixed			_upper0;
	fixed			i, j;
	
	i = find_cache (ms_sector, ls_sector);		/* find the cache for this catalog				*/
	
	if (i == (-1))								/* if can't, then don't...						*/
		return (false);
	
	if ((able_core(i + __bufptr) != c_bufptr)	/* skip blockmove/import/export if is catbuf	*/
	||  (shr(able_core(i + __bufmed), 8) != c_bufmed))
	{
		if (shr(able_core(i + __bufmed), 8) == 0) {	/* if cached in main memory						*/
			if (c_bufmed == 0)					/* if the catalog buffer is in main memory		*/
				blockmove (_location_(&ABLE_CONTEXT._able_memory_[able_core(i + __bufptr)]), _location_(&ABLE_CONTEXT._able_memory_[c_bufptr]), dir_size);	/* copy from main to main						*/
			else XPLexport (c_bufptr, 0, _location_(&ABLE_CONTEXT._able_memory_[able_core(i + __bufptr)]), dir_size);	/* copy from main to external					*/
		}										/* of cached in main memory						*/
		else {									/* cached in external memory					*/
			if (c_bufmed == 0)					/* if the catalog buffer is in main memory		*/
				XPLimport (able_core(i + __bufptr), 0, _location_(&ABLE_CONTEXT._able_memory_[c_bufptr]), dir_size);	/* copy from external to main					*/
			else for (_upper0 = dir_size - 1, j = 0; j <= _upper0; j += 16) {	/* copy from external to external				*/
				XPLimport (able_core(i + __bufptr), j, fcb, 16);	/* get a chunk from the cache					*/
				XPLexport (c_bufptr, j, fcb, 16);	/* and write it to the catalog buffer			*/
			}
		}										/* of cached in external memory					*/
	}											/* of reading it in								*/
		
	return (true);								/* return whether we read in the catalog		*/
}
	
	
/* This (internal) procedure searches the caches available for the
   .  catalog in the catalog buffer.  If one is found, the catalog is
   .  written to the cache and a boolean TRUE is returned.  Otherwise,
   .  a boolean FALSE is returned and the catalog must be written to
   .  the disk.  The cache search is preempted if a cache address other
   .  than -1 is passed. */

static	boolean	write_cache(					/* search for and write to cached catalog		*/
	fixed	i)	_recursive
{
    fixed			_upper0;
	static	fixed	j;
	
	if (i == -1)								/* if we should look for the cache				*/
	{
		i = find_cache (c_ms_sector, c_ls_sector);	/* find the cache for this catalog			*/
		
		if (i == -1)							/* no cache exists for this catolog				*/							
			return (false);						/* then must write it to dick					*/										
	}
	
	if ((able_core(i + __bufptr) == c_bufptr) && (shr(able_core(i + __bufmed), 8) == c_bufmed))
		return (true);							/* if cache is catalog buffer, no write needed  */
		
	if (i != -1) {								/* if we found such a cache, write it out		*/
		if (shr(able_core(i + __bufmed), 8) == 0) {	/* if cached in main memory						*/
			if (c_bufmed == 0)					/* if the catalog buffer is in main memory		*/
				blockmove (_location_(&ABLE_CONTEXT._able_memory_[c_bufptr]), _location_(&ABLE_CONTEXT._able_memory_[able_core(i + __bufptr)]), c_dir_size);	/* copy from main to main						*/
			else XPLimport (c_bufptr, 0, _location_(&ABLE_CONTEXT._able_memory_[able_core(i + __bufptr)]), c_dir_size);	/* copy from external to main					*/
		}										/* of cached in main memory						*/
		else {									/* cached in external memory					*/
			if (c_bufmed == 0)					/* if the catalog buffer is in main memory		*/
				XPLexport (able_core(i + __bufptr), 0, _location_(&ABLE_CONTEXT._able_memory_[c_bufptr]), c_dir_size);	/* copy from main to external					*/
			else for (_upper0 = c_dir_size - 1, j = 0; j <= _upper0; j += 16) {	/* copy from external to external				*/
				XPLimport (c_bufptr, j, fcb, 16);	/* get a chunk from the catalog buffer			*/
				XPLexport (able_core(i + __bufptr), j, fcb, 16);	/* and write it to the cache					*/
			}
		}										/* of cached in external memory					*/
	}											/* of writing it out							*/
		
	return (i != -1);							/* return whether we write out the catalog		*/
}
	
/* $subtitle Treename Caching:  Store_Catalog, Retrieve_Catalog */


/* This procedure saves information about this level of the treename
   .  so that it is possible traverse the same tree again without looking
   .  up the addresses of the intermediate levels from the disk. */

static	void	store_catalog(					/* save a catalog at this DEPTH					*/
	fixed	depth, 								/* depth (level) of catalog we're saving		*/
	fixed	level)	_recursive					/* level of catalog treename starts in			*/
{
    fixed			_upper0;
	fixed			i, j;
	
	if ((path_cached & 1) && (depth > 0) && (depth <= max_depth)) {	/* make sure we don't overflow the table		*/
		if (depth == 1) {						/* if we're on level one						*/
			topmost_level = level;				/* remember catalog we started from				*/
			name [0] = 0;						/* no names known - point to start of NAMES		*/
		}										/* of level one									*/
			
		path_depth = depth;						/* we now know about one more level				*/
		ms_sector [depth - 1] = c_ms_sector;	/* save the starting sector						*/
		ls_sector [depth - 1] = c_ls_sector;
		ms_length [depth - 1] = c_ms_length;	/* save the sector length						*/
		ls_length [depth - 1] = c_ls_length;
		dir_siz   [depth - 1] = c_dir_size;		/* and the directory size						*/
		
		j = name [depth - 1];					/* start NAMES pointer							*/
		for (_upper0 = shr(f_name [0] + 1, 1), i = 0; i <= _upper0; i++) {	/* copy the name into global area				*/
			if (j >= ((fixed) (max_depth*(f_name_len + 1))))/* did we overflow?								*/
			{									/* yes, clean up								*/
				path_depth = depth - 1;			/* we know about one less level					*/
				i = f_name [0];					/* spring out of this loop						*/
			}
			else {								/* no overflow - copy the name over				*/
				names [j] = f_name [i];			/* word for word								*/
				j = j + 1;						/* point to next available word					*/
			}
		}										/* of copying name into global area				*/
			
		if (depth < max_depth)					/* if next one still in range					*/
			name [depth] = j;					/* save start of next name						*/
	}											/* of not overflowing table						*/
}
	
	
/* This procedure retrieves information about the current level of the
   .  treename if said information was previously saved.  A boolean TRUE
   .  is returned if the information was retrieved. */

static	boolean	retrieve_catalog(				/* search for and load a saved catalog at this DEPTH	*/
	fixed	depth, 								/* depth (level) of catalog we're looking for	*/
	fixed	level)	_recursive					/* level of catalog treename starts in			*/
{
	boolean			matched;					/* True if F#NAME matches the previously saved name at the same depth	*/
	fixed			i;
	
	if ((path_cached & 1) && (depth > 0) && (depth <= path_depth)/* is there a known catalog at this level?		*/
	&& (f_name [0] == names [name [depth - 1]]))/* and can the names match?						*/
	{											/* yes, see if it matches						*/
		matched = true; i = 0;					/* assume a match								*/
		
		while (matched & 1 && (i < f_name [0])) {	/* see if the name matches at this level		*/
			if (byte(f_name, i) != byte(_location_(&(names [name [depth - 1]])), i))
				matched = false;				/* no match										*/
			i = i + 1;
		}
			
		if (depth == 1)							/* are we on the first level?					*/
			matched = (matched & (topmost_level == level));	/* yes, this is dependent on the start			*/
			
		if (matched & 1)						/* did they match?								*/
		{										/* yes, get the catalog location				*/
			c_ms_sector = ms_sector [depth - 1];	/* retrieve the starting sector					*/
			c_ls_sector = ls_sector [depth - 1];
			c_ms_length = ms_length [depth - 1];	/* retrieve the starting sector					*/
			c_ls_length = ls_length [depth - 1];
			c_dir_size  = dir_siz   [depth - 1];	/* and the directory size						*/
		}
	}											/* of known catalog at this level				*/
	else matched = false;						/* no known catalog at this level				*/
		
	return (matched);							/* return whether we found a match and loaded location	*/
}
	
/* $subtitle Treename Scanning:  Search */


/* This procedure finds the file pointed to by TREENAME (starting at
   .  character position INDEX).  The search commences from the catalog
   .  specified by LEVEL.  A boolean TRUE is returned is the catalog
   .  containing the file is found.  The search proceeds as follows:
   .     1) Scan a SUBNAME off the TREENAME (SUBNAME delimited by a colon or end of string).
   .     2) Find SUBNAME in last catalog read.
   .     3) If a colon was found, goto 1. with catalog SUBNAME in catalog buffer.
   .
   .  At the conclusion of a successful search, F#NAME contains the name
   .  of the final file and the catalog variables are set to the catalog
   .  containing the final file.  The final file is NOT located. */

static	boolean read_cat(fixed);

	/* This procedure scans the next filename off of the treename.  This
	      .  filename is converted to uppercase and stored in F#NAME.  A boolean
	      .  TRUE is returned if a COLON was found after the name.  Otherwise,
	      .  a boolean FALSE is returned. */
	
static	boolean	search_scan_name(	_recursive	/* scan the next SUBNAME from TREENAME			*/
	fixed *index,
	fixed treename[])
{
	fixed			sublen;						/* subname length								*/
	fixed			c;							/* next character from treename					*/
	boolean			gotcolon;					/* True if we find a colon						*/
	
	sublen   = 0;								/* initialize the character count				*/
	gotcolon = false;							/* no colon found yet							*/
	
    while ((*index < treename [0]) && (sublen <= shl(f_name_len, 1)) && (! (gotcolon & 1))) {	/* scan off the name							*/
    	if (byte(treename, *index) == a_colon)	/* is this a colon?								*/
			gotcolon = true;					/* yes, remember we found it					*/
		else {									/* we didn't find it - keep copying				*/
			if (sublen < shl(f_name_len, 1)) {	/* make sure we don't overflow SUBNAME (need to go one past to get the COLON)	*/
    			c = byte(treename, *index);		/* get next character from treename				*/
				if (_ILE_(c - l_a, (l_z - l_a))) c = c - (l_a - a_a);	/* uppercase it									*/
				pbyte(f_name, sublen, c);		/* copy the character into SUBNAME				*/
			}
			else *index = *index - 1;	/* don't pont past this character if we didn't process it	*/
				
			sublen = sublen + 1;				/* count characters in SUBNAME					*/
		}
    	*index = *index + 1;					/* point to next character in TREENAME			*/
	}
		
	if (sublen > shl(f_name_len, 1))			/* the filename or catalog name must be <= 2*F#NAME_LEN chars long	*/
		sublen = shl(f_name_len, 1);			/* so truncate it								*/
		
	f_name [0] = sublen;						/* set the character length						*/
	
	return (gotcolon);							/* return whether we found a colon				*/
}
	
static	boolean	search(							/* open the passed treename starting in the specified catalog	*/
	fixed	treename[], 						/* treename of file to find						*/
	fixed	index, 								/* character of treename to start scan from		*/
	fixed	level, 								/* catalog number of catalog to start looking in	*/
	boolean	catalog)							/* True if we're reading a catalog, rather than finding a file	*/
	
	
{
	/* $page */
	
	
	fixed			depth;						/* current pathname level being processed		*/
	fixed			start;						/* starting value of INDEX						*/
	boolean			gotcolon;					/* True if we scan a colon						*/
	boolean			matched;					/* True if we have a known catalog at DEPTH		*/
	boolean			found;						/* True if we've found the specified file		*/
	
	depth = 1;									/* the default first level is the specified catalog	*/
	start = index;								/* save starting value of index					*/
	matched = true;								/* assume match to read in first catalog		*/
	found = true;								/* assume we've found the file					*/
	
	while (found & 1 && (index < treename [0])) {		/* open every level of the treename		*/
		gotcolon = search_scan_name(&index, treename);	/* scan off next name					*/
		
		if (((! (gotcolon & 1)) && (index < treename [0]))/* if there's no colon after an intermediate name	*/
		|| (gotcolon & 1 && (index == treename [0]) && (index != start + 1))/* or there's a colon at the end (except leading colons)	*/
		|| ((f_name [0] == 0) && (index != start + 1))) {	/* or there's a missing name in the middle		*/
			c_status = e_treename;				/* incorrect format for treename				*/
			found = false;						/* didn't find it								*/
		}
		else if (f_name [0] == 0) depth = 0;	/* start from the top w/leading colon			*/
		else {									/* look up next name							*/
			if ((gotcolon & 1 || catalog & 1) && retrieve_catalog (depth, level) & 1) {	/* if this isn't the last name					*/
				if (! (gotcolon & 1)) {			/* if no colon, must be last cat of a cat lookup	*/
					if (! (readcat (c_ms_sector, c_ls_sector, c_dir_size, c_ms_length, c_ls_length) & 1)) return (false);
				}
				matched = true;					/* we retrieved a known catalog from this level	*/
			}									/* of if isn't last name						*/
			else {								/* force a READ CATALOG at end or if no match	*/
				if (matched & 1) {				/* if previous name matched and this one doesn't, haven't read in cat yet	*/
					if (depth == 1) {									/* if haven't read in starting cat yet			*/
						if (! (read_cat (level) & 1)) return (false);	/* read in first catalog						*/
					}
					else {												/* read in catalog from last level				*/
						if (! (readcat (c_ms_sector, c_ls_sector, c_dir_size, c_ms_length, c_ls_length) & 1)) return (false);
					}
				}								/* of previous name matched; read in first cat	*/
				matched = false;				/* no match										*/
			}
				
			if ((gotcolon & 1 || catalog & 1) && (! (matched & 1))) {	/* if don't have a known catalog at this level	*/
				if (readdir (f_name) & 1)		/* find/read in the catalog						*/
					store_catalog (depth, level);	/* keep the catalog table up to date			*/
				else found = false;				/* probably not there							*/
			}									/* of no known catalog at this level			*/
		}										/* of look up next name							*/
			
		depth = depth + 1;						/* we're about to scan another level			*/
	}											/* of opening every level of treename			*/
		
	if (depth == 1) {							/* if haven't read in top level yet (i.e., TREENAME = ':')	*/
		if (! (read_cat (level) & 1)) return (false);	/* read in first catalog						*/
	}
		
	return (found);								/* return whether or not we found it			*/
}
	
/* $subtitle Treename Scanning:  Device_Specified */


/* This procedure identifies any device specification in the TREENAME
   .  and returns the level number associated with that device.  If no
   .  device is specified, a zero is returned. */

static	fixed	device_specified_upto;			/* char position of ':' after device name       */

static	fixed	device_specified(				/* get any remote device specification			*/
	fixed	treename[])	_recursive				/* treename to scan device from					*/
{
	fixed			c;							/* next device name character					*/
	fixed			i;
	
	unsigned char p_name[64] = {0};				/* construct pascal string copy of name         */
	
	i = 0;
	while (i < treename [0] && i < 60 && (byte(treename, i) != a_colon))
		p_name[++p_name[0]] = byte(treename, i++);
		
	if ((treename [0] < 3) || (byte(treename, 2) != a_colon))/* device is 2 chars and a colon				*/
		return (0);								/* no device specified							*/
		
	f_name [0] = 2; f_name [1] = treename [1];	/* pick up suspected device name				*/
	
	c = byte(f_name, 0);						/* get first character							*/
	if (_ILE_(c - l_a, (l_z - l_a))) c = c - (l_a - a_a);	/* uppercase it									*/
	pbyte(f_name, 0, c);						/* replace first character uppercased			*/
	
	if (c == a_f) i = 2;						/* floppy?										*/
	else if (c == a_r) i = 4;					/* remote?										*/
	else if (c == a_w) i = 6;					/* winchester?									*/
	else if (c == a_t) i = 8;					/* tape?										*/
	else return (0);							/* no device specified							*/
		
	c = byte(f_name, 1);						/* get second character							*/
	
	if (c ==  a_1) i = (i | 1);					/* or in drive bit if drive one					*/
	else if (c != a_0) return (0);				/* if not zero, no device specified				*/
	
	device_specified_upto = 2;					/* two characters scanned (e.g. W0)				*/
	
	return (i);									/* we found a legal device specification - return device	*/
}
	
/* $subtitle Treename Scanning:  Get_Catalog */


/* This procedure gets the catalog that contains the file specified by
   .  TREENAME.  The search begins on LEVEL.  If a device name is specified
   .  in the treename, it overrides the passed level number.  If the catalog
   .  is found, a boolean TRUE is returned, the catalog variables contain
   .  information about this catalog, and F#NAME is the name of the file.
   .
   .  The algorithm for finding the catalog is:
   .     1) If a device name is specified, start search on THAT device.
   .     2) Else if treename starts with a colon, start search on the
   .        device the specified catalog (LEVEL) is stored on.
   .     3) Else start search on specified catalog (LEVEL). */

static	boolean	get_catalog(					/* get last catalog of TREENAME starting at LEVEL	*/
	fixed	treename[], 						/* the treename of the file to locate			*/
	fixed	level, 								/* level of catalog to start searching from (ignored in device specified in TREENAME)	*/
	boolean	catalog)	_recursive				/* True if we're reading a catalog, rather than finding a file	*/
{
	fixed			index;						/* initial character index into TREENAME		*/
	fixed			device;						/* number of any specified device				*/
	boolean			found;						/* True if we find the file						*/
	
	c_status = e_none;							/* no errors yet								*/
	f_name [0] = 0;								/* there is no name either						*/
	
	if (treename [0] == 0) {					/* check for an empty treename					*/
		c_status = e_treename;					/* this is invalid								*/
		found = false;							/* didn't find it								*/
	}
	else {										/* there's a treename							*/
		index = 0;								/* start at beginning of treename				*/
		device = device_specified (treename);	/* pick up any device specified					*/
		
		if (device != 0) {						/* if a device name was specified				*/
			level = device;						/* start searching on specified device			*/
			index = device_specified_upto;		/* start scanning after device name				*/
		}
		else {									/* no device name specified						*/
			if (level < 2)						/* if special level (actually, 0 and 1 aren't special, but we need this info)	*/
			switch (level + 2) {				/* determine device number of specified catalog	*/
				case 0:
					device = shr(able_core(loc_pcat + 1), 8);	/* path catalog									*/
					break;
				case 1:
					device = shr(a_ms_sector, 8);	/* alternate catalog							*/
					break;
				case 2:
					device = shr(able_core(loc_scat + 1), 8);	/* system catalog								*/
					break;
				case 3:
					device = shr(able_core(loc_ucat + 1), 8);	/* user catalog									*/
					break;
			}									/* of decoding device number of specified catalog	*/
			else device = level;				/* level number matches the device number here	*/
				
			if (byte(treename, 0) == a_colon)	/* if name starts with colon					*/
				level = device;					/* use device number of specified catalog		*/
		}										/* of no device name specified					*/
		
		if (find_hfs_device(device))			// if is hfs device, search
			found = search (treename, index, level, catalog);	/* open file on LEVEL; start scanning at INDEX	*/
		else if (find_device (device) == 0) {	/* if the specified device isn't configured		*/
			c_status = e_no_config;				/* don't start scanning							*/
			found = false;						/* needless to say, we didn't find the file		*/
		}
		else found = search (treename, index, level, catalog);	/* open file on LEVEL; start scanning at INDEX	*/
	}											/* of there's a treename						*/
		
	return (found);								/* return whether we found it					*/
}

/* $subtitle Cache Interface:  Cache, Cache_Treename */


/* This procedure caches the catalog buffer into the buffer pointed
   .  to by BUFPTR and sets up the catalog routines to treat that buffer
   .  as equivalent to whatever catalog was originally read into the
   .  catalog buffer.  It returns the cache number for this buffer.  If
   .  there are no available caches, a minus one (-1) is returned. */

fixed			cache(							/* cache contents of catalog buffer				*/
	fixed	bufptr, 							/* pointer to new cache							*/
	fixed	bufmed)	_recursive					/* cache media:  0 - main memory, 1 - external memory	*/
{
	boolean			found;						/* True if we find a free cache					*/
	fixed			i, j;
	
	i = caches; j = 0; found = false;
	while ((_ILT_(j, max_caches)) && (! (found & 1))) {	/* thumb through caches							*/
		if (able_core(i + __enabled) & 1) {		/* if this one's in use							*/
			i = i + __vars;						/* look at next cache							*/
			j = j + 1;
		}
		else found = true;						/* we found a cache for this catalog			*/
	}											/* of thumbing through caches					*/
		
	if (found & 1) {							/* if we found one a free set of cache variables*/
		if ((bufptr == c_bufptr) && (bufmed == c_bufmed)) {	/* if we're turning the catalog buffer into a cache	*/
			set_able_core(i + __ms_sector, -1);	/* invalidate the entry so it caches after the next READCAT	*/
			set_able_core(i + __ls_sector, -1);
			set_able_core(i + __dir_size, -1);
			catalog_cache = j;					/* remember which cache it is to keep it up-to-date	*/
		}
		else {									/* not caching the catalog buffer in itself		*/
			if ((c_bufptr == -1) || ((c_ms_sector == -1) && (c_ls_sector == -1)))/* we must have a catalog here					*/
				return (-1);					/* oops											*/
				
			set_able_core(i + __ms_sector, c_ms_sector);	/* remember where it came from					*/
			set_able_core(i + __ls_sector, c_ls_sector);
			set_able_core(i + __dir_size, c_dir_size);	/* remember the directory length				*/
			
			if (j == catalog_cache) catalog_cache = -1;	/* the catalog cache has been reassigned		*/
		}
			
		set_able_core(i + __enabled, (shl(bufmed, 8) | 1));	/* set media and enable it						*/
		set_able_core(i + __bufptr, bufptr);	/* save cache address							*/
		
		write_cache (i);						/* cache it										*/
		
		return (j);								/* return cache number							*/
	}											/* of found a free cache						*/
	else return (-1);							/* no free caches								*/
}
	
	
/* It is possible for the intermediate levels of a treename to be
   .  cached so that successive searches down part or all of the same
   .  treename can proceed without accessing the disk.  This procedure
   .  enables or disables this feature (it is initially disabled).
   .  Whenever this procedure is called with ENABLE set to TRUE,
   .  the currently cached treename is forgotten. */

void			cache_treename(					/* enable/disable treename caching				*/
	boolean	enable)	_recursive					/* True to enable treename caching				*/
	
{
	path_cached = enable;
	
	if (path_cached & 1)						/* if we just enabled caching					*/
		path_depth = 0;							/* forget currently cached treename (if any)	*/
}
	
/* $subtitle Cache Interface:  Reinit_Cache, Flush_Cache */


/* The catalog routines can handle up to two caches normally.  This
   .  is because the caching code uses a number of state variables to
   .  maintain the caches and only allocates enough state variables for
   .  two caches.  If more than two caches are needed, it is necessary
   .  to provide the catalog routines with a buffer to store the state
   .  variables for these caches.  This procedure allows you to provide
   .  this buffer.  N is the number of caches desired and BUFFER is the
   .  storage area for the state variables.  BUFFER must be N*$_VARS
   .  words long. */

/* note: buffer is a pointer to a location of able_core */
/* at least until we modify this!!!						*/

void			reinit_cache(					/* reinitialize caching to provide for N caches	*/
	fixed	n, 									/* number of caches to create					*/
	fixed	buffer)	_recursive					/* cache state variable storage area			*/
{
	fixed			i, j;
	
	max_caches = n;								/* save the number of caches created			*/
	caches = buffer;							/* and pointer to the state variables			*/
	
	i = caches;
	for (j = 0; j < max_caches; j++) {			/* initialize all caches						*/
		set_able_core(i + __enabled, (~ 1));	/* disable them all								*/
		i = i + __vars;							/* look at next cache							*/
	}
}
	
	
/* This procedure causes cache number N (set up with CACHE above) to
   .  be flushed to disk/tape.  The cache remains "enabled".  If there
   .  is no cache with number N, nothing happens.  If the cache cannot
   .  be written a boolean FALSE is returned and C#STATUS contains the
   .  error. */

boolean			flush_cache(					/* flush cache N to disk/tape					*/
	fixed	n)	_recursive						/* cache to flush								*/
{
	fixed			i;							/* pointer to cache N							*/
	
	if (_ILT_(n, max_caches)) {					/* if it's in range								*/
		i = caches + n*__vars;					/* compute pointer to cache N					*/
		
		if (able_core(i + __enabled) & 1) {		/* if the cache is enabled						*/
			n = disk_check (shr(able_core (i + __ms_sector), 8));	/* check floppy disk							*/
			
			if (n == 0) {						/* no floppy in drive?							*/
				c_status = e_no_floppy;
				return (false);
			}
				
			if ((n & d_protect) != 0) {			/* write protected floppy?						*/
				c_status = e_protect;
				return (false);
			}
				
			if (shr(able_core(i + __bufmed), 8) == 0)/* if cached in main memory						*/
				writedata (able_core(i + __ms_sector), able_core(i + __ls_sector), _location_(&ABLE_CONTEXT._able_memory_[able_core(i + __bufptr)]), able_core(i + __dir_size));	/* write from main to disk						*/
			else {								/* cached in external memory					*/
				fcb [0] = able_core(i + __bufptr); fcb [1] = 0;	/* set external memory sector					*/
				fcb [2] = 0; fcb [3] = able_core(i + __dir_size);	/* set length to write							*/
				extwrite (able_core(i + __ms_sector), able_core(i + __ls_sector), fcb);	/* write from external to disk					*/
			}									/* of external memory							*/
		}										/* of cache enabled								*/
	}											/* of in range									*/
		
	return (true);								/* wrote it alright								*/
}
	
/* $subtitle Cache Interface:  Disable_Cache, Enable_Cache */


/* This procedure temporarily disables the caching function for
   .  the catalog associated with cache number N.  If there is no
   .  cache with number N, nothing happens.
   .
   .  Warning:  If CACHE is called while some cache is disabled, the
   .            disabled cache may be reassigned the new catalog. */

void			disable_cache(					/* disable cache N								*/
	fixed	n)	_recursive						/* cache to disable								*/
	
{
	if (_ILT_(n, max_caches))					/* if it's in range								*/
		set_able_core((caches + n*__vars) + __enabled, (able_core((caches + n*__vars) + __enabled) & ((fixed) 0xFF00)));	/* disable it									*/
}
	
	
/* This procedure enables (after a disable) the caching function for
   .  the catalog associated with cache number N.  If there is no cache
   .  with number N, nothing happens. */

void			enable_cache(					/* enable cache N								*/
	fixed	n)	_recursive						/* cache to enable								*/
{
	fixed			i;
	
	if (_ILT_(n, max_caches)) {					/* if it's in range								*/
		i = caches + n*__vars;					/* get cache pointer							*/
		
		if (able_core(i + __enabled) != (~ 1)) {	/* if the cache exists							*/
			set_able_core(i + __enabled, ((able_core(i + __enabled) & ((fixed) 0xFF00)) | 1));	/* enable it									*/
			
			if (n == catalog_cache) {			/* if it's the catalog buffer cache				*/
				set_able_core(i + __ms_sector, -1);	/* invalidate the entry so it caches after the next READCAT	*/
				set_able_core(i + __ls_sector, -1);
				set_able_core(i + __dir_size, -1);
			}
		}										/* of cache exists								*/
	}											/* of in range									*/
}
	
/* $subtitle Buffer Interface:  ReadCat */

/* This procedure causes the catalog at the specified MS_SECTOR|LS_SECTOR
   .  to be read into the catalog buffer.  All of the catalog variables are
   .  set by this routine.  If READCAT is unable to determine C#MS_LENGTH
   .  and C#LS_LENGTH (e.g., for subcatalogs), they will be set to the
   .  passed MS_LENGTH and LS_LENGTH.  If there is no catalog buffer, a
   .  boolean FALSE is returned. */

boolean			readcat(						/* read in a catalog							*/
	fixed	ms_sector, 							/* device/MS sector of catalog to read			*/
	fixed	ls_sector, 							/* Ls sector of catalog to read					*/
	fixed	dir_size, 							/* word length of catalog directory				*/
	fixed	ms_length, 							/* Ms sector length of catalog estimate			*/
	fixed	ls_length)	_recursive				/* Ls sector length of catalog estimate			*/
{
	fixed			i, j;
	
	c_status = e_none;							/* no errors yet								*/
	
	if (able_core(loc_magic) != ((fixed) 12345)) {	/* check for correct operating system			*/
		c_status = e_os;
		return (false);
	}
		
	if (c_bufptr == -1) {						/* if no buffer yet, we're in trouble			*/
		c_status = e_buffer;
		return (false);
	}
	
	if (find_hfs_device(shr(ms_sector, 8)))		// If mac HFS device, is ok
		;

	else if (find_device (shr(ms_sector, 8)) == 0) {	/* not configured?								*/
		c_status = e_no_config;
		return (false);
	}
		
	if (! (read_cache (ms_sector, ls_sector, dir_size) & 1)) {	/* if it isn't cached							*/
		if (disk_check (shr(ms_sector, 8)) == 0) {	/* no floppy in drive?							*/
			c_status = e_no_floppy;
			return (false);
		}
			
		if (c_bufmed == 0)						/* if buffer in main memory						*/
			readdata (ms_sector, ls_sector, _location_(&ABLE_CONTEXT._able_memory_[c_bufptr]), dir_size);	/* read from disk to main						*/
		else {									/* buffer in external memory					*/
			fcb [0] = c_bufptr; fcb [1] = 0;	/* set external memory sector					*/
			fcb [2] = 0; fcb [3] = dir_size;	/* set length to read							*/
			extread (ms_sector, ls_sector, fcb);	/* read from disk to external					*/
		}										/* of external memory							*/
	}											/* of isn't cached								*/
		
	if (catalog_cache != -1) {					/* if the catalog buffer is being used as a cache	*/
		i = caches + catalog_cache*__vars;		/* get cache pointer							*/
		if (able_core(i + __enabled) & 1) {		/* if enabled									*/
			set_able_core(i + __ms_sector, ms_sector);	/* remember where it came from					*/
			set_able_core(i + __ls_sector, ls_sector);
			set_able_core(i + __dir_size, dir_size);	/* remember the directory length				*/
		}
	}											/* of catalog buffer being used as a cache		*/
		
	c_ms_sector = ms_sector;					/* remember device/MS sector					*/
	c_ls_sector = ls_sector;					/* and LS sector								*/
	c_dir_size = dir_size;						/* and the directory length						*/
	
	i = 0;
	while ((c_status == e_none) && (i < shr(c_dir_size, 3))) {	/* check validity - do BEFORE size for floppies	*/
		get_fcb (i, fcb);						/* get next FCB									*/
		
		for (j = f_nm; j < f_nm + f_name_len; j++) {	/* check the name								*/
			if ((fcb [j] & ((fixed) 0x8080)) != 0)/* make sure the name's okay					*/
				c_status = e_invalid;			/* invalid catalog								*/
				
            if (fcb [j] == 0x6363)				/* blank optical media 							*/
            	c_status = e_invalid; 			/* invalid catalog 								*/
		}
			
		i = i + 1;
	}											/* of checking validity							*/
		
	set_size (ms_length, ls_length);			/* try to figure out the size					*/
	
	return (c_status == e_none);				/* we got the catalog (it may be valid)			*/
}
	

/* $subtitle Buffer Interface:  get_device_size */

/* This procedure sets the catalog variables to get the overall size of a device */

boolean			get_device_size(				/* get catalog size for a device				*/
	fixed		the_device)	_recursive			/* device in question							*/
{
	c_ms_sector = 0;
	c_ls_sector = 0;
	c_dir_size  = 0;
	c_status    = e_none;
	
	if (the_device < 2) {						/* disallow for 0 and 1							*/
		c_status = e_no_config;
		return (false);
	}
			
	if (able_core(loc_magic) != ((fixed) 12345)) {	/* check for correct operating system			*/
		c_status = e_os;
		return (false);
	}
	
	if (find_hfs_device(the_device))			// If mac HFS device, is ok
		;

	else if (find_device (the_device) == 0) {	/* not configured?								*/
		c_status = e_no_config;
		return (false);
	}
		
	c_ms_sector = shl(the_device, 8);
	c_ls_sector = 0;
					
	if (find_hfs_device(the_device))			// image file on mac (t_lsubc)
		c_dir_size = 1024;
	else if ((the_device == 6) || (the_device == 7)) c_dir_size = 1024;	/* for winchester								*/
	else c_dir_size = 256;						/* otherwise 256 words (floppies)				*/
	
	c_ms_length = 0;							/* estimate sector length						*/
	c_ls_length = shr(c_dir_size, 8);			/* restrict to directory size					*/

	set_size (c_ms_length, c_ls_length);		/* try to figure out the size					*/
	
	return (c_status == e_none);				/* we got the catalog (it may be valid)			*/
}


/* $subtitle Buffer Interface:  Set_CatBuf, WriteCat */


/* This sets the catalog buffer for all subsequent catalog operations to
   .  the buffer pointed to by BUFPTR.  It is the user's responsibility to
   .  make sure the buffer is large enough to hold the catalog being read.
   .  After this is called, C#BUFPTR points to the catalog buffer. */

void			set_catbuf(						/* set the catalog buffer pointer				*/
	fixed	bufptr, 							/* catalog buffer pointer						*/
	fixed	bufmed)	_recursive					/* catalog buffer media:  0 - main memory, 1 - external memory	*/
	
{
	c_bufptr = bufptr;							/* set up pointer								*/
	c_bufmed = bufmed;							/* set media									*/
	c_ms_sector = -1; c_ls_sector = -1;			/* no catalog in memory yet						*/
	f_ms_sector = -1; f_ls_sector = -1;			/* be paranoid									*/
}
	
	
/* This procedure writes the catalog buffer to the MS_SECTOR|LS_SECTOR
   .  it was last read from.  It returns FALSE if there's no buffer. */

boolean			writecat()	_recursive			/* write out the catalog buffer					*/
{
	fixed			i;
	
	if (c_bufptr == -1) {						/* if no buffer									*/
		c_status = e_buffer;
		return (false);
	}
		
	if ((c_ms_sector == -1) && (c_ls_sector == -1)) {	/* if no catalog								*/
		c_status = e_no_dir;
		return (false);
	}
		
	if (! (write_cache (-1) & 1)) {				/* if it isn't cached							*/
		i = disk_check (shr(c_ms_sector, 8));	/* check floppy disk							*/
		
		if (i == 0) {							/* no floppy in drive?							*/
			c_status = e_no_floppy;
			return (false);
		}
			
		if ((i & 0x0002) != 0) {				/* write protected floppy?						*/
			c_status = e_protect;
			return (false);
		}

		if (c_bufmed == 0)						/* if buffer in main memory						*/
			writedata (c_ms_sector, c_ls_sector, _location_(&ABLE_CONTEXT._able_memory_[c_bufptr]), c_dir_size);	/* write from main to disk						*/
		else {									/* buffer in external memory					*/
			fcb [0] = c_bufptr; fcb [1] = 0;	/* set external memory sector					*/
			fcb [2] = 0; fcb [3] = c_dir_size;	/* set length to write							*/
			extwrite (c_ms_sector, c_ls_sector, fcb);	/* write from external to disk					*/
		}										/* of external memory							*/
	}											/* of isn't cached								*/
		
	return (true);								/* we wrote it okay								*/
}
	
/* $subtitle Buffer Interface:  FindFile */


/* This procedure searches the catalog buffer for the file with name
   .  NAME.  If it is found, the FCB number for the file is returned and
   .  all the file variables are set.  Otherwise, a minus one (-1) is
   .  returned (and C#STATUS may contain an error).  After calling
   .  FINDFILE, the global variable FNAME contains the passed NAME
   .  in FCB format. */

fixed			findfile(						/* find a file in the catalog buffer			*/
	fixed	name[])	_recursive					/* name of file to find							*/
{
	boolean			found;						/* True if we find it							*/
	fixed			i, j;
	
	if (name [0] == 0) {						/* null filename not allowed					*/
		c_status = e_name;						/* bad filename									*/
		return (-1);							/* not found									*/
	}
		
	clean_filename (name, fname);				/* clean up the name							*/
	
	i = -1; found = false;
	while ((! (found & 1)) && (i < shr(c_dir_size, 3) - 1)) {	/* search the entire catalog					*/
		i = i + 1;								/* look at next FCB								*/
		if (! (get_fcb (i, fcb) & 1)) return (-1);	/* get the next FCB								*/
			
		found = true;							/* assume it matches							*/
		for (j = 0; j < f_name_len; j++) {		/* check entire name							*/
			if (fname [j] != fcb [f_nm + j]) found = false;	/* does this entry match?						*/
		}
	}											/* of searching entire catalog					*/
		
	if (found & 1) {							/* if we found it								*/
		f_ms_sector = c_ms_sector + fcb [f_ms];	/* set starting sector							*/
		f_ls_sector = c_ls_sector + fcb [f_ls];
		if (_ILT_(f_ls_sector, c_ls_sector)) f_ms_sector = f_ms_sector + 1;
			
		f_ms_length = fcb [f_ml];				/* set sector length							*/
		f_ls_length = fcb [f_ll];
		f_words = fcb [f_wd];					/* set word length								*/
		f_type = fcb [f_ty];					/* set file type								*/
		return (i);								/* return the FCB number						*/
	}											/* of found it									*/
	else {										/* didn't find it								*/
		c_status = e_no_file;
		return (-1);
	}
}
	
/* $subtitle Buffer Interface:  ReadDir, RemoveFile */


/* This procedure searches the catalog buffer for the file with the name
   .  NAME.  If it is found and the file is a catalog, its directory is
   .  read into the catalog buffer and a boolean TRUE is returned.  All
   .  of the catalog variables are set by this routine. */

boolean			readdir(						/* read directory								*/
	fixed	name[])	_recursive					/* name of directory to read					*/
{
	fixed			i;
	
	if (findfile (name) == -1) {				/* search for the named catalog					*/
		if (c_status == e_no_file) c_status = e_no_path;	/* change file not there to catalog not there	*/
		return (false);
	}
		
	if (f_type == t_subc)  i = 256;				/* determine directory size						*/
	else if (f_type == t_lsubc) i = 1024;
	else {										/* not a directory!								*/
		c_status = e_not_cat;
		return (false);
	}
		
	return (readcat (f_ms_sector, f_ls_sector, i, f_ms_length, f_ls_length));	/* read it in									*/
}
	
	
/* This procedure removes the file named NAME from the catalog buffer.
   .  If the file isn't found, a boolean FALSE is returned.  All the file
   .  variables are set (according to the file being removed). */

boolean			removefile(						/* remove file NAME from catalog				*/
	fixed	name[])	_recursive					/* name of file to remove						*/
{
	fixed			fcb_;						/* Fcb number for file to remove				*/
	fixed			i;
	
	fcb_ = findfile (name);						/* search for file								*/
	if (fcb_ == -1) return (false);				/* no such file									*/
		
	check_polycache (f_type);					/* zap polycache if necessary					*/
	get_fcb (fcb_, fcb);						/* extract the FCB for this file				*/
	
	for (i = f_nm; i < f_nm + f_name_len; i++) {fcb [i] = 0; }	/* zap name										*/
	fcb [f_wd] = 0;								/* zap word length (but leave sector length for hole)	*/
	
	put_fcb (fcb_, fcb);						/* write block back out							*/
	
	return (true);								/* we removed the file							*/
}
	
/* $subtitle Buffer Interface:  AddFile */


/* This procedure adds (or replaces) the file named NAME with type TYPE,
   .  sector length MS_SECTORS|LS_SECTORS, and word length (mod 64K) LENGTH
   .  to the catalog buffer.  If there is no room in the catalog for the
   .  file, a boolean FALSE is returned and C#STATUS contains the reason
   .  for failure.  Otherwise, a boolean TRUE is returned and all the file
   .  variables are set. */

boolean			addfile(						/* add file NAME to catalog						*/
	fixed	name[], 							/* name of file to add							*/
	fixed	type, 								/* type of new file								*/
	fixed	ms_sectors, 						/* Ms sector length of new file					*/
	fixed	ls_sectors, 						/* Ls sector length of new file					*/
	fixed	length)	_recursive					/* word length of new file						*/
{
	fixed			hole;						/* Fcb number of correct size hole				*/
	fixed			i;
	
	if (findfile (name) != -1) {				/* see if file's already there					*/
		if (AbleCatRtns_AllowTypeReplace)		/* allow replace with different type			*/
			;
		else if (f_type == t_subc && type == t_lsubc)		/* allow replace of subc with lsubc		*/
			;
		else if (f_type == t_lsubc && type == t_subc)
			;	
		else if (f_type != type) {				/* does the type match?							*/
			c_status = e_type;					/* no, this is serious							*/
			return (false);
		}
	}
		
	if (! (removefile (name) & 1)) {			/* remove the file if already there				*/
		if (! (valid_filename (name) & 1)) {	/* new file - make sure it's name is valid		*/
			c_status = e_name;					/* bad filename									*/
			return (false);
		}
	}											/* of removing the file							*/
		
	hole = findfree (ms_sectors, ls_sectors);	/* look for a free block of the correct size	*/
	if (hole == -1) return (false);				/* not enough space								*/
		
	if ((ms_sectors == 0) && (_ILT_(ls_sectors, 256))/* if less than 64K words						*/
	&& (_IGT_(length, shl(ls_sectors, 8))))		/* and LENGTH implies more sectors than requested	*/
		length = shl(ls_sectors, 8);			/* restrict length								*/
		
	get_fcb (hole, fcb);						/* get the hole's FCB							*/
	
	for (i = 0; i < f_name_len; i++) {fcb [f_nm + i] = fname [i]; }	/* save name (FNAME was set by REMOVEFILE)		*/
	fcb [f_wd] = length;						/* save word length								*/
	fcb [f_ty] = type;							/* save file type								*/
	
	put_fcb (hole, fcb);						/* put altered FCB back							*/
	
	f_ms_sector = c_ms_sector + fcb [f_ms];		/* return starting sector and device			*/
	f_ls_sector = c_ls_sector + fcb [f_ls];
	if (_ILT_(f_ls_sector, c_ls_sector)) f_ms_sector = f_ms_sector + 1;
		
	f_ms_length = fcb [f_ml];					/* return number of sectors						*/
	f_ls_length = fcb [f_ll];
	f_words = fcb [f_wd];						/* return word length							*/
	f_type = fcb [f_ty];						/* return file type								*/
	check_polycache (f_type);					/* zap polycache if necessary					*/
	
	return (true);								/* we've added the file							*/
}
	
/* $subtitle Buffer Interface:  ShortenFile */


/* This procedure truncates a block in the catalog buffer.  The
   .  rest of the block is returned to free storage.  A boolean TRUE
   .  is returned if the operation is successful.  Otherwise, FALSE
   .  is returned and C#STATUS contains the reason for failure. */

boolean			shortenfile(					/* shorten file NAME							*/
	fixed	name[], 							/* name of file to truncate						*/
	fixed	ms_sectors, 						/* new MS sector length of file					*/
	fixed	ls_sectors, 						/* new LS sector length of file					*/
	fixed	length)	_recursive					/* new word length of file						*/
{
	fixed			fcb_;						/* Fcb number for file to truncate				*/
	fixed			hole;						/* Fcb number of a hole							*/
	
	fcb_ = findfile (name);						/* search for file								*/
	if (fcb_ == -1) return (false);				/* no such file									*/
		
	if ((shr(c_ms_sector, 8) == 8) || (shr(c_ms_sector, 8) == 9)) {	/* is this the tape drive?						*/
		c_status = e_truncate;					/* cannot truncate a tape file					*/
		return (false);
	}											/* of tape drive								*/
		
	check_polycache (f_type);					/* zap polycache if necessary					*/
	get_fcb (fcb_, fcb);						/* get file block								*/
	
	if ((_IGT_(ms_sectors, fcb [f_ml]))			/* if too large									*/
	|| ((ms_sectors == fcb [f_ml]) && (_IGT_(ls_sectors, fcb [f_ll])))) {
		c_status = e_truncate;					/* cannot expand a file with truncate			*/
		return (false);
	}											/* of too large									*/
		
	if ((ms_sectors != fcb [f_ml]) || (ls_sectors != fcb [f_ll])) {	/* if sector length's changed					*/
		if (! (consolidate() & 1)) return (false);	/* consolidate free blocks						*/
		hole = find_exact (-1, -1, false);		/* see if there are any free blocks				*/
		if (hole == -1) return (false);			/* no room for it in directory					*/
			
		get_fcb (fcb_, fcb);					/* get file block again (above calls trash FCB)	*/
		get_fcb (hole, fcb2);					/* get hole										*/
		
		fcb2 [f_ms] = fcb [f_ms] + ms_sectors;	/* start free block after this					*/
		fcb2 [f_ls] = fcb [f_ls] + ls_sectors;
		if (_ILT_(fcb2 [f_ls], ls_sectors)) fcb2 [f_ms] = fcb2 [f_ms] + 1;
			
		fcb2 [f_ml] = fcb [f_ml] - ms_sectors;	/* set up free block's length					*/
		if (_ILT_(fcb [f_ll], ls_sectors)) fcb2 [f_ml] = fcb2 [f_ml] - 1;
		fcb2 [f_ll] = fcb [f_ll] - ls_sectors;
		
		put_fcb (hole, fcb2);					/* write out free block							*/
		
		fcb [f_ml] = ms_sectors; fcb [f_ll] = ls_sectors;	/* update file's sector length					*/
	}											/* of sector length has changed					*/
		
	fcb [f_wd] = length;						/* update word length							*/
	put_fcb (fcb_, fcb);						/* and update file								*/
	
	return (true);								/* we've shortened the file						*/
}
	
/* $subtitle Buffer Interface:  RenameFile, FindStorage, FindMax */


/* This procedure renames the file named OLD_NAME to NEW_NAME in the
   .  catalog buffer.  If the file named OLD_NAME is not found, a boolean
   .  FALSE is returned.  Otherwise, a boolean TRUE is returned and the
   .  file variables are set. */

boolean			renamefile(						/* rename OLD_NAME to NEW_NAME					*/
	fixed	old_name[], 						/* name of file to look for						*/
	fixed	new_name[])	_recursive				/* new name for that file						*/
{
	fixed			fcb_;						/* Fcb number of file to rename					*/
	fixed			i;
	
	fcb_ = findfile (old_name);					/* search for file								*/
	if (fcb_ == -1) return (false);				/* no such file									*/
		
	if (! (valid_filename (new_name) & 1)) {	/* make sure the new name is valid				*/
		c_status = e_name;						/* bad filename									*/
		return (false);
	}
		
	if (findfile (new_name) != -1) {			/* make sure it isn't already there				*/
		c_status = e_duplicate;					/* duplicate filename							*/
		return (false);
	}
		
	check_polycache (f_type);					/* zap polycache if necessary					*/
	get_fcb (fcb_, fcb);						/* get the FCB									*/
	for (i = 0; i < f_name_len; i++) {fcb [f_nm + i] = fname [i]; }	/* change the name (FNAME was set by FINDFILE)	*/
	put_fcb (fcb_, fcb);						/* put back the altered FCB						*/
	
	return (true);								/* we've renamed the file						*/
}
	
	
/* This procedure searches the catalog buffer for a free contiguous
   .  block of storage that has a size of MS_SECTORS|LS_SECTORS.  If
   .  such a block is found, the FCB number of that block is returned.
   .  Otherwise, a minus one (-1) is returned and C#STATUS contains
   .  the reason for failure.  All the file variables are set. */

fixed			findstorage(					/* find storage									*/
	fixed	ms_sectors, 						/* Ms sector length of block to find			*/
	fixed	ls_sectors)	_recursive				/* Ls sector length of block to find			*/
{
	fixed			hole;						/* Fcb number of correct size hole				*/
	
	hole = findfree (ms_sectors, ls_sectors);	/* look for a hole of the correct size			*/
	
	if (hole != -1) {							/* if we have the storage						*/
		get_fcb (hole, fcb);					/* read in the hole's FCB						*/
		
		f_ms_sector = c_ms_sector + fcb [f_ms];	/* return starting sector and device			*/
		f_ls_sector = c_ls_sector + fcb [f_ls];
		if (_ILT_(f_ls_sector, c_ls_sector)) f_ms_sector = f_ms_sector + 1;
			
		f_ms_length = fcb [f_ml];				/* return number of sectors						*/
		f_ls_length = fcb [f_ll];
		f_words = shl(f_ls_length, 8);			/* return maximum file length					*/
	}											/* of if we have the storage					*/
		
	return (hole);								/* return the FCB number of the hole			*/
}
	
	
/* This procedure searches the catalog buffer for the largest free
   .  block.  If such a block is found, the file variables are set to
   .  contain the relevant information and TRUE is returned.  Otherwise,
   .  FALSE is returned and C#STATUS contains the reason for failure. */

boolean			findmax()	_recursive			/* find maximum storage available				*/

{
	return (maxfree());							/* just use MAXFREE								*/
}
	
/* $subtitle Level Interface:  Read_Cat */


/* This procedure reads the catalog on the specified LEVEL into the
   .  catalog buffer.  All of the catalog variables are set by this
   .  routine.  If READ_CAT is passed an invalid LEVEL number (or
   .  there is no catalog buffer), it will return a boolean FALSE. */

static	boolean	read_cat(						/* read catalog LEVEL							*/
	fixed	level)	_recursive					/* level of catalog to read						*/
	
{
	if (level < 2)								/* if special level								*/
	switch (level + 2) {						/* branch on level number						*/
		case 0:
		{										/* path catalog									*/
			c_ms_sector = able_core(loc_pcat + 1);	/* actual device/MS sector (this can optimize catalog buffering)	*/
			c_ls_sector = able_core(loc_pcat);	/* Ls sector									*/
			c_dir_size  = able_core(loc_pctl);	/* length of user directory						*/
			c_ms_length = able_core(loc_pmax + 1);	/* estimate sector length						*/
			c_ls_length = able_core(loc_pmax);
			break;
		}
		case 1:
		{										/* alternate catalog							*/
			c_ms_sector = a_ms_sector;			/* actual device/MS sector (this can optimize catalog buffering)	*/
			c_ls_sector = a_ls_sector;			/* Ls sector									*/
			c_dir_size  = a_dir_size;			/* length of user directory						*/
			c_ms_length = a_ms_length;			/* estimate sector length						*/
			c_ls_length = a_ls_length;
			break;
		}
		case 2:
		{										/* system catalog								*/
			c_ms_sector = able_core(loc_scat + 1);	/* actual device/MS sector (this can optimize catalog buffering)	*/
			c_ls_sector = able_core(loc_scat);	/* Ls sector									*/
			c_dir_size  = able_core(loc_sctl);	/* length of system directory					*/
			c_ms_length = able_core(loc_smax + 1);	/* estimate sector length						*/
			c_ls_length = able_core(loc_smax);
			break;
		}
		case 3:
		{										/* user catalog									*/
			c_ms_sector = able_core(loc_ucat + 1);	/* actual device/MS sector (this can optimize catalog buffering)	*/
			c_ls_sector = able_core(loc_ucat);	/* Ls sector									*/
			c_dir_size  = able_core(loc_uctl);	/* length of user directory						*/
			c_ms_length = able_core(loc_umax + 1);	/* estimate sector length						*/
			c_ls_length = able_core(loc_umax);
			break;
		}
	}											/* of special level numbers						*/
	else {										/* for other levels, it is easier:				*/
		c_ms_sector = shl(level, 8);			/* save the device number (same as level number here)	*/
		c_ls_sector = 0;						/* the catalog is at the start of the device	*/
		if (level >= ABLE_HFS_READDATA_CODE)   c_dir_size = 1024;	// HFS image file
		else if ((level == 6) || (level == 7)) c_dir_size = 1024;	// w. disk
		else c_dir_size = 256;					/* otherwise 256 words (floppies)				*/
		c_ms_length = 0;						/* estimate sector length						*/
		c_ls_length = shr(c_dir_size, 8);		/* restrict to directory size					*/
	}
		
	return (readcat (c_ms_sector, c_ls_sector, c_dir_size, c_ms_length, c_ls_length));	/* read in the catalog							*/
}
	
/* $subtitle Level Interface:  Read_Catalog, Write_Catalog */


/* This procedure reads the named catalog on the specified LEVEL into
   .  the catalog buffer.  If a null name is passed, the catalog on the
   .  specified level will be read.  All of the catalog variables are set
   .  by this routine.  A boolean TRUE is returned if the catalog is
   .  successfully read. */

boolean			read_catalog(					/* read catalog TREENAME on LEVEL				*/
	fixed	treename[], 						/* name of catalog to read						*/
	fixed	level)	_recursive					/* level to start searching for the catalog on	*/
	
{
	if (treename [0] == 0) {					/* if no name specified							*/
		f_name [0] = 0;							/* Treename routines must always set this		*/
		
		switch (level + 2) {					/* try to set the name							*/
			case 0:
				blockmove (_location_(&ABLE_CONTEXT._able_memory_[loc_pcnm]), f_name, f_name_len + 1);	/* path											*/
				break;
			case 1:
				blockmove (a_name, f_name, f_name_len + 1);	/* alternate									*/
				break;
			case 2:
				;								/* system										*/
				break;
			case 3:
				blockmove (_location_(&ABLE_CONTEXT._able_memory_[loc_ccnm]), f_name, f_name_len + 1);	/* current										*/
				break;
		}										/* of trying to set the name					*/
			
		return (read_cat (level));				/* just read that level							*/
	}											/* of no name specified							*/
	else return (get_catalog (treename, level, true));	/* need to find the catalog						*/
}
	
	
/* This procedure writes the catalog buffer to the place it was read
   .  from.  It returns FALSE if there's no buffer. */

boolean			write_catalog()	_recursive		/* write out the catalog buffer					*/

{
	return (writecat());						/* just use WRITECAT							*/
}
	
/* $subtitle Level Interface:  Delete, Replace, Truncate */

static	void	convert_or_copy_string(			/* takes able or C string; copies to a-string	*/
	void   *theName,
	fixed	treename[],
	int		isCString)
{
	fixed *it = (fixed *) theName;
	
	if (isCString)
		to_able_string((const char *) theName, treename);
		
	else
	{
		int i;
		
		for (i = 0; i <= shr(it[0]+1,1); i++)
			treename[i] = it[i];
	}
}


/* This procedure removes the file named TREENAME from the catalog on the
   .  specified LEVEL.  If the file isn't found, a boolean FALSE is returned.
   .  All the file variables are set (according to the file being deleted).
   .  Note that this procedure overwrites the contents of the catalog buffer
   .  and the catalog variables. */

boolean			XPLdelete(						/* delete file TREENAME on LEVEL				*/
	void    *theName,	 						/* treename of file to delete					*/
	fixed	level, int isCString)	_recursive	/* level of catalog to delete it from			*/
	
{
	fixed	treename[128];
	
	convert_or_copy_string(theName, treename, isCString);	/* get able string from able or C string		*/
	
	if (! (get_catalog (treename, level, false) & 1)) return (false);	/* read the catalog								*/
	if (! (removefile (f_name) & 1)) return (false);	/* remove the file								*/
		
	return (write_catalog());					/* write out the new catalog					*/
}
	
	
/* This procedure adds (or replaces) the file named TREENAME with type TYPE,
   .  sector length MS_SECTORS|LS_SECTORS, and word length (mod 64K) LENGTH
   .  to the catalog on the specified LEVEL.  If there is no room in the
   .  catalog for the file, a boolean FALSE is returned and C#STATUS
   .  contains the reason for failure.  Otherwise, a boolean TRUE is
   .  returned and all the file variables are set.  Note that this procedure
   .  overwrites the contents of the catalog buffer and the catalog variables. */

boolean			replace(						/* replace file TREENAME on LEVEL				*/
	void    *theName,	 						/* treename of file to replace					*/
	fixed	type, 								/* type of new file								*/
	fixed	ms_sectors, 						/* Ms sector length of new file					*/
	fixed	ls_sectors, 						/* Ls sector length of new file					*/
	fixed	length, 							/* word length of new file						*/
	fixed	level, int isCString)	_recursive	/* level of catalog to replace it in			*/
	
{
	fixed	treename[128];
	
	convert_or_copy_string(theName, treename, isCString);	/* get able string from able or C string		*/
	
	if (! (get_catalog (treename, level, false) & 1)) return (false);	/* read in the catalog							*/
	if (! (addfile (f_name, type, ms_sectors, ls_sectors, length) & 1)) return (false);	/* add the file									*/
		
	return (write_catalog());					/* write out the new catalog					*/
}
	
	
/* AbleCatRtns_GetCatalog
   .  gets the catalog into memory so we can see if we can add to it */

boolean			AbleCatRtns_GetCatalog(void  *theName, fixed level, int isCString)
{
	fixed	treename[128];
	
	convert_or_copy_string(theName, treename, isCString);	/* get able string from able or C string		*/
	
	if (! (read_catalog (treename, level) & 1)) return (false);	/* read in the catalog							*/
		
	return (true);
}

	
/* AbleCatRtns_AddFile
   .  Adds a file to the catalog in memory */

boolean			AbleCatRtns_AddFile(
	void    *theName,	 						/* file of file to replace						*/
	fixed	type, 								/* type of new file								*/
	fixed	ms_sectors, 						/* Ms sector length of new file					*/
	fixed	ls_sectors, 						/* Ls sector length of new file					*/
	fixed	length, int isCString)	_recursive	/* word length									*/
{
	fixed	treename[128];
	fixed   index = 0;
	
	c_status = e_none;

	convert_or_copy_string(theName, treename, isCString);	/* get able string from able or C string		*/

	search_scan_name(&index, treename);

	if (! (addfile (f_name, type, ms_sectors, ls_sectors, length) & 1)) return (false);	/* add the file									*/
		
	return (true);
}
	
	
/* AbleCatRtns_AddFile
 .  Forceibly  changes the length of the file in the catalog entry
 .  Optionally changes the name   of the file in the catalog entry */

boolean			AbleCatRtns_UpdateFile(
                                    void    *theName,	 						/* file of file to replace						*/
                                    fixed	type, 								/* type of new file								*/
                                    fixed	ms_sectors, 						/* Ms sector length of new file					*/
                                    fixed	ls_sectors, 						/* Ls sector length of new file					*/
                                    fixed	length,                             /* word length									*/
                                    int     isCString,
                                    void*   newName)	_recursive
{
    fixed	treename[128];
    fixed   newname [  5] = {0,0,0,0,0};
    fixed   index = 0;
    fixed   fcb_ = 0;
    
    c_status = e_none;
    
    convert_or_copy_string(theName, treename, isCString);       /* get able string from able or C string		*/
    
    if (newName)
        convert_or_copy_string(newName, newname, isCString);	/* get able string from able or C string		*/
    
    search_scan_name(&index, treename);
    
    fcb_ = findfile (f_name);
    
    if (fcb_ == -1)
        return false;

    get_fcb (fcb_, fcb);						/* extract the FCB for this file				*/
    
    if (newName) {
        fcb [f_nm+0] = newname[1];
        fcb [f_nm+1] = newname[2];
        fcb [f_nm+2] = newname[3];
        fcb [f_nm+3] = newname[4];
    }
    
    fcb [f_ll] = ls_sectors;					/* save ls file length in sectors               */
    fcb [f_ml] = ms_sectors;					/* save Mms file length in sectors				*/
    fcb [f_wd] = length;						/* save word length								*/
    fcb [f_ty] = type;							/* save file type								*/

    put_fcb (fcb_, fcb);						/* write block back out							*/

    if (newName) {
        f_name[0]   = 8;
        f_name[1]   = newname[1];
        f_name[2]   = newname[2];
        f_name[3]   = newname[3];
        f_name[4]   = newname[4];
        
        while (f_name[0] > 0 && byte(f_name, f_name[0]-1) == 0)
            f_name[0] = f_name[0]-1;
    }
    
    f_ms_length = fcb [f_ml];                   /* set sector length							*/
    f_ls_length = fcb [f_ll];
    f_words     = fcb [f_wd];					/* set word length								*/
    f_type      = fcb [f_ty];					/* set file type								*/

    return (true);
}

boolean			AbleCatRtns_UpdateName(
                                       void    *theName,    /* file of file to replace						*/
                                       int      isCString,
                                       fixed    newName[4])
{
    fixed	treename[128];
    fixed   index = 0;
    fixed   fcb_ = 0;
    
    c_status = e_none;
    
    convert_or_copy_string(theName, treename, isCString);	/* get able string from able or C string		*/
    
    search_scan_name(&index, treename);
    
    fcb_ = findfile (f_name);
    
    if (fcb_ == -1)
        return false;
    
    get_fcb (fcb_, fcb);						/* extract the FCB for this file				*/
    
    fcb [f_nm+0] = newName[0];
    fcb [f_nm+1] = newName[1];
    fcb [f_nm+2] = newName[2];
    fcb [f_nm+3] = newName[3];
    
    put_fcb (fcb_, fcb);						/* write block back out							*/
    
    f_name[0]   = 8;
    f_name[1]   = newName[0];
    f_name[2]   = newName[1];
    f_name[3]   = newName[2];
    f_name[4]   = newName[3];
    
    while (f_name[0] > 0 && byte(f_name, f_name[0]-1) == 0)
        f_name[0] = f_name[0]-1;
    
    f_ms_length = fcb [f_ml];                   /* set sector length							*/
    f_ls_length = fcb [f_ll];
    f_words     = fcb [f_wd];					/* set word length								*/
    f_type      = fcb [f_ty];					/* set file type								*/
    
    return (true);
}

/* AbleCatRtns_FindFile
   .  sees if a file name is in use in the cached directory */

boolean			AbleCatRtns_FindFile(			/* locate file TREENAME on LEVEL				*/
	void *	theName, int isCString)	_recursive
	
{
	fixed	treename[128];
	fixed   index = 0;
	
	c_status = e_none;

	convert_or_copy_string(theName, treename, isCString);	/* get able string from able or C string		*/
	
	search_scan_name(&index, treename);

	return (findfile (f_name) != -1);			/* search it									*/
}


/* This procedure truncates the file named TREENAME to sector length
   .  MS_SECTORS|LS_SECTORS and word length (mod 64K) LENGTH in the catalog
   .  on the specified LEVEL.  If the file is truncated, a boolean TRUE
   .  is returned and all the file variables are set.  Otherwise, a
   .  boolean FALSE is returned and C#STATUS contains the reason for
   .  failure.  Note that this procedure overwrites the contents of the
   .  catalog buffer and the catalog variables. */

boolean			truncate(						/* truncate file TREENAME on LEVEL				*/
	void    *theName,	 						/* treename of file to truncate					*/
	fixed	ms_sectors, 						/* new MS sector length of file					*/
	fixed	ls_sectors, 						/* new LS sector length of file					*/
	fixed	length, 							/* new word length of file						*/
	fixed	level, int isCString)	_recursive	/* level of catalog containing file				*/
	
{
	fixed	treename[128];
	
	convert_or_copy_string(theName, treename, isCString);	/* get able string from able or C string		*/
	
	if (! (get_catalog (treename, level, false) & 1)) return (false);	/* read in the catalog							*/
	if (! (shortenfile (f_name, ms_sectors, ls_sectors, length) & 1)) return (false);	/* add the file									*/
		
	return (write_catalog());					/* write out the new catalog					*/
}
	
/* $subtitle Level Interface:  Rename, Locate, LookStorage, LookMax */


/* This procedure renames the file named OLD_NAME to NEW_NAME in the
   .  catalog on the specified LEVEL.  If the file named OLD_NAME is not
   .  found, a boolean FALSE is returned.  Otherwise, a boolean TRUE is
   .  returned and the file variables are set.  Note that this procedure
   .  overwrites the contents of the catalog buffer and the catalog variables. */

boolean			rename_able_file(				/* rename OLD_NAME to NEW_NAME on LEVEL			*/
	void   *oldName, 							/* treename of file to look for					*/
	void   *newName, 							/* new name for that file						*/
	fixed	level, int isCString)	_recursive	/* level of catalog to search					*/
	
{
	fixed	old_name[128];
	fixed	new_name[128];
	
	convert_or_copy_string(oldName, old_name, isCString);	/* get able string from able or C string		*/
	convert_or_copy_string(newName, new_name, isCString);	/* get able string from able or C string		*/
	
	if (! (get_catalog (old_name, level, false) & 1)) return (false);	/* read in the catalog							*/
	if (! (renamefile (f_name, new_name) & 1)) return (false);	/* rename the file								*/
		
	return (write_catalog());					/* write out the new catalog					*/
}
	
	
/* This procedure searches the catalog on the specified LEVEL for the file
   .  with name TREENAME.  If it is found, a boolean TRUE is returned and all
   .  the file variables are set.  Otherwise, a boolean FALSE is returned (and
   .  C#STATUS may contain an error).  Note that this procedure overwrites
   .  the contents of the catalog buffer and the catalog variables. */

boolean			locate(							/* locate file TREENAME on LEVEL				*/
	void *	theName, 						    /* treename of file to look for					*/
	fixed	level, int isCString)	_recursive	/* level of catalog to search					*/
	
{
	fixed	treename[128];
	
	convert_or_copy_string(theName, treename, isCString);	/* get able string from able or C string		*/
	
	if (get_catalog (treename, level, false) & 1)/* if we can read the catalog					*/
		return (findfile (f_name) != -1);		/* search it									*/
	
	else return (false);						/* can't even read the catalog					*/
}
	
	
/* This procedure looks for a free FCB of size MS_SECTORS|LS_SECTORS in
   .  catalog TREENAME on the specified LEVEL.  If it finds the space, it sets
   .  the file variables (according to the hole) and returns a boolean TRUE.
   .  Otherwise, it returns a boolean FALSE and C#STATUS is set to the
   .  reason for failure.  Note that this procedure overwrites the catalog
   .  buffer and the catalog variables. */

boolean			lookstorage(					/* look for storage on TREENAME/LEVEL			*/
	void    *theName,	 						/* treename of catalog to look for storage on	*/
	fixed	ms_sectors, 						/* Ms sector length of block to find			*/
	fixed	ls_sectors, 						/* Ls sector length of block to find			*/
	fixed	level, int isCString)	_recursive	/* level of catalog to search					*/
	
{
	fixed	treename[128];
	
	convert_or_copy_string(theName, treename, isCString);	/* get able string from able or C string		*/
	
	if (read_catalog (treename, level) & 1)		/* if we can read the catalog					*/
		return (findstorage (ms_sectors, ls_sectors) != 0);	/* search it									*/
	else return (false);						/* can't even read the catalog					*/
}
	
	
/* This procedure searches for the largest free block in catalog TREENAME
   .  on the specified LEVEL.  If such a block is found, the file variables
   .  are set to contain the relevant information and TRUE is returned.
   .  Otherwise, FALSE is returned and C#STATUS contains the reason for
   .  failure. */

boolean			lookmax(						/* lookup maximum storage available on TREENAME/LEVEL	*/
	void    *theName,	 						/* treename of catalog to look for max storage on	*/
	fixed	level, int isCString)	_recursive	/* level of catalog to search					*/
	
{
	fixed	treename[128];
	
	convert_or_copy_string(theName, treename, isCString);	/* get able string from able or C string		*/
	
	if (read_catalog (treename, level) & 1)		/* if we can read the catalog					*/
		return (findmax());						/* search it									*/
	else return (false);						/* can't even read the catalog					*/
}
	
/* $subtitle Level Interface:  Enter_Catalog, Enter_Alternate */


/* This procedure changes the current catalog to be the catalog with
   .  the specified TREENAME on the specified LEVEL.  If a null name is passed,
   .  the current catalog is changed to be the specified LEVEL.  If the
   .  catalog is successfully entered, the catalog variables contain the
   .  relevent information about the catalog and TRUE is returned.  Otherwise,
   .  FALSE is returned and C#STATUS contains the error.  The file variables
   .  are overwritten by this routine. */

boolean			enter_catalog(					/* enter catalog TREENAME on LEVEL				*/
	void    *theName,	 						/* treename of catalog to enter					*/
	fixed	level, int isCString)	_recursive	/* level of catalog to search					*/
{
	fixed	i;
	fixed	treename[128];
	
	convert_or_copy_string(theName, treename, isCString);	/* get able string from able or C string		*/
	
	if (! (read_catalog (treename, level) & 1)) return (false);	/* read in the catalog							*/
		
	clean_filename (f_name, fname);				/* clean up the name to save in high memory		*/
	
	set_able_core(loc_ccnm, f_name [0]);		/* save length of name in high memory			*/
    for (i = 1; i <= f_name_len; i++) {			/* copy the name to high memory					*/
		set_able_core(loc_ccnm + i, fname [i - 1]);
	}
		
	set_able_core(loc_ucat + 1, c_ms_sector); set_able_core(loc_ucat, c_ls_sector);	/* set high memory variables					*/
	set_able_core(loc_umax + 1, c_ms_length); set_able_core(loc_umax, c_ls_length);
	set_able_core(loc_uctl, c_dir_size);
	
	set_curdev (shr(able_core(loc_ucat + 1), 8));	/* set current device in configuration			*/
	set_able_core(loc_ctab + c_curdev - c_offset, able_core(c_contab + c_curdev));	/* and copy to high memory						*/
	
	return (true);
}
	
	
/* This procedure changes the alternate catalog to be the catalog with the
   .  specified TREENAME on the specified LEVEL.  If a null name is passed,
   .  the alternate catalog is changed to be the specified LEVEL.  If the
   .  catalog is successfully entered, the catalog variables contain the
   .  relevent information about the catalog and TRUE is returned.  Otherwise,
   .  FALSE is returned and C#STATUS contains the error.  The file variables
   .  are overwritten by this routine. */

boolean			enter_alternate(				/* enter alternate catalog TREENAME on LEVEL	*/
	void    *theName,	 						/* treename of catalog to enter					*/
	fixed	level, int isCString)	_recursive	/* level of catalog to search					*/
{
	fixed	i;
	fixed	treename[128];
	
	convert_or_copy_string(theName, treename, isCString);	/* get able string from able or C string		*/
		
	if (! (read_catalog (treename, level) & 1)) return (false);	/* read in the catalog							*/
		
	clean_filename (f_name, fname);				/* clean up the name to save in A#NAME			*/
	
	a_name [0] = f_name [0];					/* save length of name							*/
    for (i = 1; i <= f_name_len; i++) {			/* copy the name to A#NAME						*/
		a_name [i] = fname [i - 1];
	}
		
	a_ms_sector = c_ms_sector; a_ls_sector = c_ls_sector;	/* set alternate catalog variables				*/
	a_ms_length = c_ms_length; a_ls_length = c_ls_length;
	a_dir_size  = c_dir_size;
	
	return (true);
}
	


void	initialize_able_catalog_routines()
{
	set_catbuf   (_allocate_able_heap(c_dir_max), 0);
	reinit_cache (__default, _allocate_able_heap((fixed) (__vars * __default )));		/* set up for two caches						*/
}
