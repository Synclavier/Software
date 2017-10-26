/* DialogUtilities.c */

// 32-bit Carbon implementation of primitive error message output

#include "Standard.h"

#include <OSUtils.h>

#include <Types.h>
#include <QuickDraw.h>
#include <Dialogs.h>
#include <Windows.h>
#include <String.h>
#include <Fonts.h>

#include "DialogUtilities.h"
#include "error.h"

// Glue
static void SetWindowPort(WindowRef window)
{
	SetPort( GetWindowPort(window) );
}

static void InvalWindow(WindowRef window)
{
	Rect	portRect;
	
	GetPortBounds(GetWindowPort(window), &portRect);
	
	InvalWindowRect (window, &portRect);
}

static Rect tempRect;

static Rect& GetWindowRect(WindowRef window)
{
	GetPortBounds(GetWindowPort(window), &tempRect);
	return tempRect;
}

static Style GetWindowTextFace(WindowRef window)	// get txFace
{
	return GetPortTextFace(GetWindowPort(window));
}

static short GetWindowTextMode(WindowRef window)	// get txMode
{
	return GetPortTextMode(GetWindowPort(window));
}

static short GetWindowTextFont(WindowRef window)	// get txFont
{
	return GetPortTextFont(GetWindowPort(window));
}

static short GetWindowTextSize(WindowRef window)	// get txSize
{
	return GetPortTextSize(GetWindowPort(window));
}

// Center a new window on another window

void center_window_on_parent_window(DialogPtr dialog, WindowPtr parent)
{
	Rect	r,dialogr,wr;
	int16	h,v;
	GrafPtr	save_port;
	BitMap	screenBits;
		
	GetPort (&save_port);
	SetWindowPort (parent    );

	GetQDGlobalsScreenBits(&screenBits);
	r       = screenBits.bounds;
	r.top  += GetMBarHeight();
	wr      = GetWindowRect(parent);
	dialogr = GetWindowRect(GetDialogWindow(dialog));
	
	LocalToGlobal((PointPtr) & wr.top   );
	LocalToGlobal((PointPtr) & wr.bottom);
	
	h = wr.left + (((wr.right - wr.left) - (dialogr.right - dialogr.left)) / 2);
	v = wr.top + (((wr.bottom - wr.top ) - (dialogr.bottom - dialogr.top)) / 3);
	
	if ((h  < r.left)
	||  (h + (dialogr.right - dialogr.left) > r.right)
	||  (v  < r.top  )
	||  (v + (dialogr.bottom - dialogr.top) > r.bottom))
	{
		h = r.left + (((r.right - r.left) - (dialogr.right - dialogr.left)) / 2);
		v = r.top + (((r.bottom - r.top ) - (dialogr.bottom - dialogr.top)) / 3);
	}
		
	MoveWindow( GetDialogWindow(dialog), h, v, FALSE );

	SetPort (save_port);

}	/* end of position_dialog */


// Center a window on the main screen

void center_window_on_main_screen(DialogPtr	dialog)
{

	Rect	r,dialogr;
	int16	h,v;
	BitMap	screenBits;
	
	GetQDGlobalsScreenBits(&screenBits);
	r = screenBits.bounds;
	r.top += GetMBarHeight();		/* omit menubar */
			
	dialogr = GetWindowRect(GetDialogWindow(dialog));
	
	h = r.left + (((r.right - r.left) - (dialogr.right - dialogr.left)) / 2);
	v = r.top + (((r.bottom - r.top) - (dialogr.bottom - dialogr.top)) / 3);
	
	MoveWindow( GetDialogWindow(dialog), h, v, FALSE );

}	/* end of position_dialog */


// Dialog Item utilities

void	set_dialog_item_enable(DialogPtr the_dialog, short which_item, Boolean enable)
{
	short 	item_type;
	Handle	item;
	Rect	item_rect;

	GetDialogItem (the_dialog, which_item, &item_type, &item, &item_rect);
	
	if (enable)
		item_type &= ~kItemDisableBit;
	else
		item_type |= kItemDisableBit;
	
	SetDialogItem(the_dialog, which_item, item_type, item, &item_rect);
}

