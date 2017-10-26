// =================================================================================
//	SoundfileTranslators.cp
// =================================================================================

//	CONTAINS: SOURCE CODE FOR EXTRACTING INFO FROM AN AIFF, WAVE OR SDII FILE.
//	REVISION HISTORY:
//		2000/05/12	Todd Yvega		Created.
//		2000/08/28	Cameron Jones	Blended.

/*ииииииииииииииииииииииииии  # I N C L U D E S  ииииииииииииииииииииииииии*/

// MSL C
#include <iostream>
#include <iomanip>					//	only needed for cout manipulators which take arguments such as setw()
#include <string.h>

// Local includes
#include "SoundfileTranslators.h"
#include "CSynclavierSoundFileHeader.h"
#include "SynclavierFileReference.h"
#include "WAVE.h"

using namespace std;

extern "C" {
	extern double x80tod(const extended80 * x80);
	extern void   dtox80(const double *x, extended80 *x80);
}


// ---------------------------------------------------------------------------
//	╔ Handy Structures and Unions
// ---------------------------------------------------------------------------

//	The following structure is supplied just to make it easy to read in the data following the chunk header
//	for any type of chunk without having to know the name of the member located there.

#pragma pack(2)

typedef struct GenericChunk {
	ChunkHeader			chunk_header;
	Byte				chunk_data[1];		//	GenericChunk.chunk_data provides a pointer to the first byte after the ChunkHeader

} GenericChunk;

typedef struct SynclavierChunk {
	unsigned int		ckID;
	int 				ckSize;
	SynclSFHeader		sfHeader;

} SynclavierChunk;

typedef struct WAVEGenericChunk {
	WAVEChunkHeader		chunk_header;
	Byte				chunk_data[1];

} WAVEGenericChunk;

typedef struct WAVESynclavierChunk {
	unsigned int		ckID;
	int                 ckSize;
	SynclSFHeader		sfHeader;

} WAVESynclavierChunk;

//	Handy struct to hold up to 64 markers...
typedef struct BigMarkerChunk {
	unsigned int 					ckID;
	int 							ckSize;
	unsigned short 					numMarkers;
	Marker 							Markers[64];
} BigMarkerChunk;

//	Make sure a ChunkBuffer is large enough to hold the largest chunk type to be read.  Don't worry about
//	unexpected chunk types.  Only their headers (8 bytes) are read into the ChunkHeader member.
typedef	union	ChunkBuffer {
	//	AIFF/AIFC chunk types
	ChunkHeader			chunk_header;
	GenericChunk		generic_chunk;
	ContainerChunk		container_chunk;
	CommonChunk			common_chunk;
	ExtCommonChunk		ext_common_chunk;
	SoundDataChunk		sound_data_chunk;
	MarkerChunk			marker_chunk;
	BigMarkerChunk		big_marker_chunk;
	InstrumentChunk		instrument_chunk;
	CommentsChunk		comments_chunk;
	SynclavierChunk		synclavier_chunk;

	//	WAVE chunk types
	WAVEChunkHeader		wave_chunk_header;
	WAVEGenericChunk	wave_generic_chunk;
	WAVEContainerChunk	wave_container_chunk;
	WAVESynclavierChunk	wave_synclavier_chunk;
	fmtChunk			fmt_chunk;
	PCMFmtSpecChunk		pcm_fmt_spec_chunk;
	factChunk			fact_chunk;
	cuePointsChunk		cue_points_chunk;
	playlistChunk		play_list_chunk;
	labelChunk			label_chunk;
	noteChunk			note_chunk;
	ltxtChunk			ltxt_chunk;
	fileChunk			file_chunk;
	assocDataListChunk	assoc_data_list_chunk;
	waveDataChunk		wave_data_chunk;
	
	//	Synclavier types
	SynclSFHeader		sf_header;
	SInt16				signal_data[3*256];
} ChunkBuffer;

#pragma options align=reset

// Swap bytes of extended80 data type - stored on disk as big-endian
static void SwapBytesExtended80(extended80* it)
{
	// extended80 is apparently big-endian even on intel macs...
	#if 0
		it->exp    = CFSwapInt16HostToBig(it->exp);
		it->man[0] = CFSwapInt16HostToBig(it->man[0]);
		it->man[1] = CFSwapInt16HostToBig(it->man[1]);
		it->man[2] = CFSwapInt16HostToBig(it->man[2]);
		it->man[3] = CFSwapInt16HostToBig(it->man[3]);
	#endif
}

	UInt32	sector;
	UInt16	word_offset;

static void SwapBytesSynclSFIndex(SynclSFIndex* it)
{
	it->sector      = CFSwapInt32HostToBig(it->sector);
	it->word_offset = CFSwapInt16HostToBig(it->word_offset);
}
	
static void SwapBytesSynclSFTime(SynclSFTime* it)
{
	it->seconds      = CFSwapInt16HostToBig(it->seconds);
	it->milliseconds = CFSwapInt16HostToBig(it->milliseconds);
	it->microseconds = CFSwapInt16HostToBig(it->microseconds);
}
	
static void SwapBytesSynclSFSymbol(SynclSFSymbol* it)
{
	SwapBytesSynclSFTime(&it->time);
	
	it->numchars      = CFSwapInt16HostToBig(it->numchars);
	
	// name stored as unsigned char
}
	
void SwapBytesSynclSFHeader(struct SynclSFHeader* it)
{
	int i;
	
	it->compatibility  = CFSwapInt16HostToBig(it->compatibility);
	it->file_data_type = CFSwapInt16HostToBig(it->file_data_type);
	it->file_data_type = CFSwapInt16HostToBig(it->file_data_type);

	SwapBytesSynclSFIndex(&it->valid_data);

	it->unused_005     = CFSwapInt16HostToBig(it->unused_005);

	SwapBytesSynclSFIndex(&it->total_data);
	SwapBytesSynclSFTime (&it->data_end);
	
	it->keyboard_decay = CFSwapInt16HostToBig(it->keyboard_decay);
	it->semitones      = CFSwapInt16HostToBig(it->semitones);
	it->vibrato_rate   = CFSwapInt16HostToBig(it->vibrato_rate);
	it->vibrato_depth  = CFSwapInt16HostToBig(it->vibrato_depth);
	it->vibrato_attack = CFSwapInt16HostToBig(it->vibrato_attack);
	it->vibrato_type   = CFSwapInt16HostToBig(it->vibrato_type);
	it->hertz          = CFSwapInt16HostToBig(it->hertz);
	
	// XPLFloating - octave - see struct definition
	
	it->period_index   = CFSwapInt16HostToBig(it->period_index);
	it->nyquist_freq   = CFSwapInt16HostToBig(it->nyquist_freq);

	SwapBytesSynclSFTime (&it->mark_start);
	SwapBytesSynclSFTime (&it->mark_end);
	SwapBytesSynclSFTime (&it->cursor_time);

	it->gain_exponent     = CFSwapInt16HostToBig(it->gain_exponent);
	it->number_of_symbols = CFSwapInt16HostToBig(it->number_of_symbols);

	SwapBytesSynclSFIndex(&it->total_length);
	SwapBytesSynclSFIndex(&it->loop_length);

	it->magic_number = CFSwapInt16HostToBig(it->magic_number);
	it->stereo       = CFSwapInt16HostToBig(it->stereo);
	it->sample_rate  = CFSwapInt16HostToBig(it->sample_rate);
	it->unused_043   = CFSwapInt16HostToBig(it->unused_043);
	it->unused_044   = CFSwapInt16HostToBig(it->unused_044);
	it->smpte_mode   = CFSwapInt16HostToBig(it->smpte_mode);
	it->smpte_bits   = CFSwapInt16HostToBig(it->smpte_bits);

	// smpte_seconds, smpte_frames, smpte_hours, smpte_minutes => stored as byte
    // file handle stored as byte
	// skip unused unused_085[127-85]
	// id_field stored as char; they need to be swapped if you will be using the 'byte' function

	SwapBytesSynclSFTime (&it->mark_offset);

	it->index_base     = CFSwapInt16HostToBig(it->index_base);
	it->id_field_bytes = CFSwapInt16HostToBig(it->id_field_bytes);

	for (i=0; i<64; i++)
		SwapBytesSynclSFSymbol(&it->symbols[i]);
}

