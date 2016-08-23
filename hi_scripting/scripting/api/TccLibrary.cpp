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

#define TCC_LIBRARY_C_VERSION 0x0013

#ifndef TCC_LIBRARY_VERSION
#error "Library Header not included"
#endif

#if TCC_LIBRARY_VERSION != TCC_LIBRARY_C_VERSION
#error "Library mismatch: " 
#endif

#ifndef USE_C_IMPLEMENTATION
#define USE_C_IMPLEMENTATION 0
#endif


#if USE_C_IMPLEMENTATION
#include <assert.h>
#include <math.h>
#include "../../../hi_core/additional_libraries/kiss_fft/kiss_fft.c"
#include "../../../hi_core/additional_libraries/kiss_fft/kiss_fftr.c"
#endif

#if TCC_CPP
#define CPP_PREFIX TccLibraryFunctions::
#else
#define CPP_PREFIX
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

// generic 2-term trigonometric window
void trigwin2(float* d, int size, double c0, double c1)
{
	if (size == 1) { d[0] = (float)(c0 + c1); return; }
	double temp = 2.0*M_PI / (double)(size - 1);
	double wre = cos(temp), wim = sin(temp);
	double re = -1.0, im = 0;
	for (int i = 0; i <= (size >> 1); i++) {
		d[size - i - 1] = d[i] = (float)(c0 + c1*re);
		temp = re; re = wre*re - wim*im; im = wre*im + wim*temp;
	}
}


// generic 4-term trigonometric window
void trigwin4(float* d, int size, double c0, double c1,
	double c2, double c3)
{
	if (size == 1) { d[0] = (float)(c0 + c1 + c2 + c3); return; }
	c0 -= c2; c1 -= (3.0*c3); c2 *= 2.0; c3 *= 4.0;
	double temp = 2.0*M_PI / (double)(size - 1);
	double wre = cos(temp), wim = sin(temp);
	double re = -1.0, im = 0;
	for (int i = 0; i <= (size >> 1); i++) {
		d[size - i - 1] = d[i] = (float)(c0 + (c1 + (c2 + c3*re)*re)*re);
		temp = re; re = wre*re - wim*im; im = wre*im + wim*temp;
	}
}


void CPP_PREFIX windowFunctionBlackman(float* d, int size)
{
	trigwin4(d, size, 0.42, 0.5, 0.08, 0);
}

void CPP_PREFIX windowFunctionRectangle(float* d, int size)
{
	vFill(d, 1.0f, size);
}

void CPP_PREFIX windowFunctionHann(float* d, int size)
{
	trigwin2(d, size, 0.5, 0.5);
}

void CPP_PREFIX windowFunctionBlackmanHarris(float* d, int size)
{
	trigwin4(d, size, 0.35875, 0.48829, 0.14128, 0.01168);
}

void CPP_PREFIX complexFFT(void* FFTState, float* in, float* out, int fftSize)
{
#if USE_C_IMPLEMENTATION || !USE_IPP
	kiss_fft((kiss_fft_cfg)FFTState, (kiss_fft_cpx*)in, (kiss_fft_cpx*)out);
#else
	static_cast<IppFFT*>(FFTState)->complexFFT(in, out, fftSize);
#endif
}

void CPP_PREFIX complexFFTInverse(void* FFTState, float* in, float* out, int fftSize)
{
#if USE_C_IMPLEMENTATION || !USE_IPP

	kiss_fft_cfg s = (kiss_fft_cfg)FFTState;
	assert(s->inverse == 1);
	kiss_fft((kiss_fft_cfg)FFTState, (kiss_fft_cpx*)in, (kiss_fft_cpx*)out);
#else
	static_cast<IppFFT*>(FFTState)->complexFFTInverse(in, out, fftSize);
#endif

}

void CPP_PREFIX complexFFTInplace(void* FFTState, float* data, int fftSize)
{
#if USE_C_IMPLEMENTATION || !USE_IPP
	size_t s = sizeof(float) * (size_t)fftSize * 2;
	float* out = (float*)malloc(s);
	complexFFT(FFTState, data, out, fftSize);
	memcpy(data, out, s);
	free((void*)out);
#else
	static_cast<IppFFT*>(FFTState)->complexFFTInplace(data, fftSize);
#endif
}

