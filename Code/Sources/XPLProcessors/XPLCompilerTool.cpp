/*	XPL Compiler Tool.c									*/

/*	Driver to allow running XPL as an MPW tool			*/

#include "XPLCompiler.h"

extern int	compiler_main   (int argc, char * const argv[]);

short	locate_include_file (void*, const char*, char **, void **, FSSpec *)	{return (0);}
void	release_include_file(void*, void *) 									{}
void	report_error_string (void*, long, long, void*)							{}

int main(int argc, char *argv[])
{
    // Debug
    if (0) {
		argc = 13;
		argv[0] = (char *)"XPLCompiler";
		argv[1] = (char *)"-md";
		argv[2] = (char *)"/Volumes/CJ Data/Dropbox/Projects/SDC/Synclavier Development 5.2/Projects/AbleBrowser/../../Able";
		argv[3] = (char *)"-od";
		argv[4] = (char *)"/Volumes/CJ Cache/XPL Build Products/";
		argv[5] = (char *)"-q";
		argv[6] = (char *)":synmains:bld-daee";
		argv[7] = (char *)"-of";
		argv[8] = (char *)":W0:SYN-5.3G";
        argv[9] = (char *)"-q";
        argv[10] = (char *)"-p3";
        argv[11] = (char *)"-m";
        argv[12] = (char *)"-d";
    }

	return (compiler_main(argc, argv));
}