// ---------------------------------------------------------------------------
//	╔ ParseAIFFSoundFile(short dataRefnum, short resourceRefnum, SynclSFHeader& sf_header)
// ---------------------------------------------------------------------------
//	Handles AIFF and AIFC files.

OSErr	ParseAIFFSoundFile(CSynclavierFileReference& fileRef, AudioDataDescriptor& descriptor, SynclSFHeader& sf_header_struct)
{
	CSynclSFHeader	sf_header(sf_header_struct);		// Create CSynclSFHeader with which to manipulate struct
	ContainerChunk	MyContainerChunk;	//	12 bytes
	OSErr			MyOSErr	= noErr;
	SInt32			num_bytes_requested = 0, num_bytes_read = 0;
	SInt64			end_of_file = 0;
	SInt64			DebugFPos = 0;
	Boolean			common_chunk_scanned     = false;
	Boolean			sound_data_chunk_scanned = false;
	Boolean			sncl_chunk_scanned       = false;
	int				i;

	//	init output struct
	sf_header.init();
	memset(&descriptor, 0, sizeof(descriptor));

	//	get and verify the 'FORM' container chunk
	num_bytes_read = num_bytes_requested = sizeof(ContainerChunk);
	
	if ((MyOSErr = fileRef.Seek(0)) != 0)
	{
		printf("ParseAIFFSoundFile: failed SetFPos (%d)\n", MyOSErr);
		return MyOSErr;
	}
	
	if ((MyOSErr = fileRef.Read(&num_bytes_read, (void *) &MyContainerChunk)) != 0)
	{
		printf("ParseAIFFSoundFile: failed FSRead (%d)\n", MyOSErr);
		return MyOSErr;
	}

	MyContainerChunk.ckID     = CFSwapInt32HostToBig(MyContainerChunk.ckID);
	MyContainerChunk.ckSize   = CFSwapInt32HostToBig(MyContainerChunk.ckSize);
	MyContainerChunk.formType = CFSwapInt32HostToBig(MyContainerChunk.formType);

	if (num_bytes_read != num_bytes_requested)
	{
		printf("ParseAIFFSoundFile: failed FSRead (%d)\n", MyOSErr);
		return MyOSErr;
	}
	
	//	check for errors not flagged in OSErr
	if (MyContainerChunk.ckID != FORMID)
	{
		printf("ParseAIFFSoundFile: file does not appear to be AIFF\n");
		return -1;
	}
	
	if ((MyContainerChunk.formType != AIFFID) && (MyContainerChunk.formType != AIFCID))
	{
		printf("ParseAIFFSoundFile: File does not contain an AIFF form descriptor\n");
		return -1;
	}
	
	// Make sure file is long enough for chunk.  (at least two AIFF file exporters are known to screw this up)
	// Simulate shorter chunk if file is too short
	
	end_of_file = MyContainerChunk.ckSize + sizeof(ChunkHeader);
    DebugFPos   = fileRef.Size();
	
	if (DebugFPos < end_of_file)
	{
		if (DebugFPos < sizeof(ChunkHeader) + 100)
		{
			printf("ParseAIFFSoundFile: file is too short to be AIFF\n");
			return -1;
		}
		
		MyContainerChunk.ckSize = (SInt32) (DebugFPos - sizeof(ChunkHeader));
	}

	//	restore FPos
	SInt64 SourceFPos = sizeof(ContainerChunk);	//	this is the position of the first local chunk in the 'FORM' chunk
	
	if ((MyOSErr = fileRef.Seek(SourceFPos)) != 0)
	{
		printf("ParseAIFFSoundFile: failed SetFPos (%d)\n", MyOSErr);
		return MyOSErr;
	}
	
	// Process chunks in the file
	while (SourceFPos < end_of_file)
	{
		ChunkBuffer	source_buffer;
		
		//	get the header of the next chunk
		num_bytes_read = num_bytes_requested = sizeof(ChunkHeader);

		if ((MyOSErr = fileRef.Read(&num_bytes_read, &source_buffer)) != 0)
		{
			printf("ParseAIFFSoundFile: failed FSRead of chunk (%d)\n", MyOSErr);
			return MyOSErr;
		}
		
		source_buffer.chunk_header.ckID   = CFSwapInt32HostToBig(source_buffer.chunk_header.ckID);
		source_buffer.chunk_header.ckSize = CFSwapInt32HostToBig(source_buffer.chunk_header.ckSize);

		if (num_bytes_read != num_bytes_requested)
		{
			printf("ParseAIFFSoundFile: missing chunk data\n");
			return -1;
		}
		
		// Advance over chunk header (that is advance SourceFPos to chunk data)
		SourceFPos += num_bytes_read;
		
		//	process based on what kind of chunk it is
		switch (source_buffer.chunk_header.ckID)
		{
			// Handle common chunk
			case CommonID:
			{
				common_chunk_scanned = true;
				
				// Assume can read entire chunk; but limit to max size of buffer
				num_bytes_read = num_bytes_requested = source_buffer.chunk_header.ckSize;
				
				if (num_bytes_requested > (sizeof(source_buffer) - sizeof(ChunkHeader)))
					num_bytes_read = num_bytes_requested = (sizeof(source_buffer) - sizeof(ChunkHeader));
					
				if (num_bytes_requested < (sizeof(CommonChunk) - sizeof(ChunkHeader)))
				{
					printf("ParseAIFFSoundFile: common chunk is too small\n");
					return -1;
				}
				
				if ((MyOSErr = fileRef.Read(&num_bytes_read, source_buffer.generic_chunk.chunk_data)) != 0)
				{
					printf("ParseAIFFSoundFile: failed FSRead of common chunk balance (%d)\n", MyOSErr);
					return MyOSErr;
				}
				
				source_buffer.common_chunk.numChannels     = CFSwapInt16HostToBig(source_buffer.common_chunk.numChannels);
				source_buffer.common_chunk.numSampleFrames = CFSwapInt32HostToBig(source_buffer.common_chunk.numSampleFrames);
				source_buffer.common_chunk.sampleSize      = CFSwapInt16HostToBig(source_buffer.common_chunk.sampleSize);
				
				SwapBytesExtended80(&source_buffer.common_chunk.sampleRate);

				if (num_bytes_read != num_bytes_requested)
				{
					printf("ParseAIFFSoundFile: missing common chunk data\n");
					return -1;
				}

				//	set a few global parameters that will affect the translation
				descriptor.bits_per_sample   = source_buffer.common_chunk.sampleSize;	// bits per sample **after** expansion
				descriptor.samples_per_frame = source_buffer.common_chunk.numChannels;
				descriptor.frames_per_file   = source_buffer.common_chunk.numSampleFrames;
				
				#if TARGET_CPU_68K
					descriptor.frames_per_second = (double) source_buffer.common_chunk.sampleRate;
				#else
					descriptor.frames_per_second = x80tod(&source_buffer.common_chunk.sampleRate);
				#endif

				// Check a couple basic things
				if (descriptor.bits_per_sample != 8 && descriptor.bits_per_sample != 16 && descriptor.bits_per_sample != 24 &&  descriptor.bits_per_sample != 32)
				{
					printf("ParseAIFFSoundFile: file contains other than 8-bit, 16-bit, 24-bit or 32-bit audio data\n");
					return -1;
				}
				
				if (descriptor.samples_per_frame != 1 && descriptor.samples_per_frame != 2)
				{
					printf("ParseAIFFSoundFile: file contains more than 2 channels of audio data\n");
					return -1;
				}
			
				//	print out the goods common to both AIFF and AIFC
				if (0)
				{
					cout	<< "\tbits_per_sample   = " << descriptor.bits_per_sample << endl
							<< "\tsamples_per_frame = " << descriptor.samples_per_frame << endl
							<< "\tframes_per_file   = " << descriptor.frames_per_file << endl
							<< "\tframes_per_second = " << descriptor.frames_per_second << endl;
				}

				// 	leave sf header untouched if it came from a SNCL record
				if (sncl_chunk_scanned)
					break;

				//	fill out sound file header for basic information
				sf_header.mData.stereo = descriptor.samples_per_frame - 1;
				sf_header.set_rates(descriptor.frames_per_second);

				//	translate numSampleFrames into data_end, mark_end and the symbol times for "$" and "END"
				SynclSFTime	time_of_last_sample = sf_header.sampleframes_to_time(descriptor.frames_per_file - 1);
				
				sf_header.mDataEnd.assign(time_of_last_sample);
				sf_header.mMarkEnd.assign(time_of_last_sample);
				
				{
					CSynclSFSymbol	symbol_0(sf_header.mData.symbols[0]);
					CSynclSFSymbol	symbol_1(sf_header.mData.symbols[1]);
					CSynclSFSymbol	symbol_2(sf_header.mData.symbols[2]);
					CSynclSFSymbol	symbol_3(sf_header.mData.symbols[3]);

					symbol_2.place_time(time_of_last_sample);
					symbol_3.place_time(time_of_last_sample);

					//	translate valid_data and total_data
					unsigned int	num_samples = descriptor.frames_per_file;
					
					if ( descriptor.samples_per_frame == 2)
						num_samples <<= 1;
						
					sf_header.mValidData.words_to_index(num_samples);
					
					num_samples = (descriptor.frames_per_file + 255) & 0xFFFFFF00;
					
					if ( descriptor.samples_per_frame == 2)
						num_samples <<= 1;
						
					sf_header.mTotalData.words_to_index(num_samples);

					//	check compression type
					if ((MyContainerChunk.formType == AIFCID)
					&&  (source_buffer.chunk_header.ckSize >= sizeof(ExtCommonChunk) - sizeof(ChunkHeader)))
					{
						if (source_buffer.ext_common_chunk.compressionType != NoneType)
						{
							printf("ParseAIFFSoundFile: file contains compressed audio data\n");
							return -1;
						}
					}
				}
				
				break;
			}

			// Handle Marker chunk
			case MarkerID:
				
				// Skip marker chunks if we got a syncl header directly
				if (sncl_chunk_scanned)
					break;

				//	Skip markers if we haven't read the common chunk yet; e.g. don't know sample rate
				if (!common_chunk_scanned)
					break;
				
				// 	Read in a big marker chunk which will hold up to 64 symbols (the max we can handle)
				num_bytes_read = num_bytes_requested = source_buffer.chunk_header.ckSize;
				
				if (num_bytes_requested > (sizeof(source_buffer) - sizeof(ChunkHeader)))
					num_bytes_read = num_bytes_requested = (sizeof(source_buffer) - sizeof(ChunkHeader));
					
				if (num_bytes_requested < sizeof(source_buffer.marker_chunk.numMarkers))
				{
					printf("ParseAIFFSoundFile: marker chunk is too small\n");
					return -1;
				}

				if ((MyOSErr = fileRef.Read(&num_bytes_read, source_buffer.generic_chunk.chunk_data)) != 0)
				{
					printf("ParseAIFFSoundFile: failed FSRead of marker chunk balance (%d)\n", MyOSErr);
					return MyOSErr;
				}
				
				source_buffer.big_marker_chunk.numMarkers = CFSwapInt16HostToBig(source_buffer.big_marker_chunk.numMarkers);
				
				for (i=0; i<64; i++)
				{
					source_buffer.big_marker_chunk.Markers[i].id       = CFSwapInt16HostToBig(source_buffer.big_marker_chunk.Markers[i].id);
					source_buffer.big_marker_chunk.Markers[i].position = CFSwapInt32HostToBig(source_buffer.big_marker_chunk.Markers[i].position);
					// markerName stored as bytes...
				}
				
				if (num_bytes_read != num_bytes_requested)
				{
					printf("ParseAIFFSoundFile: missing marker chunk data\n");
					return -1;
				}

				//	Limit what we got to 64
				if	(source_buffer.big_marker_chunk.numMarkers > 64)
					source_buffer.big_marker_chunk.numMarkers = 64;
					
				//	Skip markers if doesn't look like we got all of them
				if (num_bytes_read < (sizeof(source_buffer.big_marker_chunk.numMarkers) + source_buffer.big_marker_chunk.numMarkers*sizeof(Marker)))
					break;
					
				//	Process markers
				for (i=0; i<source_buffer.big_marker_chunk.numMarkers; i++)
				{
					Marker&	marker = source_buffer.big_marker_chunk.Markers[i];
		
					if (0)
                    {
                        // For the record, the portable way to printf Pascal strings in Mac OS X (because it doesn't support the MSL extension %#s) is
                        //    printf("%.*s", pstr[0], &pstr[1])
						printf("Got marker %.*s with id %d at position %d\n", marker.markerName[0], (char *) &marker.markerName[1], (int) marker.id, (int) marker.position);
                    }
				}
				
				break;

			// Handle Sound Data chunk
			case SoundDataID:
				sound_data_chunk_scanned = true;

				//	Read in the first part of the sound data chunk
				num_bytes_read = num_bytes_requested = source_buffer.chunk_header.ckSize;
				
				if (num_bytes_requested > (sizeof(SoundDataChunk) - sizeof(ChunkHeader)))
					num_bytes_read = num_bytes_requested = (sizeof(SoundDataChunk) - sizeof(ChunkHeader));
				
				if (num_bytes_requested < (sizeof(SoundDataChunk) - sizeof(ChunkHeader)))
				{
					printf("ParseAIFFSoundFile: common chunk is too small\n");
					return -1;
				}
				
				if ((MyOSErr = fileRef.Read(&num_bytes_read, source_buffer.generic_chunk.chunk_data)) != 0)
				{
					printf("ParseAIFFSoundFile: failed FSRead of sound data chunk balance (%d)\n", MyOSErr);
					return MyOSErr;
				}
				
				source_buffer.sound_data_chunk.offset    = CFSwapInt32HostToBig(source_buffer.sound_data_chunk.offset);
				source_buffer.sound_data_chunk.blockSize = CFSwapInt32HostToBig(source_buffer.sound_data_chunk.blockSize);
				
				if (num_bytes_read != num_bytes_requested)
				{
					printf("ParseAIFFSoundFile: missing sound data chunk data\n");
					return -1;
				}
								
				// Publish info
				// Note: SourceFPos = points past ChunkHeader
				descriptor.start_pos_in_file    = (int) (SourceFPos + sizeof(SoundDataChunk) - sizeof(ChunkHeader) + source_buffer.sound_data_chunk.offset);
				descriptor.byte_len_in_file     = (int) (source_buffer.chunk_header.ckSize + sizeof(ChunkHeader) - sizeof(SoundDataChunk) - source_buffer.sound_data_chunk.offset);
				descriptor.bytes_need_swizzling = false;
				
				//	print out the goods
				if (0)
				{
					cout	<< "\tthere are " << descriptor.byte_len_in_file << " bytes of signal data starting at FPos " << descriptor.start_pos_in_file << endl
							<< "\toffset = " << source_buffer.sound_data_chunk.offset << endl;
				}
				
				break;


			// Handle synclavier sound file header chunk
			case kSynclCreatorCode:
				sncl_chunk_scanned = true;
				
				// Check length
				if (source_buffer.chunk_header.ckSize != sizeof(SynclSFHeader))
				{
					printf("ParseAIFFSoundFile: SNCL chunk is too small\n");
					return -1;
				}

				// Assume can read entire chunk; but limit to max size of buffer
				num_bytes_read = num_bytes_requested = sizeof(SynclSFHeader);
				
				if ((MyOSErr = fileRef.Read(&num_bytes_read, &sf_header.mData)) != 0)
				{
					printf("ParseAIFFSoundFile: failed FSRead of SNCL chunk balance (%d)\n", MyOSErr);
					return MyOSErr;
				}
				
				SwapBytesSynclSFHeader(&sf_header.mData);

				if (num_bytes_read != num_bytes_requested)
				{
					printf("ParseAIFFSoundFile: missing SNCL chunk data\n");
					return -1;
				}

				break;
#if 0
			case InstrumentID:
				num_bytes_read = num_bytes_requested = sizeof(InstrumentChunk) - sizeof(ChunkHeader);
				MyOSErr = fileRef.Read(&num_bytes_read, source_buffer.GenericChunk.chunk_data);
				// will need to swap
				if ((MyOSErr != noErr) && (MyOSErr != eofErr)) return MyOSErr;
				if (num_bytes_read != num_bytes_requested) {cerr << ErrorString0 << endl; DestType = 0; return MyOSErr;}	//	error not flagged in OSErr
				SourceFPos += num_bytes_read;
				//	print out the goods
				cout << "\tdetune = " << (short)source_buffer.InstrumentChunk.detune << endl
					  << "\tstartloop marker id = " << source_buffer.InstrumentChunk.sustainLoop.beginLoop << endl
					  << "\tendloop   marker id = " << source_buffer.InstrumentChunk.sustainLoop.endLoop	<< endl
					  << "\tloop play mode      = ";
				switch (source_buffer.InstrumentChunk.sustainLoop.playMode)
				{
					case NoLooping:					cout << "NoLooping";				break;
					case ForwardLooping:			cout << "ForwardLooping";			break;
					case ForwardBackwardLooping:	cout << "ForwardBackwardLooping";	break;
					default:						cout << source_buffer.InstrumentChunk.sustainLoop.playMode << "(unrecognized)";
				}
				cout << endl;
				break;

			case CommentID:
				//	first just read in the number of comments
				//	note: S/Link places the Synclavier soundfile caption in the first comment and places all of the
				// optical categories in the second comment separated by newline characters (i.e., '\n' = chr(10) decimal)
				num_bytes_read = num_bytes_requested = sizeof(source_buffer.CommentsChunk.numComments);
				MyOSErr = fileRef.Read(&num_bytes_read, source_buffer.GenericChunk.chunk_data);
				// will need to swap
				if ((MyOSErr != noErr) && (MyOSErr != eofErr)) return MyOSErr;
				if (num_bytes_read != num_bytes_requested) {cerr << ErrorString0 << endl; DestType = 0; return MyOSErr;}	//	error not flagged in OSErr
				SourceFPos += num_bytes_read;
				if (DEBUG) cout << "\t\tnumComments = " << source_buffer.CommentsChunk.numComments << endl;

				//	do while comments exist
				for (; source_buffer.CommentsChunk.numComments != 0; source_buffer.CommentsChunk.numComments--)
				{
					//	read in the first three members of the next comments[] member
					num_bytes_read = num_bytes_requested = sizeof(source_buffer.CommentsChunk.comments->timeStamp) +
						sizeof(source_buffer.CommentsChunk.comments->marker) + sizeof(source_buffer.CommentsChunk.comments->count);
					MyOSErr = fileRef.Read(&num_bytes_read, source_buffer.CommentsChunk.comments);
					// will need to swap
					if ((MyOSErr != noErr) && (MyOSErr != eofErr)) return MyOSErr;
					if (num_bytes_read != num_bytes_requested) {cerr << ErrorString0 << endl; DestType = 0; return MyOSErr;}	//	error not flagged in OSErr
					SourceFPos += num_bytes_read;
					//	print out the goods
					cout << "\tmarker id " << source_buffer.CommentsChunk.comments->marker << " comment = \"";

					// since the comment string can be any size up to 64K characters, we'll have to visit dynamic allocation land
					try
					{
						//	pad to even bytes
						num_bytes_read = num_bytes_requested = source_buffer.CommentsChunk.comments->count + (source_buffer.CommentsChunk.comments->count & 1);
						char *string_ptr = new char[num_bytes_read];
						//	read the entire string into memory
						MyOSErr = fileRef.Read(&num_bytes_read, string_ptr);
						// will need to swap
						if ((MyOSErr != noErr) && (MyOSErr != eofErr)) {delete []string_ptr; return MyOSErr;}
						if (num_bytes_read != num_bytes_requested) {cerr << ErrorString0 << endl; delete []string_ptr; DestType = 0; return MyOSErr;}	//	error not flagged in OSErr
						SourceFPos += num_bytes_read;
						for (unsigned short i = 0; i != source_buffer.CommentsChunk.comments->count; i++)
						{
							if (string_ptr[i] == '\n')	cout << endl << "\t                       ";
							else								cout << string_ptr[i];
						}
						delete []string_ptr;
					}
					catch (bad_alloc bummer)
					{
						cerr << "\tCOULDN'T ALLOCATE MEMORY FOR STRING" << endl;							
						//	advance the pointers as if we already printed this string (pad to even byte)
						SourceFPos += num_bytes_read;
						MyOSErr = SetFPos(dataRefnum, fsFromMark, num_bytes_read);
						if ((MyOSErr != noErr) && (MyOSErr != eofErr)) return MyOSErr;
					}
					cout << "\"" << endl;
				}	//	end of do while comments exist
				break;
#endif
		}
		
		// Detect bogus scan due to incorrect chunk length at end of data file
		if (source_buffer.chunk_header.ckSize < 0)
			break;

		// Advance over chunk
		SourceFPos += source_buffer.chunk_header.ckSize;

		if ((MyOSErr = fileRef.Seek(SourceFPos)) != 0)
			break;
	}
	
	if (!common_chunk_scanned || !sound_data_chunk_scanned)
	{
		printf("ParseAIFFSoundFile: missing common or sound data chunk descriptor\n");
		return -1;
	}

	if (0)
	{
		cout	<< "\tbits_per_sample   = " << descriptor.bits_per_sample << endl
				<< "\tsamples_per_frame = " << descriptor.samples_per_frame << endl
				<< "\tframes_per_file   = " << descriptor.frames_per_file << endl
				<< "\tframes_per_second = " << descriptor.frames_per_second << endl;
		cout	<< "\tthere are " << descriptor.byte_len_in_file << " bytes of signal data starting at FPos " << descriptor.start_pos_in_file << endl;
	}

	return (noErr);
}


