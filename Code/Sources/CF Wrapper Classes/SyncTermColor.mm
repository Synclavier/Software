//
//  SyncTermColor.mm
//  Synclavier³
//
//  Created by Cameron Jones on 9/3/14.
//  Copyright (c) 2014 Synclavier Digital. All rights reserved.
//

#include "SyncTermColor.h"

// Creates new from RGBA
SyncTermColor::SyncTermColor(CGFloat red, CGFloat green, CGFloat blue, CGFloat alpha) {
    cgColor = CGColorCreateGenericRGB(red, green, blue, alpha);
    nsColor = [NSColor colorWithSRGBRed:red green:green blue:blue alpha:alpha];
}

// Creates new from NSColor; never takes ownership
SyncTermColor::SyncTermColor(SyncUIObjectRef aNSColor) {
    NSColor* it = DYNAMIC_CAST(aNSColor, NSColor);
    
    if (it == nil) {
        cgColor = NULL;
        nsColor = nil;
        
        return;
    }
    
    CGFloat red, green, blue, alpha;
    
    NSColor* rgb = [[it colorUsingColorSpace:[NSColorSpace sRGBColorSpace]] copy];
    
    [rgb getRed:&red green:&green blue:&blue alpha:&alpha];
    
    cgColor = CGColorCreateGenericRGB(red, green, blue, alpha);
    nsColor = rgb;
}

// Creates new from CGColor; never takes ownership
SyncTermColor::SyncTermColor(CGColorRef aCGColor) {
    NSColor* aNSColor = [NSColor colorWithCIColor: [CIColor colorWithCGColor: aCGColor]];

    //NSColor* aNSColor = [NSColor colorWithCGColor:aCGColor];
    
    CGFloat red, green, blue, alpha;
    
    NSColor* rgb = [aNSColor colorUsingColorSpace:[NSColorSpace genericRGBColorSpace]];
    
    [rgb getRed:&red green:&green blue:&blue alpha:&alpha];
    
    cgColor = CGColorCreateGenericRGB(red, green, blue, alpha);
    nsColor = rgb;
}

SyncTermColor::~SyncTermColor() {
    if (cgColor)
        CGColorRelease(cgColor);

    nsColor = nil;
}

// Stock colors
static  SyncTermColor* gTermBlack    = nil;
static  SyncTermColor* gTermWhite    = nil;
static  SyncTermColor* gTermGrey     = nil;
static  SyncTermColor* gTermDarkGrey = nil;
static  SyncTermColor* gTermLiteGrey = nil;
static  SyncTermColor* gTermRed      = nil;

static  SyncTermColor* gTermBOWFG    = nil; // Black-on-white foreground
static  SyncTermColor* gTermBOWBG    = nil; // Black-on-white background

SyncTermColor&  SyncTermColor::syncTermBlack() {
    if (gTermBlack == nil)
        gTermBlack = new SyncTermColor([NSColor blackColor]);
    
    return *gTermBlack;
}

SyncTermColor&  SyncTermColor::syncTermWhite() {
    if (gTermWhite == nil)
        gTermWhite = new SyncTermColor([NSColor whiteColor]);
    
    return *gTermWhite;
}

SyncTermColor&  SyncTermColor::syncTermGrey() {
    if (gTermGrey == nil)
        gTermGrey = new SyncTermColor([NSColor grayColor]);
    
    return *gTermGrey;
}

SyncTermColor&  SyncTermColor::syncTermDarkGrey() {
    if (gTermDarkGrey == nil)
        gTermDarkGrey = new SyncTermColor(0.20, 0.20, 0.20, 1.0);
    
    return *gTermDarkGrey;
}

SyncTermColor&  SyncTermColor::syncTermLiteGrey() {
    if (gTermLiteGrey == nil)
        gTermLiteGrey = new SyncTermColor([NSColor lightGrayColor]);
    
    return *gTermLiteGrey;
}

SyncTermColor&  SyncTermColor::syncTermRed() {
    if (gTermRed == nil)
        gTermRed = new SyncTermColor([NSColor redColor]);
    
    return *gTermRed;
}

SyncTermColor&  SyncTermColor::syncTermBOWFG() {
    if (gTermBOWFG == nil)
        gTermBOWFG = new SyncTermColor(0.00, 0.00, 0.00, 1.0);
    
    return *gTermBOWFG;
}

SyncTermColor&  SyncTermColor::syncTermBOWBG() {
    if (gTermBOWBG == nil)
        gTermBOWBG = new SyncTermColor(0.95, 0.95, 0.95, 1.0);
    
    return *gTermBOWBG;
}
