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
        // XPLCompiler -md  /Volumes/CJ Data/Projects/SDC/Synclavier Development 5.2/Able -q :synmains:bld-daee -p3 -m -d -of :synmains:bld-daee.
		argc = 9;
		argv[0] = (char *)"XPLCompiler";
		argv[1] = (char *)"-md";
		argv[2] = (char *)"/Volumes/CJ Data/Projects/SDC/Able";
		argv[3] = (char *)"-od";
		argv[4] = (char *)"/Volumes/CJ Movies/XPL Build Products/";
		argv[5] = (char *)"-q";
		argv[6] = (char *)":synsou:stormod:odisksou";
		argv[7] = (char *)"-of";
		argv[8] = (char *)":synlibs:odisklib";
    }

	return (compiler_main(argc, argv));
}
