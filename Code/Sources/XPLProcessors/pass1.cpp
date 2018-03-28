/*	PASS1.C									*/

/*	Translated to C:   	11/12/96 at 07:51	*/
/*	Translator Version:	0.000				*/

/* 	Pass1 from xpl 6.12						*/


#include 	<string.h>
#include 	<stdlib.h>
#include 	<stdio.h>
#include	"XPLCompiler.h"
#include	"XPLRuntime.h"

#include	"defs.h"			/* include able files.  by the way, which	*/
#include	"p1-defs.h"			/* came first, the chicken or the egg?		*/
#include	"p12-defs.h"

#include	"literals.h"
#include	"asciilit.h"


/*----------------------------------------------------------------------------------------------*/
/* Global variables 																	        */
/*----------------------------------------------------------------------------------------------*/

/* Special storage areas:
.  
.   The compiler uses storage areas for the construction of the symbol
.   table, expression trees, and other information.
.  
.   The following arrays are used: */

/* 4/14/89 - cj - fixed signed comparison for s.names.used to get */
/*                reloc statistics printout correct               */

static	fixed	stab_ptr;						/* pointer to symbol table						*/
static	fixed	stxt_ptr;						/* pointer to symbol table text area			*/
static	fixed	slit_ptr;						/* pointer to literal table text area			*/
static	fixed	stsiz;							/* size of symbol table							*/
static	fixed	trmst;							/* termination status							*/

static	fixed	com [maxcomm + 1];           	/* set up communications area 					*/
static	fixed	name[max_sym_len + 1];			/* current symbol/string constant				*/
static	fixed	fstk[fstklen];					/* floating point constants						*/
static	fixed	istk[istklen + 1];				/* insert file stack area						*/
static	FILE*	filestk[ins_levels + 1];		/* to help with insert file processing			*/
static	char*	textstk[ins_levels + 1];		/* to help with insert file processing			*/
static	FSSpec	specstk[ins_levels + 1];		/* to help with insert file processing			*/
static	fixed	hashtab [hashsize + 1];			/* and this is hash table start					*/
static	fixed	hashdep [hashsize + 1];			/* hashed depths are stored here				*/
static	fixed	stack_length;					/* length of stack frame						*/
static	fixed	fstkptr;						/* used to rotarily allocate fstk				*/

static	char	cfname [256] = {""};			/* name of insert file being processed, if any	*/
static	char	cfnmes [10]  = {""};			/* when in insert, ' in file '					*/
static	char	origdir[256] = {""};			/* original directory; for enter '*'			*/
static	char	cnam   [256] = {""};			/* able portion of entered file name			*/

static	char	in_fmes[] = {" in file "};		/* smae length as CFNMES						*/
static	fixed	ersymb [17];					/* symbol name during error						*/

static	fixed	alias_ptr = 0;					/* used to allocate alias lists					*/

static	fixed	mod_scanned = 0;				/* true if we're compiling a relocatable module	*/
static	fixed	in_begin = 0;					/* levels of BEGINs we're down (differentiates level changes due to BEGINs from PROCs)	*/
static	boolean	public_procs = false;			/* True if all procs should be forced PUBLIC	*/

static	fixed	token;							/* holds current token type						*/
static	fixed	info;							/* global information variable					*/
static	fixed	depth;							/* depth of current token						*/
static	fixed	ufsp;							/* pointer to undefined symbol					*/
static	fixed	name_pt;						/* pointer to symbol table						*/
static	fixed	dcl_symbol;						/* True if we're scanning a variable name in a DCL statement	*/
		fixed	line_no;						/* current line number							*/
static	fixed	plineno;
static	fixed	numerrs;

static	fixed	stptr;							/* symbol table pointer							*/
static	fixed	ptptr;							/* procedure table pointer						*/

static	fixed	cur_proc_def = -1;				/* current procedure definition					*/
static	fixed	cur_proc_dpt;					/* depth of current procedure - GOES when ER.CRG goes	*/
static	fixed	data_key;						/* key for data statement						*/
static	fixed	_stmts = 0;						/* # of statements parsed						*/


/*----------------------------------------------------------------------------------------------*/
/* File input/output subroutines:														        */
/*----------------------------------------------------------------------------------------------*/

/* File input/output subroutines:
.  
.   The following routines are used to read and write data from the various 
.   source, intermediate, and runtime package files.  These routines 
.   facilitate the reading of files on a byte-by-byte basis and using 
.   our own controlled buffer sizes.
.
.   Read character from source file:
.  
.   The routine 'GC' is called to get the next character from the
.   source file.  It will return the next 8-bit character (extracted
.   from the 16-bit NEDCO machine word).  If an end of file
.   condition is encountered, a value of '-1' will be returned.
.  
.   The processing of insert files is done by the statement scanner
.   ('STMT' processes the INSERT statement) and the scanner ('SCAN'
.   detects the end-of-file condition of an inserted file).  As such,
.   The 'GC' procedure does not differentiate between the end of the
.   main source file and the end of an insert file. */

static	fixed	gc()							/* procedure to get next character				*/
{
	if (in_text)								/* if file is in memory							*/
	{
		if (*in_text)
			return (*in_text++);
		else
			return (0);
	}
	else										/* else read from disk							*/
		return ((fixed) (getc(in_file)));
}
	

/*----------------------------------------------------------------------------------------------*/
/* Intermediate and object file:														        */
/*----------------------------------------------------------------------------------------------*/

/* Intermediate and object file:
.  
.   The routines 'EMIT' and 'WRITEOBJ' are used to write data to the
.   intermediate file and the object file, respectively.
.  
.   Both the intermediate file and the object file are (except for one case)
.   written sequentially.  Therefore, each routine is passed the word
.   which should be appended to the appropriate file.
.  
.   The specified data word is passed as a fixed point argument to both 
.   routines.
*/

static	fixed	abort_now;

static	void	emit(							/* emit word to file							*/
	fixed	word)
	
{
	if (inter_file_word_length	>= INTER_FILE_WORD_SIZE)
	{
		if (!abort_now)
		{
			print("\n");
			print("Program too large (interfile1).\n");
			abort_now = true;

			if (in_text)
				report_error_string (global_cpb, line_no, in_text - * (char **) in_base, &in_spec);
		}
	}
	else
		inter_file[inter_file_word_length++] = word;
}
	
static	void	lchk()							/* call to make sure line number is passed to pass2	*/
{
	static	fixed	lastnum;					/* last line number emitted to file				*/
	
	if (lastnum != line_no) {					/* write line number if new						*/
		emit (t_lnum);
		emit (line_no);
		lastnum = line_no;
	}
}


/*----------------------------------------------------------------------------------------------*/
/* Symbol table																					*/
/*----------------------------------------------------------------------------------------------*/

/* The symbol table (STABLE) can reside in either internal or external
.  memory.  It is initially placed in internal memory.  The routine INIT
.  decides if it can be moved to external memory and moves it if so.
.
.  The following routines are used to access the symbol table (STABLE). */

static	fixed	stable(							/* return the contents of array cell STABLE (index)	*/
	fixed	index)								/* index into STABLE array						*/
	
{
	_write_60(stab_ptr + shr(index, 8));		/* write sector address of array element		*/
	_write_61(index);							/* and word address								*/
	return (_read_62());						/* return the element							*/
}
	
static	void	set_stable(						/* set the contents of array cell STABLE (index) to VALUE	*/
	fixed	index, 								/* index into STABLE array						*/
	fixed	value)								/* value to set element to						*/
	
{
	_write_60(stab_ptr + shr(index, 8));		/* write sector address of array element		*/
	_write_61(index);							/* and word address								*/
	_write_62(value);							/* set the element to VALUE						*/
}
	
static	fixed	stext(							/* return the contents of array cell STEXT (index)	*/
	fixed	index)								/* index into STABLE array						*/
	
{
	_write_60(stxt_ptr + shr(index, 8));		/* write sector address of array element		*/
	_write_61(index);							/* and word address								*/
	return (_read_62());						/* return the element							*/
}
	
static	void	set_stext(						/* set the contents of array cell STEXT (index) to VALUE	*/
	fixed	index, 								/* index into STABLE array						*/
	fixed	value)								/* value to set element to						*/
	
{
	_write_60(stxt_ptr + shr(index, 8));		/* write sector address of array element		*/
	_write_61(index);							/* and word address								*/
	_write_62(value);							/* set the element to VALUE						*/
}
	
static	fixed	slit_text(						/* return the contents of array cell SLIT_TEXT (index)	*/
	fixed	base,								/* base is pointer with stride of 8 words		*/
	fixed   offset)								/* word offset									*/
{
	base   = base + shr(offset, 3);				/* normalize base & offset value				*/
	offset = offset & 7;
	
	_write_60(slit_ptr + shr(base, 5));			/* write sector address of array element		*/
	_write_61(shl(base, 3) + offset);			/* and word address								*/
	return (_read_62());						/* return the element							*/
}
	
static	void	set_slit_text(					/* set the contents of array cell SLIT_TEXT (index) to VALUE	*/
	fixed	base,								/* base is pointer with stride of 8 words		*/
	fixed   offset,								/* word offset									*/
	fixed	value)								/* value to set element to						*/
	
{
	base   = base + shr(offset, 3);				/* normalize base & offset value				*/
	offset = offset & 7;
	
	_write_60(slit_ptr + shr(base, 5));			/* write sector address of array element		*/
	_write_61(shl(base, 3) + offset);			/* and word address								*/
	_write_62(value);							/* set the element to VALUE						*/
}
	

/*----------------------------------------------------------------------------------------------*/
/* Error message routies:																		*/
/*----------------------------------------------------------------------------------------------*/

/* The following procedures are used to print error messages on the terminal. */

static	void	er_symb(						/* select a ersymb from tabl and put in 'ERSYMB'*/
	fixed	ptr)
{
    fixed			_upper0;
	static	fixed	i, j, k;
	i = (stable (ptr + s_tokn) & 255);			/* get number of words in symbol				*/
	if (i > 16) i = 16;							/* and limit for ersymb							*/
	ersymb [0] = shl(i, 1);						/* and number of bytes to print					*/
	for (_upper0 = i, i = 1; i <= _upper0; i++) {ersymb [i] = stext (ptr + s_text + i - 1); }
	if (ersymb[0] && byte(ersymb, ersymb[0] - 1) == 0)
		ersymb[0]--;
}
	
static	fixed	prior_emes1;						/* and only once per message					*/

static	void	er_emes1(						/* format one - for undef/dupl ersymb			*/
	cstring	str, 
	fixed	ptr)								/* ptr to aray									*/
{
	trmst = -1;									/* abort compilation							*/
	if (prior_emes1 == ptr) return;				/* have already prited this one					*/
	prior_emes1 = ptr;							/* this is latest one printed					*/
	er_symb (ptr);								/* extract ersymb into 'ERSYMB'					*/
	
	if (in_text)
	{
		print("%s%s%1p%s %d\n", str, " \'", ersymb, "\' at line", line_no);
		
		report_error_string (global_cpb, line_no, in_text - * (char **) in_base, &in_spec);
	}
	else
	{
		print("### %s%s%1p%s %d%s%s\n", str, " \'", ersymb, "\' at line", line_no, cfnmes, cfname);
		
		if (cfname[0])
			printf("open '%s'; line %d\n", cfname, line_no);
		else
			printf("open '%s'; line %d\n", host_source_file, line_no);
	}
	
	numerrs = numerrs + 1;
	if (numerrs > max_errs) abort_now = true;	/* an obviously fatal compilation				*/
}
	
static	void	er_ufls(						/* undefined label/ersymb message				*/
	fixed	ptr)
{
	er_emes1 ((char *) "Undefined symbol", ptr);
}
	
static	void	er_lt(							/* duplicate - defined twice					*/
	fixed	ptr)
{
	er_emes1 ((char *) "Duplicate definition for", ptr);
}
	
static	void	er_msub(						/* missing subscript							*/
	fixed	ptr)
{
	er_emes1 ((char *) "Missing subscript for", ptr);
	plineno = line_no;
}
	
/* Error messages, continued: */

static	void	er_emes2(						/* pass message string							*/
	cstring	str)
{
	trmst = -1;									/* abort compilation							*/
	if (token == t_und) er_ufls (ufsp);
	if (plineno == line_no) return;				/* one per line									*/
	plineno = line_no;							/* save - only one useless message per line		*/

	if (in_text)
	{
		print("%s%s %d\n", str, " at line", line_no);
		report_error_string (global_cpb, line_no, in_text - * (char **) in_base, &in_spec);
	}
	else
	{
		print("### %s%s %d%s%s\n", str, " at line", line_no, cfnmes, cfname);

		if (cfname[0])
			printf("open '%s'; line %d\n", cfname, line_no);
		else
			printf("open '%s'; line %d\n", host_source_file, line_no);
	}
	
	numerrs = numerrs + 1;
	if (numerrs > max_errs) abort_now = true;
}
	
static	void	er_sctl() {er_emes2 ((char *) "String constant too long"); }
static	void	er_map() {er_emes2 ((char *) "Missing apostrophe"); }
static	void	er_tmd() {er_emes2 ((char *) "Too many digits in number"); }
static	void	er_ntc() {er_emes2 ((char *) "Nested or non-terminated comment"); }
static	void	er_iptd() {er_emes2 ((char *) "Improper type declaration"); }
static	void	er_rna() {er_emes2 ((char *) "RETURN statement not allowed"); }
static	void	er_tme() {er_emes2 ((char *) "Too many END statements"); }
static	void	er_ifnc() {er_emes2 ((char *) "Incorrect format in number"); }
static	void	er_ifm() {er_emes2 ((char *) "Incorrect format"); }
static	void	er_par() {er_emes2 ((char *) "Parenthesis required around expression"); }
static	void	er_nea() {er_emes2 ((char *) "Not enough arguments supplied"); }
static	void	er_tma() {er_emes2 ((char *) "Too many arguments supplied"); }
static	void	er_atdnm() {er_emes2 ((char *) "Argument types do not match"); }
static	void	er_me() {er_emes2 ((char *) "Missing END statement"); }
static	void	er_ms() {er_emes2 ((char *) "Missing semicolon or format error"); }
static	void	er_nm() {er_emes2 ((char *) "END label does not match"); }
static	void	er_cr() {er_emes2 ((char *) "Expression not allowed"); }
static	void	er_irec() {er_emes2 ((char *) "Improper recursion"); }
static	void	er_naad() {er_emes2 ((char *) "Undeclared procedure argument"); }
static	void	er_dupw() {er_emes2 ((char *) "Duplicate \'WHEN\' statement"); }
static	void	er_fpna() {er_emes2 ((char *) "Floating point not allowed in data list"); }
static	void	er_tmnc() {er_emes2 ((char *) "Too many numeric constants"); }
static	void	er_nst()  {if (!abort_now) er_emes2 ((char *) "Out of symbol table storage"); abort_now = true; }
static	void	er_etc()  {if (!abort_now) er_emes2 ((char *) "Expression too complicated" ); abort_now = true; }
static	void	er_tmb()  {if (!abort_now) er_emes2 ((char *) "Too many nested \'BEGIN\' statements"); abort_now = true; }
static	void	er_tmni() {if (!abort_now) er_emes2 ((char *) "Too many nested insert files"); abort_now = true; }
static	void	er_tmed() {if (!abort_now) er_emes2 ((char *) "Too many externals declared" ); abort_now = true; }
static	void	er_tmpl() {if (!abort_now) er_emes2 ((char *) "Too many procedures/labels"  ); abort_now = true; }
static	void	er_tmc()  {if (!abort_now) er_emes2 ((char *) "Too many constants");  abort_now = true; }
static	void	er_wna() {er_emes2 ((char *) "\'WHEN\' not allowed in procedure"); }
static	void	er_sna() {er_emes2 ((char *) "Subscript not allowed"); }
static	void	er_mtn() {er_emes2 ((char *) "No treename specified"); }
static	void	er_atm() {er_emes2 ((char *) "Argument type does not match previous proc defn"); }
static	void	er_ram() {er_emes2 ((char *) "Recursive attribute doesn\'t match forward ref"); }
static	void	er_pbna() {er_emes2 ((char *) "Neither public nor swapping procs can be nested"); }
static	void	er_mnfs() {er_emes2 ((char *) "\'MODULE\' must be the first statement of a library"); }
static	void	er_mms() {er_emes2 ((char *) "Multiple \'MODULE\' statements not allowed"); }
static	void	er_somb() {if (!abort_now) er_emes2 ((char *) "Statement outside of module body"); abort_now = true; }
static	void	er_mnim() {er_emes2 ((char *) "Module name is missing"); }
static	void	er_crg() {er_emes2 ((char *) "Cannot reference global automatic variable"); }
static	void	er_ptl() {if (!abort_now) er_emes2 ((char *) "Program too large (variable space exceeds maximum)"); abort_now = true; }
static	void	er_amb() {er_emes2 ((char *) "XPL/C ambiguous boolean expression"); }
	
static	void	wa_wmes(						/* pass message string							*/
	cstring	str)
{
	if (plineno == line_no) return;				/* if an error already on this line, skip warning	*/
	print("### %s%s%s %d%s%s\n", "Warning: ", str, " at line", line_no, cfnmes, cfname);

	if (cfname[0])
		printf("open '%s'; line %d\n", cfname, line_no);
	else
		printf("open '%s'; line %d\n", host_source_file, line_no);
}
	
static	void	wa_pdip() {wa_wmes ((char *) "public variable declared inside a proc"); }
static	void	wa_oda() {wa_wmes ((char *) "overwriting contents of data array"); }
	

/*----------------------------------------------------------------------------------------------*/
/* Storage Allocation:																			*/
/*----------------------------------------------------------------------------------------------*/

/* Storage allocation:
.  
.  Many routines need allocatable fixed length blocks.  Since they
.  all want four word blocks, these blocks are all allocated out of
.  the same arrays.  The blocks consist of one entry in each of four
.  separate arrays.  To keep the different uses distinguishable, any
.  routines using these arrays should declare their own names for them
.  and also rename the storage allocation routines.  Note that the block
.  with index zero is never used.  This means that zero as a link may
.  be used to indicate the end of a list.  */

static	fixed	free_start = 1;					/* ptr to free, start here						*/
static	fixed	free_end;
static	fixed	blocks_in_use;

/* get a block from free storage */

static	fixed	get()
{
	fixed	block = free_start;
	if (block == 0) er_etc();
	else free_start = store1 [block];
	blocks_in_use = blocks_in_use + 1;
	
	if (blocks_in_use + 10 > _blocks)
		er_etc();
		
	return (block);
}
	
/* return a block to free storage */

static	void	rel(
	fixed	block)
{
	store1 [block] = free_start;
	free_start = block;
	blocks_in_use = blocks_in_use - 1;			/* decrement									*/
}
	

/*----------------------------------------------------------------------------------------------*/
/* "KEYS"																						*/
/*----------------------------------------------------------------------------------------------*/

/* Keys:
.  
.   There are two types of symbols that identify locations within a program: 
.     statement labels (used with GOTO), and
.     procedures (activated by a CALL statement).
.
.   When a label or procedure is defined, a unique number (starting with 1) is
.   assigned to that label or procedure.  This number is then used to identify
.     a. the destination of a GOTO,  or 
.     b. the destination of a CALL.
.
.   The procedure 'GKEY' is used to allocate the next unique number, 
.   hereinafter referred to as a 'NEXKEY'.
.  
.   Alternate keys are used in a similar way to provide expansion of the
.   number of keys possible beyond 16 bits.   Alternate keys are used
.   in the same way keys are,  but are restricted to certain intermediate
.   file record types (et.atr and et.ald as of 10/24/90). */

static	fixed	gkey()							/* used to get a key							*/
{
	nexkey = nexkey + 1;						/* incr key										*/
	
	if (_IGE_(nexkey + 20, shr(extern_base, 1))) er_tmpl();	/* did we collide with the externals? too many procs/labels	*/
		
	return (nexkey);							/* note that zero is never returned				*/
}
	
static	fixed	galt()							/* used to get an alternate key					*/
{
	altkey = altkey + 1;						/* incr alt key									*/
	
	if (_IGE_(altkey + 20, shr(extern_base, 1))) er_tmpl();	/* did we collide with the externals? too many procs/labels	*/
		
	return (altkey);							/* note that zero is never returned				*/
}
	
/* Variable and constant locations:
.
.  All variable and constant area locations are assigned by pass1.  These
.  locations are all relative to the variable area base and the constant
.  area base for the program/module being compiled.  The variable area
.  is assigned from RAM, the constant area from STR.DATA.  Public variables
.  are intermixed with local/internal variables.
.
.  External variables must be relocated in pass3 so that they point to
.  the area of storage defined by their public counterparts.  If these
.  were to be allocated during pass1, "holes" in the variable area would
.  be created during pass3 relocation (since externals are only references
.  and do not require allocated storage) which would need to be removed.
.  This would not be a problem except that pass2 does its register
.  optimization under the assumption that all variables are located
.  in their proper "relative" locations by pass1.  We need to assign
.  SOME location/identifier/key so we can identify externals for relocation
.  during pass3.  Therefore, externals are given their own address space
.  from EXTLOC.  GET_EXTLOC is called to return the next unique number
.  from this address space.  The first EXTLOC is EXTERN.BASE.  Note that
.  the formal parameters of an external procedure are external variables.
.
.  EXTERN.BASE is currently 60K.  This is because no program (and hence no
.  RAM location can exceed 60K and still work.  Ideally, EXTERN.BASE should
.  be (to minimize collisions for any maximum RAM size less than 64K):
.    (64K - (maximum.external.symbols.pass3.allows*average.number.of.procedures.per.file)). */

static	fixed	get_extloc()					/* get next unique external tag					*/
{
	extloc = extloc + 1;						/* get the next unique tag						*/
	
	if (_IGE_(extloc, 0xFFF0)) er_tmed();		/* did we wrap around? too many externals declared	*/
		
	return (extloc);							/* return next location							*/
}
	
/* Stack definitions( and routines:
. 
.  The stack is used for any important parameters during any of the
.  recursive routines in the compiler.  This is true not only for the
.  recursive descent scanning routines, but also, for example, for the
.  recursive expression compiler.  */

static	fixed	stackpt;						/* pointer to stack								*/

/* stack push and pop routines */

static	void	push(
	fixed	info)
{
	if (stackpt + 100 > stacksize) er_etc();
	stack [stackpt] = info;
	stackpt = stackpt + 1;
}
	
static	fixed	pop()
{
	stackpt = stackpt - 1;
	return (stack [stackpt]);
}
	
/* Symbol table storage:
.  
.   The symbol table forms the heart of pass1 of the compiler.  A single 16-bit
.   table is used.  The table is used at both ends (one going up, the other going
.   down) so that when it does run out of storage, there is absolutely none left.
.  
.   ******** Description of Symbol Table Entries *********
.
.   Two fixed point arrays are used for symbol processing:  HASHTAB and STABLE.
.  
.   Assume the existence of a symbol 'XYZ'.  A seven-bit hash table pointer is
.   computed from the ASCII values of the symbol characters.  This pointer
.   is used as an index into the array 'HASHTAB'.  The contents of
.   HASHTAB (<pointer>) is a pointer in the STABLE array.
.  
.   The STABLE array consists of two parts - a lower section growing up and
.   an upper section growing down.  The lower section is the main section.
.  
.   The lower section of the symbol table contains records.  There are
.   two types of records:  symbol table entries and literal strings.
.
.   For each symbol, there is a symbol table entry record in STABLE.
.   This record looks like:
.
.      word 0: 16-bit forward pointer to next bucket on this
.              hash linked list.
.      word 1: 8-bit data key/4-bit storage class/4-bit symbol depth field.
.      word 2: lower half contains the number of 16-bit words in character
.              field of symbol (i.e. (number of characters in
.              symbol plus one) divided by 2).
.              upper half contains the token type of the symbol. 
.      word 3: 16-bit information regarding symbol (either a
.              pointer into the variable area, or etc.)
.      words 4 and on: hold the ASCII characters, packed two to a word.
.  
.  The following variables are used to process symbols: */

static	fixed	s_depth;						/* level information							*/
static	fixed	hashcode;						/* computed hash information					*/
static	fixed	s_nl_stak[s_depth_max + 1], s_sl_stak[s_depth_max + 1], s_pdef[s_depth_max + 1];	/* holds recursive info							*/

/* Symbol table management:  subroutines
. 
.  Check if the symbol in 'NAME' is in the designated spot in S.NAMES: */

static	fixed	s_check(
	fixed	where)
{
	static	fixed	i, j;
	i = (stable (where + s_tokn) & 255);		/* extract length								*/
	if (i != name [0]) return (0);				/* length is not the same						*/
	for (j = 1; j <= i; j++) {					/* and compare									*/
		if (name [j] != stext (where + s_text + j - 1)) return (0);	/* not equal									*/
	}
	return (1);									/* found										*/
}
	
/* Routines to store/get certain fields from symbol record: */

static	fixed	gtok(							/* get token, pass pointer						*/
	fixed	p)
	
{
	return (shr(stable (p + s_tokn), 8));
}
	