// ---------------------------------------------------------------------------
//	╔ ParseWAVESoundFile(short dataRefnum, short resourceRefnum, SynclSFHeader& sf_header)
// ---------------------------------------------------------------------------
//	Handles WAVE files

OSErr	ParseWAVESoundFile(CSynclavierFileReference& fileRef, AudioDataDescriptor& descriptor, SynclSFHeader& sf_header_struct)
{
	CSynclSFHeader		sf_header(sf_header_struct);		// Create CSynclSFHeader with which to manipulate struct
	WAVEContainerChunk	MyContainerChunk;					//	12 bytes
	OSErr				MyOSErr	= noErr;
	SInt32				num_bytes_requested = 0, num_bytes_read = 0;
	SInt64				end_of_file	= 0;
    SInt64				DebugFPos	= 0;

	Boolean				format_chunk_scanned = false;
	Boolean				data_chunk_scanned   = false;
	Boolean				fact_chunk_scanned   = false;
	Boolean				sncl_chunk_scanned   = false;

	//	init output struct
	sf_header.init();
	memset(&descriptor, 0, sizeof(descriptor));

	//	get and verify the 'RIFF' container chunk
	num_bytes_read = num_bytes_requested = sizeof(WAVEContainerChunk);
	
	if ((MyOSErr = fileRef.Seek(0)) != 0)
	{
		printf("ParseWAVESoundFile: failed SetFPos (%d)\n", MyOSErr);
		return MyOSErr;
	}
	
	if ((MyOSErr = fileRef.Read(&num_bytes_read, &MyContainerChunk)) != 0)
	{
		printf("ParseWAVESoundFile: failed FSRead (%d)\n", MyOSErr);
		return MyOSErr;
	}

	if (num_bytes_read != num_bytes_requested)
	{
		printf("ParseWAVESoundFile: failed FSRead (%d)\n", MyOSErr);
		return MyOSErr;
	}
	
	// Tweak
	MyContainerChunk.ckID     = CFSwapInt32HostToBig(MyContainerChunk.ckID);
	MyContainerChunk.ckSize   = EndianU32_LtoN(MyContainerChunk.ckSize);
	MyContainerChunk.formType = CFSwapInt32HostToBig(MyContainerChunk.formType);

	//	check for errors not flagged in OSErr
	if ((MyContainerChunk.ckID     != WAVEFORMID)
	||  (MyContainerChunk.formType != WAVEID    ))
	{
		printf("ParseWAVESoundFile: file does not appear to be WAVE\n");
		return -1;
	}

	// Check file length	
	end_of_file = MyContainerChunk.ckSize + sizeof(WAVEChunkHeader);
    DebugFPos   = fileRef.Size();

	if (DebugFPos < end_of_file)
	{
		if (DebugFPos < sizeof(WAVEChunkHeader) + 100)
		{
			printf("ParseWAVESoundFile: file is too short to be WAVE\n");
			return -1;
		}
		
		MyContainerChunk.ckSize = (SInt32) (DebugFPos - sizeof(WAVEChunkHeader));
	}
	
	//	restore FPos
	SInt64 SourceFPos = sizeof(WAVEContainerChunk);	//	this is the position of the first local chunk in the 'RIFF' chunk
	
	if ((MyOSErr = fileRef.Seek(SourceFPos)) != 0)
	{
		printf("ParseWAVESoundFile: failed SetFPos (%d)\n", MyOSErr);
		return MyOSErr;
	}
	
	// Process chunks in the file
	while (SourceFPos < end_of_file)
	{
		ChunkBuffer	source_buffer;

		//	get the header of the next chunk
		num_bytes_read = num_bytes_requested = sizeof(WAVEChunkHeader);

		if ((MyOSErr = fileRef.Read(&num_bytes_read, &source_buffer)) != 0)
		{
			if (format_chunk_scanned && data_chunk_scanned)		// avoid bombout on extra 2 bytes in file
				break;
				
			printf("ParseWAVESoundFile: failed FSRead of chunk (%d)\n", MyOSErr);
			return MyOSErr;
		}
		
		if (num_bytes_read != num_bytes_requested)
		{
			printf("ParseWAVESoundFile: missing chunk data\n");
			return -1;
		}
		
		// Tweak
		source_buffer.wave_chunk_header.ckID   = CFSwapInt32HostToBig(source_buffer.wave_chunk_header.ckID);
		source_buffer.wave_chunk_header.ckSize = EndianU32_LtoN(source_buffer.wave_chunk_header.ckSize);

		// Advance over chunk header
		SourceFPos += num_bytes_read;
		
		//	process based on what kind of chunk it is
		switch (source_buffer.wave_chunk_header.ckID)
		{
			case FormatID:
			{
				format_chunk_scanned = true;
		
				// Assume can read entire chunk; but limit to max size of buffer
				num_bytes_read = num_bytes_requested = source_buffer.wave_chunk_header.ckSize;
				
				if (num_bytes_requested > (sizeof(source_buffer) - sizeof(WAVEChunkHeader)))
					num_bytes_read = num_bytes_requested = (sizeof(source_buffer) - sizeof(WAVEChunkHeader));
					
				if (num_bytes_requested < (sizeof(fmtChunk) - sizeof(WAVEChunkHeader)))
				{
					printf("ParseWAVESoundFile: fmt chunk is too small\n");
					return -1;
				}
				
				if ((MyOSErr = fileRef.Read(&num_bytes_read, source_buffer.wave_generic_chunk.chunk_data)) != 0)
				{
					printf("ParseWAVESoundFile: failed FSRead of fmt chunk balance (%d)\n", MyOSErr);
					return MyOSErr;
				}
				
				if (num_bytes_read != num_bytes_requested)
				{
					printf("ParseWAVESoundFile: missing fmt chunk data\n");
					return -1;
				}
				
				// Tweak
				fmtChunk& chunk = source_buffer.fmt_chunk;
				
				chunk.wFormatTag       = 1;
				chunk.wChannels		   = EndianU16_LtoN(chunk.wChannels);
				chunk.dwSamplesPerSec  = EndianU32_LtoN(chunk.dwSamplesPerSec);
				chunk.dwAvgBytesPerSec = EndianU32_LtoN(chunk.dwAvgBytesPerSec);
				chunk.wBlockAlign      = EndianU16_LtoN(chunk.wBlockAlign);
				chunk.wBitsPerSample   = EndianU16_LtoN(chunk.wBitsPerSample);
			
				// Extract information
				descriptor.bits_per_sample   = chunk.wBitsPerSample;
				descriptor.samples_per_frame = chunk.wChannels;
				descriptor.frames_per_second = (double) chunk.dwSamplesPerSec;
				
				// Check a couple basic things
				if (descriptor.bits_per_sample != 8 && descriptor.bits_per_sample != 16 && descriptor.bits_per_sample != 24 &&  descriptor.bits_per_sample != 32)
				{
					printf("ParseWAVESoundFile: file contains other than 8-bit, 16-bit, 24-bit or 32-bit audio data\n");
					return -1;
				}
				
				if (descriptor.samples_per_frame != 1 && descriptor.samples_per_frame != 2)
				{
					printf("ParseWAVESoundFile: file contains more than 2 channels of audio data\n");
					return -1;
				}
			
				//	print out
				if (0)
				{
					cout	<< "\tbits_per_sample   = " << descriptor.bits_per_sample << endl
							<< "\tsamples_per_frame = " << descriptor.samples_per_frame << endl
							<< "\tframes_per_file   = " << descriptor.frames_per_file << endl
							<< "\tframes_per_second = " << descriptor.frames_per_second << endl;
				}

				break;
			}
			
			case WAVEDataID:
			{
				data_chunk_scanned = true;
	
				waveDataChunk& chunk = source_buffer.wave_data_chunk;
				
				// If no fact chunk scanned, derive frames-per-file from data chunk length (provided we know channel count)
				if (format_chunk_scanned && !fact_chunk_scanned)
					descriptor.frames_per_file = chunk.ckSize / (descriptor.samples_per_frame * descriptor.bits_per_sample / 8);
				
				descriptor.start_pos_in_file    = (int) SourceFPos;
				descriptor.byte_len_in_file     = chunk.ckSize;
				descriptor.bytes_need_swizzling = true;
				break;
			}
			
			case FactID:
			{
				fact_chunk_scanned = true;
		
				num_bytes_read = num_bytes_requested = source_buffer.wave_chunk_header.ckSize;
				
				if (num_bytes_requested > (sizeof(source_buffer) - sizeof(WAVEChunkHeader)))
					num_bytes_read = num_bytes_requested = (sizeof(source_buffer) - sizeof(WAVEChunkHeader));
					
				if (num_bytes_requested < (sizeof(factChunk) - sizeof(WAVEChunkHeader)))
				{
					printf("ParseWAVESoundFile: fact chunk is too small\n");
					return -1;
				}
				
				if ((MyOSErr = fileRef.Read(&num_bytes_read, source_buffer.wave_generic_chunk.chunk_data)) != 0)
				{
					printf("ParseWAVESoundFile: failed FSRead of fact chunk balance (%d)\n", MyOSErr);
					return MyOSErr;
				}
				
				if (num_bytes_read != num_bytes_requested)
				{
					printf("ParseWAVESoundFile: missing fact chunk data\n");
					return -1;
				}
				
				// Tweak
				factChunk& chunk = source_buffer.fact_chunk;
				
				chunk.dwFileSize  = EndianU32_LtoN(chunk.dwFileSize);
			
				// Extracdt information
				descriptor.frames_per_file = chunk.dwFileSize;
				
				break;
			}
		
			// Handle synclavier sound file header chunk
			case kSynclCreatorCode:
				sncl_chunk_scanned = true;
				
				// Check length
				if (source_buffer.wave_chunk_header.ckSize != sizeof(SynclSFHeader))
				{
					printf("ParseWAVESoundFile: SNCL chunk is too small\n");
					return -1;
				}

				// Assume can read entire chunk; but limit to max size of buffer
				num_bytes_read = num_bytes_requested = sizeof(SynclSFHeader);
				
				if ((MyOSErr = fileRef.Read(&num_bytes_read, &sf_header.mData)) != 0)
				{
					printf("ParseWAVESoundFile: failed FSRead of SNCL chunk balance (%d)\n", MyOSErr);
					return MyOSErr;
				}
				
				SwapBytesSynclSFHeader(&sf_header.mData);
				
				if (num_bytes_read != num_bytes_requested)
				{
					printf("ParseWAVESoundFile: missing SNCL chunk data\n");
					return -1;
				}

				break;
		}
		
		// Detect bogus scan due to incorrect chunk length at end of data file
		if (source_buffer.wave_chunk_header.ckSize < 0)
			break;
			
		// Round up chunk size if not at end of file
		SourceFPos += source_buffer.wave_chunk_header.ckSize;
		
		if (((SourceFPos & 1) != 0)
		&&  (SourceFPos < end_of_file))
			SourceFPos++;
			
		if ((MyOSErr = fileRef.Seek(SourceFPos)) != 0)
			break;
	}
	
	if (!format_chunk_scanned || !data_chunk_scanned)
	{
		printf("ParseWAVESoundFile: missing fmt or data chunk descriptor\n");
		return -1;
	}
	
	if (0)
	{
		cout	<< "\tbits_per_sample   = " << descriptor.bits_per_sample << endl
				<< "\tsamples_per_frame = " << descriptor.samples_per_frame << endl
				<< "\tframes_per_file   = " << descriptor.frames_per_file << endl
				<< "\tframes_per_second = " << descriptor.frames_per_second << endl;
		cout	<< "\tthere are " << descriptor.byte_len_in_file << " bytes of signal data starting at FPos " << descriptor.start_pos_in_file << endl;
	}

	// Construct sound file header (if not one scanned)
	if (!sncl_chunk_scanned)
	{
		sf_header.mData.stereo = descriptor.samples_per_frame - 1;
		sf_header.set_rates(descriptor.frames_per_second);

		//	translate numSampleFrames into data_end, mark_end and the symbol times for "$" and "END"
		SynclSFTime	time_of_last_sample = sf_header.sampleframes_to_time(descriptor.frames_per_file - 1);
		
		sf_header.mDataEnd.assign(time_of_last_sample);
		sf_header.mMarkEnd.assign(time_of_last_sample);

		CSynclSFSymbol	symbol_0(sf_header.mData.symbols[0]);
		CSynclSFSymbol	symbol_1(sf_header.mData.symbols[1]);
		CSynclSFSymbol	symbol_2(sf_header.mData.symbols[2]);
		CSynclSFSymbol	symbol_3(sf_header.mData.symbols[3]);

		symbol_2.place_time(time_of_last_sample);
		symbol_3.place_time(time_of_last_sample);

		//	translate valid_data and total_data
		unsigned int	num_samples = descriptor.frames_per_file;
		
		if ( descriptor.samples_per_frame == 2)
			num_samples <<= 1;
			
		sf_header.mValidData.words_to_index(num_samples);
		
		num_samples = (descriptor.frames_per_file + 255) & 0xFFFFFF00;
		
		if ( descriptor.samples_per_frame == 2)
			num_samples <<= 1;
			
		sf_header.mTotalData.words_to_index(num_samples);
	}
	
	return noErr;
}


