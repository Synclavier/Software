// InterChange� Menus#if	Rez	#define SystemSevenOrBetter 1	#define SystemSevenOrLater  1		#include "Types.r"	#include "SysTypes.r"#endif// Resources contained herein:#define	FIRST_MENU_ID		APPLE_MENU_ID#define	FIRST_SUBMENU_ID	MORE_OPTIONS_SUBMENU_IDenum {											// Menus defined herein		APPLE_MENU_INDEX,							// Enums to access menus in order	FILE_MENU_INDEX,	EDIT_MENU_INDEX,	INTERCHANGE_MENU_INDEX,	NUMBER_OF_MENUS};				enum {											// Subenus defined herein		MORE_OPTIONS_SUBMENU_INDEX,					// Enums to access menus in order	NUMBER_OF_SUBMENUS};				enum{	APPLE_MENU_ID				= 32000,		// chosen to match SIOUX resource IDs...	FILE_MENU_ID,	EDIT_MENU_ID,	INTERCHANGE_MENU_ID,	NEXT_AVAILABLE_MENU_ID};enum											// submenus; need character codes as well...{	MORE_OPTIONS_SUBMENU_ID 	= 1,	NEXT_AVAILABLE_SUBMENU_ID};#define	MORE_OPTIONS_SUBMENU_CHAR "\001"	// Apple Menu#if Rez		resource 'MENU' (APPLE_MENU_ID, "Apple Menu", preload) {		APPLE_MENU_ID,		textMenuProc,		allEnabled,		enabled,		apple,		{#else	enum {APPLE_MENU_ENUM_LIST,#endif		#if	!Rez			APPLE_MENU_ABOUT,		#else			"About InterChange��", noIcon, noKey, noMark, plain,		#endif				#if	!Rez			APPLE_MENU_DIVIDER_1,		#else			"-", noIcon, noKey, noMark, plain,		#endif		#if	!Rez			APPLE_MENU_END		#else		}		#endif};// File Menu#if Rez		resource 'MENU' (FILE_MENU_ID, "File Menu", preload) {		FILE_MENU_ID,		textMenuProc,		allEnabled,		enabled,		"File",		{#else	enum {FILE_MENU_ENUM_LIST,#endif		#if	!Rez			FILE_MENU_NEW,		#else			"New�", noIcon, noKey, noMark, plain,		#endif				#if	!Rez			FILE_MENU_OPEN,		#else			"Open�", noIcon, noKey, noMark, plain,		#endif				#if	!Rez			FILE_MENU_CLOSE,		#else			"Close�", noIcon, noKey, noMark, plain,		#endif				#if	!Rez			FILE_MENU_SAVE,		#else			"Save�", noIcon, "S", noMark, plain,		#endif				#if	!Rez			FILE_MENU_DIVIDER_1,		#else			"-", noIcon, noKey, noMark, plain,		#endif				#if	!Rez			FILE_MENU_PAGE_SETUP,		#else			"Page Setup�", noIcon, noKey, noMark, plain,		#endif				#if	!Rez			FILE_MENU_PRINT,		#else			"Print�", noIcon, "P", noMark, plain,		#endif				#if	!Rez			FILE_MENU_DIVIDER_2,		#else			"-", noIcon, noKey, noMark, plain,		#endif				#if	!Rez			FILE_MENU_QUIT,		#else			"Quit", noIcon, "Q", noMark, plain,		#endif		#if	!Rez			FILE_MENU_END		#else		}		#endif};// EDIT Menu#if Rez		resource 'MENU' (EDIT_MENU_ID, "Edit Menu", preload) {		EDIT_MENU_ID,		textMenuProc,		allEnabled,		enabled,		"Edit",		{#else	enum {EDIT_MENU_ENUM_LIST,#endif		#if	!Rez			EDIT_MENU_UNDO,		#else			"Undo", noIcon, "Z", noMark, plain,		#endif				#if	!Rez			EDIT_MENU_DIVIDER_1,		#else			"-", noIcon, noKey, noMark, plain,		#endif				#if	!Rez			EDIT_MENU_CUT,		#else			"Cut", noIcon, "X", noMark, plain,		#endif				#if	!Rez			EDIT_MENU_COPY,		#else			"Copy", noIcon, "C", noMark, plain,		#endif		#if	!Rez			EDIT_MENU_PASTE,		#else			"Paste", noIcon, "V", noMark, plain,		#endif		#if	!Rez			EDIT_MENU_CLEAR,		#else			"Clear", noIcon, noKey, noMark, plain,		#endif		#if	!Rez			EDIT_MENU_DIVIDER_2,		#else			"-", noIcon, noKey, noMark, plain,		#endif		#if	!Rez			EDIT_MENU_SELECT_ALL,		#else			"Select All", noIcon, "A", noMark, plain,		#endif		#if	!Rez			EDIT_MENU_END		#else		}		#endif};// INTERCHANGE Menu#if Rez		resource 'MENU' (INTERCHANGE_MENU_ID, "InterChange�", preload) {		INTERCHANGE_MENU_ID,		textMenuProc,		allEnabled,		enabled,		"InterChange�",		{#else	enum {INTERCHANGE_MENU_ENUM_LIST,#endif		#if	!Rez			INTERCHANGE_MENU_CLOSE_LOG_WINDOW,		#else			"Close Log Window", noIcon, noKey, noMark, plain,		#endif				#if	!Rez			INTERCHANGE_MENU_HIDE_SETTINGS,		#else			"Hide Setup", noIcon, noKey, noMark, plain,		#endif				#if	!Rez			INTERCHANGE_MENU_HIDE_OPTIONS,		#else			"Hide Options", noIcon, noKey, noMark, plain,		#endif				#if	!Rez			INTERCHANGE_MENU_DIVIDER_1,		#else			"-", noIcon, noKey, noMark, plain,		#endif				#if	!Rez			INTERCHANGE_MENU_MORE_OPTIONS,		#else			"More Options", noIcon, hierarchicalMenu, MORE_OPTIONS_SUBMENU_CHAR, plain,		#endif		#if	!Rez			INTERCHANGE_MENU_END		#else		}		#endif};// More Options Submenu#if Rez		resource 'MENU' (MORE_OPTIONS_SUBMENU_ID, "MORE_OPTIONS_SUBMENU_ID", preload) {		MORE_OPTIONS_SUBMENU_ID,		textMenuProc,		allEnabled,		enabled,		"MORE_OPTIONS_SUBMENU_ID",		{#else	enum {MORE_OPTIONS_MENU_ENUM_LIST,#endif		#if	!Rez			MORE_OPTIONS_ALLOW_ERASE,		#else			"Allow Erase of Macintosh Disks", noIcon, noKey, noMark, plain,		#endif				#if	!Rez			MORE_OPTIONS_DO_EJECT,		#else			"Eject Media After Export", noIcon, noKey, noMark, plain,		#endif				#if	!Rez			MORE_OPTIONS_RECOGNIZE_DISKS,		#else			"Recognize Synclavier� Disks in All Cases", noIcon, noKey, noMark, plain,		#endif				#if	!Rez			MORE_OPTIONS_COERCE_RATE,		#else			"Limit Sample Rate setting of AIFF, Sd2f and WAVE files to 48 Khz", noIcon, noKey, noMark, plain,		#endif				#if	!Rez			MORE_OPTIONS_END		#else		}		#endif};