static	void	ptok(							/* store token; pass pointer, token				*/
	fixed	p,
	fixed	token)
	
{
	set_stable (p + s_tokn, (stable (p + s_tokn) & 0x00FF) | shl(token, 8));
}
	
static	fixed	gclas(							/* get storage class, pass pointer				*/
	fixed	p)
	
{
	return (shr(stable (p + s_clas), 4) & 0x000F);
}
	
static	void	pclas(							/* store storage class; pass pointer, storage class	*/
	fixed	p,
	fixed	clas)
	
{
	set_stable (p + s_clas, (stable (p + s_clas) & ((fixed) 0xFF0F)) | shl(clas, 4));
}
	
static	fixed	gdkey(							/* get data key, pass pointer					*/
	fixed	p)
	
{
	return (shr(stable (p + s_dkey), 8));
}
	
static	void	pdkey(							/* store data key; pass pointer, data key		*/
	fixed	p,
	fixed	dkey)
	
{
	set_stable (p + s_dkey, (stable (p + s_dkey) & 0x00FF) | shl(dkey, 8));
}
	
static	fixed	gflag(							/* get procedure flags, pass procedure pointer	*/
	fixed	p)
	
{
	if ((p == -1) || (p == -2))					/* if at the top level or in WHEN				*/
		return (0);								/* no flags										*/
	else return (shr(stable (p + p_flag), 8));
}
	
static	void	pflag(							/* store procedure flags; passed procedure pointer & flags	*/
	fixed	p,
	fixed	new_flags)
	
{
	if (! ((p == -1) || (p == -2)))				/* if not at the top level and not in when		*/
		set_stable (p + p_flag, (stable (p + p_flag) & 0x00FF) | shl(new_flags, 8));
}
	
/* Symbol table management:  symbol look-up routine */

/* S.Lookup:
.   This procedure is used to look up a symbol in the table.
.   Upon entry, the symbol is stored in the global variable
.   'NAME'.  The routine returns a pointer to the symbol
.   or a zero in the case of a new symbol. */

static	fixed	s_lookup()
{
    fixed			_upper0;
	static	fixed	i;
	
	hashcode = name [0] + shl(name [1], 1) + shl(shl(name [name [0]], 2), 1);	/* hash function								*/
	hashcode = (hashcode & hashsize);			/* hashing function								*/
	name_pt  = hashtab [hashcode];				/* get pointer into stable						*/
	depth    = hashdep [hashcode];				/* pick up first symbol's depth					*/
	
	while (name_pt != 0) {						/* and scan every symbol that exists on this link	*/
		if (s_check (name_pt) != 0) return (name_pt);	/* found - return pointer						*/
		depth   = (stable (name_pt + s_dpth) & s_depth_max);	/* extract next depth							*/
		name_pt = stable (name_pt + s_next);	/* extract next pointer							*/
	}
		
	name_pt = stptr;							/* new symbol - store characters in table here so 'NAME' is free	*/
	depth = s_depth;							/* defined (presumably) at current level		*/

	if (name [0] > s_length)					/* stext area is parallel to stable.  choose	*/
		stptr = stptr + name [0];				/* which ever is longer...						*/
	else
		stptr = stptr + s_length;
		
	if (_IGT_(stptr + 500, ptptr))				/* check for overflow if equal, then ok			*/
		er_nst();
	
	set_stable (name_pt + s_next, -1);			/* indicate a new symbol here					*/
	set_stable (name_pt + s_dpth, -1);			/* zap depth field								*/
	set_stable (name_pt + s_tokn, name [0]);	/* save length field							*/
	
	for (_upper0 = name [0], i = 1; i <= _upper0; i++) {
		set_stext (name_pt + s_text + i - 1, name [i]);	/* copy character info into table, because name gets used often	*/
	}
	
	return (0);									/* indicates a new symbol						*/
}
	
/* Symbol table management:  symbol definition routine */

/*  The S.DEFINE routine is used to enter a symbol entry into the
.   symbol table.   Multilevel variable localization is incorporated.
.  
.   S.DEFINE is passed two variables:  a pointer into the symbol table
.   (NAME.PT) and a hash table pointer.
.  
.  S.DEFINE returns a value that is a pointer into STABLE.  Note
.  that the NAME.PT returned by S.DEFINE might be different than
.  the NAME.PT passed to S.DEFINE, in the case of a localized
.  redefinition of a variable. */

static	fixed	s_define(						/* pass then - note these are local variables!!!	*/
	fixed	name_pt,
	fixed	hashcode)
{
	static	fixed	i, j, k;
	
	i = hashtab [hashcode]; j = hashdep [hashcode];	/* get forward ptr and depth for this bucket	*/
	while ((i != 0) && (j == s_depth)) {		/* detect end of list							*/
		if (i == name_pt) return (i);			/* use this as pointer - defined on this level	*/
		j = (stable (i + s_dpth) & s_depth_max);	/* get depth									*/
		i = stable (i + s_next);				/* and get forward pointer						*/
	}
		
	/* new symbol, at least on this level */
	
	if (stable (name_pt + s_next) != -1) {		/* if defined on earlier level, must duplicate ascii	*/
		i = name_pt;							/* save current pointer							*/
		name_pt = stptr;						/* and pointer to current end					*/
		j = (stable (i + s_tokn) & 255);		/* extract length of record						*/
		
		if (j > s_length)						/* stext area is parallel to stable.  choose	*/
			stptr = stptr + j;					/* which ever is longer...						*/
		else
			stptr = stptr + s_length;
		
		if (_IGT_(stptr + 500, ptptr)) er_nst();
		
		for (k = 0; k < j; k++) {				/* copy over ASCII info							*/
			set_stext (name_pt + s_text + k, stext (i + s_text + k));
		}

		set_stable (name_pt + s_tokn, j);		/* save length, with a zero token field			*/
	}
	
	set_stable (name_pt + s_next, hashtab [hashcode]);	/* pick up pointer to next guy					*/
	set_stable (name_pt + s_dpth, hashdep [hashcode]);	/* and his depth								*/
	
	ptok (name_pt, t_und);						/* indicate undefined							*/
	
	hashtab [hashcode] = name_pt;				/* store forward ptr in hash table				*/
	hashdep [hashcode] = s_depth;				/* and depth									*/
	
	return (name_pt);
}

/* Symbol table management:  localize and globalize routines */

/* S.Block procedure:
.  
.   S.BLOCK is called at the beginning of a variable localization event
.   (such as the start of a procedure definition).  The routine pushes
.   the current symbol table pointers onto an internal stack and then
.   increments the 'DEPTH' field. */

static	void	s_block()
{
	s_nl_stak [s_depth] = stptr;				/* save current symbol table pointer			*/
	s_sl_stak [s_depth] = ptptr;				/* save pointer for procedure arguments			*/
	s_depth = s_depth + 1;						/* increment depth count						*/
	if (s_depth + 2 > s_depth_max) er_tmb();
	s_pdef [s_depth] = cur_proc_def;			/* save pointer to procedure definition at this level	*/
}
	
/* S.Endblock procedure:
.
.   S.ENDBLOCK is called at the end of every level of block definitions.
.  
.   It is used to undeclare all variables, labels, and procedures that
.   are defined on inner blocks.  Undeclared labels (forward reference)
.   are detected at this level also. */

static	void	s_endblock()
{
	static	fixed	index, last, last_depth, token, info;
	
	if (_ILT_(s_names_used, stptr + (stsiz - 1) - ptptr)) s_names_used = stptr + (stsiz - 1) - ptptr;	/* stats										*/
		
	for (index = 0; index <= hashsize; index++) {	/* wipe out all defs at this level				*/
		last = hashtab [index];					/* pick up start pointer this bucket			*/
		last_depth = hashdep [index];			/* and the depth								*/
		while ((last != 0) && (last_depth == s_depth)) {	/* scan defined symbols this depth				*/
			token = gtok (last);				/* get token and info of symbol					*/
			info = stable (last + s_locn);
			if (token == t_label) {				/* for label - make sure def'd					*/
				if ((info & 1) == 0) er_ufls (last);	/* undefined									*/
			}
			else if (token == t_proc) {			/* for procedure - make sure located			*/
				if ((stable (info + p_key) & 1) == 0) er_ufls (last);	/* undefined									*/
			}
			last_depth = (stable (last + s_dpth) & s_depth_max);	/* pick up depth of next symbol					*/
			last = stable (last + s_next);		/* pick up pointer to next symbol				*/
		}
		hashtab [index] = last;					/* and save top pointer in hashtab				*/
		hashdep [index] = last_depth;			/* and associated depth							*/
	}
		
	s_depth = s_depth - 1;						/* decrement depth count						*/
	stptr = s_nl_stak [s_depth];
	ptptr = s_sl_stak [s_depth];
	
}
	
/* Define an external or public type entry:
.
.    This routine just outputs to the interfile a record
.    which contains the name, the type (public or external)
.    and the local location used.  The linker in pass3 will
.    fix up the rest. */

static	void	pub_ext_def(
	fixed	sptr, 								/* pointer into symbol table					*/
	fixed	pub_ext, 							/* T.Extern (external) or T.PUBLIC (public)		*/
	fixed	storage, 							/* storage used by this							*/
	fixed	lnum)								/* line number of definition					*/
{
    fixed			_upper0;
	static	fixed	i, len, type, info, numargs;
	fixed			fname_byte_len = 0;
	fixed			fname_word_len = 0;
	char			*the_fname     = NULL;
		
	if ((! (mod_scanned & 1)) && (pub_ext == t_extern))/* if we aren't a module and have external variables	*/
		flags = (flags | link_flag);			/* better call linker							*/
		
	type = stable (sptr + s_tokn);				/* get type information							*/
	info = stable (sptr + s_locn);				/* and info										*/
	
	/* look up length of current file name */
	
	if ((pub_ext == t_public)					/* if defining symbol, get file name and line	*/
	||  (pub_ext == t_symonly))					/* or if emitting local symbol reference		*/
	{
		if (cfname[0])
			the_fname = &cfname[0];
		else
			the_fname = &host_source_file[0];
		
		fname_byte_len = strlen(the_fname);
		fname_word_len = shr(fname_byte_len+1, 1) + 3;
	}
		
	if ((shr(type, 8) == t_label) && (pub_ext != t_extern))/* externals are passed as is					*/
		info = shr(info, 1);					/* but we need to discard the 'defined' bit for others	*/
		
	len = 4 + (type & 0x00FF);					/* every block has symbol + misc				*/
	
	if (pub_ext == t_symonly)					/* sym only: don't need arg list				*/
		numargs = -1;
	
	else if (shr(type, 8) == t_proc) {			/* procedures have more							*/
		numargs = stable (info + p_args);		/* get number of arguments						*/
		len = len + 3 + numargs + numargs;		/* procedures have type info					*/
	}
	else numargs = -1;							/* no args if not a proc						*/
	
	if (len + fname_word_len >= 250)			/* don't emit name if record would be too long	*/
		fname_word_len = 0;

	emit (t_symref);							/* t.symref										*/
	emit (shl(pub_ext, 8) + len + fname_word_len);	/* flag / len								*/
	emit (type);								/* type / symlen								*/
	emit (info);								/* info if needed								*/
	emit (storage);								/* storage used									*/
	emit (lnum);								/* line number									*/
	for (_upper0 = (type & 0x00FF) - 1, i = 0; i <= _upper0; i++) {	/* copy over symbol text						*/
		emit (stext (sptr + s_text + i));
	}
		
	if (numargs != -1) {						/* procedures have more info					*/
		
		if (pub_ext == t_extern)				/* emit key location							*/
			emit (stable (info + p_key));		/* external - defined bit is significant!		*/
		
		else emit (shr(stable (info + p_key), 1));	/* public - defined bit is extraneous			*/
		
		emit (stable (info + p_args));			/* # of args									*/
		emit (stable (info + p_rtyp) & 0x00FF);	/* returns type									*/
		
		for (i = 0; i < numargs; i++) {			/* emit two words for every argument			*/
			emit (stable (info + p_parm + p_ptyp - i - i) & 0x00FF);	/* type											*/
			
			if ((gflag (info) & p_recursive) != 0)/* are we defining a recursive procedure?		*/
				emit (recurs_parms + i);		/* yes, variable location is canonical			*/
			else emit (stable (info + p_parm + p_ploc - i - i));	/* no, send out relative variable location		*/
		}
	}											/* procedures have more info					*/

	if (fname_word_len)							/* if have a file name to emit, then do so		*/
	{
		emit(fname_word_len);					/* emit word length of this portion of record	*/
		emit(fname_byte_len);					/* emit byte length of file name				*/
		emit(lnum);								/* emit line number								*/
				
		for (i = 0; i < fname_byte_len; i+=2)
			emit((((fixed) (unsigned char) the_fname[i]) << 8) | ((fixed) (unsigned char) the_fname[i+1]));
	}
}


/* This procedure sets any compile-time flags based on the passed character. */

/* 12-5-88 - cj - changed intermediage file offsets for humungous work files */

void	init_pass1_statics()
{
	memset(com, 0, sizeof(com));
}

void	set_flags(										/* set compile-time flags						*/
	fixed	c)											/* character flag								*/
	
{
	if (c >= 'a')										/* to upper...									*/
		c -= 'a' - 'A';
		
	if (c == a_e) flags = (flags | dump_flag);			/* e: dump stats								*/
	else if (c == a_m) flags = (flags | symtab_flag);	/* m: dump symbol map							*/
	else if (c == a_x) public_procs = true;				/* x: make all procs public						*/
	else if (c == a_o) flags = (flags | skip_flag);		/* o: no optimize								*/
	else if (c == a_f) flags = (flags | force_flag);	/* f: force code to ext memory					*/
	else if (c == a_z) {
		if ((flags & debug_flag) == 0)					/* z: Haven't printed this yet					*/
			print("   ==> Generating code for debugger <==\n");
		flags = (flags | debug_flag);					/* gen better code for debugging				*/
	}
}

/*  String constant processing:  emit string constant to data areas.
.  
.   String constant and other numeric data is emitted to the object file
.   during pass1.  It is bound into the output file at the end of the
.   initialization routine.
.  
.   The following routine is called to emit a string constant
.   (stored in the array 'NAME') out to the output file.
.  
.   The string constant is written out to the file in a form that makes
.   it look like an array (fixed point, etc.).
*/

static	fixed	e_scon(							/* emit a string constant out to constant file	*/
	fixed	key,
	fixed	n)									/* key of proc this scon is passed to/parameter number it's passed as	*/
{
	static	fixed	i, j, k;
	
	i = str_data;								/* get pointer to start of string				*/
	j = name [0];								/* get number of bytes							*/
	k = shr(j + 3, 1);							/* and total # of words of data					*/
	emit (t_copys);								/* indicate copy scon							*/
	emit (k + 2);								/* emit # of words								*/
	emit (key);									/* emit key of proc scon is passed to			*/
	emit (n);									/* and the parameter number it's passed as		*/
	for (j = 0; j < k; j++) {					/* copy out entire string including # of byte field	*/
		emit (name [j]);						/* write out 16-bit word						*/
	}
	str_data = str_data + k;					/* compute amount emitted						*/
	if (_ILT_(str_data + 100, k)) er_tmc();		/* too many scons								*/
	return (i);
	
}
	
static	fixed	e_scondata()					/* emit a string constant out to constant file	*/
{
	static	fixed	i, j, k;
	
	i = str_data;								/* get pointer to start of string				*/
	j = name [0];								/* get number of bytes							*/
	k = shr(j + 3, 1);							/* and total # of words of data					*/
	emit (t_copyd);								/* indicate copy data							*/
	emit (k);									/* emit # of words								*/
	for (j = 0; j < k; j++) {					/* copy out entire string including # of byte filed	*/
		emit (name [j]);						/* write out 16-bit word						*/
	}
	str_data = str_data + k;					/* compute amount emitted						*/
	if (_ILT_(str_data + 100, k)) er_tmc();		/* too many data constants						*/
	return (i);
	
}


/* Data list initialization:
.  
.   Words of data generated by the 'DATA' statement are emitted
.   to the object file in blocks of 10.  The array 'DBUF' holds
.   up to 10 entries and is written to the intermediate file at
.   appropriate intervals. */

static	fixed	dbuf[10];						/* 9 words										*/
static	fixed	dbufp;							/* pointer into same							*/

static	void	dbuffrc()						/* call to force out							*/
{
    fixed			_upper0;
	if (dbufp != 0) {							/* only if data									*/
		str_data = str_data + dbufp;			/* and keep track of string data statistics		*/
		emit (t_copyd);							/* record indicates copy data to inter file		*/
		emit (dbufp);							/* and number of words							*/
		for (_upper0 = dbufp - 1, dbufp = 0; dbufp <= _upper0; dbufp++) {
			emit (dbuf [dbufp]);
		}
		dbufp = 0;								/* this is done									*/
	}
}												/* force routine								*/
	
static	void	dbufemit(						/* emit one word								*/
	fixed	word)
{
	dbuf [dbufp] = word;						/* save word									*/
	dbufp = dbufp + 1;
	if (dbufp == 10) dbuffrc();					/* force it if can								*/
}												/* bleep										*/
	

/* Scanner:  fixed to floating conversion.
.  
.   During pass1, numeric constants are converted from a sequence of
.   digits into an internal form.  For fixed point data, this internal
.   form is a binary representation.  For floating point data, the
.   32-bit word is generated.
.  
.   Sometimes it is necessary to convert from one form to another.
.   The following routines do that:
*/

static	fixed	fval1, fval2;					/* used to return floating pont values			*/

static	void	fixed_to_float(					/* and convert fixed to floating				*/
	fixed	val)
{
	static	fixed	sign, exp;					/* components									*/
	fval1 = 0; fval2 = val;						/* mask to 16 bits on DTSS						*/
	sign = shr(fval2, 15);						/* extract single sign bit						*/
	exp = 64 + 31;								/* start here									*/
	if (sign & 1) fval2 = -fval2;				/* mantissa is always +							*/
	if (fval2 != 0) {							/* if any mantissa								*/
		while ((fval1 & (fixed) 0x4000) == 0) { /* shift until normalized						*/
			fval1 = shl(fval1, 1);				/* extract 16-bits								*/
			if ((fval2 & (fixed) 0x8000) != 0) fval1 = (fval1 | 1);
			fval2 = shl(fval2, 1);				/* mask to 16 bits on DTSS						*/
			exp = exp - 1;
		}
		fval1 = fval1 | shl(sign, 15);
		fval2 = ((fval2 & (fixed) 0xFF80) | exp);		/* and fval2									*/
	}
}
	
/*  The following subroutine is used by the token scanner in the conversion
.   to floating point constants.  During compilation, sequences of digits
.   that represent floating point constants are converted to an internal
.   32 or 36 bit form.
.  
.   The routine 'FDIVIDE' is used to fractionally divide two numbers passed 
.   as arguments.  A value equal to the fractional division is returned.
.  
.   Obviously, the first number must be less than the second number for
.   the division to work properly.
*/

static	fixed	c1, c2;							/* global variables which hold answer			*/

static	void	fdivide(						/* perform fractional divide on 32-bit ints		*/
	ufixed	a1,
	ufixed	a2,
	ufixed	b1,
	ufixed	b2)
{
	static	fixed	i;							/* internal										*/
	
	c1 = 0; c2 = 0;								/* initialize answer							*/
	
	for (i = 0; i <= 30; i++) {					/* 31 ties, as these are 31-bit precision		*/
		a1 = shl(a1, 1);						/* shift left (DTSS mask to 16 bits)			*/
		if ((a2 & (fixed) 0x4000) != 0) a1 = (a1 | 1);
		a2 = (shl(a2, 1) & (fixed) 0x7FFF);		/* keep to 15-bit								*/
		c1 = shl(c1, 1);						/* (Dtss mask to 16 bits)						*/
		if ((c2 & (fixed) 0x4000) != 0) c1 = (c1 | 1);
		c2 = (shl(c2, 1) & (fixed) 0x7FFF);		/* keep to 15-bit for compares					*/
		if ((a1 > b1) || ((a1 == b1) && (a2 >= b2))) {	/* can subtract							*/
			c2 = (c2 | 1);						/* set answer bit								*/
			if (b2 > a2) a1 = a1 - 1;			/* is a borrow									*/
			a2 = ((a2 - b2) & (fixed) 0x7FFF);	/* keep to 15-bit quantity						*/
			a1 = a1 - b1;						/* bleep										*/
		}
	}
}


/*  Main token scanner:
.  
.   The routine 'SCAN' is called to scan the next token from the source
.   file.
.  
.   The SCAN routine examines the next character in the source file.
.   If it is a special character (such as  + , *, /, etc.) then the
.   proper token types ('TOKEN') and information ('INFO') is set
.   and SCAN returns (after discarding the single input character).
.  
.   If the next character is a digit, the digits are converted to
.   binary (either fixed or floating point, depending) and the
.   value is returned in 'INFO'.
.  
.   If the next character is an apostrophe, a string constant is scanned
.   and packed into the global array 'NAME'.
.  
.   Otherwise, characters are compacted until a terminal character is
.   found.  The symbol table is consulted for a corresponding entry.
.  
.   If the symbol is found in the table, its token type is returned
.   in 'TOKEN' and its information is returned in 'INFO'.  Otherwise,
.   a token type of 'T.UND'  will be returned.
*/

static	fixed	fstr(							/* called to store number info FSTK -  returns ptr	*/
	fixed	a1,
	fixed	a2)
{
	static	fixed	pt;
	
	pt = fstkptr;								/* current pointer								*/
	
	if (fstk [pt] != (fixed) 0x8000) er_tmnc();	/* too many numeric constants					*/
	
	fstk [pt] = a1; fstk [pt + 1] = a2;			/* store in stack								*/
	
	fstkptr = fstkptr + 2;						/* rotary buffer								*/
	
	if (fstkptr == fstklen) fstkptr = 0;		/* end											*/
	
	return (pt);								/* and return where stored						*/
}
	

/* Insert file processing:
.  
.   The routine 'POPF' is used to pop back up from an insert
.   file into the original program.
.  
.   It is called from within the scanner when an EOF statement or
.   end of file condition is encountered, when inside an insert file.
.  
.   After calling POPF, the scan should be restarted.
.  
.   First our own stack:
*/

static	fixed	ispt;							/* stack pointer - 0 means in source file		*/
static	fixed	filespt;
static	fixed	textspt;
static	fixed	specspt;
static	fixed	nchr;							/* nchr - is fixed								*/
static	fixed	lchr;							/* previous nchr								*/
static	fixed	pchrp, chrbase, chrpt, chrln;	/* macro processing								*/
static	fixed	n_info = b_eol;					/* starts at end of line						*/

static	void	ipush(							/* push word onto stack							*/
	fixed	word)
{
	if (ispt + 5 > istklen) er_tmni();			/* too many nested inserts						*/
	istk [ispt] = word;							/* store word on bottom of stack				*/
	ispt = ispt + 1;							/* incr											*/
}												/* done											*/
	
static	fixed	ipop()							/* recover word									*/
{
	ispt = ispt - 1;							/* decr											*/
	return (istk [ispt]);						/* get word and return it						*/
}
	
static	void	filepush(						/* push file pointer on to stack				*/
	FILE*	word)
{
	if (filespt + 5 > ins_levels) er_tmni();	/* too many nested inserts						*/
	filestk [filespt] = word;					/* store word on bottom of stack				*/
	filespt = filespt + 1;						/* incr											*/
}												/* done											*/
	
static	FILE*	filepop()						/* recover word									*/
{
	filespt = filespt - 1;						/* decr											*/
	return (filestk [filespt]);					/* get word and return it						*/
}
	
static	void	textpush(						/* push text pointer on to stack				*/
	char*	it)
{
	if (textspt + 5 > ins_levels) er_tmni();	/* too many nested inserts						*/
	textstk [textspt] = it;						/* store word on bottom of stack				*/
	textspt = textspt + 1;						/* incr											*/
}												/* done											*/
	
static	char*	textpop()						/* recover word									*/
{
	textspt = textspt - 1;						/* decr											*/
	return (textstk [textspt]);					/* get word and return it						*/
}

static	void	basepush(						/* push base pointer on to stack				*/
	void*	it)
{
	if (inserted_stack_pointer + 5 > ins_levels) er_tmni();
	inserted_stack [inserted_stack_pointer] = it;
	inserted_stack_pointer = inserted_stack_pointer + 1;
}
	
static	void*	basepop()						/* recover word									*/
{
	inserted_stack_pointer = inserted_stack_pointer - 1;
	return (inserted_stack [inserted_stack_pointer]);
}

static	void	specpush(						/* push spec pointer on to stack				*/
	FSSpec	it)
{
	if (specspt + 5 > ins_levels) er_tmni();	/* too many nested inserts						*/
	specstk [specspt] = it;						/* store word on bottom of stack				*/
	specspt = specspt + 1;						/* incr											*/
}
	
static	FSSpec	specpop()						/* recover word									*/
{
	specspt = specspt - 1;						/* decr											*/
	return (specstk [specspt]);					/* get word and return it						*/
}


/* Token scanner: */

/* Scanner:  get next character subroutine
. 
.  Returns the next character in 'NCHR'.  Previous character is kept
.  in 'LCHR' and information about the new character is in 'NINFO'.
*/

