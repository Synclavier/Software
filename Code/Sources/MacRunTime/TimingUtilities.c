/* TimingUtilities.c */

// Standardized Interface to Mac O/S Timing Services

// Provides precise millisecond event timing.

#ifndef	COMPILE_OSX_KERNEL
	// 	C
	#include <StdIO.h>

	//	Mac OS Includes
	#include <Types.h>
	#include <DriverServices.h>
	#include <Timer.h>
	#include <DateTimeUtils.h>
	#include <LowMem.h>
#endif

// 	Local Includes
#include "Standard.h"
#include "TimingUtilities.h"

#ifdef	COMPILE_OSX_KERNEL
	// Compatibility macro - 10.2 used AbsoluteTimes; 10.4 uses UInt64s
	#define ADD_TIMES(t1,t2) (* (UInt64 *) (t1)) += (* (UInt64 *) (t2))

	#ifdef clock_get_uptime
		#define AbsTimeType		AbsoluteTime
		#define AbsTimePtr(x)	((uint64_t *) &(x))
		#define AbsTimeVal(x)	(*((uint64_t *) &(x)))
	#else
		#define AbsTimeType		UInt64
		#define AbsTimePtr(x)	((uint64_t *) &(x))
		#define AbsTimeVal(x)	(*((uint64_t *) &(x)))
	#endif
#endif

// Configuration
#define PRINT_RESULTS	0

#if defined(__ppc__)
	#define __eieio(a) __asm__ ("eieio");
#else
	#define __eieio(a)
#endif

// Global Variables
AbsoluteTimeUnion		TU_atic_time_of_last_msec_boundary;				// AbsoluteTime when simulated D16 wrapped
uint32					TU_atic_time_of_last_msec_boundary_fraction;	// fractional part thereof; not used except to reduce long term error

AbsoluteTimeUnion		TU_atic_time_of_next_msec_boundary;				// AbsoluteTime when simulated D16 will wrap next
uint32					TU_atic_time_of_next_msec_boundary_fraction;	// fractional part thereof; not used except to reduce long term error

UInt32					TU_next_atic_time_lsb;
UInt32*					TU_next_atic_time_lsb_ptr = &TU_next_atic_time_lsb;

uint32					TU_real_time_milliseconds;
uint64					TU_real_time_microseconds;

uint32					TU_nanos_per_atic_msb;							// nano seconds per absolute time tick: msb
uint32					TU_nanos_per_atic_lsb;							// nano seconds per absolute time tick: fractional part

uint32					TU_micros_per_atic_msb;							// micro seconds per absolute time tick: msb
uint32					TU_micros_per_atic_lsb;							// micro seconds per absolute time tick: fractional part

uint64					TU_atics_per_msec_msb;							// absolute time ticks per millisecond, msb
uint32					TU_atics_per_msec_lsb;							// absolute time ticks per millisecond, fractional part

UInt32					TU_minAbsoluteTimeDelta;						// used to fetch return values about hardware platform
UInt32 					TU_theAbsoluteTimeToNanosecondNumerator;
UInt32					TU_theAbsoluteTimeToNanosecondDenominator;
UInt32 					TU_theProcessorToAbsoluteTimeNumerator;
UInt32					TU_theProcessorToAbsoluteTimeDenominator;

// Assembly language math routines

DO not use with GCC bad object code

#if defined(powerc)														// PPC version
	uint32 TU_UnsignedFractionalMultiply32x32(register uint32 arg_a, register uint32 arg_b)
	{
		asm {
			mulhwu   r3,arg_a,arg_b
			blr
		}
		return 0;														// eliminate no return val warning...
	}
#else
	uint32 TU_UnsignedFractionalMultiply32x32(register uint32 arg_a, register uint32 arg_b)
	{
		return (uint32) ((((uint64) arg_a) * ((uint64) arg_b)) >> 32);
	}
#endif

// Simultation using TickCount()
static	AbsoluteTime	TU_GetProcessorTimeFromTickCount()
{
	AbsoluteTime value;
	
	value.hi = 0;
	value.lo = TickCount();
	
	return (value);
}

