/* MachUtilities.cpp */

// Mac OS
#include <IOKit/IOKitLib.h>
#include <IOKit/iokitmig.h>
#include <CoreFoundation/CoreFoundation.h>
#include <mach/mach.h>

// Local includes
#include "MachUtilities.h"

// Constants
#define	SOURCE_LIST_SIZE	10

// File Statics
static	mach_port_t			gMachPort;
static	CFMachPortRef		gPortRef;
static	CFRunLoopSourceRef	gSourceList [SOURCE_LIST_SIZE];
static	Boolean				gSourceAdded[SOURCE_LIST_SIZE];
static	CFRunLoopRef		gSourceLoop [SOURCE_LIST_SIZE];
static	int					gSourceStyle[SOURCE_LIST_SIZE];
static	int					gSourceCode [SOURCE_LIST_SIZE];


// =================================================================================
//		¥ MU_Initialize_MachUtilities()
// =================================================================================

// Create a receive port and a CFMachPortRef wrapper for talking to the kernel
// The receive port is created for an existing IOServiceOpen connection
void	MU_Initialize_MachUtilities(io_connect_t connection)
{
	CFMachPortContext   context = {0, NULL, NULL, NULL, NULL};
	
	if (!gMachPort)
		IOCreateReceivePort(kOSAsyncCompleteMessageID, &gMachPort);
	
	if (gMachPort && !gPortRef)
		gPortRef = CFMachPortCreateWithPort(NULL, gMachPort, (CFMachPortCallBack) IODispatchCalloutFromMessage, &context, NULL);
}


// =================================================================================
//		¥ MU_SetWakePort()
// =================================================================================

// Method to pass our mach port (gMachPort) to a specific method in the kernel driver.
// I believe the value is no longer used within our kernel driver, but perhaps
// the call needs to be made anyways so that something in IOKit lines up
void	MU_SetWakePort(io_connect_t connection, int methodIndex)
{	
	// 10.4
	//	kern_return_t io_async_method_scalarI_scalarO
	//	(
	//		mach_port_t				connection,
	//		mach_port_t				wake_port,
	//		io_async_ref_t			reference,
	//		mach_msg_type_number_t	referenceCnt,
	//		int						selector,
	//		io_scalar_inband_t		input,
	//		mach_msg_type_number_t	inputCnt,
	//		io_scalar_inband_t		output,
	//		mach_msg_type_number_t *outputCnt
	//	);

	// 10.5
	//	kern_return_t	IOConnectCallAsyncScalarMethod
	//	(
	//		mach_port_t	 connection,		// In
	//		uint32_t	 selector,			// In
	//		mach_port_t	 wake_port,			// In
	//		uint64_t	*reference,			// In
	//		uint32_t	 referenceCnt,		// In
	//		const uint64_t	*input,			// In
	//		uint32_t	 inputCnt,			// In
	//		uint64_t	*output,			// Out
	//		uint32_t	*outputCnt)			// In/Out

	if (gMachPort)
	{
		#if !__LP64__
			io_async_ref_t			asyncRef = {0};
			mach_msg_type_number_t 	len		 = 0;
			io_scalar_inband_t      input	 = {(int) (((UInt64) gMachPort) >> 32), (int) (((UInt64) gMachPort) >> 0)};
			
			IOConnectSetNotificationPort(connection, methodIndex, gMachPort, 0);    // Pass reference of 0 to indicate we are a 32-bit client
			io_async_method_scalarI_scalarO(connection, gMachPort, asyncRef, 1, methodIndex, input, 2, NULL, &len);
		#else
			io_async_ref_t			asyncRef = {0};
			uint32_t				len		 = 0;
			io_scalar_inband64_t    input	 = {(((UInt64) gMachPort) >> 32) & 0x00000000FFFFFFFFll, (((UInt64) gMachPort) >> 0) & 0x00000000FFFFFFFFll};
			
			IOConnectSetNotificationPort(connection, methodIndex, gMachPort, 1);    // Pass reference of 1 to indicate we are a 64-bit client
		    IOConnectCallAsyncScalarMethod(connection, methodIndex, gMachPort, &asyncRef[0], 1, input, 2, NULL, &len);
		#endif
	}
}


