/* Syslits   $title  system-wide literals

   Modified:
   11/11/96 - CWJ - Changed order of declaration to assist with XPL translations
   08/14/91 - MWH - Added option bit for DSP
   02/07/91 - MWH - Added option bits for Poly Voice Type and the D164
   
*/
/* $subtitle Configuration Information */

/* Every XPL program has a configuration area in lowcore (pointed
.  to by the contents of the word at address ONE of the program).
.  The following definitions define the configuration area: */

extern	ufixed			mem_siz;				/* holds memory size - required to use these literals	*/

#define	c_contab		able_core(1)			/* pointer to the configuration table			*/

/* compiler information [accessed via CORE(C#CONTAB + <literal>)] */

#define	c_vstart		0						/* start  of variable area						*/
#define	c_vlngth		1						/* length of variable area						*/
#define	c_stklen		2						/* length of stack area							*/
#define	c_objloc		3						/* start  of object code						*/
#define	c_objlen		4						/* length of object code						*/
#define	c_swploc		c_objlen				/* pointer to swap area							*/
#define	c_swpram		5						/* pointer to swapping mechanism ram area		*/
#define	c_swplen		6						/* length of swap file (in sectors)				*/
#define	c_rtploc		7						/* start  of RTP								*/
#define	c_inttab		8						/* pointer to WHEN statement table				*/
#define	c_rcvptr		9						/* pointer to RCVDCHARACTER						*/
#define	c_inpbuf		10						/* pointer to terminal INPUT buffer				*/
#define	c_sbrsbw		11						/* pointer to special READ/WRITE word pair		*/
#define	c_curtrk		12						/* pointer to disk head position words (F0/F1/R0/R1, in that order)	*/
#define	c_devtab		13						/* pointer to additional device driver table	*/

#define	c_offset		16						/* highcore configuration table starts here		*/

/* system configuration/options */
#define	c_memsiz		16						/* memory size (in sectors)						*/
#define	c_cmopt			17						/* computer and music options					*/

#define	memory_sectors	able_core(c_contab + c_memsiz)	/* memory size (in sectors)						*/
#define	options_word	able_core(c_contab + c_cmopt)	/* computer and music options					*/

#define	xpl_memory_size	shl(memory_sectors, 8)			/* memory size (in words) was memory_size       */
#define	processor_type	(shr(options_word, 8) & 0x0007)	/* processor type (0: Model A, 1: Model B, 2: Model C, etc.	*/
#define	d03_speed		2						/* hardcode to 200 Hz for now					*/

/* option bits */
#define o_bootable      0x0002					/* set in options word if program is self-booting */
#define	o_d4567			0x0008					/* set in options word if D4567 is in system	*/
#define	o_d40			0x0010					/* set in options word if D40 (printer) is in system	*/
#define	o_d164			0x0020					/* set in options word if D164 (kbd filter/trigger bd) is in system	*/
#define	o_d44			0x0040					/* set in options word if D44 (mouse) is in system	*/
#define	o_pvoice		0x0080					/* set in options word if mono/3200 poly voices are installed	*/
#define	o_dsp			0x1000					/* set in options word if DSP is attached to DTD	*/
#define	o_d66			0x2000					/* set in options word if D66 is in system		*/
#define	o_d130			((fixed) 0x4000)		/* set in options word if D130 is in system		*/
#define	o_d160			((fixed) 0x8000)		/* set in options word if D160 is in system		*/

/* option tests */

#define	is_bootable	    ((options_word & o_bootable) != 0)		/* True ifis bootable							*/
#define	d4567_present	((options_word & o_d4567) != 0)			/* True if D4567 is in system					*/
#define	d40_present		((options_word & o_d40) != 0)			/* True if D40 is in system						*/
#define	d164_present	((options_word & o_d164) != 0)			/* True if D164 is in system					*/
#define	mouse_present	((options_word & o_d44) != 0)			/* True if D44 is in system						*/
#define	mono_voices_present	((options_word & o_pvoice) != 0)	/* True if 3200/mono poly voices are in system	*/
#define	dsp_present		((options_word & o_dsp) != 0)			/* True if DSP is attached to DTD				*/
#define	d66_present		((options_word & o_d66) != 0)			/* True if D66 is in system						*/
#define	d130_present	((options_word & o_d130) != 0)			/* True if D130 is in system					*/
#define	d160_present	((options_word & o_d160) != 0)			/* True if D160 is in system					*/

