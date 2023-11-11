// =================================================================================
//	SynclavierPCILib.c
// =================================================================================

// User space interface to the PCI-1 kernel extension

// Std C
#include <stdio.h>

// Unix
#include <sys/mman.h>
#include <unistd.h>

// Local
#include "SynclavierPCILib.h"
#include "PCI-1KernelDefs.h"
#include "PCI1AudioDeviceManager.h"
#include "PCI1Constants.h"
#include "AbleInterpreterCore.h"
#include "AbleInterpreterShared.h"

// Synclavier
#include "timlits.h"
#include "synclits.h"
#include "globlits.h"

#undef eieio

#ifdef __ppc__
	/*	Use eieio as a memory barrier to order stores.
	 *	Useful for device control and PTE maintenance.
	 */

	#define eieio() __asm__ volatile("eieio")

	/* 	Use sync to ensure previous stores have completed.
		This is  required when manipulating locks and/or
		maintaining PTEs or other shared structures on SMP 
		machines.
	*/

	#define sync() __asm__ volatile("sync")
#else
	#define eieio()
	#define sync()
#endif		


// =================================================================================
//		¥Forward References
// =================================================================================

static	void	HandlePCIDevicePublish    (void* objRef, int which);
static	void	HandlePCIDeviceTerminate  (void* objRef, int which);


// =================================================================================
//		¥ File Statics
// =================================================================================

#define SYNCLAVIER_SHARED_FILE_NAME "/tmp/synclavier" SYNCLAVIER_POWERPC_STRUCT_NAME

static	PCI1AudioDeviceManager*				gPCIDeviceManager;
static	io_connect_t						gPCIDeviceConnection;
static	int									gWhich;
static	SynclavierSharedStruct*				gLocalSharedStruct;
static	SynclavierSharedStruct*				gSharedSharedStruct;
static	int									gSharedSharedStructLen;
static	int									gSharedfd = (-1);
static	SynclavierSharedStruct*				gSharedStruct;
static	SynclavierPCILib_Callback			gPublishCallback;
static	SynclavierPCILib_TerminateCallback	gTerminateCallback;
static  SynclavierPCILib_WakeupCallback     gWakeupCallback;
static	void*								gObjRef;


// =================================================================================
//		¥ SynclavierPCILib_SetCallbacks();
// =================================================================================

void SynclavierPCILib_SetCallbacks( SynclavierPCILib_Callback publishCallback, SynclavierPCILib_TerminateCallback terminationCallback, void* objRef )
{
	gPublishCallback   = publishCallback;
	gTerminateCallback = terminationCallback;
	gObjRef            = objRef;
}

void SynclavierPCILib_SetWakeupCallback( SynclavierPCILib_WakeupCallback wakeupCallback )
{
    gWakeupCallback = wakeupCallback;
}


// =================================================================================
//		¥ SynclavierPCILib_PeekAtHardware();
// =================================================================================

void SynclavierPCILib_PeekAtHardware(int& numVirtuals, int& numPCI1s, int& numPCIes )
{
    numVirtuals = 0;
    numPCI1s    = 0;
    numPCIes    = 0;
    
    // Look for Virtual Kernel Extensions
    // Look for PCI-1
    // Look for PCIe
    
    PCI1AudioDeviceManager* virtualSnooper = new PCI1AudioDeviceManager(SyncVirtualPCIClassName, NULL, NULL, SYNCLAVIER_APPLICATION_NAME, kPCI1ServiceGeneric);
    PCI1AudioDeviceManager* pci1Snooper    = new PCI1AudioDeviceManager(SyncPCI1PCIClassName,    NULL, NULL, SYNCLAVIER_APPLICATION_NAME, kPCI1ServiceGeneric);
    PCI1AudioDeviceManager* btb1Snooper    = new PCI1AudioDeviceManager(SyncPCIePCIClassName,    NULL, NULL, SYNCLAVIER_APPLICATION_NAME, kPCI1ServiceGeneric);
    
    if (virtualSnooper) {
        numVirtuals = virtualSnooper->CountServices();
        delete virtualSnooper;
        virtualSnooper = NULL;
    }
    
    if (pci1Snooper) {
        numPCI1s = pci1Snooper->CountServices();
        delete pci1Snooper;
        pci1Snooper = NULL;
    }
    
    if (btb1Snooper) {
        numPCIes = btb1Snooper->CountServices();
        delete btb1Snooper;
        btb1Snooper = NULL;
    }
    
    // Look for base class
    #ifdef USE_SYSTEM_EXTENSION
        PCI1AudioDeviceManager* baseSnooper = new PCI1AudioDeviceManager(SyncBasePCIClassName, NULL, NULL, SYNCLAVIER_APPLICATION_NAME, kPCI1ServiceGeneric);
        
        if (baseSnooper) {
            numPCIes += baseSnooper->CountServices();
            delete baseSnooper;
            baseSnooper = NULL;
        }
    #endif
}


// =================================================================================
//		¥ SynclavierPCILib_InitializePCI1();
// =================================================================================

// Called by higher level software to initialize the PCI-1.  It arbitrates so that
// the hardware only gets setup once.

static  int gInitedInstances = 0;

int SynclavierPCILib_InitializePCI1( const char *hostAp, PCI1UserClientServiceTypes serviceType, int cable_load, int bus_load )
{
	static	short status = noErr;
    
    // Already inited by this ap; done.
    if (gInitedInstances++ > 0)
        return status;
	
	// Make sure we have a manager; create one if not
	if (!gPCIDeviceManager)
	{
        // Look for any base class object; could be smarter here if we wanted to...
		gPCIDeviceManager = new PCI1AudioDeviceManager(SyncBasePCIClassName, CFRunLoopGetCurrent(), NULL, hostAp, serviceType);

		if (gPCIDeviceManager)
		{
			gPCIDeviceManager->SetPublishedCallback  (HandlePCIDevicePublish,   gPCIDeviceManager);
			gPCIDeviceManager->SetTerminatedCallback (HandlePCIDeviceTerminate, gPCIDeviceManager);

			gPCIDeviceManager->ScanServices();	// HandlePCIDevicePublish should be called at this point
		}
	}

	// Init the board if we got one (unless we are MIDI driver, in which case all we want is the kernel connection and dont have any interest in the actual hardware)
	if (gPCIDeviceManager && gPCIDeviceConnection && serviceType != kPCI1ServiceMIDI && cable_load != 99 && bus_load != 99)
		gPCIDeviceManager->IssueCall(gPCIDeviceConnection, PCI1InitializePCI1, cable_load, bus_load);
		
	return (status);
}

