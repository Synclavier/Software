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
#include "SoundDesigner.h"
#include "SoundfileTranslators.h"
#include "CSynclavierSoundFileHeader.h"
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
	unsigned long		ckID;
	long 				ckSize;
	SynclSFHeader		sfHeader;

} SynclavierChunk;

typedef struct WAVEGenericChunk {
	WAVEChunkHeader		chunk_header;
	Byte				chunk_data[1];

} WAVEGenericChunk;

typedef struct WAVESynclavierChunk {
	unsigned long		ckID;
	long 				ckSize;
	SynclSFHeader		sfHeader;

} WAVESynclavierChunk;

//	Handy struct to hold up to 64 markers...
typedef struct BigMarkerChunk {
	unsigned long 					ckID;
	long 							ckSize;
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
	
void SwapBytesSynclSFHeader(SynclSFHeader* it)
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
	// skip unused unused_053[127-53]
	// id_field stored as char

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

OSErr	ParseAIFFSoundFile(short dataRefnum, short /*resourceRefnum*/, AudioDataDescriptor& descriptor, SynclSFHeader& sf_header_struct)
{
	CSynclSFHeader	sf_header(sf_header_struct);		// Create CSynclSFHeader with which to manipulate struct
	ContainerChunk	MyContainerChunk;	//	12 bytes
	OSErr			MyOSErr	= noErr;
	SInt32			num_bytes_requested = 0, num_bytes_read = 0;
	SInt32			end_of_file = 0;
	SInt32			DebugFPos = 0;
	Boolean			common_chunk_scanned     = false;
	Boolean			sound_data_chunk_scanned = false;
	Boolean			sncl_chunk_scanned       = false;
	int				i;

	//	init output struct
	sf_header.init();
	memset(&descriptor, 0, sizeof(descriptor));

	//	get and verify the 'FORM' container chunk
	num_bytes_read = num_bytes_requested = sizeof(ContainerChunk);
	
	if ((MyOSErr = SetFPos(dataRefnum, fsFromStart, 0)) != 0)
	{
		printf("ParseAIFFSoundFile: failed SetFPos (%d)\n", MyOSErr);
		return MyOSErr;
	}
	
	if ((MyOSErr = FSRead(dataRefnum, &num_bytes_read, (void *) &MyContainerChunk)) != 0)
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
	
	if ((MyOSErr = SetFPos(dataRefnum, fsFromLEOF, 0)) != 0)
	{
		printf("ParseAIFFSoundFile: failed SetFPos to EOF (%d)\n", MyOSErr);
		return MyOSErr;
	}
	
	if ((MyOSErr = GetFPos(dataRefnum, &DebugFPos)) != 0)
	{
		printf("ParseAIFFSoundFile: failed GetFPos (%d)\n", MyOSErr);
		return MyOSErr;
	}
	
	if (DebugFPos < end_of_file)
	{
		if (DebugFPos < sizeof(ChunkHeader) + 100)
		{
			printf("ParseAIFFSoundFile: file is too short to be AIFF\n");
			return -1;
		}
		
		MyContainerChunk.ckSize = DebugFPos - sizeof(ChunkHeader);
	}

	//	restore FPos
	long SourceFPos = sizeof(ContainerChunk);	//	this is the position of the first local chunk in the 'FORM' chunk
	
	if ((MyOSErr = SetFPos(dataRefnum, fsFromStart, SourceFPos)) != 0)
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

		if ((MyOSErr = FSRead(dataRefnum, &num_bytes_read, &source_buffer)) != 0)
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
				
				if ((MyOSErr = FSRead(dataRefnum, &num_bytes_read, source_buffer.generic_chunk.chunk_data)) != 0)
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
					unsigned long	num_samples = descriptor.frames_per_file;
					
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

				if ((MyOSErr = FSRead(dataRefnum, &num_bytes_read, source_buffer.generic_chunk.chunk_data)) != 0)
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
						printf("Got marker %.*s with id %d at position %d\n", marker.markerName[0], (char *) &marker.markerName[1], (int) marker.id, (int) marker.markerName);
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
				
				if ((MyOSErr = FSRead(dataRefnum, &num_bytes_read, source_buffer.generic_chunk.chunk_data)) != 0)
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
				descriptor.start_pos_in_file    = SourceFPos + sizeof(SoundDataChunk) - sizeof(ChunkHeader) + source_buffer.sound_data_chunk.offset;
				descriptor.byte_len_in_file     = source_buffer.chunk_header.ckSize + sizeof(ChunkHeader) - sizeof(SoundDataChunk) - source_buffer.sound_data_chunk.offset;
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
				
				if ((MyOSErr = FSRead(dataRefnum, &num_bytes_read, &sf_header.mData)) != 0)
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
				MyOSErr = FSRead(dataRefnum, &num_bytes_read, source_buffer.GenericChunk.chunk_data);
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
				MyOSErr = FSRead(dataRefnum, &num_bytes_read, source_buffer.GenericChunk.chunk_data);
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
					MyOSErr = FSRead(dataRefnum, &num_bytes_read, source_buffer.CommentsChunk.comments);
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
						MyOSErr = FSRead(dataRefnum, &num_bytes_read, string_ptr);
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

		if ((MyOSErr = SetFPos(dataRefnum, fsFromStart, SourceFPos)) != 0)
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
//	╔ ParseSd2fSoundFile(short dataRefnum, short resourceRefnum, SynclSFHeader& sf_header)
// ---------------------------------------------------------------------------
//	Handles Sd2f files.

OSErr	ParseSd2fSoundFile(short dataRefnum, short resourceRefnum, AudioDataDescriptor& descriptor, SynclSFHeader& sf_header_struct)
{
	CSynclSFHeader	sf_header(sf_header_struct);		// Create CSynclSFHeader with which to manipulate struct

	OSErr			MyOSErr				= noErr;
	Handle			MyResourceHandle	= NULL;
	short			ReturnedID			= 0;
	ResType			ReturnedType		= 0;
	Boolean			sncl_chunk_scanned  = false;

	short 			MyResourceID   = 0;
	Str255			MyResourceName = "\p";

	const ResType	ResourceTypeList[] = {SD2Loops_ResType, SD2Markers_ResType, SD2DocumentData_ResType, kSynclCreatorCode};
	const int		NumResTypes = sizeof(ResourceTypeList) / sizeof(ResType);
	
	short			PriorResFile = CurResFile();

	int				i;
	
	//	Init output struct
	sf_header.init();
	memset(&descriptor, 0, sizeof(descriptor));

	//	Set up for passed res file
	if (resourceRefnum == (-1))
	{
		printf("ParseSd2fSoundFile: no resource data in file\n");
		UseResFile(PriorResFile);
		return -1;
	}

	UseResFile(resourceRefnum);
	
	//	Get the three mandatory 'STR ' resources: 'sample-size' (ID = 1000), 'sample-rate' (ID = 1001), 'channels' (ID = 1002)
	for (MyResourceID = 1000; MyResourceID < 1000+3; MyResourceID++)
	{
		MyResourceHandle = Get1Resource(STR_ResType, MyResourceID);
		
		if ((MyOSErr = ResError()) != 0)
		{
			printf("ParseSd2fSoundFile: can't read required resource %d\n", MyResourceID);
			UseResFile(PriorResFile);
			return -1;
		}
		
		if (MyResourceHandle == NULL || *MyResourceHandle == NULL)
		{
			printf("ParseSd2fSoundFile: can't read required resource %d (no memory?)\n", MyResourceID);
			UseResFile(PriorResFile);
			return -1;
		}

		GetResInfo(MyResourceHandle, &ReturnedID, &ReturnedType, MyResourceName);
		
		//	print out the goods
		if (0)
			printf("resource '%4.4s' ReturnedID '%d' ReturnedName '%.*s'\n", (char *) &ReturnedType, ReturnedID, MyResourceName[0], &MyResourceName[1]);

		//	extract the info
		StringPtr	itsData = (StringPtr) (*MyResourceHandle);
		char		itsString[256];
		char*		itsEnd;

		for (i=0; i<itsData[0]; i++)
			itsString[i] = itsData[i+1];
			
		itsString[itsData[0]] = 0;

		switch (MyResourceID)
		{
			case 1000:
				descriptor.bits_per_sample = atol((char *) itsString) * 8;
				break;
				
			case 1001:
				descriptor.frames_per_second = strtod ((char *) itsString, &itsEnd);
				break;
				
			case 1002:
				descriptor.samples_per_frame = atol((char *) itsString);
				break;
		}

		ReleaseResource(MyResourceHandle);	//	clean up
	}

	// Length of file (in sample frames) =  Length of data fork / (number of channels * sample size)
	// Length of file (in seconds      ) = (Length of data fork / (number of channels * sample size) ) / sample rate
	SInt32 file_pos_bytes;
	SInt32 bytes_per_frame = descriptor.bits_per_sample * descriptor.samples_per_frame / 8;

	if ((MyOSErr = SetFPos(dataRefnum, fsFromLEOF, 0)) != 0)
	{
		printf("ParseSd2fSoundFile: failed SetFPos to EOF (%d)\n", MyOSErr);
		UseResFile(PriorResFile);
		return MyOSErr;
	}
	
	if ((MyOSErr = GetFPos(dataRefnum, &file_pos_bytes)) != 0)
	{
		printf("ParseSd2fSoundFile: failed GetFPos (%d)\n", MyOSErr);
		UseResFile(PriorResFile);
		return MyOSErr;
	}
	
	if ((MyOSErr = SetFPos(dataRefnum, fsFromStart, 0)) != 0)
	{
		printf("ParseSd2fSoundFile: failed SetFPos to start (%d)\n", MyOSErr);
		UseResFile(PriorResFile);
		return MyOSErr;
	}
	
	descriptor.frames_per_file      = file_pos_bytes / bytes_per_frame;
	descriptor.start_pos_in_file    = 0;
	descriptor.byte_len_in_file     = descriptor.frames_per_file * bytes_per_frame;
	descriptor.bytes_need_swizzling = false;

	if (0)
	{
		cout	<< "\tbits_per_sample   = " << descriptor.bits_per_sample << endl
				<< "\tsamples_per_frame = " << descriptor.samples_per_frame << endl
				<< "\tframes_per_file   = " << descriptor.frames_per_file << endl
				<< "\tframes_per_second = " << descriptor.frames_per_second << endl;
		cout	<< "\tthere are " << descriptor.byte_len_in_file << " bytes of signal data starting at FPos " << descriptor.start_pos_in_file << endl;
	}

	// Check things
	if (descriptor.bits_per_sample != 8 && descriptor.bits_per_sample != 16 && descriptor.bits_per_sample != 24 &&  descriptor.bits_per_sample != 32)
	{
		printf("ParseSd2fSoundFile: file contains other than 8-bit, 16-bit, 24-bit or 32-bit audio data\n");
		UseResFile(PriorResFile);
		return -1;
	}
	
	if (descriptor.samples_per_frame != 1 && descriptor.samples_per_frame != 2)
	{
		printf("ParseSd2fSoundFile: file contains more than 2 channels of audio data\n");
		UseResFile(PriorResFile);
		return -1;
	}
	
	//	fill out sound file header for basic information (note info gets replaced if we come across a real sncl sf header)
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
	unsigned long	num_samples = descriptor.frames_per_file;
	
	if ( descriptor.samples_per_frame == 2)
		num_samples <<= 1;
		
	sf_header.mValidData.words_to_index(num_samples);
	
	num_samples = (descriptor.frames_per_file + 255) & 0xFFFFFF00;
	
	if ( descriptor.samples_per_frame == 2)
		num_samples <<= 1;
		
	sf_header.mTotalData.words_to_index(num_samples);
	
	//	Get pertinant optional info if available (i.e., captions and loops)
	//	The optional predefined resource types are 'sdDD' (document data), 'sdML' (markers) and 'sdLL' (loops).
	//	I've only seen 'sdLL' resources in the files I'm able to generate or have found floating around.
	//	I'm uninclined to write code that I can't test so I won't work on parsing 'sdDD' or 'sdML' resources
	//	until I encounter files that contain them.
	for (i = 0; i < NumResTypes; i++)
	{
		MyResourceHandle = Get1Resource(ResourceTypeList[i], 1000);

		if ((MyOSErr = ResError()) != 0)							// Could not get additional resources
			continue;
		
		if (MyResourceHandle == NULL || *MyResourceHandle == NULL)	// No data
			continue;

		GetResInfo(MyResourceHandle, &ReturnedID, &ReturnedType, MyResourceName);
		
		switch (ReturnedType)
		{
			case SD2Loops_ResType:
			{
				SD2LoopsRecord&	record = *((SD2LoopsRecord*)*MyResourceHandle);
				LoopRecord&     loop   = record.LoopRecords[0];
				unsigned long	start  = loop.LoopStart;			// documentation implies these are sample #'s from the start
				unsigned long	end    = loop.LoopEnd;
				
				if (record.NumLoops < 1)
					break;
	
				if ((start > descriptor.frames_per_file)			// Loop start and end
				||  (end   > descriptor.frames_per_file)
				||  (end   < 1))
					break;
				
				if (end < start)									// Swap end & start
				{
					unsigned long temp = end;
					end   = start;
					start = temp;
				}
				if (descriptor.samples_per_frame == 2)				// stereo sound file: correct
					{start <<= 1; end <<= 1;}
					
				sf_header.mTotalLength.words_to_index(end);
				sf_header.mLoopLength.words_to_index(end-start);

				break;
			}
			
			case SD2Markers_ResType:
				break;

			case SD2DocumentData_ResType:
			{
				DocumentDataRecord&	record = *((DocumentDataRecord*)*MyResourceHandle);
				
				if (record.Comment[0])
				{
					int i;
					
					sf_header.mData.id_field_bytes = record.Comment[0];

					for (i=0; i<record.Comment[0]; i++)
					{
						sf_header.mData.id_field[i^1] = record.Comment[i+1];
					}
				}
				break;
			}
			
			case kSynclCreatorCode:
			{
				sncl_chunk_scanned = true;
				
				// Check length
				if (GetHandleSize(MyResourceHandle) != sizeof(SynclSFHeader))
				{
					printf("ParseSd2fSoundFile: SNCL resource is too small\n");
					return -1;
				}

				memcpy(&sf_header.mData, *MyResourceHandle, sizeof(SynclSFHeader));
				
				SwapBytesSynclSFHeader(&sf_header.mData);

				break;
			}
		}
	}

	return noErr;
}


// ---------------------------------------------------------------------------
//	╔ ParseWAVESoundFile(short dataRefnum, short resourceRefnum, SynclSFHeader& sf_header)
// ---------------------------------------------------------------------------
//	Handles WAVE files

OSErr	ParseWAVESoundFile(short dataRefnum, short /*resourceRefnum*/, AudioDataDescriptor& descriptor, SynclSFHeader& sf_header_struct)
{
	CSynclSFHeader		sf_header(sf_header_struct);		// Create CSynclSFHeader with which to manipulate struct
	WAVEContainerChunk	MyContainerChunk;					//	12 bytes
	OSErr				MyOSErr	= noErr;
	SInt32				num_bytes_requested = 0, num_bytes_read = 0;
	SInt32				end_of_file	= 0;

	Boolean				format_chunk_scanned = false;
	Boolean				data_chunk_scanned   = false;
	Boolean				fact_chunk_scanned   = false;
	Boolean				sncl_chunk_scanned   = false;

	SInt32				DebugFPos	= 0;

	//	init output struct
	sf_header.init();
	memset(&descriptor, 0, sizeof(descriptor));

	//	get and verify the 'RIFF' container chunk
	num_bytes_read = num_bytes_requested = sizeof(WAVEContainerChunk);
	
	if ((MyOSErr = SetFPos(dataRefnum, fsFromStart, 0)) != 0)
	{
		printf("ParseWAVESoundFile: failed SetFPos (%d)\n", MyOSErr);
		return MyOSErr;
	}
	
	if ((MyOSErr = FSRead(dataRefnum, &num_bytes_read, &MyContainerChunk)) != 0)
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

	if ((MyOSErr = SetFPos(dataRefnum, fsFromLEOF, 0)) != 0)
	{
		printf("ParseWAVESoundFile: failed SetFPos to EOF (%d)\n", MyOSErr);
		return MyOSErr;
	}
	
	if ((MyOSErr = GetFPos(dataRefnum, &DebugFPos)) != 0)
	{
		printf("ParseWAVESoundFile: failed GetFPos (%d)\n", MyOSErr);
		return MyOSErr;
	}
	
	if (DebugFPos < end_of_file)
	{
		if (DebugFPos < sizeof(WAVEChunkHeader) + 100)
		{
			printf("ParseWAVESoundFile: file is too short to be WAVE\n");
			return -1;
		}
		
		MyContainerChunk.ckSize = DebugFPos - sizeof(WAVEChunkHeader);
	}
	
	//	restore FPos
	long SourceFPos = sizeof(WAVEContainerChunk);	//	this is the position of the first local chunk in the 'RIFF' chunk
	
	if ((MyOSErr = SetFPos(dataRefnum, fsFromStart, SourceFPos)) != 0)
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

		if ((MyOSErr = FSRead(dataRefnum, &num_bytes_read, &source_buffer)) != 0)
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
				
				if ((MyOSErr = FSRead(dataRefnum, &num_bytes_read, source_buffer.wave_generic_chunk.chunk_data)) != 0)
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
				
				descriptor.start_pos_in_file    = SourceFPos;
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
				
				if ((MyOSErr = FSRead(dataRefnum, &num_bytes_read, source_buffer.wave_generic_chunk.chunk_data)) != 0)
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
				
				if ((MyOSErr = FSRead(dataRefnum, &num_bytes_read, &sf_header.mData)) != 0)
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
			
		if ((MyOSErr = SetFPos(dataRefnum, fsFromStart, SourceFPos)) != 0)
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
		unsigned long	num_samples = descriptor.frames_per_file;
		
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
	descriptor.start_pos_in_file = bytes_of_chunks;
	
	return (bytes_in_all);
}

OSErr	CreateAIFFSoundFile(short dataRefnum, short /*resourceRefnum*/, AudioDataDescriptor& descriptor, SynclSFHeader& sf_header_struct)
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
	if ((MyOSErr = SetFPos(dataRefnum, fsFromStart, 0)) != 0)
	{
		printf("CreateAIFFSoundFile: failed SetFPos (%d)\n", MyOSErr);
		return MyOSErr;
	}
	
	//	Fill in and write the ContainerChunk
	ContainerChunk& containerChunk = dest_buffer.container_chunk;
	
	memset(&containerChunk, 0, sizeof(ContainerChunk));
	
	containerChunk.ckID     = FORMID;
	containerChunk.ckSize   = bytes_in_all - sizeof(ChunkHeader);
	containerChunk.formType = AIFFID;
	
	num_bytes_written = num_bytes_requested = sizeof(containerChunk);
	
	containerChunk.ckID     = CFSwapInt32HostToBig(containerChunk.ckID);
	containerChunk.ckSize   = CFSwapInt32HostToBig(containerChunk.ckSize);
	containerChunk.formType = CFSwapInt32HostToBig(containerChunk.formType);
	
	if ((MyOSErr = FSWrite(dataRefnum, &num_bytes_written, &containerChunk)) != 0)
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
	
	if ((MyOSErr = FSWrite(dataRefnum, &num_bytes_written, &commonChunk)) != 0)
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
	
	if ((MyOSErr = FSWrite(dataRefnum, &num_bytes_written, &synclavierChunk)) != 0)
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
	soundDataChunk.ckSize    = sizeof(soundDataChunk) + bytes_of_data - sizeof(ChunkHeader);
	soundDataChunk.offset    = 0;
	soundDataChunk.blockSize = 512;
	
	num_bytes_written = num_bytes_requested = sizeof(soundDataChunk);
	
	soundDataChunk.ckID      = CFSwapInt32HostToBig(soundDataChunk.ckID);
	soundDataChunk.ckSize    = CFSwapInt32HostToBig(soundDataChunk.ckSize);
	soundDataChunk.offset    = CFSwapInt32HostToBig(soundDataChunk.offset);
	soundDataChunk.blockSize = CFSwapInt32HostToBig(soundDataChunk.blockSize);

	if ((MyOSErr = FSWrite(dataRefnum, &num_bytes_written, &soundDataChunk)) != 0)
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
//	╔ CreateSd2fSoundFile(short dataRefnum, short resourceRefnum, SynclSFHeader& sf_header)
// ---------------------------------------------------------------------------

long	ComputeSd2fSoundFileSize(SynclSFHeader& , AudioDataDescriptor& descriptor)
{
	//	Return where the audio data should go
	descriptor.start_pos_in_file = 0;
	
	return (descriptor.byte_len_in_file);
}

// Note: I don't have a clue whether AddResource takes care of byte swapping on Intel macs (for the standard SD2 resources). I am guessing that it does...
OSErr	CreateSd2fSoundFile(short dataRefnum, short resourceRefnum, AudioDataDescriptor& descriptor, SynclSFHeader& sf_header_struct)
{
	CSynclSFHeader	sf_header(sf_header_struct);		// Create CSynclSFHeader with which to manipulate struct
	OSErr			MyOSErr	= noErr;
	long			num_bytes_requested = 0, num_bytes_written = 0;

	short			PriorResFile = CurResFile();
	
	int				i;

	//	Check res file
	if (resourceRefnum == (-1))
	{
		printf("CreateSd2fSoundFile: resource could not be stored\n");
		UseResFile(PriorResFile);
		return -1;
	}

	UseResFile(resourceRefnum);

	//	Mandatory resources written
	//		3 Str resources (sample-size, sample-rate, channels)
	//	
	//	Optional resources written
	//		Synclavier sound file header
	//		Documentation resource

	// 	Position to start of file for grins (that's where data will go)
	if ((MyOSErr = SetFPos(dataRefnum, fsFromStart, 0)) != 0)
	{
		printf("CreateSd2fSoundFile: failed SetFPos (%d)\n", MyOSErr);
		UseResFile(PriorResFile);
		return MyOSErr;
	}
	
	//	Fill in and write the Mandatory resources
	char	info[512];
	Handle	it;

	// Str 1000 - sample size
	sprintf(info, "%d", (int) (descriptor.bits_per_sample/8));				// sample size bytes
	c2pstr(info);
	it = NewHandle(info[0]+1);

	if (!it or !*it)
	{
		printf("CreateSd2fSoundFile: Ran out of memory\n");
		UseResFile(PriorResFile);
		return MyOSErr;
	}
	
	memcpy(*it, info, info[0]+1);
	AddResource(it, STR_ResType, 1000, "\psample-size");

	// Str 1001 - sample rate
	sprintf(info, "%.3f", (float) descriptor.frames_per_second);	// frames per second
	c2pstr(info);
	it = NewHandle(info[0]+1);

	if (!it or !*it)
	{
		printf("CreateSd2fSoundFile: Ran out of memory\n");
		UseResFile(PriorResFile);
		return MyOSErr;
	}
	
	memcpy(*it, info, info[0]+1);
	AddResource(it, STR_ResType, 1001, "\pchannels");

	// Str 1002 - channels
	sprintf(info, "%d", (int) descriptor.samples_per_frame);				// samples per frame
	c2pstr(info);
	it = NewHandle(info[0]+1);

	if (!it or !*it)
	{
		printf("CreateSd2fSoundFile: Ran out of memory\n");
		UseResFile(PriorResFile);
		return MyOSErr;
	}
	
	memcpy(*it, info, info[0]+1);
	AddResource(it, STR_ResType, 1002, "\psample-size");

	// synclavier
	it = NewHandle(sizeof(sf_header.mData));

	if (!it or !*it)
	{
		printf("CreateSd2fSoundFile: Ran out of memory\n");
		UseResFile(PriorResFile);
		return MyOSErr;
	}
	
	memcpy(*it, &sf_header.mData, sizeof(sf_header.mData));
	SwapBytesSynclSFHeader((SynclSFHeader*) *it);
	AddResource(it, kSynclCreatorCode, 1000, "\psynclavier");

	// Documentation (e.g. caption)
	if (sf_header.mData.id_field_bytes)
	{
		it = NewHandle(sizeof(DocumentDataRecord));

		if (!it or !*it)
		{
			printf("CreateSd2fSoundFile: Ran out of memory\n");
			UseResFile(PriorResFile);
			return MyOSErr;
		}
		
		memset(*it, 0, sizeof(DocumentDataRecord));
		
		DocumentDataRecord& ddr = * (DocumentDataRecord*) *it;

		ddr.Version           = 1;
		ddr.HDPlayBufMultiple = 8;
		ddr.FramesPerSec      = 30;
		ddr.FilmSize          = 35;
		ddr.Tempo             = 120 << 16;
		ddr.TimeSignature     = (4 << 16) + 4;
		ddr.Zoom.v            = -256;
		ddr.Zoom.h            = 1;
		
		ddr.Scale.VFactor = 327;
		ddr.Scale.VType   = (ScaleNames) 4;
		strcpy((char*) ddr.Scale.VString, "%Scale");
		c2pstr((char*) ddr.Scale.VString);
		
		ddr.Scale.HFactor = 1;
		ddr.Scale.HType   = (ScaleNames) 0;
		strcpy((char*) ddr.Scale.HString, "sec");
		c2pstr((char*) ddr.Scale.HString);

		if (sf_header.mData.id_field_bytes < 256)
		{
			ddr.Comment[0] = sf_header.mData.id_field_bytes;
			
			for (i=0; i<sf_header.mData.id_field_bytes; i++)
				ddr.Comment[i+1] = sf_header.mData.id_field[i^1];
		}

		AddResource(it, SD2DocumentData_ResType, 1000, "\pDocumentDataRecord");
	}

	// Loops
	if (sf_header.mTotalLength.mData.sector
	||  sf_header.mTotalLength.mData.word_offset
	||  sf_header.mLoopLength.mData.sector
	||  sf_header.mLoopLength.mData.word_offset)
	{
		it = NewHandle(sizeof(SD2LoopsRecord) + sizeof(LoopRecord));

		if (!it or !*it)
		{
			printf("CreateSd2fSoundFile: Ran out of memory\n");
			UseResFile(PriorResFile);
			return MyOSErr;
		}
		
		memset(*it, 0, sizeof(SD2LoopsRecord) + sizeof(LoopRecord));
		
		SD2LoopsRecord& looprec = * (SD2LoopsRecord*) *it;

		unsigned long	totalLength = (sf_header.mTotalLength.mData.sector << 8) + sf_header.mTotalLength.mData.word_offset;
		unsigned long	loopLength  = (sf_header.mLoopLength.mData.sector  << 8) + sf_header.mLoopLength.mData.word_offset;
		
		if (descriptor.samples_per_frame == 2)				// sf in total samples; need per channel here
		{
			totalLength >>= 1;
			loopLength  >>= 1;
		}

		unsigned long	start  = totalLength - loopLength;	// start of loop
		unsigned long	end    = totalLength;				// end of loop

		looprec.Version  = 1;
		looprec.NumLoops = 1;
		looprec.LoopRecords[0].LoopStart = start;
		looprec.LoopRecords[0].LoopEnd   = totalLength;
		looprec.LoopRecords[0].LoopIndex = 1;
		looprec.LoopRecords[0].LoopSense = 117;
		looprec.LoopRecords[0].Channel   = 0;
		
		if (descriptor.samples_per_frame == 2)
		{
			looprec.NumLoops = 2;
			looprec.LoopRecords[1].LoopStart = start;
			looprec.LoopRecords[1].LoopEnd   = totalLength;
			looprec.LoopRecords[1].LoopIndex = 2;
			looprec.LoopRecords[1].LoopSense = 117;
			looprec.LoopRecords[1].Channel   = 1;
		}

		AddResource(it, SD2Loops_ResType, 1000, "\pSD2LoopsRecord");
	}

	UseResFile(PriorResFile);

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
	descriptor.start_pos_in_file = bytes_of_chunks;
	
	return (bytes_in_all);
}

OSErr	CreateWAVESoundFile(short dataRefnum, short /*resourceRefnum*/, AudioDataDescriptor& descriptor, SynclSFHeader& sf_header_struct)
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
	if ((MyOSErr = SetFPos(dataRefnum, fsFromStart, 0)) != 0)
	{
		printf("CreateWAVESoundFile: failed SetFPos (%d)\n", MyOSErr);
		return MyOSErr;
	}
	
	//	Fill in and write the ContainerChunk
	WAVEContainerChunk& containerChunk = dest_buffer.wave_container_chunk;
	
	memset(&containerChunk, 0, sizeof(WAVEContainerChunk));
	
	containerChunk.ckID     = WAVEFORMID;
	containerChunk.ckSize   = bytes_in_all - sizeof(WAVEChunkHeader);
	containerChunk.formType = WAVEID;
	
	// Tweak
	containerChunk.ckID     = CFSwapInt32HostToBig(containerChunk.ckID);
	containerChunk.ckSize   = EndianU32_LtoN(containerChunk.ckSize);
	containerChunk.formType = CFSwapInt32HostToBig(containerChunk.formType);

	// Write
	num_bytes_written = num_bytes_requested = sizeof(containerChunk);
	
	if ((MyOSErr = FSWrite(dataRefnum, &num_bytes_written, (void *) &containerChunk)) != 0)
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
	fmt_chunk.dwSamplesPerSec  = (long) descriptor.frames_per_second;
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
	
	if ((MyOSErr = FSWrite(dataRefnum, &num_bytes_written, &fmt_chunk)) != 0)
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
	
	if ((MyOSErr = FSWrite(dataRefnum, &num_bytes_written, &synclavierChunk)) != 0)
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
	dataChunk.ckSize    = sizeof(dataChunk) + bytes_of_data - sizeof(ChunkHeader);
	
	// Tweak
	dataChunk.ckID   = CFSwapInt32HostToBig(dataChunk.ckID);
	dataChunk.ckSize = EndianU32_LtoN(dataChunk.ckSize);

	// Write
	num_bytes_written = num_bytes_requested = sizeof(dataChunk);
	
	if ((MyOSErr = FSWrite(dataRefnum, &num_bytes_written, &dataChunk)) != 0)
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
