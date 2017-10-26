/* TimingUtilitiesOSX.c */

// Timing utilites - OSX User Space and Kernel version

#include "TimingUtilitiesOSX.h"

#ifdef	COMPILE_OSX_KERNEL
    #define DO_PRINTING 0
	#define PRINT_STYLE IOLog
    #include <libkern/OSByteOrder.h>
#else
	#include <StdIO.h>

	#define DO_PRINTING 0
	#define PRINT_STYLE printf
#endif

// Global Variables
TU_nanos		TU_atic_time_root;								// AbsoluteTime of time == 0
TU_nanos		TU_atic_time_of_last_msec_boundary;				// AbsoluteTime when simulated D16 wrapped
TU_nanos		TU_atic_time_of_next_msec_boundary;				// AbsoluteTime when simulated D16 will wrap next

unsigned int    TU_next_atic_time_lsb;
unsigned int*	TU_next_atic_time_lsb_ptr = &TU_next_atic_time_lsb;

uint32          TU_real_time_milliseconds;                      // Counts Synclavier real time milliseconds
uint32          TU_host_nsecs_per_real_time_msec;               // Nanoseconds of CPU time per millisecond of Synclavier real time

uint32          TU_100micro_shift_factor;						// shift right factor to scale processor host time to approximately 100 microseconds per tick
uint32          TU_host_nsecs_per_100micros;                    // host processor ticks in 100 microseconds of Synclavier real time
uint32          TU_100micro_magnitude;							// smallest power of 2 > TU_host_nsecs_per_100micros

// Compute ratios
//	Note: num / den == synclavier nanosecond per computer nanosecond
//        den / num == computer nanosecond   per synclavier nanosecond

// result.num = (uint32) theRealtime;   // synclavier nanoseconds
// result.den = (uint32) theDuration;   // computer   nanoseconds

// This routine is used to implement the Synclavier calibrated metronome function.
// This has nothing to do with the mach time base numerator/denominator computations.
// The metronome calibration works by comparing the computer generated nanosecond time base with the reference standard.
// Here we microscopically further adjust the nanosecond time base to match the real time of the reference.
static	void compute_ratios(uint32 num, uint32 den)
{
    TU_host_nsecs_per_real_time_msec = (uint32) (((((TU_nanos) 1000000LL) * ((TU_nanos) den)) + ((TU_nanos) (num >> 1))) / ((TU_nanos) num));
	
    // Used to fit times of 0 - 25 msecs in an 8-bit byte with .1 msec resolution.
    // This is used to pass MIDI time stamps into the Synclavier recording engine.
    TU_host_nsecs_per_100micros = (uint32) ((TU_host_nsecs_per_real_time_msec + 5LL) / 10LL);
	TU_100micro_shift_factor    = 0;
	TU_100micro_magnitude       = 1;
	
	while (TU_100micro_magnitude != 0 && TU_100micro_magnitude < TU_host_nsecs_per_100micros)
	{
		TU_100micro_magnitude    <<= 1;
		TU_100micro_shift_factor ++;
	}
}

// Timing Routines

// Initialize Timing Utilities
void	TU_InitializeTimingUtilities(uint32_ratio known_ratio)
{
    // Initialize sync point: 0 TU_real_time_milliseconds = current processor uptime
    TU_atic_time_root                  = TU_GetProcessorTime();                                 // AbsoluteTime of time == 0
    TU_atic_time_of_last_msec_boundary = TU_atic_time_root;                                     // AbsoluteTime when simulated D16 wrapped
    TU_atic_time_of_next_msec_boundary = TU_atic_time_of_last_msec_boundary + 1000000LL;        // AbsoluteTime when simulated D16 will wrap next
    
    TU_next_atic_time_lsb              = (unsigned int) TU_atic_time_of_last_msec_boundary;
    TU_next_atic_time_lsb_ptr          = &TU_next_atic_time_lsb;
    
    TU_real_time_milliseconds          = 0;                                                     // Millisecond real time counter
    
    if (known_ratio.num && known_ratio.den)                                                     // substitute calibrated time if available
        compute_ratios(known_ratio.num, known_ratio.den);
    else
        compute_ratios(1, 1);
}

// Publish a new ratio
void	TU_PublishNewRatios(uint32_ratio known_ratio)
{
	if (known_ratio.num && known_ratio.den)
		compute_ratios(known_ratio.num, known_ratio.den);
}

