// TimingUtilitiesOSX.h

#ifndef	__TIMINGUTILITIES__
#define __TIMINGUTILITIES__

//	Mac OS Includes

// This implementation is used both in the kernel driver and in the user space application

#ifdef	COMPILE_OSX_KERNEL
    #include <IOKit/IOLib.h>
    #include <kern/clock.h>
#else
    #include <mach/mach_time.h>
#endif

#include "Standard.h"

typedef uint64_t        TU_nanos;

extern  TU_nanos        TU_ComputeNanosecondTimeBase();                                     // compute computes a linear nanosecond  time base

static __inline__ void  TU_ReadTBR(TU_nanos &nano_ref)
{
    nano_ref = TU_ComputeNanosecondTimeBase();
}

static __inline__ TU_nanos TU_GetProcessorTime()
{
    return TU_ComputeNanosecondTimeBase();
}

static __inline__ uint32 TU_ReadTBRLsb()
{
    return (uint32) TU_ComputeNanosecondTimeBase();
}

// Variables

extern	TU_nanos		TU_atic_time_root;                                                  // AbsoluteTime of time == 0
extern	TU_nanos		TU_atic_time_of_last_msec_boundary;                                 // AbsoluteTime when simulated D16 wrapped
extern	TU_nanos		TU_atic_time_of_next_msec_boundary;                                 // AbsoluteTime when simulated D16 will wrap next

extern  unsigned int    TU_next_atic_time_lsb;
extern  unsigned int*   TU_next_atic_time_lsb_ptr;

extern	uint32          TU_real_time_milliseconds;                                          // Millisecond real time counter
extern	uint32          TU_host_nsecs_per_real_time_msec;                                   // Nanoseconds of CPU time per millisecond of Synclavier real time

extern	uint32			TU_100micro_shift_factor;                                           // shift right factor to scale processor host time to approximately 100 microseconds per tick
extern	uint32			TU_host_nsecs_per_100micros;                                        // Nanoseconds of CPU time per 100 microseconds of Synclavier real time
extern	uint32			TU_100micro_magnitude;                                              // smallest power of 2 > TU_host_nsecs_per_100micros

// Routines
extern	int				TU_PerformLinearComparison(unsigned int val1, unsigned int val2);	// Check for wrap from val1 to val2
extern	uint32 			TU_UnsignedFractionalMultiply32x32(uint32 arg_a, uint32 arg_b);		// 32bit x 32 bit unsigned fractional multiply
extern	void			TU_InitializeTimingUtilities(uint32_ratio known_ratio);				// setup timing utilities
extern	void 			TU_FastAdvanceRealTimeMilliseconds();								// advance real time milliseconds quickly according to processor time base register
extern	TU_nanos 		TU_ComputeMicrosecondTimeBase();									// compute computes a linear microsecond time base
extern	uint32_ratio	TU_MeasureUPTimeUsingD16Timer(volatile unsigned int* fifoLoc, unsigned int fifoData);							// Calibrate using D16 timer - use D16 timer on M64k board
extern	uint32_ratio	TU_MeasureUPTimeUsingD03Timer(volatile unsigned int* fifoLoc, unsigned int fifoData, unsigned int fifoWrites);	// Calibrate using D03 timer - matches Model D implementation

extern	void			TU_PublishNewRatios(uint32_ratio known_ratio);						// update new values after measuring metronome

typedef bool            (*TU_CheckD03)();
extern	uint32_ratio	TU_MeasureUPTimeUsingD03Timer(TU_CheckD03 d3_reader);               // Calibrate using D03 timer - matches Model D implementation
extern  bool            TU_Abort_Measurement;
#endif
