//
//  SynclavierFileReference.cpp
//  XPL Compiler-XCode4-10-8
//
//  Created by Cameron Jones on 8/27/14.
//
//

#include "Synclavier3Constants.h"
//#include "InterChange.h"
#include <CoreServices/CoreServices.h>

#include "SynclavierFileReference.h"
#include "CSynclavierMutableArray.h"
#include "SyncMutex.h"
#include "XPLRuntime.h"

#include <libkern/OSAtomic.h>
#include <sys/stat.h>

//
// SyncFSSpec Utility FUnctions
//

void    SyncFSSpecRetain (SyncFSSpec* it) {
    #if __LP64__
        if (it->file_ref)
            it->file_ref->Retain();
    #endif
}

void    SyncFSSpecRelease(SyncFSSpec* it) {
    #if __LP64__
        if (it->file_ref)
            it->file_ref->Release();
    
        it->file_ref = NULL;
    #endif
}


//
// Class CSynclavierFileReference implementation - Constructors
//

// We protect the innards of all CSynclavierFileReference objects using a single mutex.
// The constructors do not need protection - no one else can use the object until
// it is created.

// Create CSynclavierFileReference from a bookMark; take ownership of the bookmark.
CSynclavierFileReference::CSynclavierFileReference(CFDataRef markData) {
    Init();
    
    bookMark = markData;    // Note - may be NULL
}

// Create CSynclavierFileReference from a CFURLRef; take ownership of the CFURLRef.
CSynclavierFileReference::CSynclavierFileReference(CFURLRef cfURL) {
    Init();
    
    urlRef = cfURL;         // Note - may be NULL
}

// Create CSynclavierFileReference from a Posix or URL CFStringRef; take ownership of the CFStringRef.
CSynclavierFileReference::CSynclavierFileReference(CFStringRef pathStr, bool isPosix) {
    Init();
    
    // Posix path - done; just keep the posix path around; we will compute the URL once someone needs it
    if (isPosix)
        pathRef = pathStr;  // Note - may be NULL
    
    // Else if stringref us a URL, create the URL ref.
    else if (pathStr)
        urlRef = CFURLCreateWithString(NULL, pathStr, NULL);
}

// Create CSynclavierFileReference from a Posix or URL C string
CSynclavierFileReference::CSynclavierFileReference(const char* pathBuf, bool isPosix) {
    Init();
    
    if (pathBuf) {
        if (isPosix)
            pathRef = CFStringCreateWithFileSystemRepresentation(NULL, pathBuf);                    // Note - may be NULL or empty
        
        else {
            CFStringRef urlStr = CFStringCreateWithCString(NULL, pathBuf, kCFStringEncodingUTF8);   // Note - may be NULL or empty
            
            if (urlStr) {
                urlRef = CFURLCreateWithString(NULL, urlStr, NULL);
                CFRelease(urlStr);
            }
        }
    }
}

// Create CSynclavierFileReference from a FSRef; no owner for FSRef
CSynclavierFileReference::CSynclavierFileReference(FSRef* aFSRef) {
    Init();
    
    if (aFSRef)
        urlRef = CFURLCreateFromFSRef(NULL, aFSRef);
}

// Create CSynclavierFileReference from a SyncFSSpec
CSynclavierFileReference* CSynclavierFileReference::CopyFromFSSpec(SyncFSSpec* aFSSpec) {
    #if __LP64__
        if (aFSSpec->file_ref)
            aFSSpec->file_ref->Retain();
    
        return aFSSpec->file_ref;
    #else
        FSRef   aFSRef;
        
        if (FSpMakeFSRef(aFSSpec, &aFSRef) != noErr)
            return NULL;
        
        return new CSynclavierFileReference(&aFSRef);
    #endif
}

// Application wide file references
static  CSynclavierFileReference*   gApplicationFileRef      = NULL;
static  CSynclavierFileReference*   gApplicationResourcesRef = NULL;
static  CSynclavierFileReference*   gApplicationSupportRef   = NULL;
static  CSynclavierFileReference*   gUserDocumentsRef        = NULL;
static  SyncMutex                   gSyncFileRefMutex;

SyncMutex&  CSynclavierFileReference::GetMutex()
{
    return gSyncFileRefMutex;
}