void CPP_PREFIX complexFFTInverseInplace(void* FFTState, float* data, int fftSize)
{
#if USE_C_IMPLEMENTATION || !USE_IPP
	size_t s = sizeof(float) * (size_t)fftSize * 2;

	kiss_fft_cfg st = (kiss_fft_cfg)FFTState;
	assert(st->inverse == 1);
	assert(pow(2.0, st->nfft) == fftSize);

	float* out = (float*)malloc(s);

	complexFFTInverse(FFTState, data, out, fftSize);
	memcpy(data, out, s);

	free((void*)out);
#else
	static_cast<IppFFT*>(FFTState)->complexFFTInverseInplace(data, fftSize);
#endif
}

void CPP_PREFIX realFFT(void* FFTState, float* in, float* out, int fftSize)
{
#if USE_C_IMPLEMENTATION && !USE_IPP

	kiss_fftr_cfg st = (kiss_fftr_cfg)FFTState;
	

	kiss_fftr((kiss_fftr_cfg)FFTState, in, (kiss_fft_cpx*)out);
#else
	static_cast<IppFFT*>(FFTState)->realFFT(in, out, fftSize);
#endif
}

void CPP_PREFIX realFFTInverse(void* FFTState, float* in, float* out, int fftSize)
{
#if USE_C_IMPLEMENTATION && !USE_IPP
	

	kiss_fftr_cfg st = (kiss_fftr_cfg)FFTState;
	assert(st->substate->inverse == 1);

	kiss_fftri((kiss_fftr_cfg)FFTState, (const kiss_fft_cpx*)in, (float*)out);
#else
	static_cast<IppFFT*>(FFTState)->realFFTInverse(in, out, fftSize);
#endif
}

void CPP_PREFIX complexMultiply(float* d, float* r, int size)
{
	int i = 0; float tmp;

#if 1
	for (i = 0; i < (2 * size); i += 2) {
		tmp = d[i];
		d[i] = tmp*r[i] - d[i + 1] * r[i + 1];
		d[i + 1] = r[i] * d[i + 1] + tmp*r[i + 1];
	}

#else
	__m128 r0, r1, r2, r3, mask1, mask2;
	mask1 = _mm_castsi128_ps(_mm_set_epi32(0, 0xffffffff, 0, 0xffffffff));
	mask2 = _mm_castsi128_ps(_mm_set_epi32(0xffffffff, 0, 0xffffffff, 0));
	size <<= 1;
	if (!((reinterpret_cast<uintptr_t>(d) | reinterpret_cast<uintptr_t>(r)) & 0xF)) {
		while (i <= (size - 4)) {
			r0 = _mm_load_ps(d + i);
			r1 = _mm_shuffle_ps(r0, r0, _MM_SHUFFLE(2, 3, 0, 1));
			r2 = _mm_load_ps(r + i);
			r0 = _mm_mul_ps(r0, r2);
			r1 = _mm_mul_ps(r1, r2);
			r2 = _mm_castsi128_ps(_mm_srli_epi64(_mm_castps_si128(r0), 32));
			r3 = _mm_castsi128_ps(_mm_slli_epi64(_mm_castps_si128(r1), 32));
			r0 = _mm_and_ps(r0, mask1);
			r1 = _mm_and_ps(r1, mask2);
			r2 = _mm_sub_ps(r0, r2);
			r3 = _mm_add_ps(r1, r3);
			r3 = _mm_or_ps(r3, r2);
			_mm_store_ps(d + i, r3);
			i += 4;
		}
	}
	else {
		while (i <= (size - 4)) {
			r0 = _mm_loadu_ps(d + i);
			r1 = _mm_shuffle_ps(r0, r0, _MM_SHUFFLE(2, 3, 0, 1));
			r2 = _mm_loadu_ps(r + i);
			r0 = _mm_mul_ps(r0, r2);
			r1 = _mm_mul_ps(r1, r2);
			r2 = _mm_castsi128_ps(_mm_srli_epi64(_mm_castps_si128(r0), 32));
			r3 = _mm_castsi128_ps(_mm_slli_epi64(_mm_castps_si128(r1), 32));
			r0 = _mm_and_ps(r0, mask1);
			r1 = _mm_and_ps(r1, mask2);
			r2 = _mm_sub_ps(r0, r2);
			r3 = _mm_add_ps(r1, r3);
			r3 = _mm_or_ps(r3, r2);
			_mm_storeu_ps(d + i, r3);
			i += 4;
		}
	}
	if (size & 2) {
		tmp = d[i];
		d[i] = tmp*r[i] - d[i + 1] * r[i + 1];
		d[i + 1] = r[i] * d[i + 1] + tmp*r[i + 1];
}
#endif
}

