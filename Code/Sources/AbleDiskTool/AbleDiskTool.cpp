/*	AbleDiskTool.c										*/

/*	Driver to allow running AbleDisk as an MPW tool		*/

#include <stdio.h>

extern int		AbleDiskTool(int argc, const char *argv[]);
extern int      sync_prefs_grab_one_pref(const char* which);

// Need stub for prefs linkage
int             sync_prefs_grab_one_pref(const char* which) {return 0;}

int main(int argc, const char *argv[])
{
	if (0)
	{
		unsigned int  xyz = 'ABCD';
		unsigned char a   = xyz;
		unsigned char b   = xyz >> 24;
		unsigned char c   = xyz >> 16;
		
		const char* s = "ABCD";
		
		unsigned int abc = ((unsigned int) (s[0]) << 24) | ((unsigned int) (s[1]) << 16) | ((unsigned int) (s[2]) << 8) | ((unsigned int) (s[3]) << 0);
		
		unsigned int zqx = abc - xyz;
		
		if (zqx != 0)
			Debugger();
	}
	
    if (0)
    {
        // AbleDiskTool -s 5 -file "/Volumes/CJ Movies/XPL Build Products/Synclavier3/W0 Disk Image.simg" "-e" "CJ Data:Projects:SDC:Able:W0 Demo Files:CLARA3.ssnd" "W0:"
        argc = 8;
        argv[0] = "AbleDiskTool";
        argv[1] = "-s";
        argv[2] = "5";
        argv[3] = "-file";
        argv[4] = "/Users/cameronjones/Documents/Synclavier³/W0/USER.simg";
        argv[5] = "-e";
        argv[6] = "/Volumes/CJ Data/Projects/SDC/Able/W0 Demo Files/CLARA3.ssnd";
        argv[6] = "/Volumes/10.10/TestDLLs/•Sine.aif";
        argv[7] = "W0:";
    }

    if (0)
    {
        // AbleDiskTool W0 -e "/ATEXTFIL" "W0:ATEXTFIL"
        // AbleDiskTool -file "xtz" -e "/ATEXTFIL" "W0:"
        argc = 6;
		argv[0] = "something";
		argv[1] = "-file";
		argv[2] = "/A/Image File.simg";
		argv[3] = "-e";
		argv[4] = "/A/•NEWDATA.stmb";
		argv[5] = "W0:";
	}

	if (0)
	{
		// Works: AbleDiskTool -ic2 5 -e "/A/•NEWDATA.stmb" W0:
		// AbleDiskTool -ic2 4 -e "/A/•NEWDATA.stmb" "W1:"
		argc = 6;
		argv[0] = "AbleDiskTool";
		argv[1] = "-ic2";
		argv[2] = "4";
		argv[3] = "-e";
		argv[4] = "/A/•NEWDATA.stmb";
		argv[5] = "W1:";
	}
    
	if (0)
	{
		argc = 8;
		argv[0] = "AbleDiskTool";
		argv[1] = "-s";
		argv[2] = "5";
		argv[3] = "-file";
		argv[4] = "/Volumes/CJ Data/Projects/SDC/Synclavier Development 5.2/Synclavier Digital SynclavierX/Release 5.2 W0 Disk Image.simg";
		argv[5] = "-e";
		argv[6] = "/Volumes/CJ Data/Projects/SDC/Synclavier Development 5.2/Able/W0/*SYSTEM/";
		argv[7] = "W0:.SYSTEM";
	}
    
	if (0)
	{
		argc = 8;
		argv[0] = "AbleDiskTool";
		argv[1] = "-s";
		argv[2] = "5";
		argv[3] = "-file";
		argv[4] = "/Volumes/CJ Data/Projects/SDC/Synclavier Development 5.2/Synclavier Digital SynclavierX/Release 5.2 W0 Disk Image.simg";
		argv[5] = "-e";
		argv[6] = "/Volumes/CJ Data/Projects/SDC/Synclavier Development 5.2/Able/W0/MONITOR.sprg";
		argv[7] = "W0:MONITOR";
	}
    
	int status = AbleDiskTool(argc, argv);
	
	if (status)
		return (-9);
		
	return (0);
}
