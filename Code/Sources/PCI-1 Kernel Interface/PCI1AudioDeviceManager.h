//=============================================================================
//	PCI1AudioDeviceManager.h
//=============================================================================

#ifndef __PCI1AudioDeviceManager_h__
#define __PCI1AudioDeviceManager_h__

#include <CoreMIDI/MIDIServices.h>

#include "XPL.h"
#include "PCI1ServiceClient.h"
#include "SynclavierPCILib.h"
#include "PCI-1SharedMemory.h"


// =================================================================================
//		¥ PCI1AudioDeviceManager
// =================================================================================

// Classes manages the IOServiceClient interface to PCI-1 cards. Also includes the interface for general inter-application shared  memory associated with Synclavier PowerPC

// Number of cards supporeted
#define PCI1ServiceClientListSize	1				// Number of PCI-1 boards supported

typedef void (*PCI1AudioDeviceManagerCallout)(void *objRef, int which);

class PCI1AudioDeviceManager : public PCI1ServiceClient {
public:
    PCI1AudioDeviceManager (const char *className, CFRunLoopRef notifyRunLoop, struct __sFILE* logFIle, const char* hostAp, PCI1UserClientServiceTypes connectionType);
	~PCI1AudioDeviceManager();

	// IOServiceClient overrides
	virtual void		ServicePublished		(io_service_t ios);
	virtual void		ServiceTerminated		(io_service_t ios);

    // Class Methods
    void				SetPublishedCallback (PCI1AudioDeviceManagerCallout proc, void* objRef);
    void				SetTerminatedCallback(PCI1AudioDeviceManagerCallout proc, void* objRef);
    void				DoTermination (int index);
	
	// Static utility methods
    static void*        FetchSharedMemory  (io_connect_t ref, PCI1UserClientMemoryDescriptorID memoryID, UInt32* itsSize, UInt32 whichSubMemory);
    static void         ReleaseSharedMemory(io_connect_t ref, PCI1UserClientMemoryDescriptorID memoryID, void* itsAddress);
	static void			IssueCall		   (io_connect_t ref, UInt32 code, UInt32 in1, UInt32 in2);
	static void			IssuePointerCall   (io_connect_t ref, UInt32 code, void*  in1, void*  in2, UInt32 msb_code);
	static void			IssueUserClientCall(io_connect_t ref, UInt32 code, UInt32 in1, UInt32 in2);
    
	PCI1UserClientServiceTypes		mConnectionType;
    PCI1AudioDeviceManagerCallout	mPublishedProc;
    PCI1AudioDeviceManagerCallout	mTerminatedProc;
    void*							mPublishedObjRef;
    void*							mTerminatedObjRef;
	
	int						mNumConnectedDevices;
	io_connect_t			mDeviceConn  [PCI1ServiceClientListSize];
	UInt64					mDeviceGUID  [PCI1ServiceClientListSize];
	PCI1SharedStruct*		mPCI1Struct  [PCI1ServiceClientListSize];							// User Space Pointer to PCI1SharedStruct shared with kernel driver (allocated therein)
	SynclavierSharedStruct* mSyncStruct  [PCI1ServiceClientListSize];							// User Space Pointer to SynclavierSharedStruct shared with kernel driver (allocated therein)
	fixed*					mIntMem		 [PCI1ServiceClientListSize];							// User Space Pointer to Able Internal Memory shared with kernel driver (allocated therein)
	fixed*					mExtMem		 [PCI1ServiceClientListSize];							// User Space Pointer to Able External Memopry shared with kernel driver (allocated therein)
	fixed*					mPSimMem	 [PCI1ServiceClientListSize];							// User Space Pointer to Able Simulated Poly Memory shared with kernel driver (allocated therein)
	fixed*					mBlankMem	 [PCI1ServiceClientListSize];							// User Space Pointer to Able SCSI Buffer Memory shared with kernel driver (allocated therein)
	fixed*					mPolyMem	 [PCI1ServiceClientListSize];							// User Space Pointer to Able Poly Disk I/O Buffer Memory shared with kernel driver (allocated therein)
	fixed*					mSessionMem	 [PCI1ServiceClientListSize][PCI1SessionClientSize];	// User Space Pointer to PPCSession memory shared with kernel driver (allocated therein)
	int						mIntMemS	 [PCI1ServiceClientListSize];
	int						mExtMemS	 [PCI1ServiceClientListSize];
	int						mPSimMemS	 [PCI1ServiceClientListSize];
	int						mBlankMemS	 [PCI1ServiceClientListSize];
	int						mPolyMemS	 [PCI1ServiceClientListSize];
	int						mSessionMemS [PCI1ServiceClientListSize][PCI1SessionClientSize];
	
	MIDIDeviceRef			mMIDIDevice  [PCI1ServiceClientListSize];
};

#endif // __PCI1AudioDeviceManager_h__
