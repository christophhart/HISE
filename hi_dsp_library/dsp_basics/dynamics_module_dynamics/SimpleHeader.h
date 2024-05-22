
#ifndef __SIMPLE_HEADER_H__
#define __SIMPLE_HEADER_H__

#if _MSC_VER > 1000			// MS Visual Studio
#define INLINE __forceinline	// forces inline
#ifndef NOMINMAX
#define NOMINMAX				// for standard library min(), max()
#endif
#define _USE_MATH_DEFINES		// for math constants
#else						// other IDE's
#define INLINE 
#endif

#include <algorithm>	// for min(), max()
#include <cassert>		// for assert()
#include <cmath>

using SimpleDataType = double;

#endif	// end __SIMPLE_HEADER_H__
