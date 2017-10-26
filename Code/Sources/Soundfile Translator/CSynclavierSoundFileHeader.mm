// =================================================================================
//	CSynclavierSoundFileHeader.cp
// =================================================================================

//	CONTAINS: CONSTRUCTOR FUNCTIONS FOR SYNCLAVIER CLASSES (CXPLFloating, CSynclSFTime, CSynclSFIndex, CSynclSFSymbol and CSynclSFHeader
//	REVISION HISTORY:
//		2000/06/04	Todd Yvega		Created.
//		2000/08/28	Cameron Jones	Blended.

/*ииииииииииииииииииииииииии  # I N C L U D E S  ииииииииииииииииииииииииии*/

// Local includes
#include "CSynclavierSoundFileHeader.h"

// Functions for SynclSFTime
Float64 SynclSFTimeToFrameIndex(SynclSFTime& sfTime, UInt16 sampleRate, UInt16 periodIndex)
{
    // Samples = (samples/second)*seconds;
    // Samples/second = 30,000,000 / periodIndex;
    
    Float64 seconds = ((Float64)sfTime.microseconds/1000000.0) + ((Float64)sfTime.milliseconds/1000.0) + (Float64)sfTime.seconds;
    Float64 hz      = (sampleRate == 0) ? (30000000.0/(Float64)periodIndex) : (((Float64)sampleRate)*100.0);
    
    Float64 frame   = rint(hz*seconds);
    
    return frame;
}

// Functions for SynclSFIndex
Float64 SynclSFIndexToFrameIndex(SynclSFIndex& sfIndex, UInt16 stereo) {
    Float64 frame = (Float64)sfIndex.sector*256.0 + (Float64)sfIndex.word_offset;

    if (stereo == 1)
        frame /= 2.0;
    
    return (rint(frame));
}


// ---------------------------------------------------------------------------
//	╔ CXPLFloating
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
//	╔ CXPLFloating::assign_ratio(signed long slNumerator, signed long slDenominator)
// ---------------------------------------------------------------------------

//	Sets an XPL floating variable to the value most closely approximating the value of
//	a ratio of two signed 32 bit integers.

//	It will work without overflow for numerators and denominators in the range of
//	-2,147,483,648 to 2,147,483,647

void CXPLFloating::assign_ratio(signed int slNumerator, signed int slDenominator) {

	if (slNumerator == 0 || slDenominator == 0) {
		//	don't bother with division by zero error - just initialize float as zero
		mData.signbit	= 0;
		mData.mantissa	= 0;
		mData.exponent	= 0;
		return;
	}

	short				scale_exponent = 0;
	unsigned long long	ullNumerator;
	unsigned long		ulDenominator;
	//	deal with sign conversions and set sign bit
	if (slNumerator < 0) {
		ullNumerator	= -slNumerator;
		mData.signbit	= true;
	}
	else {
		ullNumerator	= slNumerator;
		mData.signbit   = false;
	}
	if (slDenominator < 0) {
		ulDenominator	= -slDenominator;
		mData.signbit	^= true;
	}
	else ulDenominator = slDenominator;
	//	normalize numerator and denominator setting the exponent accordingly
	mData.exponent = 64;	//	64 maps to 0
	while ((ullNumerator << 1) < ulDenominator) {
		ullNumerator <<= 1;
		mData.exponent--;
	}
	while (ullNumerator >= ulDenominator) {
		if ((ullNumerator & 1) == 0)	ullNumerator	>>= 1;
		else									ulDenominator	<<= 1;
		mData.exponent++;
	}
	while (((ulDenominator & 1) == 0) && (scale_exponent != 24)) {
		ulDenominator	>>= 1;
		scale_exponent++;
	}
	//	set the mantissa
	mData.mantissa = (int) (ullNumerator << (24 - scale_exponent)) / ulDenominator;
	//	round if necessary
	if ((((ullNumerator << (24 - scale_exponent)) % ulDenominator) << 1) >= ulDenominator) mData.mantissa++;
}	//	end of xpl_float::assign_ratio()

// ---------------------------------------------------------------------------
//	╔ CSynclSFSymbol
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
//	╔ CSynclSFSymbol::clear
// ---------------------------------------------------------------------------

void CSynclSFSymbol::clear(void) {
	CSynclSFTime	timeData(mData.time);
	
	timeData.clear();
	mData.numchars = 0;
	for (short i = 0; i != 8;) mData.name[i++] = '\0';
}

// ---------------------------------------------------------------------------
//	╔ CSynclSFSymbol::place_pstring
// ---------------------------------------------------------------------------

//	places the passed pascal string into the symbol in XPL string format
void CSynclSFSymbol::place_pstring(const unsigned char *MyString) {
	short i;
	mData.numchars	= (UInt16)MyString[0];
	for (i = 0; i < mData.numchars; i++) {
		//	must byte-swap for XPL string
		mData.name[i^1] = MyString[i+1];
	}
	//	pad to the end with spaces
	//	(SFM pads with zeros but L-page pads with spaces.  Oh well...)
	for (; i < 8; i++) mData.name[i^1] = ' ';			//	pad the rest
}


// ---------------------------------------------------------------------------
//	╔ CSynclSFHeader
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
//	╔ CSynclSFHeader::CSynclSFHeader
// ---------------------------------------------------------------------------

