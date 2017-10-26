/* XPL to C Language Translator */

/* Created - 7/14/96	C. Jones */

/* C header files */

#include 	<string.h>
#include 	<stdlib.h>
#include 	<stdio.h>
#include 	<signal.h>

#include    "XPL.h"
#include    "XPLRuntime.h"
#include    "SyncMutex.h"

#include	"defs.h"			/* include able files.  by the way, which	*/
#include	"p1-defs.h"			/* came first, the chicken or the egg?		*/
#include	"p12-defs.h"

#define		FALSE	0
#define		TRUE	1

/*--------------------------------------------------------------------------------------*/
/* Main - put at front to combat linker problems										*/
/*--------------------------------------------------------------------------------------*/

/* MWlink68K seems to make MPW tools single segment in all cases... Main is put here	*/
/* so it can be called from __startup__													*/

int translate(int argc, char *argv[]);

int main(int argc, char *argv[])
{
    // Static variable initialization
    SyncMutex::InitAll();
    
	return (translate(argc, argv));
}

/*--------------------------------------------------------------------------------------*/
/* Additional token types used by translater only									    */
/*--------------------------------------------------------------------------------------*/

/* Special token types recognized by scanner to preserve intelligent syntax:			*/

#define t_pointer		 12				/* indicates pointer data type					*/
#define	t_cstring		 15				/* indicates array arg is of type cstring		*/
#define	t_cconst         35				/* indicates c-format string to be used always	*/
#define	t_comment		118				/* indicates scanned comment (translater only	*/
#define	t_white			119				/* indicates scanned white space			 	*/
#define t_eol			120				/* indicates eol scanned						*/
#define t_octconst		121				/* indicates octal constant scanned				*/
#define t_insert		122				/* special handling needed for insert stmt	 	*/
#define	t_lbr			123				/* left  bracket								*/
#define	t_rbr			124				/* right bracket								*/
#define t_lbra			125				/* left brace									*/
#define	t_rbra			t_end			/* right brace									*/

#define	t_character		127				/* character printout specifier					*/

#define	t_apo			128				/* apostophe									*/
#define	t_octal			129				/* octal printout specifier						*/
#define	t_remapsymb		130				/* map symbol in all cases						*/
#define	t_remapproc		131				/* mapped procedure name						*/

#define	x_paren			6				/* expression enclosed in parentheses			*/

#define	s_else			30				/* handle else simply here...					*/
#define s_core			31				/* handle core assignment via statement			*/
#define	s_native		32				/* XPL Native statement							*/

#define	o_core			60				/* special handling for 'core' required			*/


/* Procedure attributes: preserve attributes like "recursive" and "swap" to allow		*/
/* for re-translation and/or compilation by the "C-Enhanced" XPL compiler.  Although	*/
/* these attributes are not required by C, the are required by the "C-Enhanced" XPL		*/
/* compiler to recreate the original able object code from the translated source.		*/

#define	attr_recursive	0x0001
#define	attr_swap		0x0002
#define	attr_swapcode	0x0004

/* special codes to identify types of end statements: */

#define	t_nop				0			/* no special required							*/
#define	t_break				1			/* emit break beore closing } (switch stmt)		*/
#define t_procend			2			/* emit #undefs for literals					*/


/*--------------------------------------------------------------------------------------*/
/* XPL Character Lookup Table (modified for translator use)								*/
/*--------------------------------------------------------------------------------------*/

typedef	short fixed;

short info_table[128] = {
/* ctl- abcdefg */	0, 0, 0, 0, 0, 0, 0, 0,
/* ctl-hijklmno */	0, b_spa, b_eol + b_comnt, 0, 0, b_eol + b_comnt, 0, 0,
/* ctl-pqrstuvw */	0, 0, 0, 0, 0, 0, 0, 0,
/* ctl-xyz[\]^_ */  0, 0, 0, 0, 0, 0, 0, 0,
/*      !"#$%&'	*/	b_spa, b_relop + b_not, 0, b_symb, b_symb + b_comnt, b_opr + o_fmu, b_opr + o_and, 0,
/*     ()*+     */	b_spec + t_lpar, b_spec + t_rpar, b_opr + o_times + b_comnt, b_opr + o_plus,
/*    	,-./	*/	b_spec + t_comma, b_opr + o_minus, b_symb, b_opr + o_div + b_comnt,
/*     0123     */	b_digit + b_symb, b_digit + b_symb, b_digit + b_symb, b_digit + b_symb, 
/*     4567		*/	b_digit + b_symb, b_digit + b_symb, b_digit + b_symb, b_digit + b_symb, 
/*     89:;		*/  b_digit + b_symb, b_digit + b_symb, b_spec + t_colon, b_spec + t_semi,
/*     <=>?     */	b_relop + b_lt, b_relop + b_eq, b_relop + b_gt, 0,
/*     @ABC		*/	0, b_symb, b_symb, b_symb,
/*     DEFG		*/	b_symb, b_symb, b_symb, b_symb,
/*     HIJK		*/  b_symb, b_symb, b_symb, b_symb,
/*     LMNO     */  b_symb, b_symb, b_symb, b_symb, 
/*     PQRS     */  b_symb, b_symb, b_symb, b_symb,
/*     TUVW     */	b_symb, b_symb, b_symb, b_symb,
/*     XYZ[		*/	b_symb, b_symb, b_symb, b_spec + t_lbr,
/*     \]^_		*/	b_opr + o_or, b_spec + t_rbr, b_relop + b_not, b_symb,
/*     `abc 	*/  0, b_symb + b_lcase, b_symb + b_lcase, b_symb + b_lcase,
/*     defg     */	b_symb + b_lcase, b_symb + b_lcase, b_symb + b_lcase, b_symb + b_lcase,
/*     hijk     */	b_symb + b_lcase, b_symb + b_lcase, b_symb + b_lcase, b_symb + b_lcase,
/*     lmno     */	b_symb + b_lcase, b_symb + b_lcase, b_symb + b_lcase, b_symb + b_lcase,
/*     pqrs     */	b_symb + b_lcase, b_symb + b_lcase, b_symb + b_lcase, b_symb + b_lcase,
/*     tuvw     */	b_symb + b_lcase, b_symb + b_lcase, b_symb + b_lcase, b_symb + b_lcase,
/*     xyz{		*/  b_symb + b_lcase, b_symb + b_lcase, b_symb + b_lcase, b_spec + t_lbra,
/*     |}~		*/	b_opr + o_or, b_spec + t_rbra, b_relop + b_not, b_spec + t_eof + b_comnt};


/*--------------------------------------------------------------------------------------*/
/* User specified global variables														*/
/*--------------------------------------------------------------------------------------*/

int	comment_pos = 72;						/* default comment position					*/
int comment_len = 72;						/* default comment length					*/
int define_pos  = 28;						/* default position of #defines...			*/

boolean	convert_includes   = FALSE;			/* true to convert include files			*/
boolean	convert_compatibly = TRUE;			/* true to conver to compatible syntax		*/
boolean	recognize_c_syntax = FALSE;			/* true to recognize C-compatible XPL		*/
boolean	create_one_file    = FALSE;			/* true to combine all insert file into 1	*/
boolean	create_xpl_strings = FALSE;			/* true to create string constants in xpl	*/
boolean show_progress      = FALSE;			/* true to emit progress info during trans	*/
boolean indent_comments    = TRUE;			/* true to indent comments to current level	*/
boolean show_extended 	   = FALSE;			/* true to show extended progress			*/
boolean do_detab           = TRUE;          /* convert tabs to spaces                   */
boolean type_cast_consts   = FALSE;         /* typecast constant literals to fixed      */
boolean is_header          = FALSE;         /* converted file is only a header file     */
int     tab_indent         = 3;

char source_file_name[512] = {""};			/* source file name							*/
char cur_dir_name    [512] = {""};			/* xpl cur dir								*/
char master_dir_name [512] = {""};			/* xpl mas dir								*/
char out_dir_name    [512] = {""};			/* output directory							*/


/*--------------------------------------------------------------------------------------*/
/* Global variables & structs															*/
/*--------------------------------------------------------------------------------------*/

typedef	struct								/* holds basic info about a symbol			*/
{	
	char        *symbol_name;
	int         type;
	long long   info;
	void        *other;
	int         scope;
}	symbol_struct;

typedef	struct								/* holds more info about rtp calls			*/
{	
	int		return_type;					/* holds return type						*/
	int		arg_types[1];					/* holds type of each arg - t_var or t_arr	*/
}	proc_struct;

#define	TOKEN_LENGTH	16384				/* allow very long comments...				*/
#define	MAIN_LENGTH		32768				/* holds code to move to main()				*/
#define	TEMP_LENGTH		16384				/* holds code till we know wher eit goes	*/

int				next        = 0;			/* global; holds next input character		*/
int				next_info   = 0;			/* info bits about next character			*/
int				next_count  = 0;			/* char pos in line of scanned character	*/
int 			ccount      = 0;			/* count character pos; for errors			*/
int 			lcount      = 1;			/* count lines; for errors					*/
int				tcount      = 0;			/* count of output characters, for tab		*/
int				pcount      = 0;			/* count of output characters, for proto	*/
int				indent		= 0;			/* indent level								*/
int				proc_level  = 0;			/* nonzero if within proc					*/
int				scope_level = 0;			/* symbol definition scope					*/

char			*token      = 0;			/* holds basic scanned token				*/
int				token_type  = 0;			/* token type indication					*/
int				token_info  = 0;			/* holds further info about scanned token	*/
symbol_struct	*token_sym  = 0;			/* points to scanned symbol info			*/
int				token_count = 0;			/* position in line of start of token		*/

char			*main_code  = 0;			/* holds main code							*/
char			*main_buf   = 0;			/* working pointer into main_code			*/
char			*temp_code  = 0;			/* temp code holder							*/
int				main_avail  = 0;
int				main_count  = 0;

boolean			insert_needed  = TRUE;		/* insert of .h still needed				*/
boolean			hmuldiv_valid  = FALSE;		/* track use if multiply/divide unit		*/
boolean			mute_output    = FALSE;		/* use to mute output for insert files		*/
boolean			remap_all_lits = FALSE;		/* remap all lits; used for read/write		*/

CFDateFormatterRef date_formater = CFDateFormatterCreate ( NULL,  CFLocaleCopyCurrent(), kCFDateFormatterLongStyle, kCFDateFormatterLongStyle);

static void		error_exit();
char 			error_file_name  [512] = {""};


/*--------------------------------------------------------------------------------------*/
/*	Symbol table management																*/
/*--------------------------------------------------------------------------------------*/

#define	STORAGE_LENGTH	256*1024			/* general allocated storage				*/
#define	SYMBOL_LENGTH   20000				/* for symmbol index						*/

char			*storage    = 0;
symbol_struct 	**symlist   = 0;
int				storage_ptr = 0;
int				sym_count   = 0;

static symbol_struct * insert_symbol(const char *symbol, int type, int info)
{
	int				symbol_length = (strlen(symbol       ) + 4) & 0xFFFFFFFC;
	int				struct_length = (sizeof(symbol_struct) + 3) & 0xFFFFFFFC;
	char			*sym_ptr;
	symbol_struct	*struct_ptr;
	
	/* make sure room in memory */
	if (storage_ptr + symbol_length + struct_length >= STORAGE_LENGTH)
		{printf("xpltoc: out of symbol table storage\n"); error_exit();}
	
	if (sym_count >= SYMBOL_LENGTH)
		{printf("xpltoc: out of symbol table storage\n"); error_exit();}
	
	/* copy symbol name into place */	
	sym_ptr = storage + storage_ptr;
	strcpy (sym_ptr, symbol);
	storage_ptr += symbol_length;
	
	/* copy symbol info into place */
	struct_ptr = (symbol_struct *) (storage + storage_ptr);
	struct_ptr->symbol_name = sym_ptr;
	struct_ptr->type        = type;
	struct_ptr->info        = info;
	struct_ptr->other       = NULL;
	struct_ptr->scope       = scope_level;
	
	storage_ptr += struct_length;
	
	/* store symbol in index */
	
	symlist[sym_count++] = struct_ptr;
	
	return (struct_ptr);
}

static char * store_literal(char *the_string)
{
	int		string_length = (strlen(the_string) + 4) & 0xFFFFFFFC;
	char 	*string_ptr   = storage + storage_ptr;
	
	/* make sure room in memory */
	if (storage_ptr + string_length >= STORAGE_LENGTH)
		{printf("xpltoc: out of symbol table storage\n"); error_exit();}
	
	/* copy string into place */	
	strcpy (string_ptr, the_string);
	storage_ptr += string_length;
	
	return (string_ptr);
}

static symbol_struct * find_symbol(const char *symbol)
{
	int i;
	
	if (sym_count >= SYMBOL_LENGTH)
		{printf("xpltoc: system error with symbol table storage (%d %d)\n", sym_count, SYMBOL_LENGTH); error_exit();}

	for (i=sym_count-1; i >= 0; i--)
	{
		if (strcmp(symbol, symlist[i]->symbol_name) == 0)
			return (symlist[i]);
	}
	
	return (0);
}

static	void	symbol(const char *name, int type, int info)
{
	insert_symbol(name, type, info);
}

static	proc_struct *	store_proc_args (int num_args, int arg_types[], int return_type)
{
	int				struct_length = (sizeof(((proc_struct *)0) -> return_type)
	                              +  sizeof(((proc_struct *)0) -> arg_types[0])*num_args
								  + 3) & 0xFFFFFFFC;
	proc_struct		*struct_ptr;
	int				i;	
	
	/* make sure room in memory */
	if (storage_ptr + struct_length >= STORAGE_LENGTH)
		{printf("xpltoc: out of symbol table storage\n"); error_exit();}
	
	/* copy arg info into place */
	struct_ptr = (proc_struct *) (storage + storage_ptr);
	
	struct_ptr->return_type = return_type;
	
	for (i=0; i < num_args; i++)
		struct_ptr->arg_types[i] = arg_types[i];

	storage_ptr += struct_length;
	
	return (struct_ptr);
}

static	symbol_struct *	defrtp (const char *name, int /* rtp loc */, int num_args,
                                int return_type, int arg1_type, int arg2_type, 
						        int arg3_type, int arg4_type)
{
	symbol_struct	*sym_struct;
	int				arg_types[4];
	
	arg_types[0] = arg1_type;
	arg_types[1] = arg2_type;
	arg_types[2] = arg3_type;
	arg_types[3] = arg4_type;
	
	sym_struct = insert_symbol(name, t_rtp, num_args);

	sym_struct->other = store_proc_args(num_args, arg_types, return_type);
	
	return (sym_struct);
}

symbol_struct *	shr_proc = NULL;
symbol_struct *	shl_proc = NULL;

static	void	construct_symbol_table()
{
	symbol ("abs", t_opr, o_math + 0);
	symbol ("log", t_opr, o_math + 1);
	symbol ("atn", t_opr, o_math + 2);
	symbol ("cos", t_opr, o_math + 3);
	symbol ("sin", t_opr, o_math + 4);
	symbol ("tan", t_opr, o_math + 5);
	symbol ("exp", t_opr, o_math + 6);
	symbol ("sqr", t_opr, o_math + 7);
	
	symbol ("int",  t_opr, o_int);
	symbol ("addr", t_opr, o_adr);
	
	symbol ("not", t_opr, o_not);
	
	if (!convert_compatibly)				/* scan shr/shl as operators if we want to convert them to <<, >>	*/
	{
		symbol ("shr", t_sdy, o_shr);
		symbol ("shl", t_sdy, o_shl);
	}
	
	if (!recognize_c_syntax)				/* handle xpl-format fdiv if not scanning XPL-compatible C			*/
		symbol ("fdiv", t_opr, o_fdi);
	
	symbol ("mod",  t_opr, o_mod);
	
	symbol ("and", t_opr, o_and);
	symbol ("or",  t_opr, o_or);
	symbol ("xor", t_opr, o_xor);
	
	symbol ("ieq", t_opr, o_eq);
	symbol ("ilt", t_opr, o_ilt);
	symbol ("ile", t_opr, o_ile);
	symbol ("igt", t_opr, o_igt);
	symbol ("ige", t_opr, o_ige);
	symbol ("ine", t_opr, o_ne );
	
	symbol ("core", t_opr, o_core);
	 
	  
	/* build symbol table:  statements and keywords_ */
	 
	symbol ("call",          t_stmt, s_call);
	symbol ("invoke",        t_stmt, s_call);
	symbol ("return",        t_stmt, s_return);
	symbol ("proc",          t_stmt, s_proc);
	symbol ("procedure",     t_stmt, s_proc);
	symbol ("begin",         t_stmt, s_begin);
	symbol ("do",            t_stmt, s_do);
	symbol ("if",            t_stmt, s_if);
	symbol ("goto",          t_stmt, s_goto);
	symbol ("enable",        t_stmt, s_enable);
	symbol ("disable",       t_stmt, s_disable);
	symbol ("stop",          t_stmt, s_stop);
	symbol ("write",         t_stmt, s_write);
	symbol ("linput",        t_stmt, s_linput);
	symbol ("input",         t_stmt, s_input);
	symbol ("print",         t_stmt, s_print);
	symbol ("send",          t_stmt, s_send);
	symbol ("when",          t_stmt, s_when);
	symbol ("module",        t_stmt, s_module);
	symbol ("library",       t_stmt, s_library);
	symbol ("insert",        t_stmt, s_insert);
	symbol ("dcl",           t_stmt, s_declare); /* pass1 only emits these in special cases (stack allocation) */
	symbol ("declare",       t_stmt, s_declare);
	symbol ("enter",         t_stmt, s_enter); /* start of statements processed by pass1 only */
	symbol ("pdl",           t_stmt, s_pdl);
	symbol ("ram",           t_stmt, s_ram);
	symbol ("configuration", t_stmt, s_config);
	symbol ("_if",		     t_stmt, s_sharp_if);
	symbol ("_elif", 	 	 t_stmt, s_sharp_elseif);
	symbol ("_else",  	 	 t_stmt, s_sharp_else  );
	symbol ("_endif",  	 	 t_stmt, s_sharp_endif );

	/* keywords: */
	  
	symbol ("loc",       	t_locat,  0); /* array specifier */
	symbol ("location", 	t_locat,  0); /* array specifier */
	symbol ("chr",       	t_pform,  1); /* print format */
	symbol ("character", 	t_pform,  1); /* print format */
	symbol ("octal",     	t_pform,  0); /* print format */
	symbol ("string",    	t_string, 0); /* print format */
	symbol ("eof",       	t_eof,    0);
	symbol ("while",    	 t_while,  0);
	symbol ("case",      	t_case,   0);
	symbol ("to",        	t_to,     0);
	symbol ("by",        	t_by,     0);
	symbol ("end",       	t_end,    0);
	symbol ("then",      	t_then,   0);
	symbol ("else",      	t_stmt,   s_else);
	symbol ("read",      	t_opr,    o_read);
	symbol ("returns",   	t_rtns,   0); 					/* definition of procedure return value */
	symbol ("recursive", 	t_recurs, attr_recursive); 		/* define a procedure to be recursive */
	symbol ("swap",      	t_swap,   attr_swap); 			/* define a procedure to be swappable */
	symbol ("swapcode",  	t_swpcode,attr_swapcode); 		/* allows swapping but not of string constants or data arrays */
	symbol ("native",		t_stmt,   s_native);			/* xpl native statement */
	 
	/* symbol types for declarations: */
	 
	symbol ("fixed",     t_type, t_var);
	symbol ("boolean",   t_type, t_var);
	symbol ("pointer",   t_type, t_pointer);
	symbol ("array",     t_type, t_arr);
	symbol ("cstring",   t_type, t_cstring);
	symbol ("label",     t_type, 5);
	symbol ("lit",       t_type, 6);
	symbol ("literally", t_type, 6);
	symbol ("data",      t_type, 7);
	
	symbol ("external",  t_storage, t_extern); /* external reference 			  */
	symbol ("public",    t_storage, t_public); /* external definition 			  */
	symbol ("static",    t_storage, s_static); /* static variable 				  */
	symbol ("automatic", t_storage, s_automatic); /* automatic variable 		  */
	
	symbol ("true",  t_const, 1); /* boolean true */
	symbol ("false", t_const, 0); /* boolean false */
	symbol ("null",  t_const, 0); /* null pointer */
	  
	/* dynamic construction of symbol table:  interrupt keywords_
	_  
	_   the format for processing interrupts via the when statement is:
	_   
	_      when lncint then do;
	_         anything;
	_         anything;
	_      end;
	_   
	_   or:
	_   
	_      when lncint then lbsyflg = 0;
	_   
	_   the following interrupt cell identifier names are recognized:
	*/
	  
	symbol ("break",     t_icell,  0);
	symbol ("ttoint",    t_icell,  1);
	symbol ("ttiint",    t_icell,  2);
	symbol ("d16int",    t_icell,  5);
	symbol ("d03int",    t_icell,  7);
	symbol ("diskerror", t_icell,  8);
	symbol ("d140int",   t_icell,  9);
	symbol ("d136int",   t_icell, 10);
	symbol ("d137int",   t_icell, 11);
	symbol ("d115int",   t_icell, 13);
	symbol ("bdb14int",  t_icell, 14);
	symbol ("bdb15int",  t_icell, 15);
	symbol ("d40int",    t_icell, 16); /* start of d54 acknowledge: id = 16 + bdb # */
	symbol ("d42int",    t_icell, 17);
	symbol ("d44int",    t_icell, 18);
	symbol ("d46int",    t_icell, 19);
	symbol ("d66int",    t_icell, 20);
	symbol ("d24int",    t_icell, 23);
	symbol ("d30int",    t_icell, 24);
	symbol ("d31int",    t_icell, 25);
	symbol ("d32int",    t_icell, 26);
	symbol ("d33int",    t_icell, 27);
	symbol ("d34int",    t_icell, 28);
	symbol ("d35int",    t_icell, 29);
	symbol ("d36int",    t_icell, 30);
	symbol ("d37int",    t_icell, 31);
	symbol ("dsp70int",  t_icell, 40); /* start of d55 acknowledge: id = 32 + bdb # */
	symbol ("d132int",   t_icell, 43);
	
	/* symbols for configuration statement: */
	
	symbol ("maxi",       t_config,  0);
	symbol ("mini",       t_config,  1);
	symbol ("dmini",      t_config,  2);
	symbol ("smini",      t_config,  3);
	symbol ("modelb",     t_config,  4);
	symbol ("modelc",     t_config,  5);
	symbol ("modeld",     t_config,  6);
	symbol ("memory",     t_config,  7);
	symbol ("muldiv",     t_config,  8);
	symbol ("nomuldiv",   t_config,  9);
	symbol ("ptype",      t_config, 10);
	symbol ("stype",      t_config, 11);
	
	/* Define commonly used Synclavier RTP variables that are used as dynamic pointers	*/
	/* to record structurs...															*/
	
	symbol ("nptr",            t_var, 0);
	symbol ("pptr",            t_var, 0);
	symbol ("gpdt",            t_arr, 0);
	symbol ("epqbuf",          t_arr, 0);
	symbol ("ipqbuf",          t_arr, 0);
	symbol ("timer_phase",     t_var, 0);
	symbol ("kbd_mono3",       t_var, 0);
	symbol ("switchdata",      t_arr, 0);
	symbol ("analog_in",       t_arr, 0);
	symbol ("samp_click_rate", t_var, 0);
	symbol ("mouse_cursor_on", t_var, 0);
	
	/* runtime package routine names: */
	  
	defrtp ("exit",          l_ter, 1, t_var, t_var, 0, 0, 0);
	defrtp ("byte",          l_byt, 2, t_var, t_arr, t_var, 0, 0);
	defrtp ("pbyte",         l_pbt, 3, t_var, t_arr, t_var, t_var, 0);
	defrtp ("rcvdcharacter", l_ich, 0, t_var, 0, 0, 0, 0);
	defrtp ("blockmove",     l_bmv, 3, t_var, t_arr, t_arr, t_var, 0);
	defrtp ("blockset",      l_bst, 3, t_var, t_arr, t_var, t_var, 0);
	defrtp ("export",        l_ept, 4, t_var, t_var, t_var, t_arr, t_var);
	defrtp ("import",        l_ipt, 4, t_var, t_var, t_var, t_arr, t_var);
	defrtp ("extset",        l_est, 4, t_var, t_var, t_var, t_var, t_var);
	defrtp ("readdata",      l_rfl, 4, t_var, t_var, t_var, t_arr, t_var);
	defrtp ("writedata",     l_wfl, 4, t_var, t_var, t_var, t_arr, t_var);
	defrtp ("swapinit",      l_swi, 1, t_var, t_var, 0, 0, 0);
	defrtp ("find_device",   l_fde, 1, t_var, t_var, 0, 0, 0);
	defrtp ("set_curdev",    l_scur,1, t_var, t_var, 0, 0, 0);
	defrtp ("extread",       l_erd, 3, t_var, t_var, t_var, t_arr, 0);
	defrtp ("extwrite",      l_ewr, 3, t_var, t_var, t_var, t_arr, 0);
	defrtp ("polyread",      l_prd, 4, t_var, t_var, t_var, t_arr, t_var);
	defrtp ("polywrite",     l_pwr, 4, t_var, t_var, t_var, t_arr, t_var);
	
	/* In c, handle these operators by function call syntax	*/
	
	defrtp ("rot",               0,     2, t_var, t_var, t_var, 0, 0);

	if (convert_compatibly)		/* scan shr, shl as function calls if converting them to C macros	*/
	{
		shr_proc = defrtp ("shr",           0,     2, t_var, t_var, t_var, 0, 0);
		shl_proc = defrtp ("shl",           0,     2, t_var, t_var, t_var, 0, 0);
	}
	
	if (recognize_c_syntax)		/* scan fdiv, fmul as function calls if scanning XPL-compatible C	*/
	{
		defrtp ("fdiv",          0,     2, t_var, t_var, t_var, 0, 0);
		defrtp ("fmul",          0,     2, t_var, t_var, t_var, 0, 0);
		defrtp ("_igt_",         0,     2, t_var, t_var, t_var, 0, 0);
		defrtp ("_ige_",         0,     2, t_var, t_var, t_var, 0, 0);
		defrtp ("_ile_",         0,     2, t_var, t_var, t_var, 0, 0);
		defrtp ("_ilt_",         0,     2, t_var, t_var, t_var, 0, 0);
		defrtp ("_scale_",       0,     3, t_var, t_var, t_var, t_var, 0);

		symbol ("#include",  t_stmt,    s_insert);
		symbol ("extern",  	 t_storage, t_extern);
	}
}


/*--------------------------------------------------------------------------------------*/
/*	Stack management																	*/
/*--------------------------------------------------------------------------------------*/

#define	STACK_SIZE		400

long long	the_stack[STACK_SIZE];
int	        stack_ptr = 0;

static	void        push(long long it)
{
	if (stack_ptr >= STACK_SIZE)
		{printf("xpltoc: out of stack storage\n"); error_exit();}
	
	the_stack[stack_ptr++] = it;
}

static	long long   pop()
{
	if (stack_ptr == 0)
		{printf("xpltoc: stack underflow\n"); error_exit();}
	
	return the_stack[--stack_ptr];
}

static	void	push_scope()
{
	push(storage_ptr);
	push(sym_count  );
}

static	void	pop_scope()
{
	sym_count   = pop();
	storage_ptr = pop();
}


/*--------------------------------------------------------------------------------------*/
/*	Insert Stack management																*/
/*--------------------------------------------------------------------------------------*/

int	insert_stack[STACK_SIZE];
int	insert_stack_ptr = 0;

static	void        insert_push(int it)
{
	if (insert_stack_ptr >= STACK_SIZE)
		{printf("xpltoc: out of insert stack storage\n"); error_exit();}
	
	insert_stack[insert_stack_ptr++] = it;
}

static	long long   insert_pop()
{
	if (insert_stack_ptr == 0)
		{printf("xpltoc: insert stack underflow\n"); error_exit();}
	
	return insert_stack[--insert_stack_ptr];
}

/*--------------------------------------------------------------------------------------*/
/*	End Stack management																*/
/*--------------------------------------------------------------------------------------*/

int	end_stack[STACK_SIZE];
int	end_stack_ptr = 0;

static	void	end_push(int it)
{
	if (end_stack_ptr >= STACK_SIZE)
		{printf("xpltoc: out of end stack storage\n"); error_exit();}
	
	end_stack[end_stack_ptr++] = it;
}

static	int		end_pop()
{
	if (end_stack_ptr == 0)
		{printf("xpltoc: end stack underflow\n"); error_exit();}
	
	return end_stack[--end_stack_ptr];
}


/*--------------------------------------------------------------------------------------*/
/*	Literal Stack management																*/
/*--------------------------------------------------------------------------------------*/

long long	lit_stack[STACK_SIZE];
long long	lit_stack_ptr = 0;
char *current_lit = NULL;

static	void        lit_push(long long it)
{
	if (lit_stack_ptr >= STACK_SIZE)
		{printf("xpltoc: out of lit stack storage\n"); error_exit();}
	
	lit_stack[lit_stack_ptr++] = it;
}

static	long long   lit_pop()
{
	if (lit_stack_ptr == 0)
		{printf("xpltoc: lit stack underflow\n"); error_exit();}
	
	return lit_stack[--lit_stack_ptr];
}


/*--------------------------------------------------------------------------------------*/
/* file proessing - input, output, insert												*/
/*--------------------------------------------------------------------------------------*/

FILE *in_file     = NULL;
FILE *out_file    = NULL;
FILE *proto_file  = NULL;

static	void	put_proto(const char *symbol);	/* need forward reference here             */

char out_file_name    [512] = {""};
char proto_file_name  [512] = {""};

#define	OUT_BUF_LENGTH		256*1024		/* storage for two-pass procedure scan			*/
#define PROTO_BUF_LENGTH	1024			/* storage for prototype						*/

char *out_buf        = NULL;				/* working variables of where to store output	*/
char *proto_buf      = NULL;				/* characters if we are buffering them for		*/

int	 out_buf_avail   = 0;					/* later output									*/
int  proto_buf_avail = 0;

handle proc_handl    = NULL;
char   *proc_code    = NULL;				/* pointer to storage allocated for holding		*/
char   *proto_code   = NULL;				/* main body of proc & prototype				*/

static void error_log()
{
	if (error_file_name[0])					/* if file name available, print it & line no	*/
		printf("file '%s'; line %d  #(or line %d)#\n", error_file_name, lcount, lcount-1);
}
	
static void error_exit()
{
	error_log();
	
	exit(1);
}
	
static void open_source_file(char *file_name)
{
	char in_file_name    [512];
	
	if (file_name[0] == ':')				/* file name begins with ":" : use master dir	*/
	{
		int i = strlen(master_dir_name);	/* get length of master directory				*/
		strcpy (in_file_name, master_dir_name);
	
		strcat (in_file_name, file_name);
	}
	
	else
	{
		strcpy (in_file_name, cur_dir_name);
		strcat (in_file_name, file_name   );
	}
	
    for (int i = 0; i < strlen(in_file_name); i++)
        if (in_file_name[i] == ':')
            in_file_name[i] = '/';
            
	if ((in_file = fopen(in_file_name, "r")) == 0)
		{printf("xpltoc: can't open source file %s (%s)\n", file_name, in_file_name); error_exit();}

	strcpy(error_file_name, in_file_name);	/* copy file name for error messages			*/
	lcount = 1;
}

static void open_output_file(char *file_name)
{
	OSErr	FSstatus;

    char output_line[512];
    char lc_name    [512];
    
    strcpy(lc_name, file_name);
	
    for (int i = 0; lc_name[i]; i++)
        lc_name[i] = tolower(lc_name[i]);
    
	strcpy (out_file_name, out_dir_name);
	strcat (out_file_name, lc_name);

    if (is_header)
        strcat (out_file_name,   ".h");
    
    else {
        strcat (out_file_name,   ".c");

        strcpy (proto_file_name, out_dir_name);
        strcat (proto_file_name, lc_name);
        strcat (proto_file_name, ".h");
    }
    
	if (out_dir_name[0] == 0)					/* if no directory specified	*/
        {printf("xpltoc: Failed open_output_file (%s) - no master directory\n", out_file_name); error_exit();}
	
    for (int i = 0; i < strlen(out_file_name); i++)
        if (out_file_name[i] == ':')
            out_file_name[i] = '/';
    
    for (int i = 0; i < strlen(proto_file_name); i++)
        if (proto_file_name[i] == ':')
            proto_file_name[i] = '/';
    
    // Create intermediate directories
    CSynclavierFileReference& fileRef = * new CSynclavierFileReference(out_file_name, true);
    CSynclavierFileRefManager mgr(&fileRef);
    
    fileRef.MkPath();
    
	/* delete prior file: */
    if (*out_file_name) {
        if ((out_file = fopen(out_file_name, "rb+")) != 0)	/* if can open...	*/
        {
            fclose(out_file);
            remove(out_file_name);
            out_file = NULL;
        }
        
        if ((out_file = fopen(out_file_name, "wb+")) == 0)
            {printf("xpltoc: can't open output file %s (%s)\n", file_name, out_file_name); error_exit();}
    }
    
    if (*proto_file_name) {
        if ((proto_file = fopen(proto_file_name, "rb+")) != 0)	/* if can open...	*/
        {
            fclose(proto_file);
            remove(proto_file_name);
            proto_file = NULL;
        }
        
        if ((proto_file = fopen(proto_file_name, "wb+")) == 0)
            {printf("xpltoc: can't open proto file %s (%s)\n", file_name, proto_file_name); error_exit();}
    }
    
	/* Construct basic header file contents: */
	sprintf(output_line, "/*\tHeader file for: %s.c", file_name);
	put_proto(output_line);
	if (strlen(file_name) <= tab_indent) put_proto("\t");
	put_proto("\t*/\n");

	put_proto("\n");
    
    CFStringRef dateString = CFDateFormatterCreateStringWithAbsoluteTime(NULL, date_formater, CFAbsoluteTimeGetCurrent());
    char now[512] = {0};
    
    CFStringGetCString(dateString, now,  sizeof(now), kCFStringEncodingUTF8);

    sprintf(output_line, "/*\tCreated: %s\t*/\n", now);

    put_proto(output_line);

	sprintf(output_line, "/*\tVersion: 0.000\t\t\t\t*/\n");
	put_proto(output_line);

	put_proto("\n");
}
		