CSynclavierFileReference*   CSynclavierFileReference::CopyAppFileRef()
{
    SyncMutexWaiter waiter(gSyncFileRefMutex);
    
    if (gApplicationFileRef == NULL) {
        CFBundleRef apBundle     = CFBundleGetMainBundle();             // Does not need to be release
        CFURLRef    apURL        = CFBundleCopyBundleURL(apBundle);     // Does need to be released
        
        gApplicationFileRef = new CSynclavierFileReference(apURL);      // Transfer ownership to CSynclavierFileReference
    }
    
    // Add an additional retain
    if (gApplicationFileRef)
        gApplicationFileRef->Retain();
    
    return gApplicationFileRef;
}

CSynclavierFileReference*   CSynclavierFileReference::CopyAppResourcesRef()
{
    SyncMutexWaiter waiter(gSyncFileRefMutex);
    
    if (gApplicationResourcesRef == NULL) {
        CFBundleRef apBundle     = CFBundleGetMainBundle();                     // Does not need to be release
        CFURLRef    resURL       = CFBundleCopyResourcesDirectoryURL(apBundle); // Does need to be released
        
        gApplicationResourcesRef = new CSynclavierFileReference(resURL);        // Transfer ownership to CSynclavierFileReference
    }
    
    // Add an additional retain
    if (gApplicationResourcesRef)
        gApplicationResourcesRef->Retain();
    
    return gApplicationResourcesRef;
}

CSynclavierFileReference*   CSynclavierFileReference::CopyAppSupportRef()
{
    SyncMutexWaiter waiter(gSyncFileRefMutex);

    if (gApplicationSupportRef == NULL) {
        CFURLRef    supportURL = (__bridge CFURLRef   ) [[[NSFileManager defaultManager] URLsForDirectory:NSApplicationSupportDirectory inDomains:NSUserDomainMask] objectAtIndex:0]; // Does need to be released
        CFStringRef folderName = (__bridge CFStringRef) [NSString stringWithUTF8String:SYNCLAVIER_APPLICATION_NAME];
        CFURLRef    sync3URL   = CFURLCreateCopyAppendingPathComponent(NULL, supportURL, folderName, true);

        gApplicationSupportRef = new CSynclavierFileReference(sync3URL);        // Transfer ownership to CSynclavierFileReference
        
        if (!gApplicationSupportRef->Reachable())
            gApplicationSupportRef->MkDir();
    }
    
    // Add an additional retain
    if (gApplicationSupportRef)
        gApplicationSupportRef->Retain();
    
    return gApplicationSupportRef;
}

CSynclavierFileReference*   CSynclavierFileReference::CopyUserDocumentsRef()
{
    SyncMutexWaiter waiter(gSyncFileRefMutex);
    
    if (gUserDocumentsRef == NULL) {
        CFURLRef    documentsURL = (__bridge CFURLRef   ) [[[NSFileManager defaultManager] URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask] objectAtIndex:0]; // Does need to be released
        CFStringRef folderName   = (__bridge CFStringRef) [NSString stringWithUTF8String:SYNCLAVIER_APPLICATION_NAME];
        CFURLRef    sync3URL     = CFURLCreateCopyAppendingPathComponent(NULL, documentsURL, folderName, true);
        
        gUserDocumentsRef = new CSynclavierFileReference(sync3URL);        // Transfer ownership to CSynclavierFileReference
        
        if (!gUserDocumentsRef->Reachable())
            gUserDocumentsRef->MkDir();
        else
            gUserDocumentsRef->Resolve();
    }
    
    // Add an additional retain
    if (gUserDocumentsRef)
        gUserDocumentsRef->Retain();
    
    return gUserDocumentsRef;
}

void CSynclavierFileReference::Init() {
    bookMark    = NULL;
    urlRef      = NULL;
    pathRef     = NULL;
    nameRef     = NULL;
    handleRef   = NULL;
    parentRef   = NULL;
    fileDes     = 0;
    fileMode    = 0;
    openCount   = 0;
    retainCount = 1;
}

void CSynclavierFileReference::Finish() {
    if (bookMark)
        CFRelease(bookMark);
    
    if (urlRef)
        CFRelease(urlRef);
    
    if (pathRef)
        CFRelease(pathRef);
    
    if (nameRef)
        CFRelease(nameRef);
    
    if (handleRef)
        CFRelease(handleRef);
    
    if (parentRef)
        CFRelease(parentRef);
    
    if (fileDes)
        close(fileDes);
}