// Advance real time milliseconds according to processor uptime
void TU_FastAdvanceRealTimeMilliseconds()
{
	TU_nanos a = TU_GetProcessorTime();                                                         // get current UpTime

	while (a >= TU_atic_time_of_next_msec_boundary)												// advance milliseconds as needed; ignore fractional atick
	{
		TU_atic_time_of_last_msec_boundary  = TU_atic_time_of_next_msec_boundary;
		TU_atic_time_of_next_msec_boundary += TU_host_nsecs_per_real_time_msec;                 // pre-compute next one
		
		*TU_next_atic_time_lsb_ptr = (unsigned int) TU_atic_time_of_next_msec_boundary;
	
		TU_real_time_milliseconds++;                                                            // one more msec in the hopper
	}
}

// C versions of fast timing routines
uint32 TU_UnsignedFractionalMultiply32x32(uint32 arg_a, uint32 arg_b)
{
    return (uint32) ((((uint64) arg_a) * ((uint64) arg_b)) >> 32);
}

int TU_PerformLinearComparison(unsigned int val1, unsigned int val2)                            // returns true if val1 >= val2 without regard to overflow
{
    unsigned int dif = val1 - val2;
    
    if ( (dif & 0x80000000) == 0)
        return 1;
    else
        return 0;
}

// Compute nanosecond time base
static  struct mach_timebase_info   timebase_info           = {0, 0};
static  bool                        timebase_is_nanoseconds = false;

TU_nanos TU_ComputeNanosecondTimeBase()
{
    TU_nanos now;
    
    #if COMPILE_OSX_KERNEL
        clock_get_uptime(&now);
    #else
        now = mach_absolute_time();
    #endif
    
    if (timebase_is_nanoseconds) return now;
    
    if (timebase_info.denom == 0) {
        #if COMPILE_OSX_KERNEL
            clock_timebase_info(&timebase_info);
        #else
            mach_timebase_info(&timebase_info);
        #endif
    }
    
    if (timebase_info.numer == timebase_info.denom) {
        timebase_is_nanoseconds = true;
        return now;
    }
    
    // host ticks * numer / denom = nanoseconds
    return (now * ((TU_nanos) timebase_info.numer)) / ((TU_nanos) timebase_info.denom);
}

TU_nanos TU_ComputeMicrosecondTimeBase()
{
    return TU_ComputeNanosecondTimeBase() / ((TU_nanos) 1000LL);
}