AbsoluteTime (*TU_GetProcessorTime)() = UpTime;

// Compute ratios
static	void compute_ratios(uint32 num, uint32 den)
{
	TU_nanos_per_atic_msb  = num / den;

	TU_nanos_per_atic_lsb  = (((uint64) (num % den)) << 32)
					       / (((uint64) (den)));

	TU_micros_per_atic_msb = ((uint64) num) / (((uint64) den) * ((uint64) 1000));

	TU_micros_per_atic_lsb = ((((uint64) num) % (((uint64) den) * ((uint64) 1000))) << 32)
					       / (((uint64) den) * ((uint64) 1000));

	TU_atics_per_msec_msb  = (((uint64) den) * ((uint64) 1000000)) / ((uint64) num);
	TU_atics_per_msec_lsb  = (((((uint64) den) * ((uint64) 1000000)) % ((uint64) num)) << 32)
					       / (((uint64) (num)));
}

// Timing Routines

// Initialize Timing Utilities
void	TU_InitializeTimingUtilities(uint32_ratio known_ratio)			// setup timing utilities
{
	AbsoluteTimeUnion	absTime;										// handy abs time
	NanosecondTimeUnion	nanoTime;										// handy nsec
	AbsoluteTimeUnion	newAbsTime;
	NanosecondTimeUnion	oneBillion;
	AbsoluteTimeUnion	deltaAbsTime;
	NanosecondTimeUnion deltaNanosecondTime;
	
	// Compute number of abs time ticks in 1,000,000,000 nanoseconds
	absTime.llt    = 0LL;												// Absolute time 0
	nanoTime.nst   = AbsoluteToNanoseconds(absTime.abt);				// Get Nanoseconds of Absolute TIme 0
	oneBillion.llt = 1000000000;										// One billion nanoseconds
	
	newAbsTime.abt = AddNanosecondsToAbsolute(absTime.abt, oneBillion.nst);
	
	deltaAbsTime.llt        = newAbsTime.llt - absTime.llt;				// Delta absolute time
	deltaNanosecondTime.llt = oneBillion.llt;							// Delta nanosecond time
	
	// Limit to ratio of reasonable 32-bit numbers
	while (((deltaAbsTime.llt | deltaNanosecondTime.llt) & 0xFFFFFFFFC0000000LL) != 0LL)
	{
		deltaAbsTime.llt        = (deltaAbsTime.llt       +1) >> 1;
		deltaNanosecondTime.llt = (deltaNanosecondTime.llt+1) >> 1;
	}
	
	TU_minAbsoluteTimeDelta					  = 1;
	
	THese are in the wrong order.
	
	Use TimingUtilitiesOSX.cpp
	
	DO NOT USE !!!!
	
	TU_theAbsoluteTimeToNanosecondNumerator   = (UInt32) deltaAbsTime.llt;			wrong
	TU_theAbsoluteTimeToNanosecondDenominator = (UInt32) deltaNanosecondTime.llt;	reversed
	TU_theProcessorToAbsoluteTimeNumerator	  = 1;
	TU_theProcessorToAbsoluteTimeDenominator  = 1;

	TU_GetProcessorTime = UpTime;										// and use UpTime
			
	if (known_ratio.num && known_ratio.den)								// substitute calibrated time if available
	{
		TU_theAbsoluteTimeToNanosecondNumerator   = known_ratio.num;
		TU_theAbsoluteTimeToNanosecondDenominator = known_ratio.den;
	}

	// Compute ratios - nano seconds per absolute tick, microseconds per absolute tick, absolute ticks per millisecond
	compute_ratios(TU_theAbsoluteTimeToNanosecondNumerator, TU_theAbsoluteTimeToNanosecondDenominator);
	
	// Initialize sync point: 0 TU_real_time_milliseconds = current processor uptime
	TU_next_atic_time_lsb_ptr = &TU_next_atic_time_lsb;

	TU_atic_time_of_last_msec_boundary.abt      = TU_GetProcessorTime();
	TU_atic_time_of_last_msec_boundary_fraction = 0;

	TU_atic_time_of_next_msec_boundary.llt      = TU_atic_time_of_last_msec_boundary.llt + TU_atics_per_msec_msb;
	TU_atic_time_of_next_msec_boundary_fraction = TU_atics_per_msec_lsb;
	*TU_next_atic_time_lsb_ptr                  = TU_atic_time_of_next_msec_boundary.abt.lo;			// publish new next atic lsb

	TU_real_time_milliseconds					= 0;
}

