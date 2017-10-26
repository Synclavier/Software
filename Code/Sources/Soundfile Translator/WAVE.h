/*
	File:		WAVE.h

	Contains:	Header information needed to parse Microsoft's WAVE formatted sounds.

	Written by: Mark Cookson	

	Copyright:	Copyright © 1999 by Apple Computer, Inc., All Rights Reserved.

				You may incorporate this Apple sample source code into your program(s) without
				restriction. This Apple sample source code has been provided "AS IS" and the
				responsibility for its operation is yours. You are not permitted to redistribute
				this Apple sample source code as "Apple sample source code" after having made
				changes. If you're going to re-distribute the source, we require that you make
				it clear in the source that the code was descended from Apple sample source
				code, but that you've made changes.

	Change History (most recent first):
				2000/08/30	Cameron Jones	changed file_size to ckSize in WAVEContainerChunk
				2000/05/30	Todd Yvega		fmtChunk was missing wBitsPerSample member
				1999/08/31	Karl Groethe	Updated for Metrowerks Codewarror Pro 2.1

*/

/*	This was made from reading a document called wave.pdf which is an excerpt from
	RIFFMCI.RTF, "Multimedia Programming Interface and Data Specification v1.0".

	This code does what I need, and worked with the WAVE files I had handy.
	It may not work in all cases.
*/

/*
	Note: As required in the paragraph after the copyright notice above, I hereby make it clear that this 
	code is descended from Apple sample source code, but that I've made changes.	-Todd Yvega	2000/05/30
*/

#ifndef __WAVE__
#define __WAVE__

#include <CoreServices/CoreServices.h>

/*
#ifndef __SOUNDSTRUCT__
#include "SoundStruct.h"
#endif

#ifndef __DBFFERRORS__
#include "DBFF_Errors.h"
#endif

#ifndef __SETUPDBHEADER__
#include "SetupDBHeader.h"
#endif

#ifndef __DEFINES__
#include "Defines.h"
#endif
*/

// --------------------------------------------------------------------------
//	¥ Ramblings on the WAVE file format
// --------------------------------------------------------------------------

//	The canonical WAVE format starts with the RIFF header:
//	  Offset  Length   Contents
//	  0       4 bytes  'RIFF'
//	  4       4 bytes  <file length - 8>
//	  8       4 bytes  'WAVE'
//	(The '8' in the second entry is the length of the first two entries. I.e., the second entry
//	 is the number of bytes that follow in the file.)
//	Next, the fmt chunk describes the sample format:
//	  12      4 bytes  'fmt '
//	  16      4 bytes  0x00000010     // Length of the fmt data (16 bytes)
//	  20      2 bytes  0x0001         // Format tag: 1 = PCM
//	  22      2 bytes  <channels>     // Channels: 1 = mono, 2 = stereo
//	  24      4 bytes  <sample rate>  // Samples per second: e.g., 44100
//	  28      4 bytes  <bytes/second> // sample rate * block align
//	  32      2 bytes  <block align>  // channels * bits/sample / 8
//	  34      2 bytes  <bits/sample>  // 8 or 16
//	Finally, the data chunk contains the sample data:
//	  36      4 bytes  'data'
//	  40      4 bytes  <length of the data block>
//	  44        bytes  <sample data>
//	The sample data must end on an even byte boundary. All numeric data fields are in the Intel
//	format of low-high byte ordering. 8-bit samples are stored as unsigned bytes, ranging from 0 to 255.
//	16-bit samples are stored as two's complement signed integers, ranging from -32768 to 32767.

/*
#define kWAVEFORMID				(1<<0)
#define kWAVEID					(1<<1)
#define kFormatID				(1<<2)
#define kWAVEListID				(1<<3)
#define kSilenceID				(1<<4)
#define kCueID					(1<<5)
#define kFactID					(1<<6)
#define kPlaylistID				(1<<7)
#define kAssocDataID			(1<<8)
#define kLabelID				(1<<9)
#define kNoteID					(1<<10)
#define kTextWithLenID			(1<<12)
#define kEmbededFileID			(1<<13)
#define kWAVEDataID				(1<<14)
*/

enum {
	WAVEFORMID					= FOUR_CHAR_CODE('RIFF'),
	WAVEID						= FOUR_CHAR_CODE('WAVE'),
	FormatID					= FOUR_CHAR_CODE('fmt '),
	WAVEListID					= FOUR_CHAR_CODE('wavl'),
	SilenceID					= FOUR_CHAR_CODE('slnt'),
	CueID						= FOUR_CHAR_CODE('cue '),
	FactID						= FOUR_CHAR_CODE('fact'),
	PlaylistID					= FOUR_CHAR_CODE('plst'),
	AssocDataID					= FOUR_CHAR_CODE('adtl'),
	LabelID						= FOUR_CHAR_CODE('labl'),
	NoteID						= FOUR_CHAR_CODE('note'),
	TextWithLenID				= FOUR_CHAR_CODE('ltxt'),
	EmbededFileID				= FOUR_CHAR_CODE('file'),
	WAVEDataID					= FOUR_CHAR_CODE('data')
};