// Measure UpTime accuracy.  Done by timing UpTime against
// the D16 timer
uint32_ratio TU_MeasureUPTimeUsingD16Timer(volatile unsigned int* fifoLoc, unsigned int fifoData)
{
	TU_nanos        startingATime, nowATime;
	TU_nanos        startingSeconds, nowTicks, lastTicks;
	int             priorD16, startingD16, endingD16;
	int             startD16, nowD16;
	TU_nanos        theDuration, theRealtime;
	uint32_ratio    result = {0, 0};
	
	// Sync to D16 boundary next (e.g. 1 microsecond, 10 microsecond, 100 microsecond or 1 millisecond)
	*fifoLoc = fifoData;								// get d16 reading; note fifoData us akreadt big-endian
	startD16 = OSSwapHostToBigInt32(*fifoLoc) & 0xFFFF;
	
	*fifoLoc = fifoData;								// get d16 reading
	nowD16 = OSSwapHostToBigInt32(*fifoLoc) & 0xFFFF;
	
	while (nowD16 == startD16)
	{
		*fifoLoc = fifoData;							// get d16 reading
        nowD16   = OSSwapHostToBigInt32(*fifoLoc) & 0xFFFF;
    }
	
	nowATime = startingATime = TU_GetProcessorTime();   // get processor time
	
	// Sync to D16 boundary again once processor caches are loaded
	*fifoLoc = fifoData;								// get d16 reading
	startD16 = OSSwapHostToBigInt32(*fifoLoc) & 0xFFFF;
	
	*fifoLoc = fifoData;								// get d16 reading
	nowD16 = OSSwapHostToBigInt32(*fifoLoc) & 0xFFFF;
	
	while (nowD16 == startD16)
	{
		*fifoLoc = fifoData;							// get d16 reading
        nowD16 = OSSwapHostToBigInt32(*fifoLoc) & 0xFFFF;
    }
	
	startingATime = TU_GetProcessorTime();              // get processor time

	// Latch starting info
	startingSeconds = startingATime;
	lastTicks       = startingATime;
	priorD16        = nowD16;
	startingD16     = nowD16;
	endingD16       = nowD16;
	
	// Time for 30 seconds
	nowTicks = TU_GetProcessorTime();

	while (nowTicks < startingSeconds + 30000000000LL)
	{
		nowTicks = TU_GetProcessorTime();
		
		// Check for wrap every 10 msecs to avoid D16 timer wrap. Most D16s were set to count microseconds.
        if (nowTicks > lastTicks + 10000000LL)
		{
			*fifoLoc = fifoData;						// get d16 reading
			startD16 = OSSwapHostToBigInt32(*fifoLoc) & 0xFFFF;
					
			*fifoLoc = fifoData;						// get d16 reading
            nowD16   = OSSwapHostToBigInt32(*fifoLoc) & 0xFFFF;
					
            // Find the next D16 tick now that 10 msecs has gone by
			while (nowD16 == startD16)
			{
				*fifoLoc = fifoData;					// get d16 reading
				nowD16   = OSSwapHostToBigInt32(*fifoLoc) & 0xFFFF;
			}
			
			nowATime = TU_GetProcessorTime();           // get processor time
		
			if (nowD16 < priorD16)
				endingD16 += (nowD16 + 65536 - priorD16);
			else
				endingD16 += (nowD16         - priorD16);
				
			priorD16  = nowD16;
			lastTicks = nowTicks;
		}
	}

    // Measure time
	theDuration = nowATime               - startingATime;				// Duration, nanoseconds
	theRealtime = ((TU_nanos) endingD16) - ((TU_nanos) startingD16);	// Real time (d16-ticks)
	
	// Convert to microseconds.  1 msec timer would yield 30,000 D16 ticks, etc.
	if      (theRealtime <    100000ll) theRealtime *= 1000000ll;				// Milliseconds to nanoseconds
	else if (theRealtime <   1000000ll) theRealtime *=  100000ll;				// .1 mseconds to nanoseconds
	else if (theRealtime <  10000000ll) theRealtime *=   10000ll;				// .01 mseconds to nanoseconds
	else if (theRealtime < 100000000ll) theRealtime *=    1000ll;				// .001 mseconds to nanoseconds

	// Make to fit into ratio of two 32 bit values
	while (theDuration > 100000000 || theRealtime > 100000000)
	{
		theDuration = ((theDuration + 1) >> 1);
		theRealtime = ((theRealtime + 1) >> 1);
	}
	
	result.num = (uint32) theRealtime;					// nanoseconds
	result.den = (uint32) theDuration;					// absolute time
	
	#if DO_PRINTING
		PRINT_STYLE("TU_MeasureUPTimeUsingD16Timer got %d / %d\n", result.num, result.den);
	#endif
	
	return result;
}


