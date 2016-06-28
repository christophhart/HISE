// MathDefs.h
// define library-specific internal math constants and macros
//
// This code is part of the ICST DSP Library version 1.2. It is released
// under the 2-clause BSD license. Copyright (c) 2008-2010, Zurich
// University of the Arts, Beat Frei. All rights reserved.

#ifndef _ICST_DSPLIB_MATHDEFS_INCLUDED
#define _ICST_DSPLIB_MATHDEFS_INCLUDED

// constants
#ifndef M_PI
	#define M_PI 3.1415926535897932384626433832795
#endif
#ifndef M_PI_FLOAT
	#define M_PI_FLOAT 3.14159265f
#endif

namespace icstdsp {		// begin library specific namespace

const float ANTI_DENORMAL_FLOAT = 1e-15f;	// approx. 10000*sqrt(FLT_MIN)
											// chosen wisely, don't change! 
const float FLT_INTMAX = 2147483520.0f;		// maximum float guaranteed to
											// be converted to a valid int
const float FLT_INTMIN = -2147483648.0f;	// minimum float guaranteed	to
											// be converted to a valid int
}	// end library specific namespace

// macros
// avoid arguments that change during the evaluation of a macro, e.g. a++
#ifndef __max
	#define __max(a,b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef __min
	#define __min(a,b) (((a) < (b)) ? (a) : (b))
#endif

#if (defined(_MSC_VER) || defined(__BORLANDC__))
	namespace icstdsp {typedef unsigned __int64 icstdsp_uint64;}
	#define I64CONST(x) x##i64
#else
	#include <stdint.h>
	namespace icstdsp {typedef uint64_t icstdsp_uint64;}
	#define I64CONST(x) x##LL			
#endif

#endif