// HandlePCIDevicePublish
static	void	HandlePCIDevicePublish( void* objRef, int which )
{
    PCI1AudioDeviceManager& obj = * (PCI1AudioDeviceManager *) objRef;
	
	// For our purposes only handle 1 PCI-1 card. Ignore other cards in system
	if (gPCIDeviceConnection == (io_connect_t) NULL)
	{
		gPCIDeviceConnection = obj.mDeviceConn[which];
		gWhich               = which;
		
		if (gPublishCallback)
			gPublishCallback(gObjRef, which);
	}
}    

// HandlePCIDeviceTerminate
static	void	HandlePCIDeviceTerminate( void* objRef, int which )
{
    PCI1AudioDeviceManager& obj = * (PCI1AudioDeviceManager *) objRef;
	
	if (gPCIDeviceConnection == obj.mDeviceConn[which])
	{
		if (gTerminateCallback)
			gTerminateCallback(gObjRef, which);
        
		gPCIDeviceConnection = NULL;
	}
}


// =================================================================================
//		¥ SynclavierPCILIB_Finalize( bool doUnprep );
// =================================================================================

// Called at end of interpretation
void SynclavierPCILIB_Finalize( bool doUnprep )
{
    // Handle case of not inited
    if (gInitedInstances == 0)
        return;
    
    // See if opened by another part of this app
    if (--gInitedInstances > 0)
        return;

	if (gPCIDeviceManager && gPCIDeviceConnection)
	{
		// shut down interpreter threads and clean up memory
		if (doUnprep)
		{
			gPCIDeviceManager->IssueCall          (gPCIDeviceConnection, PCI1UnPrepCore,        0, 0);
			gPCIDeviceManager->IssueUserClientCall(gPCIDeviceConnection, PCI1UCBroadcastChange, 0, 0);
		}

		gPCIDeviceManager->DoTermination(gWhich);	// Our termination routine gets called at this point
	}
		
	if (gPCIDeviceManager)
	{
		delete gPCIDeviceManager;
		gPCIDeviceManager = NULL;
	}
	
	if (gSharedSharedStruct)
	{
		munmap(gSharedSharedStruct, gSharedSharedStructLen);
						
		gSharedSharedStruct    = NULL;
		gSharedSharedStructLen = 0;
	}
	
	if (gSharedfd != (-1))
	{
		close(gSharedfd);
		gSharedfd = (-1);
	}
	
	gSharedStruct = NULL;
}


// =================================================================================
//		¥ SynclavierPCILib_FetchDeviceManager();
// =================================================================================

class PCI1AudioDeviceManager*
SynclavierPCILib_FetchDeviceManager ( )
{
	return gPCIDeviceManager;
}


// =================================================================================
//		¥ SynclavierPCILib_FetchServiceConnection();
// =================================================================================

unsigned int
SynclavierPCILib_FetchServiceConnection ( )
{
	return gPCIDeviceConnection;
}


// =================================================================================
//		¥ SynclavierPCILib_FetchSharedStruct();
// =================================================================================

// Slightly complicated by the fact we synthesize a shared struct to facilitate communication within the application
// even if we can't talk to the kernel

static  bool    gDidFail;
static  int     gRequiredVersion;

static struct SynclavierSharedStruct*
SynclavierPCILib_LookForSharedStruct ( int required_version_id )
{
	if (!gPCIDeviceManager || !gPCIDeviceConnection)
		return NULL;
		
	if (gPCIDeviceManager->mSyncStruct[gWhich] == NULL)
	{
		UInt32 itsSize = 0;
		void*  it      = NULL;
	
		it = gPCIDeviceManager->FetchSharedMemory(gPCIDeviceConnection, kSynclavierSharedStructID, &itsSize, 0);
	
        if (it && itsSize > sizeof(int)) {
            gRequiredVersion = * (int *) it;
            
            if (gRequiredVersion != required_version_id)
                gDidFail = true;
        }
        
		if (!it || itsSize < sizeof(SynclavierSharedStruct))
			return NULL;

		SynclavierSharedStruct& itsStruct = * (SynclavierSharedStruct *) it;
        
		if (itsStruct.struct_version != required_version_id)
			return NULL;
			
		gPCIDeviceManager->mSyncStruct[gWhich] = &itsStruct;
	}
	
	return gPCIDeviceManager->mSyncStruct[gWhich];
}

bool SynclavierPCILib_FetchFailed ( int& required_version_id )
{
    required_version_id = gRequiredVersion;
    
    return gDidFail;
}


struct SynclavierSharedStruct*
SynclavierPCILib_FetchSharedStruct ( int required_version_id )
{
	// First see if a kernel driver is present
	SynclavierSharedStruct* theStruct = SynclavierPCILib_LookForSharedStruct(required_version_id);
	
	// If no kernel driver, create an application-space shared memory object
	if (!theStruct)
	{
		if (!gSharedSharedStruct)
		{
			if (gSharedfd == (-1))
				gSharedfd = shm_open(SYNCLAVIER_SHARED_FILE_NAME, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);
		
			if (gSharedfd != (-1))
			{
				int pages = (int) ((sizeof(SynclavierSharedStruct) + PAGE_SIZE - 1) / PAGE_SIZE);
				
				gSharedSharedStructLen = pages * (int) PAGE_SIZE;
				
				// Note: Truncate will fail if already exists
				ftruncate(gSharedfd, gSharedSharedStructLen);
				
				gSharedSharedStruct = (SynclavierSharedStruct*) mmap (NULL, gSharedSharedStructLen, PROT_READ | PROT_WRITE, MAP_SHARED, gSharedfd, 0);

				// mmap returns minus 1 if file exists but is too short. So I append the struct version number to the file name; whenever the struct changes we get a new shared file
				if (gSharedSharedStruct == (void *)(-1))
					gSharedSharedStruct = NULL;
					
				if (gSharedSharedStruct)
				{
					// Init shared struct if we are first one; else check for incompatible version
					if (gSharedSharedStruct->struct_version == 0)
						init_shared_struct(*gSharedSharedStruct);
                    
					else if (gSharedSharedStruct->struct_version != SYNCLAVIER_POWERPC_STRUCT_VERSION)
					{
						munmap(gSharedSharedStruct, gSharedSharedStructLen);
						
						gSharedSharedStruct    = NULL;
						gSharedSharedStructLen = 0;
						
						close(gSharedfd);
						gSharedfd = (-1);
					}
				}
			}
		}
	
		if (gSharedSharedStruct)		
			theStruct = gSharedSharedStruct;
	}
	
	// Synthesize a shared struct to facilitate communication within the application if no kernel driver available.
	// That is, if we are running without a kernel driver synthesize a 'shared' struct which is not actually 'shared' with any other applications or drivers.
	// This would only happen here if the shm_open failed above.
    if (!theStruct)
	{
		if (!gLocalSharedStruct)
		{
			gLocalSharedStruct = new SynclavierSharedStruct;
			
			if (gLocalSharedStruct)
				init_shared_struct(*gLocalSharedStruct);
		}
			
		if (gLocalSharedStruct)
			theStruct = gLocalSharedStruct;
	}
	
	if (theStruct)
		gSharedStruct = theStruct;
	
	return theStruct;
}


