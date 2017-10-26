/* WindowUtilities.h */

#ifndef	Window_UTILITIES_H

enum	WU_CreateWindowFromNibOptions {

		WU_InstallDefaultEventHandler = 0x00000001,
		WU_ShowWindowWhenCreated      = 0x00000002,
		WU_FlushQDWhenCreated         = 0x00000004
};

void	WU_CreateWindowFromNib(const char * nibFileName, const char * windowName, WindowRef& outWindowRef, UInt32 options );

#endif