void	set_dialog_item_visibility(DialogPtr the_dialog, short which_item, Boolean visibility)
{
	if (visibility)
		ShowDialogItem (the_dialog, which_item);
	else
		HideDialogItem (the_dialog, which_item);
}

void	set_dialog_control_hilite(DialogPtr the_dialog, short which_item, int the_hilite)
{
	short 	item_type;
	Handle	item;
	Rect	item_rect;

	GetDialogItem (the_dialog, which_item, &item_type, &item, &item_rect);
	
	if ((item)
	&&  ((item_type & kControlDialogItem) != 0))
		HiliteControl ((ControlHandle) item, the_hilite);
}

void	set_dialog_control_value(DialogPtr the_dialog, short which_item, int the_value)
{
	short 	item_type;
	Handle	item;
	Rect	item_rect;

	GetDialogItem (the_dialog, which_item, &item_type, &item, &item_rect);
	
	if ((item)
	&&  ((item_type & kControlDialogItem) != 0))
		SetControlValue ((ControlHandle) item, the_value);

}

int		get_dialog_control_value(DialogPtr the_dialog, short which_item)
{
	short 	item_type;
	Handle	item;
	Rect	item_rect;

	GetDialogItem (the_dialog, which_item, &item_type, &item, &item_rect);
	
	if ((item)
	&&  ((item_type & kControlDialogItem) != 0))
		return (GetControlValue ((ControlHandle) item));

	else
		return (0);
}

void	set_dialog_user_item_value(DialogPtr the_dialog, short which_item, Handle the_value)
{
	short 	item_type;
	Handle	item;
	Rect	item_rect;

	GetDialogItem (the_dialog, which_item, &item_type, &item, &item_rect);
	
	if (item_type == kUserDialogItem || item_type == kItemDisableBit + kUserDialogItem)
		SetDialogItem(the_dialog, which_item, item_type, the_value, &item_rect);
}

void	set_dialog_text_value(DialogPtr the_dialog, short which_item, char *the_str)
{
	short 	item_type;
	Handle	item;
	Rect	item_rect;

	c2pstr(the_str);
	
	GetDialogItem (the_dialog, which_item, &item_type, &item, &item_rect);
	
	if ((item)
	&&  ((item_type & (kStaticTextDialogItem | kEditTextDialogItem)) != 0))
		SetDialogItemText(item, (unsigned char *) the_str);
	
	p2cstr((unsigned char *) the_str);
}

void	set_dialog_text_special(DialogPtr the_dialog, short which_item, char *the_str)
{
	short 	item_type;
	Handle	item;
	Rect	item_rect;
	char	display_name[512];
	char	pas_name    [512];
	int		box_width;
	GrafPtr	save_port;

	GetDialogItem (the_dialog, which_item, &item_type, &item, &item_rect);
	
	if (!item)
		return;
		
	GetPort (&save_port);
	SetWindowPort (GetDialogWindow(the_dialog));
	
	strncpy(display_name, the_str     , sizeof(display_name));					// get working copy
	strncpy(pas_name,     display_name, sizeof(pas_name    ));					// in p form
	c2pstr (pas_name);
	
	box_width = item_rect.right - item_rect.left - 5;							// get box width with margin
	
	while (StringWidth((unsigned char *) pas_name) > box_width)					// remove folder names till it fits
	{
		char *first_colon = strchr(display_name, ':');							// find first colon
		char *append_here = first_colon;
		char *second_colon;

		if (!first_colon)														// no colon - remove characters until it fits
		{
			int i = strlen(display_name);
			
			if (i == 0)															// serious problem if 0 length name and still won't fit!
			{
				SetPort (save_port);
				return;
			}
			
			display_name[i-1] = 0;												// remove last character
			
			strncpy(pas_name,     display_name, sizeof(pas_name    ));			// in p form
			c2pstr (pas_name);
			continue;
		}
		
		find_middle:;
						
		if (first_colon[1] == 0)												// end of string and still to long
		{
			int i = strlen(display_name);
			
			if (i == 0)															// serious problem if 0 length name and still won't fit!
			{
				SetPort (save_port);
				return;
			}
			
			display_name[i-1] = 0;												// remove last character
			
			strncpy(pas_name,     display_name, sizeof(pas_name    ));			// in p form
			c2pstr (pas_name);
			continue;
		}
			
		second_colon = strchr(first_colon + 1, ':');							// find next one
		
		if (!second_colon)														// still too long
		{
			int i = strlen(display_name);
			
			if (i == 0)															// serious problem if 0 length name and still won't fit!
			{
				SetPort (save_port);
				return;
			}
			
			display_name[i-1] = 0;												// remove last character
			
			strncpy(pas_name,     display_name, sizeof(pas_name    ));			// in p form
			c2pstr (pas_name);
			continue;
		}
						
		if (second_colon - first_colon == 2)									// if already truncated (eg. :…:
		{
			first_colon = second_colon;
			goto find_middle;
		}				
		
		// else shrink from second colon
		append_here++;															// keep first colon

        const char* elipsis = "…";
        while (*elipsis)
            *append_here++ = *elipsis++;															// keep first colon
           
		while ((*append_here++ = *second_colon++) != 0)							// copy rest of string
			;

		strncpy(pas_name, display_name, sizeof(pas_name));
		c2pstr (pas_name);
	}

	SetPort (save_port);
	
	if ((item)
	&&  ((item_type & (kStaticTextDialogItem | kEditTextDialogItem)) != 0))
		SetDialogItemText(item, (unsigned char *) pas_name);
}

