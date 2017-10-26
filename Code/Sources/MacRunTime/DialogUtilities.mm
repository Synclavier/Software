/* DialogUtilities.c */

// 32-bit Carbon implementation of primitive error message output

#include "Standard.h"

#include "DialogUtilities.h"
#include "AbleInterpreterResIds.h"

typedef struct stuff {
    int         errorNum;               // Error code
    const char* errorStr;               // Error string
    
} stuff;

stuff stuffs[] =
{
    {NEED_TO_RELAUNCH_ID, "The changes you have made to the Processor, M512k and Poly Memory settings will not become effective until you Quit and Relaunch " SYNCLAVIER_APPLICATION_NAME "."},
    {PCI1_IN_USE_ALERT, "The PCI-1 hardware installed in your Macintosh is being used by another application at this time."},
    {CACHE_UPDATE_NEEDED_ID, "The SCSI hardware setup has changed.  You may need to update the Sound File Directory using the Update button on the B screen."},
    {NEED_W0_IMAGE_FIRST_ALERT_ID, "The W0: Disk Image File that should have been installed with your software could not be located."},
    {NEED_W0_IMAGE_UPDATE_ALERT_ID, "The W0: Disk Image File that was installed with your new software could not be used.  Either your new software was not installed correctly or some other error occured."},
    {COULD_NOT_FIND_WINBOOT, "The file called WINBOOT (WINBOOT.sprg) that should have been installed with your software could not be located.  Either your software was not installed correctly or some other error occured."},
    {LIKELY_OUT_OF_MEMORY, "It looks like there is not enough Macintosh memory available right now to run " SYNCLAVIER_APPLICATION_NAME "."},
    {NEED_UPDATED_EDITVIEW, "An older version of EditView®, MIDINet®, AutoConform™ or TransferMation™ is trying to communicate with " SYNCLAVIER_APPLICATION_NAME ". This older version does not work correctly with " SYNCLAVIER_APPLICATION_NAME "."},
    {W0_FILE_IS_MISSING, "Your setting for W0 is incorrect. It looks like the Image File you chose is no longer available."},
    {0, NULL}
};

static const char* DU_string_for_code(int code)
{
    int which = 0;
    
    while (stuffs[which].errorNum != code) {
        if (stuffs[which].errorNum == 0)
            return NULL;
     
        which++;
    }

    return stuffs[which].errorStr;
}

static  void    DU_show_ns_alert(const char* str)
{
    if (str == NULL)
        str = "An unknown unidentified error has apparently occurred.";
    
    NSAlert* alert = [NSAlert alertWithMessageText:NSLocalizedString(@SYNCLAVIER_APPLICATION_NAME @" Notification", @"Title of error message dialog")
                                     defaultButton:nil
                                   alternateButton:nil
                                       otherButton:nil
                         informativeTextWithFormat:@"%@", [NSString stringWithUTF8String:str]];
    
    [alert runModal];
}

static  void    DU_show_ns_alert(int code)
{
    const char* str = DU_string_for_code(code);
    
    DU_show_ns_alert(str);
}

void DU_show_working_window(short dialogID, const char *line_1_msg, const char* line_2_msg )
{
    SyncPrintf("DU_show_working_window %d, %s, %s\n", dialogID, line_1_msg, line_2_msg);
}

void	DU_remove_working_dialog()
{
    SyncPrintf("DU_remove_working_dialog\n");
}

SInt16  DU_CautionAlert(SInt16 alertID, void* upp)
{
    DU_show_ns_alert(alertID);
    
    return noErr;
}

SInt16  DU_StopAlert(SInt16 alertID, void* upp)
{
    DU_show_ns_alert(alertID);
    
    return noErr;
}

void    DU_ReportErr(const char *s) {
    DU_show_ns_alert(s);
}

