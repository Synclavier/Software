///  Created by Cameron Jones on 8/26/12.
//

// Synclavier3Constants.h

#ifndef __SynclavierConstants__
#define __SynclavierConstants__

#include "SynclavierTypes.h"

// UI Object IDs and constants.
// These objects are easier to find programatically instead of using IB outlet refs, so we use UIIDs to do so.
#define SYNC_BUTTON_UIID             1000   // Object is one of the button panel button images
#define SYNC_PANEL_STRIDE             100   // Stride for encoding of panel id in button object id
#define SYNC_WHEEL_BUTTON_UIID       2000   // Object is one of 9 buttons above wheel
#define SYNC_WHEEL_IMAGE_UIID        2020   // Object is one of 8 wheel images
#define SYNC_BUTTON_DISPLAY_UIID     2030   // Object is one of 2 lines of text display in the BP display window
#define SYNC_BUTTON_DECIMAL_UIID     2040   // Object is one of 32 decimal points in the BP display window
#define SYNC_BUTTON_SCROLLER_UIID    2010   // Object is one of 2 scrollers for the button panel window
#define SYNC_BUTTON_BACKGROUND_UIID  3000   // Object is the button panel background image
#define SYNC_TERMINAL_WINDOW_UIID    4000   // Object is the terminal window background image
#define SYNC_TERMINAL_LINES_UIID     4100   // Object is one of the 25 lines of terminal text
#define SYNC_TERMINAL_DWDH_UIID      4200   // Object is one of the 12 lines of double-wide-double-high terminal text

#define SYNC_NUM_BUTTON_WINDOWS       6     // Max number of BP windows managed by the software
#define SYNC_NUM_PANELS              10     // Number of button panels...
#define SYNC_NUM_BUTTONS_PER_PANEL   16     // ... each with 16 buttons
#define SYNC_NUM_WHEEL_BUTTONS        9     // Number of buttons above the wheel
#define SYNC_NUM_WHEEL_IMAGES         8     // Number of rotated wheel images we display
#define SYNC_NUM_DISPLAY_LINES        2     // Number of lines of text deisplay
#define SYNC_NUM_DISPLAY_CHARS       16     // Number of characters in each line of the text display
#define SYNC_NUM_TERMINAL_LINES      25     // Number of text field lines in the terminal window
#define SYNC_NUM_TERMINAL_CHARS      80     // Number of characters on each line of the terminal window
#define SYNC_NUM_DWDH_LINES          12     // Number of double-wide-double-high text field lines in the terminal window
#define SYNC_NUM_DWDH_CHARS          40     // Number of double-wide-double-high characters on each line of the terminal window

#define SYNC_SCROLL_BAR_WIDTH        16     // Width of scroll bar - adjust window by this much when hiding them

// Menu and view tags
#define SYNC_ZOOM_IN_TAG            1000
#define SYNC_ZOOM_OUT_TAG           1001
#define SYNC_ZOOM_NORMAL_TAG        1002
#define SYNC_SHOW_OTHER_WINDOW_TAG  1003

#define SYNC_MENU_PF1_TAG           1100
#define SYNC_MENU_PF2_TAG           1101
#define SYNC_MENU_PF3_TAG           1102
#define SYNC_MENU_PF4_TAG           1103
#define SYNC_MENU_PAUSE_TAG         1104
#define SYNC_MENU_BREAK_TAG         1105
#define SYNC_MENU_RESET_TAG         1106
#define SYNC_MENU_CLEAR_HOLDS_TAG   1107
#define SYNC_MENU_UPDATE_PANELS_TAG 1108
#define SYNC_MENU_BAUD_9600_TAG     1109
#define SYNC_MENU_BAUD_19200_TAG    1110
#define SYNC_MENU_BAUD_38400_TAG    1111
#define SYNC_MENU_BAUD_115200_TAG   1112

