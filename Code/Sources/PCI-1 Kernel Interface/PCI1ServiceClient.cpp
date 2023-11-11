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
#include "AssertMacros.h"


// =================================================================================
//		¥ PCI1ServiceClient::PCI1ServiceClient
// =================================================================================

PCI1ServiceClient::PCI1ServiceClient(CFRunLoopRef notifyRunLoop, CFMutableDictionaryRef matchingDict, struct __sFILE* logFIle, const char* hostAp) :
	mRunLoop(notifyRunLoop), mMatchingDict(matchingDict), mLogFile(logFIle), mHostName(hostAp)
{
    kern_return_t err = KERN_SUCCESS;
    
	mMasterDevicePort 		  = NULL;
	mNotifyPort 			  = NULL;
	mRunLoopSource 			  = NULL;
	mServicePublishIterator   = NULL;
	mServiceTerminateIterator = NULL;
	mIteratorsNeedEmptying    = false;

	// This gets the master device mach port through which all messages
	// to the kernel go, and initiates communication with IOKit.
	err = IOMasterPort(MACH_PORT_NULL, &mMasterDevicePort);
    
    if (err != KERN_SUCCESS || mMasterDevicePort == MACH_PORT_NULL)
        return;

	if (mRunLoop) {
		mNotifyPort = IONotificationPortCreate(mMasterDevicePort);
        
        if (mNotifyPort == MACH_PORT_NULL)
            return;

		mRunLoopSource = IONotificationPortGetRunLoopSource(mNotifyPort);
		
        if (mRunLoopSource == NULL)
            return;
        
		CFRunLoopAddSource(mRunLoop, mRunLoopSource, kCFRunLoopDefaultMode);
		
        if (matchingDict)
        {
            CFRetain(mMatchingDict); // of which one reference is always consumed
            err = IOServiceAddMatchingNotification(mNotifyPort, kIOPublishNotification, mMatchingDict, ServicePublishCallback, this, &mServicePublishIterator);
            if (err != KERN_SUCCESS) return;
        }
        
        if (matchingDict)
        {
            CFRetain(mMatchingDict); // of which one reference is always consumed
            err = IOServiceAddMatchingNotification(mNotifyPort, kIOTerminatedNotification, mMatchingDict, ServiceTerminateCallback, this, &mServiceTerminateIterator);
            if (err != KERN_SUCCESS) return;
        }
        
		// signal that the first call to ScanServices needs to empty the publish/terminate iterators
		mIteratorsNeedEmptying = true;
	}
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
	
	if (mServicePublishIterator != MACH_PORT_NULL)
		{IOObjectRelease(mServicePublishIterator); mServicePublishIterator= MACH_PORT_NULL;}
	
	if (mServiceTerminateIterator != MACH_PORT_NULL)
		{IOObjectRelease(mServiceTerminateIterator); mServiceTerminateIterator = MACH_PORT_NULL;}

    // Can't do this
    // if (mNotifyPort != NULL)
    //    {IONotificationPortDestroy(mNotifyPort); mNotifyPort = NULL;}
    
	if (mMatchingDict)
		{CFRelease(mMatchingDict); mMatchingDict = NULL;}
    
    mMasterDevicePort = MACH_PORT_NULL;
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
    kern_return_t err = KERN_SUCCESS;

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

	io_iterator_t iter = MACH_PORT_NULL;

	CFRetain(mMatchingDict); // of which one reference is always consumed
	err = IOServiceGetMatchingServices(mMasterDevicePort, mMatchingDict, &iter);
    
    if (err != KERN_SUCCESS) return;

    if (iter)
        ServicesPublished(iter);

	if (iter)
		IOObjectRelease(iter);
}

int	PCI1ServiceClient::CountServices()
{
    io_service_t	ioServiceObj = MACH_PORT_NULL;
    int             counted      = 0;
    kern_return_t   err          = KERN_SUCCESS;

    if (mMasterDevicePort == MACH_PORT_NULL)
        return counted;
    
    if (mMatchingDict == NULL)
        return counted;
    
    io_iterator_t iter = MACH_PORT_NULL;
    
    CFRetain(mMatchingDict); // of which one reference is always consumed
    
    err = IOServiceGetMatchingServices(mMasterDevicePort, mMatchingDict, &iter);
    
    if (iter) {
        while ((ioServiceObj = IOIteratorNext(iter)) != MACH_PORT_NULL) {
            counted++;
            IOObjectRelease(ioServiceObj);
        }
    }
    
    if (iter)
        IOObjectRelease(iter);
    
    return counted;
}


// =================================================================================
//		¥ PCI1ServiceClient::ServicesPublished, ServicesTerminated, AssignLog
// =================================================================================

void	PCI1ServiceClient::ServicesPublished(io_iterator_t serviceIterator)
{
	io_service_t	ioServiceObj = MACH_PORT_NULL;

    if (!serviceIterator)
        return;
        
	while ((ioServiceObj = IOIteratorNext(serviceIterator)) != MACH_PORT_NULL) {
		ServicePublished(ioServiceObj);
		IOObjectRelease(ioServiceObj);
	}
}

void	PCI1ServiceClient::ServicesTerminated(io_iterator_t serviceIterator)
{
	io_service_t	ioServiceObj = MACH_PORT_NULL;

    if (!serviceIterator)
        return;
        
	while ((ioServiceObj = IOIteratorNext(serviceIterator)) != MACH_PORT_NULL) {
		ServiceTerminated(ioServiceObj);
		IOObjectRelease(ioServiceObj);
	}
}

void	PCI1ServiceClient::AssignLog(struct __sFILE* it)
{
	mLogFile = it;
}