void	get_dialog_text_value(DialogPtr the_dialog, short which_item, char *the_str)
{
	short 	item_type;
	Handle	item;
	Rect	item_rect;

	*the_str = 0;
	
	GetDialogItem (the_dialog, which_item, &item_type, &item, &item_rect);
	
	if ((item)
	&&  ((item_type & (kStaticTextDialogItem | kEditTextDialogItem)) != 0))
	{
		int i;
		
		GetDialogItemText(item, (unsigned char *) the_str);
		p2cstr((unsigned char *) the_str);
		
		// Remove carriage returns that may have gotten in their via paste 
		
		i = 0;
		
		while (the_str[i])
		{
			if (the_str[i] == '\r')
				the_str[i] = 0;
			else
				i++;				
		}		
	}
}

pascal void draw_box_around_item_rect(DialogPtr dialog, short itemNo)
{
	Handle	item = NULL;						/* handle to item			*/
	short	item_type;							/* type of item 			*/
	Rect	item_rect;							/* size of item 			*/
	GrafPtr	save_port;
	Pattern	gray;

	GetDialogItem (dialog, itemNo, &item_type, &item, &item_rect);	/* get item info */

	if (item)
	{
		GetPort (&save_port);					/* preserve grafport to be friendly */
		SetWindowPort (GetDialogWindow(dialog));

		GetQDGlobalsGray(&gray);

		PenNormal ();
		PenPat (&gray);
		FrameRect (&item_rect);

		SetPort (save_port);
	}
}

pascal void draw_frame_around_button (DialogPtr dialog, short itemNo)
{
	Handle	item = NULL;						/* handle to dialog item 	*/
	short	item_type;							/* type of dialog item 		*/
	Rect	item_rect;							/* bounds of dialog item 	*/
	GrafPtr	save_port;
	
	GetDialogItem (dialog, itemNo, &item_type, &item, &item_rect);	/* get default item info */
	
	GetPort (&save_port);						/* preserve grafport to be friendly */
	SetWindowPort (GetDialogWindow(dialog));
	
	PenNormal ();								/* force correct drawing mode 		*/
	PenSize (3, 3);								/* frame button 					*/
	FrameRoundRect (&item_rect, 16, 16);
	
	PenNormal ();								/* be friendly 						*/
	SetPort (save_port);
}

pascal void draw_frame_around_working_message (DialogPtr dialog, short itemNo)
{
	Handle	item = NULL;						/* handle to dialog item 	*/
	short	item_type;							/* type of dialog item 		*/
	Rect	item_rect;							/* bounds of dialog item 	*/
	GrafPtr	save_port;
	Pattern	gray;
	
	GetDialogItem (dialog, itemNo, &item_type, &item, &item_rect);	/* get default item info */
	
	GetPort (&save_port);						/* preserve grafport to be friendly */
	SetWindowPort (GetDialogWindow(dialog));
	
	GetQDGlobalsGray(&gray);
	
	PenNormal ();								/* force correct drawing mode 		*/
	PenSize (2, 2);								/* frame button 					*/
	PenPat( &gray );
	FrameRoundRect (&item_rect, 8, 8);
	
	PenNormal ();								/* be friendly 						*/
	SetPort (save_port);
}

