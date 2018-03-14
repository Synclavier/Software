//=============================================================================
//	PCI1ServiceClient.cpp
//=============================================================================

// Check for additional prefix files
#ifdef	REQUIRED_C_PREFIX
    #include REQUIRED_C_PREFIX
#endif

// Std C
#include <stdio.h>

// Local
#include "PCI1ServiceClient.h"

#undef  NULL
#define NULL 0


// =================================================================================
//		¥ PCI1ServiceClient::PCI1ServiceClient
// =================================================================================

#undef  DEBUGASSERTMSG
#define DEBUGASSERTMSG(componentSignature, options, assertionString, exceptionLabelString, errorString, fileName, lineNumber, value) {}

PCI1ServiceClient::PCI1ServiceClient(CFRunLoopRef notifyRunLoop, CFMutableDictionaryRef matchingDict, struct __sFILE* logFIle, const char* hostAp) :
	mRunLoop(notifyRunLoop), mMatchingDict(matchingDict), mLogFile(logFIle), mHostName(hostAp)
{
	mMasterDevicePort 		  = NULL;
	mNotifyPort 			  = NULL;
	mRunLoopSource 			  = NULL;
	mServicePublishIterator   = NULL;
	mServiceTerminateIterator = NULL;
	mIteratorsNeedEmptying    = false;

	// This gets the master device mach port through which all messages
	// to the kernel go, and initiates communication with IOKit.
	require_noerr(IOMasterPort(MACH_PORT_NULL, &mMasterDevicePort), errexit);

	if (mRunLoop) {
		mNotifyPort = IONotificationPortCreate(mMasterDevicePort);
		require(mNotifyPort != NULL, errexit);

		mRunLoopSource = IONotificationPortGetRunLoopSource(mNotifyPort);
		require(mRunLoopSource != NULL, errexit);
			
		CFRunLoopAddSource(mRunLoop, mRunLoopSource, kCFRunLoopDefaultMode);
		
        if (matchingDict)
        {
            CFRetain(mMatchingDict); // of which one reference is always consumed
            require_noerr(IOServiceAddMatchingNotification(mNotifyPort, kIOPublishNotification, mMatchingDict, ServicePublishCallback, this, &mServicePublishIterator), errexit);
        }
        
        if (matchingDict)
        {
            CFRetain(mMatchingDict); // of which one reference is always consumed
            require_noerr(IOServiceAddMatchingNotification(mNotifyPort, kIOTerminatedNotification, mMatchingDict, ServiceTerminateCallback, this, &mServiceTerminateIterator), errexit);
        }
        
		// signal that the first call to ScanServices needs to empty the publish/terminate iterators
		mIteratorsNeedEmptying = true;
	}
	
errexit:
	;
}


// =================================================================================
//		¥ PCI1ServiceClient::~PCI1ServiceClient
// =================================================================================

PCI1ServiceClient::~PCI1ServiceClient()
{
	if (mRunLoop != NULL && mRunLoopSource != NULL)
		CFRunLoopRemoveSource(mRunLoop, mRunLoopSource, kCFRunLoopDefaultMode);

	if (mRunLoopSource != NULL)
		CFRelease(mRunLoopSource);
	
	if (mServicePublishIterator != NULL)
		{IOObjectRelease(mServicePublishIterator); mServicePublishIterator= NULL;}
	
	if (mServiceTerminateIterator != NULL)
		{IOObjectRelease(mServiceTerminateIterator); mServiceTerminateIterator = NULL;}

    // Can't do this
    // if (mNotifyPort != NULL)
    //    {IONotificationPortDestroy(mNotifyPort); mNotifyPort = NULL;}
    
	if (mMatchingDict)
		{CFRelease(mMatchingDict); mMatchingDict = NULL;}
    
    mMasterDevicePort = NULL;
}


// =================================================================================
//		¥ PCI1ServiceClient::ServicePublishCallback, ServiceTerminateCallback, ScanServices
// =================================================================================

void		PCI1ServiceClient::ServicePublishCallback(void *refcon, io_iterator_t it)
{
    if (refcon)
        ((PCI1ServiceClient *)refcon)->ServicesPublished(it);
}

void		PCI1ServiceClient::ServiceTerminateCallback(void *refcon, io_iterator_t it)
{
    if (refcon)
        ((PCI1ServiceClient *)refcon)->ServicesTerminated(it);
}

void	PCI1ServiceClient::ScanServices()
{
	if (mMasterDevicePort == 0)
		return;

	if (mIteratorsNeedEmptying) {
		mIteratorsNeedEmptying = false;
        
        if (mServicePublishIterator)
            ServicesPublished (mServicePublishIterator);
            
        if (mServiceTerminateIterator)
            ServicesTerminated(mServiceTerminateIterator);

		return;
	}

	io_iterator_t iter = NULL;

	CFRetain(mMatchingDict); // of which one reference is always consumed
	require_noerr(IOServiceGetMatchingServices(mMasterDevicePort, mMatchingDict, &iter), errexit);
    
    if (iter)
        ServicesPublished(iter);

errexit:
	if (iter != NULL)
		IOObjectRelease(iter);
}

int	PCI1ServiceClient::CountServices()
{
    io_service_t	ioServiceObj = NULL;
    int             counted      = 0;
    
    if (mMasterDevicePort == 0)
        return counted;
    
    if (mMatchingDict == NULL)
        return counted;
    
    io_iterator_t iter = NULL;
    
    CFRetain(mMatchingDict); // of which one reference is always consumed
    require_noerr(IOServiceGetMatchingServices(mMasterDevicePort, mMatchingDict, &iter), errexit);
    
    if (iter) {
        while ((ioServiceObj = IOIteratorNext(iter)) != NULL) {
            counted++;
            IOObjectRelease(ioServiceObj);
        }
    }
    
errexit:
    if (iter)
        IOObjectRelease(iter);
    
    return counted;
}


// =================================================================================
//		¥ PCI1ServiceClient::ServicesPublished, ServicesTerminated, AssignLog
// =================================================================================

void	PCI1ServiceClient::ServicesPublished(io_iterator_t serviceIterator)
{
	io_service_t	ioServiceObj = NULL;

    if (!serviceIterator)
        return;
        
	while ((ioServiceObj = IOIteratorNext(serviceIterator)) != NULL) {
		ServicePublished(ioServiceObj);
		IOObjectRelease(ioServiceObj);
	}
}

void	PCI1ServiceClient::ServicesTerminated(io_iterator_t serviceIterator)
{
	io_service_t	ioServiceObj = NULL;

    if (!serviceIterator)
        return;
        
	while ((ioServiceObj = IOIteratorNext(serviceIterator)) != NULL) {
		ServiceTerminated(ioServiceObj);
		IOObjectRelease(ioServiceObj);
	}
}

void	PCI1ServiceClient::AssignLog(struct __sFILE* it)
{
	mLogFile = it;
}