// Publish a new ratio
void	TU_PublishNewRatios(uint32_ratio known_ratio)
{
	if (known_ratio.num && known_ratio.den)
	{
		TU_theAbsoluteTimeToNanosecondNumerator   = known_ratio.num;
		TU_theAbsoluteTimeToNanosecondDenominator = known_ratio.den;
		compute_ratios(TU_theAbsoluteTimeToNanosecondNumerator, TU_theAbsoluteTimeToNanosecondDenominator);
	}
}


// Advance real time milliseconds according to processor uptime
void TU_AdvanceRealTimeMilliseconds()
{
	AbsoluteTimeUnion	a;
	
	a.abt = TU_GetProcessorTime();																		// get current UpTime

	while (a.llt >= TU_atic_time_of_next_msec_boundary.llt)												// advance milliseconds as needed; ignore fractional atick
	{
		TU_atic_time_of_last_msec_boundary.llt      = TU_atic_time_of_next_msec_boundary.llt;
		TU_atic_time_of_last_msec_boundary_fraction = TU_atic_time_of_next_msec_boundary_fraction;
	
		TU_atic_time_of_next_msec_boundary_fraction += TU_atics_per_msec_lsb;							// pre-compute next one
		
		if (TU_atic_time_of_next_msec_boundary_fraction < TU_atics_per_msec_lsb)						// handle carry into msb
			TU_atic_time_of_next_msec_boundary.llt += (TU_atics_per_msec_msb + 1);
		else
			TU_atic_time_of_next_msec_boundary.llt += (TU_atics_per_msec_msb    );						// else handle no carry
		
		*TU_next_atic_time_lsb_ptr = TU_atic_time_of_next_msec_boundary.abt.lo;							// publish new next atic lsb
	
		TU_real_time_milliseconds++;
	}
}

// Advance real time milliseconds according to processor uptime
#if defined(powerc) do not use
	static	asm	void TU_ReadTBR (AbsoluteTime &it) __attribute__ ((noinline));
	
	static	asm void TU_ReadTBR(AbsoluteTime &it)
	{
		nofralloc
		
		@start:
		mftbu      r4
		mftb       r5
		mftbu      r6
		cmplw      r4,r6
		bne-       @start
		stw        r4,0x0000(r3)
		stw        r5,0x0004(r3)
		blr
	}
#else
	static	void 	TU_ReadTBR(AbsoluteTime &it)
	{
		it = UpTime();
	}
#endif

void TU_FastAdvanceRealTimeMilliseconds()
{
	AbsoluteTimeUnion	a;
	
	TU_ReadTBR(a.abt);																				// get current UpTime quickly

	while (a.llt >= TU_atic_time_of_next_msec_boundary.llt)											// advance milliseconds as needed; ignore fractional atick
	{
		TU_atic_time_of_last_msec_boundary.llt      = TU_atic_time_of_next_msec_boundary.llt;
		TU_atic_time_of_last_msec_boundary_fraction = TU_atic_time_of_next_msec_boundary_fraction;
	
		TU_atic_time_of_next_msec_boundary_fraction += TU_atics_per_msec_lsb;						// pre-compute next one
		
		if (TU_atic_time_of_next_msec_boundary_fraction < TU_atics_per_msec_lsb)					// handle carry into msb
			TU_atic_time_of_next_msec_boundary.llt += (TU_atics_per_msec_msb + 1);
		else
			TU_atic_time_of_next_msec_boundary.llt += (TU_atics_per_msec_msb    );					// else handle no carry
		
		*TU_next_atic_time_lsb_ptr = TU_atic_time_of_next_msec_boundary.abt.lo;						// publish new next atic lsb
		
		TU_real_time_milliseconds++;
	}
}