/*--------------------------------------------------------------------------------------*/
/* input and output character processing												*/
/*--------------------------------------------------------------------------------------*/

static	void	get()						/* handy routine - get next character into	*/
{											/* global variable 							*/
	if (current_lit)						/* if parsing a remapped literal...			*/
	{
		next = *current_lit++;				/* get it									*/
		
		if (next == 0)						/* if end of string							*/
		{
			current_lit = (char *) lit_pop();
			next_count  = lit_pop();
			next_info   = lit_pop();
			next        = lit_pop();
			return;
		}
	}
	
	else if (in_file)						/* else if reading from a file...			*/
		next = fgetc(in_file) & 127;		/* get char; ignore upper bit				*/
	else
		next = getchar() & 127;				/* get char; ignore upper bit				*/
	
	next_info  = info_table[next];			/* look up character info					*/
	next_count = ccount++;					/* save line position of this character		*/
    
    if (next == '\r')                       /* normalize to UNIX endings                */
        next = '\n';

	if (next == '\n' || next == EOF) {ccount = 0; lcount++;}
	
	if (show_extended)
		printf("%c", next);
}

static	void put(int it)					/* output a character						*/
{
	if (mute_output) return;
    
    // Handle de-tab. Recursively.
    if (it == '\t' && do_detab) {
        put(' ');
        
        while (tcount % tab_indent)
            put(' ');
        
        return;
    }
	
	if (out_buf)							/* handle outputting to temp buffer			*/
	{
		if (--out_buf_avail == 0)
			{printf("xpltoc: out of output buffer memory\n"); error_exit();}
			
		*out_buf++ = it;
		*out_buf   = 0;
	}
		
	else if (out_file)
		fputc (it, out_file);
	
	else
		printf("%c", it);
	
	if (it == '\n') tcount = 0;
	else if (it == '\t') tcount = ((tcount + tab_indent) / tab_indent) * tab_indent;
	else tcount++;
}

static	void	tab(int n)					/* tab output file to position N			*/
{
	if (mute_output) return;

	while (tcount < n)
		put('\t');
}

static	void	white(int n)				/* tab output file to position N with some	*/
{											/* white space emitted						*/
	if (mute_output) return;

	if (tcount >= n)						/* at least one white space					*/
		put('\t');
		
	while (tcount < n)
		put('\t');
}

static	char	temp_list[512] = {""};

static	void	put_symbol(const char *symbol)
{
	while (*symbol)
	{
		if (*symbol == 1)					/* handle meta tab to indent				*/
			tab(indent);
		
		else if (*symbol == 2)				/* handle meta tab to comment				*/
			tab(((indent / 24) * 24) + comment_pos);
			
		else if (*symbol == 3)				/* handle meta insertion of temp list		*/
		{
			put_symbol(temp_list);			/* emit temp list, with meta chars			*/
			temp_list[0] = 0;
		}
		
		else
			put(*symbol);
		
		symbol++;
	}
}

static	void	put_symbol_with_indent(char *symbol)		/* fix indentation issues	*/
{															/* with this handy puppy	*/
	if (*symbol)											/* indent 1 if not null		*/
		put('\t');
		
	while (*symbol)											/* emit string				*/
	{
		put(*symbol);										/* emit character			*/
		
		if (*symbol == '\n' && *(symbol+1))					/* if eol and not end of	*/
			put('\t');										/* string, indent next line	*/
		
		symbol++;
	}
}

static	void	emit_c_string(char *it)
{
	int i,j;
	
	put_symbol("\"");
	
	for (i = 0; i < strlen(it); i++)
	{
		j = (unsigned) it[i];
		
		if ((j == '\"') 						/* precede " ' \ ? with			*/
		||  (j == '\\')							/* backslash					*/
		||  (j == '\?')
		||  (j == '\'')
		||  (j == '\n')
		||  (j == '\t'))
			put_symbol("\\");
			
		if      (j == '\n') j = 'n';
		else if (j == '\t') j = 't';
			
		put(j);
	}
	
	put_symbol("\"");
}
					
static	void	emit_xpl_string(char *it)
{															/* mangle letters and cast		*/
	char	the_string[20];
	int		i,j,k,l,m;
	
	put_symbol("\"\\0\"");									/* zero fill uperr[0]			*/
	sprintf(the_string, "\"\\x%02x\"\"", (int)strlen(it));
	put_symbol(the_string);									/* put out length				*/
	
	i = strlen(it);											/* get actual length of string	*/
	j = (i+1) & 0xFFFFFFFE;									/* get # of bytes to emit		*/
	
	for (k=0; k<j; k++)
	{
		l = k ^ 1;											/* swap byte order				*/
		if (l >= i) put_symbol("\"\"\\0\"\"");
		else
		{
			m = (unsigned) it[l];
			
			if ((m == '\"') 								/* precede " ' \ ? with			*/
			||  (m == '\\')									/* backslash					*/
			||  (m == '\?')
			||  (m == '\''))
				put_symbol("\\");
				
			put(m);
		}
	}
	
	put_symbol("\"");										/* end quote					*/
	
	/* Also emit comment with the real string in it so people			*/
	/* can deal															*/
	
	put_symbol(" /* \"");
	
	for (l = 0; l < i; l++)
	{
		m = (unsigned) it[l];
		
		if ((m == '\"') 									/* precede " ' \ ? with			*/
		||  (m == '\\')										/* backslash					*/
		||  (m == '\?')
		||  (m == '\''))
			put_symbol("\\");
			
		put(m);
	}
	
	put_symbol("\" */");
}

/* emit string as data for fixed point variable */
static	void	emit_xpl_data_string(char *it)
{															/* mangle letters and cast		*/
	char	the_string[20];
	int		i,j,k;
	
	sprintf(the_string, "%d", (int)strlen(it));
	put_symbol(the_string);
	
	for (i=0; i<strlen(it); i += 2)
	{
		put_symbol(", ");
		
		j = it[i  ];
		k = it[i+1];
	
		put_symbol("'");

		if (k != 0)
		{
			if ((k == '\"') 								/* precede " ' \ ? with			*/
			||  (k == '\\')									/* backslash					*/
			||  (k == '\?')
			||  (k == '\''))
				put_symbol("\\");
				

			the_string[0] = k;
			the_string[1] = 0;
			put_symbol(the_string);
		}
		
		if ((j == '\"') 								/* precede " ' \ ? with			*/
		||  (j == '\\')									/* backslash					*/
		||  (j == '\?')
		||  (j == '\''))
			put_symbol("\\");
				
		the_string[0] = j;
		the_string[1] = 0;
		put_symbol(the_string);

		put_symbol("'");
	}

	/* Also emit comment with the real string in it so people			*/
	/* can deal															*/
	
	put_symbol(" /* \"");
	
	for (i = 0; i < strlen(it); i++)
	{
		j = (unsigned) it[i];
		
		if ((j == '\"') 									/* precede " ' \ ? with			*/
		||  (j == '\\')										/* backslash					*/
		||  (j == '\?')
		||  (j == '\''))
			put_symbol("\\");
			
		put(j);
	}
	
	put_symbol("\" */");
}

static	void	put_proto(const char *symbol)
{
	if (mute_output) return;

	while (*symbol)
	{
        // Handle de-tab. Recursively.
        if (*symbol == '\t' && do_detab) {
            put_proto(" ");
            
            while (pcount % tab_indent)
                put_proto(" ");
            
            return;
        }
        
		if (proto_buf)							/* handle outputting to temp buffer			*/
		{
			if (--proto_buf_avail == 0)
				{printf("xpltoc: out of prototype output buffer memory\n"); error_exit();}
				
			*proto_buf++ = *symbol;
			*proto_buf   = 0;
		}
		
		else if (proto_file)
			fputc (*symbol, proto_file);
		
		if (*symbol == '\n') pcount = 0;
		else if (*symbol == '\t') pcount = ((pcount + tab_indent) / tab_indent) * tab_indent;
		else pcount++;
		
		symbol++;
	}
}

static	void	proto_tab(int n)					/* tab proto file to position N			*/
{
	if (mute_output) return;

	while (pcount < n)
		put_proto("\t");
}


static	void	expand_short_comment(char *it, int len)		/* lengthen right edge			*/
{
	int i = strlen(it);										/* get length of comment		*/
	int j;													/* ignoring possible tabs		*/

	if (i < 4)												/* oops?						*/
		return;
		
	if (it[i-2] != '*' || it[i-1] != '/')					/* return if // style comment	*/
		return;
	
	i -= 2;													/* toss trailing '*' '/'		*/
	
	while (it[i-1] == ' ' || it[i-1] == '\t')				/* then trailing white space	*/
		i--;

	j = i;

	if (i >= len)
		it[j++] = '\t';
		
	else while (i < len)
	{
		it[j++] = '\t';
		i = ((i+tab_indent) / tab_indent) * tab_indent;
	}
	
	it[j++] = '*';
	it[j++] = '/';
	it[j  ] = 0;
}

static	void	convert_comment(char *it)					/* convert / * to / / comment	*/
{
	int i = strlen(it);										/* get length of comment		*/

	if (i < 4)												/* not long enough				*/
		return;
		
	if (it[i-2] != '*' || it[i-1] != '/')					/* better end in * /			*/
		return;

	if (it[0  ] != '/' || it[1  ] != '*')					/* better start w / *			*/
		return;

	i -= 2;													/* toss trailing '*' '/'		*/
	
	while (it[i-1] == ' ' || it[i-1] == '\t')				/* and trailing white space	*/
		i--;

	it[i] = 0;												/* terminate comment string		*/
	it[0] = '/';
	it[1] = '/';
}


/*--------------------------------------------------------------------------------------*/
/* token scanner subroutines															*/
/*--------------------------------------------------------------------------------------*/

static	int	scan_white_space(char *the_string, int space_left)
{
	while (next_info & b_spa)
	{
		if (space_left <= 1)
			{printf("xpltoc: expression too long or nonterminated comment\n"); error_exit();}
			
		*the_string++ = next;								/* save white space			*/
		space_left--;
		get();
	}

	*the_string = 0;										/* terminate the c string	*/
	
	return (space_left);
}

static	int	scan_comment(char *the_string, int space_left, boolean stop_at_eol)
{
	boolean 	dat_scanned      = FALSE;
	boolean 	leave_next_upper = TRUE;
	boolean		any_lower_found  = FALSE;
	
	if (space_left <= 1)
		{printf("xpltoc: expression too long or nonterminated comment\n"); error_exit();}
		
	*the_string++ = next;								/* accept & save the / or *		*/
	space_left--;
	get();
	
	while (1)											/* find end of commment			*/
	{
		if (space_left <= 5)
			{printf("xpltoc: expression too long or nonterminated comment\n"); error_exit();}
				
		while ((!stop_at_eol && ((next != '*' ) && (next != EOF)))
		||     ( stop_at_eol && ((next != '\n') && (next != EOF))))
		{
			if (next == ' ' && dat_scanned)				/* found a space after a dot 	*/
				leave_next_upper = TRUE;				/* leave cap at start of next 	*/
														/* sentence						*/
			else if (next == '.')						/* leave cap after period		*/
				dat_scanned = TRUE;

			else
				dat_scanned = FALSE;
			
			if (next >= 'A' && next <= 'Z')				/* upper case alpha				*/
			{
				if (!leave_next_upper && !any_lower_found)
					next = next + 'a' - 'A';
					
				leave_next_upper = FALSE;
			}
			
			else if (next >= 'a' && next <= 'z')		/* else if lower case found		*/
				any_lower_found = TRUE;
			
			*the_string++ = next;						/* accept & save character		*/
			space_left--;
			
			if (indent_comments && next == '\n')		/* detect eol inside comment	*/
			{
				*the_string++ = 1;						/* add tab to indent			*/
				space_left--;
			}

			get();
			
			if (space_left <= 5)
				{printf("xpltoc: expression too long or nonterminated comment\n"); error_exit();}
		}
		
		if (next == EOF) {printf( "xpltoc: nonterminated comment\n"); error_exit();}
		
		if (stop_at_eol)
		{
			*the_string = 0;
			return (space_left);						/* done							*/
		}
		
		get();											/* accept the *					*/
		
		if (next == '/')								/* look for terminator			*/
		{
			*the_string++ = '*';						/* store the *					*/
			space_left--;
			*the_string++ = '/';						/* store the /					*/
			space_left--;
			*the_string = 0;							/* terminate C string			*/
			get();										/* accept the /; get next		*/
			return (space_left);						/* done							*/
		}
			
		*the_string++ = '*';							/* else store the *				*/
		space_left--;
	}
}

static	int	scan_symbol(char *the_string, int space_left)
{
	while (next_info & b_symb)
	{
		if (space_left <= 5)
			{printf("xpltoc: symbol too long\n"); error_exit();}
	
		if ((next >= 'A') && (next <= 'Z'))		/* map to lower case; c is case sensitive 	*/
			next += 'a' - 'A';

		else if (next == '.')					/* . to single underline					*/
			next = '_';

		else if (next == '#' && !recognize_c_syntax)
		{
			next = '_';
		}
		
		else if (next == '$')
		{
			next = '_';
		}

		*the_string++ = next;
		space_left--;
		get();
	}

	*the_string = 0;							/* terminate the c string	*/
	return (space_left);
}

static	int	scan_string(char *the_string, int space_left)
{
	get();										/* accept & toss "'" or '"'	*/
	
	while (1)									/* scan to end of string	*/
	{
		if (space_left <= 5)
			{printf( "xpltoc: string constant too long\n"); error_exit();}

		if (next == EOF)
			{printf( "xpltoc: non-terminated string constant\n"); error_exit();}
		
		if (recognize_c_syntax)					/* handle C syntax			*/
		{										/* eg. csan a c string		*/
			if (next == '\\')
				get();							/* then get likely "\'?		*/
			
			else if (next == '\"')				/* else look for terminator	*/
			{
				*the_string = 0;				/* terminate c string		*/
				
				get();							/* done with terminator		*/
				
				return (space_left);
			}
		}
		
		else									/* handle xpl syntax		*/
		{
			if (next == '\'')					/* look for closing '		*/
			{
				get();							/* read it					*/
				
				if (next != '\'')				/* unless "''", done...		*/
				{
					*the_string = 0;			/* terminate c string		*/
					return (space_left);
				}
			}
		}
		
		*the_string++ = (char) next;			/* store char or '			*/
		space_left--;
		get();									/* accept and get			*/
	}
}

static	int scan_decimal()
{
	int     result = 0;
	
	while (next_info & b_digit)
	{
		result = result*10 + next - '0';
		get();
	}
	
	return result;
}

static	int	scan_hex()
{
	int     result = 0;
	
	if (((next_info & b_digit) == 0)
	&&  (next < 'A' || next > 'F')
	&&  (next < 'a' || next > 'f'))
		{printf( "xpltoc: incorrect format in hexidecimal constant\n"); error_exit();}

	while (((next_info & b_digit) != 0)
	||  (next >= 'A' && next <= 'F')
	||  (next >= 'a' && next <= 'f'))
	{
		if ((result & 0xF000) != 0)
			{printf( "xpltoc: too many digits in hexidecimal constant\n"); error_exit();}

		if (next >= 'a')
			result = result*16 + (next - 'a') + 10;
			
		else if (next >= 'A')
			result = result*16 + (next - 'A') + 10;
			
		else
			result = result*16 + (next - '0');
		
		get();
	}
	
	return (result);
}


static	int scan_octal()
{
	int     result = 0;

	get();										/* accept and toss the "				*/
	
	if (next == 'h' || next == 'H')				/* scan hex format						*/
	{
		get();									/* accept and toss the H				*/
	
		while (next != '\"')
		{
			if (next >= '0' && next <= '9')
				result = result * 16 + next - '0';
			else if (next >= 'a' && next <= 'f')
				result = result * 16 + next - 'a' + 10;
			else if (next >= 'A' && next <= 'F')
				result = result * 16 + next - 'A' + 10;
			else
				{printf( "xpltoc: incorrect format in hexidecimal constant\n"); error_exit();}
		
			get();
		}
	}
	else										/* scan octal format					*/
	{
		while (next_info & b_digit)
		{
			if (next == '8' || next == '9')
				{printf( "xpltoc: incorrect format in octal constant\n"); error_exit();}
			
			result = result*8 + next - '0';
			get();
		}
	}
	
	if (next != '\"')
		{printf( "xpltoc: incorrect format in octal constant\n"); error_exit();}

	get();										/* accept and toss the "				*/
	
	return result;
}


/*--------------------------------------------------------------------------------------*/
/* token scanner																		*/
/*--------------------------------------------------------------------------------------*/

boolean return_apos = FALSE;

static void	scan()
{
	if (next_info & b_spa)								/* scan white space for grins	*/
	{
		token_count = ccount;							/* save starting position 		*/
		
		scan_white_space(&token[0], TOKEN_LENGTH-1);

		token_type = t_white;
		token_info = 0;
		token_sym  = 0;
		
		return;
	}

	if (next_info & b_eol)								/* scan eol						*/
	{
		token_count = ccount;							/* save starting position 		*/
		
		token[0]   = next;								/* return eol char				*/
		token[1]   = 0;									/* in c string form				*/
		
		token_type = t_eol;
		token_info = 0;
		token_sym  = 0;
		
		get();											/* accept eol character			*/
		
		return;
	}

	if (next_info & b_spec)								/* special character			*/
	{													/* (),:; eof					*/
		token_count = ccount;							/* save starting position 		*/
		
		token[0]   = next;								/* return special char			*/
		token[1]   = 0;									/* in c string form				*/
		
		token_type = (next_info & b_mask);				/* get token type				*/
		token_info = 0;
		token_sym  = 0;
		
		if (token_type != t_eof) get();					/* get next, except for EOF		*/

		return;
	}
	
	if (next_info & b_opr)								/* single character operator	*/
	{													/* &*+-/\| 						*/
		token_count = ccount;							/* save starting position 		*/
		
		token[0]   = next;								/* return operator				*/
		token[1]   = 0;									/* in c string form				*/
		
		token_type = t_opr;								/* token is operator			*/
		token_info = next_info & b_mask;				/* info is which one			*/
		token_sym  = 0;
		
		get();											/* accept this & get next		*/
		
		if ((token_info == o_and) && (next_info == (b_opr + o_and)))
			get();										/* accept c-style && for &		*/
			
		if ((token_info == o_or) && (next_info == (b_opr + o_or)))
			get();										/* accept c-style || for |		*/
			
		if ((token_info == o_div)						/* look for // for comments		*/
		&   (next       == '/'  ))
		{
			scan_comment(&token[1], TOKEN_LENGTH-1, TRUE);
			token_type = t_comment;
			token_info = 0;
			token_sym  = 0;
		}
			
		if ((token_info == o_div)						/* look for "/ *" for comment   */
		&   (next       == '*'  ))
		{
			scan_comment(&token[1], TOKEN_LENGTH-1, FALSE);
			token_type = t_comment;
			token_info = 0;
			token_sym  = 0;
		}

		return;
	}

	if (next_info & b_relop)							/* relational operator			*/
	{													/* <=>^~						*/
		boolean		not_scanned = FALSE;
		int			i           = 0;
		
		token_count = ccount;							/* save starting position 		*/
		token_type  = t_opr;							/* is operator					*/

		if (next_info & b_not)							/* leading not					*/
		{
			not_scanned = TRUE;
			token[i++] = next;
			get();
		}
		
		token_info = b_relop | b_not;					/* start here...				*/
		
		while ((next_info & token_info) == b_relop)		/* scan while it is a relational, but one that we have not seen yet */
		{
			token_info = token_info + (next_info & b_mask);
			token[i++] = next;
			get();
		}
		
		token_info &= b_rel;							/* extract relationals			*/
		token[i] = 0;									/* terminate C string			*/
		
		if ((token_info == b_eq          )	        	/* accept c-style '=='							*/
		&&  (next_info  == b_relop + b_eq))
			get();

		if (token_info == (b_eq | b_lt | b_gt))			/* look for <=>					*/
			{printf( "xpltoc: incorrect relation\n"); error_exit();}
		
		if (not_scanned)
		{
			if ((token_info & b_rel) == 0)				/* handle unary not				*/
				token_info = o_not;
			
			else if (token_info == b_eq)				/* accept != for <>				*/
				token_info =  o_eq + ((b_lt | b_gt) - b_eq);
			
			else
				token_info = o_eq + (b_rel - token_info) - b_eq ;
		}
		
		else
			token_info = o_eq + (token_info - b_eq);
		
		token_sym  = 0;
		
		return;
	}

	if (next_info & b_digit)							/* scan decimal					*/
	{
		token_count = ccount;							/* save starting position 		*/
		
		if (next == '0')								/* look for leading 0 (0xFFFF)	*/
		{
			get();										/* toss leading 0				*/
			
			if (next == 'X' || next == 'x')				/* handle hex format			*/
			{
				get();
				
				token_type = t_octconst;
				token_info = scan_hex();
				token_sym  = 0;
				
				sprintf(token, "0x%04X", token_info);
				
				return;
			}
		}
		
		token_type = t_const;
		token_info = scan_decimal();
		token_sym  = 0;
		
		sprintf(token, "%d", token_info);
		
		return;
	}
	
	if (next_info & b_symb)								/* scan symbol					*/
	{
		symbol_struct *the_symb;
		
		token_count = ccount;							/* save starting position 		*/
		
		scan_symbol(&token[0], TOKEN_LENGTH-1);

		if ((the_symb = find_symbol(token))	!= 0)		/* see if exists...				*/
		{
			token_type = the_symb->type;
			token_info = the_symb->info;
			token_sym  = the_symb;
			
			/* Remap - if this symbol is a simple literal declaration, then	*/
			/* just look up what the symbol refers to.  E.G. dcl handle		*/
			/* lit 'fixed'.  If the symbol is a literal string that			*/
			/* should be translated at translation-time, then change to the	*/
			/* literal string now.  Note that the original XPL syntax is	*/
			/* lost in that case...											*/
			
			if ((token_type == t_remapsymb)				/* map name if needed			*/
			||  (token_type == t_remapproc)				/* map name if needed			*/
			||  (token_type == t_lit && remap_all_lits))
			{
				lit_push(next);							/* push current character info	*/
				lit_push(next_info);
				lit_push(next_count);
				lit_push((long long) current_lit);      /* push current literal string	*/
				
				current_lit = (char *) (token_sym->info);
				
				get();									/* get first char of lit string	*/
				scan();									/* token-ize it					*/
				return;									/* and that is our token		*/
			}
		}
		else
		{
			the_symb   = insert_symbol(token, t_und, 0);
			token_type = t_und;
			token_info = 0;
			token_sym  = the_symb;
		}
		
		return;
	}
	
	if (( recognize_c_syntax && (next == '\"' ))		/* scan string constant			*/
	||  (!recognize_c_syntax && (next == '\'')))
	{
		token_count = ccount;							/* save starting position 		*/
		
		if (return_apos)								/* return APO during literal	*/
		{												/* string scan					*/
			get();										/* accept apo					*/
			
			token_type = t_apo;
			token_info = 0;
			token_sym  = 0;
			
			return;
		}
		
		scan_string(&token[0], TOKEN_LENGTH-1);

		token_type = t_sconst;
		token_info = 0;
		token_sym  = 0;
		
		return;
	}	
	
	if (next == '\"')									/* scan octal constant			*/
	{
		token_count = ccount;							/* save starting position 		*/
		
		token_type = t_octconst;
		token_info = scan_octal();
		token_sym  = 0;
		
		sprintf(token, "\"%o\"", token_info);
		
		return;
	}	
	
	printf("xpltoc: unrecognized scan character or token %c (%d %d)\n", next, next, next_info); error_exit();
}


/*--------------------------------------------------------------------------------------*/
/* scan_space_and_comment																*/
/*--------------------------------------------------------------------------------------*/

/* Scan white space, eols and comments into string.  Used when scanning expressions		*/
/* to keep white space and comments associated with the preceeding token.				*/

static	char *	scan_space_and_comment()
{
	char	stuff      [1000] = {""};
	char	other_stuff[1000] = {""};
	
	int		i = 0;
	char	*new_stuff = 0;
	
	while (token_type == t_white || token_type == t_comment || token_type == t_eol)
	{
		int j;
		int	type = token_type;
	
		j = 0;
		while (token[j])				/* stash what we have temporarily		*/
		{								/* in 'other_stuff'						*/
			if (j >= 1000)
				{printf( "xpltoc: expression too long\n"); error_exit();}
			
			other_stuff[j] = token[j];
			j++;
			other_stuff[j] = 0;
		}
		
		scan();							/* get next								*/
		
		if (type == t_white)			/* handle white space					*/
		{
			if (token_type == t_eol)	/* white followed by eol				*/
			{
				other_stuff[0] = 1;		/* change the white space to a tab to	*/
				other_stuff[1] = 0;		/* indent								*/
			}
			
			else if (token_type == t_comment)
			{
				other_stuff[0] = 2;		/* change the white space to a tab to	*/
				other_stuff[1] = 0;		/* comment								*/
			}
			
			else						/* else keep white space as is since	*/
				;						/* it most likely separates operands	*/
		}
		
		else if (type == t_eol)			/* handle eol							*/
		{
			if (token_type == t_white)	/* eol followed by white				*/
			{
				token[0] = 1;			/* change white to a tab to indent		*/
				token[1] = 0;
			}
			
			else						/* else insert a tab to indent after	*/
			{							/* the eol and before the next token..	*/
				other_stuff[j++] = 1;
				other_stuff[j  ] = 0;
			}
		}
		
		else							/* was a comment						*/
		{
			if (token_type == t_eol)	/* comment followed by eol				*/
			{
				expand_short_comment(other_stuff, comment_len);
				convert_comment(other_stuff);
			}
							
			else						/* else keep white space as is since	*/
				;						/* it most likely separates operands	*/
		}
		
		j = 0;
		while (other_stuff[j])
		{
			if (i >= 1000)
				{printf( "xpltoc: expression too long\n"); error_exit();}
			
			stuff[i++] = other_stuff[j++];
			stuff[i  ] = 0;
		}
	}
	
	if (i == 0)							/* no stuff, return null			*/
		return (NULL);
		
	if ((new_stuff = (char *)malloc(i)) == 0)	/* else get storage & return		*/
		{printf( "xpltoc: out of memory during expression\n"); error_exit();}
		
	strcpy(new_stuff, stuff);
	
	return (new_stuff);
}


static	char *	fabricate_white_space(int num)
{
	char	*new_stuff = 0;

	if ((new_stuff = (char *)malloc(num + 1)) == 0)
		{printf( "xpltoc: out of memory for white space\n"); error_exit();}
		
	new_stuff[num] = 0;						/* terminate c string						*/
	
	while (num)								/* store spaces								*/
		new_stuff[--num] = ' ';
	
	return (new_stuff);
}
		
	
/*--------------------------------------------------------------------------------------*/
/* expression scanner																	*/
/*--------------------------------------------------------------------------------------*/
/* Tables used during expression processing:
.  
.   Look-up tables are used during expression processing to determine
.   the priority of different operations, and to distinguish dyadic
.   and monadic operators.
.  
.   The following table is used to find the priority of a given operator.
.   It is indexed by the operator type that is returned in 'INFO' by
.   the scanner.
*/
 
#define	b_monad 256 							/* indicates monadic allowed			*/
#define	b_dyad  512								/* indicated dyadic allowed				*/

int	op_table[] = {								/* and this is table					*/
b_monad + 1, b_monad + 1, b_monad + 1, b_monad + 1,
b_monad + 1, b_monad + 1, b_monad + 1, b_monad + 1, /* math ops */
0, 0, 0, 0, 0, 0, 0, 0,							/* spare								*/
b_monad + 1, b_monad + 1, 0, b_monad + 1, 		/* int, read, spare, addr				*/
b_monad + 1, b_monad + 1, b_monad + 1, 0, 		/* shr1, shl1, rot8						*/
b_monad + 1, 0, 0, 0, 							/* not									*/
b_monad + b_dyad + 3, b_monad + b_dyad + 3, 	/* minus, plus							*/
4, 4, 4, 4, 4, 4,								/* shr shl rot pat min max - elsewhere	*/
b_dyad + 2, b_dyad + 2, b_dyad + 2, b_dyad + 2, b_dyad + 2, /* div, times, fmu, fdi, mod*/
b_dyad + 5, b_dyad + 5, b_dyad + 5,				/* and or xor							*/
0, 0, 0, 0, 0, 									/* not used								*/
b_dyad + 4, b_dyad + 4, b_dyad + 4, b_dyad + 4, b_dyad + 4, b_dyad + 4, /* rels */
b_dyad + 4, b_dyad + 4, b_dyad + 4, b_dyad + 4, /* integer relational ops */
0, b_monad + 1};								/* core function						*/
  
typedef	struct
{	
	int         x_node;			/* type of the node         */
	long long   x_arg1;			/* first argument pointer   */
	long long   x_arg2;			/* second argument pointer  */
	long long   x_info;			/* extra information        */
	char *      x_pre_string;	/* comment & space			*/
	char *      x_post_string;	/* comment & space			*/
}	node;

static	int	num_blocks_in_use;

static	node* x_get()
{
	node *it = (node *) malloc(sizeof(node));
	
	if (!it)
		{printf("xpltoc: out of node storage\n"); error_exit();}
	
	it->x_pre_string  = NULL;	/* no string yet...			*/
	it->x_post_string = NULL;	/* no string yet...			*/
	
	num_blocks_in_use++;
	
	return (it);
}

static 	void x_rel(node *tree)
{
	if (tree->x_pre_string)
		{free(tree->x_pre_string ); tree->x_pre_string  = NULL;}
		
	if (tree->x_post_string)
		{free(tree->x_post_string); tree->x_post_string = NULL;}
		
	num_blocks_in_use--;
	
	free(tree);
}

static	void	er_ifm(int x)
{
	printf("xpltoc: incorrect format (%d)\n", x); error_exit();
}

static	node *	subexpr(int prior);

/* main driver: sets up global variables; scans expression */

static	node *	expr()
{
	return (subexpr(prior_max));
}


/* The procedure 'SCANARG' is used to scan off an argument that is
_  enclosed in parenthesis (Such as when processing the 'READ'
_  token_type or when scanning a subscript)
*/

static	void	toss_non_essentials();

static	node * scanarg()
{
	node *subtree1   = NULL;
	node *subtree2   = NULL;
	char *pre_string = NULL;
	
	if ((token_type != t_lpar) 						/* make sure paren exists			*/
	&&  (token_type != t_lbr ))						/* or allow brackets too...			*/
		{printf( "xpltoc: missing left parenthesis\n"); error_exit();}
	
	scan();											/* skip paren						*/
	
	pre_string = scan_space_and_comment();			/* get assoc space/com				*/
	
	subtree1 = subexpr(prior_max);					/* scan sub expression				*/
	
	if ((token_type != t_rpar)						/* scan closing paren				*/
	&&  (token_type != t_rbr ))
		{printf( "xpltoc: missing right parenthesis\n"); error_exit();}
	
	scan();											/* skip final paren					*/
	
	subtree2 = x_get();								/* construct our node				*/
	
	subtree2->x_node = x_paren;						/* node type is paren expression	*/
	subtree2->x_arg1 = 0;
	subtree2->x_arg2 = (long long) subtree1;
	subtree2->x_info = 0;
	
	subtree2->x_pre_string  = pre_string;
	subtree2->x_post_string = scan_space_and_comment();
	
	return (subtree2);
}

