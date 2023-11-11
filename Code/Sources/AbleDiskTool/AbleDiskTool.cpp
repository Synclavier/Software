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
        // -create "/Volumes/CJ_Cache/XPL Build Products/Synclavier3/W0 Disk Image.simg" 7340032 -zero
        argc = 0;
        argv[argc++] = "AbleDiskTool";
        argv[argc++] = "-create";
        argv[argc++] = "/Volumes/CJ_Cache/XPL_Build_Products/Synclavier3/W0 Disk Image.simg";
        argv[argc++] = "7340032";
        argv[argc++] = "-zero";
    }
    
    if (0)
    {
        // -w2 -s 5 -file "/Volumes/CJ_Cache/XPL_Build_Products/Synclavier3/W0 Disk Image.simg" "-e" "/Volumes/CJ_Cache/XPL_Build_Products/W0/MONITOR.sprg" "W0:MONITOR"
        argc = 0;
        argv[argc++] = "-w2";
        argv[argc++] = "-s";
        argv[argc++] = "5";
        argv[argc++] = "-file";
        argv[argc++] = "/Volumes/CJ_Cache/XPL_Build_Products/Synclavier3/W0 Disk Image.simg";
        argv[argc++] = "-e";
        argv[argc++] = "/Volumes/CJ_Cache/XPL_Build_Products/W0/MONITOR.sprg";
        argv[argc++] = "W0:MONITOR";
    }

    if (0)
    {
        // -w2 -s 5 -file "/Volumes/CJ_Cache/XPL_Build_Products/Synclavier3/W0 Disk Image.simg" "-e" "/Volumes/CJ_Cache/XPL_Build_Products/W0/*SYSTEM/" "W0:.SYSTEM"
        argc = 0;
        argv[argc++] = "-w2";
        argv[argc++] = "-s";
        argv[argc++] = "5";
        argv[argc++] = "-file";
        argv[argc++] = "/Volumes/CJ_Cache/XPL_Build_Products/Synclavier3/W0 Disk Image.simg";
        argv[argc++] = "-e";
        argv[argc++] = "/Volumes/CJ_Cache/XPL_Build_Products/W0/*SYSTEM/";
        argv[argc++] = "W0:.SYSTEM";
    }

    if (0)
    {
        // -w2 -s 5 -file ~/Documents/Synclavier³/W0/USER.simg -e ~/Desktop/TestText.txt W0:TESTTEXT
        argc = 0;
        argv[argc++] = "-w2";
        argv[argc++] = "-s";
        argv[argc++] = "5";
        argv[argc++] = "-file";
        argv[argc++] = "/Users/cameronjones/Documents/Synclavier³/W0/USER.simg";
        argv[argc++] = "-e";
        argv[argc++] = "/Users/cameronjones/Desktop/TestText.txt";
        argv[argc++] = "W0:TESTTEXT";
    }

    int status = AbleDiskTool(argc, argv);
	
	if (status)
		return (-9);
		
	return (0);
}
