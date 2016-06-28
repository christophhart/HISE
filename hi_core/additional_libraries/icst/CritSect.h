// CritSect.h
// Critical section wrapper class for internal use.
// !!! Must not be included before "common.h" !!!
//
// This code is part of the ICST DSP Library version 1.2. It is released
// under the 2-clause BSD license. Copyright (c) 2008-2010, Zurich
// University of the Arts, Beat Frei. All rights reserved.

#ifndef _ICST_DSPLIB_CRITSECT_INCLUDED
#define _ICST_DSPLIB_CRITSECT_INCLUDED

#ifdef _WIN32	// Windows specific
	#ifndef ICSTLIB_ENABLE_MFC
		#include <windows.h>
	#endif
#else			// POSIX specific
	#include <pthread.h>
#endif

namespace icstdsp {		// begin library specific namespace

class CriticalSection
{
public:
	
	CriticalSection()
	{
	#ifdef _WIN32	// Windows specific
		InitializeCriticalSection(&cs);
	#else			// POSIX specific
		pthread_mutex_init(&cs,NULL);
	#endif
	};
	
	~CriticalSection()
	{
	#ifdef _WIN32	// Windows specific
		DeleteCriticalSection(&cs);
	#else			// POSIX specific
		pthread_mutex_destroy(&cs);
	#endif
	};
	
	void Enter()
	{
	#ifdef _WIN32	// Windows specific
		EnterCriticalSection(&cs);
	#else			// POSIX specific
		pthread_mutex_lock(&cs);
	#endif
	};

	void Leave()
	{
	#ifdef _WIN32	// Windows specific
		LeaveCriticalSection(&cs);
	#else			// POSIX specific
		pthread_mutex_unlock(&cs);
	#endif
	};

private:

	#ifdef _WIN32	// Windows specific
		CRITICAL_SECTION cs;
	#else			// POSIX specific
		pthread_mutex_t cs;
	#endif
};

}	// end library specific namespace

#endif