// Clean up
CSynclavierFileReference::~CSynclavierFileReference() {
    Finish();
}


//
// Class CSynclavierFileReference implementation - Getters
//

// Get URL from bookmark or path
CFURLRef    CSynclavierFileReference::URL() {
    if (urlRef != NULL)
        return urlRef;
    
    SyncMutexWaiter waiter(gSyncFileRefMutex);

    if (bookMark != NULL) {
        urlRef = CFURLCreateByResolvingBookmarkData(NULL, bookMark, 0, NULL, NULL, NULL, NULL );
        
        if (urlRef)
            return urlRef;
    }
    
    if (pathRef != NULL) {
        urlRef = CFURLCreateWithFileSystemPath(NULL, pathRef, kCFURLPOSIXPathStyle, false);

        if (urlRef)
            return urlRef;
    }
    
    return NULL;
}

// Get Path from url
CFStringRef CSynclavierFileReference::Path() {
    if (pathRef != NULL)
        return pathRef;
    
    SyncMutexWaiter waiter(gSyncFileRefMutex);
    
    if (URL() != NULL) {
        CFURLRef absoluteURL = CFURLCopyAbsoluteURL(urlRef);
        
        if (absoluteURL) {
            pathRef = CFURLCopyFileSystemPath(absoluteURL, kCFURLPOSIXPathStyle);
            CFRelease(absoluteURL);
        }
    }
    
    return pathRef;
}

// Get name
CFStringRef CSynclavierFileReference::Name() {
    if (nameRef != NULL)
        return nameRef;
    
    SyncMutexWaiter waiter(gSyncFileRefMutex);
    
    if (URL() != NULL)
        CFURLCopyResourcePropertyForKey(urlRef, kCFURLNameKey, &nameRef, NULL);
    
    return nameRef;
}

// Get handle. The handle is the file name without the file extension.
CFStringRef CSynclavierFileReference::Handle() {
    if (handleRef != NULL)
        return handleRef;
    
    SyncMutexWaiter waiter(gSyncFileRefMutex);
    
    if (URL() == NULL)
        return NULL;

    CFURLRef noExtURL = CFURLCreateCopyDeletingPathExtension(NULL, URL());
    
    if (noExtURL == NULL)
        return NULL;
    
    handleRef = CFURLCopyLastPathComponent(noExtURL);
    
    CFRelease(noExtURL);
    
    return handleRef;
}

// Get Bookmark from url
CFDataRef   CSynclavierFileReference::Bookmark() {
    if (bookMark != NULL)
        return bookMark;
    
    SyncMutexWaiter waiter(gSyncFileRefMutex);
    
    if (URL() != NULL)
        bookMark = CFURLCreateBookmarkData(NULL, urlRef, 0, NULL, NULL, NULL);
   
    return bookMark;
}

// Parent
CFURLRef    CSynclavierFileReference::Parent() {
    if (parentRef != NULL)
        return parentRef;
    
    SyncMutexWaiter waiter(gSyncFileRefMutex);
    
    if (URL() != NULL)
        CFURLCopyResourcePropertyForKey(urlRef, kCFURLParentDirectoryURLKey, &parentRef, NULL);
    
    // Note: parentRef may still be null
    
    return parentRef;
}


//
// Class CSynclavierFileReference implementation - Creators
//

// CreateSibling
CFURLRef    CSynclavierFileReference::CreateSibling(CFStringRef siblingName, bool isDir) {
    if (URL() == NULL)
        return NULL;
    
    if (Parent() == NULL)
        return NULL;
    
    if (siblingName == NULL)
        return NULL;
    
    if (CFStringGetLength(siblingName) == 0)
        return NULL;

    CFURLRef newURL = CFURLCreateCopyAppendingPathComponent(NULL, parentRef, siblingName, (Boolean) isDir);
    
    CFRelease(siblingName); // String now owned by the new URL
    
    return newURL;
}

// CreateChild
CFURLRef    CSynclavierFileReference::CreateChild(CFStringRef childName, bool isDir) {
    if (URL() == NULL)
        return NULL;
    
    if (childName == NULL)
        return NULL;
    
    if (CFStringGetLength(childName) == 0)
        return NULL;
    
    CFURLRef newURL = CFURLCreateCopyAppendingPathComponent(NULL, urlRef, childName, (Boolean) isDir);
    
    CFRelease(childName); // String now owned by the new URL
    
    return newURL;
}