// =================================================================================
//		¥ SynclavierPCILib_FetchPCIAccessor();
// =================================================================================

struct PCI1AccessorStruct*
SynclavierPCILib_FetchPCIAccessor ( int required_version_id )
{
	SynclavierSharedStruct* it = SynclavierPCILib_FetchSharedStruct(SYNCLAVIER_POWERPC_STRUCT_VERSION);
	
	if (!it)
		return NULL;
		
	// No PCI-1 card if using local shared struct or shm_open shared struct. That is, we might have a shared struct but no PCI-1 accessor.
	if (it == gLocalSharedStruct || it == gSharedSharedStruct)
		return NULL;
	
	if (it->accessor_struct.struct_version != required_version_id)
		return NULL;
		
	return &it->accessor_struct;
}


// =================================================================================
//		¥ SynclavierPCILib_FetchPCI1SharedStruct();
// =================================================================================

struct PCI1SharedStruct*
SynclavierPCILib_FetchPCI1SharedStruct ( int required_version_id )
{
	// PCI1SharedStruct only exists if we have a real kernel extension. It is not available in the shm_open case.
	if (!gPCIDeviceManager || !gPCIDeviceConnection)
		return NULL;
		
	if (gPCIDeviceManager->mPCI1Struct[gWhich] == NULL)
	{
		UInt32 itsSize = 0;
		void*  it      = NULL;
	
		it = gPCIDeviceManager->FetchSharedMemory(gPCIDeviceConnection, kPCI1SharedStructID, &itsSize, 0);
	
		if (!it || itsSize < sizeof(PCI1SharedStruct))
			return NULL;

		PCI1SharedStruct& itsStruct = * (PCI1SharedStruct *) it;
		
		if (itsStruct.struct_version != required_version_id)
			return NULL;
			
		gPCIDeviceManager->mPCI1Struct[gWhich] = &itsStruct;
	}
	
	return gPCIDeviceManager->mPCI1Struct[gWhich];
}


// =================================================================================
//		¥ SynclavierPCILib_FetchInternalMemory(), SynclavierPCILib_FetchExternalMemory(),
//        SynclavierPCILib_FetchPolySimMemory(),  SynclavierPCILib_FetchBlankMemory(),
//        SynclavierPCILib_FetchPolyBufMemory()
// =================================================================================

// Map memory to local process. Pass in 0 for size to use current size
fixed* SynclavierPCILib_FetchInternalMemory ( int required_size_bytes )
{
	if (!gPCIDeviceManager || !gPCIDeviceConnection)
		return NULL;
    
    // SynclavierPCILib_ReleaseInternalMemory does not work; it leads to crashes. We can't reuse the memory pointers
    // stored in the device manager, since the underlying memory is reallocated in the kernel as the intepreter starts and stops.
    // So we look up (and remap) the memory ranges whenever the interpreter comes back online
	// if (1 || gPCIDeviceManager->mIntMem[gWhich] == NULL || (required_size_bytes && gPCIDeviceManager->mIntMemS[gWhich] < required_size_bytes))
	{
		UInt32 itsSize = 0;
		void*  it      = NULL;
	
		if ((required_size_bytes != 0) && (gPCIDeviceManager->mIntMem[gWhich] == NULL || gPCIDeviceManager->mIntMemS[gWhich] < required_size_bytes))
			gPCIDeviceManager->IssueCall(gPCIDeviceConnection, PCI1AllocateInternalMem, required_size_bytes, 0);

		it = gPCIDeviceManager->FetchSharedMemory(gPCIDeviceConnection, kAbleInternalMemoryID, &itsSize, 0);
	
		if (!it || itsSize < required_size_bytes)
			return NULL;

		gPCIDeviceManager->mIntMem [gWhich] = (fixed *) it;
		gPCIDeviceManager->mIntMemS[gWhich] = required_size_bytes;
	}
	
	return gPCIDeviceManager->mIntMem[gWhich];
}

void SynclavierPCILib_ReleaseInternalMemory()
{
    if (!gPCIDeviceManager || !gPCIDeviceConnection)
        return;
    
    if (gPCIDeviceManager->mIntMem [gWhich]) {
        gPCIDeviceManager->ReleaseSharedMemory(gPCIDeviceConnection, kAbleInternalMemoryID, gPCIDeviceManager->mIntMem [gWhich]);

        gPCIDeviceManager->mIntMem [gWhich] = NULL;
        gPCIDeviceManager->mIntMemS[gWhich] = 0;
    }
}

void SynclavierPCILib_ReleaseExternalMemory()
{
    if (!gPCIDeviceManager || !gPCIDeviceConnection)
        return;
    
    if (gPCIDeviceManager->mExtMem [gWhich]) {
        gPCIDeviceManager->ReleaseSharedMemory(gPCIDeviceConnection, kAbleExternalMemoryID, gPCIDeviceManager->mExtMem [gWhich]);
        
        gPCIDeviceManager->mExtMem [gWhich] = NULL;
        gPCIDeviceManager->mExtMemS[gWhich] = 0;
    }
}

