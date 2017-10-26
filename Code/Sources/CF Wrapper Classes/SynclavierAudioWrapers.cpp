//
//  SynclavierAudioWrapers.cpp
//  Synclavier³
//
//  Created by Cameron Jones on 9/1/14.
//  Copyright (c) 2014 Synclavier Digital. All rights reserved.
//

#include "AudioToolbox/AudioServices.h"
#include "SynclavierAudioWrapers.h"

// static  CFURLRef		soundFileURLRef;
// static  SystemSoundID	soundFileObject;


void SyncSysBeep(int) {
    AudioServicesPlayAlertSound(kSystemSoundID_UserPreferredAlert);
}