static	node *	subexpr(int prior)
{
	/* Expression scanner - priority zero:
	_ 
	_   If the priority is zero we handle variables, constants, subscripted
	_   arrays, procedure calls, and expressions enclosed in parens_
	*/
 	
	node *tree = 0;						/* what we eventually return				*/
	
	if (prior == 0)						/* lowest priority - vars, consts, etc.		*/
	{
		symbol_struct *expanded_sym = token_sym;
		
		/* if literal, look up (e.g. expand) the literal now and see if it perhaps 	*/
		/* references an array or procedure.  If so, handle accordingly.  If not,	*/
		/* then treat literal as simple non-subscripted variable...					*/
		
		if (token_type == t_lit)
		{
			symbol_struct *mapped_sym = find_symbol((char *) (token_sym->info));
			
			if (mapped_sym)
			{
				token_type = mapped_sym->type;
				token_info = mapped_sym->info;
				
				expanded_sym = mapped_sym;	/* in case proc call (see below)		*/
			}
		}
		
		
		/* Handle variables: */
		
		if ((token_type <= t_lit)
		||  (token_type == t_fxdata) 
		||  (token_type == t_fldata)
		||  (token_type == t_pointer)
		||  (token_type == t_pointer + t_arr)
		||  (token_type == t_cstring))
		{
			if (token_type == t_cstring)
			{
				printf("xpltoc: warning: incompatible access to cstring '%s'\n", token_sym->symbol_name);
				error_log();
				put_symbol("    (#### xpltoc: warning: incompatible access to cstring '");
				put_symbol(token_sym->symbol_name);
				put_symbol("' ####)    ");
			}
		
  			tree = x_get();
  			
			tree->x_node = x_var;							/* node type is var		*/
  			tree->x_arg1 = token_type;						/* save var type		*/
			tree->x_arg2 = NULL;							/* no subscr yet		*/
			tree->x_info = (long long) token_sym;           /* save ptr to sym		*/
			
			/* Detect access of outer proc local variables: */
			
			if (token_sym->scope && token_sym->scope < scope_level)
			{
				printf("xpltoc: warning: incompatible access to %s\n", token_sym->symbol_name);
				put_symbol("    (#### xpltoc: warning: incompatible access to '");
				put_symbol(token_sym->symbol_name);
				put_symbol("' ####)    ");
				error_log();
			}
			
			scan();											/* accept var name		*/
			
			tree->x_post_string = scan_space_and_comment();	/* get assoc space/com	*/

			/* scan off subscript if is subscripted variable */
			
			if (tree->x_arg1 & t_arr)
			{
				if ((token_type != t_lpar)                  /* make sure paren exists			*/
				&&  (token_type != t_lbr ))                 /* or allow brackets too...			*/
					{printf( "xpltoc: missing or incorrect format in subscript\n"); error_exit();}
				
				tree->x_arg2 = (long long) scanarg();
			}

			return (tree);
		} /* of variable/lit/data */
   

		/* Handle expression enclosed in parens: */
		
		if (token_type == t_lpar)	/* expression enclosed in parens	*/
		{
			tree = scanarg();		/* scan paren expression			*/
			
			return (tree);
		}
  
  		
		/* Handle constants: */
		
		if ((token_type == t_const) || (token_type == t_octconst))
		{
			tree = x_get();
			tree->x_node = x_const;
			tree->x_arg1 = token_type;
			tree->x_info = token_info;

			scan();
			
			tree->x_post_string = scan_space_and_comment();	/* get assoc space/com	*/

			return (tree);
		}
  
		
		/* $$Dyadic operators - special form: SHR, SHL, ROT
		_
		_  The functions SHR, SHL, and ROT are treated as dyadic
		_  operators by the compiler_  Instead of the normal form
		_  (<term> <operator> <term>), their format is:
		_  <operator> (<term>, <term>)_
		_
		_  The following section scans off expressions with these special
		_  dyadic operators_
		*/
		
		/* Handle special dyadic operators: */

		if (token_type == t_sdy)					/* special dyadics - shl, shr	*/
		{
			tree = x_get();									/* get node block		*/
			
			tree->x_node = x_dyad;							/* node is operator		*/
			tree->x_info = token_info;						/* o_shl, o_shr			*/
			
			scan();											/* skip to (			*/
			toss_non_essentials();							/* ignoring comnt here	*/
			
			if (token_type != t_lpar) er_ifm(2);
			
			scan();
			tree->x_pre_string = scan_space_and_comment();	/* get stuff			*/
			
			tree->x_arg1 = (long long) subexpr(prior_max);
			
			if (token_type != t_comma) er_ifm(3);
			scan();
			toss_non_essentials();							/* ignoring comnt here	*/
			
			tree->x_arg2 = (long long) subexpr(prior_max);
			
			if (token_type != t_rpar) er_ifm(4);
			
			scan();
			tree->x_post_string = scan_space_and_comment();	/* get stuff			*/
			
			return (tree);
		}

		
		/* Handle procedure & RTP calls: */
		
/* $$Procedure calls:
_ 
_   For procedure calls, the node block looks like:
_      x_node =  x_PROC
_      x_arg1 =  start of linked list of argument blocks (see below)
_      x_info =  pointer to symbol_struct for the procedure
_      x_arg2 =  type of data returned by proc (t_var if unknown/void)
_  
_   Each 'argument block' looks like:
_      in both cases:
_         x_node =  type of argument required by procedure, or t_locat for 'location'
_         x_arg1 =  link pointer to next argument block
_     
_      if argument was declared to be some sort of array:
_         x_info =  pointer to symbol_struct for the array or literal we are passing
_                   or expression tree if is t_locat
_     
_      if argument was declared to be some sort of fixed or floating variable:
_         x_info =  pointer to expression tree to pass to procedure
*/
  
		if ((token_type == t_proc) || (token_type == t_rtp))
		{
			symbol_struct	*proc_sym  = token_sym;
			proc_struct		*proc_info = (proc_struct *) expanded_sym->other;
			int				num_args   = token_info;
			int				arg_num;
			
			tree = x_get();			
			
			tree->x_node = x_proc;
			tree->x_arg1 = 0;						/* initialize list of arguments				*/
			tree->x_arg2 = proc_info->return_type;
			tree->x_info = (long long) proc_sym;    /* point to symbol record					*/

 			scan();									/* scan past procedure identifier			*/

			if (num_args)							/* if args required							*/
			{
				node *link_node = tree;
				int  *arg_list  = &(proc_info->arg_types[0]);
				
				tree->x_pre_string = scan_space_and_comment();

				if (token_type != t_lpar || !arg_list)
					{printf( "xpltoc: missing arguments for '%s'\n", proc_sym->symbol_name); error_exit();}
				
				for (arg_num = 0; arg_num < num_args; arg_num++)
				{
					node *arg_tree = x_get();
					
					link_node->x_arg1 = (long long) arg_tree;   /* link arg list together			*/
					link_node         = arg_tree;               /* for next time					*/
					
					scan();                                     /* accept ( or ,					*/
					arg_tree->x_node   = arg_list[arg_num];     /* store arg type					*/
					arg_tree->x_arg1   = NULL;
					arg_tree->x_pre_string = scan_space_and_comment();
					
					if (token_type == t_rpar)                   /* provide helpful diagnostic		*/
						{printf( "xpltoc: missing arguments for '%s'\n", proc_sym->symbol_name); error_exit();}
					
					if (arg_list[arg_num] & t_arr)              /* if arg expects array...			*/
					{
						if (token_type == t_locat)              /* handle location...				*/
						{
							arg_tree->x_node = t_locat;         /* inform compute of this fact...	*/
							scan();
							arg_tree->x_post_string = scan_space_and_comment();
							arg_tree->x_info = (long long) scanarg();	/* get arg in parens        */
						}
						
						else if (token_type == t_sconst)        /* string constant					*/
						{
							if (arg_list[arg_num] == t_cstring)
								arg_tree->x_node = t_cconst;
							else
								arg_tree->x_node = t_sconst;
							arg_tree->x_info = (long long) store_literal(token);
							scan();							/* accept sconst and move on		*/
							arg_tree->x_post_string = scan_space_and_comment();
						}
						
						else
						{
							if ((token_type != t_var + t_arr)
							&&  (token_type != t_pointer + t_arr)
							&&  (token_type != t_cstring)
							&&  (token_type != t_lit))
								{printf( "need array-type argument for call to '%s' (argument number %d)\n", proc_sym->symbol_name, arg_num); error_exit();}
							
							arg_tree->x_node = token_type;	/* inform compute of exact type		*/
							arg_tree->x_info = (long long) token_sym;
							scan();
							arg_tree->x_post_string = scan_space_and_comment();
						}
					}
					
					else									/* else arg must be expression	*/
						arg_tree->x_info = (long long)subexpr(prior_max);
				
					if (arg_num != num_args-1 && token_type != t_comma)
						{printf( "xpltoc: missing comma or too few arguments for '%s'\n", proc_sym->symbol_name); error_exit();}
				}

				if (token_type != t_rpar)
					{printf( "xpltoc: missing right parenthesis or too many arguments for '%s'\n", proc_sym->symbol_name); error_exit();}
				
				scan();			/* accept trailing paren */
			}
			
			else if (token_type == t_lpar)		/* allow () after procedure calls with no args... */
			{
				scan();
				
				if (token_type == t_rpar)
					scan();
				else
					er_ifm(11);
			}
			
			tree->x_post_string = scan_space_and_comment();

			return (tree);
		}


		/* Else must be undefined label or incorrect format: */
		
		if (token_type == t_und || token_type == t_unda) 
			{printf( "xpltoc: undefined label or symbol '%s'\n", token_sym->symbol_name); error_exit();}
		else if (token_sym)
			{printf( "xpltoc: cannot handle '%s' at this point\n", token_sym->symbol_name); error_exit();}
		else if (token[0])
			{printf( "xpltoc: cannot handle '%s' at this point\n", token); error_exit();}
		else
			er_ifm(6);
		
		tree = x_get();								/* fake const of 0 for undefined symbol */
		tree->x_node = x_const;
		tree->x_arg1 = t_var;
		tree->x_info = 0;
		
		return (tree);
	}												/* end of priority 0			*/


	/* $$Expression scanner - nonzero priority:
	_ 
	_   Handle expression of the form:
	_
	_     (<monadic operator>) <term> [<operator> <term>]*
	_ 
	_   where all of the operators are of the current priority, and the
	_   terms are expressions with all operators of higher priority than
	_   the current priority (lower priority number)_  An expression (of
	_   a given priority) consists of an optional monadic operator
	_   of that priority followed by one or more expressions of higher
	_   priority seperated by dyadic operators of the current priority.
	*/
 
	else				/* nonzero priority case: check for monadic, then dyadic oprs		*/
	{
		tree = 0;		/* nonzero indicates monadic operator found, points to node block	*/
	
		if (token_type == t_opr)						/* aha!: leading operator found		*/
		{
			if ((op_table [token_info] & 255) == prior) /* of the current priority			*/
			{
				if ((op_table [token_info] & b_monad) == 0) 
					{er_ifm(7); token_info = o_plus; }
				
				if (token_info != o_plus)				/*** ignore unary plus            ***/
				{
					tree         = x_get();				/* get a new node					*/
					tree->x_node = x_monad;
					tree->x_info = token_info;			/* type of operator					*/
				}
				
				scan();									/* scan off monadic operator */
				
				if (tree)
					tree->x_post_string = scan_space_and_comment();
				else
					toss_non_essentials();
			}
		}
		

		/* if we scanned a monadic operator, scan its operand				*/
 
		if (tree != 0)
		{
			if (tree->x_info == o_read)
				remap_all_lits = TRUE;
				
			tree->x_arg1 = (long long) subexpr(prior - 1);
			
			if (tree->x_info == o_read)
				remap_all_lits = FALSE;
		}		

		
		/* else no monadic operator found.  Scan off first operand for		*/
		/* a dyadic operator												*/
		
		else
		{
			tree = subexpr(prior - 1);	/* recurs...								*/

			if (!tree)
				{printf( "xpltoc: system error with expression scanner\n"); error_exit();}
		}
		
		
		/* get following terms */
		
		while (token_type == t_opr)			/* stop whenever we run out of operators	*/
		{
			char	*post_string = NULL;
			int		opr          = token_info;
			node	*subtree1    = tree;
			node	*subtree2    = NULL;
			
			if ((op_table [opr] & 255) != prior)		/* or get wrong priority		*/
				return (tree);
				
			if ((op_table [opr] & b_dyad) == 0)
				{er_ifm(8); opr = o_plus;}
				
			scan();							/* accept this operator						*/
			post_string = scan_space_and_comment();
			
			subtree2 = subexpr(prior - 1);
		
			tree = x_get();					/* get node block							*/
			tree->x_node = x_dyad;			/* dyadic operator							*/
			tree->x_arg1 = (long long) subtree1;
			tree->x_arg2 = (long long) subtree2;
			tree->x_info = opr;             /* add in operator type */

			tree->x_post_string = post_string;
		}
		
		return (tree);
	}
}


/*--------------------------------------------------------------------------------------*/
/* Emit complete expression to output file												*/
/*--------------------------------------------------------------------------------------*/

static	node *	find_last_operand            (node *tree);
static	node *	find_significant_operand     (node *tree);
static	boolean	check_for_constant_expression(node *tree);
static	boolean	is_pointer_operand           (node *tree);
static	boolean	check_for_read_write         (node *tree);
static	boolean	check_for_proc_call			 (node *tree);

static	void	compute(node *tree, boolean release_blocks)
{
	char	out_string[512] = {""};
	
	/* Handle variables: */
	
	if (tree->x_node == x_var)
	{
		if (tree->x_pre_string)
			put_symbol(tree->x_pre_string);

		put_symbol(((symbol_struct *)(tree->x_info))->symbol_name);
		
		if (tree->x_post_string)
			put_symbol(tree->x_post_string);

		if ((node *) (tree->x_arg2))						/* if subscript			*/
		{
			node *subtree    = (node *) (tree   ->x_arg2);	/* refer subscript		*/
			node *subsubtree = (node *) (subtree->x_arg2);	/* actual expression	*/
			
			put_symbol("[");								/* start subscript		*/
			
			if (subtree->x_pre_string)						/* prestring goes 		*/
				put_symbol(subtree->x_pre_string);			/* after the [			*/
			
			compute(subsubtree, release_blocks);			/* emit its expression	*/
			
			put_symbol("]");								/* close subscript		*/

			if (subtree->x_post_string)						/* post string goes		*/
				put_symbol(subtree->x_post_string);			/* after the ]			*/
			
			if (release_blocks) x_rel(subtree);				/* done with subtree	*/
		}
	}
	
	
	/* Handle expression enclosed in parens: */
	
	else if (tree->x_node == x_paren)						/* paren expression		*/
	{
		put_symbol("(");									/* start paren expr		*/
		
		if (tree->x_pre_string)								/* prestring goes 		*/
			put_symbol(tree->x_pre_string);					/* after the (			*/
		
		compute((node *) (tree->x_arg2), release_blocks);	/* emit its expression	*/
		
		put_symbol(")");									/* close paren expr		*/

		if (tree->x_post_string)							/* post string goes		*/
			put_symbol(tree->x_post_string);				/* after the )			*/
	}


	/* handle constants: */
	
	else if (tree->x_node == x_const)						/* constant				*/
	{
		boolean enclosed = FALSE;
		
		if (tree->x_pre_string)								/* prestring goes 		*/
			put_symbol(tree->x_pre_string);					/* before constant		*/

		if (type_cast_consts)                               /* if approaching the	*/
		{													/* danger zone, enclose	*/
			put_symbol("((fixed) ");						/* in parens			*/
			enclosed = TRUE;
		}
		
		if (tree->x_arg1 == t_octconst)
			sprintf(out_string, "0x%04X", (int)tree->x_info);
		else
			sprintf(out_string, "%d", (int)tree->x_info);
		
		put_symbol(out_string);
		
		if (enclosed)
			put_symbol(")");
			
		if (tree->x_post_string)							/* post string goes		*/
			put_symbol(tree->x_post_string);				/* after the constant	*/
	}	
	
	
	/* Handle read operator specially: */
	
	else if ((tree->x_node == x_monad)						/* monadic operator		*/
	&&       (tree->x_info == o_read ))
	{
		node	*arg1      = (node *) (tree->x_arg1);		/* get device expr		*/
		node	*last      = find_last_operand(arg1);		/* most likely paren	*/
		char	*rd_string = last->x_post_string;			/* get trailing string	*/
		int		the_dev;
		char 	device_id[512] = {""};
		
		last->x_post_string = NULL;							/* snarf post string	*/
		
		if (tree->x_pre_string)
			put_symbol(tree->x_pre_string);

		while (arg1->x_node == x_paren)						/* extract device		*/
		{													/* from parens...		*/
			node *lower = (node *)arg1->x_arg2;				/* get its deal			*/
			
			if (release_blocks)
				x_rel(arg1);
			
			arg1 = lower;
		}
		
		if (arg1->x_node != x_const)
			{printf( "xpltoc: constant expression required after 'read'\n"); error_exit();}
		
		the_dev = arg1->x_info;								/* get which device		*/

		if (the_dev == 4)
			put_symbol("_read_4");
		
		else if (the_dev == 5)
			put_symbol("_read_5");
		
		else if (the_dev == 060)
			put_symbol("_read_60");
		
		else if (the_dev == 061)
			put_symbol("_read_61");
		
		else if (the_dev == 062)
			put_symbol("_read_62");
		
		else if (the_dev == 063)
			put_symbol("_read_63");
		
		else
		{
			put_symbol("XPL_read(0");
			sprintf(device_id, "%o", the_dev);
			put_symbol(device_id);
			put_symbol(")");
		}
				
		if (rd_string)
			put_symbol(rd_string);
		
		if (release_blocks)
		{
			x_rel(arg1);
			
			if (rd_string)
				free(rd_string);
		}
		
		else
			last->x_post_string = rd_string;
	}

	/* Handle core operator specially: */
	
	else if ((tree->x_node == x_monad)						/* monadic operator		*/
	&&       (tree->x_info == o_core ))
	{
		node	*arg1 = (node *) (tree->x_arg1);			/* get core address		*/
		
		if (tree->x_pre_string)
			put_symbol(tree->x_pre_string);

		if (is_pointer_operand(arg1))						/* core(addr ...)		*/
			put_symbol("host_core");
		else
			put_symbol("able_core");
		
		if (tree->x_post_string)
			put_symbol(tree->x_post_string);
		
		compute(arg1, release_blocks);						/* emit operand			*/
	}


	/* Handle monadic operators: */
	
	else if (tree->x_node == x_monad)						/* monadic operator		*/
	{
		char	*sh_string = NULL;
		node	*arg1      = (node *) (tree->x_arg1);
		boolean	enclosed   = FALSE;
		
		if (tree->x_pre_string)
			put_symbol(tree->x_pre_string);

		switch (tree->x_info)								/* branch on type		*/
		{
			case o_not:
				put_symbol("~");
				break;
				
			case o_minus:
				put_symbol("-");
				break;
				
			case o_plus:
				put_symbol("+");
				break;
				
			case o_adr:
				put_symbol("&");
				break;

			case o_read:
				put_symbol("XPL_read");
				break;
			
			case o_math + 0:
				put_symbol("XPL_abs");
				break;
				
			case o_int:
				put_symbol("XPL_int");
				break;
			
			case o_incr:
			case o_decr:
			default:
				{printf( "xpltoc: unimplemented operator\n"); error_exit();}
				break;
		}
		
		if (tree->x_post_string)
			put_symbol(tree->x_post_string);
		
		if ((arg1->x_node == x_dyad)						/* enclose shl/shr		*/
		&&  ((arg1->x_info == o_shr) || (arg1->x_info == o_shl)))
		{
			put_symbol("(");
			enclosed  = TRUE;
			sh_string = arg1->x_post_string;
			arg1->x_post_string = NULL;
		}
		
		compute(arg1, release_blocks);						/* emit operand			*/
		
		if (enclosed)
		{
			put_symbol(")");
			
			if (sh_string)
			{
				put_symbol(sh_string);						/* emit string after parns		*/
				
				if (release_blocks)							/* if freeing blocks,			*/
					free(sh_string);						/* then free up this string		*/
				else										/* else if not releasing blocks	*/
					arg1->x_post_string = sh_string;		/* restore string for next use	*/
			}
		}
	}
	

	/* Special case for a three-argument oprand.  Xpl handled expressions of the form		*/
	/* A * B / C by computing the intermediate product (e.g. a*b) in a signed 32-bit		*/
	/* form.  Mimic that same behavior here													*/
	
	else if ((tree->x_node == x_dyad)						/* dyadic operator				*/
	&&       (tree->x_info == o_div )						/* of divide					*/
	&&       (find_significant_operand((node *)(tree->x_arg1))->x_node == x_dyad )	/* whose arg1 is a 		*/
	&&       (find_significant_operand((node *)(tree->x_arg1))->x_info == o_times))	/* multiply...			*/
	{
		node	*arg1      = find_significant_operand((node *)(tree->x_arg1));
		node	*arg2      = (node *)(tree->x_arg2);
		node	*subarg1   = (node *)(arg1->x_arg1);
		node	*subarg2   = (node *)(arg1->x_arg2);
		node	*last1     = find_last_operand(subarg1);
		node	*last2     = find_last_operand(subarg2);
		node	*last3     = find_last_operand(arg2);
		char	*string1   = NULL;
		char	*string2   = NULL;
		char	*string3   = NULL;
		
		if (tree->x_pre_string)
			put_symbol(tree->x_pre_string);

		put_symbol("_scale_(");

		string1 = last1->x_post_string;								/* snarf space after arg1	*/
		last1->x_post_string = NULL;
		
		if (((node *)(tree->x_arg1))->x_node == x_paren)			/* get comment/space after	*/
		{															/* paren if syntax is		*/
			string2 = ((node *)(tree->x_arg1))->x_post_string;		/* (a*b) / c				*/
			((node *)(tree->x_arg1))->x_post_string = NULL;
		}
		else
		{
			string2 = last2->x_post_string;							/* snarf space after arg2	*/
			last2->x_post_string = NULL;
		}
		
		string3 = last3->x_post_string;								/* snarf space after arg3	*/
		last3->x_post_string = NULL;
		
		compute(subarg1, release_blocks);							/* emit first operand		*/
		put_symbol(",");
		if (string1) put_symbol(string1);
		
		compute(subarg2, release_blocks);							/* emit second operand		*/
		put_symbol(",");
		if (string2) put_symbol(string2);
		
		compute(arg2, release_blocks);								/* emit third operand		*/
		put_symbol(")");
		if (string3) put_symbol(string3);
		
		if (string1)
		{
			if (release_blocks)
				free(string1);
			else
				last1->x_post_string = string1;
		}

		if (string2)
		{
			if (release_blocks)
				free(string2);
			else if (((node *)(tree->x_arg1))->x_node == x_paren)
				((node *)(tree->x_arg1))->x_post_string = string2;
			else
				last2->x_post_string = string2;
		}
		
		if (string3)
		{
			if (release_blocks)
				free(string3);
			else
				last3->x_post_string = string3;
		}
		
		
		/* We must release any x_paren node blocks in arg1 since we never computed	*/
		/* them.  We must also release the arg1 block itself (containing the *)		*/
		
		if (release_blocks)
		{
			node	*subtree = (node *)(tree->x_arg1);		/* point to * block		*/

			while (subtree->x_node == x_paren)				/* but go deeper if it	*/
			{												/* is a paren block		*/
				node	*subsubtree = (node *)(subtree->x_arg2);
				x_rel(subtree);
				subtree = subsubtree;
			}
			
			x_rel(subtree);									/* toss * block too		*/
		}
		
		hmuldiv_valid = FALSE;								/* after scale			*/
	}

	
	/* Special case - fmul, fdiv - convert to macro calls							*/
	/* also unsigned compares if converting compatibly								*/

	else if ((tree->x_node == x_dyad)						/* dyadic operator		*/
	&&       ((tree->x_info == o_fmu)
	||        (tree->x_info == o_fdi)
	||        (convert_compatibly && tree->x_info == o_ige)
	||        (convert_compatibly && tree->x_info == o_igt)
	||        (convert_compatibly && tree->x_info == o_ile)
	||        (convert_compatibly && tree->x_info == o_ilt)))
	{
		node	*arg1      = (node *)(tree->x_arg1);		/* get operands handy	*/
		node	*arg2      = (node *)(tree->x_arg2);
		node	*last1     = find_last_operand(arg1);
		node	*last2     = find_last_operand(arg2);
		char	*string1   = NULL;
		char	*string2   = NULL;
		int		opr        = tree->x_info;
	
		if (tree->x_pre_string)
			put_symbol(tree->x_pre_string);
			
		if (opr == o_fmu)
			put_symbol("fmul(");
		else if (opr == o_fdi)
			put_symbol("fdiv(");
		else if (opr == o_ige)
			put_symbol("_IGE_(");
		else if (opr == o_igt)
			put_symbol("_IGT_(");
		else if (opr == o_ile)
			put_symbol("_ILE_(");
		else if (opr == o_ilt)
			put_symbol("_ILT_(");
		else
			{printf( "xpltoc: unimplemented operator (1)\n"); error_exit();}
			
		string1 = last1->x_post_string;				/* snarf space after arg1		*/
		last1->x_post_string = NULL;
		
		string2 = last2->x_post_string;				/* snarf space after arg2		*/
		last2->x_post_string = NULL;
		
		compute(arg1, release_blocks);				/* emit first operand			*/
		
		put_symbol(",");
		if (string1) put_symbol(string1);
		
		compute(arg2, release_blocks);				/* emit second operand			*/
		put_symbol(")");
		if (string2) put_symbol(string2);
		
		if (string1)
		{
			if (release_blocks)
				free(string1);
			else
				last1->x_post_string = string1;
		}
		
		if (string2)
		{
			if (release_blocks)
				free(string2);
			else
				last2->x_post_string = string2;
		}
	}
	
	
	/* Handle dyadic operators: */
	
	else if (tree->x_node == x_dyad)						/* dyadic operator		*/
	{
		node	*arg1      = (node *)(tree->x_arg1);		/* get operands handy	*/
		node	*arg2      = (node *)(tree->x_arg2);
		int		opr        = tree->x_info;
		boolean encl_arg1  = FALSE;
		boolean encl_arg2  = FALSE;
		boolean	encl_res   = FALSE;
		boolean encl_eq    = FALSE;
		char    *string    = NULL;
		node    *last_node = NULL;
		char	*arg1_str  = NULL;
		node	*arg1_node = NULL;
		char	*arg2_str  = NULL;
		node	*arg2_node = NULL;
		
		
		/*  emit pre string: */
		
		if (tree->x_pre_string)
			put_symbol(tree->x_pre_string);

		
		/* if this is the multiplication of two constants, cast the result			*/
		/* to fixed, lest the C-compiler we eventually find ourselves in determines	*/
		/* the result to be a long.  E.G. fixed a = 60*1024;  if (a == 60*1024)...	*/
		/* will fail with many C compilers without type casting...					*/
		
		if ((opr == o_times)								/* this is multipy		*/
		&&  (check_for_constant_expression(arg1))			/* arg1 is const or lit	*/
		&&  (check_for_constant_expression(arg2)))			/* arg2 is const or lit	*/
		{
			last_node = find_last_operand(arg2);			/* find end of arg2		*/
			
			put_symbol("((fixed) (");						/* emit cast			*/
			encl_res = TRUE;								/* publish				*/
			
			string = last_node->x_post_string;				/* snarf string			*/
			last_node->x_post_string = NULL;				/* clear it				*/
		}
		
		
		/* for shr and shl operators, enclose arguments in parens if they included	*/
		/* operators of higher priority.  That is, in xpl "shl(x&y, 1)" wants		*/
		/* to turn into "(x&y) << 1"  instead of "x&y << 1"							*/
		/* Additionally, shr wants to be performed using unsigned operands.			*/
		/* that is "shr(x,y)" wants to become "(ufixed) x >> y"				*/
		
		if (opr == o_shr || opr == o_shl)					/* special handling		*/
		{
			/* cast first operand to unsigned for shr: */	
			if (opr == o_shr)								/* cast to unsigned for shr	*/
			{
				put_symbol("(ufixed)");
				
				/* enclose all operators if result is cast to unsigned: */
				if ((arg1->x_node == x_monad) || (arg1->x_node == x_dyad))
				{											/* enclose dyadic & operators	*/
					put_symbol("(");						/* in parens for casting		*/
					encl_arg1 = TRUE;
				}
				
				/* else just put a space after the type cast: */
				else
					put_symbol(" ");
			}
			
			/* else for shl, enclose first arg if it is operator of higher priority */
			else if (((arg1->x_node == x_monad) || (arg1->x_node == x_dyad))
			&&       ((op_table[arg1->x_info] & 255) > (op_table[o_plus] & 255)))
			{
				put_symbol("(");							/* enlose higher priority in parens	*/
				encl_arg1 = TRUE;
			}
		}
		
		
		/* else for unsigned relationals, cast arg1 to unsigned: */
		else if ((opr >= o_ilt) && (opr <= o_ige))
		{
			put_symbol("(ufixed)");
			
			/* enclose all operators if result is cast to unsigned: */
			if ((arg1->x_node == x_monad) || (arg1->x_node == x_dyad))
			{											/* enclose dyadic & operators	*/
				put_symbol("(");						/* in parens for casting		*/
				encl_arg1 = TRUE;
			}
			
			/* else just put a space after the type cast: */
			else
				put_symbol(" ");
		}
		
		
		/* else for other operators, enclose arg1 in parens if it is a shr/shl since	*/
		/* C's operator precedence is different.  E.G. "shr(a,b) + 1" wants to map		*/
		/* to "(a << b) + 1"															*/
		
		else if ((arg1->x_node == x_dyad)
		&&       ((arg1->x_info == o_shr) || (arg1->x_info == o_shl)))
		{
			put_symbol("(");								/* enlose higher priority in parens	*/
			encl_arg1 = TRUE;
		}
		
		
		/* else for == and != operators, cast the arguments to (ufixed) if they are		*/
		/* the result of a dyadic operator.  E.G.  (a + b + c) == d will fail			*/
		/* for the case a = 30000; b = 2000; c = 2000; d = 34000 otherwise				*/
		
		else if (((opr == o_eq) || (opr == o_ne))
		&&       (((find_significant_operand(arg1)->x_node == x_dyad)
		&&         ((find_significant_operand(arg1)->x_info == o_plus )
		||          (find_significant_operand(arg1)->x_info == o_minus)))
		||        ((find_significant_operand(arg2)->x_node == x_dyad)
		&&         ((find_significant_operand(arg2)->x_info == o_plus)
		||          (find_significant_operand(arg2)->x_info == o_minus)))))
		{
			arg1_node = find_last_operand(arg1);			/* find end of arg1		*/
			arg2_node = find_last_operand(arg2);			/* find end of arg2		*/
			
			arg1_str = arg1_node->x_post_string;			/* snarf string			*/
			arg1_node->x_post_string = NULL;				/* clear it				*/

			arg2_str = arg2_node->x_post_string;			/* snarf string			*/
			arg2_node->x_post_string = NULL;				/* clear it				*/

			put_symbol("(ufixed) (");
			encl_arg1 = TRUE;
			encl_eq   = TRUE;
		}
		
		
		/* emit first arg: */
		compute(arg1, release_blocks);						/* emit first operand				*/

		if (encl_arg1)										/* terminate enclosure				*/
		{
			put_symbol(")");
			
			if (arg1_str)									/* if had a string					*/
			{
				put_symbol(arg1_str);						/* now emit that					*/
				
				if (release_blocks)
					free(arg1_str);
				
				else if (arg1_node)
					arg1_node->x_post_string = arg1_str;
			}
		}
		
		
		switch (opr)										/* branch on type					*/
		{
			case o_minus:
				put_symbol("-");
				break;
				
			case o_plus:
				put_symbol("+");
				break;
				
			case o_shr:
				put_symbol(" >> ");
				break;
				
			case o_shl:
				put_symbol(" << ");
				break;
				
			case o_div:
				put_symbol("/");
				hmuldiv_valid = FALSE;
				break;
				
			case o_times:
				put_symbol("*");
				hmuldiv_valid = FALSE;
				break;
				
			case o_mod:
				put_symbol("%");
				hmuldiv_valid = FALSE;
				break;
				
			case o_and:
				put_symbol("&");
				break;
				
			case o_or:
				put_symbol("|");
				break;
				
			case o_xor:
				put_symbol("^");
				break;
				
			case o_eq:
				put_symbol("==");
				break;
				
			case o_lt:
				put_symbol("<");
				break;
				
			case o_le:
				put_symbol("<=");
				break;
				
			case o_gt:
				put_symbol(">");
				break;
				
			case o_ge:
				put_symbol(">=");
				break;
				
			case o_ne:
				put_symbol("!=");
				break;
				
			case o_ilt:
				put_symbol("<");
				break;
				
			case o_ile:
				put_symbol("<");
				break;
				
			case o_igt:
				put_symbol(">");
				break;
				
			case o_ige:
				put_symbol(">=");
				break;
				
			default:
				{printf( "xpltoc: unimplemented operator\n"); error_exit();}
				break;
		}

		if (opr != o_shr && opr != o_shl)				/* emit post string after operator	*/
		{												/* itself for non-shifts			*/
			if (tree->x_post_string)
				put_symbol(tree->x_post_string);
		}
		
		if (opr == o_shr || opr == o_shl)					/* special handling		*/
		{
			/* for shl & shr, enclose second arg if it is operator of higher priority */
			if (((arg2->x_node == x_monad) || (arg2->x_node == x_dyad))
			&&  ((op_table[arg2->x_info] & 255) > (op_table[o_plus] & 255)))
			{
				put_symbol("(");							/* enlose higher priority in parens	*/
				encl_arg2 = TRUE;
			}
		}
		
		/* else for unsigned relationals, cast arg2 to unsigned: */
		else if ((opr >= o_ilt) && (opr <= o_ige))
		{
			put_symbol("(ufixed)");
			
			/* enclose all operators if result is cast to unsigned: */
			if ((arg2->x_node == x_monad) || (arg2->x_node == x_dyad))
			{											/* enclose dyadic & operators	*/
				put_symbol("(");						/* in parens for casting		*/
				encl_arg2 = TRUE;
			}
			
			/* else just put a space after the type cast: */
			else
				put_symbol(" ");
		}

		/* else for other operators, enclose arg2 in parens if it is a shr/shl since	*/
		/* C's operator precedence is different.  E.G. "shr(a,b) + 1" wants to map		*/
		/* to "(a << b) + 1"															*/
		
		else if ((arg2->x_node == x_dyad)
		&&       ((arg2->x_info == o_shr) || (arg2->x_info == o_shl)))
		{
			put_symbol("(");								/* enlose higher priority in parens	*/
			encl_arg2 = TRUE;
		}

		
		else if (encl_eq)									/* else if ==/!= cast needed		*/
		{
			put_symbol("(ufixed) (");
			encl_arg2 = TRUE;
		}
		
				
		compute(arg2, release_blocks);						/* emit second operand				*/
		
		
		if (encl_arg2)										/* terminate enclosure				*/
		{
			put_symbol(")");
			
			if (arg2_str)									/* if had a string					*/
			{
				put_symbol(arg2_str);						/* now emit that					*/
				
				if (release_blocks)
					free(arg2_str);
				
				else if (arg2_node)
					arg2_node->x_post_string = arg2_str;
			}
		}
		
		
		if (opr == o_shr || opr == o_shl)					/* special handling		*/
		{
			if (tree->x_post_string)
				put_symbol(tree->x_post_string);
		}

		if (encl_res)										/* if we were type-casting result 	*/
		{													/* then clean up					*/
			put_symbol("))");								/* finish parens					*/
			
			if (string)										/* if had a string					*/
			{
				put_symbol(string);							/* now emit that					*/
				
				if (release_blocks)
					free(string);
				else if (last_node)
					last_node->x_post_string = string;
			}
		}
	}


	/* Handle RTP and Proc calls: */
	
	else if (tree->x_node == x_proc)						/* procedure call				*/
	{
		put_symbol(((symbol_struct *)(tree->x_info))->symbol_name);
	
		if (tree->x_pre_string)								/* pre-string before parens		*/
			put_symbol(tree->x_pre_string);

		put_symbol("(");
			
		if (tree->x_arg1)									/* if any args					*/
		{
			node *arg_tree = (node *) (tree->x_arg1);		/* point to first arg			*/
			node *next_tree;
			int	 args_which_are_calls = 0;
			
			while (arg_tree)								/* emit each arg				*/
			{
				if (arg_tree->x_pre_string)					/* before argument				*/
					put_symbol(arg_tree->x_pre_string);
					
				if (arg_tree->x_node == t_locat)			/* if "location"				*/
				{
					node *loc_expr = (node *)(arg_tree->x_info);
					
					if (!is_pointer_operand(loc_expr))
					{
						printf("xpltoc: warning: operand for 'location' is not of type pointer\n");
						put_symbol("    (#### xpltoc: operand for 'location' is not of type pointer ####)    ");
						error_log();
					}
					
					put_symbol("_location_");				/* convert to C macro			*/
					
					if (arg_tree->x_post_string)			/* before argument				*/
						put_symbol(arg_tree->x_post_string);
					
					if (check_for_proc_call(loc_expr))
					{
						if (args_which_are_calls++)
						{
							printf("xpltoc: warning: check nested procedure calls for order-of-execution problems\n");
							put_symbol("    (#### xpltoc: check nested procedure calls for order-of-execution problems ####)    ");
							error_log();
						}
					}
					
					compute(loc_expr, release_blocks);		/* emit its expression			*/
				}
				
				else if (arg_tree->x_node == t_sconst)		/* string constant				*/
				{
					char *it = (char *) (arg_tree->x_info);	/* point to string to emit		*/
					
					if (create_xpl_strings)					/* if creating an xpl constnat	*/
					{
						put_symbol("(pointer) ");			/* emit type cast				*/
						emit_xpl_string(it);
					}
					
					else									/* else emit constant in C form	*/
						emit_c_string(it);
						
					if (arg_tree->x_post_string)
						put_symbol(arg_tree->x_post_string);
				}
				
				else if (arg_tree->x_node == t_cconst)		/* c string constant			*/
				{
					char *it = (char *) (arg_tree->x_info);	/* point to string to emit		*/
					
					emit_c_string(it);
						
					if (arg_tree->x_post_string)
						put_symbol(arg_tree->x_post_string);
				}
				
				else if ((arg_tree->x_node == t_var + t_arr)			/* handle arrays	*/
				||       (arg_tree->x_node == t_pointer + t_arr)
				||       (arg_tree->x_node == t_cstring)
				||       (arg_tree->x_node == t_lit))
				{
					put_symbol(((symbol_struct *)(arg_tree->x_info))->symbol_name);
					
					if (arg_tree->x_post_string)
						put_symbol(arg_tree->x_post_string);
				}
				
				else										/* else handle expression		*/
				{
					if (check_for_read_write((node *)(arg_tree->x_info)))
					{
						printf("xpltoc: warning: read result passed as procedure argument; check order of evaluation\n");
						put_symbol("    (#### xpltoc: warning: read result passed as procedure argument; check order of evaluation ####)    ");
						error_log();
					}

					else if (check_for_proc_call((node *)(arg_tree->x_info)))
					{
						if (args_which_are_calls++)
						{
							printf("xpltoc: warning: check nested procedure calls for order-of-execution problems\n");
							put_symbol("    (#### xpltoc: check nested procedure calls for order-of-execution problems ####)    ");
							error_log();
						}
					}
					
					compute((node *)(arg_tree->x_info), release_blocks);
				}

				next_tree = (node *)(arg_tree->x_arg1);		/* get next arg					*/
				
				if (release_blocks) x_rel(arg_tree);		/* release this block			*/
				
				arg_tree = next_tree;						/* and go on to next argument	*/
				
				if (arg_tree)								/* if more, separate them		*/
					put_symbol(",");
			}
		}
	
		put_symbol(")");									/* terminate arg list			*/
		
		if (tree->x_post_string)							/* put post string after arg	*/
			put_symbol(tree->x_post_string);				/* list (or identifier if none)	*/
	}
	
	
	else
		{printf( "xpltoc: system error with x_node (compute) (%p) (%d)\n", tree, tree->x_node); error_exit();}

	
	/* Done with head node: */
	
	if (release_blocks)	x_rel(tree);
}


