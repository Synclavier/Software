// =================================================================================
//	CSynclavierSoundFileHeader.h
// =================================================================================

// Contents: STRUCTURE AND CLASS DEFINITIONS FOR SYNCLAVIER FILE FORMATS

// REVISION HISTORY:
//		2000/06/04	Todd Yvega.		Created.
//		2000/08/28	Cameron Jones.	Blended.

// Todd: Summary of changes:
// 1) Classes can vary in size due to compiler tables (mostly when using virtual functions).
//    So I created structs (whose size and layout you can more readily control) to hold the
//    data and classes to manipulate them.

#pragma once

// ---------------------------------------------------------------------------
//	╔ Enumerations
// ---------------------------------------------------------------------------

typedef enum SynclFileTypeCodes{					//	file type codes
	kFileTypeSyncSound = FOUR_CHAR_CODE('SNDF')		//	dcl t#sound	 lit ' 5';	//	sound file
} SynclFileTypeCodes;

typedef enum SynclCreatorCodes{						//	file creator codes
    kSynclCreatorCode	= FOUR_CHAR_CODE('SNCL'),
    kSynclS3CaptionCode	= FOUR_CHAR_CODE('S3CA'),
    kSynclS3HandleCode	= FOUR_CHAR_CODE('S3HA')
} SynclCreatorCodes;


#define kAFInfoDictionary_SoundFileHandle "handle"

// ---------------------------------------------------------------------------
//	╔ Handy Structures and Unions
// ---------------------------------------------------------------------------

#pragma pack(push,2)

typedef struct XPLFloating
{
	unsigned	signbit	 :	1;		// 0 = positive, 1 = negative
	unsigned	mantissa : 24;		// 0..2^24-1
	unsigned	exponent :	7;		// 0..127	(0 = -64, 64 = 0, 127 = +63)

} XPLFloating;

typedef union XPL4ByteUnion
{
	SInt16 		xpl_signed_16	[2];
	UInt16 		xpl_unsigned_16 [2];
	SInt32 		xpl_signed_32;
	UInt32 		xpl_unsigned_32;
	XPLFloating	xpl_floating;

} XPL4ByteUnion;

typedef struct SynclSFTime
{
	UInt16	seconds;
	UInt16	milliseconds;
	UInt16	microseconds;

} SynclSFTime;

extern Float64 SynclSFTimeToFrameIndex(SynclSFTime& sfTime, UInt16 sampleRate, UInt16 periodIndex);

typedef	struct	SynclSFIndex
{
	UInt32	sector;
	UInt16	word_offset;

} SynclSFIndex;

extern Float64 SynclSFIndexToFrameIndex(SynclSFIndex& sfIndex, UInt16 stereo);

typedef struct	SynclSFSymbol
{
	SynclSFTime		time;
	UInt16			numchars;
	unsigned char	name[8];

} SynclSFSymbol;


// ---------------------------------------------------------------------------
//	╔ Structure for complete Synclavier Sound File Header
// ---------------------------------------------------------------------------

typedef	struct	SynclSFHeader
{
	// first half of sector 0 starts here
	SInt16			compatibility;			//	'  0'	sound file revision
	UInt16			file_data_type;			//	'  1'	data type of the sound file
	SynclSFIndex	valid_data;				//	'  2'	number of data points in file
	UInt16			unused_005;				//	'  5'
	SynclSFIndex	total_data;				//	'  6'	total allocated data length (usually valid_data rounded up to next sector)
	SynclSFTime		data_end;				//	'  9'	the time of the last data point (time corresponding to valid_data-1)
	UInt16			keyboard_decay;			//	' 12'	keyboard decay (a time value in milliseconds)
	UInt16			semitones;				//	' 13'	pitch bend range in semitones
	UInt16			vibrato_rate;			//	' 14'	Hz*100; Vibrato is the periodic variation in pitch
	UInt16			vibrato_depth;			//	' 15'	semitones*100; pitch range
	UInt16			vibrato_attack;			//	' 16'	vibrato wave attack time in milliseconds
	UInt16			vibrato_type;			//	' 17'	twelve possible vibrato wave shapes
	UInt16			hertz;					//	' 18'	10*pitch frequency (i.e., 4400); The number of cycles per second
	XPLFloating		octave;					//	' 19'	floating point keyboard octave.cents set by user
	UInt16			period_index;			//	' 21'	the number of D66 clock ticks per sampling period (clock ticks rate = 30,000,000 Hz)
	UInt16			nyquist_freq;			//	' 22'	Nyquist frequency in Hz
	SynclSFTime		mark_start;				//	' 23'	marks where the sound begins.
	SynclSFTime		mark_end;				//	' 26'	marks where the sound ends.
	SynclSFTime		cursor_time;			//	' 29'	the time of the current cursor location.
	UInt16			gain_exponent;			//	' 32'	for filters, the scale factor which scales the output in steps of 6 dB
	UInt16			number_of_symbols;		//	' 33'	number of symbols in sound file
	SynclSFIndex	total_length;			//	' 34'	perfect looping file length in 24-bit format
	SynclSFIndex	loop_length;			//	' 37'	perfect loop length in 24-bit format
	UInt16			magic_number;			//	' 40'	used to detect possible changes in file in SFM
	UInt16			stereo;					//	' 41'	indicates if stereo or not 1-> stereo
	UInt16			sample_rate;			//	' 42'	poly system sampling rate in kHz * 10
	UInt16			unused_043;				//	' 43'
	UInt16			unused_044;				//	' 44'
	UInt16			smpte_mode;				//	' 45'	SMPTE mode
	UInt16			smpte_bits;				//	' 46'	SMPTE start time of sound file bits
	Byte			smpte_seconds;			//	' 47'	SMPTE start time of sound file seconds
	Byte			smpte_frames;			//	' 47'	SMPTE start time of sound file frames
	Byte			smpte_hours;			//	' 48'	SMPTE start time of sound file hours
	Byte			smpte_minutes;			//	' 48'	SMPTE start time of sound file minutes
	SynclSFTime		mark_offset;			//	' 49'	time corresponding to SMPTE offset in sound file
	UInt16			index_base;				//	' 52'	start of category index in this sector
    UInt16          file_handle[64];        //  ' 53'   start of file handle. C string format, including trailing null fits into 64 words.
	UInt16			unused_085[127-117];    //			(unused words 117 through 126)
	UInt16			id_field_bytes;			//	'127'	byte count of sound file caption

	// second half of sector 0 starts here (word 128)
	char			id_field[128*2];		//	'128'	word string of user data or file caption
	// sector 1 starts here
	SynclSFSymbol	symbols[64];			//			(64 symbols at 16 bytes each fills two sectors)

} SynclSFHeader;