CSynclSFHeader::CSynclSFHeader(SynclSFHeader& data) :
	mData(data),
	mValidData	(data.valid_data  ),
	mTotalData	(data.total_data  ),
	mDataEnd	(data.data_end    ),
	mOctave		(data.octave      ),
	mMarkStart	(data.mark_start  ),
	mMarkEnd	(data.mark_end    ),
	mCursorTime	(data.cursor_time ),
	mTotalLength(data.total_length),
	mLoopLength	(data.loop_length ),
	mMarkOffset	(data.mark_offset )
	{}


// ---------------------------------------------------------------------------
//	╔ CSynclSFHeader::init
// ---------------------------------------------------------------------------

void CSynclSFHeader::init(void) {
	short i;

	mData.compatibility		= 4;
	mData.file_data_type	= 1;				//	(this variable is undocumented in XPL code)
	mData.unused_005		= 0;
	mData.keyboard_decay	= 0;
	mData.semitones			= 0;
	mData.vibrato_rate		= 0;
	mData.vibrato_depth		= 0;
	mData.vibrato_attack	= 0;
	mData.vibrato_type		= 0;
	mData.hertz				= 4400;
	mData.period_index		= 30000000L/50000L;	//	default to 50,000 Hz (clock ticks rate = 30,000,000 Hz)
	mData.nyquist_freq		= 25000;
	mData.gain_exponent		= 0;
	mData.number_of_symbols	= 4;
	mData.magic_number		= 0;
	mData.stereo			= 0;
	mData.sample_rate		= 500;				//	poly system sampling rate in kHz * 10
	mData.unused_043		= 0;
	mData.unused_044		= 0;
	mData.smpte_mode		= 0;
	mData.smpte_bits		= 0;
	mData.smpte_seconds		= 0;
	mData.smpte_frames		= 0;
	mData.smpte_hours		= 0;
	mData.smpte_minutes		= 0;
	mData.index_base		= 0;

	mValidData.clear();
	mTotalData.clear();
	mDataEnd.clear();
	mOctave.assign_ratio(309,100);				//	default to 3.09 (a3)
	mMarkStart.clear();
	mMarkEnd.clear();
	mCursorTime.clear();
	mTotalLength.clear();
	mLoopLength.clear();
	mMarkOffset.clear();

	//	zero out the file handle
    for (i = 0; i != 32;    ) mData.file_handle[i++] = 0;
    
    //	zero out the unused area
	for (i = 0; i != 127-85;) mData.unused_085 [i++] = 0;
	
    //	zero out the caption area
	mData.id_field_bytes= 0;
	for (i = 0; i != 2*128;) mData.id_field[i++] = '\0';
	
    //  zero out the symbol area
	for (i=0; i<64; i++)
	{
		CSynclSFSymbol	symbol(mData.symbols[i]);
		
		symbol.clear();
	}		

	//	create the default symbols "ORIGIN", "#", "$" and "END"
	CSynclSFSymbol	symbol_0(mData.symbols[0]);
	CSynclSFSymbol	symbol_1(mData.symbols[1]);
	CSynclSFSymbol	symbol_2(mData.symbols[2]);
	CSynclSFSymbol	symbol_3(mData.symbols[3]);
	
	symbol_0.place_pstring("\pORIGIN");
	symbol_1.place_pstring("\p#");
	symbol_2.place_pstring("\p$");
	symbol_3.place_pstring("\pEND");
}


// ---------------------------------------------------------------------------
//	╔ CSynclSFHeader::set_rates(double dSampleRate)
// ---------------------------------------------------------------------------

//	set_rates() sets all members pertaining to sample rate based on the passed floating point double
void CSynclSFHeader::set_rates(double dSampleRate) {
	int	sample_rate = (int) (dSampleRate + .5);
	
	//	poly system sampling rate in kHz * 10
	mData.sample_rate   = (UInt16) ((sample_rate + 50) / 100);

	//	the number of D66 clock ticks per sampling period (clock ticks rate = 30,000,000 Hz)
	mData.period_index	= (30000000 + (sample_rate/2)) / sample_rate;

	//	since the Nyquist frequency is only used in SFM, use the effective D66 rate rather than the poly rate
	mData.nyquist_freq	= (15000000 / mData.period_index) + (((15000000 % mData.period_index) << 1) >= mData.period_index);
}


// ---------------------------------------------------------------------------
//	╔ CSynclSFHeader::sampleframes_to_time(unsigned long sampleframes)
// ---------------------------------------------------------------------------

//	sampleframes_to_time() references the sample rate as it exists in the CSynclSFHeader object
//	calling this function.  Therefore, make sure the intended sample rate has been written there
//	before calling this.
SynclSFTime CSynclSFHeader::sampleframes_to_time(unsigned int sampleframes) {
	SynclSFTime	time;
	double		frames = (double) sampleframes;								// samples
	double		rate   = ((double) mData.sample_rate) * 100.0;				// samples/second
	long long	msecs  = (long long) ((frames	* 1000000.0 / rate) + .5);	// microseconds

	if (mData.sample_rate == 0 && mData.period_index != 0)
		rate = 30000000.0 / (double) mData.period_index;
		
	time.seconds       = (UInt16) (msecs / 1000000);						// seconds	
	msecs %= 1000000;

	time.milliseconds  = (UInt16) (msecs / 1000);							// milliseconds
	msecs %= 1000;
	
	time.microseconds  = (UInt16) (msecs);									// microseconds

	return time;
}
