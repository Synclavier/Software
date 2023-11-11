//
//  SynclavierFileReference.h
//  XPL Compiler-XCode4-10-8
//
//  Created by Cameron Jones on 8/27/14.
//
//

#ifndef __XPL_Compiler_XCode4_10_8__SynclavierFileReference__
#define __XPL_Compiler_XCode4_10_8__SynclavierFileReference__

//
// SyncFSSpec Utility FUnctions
//

#if __LP64__
    // For 64-bit builds the FSSpec is replaced by a union that matches the size of the original FSSpec.
    // We do this because the struct is written to disk in the prefs file.
    // The 64-bit contents is of course meaningless when on disk.

    #pragma pack(push,2)

    typedef union {
        UInt8                           hidden[70];
        struct {
            class CSynclavierFileReference* file_ref;
            char                            file_name[62];  // Sporadically available. Used for sorting a directory, for example; also guessing at file type.
        };
    } SyncFSSpec;

    #pragma pack(pop)

#else
    typedef FSSpec SyncFSSpec;
#endif

extern  void    SyncFSSpecRetain (SyncFSSpec*);
extern  void    SyncFSSpecRelease(SyncFSSpec*);


//
// class CSynclavierFileReference
//

class   CSynclavierFileReference
{
public:
    CSynclavierFileReference(CFDataRef   markData);                     // Create from a bookmark; takes ownership of the bookmark
    CSynclavierFileReference(CFURLRef    cfURL   );                     // Create from CFURLRef; takes ownership of the CFURLRef
    CSynclavierFileReference(CFStringRef pathStr, bool isPosix = true); // Create from posix or URL path; takes ownership of the pathStr
    CSynclavierFileReference(const char* pathBuf, bool isPosix = true); // Create from posix or URL path; no ownership invovled
    CSynclavierFileReference(FSRef*      aFSRef  );                     // Create from a FSRef; no ownership invovled
    
    ~CSynclavierFileReference();
    
    // 32-bit builds - creates new from FSSpec
    // 64-bit builds - retains and returns existing CSynclavierFileReference
    static      CSynclavierFileReference* CopyFromFSSpec(SyncFSSpec* aFSSpec);  // Needs to be released
    static      CSynclavierFileReference* CopyAppFileRef();                     // Needs to be released
    static      CSynclavierFileReference* CopyAppResourcesRef();                // Needs to be released
    static      CSynclavierFileReference* CopyAppSupportRef();                  // Needs to be released
    static      CSynclavierFileReference* CopyUserDocumentsRef();               // Needs to be released
    static      class SyncMutex&          GetMutex();

    // Getters - returned item must not be released
    inline      CFURLRef    GetURL      () {return URL     ();}  // Get URL      noCopy
    inline      CFStringRef GetPath     () {return Path    ();}  // Get Path     noCopy
    inline      CFStringRef GetName     () {return Name    ();}  // Get Name     noCopy
    inline      CFStringRef GetHandle   () {return Handle  ();}  // Get Handle   noCopy
    inline      CFDataRef   GetBookmark () {return Bookmark();}  // Get Bookmark noCopy
    inline      CFURLRef    GetParent   () {return Parent  ();}  // Get Parent   noCopy
    inline      int         GetFile     () {return fileDes   ;}  // Get file descriptor (if open)
    inline      int         GetFileMode () {return fileMode  ;}  // Get file open mode  (if open)
    
    inline      CFURLRef    CopyURL     () {if (URL     ()) CFRetain(URL     ()); return URL     ();}
    inline      CFStringRef CopyPath    () {if (Path    ()) CFRetain(Path    ()); return Path    ();}
    inline      CFDataRef   CopyBookmark() {if (Bookmark()) CFRetain(Bookmark()); return Bookmark();}

    // Creators
    CFURLRef    CreateSibling(CFStringRef sibling, bool isDir); // Create sibling URLRef; needs to be released; consumes the sibling stringRef
    CFURLRef    CreateChild  (CFStringRef child,   bool isDir); // Create child   URLRef; needs to be released; consumes the child   stringRef
    short       CreateFSRef  (FSRef*      aFSRef );             // Compatibility - get FSRef; no ownership invovled
    short       CreateFSSpec (SyncFSSpec* aFSSpec);             // Compatibility - get FSSpec; needs to be released
    
    // Funtions
    bool        Path    (char* buf, int maxLen);                // Returns Posix path (file system encoding)
    short       Open    (int mode);                             // Opens the file from path in mode if need be
    long long   Size    ();                                     // Get size ** also seeks back to the start of the file **
    short       Seek    (long long where);
    short       Close   ();                                     // Closes the file if need be
    short       Sync    ();                                     // Flushes disk
    short       Delete  ();                                     // Closes and deletes the file
    short       Read    (SInt32* count, void* buffPtr);         // Reads from current position
    short       XPLRead (SInt32* count, void* buffPtr);         // Reads from current position with able byte swizzle
    short       Write   (SInt32* count, void* buffPtr);         // Writes from current position
    short       XPLWrite(SInt32* count, void* buffPtr);         // Writes to current position with able byte swizzle
    short       Clone   (CSynclavierFileReference* fileRef);    // Clones a file into the current file
    short       Skip    (int count);                            // Seeks forward in file
    short       Create     (OSType type, OSType creator, UInt16 finderFlags = 0, long long fileLength = 0);
    short       SetFinfo   (OSType type, OSType creator, UInt16 finderFlags = 0);
   
    void        Retain ();
    void        Release();
    
    void        Resolve    ();
    bool        Reachable  ();
    bool        IsDirectory();
    bool        IsAlias    ();
    bool        IsOpen     ();
    
    // Directory
    short       MkDir  ();
    short       MkPath ();
    
private:
    void        Init    ();
    void        Finish  ();
    
    CFURLRef    URL     ();                                 // Creates URL from bookmark if need be
    CFStringRef Path    ();                                 // Creates posix path from URL if need be
    CFStringRef Name    ();                                 // Creates name string from URL UTF8
    void        CName   (char *name, int maxLen);           // Creates name string from URL UTF8
    CFStringRef Handle  ();                                 // Creates handle string from URL UTF8
    CFDataRef   Bookmark();                                 // Creates bookmark from URL if need be
    CFURLRef    Parent  ();                                 // Creates parent URL if need be

    // Immutable attrributes:
    CFDataRef   bookMark;                                   // Bookmark to file
    CFURLRef    urlRef;                                     // CFURLRef created from the bookmark
    CFStringRef pathRef;                                    // Posix path
    CFStringRef nameRef;                                    // Name if retrieved
    CFStringRef handleRef;                                  // Handle if retrieved
    CFURLRef    parentRef;                                  // For parent if created
    
    // Mutable attributes
    int         fileDes;                                    // Posix filedes if file is open
    int         fileMode;                                   // Mode it is open with
    int         openCount;                                  // Count of openers
    volatile int32_t retainCount;                           // Count of owners
protected:

};


// Stack-based wrapper that closes and releases a file reference
class CSynclavierFileRefManager {
public:
    
    // constructors / destructors
    inline  CSynclavierFileRefManager (CSynclavierFileReference* ref) {reference = ref;}
    inline ~CSynclavierFileRefManager (                             ) {reference->Release();}
    
    inline CSynclavierFileReference& Reference() {return *reference;}

private:
    CSynclavierFileReference* reference;
};

#endif /* defined(__XPL_Compiler_XCode4_10_8__SynclavierFileReference__) */