#define SYNC_MENU_QUIT_TAG          1200
#define SYNC_MENU_START_REAL_TIME   1201
#define SYNC_MENU_STOP_REAL_TIME    1202
#define SYNC_MENU_LAUNCH_BTB1TEST   1203
#define SYNC_MENU_START_MONITOR     1204
#define SYNC_MENU_CLOSE_IMPORT      1205
#define SYNC_MENU_CLOSE_FORCE_QUIT  1206
#define SYNC_MENU_QUIT_SYNC3        1207
#define SYNC_MENU_SAVE              1208
#define SYNC_MENU_SAVE_AS           1209
#define SYNC_MENU_SHOW_CUR_FILE     1210
#define SYNC_MENU_EXPORT_MIDI       1211
#define SYNC_MENU_IMPORT_MIDI       1212
#define SYNC_MENU_START_D40         1213
#define SYNC_MENU_STOP_D40          1214
#define SYNC_MENU_POLL_FOR_PORTS    1215
#define SYNC_MENU_CLOSE_PORT        1216
#define SYNC_MENU_PORT_MENU_ITEMS   1900

#define SYNC_MIDI_PATCHING_TAG      1250
#define SYNC_MIDI_PATCHING_TO_TAG   1300
#define SYNC_MIDI_PATCHING_SMC_TO   1350
#define SYNC_MIDI_PATCHING_FROM_TAG 1400
#define SYNC_MIDI_PATCHING_SMC_FROM 1450

#define SYNC_REGISTRATION_FIELDS    1500    // Tags are used to save & restore text field settings

#define SYNC_IMPORT_FORMAT_CAF      1601
#define SYNC_IMPORT_FORMAT_AIFF     1602
#define SYNC_IMPORT_FORMAT_WAVE     1603
#define SYNC_IMPORT_FORMAT_SNCL     1604

#define SYNC_IMPORT_NAME_REPLACE    1610
#define SYNC_IMPORT_NAME_SKIP       1611

#define SYNC_IMPORT_D24_TAB         1700
#define SYNC_IMPORT_D24_W0          1701
#define SYNC_IMPORT_D24_W1          1702
#define SYNC_IMPORT_D24_O0          1703
#define SYNC_IMPORT_D24_O1          1704

#define SYNC_SCSI_TIMING_PREFS      1800

#define SYNC_SPINNER_TAG            1003

#define	SYNC_MAX_MIDI_CABLE_NUM     80
#define SYNC_SYNC_SYSEX_ID          0x000204
#define SYNC_MACKIE_SYSEX_ID        0x000066

typedef enum SyncButtonState
{
    SyncButtonOff       = 0x00,             // Handy state literals
    SyncButtonOn        = 0x01,
    SyncButtonBlinking  = 0x02,
    SyncButtonUnknown   = 0x03,
    SyncButtonHeld      = 0x04
    
} SyncButtonState;

typedef enum SyncButtonPressed
{
    SyncButtonPressedNO      = 0x00,        // Handy pressed literals
    SyncButtonPressedYES     = 0x01,
    SyncButtonPressedUnknown = 0x02
} SyncButtonPressed;