// Compatibility - get FSRef from CFURL
short   CSynclavierFileReference::CreateFSRef(struct FSRef* aFSRef) {
    if (URL() == NULL)
        return fnfErr;
    
    if (!CFURLGetFSRef(urlRef, aFSRef))
        return fnfErr;
    
    return noErr;
}

// Compatibility - get FSSpec from CFURL
short   CSynclavierFileReference::CreateFSSpec(SyncFSSpec* aFileSpec) {
    #if __LP64__
        memset((void *) aFileSpec, 0, sizeof(*aFileSpec));
        
        aFileSpec->file_ref = this;
        
        Retain();
    #else
        FSRef aFSRef;
        
        if (CreateFSRef(&aFSRef) != noErr)
            return fnfErr;
        
        if (FSGetCatalogInfo(&aFSRef, 0, NULL, NULL, aFileSpec, NULL) != noErr)
            return fnfErr;
    #endif
    
    return noErr;
}


//
// Class CSynclavierFileReference implementation - Funtions
//

// Get path from URL
bool    CSynclavierFileReference::Path(char* buf, int maxLen) {
    memset(buf, 0, maxLen);
    
    if (Path() == NULL)
        return false;
    
    return (bool) CFStringGetFileSystemRepresentation(pathRef, buf, (CFIndex) maxLen);
}

// Open the file in mode
static  CFIndex   gPosixSize = 0;
static  char*     gPosixBuf  = NULL;

static  bool    FillPosixBuf(CFStringRef path) {
    if (CFStringGetLength(path) == 0)
        return false;
    
    CFIndex requiredLength = CFStringGetMaximumSizeOfFileSystemRepresentation(path);
    
    if (requiredLength > gPosixSize) {
        if (gPosixBuf) {
            delete gPosixBuf;
            gPosixBuf = NULL;
        }
        
        gPosixSize = requiredLength + 1023;
        
        gPosixSize -= (gPosixSize & 1023);
        
        gPosixBuf = new char[gPosixSize];
        
        if (gPosixBuf == NULL) {
            gPosixSize = 0;
            return false;
        }
    }
    
    return CFStringGetFileSystemRepresentation(path, gPosixBuf, gPosixSize);
}

short   CSynclavierFileReference::Open(int mode) {
    SyncMutexWaiter waiter(gSyncFileRefMutex);
    
    // Count 1 more reference if already open
    if (fileDes) {
        openCount++;
        return noErr;
    }
    
    // Bail if can't get path
    if (Path() == NULL)
        return fnfErr;
    
    if (!FillPosixBuf(pathRef))
        return fnfErr;
    
    fileDes = open(gPosixBuf, mode, 0644);
    
    if (fileDes == 0)
        return fnfErr;
    
    if (fileDes == -1) {
        fileDes = 0;
        return fnfErr;
    }
    
    fileMode  = mode;
    openCount = 1;
    
    return noErr;
}

long long   CSynclavierFileReference::Size() {
    // If file is not open just get from catalog
    if (fileDes == 0) {
        CFNumberRef sizeRef = NULL;
        long long   size    = 0;
        
        // Bail if no URL
        if (!URL())
            return 0;
        
        // Bail if not reachable
        if (!Reachable())
            return 0;
        
        CFURLCopyResourcePropertyForKey(GetURL(), kCFURLFileSizeKey, &sizeRef, NULL);
        
        if (!sizeRef)
            return 0;
        
        CFNumberGetValue(sizeRef, kCFNumberSInt64Type, &size);
        
        return size;
    }

    // Note - Result would be meaningless if another thread was currently writing to the file
    long long size = (long long) lseek(fileDes, 0, SEEK_END);
    
    if (size == -1)
        size = 0;
    
    lseek(fileDes, 0, SEEK_SET);
    
    return size;
}

short   CSynclavierFileReference::Seek(long long where) {
    if (fileDes == 0)
        return fnfErr;
    
    long long at = lseek(fileDes, (off_t) where, SEEK_SET);
    
    if (at != where)
        return fnfErr;
    
    return noErr;
}

// Close
short   CSynclavierFileReference::Close() {
    SyncMutexWaiter waiter(gSyncFileRefMutex);
    
    if (fileDes == 0)
        return noErr;
    
    if (openCount > 1) {
        openCount--;
        return noErr;
    }
    
    close(fileDes);
    
    fileDes   = 0;
    fileMode  = 0;
    openCount = 0;
    
    return noErr;
}

