/* XPL Language Compiler */

/*	Translated to C:   	11/12/96 at 07:51	*/
/*	Translator Version:	0.000				*/

/* 	Translated from XPL compiler source 6.12 */


#include 	<string.h>
#include 	<stdlib.h>
#include 	<stdio.h>
#include 	<signal.h>

#include	"XPLCompiler.h"
#include    "XPLRuntime.h"
#include    "SyncMutex.h"

#include	"defs.h"			/* include able files.  by the way, which	*/
#include	"p1-defs.h"			/* came first, the chicken or the egg?		*/
#include	"p12-defs.h"


/*--------------------------------------------------------------------------------------*/
/* Global variables																		*/
/*--------------------------------------------------------------------------------------*/

void			*global_cpb				= NULL;
FILE 			*in_file      			= NULL;
FILE 			*out_file      			= NULL;
FILE 			*swap_file      		= NULL;
FILE 			*sym_file				= NULL;
FILE 			*rtp_file				= NULL;
char			*in_text				= NULL;
char			*out_data				= NULL;
void			*in_base				= NULL;
FSSpec			in_spec;

char 			object_file_name [256] = {""};
char 			swap_file_name   [256] = {""};
char			source_file_name [256] = {""};
char 			host_source_file [256] = {""};
char 			host_swap_file   [256] = {""};

handle			inter_file_handle		= NULL;
fixed 			*inter_file				= NULL;
long			inter_file_word_length  = INTER_FILE_1_START;

alias_struct	aliases[num_alias_structs];

fixed			store1 [_blocks + 1];		/* storage area one							*/
fixed			store2 [_blocks + 1];		/* symbol table pointers					*/
fixed			store3 [_blocks + 1];		/* and expression trees						*/
fixed			store4 [_blocks + 1];
fixed			store5 [_blocks + 1];

fixed			stack [stacksize + 1];

boolean	show_progress    = false;
boolean	show_p3_progress = false;
boolean	run_quietly      = false;

void*			inserted_stack[ins_levels + 1];
fixed			inserted_stack_pointer = 0;


/*--------------------------------------------------------------------------------------*/
/* Main - put at front to combat linker problems										*/
/*--------------------------------------------------------------------------------------*/

/* MWlink68K seems to make MPW tools single segment in all cases... Main is put here	*/
/* so it can be called from __startup__													*/
/* note: above comment applied to obsolute mac 68k MPW tool version...					*/

int compilexpl  	(int argc, char *argv[]);

#define num_m_512k	3

int compiler_main(int argc, char *argv[])
{
	int status;

    // Static variable initialization
    SyncMutex::InitAll();
	
	if (initialize_run_time_environment(num_m_512k*512*1024/256))	/* num of m512K cards for us		*/
		return (-1);
		
	status = compilexpl(argc, argv);

	cleanup_run_time_environment();
	
	return (status);
}


/*--------------------------------------------------------------------------------------*/
/* Main compiler execution loop:														*/
/*--------------------------------------------------------------------------------------*/

static	void	cleanup()					/* called at exit time						*/
{
	if (inter_file_handle)
		{free_big_memory(inter_file_handle); inter_file_handle = NULL; inter_file = NULL;}

	if (in_file)
		{fclose(in_file);  in_file = NULL;}
		
	if (out_file)
		{fflush(out_file); fclose(out_file); out_file = NULL;}
}


static void 	print_help()
{
	printf( "XPLCompiler: Version 1.003\n");
	printf( "       -md directory           specify Able Master Directory for : insert files\n");
	printf( "       -cd directory           specify Able Current Directory for source and insert files\n");
	printf( "       -od directory           specify output directory for object file\n");
	printf( "       -of filename            specify output file name\n");
	printf( "       -d                      dump statistics\n");
	printf( "       -m                      make symbol map\n");
	printf( "       -x                      make all procs public\n");
	printf( "       -o                      skip optimization\n");
	printf( "       -f                      force all code to external memory (e.g. model d)\n");
	printf( "       -z                      create code for debugger\n");
	printf( "       -p                      show progress whilst compiling\n");
	printf( "       -p3                     show progress during pass 3\n");
	printf( "       -q                      run quietly\n");
	printf( "       filename                source file name\n");
}