fixed* SynclavierPCILib_FetchExternalMemory ( int required_size_bytes )
{
	if (!gPCIDeviceManager || !gPCIDeviceConnection)
		return NULL;
		
    // SynclavierPCILib_ReleaseInternalMemory does not work; it leads to crashes. We can't reuse the memory pointers
    // stored in the device manager, since the underlying memory is reallocated in the kernel as the intepreter starts and stops.
    // So we look up (and remap) the memory ranges whenever the interpreter comes back online
	// if (1 || gPCIDeviceManager->mExtMem[gWhich] == NULL || (required_size_bytes && gPCIDeviceManager->mExtMemS[gWhich] < required_size_bytes))
	{
		UInt32 itsSize = 0;
		void*  it      = NULL;
		
		if ((required_size_bytes != 0) && (gPCIDeviceManager->mExtMem[gWhich] == NULL || gPCIDeviceManager->mExtMemS[gWhich] < required_size_bytes))
			gPCIDeviceManager->IssueCall(gPCIDeviceConnection, PCI1AllocateExternalMem, required_size_bytes, 0);

		it = gPCIDeviceManager->FetchSharedMemory(gPCIDeviceConnection, kAbleExternalMemoryID, &itsSize, 0);
	
		if (!it || itsSize < required_size_bytes)
			return NULL;

		gPCIDeviceManager->mExtMem [gWhich] = (fixed *) it;
		gPCIDeviceManager->mExtMemS[gWhich] = required_size_bytes;
	}
	
	return gPCIDeviceManager->mExtMem[gWhich];
}

fixed* SynclavierPCILib_FetchPolySimMemory ( int required_size_bytes )
{
	if (!gPCIDeviceManager || !gPCIDeviceConnection)
		return NULL;
		
    // SynclavierPCILib_ReleaseInternalMemory does not work; it leads to crashes. We can't reuse the memory pointers
    // stored in the device manager, since the underlying memory is reallocated in the kernel as the intepreter starts and stops.
    // So we look up (and remap) the memory ranges whenever the interpreter comes back online
	// if (1 || gPCIDeviceManager->mPSimMem[gWhich] == NULL || (required_size_bytes && gPCIDeviceManager->mPSimMemS[gWhich] < required_size_bytes))
	{
		UInt32 itsSize = 0;
		void*  it      = NULL;
		
		if ((required_size_bytes != 0) && (gPCIDeviceManager->mPSimMem[gWhich] == NULL || gPCIDeviceManager->mPSimMemS[gWhich] < required_size_bytes))
			gPCIDeviceManager->IssueCall(gPCIDeviceConnection, PCI1AllocatePolySimMem, required_size_bytes, 0);

		it = gPCIDeviceManager->FetchSharedMemory(gPCIDeviceConnection, kAblePolySimMemoryID, &itsSize, 0);
	
		if (!it || itsSize < required_size_bytes)
			return NULL;

		gPCIDeviceManager->mPSimMem [gWhich] = (fixed *) it;
		gPCIDeviceManager->mPSimMemS[gWhich] = required_size_bytes;
	}
	
	return gPCIDeviceManager->mPSimMem[gWhich];
}

fixed* SynclavierPCILib_FetchBlankMemory ( int required_size_bytes )
{
	if (!gPCIDeviceManager || !gPCIDeviceConnection)
		return NULL;
    
    // SynclavierPCILib_ReleaseInternalMemory does not work; it leads to crashes. We can't reuse the memory pointers
    // stored in the device manager, since the underlying memory is reallocated in the kernel as the intepreter starts and stops.
    // So we look up (and remap) the memory ranges whenever the interpreter comes back online
    // if (1 || gPCIDeviceManager->mBlankMem[gWhich] == NULL || (required_size_bytes && gPCIDeviceManager->mBlankMemS[gWhich] < required_size_bytes))
	{
		UInt32 itsSize = 0;
		void*  it      = NULL;

		if ((required_size_bytes != 0) && (gPCIDeviceManager->mBlankMem[gWhich] == NULL || gPCIDeviceManager->mBlankMemS[gWhich] < required_size_bytes))
			gPCIDeviceManager->IssueCall(gPCIDeviceConnection, PCI1AllocateScsiBlankMem, required_size_bytes, 0);

		it = gPCIDeviceManager->FetchSharedMemory(gPCIDeviceConnection, kAbleBlankMemoryID, &itsSize, 0);
	
		if (!it || itsSize < required_size_bytes)
			return NULL;

		gPCIDeviceManager->mBlankMem [gWhich] = (fixed *) it;
		gPCIDeviceManager->mBlankMemS[gWhich] = required_size_bytes;
	}
	
	return gPCIDeviceManager->mBlankMem[gWhich];
}

fixed* SynclavierPCILib_FetchPolyBufMemory ( int required_size_bytes )
{
	if (!gPCIDeviceManager || !gPCIDeviceConnection)
		return NULL;
		
    // SynclavierPCILib_ReleaseInternalMemory does not work; it leads to crashes. We can't reuse the memory pointers
    // stored in the device manager, since the underlying memory is reallocated in the kernel as the intepreter starts and stops.
    // So we look up (and remap) the memory ranges whenever the interpreter comes back online
    // if (1 || gPCIDeviceManager->mPolyMem[gWhich] == NULL || (required_size_bytes && gPCIDeviceManager->mPolyMemS[gWhich] < required_size_bytes))
	{
		UInt32 itsSize = 0;
		void*  it      = NULL;

		if ((required_size_bytes != 0) && (gPCIDeviceManager->mPolyMem[gWhich] == NULL || gPCIDeviceManager->mPolyMemS[gWhich] < required_size_bytes))
			gPCIDeviceManager->IssueCall(gPCIDeviceConnection, PCI1AllocatePolyBufMem, required_size_bytes, 0);

		it = gPCIDeviceManager->FetchSharedMemory(gPCIDeviceConnection, kAblePolyBufMemoryID, &itsSize, 0);
	
		if (!it || itsSize < required_size_bytes)
			return NULL;

		gPCIDeviceManager->mPolyMem [gWhich] = (fixed *) it;
		gPCIDeviceManager->mPolyMemS[gWhich] = required_size_bytes;
	}
	
	return gPCIDeviceManager->mPolyMem[gWhich];
}

