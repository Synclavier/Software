//
//  SyncMutex.h
//  Synclavier³
//
//  Created by Cameron Jones on 9/23/14.
//  Copyright (c) 2014 Synclavier Digital. All rights reserved.
//

#ifndef __Synclavier___SyncMutex__
#define __Synclavier___SyncMutex__

// • SyncMutex

// SyncMutex is a C++ wrapper around a dispatch_semaphore

class SyncMutex {
    friend class SyncMutexWaiter;
    
public:
	
	// constructors / destructors
    SyncMutex  ();
	~SyncMutex ();
    
    long Wait  ();
    long Test  ();
    void Signal();
    
            void    InitOne();
    static  void    InitAll();

private:
    dispatch_semaphore_t mutex;
    dispatch_queue_t     owner;
    int                  count;
};

// SyncMutex is a handy stack-based constructor/destructor for a SyncMutex that handles recursion

class SyncMutexWaiter {
public:
	
	// constructors / destructors
    SyncMutexWaiter (SyncMutex& inMutex);
	~SyncMutexWaiter();
    
    SyncMutex& mutex;
};

// SyncScrubber is a handy stack-based constructor/destructor that executes a code block when a procedure returns

typedef void (^SyncScrubberBlock)();

class SyncScrubber{
public:
    
    // constructors / destructors
    SyncScrubber (SyncScrubberBlock block);
    ~SyncScrubber();
    
    SyncScrubberBlock scrubberBlock;
};

#endif /* defined(__Synclavier___SyncMutex__) */