/*--------------------------------------------------------------------------------------*/
/* compute_enclosed_expression															*/
/*--------------------------------------------------------------------------------------*/

static	void	compute_enclosed_expression(node *tree)
{
	if (tree->x_node != x_paren)						/* provide parens if none		*/
	{
		node	*last   = find_last_operand(tree);		/* find last operand			*/
		char	*string = last->x_post_string;			/* snarf its string				*/
			
		last->x_post_string = NULL;
	
		put_symbol("(");
		compute(tree, TRUE);							/* emit expression				*/
		put_symbol(")");
		
		if (string)										/* and put trailing white		*/
		{												/* space after paren instead	*/
			put_symbol(string);							/* of before					*/
			free(string);
		}
	}
	
	else
		compute(tree, TRUE);							/* emit expression				*/
}


/*--------------------------------------------------------------------------------------*/
/* Emit boolean expression to output file												*/
/*--------------------------------------------------------------------------------------*/

/* Boolean expression handling:															*/
/* 	&, |, ~  ==> recurse to compute boolean operands.									*/
/*  All other operands/operators ==> construct expression tree to and the subtree		*/
/* with 1.   This will convert the XPL even/odd false/true encoding to the C			*/
/* zero/nonzer fase/true encoding														*/

static	void	xcompute(node *tree, boolean release_blocks)
{
	/* Handle expression enclosed in parens: */
	
	if (tree->x_node == x_paren)							/* paren expression		*/
	{
		put_symbol("(");									/* start paren expr		*/
		
		if (tree->x_pre_string)								/* prestring goes 		*/
			put_symbol(tree->x_pre_string);					/* after the (			*/
		
		xcompute((node *) (tree->x_arg2), release_blocks);	/* emit its expression	*/
		
		put_symbol(")");									/* close paren expr		*/

		if (tree->x_post_string)							/* post string goes		*/
			put_symbol(tree->x_post_string);				/* after the )			*/
			
		if (release_blocks)									/* free node if so		*/
			x_rel(tree);
	}


	/* Handle boolean not: */
	
	else if ((tree->x_node == x_monad)						/* monadic operator		*/
	&&       (tree->x_info == o_not  ))
	{
		char	*sh_string = NULL;
		node	*arg1      = (node *) (tree->x_arg1);
		boolean	enclosed   = FALSE;
		
		if (tree->x_pre_string)
			put_symbol(tree->x_pre_string);

		put_symbol("!");
		
		if (tree->x_post_string)
			put_symbol(tree->x_post_string);
		
		if (arg1->x_node != x_paren)
		{
			put_symbol("(");
			enclosed  = TRUE;
			sh_string = arg1->x_post_string;
			arg1->x_post_string = NULL;
		}
		
		xcompute(arg1, release_blocks);						/* emit operand			*/
		
		if (enclosed)
		{
			put_symbol(")");
			
			if (sh_string)
			{
				put_symbol(sh_string);						/* emit string after parns		*/
				
				if (release_blocks)							/* if freeing blocks,			*/
					free(sh_string);						/* then free up this string		*/
				else										/* else if not releasing blocks	*/
					arg1->x_post_string = sh_string;		/* restore string for next use	*/
			}
		}

		if (release_blocks)									/* free node if so		*/
			x_rel(tree);
	}


	/* Handle boolean && and ||: */
	
	else if ((tree->x_node == x_dyad)						/* look for & and |		*/
	&&       (tree->x_info == o_and || tree->x_info == o_or))
	{
		node	*arg1      = (node *)(tree->x_arg1);		/* get operands handy	*/
		node	*arg2      = (node *)(tree->x_arg2);
		
		/* if anding with constant of 1, then leave as is: */
		
		if ((tree->x_info == o_and)
		&&  ((arg1->x_node == x_const && arg1->x_info == 1)
		||   (arg2->x_node == x_const && arg2->x_info == 1)))
			compute(tree, release_blocks);
		
		else												/* else must look		*/
		{
			xcompute(arg1, release_blocks);					/* emit first operand	*/
	
			switch (tree->x_info)							/* branch on type		*/
			{
				case o_and:
					put_symbol("&&");
					break;
					
				case o_or:
					put_symbol("||");
					break;
					
				default:
					{printf( "xpltoc: unimplemented boolean operator\n"); error_exit();}
					break;
			}
	
			if (tree->x_post_string)
				put_symbol(tree->x_post_string);
			
			xcompute(arg2, release_blocks);					/* emit second operand	*/
			
			if (release_blocks)								/* free node if so		*/
				x_rel(tree);
		}
	}
	
	
	/* Handle boolean relational operators: */
	
	else if ((tree->x_node == x_dyad)						/* all relationals		*/
	&&       (tree->x_info >= o_eq  )
	&&       (tree->x_info <= o_ige ))
		compute(tree, release_blocks);


	/* Constants of 0 and 1 map directly: */
	
	else if ((tree->x_node == x_const)						/* constant				*/
	&&       (tree->x_info == 0 || tree->x_info == 1))
		compute(tree, release_blocks);
	
	
	/* Symbols which are literally 0 or 1 map directly: */
	
	else if ((tree->x_node == x_var)
	&&       (tree->x_arg1 == t_lit)
	&&       (strcmp((char *)(((symbol_struct *)(tree->x_info))->info), "0") == 0
	||        strcmp((char *)(((symbol_struct *)(tree->x_info))->info), "1") == 0))
		compute(tree, release_blocks);
		
	
	/* Else for all other types, and with 1 to extract XPL's significant			*/
	/* bit:																			*/
	
	else
	{
		node *andtree   = x_get();
		node *consttree = x_get();
		node *last_node = find_last_operand(tree);

		andtree->x_node = x_dyad;				/* make it a dyadic operator		*/
		andtree->x_info = o_and;				/* and								*/
		andtree->x_arg1 = (long long) tree;     /* start with what we have			*/
		andtree->x_arg2 = (long long) consttree;/* and with a 1						*/
		andtree->x_post_string = fabricate_white_space(1);
		
		consttree->x_node = x_const;			/* make it a one					*/
		consttree->x_arg1 = t_const;			/* a decimal constant				*/
		consttree->x_info = 1;					/* of 1								*/
	
		consttree->x_post_string = last_node->x_post_string;
		last_node->x_post_string = fabricate_white_space(1);
				
		compute(andtree, release_blocks);		/* and emit expression				*/
		
		if (!release_blocks)					/* if compute did not relases		*/
		{										/* blocks, then do so ourselves		*/
			free(last_node->x_post_string);
			last_node->x_post_string = consttree->x_post_string;
			consttree->x_post_string = NULL;
			x_rel(andtree);
			x_rel(consttree);
		}
	}
}


/*--------------------------------------------------------------------------------------*/
/* compute_enclosed_boolean 															*/
/*--------------------------------------------------------------------------------------*/

/* compute_enclosed_boolean is used to emit a boolean expression in parenthesis.  		*/
/* it is used to process expressions in IF and DO WHILE statements where boolean		*/
/* operators (such as && and ||) are desired in C instead of the xpl & and | operators.	*/

static	void	compute_enclosed_boolean(node *tree)
{
	if (tree->x_node != x_paren)						/* provide parens if none		*/
	{
		node	*last   = find_last_operand(tree);		/* find last operand			*/
		char	*string = last->x_post_string;			/* snarf its string				*/
			
		last->x_post_string = NULL;
	
		put_symbol("(");
		xcompute(tree, TRUE);							/* emit boolean expression		*/
		put_symbol(")");
		
		if (string)										/* and put trailing white		*/
		{												/* space after paren instead	*/
			put_symbol(string);							/* of before					*/
			free(string);
		}
	}
	
	else
		xcompute(tree, TRUE);							/* emit expression				*/
}


/*--------------------------------------------------------------------------------------*/
/* Find Last Operand																	*/
/*--------------------------------------------------------------------------------------*/

/* Used to find the last operand of an expression to clean up certain comment and		*/
/* spacing issues																		*/
static	node *	find_last_operand(node *tree)
{
	if (tree->x_node == x_var)				/* for variables							*/
	{
		if ((node *)(tree->x_arg2))			/* if subscript attached, it is last		*/
			return ((node *)(tree->x_arg2));
		else
			return (tree);
	}
	
	else if (tree->x_node == x_paren)						/* paren expression		*/
		return (tree);
		
	else if (tree->x_node == x_const)						/* constant				*/
		return (tree);
		
	else if (tree->x_node == x_monad)						/* monadic operator		*/
		return (find_last_operand((node *) (tree->x_arg1)));
			
	else if (tree->x_node == x_dyad)						/* dyadic operator		*/
		return (find_last_operand((node *) (tree->x_arg2)));

	else if (tree->x_node == x_proc)						/* procedure call		*/
		return (tree);
		
	else
		{printf( "xpltoc: system error with x_node (find_last_operand) (%p) (%d)\n", tree, tree->x_node); error_exit();}

	return (tree);								/* if we ever get here!!!				*/
}


/*--------------------------------------------------------------------------------------*/
/* Is Pointer Operand																	*/
/*--------------------------------------------------------------------------------------*/

/* This routine is used to look at the operand for the 'core' operator.  It sees 		*/
/* whether the operand is an l-value (e.g. it uses the addr operator)					*/
/* or whether it is a numeric value (e.g. "1").  The appropriate able_core or host_core	*/
/* translation routine can then be called												*/

static	boolean	is_pointer_operand(node *tree)
{
	if (tree->x_node == x_var)				/* for variables							*/
	{
		if ((tree->x_arg1 == t_pointer)		/* if pointer type, then true...			*/
		||  (tree->x_arg1 == t_pointer + t_arr))
			return (TRUE);
		else								/* else is not a pointer....				*/
			return (FALSE);
	}
	
	else if (tree->x_node == x_paren)						/* paren expression		*/
		return (is_pointer_operand((node *) (tree->x_arg2)));
		
	else if (tree->x_node == x_const)						/* constant				*/
		return (FALSE);
		
	else if (tree->x_node == x_monad)						/* monadic operator		*/
	{
		if (tree->x_info == o_adr)							/* addr fundtion: Yes!	*/
			return (TRUE);
		else												/* how about ~addr(x)?	*/
			return (is_pointer_operand((node *) (tree->x_arg1)));
	}
			
	else if (tree->x_node == x_dyad)						/* dyadic operator		*/
		return (is_pointer_operand((node *) (tree->x_arg1))	/* will get realistic	*/
		||      is_pointer_operand((node *) (tree->x_arg2)));	/* cases...			*/

	else if (tree->x_node == x_proc)						/* procedure call		*/
	{
		if ((tree->x_arg2 == t_pointer)						/* if return type is	*/
		||  (tree->x_arg2 == t_pointer + t_arr))			/* pointer, or an 		*/
			return (TRUE);									/* element of ptr array	*/
		else
			return (FALSE);
	}
	
	else
		{printf( "xpltoc: system error with x_node (is_pointer_operand) (%p) (%d)\n", tree, tree->x_node); error_exit();}

	return (FALSE);								/* if we ever get here!!!				*/
}


/*--------------------------------------------------------------------------------------*/
/* Toss Expression																		*/
/*--------------------------------------------------------------------------------------*/

/* Toss all storage for an expression tree */

static	void	toss_expression(node *tree)
{
	if (tree->x_node == x_var)
	{
		if ((node *) (tree->x_arg2))						/* if subscript			*/
		{
			node *subtree    = (node *) (tree   ->x_arg2);	/* refer subscript		*/
			node *subsubtree = (node *) (subtree->x_arg2);	/* actual expression	*/
			
			toss_expression(subsubtree);
			
			x_rel(subtree);
		}
	}
	
	else if (tree->x_node == x_paren)						/* paren expression		*/
		toss_expression((node *) (tree->x_arg2));

	else if (tree->x_node == x_const)						/* constant				*/
		;
	
	else if (tree->x_node == x_monad)						/* monadic operator		*/
		toss_expression((node *) (tree->x_arg1));

	else if (tree->x_node == x_dyad)						/* dyadic operator		*/
	{
		node	*arg1      = (node *)(tree->x_arg1);		/* get operands handy	*/
		node	*arg2      = (node *)(tree->x_arg2);

		toss_expression(arg1);
		toss_expression(arg2);
	}

	else if (tree->x_node == x_proc)						/* procedure call				*/
	{
		if (tree->x_arg1)									/* if any args					*/
		{
			node *arg_tree = (node *) (tree->x_arg1);		/* point to first arg			*/
			node *next_tree;
			
			while (arg_tree)								/* emit each arg				*/
			{
				if (arg_tree->x_node == t_locat)			/* if "location"				*/
				{
					node *loc_expr = (node *)(arg_tree->x_info);
					toss_expression(loc_expr);
				}
				
				else if (arg_tree->x_node == t_sconst)		/* string constant				*/
					;
				
				else if (arg_tree->x_node == t_cconst)		/* c string constant			*/
					;
				
				else if ((arg_tree->x_node == t_var + t_arr)			/* handle arrays	*/
				||       (arg_tree->x_node == t_pointer + t_arr)
				||       (arg_tree->x_node == t_cstring)
				||       (arg_tree->x_node == t_lit))
					;
				
				else										/* else handle expression		*/
				{
					toss_expression((node *)(arg_tree->x_info));
				}

				next_tree = (node *)(arg_tree->x_arg1);		/* get next arg					*/
				
				x_rel(arg_tree);
				
				arg_tree = next_tree;						/* and go on to next argument	*/
			}
		}
	}
		
	else
		{printf( "xpltoc: system error with x_node (toss_expression) (%p) (%d)\n", tree, tree->x_node); error_exit();}

	x_rel(tree);
}


/*--------------------------------------------------------------------------------------*/
/* check_variable_useage																*/
/*--------------------------------------------------------------------------------------*/

/* See if an expression uses a particular variable...									*/

static	boolean	check_variable_useage(node *tree, symbol_struct *variable)
{
	if (tree->x_node == x_var)				/* if node is variable, see if equals		*/
	{										/* variable, or if subscript is variable	*/
		if (((symbol_struct *)(tree->x_info)) == variable)
			return (TRUE);
			
		if (tree->x_arg1 & t_arr)
			return (check_variable_useage((node *) (tree->x_arg2), variable));
		
		return (FALSE);						/* does not access the variable				*/
	}
	
	else if (tree->x_node == x_paren)		/* paren expression							*/
		return (check_variable_useage((node *) (tree->x_arg2), variable));
		
	else if (tree->x_node == x_const)		/* constant									*/
		return (FALSE);
		
	else if (tree->x_node == x_monad)		/* monadic operator							*/
		return (check_variable_useage((node *) (tree->x_arg1), variable));
			
	else if (tree->x_node == x_dyad)		/* dyadic operator							*/
		return ((check_variable_useage((node *) (tree->x_arg1), variable))
		||      (check_variable_useage((node *) (tree->x_arg2), variable)));

	else if (tree->x_node == x_proc)		/* procedure call							*/
	{
		node *arg_tree = (node *) (tree->x_arg1);		/* point to first arg			*/
		
		while (arg_tree)
		{
			if (arg_tree->x_node == t_locat)
			{
				if (check_variable_useage((node *)(arg_tree->x_info), variable))
					return (TRUE);
			}
			
			else if (arg_tree->x_node == t_sconst)
				;
				
			else if (arg_tree->x_node == t_cconst)
				;
				
			else if ((arg_tree->x_node == t_var + t_arr    )	/* handle arrays	*/
			||       (arg_tree->x_node == t_pointer + t_arr)
			||       (arg_tree->x_node == t_cstring        )
			||       (arg_tree->x_node == t_lit            ))
			{
				if ((symbol_struct *)(arg_tree->x_info) == variable)
					return (TRUE);
			}
			
			else										/* arg is expr				*/
			{
				if (check_variable_useage((node *)(arg_tree->x_info), variable))
					return (TRUE);
			}

			arg_tree = (node *)(arg_tree->x_arg1);		/* get next arg				*/
		}
		
		return (FALSE);									/* proc does not use variable	*/
	}
		
	else
		{printf( "xpltoc: system error with x_node (check_variable_useage) (%p) (%d)\n", tree, tree->x_node); error_exit();}

	return (FALSE);								/* if we ever get here!!!				*/
}


/*--------------------------------------------------------------------------------------*/
/* check_for_constant_expression														*/
/*--------------------------------------------------------------------------------------*/

/* See if an expression is made up of constants...										*/

static	boolean	check_for_constant_expression(node *tree)
{
	if (tree->x_node == x_var)				/* for variables							*/
	{
		if (tree->x_arg1 == t_lit)			/* if actually a literal					*/
			return (TRUE);					/* then treet as constant					*/
		else
			return (FALSE);
	}
	
	else if (tree->x_node == x_paren)		/* paren expression							*/
		return (check_for_constant_expression((node *) (tree->x_arg2)));
		
	else if (tree->x_node == x_const)		/* constant									*/
		return (TRUE);
		
	else if (tree->x_node == x_monad)		/* monadic operator							*/
		return (check_for_constant_expression((node *) (tree->x_arg1)));
			
	else if (tree->x_node == x_dyad)		/* dyadic operator							*/
		return ((check_for_constant_expression((node *) (tree->x_arg1)))
		&&      (check_for_constant_expression((node *) (tree->x_arg2))));

	else if (tree->x_node == x_proc)		/* procedure call							*/
		return (FALSE);						/* except shr, shl!!!						*/
		
	else
		{printf( "xpltoc: system error with x_node (check_for_constant_expression) (%p) (%d)\n", tree, tree->x_node); error_exit();}

	return (FALSE);							/* if we ever get here!!!					*/
}


/*--------------------------------------------------------------------------------------*/
/* check_for_read_write																	*/
/*--------------------------------------------------------------------------------------*/

/* See if an expression uses read/write (order may be important!) */

static	boolean	check_for_read_write(node *tree)
{
	if (tree->x_node == x_var)				/* if node is variable, see if equals		*/
	{										/* subscript uses read...					*/
		if (tree->x_arg1 & t_arr)
			return (check_for_read_write((node *) (tree->x_arg2)));
		else
			return (FALSE);					/* does not use read...						*/
	}
			
	else if (tree->x_node == x_paren)		/* paren expression							*/
		return (check_for_read_write((node *) (tree->x_arg2)));
		
	else if (tree->x_node == x_const)		/* constant									*/
		return (FALSE);
		
	else if (tree->x_node == x_monad)		/* monadic operator							*/
	{
		if (tree->x_info == o_read)
			return (TRUE);
		else
			return (check_for_read_write((node *) (tree->x_arg1)));
	}
	
	else if (tree->x_node == x_dyad)		/* dyadic operator							*/
		return ((check_for_read_write((node *) (tree->x_arg1)))
		||      (check_for_read_write((node *) (tree->x_arg2))));

	else if (tree->x_node == x_proc)		/* procedure call							*/
	{
		node *arg_tree = (node *) (tree->x_arg1);		/* point to first arg			*/
		
		while (arg_tree)
		{
			if (arg_tree->x_node == t_locat)
			{
				if (check_for_read_write((node *)(arg_tree->x_info)))
					return (TRUE);
			}
			
			else if (arg_tree->x_node == t_sconst)
				;
				
			else if (arg_tree->x_node == t_cconst)
				;
				
			else if ((arg_tree->x_node == t_var + t_arr    )	/* handle arrays	*/
			||       (arg_tree->x_node == t_pointer + t_arr)
			||       (arg_tree->x_node == t_cstring        )
			||       (arg_tree->x_node == t_lit            ))
				;
				
			else										/* arg is expr				*/
			{
				if (check_for_read_write((node *)(arg_tree->x_info)))
					return (TRUE);
			}

			arg_tree = (node *)(arg_tree->x_arg1);		/* get next arg				*/
		}
		
		return (FALSE);									/* proc does not use read	*/
	}
		
	else
		{printf( "xpltoc: system error with x_node (check_for_read_write) (%p) (%d)\n", tree, tree->x_node); error_exit();}

	return (FALSE);							/* if we ever get here!!!					*/
}


/*--------------------------------------------------------------------------------------*/
/* check_for_proc_call																	*/
/*--------------------------------------------------------------------------------------*/

/* See if an expression calls a procedure (order may be important!) 					*/

static	boolean	check_for_proc_call(node *tree)
{
	if (tree->x_node == x_var)				/* if node is variable, see if equals		*/
	{										/* subscript uses read...					*/
		if (tree->x_arg1 & t_arr)
			return (check_for_proc_call((node *) (tree->x_arg2)));
		else
			return (FALSE);					/* does not call a proc...					*/
	}
			
	else if (tree->x_node == x_paren)		/* paren expression							*/
		return (check_for_proc_call((node *) (tree->x_arg2)));
		
	else if (tree->x_node == x_const)		/* constant									*/
		return (FALSE);
		
	else if (tree->x_node == x_monad)		/* monadic operator							*/
	{
		if (tree->x_info == o_read)			/* treat read as proc call in this case		*/
			return (TRUE);
		else
			return (check_for_proc_call((node *) (tree->x_arg1)));
	}
	
	else if (tree->x_node == x_dyad)		/* dyadic operator							*/
		return ((check_for_proc_call((node *) (tree->x_arg1)))
		||      (check_for_proc_call((node *) (tree->x_arg2))));

	else if (tree->x_node == x_proc)		/* procedure call							*/
	{
		if ((tree->x_info == (long long) shr_proc)		/* shr/shl - allow, but check		*/
		||  (tree->x_info == (long long) shl_proc))		/* args...							*/
		{
			node *arg_tree = (node *) (tree->x_arg1);	/* point to first arg			*/
			
			while (arg_tree)
			{
				if (arg_tree->x_node == t_locat)
				{
					if (check_for_proc_call((node *)(arg_tree->x_info)))
						return (TRUE);
				}
				
				else if (arg_tree->x_node == t_sconst)
					;
					
				else if (arg_tree->x_node == t_cconst)
					;
					
				else if ((arg_tree->x_node == t_var + t_arr    )	/* handle arrays	*/
				||       (arg_tree->x_node == t_pointer + t_arr)
				||       (arg_tree->x_node == t_cstring        )
				||       (arg_tree->x_node == t_lit            ))
					;
					
				else										/* arg is expr				*/
				{
					if (check_for_proc_call((node *)(arg_tree->x_info)))
						return (TRUE);
				}
	
				arg_tree = (node *)(arg_tree->x_arg1);		/* get next arg				*/
			}
			
			return (FALSE);									/* no proc call...			*/
		}
		
		return (TRUE);
	}
	
	else
		{printf( "xpltoc: system error with x_node (check_for_proc_call) (%p) (%d)\n", tree, tree->x_node); error_exit();}

	return (FALSE);							/* if we ever get here!!!					*/
}


/*--------------------------------------------------------------------------------------*/
/* Find Significant Operand																*/
/*--------------------------------------------------------------------------------------*/

/* Basically, skips over unnecessary parenthesis to get at the important node.			*/

static	node *	find_significant_operand(node *tree)
{
	if (tree->x_node == x_paren)
		return (find_significant_operand((node *) (tree->x_arg2)));
	else
		return (tree);
}


/*--------------------------------------------------------------------------------------*/
/* Command-period handling & cleanup													*/
/*--------------------------------------------------------------------------------------*/

Boolean	break_received = FALSE;

static	void	MySignalHandler(int value)
{
	int new_value  = value;
	break_received = TRUE;
}
	
static	void	clean_up(void)
{
	/* output currently scanning procedure to assist with translator debugging */
	
	out_buf         = NULL;					/* output directly to file now				*/
	proto_buf       = NULL;
	out_buf_avail   = 0;
	proto_buf_avail = 0;
	mute_output		= FALSE;
	
	if (proc_code)
	{
		put_symbol(proc_code);
		proc_code  = NULL;
	}
	
	if (proc_handl)
	{
		free_big_memory(proc_handl);
		proc_handl = NULL;
		proc_code  = NULL;
	}
	
	if (main_code)
	{
		put_symbol(main_code);
		free      (main_code);
		main_code = NULL;
	}
	
	if (proto_code)
	{
		put_proto(proto_code);
		free     (proto_code);
		proto_code = NULL;
	}
	
	/* free up allocated memory	*/
	
	if (storage)
		{free(storage); storage = NULL;}
		
	if (symlist)
		{free(symlist); symlist = NULL;}
		
	if (token)
		{free(token); token = NULL;}

	if (in_file)
		{fclose(in_file); in_file = NULL;}
		
	if (out_file)
		{fflush(out_file); fclose(out_file); out_file = NULL;}

	if (proto_file)
		{fflush(proto_file); fclose(proto_file); proto_file = NULL;}
}


/*--------------------------------------------------------------------------------------*/
/*	Statement Scanner Subroutines														*/
/*--------------------------------------------------------------------------------------*/

static	void	toss_white_space()
{
	while (token_type == t_white)
		scan();
}

static	void	toss_blank_lines()
{
	while (token_type == t_white || token_type == t_eol)
		scan();
}

static	void	toss_non_essentials()
{
	while (token_type == t_white || token_type == t_comment || token_type == t_eol)
		scan();
}

static	void	possibly_insert_white_space()
{
	toss_white_space();								/* skip to likely comment		*/
	
	if ((token_type != t_comment)					/* detect no space at all...	*/
	&&  (token_type != t_white  )
	&&  (token_type != t_eol    )					/* if next statement begins		*/
	&&  (token_type != t_eof    ))					/* immediately, add space.		*/
		put_symbol(" ");
}

static	void	process_basic_comment(boolean scan_past_eol)
{
	int	token_input_position  = 0;
	int token_output_position = 0;

	if (token_type == t_comment)					/* comment found				*/
	{
		token_input_position = tcount;				/* save for check				*/
		
		if (tcount)									/* comment started at end of ln	*/
		{
			token_output_position = ((indent / 24) * 24) + comment_pos;
			white(token_output_position);
		}
		else if (indent_comments)					/* else comment started at		*/
		{											/* start of line				*/
			token_output_position = indent;
			tab(token_output_position);
		}
				
		expand_short_comment(token, comment_len);	/* lengthen right edge			*/
		
		if (next_info & b_eol)						/* if followed by eol			*/
			convert_comment(token);
			
		put_symbol(token);							/* write out the comment		*/
		scan();
	}
	
	while (token_type == t_white || token_type == t_comment)
	{
		put_symbol(token);							/* additional space/comments	*/
		scan();										/* on same line?...				*/
	}
	
	if (token_type == t_eol)						/* and copy eol at this time	*/
	{
		put_symbol(token);
		if (scan_past_eol) scan();					/* scan token if not insert		*/
		else return;								/* else return at eol			*/
	}
	
	/* append additional blank lines (but not comments) to the preceding statement	*/
	/* since this is usually the intended effect...									*/
	
	while (token_type == t_white || token_type == t_eol || token_type == t_comment)
	{
		if (token_type == t_eol)					/* keep and clean up			*/
		{											/* blank lines					*/
			tab(indent);
			put_symbol(token);
		}
		
		else if (token_type == t_comment)			/* found a comment on next		*/
		{											/* line							*/
			if (token_count < 10)					/* if at start of line,			*/
				return;								/* leave it for next stmt		*/
		
			tab(token_output_position);
			
			expand_short_comment(token, comment_len);	/* lengthen right edge			*/
			
			if (next_info & b_eol)						/* if followed by eol			*/
				convert_comment(token);
			
			put_symbol(token);							/* write out the comment		*/
		}
		
		scan();
	}
}

static	void	process_stmt_comment(boolean scan_past_eol)
{
	possibly_insert_white_space();
	process_basic_comment(scan_past_eol);
}

		
/*--------------------------------------------------------------------------------------*/
/*	Statement Scanner - Global variables												*/
/*--------------------------------------------------------------------------------------*/

int				stmt_type             = 0;		/* returned by stmt scanner sometimes	*/

char			proc_name  [512]      = {""};	/* holds current proc name				*/
char			nestor_name[512]      = {""};	/* holds name of outer proc				*/
char			nestee_name[512]      = {""};	/* holds name of inner proc				*/
int				proc_name_length      = 0;		/* current length of name				*/
boolean			recurs_proc           = FALSE;	/* true if proc defined as recursive	*/
boolean			header_needed         = TRUE;	/* true if proc header still needed		*/
boolean			static_proc   		  = TRUE;	/* true if proc is local to this file	*/
int				proc_attributes		  = 0;		/* holds procedure attributes			*/
boolean			void_proc             = TRUE;	/* assume proc returns no value...		*/
int				return_type           = t_var;	/* procedure return type information	*/
char			*return_name          = NULL;	/* return type name						*/

int				num_uppers            = 0;		/* number of upper temps in use			*/
int				num_steps             = 0;		/* number of step  temps in use			*/
int				max_uppers			  = 0;		/* number of uppers needed by proc		*/
int				max_steps             = 0;		/* number of steps  needed by proc		*/

symbol_struct	*lab_ptr       		  = NULL;
int				args_to_scan          = 0;		/* counts proc args to declare			*/
int				arg_num               = 0;		/* identifies order of proc arg dcls	*/

static	int	stmt();								/* called recursively					*/

#define			MAX_PROC_ARGS		100			/* max # of args to procedure			*/
int				arg_types[MAX_PROC_ARGS];		/* holds arg types during definition	*/


/*--------------------------------------------------------------------------------------*/
/*	Statement Scanner - emit_xpl_proc_info												*/
/*--------------------------------------------------------------------------------------*/