fixed* SynclavierPCILib_FetchSessionMemory ( int required_size_bytes, int whichSubMemory )
{
	if (!gPCIDeviceManager || !gPCIDeviceConnection)
		return NULL;
		
    // SynclavierPCILib_ReleaseInternalMemory does not work; it leads to crashes. We can't reuse the memory pointers
    // stored in the device manager, since the underlying memory is reallocated in the kernel as the intepreter starts and stops.
    // So we look up (and remap) the memory ranges whenever the interpreter comes back online
    // if (1 || gPCIDeviceManager->mSessionMem[gWhich][whichSubMemory] == NULL || (required_size_bytes && gPCIDeviceManager->mSessionMemS[gWhich][whichSubMemory] < required_size_bytes))
	{
		UInt32 itsSize = 0;
		void*  it      = NULL;

		if ((required_size_bytes != 0) && (gPCIDeviceManager->mSessionMem[gWhich][whichSubMemory] == NULL || gPCIDeviceManager->mSessionMemS[gWhich][whichSubMemory] < required_size_bytes))
			gPCIDeviceManager->IssueCall(gPCIDeviceConnection, PCI1AllocateSessionMem, required_size_bytes, whichSubMemory);

		it = gPCIDeviceManager->FetchSharedMemory(gPCIDeviceConnection, kAblePolyBufMemoryID, &itsSize, 0);
	
		if (!it || itsSize < required_size_bytes)
			return NULL;

		gPCIDeviceManager->mSessionMem [gWhich][whichSubMemory] = (fixed *) it;
		gPCIDeviceManager->mSessionMemS[gWhich][whichSubMemory] = required_size_bytes;
	}
	
	return gPCIDeviceManager->mSessionMem[gWhich][whichSubMemory];
}


// =================================================================================
//		¥ SynclavierPCILib_BroadcastChange()
// =================================================================================

void	SynclavierPCILib_BroadcastChange()
{
	if (!gPCIDeviceManager || !gPCIDeviceConnection)
		return;
		
	gPCIDeviceManager->IssueUserClientCall(gPCIDeviceConnection, PCI1UCBroadcastChange, 0, 0);
}


// =================================================================================
//		¥ SynclavierPCILib_PrepInterpreterCore()
// =================================================================================

void	SynclavierPCILib_PrepInterpreterCore()
{
	if (!gPCIDeviceManager || !gPCIDeviceConnection)
		return;
		
	gPCIDeviceManager->IssueCall          (gPCIDeviceConnection, PCI1PrepCore,          0, 0);
	gPCIDeviceManager->IssueUserClientCall(gPCIDeviceConnection, PCI1UCBroadcastChange, 0, 0);
}


// =================================================================================
//		¥ SynclavierPCILib_UnPrepInterpreterCore()
// =================================================================================

void	SynclavierPCILib_UnPrepInterpreterCore()
{
	if (!gPCIDeviceManager || !gPCIDeviceConnection)
		return;
		
	gPCIDeviceManager->IssueCall          (gPCIDeviceConnection, PCI1UnPrepCore,        0, 0);
	gPCIDeviceManager->IssueUserClientCall(gPCIDeviceConnection, PCI1UCBroadcastChange, 0, 0);
}


// =================================================================================
//		¥ SynclavierPCILib_ReleaseMemories()
// =================================================================================

void	SynclavierPCILib_ReleaseMemories()
{
	if (!gPCIDeviceManager || !gPCIDeviceConnection)
		return;
		
	gPCIDeviceManager->IssueUserClientCall(gPCIDeviceConnection, PCI1UCReleaseMemoryRefs, 0, 0);
}


// =================================================================================
//		¥ SynclavierPCILib_PerformCalibration()
// =================================================================================

uint32_ratio	SynclavierPCILib_PerformCalibration()
{
	uint32_ratio aRatio = {0,0};
	
	if (!gPCIDeviceManager || !gPCIDeviceConnection)
		return aRatio;
		
	gPCIDeviceManager->IssueCall(gPCIDeviceConnection, PCI1Calibrate, 0, 0);
	
	if (!gPCIDeviceManager->mSyncStruct[gWhich])
		return aRatio;
	
	return gPCIDeviceManager->mSyncStruct[gWhich]->metronome_calib_data;
}


// =================================================================================
//		¥ SynclavierPCILib_UpdateRatios()
// =================================================================================

void	SynclavierPCILib_UpdateRatios(uint32_ratio newRatio)
{
	if (!gPCIDeviceManager || !gPCIDeviceConnection)
		return;

	if (!gPCIDeviceManager->mSyncStruct[gWhich])
		return;
	
	gPCIDeviceManager->mSyncStruct[gWhich]->metronome_calib_data = newRatio;
	
	gPCIDeviceManager->IssueCall(gPCIDeviceConnection, PCI1UpdateRatio, 0, 0);
}


// =================================================================================
//		¥ SynclavierPCILib_UpdateTLim();
// =================================================================================

void
SynclavierPCILib_UpdateTLim ( int cable_load, int bus_load )
{
	if (!gPCIDeviceManager || !gPCIDeviceConnection)
		return;

	gPCIDeviceManager->IssueCall(gPCIDeviceConnection, PCI1UpdateTlim, cable_load, bus_load);
}


// =================================================================================
//		¥ SynclavierPCILib_FeedPolyMemory()
// =================================================================================

void	SynclavierPCILib_FeedPolyMemory(int num_words)
{
	if (!gPCIDeviceManager || !gPCIDeviceConnection)
		return;

	gPCIDeviceManager->IssueCall(gPCIDeviceConnection, PCI1FeedPoly, num_words, 0);
}


// =================================================================================
//		¥ SynclavierPCILib_GooseKernel()
// =================================================================================

void	SynclavierPCILib_GooseKernel(int   do_interpretation)
{
	if (!gPCIDeviceManager || !gPCIDeviceConnection)
		return;

	gPCIDeviceManager->IssueCall(gPCIDeviceConnection, PCI1GooseMe, do_interpretation, 0);
}


// =================================================================================
//		¥ SynclavierPCILib_MainFinished()
// =================================================================================

void	SynclavierPCILib_MainFinished()
{
	if (!gPCIDeviceManager || !gPCIDeviceConnection)
		return;

	gPCIDeviceManager->IssueCall(gPCIDeviceConnection, PCI1MainFinished, 0, 0);
}


// =================================================================================
//		¥ SynclavierPCILib_Suspend()
// =================================================================================

void	SynclavierPCILib_Suspend()
{
	if (!gPCIDeviceManager || !gPCIDeviceConnection)
		return;

	gPCIDeviceManager->IssueCall(gPCIDeviceConnection, PCI1Suspend, 0, 0);
}


// =================================================================================
//		¥ SynclavierPCILib_GooseMIDIOutput()
// =================================================================================

void	SynclavierPCILib_GooseMIDIOutput()
{
	if (!gPCIDeviceManager || !gPCIDeviceConnection)
		return;

	gPCIDeviceManager->IssueCall(gPCIDeviceConnection, PCI1GooseMidi, 0, 0);
}


// =================================================================================
//		¥ SynclavierPCILib_RegisterMIDIClient()
// =================================================================================