// Application-wide notifications
#define SYNC_NOTIFICATION_WAKE_UP               @"SYNC_NOTIFICATION_WAKE_UP"                // Application launch; wake from sleep; becoming active on IOS
#define SYNC_NOTIFICATION_SLEEP                 @"SYNC_NOTIFICATION_SLEEP"                  // Sleeping (OSX); resigning active (IOS)
#define SYNC_NOTIFICATION_STATUS_CHANGE         @"SYNC_NOTIFICATION_STATUS_CHANGE"          // Network/Connection change
#define SYNC_NOTIFICATION_PREF_CHANGED          @"SYNC_NOTIFICATION_PREF_CHANGED"           // Prefs setting may have changed
#define SYNC_NOTIFICATION_RESIZE                @"SYNC_NOTIFICATION_RESIZE"                 // Resize due to rotation
#define SYNC_NOTIFICATION_SERVICE_DIRECTORY     @"SYNC_NOTIFICATION_SERVICE_DIRECTORY"      // Contents of service directory changed
#define SYNC_NOTIFICATION_MIDI_DIRECTORY        @"SYNC_NOTIFICATION_MIDI_DIRECTORY"         // Contents of MIDI directory changed (either input or output)
#define SYNC_NOTIFICATION_DATA_BASE_CHANGED     @"SYNC_NOTIFICATION_DATA_BASE_CHANGED"      // New data base selected or created. Also used to start a scan.
#define SYNC_NOTIFICATION_DATA_BASE_SAVED       @"SYNC_NOTIFICATION_DATA_BASE_SAVED"        // Changes in the data base were saved. Implies screens should be updated.
#define SYNC_NOTIFICATION_DATA_BASE_STATUS      @"SYNC_NOTIFICATION_DATA_BASE_STATUS"       // Asset Library Data Base status changed (rebuild started, completed or posted progress)
#define SYNC_NOTIFICATION_DATA_BASE_CLOSE       @"SYNC_NOTIFICATION_DATA_BASE_CLOSE "       // Asset Library Data Base is closing
#define SYNC_NOTIFICATION_CACHE_PREFS           @"SYNC_NOTIFICATION_CACHE_PREFS "           // Caching preferences changed
#define SYNC_NOTIFICATION_ABLE_START_STOP       @"SYNC_NOTIFICATION_ABLE_START_STOP "       // Able interpreter started or stopped
#define SYNC_NOTIFICATION_NEW_QUIT_MENU          "SYNC_NOTIFICATION_NEW_QUIT_MENU "         // Able interpreter posted a new quit menu

// Notifications also posted from C++
#define SYNC_NOTIFICATION_MIDI_PATCHING         "SYNC_NOTIFICATION_MIDI_PATCHING "          // MIDI Patching change