// Measure UpTime accuracy.  Done by timing UpTime against
// the D03 timer
uint32_ratio TU_MeasureUPTimeUsingD03Timer(volatile unsigned int* fifoLoc, unsigned int fifoData, unsigned int fifoWrites)
{
	TU_nanos        startingATime, nowATime;
    TU_nanos        startingSeconds, nowTicks;
	int             startD03, endingD03;
	TU_nanos        theDuration, theRealtime;
	uint32_ratio    result = {0, 0};
	
	// Sync to D03 boundary next (e.g. 5 millisecond boundary)
	*fifoLoc = fifoWrites;								// clear D03 bit; note fifoData and fifoWrites are big-endian already

    *fifoLoc = fifoData;								// get d03 reading
	startD03 = OSSwapHostToBigInt32(*fifoLoc) & 1;
	
	while (startD03 == 0)
	{
		*fifoLoc = fifoData;							// get d03 reading
		startD03 = OSSwapHostToBigInt32(*fifoLoc) & 1;
	}

    *fifoLoc = fifoWrites;								// clear D03 bit

    nowATime = startingATime = TU_GetProcessorTime();   // get processor time
	
    // Sync to D03 boundary next (e.g. 5 millisecond boundary) (once processor caches are loaded)
	*fifoLoc = fifoWrites;								// clear D03 bit

	*fifoLoc = fifoData;								// get d03 reading
	startD03 = OSSwapHostToBigInt32(*fifoLoc) & 1;
	
	while (startD03 == 0)
	{
		*fifoLoc = fifoData;							// get d03 reading
		startD03 = OSSwapHostToBigInt32(*fifoLoc) & 1;
	}

    *fifoLoc = fifoWrites;								// clear D03 bit
    
	startingATime = TU_GetProcessorTime();              // get processor time

	// Latch starting info
	startingSeconds = startingATime;
	endingD03       = 0;
	
	// Time for 30 seconds
	nowTicks = TU_GetProcessorTime();

	while (nowTicks < startingSeconds + 30000000000LL)
	{
		*fifoLoc = fifoData;								// get d03 reading
		startD03 = OSSwapHostToBigInt32(*fifoLoc) & 1;
		
		while (startD03 == 0)
		{
			*fifoLoc = fifoData;							// get d03 reading
			startD03 = OSSwapHostToBigInt32(*fifoLoc) & 1;
		}

        *fifoLoc = fifoWrites;								// clear D03 bit
		
        nowATime = TU_GetProcessorTime();                   // get processor time
		endingD03 += 5000;									// time advanced by 5000 microseconds

		nowTicks = TU_GetProcessorTime();
	}

	// Measure time	
	theDuration = nowATime - startingATime;                     // Duration, nanoseconds
	theRealtime = ((TU_nanos) endingD03) * ((TU_nanos) 1000);	// Real time (nanoseconds)
	
	// Make to fit into ratio of two 32 bit values
	while (theDuration > 100000000 || theRealtime > 100000000)
	{
		theDuration = ((theDuration + 1) >> 1);
		theRealtime = ((theRealtime + 1) >> 1);
	}
	
	result.num = (uint32) theRealtime;					// nanoseconds
	result.den = (uint32) theDuration;					// absolute time
	
	#if DO_PRINTING
		PRINT_STYLE("TU_MeasureUPTimeUsingD03Timer got %d / %d\n", result.num, result.den);
	#endif
	
	return result;
}

// Measure UpTime accuracy.  Done by timing UpTime against
// the D03 timer
bool            TU_Abort_Measurement;

uint32_ratio	TU_MeasureUPTimeUsingD03Timer(TU_CheckD03 d3_reader)            // Calibrate using D03 timer - matches Model D implementation
{
    TU_nanos        startingATime, nowATime;
    TU_nanos        startingSeconds, nowTicks;
    int             endingD03;
    TU_nanos        theDuration, theRealtime;
    uint32_ratio    result = {0, 0};
    
    // Sync to D03 boundary next (e.g. 5 millisecond boundary)
    while (!d3_reader())    // The first one just clears the pending count
        ;
    
    while (!d3_reader())    // The second one will wait from 0 to 5 msecs.
        ;
    
    while (!d3_reader())    // The third one will wait 5 msecs.
        ;
    
    nowATime = startingATime = TU_GetProcessorTime();   // get processor time
    
    // Sync to D03 boundary next (e.g. 5 millisecond boundary) (once processor caches are loaded)
    while (!d3_reader())
        ;
    
    startingATime = TU_GetProcessorTime();              // get processor time
    
    // Latch starting info
    startingSeconds = startingATime;
    endingD03       = 0;
    
    // Time for 30 seconds
    nowTicks = TU_GetProcessorTime();
    
    while (nowTicks < startingSeconds + 30000000000LL)
    {
        while (!d3_reader())
            ;
        
        nowATime = TU_GetProcessorTime();                   // get processor time
        endingD03 += 5000;									// time advanced by 5000 microseconds
        
        nowTicks = TU_GetProcessorTime();
        
        if (TU_Abort_Measurement)
            return result;
    }
    
    // Measure time
    theDuration = nowATime - startingATime;                     // computer   duration nanoseconds
    theRealtime = ((TU_nanos) endingD03) * ((TU_nanos) 1000);	// synclavier duration nanoseconds
    
    // Make to fit into ratio of two 32 bit values
    while (theDuration > 1000000000LL || theRealtime > 1000000000LL)
    {
        theDuration = ((theDuration + 1) >> 1);
        theRealtime = ((theRealtime + 1) >> 1);
    }
    
    result.num = (uint32) theRealtime;					// synclavier nanoseconds
    result.den = (uint32) theDuration;					// computer   nanoseconds
    
    // For example, say your computer is running at 1/2 speed.
    // Read the D3 for 30 computer seconds
    // Synclavier advances 60 seconds.
    // Ratio would be 2:1
    
    #if DO_PRINTING
        PRINT_STYLE("TU_MeasureUPTimeUsingD03Timer got %d / %d\n", result.num, result.den);
    #endif
    
    return result;
}
