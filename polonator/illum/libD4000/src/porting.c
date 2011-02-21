/* porting.c
Convenience definitions for symbols defined in the windows environment
By Dr. Daniel Levner, Church Lab, Harvard Medical School
Copyright Daniel Levner (c) 2009
*/


#include <time.h>
#include <stdio.h>
#include "porting.h"

#ifdef __cplusplus
extern "C"
{
#endif

	/* Windows Sleep() is in ms while Linux is in seconds
	   nanosleep(), which is in ns, allows a workaround
	   This may not be portable! */
	void Sleep(DWORD milliseconds)
	{
		struct timespec time_to_sleep, time_remaining;

		// need to separate seconds from ns
		time_to_sleep.tv_sec = milliseconds/1000;
		time_to_sleep.tv_nsec = (milliseconds - time_to_sleep.tv_sec*1000) * 1000000;  /* ns to ms */

		nanosleep(&time_to_sleep, &time_remaining);
		//printf("Sleep time remaining: retval = %d, s = %ld, ns = %ld\n", retval, time_remaining.tv_sec, time_remaining.tv_nsec);
	}

#ifdef __cplusplus
}
#endif

