//
//  SyncTermColor.h
//  Synclavier³
//
//  Created by Cameron Jones on 9/3/14.
//  Copyright (c) 2014 Synclavier Digital. All rights reserved.
//

#ifndef Synclavier__SyncTermColor_h
#define Synclavier__SyncTermColor_h

#include <ApplicationServices/ApplicationServices.h>

#include "Synclavier3Constants.h"

// Color container class for Termulator.
// Container will include both a CGColor and an NSColor.
// The CGColor will be in the generic rgb space.
// The NSColor will be in the srgb space.

class SyncTermColor
{
    
public:
    SyncTermColor(CGFloat red, CGFloat green, CGFloat blue, CGFloat alpha); // Creates new from RGBA
    SyncTermColor(SyncUIObjectRef aNSColor);                                // Creates new from NSColor; never takes ownership
    SyncTermColor(CGColorRef      aCGColor);                                // Creates new from CGColor; never takes ownership

    ~SyncTermColor();
    
    inline  CGColorRef      GetCG() {return cgColor;}
    inline  SyncUIObjectRef GetNS() {return nsColor;}
    
    static  SyncTermColor&  syncTermBlack   ();
    static  SyncTermColor&  syncTermWhite   ();
    static  SyncTermColor&  syncTermGrey    ();
    static  SyncTermColor&  syncTermDarkGrey();
    static  SyncTermColor&  syncTermLiteGrey();
    static  SyncTermColor&  syncTermRed     ();
    static  SyncTermColor&  syncTermBOWFG   (); // Black-on-white foreground
    static  SyncTermColor&  syncTermBOWBG   (); // Black-on-white background
    
private:
    CGColorRef      cgColor;
    SyncUIObjectRef nsColor;
};


#endif
