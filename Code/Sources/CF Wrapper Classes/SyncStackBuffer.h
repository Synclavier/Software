/*	SyncStackBuffer.h																		*/

#ifndef SyncStackBuffer_h
#define SyncStackBuffer_h

#import "CSynclavierTypes.h"

// Class to provide a stack-based self-releasing buffer
class SyncStackBuffer {
public:

    // constructors / destructors
    SyncStackBuffer (int size);
    ~SyncStackBuffer();

    SyncUint16*      Buffer()    {return                  itsBuffer;}
    char*            Chars ()    {return (char*)          itsBuffer;}
    unsigned char*   Bytes ()    {return (unsigned char*) itsBuffer;}
    int              Size  ()    {return                  itsSize;  }

    void             Resize(int newSize);

private:
    SyncUint16* itsBuffer;
    int         itsSize;
};

#endif