// Sync
short   CSynclavierFileReference::Sync() {
    if (fileDes == 0)
        return noErr;
    
    fsync(fileDes);
    
    return noErr;
}

// Delete
short   CSynclavierFileReference::Delete() {
    SyncMutexWaiter waiter(gSyncFileRefMutex);
    
    if (fileDes)
        {close(fileDes); fileDes = 0; fileMode = 0; openCount = 0;}
    
    // Bail if can't get path
    if (Path() == NULL)
        return fnfErr;
    
    if (!FillPosixBuf(pathRef))
        return fnfErr;
    
    unlink(gPosixBuf);
    
    return noErr;
}

// Read data modeled after FSRead
short   CSynclavierFileReference::Read(SInt32* count, void* buffPtr) {
    if (fileDes == 0) {             // Not open yet - fail - don't guess what mode to open
        *count = 0;
        return fnfErr;
    }
    
    ssize_t result = read(fileDes, buffPtr, (size_t) *count);
    
    if (result == -1) {
        *count = 0;
        return fnfErr;
    }
    
    if ((SInt32) result != *count) {
        *count = (SInt32) result;
        
        return eofErr;
    }
    
    return noErr;
}

// Read data modeled after FSRead; Includes byte swizzle
short   CSynclavierFileReference::XPLRead(SInt32* count, void* buffPtr) {
    short err = Read(count, buffPtr);
    
    #if __LITTLE_ENDIAN__
    {
        unsigned short* dataPtr = (unsigned short *) buffPtr;
        SInt32          i       = (*count) >> 1;
        
        while (i--) {
            *dataPtr = CFSwapInt16BigToHost(*dataPtr);
            dataPtr++;
        }
    }
    #endif
    
    return err;
}

// Write data modeled after FSWrite
short   CSynclavierFileReference::Write(SInt32* count, void* buffPtr) {
    if (fileDes == 0) {         // Not open yet - fail - don't guess what mode to open
        *count = 0;
        return fnfErr;
    }
    
    ssize_t result = write(fileDes, buffPtr, (size_t) *count);
    
    if (result == -1) {
        *count = 0;
        return fnfErr;
    }
    
    if ((SInt32) result != *count) {
        *count = (SInt32) result;
        
        return eofErr;
    }
    
    return noErr;
}

// Write data modeled after FSWrite; Includes byte swizzle
short   CSynclavierFileReference::XPLWrite(SInt32* count, void* buffPtr) {
    if (fileDes == 0) {     // Not open yet - fail - can't guess what mode to open
        *count = 0;
        return fnfErr;
    }
    
    SyncSwizzleBuffer swb;
    
    if (!swb.Buffer()) {
        *count = 0;
        return (mFulErr);
    }

    SInt32   globCount = (SInt32) *count;
    char*	 globBuf   = (char *) buffPtr;
    SInt32   globDone  = 0;

    while (globCount)
    {
        SInt32 chunkCount = swb.Size();
        ufixed* dataPtr = (ufixed *) globBuf;
        ufixed* writPtr = (ufixed *) swb.Buffer();
        int i;
        
        if (chunkCount > globCount)
            chunkCount = globCount;
        
        i = chunkCount >> 1;
        
        while (i--)
            *writPtr++ = CFSwapInt16BigToHost(*dataPtr++);
        
        SInt32 aCount = chunkCount;
        
        short err = Write(&aCount, swb.Buffer());
        
        if (err) {
            *count = globDone;
            return (err);
        }
        
        if (aCount != chunkCount) {
            *count = globDone + aCount;
            return (ioErr);
        }
        
        globDone  += chunkCount;
        globBuf   += chunkCount;
        globCount -= chunkCount;
    }
    
    *count = globDone;
    
    return noErr;
}

