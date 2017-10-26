/*	SGEN.C									*/

/*	Translated to C:   	11/12/96 at 07:51	*/
/*	Translator Version:	0.000				*/

/* 	XPL 6.12 SGEN program; generates XPL symbol table in memory */


#include 	<string.h>
#include 	<stdlib.h>
#include 	<stdio.h>

#include	"XPLCompiler.h"
#include	"XPLRuntime.h"

#include	"defs.h"			/* include able files.  by the way, which	*/
#include	"p1-defs.h"			/* came first, the chicken or the egg?		*/
#include	"p12-defs.h"


/*--------------------------------------------------------------------------------------*/
/* SGEN - former XPL program to generate XPL symbol table								*/
/*--------------------------------------------------------------------------------------*/

/* Procedures to build symbol table: */

char			built_symbol_table[2048];		/* output buffer								*/
static	fixed	built_table_ptr = 0;			/* byte pointer into output buffer				*/

static	void	pc(								/* put character into output file				*/
	fixed	byt)
	
{
	if (built_table_ptr >= 2048) { print("Error: symbol table overflow.\n"); return; }

	built_symbol_table[built_table_ptr++] = (char) byt;
	
}
	
static	void	symbol(							/* and emit a symbol							*/
	char*	nam, 
	fixed	typ, 
	fixed	inf)
{
    fixed			_upper0;
	static	fixed	i;
	
	i = strlen(nam);
	pc (i);
	for (_upper0 = i - 1, i = 0; i <= _upper0; i++) {pc (_cbyte(nam, i)); }
	if (((inf | typ) & 0xFF00) != 0) {			/* larger than one byte							*/
		print("%s%s%s\n", "Error: info field too large for symbol ", nam, ".");
	}
	pc (typ);									/* emit type - bytes only						*/
	pc (inf);
}
	
/* Procedure to define a runtime package canned procedure.  This routine
.  takes six arguments:  the name of the procedure, the transfer vector
.  address, the number of arguments (must be less than four), and four
.  argument types (all must be supplied even if only some are to be used). */

static	fixed	fixit(
	fixed	arg)
{
	if ((arg == t_var) || (arg == t_fvar))
		return (arg);
	else return (arg | t_pvar);					/* else is pointer to array						*/
}
	
static	void	defrtp(
	char*	name, 
	fixed	addr,
	fixed	numarg,
	fixed	type,
	fixed	arg1,
	fixed	arg2,
	fixed	arg3,
	fixed	arg4)
{
    fixed			_upper0;
	static	fixed	i;
	
	i = strlen(name);
	pc (i);
	for (_upper0 = i - 1, i = 0; i <= _upper0; i++) {
		pc (_cbyte(name, i));
	}
		
	if (((numarg | addr | type | arg1 | arg2 | arg3 | arg4) & 0xFF00) != 0) print("%s%s%s\n", "Error: argument type too large for ", name, ".");	/* restrict to lower byte						*/
	pc (addr); pc (numarg);
	arg1 = fixit (arg1); arg2 = fixit (arg2); arg3 = fixit (arg3); arg4 = fixit (arg4);
	
	pc (type);
	pc (arg1); pc (0);
	pc (arg2); pc (0);
	pc (arg3); pc (0);
	pc (arg4); pc (0);
	
}
	