int compilexpl(int argc, char *argv[])
{
	int				argcount = 0;
	int				i        = 0;
	
	// Debug
	#if 0
		argv[1] = "-md";
		argv[2] = "/Volumes/CJ Data/Daily Incremental/SDC/Synclavier Development 5.2/Able";
		argv[3] = ":lod:lod1";
		argc = 4;
	#endif
	
	able_exit_callout = cleanup;			/* call on our exit							*/

	if (argc <= 1)
		{print_help(); cleanup(); return (-1);}

	init_pass1_statics();
		
	for (argcount = 1; argcount < argc; )
	{
		char *the_arg = argv[argcount];
		
		if (!the_arg)
			{printf("xpltool: no arguments\n"); cleanup(); return(1);}
		
		if ((strcmp(the_arg, "-md")   == 0)
		&&  (argcount+1               <  argc)        
		&&  (strlen(argv[argcount+1]) < sizeof(ABLE_CONTEXT.able_master_dir_name)))
		{
			strcpy(ABLE_CONTEXT.able_master_dir_name, argv[argcount+1]);
			argcount += 2;
			continue;
		}

		if ((strcmp(the_arg, "-cd")   == 0)
		&&  (argcount+1               <  argc)        
		&&  (strlen(argv[argcount+1]) < sizeof(ABLE_CONTEXT.able_cur_dir_name)))
		{
			strcpy(ABLE_CONTEXT.able_cur_dir_name, argv[argcount+1]);
			argcount += 2;
			continue;
		}
		
		if ((strcmp(the_arg, "-od")   == 0)
		&&  (argcount+1               <  argc)        
		&&  (strlen(argv[argcount+1]) < sizeof(ABLE_CONTEXT.host_output_dir_name)))
		{
			strcpy(ABLE_CONTEXT.host_output_dir_name, argv[argcount+1]);
			argcount += 2;
			continue;
		}

		if ((strcmp(the_arg, "-of")   == 0)
		&&  (argcount+1               <  argc)        
		&&  (strlen(argv[argcount+1]) < sizeof(object_file_name)))
		{
			strcpy(object_file_name, argv[argcount+1]);
			argcount += 2;
			continue;
		}

		if (strcmp(the_arg, "-p") == 0)
		{
			show_progress = true;
			argcount += 1;
			continue;
		}
		
		if (strcmp(the_arg, "-p3") == 0)
		{
			show_p3_progress = true;
			argcount += 1;
			continue;
		}
		
		if (strcmp(the_arg, "-q") == 0)
		{
			run_quietly = true;
			argcount += 1;
			continue;
		}

		if (strcmp(the_arg, "-d") == 0)
		{
			set_flags((fixed) 'e');
			argcount += 1;
			continue;
		}
		
		if (strcmp(the_arg, "") == 0)
		{
			argcount += 1;
			continue;
		}
			
		if ((strcmp(the_arg, "-e")   == 0)
		||  (strcmp(the_arg, "-m")   == 0)
		||  (strcmp(the_arg, "-x")   == 0)
		||  (strcmp(the_arg, "-o")   == 0)
		||  (strcmp(the_arg, "-f")   == 0)
		||  (strcmp(the_arg, "-z")   == 0))
		{
			set_flags((fixed) (argv[argcount][1]));
			argcount += 1;
			continue;
		}
		
		if (source_file_name[0])
			{printf("xpltool: two source files specified (%s and %s)\n", source_file_name, the_arg); cleanup(); return(1);}
			
		if (strlen(the_arg) >= sizeof(source_file_name))
			{printf("xpltool: source file name too long (%s)\n", the_arg); cleanup(); return(1);}
		
		strcpy(source_file_name, the_arg);

		argcount += 1;
	}

	sgen_main();								/* build symbol table					*/

	if (source_file_name[0] == 0)
		{printf("xpltool: no source file specified\n"); cleanup(); return(1);}
	
	if (object_file_name[0] == 0)
	{
		strcpy(object_file_name, source_file_name);
		strcat(object_file_name, ".");
	}
	
	in_file  = (FILE *) open_able_file(source_file_name);
	
	strncpy (host_source_file, ABLE_CONTEXT.opened_file_name,256);

	inter_file_handle = get_big_memory(INTER_FILE_WORD_SIZE << 1);

	if (!inter_file_handle || !*inter_file_handle)
		{printf("xpltool: could not get memory for inter_file\n"); cleanup(); return(1);}
	
	inter_file = (fixed *) *inter_file_handle;
	
	if (pass1_main())
		{cleanup(); return(1);}
	
	if (pass2_main())
		{cleanup(); return(1);}
	
	if (pass3_main())
		{cleanup(); return(1);}
	
	cleanup();
	return (0);
}
