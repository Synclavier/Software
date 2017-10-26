/* CoreGraphics - CGDataProvider.h
 * Copyright (c) 1999-2008 Apple Inc.
 * All rights reserved. */

#ifndef CGDATAPROVIDEREXTRACTS_H_
#define CGDATAPROVIDEREXTRACTS_H_

typedef void (*CGDataProviderSkipBytesCallback)(void *info, size_t count);

/* Old-style callbacks for sequentially accessing data.
   `getBytes' is called to copy `count' bytes from the sequential data
     stream to `buffer'. It should return the number of bytes copied, or 0
     if there's no more data.
   `skipBytes' is called to skip ahead in the sequential data stream by
     `count' bytes.
   `rewind' is called to rewind the sequential data stream to the beginning
     of the data.
   `releaseProvider', if non-NULL, is called to release the `info' pointer
     when the provider is freed. */

struct CGDataProviderCallbacks {
    CGDataProviderGetBytesCallback getBytes;
    CGDataProviderSkipBytesCallback skipBytes;
    CGDataProviderRewindCallback rewind;
    CGDataProviderReleaseInfoCallback releaseProvider;
};
typedef struct CGDataProviderCallbacks CGDataProviderCallbacks;

/* This callback is called to copy `count' bytes at byte offset `offset'
   into `buffer'. */

typedef size_t (*CGDataProviderGetBytesAtOffsetCallback)(void *info,
    void *buffer, size_t offset, size_t count);

/* Callbacks for directly accessing data.
   `getBytePointer', if non-NULL, is called to return a pointer to the
     provider's entire block of data.
   `releaseBytePointer', if non-NULL, is called to release a pointer to the
     provider's entire block of data.
   `getBytes', if non-NULL, is called to copy `count' bytes at offset
     `offset' from the provider's data to `buffer'. It should return the
     number of bytes copied, or 0 if there's no more data.
   `releaseProvider', if non-NULL, is called when the provider is freed.
  
   At least one of `getBytePointer' or `getBytes' must be non-NULL. */

struct CGDataProviderDirectAccessCallbacks {
    CGDataProviderGetBytePointerCallback getBytePointer;
    CGDataProviderReleaseBytePointerCallback releaseBytePointer;
    CGDataProviderGetBytesAtOffsetCallback getBytes;
    CGDataProviderReleaseInfoCallback releaseProvider;
};
typedef struct CGDataProviderDirectAccessCallbacks
    CGDataProviderDirectAccessCallbacks;

/* Create a sequential-access data provider using `callbacks' to provide the
   data. `info' is passed to each of the callback functions. */

CG_EXTERN CGDataProviderRef CGDataProviderCreate(void *info,
    const CGDataProviderCallbacks *callbacks)
    CG_AVAILABLE_BUT_DEPRECATED(__MAC_10_0, __MAC_10_5, __IPHONE_NA,
	__IPHONE_NA);

/* Create a direct-access data provider using `callbacks' to supply `size'
   bytes of data. `info' is passed to each of the callback functions. */

CG_EXTERN CGDataProviderRef CGDataProviderCreateDirectAccess(void *info,
    size_t size, const CGDataProviderDirectAccessCallbacks *callbacks)
    CG_AVAILABLE_BUT_DEPRECATED(__MAC_10_0, __MAC_10_5, __IPHONE_NA,
	__IPHONE_NA);

#endif	/* CGDATAPROVIDEREXTRACTS_H_ */
