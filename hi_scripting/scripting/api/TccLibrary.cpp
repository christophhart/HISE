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

#ifndef USE_C_IMPLEMENTATION
#define USE_C_IMPLEMENTATION 0
#endif

#if USE_C_IMPLEMENTATION
#define CPP_PREFIX
#else
#define CPP_PREFIX TccLibraryFunctions::
#endif

void CPP_PREFIX vMultiply(float* dst, const float* src, int numSamples)
{
#if USE_C_IMPLEMENTATION
	for(int i = 0; i < numSamples; i++) *dst++ *= *src++;
#else
    FloatVectorOperations::multiply(dst, src, numSamples);
#endif
}

void CPP_PREFIX vMultiplyScalar(float* dst, float scalar, int numSamples)
{
#if USE_C_IMPLEMENTATION
	for(int i = 0; i < numSamples; i++) *dst++ *= scalar;
#else
	FloatVectorOperations::multiply(dst, scalar, numSamples);
#endif
}

void CPP_PREFIX vAdd(float* dst, const float* src, int numSamples)
{
#if USE_C_IMPLEMENTATION
	for(int i = 0; i < numSamples; i++) *dst++ += *src++;
#else
	FloatVectorOperations::add(dst, src, numSamples);
#endif

	
}

void CPP_PREFIX vAddScalar(float* dst, float a, int numSamples)
{
#if USE_C_IMPLEMENTATION
	for(int i = 0; i < numSamples; i++) *dst++ += a;
#else
	FloatVectorOperations::add(dst, a, numSamples);
#endif
}

void CPP_PREFIX vSub(float* dst, const float* src, int numSamples)
{
#if USE_C_IMPLEMENTATION
	for(int i = 0; i < numSamples; i++) *dst++ -= *src++;
#else
	FloatVectorOperations::subtract(dst, src, numSamples);
#endif
}

void CPP_PREFIX vFill(float* dst, float value, int numSamples)
{
#if USE_C_IMPLEMENTATION
	for(int i = 0; i < numSamples; i++) *dst++ = value;
#else
	FloatVectorOperations::fill(dst, value, numSamples);
#endif
}

void CPP_PREFIX vCopy(float* dst, const float* src, int numSamples)
{
#if USE_C_IMPLEMENTATION
	for(int i = 0; i < numSamples; i++) *dst++ = *src++;
#else
	FloatVectorOperations::copy(dst, src, numSamples);
#endif
}

float CPP_PREFIX vMinimum(const float* data, int numSamples)
{
#if USE_C_IMPLEMENTATION

#else
	
#endif

	return 0.0f;
}

float CPP_PREFIX vMaximum(const float* data, int numSamples)
{
#if USE_C_IMPLEMENTATION

#else
	
#endif

	return 1.0f;
}

void CPP_PREFIX vLimit(float* data, float minimum, float maximum, int numSamples)
{
#if USE_C_IMPLEMENTATION

#else
	
#endif

}

void CPP_PREFIX printString(const char* message)
{
#if USE_C_IMPLEMENTATION
	printf(message);
#else
	Logger::writeToLog(message);
#endif
}

void CPP_PREFIX printInt(int i)
{
#if USE_C_IMPLEMENTATION
	printf("%i", i);
#else
	Logger::writeToLog(String(i));
#endif
}

void CPP_PREFIX printDouble(double d)
{
#if USE_C_IMPLEMENTATION
    printf("%1.4f", d);
#else
	Logger::writeToLog(String(d));
#endif

	
}

void CPP_PREFIX printFloat(float f)
{
#if USE_C_IMPLEMENTATION
	printf("%1.4f", f);
#else
	Logger::writeToLog(String(f));
#endif	
}

#if USE_C_IMPLEMENTATION
#include <math.h>
#else

double CPP_PREFIX sin(double rad)
{
	return std::sin(rad);
}

float CPP_PREFIX sinf(float rad)
{
	return std::sinf(rad);
}

double CPP_PREFIX cos(double rad)
{
	return std::cos(rad);
}

double CPP_PREFIX cosf(float rad)
{
	return std::cos(rad);
}

float CPP_PREFIX pow(double base, double exp)
{
	return std::pow(base, exp);
}

float CPP_PREFIX powf(float base, float exp)
{
	return std::pow(base, exp);
}

#define ADD_FUNCTION_POINTER(f) context->addFunction((void*)f, #f)

void TccLibraryFunctions::addFunctionsToContext(TccContext* context)
{
	ADD_FUNCTION_POINTER(vMultiply);
	ADD_FUNCTION_POINTER(vMultiplyScalar);
	ADD_FUNCTION_POINTER(vAdd);
	ADD_FUNCTION_POINTER(vAddScalar);
	ADD_FUNCTION_POINTER(vSub);
	ADD_FUNCTION_POINTER(vFill);
	ADD_FUNCTION_POINTER(vCopy);
	ADD_FUNCTION_POINTER(vMinimum);
	ADD_FUNCTION_POINTER(vMaximum);
	ADD_FUNCTION_POINTER(vLimit);

	ADD_FUNCTION_POINTER(printString);
	ADD_FUNCTION_POINTER(printInt);
	ADD_FUNCTION_POINTER(printDouble);
	ADD_FUNCTION_POINTER(printFloat);

	ADD_FUNCTION_POINTER(sin);
	ADD_FUNCTION_POINTER(sinf);
	ADD_FUNCTION_POINTER(cos);
	ADD_FUNCTION_POINTER(cosf);
	ADD_FUNCTION_POINTER(pow);
	ADD_FUNCTION_POINTER(powf);
}

#undef ADD_FUNCTION_POINTER
#endif