#define	saved_nchr		store1
#define	saved_chrb		store2
#define	saved_chri		store3
#define	saved_pchrp		store4

#define	t_lbrkt			118				/* left  bracket								*/
#define	t_rbrkt			119				/* right bracket								*/
#define	t_lbra			120				/* left  brace									*/
#define	t_rbra			t_end			/* right brace									*/

static	fixed	info_table[] = {
b_spec + t_eof + b_comnt, 0, 0, 0, 0, 0, 0, 0, 0, b_spa, b_eol + b_comnt, 0, 0, b_eol + b_comnt, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, b_spa, b_relop + b_not, 0, b_symb, b_symb + b_comnt, b_opr + o_fmu, b_opr + o_and, 0, b_spec + t_lpar, b_spec + t_rpar, 
b_opr + o_times + b_comnt, b_opr + o_plus, b_spec + t_comma, b_opr + o_minus, b_symb, 
b_opr + o_div + b_comnt  , b_digit + b_symb, b_digit + b_symb, b_digit + b_symb, b_digit + b_symb, 
b_digit + b_symb, b_digit + b_symb, b_digit + b_symb, b_digit + b_symb, 
b_digit + b_symb, b_digit + b_symb, b_spec + t_colon, b_spec + t_semi, b_relop + b_lt, 
b_relop + b_eq, b_relop + b_gt, 0, 0, b_symb, b_symb, b_symb, b_symb, b_symb, 
b_symb, b_symb, b_symb, b_symb, b_symb, b_symb, b_symb, b_symb, b_symb, b_symb, 
b_symb, b_symb, b_symb, b_symb, b_symb, b_symb, b_symb, b_symb, b_symb, b_symb, 
b_symb, b_spec + t_lpar, b_opr + o_or, b_spec + t_rpar, b_relop + b_not, b_symb, 0, b_symb + b_lcase, 
b_symb + b_lcase, b_symb + b_lcase, b_symb + b_lcase, b_symb + b_lcase, 
b_symb + b_lcase, b_symb + b_lcase, b_symb + b_lcase, b_symb + b_lcase, 
b_symb + b_lcase, b_symb + b_lcase, b_symb + b_lcase, b_symb + b_lcase, 
b_symb + b_lcase, b_symb + b_lcase, b_symb + b_lcase, b_symb + b_lcase, 
b_symb + b_lcase, b_symb + b_lcase, b_symb + b_lcase, b_symb + b_lcase, 
b_symb + b_lcase, b_symb + b_lcase, b_symb + b_lcase, b_symb + b_lcase, 
b_symb + b_lcase, b_spec + t_lbra, b_opr + o_or, b_spec + t_rbra, b_relop + b_not, 0, b_spec + t_eof + b_comnt};
	
static	void	get_char()
{
	lchr = nchr;								/* save previous character						*/
	if (pchrp != 0) {							/* we are in a literal string					*/
		if (chrln == 0) {						/* end of this literal string - back down		*/
			nchr    = saved_nchr [pchrp];		/* restore NCHR									*/
			chrbase = saved_chrb [pchrp];
			chrpt   = saved_chri [pchrp];
			chrln   = shr(chrpt,8);
			chrpt   = (chrpt & 0x00FF);
			rel (pchrp);						/* release this level of literal stacking		*/
			pchrp = saved_pchrp [pchrp];		/* note rel does not use STORE4					*/
		}
		else {									/* next character comes from literal			*/
		    nchr = slit_text(chrbase, shr(chrpt, 1));
			if (chrpt & 1) nchr = shr(nchr, 8);	/* get upper half								*/
			chrpt = chrpt + 1;					/* increment char pointer						*/
			chrln = chrln - 1;					/* decrement length								*/
		}										/* if char comes from this level of literal		*/
	}											/* of literals instead of file					*/
	else nchr = gc();							/* get next character from file					*/
		
	/* common routine */
	
	if (nchr == -1) {
        nchr = a_eof;
        n_info = info_table [nchr];
    }
    
    else {
        nchr &= 0xFF;                           /* mask off random bits from literal processing */
        
        if (nchr >= 0x80)                       /* UTF-8; only allowed in string literals       */
            n_info = 0;
        else
            n_info = info_table [nchr];         /* get type bits								*/
    }
}												/* of get.char									*/
	
/*  Scanner:  number scanner and evaluator.
.  
.   The following routines assist in the conversion of fixed to
.   floating point numbers.   
. 
.   The routine 'MTEN' is called to multiply the global variables
.   VAL1 and VAL2 by 10.   
.   
.   VAL1 is a 16-bit number, VAL2 is a 15-bit number.  Placed end
.   to end, they form a 31-bit integer.
.  
.   The routine 'VAL' converts a sequence of digits (12345678) to
.   a VAL1/VAL2 31-bit integer.
*/

static	fixed	val1, val2;						/* double value returned by 'VAL'				*/

static	void	mten()							/* procedure to multiply (VAL1, VAL2) by 10		*/
{
	static	fixed	t1, t2;
	
	if ((val1 & (fixed) 0xFC00) != 0) er_tmd();
		
	t1 = (shl(val1, 3) | ((shr(val2, 12) & 7)));	/* shift three times							*/
	t2 = (shl(val2, 3) & (fixed) 0x7FFF);			/* and three times to LSB						*/
	
	val1 = val1 + val1;							/* now times 2									*/
	val2 = val2 + val2;
	
	if ((val2 & (fixed) 0x8000) != 0) val1 = (val1 | 1);
	val2 = (val2 & (fixed) 0x7FFF);
	val1 = val1 + t1;							/* now form times 10							*/
	val2 = val2 + t2;							/* and here										*/
	
	if ((val2 & (fixed) 0x8000) != 0) val1 = val1 + 1;
	val2 = val2 & (fixed) 0x7FFF;				/* and mask										*/
}
	
static	void	val()							/* and scan value of 10							*/
{
	val1 = 0; val2 = 0;							/* initialize answer							*/
	while ((n_info & b_digit) != 0) {
		mten();									/* multiply current value by 10					*/
		val2 = val2 + nchr - a_0;				/* and add in new digit							*/
		if ((val2 & (fixed) 0x8000) != 0) val1 = val1 + 1;	/* carry propagation							*/
		val2 = (val2 & (fixed) 0x7FFF);			/* and keep 15-bits only here					*/
		get_char();
	}
}
	
/* Get next character and check for the end of the line.
.  If we are at the end of the line, then the next character
.  on the new line will be returned (after the optional line
.  number and an optional space).
*/

static	void	eol_ck()
{
	fixed	plchr;
	
	if ((n_info & b_eol) != 0) {				/* check end of line							*/
		plchr = lchr;							/* save character at end of line				*/
		get_char();								/* first character of next line					*/
		line_no++;								/* increment line number						*/
		if (nchr == a_sp) get_char();			/* scan off optional blank						*/
		lchr = plchr;							/* restore lchr to end of previous line			*/
	}
	else get_char();							/* if not end of line, scan one char			*/
}
	

/*  Scanner:  main routine */
static	void	popfile();						/* forward to popfile							*/

static	void	scan()							/* main token scanner							*/
{
	static	fixed	i, j;						/* internal to 'scan'							*/
	static	fixed	point, point1;				/* temps used in literal processing				*/
	
	scan_start:;								/* return here									*/
	
	name_pt = 0;								/* assume initially that token is not symbol	*/
	
	/* ignore what should be ignored */
	
	while ((n_info & (b_spa + b_eol)) != 0) {
		eol_ck();
	}
		
	/* check for digit */
	
	if ((nchr == a_period) || ((n_info & b_digit) != 0)) {	/* number										*/
		static	fixed	man1, man2, frac1, frac2, lzer1, lzer2, inf1, inf2;
		static	fixed	exp, infexp;
		
		if (nchr == a_0)						/* look for leading 0 for 0xFFFF				*/
		{
			get_char();							/* toss leading 0								*/
			
			if (nchr == a_x || nchr == l_x)		/* handle hex format							*/
			{
				get_char();
				
				if (((n_info & b_digit) == 0)
				&&  (nchr < a_a || nchr > a_f)
				&&  (nchr < l_a || nchr > l_f))
					er_ifnc();
					
				info = 0;
				
				while (((n_info & b_digit) != 0)
				||  (nchr >= a_a && nchr <= a_f)
				||  (nchr >= l_a && nchr <= l_f))
				{
					if ((info & 0xF000) != 0) er_tmd();
					
					if (nchr >= l_a)
						info = shl(info,4) + (nchr - l_a) + 10;
						
					else if (nchr >= a_a)
						info = shl(info,4) + (nchr - a_a) + 10;
						
					else
						info = shl(info,4) + (nchr - a_0);
					
					get_char();
				}
				token = t_const;
				return;
			}
		}

		val();									/* scan off decimal number						*/
		if ((nchr == a_period) || (val1 > 1)) {	/* see if fractional part						*/
			inf1 = val1; inf2 = val2;			/* save the integer mantissa part				*/

			if (nchr == a_period) {				/* find fractional part							*/
				get_char(); val1 = 0; val2 = 1;	/* set up to compute leading zeroes				*/
				while (nchr == a_0) {
					mten();						/* multiply val by ten							*/
					get_char();
				}
				lzer1 = val1; lzer2 = val2;		/* save leading zero indicator					*/

				val();							/* now scan off fractional parg					*/
				frac1 = val1; frac2 = val2;		/* save fractional part							*/

				val1 = 0; val2 = 1;				/* now compute denominator						*/
				while ((val1 < frac1) || ((val1 == frac1) && (val2 <= frac2))) {
					mten();						/* compute denominator - power of ten			*/
				}

				fdivide (frac1, frac2, val1, val2);	/* now divide - compute fraction				*/

				man1 = c1; man2 = c2;			/* save result of fractional divide in man1, man2	*/
				man2 = (shr(man2, 2) | (shl(man1 & 3, 13)));	/* shift right by two to avoid overflow			*/
				man1 = shr(man1, 2);			/* reduce it									*/

				exp = -29;						/* start here									*/
				while ((lzer1 < man1) || ((lzer1 == man1) && (lzer2 <= man2))) {
					lzer1 = shl(lzer1, 1);
					if ((lzer2 & (fixed) 0x4000) != 0) lzer1 = (lzer1 | 1);
					lzer2 = (shl(lzer2, 1) & (fixed) 0x7FFF);
					exp = exp + 1;				/* bring up expoent								*/
				}

				fdivide (man1, man2, lzer1, lzer2);	/* and fractional divide						*/

				man1 = c1; man2 = c2;			/* and store here								*/
			}
			else {								/* no fractional part							*/
				man1 = 0; man2 = 0; exp = 0;
			}
				
			/* now normalize the number: */
			
			if ((inf1 | inf2 | man1 | man2) != 0) {	/* must normalize								*/
				infexp = 64 + 30;				/* and this is where we start					*/
				while ((inf1 & (fixed) 0x4000) == 0) {	/* shift until normalized						*/
					inf1 = shl(inf1, 1);		/* shift all the bits left						*/
					if ((inf2 & (fixed) 0x4000) != 0) inf1 = (inf1 | 1);
					inf2 = (shl(inf2, 1) & (fixed) 0x7FFF);
					if (exp >= 0) {										/* if down to mantissa level,  then shift it in	*/
						if ((man1 & (fixed) 0x8000) != 0) inf2 = (inf2 | 1);
						man1 = shl(man1, 1);
						if ((man2 & (fixed) 0x4000) != 0) man1 = (man1 | 1);
						man2 = (shl(man2, 1) & (fixed) 0x7FFF);
					}
					infexp = infexp - 1;		/* reduce exponent								*/
					exp = exp + 1;				/* and keep track of separate mantissa point	*/
				}
				if (infexp < 0) er_tmd();		/* too many digits								*/
				inf2 = ((shl(inf2, 1) & ((fixed) 0xFF80)) | infexp);	/* or in exponent field							*/
			}

			info = fstr (inf1, inf2);			/* store pon stack								*/
			token = t_fconst;					/* and is pointer								*/
		}
		else {									/* fixed point constant							*/
			token = t_const; info = (val2 | shl(val1, 15));	/* set up information							*/
		}
		return;
	}
		
	/* check for leading symbol character (other than digit)	*/
	
	if ((n_info & b_symb) != 0) {				/* symbol										*/
		point = 0;								/* initialize pointer into name					*/
		point1 = 1;								/* set false to exit loop						*/
		while (point1 & 1) {					/* accumulate symbol - note how we exit (this is fast!)	*/
			if ((n_info & b_lcase) != 0) nchr = nchr - (l_a - a_a);	/* convert to uppercase							*/
			get_char();							/* get next character - move NCHR to LCHR		*/
			if ((n_info & b_symb) != 0) {		/* means two letters							*/
				if ((n_info & b_lcase) != 0) nchr = nchr - (l_a - a_a);
				point = point + 1;				/* one full word								*/
				name [point] = (lchr | shl(nchr, 8));
				get_char();						/* get next character							*/
				if ((n_info & b_symb) == 0) point1 = 0;	/* exit loop									*/
				if (point == max_sym_len - s_length) point1 = 0;	/* break out on symbol too long					*/
			}
			else {
				point = point + 1;				/* treat as full word - zeroes in upper half	*/
				name [point] = lchr;			/* save last character							*/
				point1 = 0;
			}
		}
			
		name [0] = point;						/* fill in length field	 (length on words)		*/
		
		point = s_lookup();						/* look the symbol up in the symbol table		*/
		
		if (point == 0) {						/* undefined symbol								*/
			token = t_und;
			info = name_pt;						/* return pointer here							*/
			ufsp = name_pt;						/* and save here also							*/
			dcl_symbol = 0;						/* we aren't in a DCL							*/
			return;
		}										/* of undefined symbol							*/
			
		token = gtok (name_pt);					/* get token type for this symbol				*/
		info = stable (name_pt + s_locn);		/* and pick up info								*/
		
		if ((! (dcl_symbol & 1)) && (gclas (name_pt) == s_automatic) && (depth < cur_proc_dpt))
			er_crg();							/* cannot reference global automatic			*/
			
		if ((token == t_lit) && ((! (dcl_symbol & 1)) || (depth == s_depth))) {	/* if literal, then start it up					*/
			point1 = get();						/* get a block for stacking literal nesting		*/
			saved_nchr  [point1] = nchr;		/* saved delimiting character					*/
			saved_chrb  [point1] = chrbase;		/* saved buffer pointer							*/
			saved_chri  [point1] = chrpt | shl(chrln,8);	/* saved buffer pointer							*/
			saved_pchrp [point1] = pchrp;		/* saved back pointer							*/
			pchrp   = point1;					/* new back pointer								*/
			chrbase = info;						/* literal base									*/
			chrpt = 0;							/* this is relative byte pointer to first character	*/
		    chrln = stable(chrbase);
			nchr = lchr; get_char();			/* preserve LCHR and get first lit				*/
			
			goto scan_start;					/* and start up scan agin						*/
			
		}										/* of literal call								*/
			
		else {									/* label is not a literal						*/
			dcl_symbol = 0;						/* we aren't in a DCL							*/
			if ((token == t_stmt) && (info == s_declare)) dcl_symbol = 1;	/* next token should be a variable name; necessary - name is scanned by STMT before DCL processing	*/
			if (token != t_eof) return;			/* return directly with that					*/
			if (ispt == 0) return;				/* if eof of original source then return		*/
			popfile();							/* go back up one file							*/
			
			goto scan_start;					/* and retry									*/
			
		}
	}											/* end of symbol scan							*/
		
	
	/* Token scanner (cont):
	.
	.   Check for special character (comma, semicolon, paren...)	*/
	
	if ((n_info & b_spec) != 0) {				/* special character							*/
		token = (n_info & b_mask);
		get_char();								/* skip to next character						*/
		if (token != t_eof) return;				/* return now if not eof						*/
		if (ispt == 0) return;					/* return also if end of source file			*/
		popfile();								/* pop up to source file						*/
		goto scan_start;						/* and start scan again							*/
	}
		
	/* check for "ddddd" (octal constants)			*/
	
	if (nchr == a_quote) {						/* octal constant								*/
		get_char();								/* scan off double quote						*/
		if ((nchr == a_h) || (nchr == l_h)) {	/* detect "hex"									*/
			i = 4; j = a_a + 6;					/* hex: shift count, limits						*/
			get_char();
		}
		else {
			i = 3; j = a_0 + 8;					/* else set up octal limits						*/
		}
		info = 0;
		while (nchr != a_quote) {				/* and wait for next quote						*/
			if ((info & 0xE000) != 0) er_tmd();
			if (_ILE_((nchr - l_a), (a_z - a_a))) nchr = nchr - (l_a - a_a);	/* uppercase it									*/
			if ((nchr < a_0) || (nchr >= j)) {	/* error										*/
				er_ifnc();
				nchr = a_quote;					/* exit loop									*/
			}
			else {								/* legal digit:									*/
				if (nchr >= a_a)
					nchr = nchr - a_a + 10;
				else nchr = nchr - a_0;			/* get binary equivalent						*/
				info = (shl(info, i) | nchr);	/* and set those bits							*/
				get_char();
			}
		}
		token = t_const;
		get_char();								/* skip the second quote						*/
		return;
	}
		
	
	/*  String constants:
	.
	.   When a string constant is encountered, it is stored in the array
	.   called 'NAME'.
	.
	.   The character information is written out to the object file
	.   at a later time.
		*/
	
	if (nchr == a_apost) {						/* is string constant							*/
		get_char();								/* skip over first apostrophe					*/
		get_char();								/* move next one to lchr						*/
		i = 0;									/* initialize count								*/
		point = 1;								/* start words here								*/
		while (((lchr == a_apost) && (nchr == a_apost))
		||     ((lchr != a_apost) && (lchr != a_nul) && (lchr != a_eof))) {	/* Null means end of line						*/
			if ((lchr == a_apost) && (nchr == a_apost)) get_char();	/* convert double apostrophes to single			*/
			if (i >= 128) { er_sctl(); i = 0; point = 1; }
			if (i & 1) { name [point] = (name [point] | shl(lchr, 8)); point = point + 1; }
			else name [point] = lchr;			/* clear upper half								*/
			i = i + 1;
			get_char();							/* move nchr to lchr							*/
		}
		name [0] = i;							/* number of bytes								*/
		if (lchr != a_apost) 
			er_map();							/* missing appost								*/
		token = t_sconst;						/* token is string constant						*/
		return;									/* end of string constant						*/
	}											/* of '											*/
		
	
	/* check for operator							*/
	
	if ((n_info & b_opr) != 0) {				/* operator										*/
		token = t_opr;
		info = (n_info & b_mask);
		get_char();								/* skip to next character						*/
		
		if ((info == o_and) && (n_info == (b_opr + o_and)))
			get_char();							/* accept c-style && for &						*/
			
		if ((info == o_or) && (n_info == (b_opr + o_or)))
			get_char();							/* accept c-style || for |						*/
			
		if ((lchr == a_slash) && (nchr == a_slash)) {	/* handle -style // comments			*/
			get_char();							/* get next character into 'nchar'				*/
			while (((n_info & b_eol) == 0)		/* fast loop to find end of line				*/
			&&     ( n_info != b_spec + t_eof + b_comnt))
				get_char();
			
			eol_ck();							/* handle eol									*/
			goto scan_start;					/* start all over again							*/
		}
			
		if ((lchr == a_slash) && (nchr == a_star)) {	/* special check for / * denoting comment		*/
			static	fixed	templine_no;
			templine_no = line_no;				/* save the line number for error messages		*/
			get_char();							/* get next character into 'nchar'				*/
			while (((lchr != a_slash) || (nchr != a_star)) && (nchr != a_eof)) {	/* check for nested comm, eof					*/
				if (nchr == a_dollar) {			/* handle $ in comment field					*/
					get_char();					/* get character after $						*/
					
					if ((nchr != 'e')			/* disallow $e since it is a variable name...	*/
					&&  (nchr != 'E'))
						set_flags (nchr);		/* set compile-time flags based on character*/
				}
				if ((lchr == a_star) && (nchr == a_slash)) {
					get_char();					/* scan past slash								*/
					goto scan_start;			/* start all over again							*/
				}
				eol_ck();						/* get the next character and check for eol		*/
				while ((n_info & b_comnt) == 0) {	/* fast loop to find /, *, eof, eol, $			*/
					get_char();					/* scan comment out of literal ?				*/
				}
			}
			line_no = templine_no;				/* restore the line number where we started		*/
			er_ntc();
			goto scan_start;					/* treat the end of file normally				*/
		}
			
		return;
	}
		
	
	/* check for relational operator				*/
	
	if ((n_info & b_relop) != 0) {
		static	fixed	notflag;				/* flag to indicate negated relation			*/
		
		if ((n_info & b_not) != 0) {			/* check for leading not						*/
			notflag = 1;
			get_char();
		}
		else notflag = 0;
		
		info = (b_relop | b_not);				/* don't allow further nots						*/
		
		while ((n_info & info) == b_relop) {	/* continue through relationals that we have not yet got	*/
			info = info + (n_info & b_mask);	/* accumulating condition bits					*/
			get_char();
		}
		
		info = (info & b_rel);					/* mask to relation bits						*/
		
		if ((info   == b_eq          )	        /* accept c-style '=='							*/
		&&  (n_info == b_relop + b_eq))
			get_char();
					
		if (info == (b_eq | b_lt | b_gt)) 
			er_ifm();
		
		token = t_opr;							/* we have an operator (although it is relational)	*/
		
		if (notflag != 0)
		{
			if ((info & b_rel) == 0) {			/* check for unary not							*/
				info = o_not;
				return;
			}
			
			else if (info == b_eq)				/* accept != for <>								*/
				info = (b_lt | b_gt);
			
			else
				info = b_rel - info;			/* else simply negate relation					*/
		}
		
		info = o_eq + (info - b_eq);			/* compute operator type from relation bits		*/
		
		return;
	}
		
	/* illegal character - give error				*/
	
	token = -1;									/* invalidate the token (it means nothing anyway)	*/
	er_ifm();
	get_char();									/* bypass illegal character						*/
	
	goto scan_start;							/* and ignore it								*/
}


/* Expression scanner:  Definitions
. 
.  The expression scanner scans the current expression and produces
.  a tree representation of it (essentially a binary tree).  This permits
.  lots of optimization, although very little is actually done by this
.  routine.  Arithmetic on constants is handled (a + (1 + 2) generates the
.  same tree as a + 3).  The format of the tree is as follows: */

#define	x_node			store1					/* type of the node								*/
#define	x_arg1			store2					/* first argument pointer						*/
#define	x_arg2			store3					/* second argument pointer						*/
#define	x_info			store4					/* extra information							*/
#define	x_get			get						/* storage allocation routines					*/
#define	x_rel			rel

static	fixed	mvtype;							/* set to 0 or T.VAR for fixed, nonzero or T.FVAR for float	*/

/* $$The procedure LOOKUP_PARM searches the formal parameter list of the
.   current procedure and all its outer procedures (those with a greater
.   lexical scope) for the passed variable.  If it is found, the parameter
.   number of that variable is returned and the global variable PARM_KEY
.   contains the key of the procedure it is a formal parameter of.  If it
.   isn't found, a -1 is returned. */

static	fixed	parm_key;						/* set to key of formal parm's procedure by LOOKUP_PARM	*/

static	fixed	lookup_parm(					/* lookup a parameter definition				*/
	fixed	token, 								/* token of variable to lookup					*/
	fixed	info)								/* location of variable to lookup				*/
{
	static	fixed	parm;						/* resultant parameter number of variable		*/
	static	fixed	cptr;						/* current proc parameter pointer				*/
	static	fixed	clim;						/* lower limit of current proc's record			*/
	static	fixed	cdepth;						/* current depth								*/
	
	cdepth = s_depth;							/* start at current procedure level (S.PDEF (cdepth) = CUR.PROC.DEF here)	*/
	while ((cdepth > 0) && (s_pdef [cdepth - 1] == s_pdef [cdepth])) {	/* skip over BEGINs								*/
		cdepth = cdepth - 1;
	}
		
	parm = -1;									/* assume this variable is the parameter of NO procedure	*/
	cptr = clim;								/* get into the loop							*/
	while ((s_pdef [cdepth] != -1) && (_ILE_(cptr, clim))) {	/* search for the parameter at all levels		*/
		cptr = s_pdef [cdepth] + p_parm;		/* point to parameters of current proc			*/
		clim = cptr - shl(stable(s_pdef [cdepth] + p_args), 1);	/* lower limit of current proc record			*/
		
		while ((_IGT_(cptr, clim))				/* look thru parms until we find this variable	*/
		&& ((token != stable (cptr + p_ptyp) && 0x00FF & 1) || (info != stable (cptr + p_ploc)))) {
			cptr = cptr - 2;					/* look at next entry							*/
		}
			
		if (_IGT_(cptr, clim)) {				/* if still in range, this is a formal parm of the current proc	*/
			parm = shr(s_pdef [cdepth] + p_parm - cptr, 1);	/* save formal parameter number					*/
			parm_key = shr(stable (s_pdef [cdepth] + p_key), 1);	/* and key										*/
		}
		else {									/* not in range - prepare to look at the next level	*/
			cdepth = cdepth - 1;				/* go up a level								*/
			while ((cdepth > 0) && (s_pdef [cdepth - 1] == s_pdef [cdepth])) {	/* skip over BEGINs								*/
				cdepth = cdepth - 1;
			}
		}										/* of look at the next level					*/
	}											/* of searching for parameter at all levels		*/
		
	return (parm);								/* return formal parameter number				*/
}
	

