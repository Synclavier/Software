/*
******************************************************************* 
*	NED Toolkit - MIDI Interface File
*	
*	Paul Forstman
*
*	Copyright © 1990-91 by New England Digital Corporation
******************************************************************* 
*/

/*
	Modified:
	05/02/91	PF  - Fixed saving sequence trashes routings
	03/22/90	PF  - Added "Buss" field
	10/30/89	PF  - Created this file
*/



#ifndef NED__MIDI
#define NED__MIDI


#include "environ.h"

#define	MIDI_ID				(MIDI << 8)			/* In decimal MIDI_ID = 1280 */


/**************** resource id's					****************/

#define RECALL_SEQ_ID		MIDI_ID + 0			/* DLOG ID'S */
#define	ALERT_ID			MIDI_ID + 1
#define CONFIRM_ID			MIDI_ID + 2
#define QUERY_ID			MIDI_ID + 3

#define UTILITY_STRS_ID		MIDI_ID + 0			/* STR# ID's */
#define	ALERT_STRS_ID		MIDI_ID + 1
#define CONFIRM_STRS_ID		MIDI_ID + 2
#define QUERY_STRS_ID		MIDI_ID + 3

#define DEFAULT_CNFGR_ID	MIDI_ID + 0			/* MnCF ID's */


#ifndef rez

/**************** utility STR# indexs			****************/

#define SAVE_AS				1				
#define UNTITLED			2				


/**************** alert items and STR# indexs	****************/

#define ALERT_TEXT_ITEM		2
#define MERGE_ERR_ID		1
#define RES_ERR_ID			2
#define SAVE_RES_ERR		3
#define OPEN_RES_ERR		4
#define CREATE_RES_ERR		5
#define ADD_RES_ERR			6
#define RES_MEM_ERR			7
#define NO_REV_RES_ERR		8
#define UNKNOWN_REV_ERR		9
#define SAVE_CNFGR_RES_ERR	10
#define CLOSE_RES_ERR		11
#define NO_CNFGR_RES_ERR	12
#define MIDINET_CRASH		13
#define	MIDINET_TIMEOUT		14
#define MIDINET_NAK			15
#define PANIC_STR			16
#define ABOUT_MIDINET		17
#define IO_ERROR_STR		18
#define TYPE_MISMATCH		19
#define NO_CONNECT			20
#define NO_DEFAULTS			21
#define PASTE_ERR_ID		22
#define OVERRUN_ERR			23
#define SYSEX_TIMEOUT_ERR	24
#define BAD_INPUT_ERR		25
#define NO_PATHS			26
#define NO_INPUT_CHANNELS	27
#define NO_PB_ENTRIES		28
#define NO_CHAINS			29
#define NO_OUTPUT_CHANNELS	30
#define MEM_ERR				31
#define PORT_ERR			32
#define NO_BUSSES			33
#define CONVERT_FILE		34
#define NO_OUT_BUSSES		35
#define DUP_BUSS_CHAN		36
#define STACK_OVERFLOW		37
#define MOVE_BLOCKS_ERR		38
#define UNEQUAL_CONNECT_ERR	39
#define	NO_FROM_ERR			40
#define	UNEQUAL_INSERT_ERR	41
#define	NO_SOURCE_ERR		42
#define	INSERT_BLOCKS_ERR	43
#define	GOOD_IMPORT			44
#define	MPB_ROM_REV			45
#define	GOOD_EXPORT			46
#define	BAD_EXPORT			47


/**************** confirm items and STR# indexs	****************/

#define	CONFIRM_TEXT_ITEM	3
#define	SET_CNFGR_DEFAULT	1
#define	RESET_HARDWARE		2
#define	DELETE_FILE			3
#define REVERT_TO_SAVED		4
#define SAVE_SEQUENCE		5
#define	RECALL_SEQUENCE		6
#define REVERT_TO_SEQUENCE	7


/**************** query items and STR# indexs	****************/

#define	QUERY_TEXT_ITEM		4
#define SAVE_CHANGES		1