#define WAVE_FORMAT_PCM		(0x0001)		/* Microsoft Pulse Code Modulation (PCM) format */
#define WAVE_FORMAT_ADPCM	(0x0002)		/* A WAVE Adaptive Differential Pulse Code Modulation file I saw once */
#define WAVE_FORMAT_MULAW	(0x0007)		/* A WAVE mu-law file that I saw once */
#define WAVE_FORMAT_IMA		(0x0011)		/* A WAVE IMA4 file that I saw once */
#define IBM_FORMAT_MULAW	(0x0101)		/* IBM mu-law format */
#define IBM_FORMAT_ALAW		(0x0102)		/* IBM a-law format */
#define IBM_FORMAT_ADPCM	(0x0103)		/* IBM AVC Adaptive Differential Pulse Code Modulation format */
/*
#define kWAVEChunkBufferSize	128
*/

typedef struct WAVEChunkHeader {
	UInt32				ckID;
	SInt32				ckSize;
}WAVEChunkHeader;

typedef struct WAVEContainerChunk {
	UInt32				ckID;
	SInt32				ckSize;
	UInt32				formType;
}WAVEContainerChunk;

typedef struct fmtChunk {
	UInt32				ckID;
	SInt32				ckSize;
	short				wFormatTag;			/* Number indicating WAVE format category */
	short				wChannels;			/* Number of channels 1 for mono 2 for stereo */
	SInt32				dwSamplesPerSec;	/* Sampling rate in samples per second */
	SInt32				dwAvgBytesPerSec;	/* Average number of bytes per second (could be used to estimate buffer sizes) */
	short				wBlockAlign;		/* Block alignment in bytes of the waveform data, always process an integer multiple of this number */
	short				wBitsPerSample;		/* Sample size */
}fmtChunk;

typedef struct PCMFmtSpecChunk {
	UInt32				ckID;
	SInt32				ckSize;
	short				wBitsPerSample;		/* Sample size */
}PCMFmtSpecChunk;

typedef struct factChunk {
	UInt32				ckID;
	SInt32				ckSize;
	SInt32				dwFileSize;			/* Number of samples */
}factChunk;

typedef struct cuePointsChunk {
	UInt32				ckID;
	SInt32				ckSize;
	SInt32				dwCuePoints;		/* Count of cue points */
	/* There may be multiple number of these */
	SInt32				dwName;
	SInt32				dwPosition;
	SInt32				fccChunk;
	SInt32				dwChunkStart;
	SInt32				dwBlockStart;
	SInt32				dwSampleOffset;
}cuePointsChunk;

typedef struct playlistChunk {
	UInt32				ckID;
	SInt32				ckSize;
	SInt32				dwSegments;			/* Count of play segments */
	/* There may be multiple number of these */
	SInt32				dwName;
	SInt32				dwLength;
	SInt32				dwLoops;
}playlistChunk;

typedef struct waveDataChunk {
	UInt32				ckID;
	SInt32				ckSize;

}waveDataChunk;

typedef struct labelChunk {
	UInt32				ckID;
	SInt32				ckSize;				// I assume this should be here	-Todd Yvega	2000/05/30
	SInt32				dwName;
	char				data[1];			/* This is a null terminated string */
}labelChunk;

typedef struct noteChunk {
	UInt32				ckID;
	SInt32				ckSize;
	SInt32				dwName;
	char				data[1];			/* This is a null terminated string */
}noteChunk;

typedef struct ltxtChunk {
	UInt32				ckID;
	SInt32				ckSize;
	SInt32				dwName;
	SInt32				dwSampleLength;
	SInt32				dwPurpose;
	short				wCountry;
	short				wLanguage;
	short				wDialect;
	short				wCodePage;
	char				data[1];			/* This is a null terminated string */
}ltxtChunk;

typedef struct fileChunk {
	UInt32				ckID;
	SInt32				ckSize;
	SInt32				dwName;
	SInt32				dwMedType;
	char				data[1];			/* This is a null terminated string */
}fileChunk;

typedef struct assocDataListChunk {
	UInt32				ckID;
	SInt32				ckSize;
	labelChunk			labelInfo;
	noteChunk			noteInfo;
	ltxtChunk			ltxtInfo;
	fileChunk			fileInfo;
}assocDataListChunk;

/*
typedef union {
	WAVEChunkHeader			generic;
	WAVEContainerChunk		container;
	fmtChunk				fmt;
	factChunk				fact;
	cuePointsChunk			cuePoints;
	playlistChunk			playList;
	assocDataListChunk		assocData;
//	waveDataChunk			waveData;
	PCMFmtSpecChunk			waveData;
}WAVETemplate, *WAVETemplatePtr;

OSErr		ASoundGetWAVEHeader		(SoundInfoPtr theSoundInfo,
									long *dataStart,
									long *length);

short		SwapShort				(const short theShort);
long		SwapLong				(const long theLong);
long		ReverseLong				(const long theLong);

#define stillMoreDataWAVEToRead		((chunkFlags & kWAVEFORMID) && (!(chunkFlags & kFormatID) || !(chunkFlags & kWAVEDataID)) && (err == noErr))
*/

#endif
