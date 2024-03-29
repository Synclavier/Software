/* Pass1 definitions: */

#define	max_errs			10						/* number of (non-fatal) errors allowed before bombing out	*/
#define	prior_max		5						/* number of operator priorities (expressions parsing)	*/
#define	istklen			((fixed)(40*ins_levels))/* insert file processing stack length			*/
#define	max_sym_len		64						/* maximum symbol length						*/

/*  The following literal declarations are used throughout
.   the compiler to determine the size of some fixed-length
.   tables, conditional compiler toggles, etc.
*/

#define	fstklen			20						/* length of floating temp stack				*/
#define	hashsize			255					/* hash table size								*/

/* Symbol table definitions:
.
.    The symbol table holds symbols (identifiers) and procedure
.    definitions.  The symbols grow towards high memory and the
.    procedures grow down towards the symbols.  If they hit each
.    other, the compiler has run out of room.
.
.    The form of a procedure block is as follows: */

#define	p_key				( 0)					/* 15 bits GKEY value, 1 'defined' bit (or 16 bits GET_EXTLOC value for externals)	*/
#define	p_args			(-1)					/* number of arguments							*/
#define	p_flag			(-2)					/* procedure flags (upper half)					*/
#define	p_rtyp			(-2)					/* returned type T.VAR or T.FVAR (lower half)	*/
#define	p_dswp			(-3)					/* Noswap bit map of data dcl's that cannot swap (by data key - up to 16)	*/
#define	p_parm			(-4)					/* parameters start here:						*/
#define	p_ptyp			( 0)					/* arg flags/token type							*/
#define	p_ploc			(-1)					/* location										*/

/* argument flags:  NO arg flags yet */

/* symbol block format: */

#define	s_next			0						/* pointer to next hash							*/
#define	s_dkey			1						/* 8 bit key for data statement (upper half)	*/
#define	s_clas			1						/* 4 bit (two unused) storage class (2nd nibble)	*/
#define	s_dpth			1						/* 4 bit depth field for NEXT symbol (lower nibble)	*/
#define	s_tokn			2						/* 8 bit token field, 8 bit length in words		*/
#define	s_locn			3						/* location in ram (variables) or GKEY (labels) or info	*/

#define	s_text			0						/* start of symbol name text					*/

#define	s_length			4						/* number of header words in symbol record		*/

/* The following define the INFO field returned by the scanner when TOKEN
.  is T.STMT.  These definitions are only used by pass1 and hence are
.  separated from those in P12-DEFS.  The remaining statement definitions
.  can be found in P12-DEFS. */

#define	s_pass1			20						/* pass1 specific statements start here			*/
#define	s_enter			21						/* make succeeding INSERTs and LIBRARYs search the specified catalog	*/
#define	s_pdl				22						/* set stack length								*/
#define	s_ram				23						/* set starting address of variable area		*/
#define	s_config			24						/* set default system configuration				*/
#define s_sharp_if		25						/* #if "statement"								*/
#define	s_sharp_elseif	26						/* #elseif - translator only					*/
#define s_sharp_else		27						/* #else - translator only						*/
#define s_sharp_endif	28						/* #endif - translator only						*/
#define s_native_dcl		29						/* declaration of interpreter-native routine	*/

/* Token scanner definitions: */

#define	b_digit		((fixed) 0x8000)		/* digit										*/
#define	b_eol			((fixed) 0x4000)		/* eol											*/
#define	b_comnt		((fixed) 0x2000)		/* comment										*/
#define	b_spa			((fixed) 0x1000)		/* space										*/
#define	b_symb		0x0800					/* symbol										*/
#define	b_opr			0x0400					/* operator										*/
#define	b_spec		0x0200					/* special										*/
#define	b_lcase		0x0100					/* lowercase letter								*/
#define	b_relop		0x0080					/* relational operator							*/
#define	b_mask		0x007F					/* field for other information (operator type, etc.)	*/

/* bits for relationals (or'd together to compute type) */

#define	b_eq			0x0001					/* equal to										*/
#define	b_lt			0x0002					/* less than									*/
#define	b_gt			0x0004					/* greater than									*/
#define	b_not			0x0008					/* not											*/
#define	b_rel			0x0007					/* all the actual relations						*/

