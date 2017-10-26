//=============================================================================
//	PCI1DeviceManager.cpp
//=============================================================================

// Check for additional prefix files
#ifdef	REQUIRED_C_PREFIX
    #include REQUIRED_C_PREFIX
#endif

// Std C
#include <StdIO.h>

// Local
#include "PCI1AudioDeviceManager.h"

#undef  NULL
#define NULL 0


// =================================================================================
//		¥ GetGUID
// =================================================================================

// Extract published GUID from object. The GUID is used to distinguish between multiple IOServices.
static UInt64 GetGUID(io_service_t theObject)
{
	CFMutableDictionaryRef 	properties = NULL;
	CFStringRef 			strDesc    = NULL;
	char 					poop [64]  = {0};
	UInt64					theGuid    = 0;
	int						i,j;
	
	IORegistryEntryCreateCFProperties(theObject, &properties, kCFAllocatorDefault, kNilOptions);
	
	if (!properties)
		return (0);
		
	strDesc = (CFStringRef)CFDictionaryGetValue(properties, CFSTR(kPCI1UniqueIDKey));

	if (!strDesc)
	{
		CFRelease(properties);
		return (0);
	}
	
	if (!CFStringGetCString(strDesc, poop, sizeof(poop), kCFStringEncodingMacRoman))
	{
		CFRelease(properties);
		return (0);
	}
		
	for (i=0; i<16; i++)
	{
		j = poop[i];
		
		if (j >= 'A' & j <= 'F')
			theGuid = (theGuid << 4) | (j - 'A' + 10);
		else if (j >= 'a' & j <= 'f')
			theGuid = (theGuid << 4) | (j - 'a' + 10);
		else if (j >= '0' & j <= '9')
			theGuid = (theGuid << 4) | (j - '0');
		else
   		{
   			CFRelease(properties);
   			return (0);
   		}
	}
	
   	CFRelease(properties);
	return (theGuid+5);
}

	
// =================================================================================
//		¥ PCI1AudioDeviceManager::PCI1AudioDeviceManager, ~PCI1AudioDeviceManager
// =================================================================================

PCI1AudioDeviceManager::PCI1AudioDeviceManager(const char *className, CFRunLoopRef notifyRunLoop, struct __sFILE* logFIle, const char* hostAp, PCI1UserClientServiceTypes connectionType) :
	PCI1ServiceClient(notifyRunLoop, IOServiceMatching(className), logFIle, hostAp), mConnectionType(connectionType)
{
	int i,j;

    mPublishedProc  = NULL;
    mTerminatedProc = NULL;

	mNumConnectedDevices = 0;
	
	for (i=0; i<PCI1ServiceClientListSize; i++)
    {
		mDeviceConn  [i] = NULL;
        mDeviceGUID  [i] = NULL;
        mPCI1Struct  [i] = NULL;
        mSyncStruct  [i] = NULL;
        mIntMem      [i] = NULL;
        mExtMem      [i] = NULL;
        mPSimMem     [i] = NULL;
        mBlankMem    [i] = NULL;
        mPolyMem     [i] = NULL;
		mMIDIDevice  [i] = NULL;
        mIntMemS     [i] = 0;
        mExtMemS     [i] = 0;
        mPSimMemS    [i] = 0;
        mBlankMemS   [i] = 0;
        mPolyMemS    [i] = 0;
		
		for (j=0; j<PCI1SessionClientSize; j++)
		{
			mSessionMem [i][j] = NULL;
			mSessionMemS[i][j] = 0;
		}
    }
}

PCI1AudioDeviceManager::~PCI1AudioDeviceManager()
{
	int i;
	
	for (i=0; i<PCI1ServiceClientListSize; i++)
    {
    	// If we still have a shared struct, it presumably means the ap is quitting before the underlying device disappears.
		// So do some cleanup, although we leave dangling pointers in the (soon to be deleted) object.
    	if (mPCI1Struct[i])
    	{
    		if (mConnectionType == kPCI1ServiceMIDI)
            {
                int j;
                
                for (j=0; j<PCI1_MAX_MIDI_PORTS; j++)
                    mPCI1Struct[i]->midiInActive[j] = false;
                
                mPCI1Struct[i]->midiInPending  = false;
                mPCI1Struct[i]->midiInRunning  = false;
                mPCI1Struct[i]->midiOutPending = false;
                mPCI1Struct[i]->midiOutRunning = false;
    		}
                        
    		mPCI1Struct[i] = NULL;
    	}
    	
    	if (mSyncStruct[i])
    	{
    		if (mConnectionType == KPCI1ServiceTypeSynclavier)
            {
                int j;
                
                for (j=0; j<PCI1_MAX_MIDI_PORTS; j++)
                    mSyncStruct[i]->midiInActive[j] = false;
                
                mSyncStruct[i]->midiInPending  = false;
                mSyncStruct[i]->midiInRunning  = false;
                mSyncStruct[i]->midiOutPending = false;
                mSyncStruct[i]->midiOutRunning = false;
			}
    		
			mSyncStruct[i] = NULL;
		}

    	if (mDeviceConn[i])
    	{
			IOServiceClose(mDeviceConn[i]);
			mDeviceConn[i] = NULL;
		}
	}
}