void SynclavierPCILib_RegisterMIDIClient( void* proc, void*  arg )
{
	if (!gPCIDeviceManager || !gPCIDeviceConnection)
		return;

	gPCIDeviceManager->IssuePointerCall(gPCIDeviceConnection, PCI1SetMidi, proc,  arg, PCI1SetMidiMSB);
}


// =================================================================================
//		¥ SynclavierPCILib_GooseXMIDIOutput()
// =================================================================================

void	SynclavierPCILib_GooseXMIDIOutput()
{
	if (!gPCIDeviceManager || !gPCIDeviceConnection)
		return;

	gPCIDeviceManager->IssueCall(gPCIDeviceConnection, PCI1GooseXMidi, 0, 0);
}


// =================================================================================
//		¥ SynclavierPCILib_RegisterXMIDIClient()
// =================================================================================

void SynclavierPCILib_RegisterXMIDIClient( void* proc, void*  arg )
{
	if (!gPCIDeviceManager || !gPCIDeviceConnection)
		return;

	gPCIDeviceManager->IssuePointerCall(gPCIDeviceConnection, PCI1SetXMidi, proc,  arg, PCI1SetXMidiMSB);
}


// =================================================================================
//		¥ SynclavierPCILib_SetMIDIChans()
// =================================================================================

void SynclavierPCILib_SetMIDIChans()
{
	if (!gPCIDeviceManager || !gPCIDeviceConnection)
		return;

	gPCIDeviceManager->IssueCall(gPCIDeviceConnection, PCI1SetMIDIChans, 0, 0);
}


// =================================================================================
//		¥ SynclavierPCILib_SetMIDIChans()
// =================================================================================

void SynclavierPCILib_WakeMainLoop()
{
    // If no kernel, handle case of we are interpreter and we are interpretaing
    if (!gPCIDeviceManager || !gPCIDeviceConnection) {
        if (gWakeupCallback)
            gWakeupCallback();
        
        return;
    }
    
    gPCIDeviceManager->IssueCall(gPCIDeviceConnection, PCI1SetWakeMainLoop, 0, 0);
}


// =================================================================================
//		¥ GrabForTest(), ReleaseFromTest()
// =================================================================================

static void GrabForTest()
{
	SynclavierSharedStruct& shared = *gSharedStruct;
	unsigned long  finalTicks;
	
	while (1)
	{
		// If system is waiting to sleep, just chill
		if (shared.suspend_test_for_sleep)
		{
			Delay(60, &finalTicks);
			continue;
		}
		
		// Perform atomic test for sleep request; chill if so
		while (IncrementAtomic((SInt32 *) &shared.testing_poly_memory) != 0)		// Increment the latch need to delay if someone else had it
		{
			DecrementAtomic((SInt32 *) &shared.testing_poly_memory);				// Release the unnecessary increment
			
			Delay(60, &finalTicks);
			continue;
		}
		
		break;
	}
}

static void ReleaseFromTest()
{
	SynclavierSharedStruct& shared = *gSharedStruct;
	
	// Decrement semaphore. But detect case of going negative.
	
	// Long story: When an application crashes or is aborted (ctrl-c) it can leave
	// shared.testing_poly_memory. This causes things to grind to a halt. To work around
	// that limitation, shared.testing_poly_memory is cleared whenever an application crashes (see
	// kernel handling of client died).
	
	// So we handle here the rare case of application A crashing while application B
	// has the shared.testing_poly_memory latch taken.
	if (DecrementAtomic((SInt32 *) &shared.testing_poly_memory) == 0)
		IncrementAtomic((SInt32 *) &shared.testing_poly_memory);
}
	

// =================================================================================
//		¥ PCI1TestTimingRegister()
// =================================================================================

void	SynclavierPCILib_TestTimingRegister()
{
	if (!gPCIDeviceManager || !gPCIDeviceConnection || !gSharedStruct)
		return;
	
	GrabForTest();
	
	gPCIDeviceManager->IssueCall(gPCIDeviceConnection, PCI1TestTimingRegister, 0, 0);
	
	ReleaseFromTest();
}


// =================================================================================
//		¥ SynclavierPCILib_XPL_Read(), SynclavierPCILib_XPL_Write(), SynclavierPCILib_JamTLIMRegister()
// =================================================================================

// Could/should grab for these routines if they ever get used outside the poly test
int	SynclavierPCILib_XPL_Read(int address)
{
	SynclavierSharedStruct& shared = *gSharedStruct;

	if (!gPCIDeviceManager || !gPCIDeviceConnection || !gSharedStruct)
		return (0);

	GrabForTest();
	
	gPCIDeviceManager->IssueCall(gPCIDeviceConnection, PCI1XPLRead, address, 0);
	
	ReleaseFromTest();
	
	return (shared.accessor_struct.XPL_Read_Data);
}

void	SynclavierPCILib_XPL_Write(int address, int data)
{
	// SynclavierSharedStruct& shared = *gSharedStruct;

	if (!gPCIDeviceManager || !gPCIDeviceConnection || !gSharedStruct)
		return;

	GrabForTest();
	
	gPCIDeviceManager->IssueCall(gPCIDeviceConnection, PCI1XPLWrite, address, data);
	
	ReleaseFromTest();
}

void	SynclavierPCILib_TestBTB1(int which, int data)
{
    // SynclavierSharedStruct& shared = *gSharedStruct;
    
    if (!gPCIDeviceManager || !gPCIDeviceConnection || !gSharedStruct)
        return;
    
    GrabForTest();
    
    gPCIDeviceManager->IssueCall(gPCIDeviceConnection, PCI1TestBTB1, which, data);
    
    ReleaseFromTest();
}

void	SynclavierPCILib_JamTLIMRegister(int value)
{
	if (!gPCIDeviceManager || !gPCIDeviceConnection || !gSharedStruct)
		return;

	GrabForTest();
	
	gPCIDeviceManager->IssueCall(gPCIDeviceConnection, PCI1JamTlim, value, 0);
	
	ReleaseFromTest();
}


// =================================================================================
//		¥ SynclavierPCILib_ReadSector(), SynclavierPCILib_Write()
// =================================================================================

void	SynclavierPCILib_ReadSector()
{
	if (!gPCIDeviceManager || !gPCIDeviceConnection || !gSharedStruct)
		return;
	
	GrabForTest();
	
	gPCIDeviceManager->IssueCall(gPCIDeviceConnection, PCI1ReadSector, 0, 0);
	
	ReleaseFromTest();
}