/*  $$The routine EXPR is called to scan an arithmetic or logical expression
.    from the source file.  It is converted into an 'expression' tree
.    for further processing .
.  
.    EXPR is recursive in nature and uses the internal push down
.    stack (PUSH and POP) for recursive calls instead of the
.    built-in system PDL.  Otherwise, prohibitive amounts of
.    storage (PDL) are required for the compiler.
*/

#define	b_monad		256							/* indicates monadic allowed					*/
#define	b_dyad		512							/* indicated dyadic allowed						*/
	
static	fixed	op_table[] = {					/* and this is table							*/
	b_monad + 1, b_monad + 1, b_monad + 1, b_monad + 1, b_monad + 1, b_monad + 1, b_monad + 1, b_monad + 1, 
	0, 0, 0, 0, 0, 0, 0, 0, 					/* spare										*/
	b_monad + 1, b_monad + 1, 0, b_monad + 1, 	/* int, read, spare, addr						*/
	b_monad + 1, b_monad + 1, b_monad + 1, 0, 	/* shr1, shl1, rot8								*/
	b_monad + 1, 0, 0, 0, 						/* not											*/
	b_monad + b_dyad + 3, b_monad + b_dyad + 3, 	/* minus, plus									*/
	0, 0, 0, 0, 0, 0, 							/* shr shl rot pat min max - elsewhere			*/
	b_dyad + 2, b_dyad + 2, b_dyad + 2, b_dyad + 2, b_dyad + 2, 	/* div, times, fmu, fdi, mod					*/
	b_dyad + 5, b_dyad + 5, b_dyad + 5, 		/* and or xor									*/
	0, 0, 0, 0, 0, 								/* not used										*/
	b_dyad + 4, b_dyad + 4, b_dyad + 4, b_dyad + 4, b_dyad + 4, b_dyad + 4, b_dyad + 4, 	/* rels											*/
	b_dyad + 4, b_dyad + 4, b_dyad + 4, b_dyad + 4};	/* integer relational ops						*/
	
static	fixed	expr()
{
	static	fixed	tree, prevtree, prior;
	
	_label(expexit) _label(expstart)			/* goto EXPSTART to start, goto EXPEXIT to return	*/
	
	static	fixed	subtree1, subtree2, arg1, arg2;
	static	fixed	dkey, ppoint;				/* data key, proc pointer for proc data was dcl'd in	*/
	static	fixed	parm;						/* parameter number								*/
	static	fixed	opr;						/* holder for operator type						*/
	static	fixed	ptr;						/* alias list pointer							*/
	static	fixed	the_const;
	static	fixed	scanarg_typ;				/* set type to 0 (fixed) or 1 (float) before entering SCANARG	*/
	
	
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
	
	prior = prior_max;							/* start with maximum priority					*/
	tree = 0;									/* no tree yet									*/
	push (0);									/* push a zero for EXPEXIT						*/
	goto expstart;								/* and start the expression scan				*/
	explab0: return (tree);						/* jump back here, use normal return to return ptr to tree	*/
	
	/* The procedure 'SCANARG' is used to scan off an argument that is
	.  enclosed in parenthesis.   (Such as when processing the 'READ'
	.  token or when scanning a subscript.)
	.  
	.  The SCANARG routine returns a pointer to the scanned tree in the global
	.  variable SUBTREE1.
	*/
	
	scanarg:;									/* uses standard push/goto to activate SCANARG	*/
	if (token != t_lpar) 
		er_ifm();
	scan(); push (tree); prior = prior_max;
	push (mvtype); mvtype = scanarg_typ;		/* set up correct type							*/
	
	push (1); goto expstart; explab1:;			/* start parsing the expression					*/
	
	mvtype = pop();								/* restore										*/
	prior = 0; subtree1 = tree; tree = pop();
	if (token != t_rpar) er_ifm();
	scan();										/* skip final paren								*/
	goto expexit;								/* and return to point of call - tree is in global var SUBTREE1	*/
	
	/* $$Expression scanner - priority zero:
	. 
	.   If the priority is zero we handle variables, constants, subscripted
	.   arrays, procedure calls, and expressions enclosed in parens.
	*/
	
	expstart:;									/* enter here to start scan						*/
	if (tree != 0) prevtree = tree;				/* save previous tree (we need this to detect ADDR(data)) - skip many zeroes	*/
		
	if (prior == 0) {
		
		if ((token < t_lit) || (token == t_fxdata) || (token == t_fldata)) {	/* var, data									*/
			tree = x_get();
			x_arg1 [tree] = (token | shl(gclas (name_pt), 8)); x_info [tree] = info;
			x_node [tree] = x_var;
			if ((token & t_fvar) != 0) lchk();	/* make sure line no in file					*/
				
			if ((cur_proc_def != -1) && (cur_proc_def != -2)/* if in a proc, but not a WHEN					*/
			&& (token == t_parr)) {				/* fixed array? - watch out for ADDR (for SCON)	*/
				if ((prevtree != 0) && (x_node [prevtree] == x_monad) && (x_info [prevtree] == o_adr)) {	/* Addr of fixed array?							*/
					parm = lookup_parm (token, info);	/* see if this is the formal parameter of any procedure	*/
					if (parm != -1) {									/* if it is a formal parameter of a proc		*/
						emit (t_address);								/* emit addr of parameter code					*/
						emit (parm_key);								/* emit key of proc this is a formal parameter of	*/
						emit (parm);									/* and the parameter number						*/
					}													/* of formal parameter of a proc				*/
				}								/* of ADDR of fixed array						*/
			}									/* of fixed array								*/
				
			if ((token == t_fxdata) || (token == t_fldata)) {	/* data? - see if it can swap					*/
				if ((prevtree != 0) && (x_node [prevtree] == x_monad) && (x_info [prevtree] == o_adr)) {	/* Addr of data?								*/
					dkey = gdkey (name_pt);		/* get data key									*/
					ppoint = s_pdef [depth];	/* get pointer to procedure it was defined in	*/
					if ((ppoint != -1) && (_ILT_(info, extern_base)))/* if defined within a procedure				*/
						set_stable (ppoint + p_dswp, stable (ppoint + p_dswp) | shl(1, dkey));	/* this data statement cannot swap				*/
				}								/* of ADDR of data								*/
			}									/* of data										*/
				
			if (token & 1) {					/* scan subscript								*/
				arg1 = name_pt;					/* save pointer to variable						*/
				scan();							/* skip equals									*/
				if (token != t_lpar) er_msub (arg1);	/* missing subscr								*/
				scanarg_typ = t_var;
				
				push (2); goto scanarg; explab2:;	/* and scan expr in parens						*/
				
				x_arg2 [tree] = subtree1;		/* and this is subscript						*/
			}
			else scan();						/* else just skip over it						*/
			goto expexit;						/* return from EXPSTART							*/
		}										/* of variable									*/
			
		if (token == t_lpar) {					/* expression enclosed in parens				*/
			scanarg_typ = mvtype;
			
			push (3); goto scanarg; explab3:;	/* scan same type								*/
			
			tree = subtree1;					/* and return in tree							*/
			goto expexit;						/* and return									*/
		}
			
		if ((token == t_const) || (token == t_fconst)) {	/* constant										*/
			tree = x_get();
			x_info [tree] = info; x_node [tree] = x_const;
			if (token == t_const)
				x_arg1 [tree] = t_var;
			else {
				lchk();
				x_arg1 [tree] = t_fvar;
			}
			scan();
			
			goto expexit;						/* and return tree								*/
			
		}
			
		/* $$Dyadic operators - special form:
		.
		.  The functions SHR, SHL, and ROT are treated as dyadic
		.  operators by the compiler.  Instead of the normal form
		.  (<term> <operator> <term>), their format is:
		.  <operator> (<term>, <term>).
		.
		.  The following section scans off expressions with these special
		.  dyadic operators.
			*/
		
		if (token == t_sdy) {					/* special dyadics - shift, rotate				*/
			tree = x_get();
			x_node [tree] = x_dyad; x_info [tree] = info;	/* dyadic type									*/
			push (mvtype);
			scan();
			if (token != t_lpar) er_ifm();
			scan();
			push (tree);
			prior = prior_max;
			
			push (7); goto expstart; explab7:;
			
			prior = 0; subtree1 = tree;			/* save subtree									*/
			tree = pop(); x_arg1 [tree] = subtree1;
			if (token != t_comma) er_ifm(); scan();
			push (tree); prior = prior_max;
			
			push (8); goto expstart; explab8:;
			
			prior = 0; subtree2 = tree;
			tree = pop();
			x_arg2 [tree] = subtree2;
			if (token != t_rpar) er_ifm();
			scan();
			mvtype = pop();						/* restore type requested						*/
			subtree1 = x_arg1 [tree];			/* get first operand							*/
			if ((x_node [subtree1] == x_const) && (x_node [subtree2] == x_const)
			&& (x_arg1 [subtree1] == t_var) && (x_arg1 [subtree2] == t_var)) {
				arg1 = x_info [subtree1];
				arg2 = x_info [subtree2];		/* pick up constants							*/
				switch (x_info [tree] - o_shr) {	/* perform it									*/
					case 0:
						arg1 = shr(arg1, arg2);
						break;
					case 1:
						arg1 = shl(arg1, arg2);
						break;
					case 2:
						arg1 = rot(arg1, arg2);
						break;
				}
				x_rel (tree);
				x_rel (subtree2);
				tree = subtree1;
				x_info [tree] = arg1;			/* new constant									*/
			}
			goto expexit;
		}
		/* $$Procedure calls:
		. 
		.   For procedure calls, the node block looks like:
		.      X.NODE =  X.PROC
		.      X.ARG1 =  start of linked list of argument blocks (see below)
		.      X.ARG2 =  alias list handle/T.PROC for user proc calls, T.RTP for rtp routine calls
		.      X.INFO =  pointer to entry in S.STACK for this procedure
		.  
		.  
		.   The S.STACK entry for each procedure looks like:
		.      0:  pointer to the location block for the body of this procedure (key!)
		.      1:  number of arguments to pass to this procedure
		.      2:  proc flags/token type of variable returned - T.VAR or T.FVAR
		.      3:  for each argument mentioned above:
		.          a: arg flags/type of variable (T.VAR, T.ARR, etc) that arg
		.             was declared to be inside the procedure definition.
		.          b: relative distance of this variable location from
		.             start of variable area.
		.  
		.  
		.   Each 'argument block' looks like:
		.      if argument was declared to be some sort of array:
		.         X.ARG1 =  token type of array that we are passing to it
		.         X.INFO =  relative variable storage area of array (0)
		.     
		.      if argument was declared to be some sort of fixed or floating variable:
		.         X.ARG1 =  pointer to expression tree to pass to procedure
		.     
		.      in both cases:
		.         X.ARG2 =  link pointer to next argument block
		.         X.NODE =  type of argument required by procedure
			*/
		
		if ((token == t_proc) || (token == t_rtp)) {	/* call											*/
			static	fixed	sptr, pptr;			/* stack pointer, proc pointer					*/
			static	fixed	lim;				/* lower limit of target proc's record, lower limit of current proc's record	*/
			pptr = info;						/* point to procedure block						*/
			sptr = pptr;						/* start stack pointer at top of proc block		*/
			tree = x_get();						/* get a block for this tree					*/
			
			if ((ptr = alias_ptr++) + 8 >= num_alias_structs)
				er_etc();
			
			aliases[ptr].key = stable(pptr + p_key);
			aliases[ptr].num = 0;
			
			x_node [tree] = x_proc;
			x_info [tree] = sptr;				/* pointer into stack							*/
			x_arg1 [tree] = 0;					/* initialize list of arguments					*/
			x_arg2 [tree] = (shl(ptr, 8) | token);	/* save T.PROC or T.RTP plus handle to alias table	*/
			
			scan();								/* scan past procedure identifier				*/
			
			if ((stable (sptr + p_rtyp) & 0x00FF) == t_fvar) lchk();	/* put line # there								*/
				
			/* $$Procedure call scan (continued):
			.
			.   If any arguments are specified, scan them off and link them on
			.   a linked list:	*/
			
			if (stable (sptr + p_args) != 0) {	/* if any arguments, then scan them				*/
				lchk();
				lim = sptr + p_parm - shl(stable (sptr + p_args), 1);	/* lower limit									*/
				if (token != t_lpar) er_nea();
				token = t_comma;				/* pretend comma encountered					*/
				sptr = sptr + p_parm;			/* pointer to first argument					*/
				
				while (token == t_comma) {		/* scan off all arguments						*/
					scan();						/* scan off comma or open paren					*/
					if (sptr <= lim) {									/* too many										*/
						er_tma(); sptr = lim + 2;						/* fudge to continue ok							*/
					}
					subtree1 = x_get();			/* get a block for this argument				*/
					x_arg2 [subtree1] = x_arg1 [tree];	/* link it on									*/
					x_arg1 [tree] = subtree1;	/* to list of args								*/
					
					if ((stable (sptr + p_ptyp) & 0x00FF) == t_unda) {
						er_irec(); set_stable (sptr + p_ptyp, t_var);	/* set to fixed									*/
					}
						
					x_node [subtree1] = (stable (sptr + p_ptyp) & 0x00FF);	/* get type of arg reqd							*/
					if ((x_node [subtree1] & 1) && (token == t_locat)) {	/* 'location and  for array						*/
						x_node [subtree1] = t_var;						/* pass a variable instead - hope location is ok	*/
						scan();											/* skip 'location'								*/
					}
					if (! (x_node [subtree1] & 1)) {					/* if not array,  scan expression				*/
						push (tree);									/* on the stack in case there is another		*/
						push (sptr);									/* procedure call lurking down there			*/
						push (pptr);									/* watch out for proc calls						*/
						push (subtree1); push (lim); push (mvtype);
						mvtype = (x_node [subtree1] & t_fvar);
						prior = prior_max;								/* get complete expression						*/
						
						push (9); goto expstart; explab9:;
						
						prior = 0;										/* restore priority level						*/
						mvtype = pop();									/* restore calling type							*/
						lim = pop();									/* restore upper limit							*/
						subtree2 = tree;								/* preserve tree pointer						*/
						subtree1 = pop();								/* restore everything important					*/
						pptr = pop();
						sptr = pop();
						tree = pop();
						x_arg1 [subtree1] = subtree2;					/* point to expression tree						*/
					}
						
					/* handle arguments that are arrays:			*/
					
					else {												/* array - just scan token						*/
						if ((cur_proc_def != -1) && (cur_proc_def != -2)/* if in a proc, but not a WHEN					*/
						&& (token == t_parr)) {							/* and have a FIXED ARRAY token					*/
							parm = lookup_parm (token, info);			/* see if it's a formal parameter of a procedure	*/
							
							if (parm != -1) {							/* if this is a formal parm of a proc			*/
								fixed where;
								
								if (parm_key == shr(stable (cur_proc_def + p_key), 1))/* if a formal parameter of the current procedure	*/
									dkey = 1;							/* only one word used							*/
								else dkey = 2;							/* not a formal parameter of the current proc; two words used	*/
									
								ptr   = shr(x_arg2 [tree], 8);	/* get handle into alias list for this call		*/
								where = aliases[ptr].num;
								
								if ((where + dkey + 8) >= num_info_spaces)
									er_etc();
									
								aliases[ptr].info[where] = (shl(parm, 8) | shr(pptr + p_parm - sptr, 1));
								
								if (dkey != 1)
								{								/* if not formal parameter of current proc		*/
									aliases[ptr].info[where    ] |= (fixed) 0x8000;
									aliases[ptr].info[where + 1]  = parm_key;
								}
								
								aliases[ptr].num = where + dkey;
							}											/* of formal parameter of proc					*/
						}												/* of in a proc									*/
						if (token == t_sconst) {						/* string constant								*/
							if ((x_arg2 [tree] & 0x00FF) == t_rtp)		/* runtime package call?						*/
								info = 0;								/* no key associated with it					*/
							else if (_IGE_(stable (pptr + p_key), extern_base))/* external?									*/
								info = stable (pptr + p_key);			/* yes, get external key						*/
							else info = shr(stable (pptr + p_key), 1);	/* no, get rtp location or key					*/
							info = e_scon (info, shr(pptr + p_parm - sptr, 1));	/* emit it and save pointer						*/
						}
						else if ((token == t_fxdata) || (token == t_fldata)) {	/* data? - see if it can swap					*/
							dkey = gdkey (name_pt);						/* get data key									*/
							ppoint = s_pdef [depth];					/* get pointer to procedure it was defined in	*/
							if ((ppoint != -1) && (_ILT_(info, extern_base)))/* if defined within a procedure				*/
								set_stable (ppoint + p_dswp, stable (ppoint + p_dswp) | shl(1, dkey));	/* this data statement cannot swap				*/
						}												/* of data										*/
						else if ((token > t_fldata) || (! (token & 1))) {
							er_atdnm(); token = t_arr;
						}
						x_arg1 [subtree1] = (token | shl(gclas (name_pt), 8));	/* token type of what we are passing			*/
						x_info [subtree1] = info;						/* variable location							*/
						scan();											/* skip over argument							*/
					}													/* passing an array								*/
						
					sptr = sptr - 2;			/* move on to next arg							*/
				}								/* of argument scanning loop					*/
					
				if (token != t_rpar) er_ifm();
				if (sptr != lim) er_nea();
				scan();							/* scan off the close paren						*/
			}									/* scanning arguments							*/
		
			else								/* else no args; scan c-style ()				*/
			{
				if (token == t_lpar)
				{
					scan();
					
					if (token == t_rpar)
						scan();
					else
						er_ifm();
				}
			}
			
			goto expexit;						/* and return									*/
		}										/* procedure calls								*/
			
		if (token == t_und)
			er_ufls (info);
		else er_ifm();
		tree = x_get();
		x_node [tree] = x_const; x_arg1 [tree] = t_var;	/* return fake tree of const 0					*/
		goto expexit;							/* and return									*/
	}
		
	/* $$Expression scanner - nonzero priority:
	. 
	.   Handle expression of the form:
	.
	.     (<monadic operator>) <term> [<operator> <term>]*
	. 
	.   where all of the operators are of the current priority, and the
	.   terms are expressions with all operators of higher priority than
	.   the current priority (lower priority number).  An expression (of
	.   a given priority) then consists of an optional monadic operator
	.   of that priority followed by one or more expressions of higher
	.   priority seperated by dyadic operators of the current priority.
		*/
	
	else {
		tree = 0;								/* nonzero indicates monadic operator found, points to node block	*/
		if (token == t_opr) 					/* check for monadic operator					*/
		if ((op_table [info] & 255) == prior) {	/* of the current priority						*/
			if ((op_table [info] & b_monad) == 0)
			{ 	er_ifm(); info = o_plus; }
			if (info != o_plus) {				/*** ignore unary plus **						*/
				tree = x_get();					/* get a new node								*/
				x_info [tree] = info;			/* type of operator								*/
				x_node [tree] = x_monad;
			}
			scan();								/* scan off monadic operator					*/
		}
			
		/* process monadic operators here:				*/
		
		if (tree != 0) {						/* check for monadic operator scanned			*/
			opr = x_info [tree];				/* get operator type							*/
			push (mvtype);						/* save current type							*/
			if (opr == o_adr) lchk();			/* for good error messages						*/
			if (opr < o_int) { mvtype = t_fvar; lchk(); }
			else if (opr < o_minus) mvtype = t_var;	/* this wants fixed								*/
			push (tree);
			prior = prior - 1;
			push (10); goto expstart; explab10:;
			prior = prior + 1;
			subtree1 = tree;
			tree = pop();
			mvtype = pop();						/* restore requested type						*/
			opr = x_info [tree];				/* pick up type									*/
			if ((x_node [subtree1] == x_const) && (x_arg1 [subtree1] == t_var) && (opr >= o_not)) {
				the_const = x_info [subtree1];	/* get the constant easily accessible			*/
				switch (opr - o_not) {
					case 0:
						the_const = (~ the_const);
						break;
					case 1:
						;
						break;
					case 2:
						;
						break;
					case 3:
						;
						break;
					case 4:
						the_const = (-(the_const));
						break;
				}
				x_rel (tree);					/* release the extra node						*/
				tree = subtree1;				/* make the constant block our complete tree	*/
				x_info [tree] = the_const;		/* store new constant value						*/
			}
			else if ((x_node [subtree1] == x_const) && (x_arg1 [subtree1] == t_fvar) && (opr == o_minus)) {
				x_rel (tree);					/* no longer need -								*/
				tree = subtree1;
				opr = x_info [tree];			/* get pointer into fstk - change sign there	*/
				if (fstk [opr] != 0) fstk [opr] = (fstk [opr] ^ (fixed) 0x8000);
			}
			else x_arg1 [tree] = subtree1;
		}
			
		/* $$Expression scanner - nonzero priority (cont):
		.  
		.   Continue the expression scan here.  A leading monadic operator (if
		.   one) has been scanned.   
		.  
		.   Get the first term and then get successive operator/term pairs
		.   while each are of the current priority.
			*/
		
		else {
			prior = prior - 1;
			
			push (11); goto expstart; explab11:;
			
			prior = prior + 1;
		}
			
		/* get following terms */
		
		while (token == t_opr) {				/* stop whenever we run out of operators		*/
			if ((op_table [info] & 255) != prior) goto expexit;	/* or get wrong priority						*/
			if ((op_table [info] & b_dyad) == 0) {
				er_ifm();
				info = o_plus;
			}
			push (tree);						/* save what we have so far						*/
			push (info);						/* as well as type of operator					*/
			scan();								/* accept this operator							*/
			prior = prior - 1;
			
			push (12); goto expstart; explab12:;
			
			prior = prior + 1;
			opr = pop();						/* restore operator type						*/
			
			/* $$Now optimize for fixed point constant arithmetic:
			.
			.   During compile time, compute fixed point expressions if possible.
			*/
			
			subtree1 = pop();					/* get back what we have so far					*/
			subtree2 = tree;					/* and new part									*/
			arg1 = x_info [subtree1];
			arg2 = x_info [subtree2];
			
			if ((x_node [subtree1] == x_const) && (x_node [subtree2] == x_const)
			&& (x_arg1 [subtree1] == t_var) && (x_arg1 [subtree2] == t_var)
			&& (((opr != o_div) && (opr != o_times)) || (mvtype == t_var)))
			{									/* must be fixed point, not mul or div			*/
				switch (opr - o_minus) {
					case 0:
						arg1 = arg1 - arg2;
						break;
					case 1:
						arg1 = arg1 + arg2;
						break;
					case 2:
						; break;
					case 3:
						; break;
					case 4:
						; break;
					case 5:
						; break;
					case 6:
						; break;
					case 7:
						;
						break;
					case 8:
						arg1 = arg1/arg2;
						break;
					case 9:
						arg1 = arg1*arg2;
						break;
					case 10:
						arg1 = arg1 % arg2;
						break;
					case 11:
						arg1 = fmul(arg1, arg2);
						break;
					case 12:
						arg1 = fdiv(arg1, arg2);
						break;
					case 13:
						arg1 = arg1 & arg2;
						break;
					case 14:
						arg1 = arg1 | arg2;
						break;
					case 15:
						arg1 = arg1 ^ arg2;
						break;
					case 16:
						; break;
					case 17:
						; break;
					case 18:
						; break;
					case 19:
						; break;
					case 20:
						;
						break;
					case 21:
						arg1 = arg1 == arg2;
						break;
					case 22:
						arg1 = arg1 < arg2;
						break;
					case 23:
						arg1 = arg1 <= arg2;
						break;
					case 24:
						arg1 = arg1 > arg2;
						break;
					case 25:
						arg1 = arg1 >= arg2;
						break;
					case 26:
						arg1 = arg1 != arg2;
						break;
					case 27:
						arg1 = _ILT_(arg1, arg2);
						break;
					case 28:
						arg1 = _ILE_(arg1, arg2);
						break;
					case 29:
						arg1 = _IGT_(arg1, arg2);
						break;
					case 30:
						arg1 = _IGE_(arg1, arg2);
						break;
				}
				x_info [subtree1] = arg1;
				tree = subtree1;
				x_rel (subtree2);
			}
			else {
				tree = x_get();
				x_arg1 [tree] = subtree1;		/* set up block									*/
				x_arg2 [tree] = subtree2;
				x_info [tree] = opr;			/* add in operator type							*/
				x_node [tree] = x_dyad;			/* and most important of all, type of node		*/
			}
		}
		goto expexit;							/* and exit from nonzero priority block			*/
	}											/* of block handling nonzero priority			*/
		
	/* and our internal branching routine: */
	
	expexit:;									/* come here to exit							*/
	switch (pop()) {							/* get identifier, and branch					*/
		case 0:
			goto explab0; break;
		case 1:
			goto explab1; break;
		case 2:
			goto explab2; break;
		case 3:
			goto explab3;
			break;
		case 4:
			; break;
		case 5:
			; break;
		case 6:
			; break;
		case 7:
			goto explab7;
			break;
		case 8:
			goto explab8; break;
		case 9:
			goto explab9; break;
		case 10:
			goto explab10; break;
		case 11:
			goto explab11;
			break;
		case 12:
			goto explab12;
			break;
	}

	return (0);	
}
	