// =================================================================================
//		¥ PCI1AudioDeviceManager::ServicePublished
// =================================================================================

// Here we handle being notified that a matching kernel object has been created.
// For the BTB-1 that means the kernel driver is loaded, or the TB-1 is plugged in.
// We perform initialization and open the connection (IOServiceOpen) (which creates the user client object)
void	PCI1AudioDeviceManager::ServicePublished(io_service_t ioInterfaceObj)
{
	io_connect_t	  	connection = NULL;
	int 				i,j;

	if (!ioInterfaceObj)
		return;
		
	UInt64 itsGUID = GetGUID(ioInterfaceObj);
  
	if (!itsGUID)
		return;
    
    // Check for republish?
	for (i=0; i<PCI1ServiceClientListSize; i++)
		if (itsGUID == mDeviceGUID[i])
			return;
    
    // Look for empty slot
	for (i=0; i<PCI1ServiceClientListSize; i++)
		if (mDeviceGUID[i] == 0)
			break;
			
	if (i>=PCI1ServiceClientListSize)
		return;
	
    // This call will cause the user client to be instantiated.
    IOServiceOpen(ioInterfaceObj, mach_task_self(), mConnectionType, &connection);
        
	// Try next object if can't connect to this one
	if (!connection)
    {
        if (mLogFile) {fprintf(mLogFile, "Synclavier Digital" " %s: ServicePublished no connection\n", mHostName); fflush(mLogFile);}
		return;
    }
    
	mNumConnectedDevices++;
	mDeviceConn  [i] = connection;
	mDeviceGUID  [i] = itsGUID;
	mPCI1Struct  [i] = NULL;
	mSyncStruct  [i] = NULL;
	mIntMem      [i] = NULL;
	mExtMem      [i] = NULL;
	mPSimMem     [i] = NULL;
	mBlankMem    [i] = NULL;
	mPolyMem     [i] = NULL;
	mMIDIDevice  [i] = NULL;
	mIntMemS     [i] = 0;
	mExtMemS     [i] = 0;
	mPSimMemS    [i] = 0;
	mBlankMemS   [i] = 0;
	mPolyMemS    [i] = 0;
    
	for (j=0; j<PCI1SessionClientSize; j++)
	{
		mSessionMem [i][j] = NULL;
		mSessionMemS[i][j] = 0;
	}

    if (mPublishedProc)
        mPublishedProc(mPublishedObjRef, i);

    if (mLogFile) {fprintf(mLogFile, "Synclavier Digital" " %s: ServicePublished 0x%16llX\n", mHostName, (long long) itsGUID); fflush(mLogFile);}
}

 
// =================================================================================
//		¥ PCI1AudioDeviceManager::DoTermination
// =================================================================================

// Here we handle being notified that a kernel object is being terminated.
void	PCI1AudioDeviceManager::DoTermination(int index)
{
	int j;
	
    if (mTerminatedProc)
        mTerminatedProc(mTerminatedObjRef, index);

    if (mDeviceConn[index])
		IOServiceClose(mDeviceConn[index]);

	if (mNumConnectedDevices) mNumConnectedDevices--;
	
    if (mLogFile) {fprintf(mLogFile, "Synclavier Digital" " %s: DoTermination 0x%16llX\n", mHostName, (long long) (mDeviceGUID[index])); fflush(mLogFile);}

	mDeviceConn  [index] = NULL;
	mDeviceGUID  [index] = NULL;
	mPCI1Struct  [index] = NULL;
	mSyncStruct  [index] = NULL;
	mIntMem      [index] = NULL;
	mExtMem      [index] = NULL;
	mPSimMem     [index] = NULL;
	mBlankMem    [index] = NULL;
	mPolyMem     [index] = NULL;
	mMIDIDevice  [index] = NULL;
	mIntMemS     [index] = 0;
	mExtMemS     [index] = 0;
	mPSimMemS    [index] = 0;
	mBlankMemS   [index] = 0;
	mPolyMemS    [index] = 0;
	
	for (j=0; j<PCI1SessionClientSize; j++)
	{
		mSessionMem [index][j] = NULL;
		mSessionMemS[index][j] = 0;
	}
}