// Clone
short   CSynclavierFileReference::Clone(CSynclavierFileReference* fileRef) {
    
    short err = this->Open(O_WRONLY | O_CREAT | O_TRUNC);
    
    if (err)
        return err;
    
    err = fileRef->Open(O_RDONLY);

    if (err)
        return err;
    
    // Use the mutex-protected application-wide buffer
    SyncSwizzleBuffer swb;
				
    if (!swb.Buffer()) {
        fileRef->Close();
        this->Close();
        return (memFullErr);
    }
    
    long long globCount = fileRef->Size();
    
    while (globCount)
    {
        SInt32 chunkCount = swb.Size();;
        
        if ((long long) chunkCount > globCount)
            chunkCount = (SInt32) globCount;
        
        SInt32 count = chunkCount;
        
        err = fileRef->Read(&count, swb.Buffer());
        
        if (err || count!= chunkCount) {
            fileRef->Close();
            this->Close();
            return (readErr);
        }
        
        err = this->Write(&count, swb.Buffer());
       
        if (err || count!= chunkCount) {
            fileRef->Close();
            this->Close();
            return (writErr);
        }
        
        globCount -= chunkCount;
    }
        
    fileRef->Close();
    this->Close();
    return noErr;
}

// Skip
short   CSynclavierFileReference::Skip(int count) {
    if (fileDes == 0)           // Not open yet - fail - can't guess what mode to open
        return fnfErr;
    
    off_t result = lseek(fileDes, (off_t) count, SEEK_CUR);
    
    if (result == -1)
        return fnfErr;
     
    return noErr;
}

// Create
short   CSynclavierFileReference::Create(OSType type, OSType creator, UInt16 finderFlags, long long fileLength) {
    SyncMutexWaiter waiter(gSyncFileRefMutex);
    
    if (fileDes)
        return noErr;
    
    // Bail if can't get path
    if (Path() == NULL)
        return fnfErr;
    
    if (!FillPosixBuf(pathRef))
        return fnfErr;
    
    fileDes = creat(gPosixBuf, 0644);
    
    if (fileDes == 0)
        return fnfErr;
    
    if (fileDes == -1) {
        fileDes = 0;
        return fnfErr;
    }
    
    if (type != 0 || creator != 0)
        SetFinfo(type, creator, finderFlags);
    
    if (fileLength > 0) {
        fstore descriptor = {F_ALLOCATEALL, F_PEOFPOSMODE, 0, fileLength, 0};
        
        int result = fcntl(fileDes, F_PREALLOCATE, &descriptor);
        
        if (result == (-1)) {
            Close();
            return dskFulErr;
        }
        
        result = ftruncate(fileDes, (off_t) fileLength);
        
        if (result == (-1)) {
            Close();
            return dskFulErr;
        }
    }
    
    Close();
    
    return noErr;
}

// SetFinfo
short   CSynclavierFileReference::SetFinfo(OSType type, OSType creator, UInt16 finderFlags) {
    FSCatalogInfo   catInfo;
    FSRef           ref;
    
    memset(&catInfo, 0, sizeof(catInfo));
    
    FileInfo& fileInfo = * (FileInfo*) &catInfo.finderInfo;
    
    fileInfo.fileType    = type;
    fileInfo.fileCreator = creator;
    fileInfo.finderFlags = finderFlags;
    
    if (CreateFSRef(&ref) != noErr)
        return fnfErr;
    
    return (short) FSSetCatalogInfo(&ref, kFSCatInfoFinderInfo, &catInfo);
}

// Reference counting

void    CSynclavierFileReference::Retain() {
    OSAtomicIncrement32(&retainCount);
}

void    CSynclavierFileReference::Release() {
    if (OSAtomicDecrement32(&retainCount) == 0)
        delete this;
}