// ---------------------------------------------------------------------------
//	╔ CreateAIFFSoundFile(short dataRefnum, short resourceRefnum, SynclSFHeader& sf_header)
// ---------------------------------------------------------------------------

long	ComputeAIFFSoundFileSize(SynclSFHeader& , AudioDataDescriptor& descriptor)
{
	long	bytes_of_chunks = sizeof(ContainerChunk)
	                        + sizeof(CommonChunk)
	                        + sizeof(SynclavierChunk)
	                        + sizeof(SoundDataChunk);
	                        
	long	bytes_of_data   = descriptor.byte_len_in_file;
	
	long	bytes_in_all    = bytes_of_chunks + bytes_of_data;
	
	//	Return where the audio data should go
	descriptor.start_pos_in_file = (int) bytes_of_chunks;
	
	return (bytes_in_all);
}

OSErr	CreateAIFFSoundFile(CSynclavierFileReference& fileRef, AudioDataDescriptor& descriptor, SynclSFHeader& sf_header_struct)
{
	CSynclSFHeader	sf_header(sf_header_struct);		// Create CSynclSFHeader with which to manipulate struct
	OSErr			MyOSErr	= noErr;
	SInt32			num_bytes_requested = 0, num_bytes_written = 0;
	ChunkBuffer		dest_buffer;

	//	Chunks written:
	//		ContainerChunk
	//		CommonID
	//		kSynclCreatorCode
	//		SoundDataID
	//		... followed by audio data	

	long	bytes_of_chunks = sizeof(ContainerChunk)
	                        + sizeof(CommonChunk)
	                        + sizeof(SynclavierChunk)
	                        + sizeof(SoundDataChunk);
	                        
	long	bytes_of_data   = descriptor.byte_len_in_file;
	
	long	bytes_in_all    = bytes_of_chunks + bytes_of_data;
	 
	// 	Position to start of file for grins                     
	if ((MyOSErr = fileRef.Seek(0)) != 0)
	{
		printf("CreateAIFFSoundFile: failed SetFPos (%d)\n", MyOSErr);
		return MyOSErr;
	}
	
	//	Fill in and write the ContainerChunk
	ContainerChunk& containerChunk = dest_buffer.container_chunk;
	
	memset(&containerChunk, 0, sizeof(ContainerChunk));
	
	containerChunk.ckID     = FORMID;
	containerChunk.ckSize   = (int) (bytes_in_all - sizeof(ChunkHeader));
	containerChunk.formType = AIFFID;
	
	num_bytes_written = num_bytes_requested = sizeof(containerChunk);
	
	containerChunk.ckID     = CFSwapInt32HostToBig(containerChunk.ckID);
	containerChunk.ckSize   = CFSwapInt32HostToBig(containerChunk.ckSize);
	containerChunk.formType = CFSwapInt32HostToBig(containerChunk.formType);
	
	if ((MyOSErr = fileRef.Write(&num_bytes_written, &containerChunk)) != 0)
	{
		printf("CreateAIFFSoundFile: failed FSWrite of container chunk (%d)\n", MyOSErr);
		return MyOSErr;
	}

	if (num_bytes_written != num_bytes_requested)
	{
		printf("CreateAIFFSoundFile: failed FSWrite of container chunk (%d)\n", MyOSErr);
		return MyOSErr;
	}

	//	Fill in and write the CommonChunk
	CommonChunk& commonChunk = dest_buffer.common_chunk;
	
	memset(&commonChunk, 0, sizeof(commonChunk));
	
	commonChunk.ckID            = CommonID;
	commonChunk.ckSize          = sizeof(commonChunk) - sizeof(ChunkHeader);
	commonChunk.numChannels     = descriptor.samples_per_frame;
	commonChunk.numSampleFrames = descriptor.frames_per_file;
	commonChunk.sampleSize      = 16;
	
	#if TARGET_CPU_68K
		commonChunk.sampleRate = (extended80) descriptor.frames_per_second;
	#else
		dtox80(&descriptor.frames_per_second, &commonChunk.sampleRate);
	#endif

	num_bytes_written = num_bytes_requested = sizeof(commonChunk);
	
	commonChunk.ckID            = CFSwapInt32HostToBig(commonChunk.ckID);
	commonChunk.ckSize          = CFSwapInt32HostToBig(commonChunk.ckSize);
	commonChunk.numChannels     = CFSwapInt16HostToBig(commonChunk.numChannels);
	commonChunk.numSampleFrames = CFSwapInt32HostToBig(commonChunk.numSampleFrames);
	commonChunk.sampleSize      = CFSwapInt16HostToBig(commonChunk.sampleSize);
	
	SwapBytesExtended80(&commonChunk.sampleRate);
	
	if ((MyOSErr = fileRef.Write(&num_bytes_written, &commonChunk)) != 0)
	{
		printf("CreateAIFFSoundFile: failed FSWrite of common chunk (%d)\n", MyOSErr);
		return MyOSErr;
	}

	if (num_bytes_written != num_bytes_requested)
	{
		printf("CreateAIFFSoundFile: failed FSWrite of common chunk (%d)\n", MyOSErr);
		return MyOSErr;
	}

	//	Fill in and write the Synclavier chunk
	SynclavierChunk& synclavierChunk = dest_buffer.synclavier_chunk;
	
	memset(&synclavierChunk, 0, sizeof(synclavierChunk));
	
	synclavierChunk.ckID     = kSynclCreatorCode;
	synclavierChunk.ckSize   = sizeof(synclavierChunk) - sizeof(ChunkHeader);
	synclavierChunk.sfHeader = sf_header.mData;
	
	num_bytes_written = num_bytes_requested = sizeof(synclavierChunk);
	
	synclavierChunk.ckID   = CFSwapInt32HostToBig(synclavierChunk.ckID);
	synclavierChunk.ckSize = CFSwapInt32HostToBig(synclavierChunk.ckSize);
	
	SwapBytesSynclSFHeader(&synclavierChunk.sfHeader);
	
	if ((MyOSErr = fileRef.Write(&num_bytes_written, &synclavierChunk)) != 0)
	{
		printf("CreateAIFFSoundFile: failed FSWrite of synclavier chunk (%d)\n", MyOSErr);
		return MyOSErr;
	}

	if (num_bytes_written != num_bytes_requested)
	{
		printf("CreateAIFFSoundFile: failed FSWrite of synclavier chunk (%d)\n", MyOSErr);
		return MyOSErr;
	}

	//	Fill in and write the SoundData chunk
	SoundDataChunk& soundDataChunk = dest_buffer.sound_data_chunk;
	
	memset(&soundDataChunk, 0, sizeof(soundDataChunk));
	
	soundDataChunk.ckID      = SoundDataID;
	soundDataChunk.ckSize    = (SInt32) (sizeof(soundDataChunk) + bytes_of_data - sizeof(ChunkHeader));
	soundDataChunk.offset    = 0;
	soundDataChunk.blockSize = 512;
	
	num_bytes_written = num_bytes_requested = sizeof(soundDataChunk);
	
	soundDataChunk.ckID      = CFSwapInt32HostToBig(soundDataChunk.ckID);
	soundDataChunk.ckSize    = CFSwapInt32HostToBig(soundDataChunk.ckSize);
	soundDataChunk.offset    = CFSwapInt32HostToBig(soundDataChunk.offset);
	soundDataChunk.blockSize = CFSwapInt32HostToBig(soundDataChunk.blockSize);

	if ((MyOSErr = fileRef.Write(&num_bytes_written, &soundDataChunk)) != 0)
	{
		printf("CreateAIFFSoundFile: failed FSWrite of sound data chunk (%d)\n", MyOSErr);
		return MyOSErr;
	}

	if (num_bytes_written != num_bytes_requested)
	{
		printf("CreateAIFFSoundFile: failed FSWrite of sound data chunk (%d)\n", MyOSErr);
		return MyOSErr;
	}

	return (noErr);
}