static	void	emit_xpl_proc_info()
{
	if (proc_attributes)
	{
		while (proc_attributes)
		{
			if (proc_attributes & attr_recursive)
			{
				put_symbol("\t_recursive");
				proc_attributes ^= attr_recursive;
			}
			
			else if (proc_attributes & attr_swap)
			{
				put_symbol("\t_swap");
				proc_attributes ^= attr_swap;
			}
	
			else if (proc_attributes & attr_swapcode)
			{
				put_symbol("\t_swapcode");
				proc_attributes ^= attr_swapcode;
			}
			
			else
				{printf( "xpltoc: unimplemented procedure attribute\n"); error_exit();}
		}
	}
	
	if (proc_level >= 2)			/* preserve XPL nested information	*/
	{
		put_symbol("\t_was_nested(\"");
		put_symbol(nestor_name);
		put_symbol("\", \"");
		put_symbol(nestee_name);
		put_symbol("\")");
	}
}

							
/*--------------------------------------------------------------------------------------*/
/*	Statement Scanner - Insert statement												*/
/*--------------------------------------------------------------------------------------*/

static	void	handle_insert()
{
	char	insert_file_name[512] = {""};
	int 	i = 0;
	
	scan();													/* done with insert				*/
	toss_white_space();
		
	if (token_type != t_sconst || strlen(token) >= sizeof(insert_file_name))
		{printf( "xpltoc: incorrect syntax for 'insert' (missing file name)\n"); error_exit();}
	
	strcpy(insert_file_name, token);						/* store for our use			*/
	
	if (show_progress)
	{
		int i;
		
		for (i=0; i<proc_level; i++) printf("  ");
		printf("start of insert file: %s\n", insert_file_name);
	}
	
	scan();													/* done with name				*/
	
	if (!recognize_c_syntax)								/* if XPL format, toss semi		*/
	{
		toss_non_essentials();
		
		if (token_type != t_semi)
			{printf( "xpltoc: incorrect syntax for 'insert' (missing semicolon)\n"); error_exit();}
	
		scan();												/* toss semicolon				*/
	}
	
	toss_white_space();										/* to likely comment			*/
	
	/* If scanning a translated file, skip over insert of xpl.h								*/
	
	if ((recognize_c_syntax && strcmp(insert_file_name, "XPL.h") == 0)
	||  (recognize_c_syntax && strcmp(insert_file_name, "xpl.h") == 0))
	{
		tab(indent);
		put_symbol("#include\t");							/* map to #include				*/
		put('\"');put_symbol(insert_file_name);put('\"');	/* write out filename			*/
	
		process_stmt_comment(TRUE);							/* if not scanning the file		*/
	}
	
	else											/* else open and scan the file			*/
	{
		if (create_one_file)						/* if creating one file, toss rest of	*/
		{											/* line and continue scanning insert	*/
			while (token_type != t_eol && token_type != t_eof)	/* file						*/
				scan();
		}
		
		else
		{
			tab(indent);
			put_symbol("#include\t");							/* map to #include			*/
			put('\"');put_symbol(insert_file_name);put('\"');	/* write out filename		*/
		
			process_stmt_comment(FALSE);
		}
	
		/* Now open up insert file and scan it, so we know how to process later XPL			*/
		/* statements.  Convert it or not, as the user specifies:							*/
		
		insert_push(next);							/* save next character already scanned	*/
		insert_push(next_info);						/* and its info							*/
		insert_push(lcount);
		insert_push(next_count);
		
		insert_push((long long) in_file ); in_file  = NULL; /* save file reference			*/
	
		if (convert_includes && !create_one_file)
		{
			insert_push((long long) out_file  ); out_file   = NULL;
			insert_push((long long) proto_file); proto_file = NULL;
		}
		
		while (source_file_name[i])							/* push basic file name			*/
			insert_push((long long) source_file_name[i++]);
		
		insert_push(i);
		
		i = 0;
		while (error_file_name[i])							/* push error name too			*/
			insert_push((long long) error_file_name[i++]);
	
		insert_push(i);
		
		strcpy(source_file_name, insert_file_name);			/* copy insert name to src name */
		
		open_source_file(source_file_name);					/* open source in cur dir		*/
	
		if (convert_includes && !create_one_file)
			open_output_file(source_file_name);
		
		get();												/* first char of insert file	*/
		scan();												/* its first token				*/
		
		mute_output = (insert_stack_ptr && !convert_includes && !create_one_file);
	}
}


/*--------------------------------------------------------------------------------------*/
/*	Statement Scanner - Module statement												*/
/*--------------------------------------------------------------------------------------*/

static	void	handle_module()
{
	symbol_struct*	module_info = NULL;
	
	scan();												/* accept 'module'				*/
	toss_non_essentials();								/* get to likely food			*/

	if (token_type != t_semi) {							/* if name specified							*/
		if (token_type != t_und)						/* if there's something there					*/
		{												/* it better be undefined						*/
			er_ifm(9);									/* it's not - complain							*/
		}
		else {											/* it's there and it's undefined				*/
			module_info = insert_symbol(token, t_module, 0);
		}
		scan();											/* get the next token							*/
		toss_non_essentials();
	}													/* of if name specified							*/
	else {												/* no module name								*/
		er_ifm(10);
	}

	if (token_type != t_semi)
		{printf( "xpltoc: missing semicolon after 'module' '%c'\n", next); error_exit();}
		
	scan();												/* skip semi					*/
	toss_white_space();									/* to likely comment			*/

	tab(indent);

	put_symbol("_module(");
	if (module_info)
		put_symbol(module_info->symbol_name);
	put_symbol(")");
	
	process_stmt_comment(TRUE);							/* process comment after module	*/

	end_push(t_nop);									/* no special handling required	*/
	
	while (stmt() != t_end)								/* get statement list			*/
		;
	
	stmt_type = 0;										/* no need to report 'module'	*/
}


/*--------------------------------------------------------------------------------------*/
/*	Statement Scanner - Call, Return, Goto statements									*/
/*--------------------------------------------------------------------------------------*/

static	void	handle_call()
{
	node *tree = NULL;

	if (token_type == t_icell)
	{
		char *which = token_sym->symbol_name;
		
		scan();
		toss_non_essentials();
		
		if (token_type != t_semi)
			{printf( "xpltoc: missing semicolon after 'invoke' '%c'\n", next); error_exit();}
			
		tab(indent);
		
		put_symbol(which);
		put_symbol("()");
	}
	
	else
	{
		tree = expr();										/* get expression				*/
		
		if (token_type != t_semi)
			{printf( "xpltoc: missing semicolon after 'call' '%c'\n", next); error_exit();}
			
		scan();												/* skip semi					*/
		toss_white_space();									/* to likely comment			*/

		tab(indent);
		compute(tree, TRUE);								/* emit expression				*/
	}
	
	put_symbol(";");

	process_stmt_comment(TRUE);
}

static	void	handle_return()
{
	node *tree = NULL;
	scan();												/* accept 'return'				*/
	
	toss_non_essentials();								/* get to likely food			*/
	
	if (token_type != t_semi)							/* if not semi, must be expr	*/
	{
		void_proc = FALSE;								/* inform handle_proc...		*/
		
		tree = expr();									/* get expression				*/
		
		if (is_pointer_operand(tree))					/* if returns an addr()			*/
		{
			return_type = t_pointer;
			
			if (!return_name)							/* set return name too			*/
				return_name = (char *)"pointer";
		}

		toss_non_essentials();
	}
	
	if (token_type != t_semi)
		{printf( "xpltoc: missing semicolon after 'return' '%c'\n", next); error_exit();}
		
	scan();												/* skip semi					*/
	toss_white_space();									/* to likely comment			*/

	tab(indent);

	put_symbol("return");
	
	if (tree)											/* if return value				*/
	{
		put_symbol(" ");
		
		compute_enclosed_expression(tree);				/* emit in parens				*/
	}
	
	put_symbol(";");

	process_stmt_comment(TRUE);
}

static	void	handle_goto()
{
	scan();												/* accept 'goto'				*/
	toss_non_essentials();								/* get to likely food			*/
	
	if (!token_sym)
		{printf( "xpltoc: missing label after 'goto'\n"); error_exit();}
		
	tab(indent);
	put_symbol("goto ");
	put_symbol(token);

	scan();												/* skip symbol					*/
	toss_white_space();									/* to likely comment			*/

	if (token_type != t_semi)
		{printf( "xpltoc: missing semicolon after 'goto' '%c'\n", next); error_exit();}
		
	scan();												/* skip semi					*/
	toss_white_space();									/* to likely comment			*/

	put_symbol(";");

	process_stmt_comment(TRUE);
}

/*--------------------------------------------------------------------------------------*/
/*	Statement Scanner - Enable and Disable Statements									*/
/*--------------------------------------------------------------------------------------*/

static	void	handle_enable()
{
	scan();												/* accept 'enable'				*/
	toss_non_essentials();								/* get to likely food			*/
	
	tab(indent);
	put_symbol("XPL_enable_interrupts()");

	if (token_type != t_semi)
		{printf( "xpltoc: missing semicolon after 'enable' '%c'\n", next); error_exit();}
		
	scan();												/* skip semi					*/
	toss_white_space();									/* to likely comment			*/

	put_symbol(";");

	process_stmt_comment(TRUE);
}

static	void	handle_disable()
{
	scan();												/* accept 'disable'				*/
	toss_non_essentials();								/* get to likely food			*/
	
	tab(indent);
	put_symbol("XPL_disable_interrupts()");

	if (token_type != t_semi)
		{printf( "xpltoc: missing semicolon after 'enable' '%c'\n", next); error_exit();}
		
	scan();												/* skip semi					*/
	toss_white_space();									/* to likely comment			*/

	put_symbol(";");

	process_stmt_comment(TRUE);
}


/*--------------------------------------------------------------------------------------*/
/*	Statement Scanner - If and Else Statements											*/
/*--------------------------------------------------------------------------------------*/

static	void	handle_else();

static	void	handle_if()
{
	node 		*tree      = NULL;
	
	scan();												/* accept 'if'					*/
	toss_non_essentials();								/* get to likely food			*/
	
	tree = expr();										/* get expression				*/
	
	if (token_type == t_then)
	{
		scan();											/* accept 'then'				*/
		toss_white_space();
	}
	
	tab(indent);

	put_symbol("if ");
	
	compute_enclosed_boolean(tree);						/* emit boolean parens			*/

	process_basic_comment(TRUE);						/* process comment after then	*/
	
	if (((token_type == t_stmt)							/* if is 'do', 'begin' or 'if'	*/
	&&   (token_info == s_begin || token_info == s_do || token_info == s_if))
	||   (token_type == t_lbra)
	||   (token_type == t_while))
		stmt();											/* then just process it			*/
	
	else
	{
		indent += tab_indent;                           /* else indent for the if part	*/
		stmt();											/* explicitly					*/
		indent -= tab_indent;
	}
	
	
	/* Must look explicly for else here so if/else get combined into one statement		*/
	
	while (token_type == t_comment)
		process_basic_comment(TRUE);
	
	if (token_type == t_stmt && token_info == s_else)
		handle_else();
	
	stmt_type = 0;										/* no need to report 'if'		*/
}

static	void	handle_else()
{
	scan();												/* accept 'else'				*/
	toss_white_space();
	
	tab(indent);

	put_symbol("else ");
	
	process_basic_comment(TRUE);						/* process comment after else	*/
	
	if (((token_type == t_stmt)							/* if is 'do', 'begin' or 'if'	*/
	&&   (token_info == s_begin || token_info == s_do || token_info == s_if))
	||   (token_type == t_lbra)
	||   (token_type == t_while))
		stmt();											/* then just process it			*/
	
	else
	{
		indent += tab_indent;                           /* indent for the else part		*/
		stmt();											/* explicitly					*/
		indent -= tab_indent;
	}
	
	stmt_type = 0;										/* no need to report 'else'		*/
}


/*--------------------------------------------------------------------------------------*/
/*	Statement Scanner - #If and associated statements									*/
/*--------------------------------------------------------------------------------------*/
static	int		sharp_if_level = 0;

static	void	check_sharp_if_level()					/* make sure not in outer #if	*/
{
	if (sharp_if_level != 0 && proc_level == 0)
	{
		printf("xpltoc: error: #if executable code not handled at global scope\n");
		error_exit();
	}
}

static	void	handle_sharp_if()
{
	node 		*tree      = NULL;
	
	scan();												/* accept '#if'					*/
	toss_non_essentials();								/* get to likely food			*/
	
	tree = scanarg();									/* get expression				*/
	
	tab(indent);

	put_symbol("#if ");
	
	compute_enclosed_boolean(tree);						/* emit boolean parens			*/

	process_basic_comment(TRUE);						/* process comment after then	*/
	
	indent += tab_indent;								/* indent						*/
	sharp_if_level++;
	
	stmt_type = 0;										/* no need to report 'if'		*/
}

static	void	handle_sharp_elseif()
{
	node 		*tree      = NULL;
	
	scan();												/* accept '#elseif'				*/
	toss_non_essentials();								/* get to likely food			*/
	
	tree = scanarg();									/* get expression				*/

	if (indent)	
		indent -= tab_indent;

	tab(indent);

	put_symbol("#elif ");
	
	compute_enclosed_boolean(tree);						/* emit boolean parens			*/

	process_basic_comment(TRUE);						/* process comment after then	*/
	
	indent += tab_indent;                               /* indent						*/

	stmt_type = 0;										/* no need to report 'if'		*/
}

static	void	handle_sharp_else()
{
	scan();												/* accept '#else'				*/

	if (indent)	
		indent -= tab_indent;

	tab(indent);

	put_symbol("#else ");
	
	process_basic_comment(TRUE);						/* process comment after then	*/
	
	indent += tab_indent;                               /* indent						*/

	stmt_type = 0;										/* no need to report 'if'		*/
}

static	void	handle_sharp_endif()
{
	scan();												/* accept '#endif'				*/

	if (indent)	
		indent -= tab_indent;

	if (sharp_if_level)
		sharp_if_level--;

	tab(indent);

	put_symbol("#endif ");
	
	process_basic_comment(TRUE);						/* process comment after then	*/
	
	stmt_type = 0;										/* no need to report 'if'		*/
}


/*--------------------------------------------------------------------------------------*/
/*	Statement Scanner - Write statement													*/
/*--------------------------------------------------------------------------------------*/

static	void	handle_write()
{
	int  device;
	node *value = NULL;
	char device_id[512] = {""};
		
	scan();												/* accept 'write'				*/
	toss_non_essentials();								/* get to likely food			*/
	
	if (token_type != t_lpar)							/* best be write (				*/
		{printf( "xpltoc: syntax error with 'write' (1 %d)\n", token_type); error_exit();}
	
	remap_all_lits = TRUE;
	scan();
	toss_non_essentials();
	remap_all_lits = FALSE;
	
	if (token_type != t_const && token_type != t_octconst)
		{printf( "xpltoc: syntax error with 'write' (2 %d)\n", token_type); error_exit();}
	
	device = token_info;
	
	scan();												/* accept device id				*/
	toss_non_essentials();								/* get to likely food			*/
	
	if (token_type != t_rpar)							/* best be write (id)			*/
		{printf( "xpltoc: syntax error with 'write' (3 %d)\n", token_type); error_exit();}
	
	scan();												/* accept device id				*/
	toss_non_essentials();								/* get to likely food			*/
	
	if (token_type != t_opr || token_info != o_eq)		/* best be write (id) = 		*/
		{printf( "xpltoc: syntax error with 'write' (4 %d)\n", token_type); error_exit();}
	
	scan();												/* accept device id				*/
	toss_non_essentials();								/* get to likely food			*/
	
	value = expr();										/* get argument					*/
	
	if (token_type != t_semi)
		{printf( "xpltoc: missing ';' after 'write'\n"); error_exit();}
	
	scan();												/* accept ';'					*/
	toss_white_space();
	
	tab(indent);
	
	if ((device >= 4   && device <= 7  )				/* multiply/divide unit...		*/
	||  (device >= 060 && device <= 063))				/* or external memory			*/
	{
		switch(device)
		{
			case 4:
				put_symbol("_write_4(");
				break;
			
			case 5:
				put_symbol("_write_5(");
				break;
			
			case 6:
				put_symbol("_write_6(");
				break;
			
			case 7:
				put_symbol("_write_7(");
				break;

			case 060:
				put_symbol("_write_60(");
				break;
			
			case 061:
				put_symbol("_write_61(");
				break;
		
			case 062:
				put_symbol("_write_62(");
				break;
			
			case 063:
				put_symbol("_write_63(");
				break;
		}
		
		compute(value, TRUE);
		
		put_symbol(");");
	}
	
	else
	{
		put_symbol("XPL_write(0");
		
		sprintf(device_id, "%o", device);
		
		put_symbol(device_id);
		put_symbol(", ");
		
		compute(value, TRUE);
		
		put_symbol(");");
	}

	process_stmt_comment(TRUE);
}


/*--------------------------------------------------------------------------------------*/
/*	Statement Scanner - Core statement													*/
/*--------------------------------------------------------------------------------------*/

static	void	handle_core()
{
	node *where = NULL;
	node *value = NULL;
	char *first = NULL;
	char *secnd = NULL;
		
	scan();												/* accept 'core'				*/
	toss_non_essentials();								/* get to likely food			*/
	
	if ((token_type != t_lpar) 							/* make sure paren exists			*/
	&&  (token_type != t_lbr ))							/* or allow brackets too...			*/
		{printf( "xpltoc: syntax error with 'core' (1 %d)\n", token_type); error_exit();}
	
	scan();
	first = scan_space_and_comment();
	
	where = expr();
	
	if ((token_type != t_rpar)							/* scan closing paren				*/
	&&  (token_type != t_rbr ))
		{printf( "xpltoc: syntax error with 'core' (2 %d)\n", token_type); error_exit();}
	
	scan();
	toss_non_essentials();
	
	if (token_type != t_opr || token_info != o_eq)		/* best be core (where) = 		*/
		{printf( "xpltoc: syntax error with 'core' (3 %d)\n", token_type); error_exit();}
	
	scan();												/* accept address			*/
	secnd = scan_space_and_comment();
	
	value = expr();										/* get argument					*/
	
	if (token_type != t_semi)
		{printf( "xpltoc: missing ';' after 'core'\n"); error_exit();}
	
	scan();												/* accept ';'					*/
	toss_white_space();
	
	tab(indent);
	
	if (is_pointer_operand(where))
		put_symbol("set_host_core(");
	else
		put_symbol("set_able_core(");
	
	if (first)
		{put_symbol(first); free(first);}
		
	compute(where, TRUE);
	
	put_symbol(",");

	if (secnd)
		{put_symbol(secnd); free(secnd);}
	
	compute(value, TRUE);
	
	put_symbol(");");

	process_stmt_comment(TRUE);
}


/*--------------------------------------------------------------------------------------*/
/*	Statement Scanner - Linput statement												*/
/*--------------------------------------------------------------------------------------*/

static	void	handle_linput()
{
	node *tree = NULL;
	char *name = NULL;
		
	scan();												/* accept 'linput'				*/
	toss_non_essentials();								/* get to likely food			*/
	
	if (token_type == t_locat)							/* linput location(xyz);		*/
	{
		scan();											/* accept 'location'			*/
		toss_non_essentials();
		
		tree = scanarg();								/* get argument					*/
	}
	
	else if (token_type == t_arr || token_type == t_pointer || token_type == t_lit)
	{
		name = token_sym->symbol_name;					/* point to symbol name			*/
		scan();
		toss_non_essentials();
	}
	
	else
		{printf( "xpltoc: syntax error with 'linput'\n"); error_exit();}
	
	if (token_type != t_semi)
		{printf( "xpltoc: missing ';' after 'linput'\n"); error_exit();}
	
	scan();												/* accept ';'					*/
	toss_white_space();

	tab(indent);
	put_symbol("linput(");
	
	if (tree)
	{
		put_symbol("_location_");						/* convert to C macro			*/
		
		if (!is_pointer_operand(tree))
		{
			printf("xpltoc: warning: operand for 'location' is not of type pointer\n");
			put_symbol("    (#### xpltoc: warning: operand for 'location' is not of type pointer ####)    ");
			error_log();
		}
		
		compute(tree, TRUE);
	}
	
	else
		put_symbol(name);
		
	put_symbol(");");
	
	process_stmt_comment(TRUE);
}


/*--------------------------------------------------------------------------------------*/
/*	Statement Scanner - Print statement													*/
/*--------------------------------------------------------------------------------------*/

static	void	handle_print()
{
	int			i, num_fields = 0;
	
	int			token_list  [512];
	node		*tree_list  [512];
	char		*string_list[512];
	
	scan();												/* accept 'print'				*/
	toss_non_essentials();								/* get to likely food			*/
	
	while (1)											/* scan off formats or			*/
	{													/* expressions					*/
		if (num_fields >= 200)
			{printf( "xpltoc: too many print arguments\n"); error_exit();}
		
		if (token_type == t_sconst)						/* print 'hello'				*/
		{			
			char *where = NULL;
			
			if ((where = (char *)malloc(strlen(token) + 10)) == 0)
				{printf( "xpltoc: out of memory for 'print'\n"); error_exit();}
				
			strcpy(where, token);						/* store the string				*/
			
			token_list [num_fields] = t_sconst;			/* type is sconst				*/
			string_list[num_fields] = where;			/* point to it; free it later	*/
			num_fields++;								/* count field					*/
			
			scan();										/* accept string const			*/
			toss_non_essentials();
		}
		
		else if (token_type == t_string)				/* string(array)				*/
		{	
			scan();										/* accept 'string'				*/
			toss_non_essentials();						/* get to likely food			*/
			
			if (token_type != t_lpar)
				{printf( "xpltoc: missing () after 'string'\n"); error_exit();}
				
			scan();										/* accept 'string'				*/
			toss_non_essentials();						/* get to likely food			*/
			
			if (token_type == t_locat)
			{
				node *tree = NULL;
				
				scan();									/* accept 'location'			*/
				toss_non_essentials();
				
				tree = scanarg();						/* get argument in parens		*/
				
				token_list[num_fields] = t_locat;
				tree_list [num_fields] = tree;
				num_fields++;
			}
			
			else										/* must be array				*/
			{
				if (token_type == t_cstring)
					token_list [num_fields] = t_cstring;
				
				else if ((token_type == t_var + t_arr)
				||       (token_type == t_pointer + t_arr)
				||       (token_type == t_lit))
					token_list [num_fields] = t_arr;
				
				else
					{printf( "xpltoc: need array-type argument for 'print string()'\n"); error_exit();}
				
				string_list[num_fields] = token_sym->symbol_name;
				num_fields++;
				
				scan();									/* accept array name			*/
				toss_non_essentials();					/* get to likely food			*/
			}
			
			if (token_type != t_rpar)
				{printf( "xpltoc: missing () after 'string'\n"); error_exit();}
				
			scan();										/* accept 'paren'				*/
			toss_non_essentials();						/* get to likely food			*/
		}
			
		else if (token_type == t_pform && token_info == 0)	/* octal(expression)			*/
		{
			node *tree = NULL;
			
			scan();										/* accept 'octal'					*/
			toss_non_essentials();
			
			tree = scanarg();							/* get argument						*/
			
			token_list[num_fields] = t_octal;
			tree_list [num_fields] = tree;
			num_fields++;
		}
		
		else if (token_type == t_pform && token_info == 1)	/* chr(expression)				*/
		{													/* by mapping to proc call		*/
			node *tree = NULL;
			
			scan();											/* accept 'chr'					*/
			toss_non_essentials();
			
			tree = scanarg();								/* get argument						*/
			
			token_list[num_fields] = t_character;
			tree_list [num_fields] = tree;
			num_fields++;
		}
		
		else if (token_type == t_semi)						/* semi colon at end of print	*/
		{													/* list (no comma case)			*/
			scan();											/* skip semi					*/
			toss_white_space();								/* to likely comment			*/
	
			token_list[num_fields] = t_semi;				/* emit crlf if we get here		*/
			num_fields++;

			break;
		}
		
		else												/* else must be print i			*/
		{
			node *tree = NULL;
			
			tree = expr();									/* get expression				*/
			
			token_list[num_fields] = t_var;
			tree_list [num_fields] = tree;
			num_fields++;
		}
		
		/* check for comma for more fields... */
		
		if (token_type == t_semi)							/* semi; handled above			*/
			continue;
		
		else if (token_type == t_comma)
		{
			scan();											/* accept comma					*/
			toss_white_space();
			
			if (token_type == t_eol || token_type == t_comment)
				process_stmt_comment(TRUE);
				
			if (token_type == t_semi)						/* if print a,; then skip		*/
			{												/* the crlf						*/
				scan();										/* accept the semi				*/
				toss_white_space();
				break;
			}
			
			else
				continue;
		}
		
		else											/* else unknown...				*/
			{printf( "xpltoc: syntax error in print statement'\n"); error_exit();}
	}
	
	
	/* emit print statement */
	
	if ((num_fields    == 2       )					/* if simple "print 'hello';", then	*/
	&&  (token_list[0] == t_sconst)					/* handle specially					*/
	&&  (token_list[1] == t_semi  )
	&&  (strchr(string_list[0], '%') == NULL))
	{
		char *the_string = string_list[0];
		int	 j           = strlen(the_string);
		
		the_string[j++] = '\n';
		the_string[j  ] = 0;
		
		tab(indent);
		put_symbol("print(");
		emit_c_string(the_string);
		put_symbol(");");
		
		free(the_string);
	}
	
	else												/* else use to arg format			*/
	{
		tab(indent);
		put_symbol("print(\"");
		
		for (i=0; i<num_fields; i++)
		{
			switch (token_list[i])
			{
				case t_sconst:							/* sconst - check for simple		*/
				case t_cstring:
					put_symbol("%s");
					break;
					
				case t_locat:
				case t_arr:
					put_symbol("%a");
					break;
					
				case t_octal:
					put_symbol("0x%04X");
					break;
					
				case t_character:
					put_symbol("%c");
					break;
					
				case t_semi:
					put_symbol("\\n");
					break;
					
				case t_var:
					put_symbol("%6d");
					break;
			}
		}
		
		put_symbol("\"");
		
		for (i=0; i<num_fields; i++)
		{
			if (token_list[i] != t_semi)
				put_symbol(", ");
		
			switch (token_list[i])
			{
				case t_sconst:
					emit_c_string(string_list[i]);
					free(string_list[i]);
					break;
					
				case t_locat:
					put_symbol("_location_");

					if (!is_pointer_operand(tree_list[i]))
					{
						printf("xpltoc: warning: operand for 'location' is not of type pointer\n");
						put_symbol("    (#### xpltoc: warning: operand for 'location' is not of type pointer ####)    ");
						error_log();
					}
					
					compute(tree_list[i], TRUE);
					break;
					
				case t_arr:
				case t_cstring:
					put_symbol(string_list[i]);
					break;
					
				case t_octal:
					compute(tree_list[i], TRUE);
					break;
					
				case t_character:
					compute(tree_list[i], TRUE);
					break;
					
				case t_semi:
					break;
					
				case t_var:
					compute(tree_list[i], TRUE);
					break;
			}
		}
		
		put_symbol(");");
	}
	
	process_basic_comment(TRUE);						/* process comment after ;		*/
}


/*--------------------------------------------------------------------------------------*/
/*	Statement Scanner - Begin statement													*/
/*--------------------------------------------------------------------------------------*/

static	void	handle_begin()
{
	scan();												/* accept 'begin'				*/
	
	if (token_type == t_semi)							/* better be semi				*/
		scan();
	
	toss_white_space();

	tab(indent);

	put_symbol("{");

	indent += tab_indent;

	process_stmt_comment(TRUE);							/* process comment after begin	*/

	end_push(t_nop);									/* no special handling required	*/
	
	while (stmt() != t_end)								/* get statement list			*/
		;
	
	indent -= tab_indent;
	
	stmt_type = 0;										/* no need to report 'begin'	*/
}
	

/*--------------------------------------------------------------------------------------*/
/*	Statement Scanner - Do statements													*/
/*--------------------------------------------------------------------------------------*/