// Resolve
void    CSynclavierFileReference::Resolve() {
    SyncMutexWaiter waiter(gSyncFileRefMutex);
    
    // Loop to resolve symlinks to aliaii to symlink aliaii
    while (1) {
        CFBooleanRef isLink  = NULL;
        CFURLRef     newURL  = NULL;
        
        // Bail if no URL
        if (!URL())
            return;
        
        // Bail if not reachable
        if (!Reachable())
            return;
        
        // Done if not an alias or a symlink
        if (!IsAlias())
            return;
        
        // See if alias file or symlink
        if (!CFURLCopyResourcePropertyForKey(urlRef, kCFURLIsSymbolicLinkKey, &isLink,  NULL))
            return;

        // Bail if could not find out which it is
        if (!isLink)
            return;
        
        Boolean isSymLink = CFBooleanGetValue(isLink);
        
        CFRelease(isLink);
        
        // Resolve Symbolic Links
        if (isSymLink) {
            // Get its posix path - bail if can't
            if (Path() == NULL)
                return;
            
            // Get posix path UTF8
            if (!FillPosixBuf(pathRef))
                return;
            
            // Resolve the symbolic link
            char* newPath = realpath(gPosixBuf, NULL);
            
            // Could not get new path
            if (!newPath)
                return;
            
            // Get new URL
            newURL = CFURLCreateFromFileSystemRepresentation(NULL, (const UInt8 *) newPath, strlen(newPath)+1, false);

            // Done with resolved path
            free(newPath);
        }

        // Resolve Finder Alias file
        else {
            CFDataRef bookMark = CFURLCreateBookmarkDataFromFile(NULL, urlRef, NULL);
            
            // Bail if could not read bookmark data
            if (!bookMark)
                return;
            
            // Get new URL
            newURL = CFURLCreateByResolvingBookmarkData(NULL, bookMark, 0, NULL, NULL, NULL, NULL );
            
            // Done with bookmark
            CFRelease(bookMark);
        }
        
        
        // Bail if could not get new URL
        if (!newURL) {
            return;
        }
        
        // Toss prior URL, BookMark, Parent
        
        int retains = retainCount;      // Preserve retains in case we do a resolve when object is owned multiple times
        
        Finish();                       // Get read of earlier URL, Parent, Bookmark, Path
        Init  ();
        
        retainCount = retains;
        
        // Assign new URL
        urlRef = newURL;
    }
}

// Reachable
bool    CSynclavierFileReference::Reachable() {
    if (URL() == NULL)
        return false;
    
    return (bool) CFURLResourceIsReachable(urlRef, NULL);
}

// IsDirectory
bool    CSynclavierFileReference::IsDirectory() {
    CFBooleanRef    isDirectory = NULL;
    
    if (URL() == NULL)
        return false;
    
    if (!CFURLCopyResourcePropertyForKey(urlRef, kCFURLIsDirectoryKey, &isDirectory, NULL))
        return false;
    
    if (!isDirectory)
        return false;
    
    Boolean result = CFBooleanGetValue(isDirectory);
    
    CFRelease(isDirectory);
    
    return (bool) result;
}

// IsAlias
bool    CSynclavierFileReference::IsAlias() {
    CFBooleanRef    isAlias = NULL;
    
    if (URL() == NULL)
        return false;
    
    if (!CFURLCopyResourcePropertyForKey(urlRef, kCFURLIsAliasFileKey, &isAlias, NULL))
        return false;
    
    // True for both aliaii and symlinks
    if (!isAlias)
        return false;
    
    Boolean result = CFBooleanGetValue(isAlias);
    
    CFRelease(isAlias);
    
    return (bool) result;
}

bool    CSynclavierFileReference::IsOpen() {
    return openCount > 0;
}

