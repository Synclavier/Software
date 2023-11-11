// Interpreter Settings Dialog

extern	void		handle_interpreter_settings_menu(struct interpreter_settings* theSettings, int d03Ord16HardwareAvailable, int polyHardwareAvailable, struct interpreter_midi_patching* thePatching);

#define SystemSevenOrBetter 1
#define SystemSevenOrLater  1

#if Rez
#include	<Types.r>
#include 	<SysTypes.r>
#endif

#include	"AbleInterpreterResIds.h"

#define ROW1		(ROW_HEIGHT/3)					// pixel of row 1 (from content top )
#define COL1		(COL_WIDTH/3)					// pixel of col 1 (from content left)

#define ROW_HEIGHT	22								// height of our rows    in pixels
#define COL_WIDTH   17								// width  of our columns in pixels

#define ROW2		( ROW1+ROW_HEIGHT)				// Handy macros
#define ROW3		( ROW2+ROW_HEIGHT)
#define ROW4		( ROW3+ROW_HEIGHT)
#define ROW5		( ROW4+ROW_HEIGHT)
#define ROW6		( ROW5+ROW_HEIGHT)
#define ROW7		( ROW6+ROW_HEIGHT)
#define ROW8		( ROW7+ROW_HEIGHT)
#define ROW9		( ROW8+ROW_HEIGHT)
#define ROW10		( ROW9+ROW_HEIGHT)
#define ROW11		(ROW10+ROW_HEIGHT)
#define ROW12		(ROW11+ROW_HEIGHT)
#define ROW13		(ROW12+ROW_HEIGHT)
#define ROW14		(ROW13+ROW_HEIGHT)
#define ROW15		(ROW14+ROW_HEIGHT)
#define ROW16		(ROW15+ROW_HEIGHT)
#define ROW17		(ROW16+ROW_HEIGHT)
#define ROW18		(ROW17+ROW_HEIGHT)
#define ROW19		(ROW18+ROW_HEIGHT)
#define ROW20		(ROW19+ROW_HEIGHT)
#define ROW21		(ROW20+ROW_HEIGHT)
#define ROW22		(ROW21+ROW_HEIGHT)
#define ROW23		(ROW22+ROW_HEIGHT)
#define ROW24		(ROW23+ROW_HEIGHT)
#define ROW25		(ROW24+ROW_HEIGHT)

#define COL2		( COL1+COL_WIDTH)
#define COL3		( COL2+COL_WIDTH)
#define COL4		( COL3+COL_WIDTH)
#define COL5		( COL4+COL_WIDTH)
#define COL6		( COL5+COL_WIDTH)
#define COL7		( COL6+COL_WIDTH)
#define COL8		( COL7+COL_WIDTH)
#define COL9		( COL8+COL_WIDTH)
#define COL10		( COL9+COL_WIDTH)
#define COL11		(COL10+COL_WIDTH)
#define COL12		(COL11+COL_WIDTH)
#define COL13		(COL12+COL_WIDTH)
#define COL14		(COL13+COL_WIDTH)
#define COL15		(COL14+COL_WIDTH)
#define COL16		(COL15+COL_WIDTH)
#define COL17		(COL16+COL_WIDTH)
#define COL18		(COL17+COL_WIDTH)
#define COL19		(COL18+COL_WIDTH)
#define COL20		(COL19+COL_WIDTH)
#define COL21		(COL20+COL_WIDTH)
#define COL22		(COL21+COL_WIDTH)
#define COL23		(COL22+COL_WIDTH)
#define COL24		(COL23+COL_WIDTH)
#define COL25		(COL24+COL_WIDTH)
#define COL26		(COL25+COL_WIDTH)
#define COL27		(COL26+COL_WIDTH)
#define COL28		(COL27+COL_WIDTH)
#define COL29		(COL28+COL_WIDTH)
#define COL30		(COL29+COL_WIDTH)
#define COL31		(COL30+COL_WIDTH)
#define COL32		(COL31+COL_WIDTH)
#define COL33		(COL32+COL_WIDTH)
#define COL34		(COL33+COL_WIDTH)
#define COL35		(COL34+COL_WIDTH)
#define COL36		(COL35+COL_WIDTH)
#define COL37		(COL36+COL_WIDTH)
#define COL38		(COL37+COL_WIDTH)
#define COL39		(COL38+COL_WIDTH)
#define COL40		(COL39+COL_WIDTH)