static	void	handle_do()
{
	boolean	skip_semicolon = FALSE;
	
	if (token_type == t_while)							/* accept c-style while			*/
		skip_semicolon = TRUE;							/* for do while					*/
	
	else
	{
		scan();											/* accept 'do'					*/
		toss_non_essentials();							/* get to likely food			*/
	}
	
	if (token_type == t_semi)							/* simple do;					*/
	{
		scan();											/* accept semi					*/
		toss_white_space();								/* on to likely comment			*/
		
		tab(indent);

		put_symbol("{");
	
		indent += tab_indent;

		process_stmt_comment(TRUE);						/* process comment after then	*/
	
		end_push(t_nop);								/* no special handling required	*/
		
		while (stmt() != t_end)							/* get statement list			*/
			;
		
		indent -= tab_indent;
		
		stmt_type = 0;									/* no need to report 'do'		*/
	}
	
	else if (token_type == t_case)						/* do case						*/
	{
		int		next_case = 0;
		node	*tree;
		
		scan();											/* accept 'case'				*/
		toss_non_essentials();							/* get to likely food			*/
		
		tree = expr();									/* get expression				*/
		
		if (token_type != t_semi)
			{printf( "xpltoc: missing ';' after 'do case ()'\n"); error_exit();}
		
		scan();											/* accept 'then'				*/
		toss_white_space();
		
		tab(indent);
	
		put_symbol("switch ");
		
		compute_enclosed_expression(tree);				/* emit switch expression		*/
		
		put_symbol(" {");

		process_basic_comment(TRUE);					/* process comment after case	*/
		
		while (token_type == t_comment)					/* find start of next statement	*/
			process_basic_comment(TRUE);				/* in all cases					*/
		
		indent += tab_indent;
		
		end_push(t_nop);								/* no processing on last end	*/
	
		while (token_type != t_end)						/* get more cases				*/
		{
			char	the_string[512];
			
			sprintf(the_string, "case %d:\n", next_case++);
			tab(indent);
			put_symbol(the_string);
			
			if (((token_type == t_stmt)					/* if is 'do' or 'begin'		*/
			&&   (token_info == s_begin || token_info == s_do))
			||   (token_type == t_lbra)
			||   (token_type == t_while))
			{
				scan();									/* accept 'do' here				*/
				toss_non_essentials();					/* get to likely food			*/
	
				if (token_type != t_semi)				/* better be simple do/begin	*/
					{printf( "xpltoc: incompatible nested 'do' statements\n"); error_exit();}

				scan();									/* accept semi					*/
				toss_white_space();						/* on to likely comment			*/
		
				tab(indent);
		
				put_symbol("{");
			
				indent += tab_indent;

				process_stmt_comment(TRUE);
	
				end_push(t_break);						/* emit break before end		*/
		
				while (stmt() != t_end)					/* get statement list			*/
					;
		
				indent -= tab_indent;
			}
			
			else
			{
				indent += tab_indent;                   /* indent for the case here		*/
				
				stmt();									/* explicitly					*/
				
				tab(indent);
				put_symbol("break;\n");
			
				indent -= tab_indent;
			}
			
			while (token_type == t_comment)				/* find start of next statement	*/
				process_basic_comment(TRUE);			/* in all cases					*/
		}
		
		stmt();											/* pocess end statment			*/
		
		indent -= tab_indent;
	
		stmt_type = 0;									/* do case...					*/
	}
	
	else if (token_type == t_while)						/* handle while					*/
	{
		node 		*tree      = NULL;
		
		scan();											/* accept 'while'				*/
		toss_non_essentials();							/* get to likely food			*/

		tree = expr();									/* get expression				*/
		
		if (!skip_semicolon)							/* for normal do while			*/
		{
			if (token_type != t_semi)
				{printf( "xpltoc: missing ';' after 'do while'\n"); error_exit();}
			
			scan();										/* accept ';'					*/
			toss_white_space();
		
			tab(indent);
		
			put_symbol("while ");
			
			compute_enclosed_boolean(tree);				/* emit boolean parens			*/
			
			put_symbol(" {");
			
			process_basic_comment(TRUE);				/* process comment after while	*/
			
			indent += tab_indent;
			
			end_push(t_nop);							/* no processing on last end	*/
		
			while (stmt() != t_end)						/* scan while loop				*/
				;
				
			indent -= tab_indent;
		}
		
		else											/* else simple while () stmt..	*/
		{
			tab(indent);
		
			put_symbol("while ");
			
			compute_enclosed_boolean(tree);				/* emit boolean parens			*/
			
			process_basic_comment(TRUE);				/* process comment after while	*/
			
			if (((token_type == t_stmt)					/* if is 'do', 'begin'			*/
			&&   (token_info == s_begin || token_info == s_do))
			||   (token_type == t_lbra)
			||   (token_type == t_while))
				stmt();									/* then just process it			*/
			
			else
			{
				indent += tab_indent;                   /* indent for stmt				*/
				stmt();									/* explicitly					*/
				indent -= tab_indent;
			}
		}
		
		stmt_type = 0;									/* do while...					*/
	}
	
	else												/* must be iterative			*/
	{
		symbol_struct 	*iterated = token_sym;			/* point to itated				*/
		node			*lower = NULL;
		node			*upper = NULL;
		node			*step  = NULL;
		node			*last  = NULL;
		boolean			upper_temp = FALSE;
		boolean			step_temp  = FALSE;
		
		char			upper_name[ 20] = {""};
		char			step_name [ 20] = {""};
		char			comment   [512] = {""};
		
		if (token_type != t_var && token_type != t_pointer && token_type != t_lit)
			{printf( "xpltoc: syntax error in do loop\n"); error_exit();}
		
		scan();
		toss_non_essentials();
		
		if (token_type != t_opr || token_info != o_eq)
			{printf( "xpltoc: missing '=' in do loop\n"); error_exit();}
		
		scan();
		toss_non_essentials();
		
		lower = expr();
		
		if (token_type != t_to)
			{printf( "xpltoc: missing 'to' in do loop\n"); error_exit();}
		
		scan();
		toss_non_essentials();
		
		upper = expr();
		
		if (token_type == t_by)
		{
			scan();
			toss_non_essentials();
			
			step = expr();
		}
		
		if (token_type != t_semi)
			{printf( "xpltoc: missing ';' after do loop\n"); error_exit();}
			
		scan();
		toss_white_space();
		
		
		/* remove intermediate trailing spaces: */
		
		last = find_last_operand(lower);
		if (last->x_post_string)
			{free(last->x_post_string); last->x_post_string = NULL;}
			
		last = find_last_operand(upper);
		if (last->x_post_string)
			{free(last->x_post_string); last->x_post_string = NULL;}
			
		if (step)
		{
			if ((step->x_node == x_monad)			/* convert -1 to -1 for this case */
			&&  (step->x_info == o_minus)
			&&  (((node *)(step->x_arg1))->x_node == x_const))
			{
				int	value = ((node *)(step->x_arg1))->x_info;
				
				x_rel((node *)(step->x_arg1));

				step->x_node = x_const;
				step->x_arg1 = t_const;
				step->x_info = -value;
			}
			
			last = find_last_operand(step);
			if (last->x_post_string)
				{free(last->x_post_string); last->x_post_string = NULL;}
		}
		else
		{
			step = x_get();
			step->x_node = x_const;
			step->x_arg1 = t_const;
			step->x_info = 1;
		}
		
		/* Check complexity of upper and step; allocate temp if so						*/
		
		if (check_variable_useage(upper, iterated))			/* use temp for upper		*/
			upper_temp = TRUE;								/* if it uses iterated		*/
			
		else if (check_for_constant_expression(upper))		/* if is constant, is ok	*/
			;
			
		else if ((upper->x_node == x_dyad )					/* else if is a variable-1	*/
		&&       (upper->x_info == o_minus)
		&&       (((node *)(upper->x_arg2))->x_node == x_const)
		&&       (((node *)(upper->x_arg2))->x_info == step->x_info)
		&&       (((node *)(upper->x_arg1))->x_node == x_var)	/* plain variable		*/
		&&       (((node *)(upper->x_arg1))->x_arg2 == 0    ))	/* with no subscript	*/
			;
			
		else if ((upper->x_node == x_var)					/* else if upper is a		*/
		&&       (upper->x_arg2 == 0    ))					/* nonsubscripted variable	*/
			;
		
		else												/* else if is proc or some	*/
			upper_temp = TRUE;								/* other expr, use temp		*/
			
		if (!upper_temp && upper->x_node == x_var)
			put_symbol("   (### check bound useage ###)   ");
		
		if (check_variable_useage(step, iterated))			/* use temp for step		*/
			step_temp = TRUE;								/* if it uses iterated		*/
			
		else if (check_for_constant_expression(step))		/* if is constant, is ok	*/
			;
			
		else if ((step->x_node == x_var)					/* else if step is a		*/
		&&       (step->x_arg2 == 0    ))					/* nonsubscripted variable	*/
			;
		
		else												/* else if is proc or some	*/
			step_temp = TRUE;								/* other expr, use temp		*/
		
		
		/* emit initial assignment: */
		
		tab(indent);							/* indent						*/
		
		put_symbol("for (");

		if (upper_temp)										/* use upper temp			*/
		{
			sprintf(upper_name, "_upper%d", num_uppers++);	/* make a name for thyself	*/
			
			put_symbol(upper_name);
			put_symbol(" = ");
			compute(upper, TRUE);
			upper = NULL;
			
			put_symbol(", ");
		}
		
		if (step_temp)										/* use step temp			*/
		{
			sprintf(step_name, "_step%d", num_steps++);		/* make a name for thyself	*/
			
			put_symbol(step_name);
			put_symbol(" = ");
			compute(step, TRUE);
			step = NULL;
			
			put_symbol(", ");
		}
		
		put_symbol(iterated->symbol_name);
		put_symbol(" = ");
		
		compute(lower, TRUE);
		
		put_symbol("; ");
		
		
		/* emit comparison. XPL allowed negative iteration by constants.	*/
		/* Use >= in that case: */
		
		if ((step->x_node == x_const) && (step->x_info < 0))
		{
			put_symbol(iterated->symbol_name);
			
			put_symbol(" >= ");
			
			if (upper)
				compute(upper, TRUE);
			else
				put_symbol(upper_name);
			
			put_symbol("; ");
		}
		
		/* remove trailing -1 and then a < comparison in common cases: */
		
		else if ((upper                   )
		&&       (step                    )
		&&       (step->x_node == x_const )
		&&       (upper->x_node == x_dyad )
		&&       (upper->x_info == o_minus)
		&&       (((node *)(upper->x_arg2))->x_node == x_const)
		&&       (((node *)(upper->x_arg2))->x_info == step->x_info))
		{
			node *temp_node = (node *)(upper->x_arg1);
			x_rel((node *)(upper->x_arg2));
			x_rel(upper);
			upper = temp_node;
			
			last = find_last_operand(upper);
			if (last->x_post_string)
				{free(last->x_post_string); last->x_post_string = NULL;}
			
			put_symbol(iterated->symbol_name);
			put_symbol(" < ");
			compute(upper, TRUE);
			put_symbol("; ");
		}
		
		else
		{
			put_symbol(iterated->symbol_name);
			
			put_symbol(" <= ");
			
			if (upper)
				compute(upper, TRUE);
			else
				put_symbol(upper_name);
			
			put_symbol("; ");
		}
		
	
		/* emit increment */
		
		if (!step)							/* if temp step			*/
		{
			put_symbol(iterated->symbol_name);
			put_symbol(" += ");
			put_symbol(step_name);
			put_symbol(") {");
		}
		
		else if (step->x_node == x_const && step->x_info == 1)
		{
			put_symbol(iterated->symbol_name);
			put_symbol("++) {");
			x_rel(step);
		}
		
		else if (step->x_node == x_const && step->x_info == (-1))
		{
			put_symbol(iterated->symbol_name);
			put_symbol("--) {");
			x_rel(step);
		}
		
		else
		{
			put_symbol(iterated->symbol_name);
			put_symbol(" += ");
			compute(step, TRUE);
			put_symbol(") {");
		}
		
		process_basic_comment(TRUE);					/* process comment after iteration	*/
		
		indent += tab_indent;
		
		end_push(t_nop);
	
		while (stmt() != t_end)							/* scan loop						*/
			;
			
		indent -= tab_indent;
	
		if (num_uppers >= max_uppers)
			max_uppers = num_uppers;
			
		if (num_steps >= max_steps)
			max_steps = num_steps;

		if (upper_temp) num_uppers--;
		if (step_temp ) num_steps --;
		
		stmt_type = 0;									/* do iteration						*/
	}	
}


/*--------------------------------------------------------------------------------------*/
/*	Statement Scanner - Declare Statement												*/
/*--------------------------------------------------------------------------------------*/

/*	Declare statement syntaxes:
.
.	Basic syntax:
.		declare (symb1, symb2, symb3) (subscript if needed) (type) (attributes);
.
.	Types
.		fixed, floating, data, literal, procedure, array
.
.	Attributes:
.		public, external, recurseive, automatic
.
.	Examples:
.		dcl x		fixed public;
.		dcl (x,y) 	fixed external;
.		dcl x		recursive;
.		dcl x		automatic;
.		dcl x		proc(fixed, fixed, array) external;
.		dcl x		fixed array external;
.		dcl x		array;
*/

#define	MAX_DCLS        100						/* max symbols in dcl list or proc list			*/

/* Basic declare processing handles both C and XPL syntax from here on:					*/

static	boolean	handle_declare(char *type_name, int var_type, boolean extern_dcl, boolean static_dcl, boolean is_native)
{
	symbol_struct	*sym_struct[MAX_DCLS];		/* holds list of scanned symbols				*/
	node			*sym_subexp[MAX_DCLS];		/* possible c-style subscript					*/
	boolean			sym_array  [MAX_DCLS];		/* true if [] scanned for this symbol			*/
	node			*subexpr      = 0;			/* holds subscript expression					*/
	boolean			emit_type     = TRUE;		/* emit type for first variable in list			*/
	char			dcl_com[512]  = {0};		/* will hold declaration comment				*/
	
	int				num_syms      = 0;			/* holds number of symbols scaned				*/
	int				i             = 0;
	
	node			*init_expr    = NULL;
	boolean			more_dcls     = FALSE;
	
	symbol_struct*	map_ptr;
	int				map_cnt;
	

	/* scan off symbol list: */
	
	while (1)
	{
		if (!token_sym)						/* must get symbol								*/
			{printf( "xpltoc: no symbol for 'dcl'\n"); error_exit();}
		
		if ((token_type != t_und)			/* detect re-use of sym							*/
		&&  (token_type != t_unda))			/* in local scope								*/
		{
			token_sym  = insert_symbol(token, t_und, 0);
			token_type = t_und;
			token_info = 0;
		}
		
		if (num_syms >= MAX_DCLS)
			{printf( "xpltoc: too many symbols for 'dcl'\n"); error_exit();}
		
		sym_struct[num_syms] = token_sym;	/* store pointer								*/
		sym_subexp[num_syms] = NULL;		/* assume no subscript							*/
		sym_array [num_syms] = FALSE;		/* assume no [] as well							*/

		scan();								/* accept symbol; get next						*/
		toss_non_essentials();				/* skip to , ) ] ; type							*/
		
		if (token_type == t_lbr)			/* look for C style array subscript after		*/
		{
			scan();							/* skip [										*/
			toss_non_essentials();			/* get first token of subscript					*/
			
			if (token_type == t_rbr)		/* see if [] at this point						*/
				sym_array[num_syms] = TRUE;
			else							/* else must be <expr>]							*/
				sym_subexp[num_syms] = expr();
			
			if (token_type != t_rbr)		/* better see right bracket at this point		*/
				{printf( "xpltoc: syntax error in subscript\n"); error_exit();}
			
			scan();							/* accept ]										*/
			toss_non_essentials();
		}
	
		num_syms++;							/* count 1 more symbol							*/
		
		if (token_type == t_comma)			/* comma: get more								*/
		{
			scan();							/* skip ,										*/
			toss_non_essentials();
			continue;
		}
		
		break;								/* done with list								*/
	}
	
	if (token_type == t_rpar)				/* toss possible ) at end of dcl list (xpl)		*/
	{
		scan();								/* accept )										*/
		toss_non_essentials();				/* get type	or subscr							*/
	}

	
	/* Check for C procedure delcaration: */
	
	if (!is_native && recognize_c_syntax && token_type == t_lpar && num_syms == 1)
	{
		{printf( "xpltoc: cannot handle C proc syntax yet!\n"); error_exit();}
	}
	
	
	/* scan of possible subscript: */
	
	if ((!is_native)
	&&  ((token_type == t_lpar) 				/* if subscript...								*/
	||   (token_type == t_lbr )))
	{
		node * arg2;

		scan();									/* skip paren									*/
		toss_non_essentials();					/* get first token of subscript					*/
		
		subexpr = expr();						/* scan expression								*/

		if ((token_type != t_rpar)				/* scan closing paren							*/
		&&  (token_type != t_rbr ))
			{printf( "xpltoc: syntax error in declaration subscript\n"); error_exit();}
		
		scan();									/* skip paren									*/
		toss_non_essentials();					/* get type										*/

		
		/* Add one to subscript length to match C requirements.  Do this by removing			*/
		/* a trailing -1, or by adding one to a constant, or by adding a 1						*/
		
		arg2 = (node *)(subexpr->x_arg2);		/* get possible arg2 handy						*/
		
		if ((subexpr->x_node == x_dyad )		/* head node is dyadic							*/
		&&  (subexpr->x_info == o_minus)		/* of minus variety								*/
		&&  (arg2->x_node    == x_const)		/* and what is subtracted is a constant of 1	*/
		&&  (arg2->x_info    == 1      ))
		{
			char *post_string = arg2->x_post_string;	/* get space after the "1"				*/
			
			arg2->x_post_string = NULL;			/* null out pointer since we moved it			*/
			
			x_rel(arg2);						/* toss the constant							*/
			arg2 = (node *)(subexpr->x_arg1);	/* get first operand							*/
			x_rel(subexpr);						/* done with the -								*/
			subexpr = arg2;						/* and leave first operand itself				*/
			
			if (subexpr->x_post_string)			/* toss possible white space after the '+'		*/
				free(subexpr->x_post_string);
			
			subexpr->x_post_string = post_string;	/* move wht spc from '1' to after first exp	*/
		}
		
		else if (subexpr->x_node == x_const)	/* else if is a constant						*/
			subexpr->x_info += 1;				/* just add one to it							*/
			
		else									/* else synthesize a +1							*/
		{
			node *last_node = find_last_operand(subexpr);
			
			arg2 = x_get();						/* get a block									*/
			arg2->x_node = x_dyad;				/* make it a dyadic operator					*/
			arg2->x_info = o_plus;				/* plus one										*/
			arg2->x_arg1 = (long long) subexpr; /* start with what we have						*/
			arg2->x_post_string = fabricate_white_space(1);
			
			if (last_node->x_post_string == NULL)	/* add space before + as well				*/
				last_node->x_post_string = fabricate_white_space(1);
				
			subexpr = arg2;						/* make new block the head node					*/
			
			arg2 = x_get();						/* get a second block for the constant			*/
			subexpr->x_arg2 = (long long) arg2; /* link											*/
			
			arg2->x_node = x_const;				/* make it a one								*/
			arg2->x_arg1 = t_const;				/* a decimal constant							*/
			arg2->x_info = 1;					/* of 1											*/
		}
	}
	
	
	/* the declaration type is expected next.  See if the type is a literal for a fundamental	*/
	/* type:																					*/
	
	map_ptr = token_sym;
	map_cnt = 0;
	while (!is_native && token_type == t_lit && map_cnt < 10)	/* check for literal type definitions			*/
	{
		symbol_struct *expanded_sym = find_symbol((char *) (map_ptr->info));
	
		if (expanded_sym)						/* if literal is a string that matches			*/
		{
			token_type = expanded_sym->type;
			token_info = expanded_sym->info;
			map_ptr    = expanded_sym;
			map_cnt++;
		}
	}


	/* handle forward/external procedure declaration: */
	
	if (is_native || (token_type == t_stmt && token_info == s_proc))
	{
		int		num_args   = 0;					/* assume no args needed						*/
		int		arg_types [MAX_PROC_ARGS];
		char	*arg_names[MAX_PROC_ARGS];
		boolean arg_array [MAX_PROC_ARGS];
		boolean returns_anything = FALSE;		/* assume proc is void							*/
		char	*return_name     = NULL;
		int		return_type      = t_var;
		boolean extern_proc      = FALSE;		/* assume proc is local							*/
		int		proc_attributes  = 0;
		int		i;
		
		if (subexpr)
			{printf( "xpltoc: subscript not allowed with procedure declaration\n"); error_exit();}
		
		if (!is_native)
			{scan(); toss_non_essentials();}	/* accept 'proc'; get next						*/
		
		if (token_type == t_lpar)				/* arg list follows								*/
		{
			scan(); toss_non_essentials();		/* accept lpar; get first sym					*/
			
			if (token_type != t_rpar) while (1)	/* scan off args								*/
			{
				int		var_type   = t_var;
				char	*type_name = NULL;
				boolean	is_array   = FALSE;
				
				map_ptr = token_sym;
				map_cnt = 0;
				while (token_type == t_lit && map_cnt < 10)	/* check for literal type definitions			*/
				{
					symbol_struct *expanded_sym = find_symbol((char *) (map_ptr->info));
				
					if (expanded_sym)						/* if literal is a string that matches			*/
					{
						token_type = expanded_sym->type;
						token_info = expanded_sym->info;
						map_ptr    = expanded_sym;
						map_cnt++;
					}
				}
				
				if (token_type != t_type)
					{printf( "xpltoc: missing type in procedure declaration\n"); error_exit();}
		
				var_type  = token_info;			/* save variable type							*/
				type_name = token_sym->symbol_name;
				is_array  = FALSE;
				
				scan();							/* accept type; get next						*/
				toss_non_essentials();

				if ((token_type == t_type)		/* detect fixed array							*/
				&&  (token_info == t_arr ))
				{
					scan();						/* accept array									*/
					toss_non_essentials();
					var_type |= t_arr;			/* arg is actually array						*/
					is_array = TRUE;			/* emit [] after arg type to C					*/
				}
					
				if (num_args >= MAX_PROC_ARGS)
					{printf( "xpltoc: too many arguments in procedure declaration\n"); error_exit();}
				
				arg_types[num_args] = var_type;
				arg_names[num_args] = type_name;
				arg_array[num_args] = is_array;
				
				num_args++;
				
				if (token_type == t_comma)
				{
					scan();
					toss_non_essentials();
					continue;
				}
				else
					break;
			}
			
			if (token_type != t_rpar)		
				{printf( "xpltoc: problem with procedure declaration argument list\n"); error_exit();}
		
			scan(); toss_non_essentials();			/* accept rpar					*/
		}
		
		while ((token_type == t_rtns     )
		||     (token_type == t_recurs   )
		||     (token_type == t_swap     )
		||     (token_type == t_swpcode  )
		||     (token_type == t_type     )
		||     (token_type == t_storage  )
		||     (token_type == t_lit      ))
		{
			if (token_type == t_lit)
			{
				symbol_struct *expanded_sym = NULL;
				
				if (*(char *)(token_sym->info) == 0)			/* dcl swapable lit '' */
				{
					scan();
					toss_non_essentials();
					continue;
				}
				
				expanded_sym = find_symbol((char *) (token_sym->info));
			
				if (expanded_sym)						/* if literal is a string that matches			*/
				{
					token_type = expanded_sym->type;
					token_info = expanded_sym->info;
				}
				
				if ((token_type != t_rtns     )
				&&  (token_type != t_type     )
				&&  (token_type != t_recurs   )
				&&  (token_type != t_swap     )
				&&  (token_type != t_swpcode  )
				&&  (token_type != t_storage  ))
					{printf( "xpltoc: unrecognized attribute for procedure (%s)\n", token_sym->symbol_name); error_exit();}
			}
			
			if (token_type == t_type)
			{
				returns_anything = TRUE;
				return_name      = token_sym->symbol_name;
			}
			
			else if (token_type == t_rtns)
			{
				scan();
				toss_non_essentials();
				
				if (token_type != t_lpar)
					{printf( "xpltoc: syntax error with 'returns' attribute\n"); error_exit();}
					
				scan();
				toss_non_essentials();
				
				map_ptr = token_sym;
				map_cnt = 0;
				while (token_type == t_lit && map_cnt < 10)	/* check for literal type definitions			*/
				{
					symbol_struct *expanded_sym = find_symbol((char *) (map_ptr->info));
				
					if (expanded_sym)						/* if literal is a string that matches			*/
					{
						token_type = expanded_sym->type;
						token_info = expanded_sym->info;
						map_ptr    = expanded_sym;
						map_cnt++;
					}
				}
				
				if (token_type != t_type)
					{printf( "xpltoc: syntax error with 'returns' type\n"); error_exit();}
	
				returns_anything = TRUE;
				return_name      = token_sym->symbol_name;
	
				scan();
				toss_non_essentials();
				
				if (token_type != t_rpar)
					{printf( "xpltoc: syntax error with 'returns' attribute (1)\n"); error_exit();}
			}
		
			else if (token_type == t_storage)
			{
				if (token_info == t_extern)
					extern_proc = TRUE;
				else
					{printf( "xpltoc: incompatible procedure declaration (%s)\n", token_sym->symbol_name); error_exit();}
			}	
			
			else if (token_type == t_recurs)
				proc_attributes |= attr_recursive;
			
			else if (token_type == t_swap || token_type == t_swpcode)
				proc_attributes |= token_info;
			
			scan();
			toss_non_essentials();
		}
	
	
		if (token_type == t_comma)
			more_dcls = TRUE;
			
		else if (token_type != t_semi)
			{printf( "xpltoc: missing semicolon\n"); error_exit();}
	
		scan();								/* skip over semicolon; most likely white space or eol	*/
		toss_white_space();					/* to likely comment									*/
	
		if (num_syms != 1)					/* C only supports 1 #define					*/
			{printf( "xpltoc: incompatible procedure declaration\n"); error_exit();}

		tab(indent);						/* tab											*/
		
		if (extern_proc)
		{
			if (returns_anything)
			{
				put_symbol("extern\t");
				put_symbol(return_name);
				put_symbol("\t");
			}
			else
				put_symbol("extern\tvoid\t");
		}
		
		else
		{
			if (returns_anything)
			{
				put_symbol(return_name);
				put_symbol("\t\t\t");
			}
			else
				put_symbol("void\t\t\t");
		}
	
		put_symbol(sym_struct[0]->symbol_name);
		put_symbol("(");
		
		for (i=0; i<num_args; i++)
		{
			put_symbol(arg_names[i]);
			
			if (arg_array[i])
				put_symbol("[]");
				
			if (i != num_args - 1)
				put_symbol(", ");
		}
		
		put_symbol(");");
		
		/* store information about symbol so we can call it: */
		sym_struct[0]->type  = t_proc;						/* type is now procedure		*/
		sym_struct[0]->info  = num_args;					/* now store num args			*/
		sym_struct[0]->other = store_proc_args(num_args, arg_types, return_type);
	}

	
	/* Handle label declarations: */
	
	else if (token_type == t_type && token_info == 5)
	{
		scan();									/* accept label					*/
		toss_non_essentials();					/* better get semi				*/
		
		if (subexpr)
			{printf( "xpltoc: subscript not allowed with label declaration\n"); error_exit();}
		
		if (token_type != t_semi)
			{printf( "xpltoc: missing semicolong after label declaration\n"); error_exit();}
		
		scan();									/* accept ;						*/
		toss_white_space();						/* to likely comment			*/
		
		tab(indent);
			
		for (i=0; i<num_syms; i++)
		{
			put_symbol("_label(");
			put_symbol(sym_struct[i]->symbol_name);
			put_symbol(")");
			
			if (i < num_syms-1) put_symbol(" ");
		}
	}
	
	
	/* Handle literal declarations: */
	
	else if (token_type == t_type && token_info == 6)		/* t_lit declaration */
	{
		node	*tree = NULL;
		char	lit_string[512] = {""};
		boolean	is_location = FALSE;
		
		return_apos = TRUE;						/* return apostophes now		*/
		
		if (num_syms != 1)						/* C only supports 1 #define					*/
			{printf( "xpltoc: incompatible literal declaration\n"); error_exit();}
		
		scan();									/* accept literally				*/
		toss_non_essentials();					/* better get apo				*/
		
		if (subexpr)
			{printf( "xpltoc: subscript not allowed with literal declaration\n"); error_exit();}
		
		if (token_type != t_apo)
			{printf( "xpltoc: missing literal string\n"); error_exit();}
		
		scan();									/* accept '						*/
		toss_non_essentials();
		
		/* detect null literal: */
		
		sym_struct[0]->type = t_lit;			/* assume we will be literal	*/

		if (token_type == t_apo)				/* null string; token == t_lit	*/
			;									/* set below					*/
			
		else if (next == '\'')					/* else if single token			*/
		{
			strcpy(lit_string, token);			/* save also as t_lit for remap	*/
			
			if (token_type == t_const || token_type == t_octconst)
				tree = expr();					/* get in expression form if so	*/

			else scan();						/* accept symbol; get t_apo		*/
		}
		
		else if (token_type == t_locat)			/* dcl x 'location(xyz)'		*/
		{
			scan();								/* accept location				*/
			toss_white_space();					/* get arg						*/
			is_location = TRUE;
			tree = expr();						/* get a math expression		*/
		}
		
		else if ((token_type == t_while )		/* dcl forever  lit 'while 1'			*/
		||       (token_type == t_recurs)		/* dcl swapable lit 'recursive swap'	*/
		||       (token_type == t_stmt  )		/* dcl forward  lit 'procedure'			*/
		||       (token_type == t_opr && token_info == o_read)) /* dcl rem lit 'read(4)'*/
		{
			int i;
			
			strcpy(lit_string, token_sym->symbol_name);
			
			i = strlen(lit_string);				/* get its length				*/
			
			while (next != '\'' && i < sizeof(lit_string) - 1)
			{
				lit_string[i++] = next;
				lit_string[i  ] = 0;
				get();
			}
			
			scan();								/* get t_apo					*/
		
			sym_struct[0]->type = t_remapsymb;	/* translater must map this		*/
		}
		
		else
		{
			tree = expr();						/* get a math expression		*/
			
			if (tree->x_node == x_const)		/* if is a constant...			*/
				sprintf(lit_string, "%d", (int)tree->x_info);
		}
		
		if (token_type != t_apo)
			{printf( "xpltoc: syntax error in literal string; e.g. not an expression\n"); error_exit();}
		
		return_apos = FALSE;

		scan();									/* accept '						*/
		toss_non_essentials();
		
		sym_struct[0]->info = (long long) store_literal(lit_string);

		tab(indent);
		put_symbol("#define\t");
		
		put_symbol(sym_struct[0]->symbol_name);
		
		
		white(define_pos);
		
		if (tree)
		{
			if (is_location)
			{
				if (!is_pointer_operand(tree))
				{
					printf("xpltoc: warning: operand for 'location' is not of type pointer\n");
					put_symbol("   (#### xpltoc: warning: operand for 'location' is not of type pointer ####)    \n");
					error_log();
				}
				
			
				put_symbol("_location_");
			}
			
			compute(tree, TRUE);
		}
		
		else
			put_symbol(lit_string);
		
		if (token_type == t_comma)
			more_dcls = TRUE;
			
		else if (token_type != t_semi)
			{printf( "xpltoc: missing semicolon\n"); error_exit();}
	
		scan();								/* skip over semicolon; most likely white space or eol	*/
		toss_white_space();					/* to likely comment									*/
	}
	
	
	/* Else handle variable declaration: */
	
	else
	{
		boolean is_array = FALSE;				/* assume no array designator					*/
		
		if (!recognize_c_syntax)				/* for XPL, type must appear here				*/	
		{
			if (token_type != t_type)
				{printf( "xpltoc: missing type in declaration\n"); error_exit();}
			
			var_type  = token_info;				/* save variable type							*/
			type_name = token_sym->symbol_name;	/* and save name pointer						*/
			
			scan();								/* accept type; get next						*/
			toss_non_essentials();
		
			if (var_type == t_arr)				/* detect simple 'array' type					*/
			{
				if (subexpr)					/* special logic for dcl a(100) array			*/
					type_name = (char *)"fixed";/* need to emit "fixed[100]"					*/
			}
			
			else if ((var_type == t_var)		/* if "fixed" or "pointer", check for			*/
			||       (var_type == t_pointer))	/* "fixed data" or "fixed array"				*/
			{
				if (subexpr) var_type |= t_arr;	/* convert to array if subscript present		*/
				
				if (token_type == t_type)		/* check for further declaration type			*/
				{
					if (token_info == t_arr)	/* 'array'										*/
					{
						var_type |= t_arr;		/* 'dcl x fixed array'							*/
						
						is_array = TRUE;
						
						scan();
						toss_non_essentials();
					}
					
					else if (token_info == 7)	/* 'data'										*/
					{
						var_type = 7;			/* convert 'fixed data' to just 'data'			*/
						
						scan();
						toss_non_essentials();
					}
				}
			}
		
		
			/* Check for public, external, static, automatic: */
			
			while (token_type == t_storage)
			{
				if (token_info == t_extern)			/* make our declaration external			*/
					extern_dcl = TRUE;
					
				if (token_info == t_public)			/* public; remove 'static' attribute		*/
					static_dcl = FALSE;				/* presumably not in a procedure			*/
				
				if (token_info == s_static)			/* presumably within a procedure; add		*/
					static_dcl = TRUE;				/* a static attribute						*/
					
				if (token_info == s_automatic)		/* presumably within a procedure; remove	*/
					static_dcl = FALSE;				/* the static attribute						*/
	
				scan();
				toss_non_essentials();
			}

			
			/* correct the type name for data types: */
			
			if (var_type == 7)						/* type is data								*/
			{
				if (!extern_dcl)					/* if declared here (vs 'external')			*/
					type_name = (char *)"fixed";    /* then declare as fixed...					*/
				else
					type_name = (char*)"data";      /* else external (handle dcl x fixed data external) */
			}
			
			if (var_type == t_arr && extern_dcl)
				type_name =(char*)"array";
		}
	
	
		/* handle semicolon, comment, and eol at end of declaration */
		
		if ((var_type == 7)					/* if is data							*/
		&&  (!extern_dcl))					/* defined here...						*/
		{
			if (token_type != t_lpar)
				{printf( "xpltoc: missing left paren for type 'data'\n"); error_exit();}
		
			scan();							/* accept paren							*/
		}
		
		else								/* if not data, must be semi/eol here	*/
		{
			if (token_type == t_comma)
				more_dcls = TRUE;
				
			else if (token_type != t_semi)
				{printf( "xpltoc: missing semicolon after declaration\n"); error_exit();}
		
			scan();							/* skip over semicolon; most likely white space or eol	*/
			toss_white_space();				/* to likely comment									*/
								
			if (token_type == t_comment && strlen(token) < sizeof(dcl_com)-tab_indent)
				strcpy(dcl_com, token);
				
			
			/* Handle assignment on same line as C initialization */
			
			if ((token_type == t_und && token_sym == sym_struct[0])		/* next token is us		*/
			&&  (var_type   == t_var || var_type  == t_pointer   ))		/* and not array		*/
			{
				scan();						/* over the symbol								*/
				toss_non_essentials();		/* better get equals sign						*/
	
				if (token_type != t_opr || token_info != o_eq)
					{printf( "xpltoc: incompatible declaration initialization for '%s'\n", sym_struct[0]->symbol_name); error_exit();}
				
				scan();						/* assignment operator							*/
				toss_non_essentials();		/* get to expression							*/
				
				init_expr = expr();			/* and get what we are assigning				*/
				toss_non_essentials();		/* best get semicolon							*/
				
				if (token_type != t_semi)
					{printf( "xpltoc: missing semicolon after variable initialization for '%s'\n", sym_struct[0]->symbol_name); error_exit();}
		
				scan();						/* skip over semicolon; most likely white space or eol	*/
				toss_white_space();			/* to likely comment									*/
									
				if ((token_type == t_comment)
				&&  (strlen(token) + strlen(dcl_com) < sizeof(dcl_com) - 8 ))
				{
					if (dcl_com[0] != 0)
						strcat(dcl_com, " ");
						
					strcat(dcl_com, token);	/* get trailing comment for dcl					*/
				}
			}
		}
					
		
		/* Branch on type: */		
		
		switch(var_type)
		{
			case t_var:								/* fixed, boolean, pointer, fixed array		*/
			case t_pointer:
			case t_var + t_arr:
			case t_pointer + t_arr:
			case t_cstring:
			case 7:									/* data										*/
			{
				for (i=0; i<num_syms; i++)
				{
					int	this_type = var_type;
					
					if (this_type == 7)				/* handle data as array for now				*/
						this_type = t_arr;
					
					if (sym_subexp[i] || sym_array[i])	/* convert to array if c-style subscript	*/
						this_type |= t_arr;				/* is present								*/
					
					if (!type_name)
						{printf( "xpltoc: incorrect declaration syntax\n"); error_exit();}
				
			
					/* if declaring procedure arguments, put each declaration on one line		*/
					/* for better visibility.  Terminate procedure declaration header			*/
					/* after last arg, then continue with normal declarations, to handle		*/
					/* common XPL case of declaring args & local variables on one line...		*/
				
					if (args_to_scan)				/* if still doing proc args...				*/
					{
						stmt_type = t_unda;			/* inform proc stmt that we got a dcl		*/
						
						/* check that symbol is a t_unda, and that the args						*/
						/* are declared in the same order is in the procedure					*/
						/* declaration															*/
						
						if ((sym_struct[i]->type != t_unda)
						||  (sym_struct[i]->info != arg_num))
							{printf( "xpltoc: incompatible procedure argument type declaration\n"); error_exit();}
					
						sym_struct[i]->type  = this_type;
						arg_types[arg_num++] = this_type;
					
						tab(indent);			/* tab for declaration							*/
						put_symbol(type_name);	/* emit type - "fixed", "boolean", "pointer"	*/
						white(indent+8);
						put_symbol(sym_struct[i]->symbol_name);

						if (!static_proc)		/* emit prototype for public procs only			*/
						{
							put_proto (type_name);
							put_proto (" ");
							put_proto(sym_struct[i]->symbol_name);
						}
						
						if (is_array || sym_array[i])
						{
							put_symbol("[]");
							if (!static_proc)	/* emit prototype for public procs only			*/
								put_proto ("[]");
						}
						
						
						/* handle declaration of last procedure argument: */
						
						if ((args_to_scan -= 1) == 0)
						{
							put_symbol(")");				/* term arg list					*/
							
							if (!static_proc)				/* emit proto if public				*/
								put_proto (")");
							
							emit_xpl_proc_info();			/* preserve XPL info				*/

							if (i != num_syms-1)			/* more to go on this declaration	*/
							{
								put_symbol("\n");			/* put in line break				*/
							
								tab(indent - tab_indent);   /* finish proc header for			*/
								put_symbol("{\n");			/* imminent DCL						*/
								
								put(3);						/* temporary declaration list will go here */
								
								header_needed = FALSE;		/* not needed again					*/
							}
						}
						
						
						/* else handle case with more arguments to go: */
						
						else
						{
							if (i != num_syms-1)			/* more to go on this declaration	*/
								put_symbol(",\n");			/* on this dcl line --> insert eol	*/
							else							/* else end of this dcl stmt but	*/
								put_symbol(", ");			/* more proc args to go...			*/
							
							if (!static_proc)			/* emit prototype for public procs only	*/
								put_proto (", ");			/* use , in prototype in all cases	*/
						}
					}
					
					
					/* else handle a non-proc-arg case.  This could be a global C variable		*/
					/* or a variable defined within a C procedure								*/
					
					else						/* non-args: do on one line						*/
					{
						if ((this_type & t_arr)	/* if this is an array							*/
						&&  (!extern_dcl)		/* that as instantiated here					*/
						&&  (!subexpr)			/* with no subscript, then error.  E.G. need	*/
						&&  (!sym_subexp[i])	/* subscript if not external...					*/
						&&  (var_type != 7))	/* unless it is data, of course!				*/
							{printf( "xpltoc: subscript required for array declaration\n"); error_exit();}
				
						sym_struct[i]->type = this_type;
						
						if (emit_type)			/* emit type for first arg only					*/
						{
							emit_type = FALSE;	/* one time only								*/

							tab(indent);		/* tab for declaration				*/

							if (extern_dcl)
								put_symbol("extern\t");
								
							else if (static_dcl)
								put_symbol("static\t");
							
							put_symbol(type_name);	/* emit type - "fixed", "boolean", "pointer" */
							white(indent+16);
							
							if (proc_level == 0 && !static_dcl && !extern_dcl)
							{
								put_proto("extern\t");
								
								if (var_type == 7)				/* substitute "data" for		*/
									put_proto("data");			/* fixed in prototype file..	*/
								else if (var_type == t_arr)
									put_proto("array");
								else
									put_proto(type_name);
								
								proto_tab(16);
							}
						}
						
						put_symbol(sym_struct[i]->symbol_name);	/* emit variable name		*/
						
						if (proc_level == 0 && !static_dcl && !extern_dcl)
							put_proto(sym_struct[i]->symbol_name);
						
						
						/* emit subscript or [] if needed */
						
						if ((subexpr)						/* if has subscript explicitly		*/
						||  (sym_subexp[i])					/* or c-style subscript				*/
						||  (var_type == 7 && !extern_dcl)	/* or is non-external data...		*/
						||  (var_type == t_pointer + t_arr && extern_dcl)) /* or ext pointer array */
						{
							put_symbol("[");
							
							if (sym_subexp[i])			/* if specific c-style subscript		*/
								compute(sym_subexp[i], TRUE);
								
							else if (subexpr)
								compute(subexpr, i == (num_syms - 1));
							
							put_symbol("]");
							
							if (proc_level == 0 && !static_dcl && !extern_dcl && var_type != 7 && var_type != t_arr)
								put_proto("[]");
						}
						
						
						/* handle data declaration */
						
						if ((var_type == 7)				/* if is data							*/
						&&  (!extern_dcl)				/* defined here...						*/
						&&  (i == 0))					/* one shot only						*/
						{
							put_symbol(" = {");			/* dcl x data (0,1,2)					*/
							
							while (token_type != t_rpar)
							{
								node *tree;
								
								if (token_type == t_eof)
									{printf( "xpltoc: unexpected end of file\n"); error_exit();}
								
								if (token_type == t_white)
								{
									put_symbol(token);
									scan();
								}
								
								process_basic_comment(TRUE);
								
								if (token_type == t_sconst)
								{
									if (create_xpl_strings)
										emit_xpl_data_string(token);
									else
										emit_c_string(token);
										
									scan();
									toss_non_essentials();
								}
								
								else
								{
									tree = expr();
									tab(indent);
									compute(tree, TRUE);
								}
								
								if (token_type == t_comma)
								{
									scan();
									put_symbol(",");
									continue;
								}
							}
							
							scan();						/* accept right paren					*/
							toss_non_essentials();
						
							put_symbol("}");
							
							if (token_type != t_semi)
								{printf( "xpltoc: missing semicolon after 'data'\n"); error_exit();}
						
							scan();							/* skip over semicolon; most likely white space or eol	*/
							toss_white_space();				/* to likely comment									*/
						}
						

						/* emit initialization string if one */
						
						else if (init_expr)				/* if not data, check for				*/
						{								/* initializer							*/
							put_symbol(" = ");
							
							compute(init_expr, TRUE);
							
							init_expr = NULL;
						}
					
						if (i != num_syms-1)			/* put in separator if more to go		*/
						{
							put_symbol(", ");
							
							if (proc_level == 0 && !static_dcl && !extern_dcl)
								put_proto(", ");
						}
						else
						{
							put_symbol(";");			/* else terminate arg list				*/
							
							if (proc_level == 0 && !static_dcl && !extern_dcl)
							{	
								put_proto(";");

								if (dcl_com[0])
								{
									expand_short_comment(dcl_com, comment_len);
									convert_comment(dcl_com);
									put_proto("\t");
									proto_tab(comment_pos);
									put_proto(dcl_com);
								}
								
								put_proto("\n");
							}
						}
					}
				}
				
				break;
			}
			
			case t_fvar:				/* floating							*/
			default:
				{printf( "xpltoc: unimplemented type in declaration\n"); error_exit();}
		}
	}

	process_stmt_comment(TRUE);			/* handle dcl statement comment							*/

	return (more_dcls);
}


