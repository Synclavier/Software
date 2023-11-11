//
//  SyncAudioUtilities.h
//  Synclavier³
//
//  Created by Cameron Jones on 3/3/15.
//  Copyright (c) 2015 Synclavier Digital. All rights reserved.
//

#ifndef __Synclavier___SyncAudioUtilities__
#define __Synclavier___SyncAudioUtilities__

// Misc audio utilities for Synclavier

#include <libkern/OSAtomic.h>

#include "CSynclavierSoundFileHeader.h"

// Constants
#define SYNCLAVIER_SOUND_FILE_STASH     1
#define SYNCLAVIER_FLOATING_AUDIO_STASH 2

// Basic types
typedef float SyncFloat32;

// Struct to represent an in-memory Synclavier Sound File
typedef struct SynclavierSoundFileImage {
    SynclSFHeader           sfHeader;
    
    union {
        short               sfData[0];
        char                sfChar[0];
    };
    
} SynclavierSoundFileImage;

// Struct to represent floating point audio data
typedef struct SynclavierAudioHeader {
    int                     audioStride;
    
} SynclavierAudioHeader;

typedef struct SynclavierAudio {
    SynclavierAudioHeader   audioHeader;
    
    union {
        SyncFloat32         audioData[0];
        char                audioChar[0];
    };
    
} SynclavierAudio;


// Struct to store raw audio data
typedef struct SynclavierAudioStash {
    int                     stashType;
    volatile int32_t        stashRetains;
    long long               stashLength;
    char*                   stashPtr;
    
    union {
        char                        stashData[0];
        SynclavierSoundFileImage    stashImage;
        SynclavierAudio             stashAudio;
        
    } stashData;
    
} SynclavierAudioStash;

// Functions
extern  size_t  SynclavierAudioStashSizeForType(int type, long long length);
extern  void    SynclavierAudioStashInitForType(SynclavierAudioStash& stash, int type, long long length);

#endif /* defined(__Synclavier___SyncAudioUtilities__) */
