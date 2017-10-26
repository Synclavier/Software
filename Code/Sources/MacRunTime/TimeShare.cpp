// 	TimeShare.c

//	C. Jones
//	6/4/99

//	Handy routine to timeshare at the main event level.

// 	Local Includes
#include "TimeShare.h"

// Global Variables
static	RgnHandle	updateList[100];
static	WindowPtr	windowList[100];
static	int			updateSize;
	
// Initialize
void	TimeShare_Initialize()
{
	updateSize = 0;
}

// TimeShare
void	TimeShare_TimeShare(short events_to_toss)
{
	EventRecord e;

	if (WaitNextEvent(events_to_toss | updateMask, &e, 1, NULL))
	{
		if (e.what == updateEvt && updateSize < 100)
		{
			WindowPtr  theWindow = (WindowPtr) e.message;
			RgnHandle  updateRgn = NewRgn();
			
			if (theWindow && updateRgn && (GetWindowRegion(theWindow, kWindowUpdateRgn, updateRgn) == noErr))
			{
				GrafPtr cur_port;

				GetPort(&cur_port); 

				SetPort    (GetWindowPort(theWindow));
				BeginUpdate(theWindow);
				EndUpdate  (theWindow);

				SetPort(cur_port);

				updateList[updateSize  ] = updateRgn;
				windowList[updateSize++] = theWindow;
			}
		}
	}
}

// CleanUp
void	TimeShare_CleanUp()
{
	// Invalidate our regions

	while (updateSize-- > 0)
	{
		Point	zero = {0,0};
		GrafPtr cur_port;
		
		GetPort(&cur_port); 
		
		SetPort			(GetWindowPort(windowList[updateSize]));
		LocalToGlobal	(&zero);
		OffsetRgn		( updateList[updateSize] , -zero.h, -zero.v);
		InvalWindowRgn  ( windowList[updateSize], updateList[updateSize] );
		DisposeRgn		( updateList[updateSize] );
		
		SetPort(cur_port);
	}
}