void CPP_PREFIX realToComplex(float* c, float* r, int size)
{
	memset(c, 0, size * 2);

	for (int i = 0; i < 2 * size; i += 2)
		c[2 * i] = r[i];
}

int CPP_PREFIX nextPowerOfTwo(int size)
{
	const double l = log((double)size) / log(2);
	const double lUp = ceil(l);
	return (int)pow(2.0, lUp);
}

void CPP_PREFIX writeFloatArray(float* data, int numSamples)
{
#if MAC
	const char* fileName = "/Volumes/Shared/Development/Graph Library/data.json";
#else
	const char* fileName = "D:\\Development\\Graph Library\\data.json";
#endif

	FILE *fp;
	fp = fopen(fileName, "w");

#if USE_C_IMPLEMENTATION
	fprintf(fp, "{\"name\": \"Float Array Plotter TCC\",\n\"data\": [");
#else
	fprintf(fp, "{\"name\": \"Float Array Plotter HISE\",\n\"data\": [");
#endif

	for (int i = 0; i < numSamples; i++)
	{
		fprintf(fp, "{\"x\": %i, \"y\": %1.7f},\n", i, data[i]);
	}

	fprintf(fp, "{}]}");
	fclose(fp);
}

void CPP_PREFIX writeFrequencySpectrum(float* data, int numSamples)
{
	const int fftSize = numSamples;

	float* complexData = (float*)malloc(sizeof(float) * fftSize * 2);
	float* spectrumData = (float*)malloc(sizeof(float) * fftSize);

	windowFunctionBlackman(complexData, fftSize * 2);
	//windowFunctionBlackmanHarris(complexData, fftSize * 2); 
	//windowFunctionRectangle(complexData, fftSize * 2); 
	//windowFunctionHann(complexData, fftSize * 2); 

	for (int i = 0; i < fftSize; i++)
	{
		complexData[2 * i] *= data[i];
		complexData[2 * i + 1] = 0.0f;
	}

	void* state = createFFTState(fftSize, false, false);

	complexFFTInplace(state, complexData, fftSize);

	destroyFFTState(state);

	for (int i = 0; i < fftSize; i++)
	{
		const float a = complexData[2 * i];
		const float b = complexData[2 * i + 1];

		spectrumData[i] = sqrt(a*a + b*b);
	}

	const float normalizeScaleValue = 2.0f / (float)(fftSize);
	vMultiplyScalar(spectrumData, normalizeScaleValue, fftSize);

#if MAC
	const char* fileName = "/Volumes/Shared/Development/Graph Library/data.json";
#else
	const char* fileName = "D:\\Development\\Graph Library\\data.json";
#endif

	FILE *fp;
	fp = fopen(fileName, "w");

#if USE_C_IMPLEMENTATION
	fprintf(fp, "{\"name\": \"Frequency Plot TCC\",\n\"data\": [");
#else
	fprintf(fp, "{\"name\": \"Frequency Plot HISE\",\n\"data\": [");
#endif

	const double sampleRate = 44100.0;

	for (int i = 0; i < fftSize / 2; i++)
	{
		const double frequency = ((double)i / (double)fftSize) * sampleRate;

		fprintf(fp, "{\"x\": %1.7f, \"y\": %1.7f},\n", frequency, spectrumData[i]);
	}

	fprintf(fp, "{}]}");
	fclose(fp);

	free(spectrumData);
	free(complexData);
}

void CPP_PREFIX zeroPadVector(float* dst, const float* src, int size, int paddingSize)
{
	vCopy(dst, src, size);
	memset(dst + size, 0, sizeof(float)*(paddingSize - size));
}

