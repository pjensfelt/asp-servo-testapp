#pragma once
#include <cstdint> 

// Adding the cycle time to the time spec
#define NSEC_PER_SEC 1000000000

void timespec_add(struct timespec *ts, int64_t addtime)
{
   int64_t sec, nsec;

   nsec = addtime % NSEC_PER_SEC;
   sec = (addtime - nsec) / NSEC_PER_SEC;
   ts->tv_sec += sec;
   ts->tv_nsec += nsec;
   if ( ts->tv_nsec > NSEC_PER_SEC )
   {
      nsec = ts->tv_nsec % NSEC_PER_SEC;
      ts->tv_sec += (ts->tv_nsec - nsec) / NSEC_PER_SEC;
      ts->tv_nsec = nsec;
   }
}


void timespec_diff(struct timespec *start, struct timespec *stop,
	               struct timespec *result)
{
	if ((stop->tv_nsec - start->tv_nsec) < 0) {
	    result->tv_sec = stop->tv_sec - start->tv_sec - 1;
	    result->tv_nsec = stop->tv_nsec - start->tv_nsec + NSEC_PER_SEC;
	} else {
	    result->tv_sec = stop->tv_sec - start->tv_sec;
	    result->tv_nsec = stop->tv_nsec - start->tv_nsec;
	}

	return;
}