/* terminal/printer information */
#define	c_ptype			18										/* terminal information							*/
#define	c_stype			19										/* printer information							*/
#define	c_redrct		20										/* redirection word								*/

#define	terminal_info	able_core(c_contab + c_ptype)			/* terminal information							*/
#define	printer_info	able_core(c_contab + c_stype)			/* printer information							*/
#define	redirection_word	able_core(c_contab + c_redrct)		/* redirection word								*/

#define	terminal_model	shr(terminal_info, 8)					/* terminal model								*/
#define	terminal_type	(shr(terminal_info, 8) & 0x001F)		/* terminal type								*/
#define	terminal_graphics	shr(terminal_info, 13)				/* terminal graphics							*/
#define	terminal_nulls	(terminal_info & 0x00FF)				/* terminal nulls								*/

#define	printer_type	(shr(printer_info, 8) & 0x001F)			/* printer  type								*/
#define	printer_graphics	shr(printer_info, 13)				/* printer  graphics							*/
#define	printer_nulls	(printer_info & 0x00FF)					/* printer  nulls								*/

#define	directed_output	(redirection_word & 0x00FF)	/* output redirection information (0: to terminal; 1: to printer)	*/
#define	directed_input	shr(redirection_word, 8)	/* input redirection information (0: from terminal; 1: from printer)	*/
#define	no_redirection	0							/* cancel all redirection by setting REDIRECTION_WORD to this	*/

/* $page */

/* graphics */
#define	g_vt640			1
#define	g_dq640			2
#define	g_mg600			3
#define	g_macintosh		4

/* terminal models */
#define	t_unknown		0
#define	t_hardcopy		1
#define	t_adm3			2
#define	t_vt100			3
#define	t_datamedia		4
#define	t_z19			5

#define	t_vt640			(t_vt100 | shl(g_vt640, 5))	/* Vt100 w/graphics G#VT640						*/
#define	t_dq640			(t_vt100 | shl(g_dq640, 5))	/* Vt100 w/graphics G#DQ640						*/
#define	t_mg600			(t_vt100 | shl(g_mg600, 5))	/* Vt100 w/graphics G#MG600						*/
#define	t_macintosh		(t_vt100 | shl(g_macintosh, 5))	/* Vt100 w/graphics G#MACINTOSH					*/

/* printer types */
#define	p_unknown		0
#define	p_deciii		9
#define	p_la34			10						/* Decwriter IV - has graphics capabilities		*/
#define	p_printronix	11						/* has graphics capabilities					*/
#define	p_prism			12						/* has graphics capabilities					*/
#define	p_diablo		13						/* has graphics capabilities					*/

/* operating system version */
#define	c_version		21						/* configuration/system version number minus six (-6 is 0, -7 is 1, etc)	*/

#define	c_current_version	1					/* current OS version is -7						*/

/* storage device information */
#define	c_curdev		23						/* current device (physically encoded)			*/
#define	c_sysdev		24						/* actually points to first storage table entry */

#define	current_device	(able_core(c_contab + c_curdev))	/* current device (physically encoded)			*/
#define	system_device	(able_core(c_contab + c_sysdev))	/* system device (physically encoded)			*/

#define	curtyp			(current_device & 0x000F)	/* current device type (0=floppy, 1=winchester, 2=remote)	*/
#define	systyp			(system_device  & 0x000F)	/* system  device type (0=floppy, 1=winchester)	*/

#define	c_strdev		24						/* start of storage device table				*/
#define	c_strend		104						/* end   of storage device table (plus one)		*/

#define	c_strlen		(c_strend - c_strdev)	/* length of storage device table				*/

/* storage device table entry layout */

/* For storage device table usage: checkout :xpl:scsi for interpration.							*/
/* Also: :utilcat:condev sets them, somewhat differently...										*/

#define	s_freblk		-1						/* indicates a free block in the device table	*/

#define	c_conlen		c_strend				/* length of configuration table (since the storage table is last)	*/