#pragma pack(pop)

// ---------------------------------------------------------------------------
//	╔ class CXPLFloating
// ---------------------------------------------------------------------------

class CXPLFloating {
public:
	CXPLFloating(XPLFloating& data) : mData(data) {}

	void clear(void) {mData.signbit = mData.mantissa = mData.exponent = 0;}
	void assign_ratio(int numerator, int denominator);

	XPLFloating& mData;
};


// ---------------------------------------------------------------------------
//	╔ class CSynclSFTime
// ---------------------------------------------------------------------------

class CSynclSFTime {
public:
	CSynclSFTime(SynclSFTime& data) : mData(data) {}

	void clear(void)	 			{mData.seconds = mData.milliseconds = mData.microseconds = 0;}
	void assign(SynclSFTime& time)	{mData = time;}

	SynclSFTime& mData;
};


// ---------------------------------------------------------------------------
//	╔ class CSynclSFIndex
// ---------------------------------------------------------------------------

class CSynclSFIndex {
public:
	CSynclSFIndex(SynclSFIndex& data) : mData(data) {}

	void clear(void) {mData.sector = mData.word_offset = 0;}
	void words_to_index	(unsigned int numWords) {
		mData.word_offset	= numWords & 255;
		mData.sector		= (UInt32) numWords >> 8;
	}

	SynclSFIndex& mData;
};


// ---------------------------------------------------------------------------
//	╔ class CSynclSFSymbol
// ---------------------------------------------------------------------------

class CSynclSFSymbol {
public:
	CSynclSFSymbol(SynclSFSymbol& data) : mData(data) {}

	void clear(void);
	void place_pstring(const unsigned char *MyString);
	void place_time(const SynclSFTime& time) {mData.time = time;}

	SynclSFSymbol& mData;
};


// ---------------------------------------------------------------------------
//	╔ class CSynclSFHeader
// ---------------------------------------------------------------------------

class CSynclSFHeader {
public:
	CSynclSFHeader(SynclSFHeader& data);
	
	void			init				 (void);
	void			set_rates			 (double dSampleRate);
	void			place_symbol_string	 (UInt16 index, const unsigned char *MyString);
	SynclSFTime		sampleframes_to_time (unsigned int sampleframes);

	SynclSFHeader&  mData;				// Reference to sound file header data
	
	CSynclSFIndex	mValidData;			// Handy instantiated objects to manipulate fields of data
	CSynclSFIndex	mTotalData;
	CSynclSFTime	mDataEnd;
	CXPLFloating	mOctave;
	CSynclSFTime	mMarkStart;
	CSynclSFTime	mMarkEnd;
	CSynclSFTime	mCursorTime;
	CSynclSFIndex	mTotalLength;
	CSynclSFIndex	mLoopLength;
	CSynclSFTime	mMarkOffset;
};