// =================================================================================
//		¥ PCI1AudioDeviceManager::ServiceTerminated
// =================================================================================

void	PCI1AudioDeviceManager::ServiceTerminated(io_service_t ioInterfaceObj)
{
	int 	i;

	if (!ioInterfaceObj)
		return;
		
	UInt64 itsGUID = GetGUID(ioInterfaceObj);
	
	if (!itsGUID)
		return;

	for (i=0; i<PCI1ServiceClientListSize; i++)
		if (itsGUID == mDeviceGUID[i])
			break;
	
	if (i>=PCI1ServiceClientListSize)
		return;

	DoTermination(i);
}


// =================================================================================
//		¥ PCI1AudioDeviceManager::SetPublishedCallback, SetTerminatedCallback
// =================================================================================

void	PCI1AudioDeviceManager::SetPublishedCallback (PCI1AudioDeviceManagerCallout proc, void* objRef)
{
    mPublishedProc   = proc;
    mPublishedObjRef = objRef;
}

void	PCI1AudioDeviceManager::SetTerminatedCallback(PCI1AudioDeviceManagerCallout proc, void* objRef)
{
    mTerminatedProc   = proc;
    mTerminatedObjRef = objRef;
}


// =================================================================================
//		¥ PCI1AudioDeviceManager::FetchSharedMemory
// =================================================================================

#if !__LP64__
	typedef vm_address_t		fetch_pointer_type;
	typedef vm_size_t			fetch_size_type;
#else
	typedef mach_vm_address_t	fetch_pointer_type;
	typedef mach_vm_size_t		fetch_size_type;
#endif

void* 	PCI1AudioDeviceManager::FetchSharedMemory (io_connect_t ref, PCI1UserClientMemoryDescriptorID memoryID, UInt32* itsSize, UInt32 whichSubMemory)
{
	kern_return_t			kernResult = KERN_SUCCESS; 
	fetch_size_type 		size       = 0;
	fetch_pointer_type*		it         = NULL;
	
	if (itsSize)
		*itsSize = 0;
	
	kernResult = IOConnectMapMemory (ref, ((uint32_t) memoryID) | ((uint32_t) whichSubMemory << 8), mach_task_self(), (fetch_pointer_type *) &it, &size, kIOMapAnywhere);
	
	if (kernResult != KERN_SUCCESS)
    	return (NULL);
	
	if (itsSize)
    	*itsSize =(UInt32) size;
		
	return (it);		
}


// =================================================================================
//		¥ PCI1AudioDeviceManager::IssueCall
// =================================================================================

void	PCI1AudioDeviceManager::IssueCall(io_connect_t ref, UInt32 code, UInt32 in1, UInt32 in2)
{
	#if !__LP64__
		IOConnectMethodScalarIScalarO(ref, kPCI1SetParam, (IOItemCount) 3, (IOItemCount) 0, code, in1, in2);
	#else
		uint64_t in_data[3] = {code, in1, in2};
		IOConnectCallScalarMethod(ref, kPCI1SetParam, &in_data[0], 3, NULL, 0);
	#endif
}

void	PCI1AudioDeviceManager::IssuePointerCall(io_connect_t ref, UInt32 code, void* in1, void* in2, UInt32 msb_code)
{
#if !__LP64__
	IOConnectMethodScalarIScalarO(ref, kPCI1SetParam, (IOItemCount) 3, (IOItemCount) 0, code, in1, in2);
#else
	uint64_t in_data[3] = {code, (uint64_t) in1, (uint64_t) in2};
	IOConnectCallScalarMethod(ref, kPCI1SetParam, &in_data[0], 3, NULL, 0);
	
	if (msb_code)
	{
		uint64_t msb_data[3] = {msb_code, ((uint64_t) in1) >> 32, ((uint64_t) in2) >> 32};
		IOConnectCallScalarMethod(ref, kPCI1SetParam, &msb_data[0], 3, NULL, 0);		
	}
#endif
}

void	PCI1AudioDeviceManager::IssueUserClientCall(io_connect_t ref, UInt32 code, UInt32 in1, UInt32 in2)
{
	#if !__LP64__
		IOConnectMethodScalarIScalarO(ref, kPCI1SetUserClientParam, (IOItemCount) 3, (IOItemCount) 0, code, in1, in2);
	#else
		uint64_t in_data[3] = {code, in1, in2};
		IOConnectCallScalarMethod(ref, kPCI1SetUserClientParam, &in_data[0], 3, NULL, 0);
	#endif
}
