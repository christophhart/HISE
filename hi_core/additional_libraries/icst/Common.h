// common.h : standard includes, use this header to create a precompiled
// header file if desired
//
// This code is part of the ICST DSP Library version 1.2. It is released
// under the 2-clause BSD license. Copyright (c) 2008-2010, Zurich
// University of the Arts, Beat Frei. All rights reserved.

#ifndef _ICST_DSPLIB_COMMON_INCLUDED
#define _ICST_DSPLIB_COMMON_INCLUDED

// for testing purposes only
//#define ICSTLIB_NO_SSEOPT
//#define ICSTLIB_DEF_ROUND
//#define ICSTLIB_ENABLE_MFC
//#define ICSTLIB_USE_IPP

#ifdef USE_IPP
#define ICSTLIB_USE_IPP 1
#endif

class FFTProcessor;

// universal includes that must be provided to the outside
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#ifndef ICSTLIB_NO_SSEOPT 	
	#include <emmintrin.h>	// SSE2 intrinsics
#endif
#ifdef _WIN32
	#ifndef NOMINMAX		// no max/min macros are defined when
		#define NOMINMAX	// "windows.h" is included
	#endif
	#ifdef ICSTLIB_ENABLE_MFC
		#include <afxwin.h>	// MFC core and standard components
	#endif
#endif

// global macros that must be provided to the outside
#ifdef ICSTLIB_NO_SSEOPT
	#define ICSTDSP_SSEALIGN
#else
	#if defined(_MSC_VER)
		#define ICSTDSP_SSEALIGN __declspec(align(16))
	#else
		#define ICSTDSP_SSEALIGN __attribute__((aligned(16)))
	#endif
#endif

// library specific data types that must be provided to the outside
namespace icstdsp {		// begin library specific namespace

class Complex {public: float re; float im;};

#if (defined(_MSC_VER) || defined(__BORLANDC__))		
	typedef __int64 icstdsp_int64;
#else
	typedef int64_t icstdsp_int64;
#endif

}	// end library specific namespace

#endif

