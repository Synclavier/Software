//=============================================================================
//	PCI1ServiceClient.h
//=============================================================================

#ifndef __PCI1ServiceClient_h__
#define __PCI1ServiceClient_h__

#include <CoreServices/CoreServices.h>	// we need Debugging.h, CF, etc.
#include <IOKit/IOKitLib.h>


// =================================================================================
//		¥ PCI1ServiceClient
// =================================================================================

//	This stand-alone class provides methods that can register for kernel object matching services.

class PCI1ServiceClient {
public:
					PCI1ServiceClient(CFRunLoopRef notifyRunLoop, CFMutableDictionaryRef matchingDict, FILE* logFIle, const char* hostAp);
						// the run loop is the one in which publish/terminate service
						// notifications are received.
						
						// does NOT do an initial scan; use ScanServices
						// (to allow subclasses to finish construction first)
						
						// consumes references to matchingDict, which specifies
						// the type of service being subscribed to

                    ~PCI1ServiceClient();
	
	void			ScanServices();
						// performs an initial (or subsequent manual) iteration of the
						// instances of the services
	
    int             CountServices();
                        // performs an initial (or subsequent manual) iteration of the
                        // instances of the services
    
	virtual void	ServicePublished (io_service_t ios) = 0;
						// virtual method called when a new service is published
						
	virtual void	ServiceTerminated(io_service_t ios) = 0;
						// virtual method called when a service is terminated

public:
	static void		ServicePublishCallback		(void *refcon, io_iterator_t it);
	static void		ServiceTerminateCallback	(void *refcon, io_iterator_t it);

	void			ServicesPublished	(io_iterator_t   it);
	void			ServicesTerminated	(io_iterator_t   it);
	void			AssignLog           (FILE*           it);

	CFRunLoopRef			mRunLoop;
	CFDictionaryRef			mMatchingDict;
	struct __sFILE*			mLogFile;
	const char*				mHostName;
	
	CFRunLoopSourceRef		mRunLoopSource;
	mach_port_t				mMasterDevicePort;
	IONotificationPortRef	mNotifyPort;
	io_iterator_t			mServicePublishIterator;
	io_iterator_t			mServiceTerminateIterator;
	bool					mIteratorsNeedEmptying;
};

#endif // __PCI1ServiceClient_h__