/*
0	000 |
     4					//	'  0'	sound file revision
     1					//	'  1'	data type of the sound file
     0   117    49		//	'  2'	points to 3 word index vector with number of data points in file
     0					//	'  5' 	unused
     0   118     0		//	'  6'	points to total allocated data length. 24-bit format
     0   600     0		//	'  9'	the time of the last data point (time corresponding to valid.data-1)
     0					//	' 12'	keyboard decay (a time value in milliseconds)
     0					//	' 13'	pitch bend range in semitones
     0					//	' 14'	Hz*100; Vibrato is the periodic variation in pitch
     0					//	' 15'	semitones*100; pitch range for Random wave shapes
     0					//	' 16'	vibrato wave attack time in milliseconds
     0					//	' 17'	twelve possible vibrato wave shapes
  4400					//	' 18'	10*pitch frequency (i.e., 4400); The number of cycles per second
 25313 18370			//	' 19'	floating point keyboard octave.cents set by user					
   600					//	' 21'	the number of clock ticks per sampling period
    10					//	' 22'	in Hz
     0     0     0		//	' 23'	mark_start marks where the sound begins.
     0   600     0		//	' 26'	mark_end marks where the sound ends.
     0     0     0		//	' 29'	the time of the current cursor location.
     0					//	' 32'	for filters, the scale factor which scales the output in steps of 6 dB
     7					//	' 33'	number of symbols in sound file
     0     0     0		//	' 34'	perfect looping file length in 24-bit format
     0     0     0		//	' 37'	perfect loop length in 24-bit format
     0					//	' 40'	used to detect possible changes in file in SFM
     0					//	' 41'	indicates if stereo or not 1-> stereo
   500					//	' 42'	sound file sampling rate in kHz * 10
     0					//	' 43'	unused
     0					//	' 44'	unused
     0					//	' 45'	SMPTE mode
     0					//	' 46'	SMPTE start time bits
     0					//	' 47'	SMPTE start time of file
     0					//	' 48'	SMPTE start time of sound file
     0   592   500		//	' 49'	time corresponding to SMPTE offset in sound file
   238					//	' 52'	start of category index in this sector

0	000 |     4     1     0   117    49     0     0   118
0	008 |     0     0   600     0     0     0     0     0
0	016 |     0     0  4400 25313 18370   600    10     0
0	024 |     0     0     0   600     0     0     0     0
0	032 |     0     7     0     0     0     0     0     0
0	040 |     0     0   500     0     0     0     0     0
0	048 |     0     0   592   500   238     0     0     0
0	056 |     0     0     0     0     0     0     0     0
0	064 |     0     0     0     0     0     0     0     0
0	072 |     0     0     0     0     0     0     0     0
0	080 |     0     0     0     0     0     0     0     0
0	088 |     0     0     0     0     0     0     0     0
0	096 |     0     0     0     0     0     0     0     0
0	104 |     0     0     0     0     0     0     0     0
0	112 |     0     0     0     0     0     0     0     0
0	120 |     0     0     0     0     0     0     0    18		ииииииииииииииии
0	128 | 26708 29545 26912  8307 31085 25376 28769 26996		Thisиisиmyиcapti
0	136 | 28271     0     0     0     0     0     0     0		onииииииииииииии
0	144 |     0     0     0     0     0     0     0     0		ииииииииииииииии
0	152 |     0     0     0     0     0     0     0     0		ииииииииииииииии
0	160 |     0     0     0     0     0     0     0     0		ииииииииииииииии
0	168 |     0     0     0     0     0     0     0     0		ииииииииииииииии
0	176 |     0     0     0     0     0     0     0     0		ииииииииииииииии
0	184 |     0     0     0     0     0     0     0     0		ииииииииииииииии
0	192 |     0     0     0     0     0     0     0     0		ииииииииииииииии
0	200 |     0     0     0     0     0     0     0     0		ииииииииииииииии
0	208 |     0     0     0     0     0     0     0     0		ииииииииииииииии
0	216 |     0     0     0     0     0     0     0     0		ииииииииииииииии
0	224 |     0     0     0     0     0     0     0     0		ииииииииииииииии
0	232 |     0     0     0     0     0     0    20 16707		ииииииииииииииCA
0	240 | 17748 20295 22866 12639 22330 20303 22340 20041		TEGORY_1:WOODWIN
0	248 | 21316    12 22343 20303 11588 22343 20041 21316		DSииGWOOD-GWINDS
1	000 |     0     0     0     6 21071 18249 20041  8224		ииииииииORIGINии
1	008 |     0     0     0     1  8227  8224  8224  8224		ииииииии#иииииии
1	016 |     0     4   280     8 22861 16716 17730 12364		ииииииииMYLABEL0
1	024 |     0   137   280     8 22861 16716 17730 12620		ииииииииMYLABEL1
1	032 |     0   522   680     8 22861 16716 17730 12876		ииииииииMYLABEL2
1	040 |     0   600     0     1  8228  8224  8224  8224		ииXиииии$иииииии
1	048 |     0   600     0     3 20037  8260  8224  8224		ииXиииииENDиииии
1	056 |     0     0     0     0     0     0     0     0		ииииииииииииииии

#	the following procedure shows how the categories fit into the caption area
File ":SYNSOU:STORMOD:ARC-ACT"; Line 146	#	Stuff.Categories: proc(buf) boolean swapable;	//	get categories from user

dcl max.symbols		lit '64';	//	maximum number of symbols in the sound file symbol table
dcl symbol_length	lit ' 8';	//	symbol description is eight words long
//	64 symbols * 8 words = 2 * 256 words

dcl name_entry		lit ' 3';	//	point to the name entry location
*/