/* Check for ambiguous syntax */

/* This routine checks for expressions of the for "if (a)...".  This is done because if (a) then	*/
/* means one thing in XPL (e.g. even/odd) and something very different in C							*/
	
static	void	check_for_ambiguous_syntax(fixed subtree)
{
	fixed tree = subtree;						/* aids in recursive calls						*/
	
	switch (x_node [tree]) {					/* branch on type of node						*/
	
		case 0:
		{										/* dyadic operator								*/
			if ((x_info [tree] == o_and)		/* and, or: check each side...					*/
			||  (x_info [tree] == o_or ))
			{
				check_for_ambiguous_syntax( x_arg1 [tree]);
				check_for_ambiguous_syntax( x_arg2 [tree]);
				
				return;
			}
			
			else if (x_info [tree] >= o_eq)		/* relational op... is OK...					*/
				return;

			break;
		}
			
		case 1:
		{										/* monadic										*/
			if (x_info [tree] == o_not)
			{
				check_for_ambiguous_syntax( x_arg1 [tree]);	/* allow !0, !1						*/

				return;
			}
			break;
		}
			
		case 2:
		{										/* constant										*/
			if (x_arg1 [tree] == t_var)			/* fixed point: allow 0 and 1....				*/
			{									/* since XPL and C useage are the same...		*/
				if ((x_info [tree] == 0) || (x_info [tree] == 1) || (x_info [tree] == (-1)) || (x_info [tree] == (-2)))
					return;
			}
			break;
		}
			
		case 3:
		{										/* variable										*/
			break;
		}										/* variable										*/

		case 4:
		{										/* procedure									*/
			break;
		}										/* of procedure									*/
			
		case 5:
		{										/* X.Cal - emit location and type				*/
			break;
		}
	}											/* of do case									*/
	
	er_amb();									/* means if (xyz)... encountered				*/
}		
	
/* $$Expression compiler section:
.  
.  The routine 'COMPUTE' takes a binary tree generated by 'EXPR' and
.  dumps same out to the interfile in a form that can be reloaded by pass2.
*/

static	fixed	subtree;						/* global										*/

static	void	compute2()
{
	static	fixed	tree;
	static	fixed	_args, sptr, temp, pvtype, info;
	push (tree);								/* save orig tree								*/
	tree = subtree;								/* aids in recursive calls						*/
	
	switch (x_node [tree]) {					/* branch on type of node						*/
	
		case 0:
		{										/* dyadic operator								*/
			emit (x_node [tree] | shl(x_info [tree], 8));	/* node and info					*/
			subtree = x_arg1 [tree]; compute2();	/* write out the first argument				*/
			subtree = x_arg2 [tree]; compute2();	/* and compute second argument				*/
			break;
		}
			
		case 1:
		{										/* monadic										*/
			emit (x_node [tree] | shl(x_info [tree], 8));	/* node and type					*/
			subtree = x_arg1 [tree]; compute2();	/* write out subtree						*/
			break;
		}
			
		case 2:
		{										/* constant										*/
			emit (x_node [tree] | shl(x_arg1 [tree], 8));	/* node and arg1					*/
			if (x_arg1 [tree] == t_var) emit (x_info [tree]);	/* fixed						*/
			else {								/* floating point - write out two words			*/
				info = x_info [tree];			/* get ptr to stack								*/
				emit (fstk [info]); emit (fstk [info + 1]);
				fstk [info] = (fixed) 0x8000;	/* free up it									*/
			}
			break;
		}										/* constant										*/
			
		case 3:
		{										/* variable										*/
			emit (x_node [tree] | shl(x_arg1 [tree], 8));	/* node and type					*/
			emit (shr(x_arg1 [tree], 8));		/* storage class								*/
			emit (x_info [tree]);				/* emit relative variable location				*/
			if (x_arg1 [tree] & 1) {			/* emit subscript if one						*/
				subtree = x_arg2 [tree];
				compute2();
			}
			break;
		}										/* variable										*/
			
		/* $$Write out expression tree to intermediate file:
		.
		.   Handle procedure calls and special calls (i.e. input).
			*/
		
		case 4:
		{										/* procedure and rtp call						*/
			fixed i;
			
			emit (x_node [tree] | shl(x_arg2 [tree], 8));	/* node and type					*/

			info = x_info [tree];							/* pick up pointer to stack			*/

			if (_IGE_(stable (info + p_key), extern_base))	/* external?						*/
				emit (stable (info + p_key));				/* yes, emit external key			*/
			
			else emit (shr(stable (info + p_key), 1));		/* no, emit rtp location or key		*/
			
			emit (stable (info + p_rtyp) & 0x00FF);			/* and emit type returned (fxd, fltg) */
			
			temp = shr(x_arg2 [tree], 8);		/* point to alias list for this proc call		*/
			
			if (aliases[temp].key != stable (info + p_key)) {	/* make sure it's for this proc!				*/
				if (!abort_now)
				{
					print("\n");
					print("### Compiler system error:  alias list doesn\'t match call.\n");
					abort_now = true;

					if (in_text)
						report_error_string (global_cpb, line_no, in_text - * (char **) in_base, &in_spec);
				}
			}
			
			emit (aliases[temp].num);			/* emit word length of list						*/
			
			for (i=0; i<aliases[temp].num; i++)
				emit(aliases[temp].info[i]);
				
			_args = stable (info + p_args);		/* get number of args							*/
			temp = 1;
			while (temp <= _args) {				/* loop through									*/
				sptr = info + p_parm - _args - _args + temp + temp;	/* going backwards, ptr to stable				*/
				pvtype = x_node [x_arg1 [tree]];	/* get type of variable to pass					*/
				if (x_arg1 [tree] != 0) {		/* if argument was provided						*/
					emit (pvtype | 0x0100);		/* emit argument type (the "400" guarantees that PVTYPE is not zero - it is masked off in pass2)	*/
					if ((gflag (info) & p_recursive) != 0)/* are we calling a recursive procedure?		*/
						emit (recurs_parms + _args - temp);				/* yes, variable location is canonical			*/
					else emit (stable (sptr + p_ploc));					/* no, send out relative variable location		*/

					if (pvtype & 1) {									/* arg is an array type							*/
						emit (x_arg1 [x_arg1 [tree]]);					/* emit token type/storage class of array we are passing	*/
						emit (x_info [x_arg1 [tree]]);					/* emit variable location, or string const location	*/
					}
					else {												/* is variable									*/
						push (info);									/* save ptr to stack							*/
						push (temp); push (_args); push (pvtype); push (sptr);
						subtree = x_arg1 [x_arg1 [tree]];
						compute2();										/* emit tree									*/
						sptr = pop(); pvtype = pop(); _args = pop(); temp = pop();
						info = pop();									/* restore pointer to base of stack				*/
					}
					sptr = x_arg1 [tree];
					x_arg1 [tree] = x_arg2 [x_arg1 [tree]];	/* link											*/
					x_rel (sptr);				/* and get rid of this one						*/
				}
				temp = temp + 1;				/* increment loop counter						*/
			}									/* of arg loop									*/
			emit (0);							/* indicates end of argument list				*/
			break;
		}										/* of procedure									*/
			
		case 5:
		{										/* X.Cal - emit location and type				*/
			emit (x_node [tree] | shl(x_arg1 [tree], 8));	/* node and type								*/
			emit (x_info [tree]);				/* location i.d.								*/
			break;
		}
			
	}											/* of do case									*/
	x_rel (tree);								/* get rid of tree								*/
	subtree = tree;								/* restore										*/
	tree = pop();								/* and pop tree									*/
}
	
static	void	compute()						/* compute an expression						*/
{
	compute2();									/* call the routine to compute the expression	*/
	alias_ptr = 0;								/* clear alias list - aliases are only valid during an expression	*/
}


/* Statement scanner:
. 
.  This routine scans off the next statement including the semicolon
.  and compiles code to execute it.  A statement includes the label
.  on the statement.  This routine is obviously very recursive,
.  calling both itself and the statement list routine.
*/

/* The following routine processes the PRINT and SEND statements. */
	
static	void	stmt();							/* forward dcl needed to handle recursion correctly after translation...	*/

static	void	stmt_pstmt(						/* passed destination - 0 for SEND, 1 for PRINT	*/
	fixed	pdes)
{
	emit (pdes);								/* emit destination								*/
	
	if (token == t_semi) emit (1);				/* print a crlf									*/
	
	else {
		
		nexpr:;									/* re-enter here								*/
		
		if (token != t_semi) {					/* semi means eol								*/
			if (token != t_comma) {				/* comma's are null also						*/
				if (token == t_sconst) {		/* string constant print						*/
					info = e_scon (0, 0);		/* emit the string constant, get pointer to it	*/
					emit (3);					/* 3 = string									*/
					emit (info);				/* ptr to sconst								*/
					scan();						/* skip over sconst								*/
				}
				else if (token == t_string) {	/* identifier 'STRING'							*/
					scan();						/* skip it										*/
					if (token != t_lpar) er_ifm();
					else scan();
					if ((token == t_fxdata) || (token == t_sconst) || (token == t_arr) || (token == t_parr) || (token == t_locat)) {
						if (token == t_sconst) info = e_scon (0, 0);	/* emit string constant if so					*/
						emit (4);
						emit (token);
						emit (gclas (name_pt));
						emit (info);
						if (token == t_locat) {							/* scan and pass expression						*/
							scan(); mvtype = t_var; subtree = expr();	/* scan fixed pt expressin						*/
							compute();									/* write it out to intermediate file			*/
						}
						else scan();									/* skip token type and get paren				*/
					}
					else er_ifm();
					if (token != t_rpar) er_ifm();
					else scan();
				}
				else {							/* not a string constant						*/
					if (token == t_pform) {								/* octal, character print formats				*/
						emit (5);
						emit (info);
						mvtype = t_var;
						scan();
					}
					else {
						emit (2);
						mvtype = t_fvar;
					}													/* decide fixed, floating						*/
					subtree = expr();			/* scan expr									*/
					compute();					/* and write out expression to print			*/
				}
			}									/* of not a comma								*/
			if (token == t_comma) {				/* print next if comma here						*/
				scan();
				goto nexpr;
			}									/* print next if comma here						*/
			emit (1);							/* print crlf here								*/
			if (token != t_semi) er_ifm();
		}										/* of not a semi								*/
	}											/* of begin if output;							*/
	emit (0);									/* end of print statenent						*/
}												/* end of procedure								*/
	

/* $$Statement list routine and statement label processing: 
.  
.   The STMT.LIST procedure is used to scan off a sequence of statements
.   until an 'END' statement is encountered.  It is used during the
.   processing of do loops, procedure definitions, and so forth.
.  
.   STMT.LIST will return with TOKEN = T.SEMI after the 'END' has been
.   scanned past.
*/
	
static	void	stmt_stmt_list(					/* scan a list of statements					*/
	fixed	skipscan,							/* True if we should skip final scan (after end)	*/
	fixed   require_semi)						/* True if require semicolon					*/	
{
	push (skipscan);							/* push requested SKIPSCAN value				*/
	
	if (token == t_semi)						/* accept and toss semi from previous statement	*/
		scan();
	else if (require_semi)						/* else complain if not there if required		*/
		er_ms();
	
	while ((token != t_end) && (token != t_eof) && !abort_now) {
		stmt();
	}
		
	if (token == t_eof) er_me();
	
	if (! (pop() & 1))							/* if should scan off end statement, do so!		*/
	{
		if (lchr == '}')						/* if end was actually a brace, synthesize		*/
			{token = t_semi; info = 0;}			/* semicolon...									*/
		else
			scan();								/* else scan 'end' to get semicolon				*/
	}
	
	emit (t_end);								/* and emit T.END to stop STMT.LIST in pass2	*/
}
	

/* Scansub:
.
.  Scansub is used to scan of an expression enclosed in parens.
.  This is used during the scanning of subscripts for subscripted
.  variables and during the processing of the 'WRITE' statement.
*/
		
static	fixed	stmt_scansub()
{
	static	fixed	sub;
	if (token != t_lpar) er_par();
	mvtype = t_var;
	scan(); sub = expr();
	if (token != t_rpar) er_ifm();
	scan();
	return (sub);
}
	

/* $$Routine to get constant:
.  
.   The following subroutine is used to scan off an expression that
.   must be a constant.   It is used to scan off a subscript
.   during the processing of an array declaration.
*/
		
static	fixed	stmt_get_const(					/* scan a constant of type typ - T.FVAR or T.VAR	*/
	fixed	typ)
{
	static	fixed	block, val;
	mvtype = typ;								/* set type for scanner							*/
	block = expr();
	if (x_node [block] != x_const) er_cr();		/* constant expression required					*/
	val = x_info [block];
	if (typ != x_arg1 [block]) {				/*  must change type							*/
		if (typ == t_var) {						/* have floating, want fixed					*/
			fstk [val] = (fixed) 0x8000;
			er_fpna();							/* floating not allowed in data list			*/
		}
		else {									/* want floating,  have fixed					*/
			fixed_to_float (val);				/* float the fixed point number					*/
			val = fstr (fval1, fval2);
		}
	}
	x_rel (block);
	return (val);
}
	

static	void	stmt_copy(						/* pass data type - assign to storage config	*/
	array	data)
{
	static	fixed	i;
	
	#define	tapdrv		1						/* tape drive in system							*/
	#define	curdvt		7						/* current device type							*/
	#define	sysdvt		8						/* system device type							*/
	#define	enddvt		48						/* end of devive table + 1						*/
	
	for (i = sysdvt; i < enddvt; i++) {			/* free what's there							*/
		com [l_ctab + i] = -1;
	}
		
	com [l_ctab + curdvt] = (data [0] & 0x00FF);	/* set current device equal to system device	*/
	for (i = 0; i <= 3; i++) {
		com [l_ctab + sysdvt + i] = data [i];	/* store there									*/
	}
		
	com [l_ctab + tapdrv] = (com [l_ctab + tapdrv] & (~ shl(1, 5)));	/* remove tape drive as well					*/
}
	

static	void	stmt_stuff(						/* used for stuffing 8 bit fields				*/
	fixed	off,
	fixed	shf)
{
	static	fixed	t, q;
	t = com [l_ctab + off];						/* get original value							*/
	q = stmt_get_const (t_var);					/* get the fixed point number					*/
	if (_IGE_(q, 256)) q = 255;					/* max											*/
	com [l_ctab + off] = (t & rot(0xFF00, shf)) + shl(q, shf);	/* and put into place							*/
}


/* Popfile:
.
.  The procedure POPFILE is used to restore the compiler back to
.  an earlier source file after processing an INSERT statement.
.
.  While processing an insert file, various information is stored on
.  a stack to allow nested INSERT statements.
*/

static	void	popfile()						/* procedure to pop up one file					*/
{
	static	fixed	i;							/* internal temp								*/
	
	n_info = ipop();							/* restore character info						*/
	nchr = ipop(); chrln = ipop(); chrpt = ipop(); pchrp = ipop(); chrbase = ipop();	/* restore character info						*/
	line_no = ipop();							/* restore linenumber							*/

	i = 0;
	while ((cfname [i++] = (char) ipop()) != 0)
		;

	if (in_text)								/* if file is in memory							*/
	{
		if (in_base && global_cpb)				/* unlock & release the insert file				*/
			release_include_file(global_cpb, in_base);

		in_text = textpop();					/* go back to old one in memory					*/
		in_base = basepop();
		in_spec = specpop();
	}
	else										/* else close this file & go back to old one	*/
	{
		fclose(in_file);
		in_file = filepop();
	}
	
	emit (t_stmt | shl(s_insert, 8));			/* emit end of insert file record				*/
	emit (0);									/* zero means insert file ended					*/
	
	if (cfname [0] == 0) cfnmes [0] = 0;		/* no message if no file name					*/
}


