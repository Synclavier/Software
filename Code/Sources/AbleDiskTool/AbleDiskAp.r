// InterChangeª Dialogs

#if	Rez
	#define SystemSevenOrBetter 1
	#define SystemSevenOrLater  1
	
	#include "Types.r"
	#include "SysTypes.r"
#endif

#ifndef __ABLE_DISK_AP_R__
#define __ABLE_DISK_AP_R__


// Alert & Dialog resource id
enum												// alerts and dialogs
{
	MISSING_RESOURCE_ALERT_DIALOG_ID = 1024,
	BAD_SCSI_CONFIG_ALERT_DIALOG_ID,
	BAD_FILE_CONFIG_ALERT_DIALOG_ID,
	BAD_SIZE_STRING_ALERT_DIALOG_ID,
	ARE_YOU_SURE_ALERT_DIALOG_ID,
	SAVE_SETTINGS_DIALOG_ID,
	ENTER_IMAGE_SIZE_DIALOG_ID,
	EXPORT_DIALOG_ID,
	NEED_WHAT_TO_IMPORT_ALERT,
	NEED_WHERE_TO_PUT_IT,
	NEED_WHAT_TO_EXPORT_ALERT,
	NEED_WHERE_TO_EXPORT_TO,
	CONFIRM_ERASE_OF_DISK,
	DEVICE_NOT_CONFIGURE_ALERT,
	IMAGE_FILE_NOT_AVAILABLE,
	IMAGE_FILE_RENAMED_NOT_AVAILABLE,
	CHOSEN_FILE_RENAMED_NOT_AVAILABLE,
	INTERCHANGE_ABOUT_68K_RES_ID,
	INTERCHANGE_ABOUT_PPC_RES_ID,
	ASK_FOR_FILE_NAME_DIALOG_ID,
	DUP_FILE_NAME_DIALOG_ID
};

// PopUp Menu resource IDs	
enum												// popup menus
{
	SCSI_BUS_MENU_ID = 1024,
	SCSI_ID_MENU_ID,
	SCSI_ID_5_MENU_ID,
	SCSI_ID_4_MENU_ID,
	SCSI_ID_1_MENU_ID,
	SCSI_ID_0_MENU_ID,
	WHAT_TO_DO_MENU_ID,
	WHAT_TO_IMPORT_MENU_ID,
	LOGGING_OPTIONS_MENU_ID,
	ERROR_OPTIONS_MENU_ID,
	FILENAMES_OPTIONS_MENU_ID,
	DISK_IMAGE_OPTIONS_MENU_ID,
	SUBCATALOG_OPTIONS_MENU_ID,
	REPLACING_OPTIONS_MENU_ID,
	SOUNDFILE_OPTIONS_MENU_ID
};

// Custom resource IDs
enum												// resouce of saved settings
{
	SCSI_SETTINGS_ID = 1024
};


// Resources in compiled form

enum												// our picture
{
	INTERCHANGE_ABOUT_68K_PICT_RES_ID	= 129,
	INTERCHANGE_ABOUT_PPC_PICT_RES_ID	= 130
};

enum												// ICON and cicn for dialog boxes
{
	EXPORT_DIALOG_ICON_64 = 128,					// 64 * 64 icon
	APPLE_DISK_TYPE_ICON  = 129,
	ABLE_DISK_TYPE_ICON   = 130,
	EXPORT_DIALOG_ICON_48 = 131						// 48 * 48 icon
};

enum												// id of first of 4 spinning cursors
{
	SPIN_CURSOR_ID = 128
};

enum												// resource id of dlog & ditl for selecting directory
{
	SELECT_DIRECTORY_ID	  = 128
};


// Screen parameters
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


// Export dialog description
#define EXPORT_DIALOG_ULX			15			// global coordinate at startup
#define EXPORT_DIALOG_ULY			50

#define NUM_EXPORT_ROWS				20			// size of window in ros & columns
#define NUM_EXPORT_COLS				40

#define EXPORT_DIALOG_LRX			(EXPORT_DIALOG_ULX+COL1+NUM_EXPORT_COLS*COL_WIDTH+COL1 )
#define EXPORT_DIALOG_LRY			(EXPORT_DIALOG_ULY+ROW1+NUM_EXPORT_ROWS*ROW_HEIGHT+ROW1)


// Setup area layout
#define	SETTINGS_ROW			(ROW1)												// row of settings start
#define	W0_SETUP_ROW			(SETTINGS_ROW)										// row of W0 setup line
#define	W1_SETUP_ROW			(W0_SETUP_ROW + ROW_HEIGHT)							// row of W1 setup line
#define O0_SETUP_ROW			(W1_SETUP_ROW + ROW_HEIGHT)							// row of O0 setup line
#define O1_SETUP_ROW			(O0_SETUP_ROW + ROW_HEIGHT)							// row of O1 setup line
#define SAVE_SETTINGS_ROW		(O1_SETUP_ROW + ROW_HEIGHT + ROW_HEIGHT/3)			// row of save config buttons
#define SETTINGS_HEIGHT			(SAVE_SETTINGS_ROW + ROW_HEIGHT - SETTINGS_ROW)		// height of settings area