#define	MIDINET_TYPE		'NDMn'				/* MIDInet file type id */
#define	MAGIC_1				0x01000000			/* magic file number for 1st release */
#define	MAGIC_2				0x02000000			/* magic file number for 1st release */
#define MAX_ROWS			256    				/* number of rows allowed - must be a power of 2 for now... see note in display.c */
#define MAX_COLS			15					/* number of columns defined */
#define MAX_FLDS			21					/* number of fields defined */

#define NEW_FILE			1
#define FILE_STATUS			2
#define NEW_FILENAME		3


/**************** bit masks for flags word in block structure ****************/

#define SELECT_MASK			1
#define	SORT_MASK			2			
#define COPY_MASK			4	
#define	COPY_OUT_MASK		8
#define COPY_BACK_MASK		16


/**************** block type indexs 			****************/

#define CHANM_TYPE			1			/* channel map */
#define CF_TYPE				2			/* controller filter */
#define CONM_TYPE			3			/* controller map */
#define EF_TYPE				4			/* event filter */
#define PF_TYPE				5			/* pitch filter */
#define TP_TYPE				6			/* transpose */
#define VS_TYPE				7			/* velocity scale */
#define MI_TYPE				8			/* midi input */
#define ST_TYPE				9			/* synclavier track */
#define MO_TYPE				10			/* midi output */
#define	MON_TYPE			11			/* midi monitor */


/**************** col type indexs 				****************/

#define	MI_COL				0			/* midi input */
#define PROC1_COL			1			/* first process block column of first group */
#define	ST_COL				7			/* synclavier track */
#define PROC2_COL			8			/* first process block column of second group */
#define MO_COL				14			/* midi output */
#define NUM_PROC_COLS		6			/* number of process columns per group */


/**************** item indexs for event filter dialog	****************/

#define EF_NOTE_OFF		2			/* note off checkbox */
#define EF_NOTE_ON		3			/* note on */
#define PRESSURE		4			/* poly pressure */
#define CTRL_CHANGE		5			/* control change */
#define PRGM_CHANGE		6			/* program change */
#define PB_CHANGE		7			/* pitch bend change */
#define SYS_EX			8			/* system exclusive */
#define MTC				9			/* midi time code */
#define SYS_COMMON		10			/* system common */
#define REAL_TIME		11			/* real time messages */
#define EVENT_CLEAR		12			/* clear button */
#define EVENT_SET		13			/* set button */


/**************** routing literals					****************/

#define	MAX_MIDINET_PORTS		128							/* maximum number of physical ports (in/out) */
#define MAX_BUSSES				8							/* maximum number of input/output busses in midinet */
#define MAX_INPUT_CHANNELS		127							/* one less than NUM.INPUT.CHANNELS defined in RTP source - because MIDInet can't send on channel 127 */
#define NUM_RESERVED_CHANNELS	15							/* number of input channels MIDInet reserves for special uses */
#define	MONITOR_CHAN			MAX_INPUT_CHANNELS - 1		/* RESERVED CHANNEL #1 - used exclusively for "monitoring" midi activity */ 
#define SYNC_CHAN				MAX_INPUT_CHANNELS - 2		/* RESERVED CHANNEL #2 - used exclusively for midi in sync */
#define NUM_INPUT_CHANNELS		MAX_INPUT_CHANNELS - NUM_RESERVED_CHANNELS		/* number of input channels MIDInet actually uses to route to Synclavier */
#define MAX_CONNECTIONS			16							/* maximum number of connections on any block to or from */
#define OLD_MAX_CHAINS			15							/* original maximum number of process chains for each source */
#define MAX_CHAINS				18							/* new maximum number of process chains */


/**************** typedefs							****************/