// Compute microseconds that have elapsed since last millisecond boundary
uint32 TU_ComputeElapsedMicroseconds()
{
	uint32 				atics;
	AbsoluteTimeUnion	a;

	a.abt = TU_GetProcessorTime();																		// get current up time
	atics = (uint32) (a.llt - TU_atic_time_of_last_msec_boundary.llt);									// get atics since last posted interrupt

	return ((atics * TU_micros_per_atic_msb)															// compute microseconds since interrupt
		   +(TU_UnsignedFractionalMultiply32x32(TU_micros_per_atic_lsb, atics)));
}

// Compute microsecond time base
// Note: designed to provide for re-entrancy
// Note: assumes TU_AdvanceRealTimeMilliseconds is being called
uint64 TU_ComputeMicrosecondTimeBase()
{
	uint32 prior_TU_real_time_milliseconds;
	uint32 microseconds;
	uint64 value;
	
	do {
		prior_TU_real_time_milliseconds = TU_real_time_milliseconds;									// sample time base
		microseconds                    = TU_ComputeElapsedMicroseconds();								// compute offset; may get interrupted here

		value = (((long long) (TU_real_time_milliseconds)) * ((long long) 1000)) + ((long long) microseconds);
	} while (prior_TU_real_time_milliseconds != TU_real_time_milliseconds);

	return (value);
}

// Measure UpTime accuracy.  Done by timing UpTime against
// the real-time-clock for 30 seconds
uint32_ratio TU_MeasureUPTimeUsingTimeOfDay()
{
	AbsoluteTimeUnion	startingATime, nowATime;
	uint32 				startingSeconds, now_seconds, prior_seconds;
	OSErr  				status;
	int					i;
	uint64				theDuration, theRealtime;
	uint32_ratio		result = {0, 0};
	
	if (UpTime != 0)								// init to UpTime if available
		TU_GetProcessorTime = UpTime;

	status = ReadDateTime (&now_seconds);
	
	if (status)
		return result;
				
	// Sync starting time to second boundary
	ReadDateTime (&startingSeconds);
	startingATime.abt = TU_GetProcessorTime();
	
	while (now_seconds == startingSeconds)
	{
		ReadDateTime (&startingSeconds);
		startingATime.abt = TU_GetProcessorTime();
	}
	
	// Time for 30 seconds
	now_seconds = startingSeconds;
	for (i=0; i<30; i++)
	{
		prior_seconds = now_seconds;
		while (now_seconds == prior_seconds)
		{
			ReadDateTime (&now_seconds);
			nowATime.abt = TU_GetProcessorTime();
		}
	}
	
	theDuration = nowATime.llt - startingATime.llt;
	theRealtime = 30000000000LL;

	// Make to fit into ratio of two 32 bit values
	while (theDuration > 100000000 || theRealtime > 100000000)
	{
		theDuration = ((theDuration + 1) >> 1);
		theRealtime = ((theRealtime + 1) >> 1);
	}
	
	result.num = (uint32) theRealtime;					// nanoseconds
	result.den = (uint32) theDuration;					// absolute time
	
	if (PRINT_RESULTS)
		printf("TU_MeasureUPTimeUsingTimeOfDay got %f\n", (double) theRealtime / (double) theDuration);

	return result;
}