// Preference Identifiers
#define SYNC_PREF_PREFS_REV_NUM                 8
#define SYNC_PREF_PREFS_REV                     @"SYNC_PREF_PREFS_REV"
#define SYNC_PREF_REJOIN_RECENT_SESSION         @"SYNC_PREF_REJOIN_RECENT_SESSION"
#define SYNC_PREF_BUTTON_CLICK_VOLUME           @"SYNC_PREF_BUTTON_CLICK_VOLUME"
#define SYNC_PREF_BUTTON_DIM_SCREEN             @"SYNC_PREF_BUTTON_DIM_SCREEN"
#define SYNC_PREF_BUTTON_BRIGHTNESS             @"SYNC_PREF_BUTTON_BRIGHTNESS"
#define SYNC_PREF_ACTIVE_SENSE                  @"SYNC_PREF_ACTIVE_SENSE"
#define SYNC_PREF_ACTIVE_SENSE                  @"SYNC_PREF_ACTIVE_SENSE"
#define SYNC_PREF_FASTER_ERROR_RECOVERY         @"SYNC_PREF_FASTER_ERROR_RECOVERY"
#define SYNC_PREF_HINTS_SHOWN_GROUP_1           @"SYNC_PREF_HINTS_SHOWN_GROUP_1"
#define SYNC_PREF_VIRTUAL_DESTINATION_ID        @"SYNC_PREF_VIRTUAL_DESTINATION_ID"
#define SYNC_PREF_VIRTUAL_SOURCE_ID             @"SYNC_PREF_VIRTUAL_SOURCE_ID"
#define SYNC_PREF_RECENT_CONNECTION_NAME        @"SYNC_PREF_RECENT_CONNECTION_NAME"
#define SYNC_PREF_RECENT_CONNECTION_ADDR        @"SYNC_PREF_RECENT_CONNECTION_ADDR"
#define SYNC_PREF_RECENT_CONNECTION_PORT        @"SYNC_PREF_RECENT_CONNECTION_PORT"
#define SYNC_PREF_FAVORITES                     @"SYNC_PREF_FAVORITES"
#define SYNC_PREF_RECENT_CONNECTION             @"SYNC_PREF_RECENT_CONNECTION"
#define SYNC_PREF_TERMINAL_ZOOM                 @"SYNC_PREF_TERMINAL_ZOOM"
#define SYNC_PREF_BUTTON_PANEL_ZOOM             @"SYNC_PREF_BUTTON_PANEL_ZOOM"
#define SYNC_PREF_BUTTON_PANEL_X                @"SYNC_PREF_BUTTON_PANEL_X"
#define SYNC_PREF_BUTTON_PANEL_Y                @"SYNC_PREF_BUTTON_PANEL_Y"
#define SYNC_PREF_BUTTON_PANEL_TAG              @"SYNC_PREF_BUTTON_PANEL_TAG"
#define SYNC_PREF_PREFS_PANEL_TAB               @"SYNC_PREF_PREFS_PANEL_TAB"
#define SYNC_PREF_MIDI_WINDOW_TAB               @"SYNC_PREF_MIDI_WINDOW_TAB"
#define SYNC_PREF_PREFS_IMPORT_TAB              @"SYNC_PREF_PREFS_IMPORT_TAB"
#define SYNC_PREF_PREFS_CALIB_NUM               @"SYNC_PREF_PREFS_CALIB_NUM"
#define SYNC_PREF_PREFS_CALIB_DEN               @"SYNC_PREF_PREFS_CALIB_DEN"
#define SYNC_PREF_RESTORE_WINDOWS               @"SYNC_PREF_RESTORE_WINDOWS"
#define SYNC_PREF_RESTORE_WINDOWS_KEY           @"NSQuitAlwaysKeepsWindows"
#define SYNC_PREF_CACHE_SEARCH_RESULTS          @"SYNC_PREF_CACHE_SEARCH_RESULTS"
#define SYNC_PREF_PREFETCH_SEARCH_RESULTS       @"SYNC_PREF_PREFETCH_SEARCH_RESULTS"
#define SYNC_PREF_PREFETCH_SEARCH_LIMIT         @"SYNC_PREF_PREFETCH_SEARCH_LIMIT"
#define SYNC_PREF_TEST_BOOKMARK                 @"SYNC_PREF_TEST_BOOKMARK"
#define SYNC_PREF_SYNC_MIDI_CONTROL_TO          @"SYNC_PREF_SYNC_MIDI_CONTROL_TO"
#define SYNC_PREF_SYNC_MIDI_CONTROL_FROM        @"SYNC_PREF_SYNC_MIDI_CONTROL_FROM"
#define SYNC_PREF_SAVED_FIELD                   @"SYNC_PREF_SAVED_FIELD"
#define SYNC_PREF_INSTALLATION_ID               @"SYNC_PREF_INSTALLATION_ID"
#define SYNC_PREF_INSTALLATION_CODES            @"SYNC_PREF_INSTALLATION_CODES"
#define SYNC_PREF_INSTALLATION_CODE             @"SYNC_PREF_INSTALLATION_CODE"
#define SYNC_PREF_ASSET_DATA_BASE_PATH          @"SYNC_PREF_ASSET_DATA_BASE_PATH"
#define SYNC_PREF_COPY_DATA_BASE_ON_NEW         @"SYNC_PREF_COPY_DATA_BASE_ON_NEW"
#define SYNC_PREF_MOUNT_SERVERS_DURING_REBUILD  @"SYNC_PREF_MOUNT_SERVERS_DURING_REBUILD"
#define SYNC_PREF_MOUNT_ROOTS_DURING_STARTUP    @"SYNC_PREF_MOUNT_ROOTS_DURING_STARTUP"
#define SYNC_PREF_IMPORT_FILE_BOOKMARK          @"SYNC_PREF_IMPORT_FILE_BOOKMARK"
#define SYNC_PREF_IMPORT_CREATE_FOLDER          @"SYNC_PREF_IMPORT_CREATE_FOLDER"
#define SYNC_PREF_IMPORT_CREATE_FOLDER_BOOKMARK @"SYNC_PREF_IMPORT_CREATE_FOLDER_BOOKMARK"
#define SYNC_PREF_IMPORT_AUDIO_FORMAT           @"SYNC_PREF_IMPORT_AUDIO_FORMAT"
#define SYNC_PREF_IMPORT_NAME_REPLACE           @"SYNC_PREF_IMPORT_NAME_REPLACE"
#define SYNC_PREF_IMPORT_D24_SELECTION          @"SYNC_PREF_IMPORT_D24_SELECTION"
#define SYNC_PREF_IMPORT_ADD_TO_LIBRARY         @"SYNC_PREF_IMPORT_ADD_TO_LIBRARY"
#define SYNC_PREF_SHOW_FILES_IN_LOG             @"SYNC_PREF_SHOW_FILES_IN_LOG"
#define SYNC_PREF_DISCLOSURE_WINDOW_STATE       @"SYNC_PREF_DISCLOSURE_WINDOW_STATE"
#define SYNC_PREF_ASSET_WINDOW_TAG              @"SYNC_PREF_ASSET_WINDOW_TAG"
#define SYNC_PREF_ASSET_WINDOW_TAB              @"SYNC_PREF_ASSET_WINDOW_TAB"
#define SYNC_PREF_ASSET_WINDOW_SELECTED_ROOTS   @"SYNC_PREF_ASSET_WINDOW_SELECTED_ROOTS"
#define SYNC_PREF_ASSET_VIEW_SELECTED_URIS      @"SYNC_PREF_ASSET_VIEW_SELECTED_URIS"
#define SYNC_PREF_ASSET_VIEW_TOP_URI            @"SYNC_PREF_ASSET_VIEW_TOP_URI"
#define SYNC_PREF_ASSET_CHOOSE_FOLDER_BOOKMARK  @"SYNC_PREF_ASSET_CHOOSE_FOLDER_BOOKMARK"
#define SYNC_PREF_ASSET_RECENT_TAB              @"SYNC_PREF_ASSET_RECENT_TAB"
#define SYNC_PREF_ASSET_VIEW_ACTIVE_OBJECT      @"SYNC_PREF_ASSET_VIEW_ACTIVE_OBJECT"
#define SYNC_PREF_ASSET_VIEW_SEARCH_FIELDS      @"SYNC_PREF_ASSET_VIEW_SEARCH_FIELDS"
#define SYNC_PREF_ASSET_VIEW_ZOOM               @"SYNC_PREF_ASSET_VIEW_ZOOM"
#define SYNC_PREF_ASSET_VIEW_SELECTION          @"SYNC_PREF_ASSET_VIEW_SELECTION"
#define SYNC_PREF_REAL_TIME_POLL_DTD            @"SYNC_PREF_REAL_TIME_POLL_DTD"
#define SYNC_PREF_REAL_TIME_POLL_D70MIDI        @"SYNC_PREF_REAL_TIME_POLL_D70MIDI"
#define SYNC_PREF_REAL_TIME_POLL_D34GPI         @"SYNC_PREF_REAL_TIME_POLL_D34GPI"
#define SYNC_PREF_REAL_TIME_MONITOR_BOOT        @"SYNC_PREF_REAL_TIME_MONITOR_BOOT"
#define SYNC_PREF_REAL_TIME_PED2_MAX            @"SYNC_PREF_REAL_TIME_PED2_MAX"
#define SYNC_PREF_REAL_TIME_CREATE_CLOCK        @"SYNC_PREF_REAL_TIME_CREATE_CLOCK"
#define SYNC_PREF_REAL_TIME_B_SCREEN_CACHE       "SYNC_PREF_REAL_TIME_B_SCREEN_CACHE"
#define SYNC_PREF_REAL_TIME_MONO_VOICES          "SYNC_PREF_REAL_TIME_MONO_VOICES"
#define SYNC_PREF_REAL_TIME_BLACK_ON_WHITE      @"SYNC_PREF_REAL_TIME_BLACK_ON_WHITE"
#define SYNC_PREF_REAL_TIME_RECORD_SUSTAIN      @"SYNC_PREF_REAL_TIME_RECORD_SUSTAIN"
#define SYNC_PREF_REAL_TIME_USE_MTC             @"SYNC_PREF_REAL_TIME_USE_MTC"
#define SYNC_PREF_REAL_TIME_XPOS_MIDI_OUT       @"SYNC_PREF_REAL_TIME_XPOS_MIDI_OUT"
#define SYNC_PREF_REAL_TIME_XPOS_VALUE          @"SYNC_PREF_REAL_TIME_XPOS_VALUE"
#define SYNC_PREF_REAL_TIME_POLL_TRACE          @"SYNC_PREF_REAL_TIME_POLL_TRACE"
#define SYNC_PREF_FS_EVENTS_LAST_ID             @"SYNC_PREF_FS_EVENTS_LAST_ID"
#define SYNC_PREF_MONITOR_FILE_SYSTEM           @"SYNC_PREF_MONITOR_FILE_SYSTEM"
#define SYNC_PREF_REFRESH_ASSET_WINDOWS         @"SYNC_PREF_REFRESH_ASSET_WINDOWS"
#define SYNC_PREF_CREATE_BACKUPS                @"SYNC_PREF_CREATE_BACKUPS"
#define SYNC_PREF_BACKUP_ON_QUIT                @"SYNC_PREF_BACKUP_ON_QUIT"
#define SYNC_PREF_PREFETCH_DATA_BASE            @"SYNC_PREF_PREFETCH_DATA_BASE"
#define SYNC_PREF_BUILD_INDEX                   @"SYNC_PREF_BUILD_INDEX"
#define SYNC_PREF_BUILD_CASE_INDEX              @"SYNC_PREF_BUILD_CASE_INDEX"
#define SYNC_PREF_scsi_d24_read_time            @"SYNC_PREF_scsi_d24_read_time"
#define SYNC_PREF_scsi_d24_write_time           @"SYNC_PREF_scsi_d24_write_time"
#define SYNC_PREF_scsi_d25_read_time            @"SYNC_PREF_scsi_d25_read_time"
#define SYNC_PREF_scsi_d25_write_time           @"SYNC_PREF_scsi_d25_write_time"
#define SYNC_PREF_scsi_d27_write_time           @"SYNC_PREF_scsi_d27_write_time"
#define SYNC_PREF_scsi_end_timing               @"SYNC_PREF_scsi_end_timing"
#define SYNC_PREF_poly_read_time                @"SYNC_PREF_poly_read_time"
#define SYNC_PREF_poly_write_time               @"SYNC_PREF_poly_write_time"
#define SYNC_PREF_poly_end_timing               @"SYNC_PREF_poly_end_timing"
#define SYNC_PREF_generic_read_time             @"SYNC_PREF_generic_read_time"
#define SYNC_PREF_generic_setup_time            @"SYNC_PREF_generic_setup_time"
#define SYNC_PREF_generic_write_time            @"SYNC_PREF_generic_write_time"
#define SYNC_PREF_generic_end_timing            @"SYNC_PREF_generic_end_timing"