typedef struct {					/* midinet block structure definition */

	Handle	id;						/* copy of handle to self for ID */
	Handle	fwd;					/* handle to next block in linked list */
	Handle	bkwd;					/* handle to previous block */
	Handle	to[16];					/* 16 handles to blocks we connect to */
	Handle	from[16];				/* 16 handles to blocks that connect to us */
	Handle	data;					/* handle to our blocks data */
	int16	data_len;				/* length of data */
	int16	row;					/* row on grid where currently residing */
	int16	col;					/* col on grid where currently residing */					
	int16	type;					/* block type - input, pitch filter, track etc... */
	int16	pb;						/* process block directory entry associated with block */
	int		flags;					/* misc status flags */
	char	name[33];				/* block name array */

} MIDINetBlock,*Bptr,**Bhandle;		/* define a block pointer and a block handle */

typedef struct blk{					/* definition for simple block */
	struct blk	**fwd;				/* handle to next block if list includes multiple blocks */
	struct blk	**bkwd;				/* handle to previous block in linked list */
	int16 		row;				/* blocks row */
	int16 		col;				/* blocks col */
} block,*bptr,**bhandle;			/* define a block, ptr and handle */
		
typedef struct {
	long	fld_visible;
	int16	width;
	int16	height;
	Point	loc;
	int16	top;
	int16	left;
	int16	row;
	int16	col;
	int16	fldwidth[MAX_FLDS];
} mncf, **mncf_hndl;						/* MIDInet configuration */


/**************** prototypes						***************/