static	void	stmt()
{
    fixed			_upper0;
	static	fixed	stype, sinfo, sname, shash;
	static	fixed	lab, ptype;
	static	fixed	call_scan_flag;
	static	fixed	cur_ret;					/* holds key to jump to for 'RETURN' of current proc	*/
	static	fixed	subscr;
	static	fixed	loc_block;					/* location block (will always be needed)		*/
	static	fixed	lower, upper, step;			/* limits used for processing iterative do		*/
	static	fixed	d1, d2, d3;					/* temps										*/
	static	fixed	val;						/* val used in assignment						*/
	static	fixed	i;
	
	if (mod_scanned & 1 && (s_depth == 1))		/* is this statement after the module?			*/
		er_somb();								/* statement outside of module body - this is fatal	*/
		
	_stmts = _stmts + 1;						/* count statements								*/
	
	/* Check special C syntax equivalents: */
	
	if (token == t_lbra)						/* accept { for simple begin					*/
		{token = t_stmt; info = s_begin;}
		
	stype = token;								/* save last token in its entirety				*/
	sinfo = info;
	sname = name_pt;
	shash = hashcode;							/* and hash table pointer (in case we want to define it later)	*/
	
	while ((n_info & (b_spa + b_eol)) != 0) {	/* ignore non-syntax items to peek ahead here	*/
		eol_ck();
	}
	
	if (((token == t_proc) || (token == t_rtp))	/* if procedure name followed by '(', accept	*/
	&&  (nchr == '('))							/* as call statement...							*/
	{
		stype = t_stmt;
		sinfo = s_call;
	}
	
	else if (token == t_while)					/* accept 'while' for 'do while'				*/
	{
		stype = t_stmt;
		sinfo = s_do;
		info  = 1;
	}
	else
		scan();									/* scan to next token							*/
	
	/* check for statement label */
	
	if ((token == t_colon) && (sname != 0)) {	/* statement label								*/
		scan();									/* skip over colon								*/
		lchk();									/* emit line number if label					*/
		if (! ((token == t_stmt) && (info == s_proc))) {	/* if not procedure def, define label			*/
			if ((stype != t_und) && (stype != t_label)) er_lt (sname);	/* dupl def										*/
			else {								/* see if defined or not						*/
				if (stype == t_und) {
					sname = s_define (sname, shash);	/* define symbol getting new sname if new level	*/
					ptok (sname, t_label);		/* indicate is a label now						*/
					sinfo = shl(gkey(), 1);		/* get key, indicate unlocated so far			*/
				}
				else if (stype == t_label) {	/* see if existing label						*/
					if (sinfo & 1) er_lt (sname);						/* duplicate									*/
				}
				set_stable (sname + s_locn, sinfo | 1);	/* save new key, indicate is located			*/
				emit (t_ldef);					/* locate a label here							*/
				if (_IGE_(sinfo, extern_base))	/* external label?								*/
					emit (sinfo);				/* yes, emit external key						*/
				else emit (shr(sinfo, 1));		/* no, emit standard key #						*/
				if (token == t_end) return;		/* return now if T.END with label definition	*/
			}
			sname = name_pt;					/* update name pointer							*/
			shash = hashcode;					/* and hash table pointer (in case we want to define it later)	*/
		}
		stype = token;							/* save new token								*/
		sinfo = info;
		scan();									/* scan over first word of statement			*/
	}
	else if ((stype == t_stmt) && (sinfo == s_proc)) er_ifm();	/* procedure with no label						*/
		
	if (stype != t_semi) {						/* ignore null statement						*/
		
		/* Assignment:
		. 
		.  The following code processes the assignment statement:
		.  
		.     <variable> (subscript if required) = <expression>;
		.  
		.  The following routine scans off the variable, its possible subscript,
		.  and the expression to assign to it.  The information is then written
		.  out to the intermediate file.
		*/
		
		if ((stype < t_lit) || (stype == t_fxdata) || (stype == t_fldata)) {	/* variable - assignment						*/
			if ((stype == t_fxdata) || (stype == t_fldata)) wa_oda();	/* warn them the code isn't ROMable				*/
			if (stype & 1) {					/* expect subscript								*/
				if (token != t_lpar) er_msub (sname);	/* no subscript									*/
				subscr = stmt_scansub();
			}
			else subscr = 0;					/* plain										*/
			if ((token != t_opr) || (info != o_eq)) er_ifm();
			scan();								/* skip over =									*/
			emit (t_assign | shl(stype, 8));	/* assign and type								*/
			emit (gclas (sname));				/* storage class								*/
			emit (sinfo);						/* variable location in variable area			*/
			mvtype = (stype & t_fvar);			/* set bit if floating							*/
			val = expr();						/* scan expression for assignment				*/
			if (stype & 1) { subtree = subscr; compute2(); }	/* emit subscript if one						*/
			subtree = val; compute();			/* and emit expression to interfile				*/
		}										/* of handling assignment						*/
			
		else 
		if (stype == t_stmt) {					/* handle a statement							*/
			
			/* $$Statement scanner - other statements: */
			
			if (sinfo < s_pass1) emit (stype | shl(sinfo, 8));	/* type of statement and T.STMT in one			*/
			
			switch (sinfo) {					/* and branch on type							*/
			
			/* Statement scanner - CALL statement			*/
			
				case 0:
				{
					mvtype = t_fvar;			/* assume worst case							*/
					if ((token != t_proc) && (token != t_rtp))
						er_ifm();										/* not allowed									*/
					subtree = expr();			/* scan expression								*/
					compute();					/* and is what to call							*/ 
					break;
				}
					
				/* Statement scanner - RETURN statement:		*/
				
				case 1:
				{
					if (cur_proc_def == -1) er_rna();
					if (token != t_semi) {								/* allow passing no value						*/
						emit (1);										/* one means there is an expression				*/
						if (cur_proc_def == -2)
							mvtype = t_var;								/* fixed point for interrupt ret				*/
						else mvtype = (stable (cur_proc_def + p_rtyp) & t_fvar);	/* set bit for fxd or flt desired				*/
						emit (mvtype);									/* emit type returned							*/
						subtree = expr();								/* scan arithmetic expression					*/
						compute();										/* compute it									*/
						emit (cur_ret);									/* location to jump to							*/
					}
					else {												/* null return									*/
						emit (0); emit (cur_ret);
					}													/* null return									*/
					break;
				}
					
				case 2:
				{								/* procedure definition							*/
					/* Statement scanner - PROCEDURE definition:
					. 
					.  The following code compiles a procedure definition.
					.  
					.  A jump instruction is emitted that will transfer control
					.  around a procedure body.
					.  
					.  The compiler, however, will optimize the jumps around successive
					.  procedure definitions and emit only one jump at the start
					.  around many procedure definitions if there is no intermixed
					.  object code.  This is done by pass3.
					.  
					.  We proceed by:
					.    1. Emitting jump.
					.    2. Defining the procedure name in the symbol table.
					.    3. Entering the procedure argument symbols in the symbol table.
					.    4. Scanning the procedure body.
					.    5. Emitting a return instruction if the user did not terminate his
					.       procedure with a 'RETURN'.
					*/
					
					{
						static	fixed	arg_pt, numarg, ppoint;
						static	fixed	temp;
						static	fixed	prevdef;						/* previous defined flag (TRUE if this was defined with a FORWARD reference)	*/
						static	fixed	start_ram;						/* procedure's starting ram location			*/
						static	fixed	ret_typ;						/* returned variable type						*/
						static	fixed	lin_att;						/* linker attribute								*/
						static	fixed	plab;							/* pointer to symbol block for procedure		*/
						static	fixed	p_ptptr;						/* previous ptptr in case of previous definition of proc	*/
						static	fixed	p_lnum;							/* line number of procedure statement			*/
						static	fixed	attribute;						/* this procedure's attributes					*/
						static	fixed	prev_proc;						/* pointer to containing procedure's record		*/
						
						lab = s_define (sname, shash);					/* define proc									*/
						plab = lab;										/* save for linker call							*/
						p_lnum = line_no;								/* save line number for error messsages			*/
						
						/* Fix up the symbol table to indicate that the procedure name is
						.  a procedure and then scan off the arguments, putting them
						.  in the symbol table also.
						.
						.  A consecutive block of 5 keys are allocated for each procedure
						.  definition.  These keys are used to identify:
						.    1. the location of the start of the proc (entry)
						.    2. the location to jump to (around the procedure)
						.    3. the location to jump to in order to return from the procedure
						.    4. a bit word to indicate which registers are used by proc body
						.    5. length of stack frame for this proc
						*/
						
						start_ram = ram;								/* save starting ram location					*/
						p_ptptr = ptptr;								/* keep PTPTR in case of previous proc def		*/
						prevdef = 0;									/* not previously defined						*/
						attribute = 0;									/* no attributes either							*/
						
						if (gtok (lab) != t_und) {						/* should have been previously defined			*/
							if (gtok (lab) != t_proc) {					/* but it wasn't								*/
								er_lt (lab);							/* a label used to mark a procedure				*/
								ppoint = ptptr;							/* storage is safe if pointed to here			*/
							}											/* not a proc									*/
							else {										/* was defined previously						*/
								ppoint = stable (lab + s_locn);			/* point to procedure stack						*/
								ptptr = ppoint + p_parm;				/* point to argument list area					*/
								prevdef = 1;							/* assume normal forward ref					*/
							}
						}												/* of should have been previously defined		*/
						else {											/* if undefined, then look for args				*/
							if (_ILT_(ptptr + p_parm, stptr + 500)) er_nst();
							ppoint = ptptr;								/* table pointer								*/
							ptptr = ptptr + p_parm;						/* and use up this storage						*/
							ptok (lab, t_proc);							/* save new proc type							*/
							set_stable (lab + s_locn, ppoint);			/* and save proc info pointer					*/
						}
							
						push (cur_proc_def);							/* push this									*/
						prev_proc = cur_proc_def;						/* save this in case we need it					*/
						cur_proc_def = ppoint;							/* pointer into stack for current definition	*/
						s_block();										/* start of block here							*/
						push (cur_proc_dpt);							/* push this									*/
						cur_proc_dpt = s_depth;							/* remember depth here							*/
						
						/* Statement scanner - PROCEDURE definition (cont):
						.     Scan arguments: */
						
						numarg = 0;										/* zero count of arguments						*/
						
						if (token == t_lpar) {							/* scan off arguments, if any					*/
							token = t_comma;							/* pretend we got a comma						*/
							while (token == t_comma) {
								if (_ILT_(ptptr - 2, stptr + 500)) er_nst();
								dcl_symbol = 1;							/* we're scanning off variable names at a new scope level	*/
								scan();									/* scan over comma or open paren				*/
								numarg = numarg + 1;					/* count arguments								*/
								if (name_pt == 0) er_ifm();
								lab = s_define (name_pt, hashcode);		/* define symbol								*/
								if (gtok (lab) != t_und) er_lt (name_pt);	/* should be undefined here						*/
								ptok (lab, t_unda);						/* indicate undefined arg						*/
								set_stable (lab + s_locn, ptptr);		/* set up info to point to proc def area entry	*/
								scan();									/* scan over symbol								*/
								if (! (prevdef & 1)) {					/* allocate ram for argument					*/
									set_stable (ptptr + p_ptyp, t_unda);	/* no type known								*/
									set_stable (ptptr + p_ploc, ram);	/* allocate ram									*/
									ram = ram + 1;						/* allocate one word/argument in consecutive order (pass3 linker depends on this!)	*/
									ptptr = ptptr - 2;					/* account for used symbol table space			*/
								}
								else {									/* not so new (forward reference); type info known	*/
									if (numarg <= stable (ppoint + p_args))
										ptptr = ptptr - 2;				/* not too many									*/
									else er_tma();						/* too few is checked below						*/
								}										/* not so new									*/
							}
							if (token != t_rpar) er_ifm();
							scan();										/* scan over close paren						*/
						}
						dcl_symbol = 0;									/* done scanning off variable names				*/
						
						/* save number of arguments */
						
						if (! (prevdef & 1)) set_stable (ppoint + p_args, numarg);	/* save number of args							*/
						else {											/* previous definition; must match				*/
							ptptr = p_ptptr;							/* restore procedure pointer					*/
							if (numarg < stable (ppoint + p_args)) er_nea();	/* not same # of args							*/
						}
							
						/* Statement scanner - PROCEDURE definition (cont):
						.     Syntax scan:  Scan off procedure attributes.  For example:
						.       a: proc fixed;                       : fixed
						.       a: proc recursive;                   : recursive
						.       a: proc recursive swap;              : recursive, swappable
						.       a: proc floating public recursive;   : floating, public, recursive
						.       a: proc returns (floating);          : floating
						.
						.     There are two returns attributes:  FIXED and FLOATING (T.VAR, T.FVAR),
						.     one linker attribute:  PUBLIC (T.PUBLIC), one local storage attribute:
						.     RECURSIVE (T.RECURS), and one residence attribute (in main memory or
						.     in external memory):  SWAP (T.SWAP).  If a procedure is recursive,
						.     its RAM (variables) is allocated on the stack, rather than in static
						.     canonical locations.
						*/
						
						ret_typ = t_var;								/* default return type							*/
						
						if (token == t_rtns) {							/* has PL/I RETURNS () keyword					*/
							scan();										/* scan off left paren							*/
							if (token != t_lpar) er_ifm();
							scan();										/* scan off type field							*/
							if (token != t_type) er_iptd();
							else ret_typ = info;						/* set type										*/
							scan();										/* scan off right paren							*/
							if (token != t_rpar) er_ifm();
							scan();										/* get next token								*/
						}												/* Returns () keyword							*/
						else {											/* not RETURNS () keyword						*/
							if (token == t_type) {						/* old style return type						*/
								ret_typ = info;							/* save type for semantic check					*/
								scan();									/* get next identifier							*/
							}											/* old style									*/
						}												/* not RETURNS () keyword						*/
							
						/* semantics scan - check for illegal combinations */
						
						if ((ret_typ != t_var) && (ret_typ != t_fvar)) {	/* returns type can only be fixed or floating	*/
							er_iptd();									/* illegal return type							*/
							ret_typ = t_var;							/* but set some reasonable type					*/
						}
							
						/* now gather linker attributes					*/
						
						lin_att = 0;									/* default linker attribute: none				*/
						
						if (token == t_storage) {						/* some storage class							*/
							if (info == t_public) {						/* linker attribute								*/
								lin_att = info;							/* save info for semantic check					*/
								scan();									/* and scan to next								*/
							}											/* linker attribute								*/
							else er_iptd();								/* bad format									*/
						}												/* some type									*/
						else if (public_procs & 1) {					/* if outer procs should be forced PUBLIC		*/
							if (s_depth - in_begin == 2 + mod_scanned)	/* are we at outer level (ignoring BEGINs)?		*/
								lin_att = t_public;						/* force public									*/
						}												/* of forcing outer procs to be PUBLIC			*/
							
						push (stack_length);							/* save present stack length					*/
						stack_length = 0;								/* no stack space allocated for this procedure yet	*/
						
						if ((token == t_recurs) || ((gflag (prev_proc) & p_recursive) != 0)) {	/* recursive?									*/
							if (recurs_parms == 0) {					/* set up canonical storage for passing parameters to recursive procs	*/
								if (mod_scanned & 1) {					/* in a module									*/
									recurs_parms = get_extloc();		/* make it external								*/
									
									for (temp = 1; temp < max_recurs_parms; temp++) {	/* get a whole block of them					*/
										get_extloc();
									}
								}										/* of in module									*/
								else {									/* in MAIN program, allocate out of the variable area	*/
									recurs_parms = start_ram;			/* set the starting location					*/
									start_ram = start_ram + max_recurs_parms;	/* allocate the ram								*/
								}										/* of in MAIN									*/
							}											/* of set up canonical storage					*/
								
							ram = start_ram;							/* Undefine any storage allocated for parameters	*/
							attribute = (attribute | p_recursive);		/* it's recursive								*/
							if (prevdef & 1 && ((gflag (ppoint) & p_recursive) == 0)) er_ram();	/* previously declared, but not as recursive	*/
							if (numarg > max_recurs_parms) er_tma();	/* too many arguments							*/
							arg_pt = ppoint + p_parm;					/* start here									*/
							
							for (stack_length = 0; stack_length < numarg; stack_length++) {	/* update the parameter locations (and initialize STACK_LENGTH)	*/
								set_stable (arg_pt + p_ploc, stack_length + 1);	/* update the argument location (on stack)		*/
								arg_pt = arg_pt - 2;					/* point to next arg							*/
							}											/* of update parameter locations				*/
								
							if (token == t_recurs) scan();				/* scan to next symbol							*/
						}												/* of recursive?								*/
						else if (prevdef & 1 && ((gflag (ppoint) & p_recursive) != 0))/* if previously declared to be recursive		*/
							er_ram();									/* recursive attribute mismatch					*/
							
						if (token == t_swap) {							/* swappable procedure (indeed, this type ONLY swaps)	*/
							attribute = (attribute | p_swap | p_swapscon);	/* remember this is a swappable proc			*/
							scan();										/* scan to next token							*/
						}
							
						if (token == t_swpcode) {						/* for nonswapping scon's and data arrays		*/
							attribute = (attribute | p_swap);			/* swap, but don't swap the scon's or data arrays	*/
							scan();										/* get next token								*/
						}
							
						if (token != t_semi) er_ifm();					/* make sure it ends with a semicolon			*/
							
						if ((lin_att == t_public) || ((attribute & p_swap) != 0)) {	/* check that it's not an internal proc			*/
							if (mod_scanned & 1) {						/* if we're in a module							*/
								if (s_depth - in_begin != 3)			/* are we at MODULE's outer level (ignoring BEGINs)?	*/
									er_pbna();							/* public procedure not at outer level			*/
							}
							else {
								if (s_depth - in_begin != 2)			/* are we at outer level (ignoring BEGINs)?		*/
									er_pbna();							/* no, error									*/
							}
						}
							
						/* make sure returns type matches if previously defined	*/
						
						if (prevdef & 1) {								/* was previously defined						*/
							set_stable (ppoint + p_rtyp, stable (ppoint + p_rtyp) & 0x00FF);	/* remove any flags (erase previous assumptions)	*/
							arg_pt = ppoint + p_parm;					/* start here									*/
							while (arg_pt > ppoint + p_parm - numarg - numarg) {	/* and process each arg							*/
								set_stable (arg_pt + p_ptyp, stable (arg_pt + p_ptyp) & 0x00FF);	/* remove flags from parameters					*/
								arg_pt = arg_pt - 2;
							}
							if (stable (ppoint + p_rtyp) != ret_typ) er_iptd();	/* wrong type									*/
							if (stable (ppoint + p_key) & 1) er_lt (plab);	/* multiple declarations						*/
						}												/* was previously defined						*/
						else set_stable (ppoint + p_rtyp, ret_typ);		/* not defined - set type						*/
							
						pflag (ppoint, attribute);						/* define proc flags HERE (and not before!)		*/
						
						if (! (prevdef & 1)) {							/* forward references already defined all this	*/
							lab = gkey();								/* select key used for jump to procedure		*/
							nexkey = nexkey + proc_keys;				/* save enough keys for proc					*/
							set_stable (ppoint + p_key, shl(lab, 1));	/* save key w/o defined bit						*/
						}
						emit (start_ram);								/* keep track of variables used by this procedure (emit starting ram location)	*/
						emit (shr(stable (ppoint + p_key), 1));			/* emit key for future label defs				*/
						emit (gflag (ppoint));							/* emit procedure attributes					*/
						
						/* Statement scanner - PROCEDURE definition (cont):
						.     The arguments have been scanned and the return type set.
						.     Set at this point are:
						.       P.KEY  = key of entry*2 + defined bit
						.       P.ARGS = number of arguments
						.       P.RTYP = proc flags/return variable type
						.       P.PARM = arg flags/(T.UNDA or type), location
						.
						.     Prepare to define procedure - save everything, output code to save
						.     parameters passed in registers, etc.  */
						
						push (cur_ret);									/* and current return key						*/
						cur_ret = shr(stable (ppoint + p_key), 1) + 2;	/* current return key							*/
						push (ppoint);
						if ((gflag (ppoint) & p_recursive) == 0) {		/* if we're not defining a recursive proc		*/
							if (numarg > 4) numarg = 4;					/* restrict number of arguments assigned to regs	*/
						}
						emit (numarg);									/* emit number of arguments being passed in regs	*/
						arg_pt = ppoint + p_parm;						/* pointer to first arg							*/
						temp = numarg;									/* move number of args							*/
						if (temp > 4) temp = 4;							/* unconditionally restrict to 4				*/
						numarg = numarg - temp;							/* get remaining args to be emitted later		*/
						while (temp > 0) {								/* emit store for up to first 4 args			*/
							emit (stable (arg_pt + p_ploc));			/* and emit variable location for it			*/
							arg_pt = arg_pt - 2;						/* move ptr										*/
							temp = temp - 1;							/* decrement count of arguments left to do		*/
						}
						if ((gflag (ppoint) & p_recursive) != 0) {		/* is this a recursive proc?					*/
							temp = recurs_parms + 4;					/* point to next canonical parameter location	*/
							while (numarg > 0) {						/* emit stack stores for remaining parameters	*/
								emit (temp);							/* emit canonical location						*/
								temp = temp + 1;						/* point to next canonical location				*/
								numarg = numarg - 1;					/* decrement number of arguments left to do		*/
							}											/* of emit stores for remaining parameters		*/
						}												/* of if this is a recursive procedure			*/
							
						/* Now scan the statement list that comprises the procedure body
						.  and emit the code that will eventually become the procedure.
						.  After that, clean up and return.	*/
						
						s_sl_stak [s_depth - 1] = ptptr;				/* kludge to preserve def info at top			*/
						
						push (data_key);								/* save next data key							*/
						data_key = 0;									/* start data keys all over						*/
						
						push (lin_att  );								/* save linker attributes						*/
						push (plab     );								/* save pointer to symbol name for proc			*/
						push (p_lnum   );								/* save line number for error messages			*/
						push (prev_proc);

						stmt_stmt_list (1,1);							/* don't let STMT.LIST scan off last semicolon	*/
						
						prev_proc = pop(); p_lnum  = pop();
						plab      = pop(); lin_att = pop();
						data_key  = pop(); ppoint  = pop();
						
						emit (stable (ppoint + p_dswp));				/* emit NOSWAP bitmap for data statements		*/
						emit (ram);										/* detect how much ram area used by uncalled proc	*/
						emit (stack_length);							/* emit stack frame length						*/
						numarg = stable (ppoint + p_args);				/* get back # of arguments						*/
						set_stable (ppoint + p_key, stable (ppoint + p_key) | 1);	/* define the symbol							*/
						
						/* check that all arguments were properly defined */
						
						arg_pt = ppoint + p_parm;						/* point to parameter list inside symbol table	*/
						while (arg_pt > ppoint + p_parm - numarg - numarg) {	/* and process each arg							*/
							if ((stable (arg_pt + p_ptyp) & 0x00FF) == t_unda) {
								er_naad();
								set_stable (arg_pt + p_ptyp, t_var);	/* pretend fixed for calling					*/
							}
							arg_pt = arg_pt - 2;
						}
							
						if (lin_att == t_public)						/* emit linker information after arguments are defined	*/
							pub_ext_def (plab, lin_att, proc_keys + 1, p_lnum);	/* define it for linker							*/
						
						else if (lin_att != t_extern && prev_proc == -1)			/* if outer scope, emit source ref				*/
							pub_ext_def (plab, t_symonly, proc_keys + 1, p_lnum);	/* define it for source ref table	*/
						
						s_endblock();									/* globalize									*/
						
						if (lchr == '}')								/* if end was actually a brace					*/
							{token = t_semi; info = 0;}					/* then synthesize a semicolon					*/
						else
							scan();										/* scan HERE so we get any name at the correct level of localization	*/
						
						cur_ret = pop();								/* restore this									*/
						stack_length = pop();							/* restore stack length							*/
						cur_proc_dpt = pop();							/* and restore this for nesting					*/
						cur_proc_def = pop();							/* and restore this for nesting					*/
						
						/* check for closing proper procedure */
						
						if (token != t_semi) {							/* name may be omitted							*/
							if ((token != t_proc) || (info != ppoint))	/* procedure name we're looking for?			*/
								er_nm();								/* no, error!									*/
							else scan();								/* but if they are there, they must be right	*/
						}												/* of name may be omitted						*/
					}													/* procedure statement							*/
					break;
				}
					
				/* Statement scanner - BEGIN statement:			*/
				
				case 3:
				{
					s_block();					/* localize										*/
					in_begin = in_begin + 1;	/* remember we're in a BEGIN block (so we can treat it separately from a procedure)	*/
					stmt_stmt_list (0,0);		/* get a list of statements followed by 'END'	*/
					in_begin = in_begin - 1;	/* forget we're in this BEGIN block				*/
					s_endblock();				/* return to former locale						*/
					break;
				}
					
				/* $$Statement scanner - DO statement:			*/
				
				case 4:
				{
					emit (token);				/* emit token type - while, case, semicolon, or variable	*/
					
					/* 'do;' */
					if (token == t_semi) {
						lchk();
						stmt_stmt_list (0,1);	/* get a list of statements followed by 'END'	*/
					}
						
					/* do case routine								*/
					
					else if (token == t_case) {							/* do case										*/
						scan();											/* scan past 'CASE'								*/
						lchk();
						mvtype = t_var;									/* fixed point case expression					*/
						subtree = expr();								/* scan expression								*/
						compute();										/* compute the argument							*/
						stmt_stmt_list (0,1);
					}													/* of 'do case'									*/
						
					/* do loops - do while							*/
					
					else if (token == t_while) {						/* do while										*/
						boolean	simple_while = info;					/* stash away simple 'while' indicator...		*/
						
						scan();											/* scan off 'WHILE'								*/
						lchk();
						mvtype = t_var;									/* fixed point is desired						*/
						subtree = expr();								/* scan expression								*/
						compute();										/* write expression tree out to interfile		*/
						
						if (simple_while)								/* simple while statement						*/
						{
							stmt();										/* scan single statement						*/
							emit (t_end);								/* synthesize matching 'end' statement			*/
							call_scan_flag = 1;							/* set flag to skip semicolon scan				*/
						}
						else
							stmt_stmt_list (0,1);						/* scan a list followed by 'END'				*/
					}													/* of do while									*/
						
					/* do loops - do i = 1 to 3 by 4				*/
					
					else {												/* must be do i =								*/
						if (token >= t_lit) er_ifm();
						if (token & 1)          er_ifm();
						emit (gclas (name_pt));							/* emit storage class							*/
						emit (info);									/* emit variable location						*/
						mvtype = (token & t_fvar);						/* save variable type for scan					*/
						scan();											/* scan over variable							*/
						if (! ((token == t_opr) && (info == o_eq))) er_ifm();
						scan();											/* skip over "="								*/
						lchk();
						lower = expr();									/* scan expression for starting value			*/
						if (token != t_to) er_ifm();
						scan();											/* skip over to									*/
						upper = expr();									/* scan upper expression						*/
						if (token != t_by) {							/* assume step of one							*/
							step = x_get();								/* get block for constant						*/
							x_node [step] = x_const;					/* const step									*/
							x_arg1 [step] = t_var;						/* fixed point constant							*/
							x_info [step] = 1;							/* step of one									*/
						}												/* of constant step								*/
						else {											/* variable step								*/
							scan();										/* skip by										*/
							step = expr();								/* scan expression								*/
						}
							
						subtree = lower; compute2();					/* write out lower								*/
						subtree = upper; compute2();					/* write out upper								*/
						subtree = step; compute();						/* write out step								*/
						stmt_stmt_list (0,1);							/* and scan loop statement list					*/
					}													/* of do loops									*/
					break;
				}								/* of do statememt								*/
					
				/* $$Statement scanner - IF statement:			*/
				
				case 5:
				{
					mvtype = t_var;				/* look for fixed point							*/
					
					subtree = expr();			/* get boolean expression						*/
					
					if (token == t_then)		/* look for optional THEN						*/
						scan();					/* scan over 'THEN'								*/
					
					else						/* if no then, make sure not if (xyz)...		*/
						check_for_ambiguous_syntax(subtree);
					
					compute();					/* write out if expression						*/
					stmt();						/* scan 'THEN' clause							*/
					if (token == t_else) {		/* check if there is an 'ELSE' clause			*/
						emit (t_else);			/* let pass2 know there is an else				*/
						scan();					/* scan 'ELSE'									*/
						stmt();					/* scan else clause								*/
					}
					call_scan_flag = 1;			/* set to one to indicate skip the call			*/
					break;
				}
					
				/* Statement scanner - GOTO statement:			*/
				
				case 6:
				{
					if (token != t_label) {								/* if not label, try to define it				*/
						if (token != t_und) er_ifm();					/* must be label after GOTO statement			*/
						else {											/* is undefined - define forward reference label	*/
							lab = s_define (name_pt, hashcode);
							ptok (lab, t_label);						/* indicate label, as yet undefined				*/
							info = shl(gkey(), 1);						/* get a key here also							*/
							set_stable (lab + s_locn, info);			/* and store it in symbol table - indicates unlocated label	*/
						}
					}
					if (_IGE_(info, extern_base))/* external key?								*/
						emit (info);									/* yes, emit external key						*/
					else emit (shr(info, 1));							/* no, emit key pointer for label				*/
					scan();						/* scan past label								*/
					break;
				}
					
				/* Statement scanner - interrupts:				*/
				
				case 7:
					flags = (flags | enable_flag);	/* indicate existence of 'ENABLE'				*/
					
					break;
				case 8:
					;							/* Disable - already in interfile				*/
					
					break;
				/* Statement scanner - STOP:					*/
				
				case 9:
				{								/* stop stmt									*/
					if (token == t_semi) emit (0);						/* plain stop									*/
					else {												/* stop expr									*/
						emit (1);										/* one means something							*/
						mvtype = t_var;									/* fixed point									*/
						subtree = expr();								/* scan expr									*/
						compute();										/* compute expr									*/
					}													/* of stop										*/
					break;
				}								/* of stop stmt									*/
					
				/* $$Statement scanner - WRITE, INPUT, and PRINT:	*/
				
				case 10:
				{								/* write statement								*/
					subtree = stmt_scansub();	/* scan scan subexpression in parens			*/
					compute();					/* write out expression							*/
					if ((token != t_opr) || (info != o_eq)) er_ifm();
					scan();						/* skip over									*/
					subtree = expr(); compute();	/* scan and compute expression				*/
					break;
				}								/* write										*/
					
				case 11:
				{								/* linput string								*/
					emit (token); emit (gclas (name_pt)); emit (info);
					if (token == t_locat) {								/* location expression							*/
						scan();
						mvtype = t_var;
						subtree = expr();
						compute();
					}
					else if ((token == t_arr) || (token == t_parr)) scan();
					else er_ifm();										/* else error									*/
					break;
				}
					
				case 12:
				{								/* input										*/
					if (token == t_semi) er_ifm();
					while (token != t_semi) {	/* and loop through list						*/
						if (token >= t_lit) {
							er_ifm();
							token = t_var;
						}
						stype = token; sinfo = info; sname = name_pt;	/* save these items								*/
						scan();											/* skip over type								*/
						if (stype & 1) subscr = expr();					/* scan subscript								*/
						val = x_get(); x_node [val] = x_cal;			/* set type										*/
						if ((stype & t_fvar) == 0) {
							x_info [val] = l_fxi;
							x_arg1 [val] = t_var;
						}
						else {											/* floating										*/
							x_info [val] = l_fin;
							x_arg1 [val] = t_fvar;
						}
						emit (1);										/* 1, so zero is end							*/
						emit (t_assign | shl(stype, 8));				/* do it										*/
						emit (gclas (sname));
						emit (sinfo);
						if (stype & 1) {
							subtree = subscr;
							compute2();
						}
						subtree = val;
						compute();
						if (token == t_comma) scan();
						else if (token != t_semi) {
							er_ifm();
							token = t_semi;
						}
					}
					emit (0);					/* end of list									*/
					break;
				}								/* of input begin								*/
					
				case 13:
					stmt_pstmt (1);				/* print statement - destination is 1			*/
					
					break;
				case 14:
					stmt_pstmt (0);				/* send statement - destination is 0			*/
					
					break;
				/* Statement scanner - WHEN interrupt statement:
				.  
				.   The format for processing interrupts is:
				.  
				.     when <cell identifier> then <statement>;
				.  
				.   where <cell identifier> = 'lncint', ttoint', etc. and
				.   <statement> is any statement.
					*/
				
				case 15:
				{								/* when statement								*/
					flags = (flags | when_flag);	/* indicate a 'WHEN'							*/
					if (token != t_icell) {
						if (token == t_und)
							er_ufls (name_pt);							/* undefined									*/
						else er_ifm();
						info = 0;
					}
					if (cur_proc_def != -1) er_wna();					/* not allowed here								*/
					lab = gkey();				/* get key										*/
					nexkey = nexkey + when_keys;	/* save enough keys for WHEN					*/
					if (com [l_icell + info] != 0) er_dupw();			/* duplicate when detected						*/
					com [l_icell + info] = lab;	/* save  key									*/
					emit (lab);					/* emit key type								*/
					scan();						/* can type										*/
					if (token != t_then) er_ifm();						/* must be then									*/
					else scan();										/* skip the then								*/
						
					push (cur_proc_def);		/* save current procedure definition			*/
					push (cur_ret);
					
					cur_proc_def = -2;			/* set recognizable proc def flag for the return statement	*/
					cur_ret = lab + 2;			/* return key									*/
					stmt();						/* and scan the when statement					*/
					cur_ret = pop();			/* resture return								*/
					cur_proc_def = pop();
					call_scan_flag = 1;			/* set flag to skip scan						*/
					break;
				}								/* of the when business							*/
					
				case 16:
				{								/* invoke statement								*/
					if (token != t_icell) {
						if (token == t_und)
							er_ufls (name_pt);							/* undefined									*/
						else er_ifm();
						info = 0;
					}
					token = t_const;			/* pretend it's a constant						*/
					mvtype = t_var;				/* want fixed point								*/
					subtree = expr();			/* create a node for it							*/
					compute();					/* emit the statement to invoke					*/
					break;
				}								/* of invoke statement							*/
					
				/* Statement scanner - MODULE and LIBRARY statements:
				.
				.  The MODULE statement defines this program as a library file
				.  (relocatable binaries are produced for modules).  The format
				.  is:  module <name>;  There must be an accompanying END statement
				.  at the end of the file.
				.
				.  The library statement points to a precompiled module.  The
				.  format is:  library 'filename';	*/
				
				case 17:
				{								/* module statement								*/
					static	fixed	module_info;	/* save pointer to module name here (note the module statement can only occur once)	*/
					
					if (mod_scanned & 1) er_mms();						/* prevent multiple module statements			*/
					if (_stmts != 1)     er_mnfs();						/* module must be the first statement			*/
					if (token != t_semi) {								/* if name specified							*/
						if (token != t_und)								/* if there's something there					*/
						{												/* it better be undefined						*/
							er_ifm();									/* it's not - complain							*/
							module_info = -1;							/* and remember there wasn't a valid name		*/
						}
						else {											/* it's there and it's undefined				*/
							module_info = s_define (name_pt, hashcode);	/* define the symbol and remember its pointer	*/
							ptok (module_info, t_module);				/* set token field in symbol					*/
							set_stable (module_info + s_locn, module_info);	/* and set info field in symbol					*/
						}
						scan();											/* get the next token							*/
					}													/* of if name specified							*/
					else {												/* no module name								*/
						module_info = -1;								/* remember no name was specified				*/
						er_mnim();
					}
						
					mod_scanned = 1;			/* now we have scanned a module					*/
					flags = (flags | module_flag);	/* turn flag on for pass3						*/
					
					emit (name [0] + 1);		/* emit the module name length (in words)		*/
					i = shl(name [0], 1);		/* calculate byte length						*/
					if ((name [name [0]] & (~ 0x00FF)) == 0) i = i - 1;	/* odd number of bytes							*/
					emit (i);					/* emit byte length								*/
					
					for (_upper0 = name [0], i = 1; i <= _upper0; i++) {	/* emit the module name							*/
						emit (name [i]);
					}
						
					s_block();					/* be nice and start a new block				*/
					stmt_stmt_list (1,1);		/* scan the rest of the program; don't let STMT.LST scan last semi	*/
					s_endblock();				/* move back out again							*/
					
					if (lchr == '}')			/* if end was actually a brace					*/
						{token = t_semi; info = 0;}
					else
						scan();					/* scan HERE so we get any name at the correct level of localization	*/
					
					if (token != t_semi) {								/* name may be omitted							*/
						if ((token != t_module) || (info != module_info))/* module name we're looking for?				*/
							er_nm();									/* no, error!									*/
						else scan();									/* if name is there, it must be right			*/
					}													/* of name may be omitted						*/
					break;
				}								/* of module statement							*/
					
				case 18:
				{												/* library statement							*/
					char	the_name[256];						/* full treename of library						*/
					char	new_name[256];						/* c format of new name							*/
					
					if (token != t_sconst) er_ifm();			/* must be followed by a filename				*/
					
					if (name [0] == 0) er_mtn();				/* check for an empty filename					*/
					
					flags = (flags | link_flag);				/* turn flag on for linker initiation			*/

					for (i=0; i<name[0]; i++)					/* save name away...							*/
					{
						new_name[i    ] = byte(name, i);		/* copy name to c format						*/
						new_name[i + 1] = 0;
					}
					
					if (new_name[0] == ':')						/* file name begins with ":" 					*/
						strncpy(the_name, new_name, 256);		/* use name directly							*/
						
					else if (cnam[0])							/* else if name does not begin with ":" but		*/
					{											/* we have proessed an enter statement			*/
						strncpy (the_name, cnam, 256);			/* get able format catalog we entered			*/
						
						if (cnam[strlen(cnam)-1] != ':')		/* if catalog name does not end in ":"			*/
							strncat (the_name, ":", 256);		/* append colon									*/						

						strncat (the_name, new_name, 256);		/* and add new name to it						*/
					}
					else
						strncpy(the_name, new_name, 256);		/* use name directly							*/
					
					emit (shr(strlen(the_name) + 1, 1) + 2);	/* emit the record length (in words)			*/
					emit (line_no);								/* emit the line number we found the statement on	*/
					emit (strlen(the_name));
					
					for (_upper0 = strlen(the_name), i = 0; i < _upper0; i += 2) {	/* emit the library name						*/
						emit (((fixed) the_name[i]) | (((fixed) the_name[i+1]) << 8));
					}

					scan();
					break;
				}
					
				/* $$Statement scanner - INSERT statement:
				.  
				.   The following routine handles the initial entry of insert files:
				.   
				.     insert 'filename';
				.  
				.   The file is opened and then all pertinent information about the current
				.   file (line number, current position, etc.) is pushed onto our internal
				.   stack.  Pointers are set up to the new file and compilation continues.
					*/
				
				case 19:
				{								/* insert file									*/
					char the_name[128];			/* holds c-format name							*/
					
					scan();						/* skip string, should get semi					*/
					if (token != t_semi) er_ms();						/* missing										*/
					
					if (name [0] >= sizeof(the_name))
						{er_etc(); name [0] = sizeof(the_name)-1;}
						
					else if (name [0] == 0) er_mtn();					/* check for missing treename					*/
					
					else
					{
						FILE*	new_file = NULL;
						char*	new_text = NULL;
						void*	new_base = NULL;
						FSSpec	new_spec;
												
						for (i=0; i<name [0]; i++)
						{
							the_name[i  ] = byte(name, i);
							the_name[i+1] = 0;
						}

						if (show_progress)
							print("XPL Tool: processing file: %s\n", the_name);
						
						run_host_environment_250();						/* give cpu to host... */
	
						if (in_text)
						{
							char	full_name[1024];
							
							if (the_name[0] != ':')
							{
								strcpy(full_name, cnam);					/* start with enter name */
								
								if (cnam[0])
									strcat(full_name, ":");
									
								strcat(full_name, the_name);				/* append file name		 */
							}
							else
								strcpy(full_name, the_name);

							if (locate_include_file(global_cpb, full_name, &new_text, &new_base, &new_spec))
								break;
						}
						else
							new_file = (FILE *) open_able_file(the_name);

						emit (shr(name [0] + 1, 1) + 1);				/* emit the record length (in words)			*/
						
                        for (_upper0 = shr(name [0] + 1, 1), i = 0; i <= _upper0; i++) {	/* emit the insert filename						*/
							emit (name [i]);
						}
					
						if (in_text)
						{
							textpush(in_text);
							basepush(in_base);
							specpush(in_spec);
						}
						else
							filepush(in_file);							/* push current input file */
						
						ipush(0);
						i = strlen(cfname);								/* push current name */
						while (i)
							ipush (cfname [--i]);
						
						strncpy(cfname, ABLE_CONTEXT.opened_file_name, 256); /* save name for error messages */
						strncpy(cfnmes, in_fmes,          10 );
						
						ipush (line_no);
						ipush (chrbase); ipush (pchrp);
						ipush (chrpt); ipush (chrln); ipush (nchr);
						ipush (n_info);
						
						if (in_text)
						{
							in_text = new_text;
							in_base = new_base;
							in_spec = new_spec;							
						}
						else
							in_file = new_file;
						
						pchrp   = 0; nchr = 0;							/* save first one								*/
						n_info  = b_eol;
						line_no = 0;
						scan();											/* skip semi, get first token of next file		*/
						call_scan_flag = 1;								/*  d o  n o t  call scan						*/
					}													/* of inserting file							*/
					
					break;
				}								/* of insert									*/
					
				case 20:
				{								/* declare statement							*/
					/* Statement scanner - DECLARE statement:
					.  
					.  The following routine processes the 'DECLARE' statement:
					.
					.    declare (symb1, symb2, symb3) (subscript if needed) (attributes);
					.
					.  All of the processing of the DECLARE statement is done during
					.  pass one.
					*/
					
					{
						static	fixed	symb, symb1, submax, type;
						static	fixed	snptr, pspt, aloc, atype;
						static	fixed	a_ptptr;						/* pointer to argument list for current procedure table entry	*/
						static	fixed	delta;							/* size of current procedure table entry		*/
						static	fixed	numargs;						/* number of arguments to a forward reference procedure call	*/
						static	fixed	r_type;							/* returns type for a forward reference procedure call	*/
						static	fixed	pub_ext;						/* public or external flag						*/
						static	fixed	sclass;							/* storage class								*/
						static	fixed	p_lnum;							/* line number for linker						*/
						static	fixed	store;							/* storage used by symbol						*/
						static	fixed	recurs;							/* True if this is a forward reference recursive proc defn	*/
						static	fixed	first;							/* True if this is the first time through the symbol processing loop	*/
						static	fixed	i;
						
						p_lnum = line_no;								/* save line number in case linker needs it		*/
						
						dcl_start:;
						
						pub_ext = 0;									/* no publics or externals found yet			*/
						
						if ((gflag (cur_proc_def) & p_recursive) != 0)	/* in a recursive procedure?					*/
							sclass = s_automatic;						/* yes, automatic variables by default			*/
						else sclass = s_static;							/* no, static variables by default				*/
							
						/* build up a queue of symbols declared in list */
						
						if (token == t_lpar) {							/* list of symbols to be declared				*/
							symb = 0;									/* initialize list of symbols					*/
							token = t_comma;							/* pretend it started with a comma				*/
							while (token == t_comma) {
								dcl_symbol = 1;							/* we're scanning off a variable name in a DCL	*/
								scan();									/* scan off the comma							*/
								dcl_symbol = 0;							/* done scanning declared variable name			*/
								if (name_pt == 0) er_ifm();
								else {
									symb1 = get();						/* get a list element							*/
									store2 [symb1] = name_pt;			/* save pertinent info about symbol				*/
									store3 [symb1] = hashcode;
									store1 [symb1] = 0;					/* no forward pointer							*/
									if (symb == 0) symb = symb1;		/* for first one, stor here						*/
									else store1 [type] = symb1;			/* else link to end								*/
									type = symb1;						/* and save this pointer						*/
								}
								scan();									/* scan off symbol								*/
							}
							if (token != t_rpar) er_ifm();
						}
							
						/* Declaration statement (cont):
						.  
						.   If single symbol declaration (dcl xyz attributes subscript), then
						.   just scan the one symbol, queue it, and process the single-entry
						.   queue as normal.
							*/
						
						else {											/* single symbol								*/
							if (name_pt == 0) {
								er_ifm(); symb = 0;
							}
							else {
								symb = get();							/* get a single block							*/
								store1 [symb] = 0;						/* zero link (end of list)						*/
								store2 [symb] = name_pt;				/* and store pertinent information about the symbol	*/
								store3 [symb] = hashcode;
							}
						}
							
						scan();											/* scan over close paren or single symbol		*/
						
						/* now scan off subscript (if one) */
						
						if (token == t_lpar) {
							subscr = 1;									/* initialize									*/
							submax = stmt_get_const (t_var) + 1;		/* get fixed point subscript					*/
						}
						else subscr = 0;								/* no subscript provided						*/
							
						/* Declaration statement (continued):
						.
						.   Get type of symbol. */
						
						stype = t_var;									/* assume fixed for data types					*/
						
						if ((token == t_stmt) && (info == s_proc)) {	/* detect 'dcl xyz procedure'					*/
							if (subscr & 1) er_sna();					/* no subscripts allowed in procedure definitions	*/
							a_ptptr = ptptr + p_parm;					/* point to start of argument list in procedure table	*/
							numargs = 0;								/* no arguments scanned yet						*/
							r_type = t_var;								/* default returns type							*/
							scan();										/* scan off the next token						*/
							if (token == t_lpar) {						/* argument list?								*/
								token = t_comma;						/* pretend we found a comma						*/
								while (token == t_comma) {				/* scan off the args							*/
									scan();								/* get first argument type						*/
									numargs = numargs + 1;				/* count arguments								*/
									if (token != t_type) er_ifm();		/* check type									*/
									type = info;						/* remember what kind of argument it is			*/
									scan();								/* get next token								*/
									if ((token == t_type) && (info == t_arr)) {	/* array										*/
										if (type == t_arr) er_ifm();
										type = (type | t_arr);			/* remember it's an array						*/
										scan();							/* get the next token							*/
									}
									if (_ILT_(a_ptptr - 2, stptr + 500)) er_nst();	/* overflow										*/
									set_stable (a_ptptr + p_ptyp, type | t_pvar);	/* save type (OR in pointer bit for effect)		*/
									a_ptptr = a_ptptr - 2;				/* point to the next entry						*/
								}
								if (token != t_rpar) er_ifm();			/* must end with a right paren					*/
								scan();									/* get next token								*/
							}
							if (token == t_rtns) {						/* Pl/i style RETURNS?							*/
								scan();									/* scan off left paren							*/
								if (token != t_lpar) er_ifm();
								scan();									/* scan off type field							*/
								
								if ((token != t_type) || ((info != t_var) && (info != t_fvar)))/* if not FIXED or FLOATING						*/
									er_iptd();
								else r_type = info;
									
								scan();									/* scan off right paren							*/
								if (token != t_rpar) er_ifm();
								scan();									/* get next token								*/
							}
							type = t_pvar;								/* remember this is a forward reference proc	*/
							delta = a_ptptr - ptptr;					/* get size of current procedure table entry	*/
						}
						else {											/* not a forward reference procedure			*/
							if (token != t_type) {
								er_ifm(); info = 0;
							}
							type = info; scan();						/* save type and skip							*/
							if (type <= t_fvar) {						/* fixed, floating, or array (plain array means fixed array	*/
								if (subscr & 1) type = (type | t_arr);	/* any thing with subscript means array			*/
								if (token == t_type) {					/* check further - fixed data, fixed array, etc.	*/
									if (info == t_arr) {				/* array										*/
										if (subscr & 1) er_sna();		/* no subscript allowed							*/
										type = (type | t_arr);			/* make it an array								*/
										scan();
									}									/* array										*/
									else if (info == 7) {				/* Data type?									*/
										stype = (type & t_fvar);		/* set up STYPE									*/
										type = info;					/* set type to DATA								*/
										scan();
									}
								}
							}
						}
							
						if (token == t_storage) {						/* check for public or external					*/
							if ((info == t_public) || (info == t_extern)) {	/* public or external?							*/
								pub_ext = info;							/* save value									*/
								sclass = s_static;						/* Must be static								*/
								if ((info == t_extern) && subscr & 1) er_sna();	/* subscripts not allowed with externals		*/
							}
							else if ((info == s_static) || (info == s_automatic)) {	/* static or automatic?							*/
								sclass = info;							/* save storage class							*/
								if (type > t_farr) er_iptd();			/* only allowed for variables					*/
								if ((info == s_automatic) && ((cur_proc_def == -1) || (cur_proc_def == -2)))/* if at top level or in WHEN					*/
									er_iptd();							/* no AUTOs outside procs						*/
							}
							else er_ifm();								/* no, illegal format							*/
							scan();										/* scan off token								*/
						}
							
						if (type == t_pvar) {							/* forward reference procedure?					*/
							if (token == t_recurs)						/* if it's recursive							*/
							{											/* remember it									*/
								recurs = 1;								/* like so										*/
								if (numargs > max_recurs_parms) er_tma();	/* check limit									*/
								if (pub_ext == t_extern) {				/* if external									*/
									er_iptd();							/* no go										*/
									recurs = 0;							/* the precompiled module decides the recursion, not us	*/
								}
								scan();									/* scan to next token							*/
							}
							else recurs = 0;							/* not recursive								*/
						}												/* of forward reference procedure?				*/
							
						/* process each scanned symbol:					*/
						
						first = 1;										/* this is the first time through this loop		*/
						while (symb != 0) {
							loc_block = symb;							/* get pointer to block for current symbol		*/
							symb = store1 [symb];						/* unlink it									*/
							snptr = s_define (store2 [loc_block], store3 [loc_block]);	/* define label, get symbol ptr					*/
							rel (loc_block);							/* release list element							*/
							ptype = gtok (snptr);						/* get current type - should be T.UND or T.UNDA	*/
							
							/* Statement scanner - DECLARATION statement (cont):
							.  
							.  
							.   Procedure argument declarations:
							.  
							.   The symbols that correspond to procedure arguments are entered
							.   in the symbol table during the processing of the 'PROCEDURE'
							.   statement.  At that time, however, no information about the
							.   type of symbol or the amount of storage required for it, or the
							.   type of store instruction to perform, is available.
							.  
							.   Information is written to the intermediate file that allows the
							.   second and third passes of the compiler to generate the proper
							.   store instructions.
							.  
							.   For undeclared procedure arguments,  we start with:
							.     type in symbol block = T.UNDA
							.     info in symbol block = pointer to S.STACK location (below)
							.  
							.     S.STACK (INFO) = T.UNDA
							.     S.STACK (INFO + 1) = key number that is used to define symbol with
							.
							.   For forward reference procedures, the argument type is already in
							.   the procedure table and better match what we find in the declare
							.   statement.  Other procedures have an argument type of T.UNDA in
							.   the procedure table.
							*/
							
							if (ptype == t_unda) {						/* symbol is an undeclared procedure argument	*/
								if (type > t_farr) { er_ifm(); type = 0; }
								if ((((gflag (cur_proc_def) & p_recursive) == 0) && (sclass != s_static))/* if storage class specified and wrong			*/
								|| (((gflag (cur_proc_def) & p_recursive) != 0) && (sclass != s_automatic))) er_iptd();
									
								pspt = stable (snptr + s_locn);			/* get pointer into stack here					*/
								atype = (type | t_pvar);				/* or in pointer bit for effect					*/
								
								if ((stable (pspt + p_ptyp) & 0x00FF) != t_unda)/* is the argument type T.UNDA in the procedure table?	*/
								{										/* no, this means this was a forward reference procedure	*/
									if (atype != (stable (pspt + p_ptyp) & 0x00FF))/* does this type match that previously defined?	*/
										er_atm();						/* argument type mismatch - complain			*/
								}
									
								set_stable (pspt + p_ptyp, atype);		/* save defined type of argument in procedure info table	*/
								set_stable (snptr + s_locn, stable (pspt + p_ploc));	/* thru with ptr info; replace with variable location	*/
								ptok (snptr, atype);					/* and store new type in symbol table			*/
								pclas (snptr, sclass);					/* save storage class							*/
								
								if (pub_ext != 0) er_iptd();			/* arguments can't be external or public		*/
								if (subscr & 1) er_sna();
								subscr = 0;								/* in case there								*/
							}											/* of undeclared procedure argument				*/
								
							/* Statement scanner - DECLARATION statement (cont): 
							.  
							.   The following code precesses the declarations of new symbols
							.   (i.e. those symbols that are not undeclared procedure arguments).
							.  
							.   Branch on type of declaration and adjust symbol table/data
							.   area accordingly.
								*/
							
							else if (ptype != t_und) er_lt (snptr);
							else {										/* ptype = T.UND := new symbol					*/
								if (pub_ext == t_extern) store = 0;		/* no storage used by externals					*/
									
								switch (type) {							/* break up cases								*/
									case 0:
									{									/* fixed variable								*/
										ptok (snptr, t_var);			/* save new token in symbol table				*/
										store = 1;						/* need one word								*/
										break;
									}
										
									case 1:
									{									/* fixed array									*/
										ptok (snptr, t_arr);			/* save new token in symbol table				*/
										store = submax;					/* need SUBMAX words							*/
										if ((pub_ext != t_extern) && (! (subscr & 1))) er_ifm();	/* Array with no subscript (better to check above, but can't)	*/
										break;
									}
										
									case 2:
									{									/* floating variable							*/
										ptok (snptr, t_fvar);			/* save new token in symbol table				*/
										store = 2;						/* need two words								*/
										break;
									}
										
									case 3:
									{									/* floating array								*/
										ptok (snptr, t_farr);			/* save new token in symbol table				*/
										store = shl(submax, 1);			/* need 2*SUBMAX words							*/
										if ((pub_ext != t_extern) && (! (subscr & 1))) er_ifm();	/* Array with no subscript (better to check above, but can't)	*/
										break;
									}
										
									case 4:
									{									/* forward reference proc call					*/
										if (_ILT_(a_ptptr, stptr + 500)) er_nst();	/* overflow										*/
										ptok (snptr, t_proc);			/* define as proc here							*/
										if (pub_ext == t_extern)		/* external?									*/
										{								/* yes, flag it									*/
											set_stable (ptptr + p_key, get_extloc());	/* get a unique pointer							*/
											if ((stable (ptptr + p_key) & 0x0001) == 0)/* is it odd (i.e., defined)?					*/
												set_stable (ptptr + p_key, get_extloc());		/* no, make it odd so the defined bit is set	*/
										}
										else {							/* no, allocate keys for it						*/
											if (pub_ext == t_public) er_ifm();					/* this isn't legal								*/
											set_stable (ptptr + p_key, shl(gkey(), 1));	/* get key, store in table						*/
											nexkey = nexkey + proc_keys;	/* reserve enough keys for PROC					*/
											store = proc_keys + 1;		/* number of keys used							*/
											if (recurs & 1 && (recurs_parms == 0)) {			/* set up canonical storage for passing parameters to recursive procs	*/
												if (mod_scanned & 1) {							/* in a module									*/
													recurs_parms = get_extloc();				/* make it external								*/
													
													for (i = 1; i < max_recurs_parms; i++) {	/* get a whole block of them					*/
														get_extloc();
													}
												}												/* of in module									*/
												else {											/* in MAIN program, allocate out of the variable area	*/
													recurs_parms = ram;							/* set the starting location					*/
													ram = ram + max_recurs_parms;				/* allocate the ram								*/
												}												/* of in MAIN									*/
											}													/* of set up canonical storage for recursive procs	*/
										}
											
										a_ptptr = a_ptptr + numargs + numargs;	/* point back to start of argument list			*/
										
										for (i = 1; i <= numargs; i++) {	/* assign argument locations					*/
											if (pub_ext == t_extern)	/* external?									*/
												set_stable (a_ptptr + p_ploc, get_extloc());	/* yes, assign external locs so they're relocated in pass3 (Warning: pass3 assumes consecutive locs that immediately follow proc key)	*/
											else {												/* no, assign ram								*/
												if (recurs & 1)									/* recursive proc?								*/
													set_stable (a_ptptr + p_ploc, recurs_parms + i - 1);	/* yes, allocate from canonical					*/
												else {											/* no, allocate from ram						*/
													set_stable (a_ptptr + p_ploc, ram);			/* assign the current ram position				*/
													ram = ram + 1;								/* we've used one more word						*/
												}
											}													/* of assign ram								*/
											if (! (first & 1))			/* if this isn't the first symbol processed in the loop	*/
												set_stable (a_ptptr + p_ptyp, stable (a_ptptr + p_ptyp - delta));	/* copy type information from previous entry	*/
											a_ptptr = a_ptptr - 2;		/* point to next argument						*/
										}
											
										set_stable (ptptr + p_args, numargs);	/* takes NUMARGS arguments						*/
										set_stable (ptptr + p_rtyp, r_type);	/* and returns R.TYPE							*/
										set_stable (snptr + s_locn, ptptr);	/* save pointer to data area					*/
										pflag (ptptr, p_swap);			/* assume procedure swaps						*/
										if (recurs & 1) pflag (ptptr, gflag (ptptr) | p_recursive);	/* set recursive flag							*/
										ptptr = a_ptptr;				/* point to the end of this procedure table entry	*/
										a_ptptr = a_ptptr + delta;		/* and point to the end of the next entry (in case there is one)	*/
										break;
									}
										
									case 5:
									{									/* statement label								*/
										if (subscr & 1) er_sna();		/* no subscripts allowed here					*/
										ptok (snptr, t_label);			/* indicate a label								*/
										if (pub_ext == t_extern)		/* external?									*/
										{								/* yes, flag it									*/
											set_stable (snptr + s_locn, get_extloc());	/* yes, flag it									*/
											if ((stable (snptr + s_locn) & 0x0001) == 0)/* is the value odd?							*/
												set_stable (snptr + s_locn, get_extloc());		/* no, get an odd value so the defined bit is set	*/
										}
										else {							/* no, allocate a key for it					*/
											set_stable (snptr + s_locn, shl(gkey(), 1));	/* and get key									*/
											store = 1;					/* number of keys used							*/
										}
										break;
									}
										
									case 6:
									{									/* literal declarations							*/
										if (subscr & 1) er_sna();		/* no subscripts allowed here					*/
										ptok (snptr, t_lit);			/* indicate a literal							*/
										if (token != t_sconst) er_ifm();
										if (pub_ext != 0)      er_ifm();	/* literals are compile-time				*/
										d1 = shr(name [0] + 1, 1);		/* get number of words of text					*/
										d2 = shr(d1+7, 3);				/* compute # of 8-word strides needed			*/
										if (d2 == 0) d2 = 1;			/* allocate at least 1 word						*/
										if (_IGT_(stptr + d2 + 500, ptptr)) er_nst();
										set_stable(stptr, name[0]);		/* store byte length of literal string			*/
										for (d3 = 0; d3 < d1; d3++) {set_slit_text (stptr, d3, name [d3+1]); }	/* copy literal string into data area			*/
										set_stable (snptr + s_locn, stptr);	/* save pointer to characters in table			*/
										stptr = stptr + d2;				/* and this much longer							*/
										scan();							/* skip over string constant					*/
										break;
									}									/* of handling literal declaration				*/
										
									/* Statement scanner - DECLARATION statement (cont):
									.  
									.   The following code processes the data type of a variable:
									.  
									.      dcl xyz data (a, b, c, d);
									.   or dcl xyz data ('string');
									.  
									.   Since the data list is allowed to extend over many source lines, 
									.   it is directly copied from the source file to the object file
									.   so there is no maximum length associated with the data type of variable.
									.  
									.   Scan the data and write same to object file:
										*/
									
									case 7:
									{									/* type 'DATA'									*/
										if (subscr & 1) er_sna();		/* subscripts not allowed						*/
										ptok (snptr, (t_fxdata | stype));	/* set type of data for future use				*/
										if (pub_ext == t_extern)		/* external has no data							*/
											set_stable (snptr + s_locn, get_extloc());	/* get marker to put into interfile				*/
										else {							/* not an external								*/
											pdkey (snptr, data_key);	/* store data key for this data statement		*/
											emit (shl(s_declare, 8) | t_stmt);	/* emit a declare data record					*/
											emit (t_fxdata | stype);	/* tag it as being data							*/
											emit (data_key);			/* and emit the key								*/
											if (data_key < 255) data_key = data_key + 1;		/* assign a new key								*/
											if (token != t_lpar) er_ifm();						/* must be lpar									*/
											scan();
											if (token == t_sconst) {							/* dcl a data ('abcdefghj');					*/
												set_stable (snptr + s_locn, e_scondata());		/* emit string constant, store pointer			*/
												scan();											/* skip it										*/
											}
											else {												/* list of numbers								*/
												store = 0;										/* #											*/
												dbufp = 0;										/* and initialize buffer pointer				*/
												set_stable (snptr + s_locn, str_data);			/* set up pointer to where data starts			*/
												while (token != t_rpar) {						/* until end									*/
													sinfo = stmt_get_const (stype);				/* get data - fixed or floating constant		*/
													if (stype == t_var) dbufemit (sinfo);		/* there it is									*/
													else {										/* floating data								*/
														dbufemit (fstk [sinfo]);
														dbufemit (fstk [sinfo + 1]);
														fstk [sinfo] = (fixed) 0x8000;			/* and free up									*/
														store = store + 1;						/* plus two words for floating data				*/
													}
													store = store + 1;							/* one word for each additional					*/
													if (token == t_comma) scan();
													else if (token != t_rpar) {					/* error										*/
														er_ifm(); token = t_rpar;
													}
												}
												dbuffrc();										/* and force out data if any					*/
											}													/* of numbers									*/
											if (token != t_rpar) er_ifm();						/* error										*/
											scan();
										}								/* of not external								*/
										break;
									}									/* of type data									*/
										
								}										/* of case statement							*/
									
								if (type <= t_farr) {					/* if storage declared, allocate it				*/
									pclas (snptr, sclass);				/* save storage class							*/
									
									if (pub_ext == t_extern)			/* external?									*/
										set_stable (snptr + s_locn, get_extloc());	/* yes, flag it									*/
									else switch (sclass) {				/* no, allocate storage by class				*/
										case 0:
										{								/* static storage								*/
											set_stable (snptr + s_locn, ram);	/* located at RAM								*/
											if (_ILE_(ram + store + 100, ram)) er_ptl();				/* check for overflow							*/
											ram = ram + store;
											break;
										}
										case 1:
										{								/* automatic storage							*/
											set_stable (snptr + s_locn, stack_length + 1);	/* located at STACK_LENGTH						*/
											if (_ILE_(stack_length + store + 100, stack_length)) er_ptl();	/* check for overflow							*/
											stack_length = stack_length + store;
											break;
										}
									}									/* of storage class								*/
								}										/* of allocating storage						*/
									
								/* define a symref or symdef record if external or public	*/
								
								if (pub_ext != 0)						/* emit symbols for symbol table in linker		*/
									pub_ext_def (snptr, pub_ext, store, p_lnum);
									
								else if (type != 6 && type != 4 && cur_proc_def == -1)	/* if outer scope, emit source ref	*/ // but skip lits
									pub_ext_def (snptr, t_symonly, store, p_lnum);		/* define it for source ref table	*/ // and non-external proc forward references
								
								if (pub_ext == t_public) {				/* check that a public variable is not in a procedure	*/
									if (mod_scanned & 1) {				/* if we're in a module							*/
										if (s_depth - in_begin != 2)	/* are we at MODULE's outer level (ignoring BEGINs)?	*/
											wa_pdip();					/* public definition not allowed				*/
									}
									else {
										if (s_depth - in_begin != 1)	/* are we at outer level (ignoring BEGINs)?		*/
											wa_pdip();					/* no, warn the user							*/
									}
								}
									
							}											/* of handling a non-T.UNDA symbol				*/
								
							first = 0;									/* done with first loop							*/
						}												/* of do loop									*/
						if (token == t_comma) {							/* multiple declarations						*/
							dcl_symbol = 1;								/* we're scanning off a variable name in a DCL	*/
							scan();										/* scan over comma								*/
							dcl_symbol = 0;								/* done scanning declared variable name			*/
							goto dcl_start;								/* go back for some more						*/
						}
							
					}													/* declare statement							*/
					break;
				}
					
				/* $$Statement scanner - ENTER statement:		*/
				
				case 21:
				{														/* process enter statement						*/
					if (name [0] == 0) er_mtn();						/* check for an empty filename					*/
					else {												/* open any catalog								*/
						if (name [0] == 1 && byte(name, 0) == a_star) {	/* enter '*'									*/
							strncpy(ABLE_CONTEXT.able_cur_dir_name, origdir, 256);	/* restore original name						*/
							cnam[0] = 0;								/* return to no cnam							*/
						}
						else {											/* do it										*/
							for (i=0; i<name[0]; i++)					/* save name away...							*/
							{
								cnam[i    ] = byte(name, i);			/* copy name to c format						*/
								cnam[i + 1] = 0;
							}
							
							if (in_text)								/* CW: that's all that's needed					*/
								;
								
							else if (cnam[0] == ':')					/* file name begins with ":" : use master dir	*/
							{
								i = strlen(ABLE_CONTEXT.able_master_dir_name);			/* get length of master directory				*/
								
								strncpy (ABLE_CONTEXT.able_cur_dir_name, ABLE_CONTEXT.able_master_dir_name, 256);
							
								if (i && ABLE_CONTEXT.able_cur_dir_name[i-1] == ':')	/* remove trailing : from master dir name		*/
									ABLE_CONTEXT.able_cur_dir_name[i-1] = 0;			/* since our file name starts with :			*/
									
								strncat (ABLE_CONTEXT.able_cur_dir_name, cnam, 256);
								strncat (ABLE_CONTEXT.able_cur_dir_name, ":", 256);	/* append trailing : for mac o/s consistency	*/
							}
							
							else										/* else just enter 'x'							*/
							{
								strncat (ABLE_CONTEXT.able_cur_dir_name, cnam, 256);
								strncat (ABLE_CONTEXT.able_cur_dir_name, ":", 256);	/* append trailing : for mac o/s consistency	*/
							}
						}
					}
					scan();						/* skip to semicolon							*/
					break;
				}								/* of ENTER										*/
					
				case 22:
				{								/* pdl											*/
					pdll = stmt_get_const (t_var);	/* get a constant here							*/
					if (pdll == 0) er_ifm();							/* not allowed									*/
					break;
				}
					
				/* $$Statement scanner - RAM and CONFIGURATION:
				.  
				.   The RAM statement is used to specify an overriding ram location.	*/
				
				case 23:
				{								/* ram statement								*/
					ovram = stmt_get_const (t_var);	/* get fixed point constant						*/
					break;
				}
					
				case 24:
				{								/* configuration statement						*/
					
					#define	memsiz	0			/* size of memory								*/
					#define	prctyp	1			/* processor type								*/
					#define	muldiv	1			/* mul div in system							*/
					#define	options	1			/* options word  								*/
					#define	ptype	2			/* terminal type								*/
					#define	stype	3			/* printer type									*/
					
					static	fixed	maxi[] = { shl(0x0002, 8), 8, 77,  shl(8, 8) | 2};	/* maxifloppy  config							*/
					static	fixed	mini[] = {           0, 5, 35,  shl(5, 8) | 3};	/* minifloppy  config							*/
					static	fixed	dmin[] = {           0, 5, 80,  shl(5, 8) | 1};	/* double mini config							*/
					static	fixed	smin[] = {shl(0x0005, 8), 30, 80, shl(15, 8) | 0};	/* super mini config							*/
					
					while (token == t_config) {	/* scan of types								*/
						sinfo = info;									/* save current info							*/
						scan();											/* skip over - get another one, expression, comma, semi	*/
						switch (sinfo) {								/* branch on type								*/
							case 0:
								stmt_copy (maxi);
								break;
							case 1:
								stmt_copy (mini);
								break;
							case 2:
								stmt_copy (dmin);
								break;
							case 3:
								stmt_copy (smin);
								break;
							case 4:
								com [l_ctab + prctyp] = ((com [l_ctab + prctyp] & (~ shl(7, 8))) | shl(1, 8));
								break;
							case 5:
								com [l_ctab + prctyp] = ((com [l_ctab + prctyp] & (~ shl(7, 8))) | shl(2, 8));
								break;
							case 6:
								com [l_ctab + prctyp] = ((com [l_ctab + prctyp] & (~ shl(7, 8))) | shl(3, 8));
								break;
							case 7:
								com [l_ctab + memsiz] = shr(stmt_get_const (t_var), 8);
								break;
							case 8:
								com [l_ctab + muldiv] = (com [l_ctab + muldiv] | 0x0008);
								break;
							case 9:
								com [l_ctab + muldiv] = (com [l_ctab + muldiv] & (~ 0x0008));
								break;
							case 10:
								stmt_stuff (ptype, 8);					/* terminal type								*/
								break;
							case 11:
								stmt_stuff (stype, 8);					/* printer  type								*/
								break;
							case 12:
								com [l_ctab + options] = (com [l_ctab + options] | 0x0002);
								break;
						}
						if (token == t_comma) scan();					/* skip over separating commas					*/
					}
					break;
				}								/* of configuration stmt						*/
				
				case s_sharp_if:				/* #if statement								*/
				{
					fixed the_line = line_no;	/* save line # of #if							*/
					fixed levels   = 1;
						
					fixed the_const;

					mvtype = t_var;				/* look for fixed point							*/
					
					subtree = stmt_scansub();	/* get expression in parens						*/
												/* scanning next token							*/
					if (x_node [subtree] != x_const)
					{
						line_no = the_line;
						er_emes2 ((char *) "Constant expression required after #if");
						abort_now = true;
						x_node [subtree] = x_const;
					}
					
					the_const = x_info [subtree];
					
					x_rel(subtree);
					
					if ((the_const != 0) && (the_const != 1))
					{
						line_no = the_line;
						er_emes2 ((char *) "Constant expression does not evaluate to 0 or 1");
						abort_now = true;
						the_const = 0;
					}
					
					while (levels && !abort_now)
					{
						if (token == t_eof)			/* can't handle eof at this point  		*/
						{
							line_no = the_line;
							er_emes2 ((char *) "Unexpected end-of-file after #IF");
							abort_now = true;
							break;
						}
						
						if (token == t_cond_asm)	/* check for endif, elseif, else...		*/
						{
							if (info == ca_endif)	/* endif */
							{
								levels -= 1;		/* endif at our level - reduce			*/
								scan();
							}
							
							else if (info == ca_else)
							{
								if (levels > 1)		/* if #else found in nested inactive	*/
									scan();			/* code, keep inactive...				*/
									
								else				/* else if matching #else encountered	*/
								{					/* then invert logic					*/
									the_const ^= 1;
									scan();
								}
							}
							
							else					/* handle #elseif						*/
							{
								if (levels > 1)		/* if #elseif found in nested inactive	*/
									scan();			/* code, keep inactive...				*/
									
								else				/* else if #elseif encountered			*/
								{					/* then invert logic					*/
									token = t_stmt;
									info  = s_sharp_if;
									stmt();			/* treat as #if stmt					*/
									levels = 0;		/* and the #endif pops out of both		*/
								}
							}
						}
						
						else if (token == t_stmt && info == s_sharp_if)
						{
							if (the_const == 0)		/* if #if in inactive code, keep		*/
							{						/* in active and nest					*/
								levels += 1;
								scan();
							}
							
							else					/* else handle active #if as statement  */
								stmt();				/* e.g. level stays at 1...				*/
						}
						
						else						/* skip all tokens until endif	  		*/
						{
							if (the_const == 0)		/* if skipping, then just toss...		*/
								scan();

							else					/* else if code is active, handle		*/
								stmt();				/* a statement							*/
						}
					}
					
					call_scan_flag = 1;			/* set to one to indicate skip the call		*/
					break;
				}

				
				// Native statement
				// native (0) check_host_interupts();
				// native (1) run_host_environment(fixed,array);
				
				case s_native_dcl:				/* declaration of host-native procedure		*/
				{
					fixed symbol_entry;
					fixed proc_entry;
					fixed which = 0;
					fixed num_args = 0;
					fixed arg_types[4];
					fixed ctr;
											
					// Get native id enclosed in parens
					if (token != t_lpar)									/* check for parens							*/
						er_ifm();
					else
					{
						which = stmt_get_const (t_var);						/* get its id								*/
						
						if ((which < 0) || (which > 30000))
							{er_ifm(); which = 0;}
					}

					which += l_interp;										// offset
																
					// Define symbol as t.und for now
					if (name_pt == 0)										/* if no name following, error...			*/
						er_ifm();
					else
					{
						symbol_entry = s_define (name_pt, hashcode);		/* define it								*/
						proc_entry   = ptptr;								/* get location of proc table				*/
						
						scan();												/* scan over symbol							*/

						// Get arg type list
						if (token != t_lpar)								/* check for parens							*/
							er_ifm();
						else
							scan();

						num_args = 0;

						while (token == t_type && num_args < 4)				/* scan off arg listi						*/
						{
							if (info == t_var)
								arg_types[num_args++] = t_var;
								
							else if (info == t_arr)
								arg_types[num_args++] = t_arr;
							
							else
							{
								er_ifm();
								break;
							}
							
							scan();											/* accept and scan over type				*/
							
							if (token == t_comma) scan();					/* allow comma separators					*/
						}
						
						if (token != t_rpar)								/* check for parens							*/
							er_ifm();
						else
							scan();
						
						
						// Now fill out symbol entry

						ptok (symbol_entry, t_rtp);							/* is an RTP location						*/
						set_stable (symbol_entry + s_locn, proc_entry);		/* save proc table pointer					*/

						set_stable (proc_entry + p_key, shl(which, 1));		/* set address, shift left		*/
						set_stable (proc_entry + p_args, num_args);			/* set number of args			*/
						set_stable (proc_entry + p_rtyp, t_var);			/* set type	& flags				*/
						set_stable (proc_entry + p_dswp,0);
							
						for (ctr = 0; ctr < num_args; ctr += 1) {
							set_stable (proc_entry + p_parm - (ctr << 1)    , arg_types[ctr]);
							set_stable (proc_entry + p_parm - (ctr << 1) - 1, 0);
						}
						
						ptptr = ptptr + p_parm - shl(num_args, 1);			/* and update length			*/
						
						if (_IGE_(stptr + 500, ptptr))						/* see if we bombed				*/
							er_nst();
					}
				}
					
				/* $$End of statement routines:					*/
				
			}									/* of do case branch for statement types		*/
		}										/* of do for statements							*/
		else if (stype == t_end) er_tme();
		else if (stype == t_und) er_ufls (sname);
		else er_ifm();
			
		if (call_scan_flag != 1) {				/* should call scan								*/
			if (token != t_semi) 
				er_ms();
			scan();								/* skip over semi								*/
		}
		call_scan_flag = 0;						/* reset flag									*/
		
	}											/* of non-null statement case					*/
	else emit (t_null);							/* null statement								*/
}


