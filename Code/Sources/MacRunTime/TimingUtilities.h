// TimingUtilities.h

#ifndef	__TIMINGUTILITIES__
#define __TIMINGUTILITIES__

//	Mac OS Includes
#ifdef	COMPILE_OSX_KERNEL
	#include <IOKit/IOLib.h>
	#include <kern/clock.h>
	typedef uint64_t Nanoseconds;
#else
	#include <Types.h>
	#include <DriverServices.h>
#endif

// 	Local Includes
#include "Standard.h"

// Structs
typedef union			AbsoluteTimeUnion												// handy union to access absolute time as long longs
{
	AbsoluteTime		abt;
	uint64				llt;
}	AbsoluteTimeUnion;

typedef union			NanosecondTimeUnion												// handy union to access nanosecond times as long longs
{
	Nanoseconds			nst;
	uint64				llt;
}	NanosecondTimeUnion;

// Global Variables

extern	AbsoluteTimeUnion		TU_atic_time_of_last_msec_boundary;						// AbsoluteTime when simulated D16 wrapped
extern	uint32					TU_atic_time_of_last_msec_boundary_fraction;			// fractional part thereof; not used except to reduce long term error

extern	AbsoluteTimeUnion		TU_atic_time_of_next_msec_boundary;						// AbsoluteTime when simulated D16 will wrap next
extern	uint32					TU_atic_time_of_next_msec_boundary_fraction;			// fractional part thereof; not used except to reduce long term error
extern	UInt32*					TU_next_atic_time_lsb_ptr;								// points to memory location to update with lsb's of TU_atic_time_of_next_msec_boundary

extern	uint32					TU_real_time_milliseconds;

extern	uint32					TU_nanos_per_atic_msb;									// nano seconds per absolute time tick: msb
extern	uint32					TU_nanos_per_atic_lsb;									// nano seconds per absolute time tick: fractional part

extern	uint32					TU_micros_per_atic_msb;									// micro seconds per absolute time tick: msb
extern	uint32					TU_micros_per_atic_lsb;									// micro seconds per absolute time tick: fractional part

extern	uint64					TU_atics_per_msec_msb;									// absolute time ticks per millisecond, msb
extern	uint32					TU_atics_per_msec_lsb;									// absolute time ticks per millisecond, fractional part

extern	UInt32					TU_minAbsoluteTimeDelta;								// used to fetch return values about hardware platform
extern	UInt32 					TU_theAbsoluteTimeToNanosecondNumerator;
extern	UInt32					TU_theAbsoluteTimeToNanosecondDenominator;
extern	UInt32 					TU_theProcessorToAbsoluteTimeNumerator;
extern	UInt32					TU_theProcessorToAbsoluteTimeDenominator;

// Routines
extern	uint32 					TU_UnsignedFractionalMultiply32x32(register uint32 arg_a, register uint32 arg_b);
extern	AbsoluteTime 			(*TU_GetProcessorTime)();								// pointer to UpTime or TickCount
extern	void					TU_InitializeTimingUtilities(uint32_ratio known_ratio);	// setup timing utilities
extern	void 					TU_AdvanceRealTimeMilliseconds();						// advance real time milliseconds according to processor up time
extern	void 					TU_FastAdvanceRealTimeMilliseconds();					// advance real time milliseconds quickly according to processor time base register
extern	uint32 					TU_ComputeElapsedMicroseconds();						// compute microseconds since last computed millisecond boundary
extern	uint64 					TU_ComputeMicrosecondTimeBase();						// compute microsecond time base
extern  uint32_ratio			TU_MeasureUPTimeUsingTimeOfDay   ();
extern  uint32_ratio			TU_MeasureUPTimeUsingMicroseconds();
extern  uint32_ratio			TU_MeasureUPTimeUsingTickCount();
extern	uint32_ratio			TU_MeasureUPTimeUsingD16Timer(volatile unsigned long* fifoLoc, unsigned long fifoData);
extern	uint32_ratio			TU_MeasureUPTimeUsingD03Timer(volatile unsigned long* fifoLoc, unsigned long fifoData, unsigned long fifoWrites);

extern	void					TU_PublishNewRatios(uint32_ratio known_ratio);			// update new values after measuring metronome

#endif