#ifdef __cplusplus
extern "C" {
#endif

/* from file.c */

bool		new_midinet_file(Str255 fname);										/* create an empty midinet file with passed name */	
bool		save_midinet(void);													/* save the current midinet file */
bool		save_midinet_as(void);												/* for save with no disk file open yet */
bool		open_midinet_file(Str255 fname,int16 vnum);							/* read in a new "patch" from the disk */
bool		dispose_midinet_file(bool release_data);							/* release current file and clean up */
void		file_dirty(Boolean dirty);											/* mark the current file as clean or dirty */
bool		file_is_dirty (void);												/* returns current file status */
bool		check_write_file(Str255 fname, int16 vrefnum, int16* rnum);			/* check file selected by SFPutFile */
bool		io_check(OSErr code);												/* check for i/o error */
bool		err_check(Handle hndl, int16 code, const Str255 fname, int16 err_index);	/* check for and report system resource error */
void		get_filename (Str255 str);											/* returns current file name in "str" */
int16		get_file_number (void);												/* returns file number of current file - zero if no disk file opened */
bool		write_cnfgr_to_resource(Str255 fname);								/* write the current window configuration to the passed resource file */
bool		write_busses_to_resource(Str255 fname);								/* write the current output bussing to the file's resource */
bool		write_revision_to_resource(Str255 fname);							/* write the revision magic number to the file's resource */
mncf_hndl	get_cnfgr(void);
void		link_list(bhandle the_list, bool link_to_extern);					/* link all connecting blocks */
void		link_board(Bhandle board);											/* takes care of all list linking after a read */
void		midinet_revert(void);
void		save_sequence(void);												/* responds to Synclavier report to save sequence */
bool		recall_sequence(Str255 fname);										/* responds to Synclavier new MIDInet filename */

/* from process.c */

void		compile_process	(Bhandle block, bool is_new_type);
void		reload_midinet (void);

/* from route.c */

void		compute_routing_stack(void);										/* compute the routing of all entries in routing stack */
void		recompute_shared_routing (Bhandle block);							/* recompute any track routing sharing path of passed block */
void		send_output_switching (int16 port);									/* send the buss to "port" mapping */
void		send_all_output_switching (void);									/* send all current output routings to MRS's using bulk messages */
void		compute_synclavier_routing(void);									/* clear and then compute all synclavier sources */
void 		check_routing(Bhandle block);										/* checks for and performs updates to routing structures */
void		find_sources(Bhandle block, Boolean from_within);					/* finds all sources (inputs/tracks) connected to passed block */
void		find_sources_of_list(bhandle list);									/* finds the sources(routing structures) of all the blocks in the passed list */
void		add_to_routing_stack(int16 source);									/* adds a source to the routing_stack */	
void		synclavier_all_notes_off(int8 channel);								/* all notes off to Able */
bool		to_available(Bhandle block);										/* returns TRUE if any "to" entries available in passed block */
bool		from_available(Bhandle block);										/* returns TRUE if any "from" entries available in passed block */
void		find_num_paths(Bhandle block, int* count);							/* returns in "count" number of process paths "block" is routed to */
void		push_routing_stack(void);											/* copies current routing stack */
void		pop_routing_stack(void);											/* reads routing stack back in */
int16		get_input_buss (int16 port);										/* lookup the current input buss assignment for this port */								 
int16		allocate_input_buss (int16 port);									/* allocate a free input buss for this port */
void		compute_input_buss_usage (void);									/* zero buss assignments for inactive inputs */
void		compute_output_buss_usage (void);									/* zero buss assignments for inactive outputs */

/* from sort.c */

void		delete_track_routing(int16 track, bool delete_track);				/* delete all (unshared) blocks connected to the track */
void		disconnect_track_routing (int16 track);								/* just disable the routing without losing any blocks */
void		add_track (int16 track);											/* create a block for this track if not already displayed */
void		new_timbres (int status);											/* respond to Synvlav new timbre event */
int			new_routing (int16 track, int8 path);								/* handle MIDInet routing packets from Able */
void		sort_blocks (int16 col, int16 fld, bool valid_arguments);			/* sort the display - use passed col/fld if valid arguments */
bool		in_midinet_list(int16 row, int16 col, bhandle list);				/* is the passed block on the passed list */

/* from utils.c */

void		add_to_list(int16 row, int16 col, bhandle* the_list);				/* add this block to the current selection linked list */					
void		remove_from_list(int16 row, int16 col, bhandle* the_list);			/* remove the passed block from the_list */					
void		copy_list(bhandle source_list, bhandle* dest_list);
void		clear_midinet_list (bhandle* the_list);								/* removes all blocks except the "current" one from the current selection */
void		reinit_midi(void);													/* action to take when Able comes back */
bool		midinet_confirm (int16 str_index);
Bhandle		get_block_handle (int16 row, int16 col);							/* routine to get block handle */
void		set_block_handle (int16 row, int16 col, Bhandle hndl);				/* routine to set block handle */
void 		clear_clipboard(Bhandle* board);
void		paste_undo(Bhandle* clip_board, bool link_to_external, bhandle *list, Bhandle merge_block);		/* special paste */
void		disconnect(int16 row1, int16 col1, int16 row2, int16 col2);			/* disconnect two blocks */
void		create_midinet_block(int16, int16);									/* creates an empty block at the passed location */
void 		delete_midinet_block(int16 row, int16 col);							/* delete the current block */ 
void		set_block_type(int16, int16, int16);								/* assigns a type to the passed block */
void		add_to_clipboard(Bhandle Block, Bhandle* board, bool clr_trk_names);
void 		remove_from_clipboard(Bhandle block, Bhandle* board);
Bhandle		on_board(Handle id, Bhandle board);
void		initialize_midi(void);												/* main initialization routine for midi */
bool		get_monitor_mode (void);
void		set_monitor_mode (bool);
void		reset_midi (void);													/* action to take if Able goes away */

#ifdef __cplusplus
}
#endif


/**************** externals							****************/

extern Bhandle			block_handle;			/* points to head of block linked list */
extern int				max_midi_chan;			/* max number of original midi outputs 0-15 */
extern int				max_midinet_chan;		/* max number of MIDInet ports */
extern int16			routing_stack[128];		/* holds sources (MIDI Inputs/Synclavier Tracks) found by "find_sources" */
extern unsigned char	num_midinet_ports;		/* run time configuration of router/switchers */
extern char				in_busses[];			/* input buss array */
extern char*			out_busses;				/* pointer to output buss array */
extern int				num_chains;				/* holds number of process chains available in MIDInet hardware */
extern int				rom_rev;				/* MPB rom revision number */
extern bool				in_snapshot;			/* indicates we are in the middle of a MIDInet routing snapshot */
extern bool				more_pe_seq_name;		/* if TRUE another pe_seq_name event was received during last snapshot */


#endif		/* not rez */

#endif		/* NED__MIDI */