void	sgen_main()
{
	built_table_ptr = 0;
	
	/* First define operator symbols: */
	
	symbol ((char *) "ABS", t_opr, o_math + 0);
	symbol ((char *) "LOG", t_opr, o_math + 1);
	symbol ((char *) "ATN", t_opr, o_math + 2);
	symbol ((char *) "COS", t_opr, o_math + 3);
	symbol ((char *) "SIN", t_opr, o_math + 4);
	symbol ((char *) "TAN", t_opr, o_math + 5);
	symbol ((char *) "EXP", t_opr, o_math + 6);
	symbol ((char *) "SQR", t_opr, o_math + 7);
	
	symbol ((char *) "INT",  t_opr, o_int);
	symbol ((char *) "READ", t_opr, o_read);
	symbol ((char *) "CORE", t_arr, 0);
	symbol ((char *) "ADDR", t_opr, o_adr);
	
	symbol ((char *) "NOT", t_opr, o_not);
	
	symbol ((char *) "SHR", t_sdy, o_shr);
	symbol ((char *) "SHL", t_sdy, o_shl);
	symbol ((char *) "ROT", t_sdy, o_rot);
	
	symbol ((char *) "MOD",  t_opr, o_mod);
	symbol ((char *) "FDIV", t_opr, o_fdi);
	
	symbol ((char *) "AND", t_opr, o_and);
	symbol ((char *) "OR",  t_opr, o_or);
	symbol ((char *) "XOR", t_opr, o_xor);
	
	symbol ((char *) "IEQ", t_opr, o_eq);
	symbol ((char *) "ILT", t_opr, o_ilt);
	symbol ((char *) "ILE", t_opr, o_ile);
	symbol ((char *) "IGT", t_opr, o_igt);
	symbol ((char *) "IGE", t_opr, o_ige);
	symbol ((char *) "INE", t_opr, o_ne);
	
	/* Build symbol table:  Statements and keywords. */
	
	symbol ((char *) "CALL",          t_stmt, s_call);
	symbol ((char *) "RETURN",        t_stmt, s_return);
	symbol ((char *) "PROC",          t_stmt, s_proc);
	symbol ((char *) "PROCEDURE",     t_stmt, s_proc);
	symbol ((char *) "BEGIN",         t_stmt, s_begin);
	symbol ((char *) "DO",            t_stmt, s_do);
	symbol ((char *) "IF",            t_stmt, s_if);
	symbol ((char *) "GOTO",          t_stmt, s_goto);
	symbol ((char *) "ENABLE",        t_stmt, s_enable);
	symbol ((char *) "DISABLE",       t_stmt, s_disable);
	symbol ((char *) "STOP",          t_stmt, s_stop);
	symbol ((char *) "WRITE",         t_stmt, s_write);
	symbol ((char *) "LINPUT",        t_stmt, s_linput);
	symbol ((char *) "INPUT",         t_stmt, s_input);
	symbol ((char *) "PRINT",         t_stmt, s_print);
	symbol ((char *) "SEND",          t_stmt, s_send);
	symbol ((char *) "WHEN",          t_stmt, s_when);
	symbol ((char *) "INVOKE",        t_stmt, s_invoke);
	symbol ((char *) "MODULE",        t_stmt, s_module);
	symbol ((char *) "LIBRARY",       t_stmt, s_library);
	symbol ((char *) "INSERT",        t_stmt, s_insert);
	symbol ((char *) "DCL",           t_stmt, s_declare);	/* pass1 only emits these in special cases (stack allocation)	*/
	symbol ((char *) "DECLARE",       t_stmt, s_declare);
	symbol ((char *) "ENTER",         t_stmt, s_enter);	/* start of statements processed by pass1 only	*/
	symbol ((char *) "PDL",           t_stmt, s_pdl);
	symbol ((char *) "RAM",           t_stmt, s_ram);
	symbol ((char *) "CONFIGURATION", t_stmt, s_config);
	symbol ((char *) "#IF",		     t_stmt, s_sharp_if);
	symbol ((char *) "NATIVE",		 t_stmt, s_native_dcl);
	
	/* Keywords: */
	
	symbol ((char *) "LOC",       t_locat,  0);			/* array specifier								*/
	symbol ((char *) "LOCATION",  t_locat,  0);			/* array specifier								*/
	symbol ((char *) "CHR",       t_pform,  1);			/* print format									*/
	symbol ((char *) "CHARACTER", t_pform,  1);			/* print format									*/
	symbol ((char *) "OCTAL",     t_pform,  0);			/* print format									*/
	symbol ((char *) "STRING",    t_string, 0);			/* print format									*/
	symbol ((char *) "EOF",       t_eof,    0);
	symbol ((char *) "WHILE",     t_while,  0);
	symbol ((char *) "CASE",      t_case,   0);
	symbol ((char *) "TO",        t_to,     0);
	symbol ((char *) "BY",        t_by,     0);
	symbol ((char *) "END",       t_end,    0);
	symbol ((char *) "THEN",      t_then,   0);
	symbol ((char *) "ELSE",      t_else,   0);
	symbol ((char *) "READ",      t_opr,    o_read);
	symbol ((char *) "RETURNS",   t_rtns,   0);			/* definition of procedure return value			*/
	symbol ((char *) "RECURSIVE", t_recurs, 0);			/* define a procedure to be recursive			*/
	symbol ((char *) "SWAP",      t_swap,   0);			/* define a procedure to be swappable			*/
	symbol ((char *) "SWAPCODE",  t_swpcode,0);			/* allows swapping but not of string constants or data arrays	*/
	
	symbol ((char *) "#ENDIF",  	 t_cond_asm, ca_endif );
	symbol ((char *) "#ELSE",  	 t_cond_asm, ca_else  );
	symbol ((char *) "#ELIF", 	 t_cond_asm, ca_elseif);
	
	/* Symbol types for declarations: */
	
	symbol ((char *) "FIXED",     t_type, t_var);
	symbol ((char *) "BOOLEAN",   t_type, t_var);
	symbol ((char *) "POINTER",   t_type, t_var);
	symbol ((char *) "ARRAY",     t_type, t_arr);
	symbol ((char *) "FLOATING",  t_type, t_fvar);
	symbol ((char *) "LABEL",     t_type, 5);
	symbol ((char *) "LIT",       t_type, 6);
	symbol ((char *) "LITERALLY", t_type, 6);
	symbol ((char *) "DATA",      t_type, 7);
	
	symbol ((char *) "EXTERNAL",  t_storage, t_extern);	/* external reference							*/
	symbol ((char *) "PUBLIC",    t_storage, t_public);	/* external definition							*/
	symbol ((char *) "STATIC",    t_storage, s_static);	/* static variable								*/
	symbol ((char *) "AUTOMATIC", t_storage, s_automatic);	/* automatic variable							*/
	
	symbol ((char *) "TRUE",  t_const, 1);				/* boolean TRUE									*/
	symbol ((char *) "FALSE", t_const, 0);				/* boolean FALSE								*/
	symbol ((char *) "NULL",  t_const, 0);				/* null pointer									*/
	
	/* Dynamic construction of symbol table:  Interrupt keywords.
	.  
	.   The format for processing interrupts via the WHEN statement is:
	.   
	.      when lncint then do;
	.         anything;
	.         anything;
	.      end;
	.   
	.   or:
	.   
	.      when lncint then lbsyflg = 0;
	.   
	.   The following interrupt cell identifier names are recognized:
	*/
	
	symbol ((char *) "BREAK",     t_icell,  0);
	symbol ((char *) "TTOINT",    t_icell,  1);
	symbol ((char *) "TTIINT",    t_icell,  2);
	symbol ((char *) "D16INT",    t_icell,  5);
	symbol ((char *) "D03INT",    t_icell,  7);
	symbol ((char *) "DISKERROR", t_icell,  8);
	symbol ((char *) "D140INT",   t_icell,  9);
	symbol ((char *) "D136INT",   t_icell, 10);
	symbol ((char *) "D137INT",   t_icell, 11);
	symbol ((char *) "D115INT",   t_icell, 13);
	symbol ((char *) "BDB14INT",  t_icell, 14);
	symbol ((char *) "BDB15INT",  t_icell, 15);
	symbol ((char *) "D40INT",    t_icell, 16);			/* start of D54 acknowledge: ID = 16 + BDB #	*/
	symbol ((char *) "D42INT",    t_icell, 17);
	symbol ((char *) "D44INT",    t_icell, 18);
	symbol ((char *) "D46INT",    t_icell, 19);
	symbol ((char *) "D66INT",    t_icell, 20);
	symbol ((char *) "D24INT",    t_icell, 23);
	symbol ((char *) "D30INT",    t_icell, 24);
	symbol ((char *) "D31INT",    t_icell, 25);
	symbol ((char *) "D32INT",    t_icell, 26);
	symbol ((char *) "D33INT",    t_icell, 27);
	symbol ((char *) "D34INT",    t_icell, 28);
	symbol ((char *) "D35INT",    t_icell, 29);
	symbol ((char *) "D36INT",    t_icell, 30);
	symbol ((char *) "D37INT",    t_icell, 31);
	symbol ((char *) "DSP70INT",  t_icell, 40);			/* start of D55 acknowledge: ID = 32 + BDB #	*/
	symbol ((char *) "D132INT",   t_icell, 43);
	
	/* Symbols for CONFIGURATION statement: */
	
	symbol ((char *) "MAXI",       t_config,  0);
	symbol ((char *) "MINI",       t_config,  1);
	symbol ((char *) "DMINI",      t_config,  2);
	symbol ((char *) "SMINI",      t_config,  3);
	symbol ((char *) "MODELB",     t_config,  4);
	symbol ((char *) "MODELC",     t_config,  5);
	symbol ((char *) "MODELD",     t_config,  6);
	symbol ((char *) "MEMORY",     t_config,  7);
	symbol ((char *) "MULDIV",     t_config,  8);
	symbol ((char *) "NOMULDIV",   t_config,  9);
	symbol ((char *) "PTYPE",      t_config, 10);
	symbol ((char *) "STYPE",      t_config, 11);
	symbol ((char *) "HAS_BOOT",   t_config, 12);
	
	/* Runtime package routine names: */
	
	pc (0);										/* one byte to separate labels, RTPs			*/
	defrtp ((char *) "EXIT",          l_ter, 1, t_var, t_var, 0, 0, 0);
	defrtp ((char *) "BYTE",          l_byt, 2, t_var, t_arr, t_var, 0, 0);
	defrtp ((char *) "PBYTE",         l_pbt, 3, t_var, t_arr, t_var, t_var, 0);
	defrtp ((char *) "RCVDCHARACTER", l_ich, 0, t_var, 0, 0, 0, 0);
	defrtp ((char *) "BLOCKMOVE",     l_bmv, 3, t_var, t_arr, t_arr, t_var, 0);
	defrtp ((char *) "BLOCKSET",      l_bst, 3, t_var, t_arr, t_var, t_var, 0);
	defrtp ((char *) "EXPORT",        l_ept, 4, t_var, t_var, t_var, t_arr, t_var);
	defrtp ((char *) "IMPORT",        l_ipt, 4, t_var, t_var, t_var, t_arr, t_var);
	defrtp ((char *) "EXTSET",        l_est, 4, t_var, t_var, t_var, t_var, t_var);
	defrtp ((char *) "READDATA",      l_rfl, 4, t_var, t_var, t_var, t_arr, t_var);
	defrtp ((char *) "WRITEDATA",     l_wfl, 4, t_var, t_var, t_var, t_arr, t_var);
	defrtp ((char *) "SWAPINIT",      l_swi, 1, t_var, t_var, 0, 0, 0);
	defrtp ((char *) "FIND_DEVICE",   l_fde, 1, t_var, t_var, 0, 0, 0);
	defrtp ((char *) "SET_CURDEV",    l_scur,1, t_var, t_var, 0, 0, 0);
	defrtp ((char *) "EXTREAD",       l_erd, 3, t_var, t_var, t_var, t_arr, 0);
	defrtp ((char *) "EXTWRITE",      l_ewr, 3, t_var, t_var, t_var, t_arr, 0);
	defrtp ((char *) "POLYREAD",      l_prd, 4, t_var, t_var, t_var, t_arr, t_var);
	defrtp ((char *) "POLYWRITE",     l_pwr, 4, t_var, t_var, t_var, t_arr, t_var);
	
	pc (0);										/* and a final zero								*/
}