// =================================================================================
//		¥ MU_Finalize_MachUtilities()
// =================================================================================

// Clean up stuff we created
void	MU_Finalize_MachUtilities()
{
	int i;
	
	// Remove and free up run loop sources
	for (i=0; i<SOURCE_LIST_SIZE; i++)
	{
		if (gSourceList[i])
		{
			if (gSourceAdded[i])
				CFRunLoopRemoveSource(gSourceLoop[i], gSourceList[i], kCFRunLoopDefaultMode);
		
			CFRelease(gSourceList[i]);
			
			gSourceList [i] = NULL;
			gSourceLoop [i] = NULL;
			gSourceAdded[i] = false;
		}
	}

	// Relese the receive port
	if (gPortRef)
	{
		CFMachPortInvalidate(gPortRef);
		// CFRelease           (gPortRef); seems to cause crash...
		gPortRef = NULL;
	}

	if (gMachPort)
	{
		mach_port_destroy( mach_task_self(), gMachPort );
		gMachPort = NULL;
	}
}


// =================================================================================
//		¥ MU_AddAsyncCallback()
// =================================================================================

// Add an asynchronous callback
void	MU_AddAsyncCallback(CFRunLoopRef runLoop, io_connect_t ref, int style, int code, void* proc, void* arg, int msb_code)
{
	int i;
	
	// Find a free slot
	for (i=0; i<SOURCE_LIST_SIZE; i++)
		if (!gSourceList[i])
			break;
	
	// Create the run loop source
	if (i<SOURCE_LIST_SIZE && gPortRef)
	{
		gSourceList[i] = CFMachPortCreateRunLoopSource(NULL, gPortRef, 0);

		if (gSourceList[i] && runLoop && !CFRunLoopContainsSource(runLoop, gSourceList[i], kCFRunLoopDefaultMode))
		{
			CFRunLoopAddSource(runLoop, gSourceList[i], kCFRunLoopDefaultMode);
			gSourceAdded[i] = true;
			gSourceLoop [i] = runLoop;
			gSourceStyle[i] = style;
			gSourceCode [i] = code;
		}
	}
	
	// Register the callback with the user client
	#if !__LP64__
		IOConnectMethodScalarIScalarO(ref, style, (IOItemCount) 3, (IOItemCount) 0, code, proc, arg);
	#else
		uint64_t in_data[3] = {(uint64_t) code, (uint64_t) proc, (uint64_t) arg};
		IOConnectCallScalarMethod(ref, style, &in_data[0], 3, NULL, 0);
		
		if (msb_code)
		{
			uint64_t msb_data[3] = {(uint64_t) msb_code, ((uint64_t) proc) >> 32, ((uint64_t) arg) >> 32};
			IOConnectCallScalarMethod(ref, style, &msb_data[0], 3, NULL, 0);		
		}
#endif
}


// =================================================================================
//		¥ MU_RemoveAsyncCallback()
// =================================================================================

// Remove an asynchronous callback
void	MU_RemoveAsyncCallback(io_connect_t ref, int style, int code)
{
	int i;
	
	// See if on the list
	for (i=0; i<SOURCE_LIST_SIZE; i++)
	{
		if (gSourceList[i] && gSourceStyle[i] == style && gSourceCode [i] == code)
		{
			#if !__LP64__
				IOConnectMethodScalarIScalarO(ref, style, (IOItemCount) 3, (IOItemCount) 0, code, NULL, NULL);
			#else
				uint64_t in_data[3] = {(uint64_t) code, NULL, NULL};
				IOConnectCallScalarMethod(ref, style, &in_data[0], 3, NULL, 0);
			#endif
		
			if (gSourceAdded[i])
				CFRunLoopRemoveSource(gSourceLoop[i], gSourceList[i], kCFRunLoopDefaultMode);
			
			CFRelease(gSourceList[i]);
			
			gSourceList [i] = NULL;
			gSourceLoop [i] = NULL;
			gSourceAdded[i] = false;
			gSourceStyle[i] = 0;
			gSourceCode [i] = 0;
		}
	}
}

