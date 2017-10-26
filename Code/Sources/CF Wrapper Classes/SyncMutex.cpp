//
//  SyncMutex.cpp
//  Synclavier³
//
//  Created by Cameron Jones on 9/23/14.
//  Copyright (c) 2014 Synclavier Digital. All rights reserved.
//

#include "SyncMutex.h"

// SyncMutex is a C++ wrapper around a dispatch_semaphore
static  const   int     SyncMutexInitSize = 30;

// During static initializer time we build a list of mutexes
// that must explitly be initialized (after Objective-C is set up, for example)
static  SyncMutex*      SyncMutexInitList[SyncMutexInitSize];
static  int             SyncMutexInitStuffed;
static  bool            SyncMutexInitInited;

SyncMutex::SyncMutex()
{
    mutex = NULL;
    owner = NULL;
    count = 0;
    
    // For static mutexes, create list of those needing dispatch semaphores.
    // This lets us control when static SyncMutex are allocated.
    if (SyncMutexInitInited)
        InitOne();

    else if (SyncMutexInitStuffed < SyncMutexInitSize)
        SyncMutexInitList[SyncMutexInitStuffed++] = this;
    
    else
        abort();
}

SyncMutex::~SyncMutex()
{
    if (mutex)
        dispatch_release(mutex);
    
    mutex = NULL;
    owner = NULL;
    count = 0;
}

long SyncMutex::Wait  ()
{
    return dispatch_semaphore_wait(mutex, DISPATCH_TIME_FOREVER);
}

long SyncMutex::Test  ()
{
    return dispatch_semaphore_wait(mutex, DISPATCH_TIME_NOW);
}

void SyncMutex::Signal()
{
    dispatch_semaphore_signal(mutex);
}


void SyncMutex::InitOne()
{
    if (mutex == NULL)
        mutex = dispatch_semaphore_create(1);
}

void SyncMutex::InitAll()
{
    while (SyncMutexInitStuffed > 0)
        SyncMutexInitList[--SyncMutexInitStuffed]->InitOne();
    
    SyncMutexInitInited = true;
}

// SyncMutex is a handy stack-based constructor/destructor for a SyncMutex that handles recursion

SyncMutexWaiter::SyncMutexWaiter(SyncMutex& inMutex) : mutex(inMutex)
{
    // Recursive call - continue
    if (mutex.owner == dispatch_get_current_queue())
        mutex.count++;
    
    // New call - grab the semaphore; establish us as owner
    else {
        dispatch_semaphore_wait(mutex.mutex, DISPATCH_TIME_FOREVER);
        mutex.owner = dispatch_get_current_queue();
        mutex.count = 0;
    }
}

SyncMutexWaiter::~SyncMutexWaiter()
{
    // Recursive call - decrement holds
    if (mutex.count)
        mutex.count--;
    
    // Finished - release mutex
    else {
        mutex.owner = NULL;
        dispatch_semaphore_signal(mutex.mutex);
    }
}

// SyncScrubber is a handy stack-based constructor/destructor that executes a code block when a procedure returns

SyncScrubber::SyncScrubber(SyncScrubberBlock block)
{
    scrubberBlock = block;
}

SyncScrubber::~SyncScrubber()
{
    scrubberBlock();
}