// Broken since CSynclavierMutableArray is no longer based on CFArray...
// No longer used
#if 0
    // Directory functions
    static  CSynclavierMutableArray& gListingProperties() {
        static CSynclavierMutableArray* listingArray;
        
        if (listingArray == NULL)
            listingArray = new CSynclavierMutableArray(10);
        
        return (*listingArray);
    }

    #if MAC_OS_X_VERSION_MIN_REQUIRED==MAC_OS_X_VERSION_10_6
        #define kCFURLEnumeratorDirectoryPostOrderSuccess 4
    #endif

    CSynclavierMutableArray&    CSynclavierFileReference::Listing(CFOptionFlags flags) {
        CSynclavierMutableArray& listing = * new CSynclavierMutableArray(100);
        
        // Broken since CSynclavierMutableArray is no longer based on CFArray...
        // Grab mutex once to check things. Then mutex is not needed for directory construction.
        {
            SyncMutexWaiter          waiter(gSyncFileRefMutex);
            CSynclavierMutableArray& listingProperties = gListingProperties();
            
            Resolve();
        
            if (!Reachable() || !IsDirectory() || !&listing )
                return listing;
        
            // Build - once - list of properties to prefetch
            if (listingProperties.Count() == 0) {
                listingProperties.PushLast(kCFURLNameKey);
                listingProperties.PushLast(kCFURLLocalizedNameKey);
                listingProperties.PushLast(kCFURLCreationDateKey);
                listingProperties.PushLast(kCFURLContentModificationDateKey);
                listingProperties.PushLast(kCFURLTypeIdentifierKey);
                listingProperties.PushLast(kCFURLLocalizedTypeDescriptionKey);
                listingProperties.PushLast(kCFURLFileSizeKey);
                listingProperties.PushLast(kCFURLIsDirectoryKey);
                listingProperties.PushLast(kCFURLIsAliasFileKey);
            }
        }
        
        CFURLEnumeratorRef enumerator = CFURLEnumeratorCreateForDirectoryURL(NULL, URL(), flags, gListingProperties());
        
        if (enumerator) {
            CFURLRef              childURL         = NULL;
            CFURLEnumeratorResult enumeratorResult = kCFURLEnumeratorEnd;
            
            enumeratorResult = CFURLEnumeratorGetNextURL(enumerator, (CFURLRef *)&childURL, NULL);

            while (enumeratorResult == kCFURLEnumeratorSuccess || enumeratorResult == kCFURLEnumeratorDirectoryPostOrderSuccess) {
                #if 0
                {
                    CFStringRef name = NULL;
                    CFStringRef type = NULL;
                    CFStringRef ltyp = NULL;
                    CFNumberRef size = NULL;
                    CFBooleanRef isDirectory = NULL;
                    CFBooleanRef isAlias     = NULL;
                    
                    long long   flen = 0;
                    
                    CFURLCopyResourcePropertyForKey(childURL, kCFURLNameKey, &name, NULL);
                    CFURLCopyResourcePropertyForKey(childURL, kCFURLTypeIdentifierKey, &type, NULL);
                    CFURLCopyResourcePropertyForKey(childURL, kCFURLLocalizedTypeDescriptionKey, &ltyp, NULL);
                    CFURLCopyResourcePropertyForKey(childURL, kCFURLFileSizeKey, &size, NULL);
                    CFURLCopyResourcePropertyForKey(childURL, kCFURLIsDirectoryKey, &isDirectory, NULL);
                    CFURLCopyResourcePropertyForKey(childURL, kCFURLIsAliasFileKey, &isAlias, NULL);
                    
                    if (size)
                        CFNumberGetValue (size, kCFNumberSInt64Type, &flen);
                    
                    SyncPrintf("Listing: Name %s Type %s Ltyp %s Blok %lld\n", SyncCFSTR(name), SyncCFSTR(type), SyncCFSTR(ltyp), flen);
                    
                    if (name) CFRelease(name);
                    if (type) CFRelease(type);
                    if (ltyp) CFRelease(ltyp);
                    if (size) CFRelease(size);
                    if (size) CFRelease(isDirectory);
                    if (size) CFRelease(isAlias);
                }
                #endif
                
                CFRetain(childURL);
                
                listing.PushLast(childURL);

                enumeratorResult = CFURLEnumeratorGetNextURL(enumerator, (CFURLRef *)&childURL, NULL);
            }
        
            CFRelease(enumerator);
        }
        
        return listing;
    }
#endif

// MkDir
short   CSynclavierFileReference::MkDir() {
    SyncMutexWaiter waiter(gSyncFileRefMutex);
    
    if (fileDes)
        return noErr;
    
    // Bail if can't get path
    if (Path() == NULL)
        return fnfErr;
    
    if (!FillPosixBuf(pathRef))
        return fnfErr;
    
    if (mkdir (gPosixBuf, 0755) != 0)
        return fnfErr;
    
    return noErr;
}

// MkPath
short   CSynclavierFileReference::MkPath() {
    CFURLRef    parentList[100];
    int         parentCount = 0;
    
    if (Reachable())
        return noErr;
    
    CFURLRef parent = CFURLCreateCopyDeletingLastPathComponent(NULL, URL());
    
    while (parent && !CFURLResourceIsReachable(parent, NULL)) {
        parentList[parentCount++] = parent;
        parent = CFURLCreateCopyDeletingLastPathComponent(NULL, parent);
    }
    
    // Create intermediate directories
    while (parentCount > 0) {
        CSynclavierFileReference* parentFileRefRef = new CSynclavierFileReference(parentList[--parentCount]); // Reference is consumed
        
        if (!parentFileRefRef)
            return memFullErr;
        
        CSynclavierFileReference& parentFileRef = *parentFileRefRef;
        
        if (int err = parentFileRef.MkDir())
            return err;
        
        parentFileRef.Release();
    }
    
    // Make ourselves if we are a directory. If we are a file, then we are done; path is created
    if (IsDirectory())
        return MkDir();
    else
        return noErr;
}