/*--------------------------------------------------------------------------------------*/
/*	Statement Scanner - Fabricate Temp List												*/
/*--------------------------------------------------------------------------------------*/

/* Fabricate Temp List constructs a list of temporary variables we need in C to			*/
/* handle XPL's iterative do loop structure.											*/

static	void	fabricate_temp_list()
{
	int i;
	char	name[20];
	
	if (max_uppers + max_steps == 0)			/* none needed if none needed			*/
		return;
		
	temp_list[0] = 1;							/* tab to indent needed					*/
	temp_list[1] = 0;
	
	strcat(temp_list, "    fixed\t\t\t");
	
	for (i=0; i<max_uppers; i++)
	{
		sprintf(name, "_upper%d", i);
		strncat(temp_list, name, sizeof(temp_list)-1);
	
		if (i != max_uppers -1)
			strncat(temp_list, ", ", sizeof(temp_list)-1);
	}
	
	if (max_uppers && max_steps)
		strncat(temp_list, ", ", sizeof(temp_list)-1);

	for (i=0; i<max_steps; i++)
	{
		sprintf(name, "_step%d", i);
		strncat(temp_list, name, sizeof(temp_list)-1);
	
		if (i != max_steps -1)
			strncat(temp_list, ", ", sizeof(temp_list)-1);
	}
	
	strncat(temp_list, ";\n", sizeof(temp_list)-1);
}


/*--------------------------------------------------------------------------------------*/
/*	Statement Scanner - Procedure definition											*/
/*--------------------------------------------------------------------------------------*/

/* Procedure definition syntaxes:
.       a: proc fixed;                       : fixed
.       a: proc recursive;                   : recursive
.       a: proc recursive swap;              : recursive, swappable
.       a: proc floating public recursive;   : floating, public, recursive
.       a: proc returns (floating);          : floating
*/

static	void	pop_code();

static	void	handle_procedure()
{
	int				num_args               = 0;
	char			proc_com [1024]        = {0};
	char			proc_hdr [TEMP_LENGTH] = {0};
	symbol_struct	*sym_struct;
	symbol_struct	*map_ptr;
	int				map_cnt;
	
	/* Begin by pushing the comments and stuff that preceded the proc definition.		*/
	/* We will emit these at the right time later...									*/
	
	strcpy(proc_hdr, temp_code);				/* stash preceding comment(s)			*/
	temp_code[0] = 0;							/* none to write on pop					*/
	pop_code();									/* restore output pointers				*/
	
	/* Record symbol definition as procedure with no arguments for now.  That will keep	*/
	/* the expression scanner from blowing up if someone tries an illegal recursion.	*/
	/* We will patch in the argument after it is declared.								*/
	
	if (!lab_ptr)
		{printf( "xpltoc: missing label for procedure definition\n"); error_exit();}

	sym_struct       = lab_ptr;					/* save pointer to proc block	*/
	sym_struct->type = t_proc;					/* type is now procedure		*/
	
	push(proc_name_length);						/* save length of current name	*/
	
	strcpy(nestor_name, proc_name);				/* save outer proc name			*/
	strcpy(nestee_name, sym_struct->symbol_name);
	
	if (proc_name_length + strlen(sym_struct->symbol_name) >= sizeof(proc_name) - 1)
		{printf("xpltoc: nested procedure name too long\n"); error_exit();}
	
	if (proc_name_length)						/* separate names with _		*/
		strcat (proc_name, "_");
	
	strcat (proc_name, sym_struct->symbol_name);
	proc_name_length = strlen(proc_name);
	
	if (proc_level)								/* if this is a nested proc		*/
	{											/* then remap name...			*/
		sym_struct->type = t_remapproc;			/* remap at  lowest level		*/
		sym_struct->info = (long long) store_literal(proc_name);
		sym_struct = insert_symbol(proc_name, t_proc, 0);
	}
	
	if (show_progress)
	{
		int i;
		
		for (i=0; i<proc_level; i++) printf("  ");
		printf("start of procedure: %s\n", proc_name);
	}
	
	/* Push global variables here so we can handle XPL's nested procedure		*/
	/* definitions.																*/
	
	push(num_uppers);
	push(num_steps);
	push(max_uppers);
	push(max_steps);
	
	push(indent);
	push(args_to_scan);							/* at this point, proc is		*/
	push(arg_num);								/* defined with 0 args			*/
	
	push(static_proc);
	push(recurs_proc);
	push(header_needed);
	push(proc_attributes);
	push(void_proc);
	push(return_type);
	push((long long) return_name);
	
	push((long long) out_buf);
	push((long long) out_buf_avail);
	push((long long) proc_code);
	push((long long) proc_handl);
	push((long long) proto_buf);
	push((long long) proto_buf_avail);
	push((long long) proto_code);
	
	push(tcount);
	push(pcount);
	push_scope();								/* push scope before arg list	*/
	push(sym_count);							/* push symcount for use in end	*/

	num_uppers   = 0;							/* new temps for this proc		*/
	num_steps    = 0;
	max_uppers   = 0;
	max_steps    = 0;

	indent       = 0;							/* start all procs at outer lvl	*/
	args_to_scan = 0;
	arg_num      = 0;
	num_args     = 0;
	
	static_proc      = TRUE;					/* assume static, unless public	*/
	recurs_proc      = FALSE;					/* procs default non-recurs		*/
	header_needed    = TRUE;					/* header needed at some time	*/
	proc_attributes  = 0;						/* init to no attributes		*/
	void_proc        = TRUE;					/* assume returns void			*/
	return_type      = t_var;					/* assume returns fixed			*/
	return_name      = NULL;					/* and assume no name avail		*/
	
	scope_level++;								/* define args in next scope	*/
	

	/* Set up temporary output buffer for storing procedure code in.  Then we can pre-pend		*/
	/* the return type and emit the procedure body to the output file when we are done...		*/
	
	proc_handl = get_big_memory(OUT_BUF_LENGTH);
	
	if (proc_handl)
		{out_buf = proc_code = (char *) (*proc_handl);}
	else
		{out_buf = proc_code = NULL;}
	
	proto_buf = proto_code = (char *)malloc(PROTO_BUF_LENGTH);

	if (!out_buf || !proto_buf)
		{printf("xpltoc: could not get procedure definition buffer memory\n"); error_exit();}
		
	*out_buf   = 0;
	*proto_buf = 0;
	
	out_buf_avail   = OUT_BUF_LENGTH   - 5;
	proto_buf_avail = PROTO_BUF_LENGTH - 5;
	
	
	/* Scan off basic procedure header and argument list (sans type, of course!) */
	
	scan(); toss_non_essentials();				/* accept 'proc'; get next		*/
	
	if (token_type == t_lpar)					/* if arg list					*/
	{
		scan(); toss_non_essentials();			/* accept lpar; get first sym	*/
		
		while (1)								/* scan off arg					*/
		{
			if (!token_sym)						/* need symbol					*/
				{printf( "xpltoc: problem with argument list\n"); error_exit();}
			
			if (token_type != t_und)			/* if re-used symbol			*/
				token_sym  = insert_symbol(token, t_und, 0);
			
			token_sym->type = t_unda;			/* now is undefined argument	*/
			token_sym->info = num_args++;

			scan();								/* skip argument				*/
			toss_non_essentials();
			
			if (token_type == t_comma)			/* more args...					*/
			{
				scan();							/* accept comma; look for more	*/
				toss_non_essentials();
			}
			
			else								/* else better be paren			*/
				break;
		}

		if (token_type != t_rpar)		
			{printf( "xpltoc: problem with argument list\n"); error_exit();}
		
		if (num_args >= MAX_PROC_ARGS)
			{printf( "xpltoc: too many procedure arguments\n"); error_exit();}
		
		scan(); toss_non_essentials();			/* accept rpar					*/
	}
	
	while ((token_type == t_rtns     )
	||     (token_type == t_type     )
	||     (token_type == t_recurs   )
	||     (token_type == t_swap     )
	||     (token_type == t_swpcode  )
	||     (token_type == t_storage  )
	||     (token_type == t_lit      ))
	{
		if (token_type == t_lit)
		{
			symbol_struct *expanded_sym = NULL;
			
			if (*(char *)(token_sym->info) == 0)			/* dcl swapable lit '' */
			{
				scan();
				toss_non_essentials();
				continue;
			}
			
			expanded_sym = find_symbol((char *) (token_sym->info));
		
			if (expanded_sym)						/* if literal is a string that matches			*/
			{
				token_type = expanded_sym->type;
				token_info = expanded_sym->info;
			}
			
			if ((token_type != t_rtns     )
			&&  (token_type != t_type     )
			&&  (token_type != t_recurs   )
			&&  (token_type != t_swap     )
			&&  (token_type != t_swpcode  )
			&&  (token_type != t_storage  ))
				{printf( "xpltoc: unrecognized attribute for procedure (%s)\n", token_sym->symbol_name); error_exit();}
		}
	
		if (token_type == t_rtns)
		{
			scan();
			toss_non_essentials();
			
			if (token_type != t_lpar)
				{printf( "xpltoc: syntax error with 'returns' attribute\n"); error_exit();}
				
			scan();
			toss_non_essentials();
			
			map_ptr = token_sym;
			map_cnt = 0;
			while (token_type == t_lit && map_cnt < 10)	/* check for literal type definitions			*/
			{
				symbol_struct *expanded_sym = find_symbol((char *) (map_ptr->info));
			
				if (expanded_sym)						/* if literal is a string that matches			*/
				{
					token_type = expanded_sym->type;
					token_info = expanded_sym->info;
					map_ptr    = expanded_sym;
					map_cnt++;
				}
			}
	
			if (token_type != t_type)
				{printf( "xpltoc: syntax error with 'returns' type\n"); error_exit();}

			return_type = token_info;
			return_name = token_sym->symbol_name;

			scan();
			toss_non_essentials();
			
			if (token_type != t_rpar)
				{printf( "xpltoc: syntax error with 'returns' attribute (1)\n"); error_exit();}
		}
		
		else if (token_type == t_type)
		{
			return_type = token_info;
			return_name = token_sym->symbol_name;
		}
			
		else if (token_type == t_storage)
		{
			if (token_info == t_public)
				static_proc = FALSE;
			else
				{printf( "xpltoc: incompatible procedure argument (%s)\n", token_sym->symbol_name); error_exit();}
		}	
		
		else if (token_type == t_recurs)
		{
			recurs_proc = TRUE;
			proc_attributes |= attr_recursive;
		}
		
		else if (token_type == t_swap || token_type == t_swpcode)
			proc_attributes |= token_info;
		
		scan();
		toss_non_essentials();
	}

	if (token_type != t_semi)
		{printf( "xpltoc: missing semicolon after procedure definition (%d)\n", token_type); error_exit();}

	scan();										/* skip over semicolon; most likely white space or eol	*/
	toss_white_space();							/* to likely comment									*/
	
	
	/* Emit start of procedure definitions. Note we start emitting the the temp	*/
	/* buffer at this point:													*/
	
	tcount = ((tcount + indent) / tab_indent) * tab_indent;		/* simulate tab to indent we will do later	*/
	pcount = ((pcount + indent) / tab_indent) * tab_indent;

	tcount = ((tcount + ((16/tab_indent)*tab_indent)) / tab_indent) * tab_indent;			/* simulate text and tabs we will do later	*/
	pcount = ((pcount + ((16/tab_indent)*tab_indent)) / tab_indent) * tab_indent;

	put_symbol(sym_struct->symbol_name);		/* emit name					*/
	
	if (!static_proc)							/* public proc					*/
		put_proto (sym_struct->symbol_name);

	proc_com[0] = 0;							/* assume no comment			*/
	
	if (num_args == 0)
	{
		put_symbol("()");
		
		if (!static_proc)		/* emit prototype for public procs only			*/
			put_proto ("()");
		
		if (token_type == t_comment && strlen(token) < sizeof(proc_com)-4)
			strcpy(proc_com, token);
			
		emit_xpl_proc_info();
			
		if (header_needed)			/* guaranteed for non-arg case, however		*/
		{
			if (token_type == t_comment || token_type == t_eol)
			{
				process_stmt_comment(TRUE);	/* handle proc comment				*/
				tab(indent);
				put_symbol("{\n");
			}
			
			else					/* small proc on one line; leave as such	*/
				put_symbol(" {");
				
			put(3);					/* temporary declaration list will go here */
						
			header_needed = FALSE;
		}
	}
	
	else
	{
		put_symbol("(");
		
		if (!static_proc)		/* emit prototype for public procs only			*/
			put_proto ("(");
		
		if (token_type == t_comment && strlen(token) < sizeof(proc_com)-4)
			strcpy(proc_com, token);
			
		process_stmt_comment(TRUE);				/* handle proc comment			*/
		
		indent += tab_indent;                   /* indent for arg dcls			*/
		
		args_to_scan = num_args;				/* inform future dcl stmts		*/
		
		while (args_to_scan)					/* get their DCL statements		*/
			if (stmt() != t_unda)
				{printf( "xpltoc: incompatible argument declarations\n"); error_exit();}

		toss_blank_lines();						/* toss likely blank line		*/

		indent -= tab_indent;                   /* back out for start of proc	*/
		
		sym_struct->info  = num_args;			/* now store num args			*/
		sym_struct->other = store_proc_args(num_args, arg_types, return_type);
		
		if (header_needed)
		{
			tab(indent);
			put_symbol("{\n");
			
			put(3);								/* temporary declaration list will go here */
						
			header_needed = FALSE;
		}
	}
	
	if (!static_proc)							/* emit prototype for public procs only			*/
	{
		put_proto (";\t");
	
		if (proc_com[0])
		{
			expand_short_comment(proc_com, comment_len);
			convert_comment(proc_com);
			proto_tab(comment_pos);
			put_proto(proc_com);
		}
		
		put_proto ("\n");
	}
	
	proc_level += 1;
	indent     += tab_indent;
	
	lab_ptr = NULL;								/* done with label				*/
	
	end_push(t_procend);						/* no handling required			*/
	
	while (stmt() != t_end)						/* process till end				*/
		;
		
	/* Note: Arg list was stored above arg symbols, therefore it will be trashed		*/
	/* when we pop scope.  Copy arg list to temp array and re-insert it after we		*/
	/* pop scope.  This mechanism will handle recursive procedure calls and normal		*/
	/* procedure calls.																	*/
	
	{
		int i;
		
		for (i=0; i < num_args; i++)
			arg_types[i] = ((proc_struct *)(sym_struct->other))->arg_types[i];
		
		pop_scope();
		
		sym_struct->other = store_proc_args(num_args, arg_types, return_type);
	}
	
	
	/* If our scan for blank lines came across the next procedure name, the	*/
	/* current token will be a newly defined symbol that we have just			*/
	/* trashed by poping scope.  Reinsert it if that is the case...				*/
	
	if (token_type == t_und)
		token_sym = insert_symbol(token, token_type, token_info);
	
	
	/* Clean up after procedure definition: */
	
	stmt_type = 0;								/* no need to report t_proc		*/
	
	indent      -= tab_indent;
	proc_level  -= 1;
	
	
	/* Copy procedure body to the output file: */
	
	out_buf    = NULL;							/* output directly to file now			*/
	proto_buf  = NULL;
	
	pcount     = pop();							/* restore tab settings...				*/
	tcount     = pop();
	
	put_symbol(proc_hdr);						/* emit any comments preceding the proc	*/
	
	tab(indent);								/* tab to indent in actual output file	*/
	
	if (static_proc)							/* static proc							*/
	{
		if (void_proc)							/* static void							*/
			put_symbol("static\tvoid\t");
		
		else if (return_name)					/* static specified						*/
		{
			put_symbol("static\t");
			put_symbol(return_name);
			put_symbol("\t");
		}
		
		else									/* static fixed							*/
			put_symbol("static\tfixed\t");
	}
	
	else										/* public proc							*/
	{
		if (void_proc)
			put_symbol("void\t\t\t");			/* void									*/

		else if (return_name)					/* specified							*/
		{
			put_symbol(return_name);
			put_symbol("\t\t\t");
		}

		else
			put_symbol("fixed\t\t\t");			/* fixed								*/
	
		if (void_proc)
			put_proto ("extern\tvoid\t");		/* extern void							*/
		
		else if (return_name)					/* specified							*/
		{
			put_proto ("extern\t");
			put_proto(return_name);
			put_proto("\t");
		}

		else
			put_proto ("extern\tfixed\t");		/* extern fixed							*/
	}
	
	fabricate_temp_list();

	put_symbol(proc_code );						/* emit proc body to output file		*/
	put_proto (proto_code);						/* emit prototype to proto file			*/
	
	free_big_memory(proc_handl);
	free           (proto_code);
	proc_code = NULL;
	
	scope_level--;								/* return to prior scope				*/

	proto_code       = (char  *) pop();			/* restore possible nested buffer		*/
	proto_buf_avail  = (int)     pop();
	proto_buf        = (char  *) pop();
	proc_handl       = (void **) pop();
	proc_code        = (char  *) pop();
	out_buf_avail    = (int)     pop();
	out_buf          = (char  *) pop();
	
	return_name      = (char * ) pop();
	return_type      = pop();
	void_proc        = pop();
	proc_attributes  = pop();
	header_needed    = pop();
	recurs_proc      = pop();
	static_proc      = pop();

	arg_num          = pop();
	args_to_scan     = pop();
	indent           = pop();					/* restore indentation					*/

	max_steps        = pop();
	max_uppers       = pop();
	num_steps        = pop();
	num_uppers       = pop();
	
	proc_name_length = pop();
	
	proc_name[proc_name_length] = 0;			/* uncatenate our local procedure name	*/
	strcpy(nestor_name, proc_name);				/* restore outer proc name for next		*/
}


/*--------------------------------------------------------------------------------------*/
/*	Statement Scanner - When statement													*/
/*--------------------------------------------------------------------------------------*/

/* when break then call exit(-1); */

static	void	handle_when()
{
	char		when_com [1024]        = {0};
	char		when_hdr [TEMP_LENGTH] = {0};
	boolean		do_scanned = FALSE;

	
	/* Begin by pushing the comments and stuff that preceded the when definition.		*/
	/* We will emit these at the right time later...									*/
	
	strcpy(when_hdr, temp_code);				/* stash preceding comment(s)			*/
	temp_code[0] = 0;							/* none to write on pop					*/
	pop_code();									/* restore output pointers				*/
	
	scan();										/* accept 'when'						*/
	toss_non_essentials();						/* get to good stuff					*/
	
	if (token_type != t_icell)
		{printf("xpltoc: missing interrupt specifier after 'when'\n"); error_exit();}
		
	push(proc_name_length);						/* save length of current proc name		*/
	
	strcpy(nestor_name, proc_name);				/* save outer proc name					*/
	strcpy(nestee_name, "when_");				/* create proc name "when_break"		*/
	strcat(nestee_name, token);
	
	scan();										/* accept interrupt name				*/
	toss_non_essentials();						/* get to good stuff					*/
	
	if (token_type != t_then)
		{printf( "xpltoc: missing 'then' after 'when'\n"); error_exit();}
	
	scan();										/* accept 'then'						*/
	toss_non_essentials();


	/* Handle comment intelligently if a complex when: */
	
	if (((token_type == t_stmt)					/* if is 'do' or 'begin'		*/
	&&   (token_info == s_begin || token_info == s_do))
	||   (token_type == t_lbra)
	||   (token_type == t_while))
	{
		do_scanned = TRUE;
	
		scan();									/* accept do or begin					*/
		toss_non_essentials();
		
		if (token_type != t_semi)
			{printf( "xpltoc: missing semicolon after 'do' or 'begin'\n"); error_exit();}
			
		scan();											/* skip semi					*/
		toss_white_space();								/* to likely comment			*/
	}
	
	
	/* construct procedure name "when_xxxx": */
	
	if (proc_name_length + strlen(nestee_name) >= sizeof(proc_name) - 1)
		{printf("xpltoc: nested procedure name too long\n"); error_exit();}
	
	if (proc_name_length)						/* separate names with _				*/
		strcat (proc_name, "_");
	
	strcat (proc_name, nestee_name);
	proc_name_length = strlen(proc_name);
	
	if (proc_level)								/* if this is a nested proc		*/
		{printf("xpltoc: canot handle nested when\n"); error_exit();}
	
	
	/* Push global variables here so we can handle XPL's nested procedure		*/
	/* definitions.																*/
	
	push(num_uppers);
	push(num_steps);
	push(max_uppers);
	push(max_steps);
	
	push(indent);
	push(args_to_scan);							/* at this point, proc is		*/
	push(arg_num);								/* defined with 0 args			*/
	
	push(static_proc);
	push(recurs_proc);
	push(header_needed);
	push(proc_attributes);
	push(void_proc);
	push(return_type);
	push((long long) return_name);
	
	push((long long) out_buf);
	push((long long) out_buf_avail);
	push((long long) proc_code);
	push((long long) proc_handl);
	push((long long) proto_buf);
	push((long long) proto_buf_avail);
	push((long long) proto_code);
	
	push(tcount);
	push(pcount);
	push_scope();								/* push scope before arg list	*/

	num_uppers   = 0;							/* new temps for this proc		*/
	num_steps    = 0;
	max_uppers   = 0;
	max_steps    = 0;

	indent       = 0;							/* start all procs at outer lvl	*/
	args_to_scan = 0;
	arg_num      = 0;
	
	static_proc      = TRUE;					/* assume static, unless public	*/
	recurs_proc      = FALSE;					/* procs default non-recurs		*/
	header_needed    = TRUE;					/* header needed at some time	*/
	proc_attributes  = 0;						/* init to no attributes		*/
	void_proc        = TRUE;					/* assume returns void			*/
	return_type      = t_var;					/* assume returns fixed			*/
	return_name      = NULL;					/* and assume no name avail		*/
	
	scope_level++;								/* define args in next scope	*/
	
	
	/* Set up temporary output buffer for storing when code in.  Then we can pre-pend		*/
	/* the return type and emit the when body to the output file when we are done...		*/
	
	proc_handl = get_big_memory(OUT_BUF_LENGTH);
	
	if (proc_handl)
		{out_buf = proc_code = (char *) (*proc_handl);}
	else
		{out_buf = proc_code = NULL;}

	proto_buf = proto_code = (char *)malloc(PROTO_BUF_LENGTH);

	if (!out_buf || !proto_buf)
		{printf("xpltoc: could not get when definition buffer memory\n"); error_exit();}
		
	*out_buf   = 0;
	*proto_buf = 0;
	
	out_buf_avail   = OUT_BUF_LENGTH   - 5;
	proto_buf_avail = PROTO_BUF_LENGTH - 5;
	
		
	/* Emit start of when definitions. Note we start emitting to the temp	*/
	/* buffer at this point:												*/
	
	tcount = ((tcount + indent) / tab_indent) * tab_indent;                         /* simulate tab to indent we will do later	*/
	pcount = ((pcount + indent) / tab_indent) * tab_indent;

	tcount = ((tcount + ((16/tab_indent)*tab_indent)) / tab_indent) * tab_indent;       /* simulate text and tabs we will do later	*/
	pcount = ((pcount + ((16/tab_indent)*tab_indent)) / tab_indent) * tab_indent;

	put_symbol(proc_name);						/* emit name					*/
	put_proto (proc_name);

	when_com[0] = 0;							/* assume no comment			*/
	
	put_symbol("()");
	put_proto ("()");
	
	if (do_scanned)								/* found do or begin			*/
	{
		if (token_type == t_comment && strlen(token) < sizeof(when_com)-4)
			strcpy(when_com, token);
			
		process_stmt_comment(TRUE);				/* handle proc comment			*/
	}
	
	else
		put_symbol("\n");						/* else synth line break		*/
	
	if (header_needed)
	{
		tab(indent);
		put_symbol("{\n");
		
		put(3);									/* temporary declaration list will go here 		*/
					
		header_needed = FALSE;
	}
	
	put_proto (";\t");

	if (when_com[0])
	{
		expand_short_comment(when_com, comment_len);
		convert_comment(when_com);
		proto_tab(comment_pos);
		put_proto(when_com);
	}
	
	put_proto ("\n");
	
	proc_level += 1;
	indent     += tab_indent;
	
	if (do_scanned)
	{
		end_push(t_nop);						/* no handling required			*/
		
		while (stmt() != t_end)					/* process till end				*/
			;
	}
	
	else										/* get single when statement	*/
	{
		stmt();
		
		tab(indent - tab_indent);
		put_symbol("}\n");
	}	

	pop_scope();
	
	
	/* If our scan for blank lines came across the next procedure name, the	*/
	/* current token will be a newly defined symbol that we have just			*/
	/* trashed by poping scope.  Reinsert it if that is the case...				*/
	
	if (token_type == t_und)
		token_sym = insert_symbol(token, token_type, token_info);
	
	
	/* Clean up after when definition: */
	
	stmt_type = 0;								/* no need to report t_proc/t_end		*/
	
	indent      -= tab_indent;
	proc_level  -= 1;
	
	
	/* Copy when body to the output file: */
	
	out_buf    = NULL;							/* output directly to file now			*/
	proto_buf  = NULL;
	
	pcount     = pop();							/* restore tab settings...				*/
	tcount     = pop();
	
	put_symbol(when_hdr);						/* emit any comments preceding the when	*/
	
	tab(indent);								/* tab to indent in actual output file	*/
	
	if (void_proc)
		put_symbol("void\t\t\t");				/* void									*/

	else if (return_name)						/* specified							*/
	{
		put_symbol(return_name);
		put_symbol("\t\t\t");
	}

	else
		put_symbol("fixed\t\t\t");				/* fixed								*/

	if (void_proc)
		put_proto ("extern\tvoid\t");			/* extern void							*/
	
	else if (return_name)						/* specified							*/
	{
		put_proto ("extern\t");
		put_proto(return_name);
		put_proto("\t");
	}

	else
		put_proto ("extern\tfixed\t");		/* extern fixed							*/
	
	fabricate_temp_list();

	put_symbol(proc_code );						/* emit proc body to output file		*/
	put_proto (proto_code);						/* emit prototype to proto file			*/
	
	free_big_memory(proc_handl);
	free           (proto_code);
	proc_code = NULL;
	
	scope_level--;								/* return to prior scope				*/

	proto_code       = (char  *) pop();			/* restore possible nested buffer		*/
	proto_buf_avail  = (int)     pop();
	proto_buf        = (char  *) pop();
	proc_handl       = (void **) pop();
	proc_code        = (char  *) pop();
	out_buf_avail    = (int)     pop();
	out_buf          = (char  *) pop();
	
	return_name      = (char  *) pop();
	return_type      = pop();
	void_proc        = pop();
	proc_attributes  = pop();
	header_needed    = pop();
	recurs_proc      = pop();
	static_proc      = pop();

	arg_num          = pop();
	args_to_scan     = pop();
	indent           = pop();					/* restore indentation					*/

	max_steps        = pop();
	max_uppers       = pop();
	num_steps        = pop();
	num_uppers       = pop();
	
	proc_name_length = pop();
	
	proc_name[proc_name_length] = 0;			/* uncatenate our local procedure name	*/
	strcpy(nestor_name, proc_name);				/* restore outer proc name for next		*/
}


/*--------------------------------------------------------------------------------------*/
/*	Statement Scanner - PDL, RAM, CONFIG, LIBRARY										*/
/*--------------------------------------------------------------------------------------*/

static	void	handle_pdl()
{
	node *tree = NULL;
	
	scan();												/* accept 'pdl'					*/
	toss_non_essentials();								/* get to likely food			*/
	tree = expr();										/* get expression				*/
	
	if (token_type != t_semi)
		{printf( "xpltoc: missing semicolon after 'pdl' '%c'\n", next); error_exit();}
		
	scan();												/* skip semi					*/
	toss_white_space();									/* to likely comment			*/

	tab(indent);

	put_symbol("_pdl");
	
	compute_enclosed_expression(tree);					/* emit in parens				*/
	
	process_stmt_comment(TRUE);
}

static	void	handle_ram()
{
	node *tree = NULL;
	
	scan();												/* accept 'ram'					*/
	toss_non_essentials();								/* get to likely food			*/
	tree = expr();										/* get expression				*/
	
	if (token_type != t_semi)
		{printf( "xpltoc: missing semicolon after 'ram' '%c'\n", next); error_exit();}
		
	scan();												/* skip semi					*/
	toss_white_space();									/* to likely comment			*/

	tab(indent);

	put_symbol("_ram");
	
	compute_enclosed_expression(tree);					/* emit in parens				*/
	
	process_stmt_comment(TRUE);
}

