/* ThreadUtilities.cpp */

// Mac OS
#include <CoreServices/CoreServices.h>
#include <mach/thread_act.h>

// Local includes
#include "ThreadUtilities.h"

// Constants
#define	LOG_STUFF 0

// =================================================================================
//		¥ ThU_SetRealTimePolicy()
// =================================================================================

void	ThU_SetRealTimePolicy ()
{
	// Set THREAD_TIME_CONSTRAINT_POLICY
	Nanoseconds		periodNanos;
	Nanoseconds		computationNanos;
	Nanoseconds		constraintNanos;
	AbsoluteTime	periodAbs;
	AbsoluteTime	computationAbs;
	AbsoluteTime	constraintAbs;
	struct thread_time_constraint_policy theTCPolicy;
	OSStatus		result;
	
	periodNanos.hi      = 0; periodNanos.lo      = 1000000;			// one millisecond period
	computationNanos.hi = 0; computationNanos.lo =   50000;			// 50 microsecond computation
	constraintNanos.hi  = 0; constraintNanos.lo  =  100000;			// 100 microsecond constraint
	
	periodAbs      = NanosecondsToAbsolute(periodNanos     );
	computationAbs = NanosecondsToAbsolute(computationNanos);
	constraintAbs  = NanosecondsToAbsolute(constraintNanos );

	theTCPolicy.period      = periodAbs.lo;
	theTCPolicy.computation = computationAbs.lo;
	theTCPolicy.constraint  = constraintAbs.lo;
	theTCPolicy.preemptible = true;

	result = thread_policy_set(mach_thread_self(), THREAD_TIME_CONSTRAINT_POLICY, (thread_policy_t)&theTCPolicy, THREAD_TIME_CONSTRAINT_POLICY_COUNT);
	
	#if LOG_STUFF
		printf("THREAD_TIME_CONSTRAINT_POLICY 0x%08x (%d %d %d)\n", (int) result, (int) theTCPolicy.period, (int) theTCPolicy.computation, (int) theTCPolicy.constraint);
	#endif
}


// =================================================================================
//		¥ ThU_SetRealTimePolicy()
// =================================================================================

void	ThU_SetStandardPolicy ()
{
	thread_standard_policy	policy_info;
	OSStatus				result;
	
	// Set standard policy
	policy_info.no_data = NULL;

	result = thread_policy_set(mach_thread_self(), THREAD_STANDARD_POLICY, (thread_policy_t) &policy_info, THREAD_STANDARD_POLICY_COUNT);
	
	#if LOG_STUFF
		printf("THREAD_STANDARD_POLICY_COUNT 0x%08x\n", (int) result);
	#endif
}

