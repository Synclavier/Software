/*	xpl_compiler.h				*/

/*	Created: 11/12/96 at 07:51	*/
/*	Version: 0.000				*/

/* general local header file	*/
/* for communication amongst	*/
/* different components of the	*/
/* XPL compiler					*/

#include "XPL.h"

#include <stdio.h>

extern	int 	compiler_main   	(int argc,  char *argv[]);
extern	short	locate_include_file (void* pb,  const char* inclname, char **textH, void **baseH, FSSpec *specH);
extern	void	release_include_file(void* pb,  void *base) ;
extern	void	report_error_string (void *pb, long lineno, long offset, void *spec);

#define INTER_FILE_WORD_SIZE	(2048*1024)	/* size in 16-bit words...								*/
#define	INTER_FILE_1_START      ( 256*1024) /* word offset of start of pass1 inter file				*/
#define	INTER_FILE_2_START      ( 128*1024) /* word offset of start of pass2 inter file				*/

#define num_alias_structs	40				/* number of structs available							*/
#define num_info_spaces		40				/* number of elements available in each					*/

#define	_blocks				310				/* temp blocks											*/
#define	stacksize			2100			/* push down stack										*/
#define	ins_levels			25				/* allow 20 levels of insert files						*/

typedef	struct								/* struct for memory  management of alias lists			*/
{
	fixed	key;
	fixed	num;

	fixed	info[num_info_spaces];

}	alias_struct;

extern	char			built_symbol_table[2048];		/* holds pre-constructed symbol table		*/
extern	alias_struct	aliases[num_alias_structs];		/* holds structs for alias mapping			*/

extern	void	*global_cpb;				/* CW parameter block									*/

extern	FILE 	*in_file;					/* input file											*/
extern	FILE 	*out_file;					/* output file											*/
extern	FILE 	*swap_file;					/* swap file											*/
extern	FILE 	*sym_file;					/* sym file												*/
extern	FILE 	*rtp_file;					/* rtp file												*/
extern	char	*in_text;					/* input file if in memory								*/
extern	char	*out_data;					/* output file if in memory								*/
extern	void	*in_base;					/* handle to in-memory file (for closing)				*/
extern	FSSpec	in_spec;					/* FSSpec for file for error message					*/

extern	char	source_file_name [256];
extern	char 	object_file_name [256];
extern	char 	swap_file_name   [256];
extern	char	host_source_file [256];
extern	char	host_swap_file   [256];

extern	fixed 	*inter_file;				/* intermediate file work space							*/
extern	long	inter_file_word_length;

extern	void	sgen_main();				/* constructs xpl symbol table into built_symbol_table	*/
extern	void	set_flags(fixed);			/* set compile-time flags								*/
extern	void	init_pass1_statics();		/* init (some!) pass1 statics for re-entry				*/

extern	long	pass1_main();				/* performs compilation pass 1							*/
extern	long	pass2_main();				/* performs compilation pass 2							*/
extern	long	pass3_main();				/* performs compilation pass 3							*/

/* shared varaibles: */

extern	fixed	store1 [_blocks + 1];		/* storage area one										*/
extern	fixed	store2 [_blocks + 1];		/* symbol table pointers								*/
extern	fixed	store3 [_blocks + 1];		/* and expression trees									*/
extern	fixed	store4 [_blocks + 1];
extern	fixed	store5 [_blocks + 1];

extern	fixed	stack  [stacksize + 1];

extern	boolean	show_progress;
extern	boolean show_p3_progress;
extern	boolean	run_quietly;

extern	void*	inserted_stack[ins_levels + 1];
extern	fixed	inserted_stack_pointer;