static	void	handle_config()
{
	node *tree = NULL;
	
	scan();												/* accept 'configuration'		*/
	toss_non_essentials();								/* get to likely food			*/

	tab(indent);
	put_symbol("_configuration(\"");
	
	while (token_type == t_config)
	{
		put_symbol(token_sym->symbol_name);				/* emig name					*/
		
		if (token_info == 7)							/* handle memory size expr		*/
		{
			scan();										/* accept memsiz				*/
			toss_non_essentials();						/* get to likely food			*/
			
			tree = expr();								/* get expression				*/
			
			compute_enclosed_expression(tree);			/* emit in parens				*/
				
			tree = NULL;
		}

		else											/* others have no args			*/
		{
			scan();										/* accept memsiz				*/
			toss_non_essentials();						/* get to likely food			*/
		}
		
		if (token_type == t_comma)
		{
			put_symbol(", ");							/* emit separator				*/
			scan();										/* accept memsiz				*/
			toss_non_essentials();						/* get to likely food			*/
		}
	}
		
	if (token_type != t_semi)
		{printf( "xpltoc: missing semicolon after 'configuration' '%c'\n", next); error_exit();}
		
	scan();												/* skip semi					*/
	toss_white_space();									/* to likely comment			*/

	put_symbol("\")");

	process_stmt_comment(TRUE);
}

static	void	handle_library()
{
	scan();												/* accept 'library'				*/
	toss_non_essentials();								/* get to likely food			*/

	if (token_type != t_sconst)
		{printf( "xpltoc: syntax error after 'library'\n"); error_exit();}
	
	tab(indent);

	put_symbol("_library(");
	
	emit_c_string(token);
	
	put_symbol(")");
	
	scan();												/* accept lib namd				*/
	toss_non_essentials();								/* get to likely food			*/

	if (token_type != t_semi)
		{printf( "xpltoc: missing semicolon after 'library' '%c'\n", next); error_exit();}
		
	scan();												/* skip semi					*/
	toss_white_space();									/* to likely comment			*/

	process_stmt_comment(TRUE);
}

static	void	handle_enter()
{
	scan();												/* accept 'enter'				*/
	toss_non_essentials();								/* get to likely food			*/

	if (token_type != t_sconst)
		{printf( "xpltoc: syntax error after 'enter'\n"); error_exit();}
	
	tab(indent);

	put_symbol("_enter(");
	
	emit_c_string(token);
	
	put_symbol(")");
	
	scan();												/* accept cc namd				*/
	toss_non_essentials();								/* get to likely food			*/

	if (token_type != t_semi)
		{printf( "xpltoc: missing semicolon after 'library' '%c'\n", next); error_exit();}
		
	scan();												/* skip semi					*/
	toss_white_space();									/* to likely comment			*/

	process_stmt_comment(TRUE);
}


/*--------------------------------------------------------------------------------------*/
/*	Output control																		*/
/*--------------------------------------------------------------------------------------*/

/* XPL allowed main code to be dispersed between between procedure definitions, and		*/
/* also allowed local (e.g. nested) procedure definitions.  C does not.  To work around	*/
/* these issues, the translater moves all non-procedure code to a new synthesized		*/
/* procedure called 'main' which is emitted at the end of the output file.				*/
/* Additionally, nested procedures are moved to the outer level.						*/

/* The following contortions are used to keep comments with their associated code		*/
/* (at least as best we can!) and to move statements to where they need to be...		*/

char *prior_buf;  								/* handy globals to keep track of		*/
int	  prior_avail;								/* where code is going					*/
int   prior_count;

char *main_prior_buf;
int	  main_prior_avail;
int   main_prior_count;

int   main_level = 0;

static	void	push_code()						/* handy routine to save current		*/
{												/* output pointers and set up to emit	*/
	prior_buf     = out_buf;					/* code to temp area					*/
	prior_avail   = out_buf_avail;
	prior_count   = tcount;
	
	out_buf       = temp_code;					/* emit to start of temp area			*/
	out_buf_avail = TEMP_LENGTH;				/* leave tcount where it is...			*/
	temp_code[0]  = 0;
}

static	void	pop_code()						/* return from temp_code to out_buf		*/
{
	out_buf       = prior_buf;					/* restore output pointer				*/
	out_buf_avail = prior_avail;				/* restore room left					*/
	tcount        = prior_count;				/* and tab position						*/
	
	put_symbol(temp_code);						/* emit temp_code, if any				*/
	temp_code[0] = 0;							/* and done with temp code				*/
}

static	void	set_main()						/* switch from temp_code to main_code	*/
{
 	if (main_level++ == 0)						/* if outer most statement, then		*/
	{											/* switch to main buffer				*/
		out_buf       = main_buf;				/* set up for main code output			*/
		out_buf_avail = main_avail;				/* restore room left					*/
		tcount        = main_count;				/* and tab position						*/
		
		main_prior_buf   = prior_buf;			/* save what we were sending to before	*/
		main_prior_avail = prior_avail;			/* we pushed...							*/
		main_prior_count = prior_count;
		
		indent += tab_indent;                   /* indent main code						*/
	}
	
	else										/* else just pop code to get main back	*/
	{
		out_buf       = prior_buf;				/* restore output pointer				*/
		out_buf_avail = prior_avail;			/* restore room left					*/
		tcount        = prior_count;			/* and tab position						*/
	}
	
	put_symbol_with_indent(temp_code);			/* emit temp_code to main				*/
	temp_code[0] = 0;							/* and done with temp code				*/
}

static	void	pop_main()						/* switch from main_code back to outbuf	*/
{
	if (--main_level)							/* leave in main until 'end'			*/
		return;
		
	main_buf      = out_buf;					/* save current working pointers		*/
	main_avail    = out_buf_avail;				/* restore room left					*/
	main_count    = tcount;						/* and tab position						*/
	
	out_buf       = main_prior_buf;				/* restore output pointer				*/
	out_buf_avail = main_prior_avail;			/* restore room left					*/
	tcount        = main_prior_count;			/* and tab position						*/

	indent -= tab_indent;
}


/*--------------------------------------------------------------------------------------*/
/*	Statement Scanner Driver															*/
/*--------------------------------------------------------------------------------------*/

static	int	stmt()
{
	symbol_struct *map_ptr;
	int            map_cnt;
	
	stmt_type = 0;										/* init to null return type		*/
	
	push_code();										/* push comments to temp		*/

	while (1)											/* process non essentials		*/
	{													/* until statement body is		*/
		if (break_received)								/* encountered					*/
			exit(-9);
	
		/* ignore white space until start of statement	*/
		if (token_type == t_white)
			{scan(); continue;}
		
		/* but preserve blank lines						*/	
		if (token_type == t_eol)
			{tab(indent); put_symbol(token); scan(); continue;}
		
		/* check for comment preceding statement		*/
		if (token_type == t_comment)
		{
			if (tcount || indent_comments)
				tab(indent);
	
			put_symbol(token);				/* emit comment								*/
			scan();							/* get next token (likely eol)				*/
			continue;
		}
		

		/* handle special cases: */

		if (token_type == t_lbra)			/* accept { for simple begin				*/
			{token_type = t_stmt; token_info = s_begin;}
				
		if (token_type == t_opr && token_info == o_core)
		{
			token_type = t_stmt;			/* map core to statement for core(x) =...	*/
			token_info = s_core;
		}
		
		if (token_type == t_eof)			/* handle end of file encountered			*/
		{
			if (insert_stack_ptr)			/* if end of insert file, pop original		*/
			{								/* source file info & continue				*/
				int i;
				
				
				/* clean up insert file processing: */
				
				if (in_file)				/* close the insert file					*/
					{fclose(in_file); in_file  = NULL;}

				if (convert_includes && !create_one_file)		/* close output file if we were creating	*/
				{
					if (out_file)    {fflush(out_file  ); fclose(out_file  ); out_file   = NULL;}
					if (proto_file)  {fflush(proto_file); fclose(proto_file); proto_file = NULL;}
				}
				
				
				/* pop prior error message file name : */
				
				i = insert_pop();			/* get length of original error name		*/
		
				if (i >= sizeof(error_file_name))
					{printf( "xpltoc: system error with 'insert'\n"); error_exit();}
		
				error_file_name[i] = 0;
				while (i)
					error_file_name[--i] = (char) insert_pop();
				
				
				/* pop original source file name: */
				
				i = insert_pop();			/* get length of original file name			*/
		
				if (i >= sizeof(source_file_name))
					{printf( "xpltoc: system error with 'insert'\n"); error_exit();}
		
				source_file_name[i] = 0;
				while (i)
					source_file_name[--i] = (char) insert_pop();
				
				
				/* pop original output file if we had pushed it: */
				
				if (convert_includes && !create_one_file)
				{
					proto_file = (FILE *) insert_pop();
					out_file   = (FILE *) insert_pop();
				}
				
				in_file  = (FILE *) insert_pop();
				
				next_count = insert_pop();
				lcount     = insert_pop();
				next_info  = insert_pop();
				next       = insert_pop();
		
				scan();						/* now scan next token of source file		*/
				
				mute_output = (insert_stack_ptr && !convert_includes && !create_one_file);
				
				continue;					/* and process it as a statement			*/
			}
			
			pop_code();						/* back to current output					*/
			stmt_type = t_end;				/* caller might want to know...				*/
			break;							/* treat as end to avoid hanging...			*/
		}
		

		/* Handle assorted statements */
		
		if ((token_type == t_proc)			/* for proc name or rtp, scan ahead			*/
		||  (token_type == t_rtp ))			/* to look for (							*/
		{
			while (next_info & (b_spa | b_eol))
				get();
		}
		
		if ((token_type == t_stmt )
		||  (token_type == t_while)
		||  ((token_type == t_proc ) && (next == '('))
		||  ((token_type == t_rtp  ) && (next == '(')))
		{
			int	which_case;
		
			/* Create header in output file immediately before	*/
			/* first actual source statement:					*/
			
			if (insert_needed)								/* emit #include			*/
			{												/* of header file			*/
				char temp_str[512] = {0};
				
				pop_code();									/* had best be to file!		*/
				
				insert_needed = FALSE;						/* just before first stmt	*/
				
                CFStringRef dateString = CFDateFormatterCreateStringWithAbsoluteTime(NULL, date_formater, CFAbsoluteTimeGetCurrent());
                char now[512] = {0};
                
                CFStringGetCString(dateString, now,  sizeof(now), kCFStringEncodingUTF8);
                
				sprintf(temp_str, "/*\tTranslated to C:   \t%s\t*/\n", now);
				put_symbol(temp_str);
			
				sprintf(temp_str, "/*\tTranslator Version:\t0.000\t\t\t\t*/\n");
				put_symbol(temp_str);
			
				put_symbol("\n");
				
				put_symbol("#include\t\"XPL.h\"\n");
                
                if (!is_header) {
                    put_symbol("#include\t\"");
                    put_symbol(source_file_name);
                    put_symbol(".h\"\n");
                }
				
                put_symbol("\n");

                push_code();					/* set up for temp again				*/
			}
			
			
			/* branch on statement type: */
			
			which_case = token_info;
			
			if (token_type == t_while)			/* map while to a do statement			*/
				which_case = s_do;

			else if (((token_type == t_proc ) && (next == '('))
			||       ((token_type == t_rtp  ) && (next == '(')))
				which_case = s_call;
				
			switch (which_case)
			{
				case s_call:							/* call statement - executable		*/
					check_sharp_if_level();				/* make sure not in outer #if		*/
				
					if (proc_level == 0) set_main();	/* emit to main if main level code	*/
					else                 pop_code();	/* else emit to current level		*/
					
					if (token_type == t_stmt)			/* if 'call'						*/
					{
						scan();							/* accept 'call'					*/
						toss_non_essentials();			/* get to likely food				*/
					}
	
					handle_call();
					
					if (proc_level == 0) pop_main();	/* pop from main now				*/
					
					break;

				case s_return:							/* return statement - executable	*/
					check_sharp_if_level();				/* make sure not in outer #if		*/

					if (proc_level == 0) set_main();	/* emit to main if main level code	*/
					else                 pop_code();	/* else emit to current level		*/
					
					handle_return();

					if (proc_level == 0) pop_main();	/* pop from main now				*/
					
					break;
					
				case s_do:								/* do statement						*/
					check_sharp_if_level();				/* make sure not in outer #if		*/
					
					if (proc_level == 0) set_main();	/* emit to main if main level code	*/
					else                 pop_code();	/* else emit to current level		*/
					
					handle_do();

					if (proc_level == 0) pop_main();	/* pop from main now				*/
					
					break;
					
				case s_begin:							/* begin statement					*/
					check_sharp_if_level();				/* make sure not in outer #if		*/
					
					if (proc_level == 0) set_main();	/* emit to main if main level code	*/
					else                 pop_code();	/* else emit to current level		*/
					
					handle_begin();

					if (proc_level == 0) pop_main();	/* pop from main now				*/
					
					break;
					
				case s_if:								/* if statement - executable		*/
					check_sharp_if_level();				/* make sure not in outer #if		*/
					
					if (proc_level == 0) set_main();	/* emit to main if main level code	*/
					else                 pop_code();	/* else emit to current level		*/
					
					handle_if();

					if (proc_level == 0) pop_main();	/* pop from main now				*/
					
					break;
				
				case s_goto:							/* goto statement - executable		*/
					check_sharp_if_level();				/* make sure not in outer #if		*/
					
					if (proc_level == 0) set_main();	/* emit to main if main level code	*/
					else                 pop_code();	/* else emit to current level		*/
					
					handle_goto();

					if (proc_level == 0) pop_main();	/* pop from main now				*/
					
					break;
				
				case s_enable:							/* enable statement - executable	*/
					check_sharp_if_level();				/* make sure not in outer #if		*/
					
					if (proc_level == 0) set_main();	/* emit to main if main level code	*/
					else                 pop_code();	/* else emit to current level		*/
					
					handle_enable();

					if (proc_level == 0) pop_main();	/* pop from main now				*/
					
					break;
				
				case s_disable:							/* disable statement - executable	*/
					check_sharp_if_level();				/* make sure not in outer #if		*/
					
					if (proc_level == 0) set_main();	/* emit to main if main level code	*/
					else                 pop_code();	/* else emit to current level		*/
					
					handle_disable();

					if (proc_level == 0) pop_main();	/* pop from main now				*/
					
					break;

				case s_write:							/* write statement - executable		*/
					check_sharp_if_level();				/* make sure not in outer #if		*/
					
					if (proc_level == 0) set_main();	/* emit to main if main level code	*/
					else                 pop_code();	/* else emit to current level		*/
					
					handle_write();

					if (proc_level == 0) pop_main();	/* pop from main now				*/
					
					break;

				case s_core:							/* core statement - executable		*/
					check_sharp_if_level();				/* make sure not in outer #if		*/
					
					if (proc_level == 0) set_main();	/* emit to main if main level code	*/
					else                 pop_code();	/* else emit to current level		*/
					
					handle_core();

					if (proc_level == 0) pop_main();	/* pop from main now				*/
					
					break;

				case s_linput:							/* linput statement - executable		*/
					check_sharp_if_level();				/* make sure not in outer #if		*/
					
					if (proc_level == 0) set_main();	/* emit to main if main level code	*/
					else                 pop_code();	/* else emit to current level		*/
					
					handle_linput();

					if (proc_level == 0) pop_main();	/* pop from main now				*/
					
					break;

				case s_print:							/* print statement - executable		*/
					check_sharp_if_level();				/* make sure not in outer #if		*/
					
					if (proc_level == 0) set_main();	/* emit to main if main level code	*/
					else                 pop_code();	/* else emit to current level		*/
					
					handle_print();

					if (proc_level == 0) pop_main();	/* pop from main now				*/
					
					break;

				case s_when:							/* when statement - non-executable	*/
					handle_when();
					break;

				case s_library:							/* library - non-executable			*/
					pop_code();
					handle_library();
					break;
				
				case s_module:							/* module - non-executable			*/
					pop_code();
					handle_module();
					break;
				
				case s_enter:							/* enter - non-executable			*/
					pop_code();
					handle_enter();
					break;
				
				case s_insert:							/* insert - non-executable			*/
					pop_code();
					handle_insert();
					break;
				
				case s_declare:
				{
					boolean	static_dcl;			/* true to emit 'static' attribute				*/
					
					pop_code();					/* non executable...							*/
					
					scan();						/* accept dcl; get next							*/
					toss_non_essentials();		/* skip to label (list)							*/

					if (proc_level)				/* if inside procedure							*/
					{
						if (recurs_proc)		/* recursive proc: let vars on stk				*/
							static_dcl = FALSE;	
						else					/* else with XPL, all variables were static		*/
							static_dcl = TRUE;	/* unless explicitly declared automatic, or		*/
					}							/* defined within a recursive procedure			*/
					
					else						/* else for globals, emit static attribute		*/
						static_dcl = TRUE;		/* unless declared public						*/
						
					if (token_type == t_lpar)	/* toss ( if xpl syntax							*/
					{
						scan();
						toss_non_essentials();
					}
	
					while (handle_declare(NULL, t_var, FALSE, static_dcl, FALSE))
					{
						if (token_type == t_lpar)
						{
							scan();
							toss_non_essentials();
						}
					}

					break;
				}

				case s_native:							/* native statement - non executable		*/
				{
					node *tree = NULL;
			
					pop_code();							/* non executable...						*/
					
					scan();								/* accept native; get next						*/
					toss_non_essentials();

					tree = scanarg();					/* get native code							*/
					toss_expression(tree);
					
					toss_non_essentials();
	
					while (handle_declare(NULL, t_proc, TRUE, TRUE, TRUE))
					{
						if (token_type == t_lpar)
						{
							scan();
							toss_non_essentials();
						}
					}

					break;
				}
				
				case s_proc:
					handle_procedure();
					break;
					
				case s_pdl:
					pop_code();
					handle_pdl();
					break;
					
				case s_ram:
					pop_code();
					handle_ram();
					break;
					
				case s_config:
					pop_code();
					handle_config();
					break;
				
				case s_sharp_if:						/* #if statement					*/
					pop_code();
					handle_sharp_if();
					break;
				
				case s_sharp_elseif:					/* #elseif statement				*/
					pop_code();
					handle_sharp_elseif();
					break;
				
				case s_sharp_else:						/* #else statement					*/
					pop_code();
					handle_sharp_else();
					break;
				
				case s_sharp_endif:						/* #endif statement					*/
					pop_code();
					handle_sharp_endif();
					break;
				
				default:
					pop_code();
					printf( "xpltoc: unimplemented statement %s\n", token);
					error_exit();
					break;
			}
			
			break;							/* after 1 statement						*/
		}
		
		else if (token_type == t_semi)		/* semicolon is null statement				*/
		{
			pop_code();						/* semi is non-executable at outer level	*/
			
			scan();							/* accept semi								*/
			toss_white_space();				/* to likely comment						*/

			tab(indent);					/* emit null statement						*/
			put_symbol(";");
	
			process_stmt_comment(TRUE);		/* process comment							*/

			stmt_type = 0;
			
			break;							/* got one statement						*/
		}
		
		else if (token_type == t_end)		/* got an 'end'								*/
		{
			int end_type;
			
			pop_code();						/* non executable...						*/
			
			if (end_stack_ptr == 0)
				{printf("xpltoc: too many 'end' statements (1)\n"); error_exit();}
				
			end_type = end_pop();			/* get end type we are processing			*/
				
			stmt_type = t_end;				/* caller might want to know...				*/
		
			if (token[0] == '}')			/* if end was a brace,						*/
				{token_type = t_semi; token_info = 0;}
			else
				{scan(); toss_non_essentials();}
			
			if ((token_type == t_proc  )	/* might be end xyz...						*/
			||  (token_type == t_module))
				{scan(); toss_non_essentials();}
			
			if (token_type != t_semi)
				{printf( "xpltoc: missing semicolon\n"); error_exit();}

			scan();							/* skip over semicolon; most likely white space or eol	*/
			toss_white_space();				/* to likely comment									*/

			if (end_type == t_break)		/* emit break 											*/
			{
				tab(indent);
				put_symbol("break;\n");
			}
			
			if (end_type == t_procend)		/* emit #undef for local literals						*/
			{
				int i = pop();				/* get starting symcount 								*/
				
				while (i < sym_count)
				{
					symbol_struct* it = symlist[i++];
					
					if ((it->type == t_lit || it->type == t_remapsymb)
					&&  (it->scope == scope_level))
					{
						tab(indent);
						put_symbol("#undef\t");
						put_symbol(it->symbol_name);
						put_symbol("\n");
					}
				}
			}
			
			if (indent > 0)
			{
				tab(indent - tab_indent);
				put_symbol("}");
			}
			
			else							/* skip end to module statement				*/
				tab(indent - tab_indent);
	
			process_stmt_comment(TRUE);		/* process end statement comment			*/
			
			break;							/* got one statement						*/
		}
		

		/* Check for special C-compatiable XPL statements:								*/
		
		
		/* ### for complete c analysis --> need to check for literal type or token!		*/
		
		else if ((token_type == t_type   )	/* look for static, extern, fixed...		*/
		||	     (token_type == t_storage))
		{
			boolean	extern_dcl = FALSE;
			boolean static_dcl = FALSE;
			int     var_type   = t_var;
			char	*type_name = NULL;
			
			pop_code();						/* non executable...						*/
			
			while (token_type == t_type || token_type == t_storage)
			{
				if (token_type == t_type)
				{
					var_type = token_info;	/* e.g. fixed, pointer, boolean				*/
					type_name = token_sym->symbol_name;
				}
				
				else if (token_info == t_extern)
					extern_dcl = TRUE;
				
				else if (token_info == s_static)
					static_dcl = TRUE;
				
				else
					{printf( "xpltoc: incorrect syntax in variable declaration\n"); error_exit();}
			
				scan();
				toss_non_essentials();
			}
			
			handle_declare(type_name, var_type, extern_dcl, static_dcl, FALSE);
			
			break;							/* got one statement						*/
		}
			
		else if (token_sym)					/* not a semi; not a stmt; is symbol...		*/
		{
			char	*string1 = NULL;
			int		lab_type = token_type;
			int		lab_info = token_info;
			
			if (args_to_scan)
				{printf( "xpltoc: symbol '%s' not allowed during procedure argument declarations\n", token_sym->symbol_name); error_exit();}
			
			lab_ptr = token_sym;			/* save pointer								*/
			
			scan();							/* accept new label or variable name		*/
			string1 = scan_space_and_comment();
			
			if (token_type == t_colon)		/* symbol: look for proc or label			*/
			{
				char	*colon_string = NULL;
				
				if (lab_ptr->type != t_und)	/* detect re-use of label in local scope	*/
					lab_ptr = insert_symbol(lab_ptr->symbol_name, t_und, 0);

				lab_ptr->type = t_label;	/* type is label (for now...)				*/

				scan();						/* accept colon								*/
				colon_string = scan_space_and_comment();
				
				if (token_type == t_stmt && token_info == s_proc)
				{
					if (string1)			/* toss incompatible white space here		*/
						{free(string1); string1 = NULL;}
				
					if (colon_string)		/* toss incompatible white space here		*/
						{free(colon_string); colon_string = NULL;}
				
					continue;				/* handle procedure statement directly		*/
				}
				
				check_sharp_if_level();				/* make sure not in outer #if		*/
				
				if (proc_level == 0) set_main();	/* emit to main if main level code	*/
				else                 pop_code();	/* else emit to current level		*/
				
				tab(indent);
				
				put_symbol(lab_ptr->symbol_name);
			
				if (string1)
					put_symbol(string1);
					
				put_symbol(":");

				if (colon_string)
					put_symbol(colon_string);
					
				if (proc_level == 0) pop_main();	/* pop from main now				*/
				
				if (string1)
					{free(string1); string1 = NULL;}
			
				if (colon_string)
					{free(colon_string); colon_string = NULL;}
			
				push_code();				/* want to try label:dcl x fixed???			*/
				
				continue;
			}

			map_ptr = lab_ptr;
			map_cnt = 0;
			while (lab_type == t_lit && map_cnt < 10)	/* check for literal type definitions			*/
			{
				symbol_struct *expanded_sym = find_symbol((char *) (map_ptr->info));
			
				if (expanded_sym)						/* if literal is a string that matches			*/
				{
					lab_type   = expanded_sym->type;
					lab_info   = expanded_sym->info;
					map_ptr    = expanded_sym;
					map_cnt++;
				}
				
				else if (token_type == t_opr && token_info == o_eq)
					lab_type = t_var;		/* else assume lit can be assigned to...		*/
			}
				
			if (lab_type == t_var			/* variable at this point -> handle assignment	*/
			||  lab_type == t_pointer
			||  lab_type == t_var + t_arr
			||  lab_type == t_pointer + t_arr
			||  lab_type == t_cstring)
			{
				char *string2 = NULL;
				node *subscr  = NULL;
				node *arg     = NULL;
				
				if (lab_ptr->scope && lab_ptr->scope < scope_level)
				{
					printf("xpltoc: warning: incompatible access to %s\n", lab_ptr->symbol_name);
					put_symbol("   (#### xpltoc: warning: incompatible access to  ");
					put_symbol(lab_ptr->symbol_name);
					put_symbol(" ####)    ");
					error_log();
				}
			
				check_sharp_if_level();					/* make sure not in outer #if		*/
				
				if (proc_level == 0) set_main();		/* emit to main if main level code	*/
				else                 pop_code();		/* else emit to current level		*/
					
				if (lab_type & t_arr)		/* best be subscript if is array				*/
				{
					if ((token_type != t_lpar) 			/* make sure paren exists			*/
					&&  (token_type != t_lbr ))			/* or allow brackets too...			*/
						{printf( "xpltoc: missing or incorrect format in subscript for '%s'\n", lab_ptr->symbol_name); error_exit();}
				
					subscr = scanarg();		/* get the subscript as an x_paren				*/
				}
				
				if (token_type != t_opr || token_info != o_eq)
					{printf( "xpltoc: expected assignment for '%s'\n", lab_ptr->symbol_name); error_exit();}
				
				scan();						/* assignment operator							*/
				string2 = scan_space_and_comment();
				
				arg = expr();				/* and get what we are assigning				*/
				
				
				/* emit assignment: */
				
				tab(indent);							/* indent						*/
				put_symbol(lab_ptr->symbol_name);		/* put symbol name				*/
				
				if (string1)
					{put_symbol(string1); free(string1); string1 = NULL;}
					
				if (subscr)								/* emit subscript				*/
				{
					put_symbol("[");					/* start subscript		*/
					
					if (subscr->x_pre_string)
						put_symbol(subscr->x_pre_string);
					
					compute((node *)(subscr->x_arg2), TRUE);
					
					put_symbol("]");
		
					if (subscr->x_post_string)
						put_symbol(subscr->x_post_string);
					
					x_rel(subscr);
				}
				
				put_symbol("=");
				
				if (string2)
					{put_symbol(string2); free(string2); string2 = NULL;}
					
				compute(arg, TRUE);							/* emit expression				*/
				put_symbol(";");
		
				if (token_type != t_semi)
					{printf( "xpltoc: missing semicolon after assignment to '%s'\n", lab_ptr->symbol_name); error_exit();}
					
				scan();										/* skip semi					*/
				toss_white_space();							/* to likely comment			*/

				process_stmt_comment(TRUE);

				if (proc_level == 0) pop_main();			/* pop from main now			*/
				
				stmt_type = 0;								/* did an assignement...		*/
				
				break;										/* got an assignment statement	*/
			}
			
			else
				{printf( "xpltoc: syntax error or undefined symbol processing '%s'\n", lab_ptr->symbol_name); error_exit();}
		}

		else
		{		
			printf( "xpltoc: unrecognized statement or syntax error (%d %s) line %d\n", token_type, token, lcount);
			error_exit();
		}
	}
	
	return (stmt_type);						/* return possible t_end...		*/
}


/*--------------------------------------------------------------------------------------*/
/*	main() for xpltoc																	*/
/*--------------------------------------------------------------------------------------*/

static void print_help()
{
	printf( "XPLTranslator: Version 1.000\n");
	printf( "usage: -c                      recognize \"C\" syntax\n");
	printf( "       -x                      create string constants in XPL format\n");
	printf( "       -i                      translate inserted files\n");
	printf( "       -1                      combine all insert files into one output file; implies -i\n");
	printf( "       -p                      show progress during translation\n");
    printf( "       -e                      show extended progress during stranslation\n");
    printf( "       -h                      converted file is a header file\n");
	printf( "       -m directory            specify Able Master Directory for : insert files\n");
	printf( "       -o directory            specify output directory for translated files\n");
	printf( "       -d directory            specify Able Current Directory for source and insert files\n");
	printf( "       filename                source file name\n");
}

int translate(int argc, char *argv[])
{
	int				argcount = 0;
	int				i        = 0;
	char			main_proc_name[512] = {""};
    
    if (0) {
        argc = 7;
        argv[0] = (char *)"XPLTranslator";
        argv[1] = (char *)"-m";
        argv[2] = (char *)"/Volumes/CJ Data/Projects/SDC/Able";
        argv[3] = (char *)"-o";
        argv[4] = (char *)"/Volumes/CJ Movies/XPL Build Products/Translated";
        argv[5] = (char *)"-h";
        argv[6] = (char *)":SYNLITS:PRMLITS";
    }

	/* set up to clean-up on termination */
	
	atexit (clean_up);
	signal(SIGINT, MySignalHandler);
    
	/* allocate memory we know we need */
	
	storage    = (char *)malloc(STORAGE_LENGTH);
	symlist    = (symbol_struct **) malloc(SYMBOL_LENGTH * sizeof(symbol_struct *));
	token      = (char *)malloc(TOKEN_LENGTH  );
	main_code  = (char *)malloc(MAIN_LENGTH   );
	temp_code  = (char *)malloc(TEMP_LENGTH   );
	
	main_buf   = main_code;
	main_avail = MAIN_LENGTH;
	
	if (!storage || !token || !symlist || !main_code || !temp_code)
		{printf("xpltoc: could not get memory\n"); error_exit();}
	
	token    [0] = 0;						/* init strings in all cases */
	main_code[0] = 0;
	temp_code[0] = 0;
	
	/* process compiler arguments */
	
	if (argc <= 1)
		{print_help(); error_exit();}

	for (argcount = 1; argcount < argc; )
	{
		char *the_arg = argv[argcount];
		
		if (!the_arg)
			exit(1);
		
		if ((strcmp(the_arg, "-c") == 0))
		{
			recognize_c_syntax = TRUE;
			insert_needed      = FALSE;
			argcount += 1;
			continue;
		}

		if ((strcmp(the_arg, "-x") == 0))
		{
			create_xpl_strings = TRUE;
			argcount += 1;
			continue;
		}

		if ((strcmp(the_arg, "-p") == 0))
		{
			show_progress = TRUE;
			argcount += 1;
			continue;
		}

		if ((strcmp(the_arg, "-e") == 0))
		{
			show_extended = TRUE;
			argcount += 1;
			continue;
		}

        if ((strcmp(the_arg, "-h") == 0))
        {
            is_header = TRUE;
            argcount += 1;
            continue;
        }
        
		if ((strcmp(the_arg, "-i") == 0))
		{
			convert_includes = TRUE;
			argcount += 1;
			continue;
		}

		if ((strcmp(the_arg, "-1") == 0))
		{
			create_one_file = TRUE;
			argcount += 1;
			continue;
		}

		if ((strcmp(the_arg, "-m")   == 0)
		&&  (argcount+1               <  argc)        
		&&  (strlen(argv[argcount+1]) < sizeof(master_dir_name)))
		{
			strcpy(master_dir_name, argv[argcount+1]);
			argcount += 2;
			continue;
		}

		if ((strcmp(the_arg, "-o")   == 0)
		&&  (argcount+1               <  argc)        
		&&  (strlen(argv[argcount+1]) < sizeof(out_dir_name)))
		{
			strcpy(out_dir_name, argv[argcount+1]);
			argcount += 2;
			continue;
		}

		if ((strcmp(the_arg, "-d")   == 0)
		&&  (argcount+1               <  argc)        
		&&  (strlen(argv[argcount+1]) < sizeof(cur_dir_name)))
		{
			strcpy(cur_dir_name, argv[argcount+1]);
			argcount += 2;
			continue;
		}
		
		if (source_file_name[0])
			{printf("xpltoc: two source files specified\n"); print_help(); error_exit();}
			
		if (strlen(the_arg) >= sizeof(source_file_name))
			{printf("xpltoc: source file name too long\n"); print_help(); error_exit();}
		
		strcpy(source_file_name, the_arg);

		argcount += 1;
	}
	
	
	/* Loop processing source file */

	construct_symbol_table();							/* build symbol table			*/
	
	open_source_file(source_file_name);					/* open source 					*/
	open_output_file(source_file_name);					/* open out file				*/

	get();												/* prime with first character	*/
	scan();												/* scan off first token			*/
	
	while (token_type != t_eof || insert_stack_ptr)		/* wait for EOF of main source	*/
		stmt();											/* process token & get next		*/
	
	if (main_code[0])									/* emit main code if any...		*/
	{
		strcpy(main_proc_name, source_file_name);		/* synthesize main name			*/
		
		for (i=0; i<strlen(main_proc_name); i++)		/* clean up file name chars		*/
		{
			char j = main_proc_name[i];					/* get file name character		*/
			
			if (j>= 'a' && j <= 'z')					/* keep lower case				*/
				;
		
			else if (j>= '0' && j <= '9')				/* keep digits					*/
				;
				
			else if (j>='A' && j <= 'Z')				/* map upper to lower			*/
				j += ('a' - 'A');
		
			else j = '_';								/* replace all others with _	*/
			
			main_proc_name[i] = j;
		}
		
		strcat(main_proc_name, "_main");
		
		put_symbol("void\t");
		put_symbol(main_proc_name);
		put_symbol("()\n{\n");

		fabricate_temp_list();
		
		put_symbol(temp_list);
		temp_list[0] = 0;
		
		put_symbol(main_code);
		main_code[0] = 0;
		
		put_symbol("}\n");

		put_proto("extern\tvoid\t");
		put_proto(main_proc_name);
		put_proto("();\n");
	}
	
	printf("Translation Completed.  In use: %d\n", num_blocks_in_use);
	
	return (0);
}