// Measure UpTime accuracy.  Done by timing UpTime against
// the real-time-clock for 30 seconds
uint32_ratio TU_MeasureUPTimeUsingMicroseconds()
{
	AbsoluteTimeUnion	startingATime, nowATime;
	UnsignedWide		startingSeconds, now_seconds;
	long				startTicks;
	uint64				theDuration, theRealtime;
	uint32_ratio		result = {0, 0};
	
	if (UpTime != 0)									// init to UpTime if available
		TU_GetProcessorTime = UpTime;

	// Latch starting time
	Microseconds (&startingSeconds);
	startingATime.abt = TU_GetProcessorTime();
	
	// Latch starting time twice to run from cache
	Microseconds (&startingSeconds);
	startingATime.abt = TU_GetProcessorTime();
	
	// Time for 30 seconds
	startTicks = TickCount();
	
	while (TickCount() < startTicks + 1800)
	{
		long	now = TickCount();
		
		Microseconds (&now_seconds);
		nowATime.abt = TU_GetProcessorTime();

		while	(TickCount() == now)
			;
	}

	// Latch ending time
	Microseconds (&now_seconds);
	nowATime.abt = TU_GetProcessorTime();
	
	theDuration = nowATime.llt - startingATime.llt;		// Duration, absolute processor time
	theRealtime = ((uint64) 1000) * ((* (uint64*) &now_seconds) - (* (uint64*) &startingSeconds));

	// Make to fit into ratio of two 32 bit values
	while (theDuration > 100000000 || theRealtime > 100000000)
	{
		theDuration = ((theDuration + 1) >> 1);
		theRealtime = ((theRealtime + 1) >> 1);
	}
	
	result.num = (uint32) theRealtime;					// nanoseconds
	result.den = (uint32) theDuration;					// absolute time
	
	if (PRINT_RESULTS)
		printf("TU_MeasureUPTimeUsingMicroseconds got %f\n", (double) theRealtime / (double) theDuration);

	return result;
}


// Measure UpTime accuracy.  Done by timing UpTime against
// the real-time-clock for 30 seconds
uint32_ratio TU_MeasureUPTimeUsingTickCount()
{
	AbsoluteTimeUnion	startingATime, nowATime;
	long				startingSeconds, now_seconds;
	long				startTicks, endTicks, nowTicks;
	uint64				theDuration, theRealtime;
	uint32_ratio		result = {0, 0};
	
	if (UpTime != 0)									// init to UpTime if available
		TU_GetProcessorTime = UpTime;

	// Sync to TickCount()
	startTicks = TickCount();
	nowTicks   = TickCount();
	
	while (nowTicks == startTicks)
		nowTicks = TickCount();
	
	startingATime.abt = TU_GetProcessorTime();

	// Sync to TickCount() again
	startTicks = TickCount();
	nowTicks   = TickCount();
	
	while (nowTicks == startTicks)
		nowTicks = TickCount();
	
	startingATime.abt = TU_GetProcessorTime();
	startingSeconds = nowTicks;

	// Time for 30 seconds
	nowTicks = TickCount();
	while (nowTicks < startingSeconds + 1800)
		nowTicks = TickCount();

	// Sync to TickCount() again
	endTicks = TickCount();
	nowTicks = TickCount();
	
	while (nowTicks == endTicks)
		nowTicks = TickCount();
	
	nowATime.abt = TU_GetProcessorTime();
	now_seconds  = nowTicks;
	
	theDuration = nowATime.llt - startingATime.llt;		// Duration, absolute processor time
	theRealtime = ((uint64) 16666667) * (((uint64) now_seconds) - ((uint64) startingSeconds));

	// Make to fit into ratio of two 32 bit values
	while (theDuration > 100000000 || theRealtime > 100000000)
	{
		theDuration = ((theDuration + 1) >> 1);
		theRealtime = ((theRealtime + 1) >> 1);
	}
	
	result.num = (uint32) theRealtime;					// nanoseconds
	result.den = (uint32) theDuration;					// absolute time
	
	if (PRINT_RESULTS)
		printf("TU_MeasureUPTimeUsingTickCount got %f\n", (double) theRealtime / (double) theDuration);

	return result;
}