// Move everything in a dialog box
void	shift_dialog_contents(DialogPtr dialog, short delta_h, short delta_v, int num_items_plus_one)
{
	int i;
	
	for (i=1; i<num_items_plus_one; i++)			
	{
		short 	item_type;
		Handle	item;
		Rect	item_rect;

		GetDialogItem (dialog, i, &item_type, &item, &item_rect);

		if (item)
		{
			item_rect.top    += delta_v;
			item_rect.bottom += delta_v;
			
			item_rect.left   += delta_h;
			item_rect.right  += delta_h;

			SetDialogItem(dialog, i, item_type, item, &item_rect);
			
			if ((item)																// move controls to new place
			&&  ((item_type & kControlDialogItem) != 0))
				MoveControl ((ControlHandle) item, item_rect.left, item_rect.top);
			
			if ((item)																// selec text field so it gets moved
			&&  ((item_type & kEditTextDialogItem) != 0))							// for TEEdit */
				SelectDialogItemText (dialog, i, 0, 0); 
		}
	}
}	


// Busy dialog

static	DialogPtr		busyDlg = NULL;
static	Cursor			watchcursor;

void DU_show_working_window(short dialogID, const char *line_1_msg, const char* line_2_msg )
{
	char	line_1 [256] = "";
	char	line_2 [256] = "";
	int		margin;
	GrafPtr saveport;

	GetPort( &saveport );

	busyDlg = GetNewDialog( dialogID, NULL, (WindowPtr)-1L ); 
	
	center_window_on_parent_window( busyDlg, FrontWindow() );
	
	ShowWindow ( GetDialogWindow(busyDlg) );				// make the window visible
	DrawDialog ( busyDlg );				// draw it

	watchcursor = **GetCursor( watchCursor );

	SetCursor( &watchcursor );

	SetWindowPort(GetDialogWindow(busyDlg));						// add our text

	strcpy(line_1, line_1_msg);
	strcpy(line_2, line_2_msg);
	
	if (*line_1_msg || *line_2_msg)
	{
		int 	pic_width  = 334;								// eventually will need to look up from picture rectangle
		int 	pic_height = 256;
		
		c2pstr(line_1);
		c2pstr(line_2);
		
		BeginUpdate(GetDialogWindow(busyDlg));

		TextFont(kFontIDTimes);
		TextFace(0);
		TextSize(12);

		margin = ((pic_width - StringWidth((unsigned char *) line_1)) >> 1);

		MoveTo(margin, 200);
		DrawString((unsigned char *) line_1);

		TextFace(0);
		TextSize(12);

		margin = ((pic_width - StringWidth((unsigned char *) line_2)) >> 1);

		MoveTo(margin, 220);
		DrawString((unsigned char *) line_2);

		EndUpdate( GetDialogWindow(busyDlg) );
	}
	
	QDFlushPortBuffer(GetWindowPort(GetDialogWindow(busyDlg)), NULL);
	
	SetPort( saveport );
}

void	DU_remove_working_dialog()
{
	Cursor arrow;
	
	if (busyDlg)
	{
		GrafPtr saveport;

		GetPort( &saveport );
		SetWindowPort( GetDialogWindow(busyDlg)   );
		
		DisposeDialog( busyDlg );
		
		busyDlg = NULL;	
		
		SetPort( saveport );
		
		GetQDGlobalsArrow(&arrow);

		SetCursor(&arrow);
	}
}

SInt16  DU_CautionAlert(SInt16 alertID, void* upp)
{
    SetDialogFont(kFontIDGeneva);
    return CautionAlert(alertID, (ModalFilterUPP) upp);
}

SInt16  DU_StopAlert(SInt16 alertID, void* upp)
{
    SetDialogFont(kFontIDGeneva);
    return StopAlert(alertID, (ModalFilterUPP) upp);
}

void    DU_ReportErr(const char *s) {
    char p[256] = "";
    
    strcpy(p, s);
    reporterr(c2pstr(p));
}