#define	s_devtyp		0	/* 8 bits:					4 bits:				4 bits:						*/
							/* 	1 = SCSI					0 = drive 0			0 = Floppy				*/
							/* 								1 = drive 1			1 = W0/W1 winchester	*/
							/*													2 = R0/R1 floppy		*/
							/*													3 = T0/T1 tape			*/
							/*													4 = O0/O1 optical		*/
							/*	5 = for M/O Optical														*/
							/* e.g bits 10,11: 0 == 12", 1 == 5"										*/
						
#define	s_seccyl		1 	/* sectors/cylinder 														*/
#define	s_totcyl		2 	/* total cylinders 															*/

#define	s_spdtrk		3	/* sectors per track/stepping speed for floppies							*/
#define	s_devadr		3	/* device address (drive/controller) for Winchesters						*/
							/* 4 bits: unknown	4 bits: LUN		4 bits: which board		4 bits: target	*/
							/* note: for board 1, LUN becomes host ID and LUN is zero					*/
#define	s_blklen		4	/* length of device table entry												*/


/* D40q baud rates (SHL by 8 before writing to D41, D43, D45, or D51) */

#define	b_300			0						/* 300 baud										*/
#define	b_1200			1						/* 1200 baud									*/
#define	b_2400			2						/* 2400 baud									*/
#define	b_4800			3						/* 4800 baud									*/
#define	b_9600			4						/* 9600 baud									*/
#define	b_19200			5						/* 19200 baud									*/
#define	b_38400			6						/* 38400 baud									*/
#define	b_76800			7						/* 76800 baud									*/
#define	b_enable		((fixed) 0x8000)		/* Or in to enable baud rate selection			*/

/* $subtitle Symbolic Definitions For Upper Memory */

/* The ABLE Series Monitor stores many important items of information
.  in the upper part of memory in order to preserve them between
.  program overlays (and to pass certain information to cooperative
.  programs).  This information includes items such as the name of
.  the user's current file, where on the disk it is stored, etc. */

/* current file definitions */
#define	loc_cfn			(mem_siz -   5)			/* current file name							*/
#define	loc_ftyp		(mem_siz -   6)			/* current file type							*/
#define	loc_svd			(mem_siz -   7)			/* True if file is saved						*/
#define	loc_strd		(mem_siz -   8)			/* True if file stored							*/
#define	loc_strn		(mem_siz -  13)			/* stored name of file							*/
#define	loc_cmed		(mem_siz -  14)			/* current file media:  0 = file, 1 = core		*/
#define	loc_csec		(mem_siz -  16)			/* starting sector/device of current file or word ptr if in core	*/
#define	loc_csln		(mem_siz -  17)			/* number of sectors in current file			*/
#define	loc_clen		(mem_siz -  18)			/* current file length mod 64K					*/

/* system, path, and user (current) catalog definitions */
#define	loc_scat		(mem_siz -  20)			/* system catalog sector						*/
#define	loc_sctl		(mem_siz -  21)			/* system catalog length						*/
#define	loc_smax		(mem_siz -  23)			/* system catalog maximum						*/
#define	loc_pcat		(mem_siz -  25)			/* path   catalog sector (0 if no path)			*/
#define	loc_pctl		(mem_siz -  26)			/* path   catalog length						*/
#define	loc_pmax		(mem_siz -  28)			/* path   catalog maximum						*/
#define	loc_pcnm		(mem_siz -  33)			/* path   catalog name							*/
#define	loc_ucat		(mem_siz -  35)			/* user   catalog sector						*/
#define	loc_uctl		(mem_siz -  36)			/* user   catalog length						*/
#define	loc_umax		(mem_siz -  38)			/* user   catalog maximum						*/
#define	loc_ccnm		(mem_siz -  43)			/* user   catalog name							*/

/* work file definitions */
#define	loc_wmed		(mem_siz -  44)			/* work file media:  0 = file, 1 = core			*/
#define	loc_wsec		(mem_siz -  46)			/* starting sector of work file or word ptr if in core	*/
#define	loc_wsln		(mem_siz -  47)			/* number of sectors in work file				*/
#define	loc_wlen		(mem_siz -  48)			/* work file length mod 64K						*/

/* command file definitions */
#define	loc_perform		(mem_siz -  49)			/* indicates in a command file					*/
#define	loc_perptr		(mem_siz -  50)			/* byte pointer into command file				*/
#define	loc_perfsec		(mem_siz -  52)			/* starting sector of command file				*/
#define	loc_perflen		(mem_siz -  53)			/* word length of command file					*/

