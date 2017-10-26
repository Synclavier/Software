/* Catalog routines (no swapping) */

/*	Translated to C:   	12/10/96 at 11:37	*/
/*	Translator Version:	0.000				*/

/* Catalog routines:
.     VALID_FILECHAR (c): returns TRUE if character C is valid in a filename
.     VALID_FILENAME (name): returns TRUE if passed filename is valid
.     CLEAN_FILENAME (name, fcb_name): converts XPL string format to FCB name format
.     CLEAN_FCBNAME (fcb_name, name): converts FCB name to XPL string format
.
.     CACHE (bufptr, bufmed): cache contents of catalog buffer in buffer BUFPTR on BUFMED
.     REINIT_CACHE (n, buffer): reinitialize caching to provide for N caches
.     FLUSH_CACHE (n): flush cache N to disk/tape
.     DISABLE_CACHE (n): disable cache N
.     ENABLE_CACHE (n): enable cache N
.     CACHE_TREENAME (enable): enable/disable treename caching
.
.     SET_CATBUF (bufptr, bufmed): set the catalog buffer pointer
.     READCAT (ms_sector, ls_sector, dir_size, ms_length, ls_length): read in a catalog
.     WRITECAT: write out the catalog buffer
.     READDIR (name): read in catalog NAME
.
.     GET_FCB (fcb#, fcb): extract an FCB from the catalog buffer
.     PUT_FCB (fcb#, fcb): replace an FCB into the catalog buffer
.
.     FINDFILE (name): find a file in the catalog buffer
.     REMOVEFILE (name): remove file NAME from catalog
.     ADDFILE (name, type, ms_sectors, ls_sectors, length): add file NAME to catalog
.     SHORTENFILE (name, ms_sectors, ls_sectors, length): shorten file NAME
.     RENAMEFILE (old_name, new_name): rename OLD_NAME to NEW_NAME
.     FINDSTORAGE (ms_sectors, ls_sectors): find storage
.     FINDMAX: find maximum storage available
.
.     READ_CATALOG (treename, level): read catalog TREENAME on LEVEL
.     WRITE_CATALOG: write out the catalog buffer
.     DELETE (treename, level): delete file TREENAME on LEVEL
.     REPLACE (treename, type, ms_sectors, ls_sectors, length, level): replace file TREENAME on LEVEL
.     TRUNCATE (treename, ms_sectors, ls_sectors, length, level): truncate file TREENAME on LEVEL
.     RENAME (old_name, new_name, level): rename OLD_NAME to NEW_NAME on LEVEL
.     LOCATE (treename, level): locate file TREENAME on LEVEL
.     LOOKSTORAGE (treename, ms_sectors, ls_sectors, level): look for storage on TREENAME/LEVEL
.     LOOKMAX (treename, level): lookup maximum storage available on TREENAME/LEVEL
.     ENTER_CATALOG (treename, level): enter catalog TREENAME on LEVEL
.     ENTER_ALTERNATE (treename, level): enter alternate catalog TREENAME on LEVEL
*/

#include "XPL.h"

/* Catalog entry definitions */

#define	c_nm			0						/* filename (four words)						*/
#define	c_ls			4						/* Ls starting sector							*/
#define	c_ll			5						/* Ls sector length								*/
#define	c_wd			6						/* word length (modulo 64K)						*/
#define	c_ty			7						/* Ms starting sector (8 bits)/MS sector length (4 bits)/file type (4 bits)	*/
#define	c_len			8						/* number of words in a catalog entry			*/

#define	c_dir_max		1024					/* maximum directory size (words)				*/


/* Fcb definitions (used with GET_FCB and PUT_FCB) */

#define	f_nm			0						/* filename (four words)						*/
#define	f_ls			4						/* Ls starting sector							*/
#define	f_ms			5						/* Ms starting sector							*/
#define	f_ll			6						/* Ls sector length								*/
#define	f_ml			7						/* Ms sector length								*/
#define	f_wd			8						/* word length (modulo 64K)						*/
#define	f_ty			9						/* file type									*/
#define	f_len			10						/* number of words in an FCB					*/

#define	f_name_len		4						/* number of words in a filename				*/


/* File type definitions */

