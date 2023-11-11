/*	SyncStackBuffer.cpp															*/

// Local includes
#include "SyncStackBuffer.h"

// Handy stack-based buffer
// Class to provide a stack-based self-releasing buffer
SyncStackBuffer::SyncStackBuffer(int size) {
    itsSize = size;

    if (itsSize > 0)
        itsBuffer = (SyncUint16*) valloc(size);
    else
        itsBuffer = NULL;
}

SyncStackBuffer::~SyncStackBuffer() {
    if (itsBuffer)
        free(itsBuffer);
}

void SyncStackBuffer::Resize(int newSize) {
    if (itsBuffer)
        free(itsBuffer);

    itsSize = newSize;

    if (itsSize > 0)
        itsBuffer = (SyncUint16*) valloc(itsSize);

    else
        itsBuffer = NULL;
}

