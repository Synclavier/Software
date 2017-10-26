//
//  SyncAudioUtilities.cpp
//  Synclavier³
//
//  Created by Cameron Jones on 3/3/15.
//  Copyright (c) 2015 Synclavier Digital. All rights reserved.
//

#include "SyncAudioUtilities.h"

size_t  SynclavierAudioStashSizeForType(int type, long long length) {
    size_t baseLength = offsetof(SynclavierAudioStash, stashData);
    
    if (type == SYNCLAVIER_SOUND_FILE_STASH)
        return baseLength + (size_t) length;
    
    // Not implemented yet
    if (type == SYNCLAVIER_FLOATING_AUDIO_STASH)
        return 0;
    
    return 0;
}

void    SynclavierAudioStashInitForType(SynclavierAudioStash& stash, int type, long long length) {
    stash.stashType    = type;
    stash.stashLength  = length;
    stash.stashRetains = 1;
    
    // Init only the header area
    if (type == SYNCLAVIER_SOUND_FILE_STASH) {
        memset(&stash.stashData.stashImage.sfHeader,    0, sizeof(stash.stashData.stashImage.sfHeader   ));
    }
    
    if (type == SYNCLAVIER_FLOATING_AUDIO_STASH) {
        memset(&stash.stashData.stashAudio.audioHeader, 0, sizeof(stash.stashData.stashAudio.audioHeader));
    }
}