#define	t_text			0						/* text file									*/
#define	t_exec			1						/* executable binary							*/
#define	t_reloc			2						/* relocatable binary							*/
#define	t_data			3						/* data file									*/
#define	t_sync			4						/* synclavier sequence							*/
#define	t_sound			5						/* sound file									*/
#define	t_subc			6						/* subcatalog									*/
#define	t_lsubc			7						/* large subcatalog								*/
#define	t_dump			8						/* dump file									*/
#define	t_spect			9						/* spectral file								*/
#define	t_index			10						/* index file									*/
#define	t_timbre		11						/* synclavier timbre							*/
#define	t_max			11						/* maximum defined filetype						*/


/* Errors returned in C#STATUS */

#define	e_none			0						/* no error encountered							*/
#define	e_os			1						/* operating system error - magic number not set	*/
#define	e_buffer		2						/* no catalog buffer allocated					*/
#define	e_no_dir		3						/* no directory in memory						*/
#define	e_no_config		4						/* device not configured						*/
#define	e_no_floppy		5						/* no floppy in drive							*/
#define	e_fcb			6						/* Fcb number out of bounds						*/
#define	e_level			7						/* level number out of bounds					*/
#define	e_storage		8						/* not enough available storage					*/
#define	e_cstorage		9						/* not enough contiguous storage available		*/
#define	e_dir_full		10						/* no entries left in the directory				*/
#define	e_invalid		11						/* invalid directory							*/
#define	e_name			12						/* invalid filename specified for operation requested	*/
#define	e_duplicate		13						/* duplicate filename							*/
#define	e_no_file		14						/* file not found								*/
#define	e_not_cat		15						/* name specified is required to be a catalog, but isn't	*/
#define	e_treename		16						/* incorrect format for treename				*/
#define	e_no_path		17						/* intermediate catalog (in treename) not found	*/
#define	e_type			18						/* type mismatch between saved file and replaced file (ADDFILE/REPLACE only)	*/
#define	e_protect		19						/* write protected floppy						*/
#define	e_too_large		20						/* file too large (>= 2^20 sectors)				*/
#define	e_truncate		21						/* truncation error - trying to expand a file (or truncate tape file)	*/
#define e_diskerror		22						/* disk error: the disk could not be read       */

/* Caching literals */

#define	__vars			5						/* five variables/cache							*/


/* Global catalog variables */

extern	fixed	c_status;						/* status of last catalog operation				*/
extern	fixed	c_bufptr;						/* pointer to the catalog buffer				*/
extern  fixed*  c_bufhost;                      /* catalog buffer in host computer memory       */
extern	fixed	c_bufmed;						/* catalog buffer media:  0 - main memory, 1 - external memory	*/
extern	fixed	c_ms_sector;					/* device and MS starting sector of catalog		*/
extern	fixed	c_ls_sector;					/* Ls starting sector of catalog				*/
extern	fixed	c_ms_length;					/* Ms sector length of catalog (including directory)	*/
extern	fixed	c_ls_length;					/* Ls sector length of catalog (including directory)	*/
extern	fixed	c_dir_size;						/* size of catalog directory (in words)			*/


/* Global file variables */

extern	array	f_name;							/* name of last filename scanned off treename	*/
extern	fixed	f_ms_sector;					/* device and MS starting sector of file		*/
extern	fixed	f_ls_sector;					/* Ls starting sector of file					*/
extern	fixed	f_ms_length;					/* Ms sector length of file						*/
extern	fixed	f_ls_length;					/* Ls sector length of file						*/
extern	fixed	f_words;						/* word length of file (modulo 64K)				*/
extern	fixed	f_type;							/* type of file									*/


/* Alternate catalog variables (for reference only - do NOT modify) */

extern	array	a_name;							/* name of alternate catalog					*/
extern	fixed	a_ms_sector;					/* device and MS starting sector of catalog		*/
extern	fixed	a_ls_sector;					/* Ls starting sector of catalog				*/
extern	fixed	a_ms_length;					/* Ms sector length of catalog (including directory)	*/
extern	fixed	a_ls_length;					/* Ls sector length of catalog (including directory)	*/
extern	fixed	a_dir_size;						/* size of catalog directory (in words)			*/


/* Filename processing */

extern	boolean	valid_filechar(fixed);			/* return TRUE if specified character is valid in a filename	*/
extern	boolean	valid_filename(fixed[]);		/* return TRUE if passed filename is valid		*/
extern	void	clean_filename(fixed[], fixed[]);	/* convert XPL string format to FCB name format	*/
extern	void	clean_fcbname(fixed[], fixed[]);	/* convert FCB name to XPL string format		*/