void	SynclavierPCILib_WriteSector()
{
	if (!gPCIDeviceManager || !gPCIDeviceConnection || !gSharedStruct)
		return;

	GrabForTest();
	
	gPCIDeviceManager->IssueCall(gPCIDeviceConnection, PCI1WriteSector, 0, 0);

	ReleaseFromTest();
}


// =================================================================================
//		¥ SynclavierPCILib_WritePoly(), SynclavierPCILib_ReadPoly()
// =================================================================================

void	SynclavierPCILib_WritePoly(int	num_words)
{
	if (!gPCIDeviceManager || !gPCIDeviceConnection || !gSharedStruct)
		return;

	GrabForTest();
	
	gPCIDeviceManager->IssueCall(gPCIDeviceConnection, PCI1WritePoly, num_words, 0);

	ReleaseFromTest();
}

void	SynclavierPCILib_ReadPoly(int	num_words)
{
	if (!gPCIDeviceManager || !gPCIDeviceConnection || !gSharedStruct)
		return;

	GrabForTest();
	
	gPCIDeviceManager->IssueCall(gPCIDeviceConnection, PCI1ReadPoly, num_words, 0);
	
	ReleaseFromTest();
}


// =================================================================================
//		¥ SynclavierPCILib_FetchDevReadCode();
// =================================================================================

unsigned int
SynclavierPCILib_FetchDevReadCode( int which )
{
	SynclavierSharedStruct& shared = *gSharedStruct;

	if (!gSharedStruct)
		return 0;
		
	if (shared.accessor_struct.snarfed)
		return (shared.accessor_struct.devreads[which]);
	else
		return (0);
}


// =================================================================================
//		¥ SynclavierPCILib_GetNextInterChangeDataPointer(), SynclavierPCILib_PostNextInterChangeMessage();
// =================================================================================

SynclavierPowerPCMessage*	SynclavierPCILib_GetNextInterChangeDataPointer()
{
	SynclavierSharedStruct& shared = *gSharedStruct;

	if (!gSharedStruct)
		return NULL;
		
	return &shared.to_interchange_data[shared.to_interchange_write_ptr];
}

void						SynclavierPCILib_PostNextInterChangeMessage(int message_code)
{
	SynclavierSharedStruct& shared = *gSharedStruct;

	if (!gSharedStruct)
		return;
		
	int write_ptr = shared.to_interchange_write_ptr;
	
	shared.to_interchange_message[write_ptr] = message_code;
	
	if (++write_ptr == shared.to_interchange_size)
		write_ptr = 0;
		
	eieio();

	shared.to_interchange_write_ptr = write_ptr;

	eieio();
}


// =================================================================================
//		¥ SynclavierPCILib_GetNextSynclavierDataPointer(), SynclavierPCILib_PostNextSynclavierMessage();
// =================================================================================

SynclavierPowerPCMessage*	SynclavierPCILib_GetNextSynclavierDataPointer()
{
	SynclavierSharedStruct& shared = *gSharedStruct;

	if (!gSharedStruct)
		return NULL;
		
	return &shared.to_synclavier_data[shared.to_synclavier_write_ptr];
}

void						SynclavierPCILib_PostNextSynclavierMessage(int message_code)
{
	SynclavierSharedStruct& shared = *gSharedStruct;

	if (!gSharedStruct)
		return;
		
	int write_ptr = shared.to_synclavier_write_ptr;
	
	shared.to_synclavier_message[write_ptr] = message_code;
	
	if (++write_ptr == shared.to_synclavier_size)
		write_ptr = 0;
		
	eieio();

	shared.to_synclavier_write_ptr = write_ptr;

	eieio();
    
    SynclavierPCILib_WakeMainLoop();
}


// =================================================================================
//		¥ SynclavierPCILib_ParseUSBMIDIEvent();
// =================================================================================

// Convert an PCI1MIDIEvent to MIDI bytes - return # of bytes generated
int SynclavierPCILib_ParseUSBMIDIEvent(struct PCI1MIDIEvent& inEvent, unsigned char* outData, bool* isSysex, bool* isSysexEnd)
{
    *isSysex    = false;
    *isSysexEnd = false;
    
	switch (inEvent.type)
	{
		case	0x0:												// 1, 2 or 3 Miscellaneous function codes. Reserved for future extensions
		case	0x1:												// 1, 2 or 3 Cable events. Reserved for future expansion.
			return 0;
			
		case	0x02:												// 2 Two-byte System Common messages like MTC, SongSelect, etc.
			*outData++ = inEvent.byte1;
			*outData++ = inEvent.byte2;
			return (2);
			
		case	0x03:												// 3 Three-byte System Common messages like SPP, etc.
			*outData++ = inEvent.byte1;
			*outData++ = inEvent.byte2;
			*outData++ = inEvent.byte3;
			return (3);
			
		case	0x04:												// 3 SysEx starts or continues
            *isSysex = true;
			
            if (inEvent.byte3 == 0xF7)								// Sysex ends in this backet
                *isSysexEnd = true;
			
            *outData++ = inEvent.byte1;
			*outData++ = inEvent.byte2;
			*outData++ = inEvent.byte3;
			return (3);

		case	0x05:												// 1 Single-byte System Common Message or SysEx ends with following single byte.
			if (inEvent.byte1 == 0xF7)								// If end of sysex
				{*isSysex = true; *isSysexEnd = true;}
			
            *outData++ = inEvent.byte1;
			
			return (1);

		case	0x06:												// 2 SysEx ends with following two bytes.
            *isSysex    = true;
			*isSysexEnd = true;
            
            *outData++ = inEvent.byte1;
			*outData++ = inEvent.byte2;
			return (2);
			
		case	0x07:												// 3 SysEx ends with following three bytes.
            *isSysex    = true;
			*isSysexEnd = true;
            
            *outData++ = inEvent.byte1;
			*outData++ = inEvent.byte2;
			*outData++ = inEvent.byte3;
			return (3);
			
		case	0x08:												// 3 Note-off
		case	0x09:												// 3 Note-on
		case	0x0A:												// 3 Poly-KeyPress
		case	0x0B:												// 3 Control Change
		case	0x0E:												// 3 PitchBend Change
            *outData++ = inEvent.byte1;
			*outData++ = inEvent.byte2;
			*outData++ = inEvent.byte3;
			return (3);
			
		case	0x0C:												// 2 Program Change
		case	0x0D:												// 2 Channel Pressure
            *outData++ = inEvent.byte1;
			*outData++ = inEvent.byte2;
			return (2);
			
		case	0x0F:												// 1 Single Byte (e.g. timing clock, start, stop, sensing, reset) or unparsed
            *outData++ = inEvent.byte1;
            
            if (inEvent.byte1 == 0xF0)								// Is start of sysex
                *isSysex = true;
            
            if (inEvent.byte1 == 0xF7)								// Is end of sysex
                {*isSysex = true; *isSysexEnd = true;}
			
            return (1);
	}
    
    return (0); // Bogus
}