/* Pass one initialization:
.
.  Here we set up the global variables and input/output file buffers. We
.  also read in the XPL symbol table from the disk. */

static	ufixed	snarfed_config_table[48] =
{
	0000374, 0141471, 0101400, 0026000, 0000000, 0000001, 0000000, 0000001,
 	0000401, 0000000, 0000000, 0000005, 0002400, 0000036, 0000120, 0007400,
	0000421, 0000000, 0000000, 0000004, 0002404, 0000220, 0015237, 0000001, 
	0177777, 0177777, 0177777, 0177777, 0177777, 0177777, 0177777, 0177777, 
	0177777, 0177777, 0177777, 0177777, 0177777, 0177777, 0177777, 0177777, 
	0177777, 0177777, 0177777, 0177777, 0177777, 0177777, 0177777, 0177777
};

static	void	init()
{
	static	fixed	chr;						/* last character read from the symbol table file	*/
	fixed			i = 0;
	fixed			toklen;

	stab_ptr = 0;								/* put the symbol table at the start of external memory	*/
	stsiz    =	/* 65536 */ - max_sym_len;		/* minus MAX_SYM_LEN because we leap before we look (i.e., save symbols before checking if there's room)	*/
	stxt_ptr = 256;								/* start text area after symbol table */
	slit_ptr = 512;								/* start of literall area */

	_st = stsiz;								/* initialize for proper stats printout in pass3	*/
	pdll = 1024;								/* default user pdl is 1024 words				*/
	ram = 1;									/* start variable area at location 1 to keep 0 unused	*/
	extloc = extern_base - 1;					/* start externals here							*/
	recurs_parms = 0;							/* no recursive parameters						*/
	
	stptr = 1;									/* symbol table pointer - location 1 not used	*/
	ptptr = stsiz - 1;							/* procedure table pointer - starts at top of symbol table	*/
	
	for (chr = 0; chr < configlen; chr++)		/* copy the configuration area over into the interpass communications area	*/
		com [l_ctab + chr] = snarfed_config_table[chr];
		
	/* Dynamic construction of symbol table:
	.  
	.   The symbol table is dynamically constructed at runtime.
	.   This is because  
	.     1. it is always changing,  and 
	.     2. the hashcode function and buckets would be 
	.        too difficult to compile in as just data.
		*/
	
	chr = built_symbol_table[i++];				/* get first character							*/
	
	while (chr != 0) {							/* and look for the null at the end of symbol disk	*/
		token  = 0;
		toklen = chr;
		
		for (info = 0; info < toklen; info++) {	/* and scan off the bytes						*/
			chr = built_symbol_table[i++];		/* get the byte									*/
			if (info & 1) name [token] = (name [token] | shl(chr, 8));	/* or in second byte							*/
			else {								/* first byte									*/
				token = token + 1;
				name [token] = chr;				/* save it										*/
			}
		}
			
		name [0] = token;						/* save count									*/
		
		token = built_symbol_table[i++]; 
		info  = built_symbol_table[i++];		/* get new stuff								*/
		
		s_lookup();								/* get the symbol								*/
		
		chr = s_define (name_pt, hashcode);		/* define the symbol							*/
		ptok (chr, token); set_stable (chr + s_locn, info);	/* save symbol info					*/
		
		chr = built_symbol_table[i++];			/* and get next,  end with zero					*/
	}											/* of symbol read loop							*/
		
	/* now read in the symbol definitions for the runtime package routines */
	
	chr = built_symbol_table[i++];				/* get the next character						*/
	
	while (chr != 0) {							/* and terminates with a zero					*/
		token  = 0;
		toklen = chr;
		
		for (info = 0; info < toklen; info++) {	/* and scan off the bytes						*/
			chr = built_symbol_table[i++];		/* get byte										*/
			if (info & 1) name [token] = (name [token] | shl(chr, 8));	/* or in next byte								*/
			else {
				token = token + 1;
				name [token] = chr;
			}
		}
			
		name [0] = token;
		
		s_lookup();								/* look it up in symbol table					*/
		chr = s_define (name_pt, hashcode);		/* define it									*/
		ptok (chr, t_rtp); set_stable (chr + s_locn, ptptr);			/* save pointer					*/
		set_stable (ptptr + p_key, shl(built_symbol_table[i++], 1));	/* get address, shift left		*/
		set_stable (ptptr + p_args, built_symbol_table[i++]);			/* get number of args			*/
		set_stable (ptptr + p_rtyp, built_symbol_table[i++]);			/* get type						*/
		
		for (chr = 0; chr <= 6; chr += 2) {		/* get args												*/
			set_stable (ptptr + p_parm - chr, built_symbol_table[i++]);
			set_stable (ptptr + p_parm - chr - 1, built_symbol_table[i++]);
		}
			
		ptptr = ptptr + p_parm - shl(stable (ptptr + p_args), 1);		/* and update length			*/
		chr = built_symbol_table[i++];
	}											/* and that is it								*/
	
	for (token = 0; token <= 255; token++) {	/* save one sector for the interpass comm area	*/
		emit (0);
	}
}
	