// ---------------------------------------------------------------------------
//	╔ CreateWAVESoundFile(short dataRefnum, short resourceRefnum, SynclSFHeader& sf_header)
// ---------------------------------------------------------------------------

long	ComputeWAVESoundFileSize(SynclSFHeader& , AudioDataDescriptor& descriptor)
{
	long	bytes_of_chunks = sizeof(WAVEContainerChunk)
	                        + sizeof(fmtChunk)
	                        + sizeof(WAVESynclavierChunk)
	                        + sizeof(waveDataChunk);
	                        
	long	bytes_of_data   = descriptor.byte_len_in_file;
	
	long	bytes_in_all    = bytes_of_chunks + bytes_of_data;
	
	//	Return where the audio data should go
	descriptor.start_pos_in_file = (int) bytes_of_chunks;
	
	return (bytes_in_all);
}

OSErr	CreateWAVESoundFile(class CSynclavierFileReference& fileRef, AudioDataDescriptor& descriptor, SynclSFHeader& sf_header_struct)
{
	CSynclSFHeader	sf_header(sf_header_struct);		// Create CSynclSFHeader with which to manipulate struct
	OSErr			MyOSErr	= noErr;
	SInt32			num_bytes_requested = 0, num_bytes_written = 0;
	ChunkBuffer		dest_buffer;

	//	Chunks written:
	//		WAVEContainerChunk
	//		fmtChunk
	//		WAVESynclavierChunk
	//		waveDataChunk
	//		... followed by audio data	

	long	bytes_of_chunks = sizeof(WAVEContainerChunk)
	                        + sizeof(fmtChunk)
	                        + sizeof(WAVESynclavierChunk)
	                        + sizeof(waveDataChunk);
	                        
	long	bytes_of_data   = descriptor.byte_len_in_file;
	
	long	bytes_in_all    = bytes_of_chunks + bytes_of_data;
	 
	// 	Position to start of file for grins                     
	if ((MyOSErr = fileRef.Seek(0)) != 0)
	{
		printf("CreateWAVESoundFile: failed SetFPos (%d)\n", MyOSErr);
		return MyOSErr;
	}
	
	//	Fill in and write the ContainerChunk
	WAVEContainerChunk& containerChunk = dest_buffer.wave_container_chunk;
	
	memset(&containerChunk, 0, sizeof(WAVEContainerChunk));
	
	containerChunk.ckID     = WAVEFORMID;
	containerChunk.ckSize   = (SInt32) (bytes_in_all - sizeof(WAVEChunkHeader));
	containerChunk.formType = WAVEID;
	
	// Tweak
	containerChunk.ckID     = CFSwapInt32HostToBig(containerChunk.ckID);
	containerChunk.ckSize   = EndianU32_LtoN(containerChunk.ckSize);
	containerChunk.formType = CFSwapInt32HostToBig(containerChunk.formType);

	// Write
	num_bytes_written = num_bytes_requested = sizeof(containerChunk);
	
	if ((MyOSErr = fileRef.Write(&num_bytes_written, (void *) &containerChunk)) != 0)
	{
		printf("CreateWAVESoundFile: failed FSWrite of container chunk (%d)\n", MyOSErr);
		return MyOSErr;
	}

	if (num_bytes_written != num_bytes_requested)
	{
		printf("CreateWAVESoundFile: failed FSWrite of container chunk (%d)\n", MyOSErr);
		return MyOSErr;
	}

	//	Fill in and write the fmtChunk
	fmtChunk& fmt_chunk = dest_buffer.fmt_chunk;
	
	memset(&fmt_chunk, 0, sizeof(fmt_chunk));
	
	fmt_chunk.ckID             = FormatID;
	fmt_chunk.ckSize           = sizeof(fmt_chunk) - sizeof(WAVEChunkHeader);
	fmt_chunk.wFormatTag       = 1;
	fmt_chunk.wChannels        = descriptor.samples_per_frame;
	fmt_chunk.dwSamplesPerSec  = (SInt32) descriptor.frames_per_second;
	fmt_chunk.dwAvgBytesPerSec = fmt_chunk.dwSamplesPerSec * (descriptor.samples_per_frame * descriptor.bits_per_sample / 8);
	fmt_chunk.wBlockAlign      = descriptor.samples_per_frame * descriptor.bits_per_sample / 8;
	fmt_chunk.wBitsPerSample   = descriptor.bits_per_sample;
	
	// Tweak
	fmt_chunk.ckID			   = CFSwapInt32HostToBig(fmt_chunk.ckID);
	fmt_chunk.ckSize   		   = EndianU32_LtoN(fmt_chunk.ckSize);
	fmt_chunk.wFormatTag	   = EndianU16_LtoN(fmt_chunk.wFormatTag);
	fmt_chunk.wChannels		   = EndianU16_LtoN(fmt_chunk.wChannels);
	fmt_chunk.dwSamplesPerSec  = EndianU32_LtoN(fmt_chunk.dwSamplesPerSec);
	fmt_chunk.dwAvgBytesPerSec = EndianU32_LtoN(fmt_chunk.dwAvgBytesPerSec);
	fmt_chunk.wBlockAlign      = EndianU16_LtoN(fmt_chunk.wBlockAlign);
	fmt_chunk.wBitsPerSample   = EndianU16_LtoN(fmt_chunk.wBitsPerSample);	

	// Write
	num_bytes_written = num_bytes_requested = sizeof(fmt_chunk);
	
	if ((MyOSErr = fileRef.Write(&num_bytes_written, &fmt_chunk)) != 0)
	{
		printf("CreateWAVESoundFile: failed FSWrite of common chunk (%d)\n", MyOSErr);
		return MyOSErr;
	}

	if (num_bytes_written != num_bytes_requested)
	{
		printf("CreateWAVESoundFile: failed FSWrite of common chunk (%d)\n", MyOSErr);
		return MyOSErr;
	}

	//	Fill in and write the Synclavier chunk
	WAVESynclavierChunk& synclavierChunk = dest_buffer.wave_synclavier_chunk;
	
	memset(&synclavierChunk, 0, sizeof(synclavierChunk));
	
	synclavierChunk.ckID     = kSynclCreatorCode;
	synclavierChunk.ckSize   = sizeof(synclavierChunk) - sizeof(ChunkHeader);
	synclavierChunk.sfHeader = sf_header.mData;
	
	// Tweak
	synclavierChunk.ckID     = CFSwapInt32HostToBig(synclavierChunk.ckID);
	synclavierChunk.ckSize   = EndianU32_LtoN(synclavierChunk.ckSize);
	SwapBytesSynclSFHeader(&synclavierChunk.sfHeader);	// Note: SynclSFHeader is written in big-endian style even though most WAV file entities are little-endian
	
	// Write
	num_bytes_written = num_bytes_requested = sizeof(synclavierChunk);
	
	if ((MyOSErr = fileRef.Write(&num_bytes_written, &synclavierChunk)) != 0)
	{
		printf("CreateWAVESoundFile: failed FSWrite of synclavier chunk (%d)\n", MyOSErr);
		return MyOSErr;
	}

	if (num_bytes_written != num_bytes_requested)
	{
		printf("CreateWAVESoundFile: failed FSWrite of synclavier chunk (%d)\n", MyOSErr);
		return MyOSErr;
	}

	//	Fill in and write the SoundData chunk
	waveDataChunk& dataChunk = dest_buffer.wave_data_chunk;
	
	memset(&dataChunk, 0, sizeof(dataChunk));
	
	dataChunk.ckID      = WAVEDataID;
	dataChunk.ckSize    = (SInt32) (sizeof(dataChunk) + bytes_of_data - sizeof(ChunkHeader));
	
	// Tweak
	dataChunk.ckID   = CFSwapInt32HostToBig(dataChunk.ckID);
	dataChunk.ckSize = EndianU32_LtoN(dataChunk.ckSize);

	// Write
	num_bytes_written = num_bytes_requested = sizeof(dataChunk);
	
	if ((MyOSErr = fileRef.Write(&num_bytes_written, &dataChunk)) != 0)
	{
		printf("CreateWAVESoundFile: failed FSWrite of sound data chunk (%d)\n", MyOSErr);
		return MyOSErr;
	}

	if (num_bytes_written != num_bytes_requested)
	{
		printf("CreateWAVESoundFile: failed FSWrite of sound data chunk (%d)\n", MyOSErr);
		return MyOSErr;
	}

	return (noErr);
}