/* Caching */

extern	fixed	cache(fixed, fixed);			/* cache contents of catalog buffer				*/

extern	void	reinit_cache(fixed, fixed);		/* reinitialize caching to provide for N caches	*/

												/* note: was fixed, fixed *; second arg			*/
												/* is able_core space pointer...				*/

extern	boolean	flush_cache(fixed);				/* flush cache N to disk/tape					*/
extern	void	disable_cache(fixed);			/* disable cache N								*/
extern	void	enable_cache(fixed);			/* enable cache N								*/
extern	void	cache_treename(boolean);		/* enable/disable treename caching				*/


/* Buffer interface */

extern	void	set_catbuf(fixed, fixed);		/* set the catalog buffer pointer				*/
												/* note: was pointer, fixed						*/
												/* first arg points to cat buffer in able_core	*/
												/* data space									*/
												
extern	boolean	readcat(fixed, fixed, fixed, fixed, fixed);	/* read in a catalog							*/
extern	boolean	writecat();						/* write out the catalog buffer					*/
extern	boolean	readdir(fixed[]);				/* read in catalog NAME							*/

extern	boolean	get_fcb(fixed, fixed[]);		/* extract an FCB from the catalog buffer		*/
extern	boolean	put_fcb(fixed, fixed[]);		/* replace an FCB into the catalog buffer		*/

extern	fixed	findfile(fixed[]);				/* find a file in the catalog buffer			*/
extern	boolean	removefile(fixed[]);			/* remove file NAME from catalog				*/
extern	boolean	addfile(fixed[], fixed, fixed, fixed, fixed);	/* add file NAME to catalog     */
extern	boolean	shortenfile(fixed[], fixed, fixed, fixed);      /* shorten file NAME            */
extern	boolean	renamefile(fixed[], fixed[]);	/* rename OLD_NAME to NEW_NAME					*/
extern	fixed	findstorage(fixed, fixed);		/* find storage									*/
extern	boolean	findmax();						/* find maximum storage available				*/


/* Level interface */

extern	boolean get_device_size(fixed);			/* get catalog size for a device				*/

extern	boolean	read_catalog(fixed[], fixed);	/* read catalog TREENAME on LEVEL				*/
extern	boolean	write_catalog();				/* write out the catalog buffer					*/

extern	boolean	XPLdelete(void *, fixed, int isCString);								/* delete file NAME on LEVEL					*/
extern	boolean	replace(void *, fixed, fixed, fixed, fixed, fixed, int isCString);		/* replace file NAME on LEVEL					*/
extern  boolean	AbleCatRtns_GetCatalog ( void  *theName, fixed level, int isCString);   /* gets the catalog into memory					*/
extern	boolean	AbleCatRtns_AddFile    (void *, fixed, fixed, fixed, fixed, int isCString);
extern	boolean	AbleCatRtns_UpdateFile (void *, fixed, fixed, fixed, fixed, int isCString, void* newName);
extern  boolean AbleCatRtns_UpdateName (void    *theName, int isCString, fixed newName[4]);
extern	boolean	AbleCatRtns_FindFile   (void *, int isCString);
extern	boolean	truncate(void *, fixed, fixed, fixed, fixed, int isCString);			/* truncate file NAME on LEVEL					*/
extern	boolean	rename_able_file(void *, void *, fixed, int isCString);					/* rename OLD_NAME to NEW_NAME on LEVEL			*/
extern	boolean	locate(void *, fixed, int isCString);									/* locate file NAME on LEVEL					*/
extern	boolean	lookstorage(void *, fixed, fixed, fixed, int isCString);				/* look for storage on TREENAME/LEVEL			*/
extern	boolean	lookmax(void *, fixed, int isCString);									/* lookup maximum storage available on TREENAME/LEVEL	*/

extern	boolean	enter_catalog(void *, fixed, int isCString);							/* enter catalog NAME on LEVEL					*/
extern	boolean	enter_alternate(void *, fixed, int isCString);							/* enter alternate catalog NAME on LEVEL		*/

// Host interface
extern  fixed	AbleCatRtns_AllowTypeReplace;	// true to allow replace file to have different file type
extern	fixed	AbleCatRtns_SoundFileChanged;	// set true of subcat or sound file changes

extern	void	initialize_able_catalog_routines();