#define SYNC_PREF_ADD_TO_PARTIAL_STYLE          @"SYNC_PREF_ADD_TO_PARTIAL_STYLE"
#define SYNC_PREF_RECALL_SOUND_STYLE            @"SYNC_PREF_RECALL_SOUND_STYLE"

#define SYNC_PREF_soundOpenALWindow             @"SYNC_PREF_soundOpenALWindow"
#define SYNC_PREF_soundShowTab                  @"SYNC_PREF_soundShowTab"
#define SYNC_PREF_soundWarn                     @"SYNC_PREF_soundWarn"
#define SYNC_PREF_soundFilter                   @"SYNC_PREF_soundFilter"
#define SYNC_PREF_soundSelect                   @"SYNC_PREF_soundSelect"
#define SYNC_PREF_soundCallUp                   @"SYNC_PREF_soundCallUp"
#define SYNC_PREF_soundAlwaysAdd                @"SYNC_PREF_soundAlwaysAdd"

#define SYNC_PREF_timbreOpenALWindow            @"SYNC_PREF_timbreOpenALWindow"
#define SYNC_PREF_timbreShowTab                 @"SYNC_PREF_timbreShowTab"
#define SYNC_PREF_timbreWarn                    @"SYNC_PREF_timbreWarn"
#define SYNC_PREF_timbreFilter                  @"SYNC_PREF_timbreFilter"
#define SYNC_PREF_timbreSelect                  @"SYNC_PREF_timbreSelect"
#define SYNC_PREF_timbreCallUp                  @"SYNC_PREF_timbreCallUp"