// Measure UpTime accuracy.  Done by timing UpTime against
// the D16 timer
uint32_ratio TU_MeasureUPTimeUsingD16Timer(volatile unsigned long* fifoLoc, unsigned long fifoData)
{
	AbsoluteTimeUnion	startingATime, nowATime;
	long				startingSeconds;
	long				priorD16, startingD16, endingD16;
	long				startTicks, nowTicks, lastTicks;
	long				startD16, nowD16;
	uint64				theDuration, theRealtime;
	uint32_ratio		result = {0, 0};
	
	if (UpTime != 0)									// init to UpTime if available
		TU_GetProcessorTime = UpTime;

	// Sync to processor ticks once to load processor cache.
	
	// Sync to TickCount() boundary first (e.g. 16.67 msecs) for 30 second timing
	startTicks = TickCount();
	nowTicks   = TickCount();
	
	while (nowTicks == startTicks)
		nowTicks = TickCount();
	
	// Sync to D16 boundary next (e.g. 1 microsecond, 10 microsecond, 100 microsecond or 1 millisecond)
	*fifoLoc = fifoData;								// get d16 reading
	__eieio();
	startD16 = (*fifoLoc) & 0xFFFF;
	__eieio();
	
	*fifoLoc = fifoData;								// get d16 reading
	__eieio();
	nowD16 = (*fifoLoc) & 0xFFFF;
	__eieio();
	
	while (nowD16 == startD16)
	{
		*fifoLoc = fifoData;							// get d16 reading
		__eieio();
		nowD16 = (*fifoLoc) & 0xFFFF;
		__eieio();
	}
	
	startingATime.abt = TU_GetProcessorTime();			// get processor time
	
	// Sync to TickCount() again now that processor caches are laoded
	startTicks = TickCount();
	nowTicks   = TickCount();
	
	while (nowTicks == startTicks)
		nowTicks = TickCount();
	
	// Sync to D16 boundary next (e.g. 1 microsecond, 10 microsecond, 100 microsecond or 1 millisecond)
	*fifoLoc = fifoData;								// get d16 reading
	__eieio();
	startD16 = (*fifoLoc) & 0xFFFF;
	__eieio();
	
	*fifoLoc = fifoData;								// get d16 reading
	__eieio();
	nowD16 = (*fifoLoc) & 0xFFFF;
	__eieio();
	
	while (nowD16 == startD16)
	{
		*fifoLoc = fifoData;							// get d16 reading
		__eieio();
		nowD16 = (*fifoLoc) & 0xFFFF;
		__eieio();
	}
	
	startingATime.abt = TU_GetProcessorTime();			// get processor time

	// Latch starting info
	startingSeconds = nowTicks;
	lastTicks       = nowTicks;
	priorD16        = nowD16;
	startingD16     = nowD16;
	endingD16       = nowD16;
	
	// Time for 30 seconds
	nowTicks  = TickCount();

	while (nowTicks < startingSeconds + 1800)
	{
		nowTicks = TickCount();
		
		// Check for wrap every tickcount
		if (nowTicks != lastTicks)
		{
			*fifoLoc = fifoData;						// get d16 reading
			__eieio();
			startD16 = (*fifoLoc) & 0xFFFF;
			__eieio();
			
			*fifoLoc = fifoData;						// get d16 reading
			__eieio();
			nowD16 = (*fifoLoc) & 0xFFFF;
			__eieio();
			
			while (nowD16 == startD16)
			{
				*fifoLoc = fifoData;					// get d16 reading
				__eieio();
				nowD16 = (*fifoLoc) & 0xFFFF;
				__eieio();
			}
			
			nowATime.abt = TU_GetProcessorTime();		// get processor time
		
			if (nowD16 < priorD16)
				endingD16 += (nowD16 + 65536 - priorD16);
			else
				endingD16 += (nowD16         - priorD16);
				
			priorD16  = nowD16;
			lastTicks = nowTicks;
		}
	}

	// Measure time	
	theDuration = nowATime.llt - startingATime.llt;					// Duration, absolute processor time
	theRealtime = ((uint64) endingD16) - ((uint64) startingD16);	// Real time (d16-ticks)
	
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
	
	if (PRINT_RESULTS)
		printf("TU_MeasureUPTimeUsingD16Timer got %f\n", (double) theRealtime / (double) theDuration);

	return result;
}