#define LABEL_WIDTH				( 1 * COL_WIDTH)									// width of device label
#define	BUS_MENU_WIDTH			(13 * COL_WIDTH)									// width in columns of bus pop-up menu
#define	ID_MENU_WIDTH			( 4	* COL_WIDTH)									// width in columns of id pop-up menu
#define	GET_INFO_WIDTH			( 4	* COL_WIDTH)									// width in columns of get info button
#define	CHOOSE_WIDTH			( 4 * COL_WIDTH)									// width of choose button
#define CREATE_WIDTH			( 4 * COL_WIDTH)									// width of create button
#define DISK_NAME_WIDTH			(14 * COL_WIDTH)					// width of disk image file name field

#define SETUP_LABEL_COL			(COL1)												// col of device lable (w0, w1, etc.)
#define SETUP_BUS_COL			(SETUP_LABEL_COL  + LABEL_WIDTH	    + COL_WIDTH)	// col of bus selection pop-up
#define SETUP_ID_COL			(SETUP_BUS_COL    + BUS_MENU_WIDTH  + COL_WIDTH)	// col of device id selection pop-up
#define SETUP_INFO_COL			(SETUP_ID_COL     + ID_MENU_WIDTH   + COL_WIDTH)	// col of get info button

#define	SETUP_CH00SE_COL		(SETUP_BUS_COL    + BUS_MENU_WIDTH  + COL_WIDTH) 	// col of choose button
#define	SETUP_CREATE_COL		(SETUP_CH00SE_COL + CHOOSE_WIDTH    + COL_WIDTH) 	// col of create button
#define SETUP_NAME_COL			(SETUP_CREATE_COL + CREATE_WIDTH    + COL_WIDTH)	// col of disk image file name
#define SETUP_ICON_COL			(SETUP_NAME_COL   + DISK_NAME_WIDTH - COL_WIDTH) 	// col of disk type icon

// What to do layout
#define	DO_IT_ROW				(SETTINGS_ROW + SETTINGS_HEIGHT + ROW_HEIGHT)		// row of do it region
#define	WHAT_TO_DO_ROW			(DO_IT_ROW)
#define	WHAT_TO_DO_COL			(COL1)
#define WHAT_TO_DO_TITLE_WIDTH	(7  * COL_WIDTH)
#define	WHAT_TO_DO_MENU_WIDTH	(WHAT_TO_DO_TITLE_WIDTH + (24 * COL_WIDTH))

#define	WHAT_TO_IMPORT_ROW		(WHAT_TO_DO_ROW + ROW_HEIGHT + (ROW_HEIGHT/2))
#define	WHAT_TO_IMPORT_WIDTH	(WHAT_TO_DO_TITLE_WIDTH + (11 * COL_WIDTH))
#define WHAT_TO_IMPORT_COL		(WHAT_TO_DO_COL + WHAT_TO_DO_TITLE_WIDTH)

#define	WHAT_TO_EXPORT_ROW		(WHAT_TO_DO_ROW + ROW_HEIGHT + (ROW_HEIGHT/2))
#define WHAT_TO_EXPORT_COL		(WHAT_TO_DO_COL + WHAT_TO_DO_TITLE_WIDTH)
#define CHOOSE_FOLDER_WIDTH		(12 * COL_WIDTH)
#define CHOSEN_TEXT_COL			(WHAT_TO_EXPORT_COL + CHOOSE_FOLDER_WIDTH + COL_WIDTH)

#define	WHERE_TO_PUT_IT_ROW		(WHAT_TO_EXPORT_ROW  + ROW_HEIGHT + (ROW_HEIGHT/2))

#define DO_IT_BUTTON_ROW		(WHERE_TO_PUT_IT_ROW + ROW_HEIGHT + (ROW_HEIGHT/2))
#define DO_IT_BUTTON_HEIGHT		(2*ROW_HEIGHT)

#define DO_IT_HEIGHT			(DO_IT_BUTTON_ROW + DO_IT_BUTTON_HEIGHT + (ROW_HEIGHT/2) - DO_IT_ROW)

// Options layout
#define OPTIONS_ROW				(DO_IT_ROW + DO_IT_HEIGHT + (ROW_HEIGHT/2))
#define OPTIONS_MENU_COL		(WHAT_TO_DO_COL + WHAT_TO_DO_TITLE_WIDTH)
#define OPTIONS_MENU_WIDTH		(26*COL_WIDTH)
#define OPTIONS_TITLE_COL		(COL2)
#define OPTIONS_TITLE_WIDTH		(5  * COL_WIDTH)

#define	LOGGING_OPTIONS_ROW		(OPTIONS_ROW                        )
#define	ERROR_OPTIONS_ROW		(LOGGING_OPTIONS_ROW    + ROW_HEIGHT)
#define	FILENAMES_OPTIONS_ROW	(ERROR_OPTIONS_ROW      + ROW_HEIGHT)
#define	DISK_IMAGE_OPTIONS_ROW	(FILENAMES_OPTIONS_ROW  + ROW_HEIGHT)
#define	SUBCATALOG_OPTIONS_ROW	(DISK_IMAGE_OPTIONS_ROW + ROW_HEIGHT)
#define	REPLACING_OPTIONS_ROW	(SUBCATALOG_OPTIONS_ROW + ROW_HEIGHT)
#define SOUNDFILE_OPTIONS_ROW	(REPLACING_OPTIONS_ROW  + ROW_HEIGHT)
#define OPTIONS_HEIGHT			(SOUNDFILE_OPTIONS_ROW  + ROW_HEIGHT - OPTIONS_ROW)

#endif