#define SYNC_PREF_sequenceOpenALWindow          @"SYNC_PREF_sequenceOpenALWindow"
#define SYNC_PREF_sequenceShowTab               @"SYNC_PREF_sequenceShowTab"
#define SYNC_PREF_sequenceWarn                  @"SYNC_PREF_sequenceWarn"
#define SYNC_PREF_sequenceFilter                @"SYNC_PREF_sequenceFilter"
#define SYNC_PREF_sequenceSelect                @"SYNC_PREF_sequenceSelect"
#define SYNC_PREF_sequenceCallUp                @"SYNC_PREF_sequenceCallUp"

#define SYNC_PREF_W1_DEVICE_TYPE                @"SYNC_PREF_W1_DEVICE_TYPE"
#define SYNC_PREF_W1_SCSI_ID                    @"SYNC_PREF_W1_SCSI_ID"
#define SYNC_PREF_W1_FILE_BOOKMARK              @"SYNC_PREF_W1_FILE_BOOKMARK"

#define SYNC_PREF_O0_DEVICE_TYPE                @"SYNC_PREF_O0_DEVICE_TYPE"
#define SYNC_PREF_O0_SCSI_ID                    @"SYNC_PREF_O0_SCSI_ID"
#define SYNC_PREF_O0_FILE_BOOKMARK              @"SYNC_PREF_O0_FILE_BOOKMARK"