// Measure UpTime accuracy.  Done by timing UpTime against
// the D03 timer
uint32_ratio TU_MeasureUPTimeUsingD03Timer(volatile unsigned long* fifoLoc, unsigned long fifoData, unsigned long fifoWrites)
{
	AbsoluteTimeUnion	startingATime, nowATime;
	long				startingSeconds;
	long				endingD03;
	long				startTicks, nowTicks;
	long				startD03;
	uint64				theDuration, theRealtime;
	uint32_ratio		result = {0, 0};
	
	if (UpTime != 0)									// init to UpTime if available
		TU_GetProcessorTime = UpTime;

	// Sync to processor ticks once to load processor cache.
	
	// Sync to TickCount() boundary first (e.g. 16.67 msecs) for 30 second timing
	startTicks = TickCount();
	nowTicks   = TickCount();
	
	while (nowTicks == startTicks)
		nowTicks = TickCount();
	
	// Sync to D03 boundary next (e.g. 5 millisecond boundary)
	*fifoLoc = fifoWrites;								// clear D03 bit
	__eieio();

	*fifoLoc = fifoData;								// get d03 reading
	__eieio();
	startD03 = (*fifoLoc) & 1;
	__eieio();
	
	while (startD03 == 0)
	{
		*fifoLoc = fifoData;							// get d03 reading
		__eieio();
		startD03 = (*fifoLoc) & 1;
		__eieio();
	}

	startingATime.abt = TU_GetProcessorTime();			// get processor time
	
	*fifoLoc = fifoWrites;								// clear D03 bit
	__eieio();

	// Sync to TickCount() again now that processor caches are laoded
	startTicks = TickCount();
	nowTicks   = TickCount();
	
	while (nowTicks == startTicks)
		nowTicks = TickCount();
	
	// Sync to D03 boundary next (e.g. 5 millisecond boundary)
	*fifoLoc = fifoWrites;								// clear D03 bit
	__eieio();

	*fifoLoc = fifoData;								// get d03 reading
	__eieio();
	startD03 = (*fifoLoc) & 1;
	__eieio();
	
	while (startD03 == 0)
	{
		*fifoLoc = fifoData;							// get d03 reading
		__eieio();
		startD03 = (*fifoLoc) & 1;
		__eieio();
	}

	startingATime.abt = TU_GetProcessorTime();			// get processor time

	*fifoLoc = fifoWrites;								// clear D03 bit
	__eieio();

	// Latch starting info
	startingSeconds = nowTicks;
	endingD03       = 0;
	
	// Time for 30 seconds
	nowTicks  = TickCount();

	while (nowTicks < startingSeconds + 1800)
	{
		*fifoLoc = fifoData;								// get d03 reading
		__eieio();
		startD03 = (*fifoLoc) & 1;
		__eieio();
		
		while (startD03 == 0)
		{
			*fifoLoc = fifoData;							// get d03 reading
			__eieio();
			startD03 = (*fifoLoc) & 1;
			__eieio();
		}

		nowATime.abt = TU_GetProcessorTime();				// get processor time
		endingD03 += 5000;									// time advanced by 5000 microseconds

		*fifoLoc = fifoWrites;								// clear D03 bit
		__eieio();
		
		nowTicks = TickCount();
	}

	// Measure time	
	theDuration = nowATime.llt - startingATime.llt;			// Duration, absolute processor time
	theRealtime = ((uint64) endingD03) * ((uint64) 1000);	// Real time (nanoseconds)
	
	// Make to fit into ratio of two 32 bit values
	while (theDuration > 100000000 || theRealtime > 100000000)
	{
		theDuration = ((theDuration + 1) >> 1);
		theRealtime = ((theRealtime + 1) >> 1);
	}
	
	result.num = (uint32) theRealtime;					// nanoseconds
	result.den = (uint32) theDuration;					// absolute time
	
	if (PRINT_RESULTS)
		printf("TU_MeasureUPTimeUsingD03Timer got %f\n", (double) theRealtime / (double) theDuration);

	return result;
}