// Pop menus for cable timing dialog

#define	CABLE_MENU_WIDTH		(14*COL_WIDTH)
#define	NET_MENU_WIDTH		    (30*COL_WIDTH)
#define	SETTINGS_DIALOG_RIGHT	(COL40)

// Cable length popup menu
#if Rez
resource 'MENU' (CABLE_LENGTH_MENU_ID, "Cable Length", preload) {
    CABLE_LENGTH_MENU_ID,
    textMenuProc,
    allEnabled,
    enabled,
    "Cable Length",
    {
#else
        enum {CABLE_LENGTH_MENU_ENUM_LIST,
#endif
            
#if	!Rez
			CABLE_LENGTH_MENU_20,
#else
			"20 ft      (6 m)", noIcon, noKey, noMark, plain,
#endif
            
#if	!Rez
			CABLE_LENGTH_MENU_50,
#else
			"50 ft    (15 m)", noIcon, noKey, noMark, plain,
#endif
            
#if	!Rez
			CABLE_LENGTH_MENU_100,
#else
			"100 ft  (30 m)", noIcon, noKey, noMark, plain,
#endif
            
#if	!Rez
			CABLE_LENGTH_MENU_150,
#else
			"150 ft  (45 m)", noIcon, noKey, noMark, plain,
#endif
            
#if	!Rez
			CABLE_LENGTH_MENU_END
#else
		}
#endif
    };
    
#if	Rez
	resource 'CNTL' (CABLE_LENGTH_MENU_ID, preload, purgeable) {
		{0, 0, ROW_HEIGHT, CABLE_MENU_WIDTH},				//	rectangle of control
		popupTitleLeftJust, 								//	title position
		visible, 											//	make control visible
		COL_WIDTH*6, 										//	pixel width of title
		CABLE_LENGTH_MENU_ID, 								//	'MENU' resource ID
		popupMenuCDEFProc, 									//	control definition ID
		0, 													//	reference value
		"Cable Length:" 									//	control title
	};
#endif
    
    
    // Bus loading
#if Rez
	resource 'MENU' (BUS_LOADING_MENU_ID, "Bus Loading", preload) {
		BUS_LOADING_MENU_ID,
		textMenuProc,
		allEnabled,
		enabled,
		"Bus Loading",
		{
#else
            enum {BUS_LOADING_MENU_ENUM_LIST,
#endif
                
#if	!Rez
                BUS_LOADING_MENU_LOW,
#else
                "Low                   ", noIcon, noKey, noMark, plain,
#endif
                
#if	!Rez
                BUS_LOADING_MENU_MEDIUM,
#else
                "Medium            ", noIcon, noKey, noMark, plain,
#endif
                
#if	!Rez
                BUS_LOADING_MENU_HIGH,
#else
                "High               ", noIcon, noKey, noMark, plain,
#endif
                
#if	!Rez
                BUS_LOADING_MENU_END
#else
            }
#endif
        };
        
#if	Rez
        resource 'CNTL' (BUS_LOADING_MENU_ID, preload, purgeable) {
            {0, 0, ROW_HEIGHT, CABLE_MENU_WIDTH},				//	rectangle of control
            popupTitleLeftJust, 								//	title position
            visible, 											//	make control visible
            COL_WIDTH*6, 										//	pixel width of title
            BUS_LOADING_MENU_ID, 								//	'MENU' resource ID
            popupMenuCDEFProc, 									//	control definition ID
            0, 													//	reference value
            "Bus Loading:" 										//	control title
        };
#endif
        
        // Net VIsible
#if Rez
        resource 'MENU' (NET_VISIBLE_MENU_ID, "Net Visible", preload) {
            NET_VISIBLE_MENU_ID,
            textMenuProc,
            0,
            enabled,
            "Net Visible",
            {
#else
                enum {NET_VISIBLE_MENU_ENUM_LIST,
#endif
                    
#if	!Rez
                    NET_VISIBLE_MENU_NOT_VISIBLE,
#else
                    "Program Linking Not Available In OS X", noIcon, noKey, noMark, plain,
#endif
                    
#if	!Rez
                    NET_VISIBLE_MENU_VISIBLE,
#else
                    "Program Linking Not Available In OS X", noIcon, noKey, noMark, plain,
#endif
                    
#if	!Rez
                    NET_VISIBLE_MENU_END
#else
                }
#endif
            };
            
#if	Rez
            resource 'CNTL' (NET_VISIBLE_MENU_ID, preload, purgeable) {
                {0, 0, ROW_HEIGHT, NET_MENU_WIDTH},					//	rectangle of control
                popupTitleLeftJust, 								//	title position
                visible, 											//	make control visible
                COL_WIDTH*6, 										//	pixel width of title
                NET_VISIBLE_MENU_ID, 								//	'MENU' resource ID
                popupMenuCDEFProc, 									//	control definition ID
                0, 													//	reference value
                "Network:" 											//	control title
            };
#endif
            
            // Use MP if available
#if Rez
            resource 'MENU' (USE_MP_MENU_ID, "Use MP", preload) {
                USE_MP_MENU_ID,
                textMenuProc,
                0,
                enabled,
                "Use MP",
                {
#else
                    enum {USE_MP_MENU_ENUM_LIST,
#endif
                        
#if	!Rez
                        USE_MP_MENU_DO_NOT_USE,
#else
                        "Processor Selection Is Automatic In OS X     ", noIcon, noKey, noMark, plain,
#endif
                        
#if	!Rez
                        USE_MP_MENU_USE_MP,
#else
                        "Processor Selection Is Automatic In OS X     ", noIcon, noKey, noMark, plain,
#endif
                        
#if	!Rez
                        USE_MP_MENU_END
#else
                    }
#endif
                };
                
#if	Rez
                resource 'CNTL' (USE_MP_MENU_ID, preload, purgeable) {
                    {0, 0, ROW_HEIGHT, NET_MENU_WIDTH},					//	rectangle of control
                    popupTitleLeftJust, 								//	title position
                    visible, 											//	make control visible
                    COL_WIDTH*6, 										//	pixel width of title
                    USE_MP_MENU_ID, 									//	'MENU' resource ID
                    popupMenuCDEFProc, 									//	control definition ID
                    0, 													//	reference value
                    "Processor:" 										//	control title
                };
#endif
                
                // MP not available
#if Rez
                resource 'MENU' (USE_MP_NO_MP_ID, "No MP Available", preload) {
                    USE_MP_NO_MP_ID,
                    textMenuProc,
                    allEnabled,
                    enabled,
                    "No MP Available",
                    {
                        "No secondary processor is available on this Macintosh       ", noIcon, noKey, noMark, plain,
                    }
                };
                
                resource 'CNTL' (USE_MP_NO_MP_ID, preload, purgeable) {
                    {0, 0, ROW_HEIGHT, NET_MENU_WIDTH},					//	rectangle of control
                    popupTitleLeftJust, 								//	title position
                    visible, 											//	make control visible
                    COL_WIDTH*6, 										//	pixel width of title
                    USE_MP_NO_MP_ID, 									//	'MENU' resource ID
                    popupMenuCDEFProc, 									//	control definition ID
                    0, 													//	reference value
                    "Processor:" 										//	control title
                };
#endif
                
                // M512k Usage
#if Rez
                resource 'MENU' (M512K_MENU_ID, "M512k Menu", preload) {
                    M512K_MENU_ID,
                    textMenuProc,
                    allEnabled,
                    enabled,
                    "M512k Menu",
                    {
#else
                        enum {M512K_MENU_ENUM_LIST,
#endif
                            
#if	!Rez
                            M512K_MENU_MODEL_3,
#else
                            "Provide 3 M512k Cards                                                             ", noIcon, noKey, noMark, plain,
#endif
                            
#if	!Rez
                            M512K_MENU_MODEL_4,
#else
                            "Provide 4 M512k Cards", noIcon, noKey, noMark, plain,
#endif
                            
#if	!Rez
                            M512K_MENU_MODEL_5,
#else
                            "Provide 5 M512k Cards", noIcon, noKey, noMark, plain,
#endif
                            
#if	!Rez
                            M512K_MENU_MODEL_10,
#else
                            "Provide 10 M512k Cards", noIcon, noKey, noMark, plain,
#endif
                            
#if	!Rez
                            M512K_MENU_END
#else
                        }
#endif
                    };
                    
#if	Rez
                    resource 'CNTL' (M512K_MENU_ID, preload, purgeable) {
                        {0, 0, ROW_HEIGHT, NET_MENU_WIDTH},					//	rectangle of control
                        popupTitleLeftJust, 								//	title position
                        visible, 											//	make control visible
                        COL_WIDTH*6, 										//	pixel width of title
                        M512K_MENU_ID, 										//	'MENU' resource ID
                        popupMenuCDEFProc, 									//	control definition ID
                        0, 													//	reference value
                        "M512k Cards:" 										//	control title
                    };
#endif
                    
                    
                    // POLY simulated memory
#if Rez
                    resource 'MENU' (POLY_MENU_ID, "Poly Menu", preload) {
                        POLY_MENU_ID,
                        textMenuProc,
                        allEnabled,
                        enabled,
                        "Poly Menu",
                        {
#else
                            enum {POLY_MENU_ENUM_LIST,
#endif
                                
#if	!Rez
                                POLY_MENU_MODEL_5,
#else
                                "Simulate 5 Megabytes Poly Memory                                              ", noIcon, noKey, noMark, plain,
#endif
                                
#if	!Rez
                                POLY_MENU_MODEL_25,
#else
                                "Simulate 25 Megabytes Poly Memory", noIcon, noKey, noMark, plain,
#endif
                                
#if	!Rez
                                POLY_MENU_MODEL_100,
#else
                                "Simulate 100 Megabytes Poly Memory", noIcon, noKey, noMark, plain,
#endif
                                
#if	!Rez
                                POLY_MENU_MODEL_200,
#else
                                "Simulate 200 Megabytes Poly Memory", noIcon, noKey, noMark, plain,
#endif
                                
#if	!Rez
                                POLY_MENU_MODEL_512,
#else
                                "Simulate 512 Megabytes Poly Memory", noIcon, noKey, noMark, plain,
#endif
                                
#if	!Rez
                                POLY_MENU_END
#else
                            }
#endif
                        };
                        
#if	Rez
                        resource 'CNTL' (POLY_MENU_ID, preload, purgeable) {
                            {0, 0, ROW_HEIGHT, NET_MENU_WIDTH},					//	rectangle of control
                            popupTitleLeftJust, 								//	title position
                            visible, 											//	make control visible
                            COL_WIDTH*6, 										//	pixel width of title
                            POLY_MENU_ID, 										//	'MENU' resource ID
                            popupMenuCDEFProc, 									//	control definition ID
                            0, 													//	reference value
                            "Poly Memory:" 										//	control title
                        };
#endif
                        
                        
                        // Cable Timing Dialog
                        // Handle the settings menu
#if !Rez
                        enum {SETTINGS_DIALOG_OK_FRAME_ITEM  =  3,
                            SETTINGS_DIALOG_CABLE_ITEM     =  6,
                            SETTINGS_DIALOG_BUS_ITEM       =  7,
                            SETTINGS_DIALOG_NETWORK_ITEM   =  9,
                            SETTINGS_DIALOG_MP_ITEM        = 11,
                            SETTINGS_DIALOG_MP_NAVAIL_ITEM = 12,
                            SETTINGS_DIALOG_M512K_ITEM     = 13,
                            SETTINGS_DIALOG_POLY_ITEM      = 14,
                            SETTINGS_DIALOG_CAL_MET        = 16};
#endif
                        
#if Rez
                        resource 'DLOG' (SETTINGS_DIALOG_ID, "Settings", purgeable) {
                            { ROW2, COL2, ROW24, SETTINGS_DIALOG_RIGHT},
                            documentProc, invisible, noGoAway, 0x0, SETTINGS_DIALOG_ID, "Settings Entry", alertPositionParentWindow
                        };
                        
                        resource 'DITL' (SETTINGS_DIALOG_ID, "Settings Entry", purgeable) {
                            {
                                // Deafult Button: Done
                                { ROW21, COL32, ROW22, COL37 },
                                Button { enabled, "Done" },
                                
                                // Cancel
                                { ROW21, COL24, ROW22, COL29 },
                                Button { enabled, "Cancel" },
                                
                                // Default Button Frame
                                { ROW21 - 4, COL32 - 4, ROW22 + 4, COL37 + 4 },
                                UserItem { disabled },
                                
                                // Instructions par 1
                                { ROW1, COL7, ROW4, SETTINGS_DIALOG_RIGHT - 2*COL_WIDTH },
                                StaticText {
                                    disabled,
                                    "Use the Cable Length and Bus Loading menus to describe your system "
                                    "configuration.  If you experience system hangs or crashing, you most "
                                    "likely need to set a longer cable length or heavier bus loading."
                                },
                                
                                // Instructions par 2
                                { ROW3 + ROW_HEIGHT/2, COL7, ROW5 + ROW_HEIGHT/2, SETTINGS_DIALOG_RIGHT - 2*COL_WIDTH },
                                StaticText {
                                    disabled,
                                    "A shorter cable length and lighter bus loading provide somewhat faster access "
                                    "to small, lightly-loaded systems."
                                },
                                
                                { ROW5 + ROW_HEIGHT/2, COL7, ROW5 + ROW_HEIGHT/2 + ROW_HEIGHT, COL7 + CABLE_MENU_WIDTH },
                                Control { enabled, CABLE_LENGTH_MENU_ID },
                                
                                { ROW7, COL7, ROW7 + ROW_HEIGHT, COL7 + CABLE_MENU_WIDTH },
                                Control { enabled, BUS_LOADING_MENU_ID },
                                
                                // Instructions for network
                                { ROW8 + ROW_HEIGHT/2, COL7, ROW11, SETTINGS_DIALOG_RIGHT - 2*COL_WIDTH },
                                StaticText {
                                    disabled,
                                    "Use the following menu to enable or disable access to this SynclavierX "
                                    "from your network. You must Quit and relaunch SynclavierX "
                                    "when you change this setting."
                                },
                                
                                { ROW11, COL7, ROW11 + ROW_HEIGHT, COL7 + NET_MENU_WIDTH },
                                Control { disabled, NET_VISIBLE_MENU_ID },
                                
                                // Instructions for MP
                                { ROW13, COL7, ROW14, SETTINGS_DIALOG_RIGHT - 2*COL_WIDTH },
                                StaticText {
                                    disabled,
                                    "Additional Options:"
                                },
                                
                                { ROW14, COL7, ROW14 + ROW_HEIGHT, COL7 + NET_MENU_WIDTH },
                                Control { enabled, USE_MP_MENU_ID },
                                
                                { ROW14, COL7, ROW14 + ROW_HEIGHT, COL7 + NET_MENU_WIDTH },
                                Control { enabled, USE_MP_NO_MP_ID },
                                
                                { ROW15 + ROW_HEIGHT/2, COL7, ROW16 + ROW_HEIGHT/2, COL7 + NET_MENU_WIDTH },
                                Control { enabled, M512K_MENU_ID },
                                
                                { ROW17, COL7, ROW18 + ROW_HEIGHT/2, COL7 + NET_MENU_WIDTH },
                                Control { enabled, POLY_MENU_ID },
                                
                                { ROW18 + ROW_HEIGHT/2, COL7, ROW19 + ROW_HEIGHT/2, COL13 },
                                StaticText { disabled, "Metronome:" },
                                
                                { ROW18 + ROW_HEIGHT/2, COL13, ROW19 + ROW_HEIGHT/2, COL24 },
                                Button { enabled, "Calibrate Metronome"},
                                
                                // Icon
                                { ROW1, ROW1, ROW1+64, ROW1+64},
                                Icon { disabled, SETTINGS_DIALOG_ICON_64 },
                            }
                        };
#endif
                        
                        
                        // Must relaunch app to make changes effective
#if Rez
                        resource 'ALRT' (NEED_TO_RELAUNCH_ID, "NEED_TO_RELAUNCH_ID", purgeable) {
                            {ROW5, COL2, ROW12, COL23},
                            NEED_TO_RELAUNCH_ID,
                            {	/* array: 4 elements */
                                /* [1] */
                                Cancel, visible, sound1,
                                /* [2] */
                                Cancel, visible, sound1,
                                /* [3] */
                                Cancel, visible, sound1,
                                /* [4] */
                                Cancel, visible, sound1,
                            },
                            alertPositionParentWindow
                        };
                        
                        resource 'DITL' (NEED_TO_RELAUNCH_ID, "NEED_TO_RELAUNCH_ID", purgeable) {
                            {	/* array DITLarray: 2 elements */
                                /* [1] */
                                {ROW1, COL5, ROW4, COL21},
                                StaticText {
                                    disabled,
                                    "The changes you have made to the Processor, M512k and Poly Memory settings will not "
                                    "become effective until you Quit and Relaunch ^0"
                                },
                                
                                /* [2] */
                                {ROW5, COL8, ROW7, COL15},
                                Button {
                                    enabled,
                                    "OK"
                                },
                            }
                        };
#endif
                        
                        // PCI-1 is in use by another application
#if Rez
                        resource 'ALRT' (PCI1_IN_USE_ALERT, "PCI1_IN_USE_ALERT", purgeable) {
                            {ROW5, COL2, ROW17, COL40},
                            PCI1_IN_USE_ALERT,
                            {	/* array: 4 elements */
                                /* [1] */
                                Cancel, visible, sound1,
                                /* [2] */
                                Cancel, visible, sound1,
                                /* [3] */
                                Cancel, visible, sound1,
                                /* [4] */
                                Cancel, visible, sound1,
                            },
                            alertPositionParentWindow
                        };
                        
                        resource 'DITL' (PCI1_IN_USE_ALERT, "PCI1_IN_USE_ALERT", purgeable) {
                            {	/* array DITLarray: 2 elements */
                                /* [1] */
                                {ROW1, COL5, ROW4, COL38},
                                StaticText {
                                    disabled,
                                    "The PCI-1 hardware installed in your Macintosh is being used by another application at this time.  Or "
                                    "perhaps the application that last used the PCI-1 hardware did not Quit correctly."
                                },
                                
                                /* [2] */
                                {ROW10, COL2, ROW12, COL12},
                                Button {
                                    enabled,
                                    "Quit"
                                },
                                
                                /* [3] */
                                {ROW10, COL15, ROW12, COL25},
                                Button {
                                    enabled,
                                    "Continue With No Voices"
                                },
                                
                                /* [4] */
                                {ROW10, COL28, ROW12, COL38},
                                Button {
                                    enabled,
                                    "Quit Then Restart"
                                },
                                
                                {ROW4, COL5, ROW6, COL38},
                                StaticText {
                                    disabled,
                                    "If another application is currently using the PCI-1 hardware, "
                                    "you must Quit that other application before SynclavierX can access "
                                    "your Synclavier® voices."
                                },
                                
                                
                                {ROW6, COL5, ROW8, COL38},
                                StaticText {
                                    disabled,
                                    "Or, if the application that last used the PCI-1 hardware did not Quit correctly "
                                    "you must Restart your Macintosh."
                                },
                                
                                {ROW8, COL5, ROW9, COL28},
                                StaticText {
                                    disabled,
                                    "What would you like me to do?"
                                },
                            }
                        };
#endif
                        
                        
                        // Should update sound file cache
#if Rez
                        resource 'ALRT' (CACHE_UPDATE_NEEDED_ID, "CACHE_UPDATE_NEEDED_ID", purgeable) {
                            {ROW5, COL2, ROW11, COL28},
                            CACHE_UPDATE_NEEDED_ID,
                            {	/* array: 4 elements */
                                /* [1] */
                                Cancel, visible, sound1,
                                /* [2] */
                                Cancel, visible, sound1,
                                /* [3] */
                                Cancel, visible, sound1,
                                /* [4] */
                                Cancel, visible, sound1,
                            },
                            alertPositionMainScreen
                        };
                        
                        resource 'DITL' (CACHE_UPDATE_NEEDED_ID, "CACHE_UPDATE_NEEDED_ID", purgeable) {
                            {	/* array DITLarray: 2 elements */
                                /* [1] */
                                {ROW1, COL5, ROW3 + ROW_HEIGHT/2, COL25},
                                StaticText {
                                    disabled,
                                    "InterChange™ has moved one or more subcatalogs or sound files.  Please update "
                                    "the Sound File Directory using the Update button on the B screen."
                                },
                                
                                /* [2] */
                                {ROW4, COL2, ROW6, COL12},
                                Button {
                                    enabled,
                                    "OK"
                                },
                                
                                /* [3] */
                                {ROW4, COL15, ROW6, COL25},
                                Button {
                                    enabled,
                                    "Thank you, and don't\never warn me again!"
                                },
                                
                            }
                        };
#endif
                        
                        