void* CPP_PREFIX createFFTState(int size, bool isReal, bool isInverse)
{
#if USE_C_IMPLEMENTATION || !USE_IPP
	if (isReal)
	{
		return kiss_fftr_alloc(size, isInverse, 0, 0);
	}
	else
	{
		return kiss_fft_alloc(size, isInverse, 0, 0);
	}
#else

	const int N = (int)log2((double)size);
	return new IppFFT(isReal ? IppFFT::DataType::RealFloat : IppFFT::DataType::ComplexFloat, isReal ? N+2 : N+1);

#endif
}

void CPP_PREFIX destroyFFTState(void* state)
{
#if USE_C_IMPLEMENTATION || !USE_IPP
	if (state != nullptr)
	{
		free(state);
		state = nullptr;
	}
#else

	if (IppFFT* ippState = reinterpret_cast<IppFFT*>(state))
	{
		delete ippState;
		state = nullptr;
	}
	
#endif
}

bool CPP_PREFIX varToBool(void* v)
{
#if USE_C_IMPLEMENTATION
    return false;
#else
	var* va = reinterpret_cast<var*>(v);
	return (bool)(*va);
#endif
}

int CPP_PREFIX varToInt(void* v)
{
#if USE_C_IMPLEMENTATION
    return 0;
#else
	var* va = reinterpret_cast<var*>(v);
	return (int)(*va);
#endif
}

double CPP_PREFIX varToDouble(void* v)
{
#if USE_C_IMPLEMENTATION
    return 0.0;
#else
	var* va = reinterpret_cast<var*>(v);
	return (double)(*va);
#endif
}

float CPP_PREFIX varToFloat(void* v)
{
#if USE_C_IMPLEMENTATION
    return 0.0f;
#else
	var* va = reinterpret_cast<var*>(v);
    return (float)(*va);
#endif
}

float* CPP_PREFIX getVarBufferData(void* v)
{
#if USE_C_IMPLEMENTATION
    return NULL;
#else
	var* va = reinterpret_cast<var*>(v);

	if (va->isBuffer())
	{
		return va->getBuffer()->buffer.getWritePointer(0);
	}

#endif
	return NULL;

}

int CPP_PREFIX getVarBufferSize(void* v)
{
#if USE_C_IMPLEMENTATION
#else
	var* va = reinterpret_cast<var*>(v);

	if (va->isBuffer())
	{
		return va->getBuffer()->size;
	}
#endif
    
	return -1;
}


#if USE_C_IMPLEMENTATION

#else


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

	ADD_FUNCTION_POINTER(windowFunctionBlackman);
	ADD_FUNCTION_POINTER(windowFunctionRectangle);
	ADD_FUNCTION_POINTER(windowFunctionHann);
	ADD_FUNCTION_POINTER(windowFunctionBlackmanHarris);

	ADD_FUNCTION_POINTER(createFFTState);
	ADD_FUNCTION_POINTER(destroyFFTState);

	ADD_FUNCTION_POINTER(complexFFT);
	ADD_FUNCTION_POINTER(complexFFTInverse);
	ADD_FUNCTION_POINTER(complexFFTInplace);
	ADD_FUNCTION_POINTER(complexFFTInverseInplace);

	ADD_FUNCTION_POINTER(realFFT);
	ADD_FUNCTION_POINTER(realFFTInverse);

	ADD_FUNCTION_POINTER(realToComplex);
	ADD_FUNCTION_POINTER(complexMultiply);
	ADD_FUNCTION_POINTER(zeroPadVector);
	ADD_FUNCTION_POINTER(nextPowerOfTwo);

	ADD_FUNCTION_POINTER(writeFloatArray);
	ADD_FUNCTION_POINTER(writeFrequencySpectrum);

	ADD_FUNCTION_POINTER(varToBool);
	ADD_FUNCTION_POINTER(varToInt);
	ADD_FUNCTION_POINTER(varToDouble);
	ADD_FUNCTION_POINTER(varToFloat);
	ADD_FUNCTION_POINTER(getVarBufferData);
	ADD_FUNCTION_POINTER(getVarBufferSize);

}

#undef ADD_FUNCTION_POINTER
#endif
