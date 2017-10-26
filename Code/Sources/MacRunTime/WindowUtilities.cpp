//--------------------------------------------------------------------------------------------
// Window Utilities
//--------------------------------------------------------------------------------------------

#include <Carbon/Carbon.h>

// Local includes
#include "WindowUtilities.h"

//QuickDraw Deprecated Includes
extern void
QDFlushPortBuffer(
                  CGrafPtr    port,
                  RgnHandle   region);

// For now, optimize for 1 nib file.
static IBNibRef     sNibRef;
static CFStringRef	sNibName;

// Default window handler detects close of self
static const EventTypeSpec  kWU_DefaultEvents[] =
{
	{ kEventClassWindow, kEventWindowClosed }
};

static OSStatus WU_DefaultEventHandler( EventHandlerCallRef inCaller, EventRef inEvent, void* inRefcon )	// Note inRefCon is a WindowRef * !!!!
{
    OSStatus    err        = eventNotHandledErr;
	WindowRef*	windowRefP = (WindowRef*) inRefcon;
	
	// Already closed?
	if (!windowRefP || !*windowRefP)
		return err;

	switch ( GetEventKind( inEvent ) )
	{
		case kEventWindowClosed:
			*windowRefP = NULL;
				
			err = noErr;
			break;
	};
    
    return err;
}

static DEFINE_ONE_SHOT_HANDLER_GETTER( WU_DefaultEventHandler )

void	WU_CreateWindowFromNib(const char * nibFileName, const char * windowName, WindowRef& outWindowRef, UInt32 options )
{
	CFStringRef	nibName = CFStringCreateWithCString(kCFAllocatorDefault, nibFileName, kCFStringEncodingUTF8);
	CFStringRef	winName = CFStringCreateWithCString(kCFAllocatorDefault, windowName,  kCFStringEncodingUTF8);
	
	outWindowRef = NULL;
	
	// Check for changing nib files
	if (sNibRef && !CFEqual(sNibName, nibName))
	{
		DisposeNibReference(sNibRef);  sNibRef =  NULL;
		CFRelease          (sNibName); sNibName = NULL;
	}

	// Cache nib file reference
	if (!sNibRef)
	{
		CreateNibReference(nibName, &sNibRef);
		sNibName = nibName;
		CFRetain(sNibName);
	}
	
	if (!sNibRef)
		return;
		
	CreateWindowFromNib( sNibRef, winName, &outWindowRef);
	
	// Apply options
	if (outWindowRef)
	{
		if (options & WU_ShowWindowWhenCreated)
			ShowWindow(outWindowRef);
			
		if (options & WU_FlushQDWhenCreated)
			QDFlushPortBuffer(GetWindowPort(outWindowRef), NULL);
			
		if (WU_InstallDefaultEventHandler)
			InstallWindowEventHandler( outWindowRef, GetWU_DefaultEventHandlerUPP(),
                               GetEventTypeCount( kWU_DefaultEvents ), kWU_DefaultEvents,
                               &outWindowRef, NULL );
	}
}