// =================================================================================
//		¥ SynclavierPCILib_ConstructUSBMidiEvent();
// =================================================================================

// Convert a stream of MIDI bytes to a stream of PCI1MIDIEvents
int	SynclavierPCILib_ConstructUSBMidiEvent(int cableNum, SynclavierPCILib_MIDIByteStash& inBytes, struct PCI1MIDIEvent& outEvent)
{
	// Init to zeroes
	outEvent.type  = outEvent.cable = 0;
	outEvent.byte1 = outEvent.byte2 = outEvent.byte3 = outEvent.filler = 0;
	
    // Bad parameter
    if (inBytes.numBytes == 0)
        return (0);

    // Get MIDI byte code handy
    unsigned char	byteCode = inBytes.byte1 >> 4;
    
    switch (byteCode)
    {	// Data byte - presumably left over from sysex
        case	0x0:
        case	0x1:
        case	0x2:
        case	0x3:
        case	0x4:
        case	0x5:
        case	0x6:
        case	0x7:
            if (inBytes.numBytes == 1)								// 1 byte avail - wait for next
                return (0);
                
            if (inBytes.numBytes == 2 && inBytes.byte2 == 0xF7)		// 2 bytes end of sysex
            {
				outEvent.type  = 6;
				outEvent.cable = cableNum;
                outEvent.byte1 = inBytes.byte1;
                outEvent.byte2 = inBytes.byte2;
                return (2);
            }
            
            if (inBytes.numBytes == 2)								// 2 bytes avail - wait for 3rd byte
                return (0);
                
            if (inBytes.byte3 == 0xF7)								// 3 bytes end of sysex
            {
				outEvent.type  = 7;
				outEvent.cable = cableNum;
                outEvent.byte1 = inBytes.byte1;
                outEvent.byte2 = inBytes.byte2;
                outEvent.byte3 = inBytes.byte3;
                return (3);
            }
            
			outEvent.type  = 4;										// Sysex starts and continues
			outEvent.cable = cableNum;
            outEvent.byte1 = inBytes.byte1;
            outEvent.byte2 = inBytes.byte2;
            outEvent.byte3 = inBytes.byte3;
            return (3);
        
        // 3-byte packets
        case 	0x8:		// note-off
        case 	0x9:		// note-on
        case 	0xA:		// poly pressure
        case 	0xB:		// control change
        case 	0xE:		// pitch bend
            if (inBytes.numBytes == 3)
            {
				outEvent.type  = byteCode;
				outEvent.cable = cableNum;
                outEvent.byte1 = inBytes.byte1;
                outEvent.byte2 = inBytes.byte2;
                outEvent.byte3 = inBytes.byte3;
                return (3);
            }
            
            // Wait for third byte; didn't use any
            return (0);
            
        case 	0xC:		// program change
        case 	0xD:		// mono pressure
            if (inBytes.numBytes >= 2)
            {
				outEvent.type  = byteCode;
				outEvent.cable = cableNum;
                outEvent.byte1 = inBytes.byte1;
                outEvent.byte2 = inBytes.byte2;
                return (2);
            }
           
             // Wait for second byte; didn't use any
            return (0);
        
        case 	0xF:		// system message
            switch (inBytes.byte1)
            {
                case 0xF0:	// sysex start
                    if (inBytes.numBytes == 1)								// 1 byte avail - wait for next
                        return (0);
                        
                    if (inBytes.numBytes == 2 && inBytes.byte2 == 0xF7)		// 2 bytes sysex start and end
                    {
						outEvent.type  = 6;
						outEvent.cable = cableNum;
                        outEvent.byte1 = inBytes.byte1;
                        outEvent.byte2 = inBytes.byte2;
                        return (2);
                    }
                    
                    if (inBytes.numBytes == 2)								// 2 byte start of sysex - wait for 3rd byte
                        return (0);
                        
                    if (inBytes.byte3 == 0xF7)								// 3 bytes sysex start and end
                    {
						outEvent.type  = 7;
						outEvent.cable = cableNum;
                        outEvent.byte1 = inBytes.byte1;
                        outEvent.byte2 = inBytes.byte2;
                        outEvent.byte3 = inBytes.byte3;
                        return (3);
                    }
                    
					outEvent.type  = 4;
					outEvent.cable = cableNum;
                    outEvent.byte1 = inBytes.byte1;
                    outEvent.byte2 = inBytes.byte2;
                    outEvent.byte3 = inBytes.byte3;
                    return (3);
                    
                case 0xF8:	// clock
                case 0xFA:	// start
                case 0xFB:	// continue
                case 0xFC:	// stop
                case 0xFE:	// active sensing
                case 0xFF:	// system reset
					outEvent.type  = byteCode;
					outEvent.cable = cableNum;
                    outEvent.byte1 = inBytes.byte1;
                    return (1);
               
                case 0xF6:	// tune request (0)
                case 0xF7:	// EOX
					outEvent.type  = 5;
					outEvent.cable = cableNum;
                    outEvent.byte1 = inBytes.byte1;
                    return (1);
 
                case 0xF1:	// MTC (1)
                case 0xF3:	// song select (1)
                    if (inBytes.numBytes >= 2)
                    {
						outEvent.type  = 2;							// 2-byte system common
						outEvent.cable = cableNum;
                        outEvent.byte1 = inBytes.byte1;
                        outEvent.byte2 = inBytes.byte2;
                        return (2);
                    }
                    
                    // Wait for second byte; didn't use any
                    return (0);

                case 0xF2:	// song pointer (2)
                    if (inBytes.numBytes >= 3)
                    {
						outEvent.type  = 3;							// 3-byte system common
						outEvent.cable = cableNum;
                        outEvent.byte1 = inBytes.byte1;
                        outEvent.byte2 = inBytes.byte2;
                        outEvent.byte3 = inBytes.byte3;
                        return (3);
                    }
                    
                    // Wait for third byte; didn't use any
                    return (0);
            }
    }
    
    // Bogus
    inBytes.numBytes = 0;
    return 0;
}

