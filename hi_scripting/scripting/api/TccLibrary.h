/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#ifndef TCCLIBRARY_H_INCLUDED
#define TCCLIBRARY_H_INCLUDED

#if TCC_INCLUDE
#define TCCLIBDEF extern
#else
#define TCCLIBDEF static
#endif

#if !TCC_INCLUDE
static void addFunctionsToContext(TccContext* context);
#endif

/***************************************************************************************
*																					   *
*	VECTOR OPERATIONS																   *
*																					   *
***************************************************************************************/

/** dst *= src */
TCCLIBDEF void vMultiply(float* dst, const float* src, int numSamples);

/** dst *= scalar */
TCCLIBDEF void vMultiplyScalar(float* dst, float scalar, int numSamples);

/** dst += src. */
TCCLIBDEF void vAdd(float* dst, const float* src, int numSamples);

/** dst += a */
TCCLIBDEF void vAddScalar(float* dst, float a, int numSamples);

/** dst -= s */
TCCLIBDEF void vSub(float* dst, const float* s, int numSamples);

/** dst = value. */
TCCLIBDEF void vFill(float* dst, float value, int numSamples);

/** dst = src. */
TCCLIBDEF void vCopy(float* dst, const float* src, int numSamples);

/** return =  min(data). */
TCCLIBDEF float vMinimum(const float* data, int numSamples);

/** return max(data). */
TCCLIBDEF float vMaximum(const float* data, int numSamples);

/** data = min(min, max(max, d)) */
TCCLIBDEF void vLimit(float* data, float minimum, float maximum, int numSamples);

/***************************************************************************************
*																					   *
*	DEBUG / PRINT FUNCTIONS															   *
*																					   *
***************************************************************************************/

TCCLIBDEF void printString(const char* message);
TCCLIBDEF void printInt(int i);
TCCLIBDEF void printDouble(double d);
TCCLIBDEF void printFloat(float f);

/***************************************************************************************
*																					   *
*	MATH ROUTINES																	   *
*																					   *
***************************************************************************************/

TCCLIBDEF double sin(double rad);
TCCLIBDEF float sinf(float rad);

TCCLIBDEF double cos(double rad);
TCCLIBDEF double cosf(float rad);

TCCLIBDEF float pow(double base, double exp);
TCCLIBDEF float powf(float base, float exp);



#endif  // TCCLIBRARY_H_INCLUDED
