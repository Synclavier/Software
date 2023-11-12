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
        // -w2 -s 5 -file /Volumes/CJ_Cache/XPL Build Products/Synclavier3/W0 Disk Image.simg -e /Volumes/CJ_Cache/Synclavier-Software/Projects/AbleBrowser/../../Able/W0 Demo Files/CLARA3.ssnd W0:
        argc = 0;
        argv[argc++] = "-w2";
        argv[argc++] = "-s";
        argv[argc++] = "5";
        argv[argc++] = "-file";
        argv[argc++] = "/Volumes/CJ_Cache/XPL Build Products/Synclavier3/W0 Disk Image.simg";
        argv[argc++] = "-e";
        argv[argc++] = "/Volumes/CJ_Cache/Synclavier-Software/Projects/AbleBrowser/../../Able/W0 Demo Files/CLARA3.ssnd";
        argv[argc++] = "W0:";
    }

    int status = AbleDiskTool(argc, argv);
	
	if (status)
		return (-9);
		
	return (0);
}
