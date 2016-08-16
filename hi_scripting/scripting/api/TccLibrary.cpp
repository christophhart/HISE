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


void TccLibraryFunctions::vMultiply(float* dst, const float* src, int numSamples)
{
	FloatVectorOperations::multiply(dst, src, numSamples);
}

void TccLibraryFunctions::vMultiplyScalar(float* dst, float scalar, int numSamples)
{
	FloatVectorOperations::multiply(dst, scalar, numSamples);
}

void TccLibraryFunctions::vAdd(float* dst, const float* src, int numSamples)
{
	FloatVectorOperations::add(dst, src, numSamples);
}

void TccLibraryFunctions::vAddScalar(float* dst, float a, int numSamples)
{
	FloatVectorOperations::add(dst, a, numSamples);
}

void TccLibraryFunctions::vSub(float* dst, const float* src, int numSamples)
{
	FloatVectorOperations::subtract(dst, src, numSamples);
}

void TccLibraryFunctions::vFill(float* dst, float value, int numSamples)
{
	FloatVectorOperations::fill(dst, value, numSamples);
}

void TccLibraryFunctions::vCopy(float* dst, const float* src, int numSamples)
{
	FloatVectorOperations::copy(dst, src, numSamples);
}

float TccLibraryFunctions::vMinimum(const float* data, int numSamples)
{
	return 0.0f;
}

float TccLibraryFunctions::vMaximum(const float* data, int numSamples)
{
	return 1.0f;
}

void TccLibraryFunctions::vLimit(float* data, float minimum, float maximum, int numSamples)
{
	
}

void TccLibraryFunctions::printString(const char* message)
{
	Logger::writeToLog(message);
}

void TccLibraryFunctions::printInt(int i)
{
	Logger::writeToLog(String(i));
}

void TccLibraryFunctions::printDouble(double d)
{
	Logger::writeToLog(String(d));
}

void TccLibraryFunctions::printFloat(float f)
{
	Logger::writeToLog(String(f));
}

double TccLibraryFunctions::sin(double rad)
{
	return std::sin(rad);
}

float TccLibraryFunctions::sinf(float rad)
{
	return std::sinf(rad);
}

double TccLibraryFunctions::cos(double rad)
{
	return std::cos(rad);
}

double TccLibraryFunctions::cosf(float rad)
{
	return std::cos(rad);
}

float TccLibraryFunctions::pow(double base, double exp)
{
	return std::pow(base, exp);
}

float TccLibraryFunctions::powf(float base, float exp)
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
