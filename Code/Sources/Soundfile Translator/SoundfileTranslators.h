// ==========================================================================
//	SoundfileTranslators.h
// ==========================================================================

// Contents: struct and routine declarations for SoundfileTranslators.cp

// REVISION HISTORY:
//		2000/06/04	Todd Yvega		Created.
//		2000/08/28	Cameron Jones	Blended.

#pragma once

#import <AudioToolbox/AudioToolbox.h>

// Stack-based wrapper that closes and releases an AudioFileID
class CSynclavierAudioFileIDManager {
public:
    
    // constructors / destructors
    inline  CSynclavierAudioFileIDManager (AudioFileID fileID) {fileIDRef = fileID;}
    inline ~CSynclavierAudioFileIDManager (                  ) {if (fileIDRef) AudioFileClose(fileIDRef);}
    
    inline AudioFileID FileID()                   {return fileIDRef;  }
    inline void        NewId (AudioFileID fileID) {fileIDRef = fileID;}
    
private:
    AudioFileID fileIDRef;
};

// --------------------------------------------------------------------------
//	¥ Struct used to store information describing where in a file audio data is
// --------------------------------------------------------------------------

typedef struct AudioDataDescriptor {
	// Audio data info:
	int     start_pos_in_file;			// byte offset from start of file where audio data starts
	int     byte_len_in_file;			// number of bytes (starting at start_pos_in_file) that contain audio data
	int     bytes_need_swizzling;		// true if audio data bytes need swizzling (e.g. WAV files)
										//		Note: if (bits_per_sample == 8) bytes_need_swizzling indicates audio data offset by +128
	// Audio content info:
	int     bits_per_sample;			// bits per sample - e.g. 8 or 16
	int     samples_per_frame;			// samples per frame - e.g. 1 or 2
	int     frames_per_file;			// frames per file
	double	frames_per_second;			// sample rate frames per second

} AudioDataDescriptor;

void SwapBytesSynclSFHeader(struct SynclSFHeader* header);

OSErr ParseAIFFSoundFile(short dataRefnum, short resourceRefnum,  struct AudioDataDescriptor& descriptor, struct SynclSFHeader& sf_header);
OSErr ParseAIFFSoundFile(class CSynclavierFileReference& fileRef, struct AudioDataDescriptor& descriptor, struct SynclSFHeader& sf_header_struct);
OSErr ParseSd2fSoundFile(short dataRefnum, short resourceRefnum,  struct AudioDataDescriptor& descriptor, struct SynclSFHeader& sf_header);
OSErr ParseWAVESoundFile(short dataRefnum, short resourceRefnum,  struct AudioDataDescriptor& descriptor, struct SynclSFHeader& sf_header);
OSErr ParseWAVESoundFile(class CSynclavierFileReference& fileRef, struct AudioDataDescriptor& descriptor, struct SynclSFHeader& sf_header);

long  ComputeAIFFSoundFileSize(SynclSFHeader& sf_header_struct, AudioDataDescriptor& descriptor);
long  ComputeSd2fSoundFileSize(SynclSFHeader& sf_header_struct, AudioDataDescriptor& descriptor);
long  ComputeWAVESoundFileSize(SynclSFHeader& sf_header_struct, AudioDataDescriptor& descriptor);

OSErr CreateAIFFSoundFile(short dataRefnum, short resourceRefnum,  struct AudioDataDescriptor& descriptor, struct SynclSFHeader& sf_header);
OSErr CreateAIFFSoundFile(class CSynclavierFileReference& fileRef, struct AudioDataDescriptor& descriptor, struct SynclSFHeader& sf_header);
OSErr CreateSd2fSoundFile(short dataRefnum, short resourceRefnum,  struct AudioDataDescriptor& descriptor, struct SynclSFHeader& sf_header);
OSErr CreateWAVESoundFile(short dataRefnum, short resourceRefnum,  struct AudioDataDescriptor& descriptor, struct SynclSFHeader& sf_header);
OSErr CreateWAVESoundFile(class CSynclavierFileReference& fileRef, struct AudioDataDescriptor& descriptor, struct SynclSFHeader& sf_header);