#define SYNC_PREF_MIDI_TO                       @"SYNC_PREF_MIDI_TO"
#define SYNC_PREF_MIDI_FROM                     @"SYNC_PREF_MIDI_FROM"

#define SYNC_PREF_TIMBRE_EXPORT_UNIVERSE        @"SYNC_PREF_TIMBRE_EXPORT_UNIVERSE"
#define SYNC_PREF_TIMBRE_EXPORT_DOMAIN          @"SYNC_PREF_TIMBRE_EXPORT_DOMAIN"
#define SYNC_PREF_TIMBRE_EXPORT_LIBRARY         @"SYNC_PREF_TIMBRE_EXPORT_LIBRARY"
#define SYNC_PREF_TIMBRE_EXPORT_LEVELS          @"SYNC_PREF_TIMBRE_EXPORT_LEVELS"
#define SYNC_PREF_TIMBRE_EXPORT_PREPEND         @"SYNC_PREF_TIMBRE_EXPORT_PREPEND"

#define SYNC_PREF_BAUD_RATE                     @"SYNC_PREF_BAUD_RATE"
#define SYNC_PREF_SERIAL_PORT_NAME              @"SYNC_PREF_SERIAL_PORT_NAME"

#define SYNC_PREF_CONNECTION_NAME               @"SYNC_PREF_CONNECTION_NAME"
#define SYNC_PREF_CONNECTION_ADDR               @"SYNC_PREF_CONNECTION_ADDR"
#define SYNC_PREF_CONNECTION_PORT               @"SYNC_PREF_CONNECTION_PORT"

typedef enum SyncHintsGroup1
{
    SyncHintWelcomeHint      = 0x00000001,
    SyncHintConnectionHint   = 0x00000002,

} SyncHintsGroup1;

#define SYNC_MEASURE_TIME_A         CFTimeInterval abc, xyz = CACurrentMediaTime();
#define SYNC_MEASURE_TIME_B         {abc = CACurrentMediaTime(); NSLog(@"Time: %0.9f",abc - xyz);xyz = abc;}

#endif