static	fixed	pass1()
{
	fixed i;
	
	init();										/* initialize									*/
	
	s_block();									/* start a block								*/
	
	scan();										/* get first token								*/
	
	while (token != t_eof && !abort_now) {		/* compile until we reach the end of the file	*/
		stmt();
	}
		
	s_endblock();								/* end the block								*/
	
	emit (t_eof);								/* emit end of file								*/
	emit (t_eof);								/* remember pass two reads one extra word		*/
	
	/* check for clean termination of pass1 */
	
	if (_IGE_(ram + 100, extern_base)) er_ptl();/* was too much ram allocated?					*/
		
	if ((stackpt | blocks_in_use) != 0) {		/* did not deallocate all of STORE				*/
		print("\n");
		print("%s%6d%s%6d%s\n", "### Compiler system error detected (stackpt", stackpt, "  blocks", blocks_in_use, ")");
		abort_now = true;

		if (in_text)
			report_error_string (global_cpb, line_no, in_text - * (char **) in_base, &in_spec);
	}
	
	iflng = (fixed) (((inter_file_word_length - INTER_FILE_1_START) + 255) / 256);	/* compute file length (sectors)				*/
	
	for (i=0; i<=maxcomm; i++)					/* write comm area to start of interfile		*/
		inter_file[INTER_FILE_1_START + i] = com[i];
		
	return (trmst);								/* return the termination status				*/
}

	
long	pass1_main()
{
	fixed	term_stat;							/* termintion status							*/

	strncpy(origdir, ABLE_CONTEXT.able_cur_dir_name, 256);	/* save original directory						*/
	
	trmst        =  0;							/* re-init static data */
	abort_now    =  0;
	numerrs      =  0;
	line_no      =  0;
	plineno      =  0;
	alias_ptr    =  0;
	mod_scanned  =  0;
	in_begin     =  0;
	public_procs =  false;
	depth        =  0;
	cur_proc_def = -1;
	_stmts       =  0;
	s_depth      =  0;
	ispt         =  0;
	n_info		 = b_eol;
	filespt      =  0;
	textspt      =  0;
	specspt      =  0;
	prior_emes1  =  0;
	free_start   =  1;
	free_end     =  0;
	stackpt      =  0;
	dbufp        =  0;
	pchrp        =  0;

	blocks_in_use = 0;
	
	memset(s_nl_stak, 0, sizeof(s_nl_stak));
	memset(s_sl_stak, 0, sizeof(s_sl_stak));
	memset(s_pdef,    0, sizeof(s_pdef   ));

	memset(hashtab,   0, sizeof(hashtab  ));
	memset(hashdep,   0, sizeof(hashdep  ));


	/* Runtime initialization of free storage -- links up (as free storage)
	.  all blocks with index greater than or equal to the initial value of
	.  'FREE'.  This permits using some of the blocks initially.  */
	
	store1 [free_start] = 0;					/* zero indicates end of list					*/
	free_end = free_start;
	for (free_start = free_start + 1; free_start <= _blocks; free_start++) {
		store1 [free_start] = free_start - 1;
	}
	free_start = _blocks;

	s_pdef [s_depth] = cur_proc_def;			/* no procedures at this level					*/
	
	for (fstkptr = 0; fstkptr < fstklen; fstkptr++) {
		fstk [fstkptr] = (fixed) 0x8000;		/* indicate free slots							*/
	}
	fstkptr = 0;								/* and start with zero							*/
	
	if (!run_quietly)
		print("Scientific XPL version 6.12 - March 29, 1999\n");
		
	term_stat = pass1();
	
	if (term_stat)	
		return(-1);
	
	if (show_progress)
		printf("XPL Tool: Pass 1 completed:  statements: %8d   interfile: %8d\n", _stmts, (int) (inter_file_word_length - INTER_FILE_1_START));
	
	return (0);
}