/* system file locations/lengths (named after the system file they're most often used for) */
#define	loc_mon			(mem_siz -  55)			/* monitor (starting sector at LOC.MON, word length (or -1) at LOC.MON - 1)	*/
#define	loc_p1			(mem_siz -  58)			/* pass1										*/
#define	loc_p2			(mem_siz -  61)			/* pass2										*/
#define	loc_p3			(mem_siz -  64)			/* pass3										*/
#define	loc_st			(mem_siz -  67)			/* symbol table									*/
#define	loc_rt			(mem_siz -  70)			/* runtime file									*/
#define	loc_mplt		(mem_siz -  73)			/* music printing								*/

/* overlay parameters/return values */
#define	loc_rst			(mem_siz -  75)			/* run status (to MONITOR)						*/
#define	loc_usr1		(mem_siz -  76)			/* user defined									*/
#define	loc_usr2		(mem_siz -  77)			/* user defined									*/
#define	loc_usr3		(mem_siz -  78)			/* user defined									*/

/* monitor bit definitions */
#define	loc_monbits		(mem_siz -  79)			/* bits for monitor ON/OFF states				*/
#define	m_log			1						/* history logging on/off						*/

/* words (MEM.SIZ - 80) through (MEM.SIZ - 82) are currently unused */

/* $page */

/* Monitor state variables */
#define	loc_tyb			(mem_siz - 138)			/* 110 char tty buffer (plus length)			*/
#define	loc_tybp		(mem_siz - 139)			/* byte pointer into tty buffer					*/
#define	loc_inch		(mem_siz - 140)			/* input character								*/
#define	loc_cnum		(mem_siz - 141)			/* current command number						*/
#define	loc_prmt		(mem_siz - 146)			/* 8 char command prompt						*/
#define	loc_screen		(mem_siz - 147)			/* lines/screen on the terminal (0 means the terminal isn't screen-oriented)	*/

/* Monitor bootload state variables */
#define	loc_magic		(mem_siz - 148)			/* location of magic number						*/
#define	loc_psys		(mem_siz - 149)			/* previous SYSTYP to detect bootload error		*/

/* Sed state variables */
#define	loc_sed1		(mem_siz - 150)			/* reserved for sed use							*/
#define	loc_sed2		(mem_siz - 151)			/* reserved for sed use							*/

/* configuration/device information */
#define	loc_ctab		(mem_siz - 239)			/* system configuration							*/
#define	loc_headpos		(mem_siz - 243)			/* drive F0, F1, R0, and R1 head positions		*/
#define	loc_emsize		(mem_siz - 244)			/* external memory size less amount reserved for MONITOR (in sectors)	*/

/* word (MEM.SIZ - 245) is currently unused */

/* Synclavier (r)/SCRIPT state variables */
#define	loc_synrtpn		(mem_siz - 250)			/* 8 char name of default Synclavier RTP		*/
#define	loc_synmed		(mem_siz - 251)			/* synclavier file media (0 = normal memory, 1 = expanded memory)	*/
#define	loc_synmisc		(mem_siz - 252)			/* pointer to synclavier miscellaneous area		*/
#define	loc_syntimb		(mem_siz - 253)			/* pointer to synclavier timbre area			*/
#define	loc_synseq		(mem_siz - 254)			/* pointer to synclavier sequence notes			*/
#define	loc_synstat		(mem_siz - 255)			/* synclavier status word						*/
#define	loc_synret		(mem_siz - 256)			/* pointer to word trio (above) describing program to return to if no room	*/

/* overlay routine (768 words) */
#define	loc_load		(mem_siz - 1024)		/* load/overlay routine stored here				*/

#define	mem_offs		256						/* offset from a one K boundary (above 16K and below MEM.SIZ) of memory size word	*/

/* Note:  all longs are stored with the LS at the location and the MS at the location + 1. */


/* external memory state variables (only there if core(loc.emsize) <> 0) */
#define	loc_emarea		loc_emsize				/* sector start of external memory state variables	*/

/* word offsets from LOC.EMAREA */
#define	em_polycache	0						/* True if poly cache valid						*/
#define	em_perform		256						/* nested perform file state (50 words + 56 words command line)	*/
