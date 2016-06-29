// BlkDsp.cpp
//
// This code is part of the ICST DSP Library version 1.2. It is released
// under the 2-clause BSD license. Copyright (c) 2008-2010, Zurich
// University of the Arts, Beat Frei. All rights reserved.

#if DONT_INCLUDE_HEADERS_IN_CPP
#else
#include "common.h"
#include "CritSect.h"	// must not be included before "common.h"
#include "BlkDsp.h"
#include "fftooura.h"
#include "MathDefs.h"
#include "SpecMath.h"
#endif

#include <climits>
#include <algorithm>	// STL used for: "median()", "quantile()", "sort"
#ifdef ICSTLIB_USE_IPP
	#include <ipp.h>	// Intel Performance Primitives package
#endif


FFTProcessor::FFTProcessor(int fftDataType)
{
	fftData = new IppFFT((IppFFT::DataType)fftDataType);
}


IppFFT * FFTProcessor::getFFTObject()
{
	return fftData.get();
}

//******************************************************************************
//* FFT routines, use Ooura's FFT package (double original + float modification)
//*
// standard FFT. size is a power of 2.
// d[] = re[0],im[0],..,re[size-1],im[size-1].
void FFTProcessor::fft(float* d, int size)
{
#ifdef ICSTLIB_USE_IPP

	fftData->complexFFT(d, size);

#if 0
	int order = twopownton(size);
	if ((fftfSpec != NULL) && (order <= IPP_FFT_MAX_POWER_OF_TWO)) {
		ippsFFTFwd_CToC_32fc_I((Ipp32fc*)d, fftfSpec[order], NULL);
	}
	else 
	{
		// Must be initialised!
		jassertfalse;
	}
#endif

#else
	cdft(2*size, -1, d);
#endif
}

void FFTProcessor::fft(double* d, int size)
{
#ifdef ICSTLIB_USE_IPP

	fftData->complexFFT(d, size);

#if 0
	int order = twopownton(size);
	if ((fftdSpec != NULL) && (order <= IPP_FFT_MAX_POWER_OF_TWO)) {
		ippsFFTFwd_CToC_64fc_I((Ipp64fc*)d, fftdSpec[order], NULL);
	}
	else 
	{
		// Must be initialised!
		jassertfalse;
	}
#endif

#else
	cdft(2*size, -1, d);
#endif
}

// standard IFFT. size is a power of 2.
// d[] = re[0],im[0],..,re[size-1],im[size-1].
void FFTProcessor::ifft(float* d, int size)
{
#ifdef ICSTLIB_USE_IPP

	fftData->complexInverseFFT(d, size);

#if 0
	int order = twopownton(size);
	if ((fftfSpec != NULL) && (order <= IPP_FFT_MAX_POWER_OF_TWO)) {
		ippsFFTInv_CToC_32fc_I((Ipp32fc*)d, fftfSpec[order], NULL);
	}
	else
	{
		// Must be initialised!
		jassertfalse;
	}
#endif
#else
	cdft(2*size, 1, d);
	mul(d, 1.0f/static_cast<float>(size), 2*size);
#endif
}

void FFTProcessor::ifft(double* d, int size)
{
#ifdef ICSTLIB_USE_IPP

	fftData->complexInverseFFT(d, size);

#if 0
	int order = twopownton(size);
	if ((fftdSpec != NULL) && (order <= IPP_FFT_MAX_POWER_OF_TWO)) {
		ippsFFTInv_CToC_64fc_I((Ipp64fc*)d, fftdSpec[order], NULL);
	}
	else
	{
		// Must be initialised!
		jassertfalse;
	}
#endif
#else
	double norm = 1.0/static_cast<double>(size);
	cdft(2*size, 1, d);
	for (int i=0; i<(2*size); i++) {d[i] *= norm;}
#endif
}

// FFT of real data. size is a power of 2.
// in: d[] = re[0],re[1],..,re[size-1]. 
// out: d[] = re[0],*re[size/2]*,re[1],im[1],..,re[size/2-1],im[size/2-1].
void FFTProcessor::realfft(float* d, int size)
{
#ifdef ICSTLIB_USE_IPP

	fftData->realFFT(d, size);

#if 0
	int order = twopownton(size);
	if ((fftfSpec != NULL) && (order <= IPP_FFT_MAX_POWER_OF_TWO)) {
		ippsFFTFwd_RToPerm_32f_I((Ipp32f*)d, rfftfSpec[order], NULL);
	}
	else 
	{
		// Must be initialised
		jassertfalse;
	}
#endif
#else
	rdft(size, 1, d);		
	cpxconj(d, size>>1);	// preserve aligned processing if d aligned
	d[1] = -d[1];			//
#endif
}

void FFTProcessor::realfft(double* d, int size)
{
#ifdef ICSTLIB_USE_IPP

	fftData->realFFT(d, size);

#if 0
	int order = twopownton(size);
	if ((fftfSpec != NULL) && (order <= IPP_FFT_MAX_POWER_OF_TWO)) {
		ippsFFTFwd_RToPerm_64f_I((Ipp64f*)d, rfftdSpec[order], NULL);
	}
	else 
	{
		// Must be initialised!
		jassertfalse;
	}
#endif
#else
	rdft(size, 1, d);
	for (int i=3; i<size; i+=2) {d[i] = -d[i];}
#endif
}

// IFFT to real data. size is a power of 2.
// in: d[] = re[0],*re[size/2]*,re[1],im[1],..,re[size/2-1],im[size/2-1].
// out: d[] = re[0],re[1],..,re[size-1].
void FFTProcessor::realifft(float* d, int size)
{
#ifdef ICSTLIB_USE_IPP

	fftData->realInverseFFT(d, size);

#if 0
	int order = twopownton(size);
	if ((fftfSpec != NULL) && (order <= IPP_FFT_MAX_POWER_OF_TWO)) {
		ippsFFTInv_PermToR_32f_I((Ipp32f*)d, rfftfSpec[order], NULL);
	}
	else 
	{
		// Must be initialised!
		jassertfalse;
	}
#endif
#else
	mul(d, 2.0f/static_cast<float>(size), size);
	cpxconj(d, size>>1);	// preserve aligned processing if d aligned
	d[1] = -d[1];			//
	rdft(size, -1, d);
#endif
}

void FFTProcessor::realifft(double* d, int size)
{
#ifdef ICSTLIB_USE_IPP

	fftData->realInverseFFT(d, size);

#if 0
	int order = twopownton(size);
	if ((fftfSpec != NULL) && (order <= IPP_FFT_MAX_POWER_OF_TWO)) {
		ippsFFTInv_PermToR_64f_I((Ipp64f*)d, rfftdSpec[order], NULL);
	}
	else 
	{
		// Must be initialised!
		jassertfalse;
	}
#endif
#else
	int i; double norm = 2.0/static_cast<double>(size); double mnorm = -norm;
	for (i=0; i<size; i+=2) {d[i] *= norm;}
	d[1] *= norm;
	for (i=3; i<size; i+=2) {d[i] *= mnorm;}
	rdft(size, -1, d);
#endif
}

// FFT of symmetrical real data. size is a power of 2.
// in:	d[] = re[0],re[1],..,re[size], contains lower half original data
// out:	d[] = re[0],re[1],..,re[size], contains lower half spectrum
void VectorFunctions::realsymfft(float* d, int size)
{
	mul(d, 2.0f, size);		// preserve aligned processing if d aligned 
	d[0] *= 0.5f;			//
	dfct(size, d);	
}
void VectorFunctions::realsymfft(double* d, int size)
{
	for (int i=1; i<size; i++) {d[i] *= 2.0;}
	dfct(size, d);
}

// IFFT to symmetrical real data. size is a power of 2.	
// in:	d[] = re[0],re[1],..,re[size], contains lower half spectrum
// out:	d[] = re[0],re[1],..,re[size], contains lower half original data	
void VectorFunctions::realsymifft(float* d, int size)
{
	d[0] *= 0.5f; d[size] *= 0.5f; 
	dfct(size, d);
	mul(d, 1.0f/static_cast<float>(size), size+1);
}
void VectorFunctions::realsymifft(double* d, int size)
{
	int i; double norm = 1.0/static_cast<double>(size);
	d[0] *= 0.5; d[size] *= 0.5; 
	dfct(size, d);
	for (i=0; i<=size; i++) {d[i] *= norm;}
}

// DCT type 2. size is a power of 2.
// d[] = re[0],re[1],..,re[size-1]. 
void VectorFunctions::dct(float* d, int size)
{
	ddct(size, -1, d);
}

void VectorFunctions::dct(double* d, int size)
{
	ddct(size, -1, d);
}
		
// IDCT type 2 (= DCT type 3). size is a power of 2.
// d[] = re[0],re[1],..,re[size-1].
void VectorFunctions::idct(float* d, int size)
{
	d[0] *= 0.5f;
	ddct(size, 1, d);
	mul(d, 2.0f/static_cast<float>(size), size);
}

void VectorFunctions::idct(double* d, int size)
{
	double norm = 2.0/static_cast<double>(size);
	d[0] *= 0.5;
	ddct(size, 1, d);
	for (int i=0; i<size; i++) {d[i] *= norm;}
}

// DST type 2. size is a power of 2.
// d[] = re[0],re[1],..,re[size-1]
void VectorFunctions::dst(float* d, int size)
{
	ddst(size, -1, d);
	float tmp = d[0]; memmove(d,d+1,(size-1)*sizeof(float)); d[size-1] = tmp;
}
void VectorFunctions::dst(double* d, int size)
{
	ddst(size, -1, d);
	double tmp = d[0]; memmove(d,d+1,(size-1)*sizeof(double)); d[size-1] = tmp;
}

// IDST type 2. size is a power of 2.
// d[] = re[0],re[1],..,re[size-1].
void VectorFunctions::idst(float* d, int size)
{
	float tmp = d[size-1];
	memmove(d+1,d,(size-1)*sizeof(float)); d[0] = tmp; 
	d[0] *= 0.5f;
	ddst(size, 1, d);
	mul(d, 2.0f/static_cast<float>(size), size);
}
void VectorFunctions::idst(double* d, int size)
{
	double tmp = d[size-1], norm = 2.0/static_cast<double>(size);
	memmove(d+1,d,(size-1)*sizeof(double)); d[0] = tmp; 
	d[0] *= 0.5;
	ddst(size, 1, d);
	for (int i=0; i<size; i++) {d[i] *= norm;}
}

// return single k-th bin of realfft of arbitrary size using the Goertzel
// algorithm, about 0.5*log2(size) times faster than float version of realfft
Complex VectorFunctions::goertzel(float* d, int size, int k)
{
	double tmp,c1,c2,s1=0,s2=0;
	tmp = 2.0*M_PI*static_cast<double>(k)/static_cast<double>(size);
	c1 = 2.0*cos(tmp); c2 = sin(tmp);
	int i, rm = size - ((size>>2)<<2); 
	for (i=0; i<rm; i++) {
		tmp = static_cast<double>(d[i]) + c1*s1 - s2;
		s2=s1;
		s1=tmp;
	}
	for (i=rm; i<size; i+=4) {
		s2 -= static_cast<double>(d[i]); s2 -= c1*s1;
		s1 -= static_cast<double>(d[i+1]); s1 += c1*s2;
		s2 += static_cast<double>(d[i+2]); s2 -= c1*s1;
		s1 += static_cast<double>(d[i+3]); s1 += c1*s2;
	}
	Complex out;
	out.re = static_cast<float>(0.5*c1*s1 - s2);
	out.im = static_cast<float>(c2*s1);
	return out;
}

// haar wavelet transform
// d[0..size-1] -> d[[0],[0],[0..1],[0..3],..,[0..size/2-1]]
void VectorFunctions::hwt(float* d, int size)
{
	static const float scl = sqrtf(0.5f);
	int i,j,k;
	float* r; r = new float[size];
	memcpy(r,d,size*sizeof(float));

	while (size >= 8) {
		size >>= 1;
		for (i=0; i<size; i+=4) {
			j = i<<1; k = size + i;
			d[k]	= scl*(r[j] - r[j+1]);
			d[k+1]	= scl*(r[j+2] - r[j+3]);
			d[k+2]	= scl*(r[j+4] - r[j+5]);
			d[k+3]	= scl*(r[j+6] - r[j+7]);
			r[i]	= scl*(r[j] + r[j+1]);
			r[i+1]	= scl*(r[j+2] + r[j+3]);
			r[i+2]	= scl*(r[j+4] + r[j+5]);
			r[i+3]	= scl*(r[j+6] + r[j+7]);
		}
	}
	if (size > 2) {
		d[2] = scl*(r[0] - r[1]);
		r[0] = scl*(r[0] + r[1]);
		d[3] = scl*(r[2] - r[3]);
		r[1] = scl*(r[2] + r[3]);
	}
	d[0] = scl*(r[0] + r[1]);
	d[1] = scl*(r[0] - r[1]);
	
	delete[] r;
}

// inverse haar wavelet transform
// d[[0],[0],[0..1],[0..3],..,[0..size/2-1]] -> d[0..size-1]	
void VectorFunctions::ihwt(float* d, int size)
{
	static const float scl = sqrtf(0.5f);
	int h,i,j,k;
	float* r; r = new float[size];
	memcpy(r,d,size*sizeof(float));

	d[1] = scl*(r[0] - r[1]);
	d[0] = scl*(r[0] + r[1]);
	if (size > 2) {
		d[3] = scl*(d[1] - r[3]);
		d[2] = scl*(d[1] + r[3]);
		d[1] = scl*(d[0] - r[2]);
		d[0] = scl*(d[0] + r[2]);
	}
	for (h=4; h<size; h<<=1) {
		for (i=h-4; i>=0; i-=4) {
			j = i<<1; k = h + i;
			d[j+7] = scl*(d[i+3] - r[k+3]);
			d[j+6] = scl*(d[i+3] + r[k+3]);
			d[j+5] = scl*(d[i+2] - r[k+2]);
			d[j+4] = scl*(d[i+2] + r[k+2]);
			d[j+3] = scl*(d[i+1] - r[k+1]);
			d[j+2] = scl*(d[i+1] + r[k+1]);
			d[j+1] = scl*(d[i] - r[k]);
			d[j] = scl*(d[i] + r[k]);
		}
	}

	delete[] r;
}

//******************************************************************************
//* signals + window functions
//*
//* all windows are symmetric: d[0] = d[size-1]
//*
// linear segment
void VectorFunctions::linear(float* d, int size, float start, float end)
{
	if (size == 1) {d[0] = 0.5f*(start + end); return;}
	int i=0;
	double delta = static_cast<double>(end - start)/static_cast<double>(size-1);
	double x0 = static_cast<double>(start);
	double x1 = x0 + delta;
	double x2 = x1 + delta;
	double x3 = x2 + delta;
	double qdelta = 4.0*delta;
#ifndef ICSTLIB_NO_SSEOPT  
	if (reinterpret_cast<uintptr_t>(d) & 0xF) {
#endif
	while (i <= (size-4)) {
		d[i] = static_cast<float>(x0); x0 += qdelta;
		d[i+1] = static_cast<float>(x1); x1 += qdelta;
		d[i+2] = static_cast<float>(x2); x2 += qdelta;
		d[i+3] = static_cast<float>(x3); x3 += qdelta;
		i+=4;
	}
#ifndef ICSTLIB_NO_SSEOPT
	}
	else {
		__m128d mdelta = _mm_set1_pd(qdelta);
		__m128d acc0 = _mm_set_pd(x1 , x0);
		__m128d acc1 = _mm_set_pd(x3 , x2);
		__m128 r0,r1;
		while (i <= (size-4)) {
			r0 = _mm_cvtpd_ps(acc0);
			r1 = _mm_cvtpd_ps(acc1);
			acc0 = _mm_add_pd(acc0 , mdelta);
			acc1 = _mm_add_pd(acc1 , mdelta);
			r0 = _mm_movelh_ps(r0 , r1);
			_mm_store_ps(d+i , r0);
			i+=4;
		}
		_mm_store_sd(&x0 , acc0);
	}	
#endif
	while (i < size) {d[i] = static_cast<float>(x0); x0 += delta; i++;}
}

// exponential segment
void VectorFunctions::exponential(float* d, int size, float start, float end, float time)
{
	if (size == 1) {
		if (time < (-0.5f/logf(FLT_EPSILON))) {d[0] = end;}
		else {d[0] = start + (end-start)/(1.0f + expf(-0.5f/time));}
		return;
	}
	double a,delta,invsize;
	double x = static_cast<double>(start), timed = static_cast<double>(time);
	if (time <= 0) {a = static_cast<double>(end); delta=0;}
	else {
		invsize = 1.0/static_cast<double>(size-1);
		if (timed*sqrt(DBL_EPSILON) > invsize) {	// linear segment to avoid
			linear(d,size,start,end); return;		// ill-conditioned case
		}
		else {
			timed = 1.0/timed;
			a = x + static_cast<double>(end-start)/(1.0-exp(-timed));
			delta = exp(-timed*invsize);
		}
	}
	int i=0; double x0,x1,x2,x3,qdelta;
	x0 = x - a;
	x1 = delta*x0;
	x2 = delta*x1;
	x3 = delta*x2;
	qdelta = delta*delta;
	qdelta *= qdelta;
	while (i <= (size-4)) {
		d[i] = static_cast<float>(x0 + a); x0 *= qdelta;
		d[i+1] = static_cast<float>(x1 + a); x1 *= qdelta;
		d[i+2] = static_cast<float>(x2 + a); x2 *= qdelta;
		d[i+3] = static_cast<float>(x3 + a); x3 *= qdelta;
		i+=4;
	}
	while (i < size) {d[i] = static_cast<float>(x0 + a); x0 *= delta; i++;}
}

// logarithmically spaced data points
void VectorFunctions::logspace(float* d, int size, float start, float end)
{
	int i=0; double x0=0, x1,x2,x3, delta=0, qdelta;
	if (start*end > 0) {
		if (size == 1) {
			if (start > 0) {d[0] = sqrtf(start*end);}
			else {d[0] = -sqrtf(start*end);}
			return;
		}
		x0 = static_cast<double>(start);
		delta = pow(static_cast<double>(end/start),
					1.0/static_cast<double>(size-1));
	}
	x1 = x0*delta;
	x2 = x1*delta;
	x3 = x2*delta;
	qdelta = delta*delta;
	qdelta *= qdelta;
	while (i <= (size-4)) {
		d[i] = static_cast<float>(x0); x0 *= qdelta;
		d[i+1] = static_cast<float>(x1); x1 *= qdelta;
		d[i+2] = static_cast<float>(x2); x2 *= qdelta;
		d[i+3] = static_cast<float>(x3); x3 *= qdelta;
		i+=4;
	}
	while (i < size) {d[i] = static_cast<float>(x0); x0 *= delta; i++;}
}

// sine function
// phase as fraction of a period, center=true: phase relative to center
// return endpoint phasor angle as fraction of a period
float VectorFunctions::sine(float* d, int size, float periods, float phase, 
						bool center)
{
	if (center) {phase -= 0.5f*periods;}
	float endphase = phase + periods; endphase -= floorf(endphase);
	if (size == 1) {d[0] = sinf(2.0f*M_PI_FLOAT*phase); return endphase;}
	double temp = 2.0*M_PI*static_cast<double>(phase);
	double re2, re = cos(temp), im = sin(temp);
	temp = 2.0*M_PI*static_cast<double>(periods)/static_cast<double>(size-1);
	double wre = cos(temp), wim = sin(temp);
	int i, rm = size - ((size>>2)<<2); 
	for (i=0; i<rm; i++) {
		d[i] = static_cast<float>(im);
		temp = re; re = wre*re - wim*im; im = wre*im + wim*temp;
	}
	for (i=rm; i<size; i+=4) {
		d[i] = static_cast<float>(im);
		re2 = wre*re - wim*im; im = wre*im + wim*re;
		d[i+1] = static_cast<float>(im);
		re = wre*re2 - wim*im; im = wre*im + wim*re2;
		d[i+2] = static_cast<float>(im);
		re2 = wre*re - wim*im; im = wre*im + wim*re;
		d[i+3] = static_cast<float>(im);
		re = wre*re2 - wim*im; im = wre*im + wim*re2;
	}
	return endphase;
}

// linear chirp: a version of "sine" with linearly changing frequency 
float VectorFunctions::chirp(float* d, int size, float startpd, float endpd, 
					float phase)
{
	float endphase = phase + 0.5f*(startpd+endpd); endphase -= floorf(endphase);
	if (size == 1) {d[0] = sinf(2.0f*M_PI_FLOAT*phase); return endphase;}
	double temp = 1.0/static_cast<double>(size-1);
	double alpha = 2.0*M_PI*static_cast<double>(startpd)*temp;
	double beta = M_PI*static_cast<double>(endpd-startpd)*temp*temp;
	double hre = cos(alpha+beta), him = sin(alpha+beta);
	double cre = cos(2.0*beta), cim = sin(2.0*beta);
	temp = 2.0*M_PI*static_cast<double>(phase);
	double fre = cos(temp), fim = sin(temp);
	double fre2,hre2;
	int i, rm = size - ((size>>2)<<2);
	for (i=0; i<rm; i++) {
		d[i] = static_cast<float>(fim);
		temp = fre; fre = hre*fre - him*fim; fim = hre*fim + him*temp;	
		temp = hre; hre = cre*hre - cim*him; him = cre*him + cim*temp;
	}
	for (i=rm; i<size; i+=4) {
		d[i] = static_cast<float>(fim);
		fre2 = hre*fre - him*fim; fim = hre*fim + him*fre;	
		hre2 = cre*hre - cim*him; him = cre*him + cim*hre;
		d[i+1] = static_cast<float>(fim);
		fre = hre2*fre2 - him*fim; fim = hre2*fim + him*fre2;	
		hre = cre*hre2 - cim*him; him = cre*him + cim*hre2;
		d[i+2] = static_cast<float>(fim);
		fre2 = hre*fre - him*fim; fim = hre*fim + him*fre;	
		hre2 = cre*hre - cim*him; him = cre*him + cim*hre;
		d[i+3] = static_cast<float>(fim);
		fre = hre2*fre2 - him*fim; fim = hre2*fim + him*fre2;	
		hre = cre*hre2 - cim*him; him = cre*him + cim*hre2;
	}
	return endphase;
}

// exponential chirp: a version of "sine" with exponentially changing frequency
// 4 times faster than sin(x)-based version, absolute error < 1.1e-6 
float VectorFunctions::expchirp(	float* d, int size, float startpd, float endpd,	 
						float phase	)
{
	int i; double alpha=0, delta=1.0, beta,gamma,lambda,spd,epd,ph,x;
	ph = static_cast<double>(phase);
	x = ph = ph - floor(ph);
	spd = static_cast<double>(__max(startpd,FLT_MIN));
	epd = static_cast<double>(__max(endpd,FLT_MIN));
	gamma = 1.0/static_cast<double>(__max(size-1,1));
	lambda = gamma*log(epd/spd);
	beta = exp(lambda);
	if (fabs(lambda) > 1e-7) {gamma *= spd*(beta - 1.0)/lambda;}
	else {gamma *= spd;}	
	for (i=0; i<size; i++) {
		x = ph + gamma*alpha;
		SpecMath::fsplit(x);
		d[i] = static_cast<float>(2.0*M_PI*x); 
		alpha += delta;
		delta *= beta;
	}
	fsin(d,size);
	if (size == 1) {x = ph + gamma*alpha; SpecMath::fsplit(x);}
	return static_cast<float>(x);
}

// complex phasor
// phase=0: d(0) = 1+0j, other parameters s. sine
float VectorFunctions::cpxphasor(float* d, int size, float periods, float phase, 
						bool center)
{
	if (center) {phase -= 0.5f*periods;}
	float endphase = phase + periods; endphase -= floorf(endphase);
	double temp = 2.0*M_PI*static_cast<double>(phase);
	double re2, re = cos(temp), im = sin(temp);
	if (size == 1) {
		d[0] = static_cast<float>(re); d[1] = static_cast<float>(im);
		return endphase;
	}
	temp = 2.0*M_PI*static_cast<double>(periods)/static_cast<double>(size-1);
	double wre = cos(temp), wim = sin(temp);
	int i, rm = 2*(size - ((size>>2)<<2)); 
	for (i=0; i<rm; i+=2) {
		d[i] = static_cast<float>(re); d[i+1] = static_cast<float>(im);
		temp = re; re = wre*re - wim*im; im = wre*im + wim*temp;
	}
	for (i=rm; i<(2*size); i+=8) {
		d[i] = static_cast<float>(re); d[i+1] = static_cast<float>(im);
		re2 = wre*re - wim*im; im = wre*im + wim*re;
		d[i+2] = static_cast<float>(re2); d[i+3] = static_cast<float>(im);
		re = wre*re2 - wim*im; im = wre*im + wim*re2;
		d[i+4] = static_cast<float>(re); d[i+5] = static_cast<float>(im);
		re2 = wre*re - wim*im; im = wre*im + wim*re;
		d[i+6] = static_cast<float>(re2); d[i+7] = static_cast<float>(im);
		re = wre*re2 - wim*im; im = wre*im + wim*re2;
	}
	return endphase;
}

// sawtooth wave
// phase=0: d[0 or center] = 0 (1st occurence from the left in shapes below)  
// symmetry: 0 -> |\|\, 0.5 -> /\/\, 1 -> /|/|
// other parameters s. "sine"
float VectorFunctions::saw(float* d, int size, float periods, float symmetry,
					float phase, bool center)
{
	if (center) {phase -= 0.5f*periods;}
	phase -= floorf(phase);
	float endphase = phase + periods; endphase -= floorf(endphase);
	double dx=0, x = 2.0*static_cast<double>(phase);
	if (size > 1) {
		dx = 2.0*static_cast<double>(periods)/static_cast<double>(size-1);
	}
	float sym = __max(FLT_EPSILON,__min(1.0f-FLT_EPSILON,symmetry));
	float y, c1 = 1.0f/sym, c2 = 1.0f/(sym-1.0f);
	for (int i=0; i<size; i++) {
		if (x > 1.0) {x -= 2.0;} 
		y = static_cast<float>(x);
		x += dx;
		if (fabsf(y) <= sym) {d[i] = c1*y;}
		else {
			if (y >= 0) {d[i] = c2*(y-1.0f);}
			else {d[i] = c2*(y+1.0f);}
		}	
	}
	return endphase;
}

// auxiliary routine for random number generation using SSE2 vector operations
// MWC algorithm with an approximate period of 2^63
#ifndef ICSTLIB_NO_SSEOPT 
void VectorFunctions::randsse(float* d, int size, int n)
{
	static CriticalSection cs;
	static __m128i x[4] = {	_mm_set1_epi32(0), _mm_set1_epi32(0),
							_mm_set1_epi32(0), _mm_set1_epi32(0)	};
	static __m128i c[4] = {	_mm_set_epi32(0, 0xf558103d, 0, 0x2af59645),
							_mm_set_epi32(0, 0x95bd286e, 0, 0xb117c8f0),
							_mm_set_epi32(0, 0x0c25e8ac, 0, 0x7c0fe0da),
							_mm_set_epi32(0, 0x464fc570, 0, 0xacdf036d)	};

	ScopedLock sl(cs);							// single thread access on

	int i=0, j;
	float offset = -3.0f*static_cast<float>(n);
	__m128i msk = _mm_set_epi32(0x0 , 0xffffffff, 0x0, 0xffffffff);
	__m128i cnvmsk1 = _mm_set1_epi32(0x007fffff);
	__m128i cnvmsk2 = _mm_set1_epi32(0x40000000);
	__m128i a = _mm_set1_epi32(0xffffda61);
	__m128i x0 = _mm_load_si128(x);
	__m128i x1 = _mm_load_si128(x+1);
	__m128i x2 = _mm_load_si128(x+2);
	__m128i x3 = _mm_load_si128(x+3);
	__m128i c0 = _mm_load_si128(c);
	__m128i c1 = _mm_load_si128(c+1);
	__m128i c2 = _mm_load_si128(c+2);
	__m128i c3 = _mm_load_si128(c+3);
	__m128i acc0,acc1;
	__m128 facc0,facc1;
	
	do {
		facc0 =  _mm_set1_ps(offset);
		facc1 =  _mm_set1_ps(offset);
		j = n;
		while (j > 0) {
			
			x0 = _mm_mul_epu32(x0 , a);
			x1 = _mm_mul_epu32(x1 , a);
			x2 = _mm_mul_epu32(x2 , a);
			x3 = _mm_mul_epu32(x3 , a);
			
			x0 = _mm_add_epi64(x0 , c0);
			x1 = _mm_add_epi64(x1 , c1);
			x2 = _mm_add_epi64(x2 , c2);
			x3 = _mm_add_epi64(x3 , c3);
			
			c0 = _mm_srli_epi64(x0 , 32);
			c1 = _mm_srli_epi64(x1 , 32);
			c2 = _mm_srli_epi64(x2 , 32);
			c3 = _mm_srli_epi64(x3 , 32);
			
			x0 = _mm_and_si128(x0 , msk);
			x2 = _mm_and_si128(x2 , msk);
			acc0 = _mm_slli_epi64(x1, 32);
			acc1 = _mm_slli_epi64(x3, 32);
			acc0 = _mm_or_si128(acc0, x0);
			acc1 = _mm_or_si128(acc1, x2);	
			
			acc0 = _mm_and_si128(acc0 , cnvmsk1);
			acc1 = _mm_and_si128(acc1 , cnvmsk1);
			acc0 = _mm_or_si128(acc0 , cnvmsk2);
			acc1 = _mm_or_si128(acc1 , cnvmsk2);
			facc0 = _mm_add_ps(facc0, _mm_castsi128_ps( acc0 ));
			facc1 = _mm_add_ps(facc1, _mm_castsi128_ps( acc1 ));
			
			j--;		
		}
		if (i > (size-8)) break;
		_mm_storeu_ps(d+i , facc0);
		_mm_storeu_ps(d+i+4 , facc1);
		i += 8;
	}
	while (i < size);
	if (size & 4) {_mm_storeu_ps(d+i , facc0); facc0 = facc1; i+=4;}
	if (size & 2) {
		_mm_store_ss(d+i , facc0);
		facc0 = _mm_castsi128_ps(_mm_srli_si128(_mm_castps_si128(facc0) , 4));
		_mm_store_ss(d+i+1 , facc0);
		facc0 = _mm_castsi128_ps(_mm_srli_si128(_mm_castps_si128(facc0) , 4));
		i+=2;
	}
	if (size & 1) {_mm_store_ss(d+i , facc0);}
	_mm_store_si128(x , x0);
	_mm_store_si128(x+1 , x1);
	_mm_store_si128(x+2 , x2);
	_mm_store_si128(x+3 , x3);
	_mm_store_si128(c , c0);
	_mm_store_si128(c+1 , c1);
	_mm_store_si128(c+2 , c2);
	_mm_store_si128(c+3 , c3);

	
}
#endif

// noise, uniformly distributed (-1..1) 
void VectorFunctions::unoise(float* d, int size)
{
#ifdef ICSTLIB_NO_SSEOPT
	static CriticalSection cs;
	static unsigned int x = 1;

	cs.Enter();							// single thread access on

	union {float ftmp; unsigned int uitmp;};
	for (int i=0; i<size; i++) {
		x = 663608941*x;
		uitmp = (x >> 9) | 0x40000000;
		d[i] = ftmp - 3.0f;
	}

	cs.Leave();							// single thread access off

#else
	randsse(d, size, 1);
#endif
}

// noise, gaussian with variance 1
// apxorder = number of uniformly distributed random values summed
// to obtain an output value (typ. 10, 2 -> triangular distribution)
void VectorFunctions::gnoise(float* d, int size, int apxorder)
{
	apxorder = __min(apxorder,255);
	float scl = sqrtf(3.0f/static_cast<float>(apxorder));
#ifdef ICSTLIB_NO_SSEOPT
	static CriticalSection cs;
	static unsigned int x = 1;

	cs.Enter();							// single thread access on

	int i,j,temp;
	scl *= (512.0f/static_cast<float>(INT_MAX));
	for (i=0; i<size; i++) {
		temp = 0;
		for (j=0; j<apxorder; j++) {
			x = 663608941*x;
			temp += static_cast<int>(x >> 9);
		}
		temp -= (apxorder<<22);
		d[i] = scl*(static_cast<float>(temp));
	}

	cs.Leave();							// single thread access off

#else
	randsse(d, size, apxorder);
	mul(d, scl, size);
#endif
}		

// noise, exponentially distributed (var=1) positive values			
void VectorFunctions::enoise(float* d, int size)
{
	unoise(d,size);
	for (int i=0; i<size; i++) {d[i] = 0.5f + 0.5f*d[i];}
	logabs(d,size);
	mul(d,-1.0f,size);
}

// noise, standard cauchy distributed
void VectorFunctions::cnoise(float* d, int size)
{	
	static const float c1 = -1.583915135f;
	static const float c2 = 0.269032248f;
	static const float c3 = -1.008330496f;
	int i; float x,y;
	VectorFunctions::unoise(d,size);		// d[i] = uniformly distributed noise -1..1
	for (i=0; i<size; i++) {	// d[i] = approx. tan(pi/2*d[i])
		x = d[i];
		if (fabs(x) <= 0.5f) {
			y = x*x;
			d[i] = x*(c1 + c2*y)/(c3 + y);
		}
		else {
			if(x > 0) {x = 1.00000013f - x;} else {x = -1.00000013f - x;}
			y = x*x;
			d[i] = (c3 + y)/(x*(c1 + c2*y));	
		}
	}
}

// sinc window
void VectorFunctions::sinc(float* d, int size, double periods)
{
	if (size == 1) {d[0] = 1.0f; return;}
	double phi = 2.0*M_PI*periods/static_cast<double>(size-1);
	double wre = cos(phi), wim = sin(phi);
	double x = -M_PI*periods;
	double re = cos(x), im = sin(x);
	double temp;
	for (int i=0; i<(size>>1); i++) {
		d[size-i-1] = d[i] = static_cast<float>(im/x);
		temp = re; re = wre*re - wim*im; im = wre*im + wim*temp;
		x += phi;
	}
	if (size & 1) {d[size>>1] = 1.0f;}
}

// triangular window
void VectorFunctions::triangle(float* d, int size)
{	
	if (size == 1) {d[0] = 1.0f; return;}
	double x=0, delta=2.0/static_cast<double>(size-1);
	for (int i=0; i<=(size>>1); i++) {
		d[size-i-1] = d[i] = static_cast<float>(x); x+=delta;}
}

// generic 2-term trigonometric window
void VectorFunctions::trigwin2(float* d, int size, double c0, double c1)
{
	if (size == 1) {d[0] = static_cast<float>(c0 + c1); return;}
	double temp = 2.0*M_PI/static_cast<double>(size-1);
	double wre = cos(temp), wim = sin(temp);
	double re=-1.0, im=0;
	for (int i=0; i<=(size>>1); i++) {
		d[size-i-1] = d[i] = static_cast<float>(c0 + c1*re);
		temp = re; re = wre*re - wim*im; im = wre*im + wim*temp;
	}
}

// generic 4-term trigonometric window
void VectorFunctions::trigwin4(	float* d, int size, double c0, double c1,
						double c2, double c3						)
{
	if (size == 1) {d[0] = static_cast<float>(c0 + c1 + c2 + c3); return;}
	c0 -= c2; c1 -= (3.0*c3); c2 *= 2.0; c3 *= 4.0;
	double temp = 2.0*M_PI/static_cast<double>(size-1);
	double wre = cos(temp), wim = sin(temp);
	double re=-1.0, im=0;
	for (int i=0; i<=(size>>1); i++) {
		d[size-i-1] = d[i] = static_cast<float> (c0+(c1+(c2+c3*re)*re)*re);
		temp = re; re = wre*re - wim*im; im = wre*im + wim*temp;
	}
}

// hann window
void VectorFunctions::hann(float* d, int size)
	{trigwin2(d, size, 0.5, 0.5);}

// hamming window		
void VectorFunctions::hamming(float* d, int size)
	{trigwin2(d, size, 0.53836, 0.46164);}

// blackman window
void VectorFunctions::blackman(float* d, int size)
		{trigwin4(d, size, 0.42, 0.5, 0.08, 0);}
			
// 3-term blackman-harris window
void VectorFunctions::bhw3(float* d, int size)
	{trigwin4(d, size, 0.42323, 0.49755, 0.07922, 0);}

// 4-term blackman-harris window
void VectorFunctions::bhw4(float* d, int size)
	{trigwin4(d, size, 0.35875, 0.48829, 0.14128, 0.01168);}

// 5-term flat top window
void VectorFunctions::flattop(float* d, int size)
{
	if (size == 1) {d[0] = 1.0f; return;}
	const double c0=-0.0555, c1=0.1651, c2=0.5005, c3=0.3344, c4=0.0555;
	double temp = 2.0*M_PI/static_cast<double>(size-1);
	double wre = cos(temp), wim = sin(temp);
	double re=-1.0, im=0;
	for (int i=0; i<=(size>>1); i++) {
		d[size-i-1] = d[i] = static_cast<float>(c0+(c1+(c2+(c3+c4*re)*re)*re)*re);
		temp = re; re = wre*re - wim*im; im = wre*im + wim*temp;
	}		
}

// gaussian window, sigma = standard deviation
void VectorFunctions::gauss(float* d, int size, double sigma)
{
	double a,b,x;
	double temp = sigma*static_cast<double>(size);
	temp *= temp;
	if (size & 1) {a = exp(-2.0/temp); b = a*a; x = 1.0;}
	else {x = exp(-0.5/temp); a = x*x; a *= a; a *= a; b = a;}
	for (int i=(size>>1); i<size; i++) {
		d[size-i-1] = d[i] = static_cast<float>(x);
		x *= a; a *= b;	
	}
}

// kaiser window
void VectorFunctions::kaiser(float* d, int size,	double alpha)
{
	if (size == 1) {d[0] = 1.0f; return;}
	double delta = 2.0/static_cast<double>(size-1);
	double n = 1.0/bessi0(alpha);
	double x=0;
	for (int i=0; i<=(size>>1); i++) {
		d[size-i-1] = d[i] = static_cast<float>(n*bessi0(alpha*sqrt((2.0-x)*x)));
		x += delta;
	}
}

//******************************************************************************
//* elementary real array operations
//*
// |d| -> d
void VectorFunctions::abs(float* d, int size)
{
	int i=0;
#ifndef ICSTLIB_NO_SSEOPT  
	if (reinterpret_cast<uintptr_t>(d) & 0xF) {
#endif
		for (i=0; i<size; i++) {d[i] = fabsf(d[i]);}
#ifndef ICSTLIB_NO_SSEOPT
	}
	else {
		__m128 r0,r1,r2,r3;
		__m128 absmask = _mm_castsi128_ps(_mm_set1_epi32(0x7FFFFFFF));
		while (i <= (size-16)) {
			r0 = _mm_load_ps(d+i);
			r0 = _mm_and_ps(r0 , absmask);
			r1 = _mm_load_ps(d+i+4);
			r1 = _mm_and_ps(r1 , absmask);
			r2 = _mm_load_ps(d+i+8);
			r2 = _mm_and_ps(r2 , absmask);
			_mm_store_ps(d+i , r0);
			r3 = _mm_load_ps(d+i+12);
			r3 = _mm_and_ps(r3 , absmask);
			_mm_store_ps(d+i+4 , r1);
			_mm_store_ps(d+i+8 , r2);
			_mm_store_ps(d+i+12 , r3);
			i += 16;
		}
		if (size & 8) {
			r0 = _mm_load_ps(d+i);
			r1 = _mm_load_ps(d+i+4);
			r0 = _mm_and_ps(r0 , absmask);
			r1 = _mm_and_ps(r1 , absmask);
			_mm_store_ps(d+i , r0);
			_mm_store_ps(d+i+4 , r1);
			i += 8;
		}
		if (size & 4) {
			r0 = _mm_load_ps(d+i);
			r0 = _mm_and_ps(r0 , absmask);
			_mm_store_ps(d+i , r0);
			i += 4;
		}
		if (size & 2) {d[i] = fabsf(d[i]); d[i+1] = fabsf(d[i+1]); i+=2;}
		if (size & 1) {d[i] = fabsf(d[i]);}
	}
#endif	
}	

// sgn(d) -> d
// 1 or -1, the sign of 0 follows the IEEE 754 float definition
void VectorFunctions::sgn(float* d, int size)
{
#ifdef ICSTLIB_NO_SSEOPT 
	union {float a; int b;};
	for (int i=0; i<size; i++) {
		a = d[i];
		b = (b & 0x80000000) | 0x3f800000;
		d[i] = a;
	}
#else
	int i=0;
	__m128 r0,r1;
	__m128 andmask = _mm_castsi128_ps(_mm_set1_epi32(0x80000000));
	__m128 ormask = _mm_castsi128_ps(_mm_set1_epi32(0x3f800000));
	if (!(reinterpret_cast<uintptr_t>(d) & 0xF)) {
		while (i <= (size-8)) {
			r0 = _mm_load_ps(d+i);
			r1 = _mm_load_ps(d+i+4);
			r0 = _mm_and_ps(r0 , andmask);
			r0 = _mm_or_ps(r0 , ormask);
			r1 = _mm_and_ps(r1 , andmask);
			r1 = _mm_or_ps(r1 , ormask);
			_mm_store_ps(d+i , r0);
			_mm_store_ps(d+i+4 , r1);
			i += 8;
		}
	}
	else {
		while (i <= (size-8)) {
			r0 = _mm_loadu_ps(d+i);
			r1 = _mm_loadu_ps(d+i+4);
			r0 = _mm_and_ps(r0 , andmask);
			r0 = _mm_or_ps(r0 , ormask);
			r1 = _mm_and_ps(r1 , andmask);
			r1 = _mm_or_ps(r1 , ormask);
			_mm_storeu_ps(d+i , r0);
			_mm_storeu_ps(d+i+4 , r1);
			i += 8;
		}
	}
	if (size & 4) {
		r0 = _mm_loadu_ps(d+i);
		r0 = _mm_and_ps(r0 , andmask);
		r0 = _mm_or_ps(r0 , ormask);
		_mm_storeu_ps(d+i , r0);
		i += 4;
	}
	if (size & 2) {
		r0 = _mm_load_ss(d+i);
		r0 = _mm_and_ps(r0 , andmask);
		r0 = _mm_or_ps(r0 , ormask);
		_mm_store_ss(d+i , r0);
		r1 = _mm_load_ss(d+i+1);
		r1 = _mm_and_ps(r1 , andmask);
		r1 = _mm_or_ps(r1 , ormask);
		_mm_store_ss(d+i+1 , r1);
		i += 2;
	}
	if (size & 1) {
		r0 = _mm_load_ss(d+i);
		r0 = _mm_and_ps(r0 , andmask);
		r0 = _mm_or_ps(r0 , ormask);
		_mm_store_ss(d+i , r0);
	}
#endif	
}				

// fast reciprocal
void VectorFunctions::finv(float* d, int size)
{
#ifdef ICSTLIB_NO_SSEOPT  
	for (int i=0; i<size; i++) {d[i] = 1.0f/d[i];}
#else
	__m128 r0,r1,r2;
	int i=0;
	while (i <= (size-4)) {
		r0 = _mm_loadu_ps(d+i);
		r1 = _mm_rcp_ps(r0);
		r0 = _mm_mul_ps(r0 , r1);
		r2 = _mm_add_ps(r1 , r1);
		r0 = _mm_mul_ps(r0 , r1);
		r2 = _mm_sub_ps(r2 , r0);
		_mm_storeu_ps(d+i , r2);
		i+=4;
	}
	while (i < size) {
		r0 = _mm_load_ss(d+i);
		r1 = _mm_rcp_ss(r0);
		r0 = _mm_mul_ss(r0 , r1);
		r2 = _mm_add_ss(r1 , r1);
		r0 = _mm_mul_ss(r0 , r1);
		r2 = _mm_sub_ss(r2 , r0);
		_mm_store_ss(d+i , r2);
		i++;
	}
#endif		
}	

// fast square root
// full precision, faster than rsqrt-newton version on Core2
void VectorFunctions::fsqrt(float* d, int size)
{
#ifdef ICSTLIB_NO_SSEOPT  
	for (int i=0; i<size; i++) {d[i] = sqrtf(d[i]);}
#else
	__m128 r0; int i=0;
	while (i <= (size - 4)) {
		r0 = _mm_loadu_ps(d+i);
		r0 = _mm_sqrt_ps(r0);
		_mm_storeu_ps(d+i , r0);
		i+=4;
	}		
	while (i < size) {
		r0 = _mm_load_ss(d+i);
		r0 = _mm_sqrt_ss(r0);
		_mm_store_ss(d+i , r0);
		i++;
	}
#endif
}

// fast cosine
// absolute error < 1e-6 for |d| < 2pi
void VectorFunctions::fcos(float* d, int size)
{
#ifdef ICSTLIB_NO_SSEOPT
	for (int i=0; i<size; i++) {d[i] = cosf(d[i]);}
#else
	__m128 r0, r1, r2, r3, r4, r5;
	__m128 absmask = _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff));
	__m128i tmp1,tmp2;
	int i=0;
	if (!(reinterpret_cast<uintptr_t>(d) & 0xF)) {
		while (i <= (size - 8)) {
			r0 = _mm_load_ps(d+i);
			r3 = _mm_load_ps(d+i+4);
			r0 = _mm_mul_ps(r0 , _mm_set1_ps(0.5f/M_PI_FLOAT));
			r3 = _mm_mul_ps(r3 , _mm_set1_ps(0.5f/M_PI_FLOAT));
			tmp1 = _mm_cvttps_epi32(r0);
			tmp2 = _mm_cvttps_epi32(r3);
			r1 = _mm_cvtepi32_ps(tmp1);
			r4 = _mm_cvtepi32_ps(tmp2);
			r0 = _mm_sub_ps(r0 , r1);
			r3 = _mm_sub_ps(r3 , r4);
			r0 = _mm_and_ps(r0 , absmask);
			r0 = _mm_mul_ps(r0 , _mm_set1_ps(4.0f));
			r3 = _mm_and_ps(r3 , absmask);
			r3 = _mm_mul_ps(r3 , _mm_set1_ps(4.0f));
			r0 = _mm_sub_ps(r0 , _mm_set1_ps(2.0f));
			r3 = _mm_sub_ps(r3 , _mm_set1_ps(2.0f));
			r0 = _mm_and_ps(r0 , absmask);
			r0 = _mm_sub_ps(r0 , _mm_set1_ps(1.0f));
			r3 = _mm_and_ps(r3 , absmask);
			r3 = _mm_sub_ps(r3 , _mm_set1_ps(1.0f));
			r1 = _mm_mul_ps(r0 , r0);
			r4 = _mm_mul_ps(r3 , r3);
			r2 = _mm_mul_ps(r1 , _mm_set1_ps(-0.0043311f));
			r5 = _mm_mul_ps(r4 , _mm_set1_ps(-0.0043311f));
			r2 = _mm_add_ps(r2 , _mm_set1_ps(0.0794309f));
			r5 = _mm_add_ps(r5 , _mm_set1_ps(0.0794309f));
			r2 = _mm_mul_ps(r2 , r1);
			r5 = _mm_mul_ps(r5 , r4);
			r2 = _mm_add_ps(r2 , _mm_set1_ps(-0.6458911f));
			r5 = _mm_add_ps(r5 , _mm_set1_ps(-0.6458911f));
			r2 = _mm_mul_ps(r2 , r1);
			r5 = _mm_mul_ps(r5 , r4);
			r2 = _mm_add_ps(r2 , _mm_set1_ps(1.5707908f));
			r5 = _mm_add_ps(r5 , _mm_set1_ps(1.5707908f));
			r2 = _mm_mul_ps(r2 , r0);
			r5 = _mm_mul_ps(r5 , r3);
			_mm_store_ps(d+i , r2);
			_mm_store_ps(d+i+4 , r5);
			i+=8;
		}			
		if (size & 4) {
			r0 = _mm_load_ps(d+i);
			r0 = _mm_mul_ps(r0 , _mm_set1_ps(0.5f/M_PI_FLOAT));
			tmp1 = _mm_cvttps_epi32(r0);
			r1 = _mm_cvtepi32_ps(tmp1);
			r0 = _mm_sub_ps(r0 , r1);
			r0 = _mm_and_ps(r0 , _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff)));
			r0 = _mm_mul_ps(r0 , _mm_set1_ps(4.0f));
			r0 = _mm_sub_ps(r0 , _mm_set1_ps(2.0f));
			r0 = _mm_and_ps(r0 , _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff)));
			r0 = _mm_sub_ps(r0 , _mm_set1_ps(1.0f));
			r1 = _mm_mul_ps(r0 , r0);
			r2 = _mm_mul_ps(r1 , _mm_set1_ps(-0.0043311f));
			r2 = _mm_add_ps(r2 , _mm_set1_ps(0.0794309f));
			r2 = _mm_mul_ps(r2 , r1);
			r2 = _mm_add_ps(r2 , _mm_set1_ps(-0.6458911f));
			r2 = _mm_mul_ps(r2 , r1);
			r2 = _mm_add_ps(r2 , _mm_set1_ps(1.5707908f));
			r2 = _mm_mul_ps(r2 , r0);
			_mm_store_ps(d+i , r2);
			i+=4;
		}
	}
	else {
		while (i <= (size - 8)) {
			r0 = _mm_loadu_ps(d+i);
			r3 = _mm_loadu_ps(d+i+4);
			r0 = _mm_mul_ps(r0 , _mm_set1_ps(0.5f/M_PI_FLOAT));
			r3 = _mm_mul_ps(r3 , _mm_set1_ps(0.5f/M_PI_FLOAT));
			tmp1 = _mm_cvttps_epi32(r0);
			tmp2 = _mm_cvttps_epi32(r3);
			r1 = _mm_cvtepi32_ps(tmp1);
			r4 = _mm_cvtepi32_ps(tmp2);
			r0 = _mm_sub_ps(r0 , r1);
			r3 = _mm_sub_ps(r3 , r4);
			r0 = _mm_and_ps(r0 , absmask);
			r0 = _mm_mul_ps(r0 , _mm_set1_ps(4.0f));
			r3 = _mm_and_ps(r3 , absmask);
			r3 = _mm_mul_ps(r3 , _mm_set1_ps(4.0f));
			r0 = _mm_sub_ps(r0 , _mm_set1_ps(2.0f));
			r3 = _mm_sub_ps(r3 , _mm_set1_ps(2.0f));
			r0 = _mm_and_ps(r0 , absmask);
			r0 = _mm_sub_ps(r0 , _mm_set1_ps(1.0f));
			r3 = _mm_and_ps(r3 , absmask);
			r3 = _mm_sub_ps(r3 , _mm_set1_ps(1.0f));
			r1 = _mm_mul_ps(r0 , r0);
			r4 = _mm_mul_ps(r3 , r3);
			r2 = _mm_mul_ps(r1 , _mm_set1_ps(-0.0043311f));
			r5 = _mm_mul_ps(r4 , _mm_set1_ps(-0.0043311f));
			r2 = _mm_add_ps(r2 , _mm_set1_ps(0.0794309f));
			r5 = _mm_add_ps(r5 , _mm_set1_ps(0.0794309f));
			r2 = _mm_mul_ps(r2 , r1);
			r5 = _mm_mul_ps(r5 , r4);
			r2 = _mm_add_ps(r2 , _mm_set1_ps(-0.6458911f));
			r5 = _mm_add_ps(r5 , _mm_set1_ps(-0.6458911f));
			r2 = _mm_mul_ps(r2 , r1);
			r5 = _mm_mul_ps(r5 , r4);
			r2 = _mm_add_ps(r2 , _mm_set1_ps(1.5707908f));
			r5 = _mm_add_ps(r5 , _mm_set1_ps(1.5707908f));
			r2 = _mm_mul_ps(r2 , r0);
			r5 = _mm_mul_ps(r5 , r3);
			_mm_storeu_ps(d+i , r2);
			_mm_storeu_ps(d+i+4 , r5);
			i+=8;
		}			
		if (size & 4) {
			r0 = _mm_loadu_ps(d+i);
			r0 = _mm_mul_ps(r0 , _mm_set1_ps(0.5f/M_PI_FLOAT));
			tmp1 = _mm_cvttps_epi32(r0);
			r1 = _mm_cvtepi32_ps(tmp1);
			r0 = _mm_sub_ps(r0 , r1);
			r0 = _mm_and_ps(r0 , _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff)));
			r0 = _mm_mul_ps(r0 , _mm_set1_ps(4.0f));
			r0 = _mm_sub_ps(r0 , _mm_set1_ps(2.0f));
			r0 = _mm_and_ps(r0 , _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff)));
			r0 = _mm_sub_ps(r0 , _mm_set1_ps(1.0f));
			r1 = _mm_mul_ps(r0 , r0);
			r2 = _mm_mul_ps(r1 , _mm_set1_ps(-0.0043311f));
			r2 = _mm_add_ps(r2 , _mm_set1_ps(0.0794309f));
			r2 = _mm_mul_ps(r2 , r1);
			r2 = _mm_add_ps(r2 , _mm_set1_ps(-0.6458911f));
			r2 = _mm_mul_ps(r2 , r1);
			r2 = _mm_add_ps(r2 , _mm_set1_ps(1.5707908f));
			r2 = _mm_mul_ps(r2 , r0);
			_mm_storeu_ps(d+i , r2);
			i+=4;
		}
	}
	while (i < size) {d[i] = cosf(d[i]); i++;}
#endif
}
	
// fast sine
// absolute error < 1.1e-6 for |d| < 2pi
void VectorFunctions::fsin(float* d, int size)
{
#ifdef ICSTLIB_NO_SSEOPT  
		for (int i=0; i<size; i++) {d[i] = sinf(d[i]);}
#else
		add(d, -0.5f*M_PI_FLOAT, size);
		fcos(d, size);
#endif		
}

// ln|d| -> d
// absolute error: 4e-7
// (optimization note: although the SSE code looks like causing dependency stalls,
//  the routine is very fast on Core2 and Pentium M CPUs, probably due to efficient
//  register renaming and out-of-order execution, performance on P4 yet unknown)
void VectorFunctions::logabs(float* d, int size)
{
	static const float ln2 = logf(2.0f);
#ifdef ICSTLIB_NO_SSEOPT
	static const float inv2pow23 = 1.0f/8388608.0f;
	float p;
	union {float a; int x;};
	for (int i=0; i<size; i++) {
		a = d[i];
		p = inv2pow23*static_cast<float>(x & 0x007fffff);
		p = (0.999974853f + (-0.499365619f + (0.327615235f + (-0.224147341f
			+ (0.132157178f + (-0.053340996f +	0.010253873f*p)*p)*p)*p)*p)*p)*p;
		d[i] = p + ln2*static_cast<float>(((x >> 23) & 0x000000ff) - 127);
	}		
#else
	__m128 r0,r1,r2,c1,c2,c3,msk1,msk2,msk3;
	msk1 = _mm_castsi128_ps(_mm_set1_epi32(0x007fffff));
	msk2 = _mm_castsi128_ps(_mm_set1_epi32(0x3f800000));
	msk3 = _mm_castsi128_ps(_mm_set1_epi32(0x7f800000));
	c1 = _mm_set1_ps(1.0f);
	c2 = _mm_set1_ps(1.0f + 127.0f/256.0f);
	c3 = _mm_set1_ps(256.0f*ln2);
	int i=0;
	if (!(reinterpret_cast<uintptr_t>(d) & 0xF)) {
		while (i <= (size-4)) {
			r0 = _mm_load_ps(d+i);
			r1 = _mm_and_ps(r0 , msk1);
			r1 = _mm_or_ps(r1 , msk2);
			r1 = _mm_sub_ps(r1 , c1);
			r0 = _mm_and_ps(r0 , msk3);
			r0 = _mm_castsi128_ps(_mm_srli_epi32(_mm_castps_si128(r0) , 8));
			r2 = _mm_mul_ps(r1 , _mm_set1_ps(0.010253873f));
			r0 = _mm_or_ps(r0 , msk2);
			r0 = _mm_sub_ps(r0 , c2);
			r2 = _mm_add_ps(r2 , _mm_set1_ps(-0.053340996f));
			r2 = _mm_mul_ps(r2 , r1);
			r0 = _mm_mul_ps(r0 , c3);
			r2 = _mm_add_ps(r2 , _mm_set1_ps(0.132157178f));
			r2 = _mm_mul_ps(r2 , r1);
			r2 = _mm_add_ps(r2 , _mm_set1_ps(-0.224147341f));
			r2 = _mm_mul_ps(r2 , r1);
			r2 = _mm_add_ps(r2 , _mm_set1_ps(0.327615235f));
			r2 = _mm_mul_ps(r2 , r1);
			r2 = _mm_add_ps(r2 , _mm_set1_ps(-0.499365619f));
			r2 = _mm_mul_ps(r2 , r1);
			r2 = _mm_add_ps(r2 , _mm_set1_ps(0.999974853f));
			r2 = _mm_mul_ps(r2 , r1);
			r2 = _mm_add_ps(r2 , r0);
			_mm_store_ps(d+i , r2);
			i += 4;
		}
	}
	else {
		while (i <= (size-4)) {
			r0 = _mm_loadu_ps(d+i);
			r1 = _mm_and_ps(r0 , msk1);
			r1 = _mm_or_ps(r1 , msk2);
			r1 = _mm_sub_ps(r1 , c1);
			r0 = _mm_and_ps(r0 , msk3);
			r0 = _mm_castsi128_ps(_mm_srli_epi32(_mm_castps_si128(r0) , 8));
			r2 = _mm_mul_ps(r1 , _mm_set1_ps(0.010253873f));
			r0 = _mm_or_ps(r0 , msk2);
			r0 = _mm_sub_ps(r0 , c2);
			r2 = _mm_add_ps(r2 , _mm_set1_ps(-0.053340996f));
			r2 = _mm_mul_ps(r2 , r1);
			r0 = _mm_mul_ps(r0 , c3);
			r2 = _mm_add_ps(r2 , _mm_set1_ps(0.132157178f));
			r2 = _mm_mul_ps(r2 , r1);
			r2 = _mm_add_ps(r2 , _mm_set1_ps(-0.224147341f));
			r2 = _mm_mul_ps(r2 , r1);
			r2 = _mm_add_ps(r2 , _mm_set1_ps(0.327615235f));
			r2 = _mm_mul_ps(r2 , r1);
			r2 = _mm_add_ps(r2 , _mm_set1_ps(-0.499365619f));
			r2 = _mm_mul_ps(r2 , r1);
			r2 = _mm_add_ps(r2 , _mm_set1_ps(0.999974853f));
			r2 = _mm_mul_ps(r2 , r1);
			r2 = _mm_add_ps(r2 , r0);
			_mm_storeu_ps(d+i , r2);
			i += 4;
		}
	}
	while (i < size) {
		r0 = _mm_load_ss(d+i);
		r1 = _mm_and_ps(r0 , msk1);
		r1 = _mm_or_ps(r1 , msk2);
		r1 = _mm_sub_ss(r1 , c1);
		r0 = _mm_and_ps(r0 , msk3);
		r0 = _mm_castsi128_ps(_mm_srli_epi32(_mm_castps_si128(r0) , 8));
		r2 = _mm_mul_ss(r1 , _mm_set_ss(0.010253873f));
		r0 = _mm_or_ps(r0 , msk2);
		r0 = _mm_sub_ss(r0 , c2);
		r2 = _mm_add_ss(r2 , _mm_set_ss(-0.053340996f));
		r2 = _mm_mul_ss(r2 , r1);
		r0 = _mm_mul_ss(r0 , c3);
		r2 = _mm_add_ss(r2 , _mm_set_ss(0.132157178f));
		r2 = _mm_mul_ss(r2 , r1);
		r2 = _mm_add_ss(r2 , _mm_set_ss(-0.224147341f));
		r2 = _mm_mul_ss(r2 , r1);
		r2 = _mm_add_ss(r2 , _mm_set_ss(0.327615235f));
		r2 = _mm_mul_ss(r2 , r1);
		r2 = _mm_add_ss(r2 , _mm_set_ss(-0.499365619f));
		r2 = _mm_mul_ss(r2 , r1);
		r2 = _mm_add_ss(r2 , _mm_set_ss(0.999974853f));
		r2 = _mm_mul_ss(r2 , r1);
		r2 = _mm_add_ss(r2 , r0);
		_mm_store_ss(d+i , r2);
		i++;
	}		
#endif		
}

// exp(d) -> d
// relative error: 3e-7	
void VectorFunctions::fexp(float* d, int size)
{
#ifdef ICSTLIB_NO_SSEOPT  
	for (int i=0; i<size; i++) {d[i] = expf(d[i]);}
#else
	int i=0;
	__m128 r0,r1,r2,r3,r4,r5,r6,r7,a0,a1,a2,a3,a4,a5;
	__m128 xmin = _mm_set1_ps(-87.4f);
	__m128 xmax = _mm_set1_ps(88.8f);
	__m128 rln2 = _mm_set1_ps(1.44269504f);
	__m128i e0,e2,e4,e6;
	__m128i c127 = _mm_set1_epi32(127);
	#ifndef ICSTLIB_DEF_ROUND
		__m128 signmask = _mm_castsi128_ps(_mm_set1_epi32(0x80000000));
		__m128 onediv2 = _mm_set1_ps(0.5f);
	#endif
	a0 = _mm_set1_ps(1.00000008f);
	a1 = _mm_set1_ps(0.69314727f);
	a2 = _mm_set1_ps(0.24022075f);
	a3 = _mm_set1_ps(0.05550198f);
	a4 = _mm_set1_ps(0.00967839f);
	a5 = _mm_set1_ps(0.00134508f);
	if (!(reinterpret_cast<uintptr_t>(d) & 0xF)) {
		while (i <= (size-16)) {
			r0 = _mm_load_ps(d+i);
			r2 = _mm_load_ps(d+i+4);
			r4 = _mm_load_ps(d+i+8);
			r6 = _mm_load_ps(d+i+12);
			r0 = _mm_max_ps(r0 , xmin);
			r2 = _mm_max_ps(r2 , xmin);
			r4 = _mm_max_ps(r4 , xmin);
			r6 = _mm_max_ps(r6 , xmin);
			r0 = _mm_min_ps(r0 , xmax);
			r2 = _mm_min_ps(r2 , xmax);
			r4 = _mm_min_ps(r4 , xmax);
			r6 = _mm_min_ps(r6 , xmax);
			r0 = _mm_mul_ps(r0 , rln2);
			r2 = _mm_mul_ps(r2 , rln2);
			r4 = _mm_mul_ps(r4 , rln2);
			r6 = _mm_mul_ps(r6 , rln2);
			#ifdef ICSTLIB_DEF_ROUND
				e0 = _mm_cvtps_epi32(r0);
				e2 = _mm_cvtps_epi32(r2);
				e4 = _mm_cvtps_epi32(r4);
				e6 = _mm_cvtps_epi32(r6);
			#else
				r1 = _mm_and_ps(r0 , signmask);
				r3 = _mm_and_ps(r2 , signmask);
				r5 = _mm_and_ps(r4 , signmask);
				r7 = _mm_and_ps(r6 , signmask);
				r1 = _mm_xor_ps(r1 , onediv2);
				r3 = _mm_xor_ps(r3 , onediv2);
				r5 = _mm_xor_ps(r5 , onediv2);
				r7 = _mm_xor_ps(r7 , onediv2);
				r1 = _mm_add_ps(r1 , r0);
				r3 = _mm_add_ps(r3 , r2);
				r5 = _mm_add_ps(r5 , r4);
				r7 = _mm_add_ps(r7 , r6);
				e0 = _mm_cvttps_epi32(r1);
				e2 = _mm_cvttps_epi32(r3);
				e4 = _mm_cvttps_epi32(r5);
				e6 = _mm_cvttps_epi32(r7);
			#endif
			r1 = _mm_cvtepi32_ps(e0);
			r3 = _mm_cvtepi32_ps(e2);
			r5 = _mm_cvtepi32_ps(e4);
			r7 = _mm_cvtepi32_ps(e6);
			r0 = _mm_sub_ps(r0 , r1);
			r2 = _mm_sub_ps(r2 , r3);
			r4 = _mm_sub_ps(r4 , r5);
			r6 = _mm_sub_ps(r6 , r7);
			e0 = _mm_add_epi32(e0 , c127);
			e2 = _mm_add_epi32(e2 , c127);
			e4 = _mm_add_epi32(e4 , c127);
			e6 = _mm_add_epi32(e6 , c127);
			e0 = _mm_slli_epi32(e0 , 23);
			e2 = _mm_slli_epi32(e2 , 23);
			e4 = _mm_slli_epi32(e4 , 23);
			e6 = _mm_slli_epi32(e6 , 23);
			r1 = _mm_mul_ps(r0 , a5);
			r3 = _mm_mul_ps(r2 , a5);
			r5 = _mm_mul_ps(r4 , a5);
			r7 = _mm_mul_ps(r6 , a5);
			r1 = _mm_add_ps(r1 , a4);
			r3 = _mm_add_ps(r3 , a4);
			r5 = _mm_add_ps(r5 , a4);
			r7 = _mm_add_ps(r7 , a4);
			r1 = _mm_mul_ps(r1 , r0);
			r3 = _mm_mul_ps(r3 , r2);
			r5 = _mm_mul_ps(r5 , r4);
			r7 = _mm_mul_ps(r7 , r6);
			r1 = _mm_add_ps(r1 , a3);
			r3 = _mm_add_ps(r3 , a3);
			r5 = _mm_add_ps(r5 , a3);
			r7 = _mm_add_ps(r7 , a3);
			r1 = _mm_mul_ps(r1 , r0);
			r3 = _mm_mul_ps(r3 , r2);
			r5 = _mm_mul_ps(r5 , r4);
			r7 = _mm_mul_ps(r7 , r6);
			r1 = _mm_add_ps(r1 , a2);
			r3 = _mm_add_ps(r3 , a2);
			r5 = _mm_add_ps(r5 , a2);
			r7 = _mm_add_ps(r7 , a2);
			r1 = _mm_mul_ps(r1 , r0);
			r3 = _mm_mul_ps(r3 , r2);
			r5 = _mm_mul_ps(r5 , r4);
			r7 = _mm_mul_ps(r7 , r6);
			r1 = _mm_add_ps(r1 , a1);
			r3 = _mm_add_ps(r3 , a1);
			r5 = _mm_add_ps(r5 , a1);
			r7 = _mm_add_ps(r7 , a1);
			r1 = _mm_mul_ps(r1 , r0);
			r3 = _mm_mul_ps(r3 , r2);
			r5 = _mm_mul_ps(r5 , r4);
			r7 = _mm_mul_ps(r7 , r6);
			r1 = _mm_add_ps(r1 , a0);
			r3 = _mm_add_ps(r3 , a0);
			r5 = _mm_add_ps(r5 , a0);
			r7 = _mm_add_ps(r7 , a0);
			r1 = _mm_mul_ps(r1 , _mm_castsi128_ps(e0));
			r3 = _mm_mul_ps(r3 , _mm_castsi128_ps(e2));
			r5 = _mm_mul_ps(r5 , _mm_castsi128_ps(e4));
			r7 = _mm_mul_ps(r7 , _mm_castsi128_ps(e6));
			_mm_store_ps(d+i , r1);
			_mm_store_ps(d+i+4 , r3);
			_mm_store_ps(d+i+8 , r5);
			_mm_store_ps(d+i+12 , r7);
			i+=16;
		}
	}
	else {
		while (i <= (size-16)) {
			r0 = _mm_loadu_ps(d+i);
			r2 = _mm_loadu_ps(d+i+4);
			r4 = _mm_loadu_ps(d+i+8);
			r6 = _mm_loadu_ps(d+i+12);
			r0 = _mm_max_ps(r0 , xmin);
			r2 = _mm_max_ps(r2 , xmin);
			r4 = _mm_max_ps(r4 , xmin);
			r6 = _mm_max_ps(r6 , xmin);
			r0 = _mm_min_ps(r0 , xmax);
			r2 = _mm_min_ps(r2 , xmax);
			r4 = _mm_min_ps(r4 , xmax);
			r6 = _mm_min_ps(r6 , xmax);
			r0 = _mm_mul_ps(r0 , rln2);
			r2 = _mm_mul_ps(r2 , rln2);
			r4 = _mm_mul_ps(r4 , rln2);
			r6 = _mm_mul_ps(r6 , rln2);
			#ifdef ICSTLIB_DEF_ROUND
				e0 = _mm_cvtps_epi32(r0);
				e2 = _mm_cvtps_epi32(r2);
				e4 = _mm_cvtps_epi32(r4);
				e6 = _mm_cvtps_epi32(r6);
			#else
				r1 = _mm_and_ps(r0 , signmask);
				r3 = _mm_and_ps(r2 , signmask);
				r5 = _mm_and_ps(r4 , signmask);
				r7 = _mm_and_ps(r6 , signmask);
				r1 = _mm_xor_ps(r1 , onediv2);
				r3 = _mm_xor_ps(r3 , onediv2);
				r5 = _mm_xor_ps(r5 , onediv2);
				r7 = _mm_xor_ps(r7 , onediv2);
				r1 = _mm_add_ps(r1 , r0);
				r3 = _mm_add_ps(r3 , r2);
				r5 = _mm_add_ps(r5 , r4);
				r7 = _mm_add_ps(r7 , r6);
				e0 = _mm_cvttps_epi32(r1);
				e2 = _mm_cvttps_epi32(r3);
				e4 = _mm_cvttps_epi32(r5);
				e6 = _mm_cvttps_epi32(r7);
			#endif
			r1 = _mm_cvtepi32_ps(e0);
			r3 = _mm_cvtepi32_ps(e2);
			r5 = _mm_cvtepi32_ps(e4);
			r7 = _mm_cvtepi32_ps(e6);
			r0 = _mm_sub_ps(r0 , r1);
			r2 = _mm_sub_ps(r2 , r3);
			r4 = _mm_sub_ps(r4 , r5);
			r6 = _mm_sub_ps(r6 , r7);
			e0 = _mm_add_epi32(e0 , c127);
			e2 = _mm_add_epi32(e2 , c127);
			e4 = _mm_add_epi32(e4 , c127);
			e6 = _mm_add_epi32(e6 , c127);
			e0 = _mm_slli_epi32(e0 , 23);
			e2 = _mm_slli_epi32(e2 , 23);
			e4 = _mm_slli_epi32(e4 , 23);
			e6 = _mm_slli_epi32(e6 , 23);
			r1 = _mm_mul_ps(r0 , a5);
			r3 = _mm_mul_ps(r2 , a5);
			r5 = _mm_mul_ps(r4 , a5);
			r7 = _mm_mul_ps(r6 , a5);
			r1 = _mm_add_ps(r1 , a4);
			r3 = _mm_add_ps(r3 , a4);
			r5 = _mm_add_ps(r5 , a4);
			r7 = _mm_add_ps(r7 , a4);
			r1 = _mm_mul_ps(r1 , r0);
			r3 = _mm_mul_ps(r3 , r2);
			r5 = _mm_mul_ps(r5 , r4);
			r7 = _mm_mul_ps(r7 , r6);
			r1 = _mm_add_ps(r1 , a3);
			r3 = _mm_add_ps(r3 , a3);
			r5 = _mm_add_ps(r5 , a3);
			r7 = _mm_add_ps(r7 , a3);
			r1 = _mm_mul_ps(r1 , r0);
			r3 = _mm_mul_ps(r3 , r2);
			r5 = _mm_mul_ps(r5 , r4);
			r7 = _mm_mul_ps(r7 , r6);
			r1 = _mm_add_ps(r1 , a2);
			r3 = _mm_add_ps(r3 , a2);
			r5 = _mm_add_ps(r5 , a2);
			r7 = _mm_add_ps(r7 , a2);
			r1 = _mm_mul_ps(r1 , r0);
			r3 = _mm_mul_ps(r3 , r2);
			r5 = _mm_mul_ps(r5 , r4);
			r7 = _mm_mul_ps(r7 , r6);
			r1 = _mm_add_ps(r1 , a1);
			r3 = _mm_add_ps(r3 , a1);
			r5 = _mm_add_ps(r5 , a1);
			r7 = _mm_add_ps(r7 , a1);
			r1 = _mm_mul_ps(r1 , r0);
			r3 = _mm_mul_ps(r3 , r2);
			r5 = _mm_mul_ps(r5 , r4);
			r7 = _mm_mul_ps(r7 , r6);
			r1 = _mm_add_ps(r1 , a0);
			r3 = _mm_add_ps(r3 , a0);
			r5 = _mm_add_ps(r5 , a0);
			r7 = _mm_add_ps(r7 , a0);
			r1 = _mm_mul_ps(r1 , _mm_castsi128_ps(e0));
			r3 = _mm_mul_ps(r3 , _mm_castsi128_ps(e2));
			r5 = _mm_mul_ps(r5 , _mm_castsi128_ps(e4));
			r7 = _mm_mul_ps(r7 , _mm_castsi128_ps(e6));
			_mm_storeu_ps(d+i , r1);
			_mm_storeu_ps(d+i+4 , r3);
			_mm_storeu_ps(d+i+8 , r5);
			_mm_storeu_ps(d+i+12 , r7);
			i+=16;
		}
	}
	while (i <= (size-4)) {
		r0 = _mm_loadu_ps(d+i);
		r0 = _mm_max_ps(r0 , xmin);
		r0 = _mm_min_ps(r0 , xmax);
		r0 = _mm_mul_ps(r0 , rln2);
		#ifdef ICSTLIB_DEF_ROUND
			e0 = _mm_cvtps_epi32(r0);	
		#else
			r1 = _mm_and_ps(r0 , signmask);
			r1 = _mm_xor_ps(r1 , onediv2);
			r1 = _mm_add_ps(r1 , r0);
			e0 = _mm_cvttps_epi32(r1);
		#endif
		r1 = _mm_cvtepi32_ps(e0);
		r0 = _mm_sub_ps(r0 , r1);
		e0 = _mm_add_epi32(e0 , c127);
		e0 = _mm_slli_epi32(e0 , 23);
		r1 = _mm_mul_ps(r0 , a5);
		r1 = _mm_add_ps(r1 , a4);
		r1 = _mm_mul_ps(r1 , r0);
		r1 = _mm_add_ps(r1 , a3);
		r1 = _mm_mul_ps(r1 , r0);
		r1 = _mm_add_ps(r1 , a2);
		r1 = _mm_mul_ps(r1 , r0);
		r1 = _mm_add_ps(r1 , a1);
		r1 = _mm_mul_ps(r1 , r0);
		r1 = _mm_add_ps(r1 , a0);
		r1 = _mm_mul_ps(r1 , _mm_castsi128_ps(e0));
		_mm_storeu_ps(d+i , r1);
		i+=4;
	}
	while (i < size) {
		r0 = _mm_load_ss(d+i);
		r0 = _mm_max_ss(r0 , xmin);
		r0 = _mm_min_ss(r0 , xmax);
		r0 = _mm_mul_ss(r0 , rln2);
		#ifdef ICSTLIB_DEF_ROUND
			e0 = _mm_cvtps_epi32(r0);	
		#else
			r1 = _mm_and_ps(r0 , signmask);
			r1 = _mm_xor_ps(r1 , onediv2);
			r1 = _mm_add_ss(r1 , r0);
			e0 = _mm_cvttps_epi32(r1);
		#endif
		r1 = _mm_cvtepi32_ps(e0);
		r0 = _mm_sub_ss(r0 , r1);
		e0 = _mm_add_epi32(e0 , c127);
		e0 = _mm_slli_epi32(e0 , 23);
		r1 = _mm_mul_ss(r0 , a5);
		r1 = _mm_add_ss(r1 , a4);
		r1 = _mm_mul_ss(r1 , r0);
		r1 = _mm_add_ss(r1 , a3);
		r1 = _mm_mul_ss(r1 , r0);
		r1 = _mm_add_ss(r1 , a2);
		r1 = _mm_mul_ss(r1 , r0);
		r1 = _mm_add_ss(r1 , a1);
		r1 = _mm_mul_ss(r1 , r0);
		r1 = _mm_add_ss(r1 , a0);
		r1 = _mm_mul_ss(r1 , _mm_castsi128_ps(e0));
		_mm_store_ss(d+i , r1);
		i++;
	}
#endif
}

// reverse element order
void VectorFunctions::reverse(float* d, int size) 
{
	int j = size-1; float x; 
	for (int i=0; i<(size>>1); i++,j--) {x=d[i]; d[i]=d[j]; d[j]=x;}
}			

// normalize d to norm=1, return scale factor
float VectorFunctions::normalize(float* d, int size)
{
	float n = norm(d,size);
	if (n >= FLT_MIN) {n = 1.0f/n; mul(d,n,size); return n;}
	return 1.0f;
}			

// return sum of d
float VectorFunctions::sum(const float* d, int size)
{
#ifdef ICSTLIB_NO_SSEOPT  
	int i, rm = size - ((size>>4)<<4); float x=0; double y;
	for (i=0; i<rm; i++) {x += d[i];}
	y = static_cast<double>(x);
	for (i=rm; i<size; i+=16) {
		x=0;
		x += d[i];		x += d[i+1];	x += d[i+2];	x += d[i+3];
		x += d[i+4];	x += d[i+5];	x += d[i+6];	x += d[i+7];
		x += d[i+8];	x += d[i+9];	x += d[i+10];	x += d[i+11];
		x += d[i+12];	x += d[i+13];	x += d[i+14];	x += d[i+15];
		y += static_cast<double>(x);
	}
	return static_cast<float>(y);
#else
	int i=0;
	__m128 r0,r1,r2,r3,acc0,acc1,acc2,acc3;
	__m128d tmp0,tmp1,sum0,sum1;
	float res[4];
	sum0 = _mm_setzero_pd(); sum1 = _mm_setzero_pd();
	if (!(reinterpret_cast<uintptr_t>(d) & 0xF)) {
		while (i <= (size-32)) {
			acc0 = _mm_load_ps(d+i);
			acc1 = _mm_load_ps(d+i+4);
			acc2 = _mm_load_ps(d+i+8);
			acc3 = _mm_load_ps(d+i+12);
			r0 = _mm_load_ps(d+i+16);
			acc0 = _mm_add_ps(acc0 , r0);
			r1 = _mm_load_ps(d+i+20);
			acc1 = _mm_add_ps(acc1 , r1);
			r2 = _mm_load_ps(d+i+24);
			acc2 = _mm_add_ps(acc2 , r2);
			r3 = _mm_load_ps(d+i+28);
			acc3 = _mm_add_ps(acc3 , r3);
			acc0 = _mm_add_ps(acc0 , acc1);
			acc2 = _mm_add_ps(acc2 , acc3);
			acc0 = _mm_add_ps(acc0 , acc2);
			acc1 = _mm_movehl_ps(acc1 , acc0);
			tmp0 = _mm_cvtps_pd(acc0);
			tmp1 = _mm_cvtps_pd(acc1);
			sum0 = _mm_add_pd(sum0 , tmp0);
			sum1 = _mm_add_pd(sum1 , tmp1);
			i += 32;
		}
	}
	else {
		while (i <= (size-32)) {
			acc0 = _mm_loadu_ps(d+i);
			acc1 = _mm_loadu_ps(d+i+4);
			acc2 = _mm_loadu_ps(d+i+8);
			acc3 = _mm_loadu_ps(d+i+12);
			r0 = _mm_loadu_ps(d+i+16);
			acc0 = _mm_add_ps(acc0 , r0);
			r1 = _mm_loadu_ps(d+i+20);
			acc1 = _mm_add_ps(acc1 , r1);
			r2 = _mm_loadu_ps(d+i+24);
			acc2 = _mm_add_ps(acc2 , r2);
			r3 = _mm_loadu_ps(d+i+28);
			acc3 = _mm_add_ps(acc3 , r3);
			acc0 = _mm_add_ps(acc0 , acc1);
			acc2 = _mm_add_ps(acc2 , acc3);
			acc0 = _mm_add_ps(acc0 , acc2);
			acc1 = _mm_movehl_ps(acc1 , acc0);
			tmp0 = _mm_cvtps_pd(acc0);
			tmp1 = _mm_cvtps_pd(acc1);
			sum0 = _mm_add_pd(sum0 , tmp0);
			sum1 = _mm_add_pd(sum1 , tmp1);
			i += 32;
		}
	}
	acc0 = _mm_cvtpd_ps(sum0);
	acc1 = _mm_cvtpd_ps(sum1);
	if (size & 16) {
		r0 = _mm_loadu_ps(d+i);
		acc0 = _mm_add_ps(acc0 , r0);
		r1 = _mm_loadu_ps(d+i+4);
		acc1 = _mm_add_ps(acc1 , r1);
		r0 = _mm_loadu_ps(d+i+8);
		acc0 = _mm_add_ps(acc0 , r0);
		r1 = _mm_loadu_ps(d+i+12);
		acc1 = _mm_add_ps(acc1 , r1);
		i += 16;
	}
	if (size & 8) {
		r0 = _mm_loadu_ps(d+i);
		acc0 = _mm_add_ps(acc0 , r0);
		r1 = _mm_loadu_ps(d+i+4);
		acc1 = _mm_add_ps(acc1 , r1);
		i += 8;
	}
	if (size & 4) {
		r0 = _mm_loadu_ps(d+i);
		acc0 = _mm_add_ps(acc0 , r0);
		i += 4;
	}
	acc0 = _mm_add_ps(acc0 , acc1);
	_mm_storeu_ps(res , acc0);
	if (size & 2) {res[0] += d[i]; res[1] += d[i+1]; i+=2;}
	if (size & 1) {res[0] += d[i];}
	return res[0] + res[1] + res[2] + res[3];
#endif	
}			

// return signal energy of d: <d,d>
float VectorFunctions::energy(float* d, int size)
{
#ifdef ICSTLIB_NO_SSEOPT  
	int i, rm = size - ((size>>3)<<3);
	float x, y=0; double z;
	for (i=0; i<rm; i++) {x=d[i]; y += (x*x);}
	z = static_cast<double>(y);
	for (i=rm; i<size; i+=8) {
		y=0;	
		x=d[i]; y += (x*x);		x=d[i+1]; y += (x*x);
		x=d[i+2]; y += (x*x);	x=d[i+3]; y += (x*x);
		x=d[i+4]; y += (x*x);	x=d[i+5]; y += (x*x);
		x=d[i+6]; y += (x*x);	x=d[i+7]; y += (x*x);
		z += static_cast<double>(y);
	}
	return static_cast<float>(z);
#else
	__m128 r0,r1,r2,r3,acc0,acc1,acc2,acc3;
	__m128d tmp0,tmp1,sum0,sum1;
	float res[4]; int i=0;
	sum0 = _mm_setzero_pd(); sum1 = _mm_setzero_pd();
	if (!(reinterpret_cast<uintptr_t>(d) & 0xF)) {
		while (i <= (size-32)) {
			r0 = _mm_load_ps(d+i);
			r1 = _mm_load_ps(d+i+4);
			acc0 = _mm_mul_ps(r0 , r0);
			acc1 = _mm_mul_ps(r1 , r1);
			r2 = _mm_load_ps(d+i+8);
			r3 = _mm_load_ps(d+i+12);
			acc2 = _mm_mul_ps(r2 , r2);
			acc3 = _mm_mul_ps(r3 , r3);
			r0 = _mm_load_ps(d+i+16);
			r1 = _mm_load_ps(d+i+20);
			r0 = _mm_mul_ps(r0 , r0);
			r1 = _mm_mul_ps(r1 , r1);
			r2 = _mm_load_ps(d+i+24);
			r3 = _mm_load_ps(d+i+28);
			r2 = _mm_mul_ps(r2 , r2);
			r3 = _mm_mul_ps(r3 , r3);
			acc0 = _mm_add_ps(acc0 , r0);
			acc1 = _mm_add_ps(acc1 , r1);
			acc2 = _mm_add_ps(acc2 , r2);
			acc3 = _mm_add_ps(acc3 , r3);
			acc0 = _mm_add_ps(acc0 , acc1);
			acc2 = _mm_add_ps(acc2 , acc3);
			acc0 = _mm_add_ps(acc0 , acc2);
			acc1 = _mm_movehl_ps(acc1 ,acc0);
			tmp0 = _mm_cvtps_pd(acc0);
			tmp1 = _mm_cvtps_pd(acc1);
			sum0 = _mm_add_pd(sum0 , tmp0);
			sum1 = _mm_add_pd(sum1 , tmp1);
			i += 32;
		}
	}
	else {
		while (i <= (size-32)) {
			r0 = _mm_loadu_ps(d+i);
			r1 = _mm_loadu_ps(d+i+4);
			acc0 = _mm_mul_ps(r0 , r0);
			acc1 = _mm_mul_ps(r1 , r1);
			r2 = _mm_loadu_ps(d+i+8);
			r3 = _mm_loadu_ps(d+i+12);
			acc2 = _mm_mul_ps(r2 , r2);
			acc3 = _mm_mul_ps(r3 , r3);
			r0 = _mm_loadu_ps(d+i+16);
			r1 = _mm_loadu_ps(d+i+20);
			r0 = _mm_mul_ps(r0 , r0);
			r1 = _mm_mul_ps(r1 , r1);
			r2 = _mm_loadu_ps(d+i+24);
			r3 = _mm_loadu_ps(d+i+28);
			r2 = _mm_mul_ps(r2 , r2);
			r3 = _mm_mul_ps(r3 , r3);
			acc0 = _mm_add_ps(acc0 , r0);
			acc1 = _mm_add_ps(acc1 , r1);
			acc2 = _mm_add_ps(acc2 , r2);
			acc3 = _mm_add_ps(acc3 , r3);
			acc0 = _mm_add_ps(acc0 , acc1);
			acc2 = _mm_add_ps(acc2 , acc3);
			acc0 = _mm_add_ps(acc0 , acc2);
			acc1 = _mm_movehl_ps(acc1 ,acc0);
			tmp0 = _mm_cvtps_pd(acc0);
			tmp1 = _mm_cvtps_pd(acc1);
			sum0 = _mm_add_pd(sum0 , tmp0);
			sum1 = _mm_add_pd(sum1 , tmp1);
			i += 32;
		}
	}
	acc0 = _mm_cvtpd_ps(sum0);
	acc1 = _mm_cvtpd_ps(sum1);
	if (size & 16) {
		r0 = _mm_loadu_ps(d+i);
		r1 = _mm_loadu_ps(d+i+4);
		r0 = _mm_mul_ps(r0 , r0);
		r1 = _mm_mul_ps(r1 , r1);
		r2 = _mm_loadu_ps(d+i+8);
		r3 = _mm_loadu_ps(d+i+12);
		acc0 = _mm_add_ps(acc0 , r0);
		acc1 = _mm_add_ps(acc1 , r1);
		r2 = _mm_mul_ps(r2 , r2);
		r3 = _mm_mul_ps(r3 , r3);
		acc0 = _mm_add_ps(acc0 , r2);
		acc1 = _mm_add_ps(acc1 , r3);
		i += 16;
	}
	if (size & 8) {
		r0 = _mm_loadu_ps(d+i);
		r1 = _mm_loadu_ps(d+i+4);
		r0 = _mm_mul_ps(r0 , r0);
		r1 = _mm_mul_ps(r1 , r1);
		acc0 = _mm_add_ps(acc0 , r0);
		acc1 = _mm_add_ps(acc1 , r1);
		i += 8;
	}
	if (size & 4) {
		r0 = _mm_loadu_ps(d+i);
		r0 = _mm_mul_ps(r0 , r0);
		acc0 = _mm_add_ps(acc0 , r0);
		i += 4;
	}
	acc0 = _mm_add_ps(acc0 , acc1);
	_mm_storeu_ps(res , acc0);
	if (size & 2) {res[0] += (d[i]*d[i]); res[1] += (d[i+1]*d[i+1]); i+=2;}
	if (size & 1) {res[0] += (d[i]*d[i]);}
	return res[0] + res[1] + res[2] + res[3];
#endif
}

// return L2 vector norm of d: sqrt(<d,d>)
float VectorFunctions::norm(float* d, int size) {
	return sqrtf(energy(d,size));}	

// return signal power of d: <d,d>/size
float VectorFunctions::power(float* d, int size) {
	return energy(d,size)/static_cast<float>(size);}

// return RMS value of d: sqrt(<d,d>/size)
float VectorFunctions::rms(float* d, int size) {
	return sqrtf(energy(d,size)/static_cast<float>(size));}

// return index of maximum d
int VectorFunctions::maxi(float* d, int size)
{
	float max=d[0]; int i,idx=0, rm = size - ((size>>2)<<2);
	for (i=1; i<rm; i++) {if (d[i]>max) {max=d[i]; idx=i;}}
	for (i=rm; i<size; i+=4) {
		if ((d[i] > max) || (d[i+1] > max) || (d[i+2] > max) || (d[i+3] > max)) {
			if (d[i] > max) {max=d[i]; idx=i;}
			if (d[i+1] > max) {max=d[i+1]; idx=i+1;}
			if (d[i+2] > max) {max=d[i+2]; idx=i+2;}
			if (d[i+3] > max) {max=d[i+3]; idx=i+3;}
		}
	}
	return idx;
}

// return index of minimum d	
int VectorFunctions::mini(float* d, int size)
{
	float min=d[0]; int i,idx=0, rm = size - ((size>>2)<<2);
	for (i=1; i<rm; i++) {if (d[i]<min) {min=d[i]; idx=i;}}
	for (i=rm; i<size; i+=4) {
		if ((d[i] < min) || (d[i+1] < min) || (d[i+2] < min) || (d[i+3] < min)) {
			if (d[i] < min) {min=d[i]; idx=i;}
			if (d[i+1] < min) {min=d[i+1]; idx=i+1;}
			if (d[i+2] < min) {min=d[i+2]; idx=i+2;}
			if (d[i+3] < min) {min=d[i+3]; idx=i+3;}
		}	
	}
	return idx;
}

// return index i of element with maximum |d[i] - r|
int VectorFunctions::farthesti(float* d, float r, int size)
{
	int i,idx=0;
	float x, diff = fabsf(d[0] - r);
	for (i=1; i<size; i++) {
		x = fabsf(d[i] - r);
		if (x > diff) {diff=x; idx=i;} 
	}
	return idx;
}

// return index i of element with minimum |d[i] - r|
int VectorFunctions::nearesti(float* d, float r, int size)
{
	int i,idx=0;
	float x, diff = fabsf(d[0] - r);
	for (i=1; i<size; i++) {
		x = fabsf(d[i] - r);
		if (x < diff) {diff=x; idx=i;} 
	}
	return idx;
}

// fill d with cumulative sum of d
void VectorFunctions::cumsum(float* d, int size)
{
	double x=0;
	for (int i=0; i<size; i++) {
		x += static_cast<double>(d[i]); d[i] = static_cast<float>(x);
	}
}

// differentiate d, normalize for d=linear(0..1) -> d'=1
// uses 3 points for d[0..1] and d[size-2..size-1], 5 points otherwise
// consider discarding 3-point values for highest precision  
void VectorFunctions::diff(float* d, int size)
{
	float temp,dm1, dm2=0;
	float c1 = 0.5f*static_cast<float>(size-1);
	float c2 = c1/6.0f;
	float c3 = 8.0f*c2;
	if (size == 1) {d[0]=0; return;}
	if (size == 2) {d[0] = d[1] = d[1]-d[0]; return;}
	dm1 = d[0]; d[0] = c1*(4.0f*d[1] - d[2] - 3.0f*d[0]);
	if (size > 2) {temp=d[1]; d[1] = c1*(d[2]-dm1); dm2=dm1; dm1=temp;}
	for (int i=2; i<(size-2); i++) {
		temp=d[i]; 	
		d[i] = c3*(d[i+1]-dm1) - c2*(d[i+2]-dm2); 
		dm2=dm1; dm1=temp;
	}
	if (size > 3) {
		temp=d[size-2]; d[size-2] = c1*(d[size-1]-dm1); 
		dm2=dm1; dm1=temp;
	}
	d[size-1] = c1*(3.0f*d[size-1] - 4.0f*dm1 + dm2);
}			

// integrate d, normalize for d=1 -> int(d)=(0..1)
// lf=f:	trapezoidal rule, y[n+1] = y[n] + 0.5*(x[n+1]+x[n])
// lf=t:	4th order rule, 
//			y[n+1] = y[n] + 13/24*(x[n+1]+x[n]) - 1/24*(x[n+2]+x[n-1])
void VectorFunctions::integrate(float* d, int size, bool lf, 
						   float dprev, float dnext)
{
	int i; float x,xm1,xm2,xp1,c1,c2; double y=0;
	if (size == 1) {d[0]=0; return;}
	if (!lf) {											// trapezoidal rule
		c1 = 0.5f/static_cast<float>(size-1);
		xm1=d[0]; d[0]=0;
		for (i=1; i<size; i++) {
			x=d[i]; y += static_cast<double>(c1*(x+xm1)); xm1=x; 
			d[i] = static_cast<float>(y); 
		}	
	}
	else {												// 4th order rule
		c1 = 1.0f/(24.0f*static_cast<float>(size-1)); 
		c2 = 13.0f*c1;	
		xm2=dprev; xm1=d[0]; xp1=d[1]; d[0]=0;
		for (i=1; i<(size-1); i++) {
			x=xp1; xp1=d[i+1];
			y += static_cast<double>(c2*(x+xm1) - c1*(xp1+xm2)); 
			xm2=xm1; xm1=x;
			d[i] = static_cast<float>(y); 
		}	
		y += static_cast<double>(c2*(xp1+xm1) - c1*(dnext+xm2));
		d[size-1] = static_cast<float>(y); 	
	}
}
																							
// c -> d
void VectorFunctions::set(float* d, float c, int size)
{
#ifdef ICSTLIB_NO_SSEOPT  
	for (int i=0; i<size; i++) {d[i] = c;}
#else
	int i=0;
	__m128 ar = _mm_set_ps1(c);
	if (!(reinterpret_cast<uintptr_t>(d) & 0xF)) {
		while (i <= (size-16)) {
			_mm_store_ps(d+i , ar);
			_mm_store_ps(d+i+4 , ar);
			_mm_store_ps(d+i+8 , ar);
			_mm_store_ps(d+i+12 , ar);
			i+=16;
		}
	}
	else {
		while (i <= (size-16)) {
			_mm_storeu_ps(d+i , ar);
			_mm_storeu_ps(d+i+4 , ar);
			_mm_storeu_ps(d+i+8 , ar);
			_mm_storeu_ps(d+i+12 , ar);
			i+=16;
		}
	}
	if (size & 8) {
		_mm_storeu_ps(d+i , ar);
		_mm_storeu_ps(d+i+4 , ar);
		i+=8;
	}
	if (size & 4) {
		_mm_storeu_ps(d+i , ar);
		i+=4;
	}
	if (size & 2) {d[i] = c; d[i+1] = c; i+=2;}
	if (size & 1) {d[i] = c;}
#endif
}

// if |d[n]|<=lim: sign(d[n])*rep -> d[n]
void VectorFunctions::prune(float* d, float lim, float rep, int size) 
{
#ifdef ICSTLIB_NO_SSEOPT  
	for (int i=0; i<size; i++) {
		if (fabsf(d[i]) <= lim) {d[i] = (d[i] >= 0) ? rep : -rep;}
	}
#else
	int i=0;
	__m128 r0,r1,r2;
	__m128 absmask = _mm_castsi128_ps(_mm_set1_epi32(0x7FFFFFFF));
	__m128 sgnmask = _mm_castsi128_ps(_mm_set1_epi32(0x80000000));
	__m128 limit = _mm_set1_ps(lim);
	__m128 rpl = _mm_set1_ps(rep);
	if (!(reinterpret_cast<uintptr_t>(d) & 0xF)) {
		while (i <= (size-4)) {
			r0 = _mm_load_ps(d+i);
			r1 = _mm_and_ps(r0 , absmask);
			r2 = _mm_and_ps(r0 , sgnmask);
			r2 = _mm_xor_ps(r2 , rpl);
			r2 = _mm_sub_ps(r2 , r0);
			r1 = _mm_cmple_ps(r1 , limit);
			r1 = _mm_and_ps(r1 , r2);
			r0 = _mm_add_ps(r0 , r1);
			_mm_store_ps(d+i , r0);
			i += 4;
		}
	}
	else {
		while (i <= (size-4)) {
			r0 = _mm_loadu_ps(d+i);
			r1 = _mm_and_ps(r0 , absmask);
			r2 = _mm_and_ps(r0 , sgnmask);
			r2 = _mm_xor_ps(r2 , rpl);
			r2 = _mm_sub_ps(r2 , r0);
			r1 = _mm_cmple_ps(r1 , limit);
			r1 = _mm_and_ps(r1 , r2);
			r0 = _mm_add_ps(r0 , r1);
			_mm_storeu_ps(d+i , r0);
			i += 4;
		}
	}
	while (i < size) {
		if (fabsf(d[i]) <= lim) {d[i] = (d[i] >= 0) ? rep : -rep;}
		i++;
	}
#endif	
}

// limit d
void VectorFunctions::limit(float* d, int size, float hi, float lo)
{
#ifdef ICSTLIB_NO_SSEOPT  
	for (int i=0; i<size; i++) {d[i] = __max(__min(d[i],hi),lo);}
#else
	int i=0;
	__m128 r0,r1,xhi,xlo;
	xhi = _mm_set1_ps(hi);
	xlo = _mm_set1_ps(lo);
	if (!(reinterpret_cast<uintptr_t>(d) & 0xF)) {
		while (i <= (size-8)) {
			r0 = _mm_load_ps(d+i);
			r0 = _mm_min_ps(r0 , xhi);
			r1 = _mm_load_ps(d+i+4);
			r0 = _mm_max_ps(r0 , xlo);
			r1 = _mm_min_ps(r1 , xhi);
			_mm_store_ps(d+i , r0);
			r1 = _mm_max_ps(r1 , xlo);
			_mm_store_ps(d+i+4 , r1);
			i += 8;
		}
	}
	else {
		while (i <= (size-8)) {
			r0 = _mm_loadu_ps(d+i);
			r0 = _mm_min_ps(r0 , xhi);
			r1 = _mm_loadu_ps(d+i+4);
			r0 = _mm_max_ps(r0 , xlo);
			r1 = _mm_min_ps(r1 , xhi);
			_mm_storeu_ps(d+i , r0);
			r1 = _mm_max_ps(r1 , xlo);
			_mm_storeu_ps(d+i+4 , r1);
			i += 8;
		}
	}
	if (size & 4) {
		r0 = _mm_loadu_ps(d+i);
		r0 = _mm_min_ps(r0 , xhi);
		r0 = _mm_max_ps(r0 , xlo);
		_mm_storeu_ps(d+i , r0);
		i += 4;
	}
	if (size & 2) {
		r0 = _mm_load_ss(d+i);
		r0 = _mm_min_ss(r0 , xhi);
		r1 = _mm_load_ss(d+i+1);
		r0 = _mm_max_ss(r0 , xlo);
		r1 = _mm_min_ss(r1 , xhi);
		_mm_store_ss(d+i , r0);
		r1 = _mm_max_ss(r1 , xlo);
		_mm_store_ss(d+i+1 , r1);
		i += 2;
	}
	if (size & 1) {
		r0 = _mm_load_ss(d+i);
		r0 = _mm_min_ss(r0 , xhi);
		r0 = _mm_max_ss(r0 , xlo);
		_mm_store_ss(d+i , r0);
	}
#endif
}	

// c*d -> d
void VectorFunctions::mul(float* d, float c, int size)
{
	int i=0;
#ifndef ICSTLIB_NO_SSEOPT  
	if (reinterpret_cast<uintptr_t>(d) & 0xF) {
#endif
		for (i=0; i<size; i++) {d[i] *= c;}
#ifndef ICSTLIB_NO_SSEOPT
	}
	else {
		__m128 r0,r1,r2;
		r2 = _mm_set1_ps(c);
		while (i <= (size-8)) {
			r0 = _mm_load_ps(d+i);
			r0 = _mm_mul_ps(r0 , r2);
			_mm_store_ps(d+i , r0);
			r1 = _mm_load_ps(d+i+4);
			r1 = _mm_mul_ps(r1 , r2);
			_mm_store_ps(d+i+4 , r1);
			i += 8;
		}
		if (size & 4) {
			r0 = _mm_load_ps(d+i);
			r0 = _mm_mul_ps(r0 , r2);
			_mm_store_ps(d+i , r0);
			i += 4;
		}
		if (size & 2) {d[i] *= c; d[i+1] *= c; i+=2;}
		if (size & 1) {d[i] *= c;}
	}
#endif
}	

// fill d with element-wise product of d*r
void VectorFunctions::mul(float* d, float* r, int size)
{
	int i=0;
#ifndef ICSTLIB_NO_SSEOPT  
	if ((reinterpret_cast<uintptr_t>(d) | reinterpret_cast<uintptr_t>(r)) & 0xF) {
#endif
		for (i=0; i<size; i++) {d[i] *= r[i];}
#ifndef ICSTLIB_NO_SSEOPT
	}
	else {
		__m128 r0,r1,r2,r3;
		while (i <= (size-8)) {
			r0 = _mm_load_ps(d+i);
			r1 = _mm_load_ps(r+i);
			r0 = _mm_mul_ps(r0 , r1);
			_mm_store_ps(d+i , r0);
			r2 = _mm_load_ps(d+i+4);
			r3 = _mm_load_ps(r+i+4);
			r2 = _mm_mul_ps(r2 , r3);
			_mm_store_ps(d+i+4 , r2);
			i += 8;
		}
		if (size & 4) {
			r0 = _mm_load_ps(d+i);
			r1 = _mm_load_ps(r+i);
			r0 = _mm_mul_ps(r0 , r1);
			_mm_store_ps(d+i , r0);
			i += 4;
		}
		if (size & 2) {d[i] *= r[i]; d[i+1] *= r[i+1]; i+=2;}
		if (size & 1) {d[i] *= r[i];}
	}
#endif
}	

// return dot product: <d,r>
float VectorFunctions::dotp(float* d, float* r, int size)
{
#ifdef ICSTLIB_NO_SSEOPT  
	int i, rm = size - ((size>>4)<<4); float x=0; double y; 
	for (i=0; i<rm; i++) {x += (d[i]*r[i]);}
	y = static_cast<double>(x);
	for (i=rm; i<size; i+=16) {
		x=0;
		x += (d[i]*r[i]);		x += (d[i+1]*r[i+1]);
		x += (d[i+2]*r[i+2]);	x += (d[i+3]*r[i+3]);
		x += (d[i+4]*r[i+4]);	x += (d[i+5]*r[i+5]);
		x += (d[i+6]*r[i+6]);	x += (d[i+7]*r[i+7]);
		x += (d[i+8]*r[i+8]);	x += (d[i+9]*r[i+9]);
		x += (d[i+10]*r[i+10]); x += (d[i+11]*r[i+11]);
		x += (d[i+12]*r[i+12]); x += (d[i+13]*r[i+13]);
		x += (d[i+14]*r[i+14]); x += (d[i+15]*r[i+15]);
		y += static_cast<double>(x);
	}
	return static_cast<float>(y);
#else
	int i=0;
	__m128 r0,r1,r2,r3,r4,r5,acc0,acc1;
	__m128d tmp0,tmp1,sum0,sum1;
	float res[4];
	sum0 = _mm_setzero_pd(); sum1 = _mm_setzero_pd();
	if (!((reinterpret_cast<uintptr_t>(d) | reinterpret_cast<uintptr_t>(r)) & 0xF)) {
		while (i <= (size-32)) {
			r0 = _mm_load_ps(d+i);
			r1 = _mm_load_ps(d+i+4);
			r2 = _mm_load_ps(r+i);
			acc0 = _mm_mul_ps(r0 , r2);
			r3 = _mm_load_ps(r+i+4);
			acc1 = _mm_mul_ps(r1 , r3);
			r0 = _mm_load_ps(d+i+8);
			r1 = _mm_load_ps(d+i+12);
			r2 = _mm_load_ps(r+i+8);
			r0 = _mm_mul_ps(r0 , r2);
			r3 = _mm_load_ps(r+i+12);
			r1 = _mm_mul_ps(r1 , r3);
			r4 = _mm_load_ps(d+i+16);
			acc0 = _mm_add_ps(acc0 , r0);
			r5 = _mm_load_ps(d+i+20);
			acc1 = _mm_add_ps(acc1 , r1);
			r2 = _mm_load_ps(r+i+16);
			r4 = _mm_mul_ps(r4 , r2);
			r3 = _mm_load_ps(r+i+20);
			r5 = _mm_mul_ps(r5 , r3);
			r0 = _mm_load_ps(d+i+24);
			acc0 = _mm_add_ps(acc0 , r4);
			r1 = _mm_load_ps(d+i+28);
			acc1 = _mm_add_ps(acc1 , r5);
			r2 = _mm_load_ps(r+i+24);
			r0 = _mm_mul_ps(r0 , r2);
			r3 = _mm_load_ps(r+i+28);
			r1 = _mm_mul_ps(r1 , r3);		
			acc0 = _mm_add_ps(acc0 , r0);
			acc1 = _mm_add_ps(acc1 , r1);
			acc0 = _mm_add_ps(acc0 , acc1);
			acc1 = _mm_movehl_ps(acc1 , acc0);
			tmp0 = _mm_cvtps_pd(acc0);
			tmp1 = _mm_cvtps_pd(acc1);
			sum0 = _mm_add_pd(sum0 , tmp0);
			sum1 = _mm_add_pd(sum1 , tmp1);
			i+=32;
		}
		acc0 = _mm_cvtpd_ps(sum0);
		acc1 = _mm_cvtpd_ps(sum1);
		if (size & 16) {
			r0 = _mm_load_ps(d+i);
			r1 = _mm_load_ps(d+i+4);
			r2 = _mm_load_ps(r+i);
			r0 = _mm_mul_ps(r0 , r2);
			r3 = _mm_load_ps(r+i+4);
			r1 = _mm_mul_ps(r1 , r3);
			acc0 = _mm_add_ps(acc0 , r0);
			acc1 = _mm_add_ps(acc1 , r1);
			r0 = _mm_load_ps(d+i+8);
			r1 = _mm_load_ps(d+i+12);
			r2 = _mm_load_ps(r+i+8);
			r0 = _mm_mul_ps(r0 , r2);
			r3 = _mm_load_ps(r+i+12);
			r1 = _mm_mul_ps(r1 , r3);
			acc0 = _mm_add_ps(acc0 , r0);
			acc1 = _mm_add_ps(acc1 , r1);
			i+=16;
		}
		if (size & 8) {
			r0 = _mm_load_ps(d+i);
			r1 = _mm_load_ps(d+i+4);
			r2 = _mm_load_ps(r+i);
			r0 = _mm_mul_ps(r0 , r2);
			r3 = _mm_load_ps(r+i+4);
			r1 = _mm_mul_ps(r1 , r3);
			acc0 = _mm_add_ps(acc0 , r0);
			acc1 = _mm_add_ps(acc1 , r1);
			i+=8;
		}
		if (size & 4) {
			r0 = _mm_load_ps(d+i);
			r2 = _mm_load_ps(r+i);
			r0 = _mm_mul_ps(r0 , r2);
			acc0 = _mm_add_ps(acc0 , r0);
			i+=4;
		}
	}
	else {
		while (i <= (size-32)) {
			r0 = _mm_loadu_ps(d+i);
			r1 = _mm_loadu_ps(d+i+4);
			r2 = _mm_loadu_ps(r+i);
			acc0 = _mm_mul_ps(r0 , r2);
			r3 = _mm_loadu_ps(r+i+4);
			acc1 = _mm_mul_ps(r1 , r3);
			r0 = _mm_loadu_ps(d+i+8);
			r1 = _mm_loadu_ps(d+i+12);
			r2 = _mm_loadu_ps(r+i+8);
			r0 = _mm_mul_ps(r0 , r2);
			r3 = _mm_loadu_ps(r+i+12);
			r1 = _mm_mul_ps(r1 , r3);
			r4 = _mm_loadu_ps(d+i+16);
			acc0 = _mm_add_ps(acc0 , r0);
			r5 = _mm_loadu_ps(d+i+20);
			acc1 = _mm_add_ps(acc1 , r1);
			r2 = _mm_loadu_ps(r+i+16);
			r4 = _mm_mul_ps(r4 , r2);
			r3 = _mm_loadu_ps(r+i+20);
			r5 = _mm_mul_ps(r5 , r3);
			r0 = _mm_loadu_ps(d+i+24);
			acc0 = _mm_add_ps(acc0 , r4);
			r1 = _mm_loadu_ps(d+i+28);
			acc1 = _mm_add_ps(acc1 , r5);
			r2 = _mm_loadu_ps(r+i+24);
			r0 = _mm_mul_ps(r0 , r2);
			r3 = _mm_loadu_ps(r+i+28);
			r1 = _mm_mul_ps(r1 , r3);		
			acc0 = _mm_add_ps(acc0 , r0);
			acc1 = _mm_add_ps(acc1 , r1);
			acc0 = _mm_add_ps(acc0 , acc1);
			acc1 = _mm_movehl_ps(acc1 , acc0);
			tmp0 = _mm_cvtps_pd(acc0);
			tmp1 = _mm_cvtps_pd(acc1);
			sum0 = _mm_add_pd(sum0 , tmp0);
			sum1 = _mm_add_pd(sum1 , tmp1);
			i+=32;
		}
		acc0 = _mm_cvtpd_ps(sum0);
		acc1 = _mm_cvtpd_ps(sum1);
		if (size & 16) {
			r0 = _mm_loadu_ps(d+i);
			r1 = _mm_loadu_ps(d+i+4);
			r2 = _mm_loadu_ps(r+i);
			r0 = _mm_mul_ps(r0 , r2);
			r3 = _mm_loadu_ps(r+i+4);
			r1 = _mm_mul_ps(r1 , r3);
			acc0 = _mm_add_ps(acc0 , r0);
			acc1 = _mm_add_ps(acc1 , r1);
			r0 = _mm_loadu_ps(d+i+8);
			r1 = _mm_loadu_ps(d+i+12);
			r2 = _mm_loadu_ps(r+i+8);
			r0 = _mm_mul_ps(r0 , r2);
			r3 = _mm_loadu_ps(r+i+12);
			r1 = _mm_mul_ps(r1 , r3);
			acc0 = _mm_add_ps(acc0 , r0);
			acc1 = _mm_add_ps(acc1 , r1);
			i+=16;
		}
		if (size & 8) {
			r0 = _mm_loadu_ps(d+i);
			r1 = _mm_loadu_ps(d+i+4);
			r2 = _mm_loadu_ps(r+i);
			r0 = _mm_mul_ps(r0 , r2);
			r3 = _mm_loadu_ps(r+i+4);
			r1 = _mm_mul_ps(r1 , r3);
			acc0 = _mm_add_ps(acc0 , r0);
			acc1 = _mm_add_ps(acc1 , r1);
			i+=8;
		}
		if (size & 4) {
			r0 = _mm_loadu_ps(d+i);
			r2 = _mm_loadu_ps(r+i);
			r0 = _mm_mul_ps(r0 , r2);
			acc0 = _mm_add_ps(acc0 , r0);
			i+=4;
		}
	}
	acc0 = _mm_add_ps(acc0 , acc1);
	_mm_storeu_ps(res , acc0);
	if (size & 2) {res[0] += (d[i]*r[i]); res[1] += (d[i+1]*r[i+1]); i+=2;}
	if (size & 1) {res[0] += (d[i]*r[i]);}
	return res[0] + res[1] + res[2] + res[3];
#endif
}

// return squared distance	
float VectorFunctions::sdist(float* d, float* r, int size)
{
#ifndef ICSTLIB_NO_SSEOPT  
	if ((reinterpret_cast<uintptr_t>(d) | reinterpret_cast<uintptr_t>(r)) & 0xF) {
#endif
		float acc=0, tmp;
		for (int i=0; i<size; i++) {tmp = d[i] - r[i]; acc += (tmp*tmp);}
		return acc;
#ifndef ICSTLIB_NO_SSEOPT
	}
	else {		
		__m128 r0,acc0,r1,acc1,r2,acc2,r3,acc3,tmp0,tmp1,tmp2,tmp3;
		float res[4], tmp; int i=0;
		acc0 = _mm_setzero_ps();
		acc1 = _mm_setzero_ps();
		acc2 = _mm_setzero_ps(); 
		acc3 = _mm_setzero_ps(); 
		while (i <= (size-16)) {
			tmp0 = _mm_load_ps(r+i);
			r0 = _mm_load_ps(d+i);
			tmp1 = _mm_load_ps(r+i+4);
			r1 = _mm_load_ps(d+i+4);
			tmp2 = _mm_load_ps(r+i+8);
			r2 = _mm_load_ps(d+i+8);
			tmp3 = _mm_load_ps(r+i+12);
			r3 = _mm_load_ps(d+i+12);
			r0 = _mm_sub_ps(r0 , tmp0);
			r1 = _mm_sub_ps(r1 , tmp1);
			r2 = _mm_sub_ps(r2 , tmp2);
			r3 = _mm_sub_ps(r3 , tmp3);
			r0 = _mm_mul_ps(r0 , r0);
			r1 = _mm_mul_ps(r1 , r1);
			r2 = _mm_mul_ps(r2 , r2);
			r3 = _mm_mul_ps(r3 , r3);
			acc0 = _mm_add_ps(acc0 , r0);
			acc1 = _mm_add_ps(acc1 , r1);
			acc2 = _mm_add_ps(acc2 , r2);
			acc3 = _mm_add_ps(acc3 , r3);
			i+=16;
		}
		acc0 = _mm_add_ps(acc0 , acc1);
		acc2 = _mm_add_ps(acc2 , acc3);
		acc0 = _mm_add_ps(acc0 , acc2);
		_mm_storeu_ps(res , acc0);
		while (i < size) {tmp = d[i] - r[i]; res[0] += (tmp*tmp); i++;}
		return res[0] + res[1] + res[2] + res[3];
	}
#endif
}

// r -> d, regions may overlap
void VectorFunctions::copy(float* d, float* r, int size) {
	memmove(d,r,static_cast<size_t>(size)*sizeof(float));}

// r <-> d
void VectorFunctions::swap(float* d, float* r, int size)
{
	int i=0; float x;
#ifndef ICSTLIB_NO_SSEOPT  
	if ((reinterpret_cast<uintptr_t>(d) | reinterpret_cast<uintptr_t>(r)) & 0xF) {
#endif
		for (i=0; i<size; i++) {x=d[i]; d[i]=r[i]; r[i]=x;}
#ifndef ICSTLIB_NO_SSEOPT
	}
	else {
		__m128 r0,r1,r2,r3;
		while (i <= (size-8)) {
			r0 = _mm_load_ps(d+i);
			r1 = _mm_load_ps(r+i);
			r2 = _mm_load_ps(d+i+4);
			r3 = _mm_load_ps(r+i+4);
			_mm_store_ps(r+i , r0);
			_mm_store_ps(d+i , r1);
			_mm_store_ps(r+i+4 , r2);
			_mm_store_ps(d+i+4 , r3);
			i += 8;
		}
		if (size & 4) {
			r0 = _mm_load_ps(d+i);
			r1 = _mm_load_ps(r+i);
			_mm_store_ps(r+i , r0);
			_mm_store_ps(d+i , r1);
			i += 4;
		}
		if (size & 2) {
			x=d[i]; d[i]=r[i]; r[i]=x;
			x=d[i+1]; d[i+1]=r[i+1]; r[i+1]=x;
			i+=2;
		}
		if (size & 1) {x=d[i]; d[i]=r[i]; r[i]=x;}
	}
#endif	
}

// max(d,r) -> d
void VectorFunctions::max(float* d, float* r, int size)
{
	int i=0;
#ifndef ICSTLIB_NO_SSEOPT
	if ((reinterpret_cast<uintptr_t>(d) | reinterpret_cast<uintptr_t>(r)) & 0xF) {
#endif
		for (i=0; i<size; i++) {d[i] = __max(d[i],r[i]);}
#ifndef ICSTLIB_NO_SSEOPT
	}
	else {
		__m128 r0,r1,r2,r3;
		while (i <= (size-8)) {
			r0 = _mm_load_ps(d+i);
			r1 = _mm_load_ps(r+i);
			r0 = _mm_max_ps(r0 , r1);
			_mm_store_ps(d+i , r0);
			r2 = _mm_load_ps(d+i+4);
			r3 = _mm_load_ps(r+i+4);
			r2 = _mm_max_ps(r2 , r3);
			_mm_store_ps(d+i+4 , r2);
			i += 8;
		}
		if (size & 4) {
			r0 = _mm_load_ps(d+i);
			r1 = _mm_load_ps(r+i);
			r0 = _mm_max_ps(r0 , r1);
			_mm_store_ps(d+i , r0);
			i += 4;
		}
		if (size & 2) {d[i] = __max(d[i],r[i]); d[i+1] = __max(d[i+1],r[i+1]); i+=2;}
		if (size & 1) {d[i] = __max(d[i],r[i]);}
	}
#endif
}		

// min(d,r) -> d
void VectorFunctions::min(float* d, float* r, int size)
{
	int i=0;
#ifndef ICSTLIB_NO_SSEOPT
	if ((reinterpret_cast<uintptr_t>(d) | reinterpret_cast<uintptr_t>(r)) & 0xF) {
#endif
		for (i=0; i<size; i++) {d[i] = __min(d[i],r[i]);}
#ifndef ICSTLIB_NO_SSEOPT
	}
	else {
		__m128 r0,r1,r2,r3;
		while (i <= (size-8)) {
			r0 = _mm_load_ps(d+i);
			r1 = _mm_load_ps(r+i);
			r0 = _mm_min_ps(r0 , r1);
			_mm_store_ps(d+i , r0);
			r2 = _mm_load_ps(d+i+4);
			r3 = _mm_load_ps(r+i+4);
			r2 = _mm_min_ps(r2 , r3);
			_mm_store_ps(d+i+4 , r2);
			i += 8;
		}
		if (size & 4) {
			r0 = _mm_load_ps(d+i);
			r1 = _mm_load_ps(r+i);
			r0 = _mm_min_ps(r0 , r1);
			_mm_store_ps(d+i , r0);
			i += 4;
		}
		if (size & 2) {d[i] = __min(d[i],r[i]); d[i+1] = __min(d[i+1],r[i+1]); i+=2;}
		if (size & 1) {d[i] = __min(d[i],r[i]);}
	}
#endif
}

// d+c -> d
void VectorFunctions::add(float* d, float c, int size)
{
	int i=0;
#ifndef ICSTLIB_NO_SSEOPT  
	if (reinterpret_cast<uintptr_t>(d) & 0xF) {
#endif
		for (i=0; i<size; i++) {d[i] += c;}
#ifndef ICSTLIB_NO_SSEOPT
	}
	else {
		__m128 r0,r1,r2;
		r2 = _mm_set1_ps(c);
		while (i <= (size-8)) {
			r0 = _mm_load_ps(d+i);
			r0 = _mm_add_ps(r0 , r2);
			_mm_store_ps(d+i , r0);
			r1 = _mm_load_ps(d+i+4);
			r1 = _mm_add_ps(r1 , r2);
			_mm_store_ps(d+i+4 , r1);
			i += 8;
		}
		if (size & 4) {
			r0 = _mm_load_ps(d+i);
			r0 = _mm_add_ps(r0 , r2);
			_mm_store_ps(d+i , r0);
			i += 4;
		}
		if (size & 2) {d[i] += c; d[i+1] += c; i+=2;}
		if (size & 1) {d[i] += c;}
	}
#endif
}	

// d+r -> d
void VectorFunctions::add(float* d, float* r, int size)
{
	int i=0;
#ifndef ICSTLIB_NO_SSEOPT  
	if ((reinterpret_cast<uintptr_t>(d) | reinterpret_cast<uintptr_t>(r)) & 0xF) {
#endif
		for (i=0; i<size; i++) {d[i] += r[i];}
#ifndef ICSTLIB_NO_SSEOPT
	}
	else {
		__m128 r0,r1,r2,r3;
		while (i <= (size-8)) {
			r0 = _mm_load_ps(d+i);
			r1 = _mm_load_ps(r+i);
			r0 = _mm_add_ps(r0 , r1);
			_mm_store_ps(d+i , r0);
			r2 = _mm_load_ps(d+i+4);
			r3 = _mm_load_ps(r+i+4);
			r2 = _mm_add_ps(r2 , r3);
			_mm_store_ps(d+i+4 , r2);
			i += 8;
		}
		if (size & 4) {
			r0 = _mm_load_ps(d+i);
			r1 = _mm_load_ps(r+i);
			r0 = _mm_add_ps(r0 , r1);
			_mm_store_ps(d+i , r0);
			i += 4;
		}
		if (size & 2) {d[i] += r[i]; d[i+1] += r[i+1]; i+=2;}
		if (size & 1) {d[i] += r[i];}
	}
#endif
}	

// d-c -> d
void VectorFunctions::sub(float* d, float c, int size) {add(d,-c,size);}
	
// d-r -> d
void VectorFunctions::sub(float* d, float* r, int size)
{
	int i=0;
#ifndef ICSTLIB_NO_SSEOPT  
	if ((reinterpret_cast<uintptr_t>(d) | reinterpret_cast<uintptr_t>(r)) & 0xF) {
#endif
		for (i=0; i<size; i++) {d[i] -= r[i];}
#ifndef ICSTLIB_NO_SSEOPT
	}
	else {
		__m128 r0,r1,r2,r3;
		while (i <= (size-8)) {
			r0 = _mm_load_ps(d+i);
			r1 = _mm_load_ps(r+i);
			r0 = _mm_sub_ps(r0 , r1);
			_mm_store_ps(d+i , r0);
			r2 = _mm_load_ps(d+i+4);
			r3 = _mm_load_ps(r+i+4);
			r2 = _mm_sub_ps(r2 , r3);
			_mm_store_ps(d+i+4 , r2);
			i += 8;
		}
		if (size & 4) {
			r0 = _mm_load_ps(d+i);
			r1 = _mm_load_ps(r+i);
			r0 = _mm_sub_ps(r0 , r1);
			_mm_store_ps(d+i , r0);
			i += 4;
		}
		if (size & 2) {d[i] -= r[i]; d[i+1] -= r[i+1]; i+=2;}
		if (size & 1) {d[i] -= r[i];}
	}
#endif
}	

// multiply-accumulate: d + c*r -> d
void VectorFunctions::mac(float* d, float* r, float c, int size) {
	int i=0;
#ifndef ICSTLIB_NO_SSEOPT  
	if ((reinterpret_cast<uintptr_t>(d) | reinterpret_cast<uintptr_t>(r)) & 0xF) {
#endif
		for (i=0; i<size; i++) {d[i] += (c*r[i]);}
#ifndef ICSTLIB_NO_SSEOPT
	}
	else {
		__m128 r0,r1,r2,r3,r4,r5,r6,r7;
		__m128 scl = _mm_set1_ps(c);
		while (i <= (size-16)) {
			r0 = _mm_load_ps(r+i);
			r2 = _mm_load_ps(r+i+4);
			r4 = _mm_load_ps(r+i+8);
			r6 = _mm_load_ps(r+i+12);
			r0 = _mm_mul_ps(r0 , scl);
			r2 = _mm_mul_ps(r2 , scl);
			r4 = _mm_mul_ps(r4 , scl);
			r6 = _mm_mul_ps(r6 , scl);
			r1 = _mm_load_ps(d+i);
			r3 = _mm_load_ps(d+i+4);
			r5 = _mm_load_ps(d+i+8);
			r7 = _mm_load_ps(d+i+12);
			r0 = _mm_add_ps(r0 , r1);
			r2 = _mm_add_ps(r2 , r3);
			r4 = _mm_add_ps(r4 , r5);
			r6 = _mm_add_ps(r6 , r7);
			_mm_store_ps(d+i , r0);
			_mm_store_ps(d+i+4 , r2);
			_mm_store_ps(d+i+8 , r4);
			_mm_store_ps(d+i+12 , r6);
			i+=16;
		}
		if (size & 8) {
			r0 = _mm_load_ps(r+i);
			r2 = _mm_load_ps(r+i+4);
			r0 = _mm_mul_ps(r0 , scl);
			r2 = _mm_mul_ps(r2 , scl);
			r1 = _mm_load_ps(d+i);
			r3 = _mm_load_ps(d+i+4);
			r0 = _mm_add_ps(r0 , r1);
			r2 = _mm_add_ps(r2 , r3);
			_mm_store_ps(d+i , r0);
			_mm_store_ps(d+i+4 , r2);
			i+=8;
		}
		if (size & 4) {
			r0 = _mm_load_ps(r+i);
			r0 = _mm_mul_ps(r0 , scl);
			r1 = _mm_load_ps(d+i);
			r0 = _mm_add_ps(r0 , r1);
			_mm_store_ps(d+i , r0);
			i+=4;
		}
		if (size & 2) {d[i] += (c*r[i]); d[i+1] += (c*r[i+1]); i+=2;}
		if (size & 1) {d[i] += (c*r[i]);}
	}
#endif
}	

// convolution(d,r) -> d
// input:	d[0..dsize-1], r[0..rsize-1]
// output:	d[0..dsize+rsize-2]
void VectorFunctions::conv(float* d, float* r, int dsize, int rsize)
{
	dsize--; rsize--;
	float x;
	int i,j, dtot = dsize+rsize;
	for (i=dsize+1; i<=dtot; i++) {d[i]=0;}
	if (rsize >= dsize) {
		for (i=dtot; i>=rsize; i--) {
			x=0; for (j=i-dsize; j<=rsize; j++) {x += (d[i-j]*r[j]);} d[i]=x;}
	}
	else {
		for (i=dtot; i>=dsize; i--) {
			x=0; for (j=i-dsize; j<=rsize; j++) {x += (d[i-j]*r[j]);} d[i]=x;}
#ifdef ICSTLIB_NO_SSEOPT		
		for (i=dsize-1; i>=rsize; i--) {
			x=0; for (j=0; j<=rsize; j++) {x += (d[i-j]*r[j]);} d[i]=x;}
#else
		__m128 cf,tmp0,tmp1,tmp2,tmp3,r0,r1,r2,r3;
		i = dsize-1;
		if (reinterpret_cast<uintptr_t>(r) & 0xF) {	// r unaligned
			while (i >= (rsize+3)) {
				r0 = _mm_setzero_ps();
				r1 = _mm_setzero_ps();
				r2 = _mm_setzero_ps();
				r3 = _mm_setzero_ps();
				j = 0;
				while (j <= (rsize-3)) {
					cf = _mm_loadu_ps(r+j);
					cf = _mm_shuffle_ps(cf , cf, _MM_SHUFFLE(0,1,2,3));
					tmp0 = _mm_loadu_ps(d+i-j-6);
					tmp3 = _mm_loadu_ps(d+i-j-3);
					tmp1 = _mm_shuffle_ps(tmp0 , tmp3, _MM_SHUFFLE(1,0,2,1));
					tmp2 = _mm_shuffle_ps(tmp0 , tmp3, _MM_SHUFFLE(2,1,3,2));
					tmp0 = _mm_mul_ps(tmp0 , cf);
					tmp3 = _mm_mul_ps(tmp3 , cf);
					tmp1 = _mm_mul_ps(tmp1 , cf);
					tmp2 = _mm_mul_ps(tmp2 , cf);
					r0 = _mm_add_ps(r0 , tmp0);
					r3 = _mm_add_ps(r3 , tmp3);
					r1 = _mm_add_ps(r1 , tmp1);
					r2 = _mm_add_ps(r2 , tmp2);
					j+=4;
				}
				tmp0 = _mm_setzero_ps();
				while (j <= rsize) {
					cf = _mm_load1_ps(r+j);
					tmp1 = _mm_loadu_ps(d+i-j-3);
					tmp1 = _mm_mul_ps(tmp1 , cf);
					tmp0 = _mm_add_ps(tmp0 , tmp1);
					j++;
				}
				tmp1 = _mm_unpacklo_ps(r0 , r1);
				tmp2 = _mm_unpackhi_ps(r0 , r1);
				r0 = _mm_add_ps(tmp1 , tmp2);
				tmp1 = _mm_unpacklo_ps(r2 , r3);
				tmp2 = _mm_unpackhi_ps(r2 , r3);
				r2 = _mm_add_ps(tmp1 , tmp2);
				tmp1 = _mm_movelh_ps(r0 , r2);
				tmp2 = _mm_movehl_ps(r2 , r0);
				r0 = _mm_add_ps(tmp1 , tmp2);
				r0 = _mm_add_ps(r0 , tmp0);
				_mm_storeu_ps(d+i-3 , r0);
				i-=4;
			}
		}
		else {	// r aligned, faster
			while (i >= (rsize+3)) {
				if ((reinterpret_cast<uintptr_t>(d+i-3) & 0xF) == 0) {break;}
				x = 0;
				for (j=0; j<=rsize; j++) {x += (d[i-j]*r[j]);}
				d[i] = x;
				i--;
			}
			while (i >= (rsize+3)) {
				r0 = _mm_setzero_ps();
				r1 = _mm_setzero_ps();
				r2 = _mm_setzero_ps();
				r3 = _mm_setzero_ps();
				j = 0;
				while (j <= (rsize-4)) {
					cf = _mm_load_ps(r+j);
					cf = _mm_shuffle_ps(cf , cf, _MM_SHUFFLE(0,1,2,3));
					tmp0 = _mm_load_ps(d+i-j-7);
					tmp3 = _mm_load_ps(d+i-j-3);
					tmp1 = _mm_shuffle_ps(tmp0 , tmp3, _MM_SHUFFLE(1,0,3,2));
					tmp0 = _mm_shuffle_ps(tmp0 , tmp1, _MM_SHUFFLE(2,1,2,1));
					tmp2 = _mm_shuffle_ps(tmp1 , tmp3, _MM_SHUFFLE(2,1,2,1));
					tmp3 = _mm_mul_ps(tmp3 , cf);
					tmp1 = _mm_mul_ps(tmp1 , cf);
					tmp0 = _mm_mul_ps(tmp0 , cf);
					tmp2 = _mm_mul_ps(tmp2 , cf);
					r3 = _mm_add_ps(r3 , tmp3);
					r1 = _mm_add_ps(r1 , tmp1);
					r0 = _mm_add_ps(r0 , tmp0);
					r2 = _mm_add_ps(r2 , tmp2);
					j+=4;
				}
				tmp0 = _mm_setzero_ps();
				while (j <= rsize) {
					cf = _mm_load1_ps(r+j);
					tmp1 = _mm_loadu_ps(d+i-j-3);
					tmp1 = _mm_mul_ps(tmp1 , cf);
					tmp0 = _mm_add_ps(tmp0 , tmp1);
					j++;
				}
				tmp1 = _mm_unpacklo_ps(r0 , r1);
				tmp2 = _mm_unpackhi_ps(r0 , r1);
				r0 = _mm_add_ps(tmp1 , tmp2);
				tmp1 = _mm_unpacklo_ps(r2 , r3);
				tmp2 = _mm_unpackhi_ps(r2 , r3);
				r2 = _mm_add_ps(tmp1 , tmp2);
				tmp1 = _mm_movelh_ps(r0 , r2);
				tmp2 = _mm_movehl_ps(r2 , r0);
				r0 = _mm_add_ps(tmp1 , tmp2);
				r0 = _mm_add_ps(r0 , tmp0);
				_mm_store_ps(d+i-3 , r0);
				i-=4;
			}
		}
		while (i >= rsize) {
			x = 0;
			for (j=0; j<=rsize; j++) {x += (d[i-j]*r[j]);}
			d[i] = x;
			i--;
		}
#endif		
	}
	for (i=rsize-1; i>=0; i--) {
		x=0; for (j=0; j<=i; j++) {x += (d[i-j]*r[j]);} d[i]=x;}
}		 

// cross correlation(d,r) -> d
// input:	d[0..dsize-1], r[0..rsize-1]
// output:	d[0..pts-1], pts<=dsize
void VectorFunctions::ccorr(float* d, float* r, int dsize, int rsize, int pts)
{
	float x;
	int i,j, n = __max(0,__min(pts,dsize-rsize+1));
#ifdef ICSTLIB_NO_SSEOPT
	for (i=0; i<n; i++) {
		x = 0;
		for (j=0; j<rsize; j++) {x += (d[i+j]*r[j]);}
		d[i] = x;
	}
#else
	__m128 cf,tmp0,tmp1,tmp2,tmp3,r0,r1,r2,r3;
	i=0;
	if ((reinterpret_cast<uintptr_t>(d) | reinterpret_cast<uintptr_t>(r)) & 0xF) {
		while (i < (n-3)) {
			r0 = _mm_setzero_ps();
			r1 = _mm_setzero_ps();
			r2 = _mm_setzero_ps();
			r3 = _mm_setzero_ps();
			j=0;
			while (j < (rsize-3)) {
				cf = _mm_loadu_ps(r+j);
				tmp0 = _mm_loadu_ps(d+i+j);
				tmp3 = _mm_loadu_ps(d+i+j+3);
				tmp1 = _mm_shuffle_ps(tmp0 , tmp3, _MM_SHUFFLE(1,0,2,1));
				tmp2 = _mm_shuffle_ps(tmp0 , tmp3, _MM_SHUFFLE(2,1,3,2));
				tmp0 = _mm_mul_ps(tmp0 , cf);
				tmp3 = _mm_mul_ps(tmp3 , cf);
				tmp1 = _mm_mul_ps(tmp1 , cf);
				tmp2 = _mm_mul_ps(tmp2 , cf);
				r0 = _mm_add_ps(r0 , tmp0);
				r3 = _mm_add_ps(r3 , tmp3);
				r1 = _mm_add_ps(r1 , tmp1);
				r2 = _mm_add_ps(r2 , tmp2);
				j+=4;
			}
			tmp0 = _mm_setzero_ps();
			while (j < rsize) {
				cf = _mm_load1_ps(r+j);
				tmp1 = _mm_loadu_ps(d+i+j);
				tmp1 = _mm_mul_ps(tmp1 , cf);
				tmp0 = _mm_add_ps(tmp0 , tmp1);
				j++;
			}	
			tmp1 = _mm_unpacklo_ps(r0 , r1);
			tmp2 = _mm_unpackhi_ps(r0 , r1);
			r0 = _mm_add_ps(tmp1 , tmp2);
			tmp1 = _mm_unpacklo_ps(r2 , r3);
			tmp2 = _mm_unpackhi_ps(r2 , r3);
			r2 = _mm_add_ps(tmp1 , tmp2);
			tmp1 = _mm_movelh_ps(r0 , r2);
			tmp2 = _mm_movehl_ps(r2 , r0);
			r0 = _mm_add_ps(tmp1 , tmp2);
			r0 = _mm_add_ps(r0 , tmp0);
			_mm_storeu_ps(d+i , r0);
			i+=4;
		}		
	}
	else {	// data aligned, faster
		while (i < (n-3)) {
			r0 = _mm_setzero_ps();
			r1 = _mm_setzero_ps();
			r2 = _mm_setzero_ps();
			r3 = _mm_setzero_ps();
			j=0;
			while (j < (rsize-4)) {
				cf = _mm_load_ps(r+j);
				tmp0 = _mm_load_ps(d+i+j);
				tmp3 = _mm_load_ps(d+i+j+4);
				tmp2 = _mm_shuffle_ps(tmp0 , tmp3, _MM_SHUFFLE(1,0,3,2));
				tmp3 = _mm_shuffle_ps(tmp2 , tmp3, _MM_SHUFFLE(2,1,2,1));
				tmp1 = _mm_shuffle_ps(tmp0 , tmp2, _MM_SHUFFLE(2,1,2,1));
				tmp0 = _mm_mul_ps(tmp0 , cf);
				tmp2 = _mm_mul_ps(tmp2 , cf);
				tmp3 = _mm_mul_ps(tmp3 , cf);
				tmp1 = _mm_mul_ps(tmp1 , cf);
				r0 = _mm_add_ps(r0 , tmp0);
				r2 = _mm_add_ps(r2 , tmp2);
				r3 = _mm_add_ps(r3 , tmp3);
				r1 = _mm_add_ps(r1 , tmp1);
				j+=4;
			}
			tmp0 = _mm_setzero_ps();
			while (j < rsize) {
				cf = _mm_load1_ps(r+j);
				tmp1 = _mm_loadu_ps(d+i+j);
				tmp1 = _mm_mul_ps(tmp1 , cf);
				tmp0 = _mm_add_ps(tmp0 , tmp1);
				j++;
			}	
			tmp1 = _mm_unpacklo_ps(r0 , r1);
			tmp2 = _mm_unpackhi_ps(r0 , r1);
			r0 = _mm_add_ps(tmp1 , tmp2);
			tmp1 = _mm_unpacklo_ps(r2 , r3);
			tmp2 = _mm_unpackhi_ps(r2 , r3);
			r2 = _mm_add_ps(tmp1 , tmp2);
			tmp1 = _mm_movelh_ps(r0 , r2);
			tmp2 = _mm_movehl_ps(r2 , r0);
			r0 = _mm_add_ps(tmp1 , tmp2);
			r0 = _mm_add_ps(r0 , tmp0);
			_mm_store_ps(d+i , r0);
			i+=4;
		}
	}
	while (i < n) {
		x = 0;
		for (j=0; j<rsize; j++) {x += (d[i+j]*r[j]);}
		d[i] = x;
		i++;
	}
#endif	
	for (i=n; i<pts; i++) {
		x = 0;
		for (j=0; j<(dsize-i); j++) {x += (d[i+j]*r[j]);}
		d[i] = x;
	}
}

// unbiased autocorrelation of r[0..rsize-1] -> d[0..dsize-1], dsize<=rsize
void VectorFunctions::uacorr(float* d, float* r, int dsize, int rsize)
{
#ifdef ICSTLIB_NO_SSEOPT
	float x; int i,j;
	for (i=0; i<dsize; i++) {
		x = 0;
		for (j=0; j<=(rsize-dsize); j++) {x += (r[i+j]*r[j]);}
		d[i] = x;
	}
#else
	__m128 cf,tmp0,tmp1,tmp2,tmp3,r0,r1,r2,r3;
	int i=0, j;
	if (reinterpret_cast<uintptr_t>(r) & 0xF) {	// r unaligned
		while (i <= (dsize-4)) {
			r0 = _mm_setzero_ps();
			r1 = _mm_setzero_ps();
			r2 = _mm_setzero_ps();
			r3 = _mm_setzero_ps();
			j=0;
			while (j <= (rsize-dsize-3)) {
				cf = _mm_loadu_ps(r+j);
				tmp0 = _mm_loadu_ps(r+i+j);
				tmp3 = _mm_loadu_ps(r+i+j+3);
				tmp1 = _mm_shuffle_ps(tmp0 , tmp3, _MM_SHUFFLE(1,0,2,1));
				tmp2 = _mm_shuffle_ps(tmp0 , tmp3, _MM_SHUFFLE(2,1,3,2));
				tmp0 = _mm_mul_ps(tmp0 , cf);
				tmp3 = _mm_mul_ps(tmp3 , cf);
				tmp1 = _mm_mul_ps(tmp1 , cf);
				tmp2 = _mm_mul_ps(tmp2 , cf);
				r0 = _mm_add_ps(r0 , tmp0);
				r3 = _mm_add_ps(r3 , tmp3);
				r1 = _mm_add_ps(r1 , tmp1);
				r2 = _mm_add_ps(r2 , tmp2);
				j+=4;
			}
			tmp0 = _mm_setzero_ps();
			while (j <= (rsize-dsize)) {
				cf = _mm_load1_ps(r+j);
				tmp1 = _mm_loadu_ps(r+i+j);
				tmp1 = _mm_mul_ps(tmp1 , cf);
				tmp0 = _mm_add_ps(tmp0 , tmp1);
				j++;
			}	
			tmp1 = _mm_unpacklo_ps(r0 , r1);
			tmp2 = _mm_unpackhi_ps(r0 , r1);
			r0 = _mm_add_ps(tmp1 , tmp2);
			tmp1 = _mm_unpacklo_ps(r2 , r3);
			tmp2 = _mm_unpackhi_ps(r2 , r3);
			r2 = _mm_add_ps(tmp1 , tmp2);
			tmp1 = _mm_movelh_ps(r0 , r2);
			tmp2 = _mm_movehl_ps(r2 , r0);
			r0 = _mm_add_ps(tmp1 , tmp2);
			r0 = _mm_add_ps(r0 , tmp0);
			_mm_storeu_ps(d+i , r0);
			i+=4;
		}		
	}
	else {	// r aligned, faster
		while (i <= (dsize-4)) {
			r0 = _mm_setzero_ps();
			r1 = _mm_setzero_ps();
			r2 = _mm_setzero_ps();
			r3 = _mm_setzero_ps();
			j=0;
			while (j <= (rsize-dsize-4)) {
				cf = _mm_load_ps(r+j);
				tmp0 = _mm_load_ps(r+i+j);
				tmp3 = _mm_load_ps(r+i+j+4);
				tmp2 = _mm_shuffle_ps(tmp0 , tmp3, _MM_SHUFFLE(1,0,3,2));
				tmp3 = _mm_shuffle_ps(tmp2 , tmp3, _MM_SHUFFLE(2,1,2,1));
				tmp1 = _mm_shuffle_ps(tmp0 , tmp2, _MM_SHUFFLE(2,1,2,1));
				tmp0 = _mm_mul_ps(tmp0 , cf);
				tmp2 = _mm_mul_ps(tmp2 , cf);
				tmp3 = _mm_mul_ps(tmp3 , cf);
				tmp1 = _mm_mul_ps(tmp1 , cf);
				r0 = _mm_add_ps(r0 , tmp0);
				r2 = _mm_add_ps(r2 , tmp2);
				r3 = _mm_add_ps(r3 , tmp3);
				r1 = _mm_add_ps(r1 , tmp1);
				j+=4;
			}
			tmp0 = _mm_setzero_ps();
			while (j <= (rsize-dsize)) {
				cf = _mm_load1_ps(r+j);
				tmp1 = _mm_loadu_ps(r+i+j);
				tmp1 = _mm_mul_ps(tmp1 , cf);
				tmp0 = _mm_add_ps(tmp0 , tmp1);
				j++;
			}	
			tmp1 = _mm_unpacklo_ps(r0 , r1);
			tmp2 = _mm_unpackhi_ps(r0 , r1);
			r0 = _mm_add_ps(tmp1 , tmp2);
			tmp1 = _mm_unpacklo_ps(r2 , r3);
			tmp2 = _mm_unpackhi_ps(r2 , r3);
			r2 = _mm_add_ps(tmp1 , tmp2);
			tmp1 = _mm_movelh_ps(r0 , r2);
			tmp2 = _mm_movehl_ps(r2 , r0);
			r0 = _mm_add_ps(tmp1 , tmp2);
			r0 = _mm_add_ps(r0 , tmp0);
			_mm_storeu_ps(d+i , r0);
			i+=4;
		}
	}
	float x;
	while (i < dsize) {
		x = 0;
		for (j=0; j<=(rsize-dsize); j++) {x += (r[i+j]*r[j]);}
		d[i] = x;
		i++;
	}
#endif	
}

// biased autocorrelation of r[0..rsize-1] -> d[0..dsize-1], dsize<=rsize
void VectorFunctions::bacorr(float* d, float* r, int dsize, int rsize)
{
#ifdef ICSTLIB_NO_SSEOPT  
	float x; int i,j;
	for (i=0; i<dsize; i++) {
		x = 0;
		for (j=0; j<(rsize-i); j++) {x += (r[i+j]*r[j]);}
		d[i] = x;
	}
#else
	__m128 cf,tmp0,tmp1,tmp2,tmp3,r0,r1,r2,r3;
	int i=0, j;
	if (reinterpret_cast<uintptr_t>(r) & 0xF) {	// r unaligned
		while (i <= (dsize-4)) {
			r0 = _mm_setzero_ps();
			r1 = _mm_setzero_ps();
			r2 = _mm_setzero_ps();
			r3 = _mm_setzero_ps();
			j=0;
			while (j < (rsize-i-6)) {
				cf = _mm_loadu_ps(r+j);
				tmp0 = _mm_loadu_ps(r+i+j);
				tmp3 = _mm_loadu_ps(r+i+j+3);
				tmp1 = _mm_shuffle_ps(tmp0 , tmp3, _MM_SHUFFLE(1,0,2,1));
				tmp2 = _mm_shuffle_ps(tmp0 , tmp3, _MM_SHUFFLE(2,1,3,2));
				tmp0 = _mm_mul_ps(tmp0 , cf);
				tmp3 = _mm_mul_ps(tmp3 , cf);
				tmp1 = _mm_mul_ps(tmp1 , cf);
				tmp2 = _mm_mul_ps(tmp2 , cf);
				r0 = _mm_add_ps(r0 , tmp0);
				r3 = _mm_add_ps(r3 , tmp3);
				r1 = _mm_add_ps(r1 , tmp1);
				r2 = _mm_add_ps(r2 , tmp2);
				j+=4;
			}
			tmp0 = _mm_setzero_ps();
			while (j < (rsize-i-3)) {
				cf = _mm_load1_ps(r+j);
				tmp1 = _mm_loadu_ps(r+i+j);
				tmp1 = _mm_mul_ps(tmp1 , cf);
				tmp0 = _mm_add_ps(tmp0 , tmp1);
				j++;
			}	
			tmp1 = _mm_unpacklo_ps(r0 , r1);
			tmp2 = _mm_unpackhi_ps(r0 , r1);
			r0 = _mm_add_ps(tmp1 , tmp2);
			tmp1 = _mm_unpacklo_ps(r2 , r3);
			tmp2 = _mm_unpackhi_ps(r2 , r3);
			r2 = _mm_add_ps(tmp1 , tmp2);
			tmp1 = _mm_movelh_ps(r0 , r2);
			tmp2 = _mm_movehl_ps(r2 , r0);
			r0 = _mm_add_ps(tmp1 , tmp2);
			r0 = _mm_add_ps(r0 , tmp0);
			_mm_storeu_ps(d+i , r0);
			d[i] += (r[j+2]*r[i+j+2]);
			d[i+1] += (r[j+1]*r[i+j+2]);
			d[i+2] += (r[j]*r[i+j+2]);
			d[i] += (r[j+1]*r[i+j+1]);
			d[i+1] += (r[j]*r[i+j+1]);
			d[i] += (r[j]*r[i+j]);
			i+=4;	
		}		
	}
	else {	// r aligned, faster
		while (i <= (dsize-4)) {
			r0 = _mm_setzero_ps();
			r1 = _mm_setzero_ps();
			r2 = _mm_setzero_ps();
			r3 = _mm_setzero_ps();
			j=0;
			while (j < (rsize-i-7)) {
				cf = _mm_load_ps(r+j);
				tmp0 = _mm_load_ps(r+i+j);
				tmp3 = _mm_load_ps(r+i+j+4);
				tmp2 = _mm_shuffle_ps(tmp0 , tmp3, _MM_SHUFFLE(1,0,3,2));
				tmp3 = _mm_shuffle_ps(tmp2 , tmp3, _MM_SHUFFLE(2,1,2,1));
				tmp1 = _mm_shuffle_ps(tmp0 , tmp2, _MM_SHUFFLE(2,1,2,1));
				tmp0 = _mm_mul_ps(tmp0 , cf);
				tmp2 = _mm_mul_ps(tmp2 , cf);
				tmp3 = _mm_mul_ps(tmp3 , cf);
				tmp1 = _mm_mul_ps(tmp1 , cf);
				r0 = _mm_add_ps(r0 , tmp0);
				r2 = _mm_add_ps(r2 , tmp2);
				r3 = _mm_add_ps(r3 , tmp3);
				r1 = _mm_add_ps(r1 , tmp1);
				j+=4;
			}
			tmp0 = _mm_setzero_ps();
			while (j < (rsize-i-3)) {
				cf = _mm_load1_ps(r+j);
				tmp1 = _mm_loadu_ps(r+i+j);
				tmp1 = _mm_mul_ps(tmp1 , cf);
				tmp0 = _mm_add_ps(tmp0 , tmp1);
				j++;
			}	
			tmp1 = _mm_unpacklo_ps(r0 , r1);
			tmp2 = _mm_unpackhi_ps(r0 , r1);
			r0 = _mm_add_ps(tmp1 , tmp2);
			tmp1 = _mm_unpacklo_ps(r2 , r3);
			tmp2 = _mm_unpackhi_ps(r2 , r3);
			r2 = _mm_add_ps(tmp1 , tmp2);
			tmp1 = _mm_movelh_ps(r0 , r2);
			tmp2 = _mm_movehl_ps(r2 , r0);
			r0 = _mm_add_ps(tmp1 , tmp2);
			r0 = _mm_add_ps(r0 , tmp0);
			_mm_storeu_ps(d+i , r0);
			d[i] += (r[j+2]*r[i+j+2]);
			d[i+1] += (r[j+1]*r[i+j+2]);
			d[i+2] += (r[j]*r[i+j+2]);
			d[i] += (r[j+1]*r[i+j+1]);
			d[i+1] += (r[j]*r[i+j+1]);
			d[i] += (r[j]*r[i+j]);
			i+=4;
		}
	}
	float x;
	while (i < dsize) {
		x = 0;
		for (j=0; j<(rsize-i); j++) {x += (r[i+j]*r[j]);}
		d[i] = x;
		i++;
	}
#endif	
}

// fast convolution(d,r) via FFT -> d
// input:	d[0..dsize-1], r[0..rsize-1]
// output:	d[0..dsize+rsize-2]	
// size of both d and r >= nexthigherpow2(dsize+rsize)
void FFTProcessor::fastConvolution(float* d, float* r, int dsize, int rsize)
{
	int tsize = VectorFunctions::nexthipow2(rsize + dsize);
	VectorFunctions::set(d + dsize, 0, tsize - dsize);
	VectorFunctions::set(r + rsize, 0, tsize - rsize);
	realfft(d,tsize);
	realfft(r,tsize);
	float tmp = d[1]*r[1];	// process aligned if d and r are aligned	
	d[1] = 0;				//
	VectorFunctions::cpxmul(d,r,tsize>>1);	// 
	d[1] = tmp;				//
	realifft(d,tsize);
}

// FFT-based fast cross correlation(d,r) -> d
// input:	d[0..dsize-1], r[0..rsize-1]
// output:	d[0..dsize-1]
// size of both d and r => nexthipow2(dsize+rsize)
void FFTProcessor::fastCrossCorrelation(float* d, float* r, int dsize, int rsize)
{
	int tsize = VectorFunctions::nexthipow2(rsize + dsize);
	int hsize = tsize>>1;	
	VectorFunctions::set(d+dsize, 0, tsize-dsize);
	VectorFunctions::set(r + rsize, 0, tsize - rsize);
	realfft(d,tsize);
	realfft(r,tsize);
	VectorFunctions::cpxconj(r, hsize);		// process aligned if d and r are aligned
	float tmp = -d[1]*r[1];	// 	
	d[1] = 0;				//
	VectorFunctions::cpxmul(d, r, hsize);		// 
	d[1] = tmp;				//
	realifft(d,tsize);
}

// FFT-based fast biased autocorrelation of d
// d[0..size-1] -> d[0..size-1], size of d => nexthipow2(2*size)
void FFTProcessor::fastAutoCorrelation(float* d, int size)
{
	int i, tsize = VectorFunctions::nexthipow2(2 * size);
	VectorFunctions::set(d + size, 0, tsize - size);
	realfft(d,tsize);
	d[0] *= d[0]; d[1] *= d[1];
	for (i=2; i<tsize; i+=2) {d[i] = d[i]*d[i] + d[i+1]*d[i+1]; d[i+1]=0;} 
	realifft(d,tsize);
}			
												
//******************************************************************************
//* elementary complex array operations
//* 
//* format of d: re[0] im[0] re[1] ... im[size-1]
//*
// conjugate d: d -> d*
void VectorFunctions::cpxconj(float* d, int size) {
	int i=0;
#ifndef ICSTLIB_NO_SSEOPT  
	if (reinterpret_cast<uintptr_t>(d) & 0xF) {
#endif
		for (i=1; i<(2*size); i+=2) {d[i] *= -1.0f;}
#ifndef ICSTLIB_NO_SSEOPT
	}
	else {
		__m128 r0, r1;
		__m128 mask = _mm_castsi128_ps(_mm_set_epi32(0x80000000,0,0x80000000,0));
		size <<= 1;
		while (i <= (size - 8)) {
			r0 = _mm_load_ps(d+i);
			r0 = _mm_xor_ps(r0 , mask);
			r1 = _mm_load_ps(d+i+4);
			r1 = _mm_xor_ps(r1 , mask);
			_mm_store_ps(d+i , r0);
			_mm_store_ps(d+i+4 , r1);
			i+=8;
		}
		if (size & 4) {
			r0 = _mm_load_ps(d+i);
			r0 = _mm_xor_ps(r0 , mask);
			_mm_store_ps(d+i , r0);
			i+=4;
		}
		if (size & 2) {d[i+1] *= -1.0f;}
	}
#endif		
}

// return sum
Complex VectorFunctions::cpxsum(float* d, int size)
{
	int i=0;
#ifndef ICSTLIB_NO_SSEOPT  
	if (reinterpret_cast<uintptr_t>(d) & 0xF) {
#endif
		int rm = 2*(size - ((size>>3)<<3));
		float a=0,b=0; double x,y; Complex res;
		for (i=0; i<rm; i+=2) {a += d[i]; b += d[i+1];}
		x = static_cast<double>(a); y = static_cast<double>(b);
		for (i=rm; i<(2*size); i+=16) {
			a = b = 0;
			a += d[i];		b += d[i+1];	a += d[i+2];	b += d[i+3];
			a += d[i+4];	b += d[i+5];	a += d[i+6];	b += d[i+7];
			a += d[i+8];	b += d[i+9];	a += d[i+10];	b += d[i+11];
			a += d[i+12];	b += d[i+13];	a += d[i+14];	b += d[i+15];
			x += static_cast<double>(a);	y += static_cast<double>(b);
		}
		res.re = static_cast<float>(x); res.im = static_cast<float>(y);
		return res;
#ifndef ICSTLIB_NO_SSEOPT
	}
	else {
		__m128 r0,r1,r2,r3,acc0,acc1,acc2,acc3;
		__m128d tmp0,tmp1,sum0,sum1;
		float res[4]; Complex rval;
		sum0 = _mm_setzero_pd(); sum1 = _mm_setzero_pd();
		size <<= 1;
		while (i <= (size-32)) {
			acc0 = _mm_load_ps(d+i);
			acc1 = _mm_load_ps(d+i+4);
			acc2 = _mm_load_ps(d+i+8);
			acc3 = _mm_load_ps(d+i+12);
			r0 = _mm_load_ps(d+i+16);
			acc0 = _mm_add_ps(acc0 , r0);
			r1 = _mm_load_ps(d+i+20);
			acc1 = _mm_add_ps(acc1 , r1);
			r2 = _mm_load_ps(d+i+24);
			acc2 = _mm_add_ps(acc2 , r2);
			r3 = _mm_load_ps(d+i+28);
			acc3 = _mm_add_ps(acc3 , r3);
			acc0 = _mm_add_ps(acc0 , acc1);
			acc2 = _mm_add_ps(acc2 , acc3);
			acc0 = _mm_add_ps(acc0 , acc2);
			acc1 = _mm_movehl_ps(acc1 , acc0);
			tmp0 = _mm_cvtps_pd(acc0);
			tmp1 = _mm_cvtps_pd(acc1);
			sum0 = _mm_add_pd(sum0 , tmp0);
			sum1 = _mm_add_pd(sum1 , tmp1);
			i+=32;
		}
		acc0 = _mm_cvtpd_ps(sum0);
		acc1 = _mm_cvtpd_ps(sum1);
		if (size & 16) {
			r0 = _mm_load_ps(d+i);
			acc0 = _mm_add_ps(acc0 , r0);
			r1 = _mm_load_ps(d+i+4);
			acc1 = _mm_add_ps(acc1 , r1);
			r0 = _mm_load_ps(d+i+8);
			acc0 = _mm_add_ps(acc0 , r0);
			r1 = _mm_load_ps(d+i+12);
			acc1 = _mm_add_ps(acc1 , r1);
			i+=16;
		}
		if (size & 8) {
			r0 = _mm_load_ps(d+i);
			acc0 = _mm_add_ps(acc0 , r0);
			r1 = _mm_load_ps(d+i+4);
			acc1 = _mm_add_ps(acc1 , r1);
			i+=8;
		}
		if (size & 4) {
			r0 = _mm_load_ps(d+i);
			acc0 = _mm_add_ps(acc0 , r0);
			i+=4;
		}
		acc0 = _mm_add_ps(acc0 , acc1);
		_mm_storeu_ps(res , acc0);
		if (size & 2) {res[0] += d[i]; res[1] += d[i+1];}
		rval.re = res[0] + res[2]; rval.im = res[1] + res[3];
		return rval;
	}
#endif
}			

// 1/d -> d
void VectorFunctions::cpxinv(float* d, int size, bool fullrange)
{
	static const double LLIM = 1.0/(static_cast<double>(FLT_MAX)*static_cast<double>(FLT_MAX));
	int i=0; float x;
#ifndef ICSTLIB_NO_SSEOPT  
	if (fullrange) {
#else
	static_cast<void>(fullrange);	// inform compiler of intentionally unused parameter 
#endif
		double ad,bd,xd;
		for (i=0; i<(2*size); i+=2) {
			x = d[i]*d[i] + d[i+1]*d[i+1];
			if (x >= FLT_MIN) {x = 1.0f/x; d[i] *= x; d[i+1] *= (-x);}
			else {
				ad = static_cast<double>(d[i]); bd = static_cast<double>(d[i+1]);
				xd = ad*ad + bd*bd;
				xd = (xd > LLIM) ? 1.0/xd : HUGE_VAL;
				d[i] = static_cast<float>(ad*xd);
				d[i+1] = static_cast<float>(-bd*xd);
			}
		}		
#ifndef ICSTLIB_NO_SSEOPT
	}
	else {
		__m128 r0,r1,r2,r3,r4,r5,mask1,mask2,mask3;
		mask1 = _mm_castsi128_ps(_mm_set_epi32(0,0xffffffff,0,0xffffffff));
		mask2 = _mm_castsi128_ps(_mm_set_epi32(0xffffffff,0,0xffffffff,0));
		mask3 = _mm_castsi128_ps(_mm_set_epi32(0x80000000,0,0x80000000,0));
		size <<= 1;
		if (!(reinterpret_cast<uintptr_t>(d) & 0xF)) {
			while (i <= (size - 8)) {
				r0 = _mm_load_ps(d+i);
				r2 = _mm_mul_ps(r0 , r0);
				r1 = _mm_load_ps(d+i+4);
				r3 = _mm_mul_ps(r1 , r1);
				r4 = _mm_castsi128_ps(_mm_srli_epi64(_mm_castps_si128(r2) , 32));
				r2 = _mm_and_ps(r2 , mask1);
				r5 = _mm_castsi128_ps(_mm_slli_epi64(_mm_castps_si128(r3) , 32));
				r3 = _mm_and_ps(r3 , mask2);
				r2 = _mm_or_ps(r2 , r3);
				r4 = _mm_or_ps(r4 , r5);
				r2 = _mm_add_ps(r4 , r2);
				r3 = _mm_rcp_ps(r2);
				r2 = _mm_mul_ps(r2 , r3);
				r4 = _mm_add_ps(r3 , r3);
				r2 = _mm_mul_ps(r2 , r3);
				r0 = _mm_xor_ps(r0 , mask3);
				r1 = _mm_xor_ps(r1 , mask3);
				r4 = _mm_sub_ps(r4 , r2);
				r2 = _mm_shuffle_ps(r4 , r4 , _MM_SHUFFLE(2,2,0,0));
				r3 = _mm_shuffle_ps(r4 , r4 , _MM_SHUFFLE(3,3,1,1));
				r2 = _mm_mul_ps(r0 , r2);
				r3 = _mm_mul_ps(r1 , r3);
				_mm_store_ps(d+i , r2);
				_mm_store_ps(d+i+4 , r3);
				i+=8;
			}
		}
		else {
			while (i <= (size - 8)) {
				r0 = _mm_loadu_ps(d+i);
				r2 = _mm_mul_ps(r0 , r0);
				r1 = _mm_loadu_ps(d+i+4);
				r3 = _mm_mul_ps(r1 , r1);
				r4 = _mm_castsi128_ps(_mm_srli_epi64(_mm_castps_si128(r2) , 32));
				r2 = _mm_and_ps(r2 , mask1);
				r5 = _mm_castsi128_ps(_mm_slli_epi64(_mm_castps_si128(r3) , 32));
				r3 = _mm_and_ps(r3 , mask2);
				r2 = _mm_or_ps(r2 , r3);
				r4 = _mm_or_ps(r4 , r5);
				r2 = _mm_add_ps(r4 , r2);
				r3 = _mm_rcp_ps(r2);
				r2 = _mm_mul_ps(r2 , r3);
				r4 = _mm_add_ps(r3 , r3);
				r2 = _mm_mul_ps(r2 , r3);
				r0 = _mm_xor_ps(r0 , mask3);
				r1 = _mm_xor_ps(r1 , mask3);
				r4 = _mm_sub_ps(r4 , r2);
				r2 = _mm_shuffle_ps(r4 , r4 , _MM_SHUFFLE(2,2,0,0));
				r3 = _mm_shuffle_ps(r4 , r4 , _MM_SHUFFLE(3,3,1,1));
				r2 = _mm_mul_ps(r0 , r2);
				r3 = _mm_mul_ps(r1 , r3);
				_mm_storeu_ps(d+i , r2);
				_mm_storeu_ps(d+i+4 , r3);
				i+=8;
			}
		}
		while (i < size) {
			x = d[i]*d[i] + d[i+1]*d[i+1];
			x = (x > FLT_MIN) ? 1.0f/x : static_cast<float>(HUGE_VAL);
			d[i] *= x; d[i+1] *= (-x);
			i+=2;
		}		
	}
#endif	
}

// fill d with magnitude of r
void VectorFunctions::cpxmag(float* d, float* r, int size)
{
#ifdef ICSTLIB_NO_SSEOPT
	int i, j=0;
	for (i=0; i<size; i++) {d[i] = sqrtf(r[j]*r[j] + r[j+1]*r[j+1]); j+=2;}	
#else
	cpxpow(d,r,size);
	fsqrt(d,size);
#endif		
}

// fill d with magnitude^2 of r
void VectorFunctions::cpxpow(float* d, float* r, int size)
{
	int i=0, j=0;
#ifndef ICSTLIB_NO_SSEOPT  
	if ((reinterpret_cast<uintptr_t>(d) | reinterpret_cast<uintptr_t>(r)) & 0xF) {
#endif
		for (i=0; i<size; i++) {d[i] = r[j]*r[j] + r[j+1]*r[j+1]; j+=2;}		
#ifndef ICSTLIB_NO_SSEOPT
	}
	else {
		__m128 r0,r1,r2,r3;
		size <<= 1;
		while (i <= (size - 8)) {
			r0 = _mm_load_ps(r+i);
			r0 = _mm_mul_ps(r0 , r0);
			r2 = _mm_load_ps(r+i+4);
			r2 = _mm_mul_ps(r2 , r2);
			r1 = _mm_shuffle_ps(r0 , r2 , _MM_SHUFFLE(2,0,2,0));
			r3 = _mm_shuffle_ps(r0 , r2 , _MM_SHUFFLE(3,1,3,1));
			r3 = _mm_add_ps(r3 , r1);
			_mm_store_ps(d+j , r3);
			i+=8;
			j+=4;
		}		
		if (size & 4) {
			d[j] = r[i]*r[i] + r[i+1]*r[i+1];
			d[j+1] = r[i+2]*r[i+2] + r[i+3]*r[i+3];
			i+=4; j+=2;
		}
		if (size & 2) {d[j] = r[i]*r[i] + r[i+1]*r[i+1];}
	}
#endif	
}

// return signal energy: <d,d*>
float VectorFunctions::cpxenergy(float* d, int size) {return energy(d,2*size);}

// return L2 vector norm: sqrt(<d,d*>)		
float VectorFunctions::cpxnorm(float* d, int size) {return sqrtf(energy(d,2*size));}

// return signal power: <d,d*>/size
float VectorFunctions::cpxpower(float* d, int size) {
	return energy(d,2*size)/static_cast<float>(size);}

// return RMS value of d: sqrt(<d,d*>/size)
float VectorFunctions::cpxrms(float* d, int size) {
	return sqrtf(energy(d,2*size)/static_cast<float>(size));}

// normalize d to cpxnorm=1, return norm factor
float VectorFunctions::cpxnormalize(float* d, int size) {return normalize(d,2*size);}		

// if |d[n]|<=lim: rep -> Re(d[n]), 0 -> Im(d[n]) 
void VectorFunctions::cpxprune(float* d, float lim, float rep, int size)
{
	int i=0;
	lim *= lim;
#ifdef ICSTLIB_NO_SSEOPT  
	for (i=0; i<(2*size); i+=2) {
		if ((d[i]*d[i] + d[i+1]*d[i+1]) <= lim) {d[i]=rep; d[i+1]=0;}
	}		
#else
	__m128 r0,r1,r2,r3;
	__m128 limit = _mm_set1_ps(lim), rpl = _mm_set_ps(0,rep,0,rep);
	size <<= 1;
	if (!(reinterpret_cast<uintptr_t>(d) & 0xF)) {
		while (i <= (size - 4)) {
			r0 = _mm_load_ps(d+i);
			r1 = _mm_mul_ps(r0 , r0);
			r3 = _mm_sub_ps(rpl , r0);
			r2 = _mm_shuffle_ps(r1 , r1 , _MM_SHUFFLE(2,3,0,1));
			r1 = _mm_add_ps(r1 , r2);
			r1 = _mm_cmple_ps(r1 , limit);
			r3 = _mm_and_ps(r3 , r1);
			r3 = _mm_add_ps(r3 , r0);
			_mm_store_ps(d+i , r3);
			i+=4;
		}
	}
	else {
		while (i <= (size - 4)) {
			r0 = _mm_loadu_ps(d+i);
			r1 = _mm_mul_ps(r0 , r0);
			r3 = _mm_sub_ps(rpl , r0);
			r2 = _mm_shuffle_ps(r1 , r1 , _MM_SHUFFLE(2,3,0,1));
			r1 = _mm_add_ps(r1 , r2);
			r1 = _mm_cmple_ps(r1 , limit);
			r3 = _mm_and_ps(r3 , r1);
			r3 = _mm_add_ps(r3 , r0);
			_mm_storeu_ps(d+i , r3);
			i+=4;
		}
	}
	if (size & 2) {
		if ((d[i]*d[i] + d[i+1]*d[i+1]) <= lim) {d[i]=rep; d[i+1]=0;}
	}
#endif		
}

// c -> d
void VectorFunctions::cpxset(float* d, Complex c, int size)
{
	int i=0;
#ifndef ICSTLIB_NO_SSEOPT  
	if (reinterpret_cast<uintptr_t>(d) & 0xF) {
#endif
		for (i=0; i<(2*size); i+=2) {d[i] = c.re; d[i+1] = c.im;}
#ifndef ICSTLIB_NO_SSEOPT
	}
	else {
		__m128 r0 = _mm_set_ps(c.im, c.re, c.im, c.re);
		size <<= 1;
		while (i <= (size - 8)) {
			_mm_store_ps(d+i , r0);
			_mm_store_ps(d+i+4 , r0);
			i+=8;
		}
		if (size & 4) {
			_mm_store_ps(d+i , r0);
			i+=4;
		}
		if (size & 2) {d[i] = c.re; d[i+1] = c.im;}
	}
#endif
}

// r -> d, may overlap
void VectorFunctions::cpxcopy(float* d, float* r, int size){
	memmove(d,r,static_cast<size_t>(2*size)*sizeof(float));}

// r <-> d
void VectorFunctions::cpxswap(float* d, float* r, int size) {swap(d,r,2*size);}

// d+c -> d
void VectorFunctions::cpxadd(float* d, Complex c, int size) 
{
	int i=0;
#ifndef ICSTLIB_NO_SSEOPT  
	if (reinterpret_cast<uintptr_t>(d) & 0xF) {
#endif
		for (i=0; i<(2*size); i+=2) {d[i] += c.re; d[i+1] += c.im;}
#ifndef ICSTLIB_NO_SSEOPT
	}
	else {
		__m128 r0,r1, r2 = _mm_set_ps(c.im, c.re, c.im, c.re);
		size <<= 1;
		while (i <= (size - 8)) {
			r0 = _mm_load_ps(d+i);
			r0 = _mm_add_ps(r0 , r2);
			r1 = _mm_load_ps(d+i+4);
			r1 = _mm_add_ps(r1 , r2);
			_mm_store_ps(d+i , r0);
			_mm_store_ps(d+i+4 , r1);
			i+=8;
		}
		if (size & 4) {
			r0 = _mm_load_ps(d+i);
			r0 = _mm_add_ps(r0 , r2);
			_mm_store_ps(d+i , r0);
			i+=4;
		}
		if (size & 2) {d[i] += c.re; d[i+1] += c.im;}
	}
#endif
}

// d+r -> d
void VectorFunctions::cpxadd(float* d, float* r, int size) {add(d,r,2*size);}

// d-c -> d
void VectorFunctions::cpxsub(float* d, Complex c, int size) {
	c.re *= -1.0f; c.im *= -1.0f; cpxadd(d, c, size);}

// d-r -> d
void VectorFunctions::cpxsub(float* d, float* r, int size) {sub(d,r,2*size);}

// c*d -> d
void VectorFunctions::cpxmul(float* d, Complex c, int size)
{		
	int i=0; float tmp;
#ifdef ICSTLIB_NO_SSEOPT  
	for (i=0; i<(2*size); i+=2) {
		tmp = d[i];
		d[i] = tmp*c.re - d[i+1]*c.im;
		d[i+1] = d[i+1]*c.re + tmp*c.im;
	}
#else
	__m128 r0,r1,r2,r3,r4,r5,mask1,mask2;
	mask1 = _mm_castsi128_ps(_mm_set_epi32(0,0xffffffff,0,0xffffffff));
	mask2 = _mm_castsi128_ps(_mm_set_epi32(0xffffffff,0,0xffffffff,0));	
	r4 = _mm_set_ps(c.im, c.re, c.im, c.re);
	r5 = _mm_shuffle_ps(r4, r4, _MM_SHUFFLE(2,3,0,1));
	size <<= 1;
	if (!(reinterpret_cast<uintptr_t>(d) & 0xF)) {
		while (i <= (size - 4)) {
			r1 = _mm_load_ps(d+i);
			r0 = _mm_mul_ps(r4 , r1);
			r1 = _mm_mul_ps(r5 , r1);
			r2 = _mm_castsi128_ps(_mm_srli_epi64(_mm_castps_si128(r0) , 32));
			r3 = _mm_castsi128_ps(_mm_slli_epi64(_mm_castps_si128(r1) , 32));
			r0 = _mm_and_ps(r0 , mask1); 
			r1 = _mm_and_ps(r1, mask2);			
			r2 = _mm_sub_ps(r0 , r2);
			r3 = _mm_add_ps(r1 , r3);
			r3 = _mm_or_ps(r3 , r2);
			_mm_store_ps(d+i , r3);
			i+=4;
		}
	}
	else {
		while (i <= (size - 4)) {
			r1 = _mm_loadu_ps(d+i);
			r0 = _mm_mul_ps(r4 , r1);
			r1 = _mm_mul_ps(r5 , r1);
			r2 = _mm_castsi128_ps(_mm_srli_epi64(_mm_castps_si128(r0) , 32));
			r3 = _mm_castsi128_ps(_mm_slli_epi64(_mm_castps_si128(r1) , 32));
			r0 = _mm_and_ps(r0 , mask1); 
			r1 = _mm_and_ps(r1, mask2);			
			r2 = _mm_sub_ps(r0 , r2);
			r3 = _mm_add_ps(r1 , r3);
			r3 = _mm_or_ps(r3 , r2);
			_mm_storeu_ps(d+i , r3);
			i+=4;
		}
	}
	if (size & 2) {
		tmp = d[i];
		d[i] = tmp*c.re - d[i+1]*c.im;
		d[i+1] = d[i+1]*c.re + tmp*c.im;
	}
#endif	
}

// d*r -> d
void VectorFunctions::cpxmul(float* d, float* r, int size)
{
	int i=0; float tmp;
#ifdef ICSTLIB_NO_SSEOPT  
	for (i=0; i<(2*size); i+=2) {
		tmp = d[i]; 
		d[i] = tmp*r[i] - d[i+1]*r[i+1];
		d[i+1] = r[i]*d[i+1] + tmp*r[i+1];
	}
#else
	__m128 r0,r1,r2,r3,mask1,mask2;
	mask1 = _mm_castsi128_ps(_mm_set_epi32(0,0xffffffff,0,0xffffffff));
	mask2 = _mm_castsi128_ps(_mm_set_epi32(0xffffffff,0,0xffffffff,0));		
	size <<= 1;
	if (!((reinterpret_cast<uintptr_t>(d) | reinterpret_cast<uintptr_t>(r)) & 0xF)) {
		while (i <= (size - 4)) {
			r0 = _mm_load_ps(d+i);
			r1 = _mm_shuffle_ps(r0, r0, _MM_SHUFFLE(2,3,0,1));
			r2 = _mm_load_ps(r+i);
			r0 = _mm_mul_ps(r0 , r2);
			r1 = _mm_mul_ps(r1 , r2);
			r2 = _mm_castsi128_ps(_mm_srli_epi64(_mm_castps_si128(r0) , 32));
			r3 = _mm_castsi128_ps(_mm_slli_epi64(_mm_castps_si128(r1) , 32));
			r0 = _mm_and_ps(r0 , mask1); 
			r1 = _mm_and_ps(r1, mask2);			
			r2 = _mm_sub_ps(r0 , r2);
			r3 = _mm_add_ps(r1 , r3);
			r3 = _mm_or_ps(r3 , r2);
			_mm_store_ps(d+i , r3);
			i+=4;
		}
	}
	else {
		while (i <= (size - 4)) {
			r0 = _mm_loadu_ps(d+i);
			r1 = _mm_shuffle_ps(r0, r0, _MM_SHUFFLE(2,3,0,1));
			r2 = _mm_loadu_ps(r+i);
			r0 = _mm_mul_ps(r0 , r2);
			r1 = _mm_mul_ps(r1 , r2);
			r2 = _mm_castsi128_ps(_mm_srli_epi64(_mm_castps_si128(r0) , 32));
			r3 = _mm_castsi128_ps(_mm_slli_epi64(_mm_castps_si128(r1) , 32));
			r0 = _mm_and_ps(r0 , mask1); 
			r1 = _mm_and_ps(r1, mask2);			
			r2 = _mm_sub_ps(r0 , r2);
			r3 = _mm_add_ps(r1 , r3);
			r3 = _mm_or_ps(r3 , r2);
			_mm_storeu_ps(d+i , r3);
			i+=4;
		}
	}
	if (size & 2) {
		tmp = d[i]; 
		d[i] = tmp*r[i] - d[i+1]*r[i+1];
		d[i+1] = r[i]*d[i+1] + tmp*r[i+1];	
	}
#endif
}

// d + c*r -> d
void VectorFunctions::cpxmac(float* d, float* r, Complex c, int size)
{
	int i=0;
#ifdef ICSTLIB_NO_SSEOPT  
	for (i=0; i<(2*size); i+=2) {
		d[i] += (r[i]*c.re - r[i+1]*c.im);
		d[i+1] += (r[i]*c.im + r[i+1]*c.re);
	}
#else
	__m128 r0,r1,r2,r3,r4,r5,mask1,mask2;
	mask1 = _mm_castsi128_ps(_mm_set_epi32(0,0xffffffff,0,0xffffffff));
	mask2 = _mm_castsi128_ps(_mm_set_epi32(0xffffffff,0,0xffffffff,0));	
	r4 = _mm_set_ps(c.im, c.re, c.im, c.re);
	r5 = _mm_shuffle_ps(r4, r4, _MM_SHUFFLE(2,3,0,1));
	size <<= 1;
	if (!((reinterpret_cast<uintptr_t>(d) | reinterpret_cast<uintptr_t>(r)) & 0xF)) {
		while (i <= (size - 4)) {
			r1 = _mm_load_ps(r+i);
			r0 = _mm_mul_ps(r4 , r1);
			r1 = _mm_mul_ps(r5 , r1);
			r2 = _mm_castsi128_ps(_mm_srli_epi64(_mm_castps_si128(r0) , 32));
			r3 = _mm_castsi128_ps(_mm_slli_epi64(_mm_castps_si128(r1) , 32));
			r0 = _mm_and_ps(r0 , mask1); 
			r1 = _mm_and_ps(r1, mask2);			
			r2 = _mm_sub_ps(r0 , r2);
			r3 = _mm_add_ps(r1 , r3);
			r3 = _mm_or_ps(r3 , r2);
			r2 = _mm_load_ps(d+i);
			r3 = _mm_add_ps(r3 , r2);
			_mm_store_ps(d+i , r3);
			i+=4;
		}
	}
	else {
		while (i <= (size - 4)) {
			r1 = _mm_loadu_ps(r+i);
			r0 = _mm_mul_ps(r4 , r1);
			r1 = _mm_mul_ps(r5 , r1);
			r2 = _mm_castsi128_ps(_mm_srli_epi64(_mm_castps_si128(r0) , 32));
			r3 = _mm_castsi128_ps(_mm_slli_epi64(_mm_castps_si128(r1) , 32));
			r0 = _mm_and_ps(r0 , mask1); 
			r1 = _mm_and_ps(r1, mask2);			
			r2 = _mm_sub_ps(r0 , r2);
			r3 = _mm_add_ps(r1 , r3);
			r3 = _mm_or_ps(r3 , r2);
			r2 = _mm_loadu_ps(d+i);
			r3 = _mm_add_ps(r3 , r2);
			_mm_storeu_ps(d+i , r3);
			i+=4;
		}	
	}
	if (size & 2) {
		d[i] += (r[i]*c.re - r[i+1]*c.im);
		d[i+1] += (r[i]*c.im + r[i+1]*c.re);
	}
#endif	
}

// fill d with argument of r
void VectorFunctions::cpxarg(float* d, float* r, int size)
{
	int i=0, j=0;
#ifdef ICSTLIB_NO_SSEOPT  
	for (i=0; i<size; i++,j+=2) {d[i] = atan2f(r[j+1],r[j]);}
#else
	__m128 r0, r1, r2, r3, r4, r5, r6, r7, smask;
	__m128 signmask = _mm_castsi128_ps(_mm_set1_epi32(0x80000000));
	__m128 absmask = _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff));
	__m128 fltmin = _mm_set1_ps(FLT_MIN);
	size <<= 1;
	while (j <= (size - 8)) {
		r5 = _mm_loadu_ps(r+j);
		r4 = _mm_loadu_ps(r+j+4);
		r0 = _mm_shuffle_ps(r5, r4, _MM_SHUFFLE(2,0,2,0));
		r1 = _mm_shuffle_ps(r5, r4, _MM_SHUFFLE(3,1,3,1));
		r2 = _mm_and_ps(r1 , absmask);
		r2 = _mm_max_ps(r2 , fltmin);
		smask = _mm_cmplt_ps(r0 , _mm_setzero_ps());
		smask = _mm_and_ps(smask , signmask);
		r0 = _mm_xor_ps(r0 , smask);
		r4 = _mm_sub_ps(r0 , r2);
		r3 = _mm_add_ps(r0 , r2);
		r4 = _mm_xor_ps(r4 , smask);
		r0 = _mm_rcp_ps(r3);
		r3 = _mm_mul_ps(r3 , r0);
		r5 = _mm_add_ps(r0 , r0);
		r3 = _mm_mul_ps(r3 , r0);
		r5 = _mm_sub_ps(r5 , r3);
		r0 = _mm_mul_ps(r5 , r4);
		r2 = _mm_set1_ps(0.5f*M_PI_FLOAT);
		r4 = _mm_set1_ps(0.25f*M_PI_FLOAT);
		r4 = _mm_xor_ps(r4 , smask);
		r2 = _mm_sub_ps(r2 , r4);
		smask = _mm_cmplt_ps(r1 , _mm_setzero_ps());
		smask = _mm_and_ps(smask , signmask);
		r1 = _mm_mul_ps(r0 , r0);
		r0 = _mm_xor_ps(r0 , smask);
		r2 = _mm_xor_ps(r2 , smask);
		r3 = _mm_mul_ps(r1 , _mm_set1_ps(-0.00501112f));
		r4 = _mm_mul_ps(r1 , _mm_set1_ps(-0.14047120f));
		r5 = _mm_mul_ps(r1 , r1);
		r6 = _mm_mul_ps(r1 , _mm_set1_ps(-0.06083627f));
		r7 = _mm_mul_ps(r1 , _mm_set1_ps(-0.33332227f));
		r3 = _mm_add_ps(r3 , _mm_set1_ps(0.02530048f));
		r4 = _mm_add_ps(r4 , _mm_set1_ps(0.19973928f));
		r1 = _mm_mul_ps(r5 , r5);
		r3 = _mm_mul_ps(r3 , r5);
		r6 = _mm_add_ps(r6 , _mm_set1_ps(0.09999908f));
		r4 = _mm_mul_ps(r4 , r5);
		r7 = _mm_add_ps(r7 , _mm_set1_ps(0.99999992f));
		r6 = _mm_mul_ps(r6 , r1);
		r3 = _mm_mul_ps(r3 , r1);
		r4 = _mm_add_ps(r4 , r7);
		r6 = _mm_add_ps(r6 , r4);
		r6 = _mm_add_ps(r6 , r3);
		r6 = _mm_mul_ps(r6 , r0);
		r6 = _mm_sub_ps(r2 , r6);
		_mm_storeu_ps(d+i , r6);
		i+=4; j+=8;
	}
	while (j < size) {d[i] = atan2f(r[j+1],r[j]); j+=2; i++;}
#endif	
}

// return complex dot product: <d,r*>
Complex VectorFunctions::cpxdotp(float* d, float* r, int size)
{
	int i=0; Complex res;
#ifdef ICSTLIB_NO_SSEOPT  
	int rm = 2*(size - ((size>>2)<<2));
	float rsum=0,isum=0; double x,y;
	for (i=0; i<rm; i+=2) {
		rsum += (d[i]*r[i] + d[i+1]*r[i+1]); isum += (d[i+1]*r[i] - d[i]*r[i+1]);
	}
	x = static_cast<double>(rsum); y = static_cast<double>(isum); 
	for (i=rm; i<(2*size); i+=8) {
		rsum = isum = 0;
		rsum += (d[i]*r[i] + d[i+1]*r[i+1]);
		isum += (d[i+1]*r[i] - d[i]*r[i+1]);
		rsum += (d[i+2]*r[i+2] + d[i+3]*r[i+3]);
		isum += (d[i+3]*r[i+2] - d[i+2]*r[i+3]);
		rsum += (d[i+4]*r[i+4] + d[i+5]*r[i+5]);
		isum += (d[i+5]*r[i+4] - d[i+4]*r[i+5]);
		rsum += (d[i+6]*r[i+6] + d[i+7]*r[i+7]);
		isum += (d[i+7]*r[i+6] - d[i+6]*r[i+7]);
		x += static_cast<double>(rsum); y += static_cast<double>(isum); 
	}
	res.re = static_cast<float>(x); res.im = static_cast<float>(y);
	return res;		
#else
	__m128 r0,r1,r2,r3,r4,r5,r6,r7,mask1,mask2;
	__m128d acc,dtmp; 
	mask1 = _mm_castsi128_ps(_mm_set_epi32(0,0xffffffff,0,0xffffffff));
	mask2 = _mm_castsi128_ps(_mm_set_epi32(0xffffffff,0,0xffffffff,0));
	acc = _mm_setzero_pd();
	size <<= 1;
	if (!((reinterpret_cast<uintptr_t>(d) | reinterpret_cast<uintptr_t>(r)) & 0xF)) {
		while (i <= (size - 16)) {
			r0 = _mm_load_ps(d+i);
			r1 = _mm_shuffle_ps(r0, r0, _MM_SHUFFLE(2,3,0,1));
			r2 = _mm_load_ps(r+i);
			r6 = _mm_mul_ps(r0 , r2);
			r7 = _mm_mul_ps(r1 , r2);
			r3 = _mm_load_ps(d+i+4);
			r4 = _mm_shuffle_ps(r3, r3, _MM_SHUFFLE(2,3,0,1));
			r5 = _mm_load_ps(r+i+4);
			r3 = _mm_mul_ps(r3 , r5);
			r4 = _mm_mul_ps(r4 , r5);
			r6 = _mm_add_ps(r3 , r6);
			r7 = _mm_add_ps(r4 , r7);
			r0 = _mm_load_ps(d+i+8);
			r1 = _mm_shuffle_ps(r0, r0, _MM_SHUFFLE(2,3,0,1));
			r2 = _mm_load_ps(r+i+8);
			r0 = _mm_mul_ps(r0 , r2);
			r1 = _mm_mul_ps(r1 , r2);
			r6 = _mm_add_ps(r0 , r6);
			r7 = _mm_add_ps(r1 , r7);
			r3 = _mm_load_ps(d+i+12);
			r4 = _mm_shuffle_ps(r3, r3, _MM_SHUFFLE(2,3,0,1));
			r5 = _mm_load_ps(r+i+12);
			r3 = _mm_mul_ps(r3 , r5);
			r4 = _mm_mul_ps(r4 , r5);
			r6 = _mm_add_ps(r3 , r6);
			r7 = _mm_add_ps(r4 , r7);
			r2 = _mm_castsi128_ps(_mm_srli_epi64(_mm_castps_si128(r6) , 32));
			r3 = _mm_castsi128_ps(_mm_slli_epi64(_mm_castps_si128(r7) , 32));
			r6 = _mm_and_ps(r6 , mask1);
			r7 = _mm_and_ps(r7 , mask2);
			r6 = _mm_add_ps(r6 , r2); 
			r7 = _mm_sub_ps(r3 , r7);
			r6 = _mm_or_ps(r6 , r7);
			r7 = _mm_movehl_ps(r6 , r6);
			r7 = _mm_add_ps(r7 , r6);
			dtmp = _mm_cvtps_pd(r7);
			acc = _mm_add_pd(acc, dtmp); 
			i+=16;
		}
	}
	else {
		while (i <= (size - 16)) {
			r0 = _mm_loadu_ps(d+i);
			r1 = _mm_shuffle_ps(r0, r0, _MM_SHUFFLE(2,3,0,1));
			r2 = _mm_loadu_ps(r+i);
			r6 = _mm_mul_ps(r0 , r2);
			r7 = _mm_mul_ps(r1 , r2);
			r3 = _mm_loadu_ps(d+i+4);
			r4 = _mm_shuffle_ps(r3, r3, _MM_SHUFFLE(2,3,0,1));
			r5 = _mm_loadu_ps(r+i+4);
			r3 = _mm_mul_ps(r3 , r5);
			r4 = _mm_mul_ps(r4 , r5);
			r6 = _mm_add_ps(r3 , r6);
			r7 = _mm_add_ps(r4 , r7);
			r0 = _mm_loadu_ps(d+i+8);
			r1 = _mm_shuffle_ps(r0, r0, _MM_SHUFFLE(2,3,0,1));
			r2 = _mm_loadu_ps(r+i+8);
			r0 = _mm_mul_ps(r0 , r2);
			r1 = _mm_mul_ps(r1 , r2);
			r6 = _mm_add_ps(r0 , r6);
			r7 = _mm_add_ps(r1 , r7);
			r3 = _mm_loadu_ps(d+i+12);
			r4 = _mm_shuffle_ps(r3, r3, _MM_SHUFFLE(2,3,0,1));
			r5 = _mm_loadu_ps(r+i+12);
			r3 = _mm_mul_ps(r3 , r5);
			r4 = _mm_mul_ps(r4 , r5);
			r6 = _mm_add_ps(r3 , r6);
			r7 = _mm_add_ps(r4 , r7);
			r2 = _mm_castsi128_ps(_mm_srli_epi64(_mm_castps_si128(r6) , 32));
			r3 = _mm_castsi128_ps(_mm_slli_epi64(_mm_castps_si128(r7) , 32));
			r6 = _mm_and_ps(r6 , mask1);
			r7 = _mm_and_ps(r7 , mask2);
			r6 = _mm_add_ps(r6 , r2); 
			r7 = _mm_sub_ps(r3 , r7);
			r6 = _mm_or_ps(r6 , r7);
			r7 = _mm_movehl_ps(r6 , r6);
			r7 = _mm_add_ps(r7 , r6);
			dtmp = _mm_cvtps_pd(r7);
			acc = _mm_add_pd(acc, dtmp); 
			i+=16;
		}
	}
	if (size & 8) {
		r0 = _mm_loadu_ps(d+i);
		r1 = _mm_shuffle_ps(r0, r0, _MM_SHUFFLE(2,3,0,1));
		r2 = _mm_loadu_ps(r+i);
		r6 = _mm_mul_ps(r0 , r2);
		r7 = _mm_mul_ps(r1 , r2);
		r3 = _mm_loadu_ps(d+i+4);
		r4 = _mm_shuffle_ps(r3, r3, _MM_SHUFFLE(2,3,0,1));
		r5 = _mm_loadu_ps(r+i+4);
		r3 = _mm_mul_ps(r3 , r5);
		r4 = _mm_mul_ps(r4 , r5);
		r6 = _mm_add_ps(r3 , r6);
		r7 = _mm_add_ps(r4 , r7);
		r2 = _mm_castsi128_ps(_mm_srli_epi64(_mm_castps_si128(r6) , 32));
		r3 = _mm_castsi128_ps(_mm_slli_epi64(_mm_castps_si128(r7) , 32));
		r6 = _mm_and_ps(r6 , mask1);
		r7 = _mm_and_ps(r7 , mask2);
		r6 = _mm_add_ps(r6 , r2); 
		r7 = _mm_sub_ps(r3 , r7);
		r6 = _mm_or_ps(r6 , r7);
		r7 = _mm_movehl_ps(r6 , r6);
		r7 = _mm_add_ps(r7 , r6);
		dtmp = _mm_cvtps_pd(r7);
		acc = _mm_add_pd(acc, dtmp); 
		i+=8;
	}
	r0 = _mm_cvtpd_ps(acc);
	_mm_store_ss(&res.re , r0);
	r0 = _mm_castsi128_ps(_mm_srli_epi64(_mm_castps_si128(r0) , 32));
	_mm_store_ss(&res.im , r0);
	while (i < size) {
		res.re += (d[i]*r[i] + d[i+1]*r[i+1]);
		res.im += (d[i+1]*r[i] - d[i]*r[i+1]);
		i+=2;
	}
	return res;
#endif
}

// Re(d) -> re
void VectorFunctions::cpxre(float* re, float* d, int size) {deinterleave(re,d,size,2,0);} 

// Im(d) -> im
void VectorFunctions::cpxim(float* im, float* d, int size) {deinterleave(im,d,size,2,1);}		

// re -> Re(d), im -> Im(d)
// if re=NULL: 0 -> Re(d), if im=NULL: 0 -> Im(d)   											
void VectorFunctions::realtocpx(float* d, float* re,	float* im, int size)
{
	int i,j = 2*(size-1);
	if (!im) {for (i=size-1; i>=0; i--,j-=2) {d[j]=re[i]; d[j+1]=0;}}
	else if (!re) {for (i=size-1; i>=0; i--,j-=2) {d[j]=0; d[j+1]=im[i];}}
	else {for (i=size-1; i>=0; i--,j-=2) {d[j]=re[i]; d[j+1]=im[i];}}
}

// polar (mag,arg) to cartesian d
// relative error < 1.1e-6 for |arg| < 2pi
void VectorFunctions::cpxptc(float* d, float* mag, float* arg, int size)
{
	int i=0,j=0;
#ifdef ICSTLIB_NO_SSEOPT
	for (i=0; i<size; i++,j+=2) {
		d[j] = mag[i]*cosf(arg[i]);
		d[j+1] = mag[i]*sinf(arg[i]);
	}
#else
	__m128 r0,r1,r2,r3,r4, offset = _mm_set1_ps(-0.5f*M_PI_FLOAT);	
	if (!(reinterpret_cast<uintptr_t>(d) & 0xF)) {	
		while (i <= (size - 4)) {
			r0 = _mm_loadu_ps(arg+i);
			r1 = _mm_add_ps(r0 , offset);
			r2 = _mm_unpacklo_ps(r0 , r1);
			r3 = _mm_unpackhi_ps(r0 , r1);
			_mm_store_ps(d+j , r2);
			_mm_store_ps(d+j+4 , r3);
			j+=8;
			i+=4;
		}
		while (i < size) {d[j] = arg[i]; d[j+1] = arg[i] - 0.5f*M_PI_FLOAT; j+=2; i++;}
		fcos(d, 2*size);
		i=j=0;
		while (i <= (size - 4)) {
			r0 = _mm_load_ps(d+j);
			r1 = _mm_load_ps(d+j+4);
			r2 = _mm_loadu_ps(mag+i);
			r3 = _mm_unpacklo_ps(r2 , r2);
			r4 = _mm_unpackhi_ps(r2 , r2);
			r0 = _mm_mul_ps(r0 , r3);
			r1 = _mm_mul_ps(r1 , r4);
			_mm_store_ps(d+j , r0);
			_mm_store_ps(d+j+4 , r1);
			j+=8;
			i+=4;
		}
	}
	else {
		while (i <= (size - 4)) {
			r0 = _mm_loadu_ps(arg+i);
			r1 = _mm_add_ps(r0 , offset);
			r2 = _mm_unpacklo_ps(r0 , r1);
			r3 = _mm_unpackhi_ps(r0 , r1);
			_mm_storeu_ps(d+j , r2);
			_mm_storeu_ps(d+j+4 , r3);
			j+=8;
			i+=4;
		}
		while (i < size) {d[j] = arg[i]; d[j+1] = arg[i] - 0.5f*M_PI_FLOAT; j+=2; i++;}
		fcos(d, 2*size);
		i=j=0;
		while (i <= (size - 4)) {
			r0 = _mm_loadu_ps(d+j);
			r1 = _mm_loadu_ps(d+j+4);
			r2 = _mm_loadu_ps(mag+i);
			r3 = _mm_unpacklo_ps(r2 , r2);
			r4 = _mm_unpackhi_ps(r2 , r2);
			r0 = _mm_mul_ps(r0 , r3);
			r1 = _mm_mul_ps(r1 , r4);
			_mm_storeu_ps(d+j , r0);
			_mm_storeu_ps(d+j+4 , r1);
			j+=8;
			i+=4;
		}
	}
	while (i < size) {d[j] *= mag[i]; d[j+1] *= mag[i]; j+=2; i++;}
#endif
}

// arg{d} and magnitude mag -> d
void VectorFunctions::cpxcombine(float* d, float* mag, int size)
{
	int i=0,j=0; float norm;
#ifdef ICSTLIB_NO_SSEOPT  
	for (i=0; i<size; i++,j+=2) {
		norm = mag[i]/sqrtf(__max(d[j]*d[j] + d[j+1]*d[j+1], FLT_MIN));
		d[j]*=norm; d[j+1]*=norm;
	}
#else
	__m128 r0,r1,r2,r3,r4,r5,cmul3,cdiv2,mask1,mask2,fltmin;
	mask1 = _mm_castsi128_ps(_mm_set_epi32(0,0xffffffff,0,0xffffffff));
	mask2 = _mm_castsi128_ps(_mm_set_epi32(0xffffffff,0,0xffffffff,0));
	cmul3 = _mm_set1_ps(3.0f);
	cdiv2 = _mm_set1_ps(0.5f);
	fltmin = _mm_set1_ps(FLT_MIN);
	size <<= 1;
	if (!((reinterpret_cast<uintptr_t>(d) | reinterpret_cast<uintptr_t>(mag)) & 0xF)) {
		while (j <= (size - 8)) {
			r0 = _mm_load_ps(d+j);
			r2 = _mm_mul_ps(r0 , r0);
			r1 = _mm_load_ps(d+j+4);
			r3 = _mm_mul_ps(r1 , r1);
			r4 = _mm_castsi128_ps(_mm_srli_epi64(_mm_castps_si128(r2) , 32));
			r2 = _mm_and_ps(r2 , mask1);
			r5 = _mm_castsi128_ps(_mm_slli_epi64(_mm_castps_si128(r3) , 32));
			r3 = _mm_and_ps(r3 , mask2);
			r2 = _mm_or_ps(r2 , r3);
			r4 = _mm_or_ps(r4 , r5);
			r2 = _mm_add_ps(r4 , r2);
			r5 = _mm_load_ps(mag+i);
			r2 = _mm_max_ps(r2 , fltmin);
			r5 = _mm_mul_ps(r5 , cdiv2);
			r3 = _mm_rsqrt_ps(r2);
			r2 = _mm_mul_ps(r2 , r3);
			r5 = _mm_shuffle_ps(r5 , r5 , _MM_SHUFFLE(3,1,2,0));
			r4 = _mm_mul_ps(r5 , r3);
			r2 = _mm_mul_ps(r2 , r3);
			r2 = _mm_sub_ps(cmul3 , r2);
			r4 = _mm_mul_ps(r2 , r4);
			r2 = _mm_shuffle_ps(r4 , r4 , _MM_SHUFFLE(2,2,0,0));
			r3 = _mm_shuffle_ps(r4 , r4 , _MM_SHUFFLE(3,3,1,1));
			r2 = _mm_mul_ps(r0 , r2);
			r3 = _mm_mul_ps(r1 , r3);
			_mm_store_ps(d+j , r2);
			_mm_store_ps(d+j+4 , r3);
			i+=4;
			j+=8;
		}
	}
	else {
		while (j <= (size - 8)) {
			r0 = _mm_loadu_ps(d+j);
			r2 = _mm_mul_ps(r0 , r0);
			r1 = _mm_loadu_ps(d+j+4);
			r3 = _mm_mul_ps(r1 , r1);
			r4 = _mm_castsi128_ps(_mm_srli_epi64(_mm_castps_si128(r2) , 32));
			r2 = _mm_and_ps(r2 , mask1);
			r5 = _mm_castsi128_ps(_mm_slli_epi64(_mm_castps_si128(r3) , 32));
			r3 = _mm_and_ps(r3 , mask2);
			r2 = _mm_or_ps(r2 , r3);
			r4 = _mm_or_ps(r4 , r5);
			r2 = _mm_add_ps(r4 , r2);
			r5 = _mm_loadu_ps(mag+i);
			r2 = _mm_max_ps(r2 , fltmin);
			r5 = _mm_mul_ps(r5 , cdiv2);
			r3 = _mm_rsqrt_ps(r2);
			r2 = _mm_mul_ps(r2 , r3);
			r5 = _mm_shuffle_ps(r5 , r5 , _MM_SHUFFLE(3,1,2,0));
			r4 = _mm_mul_ps(r5 , r3);
			r2 = _mm_mul_ps(r2 , r3);
			r2 = _mm_sub_ps(cmul3 , r2);
			r4 = _mm_mul_ps(r2 , r4);
			r2 = _mm_shuffle_ps(r4 , r4 , _MM_SHUFFLE(2,2,0,0));
			r3 = _mm_shuffle_ps(r4 , r4 , _MM_SHUFFLE(3,3,1,1));
			r2 = _mm_mul_ps(r0 , r2);
			r3 = _mm_mul_ps(r1 , r3);
			_mm_storeu_ps(d+j , r2);
			_mm_storeu_ps(d+j+4 , r3);
			i+=4;
			j+=8;
		}
	}
	while (j < size) {
		norm = mag[i]/sqrtf(__max(d[j]*d[j] + d[j+1]*d[j+1], FLT_MIN));
		d[j]*=norm; d[j+1]*=norm;
		i++; j+=2;
	}
#endif	
}

//******************************************************************************
//* special array operations
//*
// copy float to double
void VectorFunctions::ftod(double* d, float* r, int size) {
	for (int i=0; i<size; i++) {d[i] = static_cast<double>(r[i]);}}

// copy double to float
void VectorFunctions::dtof(float* d, double* r, int size) {
	for (int i=0; i<size; i++) {d[i] = static_cast<float>(r[i]);}}	

// copy and clip float to int
void VectorFunctions::ftoi(int* d, float* r, int size)
{
#ifdef ICSTLIB_DEF_ROUND
	#ifdef ICSTLIB_NO_SSEOPT
		union {volatile double x; int y[2];};
		for (int i=0; i<size; i++) {
			if (r[i] > FLT_INTMAX) {d[i] = INT_MAX;}
			else if (r[i] < FLT_INTMIN) {d[i] = INT_MIN;}
			else {
				x = static_cast<double>(r[i]);
				x += 4503603922337792.0;
				d[i] = y[0];
			}
		}
	#else
		int i=0;
		__m128 r0,r2, maxval = _mm_set1_ps(FLT_INTMAX), minval = _mm_set1_ps(FLT_INTMIN);
		__m128 mask = _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff));
		__m128i r1;
		if (!((reinterpret_cast<uintptr_t>(d) | reinterpret_cast<uintptr_t>(r)) & 0xF)) {
			while (i <= (size - 4)) {
				r0 = _mm_load_ps(r+i);
				r2 = _mm_cmpgt_ps(r0 , maxval);
				r0 = _mm_min_ps(r0 , maxval);	// only required if exception unmasked
				r0 = _mm_max_ps(r0 , minval);	//
				r1 = _mm_cvtps_epi32(r0);
				r2 = _mm_and_ps(r2 , mask);
				r1 = _mm_or_si128(r1 , _mm_castps_si128(r2));
				_mm_store_si128(reinterpret_cast<__m128i*>(d+i) , r1);
				i+=4;
			}
		}
		else {
			while (i <= (size - 4)) {
				r0 = _mm_loadu_ps(r+i);
				r2 = _mm_cmpgt_ps(r0 , maxval);
				r0 = _mm_min_ps(r0 , maxval);
				r0 = _mm_max_ps(r0 , minval);
				r1 = _mm_cvtps_epi32(r0);
				r2 = _mm_and_ps(r2 , mask);
				r1 = _mm_or_si128(r1 , _mm_castps_si128(r2));
				_mm_storeu_si128(reinterpret_cast<__m128i*>(d+i) , r1);
				i+=4;
			}
		}
		while (i < size) {
			if (r[i] > FLT_INTMAX) {d[i] = INT_MAX;}
			else {
				r0 = _mm_load_ss(r+i);
				r0 = _mm_min_ss(r0 , maxval);
				r0 = _mm_max_ss(r0 , minval);
				d[i] = _mm_cvtss_si32(r0);
			}
			i++;
		}
	#endif
#else
	#ifdef ICSTLIB_NO_SSEOPT
		for (int i=0; i<size; i++) {
			if (r[i] > FLT_INTMAX) {d[i] = INT_MAX;}
			else if (r[i] < FLT_INTMIN) {d[i] = INT_MIN;}
			else {
				d[i] = ((r[i] >= 0) ?	static_cast<int>(r[i] + 0.5f) :
										static_cast<int>(r[i] - 0.5f));
			}
		}
	#else
		int i=0;
		__m128 r0,r2,r3;
		__m128 maxval = _mm_set1_ps(FLT_INTMAX), minval = _mm_set1_ps(FLT_INTMIN);
		__m128 maxmask = _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff));
		__m128 sigmask = _mm_castsi128_ps(_mm_set1_epi32(0x80000000));
		__m128i r1;
		if (!((reinterpret_cast<uintptr_t>(d) | reinterpret_cast<uintptr_t>(r)) & 0xF)) {
			while (i <= (size - 4)) {
				r0 = _mm_load_ps(r+i);
				r3 = _mm_and_ps(r0 , sigmask);
				r3 = _mm_xor_ps(r3 , _mm_set1_ps(0.5f));
				r2 = _mm_cmpgt_ps(r0 , maxval);
				r0 = _mm_min_ps(r0 , maxval);	// only required if exception unmasked
				r0 = _mm_max_ps(r0 , minval);	//
				r0 = _mm_add_ps(r0 , r3);
				r1 = _mm_cvttps_epi32(r0);
				r2 = _mm_and_ps(r2 , maxmask);
				r1 = _mm_or_si128(r1 , _mm_castps_si128(r2));
				_mm_store_si128(reinterpret_cast<__m128i*>(d+i) , r1);
				i+=4;
			}
		}
		else {
			while (i <= (size - 4)) {
				r0 = _mm_loadu_ps(r+i);
				r3 = _mm_and_ps(r0 , sigmask);
				r3 = _mm_xor_ps(r3 , _mm_set1_ps(0.5f));
				r2 = _mm_cmpgt_ps(r0 , maxval);
				r0 = _mm_min_ps(r0 , maxval);
				r0 = _mm_max_ps(r0 , minval);
				r0 = _mm_add_ps(r0 , r3);
				r1 = _mm_cvttps_epi32(r0);
				r2 = _mm_and_ps(r2 , maxmask);
				r1 = _mm_or_si128(r1 , _mm_castps_si128(r2));
				_mm_storeu_si128(reinterpret_cast<__m128i*>(d+i) , r1);
				i+=4;
			}
		}
		while (i < size) {
			if (r[i] > FLT_INTMAX) {d[i] = INT_MAX;}
			else {
				r0 = _mm_load_ss(r+i);
				r3 = _mm_and_ps(r0 , sigmask);
				r3 = _mm_xor_ps(r3 , _mm_set_ss(0.5f));
				r0 = _mm_min_ss(r0 , maxval);
				r0 = _mm_max_ss(r0 , minval);
				r0 = _mm_add_ss(r0 , r3);
				d[i] = _mm_cvttss_si32(r0);
			}
			i++;
		}
	#endif
#endif
}

// copy int to float
void VectorFunctions::itof(float* d, int* r, int size)
{
#ifdef ICSTLIB_NO_SSEOPT 
	for (int i=0; i<size; i++) {d[i] = static_cast<float>(r[i]);}
#else
	int i=0; __m128 r0 = _mm_setzero_ps(); __m128i r1;
	if (!((reinterpret_cast<uintptr_t>(d) | reinterpret_cast<uintptr_t>(r)) & 0xF)) {
		while (i <= (size - 4)) {
			r1 = _mm_load_si128(reinterpret_cast<__m128i*>(r+i));
			r0 = _mm_cvtepi32_ps(r1);
			_mm_store_ps(d+i , r0);
			i+=4;
		}
	}
	else {
		while (i <= (size - 4)) {
			r1 = _mm_loadu_si128(reinterpret_cast<__m128i*>(r+i));
			r0 = _mm_cvtepi32_ps(r1);
			_mm_storeu_ps(d+i , r0);
			i+=4;
		}
	}
	while (i < size) {
		r0 = _mm_cvtsi32_ss(r0 , r[i]);
		_mm_store_ss(d+i , r0);
		i++;
	}
#endif
}

// interleave r to d, skipped d elements remain unchanged
void VectorFunctions::interleave(float* d,float* r,int rsize,int interval,int offset) 
{
	int i, j=offset, rm = rsize - ((rsize>>2)<<2);
	for (i=0; i<rm; i++) {d[j]=r[i]; j+=interval;}
	for (i=rm; i<rsize; i+=4) {
		d[j]=r[i]; j+=interval;		d[j]=r[i+1]; j+=interval;
		d[j]=r[i+2]; j+=interval;	d[j]=r[i+3]; j+=interval;
	}
}	
	 
// deinterleave r to d
void VectorFunctions::deinterleave(float* d,float* r,int dsize,int interval,int offset)
{
	int i, j=offset, rm = dsize - ((dsize>>2)<<2); 
	for (i=0; i<rm; i++) {d[i]=r[j]; j+=interval;}
	for (i=rm; i<dsize; i+=4) {
		d[i]=r[j]; j+=interval;		d[i+1]=r[j]; j+=interval;
		d[i+2]=r[j]; j+=interval;	d[i+3]=r[j]; j+=interval;
	}
}

// map d = rlo..rhi linearly to d = dlo..dhi
void VectorFunctions::maplin(float* d, int size, float dhi, float dlo,			
					float rhi, float rlo)
{
	int i=0;
	float offset = 0.5f*(dhi + dlo);
	float scale = rhi - rlo;
	if (fabsf(scale) >= FLT_MIN) {
		scale = (dhi - dlo)/scale;
		offset -= 0.5f*scale*(rhi + rlo);
	}
#ifndef ICSTLIB_NO_SSEOPT  
	if (reinterpret_cast<uintptr_t>(d) & 0xF) {
#endif	
		for (i=0; i<size; i++) {d[i] = offset + scale*d[i];}
#ifndef ICSTLIB_NO_SSEOPT
	}
	else {
		__m128 r0;
		__m128 r1 = _mm_set1_ps(offset);
		__m128 r2 = _mm_set1_ps(scale);
		while (i <= (size - 4)) {
			r0 = _mm_load_ps(d+i);
			r0 = _mm_mul_ps(r0 , r2);
			r0 = _mm_add_ps(r0 , r1);
			_mm_store_ps(d+i , r0);
			i+=4;
		}
		while (i < size) {d[i] = offset + scale*d[i]; i++;}
	}
#endif
}

// linear fade from d to r with linear mixing -> d
void VectorFunctions::xfade(float* d, float* r, int size)
{
	if (size == 1) {d[0] = 0.5f*(r[0] + d[0]); return;}
	double x=0, delta = 1.0/static_cast<double>(size-1);
	for (int i=0; i<size; i++) {
		d[i] += static_cast<float>(x)*(r[i] - d[i]);
		x += delta;
	}
}

// arbitrary fade with linear mixing: (1-w)*d + w*r -> d	
void VectorFunctions::xfade(float* d, float* r, float* w, int size) {
	for (int i=0; i<size; i++) {d[i] += w[i]*(r[i] - d[i]);}}

// power fade version of xfade
// d*cos(x) + r*sin(x) -> d	with x = 0..pi/2
void VectorFunctions::pxfade(float* d, float* r, int size)
{
	if (size == 1) {d[0] = 0.70710678f*(r[0] + d[0]); return;}
	int i, rm = size - ((size>>2)<<2);
	double temp = 0.5*M_PI/static_cast<double>(size-1);
	double wre = cos(temp), wim = sin(temp);
	double re=1.0, im=0, re2;
	for (i=0; i<rm; i++) {
		d[i] = d[i]*static_cast<float>(re) + r[i]*static_cast<float>(im);
		temp = re; re = wre*re - wim*im; im = wre*im + wim*temp;
	}
	for (i=rm; i<size; i+=4) {
		d[i] = d[i]*static_cast<float>(re) + r[i]*static_cast<float>(im);
		re2 = wre*re - wim*im; im = wre*im + wim*re;
		d[i+1] = d[i+1]*static_cast<float>(re2) + r[i+1]*static_cast<float>(im);
		re = wre*re2 - wim*im; im = wre*im + wim*re2;
		d[i+2] = d[i+2]*static_cast<float>(re) + r[i+2]*static_cast<float>(im);
		re2 = wre*re - wim*im; im = wre*im + wim*re;
		d[i+3] = d[i+3]*static_cast<float>(re2) + r[i+3]*static_cast<float>(im);
		re = wre*re2 - wim*im; im = wre*im + wim*re2;
	}
}

// evaluate power series: sum(i=0..order){c[i]*d^i} -> d
void VectorFunctions::polyval(float* d, int dsize, float* c,	int order)
{
#ifdef ICSTLIB_NO_SSEOPT 
	int i,j; float x;	
	for (i=0; i<dsize; i++) {
		x = c[order];
		for (j=order-1; j>=0; j--) {x = d[i]*x + c[j];}
		d[i] = x;
	}
#else
	__m128 r0,r1,r2,r3,r4;
	__m128 inbuf[4];
	int i=0, j; float x;
	while (i <= (dsize - 16)) {
		inbuf[0] = _mm_loadu_ps(d+i);
		inbuf[1] = _mm_loadu_ps(d+i+4);
		inbuf[2] = _mm_loadu_ps(d+i+8);
		inbuf[3] = _mm_loadu_ps(d+i+12);
		r0 = r1 = r2 = r3 = _mm_load1_ps(c+order);
		for (j=order-1; j>=0; j--) {
			r4 = _mm_load1_ps(c+j);
			r0 = _mm_mul_ps(r0 , inbuf[0]);
			r1 = _mm_mul_ps(r1 , inbuf[1]);
			r2 = _mm_mul_ps(r2 , inbuf[2]);
			r3 = _mm_mul_ps(r3 , inbuf[3]);
			r0 = _mm_add_ps(r0 , r4);
			r1 = _mm_add_ps(r1 , r4);
			r2 = _mm_add_ps(r2 , r4);
			r3 = _mm_add_ps(r3 , r4);
		}
		_mm_storeu_ps(d+i , r0);
		_mm_storeu_ps(d+i+4 , r1);
		_mm_storeu_ps(d+i+8 , r2);
		_mm_storeu_ps(d+i+12 , r3);
		i+=16;
	}
	if (dsize & 8) {
		r2 = _mm_loadu_ps(d+i);
		r3 = _mm_loadu_ps(d+i+4);
		r0 = r1 = _mm_load1_ps(c+order);
		for (j=order-1; j>=0; j--) {
			r4 = _mm_load1_ps(c+j);
			r0 = _mm_mul_ps(r0 , r2);
			r1 = _mm_mul_ps(r1 , r3);
			r0 = _mm_add_ps(r0 , r4);
			r1 = _mm_add_ps(r1 , r4);
		}
		_mm_storeu_ps(d+i , r0);
		_mm_storeu_ps(d+i+4 , r1);
		i+=8;
	}
	while (i < dsize) {
		x = c[order];
		for (j=order-1; j>=0; j--) {x = d[i]*x + c[j];}
		d[i] = x;
		i++;
	}
#endif
}	
void VectorFunctions::polyval(float* d, int dsize, double* c, int order)
{
	int i,j; double x,y;
	for (i=0; i<dsize; i++) {
		x=c[order]; y = static_cast<double>(d[i]);
		for (j=order-1; j>=0; j--) {x = y*x + c[j];}
		d[i] = static_cast<float>(x);
	}
}

// evaluate power series with complex argument d = {re(0),im(0),..,im(dsize-1)}
// sum(i=0..order){c[i]*d^i} -> d
void VectorFunctions::cpxpolyval(float* d, int dsize, float* c, int order)
{
	int i,j; float a,b,r,s,tmp;
	if (order < 1) {
		for (i=0; i<(2*dsize); i+=2) {d[i] = c[0]; d[i+1] = 0;}
		return;
	}
	for (i=0; i<(2*dsize); i+=2) {
		a = c[order]; b = c[order-1];
		r = 2.0f*d[i]; s = d[i]*d[i] + d[i+1]*d[i+1]; 
		for (j=order-2; j>=0; j--) {
			tmp = a;
			a = b + r*a;
			b = c[j] - s*tmp;
		}
		d[i] = a*d[i] + b;
		d[i+1] *= a;
	}
}
void VectorFunctions::cpxpolyval(float* d, int dsize, double* c, int order)
{
	int i,j; float tmp; double x,y,z,a,b,r,s;
	if (order < 1) {
		tmp = static_cast<float>(c[0]);
		for (i=0; i<(2*dsize); i+=2) {d[i] = tmp; d[i+1] = 0;}
		return;
	}
	for (i=0; i<(2*dsize); i+=2) {
		a = c[order]; b = c[order-1];
		x = static_cast<double>(d[i]); y = static_cast<double>(d[i+1]);
		r = 2.0*x; s = x*x + y*y; 
		for (j=order-2; j>=0; j--) {
			z = a;
			a = b + r*a;
			b = c[j] - s*z;
		}
		d[i] = static_cast<float>(a*x + b);
		d[i+1] = static_cast<float>(a*y);
	}
}

// evaluate chebyshev series: sum(i=0..order){c[i]*T[i](d)} -> d 
void VectorFunctions::chebyval(float* d, int dsize, float* c, int order)
{
#ifdef ICSTLIB_NO_SSEOPT 
	int i,j; float a,x,y,z;
	for (i=0; i<dsize; i++) {						
		x=0; y=0; a = 2.0f*d[i];
		for (j=order; j>=1; j--) {z=x; x = a*x + c[j] - y; y=z;}
		d[i] = 0.5f*a*x + c[0] - y;
	}
#else
	__m128 r1,r2,r3,r4,r5,r6,r7;
	__m128 inbuf[2];
	int i=0, j; float a,x,y,z;
	while (i <= (dsize - 8)) {
		r4 = _mm_loadu_ps(d+i);
		inbuf[0] = _mm_add_ps(r4 , r4);
		r7 = _mm_loadu_ps(d+i+4);
		inbuf[1] = _mm_add_ps(r7 , r7);
		r1 = _mm_setzero_ps();
		r2 = _mm_setzero_ps();
		r5 = _mm_setzero_ps();
		r6 = _mm_setzero_ps();
		for (j=order; j>0; j--) {
			r3 = r1;
			r7 = r5;
			r1 = _mm_mul_ps(r1 , inbuf[0]);
			r4 = _mm_load1_ps(c+j);
			r5 = _mm_mul_ps(r5 , inbuf[1]);
			r2 = _mm_sub_ps(r4 , r2);
			r6 = _mm_sub_ps(r4 , r6);
			r1 = _mm_add_ps(r1 , r2);
			r5 = _mm_add_ps(r5 , r6);
			r2 = r3;
			r6 = r7;
		}
		r4 = _mm_loadu_ps(d+i);
		r1 = _mm_mul_ps(r1 , r4);
		r7 = _mm_loadu_ps(d+i+4);
		r5 = _mm_mul_ps(r5 , r7);
		r4 = _mm_load1_ps(c);
		r2 = _mm_sub_ps(r4 , r2);
		r6 = _mm_sub_ps(r4 , r6);
		r4 = _mm_add_ps(r1 , r2);
		r6 = _mm_add_ps(r5 , r6);
		_mm_storeu_ps(d+i , r4);
		_mm_storeu_ps(d+i+4 , r6);
		i+=8;
	}
	while (i < dsize) {
		x=0; y=0; a = 2.0f*d[i];
		for(j=order; j>=1; j--) {z=x; x = a*x + c[j] - y; y=z;}
		d[i] = 0.5f*a*x + c[0] - y;
		i++;
	}
#endif
}
void VectorFunctions::chebyval(float* d, int dsize, double* c, int order)
{
	int i,j; double a,x,y,z;
	for (i=0; i<dsize; i++) {						
		x=0; y=0; a = 2.0*static_cast<double>(d[i]);
		for (j=order; j>=1; j--) {z=x; x = a*x + c[j] - y; y=z;}
		d[i] = static_cast<float>(0.5*a*x + c[0] - y);
	}
}

// unwrap phase data
void VectorFunctions::unwrap(float* d, int size)
{
	static const float TWOPI = 2.0f*M_PI_FLOAT;
	float tmp,phase,delta, offset=0, prevphase=d[0];
	for (int i=1; i<size; i++) {
		phase = d[i] + offset;
		delta = phase - prevphase;
		tmp = fabsf(delta);
		if (tmp < 10.0f*TWOPI) {
			if (delta > M_PI_FLOAT) {
				offset -= TWOPI; delta -= TWOPI;
				while (delta > M_PI_FLOAT) {offset -= TWOPI; delta -= TWOPI;}
			}
			else if (delta < -M_PI_FLOAT) {
				offset += TWOPI; delta += TWOPI;
				while (delta < -M_PI_FLOAT) {offset += TWOPI; delta += TWOPI;}
			}
		}
		else {
			tmp = TWOPI*floorf((tmp + M_PI_FLOAT)/TWOPI*(1.0f - FLT_EPSILON));
			if (delta > M_PI_FLOAT) {
				offset -= tmp; delta -= tmp;
				if (delta > M_PI_FLOAT) {offset -= TWOPI; delta -= TWOPI;}
			}
			if (delta < -M_PI_FLOAT) {
				offset += tmp; delta += tmp;
				if (delta < -M_PI_FLOAT) {offset += TWOPI; delta += TWOPI;}
			}
		}
		prevphase += delta; d[i] = prevphase;
	}
}			

// fill in indices of peaks of d, return peak count
int VectorFunctions::findpeaks(float* d, int* idx, int size)
{
	int i=1,icnt=0;
	while (i<(size-1)) {
		if ((d[i]>=d[i-1]) && (d[i]>d[i+1])) {idx[icnt]=i; icnt++; i+=2;}
		else {i++;}
	}
	return icnt;
}

// fill in indices of dips of d, return dip count
int VectorFunctions::finddips(float* d, int* idx, int size)
{
	int i=1,icnt=0;
	while (i<(size-1)) {
		if ((d[i-1]>d[i]) && (d[i+1]>=d[i])) {idx[icnt]=i; icnt++; i+=2;}
		else {i++;}
	}
	return icnt;
}

// create two subsets of d
// if sel[i] > 0:	{d[i] -> a[j], i -> aidx[j], j++} else {dito for b}
// return size of a
// output options:		a = NULL -> no a,b
//						aidx = NULL -> no aidx,bidx
// additional option:	b = NULL -> no b,bidx
int VectorFunctions::select(float* d, float* sel, float* a, float* b,
				   int* aidx, int* bidx, int size			)
{
	int i, j=0, k=0;
	if (b) {
		if ((a != NULL) && (aidx != NULL)) {
			for (i=0; i<size; i++) {
				if (sel[i] > 0) {a[j] = d[i]; aidx[j] = i; j++;}
				else {b[k] = d[i]; bidx[k] = i; k++;} 
			}
		}
		else {
			if (a) {
				for (i=0; i<size; i++) {
					if (sel[i] > 0) {a[j] = d[i]; j++;}
					else {b[k] = d[i]; k++;} 
				}
			}
			else {
				for (i=0; i<size; i++) {
					if (sel[i] > 0) {aidx[j] = i; j++;}
					else {bidx[k] = i; k++;} 
				}	
			}	
		}
	}
	else {
		if ((a != NULL) && (aidx != NULL)) {
			for (i=0; i<size; i++) {
				if (sel[i] > 0) {a[j] = d[i]; aidx[j] = i; j++;}
			}
		}
		else {
			if (a) {
				for (i=0; i<size; i++) {
					if (sel[i] > 0) {a[j] = d[i]; j++;}
				}
			}
			else {
				for (i=0; i<size; i++) {
					if (sel[i] > 0) {aidx[j] = i; j++;}
				}	
			}
		}
	}
	return j;
}

// return i of d[i] closest to but not greater than x
// !!! d must increase monotonically !!!
int VectorFunctions::mtabinv(float* d, float x, int size)
{
	int nhf, offset=0, n=size;
	while (n > 1) {
		nhf = n>>1;
		if (d[offset+nhf] <= x) {offset += nhf;}
		n -= nhf;
	}
	return offset;
}	

// fast linear interpolated cyclic table lookup -> d[0..dsize-1]
// t: table[0..2^ltsize], !!! t[0]=t[2^ltsize] !!! 
// start=table start index [0..2^ltsize), step=index increment
// return next start index
float VectorFunctions::linlookup(float* d, int dsize, float* t, int ltsize,
						float start, float step)
{
	unsigned int fracmask = 0xffffffff >> ltsize;
	float fracscl = (1.0f/4294967296.0f)*static_cast<float>(1 << ltsize);
	float fracspan = static_cast<float>(fracmask+1);
	unsigned int incpos = static_cast<unsigned int>(0.5f + step*fracspan); 
	unsigned int pos =  static_cast<unsigned int>(0.5f + start*fracspan);
	int idxsft = 32 - ltsize;
	int fracsft = ltsize - 9;
	unsigned int idx; int i;
	union {float frac; unsigned int temp;};
	if (fracsft < 0) {
		fracsft = -fracsft;
		for (i=0; i<dsize; i++) {
			idx = pos >> idxsft;
			temp = ((pos & fracmask) >> fracsft) | 0x3f800000;	// fast uint to float
			d[i] = t[idx] + (frac - 1.0f)*(t[idx+1] - t[idx]);
			pos += incpos;										// free wrap-around
		}
	}
	else {
		for (i=0; i<dsize; i++) {
			idx = pos >> idxsft;
			temp = ((pos & fracmask) << fracsft) | 0x3f800000;
			d[i] = t[idx] + (frac - 1.0f)*(t[idx+1] - t[idx]);
			pos += incpos;
		}
	}
	return fracscl*static_cast<float>(pos);
}

// quicksort d for descending order, fill idx with corresponding indices 
// about 3x faster than MSVC6 qsort function (which gives no index),
// higher performance attributed to elimination of the compare function call
// NOTE:	quicksort is fast for randomly ordered arrays but slow for already
//			well-ordered ones (C = nlog(n) bounded alternative: introsort)
void VectorFunctions::isort(float* d, int* idx, int size) 
{
	for (int i=0; i<size; i++) {idx[i]=i;}
	qs(d,idx,size-1,0);
}

//******************************************************************************
//* elementary real matrix operations
//*
//* matrix a(row,column): {a11,..,a1n,a21,..,a2n,..,am1,..,amn}
//* column vector: {r1,...,rn}
// return determinant of matrix a(n,n)
float VectorFunctions::mdet(float* a, int n)
{
	float* t; t = new float[n*n];
	copy(t,a,n*n);
	float det = lu(t,n);
	delete[] t;
	return det;
}			

// return trace of matrix a(n,n)
float VectorFunctions::mtrace(float* a, int n)
{
	float acc=0;
	for (int i=0; i<(n*n); i += (n+1)) {acc += a[i];}
	return acc;
}

// transpose matrix a(m,n)
void VectorFunctions::mxpose(float* a, int m, int n)
{
	int i,j,sw1, sw2=0; float tmp; float* t;
	if (n == m) {									// square matrix:
		sw1=1;										// fast, in-place
		for (i=0; i<(n-1); i++) {					
			sw2 = sw1 + n - 1;
			for (j=i+1; j<n; j++) {
				tmp = a[sw1];
				a[sw1] = a[sw2];
				a[sw2] = tmp;
				sw1++; sw2 += n;
			}
			sw1 += (i+2);
		}
	}
	else {
		t = new float[n*m];
		if (m > n) {								// m>n matrix
			for (i=0; i<n; i++) {
				sw1 = i;
				for (j=0; j<m; j++) {
					t[sw2] = a[sw1];
					sw1 += n; sw2++;
				}
			}
		}
		else {										// m<n matrix
			for (i=0; i<m; i++) {
				sw1 = i;
				for (j=0; j<n; j++) {
					t[sw1] = a[sw2];
					sw1 += m; sw2++;
				}
			}
		}
		copy(a,t,n*m);
		delete[] t;
	}
}

// get identity matrix a(n,n)
void VectorFunctions::mident(float* a, int n) {
	set(a,0,n*n); for (int i=0; i<(n*n); i += (n+1)) {a[i] = 1.0f;}}

// c*a(m,n) -> a(m,n)
void VectorFunctions::mmul(float* a, float c, int m, int n) {mul(a,c,m*n);}	

// matrix a(m,n) + b(m,n) -> a(m,n)
void VectorFunctions::madd(float* a, float* b, int m, int n) {add(a,b,m*n);}

// a(m,n) - b(m,n) -> a(m,n)
void VectorFunctions::msub(float* a, float* b, int m, int n) {sub(a,b,m*n);}

// matrix a(m,n) * vector r(n) -> vector d(m)
void VectorFunctions::mmulv(float* d, float* a, float* r, int m, int n)
{
#ifdef ICSTLIB_NO_SSEOPT
	int i=0, j;
	float acc0a,acc0b,acc0c,acc0d;
	while (i <= (m-4)) {
		acc0a = acc0b = acc0c = acc0d = 0; j = 0;
		while (j < n) {
			acc0a += a[j]*r[j];
			acc0b += a[j+n]*r[j];
			acc0c += a[j+2*n]*r[j];
			acc0d += a[j+3*n]*r[j];
			j++;
		}
		d[i] = acc0a; d[i+1] = acc0b; d[i+2] = acc0c; d[i+3] = acc0d;
		a += (4*n);
		i+=4;
	}
	if (m & 2) {
		acc0a = acc0b = 0; j = 0;
		while (j < n) {
			acc0a += a[j]*r[j];
			acc0b += a[j+n]*r[j];
			j++;
		}
		d[i] = acc0a; d[i+1] = acc0b;
		a += (2*n);
		i+=2;
	}
	if (m & 1) {
		acc0a = 0; j = 0;
		while (j < n) {
			acc0a += a[j]*r[j];
			j++;
		}
		d[i] = acc0a;
	}	
#else	
	int i=0, j, p, q=2*n;
	__m128 acc0,acc1,acc2,acc3,r0,r1,r2,r3,r4;
	if (!((reinterpret_cast<uintptr_t>(a) | reinterpret_cast<uintptr_t>(r) | (n<<2)) & 0xF)) {
		while (i <= (m-4)) {
			acc0 = _mm_setzero_ps();
			acc1 = _mm_setzero_ps();
			acc2 = _mm_setzero_ps();
			acc3 = _mm_setzero_ps();
			for (j=0,p=n; j<n; j+=4,p+=4) {
				r0 = _mm_load_ps(r+j);
				r1 = _mm_load_ps(a+j);
				r2 = _mm_load_ps(a+p); a+=q;
				r3 = _mm_load_ps(a+j);
				r4 = _mm_load_ps(a+p); a-=q;
				r1 = _mm_mul_ps(r1 , r0); 
				r2 = _mm_mul_ps(r2 , r0);
				r3 = _mm_mul_ps(r3 , r0);
				r4 = _mm_mul_ps(r4 , r0);
				acc0 = _mm_add_ps(acc0 , r1); 
				acc1 = _mm_add_ps(acc1 , r2);
				acc2 = _mm_add_ps(acc2 , r3);
				acc3 = _mm_add_ps(acc3 , r4);
			}
			a += (4*n);
			r0 = _mm_unpacklo_ps(acc0 , acc1);
			acc0 = _mm_unpackhi_ps(acc0 , acc1);
			r2 = _mm_unpacklo_ps(acc2 , acc3);
			acc2 = _mm_unpackhi_ps(acc2 , acc3);
			r0 = _mm_add_ps(r0 , acc0);
			r2 = _mm_add_ps(r2 , acc2);
			r1 = _mm_movelh_ps(r0 , r2);
			r3 = _mm_movehl_ps(r2 , r0);
			r3 = _mm_add_ps(r1 , r3);
			_mm_storeu_ps(d+i , r3);
			i+=4;		
		}
		if (m & 2) {
			acc0 = _mm_setzero_ps();
			acc1 = _mm_setzero_ps();
			for (j=0,p=n; j<n; j+=4,p+=4) {
				r0 = _mm_load_ps(r+j);
				r1 = _mm_load_ps(a+j);
				r2 = _mm_load_ps(a+p);
				r1 = _mm_mul_ps(r1 , r0); 
				r2 = _mm_mul_ps(r2 , r0);
				acc0 = _mm_add_ps(acc0 , r1); 
				acc1 = _mm_add_ps(acc1 , r2);
			}
			a += q;
			r0 = _mm_unpacklo_ps(acc0 , acc1);
			r1 = _mm_unpackhi_ps(acc0 , acc1);
			r0 = _mm_add_ps(r0 , r1);
			r1 = _mm_movehl_ps(r1 , r0);
			r0 = _mm_add_ps(r0 , r1);
			r1 = _mm_castsi128_ps(_mm_srli_epi64(_mm_castps_si128(r0) , 32));
			_mm_store_ss(d+i , r0);
			_mm_store_ss(d+i+1 , r1);
			i+=2;		
		}
		if (m & 1) {
			acc0 = _mm_setzero_ps();
			for (j=0; j<n; j+=4) {
				r0 = _mm_load_ps(r+j);
				r1 = _mm_load_ps(a+j);
				r1 = _mm_mul_ps(r1 , r0); 
				acc0 = _mm_add_ps(acc0 , r1); 
			}
			r0 = _mm_movehl_ps(acc0 , acc0);
			r0 = _mm_add_ps(r0 , acc0);
			r1 = _mm_castsi128_ps(_mm_srli_epi64(_mm_castps_si128(r0) , 32));
			r0 = _mm_add_ss(r0 , r1);
			_mm_store_ss(d+i , r0);
		}
	}
	else {
		while (i <= (m-4)) {
			acc0 = _mm_setzero_ps();
			acc1 = _mm_setzero_ps();
			acc2 = _mm_setzero_ps();
			acc3 = _mm_setzero_ps();
			for (j=0,p=n; j<=(n-4); j+=4,p+=4) {
				r0 = _mm_loadu_ps(r+j);
				r1 = _mm_loadu_ps(a+j);
				r2 = _mm_loadu_ps(a+p); a+=q;
				r3 = _mm_loadu_ps(a+j);
				r4 = _mm_loadu_ps(a+p); a-=q;
				r1 = _mm_mul_ps(r1 , r0); 
				r2 = _mm_mul_ps(r2 , r0);
				r3 = _mm_mul_ps(r3 , r0);
				r4 = _mm_mul_ps(r4 , r0);
				acc0 = _mm_add_ps(acc0 , r1); 
				acc1 = _mm_add_ps(acc1 , r2);
				acc2 = _mm_add_ps(acc2 , r3);
				acc3 = _mm_add_ps(acc3 , r4);
			}
			a += (4*n);
			r0 = _mm_unpacklo_ps(acc0 , acc1);
			acc0 = _mm_unpackhi_ps(acc0 , acc1);
			r2 = _mm_unpacklo_ps(acc2 , acc3);
			acc2 = _mm_unpackhi_ps(acc2 , acc3);
			r0 = _mm_add_ps(r0 , acc0);
			r2 = _mm_add_ps(r2 , acc2);
			r1 = _mm_movelh_ps(r0 , r2);
			r3 = _mm_movehl_ps(r2 , r0);
			r3 = _mm_add_ps(r1 , r3);
			_mm_storeu_ps(d+i , r3);
			i+=4;		
		}
		if (m & 2) {
			acc0 = _mm_setzero_ps();
			acc1 = _mm_setzero_ps();
			for (j=0,p=n; j<=(n-4); j+=4,p+=4) {
				r0 = _mm_loadu_ps(r+j);
				r1 = _mm_loadu_ps(a+j);
				r2 = _mm_loadu_ps(a+p);
				r1 = _mm_mul_ps(r1 , r0); 
				r2 = _mm_mul_ps(r2 , r0);
				acc0 = _mm_add_ps(acc0 , r1); 
				acc1 = _mm_add_ps(acc1 , r2);
			}
			a += q;
			r0 = _mm_unpacklo_ps(acc0 , acc1);
			r1 = _mm_unpackhi_ps(acc0 , acc1);
			r0 = _mm_add_ps(r0 , r1);
			r1 = _mm_movehl_ps(r1 , r0);
			r0 = _mm_add_ps(r0 , r1);
			r1 = _mm_castsi128_ps(_mm_srli_epi64(_mm_castps_si128(r0) , 32));
			_mm_store_ss(d+i , r0);
			_mm_store_ss(d+i+1 , r1);
			i+=2;		
		}
		if (m & 1) {
			acc0 = _mm_setzero_ps();
			for (j=0; j<=(n-4); j+=4) {
				r0 = _mm_loadu_ps(r+j);
				r1 = _mm_loadu_ps(a+j);
				r1 = _mm_mul_ps(r1 , r0); 
				acc0 = _mm_add_ps(acc0 , r1); 
			}
			a += n;
			r0 = _mm_movehl_ps(acc0 , acc0);
			r0 = _mm_add_ps(r0 , acc0);
			r1 = _mm_castsi128_ps(_mm_srli_epi64(_mm_castps_si128(r0) , 32));
			r0 = _mm_add_ss(r0 , r1);
			_mm_store_ss(d+i , r0);
		}
		a -= (n*m);
		p = n & 0x3;
		j = n - p;
		float x0,x1,x2;
		switch(p) {
			case 1:
				x0 = r[j];
				for (i=0; i<m; i++) {d[i] += (a[j]*x0); j+=n;} 
				break;
			case 2:
				x0 = r[j]; x1 = r[j+1];
				for (i=0; i<m; i++) {d[i] += (a[j]*x0 + a[j+1]*x1); j+=n;} 
				break;
			case 3:
				x0 = r[j]; x1 = r[j+1]; x2 = r[j+2];
				for (i=0; i<m; i++) {d[i] += (a[j]*x0 + a[j+1]*x1 + a[j+2]*x2); j+=n;} 
				break;
			default:	break;
		}		
	}
#endif
}

// transpose of matrix a(m,n) * vector r(m) -> vector d(n)
void VectorFunctions::mtmulv(float* d, float* a,	float* r, int m, int n)
{
	int i=0, j,k; float acc0a;
#ifdef ICSTLIB_NO_SSEOPT
	float acc0b,acc0c,acc0d,x;
	while (i <= (n-4)) {
		acc0a = acc0b = acc0c = acc0d = 0;
		for (j=0,k=0; j<m; j++,k+=n) {
			x = r[j];
			acc0a += a[k]*x;
			acc0b += a[k+1]*x;
			acc0c += a[k+2]*x;
			acc0d += a[k+3]*x;
		}
		d[i] = acc0a; d[i+1] = acc0b; d[i+2] = acc0c; d[i+3] = acc0d;
		a+=4; i+=4;
	}
#else
	__m128 acc,r0,r1;
	while (i <= (n-4)) {
		acc = _mm_setzero_ps();
		for (j=0,k=0; j<m; j++,k+=n) {
			r0 = _mm_load1_ps(r+j);
			r1 = _mm_loadu_ps(a+k);
			r1 = _mm_mul_ps(r1 , r0);
			acc = _mm_add_ps(acc , r1);
		}
		_mm_storeu_ps(d+i , acc);
		a+=4; i+=4;
	}
#endif
	while (i < n) {
		acc0a = 0;
		for (j=0,k=0; j<m; j++,k+=n) {acc0a += a[k]*r[j];}
		d[i] = acc0a;		
		a++; i++;
	}
}

// matrices a(m,n)*b(n,p) -> matrix c(m,p) 
void VectorFunctions::mmulm(float* c, float* a, float* b, int m, int n, int p)
{
	float* t; t = new float[n];
	for (int i=0; i<p; i++) {
		deinterleave(t,b,n,p,i);
		mmulv(c+i*m,a,t,m,n);
	}
	mxpose(c,p,m);
	delete[] t;
}

// inverse of matrix b(m,m) * matrix a(m,n) -> matrix a(m,n)
// return determinant of b, if det(b)=0: a remains unchanged
float VectorFunctions::minvmulm(float* a, float* b, int m, int n)
{
	int i,j,k,p,q,r; float acc,norm,det;
	int* idx; idx = new int[m];
	float* t; t = new float[m*m];

	copy(t,b,m*m);									// lu() alters matrix
	det = lu(t,m,idx);								// LU decomposition
	if (det == 0) {return 0;}						// singular matrix
	
	for (i=0; i<m; i++) {							// forward substitution
		p = n*idx[i]; r = n*i;
		for (j=0; j<n; j++,p++,r++) {
			acc = a[p];
			a[p] = a[r];
			for (k=j,q=i*m; k<r; k+=n,q++) {acc -= (t[q]*a[k]);}
			a[r] = acc;
		}
	}
	p = m*n;
	for (i=m-1; i>=0; i--) {						// back substitution
		norm = 1.0f/t[i*m+i];
		r = n*i;
		for (j=0; j<n; j++,r++) {
			acc = a[r];
			for (k=r+n,q=i*m+i+1; k<(p+j); k+=n,q++) {acc -= (t[q]*a[k]);}
			a[r] = acc*norm;
		}
	}

	delete[] t; delete[] idx;
	return det;
}			

//******************************************************************************
//* filters + delays
//*
// static delay: r delayed by n sampling intervals -> d
// c[0..n-1],cp: continuation data (init:0)
void VectorFunctions::delay(float* d, float* r, int size, float* c, int& cp, int n)
{
	int x,y,z;
	x = __min(n-1,__max(0,cp));
	if (n < size) {
		if (n <= 0) {memcpy(d,r,size*sizeof(float)); return;}
		y = size - n;
		z = n - x; 		
		memcpy(d,c+x,z*sizeof(float));
		memcpy(d+n,r,y*sizeof(float));
		memcpy(d+z,c,x*sizeof(float));
		memcpy(c+x,r+y,z*sizeof(float));
		memcpy(c,r+size-x,x*sizeof(float));	
	}
	else {
		y = x + size;
		if (y <= n) {
			memcpy(d,c+x,size*sizeof(float));
			memcpy(c+x,r,size*sizeof(float));	
		}
		else {
			z = n - x; 
			memcpy(d,c+x,z*sizeof(float));
			memcpy(c+x,r,z*sizeof(float));
			y -= n;
			memcpy(d+z,c,y*sizeof(float));
			memcpy(c,r+z,y*sizeof(float));
		}
		cp = y;
	}
}

// static FIR filter
// H(z) = b[0] + b[1]z^-1 +... + b[order]z^-order
// c[0..order-1]:	continuation data (init:0)
// NOTE: time-varying versions are made of 2 static filters and xfade
void VectorFunctions::fir(float* d, int size, float* b, int order, float* c)
{
	int i,j; float acc;
	int len = __min(order,size);
	float* temp; temp = new float[order];
	for (i=0; i<len; i++) {temp[i] = d[size-i-1];}
	for (i=len; i<order; i++) {temp[i] = c[i-len];}
#ifdef ICSTLIB_NO_SSEOPT
	for (i=size-1; i>=order; i--) {
		acc = b[0]*d[i]; for (j=1; j<=order; j++) {acc += (b[j]*d[i-j]);}
		d[i]=acc;
	}
#else
	i = size-16; int z;
	__m128 cf,r0,r1,r2,r3,r4,r5,r6,r7;
	while (i >= order) {
		r0 = _mm_setzero_ps();
		r1 = _mm_setzero_ps();
		r2 = _mm_setzero_ps();
		r3 = _mm_setzero_ps();
		z = i;
		for (j=0; j<=order; j++) {
			cf = _mm_set1_ps(b[j]);
			r4 = _mm_loadu_ps(d+z);
			r5 = _mm_loadu_ps(d+z+4);
			r6 = _mm_loadu_ps(d+z+8);
			r7 = _mm_loadu_ps(d+z+12);
			r4 = _mm_mul_ps(r4 , cf);
			r5 = _mm_mul_ps(r5 , cf);
			r6 = _mm_mul_ps(r6 , cf);
			r7 = _mm_mul_ps(r7 , cf);
			r0 = _mm_add_ps(r0 , r4);
			r1 = _mm_add_ps(r1 , r5);
			r2 = _mm_add_ps(r2 , r6);
			r3 = _mm_add_ps(r3 , r7);
			z--;
		}
		_mm_storeu_ps(d+i , r0);
		_mm_storeu_ps(d+i+4 , r1);
		_mm_storeu_ps(d+i+8 , r2);
		_mm_storeu_ps(d+i+12 , r3);
		i-=16;
	}
	i+=15;
	while (i >= order) {
		acc = b[0]*d[i]; for (j=1; j<=order; j++) {acc += (b[j]*d[i-j]);}
		d[i]=acc;
		i--;
	}
#endif
	for (i=len-1; i>=0; i--) {
		acc = b[0]*d[i]; for (j=1; j<=i; j++) {acc += (b[j]*d[i-j]);}
		for (j=i+1; j<=order; j++) {acc += (b[j]*c[j-i-1]);}
		d[i]=acc;
	}
	for (i=0; i<order; i++) {c[i] = temp[i];}
	delete[] temp;
}
void VectorFunctions::fir(float* d, int size, double* b, int order, float* c)
{
	int i,j; double acc;
	int len = __min(order,size);
	float* temp; temp = new float[order];
	for (i=0; i<len; i++) {temp[i]=d[size-i-1];}
	for (i=len; i<order; i++) {temp[i]=c[i-len];}
	for (i=size-1; i>=order; i--) {
		acc = b[0]*static_cast<double>(d[i]);
		for (j=1; j<=order; j++) {
			acc += (b[j]*static_cast<double>(d[i-j]));}
		d[i] = static_cast<float>(acc);
	}
	for (i=len-1; i>=0; i--) {
		acc = b[0]*static_cast<double>(d[i]);
		for (j=1; j<=i; j++) {
			acc += (b[j]*static_cast<double>(d[i-j]));}
		for (j=i+1; j<=order; j++) {
			acc += (b[j]*static_cast<double>(c[j-i-1]));}
		d[i] = static_cast<float>(acc);
	}
	for (i=0; i<order; i++) {c[i]=temp[i];}
	delete[] temp;
}

// static IIR filter
// H(z) = 1/(1 + a[1]z^-1 + ... + a[order]z^-order)
// c[0..order-1]: continuation data (init:0)	
void VectorFunctions::iir(float* d, int size, double* a, int order, double* c)
{
	int i,j; double x;
	for (i=0; i<size; i++) {
		x = static_cast<double>(d[i]); 
		for (j=order; j>1; j--) {x -= (a[j]*c[j-1]); c[j-1]=c[j-2];}
		x -= (a[1]*c[0]);
		if (fabs(x) < static_cast<double>(ANTI_DENORMAL_FLOAT)) {x=0;}
		c[0]=x; d[i] = static_cast<float>(x);
	}
}

// static 1st order section
// H(z) = 	(b[0] + b[1]z^-1)/(1 + a[1]z^-1)
// c[0]: continuation data (init:0)
void VectorFunctions::iir1(float* d, int size, float* a, float* b, float* c)
{
	// anti-denormal RNG, thread-safe in practice as occasionally overwriting
	// x by another call is harmless and write to int is atomic
	static unsigned int x = 1;
	union {float ftmp; unsigned int uitmp;};
	x = 663608941*x;
	uitmp = (x >> 9) | 0x40000000;
	
	// filter (looks awfully complicated but is really fast on Core2)
	int i=0, rm = size - ((size>>2)<<2);
	float out0,out1,in1;
	float acc = c[0], b0 = b[0] , b1 = b[1] - a[1]*b[0], a1 = -a[1];
	float ab = a1*b1, aa = a1*a1;
	float adn[4] = {	0.25f*ANTI_DENORMAL_FLOAT*(ftmp + 2.0f),
						-0.15f*ANTI_DENORMAL_FLOAT*ftmp,
						0.08f*ANTI_DENORMAL_FLOAT,
						-0.37f*ANTI_DENORMAL_FLOAT				};
	for (i=0; i<rm; i++) {
		out0 = b1*acc + b0*d[i];
		acc = a1*acc + d[i] + adn[i];
		d[i] = out0;
	}
	for (i=rm; i<size; i+=4) {
		in1 = d[i+1] + adn[0];
		out0 = b1*acc + b0*d[i];
		out1 = ab*acc + b1*d[i] + b0*d[i+1] + adn[1];
		acc = aa*acc + a1*d[i] + in1;
		d[i] = out0 + adn[2];
		d[i+1] = out1;
		in1 = d[i+3] + adn[0];
		out0 = b1*acc + b0*d[i+2];
		out1 = ab*acc + b1*d[i+2] + b0*d[i+3] + adn[1];
		acc = aa*acc + a1*d[i+2] + in1;
		d[i+2] = out0 + adn[2];
		d[i+3] = out1;
	}
	c[0] = acc;	
}			

// time-varying 1st order section
// start coefficients: as[1],bs[0],bs[1], end coefficients: ae[1],be[0],be[1]
// H(z) = (bs[0]..be[0] + bs[1]..be[1]*z^-1)/(1 + as[1]..ae[1]*z^-1)
// c[0]: continuation data (init:0)
void VectorFunctions::viir1(float* d, int size, float* as, float* ae,
				   float* bs, float* be, float* c)
{
	// anti-denormal RNG, thread-safe in practice as occasionally overwriting
	// x by another call is harmless and write to int is atomic
	static unsigned int x = 1;
	union {float ftmp; unsigned int uitmp;};
	x = 663608941*x;
	uitmp = (x >> 9) | 0x40000000;
	
	// filter
	float scl = 1.0f/__max(1.0f,static_cast<float>(size-1));
	float ainc = scl*(ae[1] - as[1]);
	float b0inc = scl*(be[0] - bs[0]);
	float b1inc = scl*(be[1] - bs[1]);
	float a = -as[1] + ainc;
	float b0 = bs[0] - b0inc;
	float b1 = bs[1];
	float tmp, tmp2, acc = c[0];
	float adn[3] = {	0.25f*ANTI_DENORMAL_FLOAT*(ftmp + 2.0f),
						-0.15f*ANTI_DENORMAL_FLOAT*ftmp,
						0.08f*ANTI_DENORMAL_FLOAT*(ftmp - 2.761f)	};
	for (int i=0; i<size; i++) {
		tmp = d[i] + adn[i&1];
		tmp2 = b1*acc;
		a -= ainc;
		acc = a*acc + tmp;
		b0 += b0inc;
		d[i] = b0*acc + tmp2 + adn[2];
		b1 += b1inc;
	}
	c[0] = acc;
}	

// static direct form biquad
// H(z) = (b[0] + b[1]z^-1 + b[2]^-2)/(1 + a[1]z^-1 + a[2]z^-2)
// c[0..1]: continuation data (init:0)
// NOTE:time-varying versions can be made of 2 static filters and xfade, if
//		modulation speed and natural transitions are important, alternative
//		filter structures (Chamberlin, Gold-Rader) are strongly recommended 
void VectorFunctions::biquad(float* d, int size, double* a, double* b, double* c)
{
	// anti-denormal RNG, thread-safe in practice as occasionally overwriting
	// z by another call is harmless and write to int is atomic
	static unsigned int z = 1;
	union {float ftmp; unsigned int uitmp;};
	z = 663608941*z;
	uitmp = (z >> 9) | 0x40000000;
	
	// filter
	double a1 = -a[1], a2 = -a[2];
	double b0 = b[0], b1 = b[1] - a[1]*b[0], b2 = b[2] - a[2]*b[0];
	double x = c[0], y = c[1], in = static_cast<double>(d[0]);
	double r0,r1,r2,r3,r4;
	double adscl = __min(fabs(1.0 - a1 - a2), fabs(1.0 + a1 - a2)); 
	double adn[2] = {
		adscl*static_cast<double>(0.25f*ANTI_DENORMAL_FLOAT*(ftmp + 2.0f)), 
		adscl*static_cast<double>(-0.15f*ANTI_DENORMAL_FLOAT*ftmp)
	};
	size--;
	for (int i=0; i<size; i++) {
		r0 = b1*in; 
		r1 = b0*in;
		r2 = b2*in;
		r3 = a1*x;
		r4 = a2*x;
		r0 += y;
		r1 += x;
		x = r0 + r3;
		in = static_cast<double>(d[i+1]);
		y = r2 + r4 + adn[i & 1];
		d[i] = static_cast<float>(r1);
	}
	r0 = b1*in; 
	r1 = b0*in;
	r2 = b2*in;
	c[0] = y + r0 + a1*x;
	c[1] = r2 + a2*x + adn[size & 1];
	d[size] = static_cast<float>(r1 + x);
}

// 3-point median filter
// c[0..1]: continuation data (init:0)
void VectorFunctions::med3(float* d, int size, float* c)
{
	int i; float x,t0,t1;							
	if (size == 1) {t0 = c[1];} else {t0 = d[size-2];}
	t1 = d[size-1]; 
#ifdef ICSTLIB_NO_SSEOPT
	for (i=size-1; i>1; i--) {	
		x = d[i] - d[i-2];
		if (x*(d[i-2] - d[i-1]) >= 0) {d[i] = d[i-2];}
		else if (x*(d[i-1] - d[i]) < 0) {d[i] = d[i-1];}
	}
#else
	i = size-4; 
	__m128 r0,r1,r2,r3,r4,r5,r6;
	while (i > 1) {
		r0 = _mm_loadu_ps(d+i-2);
		r1 = _mm_loadu_ps(d+i-1);
		r2 = _mm_loadu_ps(d+i);
		r3 = _mm_min_ps(r0 , r1);
		r4 = _mm_max_ps(r0 , r1);
		r5 = _mm_cmpgt_ps(r3 , r2);
		r6 = _mm_cmplt_ps(r4 , r2);
		r3 = _mm_and_ps(r3 , r5);
		r4 = _mm_and_ps(r4 , r6);
		r3 = _mm_or_ps(r4 , r3);
		r5 = _mm_or_ps(r5 , r6);
		r2 = _mm_andnot_ps(r5 , r2);
		r3 = _mm_or_ps(r3 , r2);
		_mm_storeu_ps(d+i , r3);
		i -= 4;
	}
	i += 3;
	while (i > 1) {
		r0 = _mm_load_ss(d+i-2);
		r1 = _mm_load_ss(d+i-1);
		r2 = _mm_load_ss(d+i);
		r3 = _mm_min_ss(r0 , r1);
		r4 = _mm_max_ss(r0 , r1);
		r5 = _mm_cmpgt_ss(r3 , r2);
		r6 = _mm_cmplt_ss(r4 , r2);
		r3 = _mm_and_ps(r3 , r5);
		r4 = _mm_and_ps(r4 , r6);
		r3 = _mm_or_ps(r4 , r3);
		r5 = _mm_or_ps(r5 , r6);
		r2 = _mm_andnot_ps(r5 , r2);
		r3 = _mm_or_ps(r3 , r2);
		_mm_store_ss(d+i , r3);
		i--;
	}
#endif
	x = d[1]-c[1];
	if (x*(c[1]-d[0]) >= 0) {d[1] = c[1];}
	else if (x*(d[0]-d[1]) < 0) {d[1] = d[0];}
	x = d[0]-c[0];
	if (x*(c[0]-c[1]) >= 0) {d[0] = c[0];}
	else if (x*(c[1]-d[0]) < 0) {d[0] = c[1];}
	c[0]=t0; c[1]=t1;	
}

// 5-point median filter
// c[0..3]: continuation data (init:0)
void VectorFunctions::med5(float* d, int size, float* c)
{
	int i; float t0; float s[5];				
	for (i=0; i<size; i++) {
		s[0]=c[0]; s[1]=c[1]; s[2]=c[2]; s[3]=c[3]; s[4]=d[i];
		if (s[1]<s[0]) {t0=s[0]; s[0]=s[1]; s[1]=t0;}
		if (s[2]<s[0]) {t0=s[0]; s[0]=s[2]; s[2]=t0;}
		if (s[3]<s[0]) {t0=s[0]; s[0]=s[3]; s[3]=t0;}
		if (s[4]<s[0]) {s[4]=s[0];}
		if (s[2]<s[1]) {t0=s[1]; s[1]=s[2]; s[2]=t0;}
		if (s[3]<s[1]) {t0=s[1]; s[1]=s[3]; s[3]=t0;}
		if (s[4]<s[1]) {s[4]=s[1];}
		if (s[3]<s[2]) {s[2]=s[3];}
		if (s[4]<s[2]) {s[2]=s[4];}
		c[0]=c[1]; c[1]=c[2]; c[2]=c[3]; c[3]=d[i]; d[i]=s[2];
	}									
}

//******************************************************************************
//* system analysis
//*
// calculate frequency response	at frequencies f[0..size-1] relative to fs
//			sum(i=0..border){b[i]z^-i}
//  H(z) =	--------------------------
//			sum(i=0..aorder){a[i]z^-i}
void VectorFunctions::freqz(	float* mag,	float* phase, float* f, int size,
					double* a, int aorder, double* b, int border)				
{
	int i,j=0; float omega; float* av; float* bv;
	av = new float[2*size]; bv = new float[2*size];
	for (i=0; i<size; i++, j+=2) {
		omega = 2.0f*M_PI_FLOAT*f[i];
		av[j] = cosf(omega); av[j+1] = -sinf(omega); 
	}
	cpxcopy(bv,av,size);
	cpxpolyval(av,size,a,aorder); cpxpolyval(bv,size,b,border);
	cpxinv(av,size); cpxmul(av,bv,size);
	cpxmag(mag,av,size); cpxarg(phase,av,size); unwrap(phase,size);
	delete[] av; delete[] bv;
}

// calculate frequency response	at absolute frequencies f[0..size-1]
//			sum(i=0..border){b[i]s^i}		
//	G(s) =	-------------------------							
//			sum(i=0..aorder){a[i]s^i}											 
void VectorFunctions::freqs(float* mag, float* phase, float* f, int size,
				   float* a, int aorder, float* b, int border)
{
	float* av; av = new float[2*size]; float* bv; bv = new float[2*size];
	mul(f,2.0f*M_PI_FLOAT,size); set(phase,0,size);
	realtocpx(av,phase,f,size); cpxcopy(bv,av,size); 
	cpxpolyval(av,size,a,aorder); cpxinv(av,size);
	cpxpolyval(bv,size,b,border); cpxmul(av,bv,size);
	cpxmag(mag,av,size); cpxarg(phase,av,size); unwrap(phase,size);
	delete[] av; delete[] bv;
}

// calculate group delay at frequencies f[0..size-1] relative to fs
//			sum(i=0..border){b[i]z^-i}
//	H(z) =	--------------------------
//			sum(i=0..aorder){a[i]z^-i}
void VectorFunctions::gdelz(	float* gdelay, float* f, int size,				
					double* a, int aorder, double* b, int border)
{
	int i,j=0; float omega;
	float* av; float* bv; float* dav; float* dbv; double* da; double* db;
	av = new float[2*size]; bv = new float[2*size];
	dav = new float[2*size]; dbv = new float[2*size];
	da = new double[aorder+1]; db = new double[border+1];

	// calculate phasors from frequencies
	for (i=0; i<size; i++, j+=2) {
		omega = 2.0f*M_PI_FLOAT*f[i];
		av[j] = bv[j] = dav[j] = dbv[j] = cosf(omega);
		av[j+1] = bv[j+1] = dav[j+1] = dbv[j+1] = -sinf(omega); 
	}

	// calculate z*dA(z)/dz and z*dB(z)/dz
	for (i=0; i<=aorder; i++) {da[i] = a[i]*static_cast<double>(i);}
	for (i=0; i<=border; i++) {db[i] = b[i]*static_cast<double>(i);}
	
	// calculate group delay 
	cpxpolyval(av,size,a,aorder); cpxpolyval(bv,size,b,border);
	cpxpolyval(dav,size,da,aorder); cpxpolyval(dbv,size,db,border);
	cpxinv(av,size); cpxinv(bv,size); 
	cpxmul(av,dav,size); cpxmul(bv,dbv,size);
	cpxre(av,av,size); cpxre(gdelay,bv,size);
	sub(gdelay,av,size);

	// clean up
	delete[] av;delete[] bv;delete[] dav;delete[] dbv;delete[] da;delete[] db;		
}

// calculate group delay at absolute frequencies f[0..size-1]
//			sum(i=0..border){b[i]s^i}
//	G(s) =	-------------------------
//			sum(i=0..aorder){a[i]s^i}
void VectorFunctions::gdels(	float* gdelay, float* f, int size,										
					float* a, int aorder, float* b, int border)
{
	float* av; float* bv; float* dav; float* dbv; float* da; float* db;
	av = new float[2*size]; bv = new float[2*size];
	dav = new float[2*size]; dbv = new float[2*size];
	da = new float[aorder+1]; db = new float[border+1];

	// calculate phasors from frequencies
	mul(f,2.0f*M_PI_FLOAT,size); set(bv,0,size);
	realtocpx(av,bv,f,size);
	cpxcopy(bv,av,size); cpxcopy(dav,av,size); cpxcopy(dbv,av,size);

	// calculate dA(s)/ds and dB(s)/ds
	copy(da,a,aorder+1); copy(db,b,border+1);
	SpecMath::diffpoly(da,aorder); SpecMath::diffpoly(db,border);
	
	// calculate group delay 
	cpxpolyval(av,size,a,aorder); cpxpolyval(bv,size,b,border);
	cpxpolyval(dav,size,da,aorder); cpxpolyval(dbv,size,db,border);
	cpxinv(av,size); cpxinv(bv,size); 
	cpxmul(av,dav,size); cpxmul(bv,dbv,size);
	cpxre(gdelay,av,size); cpxre(bv,bv,size);
	sub(gdelay,bv,size);

	// clean up
	delete[] av;delete[] bv;delete[] dav;delete[] dbv;delete[] da;delete[] db;	
}

//******************************************************************************
//* statistics
//*
// return arithmetic mean
float VectorFunctions::mean(float* d, int size) {
	return VectorFunctions::sum(d,size)/static_cast<float>(size);}

// return arithmetic mean of data pairs:
// {d[i] = unnormalized probability, r[i] = value}
float VectorFunctions::pdmean(float* d, float* r, int size)
{
	int i=0; double xsum=0, ysum=0;
	
#ifndef ICSTLIB_NO_SSEOPT  
	if ((reinterpret_cast<uintptr_t>(d) | reinterpret_cast<uintptr_t>(r)) & 0xF) {
#endif	
		int rm = size - ((size>>2)<<2);
		float x=0, y=0;
		for (i=0; i<rm; i++) {x += (r[i]*d[i]); y += d[i];}
		xsum = static_cast<double>(x); ysum = static_cast<double>(y);
		for (i=rm; i<size; i+=4) {
			x = y = 0;
			x += (r[i]*d[i]);		y += d[i];
			x += (r[i+1]*d[i+1]);	y += d[i+1];
			x += (r[i+2]*d[i+2]);	y += d[i+2];
			x += (r[i+3]*d[i+3]);	y += d[i+3];
			xsum += static_cast<double>(x); ysum += static_cast<double>(y);	
		}
#ifndef ICSTLIB_NO_SSEOPT
	}
	else {
		__m128 r0,r1,r2,r3,acc0,acc1,acc2,acc3;
		int j; float res[8];
		while (i <= (size-256)) {
			acc0 = _mm_setzero_ps();
			acc1 = _mm_setzero_ps();
			acc2 = _mm_setzero_ps();
			acc3 = _mm_setzero_ps();
			for (j=0; j<16; j++) {
				r0 = _mm_load_ps(d+i);
				r1 = _mm_load_ps(r+i);
				r2 = _mm_load_ps(d+i+4);
				r3 = _mm_load_ps(r+i+4);
				r1 = _mm_mul_ps(r0 , r1);
				r3 = _mm_mul_ps(r2 , r3);
				acc0 = _mm_add_ps(acc0 , r0);
				acc2 = _mm_add_ps(acc2 , r2);
				acc1 = _mm_add_ps(acc1 , r1);
				acc3 = _mm_add_ps(acc3 , r3);
				r0 = _mm_load_ps(d+i+8);
				r1 = _mm_load_ps(r+i+8);
				r2 = _mm_load_ps(d+i+12);
				r3 = _mm_load_ps(r+i+12);
				r1 = _mm_mul_ps(r0 , r1);
				r3 = _mm_mul_ps(r2 , r3);
				acc0 = _mm_add_ps(acc0 , r0);
				acc2 = _mm_add_ps(acc2 , r2);
				acc1 = _mm_add_ps(acc1 , r1);
				acc3 = _mm_add_ps(acc3 , r3);
				i += 16;
			}
			acc0 = _mm_add_ps(acc0 , acc2);
			acc1 = _mm_add_ps(acc1 , acc3);
			_mm_storeu_ps(res+4 , acc0);
			_mm_storeu_ps(res , acc1);
			xsum += static_cast<double>(res[0]);
			ysum += static_cast<double>(res[4]);
			xsum += static_cast<double>(res[1]);
			ysum += static_cast<double>(res[5]);
			xsum += static_cast<double>(res[2]);
			ysum += static_cast<double>(res[6]);
			xsum += static_cast<double>(res[3]);
			ysum += static_cast<double>(res[7]);
		}
		acc0 = _mm_setzero_ps();
		acc1 = _mm_setzero_ps();
		acc2 = _mm_setzero_ps();
		acc3 = _mm_setzero_ps();	
		while (i <= (size-8)) {
			r0 = _mm_load_ps(d+i);
			r1 = _mm_load_ps(r+i);
			r2 = _mm_load_ps(d+i+4);
			r3 = _mm_load_ps(r+i+4);
			r1 = _mm_mul_ps(r0 , r1);
			r3 = _mm_mul_ps(r2 , r3);
			acc0 = _mm_add_ps(acc0 , r0);
			acc2 = _mm_add_ps(acc2 , r2);
			acc1 = _mm_add_ps(acc1 , r1);
			acc3 = _mm_add_ps(acc3 , r3);
			i += 8;
		}
		acc0 = _mm_add_ps(acc0 , acc2);
		acc1 = _mm_add_ps(acc1 , acc3);
		_mm_storeu_ps(res+4 , acc0);
		_mm_storeu_ps(res , acc1);
		xsum += static_cast<double>(res[0]);
		ysum += static_cast<double>(res[4]);
		xsum += static_cast<double>(res[1]);
		ysum += static_cast<double>(res[5]);
		xsum += static_cast<double>(res[2]);
		ysum += static_cast<double>(res[6]);
		xsum += static_cast<double>(res[3]);
		ysum += static_cast<double>(res[7]);
		while (i < size) {
			xsum += static_cast<double>(r[i]*d[i]);
			ysum += static_cast<double>(d[i]);
			i++;
		}
	}
#endif
	if (ysum >= static_cast<double>(FLT_MIN)) {return static_cast<float>(xsum/ysum);}
	else {return mean(r,size);}
}

// return unbiased variance	using 2-pass algorithm with double precision accumulation
float VectorFunctions::var(float* d, int size)
{
	if (size < 2) {return 0;}				// no worse than any other value
	int i=0; double xsum=0;
	float m = mean(d,size);

#ifndef ICSTLIB_NO_SSEOPT
	if (reinterpret_cast<uintptr_t>(d) & 0xF) {
#endif		
		float z1,z2, x1=0, x2=0;
		int rm = size - ((size>>2)<<2);
		for (i=0; i<rm; i++) {
			z1 = d[i] - m;
			x1 += (z1*z1);
		}
		xsum = static_cast<double>(x1);
		for (i=rm; i<size; i+=4) {
			z1 = d[i] - m;
			z2 = d[i+1] - m;
			x1 = z1*z1;
			x2 = z2*z2;
			z1 = d[i+2] - m;
			z2 = d[i+3] - m;
			x1 += (z1*z1);
			x2 += (z2*z2);
			xsum += static_cast<double>(x1 + x2);
		}
#ifndef ICSTLIB_NO_SSEOPT
	}
	else {
		__m128 r0,r1,r2,r3,acc0,acc1;
		__m128 mu = _mm_set1_ps(m);
		int j; float res[4], z;
		while (i <= (size-256)) {
			acc0 = _mm_setzero_ps();
			acc1 = _mm_setzero_ps();
			for (j=0; j<16; j++) {
				r0 = _mm_load_ps(d+i);
				r1 = _mm_load_ps(d+i+4);
				r2 = _mm_load_ps(d+i+8);
				r3 = _mm_load_ps(d+i+12);
				r0 = _mm_sub_ps(r0 , mu);
				r1 = _mm_sub_ps(r1 , mu);
				r2 = _mm_sub_ps(r2 , mu);
				r3 = _mm_sub_ps(r3 , mu);
				r0 = _mm_mul_ps(r0 , r0);
				r1 = _mm_mul_ps(r1 , r1);
				r2 = _mm_mul_ps(r2 , r2);
				r3 = _mm_mul_ps(r3 , r3);
				acc0 = _mm_add_ps(acc0 , r0);
				acc1 = _mm_add_ps(acc1 , r1);
				acc0 = _mm_add_ps(acc0 , r2);
				acc1 = _mm_add_ps(acc1 , r3);
				i += 16;
			}
			acc0 = _mm_add_ps(acc0 , acc1);
			_mm_storeu_ps(res , acc0);
			xsum += static_cast<double>(res[0]);
			xsum += static_cast<double>(res[1]);
			xsum += static_cast<double>(res[2]);
			xsum += static_cast<double>(res[3]);
		}
		acc0 = _mm_setzero_ps();
		acc1 = _mm_setzero_ps();
		while (i <= (size-16)) {
			r0 = _mm_load_ps(d+i);
			r1 = _mm_load_ps(d+i+4);
			r2 = _mm_load_ps(d+i+8);
			r3 = _mm_load_ps(d+i+12);
			r0 = _mm_sub_ps(r0 , mu);
			r1 = _mm_sub_ps(r1 , mu);
			r2 = _mm_sub_ps(r2 , mu);
			r3 = _mm_sub_ps(r3 , mu);
			r0 = _mm_mul_ps(r0 , r0);
			r1 = _mm_mul_ps(r1 , r1);
			r2 = _mm_mul_ps(r2 , r2);
			r3 = _mm_mul_ps(r3 , r3);
			acc0 = _mm_add_ps(acc0 , r0);
			acc1 = _mm_add_ps(acc1 , r1);
			acc0 = _mm_add_ps(acc0 , r2);
			acc1 = _mm_add_ps(acc1 , r3);
			i += 16;
		}
		while (i <= (size-4)) {
			r0 = _mm_load_ps(d+i);
			r0 = _mm_sub_ps(r0 , mu);
			r0 = _mm_mul_ps(r0 , r0);
			acc0 = _mm_add_ps(acc0 , r0);
			i += 4;
		}
		while (i < size) {z = d[i] - m; xsum += (z*z); i++;}
		acc0 = _mm_add_ps(acc0 , acc1);
		_mm_storeu_ps(res , acc0);
		xsum += static_cast<double>(res[0]);
		xsum += static_cast<double>(res[1]);
		xsum += static_cast<double>(res[2]);
		xsum += static_cast<double>(res[3]);
	}
#endif
	return static_cast<float>(xsum)/static_cast<float>(size-1);
}

// return unbiased variance of data pairs:
// {d[i] = unnormalized probability, r[i] = value}
float VectorFunctions::pdvar(float* d, float* r, int size)
{
	int i=0; float t; double vsum=0, xsum=0, ysum=0;
	float z = pdmean(d,r,size);
	
#ifdef ICSTLIB_NO_SSEOPT
	int rm = size - ((size>>2)<<2);
	float v=0, x=0, y=0;
	for (i=0; i<rm; i++) {
		v += (d[i]*d[i]);
		t = r[i] - z;
		x += (t*t*d[i]);
		y += d[i];
	}
	vsum = static_cast<double>(v);
	xsum = static_cast<double>(x);
	ysum = static_cast<double>(y);
	for (i=rm; i<size; i+=4) {
		v = x = y = 0;
		v += (d[i]*d[i]);		t = r[i] - z;
		x += (t*t*d[i]);		y += d[i];
		v += (d[i+1]*d[i+1]);	t = r[i+1] - z;
		x += (t*t*d[i+1]);		y += d[i+1];
		v += (d[i+2]*d[i+2]);	t = r[i+2] - z;
		x += (t*t*d[i+2]);		y += d[i+2];
		v += (d[i+3]*d[i+3]);	t = r[i+3] - z;
		x += (t*t*d[i+3]);		y += d[i+3];
		vsum += static_cast<double>(v);
		xsum += static_cast<double>(x);
		ysum += static_cast<double>(y);	
	}
#else
	__m128 r0,r1,r2,acc0,acc1,acc2;
	__m128 mu = _mm_set1_ps(z);
	int j; float res[12];
	while (i <= (size-256)) {
		acc0 = _mm_setzero_ps();
		acc1 = _mm_setzero_ps();
		acc2 = _mm_setzero_ps();
		for (j=0; j<64; j++) {
			r0 = _mm_loadu_ps(r+i);
			r1 = _mm_loadu_ps(d+i);
			r0 = _mm_sub_ps(r0 , mu);
			r2 = _mm_mul_ps(r1 , r1);
			acc1 = _mm_add_ps(acc1 , r1);
			r0 = _mm_mul_ps(r0 , r0);
			acc2 = _mm_add_ps(acc2 , r2);
			r0 = _mm_mul_ps(r0 , r1);
			acc0 = _mm_add_ps(acc0 , r0);
			i += 4;
		}
		_mm_storeu_ps(res , acc0);
		_mm_storeu_ps(res+4 , acc1);
		_mm_storeu_ps(res+8 , acc2);
		xsum += static_cast<double>(res[0]);
		ysum += static_cast<double>(res[4]);
		vsum += static_cast<double>(res[8]);
		xsum += static_cast<double>(res[1]);
		ysum += static_cast<double>(res[5]);
		vsum += static_cast<double>(res[9]);
		xsum += static_cast<double>(res[2]);
		ysum += static_cast<double>(res[6]);
		vsum += static_cast<double>(res[10]);
		xsum += static_cast<double>(res[3]);
		ysum += static_cast<double>(res[7]);
		vsum += static_cast<double>(res[11]);
	}
	acc0 = _mm_setzero_ps();
	acc1 = _mm_setzero_ps();
	acc2 = _mm_setzero_ps();
	while (i <= (size-4)) {
		r0 = _mm_loadu_ps(r+i);
		r1 = _mm_loadu_ps(d+i);
		r0 = _mm_sub_ps(r0 , mu);
		r2 = _mm_mul_ps(r1 , r1);
		acc1 = _mm_add_ps(acc1 , r1);
		r0 = _mm_mul_ps(r0 , r0);
		acc2 = _mm_add_ps(acc2 , r2);
		r0 = _mm_mul_ps(r0 , r1);
		acc0 = _mm_add_ps(acc0 , r0);
		i += 4;
	}
	while (i < size) {
		vsum += static_cast<double>(d[i]*d[i]);
		t = r[i] - z;
		xsum += static_cast<double>(t*t*d[i]);
		ysum += static_cast<double>(d[i]);
		i++;
	}	
	_mm_storeu_ps(res , acc0);
	_mm_storeu_ps(res+4 , acc1);
	_mm_storeu_ps(res+8 , acc2);
	xsum += static_cast<double>(res[0]);
	ysum += static_cast<double>(res[4]);
	vsum += static_cast<double>(res[8]);
	xsum += static_cast<double>(res[1]);
	ysum += static_cast<double>(res[5]);
	vsum += static_cast<double>(res[9]);
	xsum += static_cast<double>(res[2]);
	ysum += static_cast<double>(res[6]);
	vsum += static_cast<double>(res[10]);
	xsum += static_cast<double>(res[3]);
	ysum += static_cast<double>(res[7]);
	vsum += static_cast<double>(res[11]);
#endif
	if ((ysum*ysum) > vsum) {
		return static_cast<float>(xsum*ysum/(ysum*ysum - vsum));}
	else {return var(r,size);}
}

// return average absolute deviation
// m = mean or median
float VectorFunctions::aad(float* d, int size, float m)
{
	int i=0; double x=0;

#ifdef ICSTLIB_NO_SSEOPT
	for (i=0; i<size; i++) {
		x += static_cast<double>(fabsf(d[i] - m));
	}
#else
	__m128 r0,r1,acc0,acc1;
	__m128 mu = _mm_set1_ps(m);
	__m128 absmask = _mm_castsi128_ps(_mm_set1_epi32(0x7FFFFFFF));
	int j; float res[4];
	while (i <= (size-256)) {
		acc0 = _mm_setzero_ps();
		acc1 = _mm_setzero_ps();
		for (j=0; j<32; j++) {
			r0 = _mm_loadu_ps(d+i);
			r1 = _mm_loadu_ps(d+i+4);
			r0 = _mm_sub_ps(r0 , mu);
			r1 = _mm_sub_ps(r1 , mu);
			r0 = _mm_and_ps(r0 , absmask);
			r1 = _mm_and_ps(r1 , absmask);
			acc0 = _mm_add_ps(acc0 , r0);
			acc1 = _mm_add_ps(acc1 , r1);
			i += 8;
		}
		acc0 = _mm_add_ps(acc0 , acc1);
		_mm_storeu_ps(res , acc0);
		x += static_cast<double>(res[0]);
		x += static_cast<double>(res[1]);
		x += static_cast<double>(res[2]);
		x += static_cast<double>(res[3]);
	}
	acc0 = _mm_setzero_ps();
	acc1 = _mm_setzero_ps();
	while (i <= (size-8)) {
		r0 = _mm_loadu_ps(d+i);
		r1 = _mm_loadu_ps(d+i+4);
		r0 = _mm_sub_ps(r0 , mu);
		r1 = _mm_sub_ps(r1 , mu);
		r0 = _mm_and_ps(r0 , absmask);
		r1 = _mm_and_ps(r1 , absmask);
		acc0 = _mm_add_ps(acc0 , r0);
		acc1 = _mm_add_ps(acc1 , r1);
		i += 8;
	}
	while (i < size) {
		x += static_cast<double>(fabsf(d[i] - m));
		i++;
	}
	acc0 = _mm_add_ps(acc0 , acc1);
	_mm_storeu_ps(res , acc0);
	x += static_cast<double>(res[0]);
	x += static_cast<double>(res[1]);
	x += static_cast<double>(res[2]);
	x += static_cast<double>(res[3]);
#endif
	return static_cast<float>(x)/static_cast<float>(size);
}

// return median
// uses STL selector which is O(n) on average and thus faster than sort + pick
float VectorFunctions::median(float* d, int size)
{
	float* temp; temp = new float[size];
	VectorFunctions::copy(temp,d,size);
	std::nth_element(temp, temp + (size>>1), temp+size);
	float x = temp[size>>1];
	if ((size & 1) == 0) {x = 0.5f*(x + temp[maxi(temp,size>>1)]);}
	delete[] temp;
	return x;
}

// return median absolute deviation
float VectorFunctions::mad(float* d, int size)
{
	float x, m = median(d,size);
	float* temp; temp = new float[size];
	copy(temp,d,size);
	sub(temp,m,size);
	abs(temp,size);
	x = median(temp,size);
	delete[] temp;
	return x;
}

// return p-quantile
// uses Hazen's rule and STL selector which is O(n) on average
float VectorFunctions::quantile(float* d, float p, int size)
{
	double pfrac = static_cast<double>(p)*static_cast<double>(size) - 0.5;
	if (pfrac <= 0) {return d[mini(d,size)];}
	if (pfrac >= static_cast<double>(size-1)) {return d[maxi(d,size)];}
	int pint = SpecMath::fsplit(pfrac);
	float* temp; temp = new float[size];
	VectorFunctions::copy(temp,d,size);
	std::nth_element(temp, temp+pint, temp+size);
	float xlo = temp[pint];
	float xhi = temp[pint + 1 + mini(temp+pint+1, size-pint-1)];
	delete[] temp;
	return xlo + static_cast<float>(pfrac)*(xhi - xlo);
}

// return geometric mean
float VectorFunctions::gmean(float* d, int size)
{
	const double LLIM = 1.000001*DBL_MIN/static_cast<double>(FLT_MIN);
	const double ULIM = 0.999999*DBL_MAX/static_cast<double>(FLT_MAX);
	const double LSLIM = 1.000001*sqrt(DBL_MIN);
	const double USLIM = 0.999999*sqrt(DBL_MAX);
	double x=1.0, x2=1.0, y=0;
	for (int i=0; i<(size-1); i+=2) {
		x *= static_cast<double>(d[i]);
		x2 *= static_cast<double>(d[i+1]);
		if ((x < DBL_MIN) || (x2 < DBL_MIN)) {return 0;}
		if ((x < LLIM) || (x > ULIM)) {y += log(x); x = 1.0;}
		if ((x2 < LLIM) || (x2 > ULIM)) {y += log(x2); x2 = 1.0;}
	}
	if (size & 1) {
		x *= static_cast<double>(d[size-1]);
		if (x < DBL_MIN) {return 0;}
	}
	if ((x < LSLIM) || (x > USLIM) || (x2 < LSLIM) || (x2 > USLIM)) {y += log(x) + log(x2);}
	else {y += log(x*x2);}
	return expf(static_cast<float>(y)/static_cast<float>(size));
}

// return harmonic mean			
float VectorFunctions::hmean(float* d, int size)
{
	int i=0; double x=0;

#ifdef ICSTLIB_NO_SSEOPT
	 for (i=0; i<size; i++) {
		 if (d[i] >= FLT_MIN) {x += static_cast<double>(1.0f/d[i]);}
		 else return 0;
	 }
#else
	__m128 r0,r1,r2;
	__m128 flag = _mm_setzero_ps();
	__m128 lim = _mm_set1_ps(FLT_MIN);
	while (i <= (size-4)) {	// check whether all input values are > 0
		r0 = _mm_loadu_ps(d+i);
		r0 = _mm_cmplt_ps(r0 , lim);
		flag = _mm_or_ps(flag , r0);
		i += 4;
	}
	while (i < size) { // check epilogue, accumulate reciprocals prologue
		if (d[i] >= FLT_MIN) {x += static_cast<double>(1.0f/d[i]);}
		else return 0;
		i++;
	}
	int res[4];
	_mm_storeu_si128(reinterpret_cast<__m128i*>(res) , _mm_castps_si128(flag));
	if ((res[0] | res[1] | res[2] | res[3]) != 0) {return 0;}
	i=0;
	__m128d tmp, acc = _mm_setzero_pd();
	while (i <= (size-4)) { // accumulate reciprocals
		r0 = _mm_loadu_ps(d+i);
		r1 = _mm_rcp_ps(r0);
		r0 = _mm_mul_ps(r0 , r1);
		r2 = _mm_add_ps(r1 , r1);
		r0 = _mm_mul_ps(r0 , r1);
		r2 = _mm_sub_ps(r2 , r0);
		tmp = _mm_cvtps_pd(r2);
		r2 = _mm_movehl_ps(r2 ,r2);
		acc = _mm_add_pd(acc , tmp);
		tmp = _mm_cvtps_pd(r2);
		acc = _mm_add_pd(acc , tmp);
		i += 4;
	}
	double dres[2];
	_mm_storeu_pd(dres , acc);
	x += dres[0] + dres[1];
#endif
	return static_cast<float>(static_cast<double>(size)/x);
}

// return covariance
float VectorFunctions::cov(float* x, float* y, int size)
{
	int i=0; double a=0;
	float xm = mean(x,size), ym = mean(y,size);
	
#ifdef ICSTLIB_NO_SSEOPT
	for (i=0; i<size; i++) {a += static_cast<double>((x[i] - xm)*(y[i] - ym));}
#else
	int j; float res[4];
	__m128 xmu = _mm_set1_ps(xm);
	__m128 ymu = _mm_set1_ps(ym);
	__m128 r0,r1,r2,r3,acc0,acc1;
	if (!((reinterpret_cast<uintptr_t>(x) | reinterpret_cast<uintptr_t>(y)) & 0xF)) {
		while (i <= (size-256)) {
			acc0 = _mm_setzero_ps();
			acc1 = _mm_setzero_ps();
			for (j=0; j<32; j++) {
				r0 = _mm_load_ps(x+i);
				r1 = _mm_load_ps(y+i);
				r2 = _mm_load_ps(x+i+4);
				r3 = _mm_load_ps(y+i+4);
				r0 = _mm_sub_ps(r0 , xmu);
				r1 = _mm_sub_ps(r1 , ymu);
				r2 = _mm_sub_ps(r2 , xmu);
				r3 = _mm_sub_ps(r3 , ymu);
				r0 = _mm_mul_ps(r0 , r1);
				r2 = _mm_mul_ps(r2 , r3);
				acc0 = _mm_add_ps(acc0 , r0);
				acc1 = _mm_add_ps(acc1 , r2);
				i += 8;
			}
			acc0 = _mm_add_ps(acc0 , acc1);
			_mm_storeu_ps(res , acc0);
			a += static_cast<double>(res[0]);
			a += static_cast<double>(res[1]);
			a += static_cast<double>(res[2]);
			a += static_cast<double>(res[3]);
		}
		acc0 = _mm_setzero_ps();
		acc1 = _mm_setzero_ps();
		while (i <= (size-8)) {
			r0 = _mm_load_ps(x+i);
			r1 = _mm_load_ps(y+i);
			r2 = _mm_load_ps(x+i+4);
			r3 = _mm_load_ps(y+i+4);
			r0 = _mm_sub_ps(r0 , xmu);
			r1 = _mm_sub_ps(r1 , ymu);
			r2 = _mm_sub_ps(r2 , xmu);
			r3 = _mm_sub_ps(r3 , ymu);
			r0 = _mm_mul_ps(r0 , r1);
			r2 = _mm_mul_ps(r2 , r3);
			acc0 = _mm_add_ps(acc0 , r0);
			acc1 = _mm_add_ps(acc1 , r2);
			i += 8;
		}		
	}
	else { 
		while (i <= (size-256)) {
			acc0 = _mm_setzero_ps();
			acc1 = _mm_setzero_ps();
			for (j=0; j<32; j++) {
				r0 = _mm_loadu_ps(x+i);
				r1 = _mm_loadu_ps(y+i);
				r2 = _mm_loadu_ps(x+i+4);
				r3 = _mm_loadu_ps(y+i+4);
				r0 = _mm_sub_ps(r0 , xmu);
				r1 = _mm_sub_ps(r1 , ymu);
				r2 = _mm_sub_ps(r2 , xmu);
				r3 = _mm_sub_ps(r3 , ymu);
				r0 = _mm_mul_ps(r0 , r1);
				r2 = _mm_mul_ps(r2 , r3);
				acc0 = _mm_add_ps(acc0 , r0);
				acc1 = _mm_add_ps(acc1 , r2);
				i += 8;
			}
			acc0 = _mm_add_ps(acc0 , acc1);
			_mm_storeu_ps(res , acc0);
			a += static_cast<double>(res[0]);
			a += static_cast<double>(res[1]);
			a += static_cast<double>(res[2]);
			a += static_cast<double>(res[3]);
		}
		acc0 = _mm_setzero_ps();
		acc1 = _mm_setzero_ps();
		while (i <= (size-8)) {
			r0 = _mm_loadu_ps(x+i);
			r1 = _mm_loadu_ps(y+i);
			r2 = _mm_loadu_ps(x+i+4);
			r3 = _mm_loadu_ps(y+i+4);
			r0 = _mm_sub_ps(r0 , xmu);
			r1 = _mm_sub_ps(r1 , ymu);
			r2 = _mm_sub_ps(r2 , xmu);
			r3 = _mm_sub_ps(r3 , ymu);
			r0 = _mm_mul_ps(r0 , r1);
			r2 = _mm_mul_ps(r2 , r3);
			acc0 = _mm_add_ps(acc0 , r0);
			acc1 = _mm_add_ps(acc1 , r2);
			i += 8;
		}
	}
	while (i < size) {a += static_cast<double>((x[i] - xm)*(y[i] - ym)); i++;}
	acc0 = _mm_add_ps(acc0 , acc1);
	_mm_storeu_ps(res , acc0);
	a += static_cast<double>(res[0]);
	a += static_cast<double>(res[1]);
	a += static_cast<double>(res[2]);
	a += static_cast<double>(res[3]);
#endif
	return static_cast<float>(a)/static_cast<float>(size);
}

// return pearson's product-moment correlation
float VectorFunctions::cpears(float* x, float* y, int size)
{
	float tx,ty, xm = mean(x,size), ym = mean(y,size);
	double a=0, b=0, c=0; int i=0;
	
#ifdef ICSTLIB_NO_SSEOPT
	for (i=0; i<size; i++) {
		tx = x[i] - xm; ty = y[i] - ym;
		a += static_cast<double>(tx*ty);
		b += static_cast<double>(tx*tx);
		c += static_cast<double>(ty*ty);
	}
#else
	int j; float res[12];
	__m128 xmu = _mm_set1_ps(xm);
	__m128 ymu = _mm_set1_ps(ym);
	__m128 r0,r1,r2,acc0,acc1,acc2;
	if (!((reinterpret_cast<uintptr_t>(x) | reinterpret_cast<uintptr_t>(y)) & 0xF)) {
		while (i <= (size-256)) {
			acc0 = _mm_setzero_ps();
			acc1 = _mm_setzero_ps();
			acc2 = _mm_setzero_ps();
			for (j=0; j<64; j++) {
				r0 = _mm_load_ps(x+i);
				r1 = _mm_load_ps(y+i);
				r0 = _mm_sub_ps(r0 , xmu);
				r1 = _mm_sub_ps(r1 , ymu);
				r2 = _mm_mul_ps(r0 , r0);
				r0 = _mm_mul_ps(r0 , r1);
				r1 = _mm_mul_ps(r1 , r1);
				acc2 = _mm_add_ps(acc2 , r2);
				acc0 = _mm_add_ps(acc0 , r0);
				acc1 = _mm_add_ps(acc1 , r1);
				i += 4;
			}
			_mm_storeu_ps(res , acc0);
			_mm_storeu_ps(res+4 , acc2);
			_mm_storeu_ps(res+8 , acc1);
			a += static_cast<double>(res[0]);
			b += static_cast<double>(res[4]);
			c += static_cast<double>(res[8]);
			a += static_cast<double>(res[1]);
			b += static_cast<double>(res[5]);
			c += static_cast<double>(res[9]);
			a += static_cast<double>(res[2]);
			b += static_cast<double>(res[6]);
			c += static_cast<double>(res[10]);
			a += static_cast<double>(res[3]);
			b += static_cast<double>(res[7]);
			c += static_cast<double>(res[11]);
		}
		acc0 = _mm_setzero_ps();
		acc1 = _mm_setzero_ps();
		acc2 = _mm_setzero_ps();		
		while (i <= (size-4)) {
			r0 = _mm_load_ps(x+i);
			r1 = _mm_load_ps(y+i);
			r0 = _mm_sub_ps(r0 , xmu);
			r1 = _mm_sub_ps(r1 , ymu);
			r2 = _mm_mul_ps(r0 , r0);
			r0 = _mm_mul_ps(r0 , r1);
			r1 = _mm_mul_ps(r1 , r1);
			acc2 = _mm_add_ps(acc2 , r2);
			acc0 = _mm_add_ps(acc0 , r0);
			acc1 = _mm_add_ps(acc1 , r1);
			i += 4;
		}
	}
	else {
		while (i <= (size-256)) {
			acc0 = _mm_setzero_ps();
			acc1 = _mm_setzero_ps();
			acc2 = _mm_setzero_ps();
			for (j=0; j<64; j++) {
				r0 = _mm_loadu_ps(x+i);
				r1 = _mm_loadu_ps(y+i);
				r0 = _mm_sub_ps(r0 , xmu);
				r1 = _mm_sub_ps(r1 , ymu);
				r2 = _mm_mul_ps(r0 , r0);
				r0 = _mm_mul_ps(r0 , r1);
				r1 = _mm_mul_ps(r1 , r1);
				acc2 = _mm_add_ps(acc2 , r2);
				acc0 = _mm_add_ps(acc0 , r0);
				acc1 = _mm_add_ps(acc1 , r1);
				i += 4;
			}
			_mm_storeu_ps(res , acc0);
			_mm_storeu_ps(res+4 , acc2);
			_mm_storeu_ps(res+8 , acc1);
			a += static_cast<double>(res[0]);
			b += static_cast<double>(res[4]);
			c += static_cast<double>(res[8]);
			a += static_cast<double>(res[1]);
			b += static_cast<double>(res[5]);
			c += static_cast<double>(res[9]);
			a += static_cast<double>(res[2]);
			b += static_cast<double>(res[6]);
			c += static_cast<double>(res[10]);
			a += static_cast<double>(res[3]);
			b += static_cast<double>(res[7]);
			c += static_cast<double>(res[11]);
		}
		acc0 = _mm_setzero_ps();
		acc1 = _mm_setzero_ps();
		acc2 = _mm_setzero_ps();		
		while (i <= (size-4)) {
			r0 = _mm_loadu_ps(x+i);
			r1 = _mm_loadu_ps(y+i);
			r0 = _mm_sub_ps(r0 , xmu);
			r1 = _mm_sub_ps(r1 , ymu);
			r2 = _mm_mul_ps(r0 , r0);
			r0 = _mm_mul_ps(r0 , r1);
			r1 = _mm_mul_ps(r1 , r1);
			acc2 = _mm_add_ps(acc2 , r2);
			acc0 = _mm_add_ps(acc0 , r0);
			acc1 = _mm_add_ps(acc1 , r1);
			i += 4;
		}
	}
	while (i < size) {
		tx = x[i] - xm; ty = y[i] - ym;
		a += static_cast<double>(tx*ty);
		b += static_cast<double>(tx*tx);
		c += static_cast<double>(ty*ty);
		i++;
	}
	_mm_storeu_ps(res , acc0);
	_mm_storeu_ps(res+4 , acc2);
	_mm_storeu_ps(res+8 , acc1);
	a += static_cast<double>(res[0]);
	b += static_cast<double>(res[4]);
	c += static_cast<double>(res[8]);
	a += static_cast<double>(res[1]);
	b += static_cast<double>(res[5]);
	c += static_cast<double>(res[9]);
	a += static_cast<double>(res[2]);
	b += static_cast<double>(res[6]);
	c += static_cast<double>(res[10]);
	a += static_cast<double>(res[3]);
	b += static_cast<double>(res[7]);
	c += static_cast<double>(res[11]);	
#endif
	b = sqrt(b*c); 
	return (b >= static_cast<double>(FLT_MIN)) ? static_cast<float>(a/b) : 0;
}

// return spearman's rank correlation rho
float VectorFunctions::cspear(float* x, float* y, int size)
{
	if (size < 2) {return 0;}
	int i; float tmp, c=0; double sum=0;
	float* tx; tx = new float[size];
	float* ty; ty = new float[size];
	int* idx; idx = new int[size];
	copy(tx,x,size);
	isort(tx,idx,size);
	for (i=0; i<size; i++) {ty[i] = y[idx[i]];}
	isort(ty,idx,size);
	for (i=0; i<size; i++) {
		tmp = static_cast<float>(idx[i]) - c;
		sum += static_cast<double>(tmp*tmp);
		c += 1.0f;
	}
	delete[] tx; delete[] ty; delete[] idx;
	tmp = static_cast<float>(size);
	return 1.0f - 6.0f*static_cast<float>(sum)/(tmp*(tmp*tmp - 1.0f));
}

// return kendall's rank correlation tau
// complexity = O(n*n), consider cspear for large size
// (optimization note: faster algorithms with O(nlogn) exist)
float VectorFunctions::ckend(float* x, float* y, int size)
{
	if (size < 2) {return 0;}
	int i,j; float xr,yr; double sum=0;
	union {float a; int tmp;};
	for (i=0; i<(size-1); i++) {
		xr = x[i]; yr = y[i];
		for (j=i+1; j<size; j++) {
			a = (x[j] - xr)*(y[j] - yr);
			tmp = (tmp & 0x80000000) | 0x3f800000;		// fast sgn(a)
			sum += static_cast<double>(a);
		}
	}
	a = static_cast<float>(size);
	return 2.0f*static_cast<float>(sum)/(a*a - a);
}

// linear regression
// get weights c so that y is approximated by yest in a least-square sense
// yest[0..size-1] = c[0] + sum(i=1..n){c[i]*x[(i-1)*size..i*size-1]}
// replace y by residual y - yest
// return [coefficient of determination, model efficiency]
// if p not NULL:	p-value of the hypothesis that all c are 0 -> p[0]
//					f-value of the underlying f-statistic -> p[1]
// if sdc not NULL: standard deviation of c[0..n] -> sdc[0..n]
// if cov[(n+1)*(n+1)] not NULL: if cov[0] <= 0: save c covariance matrix
//								 if cov[0] > 0: use c covariance matrix
Complex VectorFunctions::linreg(	float* c, float* x, float* y, int size, int n,
					float* p, float* sdc, float* cov				)
{
	n++;
	float* m = new float[n*n];
	float* a = new float[n];
	int i,j,v,q; float df1,df2,tmp,tmp2; Complex r;
	bool usecov=false, savecov=false;
	if (cov) {savecov = !(usecov = (cov[0] > 0));}

	// calculate regression coefficients
	if (usecov) {// use existing c covariance matrix
		copy(m,cov,n*n);
		a[0] = sum(y,size);
		for (i=1,q=0; i<n; i++,q+=size) {
			a[i] = dotp(x+q,y,size);
		}
	}
	else {// compute new c covariance matrix
		a[0] = sum(y,size);
		m[0] = static_cast<float>(size);
		for (j=1; j<n; j++) {
			m[j] = m[j*n] = sum(x+(j-1)*size,size);
		}
		for (i=1; i<n; i++) {
			q = (i-1)*size;
			v = i*(n+1);
			a[i] = dotp(x+q,y,size);
			m[v] = energy(x+q,size);
			for (j=1; j<(n-i); j++) {
				m[v+j] = m[v+j*n] = dotp(x+q+j*size,x+q,size);
			}	
		}
		if (savecov) {copy(cov,m,n*n);}
	}
	if (sdc) {
		float* invm = new float[n*n];
		mident(invm,n);
		if (minvmulm(invm,m,n,n) != 0) {
			for (i=0; i<n; i++) {sdc[i] = invm[i*(n+1)];}
			mmulv(c,invm,a,n,n);
		}
		else {
			set(sdc,0,n);
			set(c,0,n);
		}
		delete[] invm;
	}
	else {
		if (minvmulm(a, m, n, 1) != 0) {copy(c,a,n);}
		else {set(c,0,n);}
	}

	// calculate coefficient of determination, residual, model efficiency,
	// and (optional) standard deviation of regression coefficients
	r.re = var(y,size);
	sub(y,c[0],size);
	for (i=1,q=0; i<n; i++,q+=size) {mac(y,x+q,-c[i],size);}
	tmp = var(y,size);
	if (r.re > FLT_MIN) {
		tmp2 = __max(tmp/r.re, FLT_MIN);
		r.re = 1.0f - tmp2;
		r.im = -logf(tmp2) - 2.0f*static_cast<float>(n)/static_cast<float>(size);
	}
	else {
		tmp2 = 0;
		r.re = 1.0f;
		r.im = -logf(FLT_MIN);
	}
	if (sdc) {
		if (size > n) {
			tmp *= (static_cast<float>(size - 1)/static_cast<float>(size - n));
			mul(sdc,tmp,n);
			fsqrt(sdc,n);
		}
		else {set(sdc,0,n);}
	}

	// significance test (optional)
	if (p) {
		if ((size > n) && (n > 1)) {
			df1 = static_cast<float>(n-1);
			df2 = static_cast<float>(size - n);
			if (size < 1000000) {
				p[0] = SpecMath::rbeta(tmp2, 0.5f*df2, 0.5f*df1);
				p[1] = df2*(1.0f - tmp2)/__max(df1*tmp2, df2*FLT_MIN);
			}
			else {// rbeta approximation by B. Li + E.B. Martin 2002
				if (tmp2 > 1.0e-6f) {
					p[1] = df2*(1.0f - tmp2)/(df1*tmp2);
					tmp2 = df2 + 0.5f*df1 - 1.0f + 0.166666667f*df1*p[1];
					tmp2 /= (df2 + 0.666666667f*df1*p[1]);
					p[0] = 1.0f - SpecMath::rgamma(0.5f*df1, 0.5f*df1*p[1]*tmp2);
				}
				else {p[0] = 0; p[1] = FLT_MAX;}
			}
		}
		else {p[0] = 0; p[1] = FLT_MAX;}
	}

	delete[] a; delete[] m;
	return r;
}

// fill c with coefficients of least squares polynomial approximation
// ye(x) = sum(i=0..d){c[i]*x^i} of value pairs (x,y)[0..size-1] weighted
// with w[0..size-1], w = NULL: no weights
// calculate residual y - ye -> y
void VectorFunctions::polyfit(float* c, int d, int size, float* x, float* y, float* w)
{
	float* m = new float[(d+1)*(d+1)];
	float* a = new float[d+1];
	float* xn = new float[size];
	float* tmp = new float[size];
	float* tmp2 = new float[2*(d+1)];
	int i,j; float norm;

	// compute coefficients
	copy(tmp,x,size);
	abs(tmp,size);
	norm = 1.0f/tmp[maxi(tmp,size)];
	copy(xn,x,size);
	mul(xn,norm,size);
	if (w) {copy(tmp,w,size);} else {set(tmp,1.0f,size);}
	for (i=0; i<=d; i++) {
		tmp2[i] = sum(tmp,size);
		a[i] = dotp(tmp,y,size);
		mul(tmp,xn,size);
	}
	for (i=d+1; i<(2*d+1); i++) {
		tmp2[i] = sum(tmp,size);
		mul(tmp,xn,size);
	}
	tmp2[2*d+1] = sum(tmp,size);
	for (i=0,j=0; i<=d; i++,j+=(d+1)) {
		copy(m+j,tmp2+i,d+1);
	}
	if (minvmulm(a, m, d+1, 1) != 0) {copy(c,a,d+1);}
	else {set(c,0,d+1);}
	SpecMath::scalepoly(c,norm,d);

	// compute residual
	copy(tmp,x,size);
	polyval(tmp,size,c,d);
	sub(y,tmp,size);

	delete[] tmp2; delete[] tmp; delete[] xn; delete[] a; delete[] m;
}

// create histogram bin[bins] of data d[dsize] with range min..max
// return number of outliers
int VectorFunctions::histogram(float* d, int* bin, int dsize, int bins, 
					   float dmin, float dmax)
{
	int i, outs=0;
	float fbin = static_cast<float>(bins);
	float x, r = fbin/(dmax-dmin);
	for (i=0; i<dsize; i++) {
		x = r*(d[i] - dmin);
		if ((x >= 0) && (x < fbin)) {bin[static_cast<int>(x)]++;}
		else if (x == fbin) {bin[bins-1]++;}
		else {outs++;}
	}
	return outs;
}

// get quantile-quantile pairs by reordering x and y data
// norm=true: x replaced by normally distributed data
void VectorFunctions::qqpairs(float* x, float* y, int size, bool norm)
{
	std::sort(y,y+size);
	if (norm) {
		double delta = 1.0/(static_cast<double>(size) + 0.25);
		double yi = 0.625*delta;
		for (int i=0; i<size; i++) {
			x[i] = SpecMath::probit(static_cast<float>(yi));
			yi += delta;
		}
	}
	else {std::sort(x,x+size);}
}

// find potential outliers using modified z-score algorithm
// score[0..size-1]: outlier if > 0, range -1..inf
// d[0..size-1]: data
// t = detection threshold, default: 1.0
// return number of outliers
int VectorFunctions::outliers(float* score, float* d, int size, float th)
{
	copy(score, d, size);
	sub(score, median(d,size), size);
	abs(score, size);
	float ref = 5.18f*th*median(score,size);
	if (ref > FLT_MIN) {ref = 1.0f/ref;}
	else {set(score,-1.0f,size); return 0;}
	int i, cnt=0;
	for (i=0; i<size; i++) {
		score[i] = ref*score[i] - 1.0f;
		if (score[i] > 0) {cnt++;}
	}
	return cnt;
}

// perform two-sided Grubbs test and return p-value
// d[0..size-1] = normally distributed data
// optional out (idx not NULL): idx[0] = index of farthest value
// optional in (mv not NULL): m[0,1] = mean,variance of d -> faster
float VectorFunctions::grubbs(float* d, int size, int* idx, float* mv)
{
	int i; float x,tmp,m,v;
	float n = static_cast<float>(size);
	float nu = __min(n - 2.0f, 1000000.0f);
	if (mv) {m = mv[0]; v = mv[1];} else {m = mean(d,size); v = var(d,size);}
	i = farthesti(d,m,size);
	if (idx) {idx[0] = i;}
	if (size < 3) {return 0;}
	x = d[i] - m;
	x = x*x*n;
	x = sqrtf((n - 2.0f)*x/(v*(n - 1.0f)*(n - 1.0f) - x));
	tmp = sqrtf(x*x + nu);
	return 2.0f*n*SpecMath::rbeta(0.5f - 0.5f*x/tmp, 0.5f*nu, 0.5f*nu);
}

// perform t-test and return [p-value, Cohen's d]
// xsize,ysize = number of x,y data points
// xmean,ymean = arithmetic mean of x,y: mean(x),mean(y)
// xvar,yvar = unbiased variance of x,y: var(x),var(y)
// onesided = false(true): test for mean(x) - mean(y) =(<=) ref
// ysize = 0: 1-sample or paired 2-sample version
// if t not NULL: t-value -> t[0]
Complex VectorFunctions::ttest(	float ref, int xsize, float xmean, float xvar,
					bool onesided, int ysize, float ymean, float yvar, float* t)
{
	static const float IEPS = 10.0f/sqrtf(FLT_EPSILON);
	float tt,nu,tmp; Complex r;
	float nx = static_cast<float>(__max(xsize,2));
	float ny = static_cast<float>(__max(ysize,2));
	xvar = __max(xvar,FLT_MIN); yvar = __max(yvar,FLT_MIN);
	if (ysize <= 0) { // one-sample or paired two-sample
		nu = nx - 1.0f;
		r.im = (xmean - ref)/sqrtf(xvar);
		tt = r.im*sqrtf(nx);
	}
	else { // unpaired two-sample
		nu = nx + ny - 2.0f;
		r.im = (xmean - ymean - ref)*sqrtf(nu/((nx-1.0f)*xvar + (ny-1.0f)*yvar));
		tt = r.im*sqrtf(nx*ny/(nx + ny));
	}
	if (t) {t[0] = tt;}
	nu = __min(nu, 1000000.0f);
	tmp = fabsf(tt);
	if (onesided) {
		if (tmp < (IEPS*nu)) {
			tmp = tt/sqrtf(tmp*tmp + nu);
			r.re = SpecMath::rbeta(0.5f - 0.5f*tmp, 0.5f*nu, 0.5f*nu);
		}
		else {r.re = (tt > 0) ? 0 : 1.0f;}	
	}
	else {
		if (tmp < (IEPS*nu)) {
			tmp /= sqrtf(tmp*tmp + nu);
			r.re = 2.0f*SpecMath::rbeta(0.5f - 0.5f*tmp, 0.5f*nu, 0.5f*nu);
		}
		else {r.re = 0;}
	}
	return r;
}

// perform f-test and return p-value
// xvar,yvar = unbiased variance of x,y: var(x),var(y)
// onesided = false(true): test for var(x) =(<=) var(y)
// ysize <= 0: known y variance
// use Levene test for size > 1e6 
float VectorFunctions::ftest(float xvar, int xsize, float yvar, int ysize, bool onesided)
{
	float xnu = static_cast<float>(__max(xsize,2) - 1);
	float ynu = static_cast<float>(__max(ysize,2) - 1);
	float tmp;
	if (ysize <= 0) {// special case: known y variance -> chi-square test
		if (onesided) {
			if (yvar < FLT_MIN) {return (xvar < FLT_MIN) ? 0.5f : 0;}
			return 1.0f - SpecMath::rgamma(0.5f*xnu, 0.5f*xnu*xvar/yvar); 
		}
		else {
			if (yvar < FLT_MIN) {return (xvar < FLT_MIN) ? 1.0f : 0;}
			tmp = 2.0f*SpecMath::rgamma(0.5f*xnu, 0.5f*xnu*xvar/yvar);
			return (tmp > 1.0f) ? (2.0f - tmp) : tmp;
		}
	}
	if (onesided) {// normal case: estimated x and y variance -> f test
		if ((xvar < FLT_MIN) && (yvar < FLT_MIN)) {return 0.5f;}
		tmp = yvar*ynu;
		tmp /= (tmp + xvar*xnu);
		return SpecMath::rbeta(tmp, 0.5f*ynu, 0.5f*xnu);
	}
	else {
		if ((xvar < FLT_MIN) && (yvar < FLT_MIN)) {return 1.0f;}
		if (xvar > yvar) {
			tmp = yvar*ynu;
			tmp /= (tmp + xvar*xnu);
			tmp = 2.0f*SpecMath::rbeta(tmp, 0.5f*ynu, 0.5f*xnu);
		}
		else {
			tmp = xvar*xnu;
			tmp /= (tmp + yvar*ynu);
			tmp = 2.0f*SpecMath::rbeta(tmp, 0.5f*xnu, 0.5f*ynu);
		}
		return (tmp > 1.0f) ? (2.0f - tmp) : tmp; 
	}
}

// perform Levene or Brown-Forsythe test and return p-value
// x[0..xsize-1] = x data, y[0..ysize-1] = y data
// onesided = false(true): test for var(x) =(<=) var(y)
// bf = false(true): Levene(Brown-Forsythe) test
float VectorFunctions::levtest(float* x, int xsize, float* y, int ysize,
					  bool onesided, bool bf)
{
	float* zx = new float[xsize];
	float* zy = new float[ysize];
	copy(zx,x,xsize);	copy(zy,y,ysize);
	if (bf) {	sub(zx,median(zx,xsize),xsize);	sub(zy,median(zy,ysize),ysize);	}
	else {		sub(zx,mean(zx,xsize),xsize);	sub(zy,mean(zy,ysize),ysize);	}
	abs(zx,xsize);		abs(zy,ysize);
	Complex r =  ttest(	0, xsize, mean(zx,xsize), var(zx,xsize), onesided,
					ysize, mean(zy,ysize), var(zy,ysize)				);
	delete[] zx; delete[] zy;
	return r.re;
}

// perform chi-square test
// return [p-value, chi-square value]
// fo[0..size-1] = observed frequencies
// fe[0..size-1] = expected frequencies or probabilities
// fe[i] > 0 required for all i
// size = number of bins (< 1e6), degrees of freedom = size - rdf
// yates = true: use yates correction
Complex VectorFunctions::chi2test(float* fo, float* fe, int size, int rdf, bool yates)
{
	int i; float t,df, c=0; Complex r;
	mul(fe,sum(fo,size)/sum(fe,size),size);
	if (yates) {
		for (i=0; i<size; i++) {
			t = fabsf(fe[i] - fo[i]) - 0.5f;
			c += (t*t/fe[i]);
		}
	}
	else {
		for (i=0; i<size; i++) {
			t = fe[i] - fo[i];
			c += (t*t/fe[i]);
		}
	}
	r.im = c;
	df = static_cast<float>(__max(size-rdf, 1));
	r.re = 1.0f - SpecMath::rgamma(0.5f*df, 0.5f*c);
	return r;
}

// perform binomial test and return p-value
// pa = probability of category A
// asize = number of category A elements
// tsize = sample size (< 1e6)
// onesided =	true:	return [P(asize <= X)], exact value
//				false:	return [min(1, 2*min(P(asize <= X), P(asize >= X)))],
//						a conservative estimate that is quite good if the
//						distribution is approximately symmetrical around the
//						mean, which is the case for tsize*pa > 5
float VectorFunctions::binomtest(float pa, int asize, int tsize, bool onesided)
{
	float asz = static_cast<float>(asize);
	float tsz = static_cast<float>(tsize);
	if (asize > tsize) {return 0;}
	if (onesided) {
		if (asize <= 0) {return 1.0f;}
		return SpecMath::rbeta(pa, asz, tsz - asz + 1.0f);
	}
	else {
		if (asz > (pa*tsz)) {
			return __min(1.0f, 2.0f*SpecMath::rbeta(pa, asz, tsz - asz + 1.0f));
		}
		else {
			if (asize == tsize) {return 1.0f;}
			return __min(1.0f, 2.0f*SpecMath::rbeta(1.0f - pa, tsz - asz, asz + 1.0f));
		}
	}
}

// return cumulative distribution function of the normal distribution
float VectorFunctions::cdfn(float x, float mean, float std)
{
	static const float scl = 1.0f/sqrtf(2.0f);
	if (std > 0) {return 0.5f*SpecMath::erfcf(scl*(mean - x)/std);}
	else {
		if (x > mean) {return 1.0f;}
		else if (x < mean) {return 0;}
		else {return 0.5f;}
	}
}

// return cumulative distribution function complement of the normal distribution
float VectorFunctions::cdfcn(float x, float mean, float std)
{
	static const float scl = 1.0f/sqrtf(2.0f);
	if (std > 0) {return 0.5f*SpecMath::erfcf(scl*(x - mean)/std);}
	else {
		if (x > mean) {return 0;}
		else if (x < mean) {return 1.0f;}
		else {return 0.5f;}
	}
}

// return a-level confidence interval of normally distributed data -> [xlo, xhi]
Complex VectorFunctions::civn(float mean, float std, float a)
{
	Complex r;
	float x = std*SpecMath::probit(0.5f*(a + 1.0f));
	r.re = mean - x; r.im = mean + x;
	return r;
}

//******************************************************************************
//* storage
//*
// save d to file, return number of elements written or -1 if no access
// append = true: append data if file already exists
int VectorFunctions::save(float* d, int size, char* filename, bool append)
{
	FILE* file; int count;
	if (filename[0] == 0) return -1;
	if (append) {if ((file = fopen(filename,"a+b")) == NULL) return -1;}
	else {if ((file = fopen(filename,"wb")) == NULL) return -1;}
	count = fwrite(d, sizeof(float), __max(size,0), file);
	fclose(file);
	return count;
}

// fill d with file data, start reading at an offset specified in elements
// return number of elements read or -1 if no access
int VectorFunctions::load(float* d, int size, char* filename, int offset)
{
	FILE* file; int count;
	if (filename[0] == 0) return -1;
	if ((file = fopen(filename,"rb")) == NULL) return -1;
	if (fseek(file, offset*sizeof(float), SEEK_SET) != 0) {
		fclose(file); return -1;}
	count = fread(d, sizeof(float), __max(size,0), file);
	fclose(file);
	return count;
}

// return number of elements in file or -1 if no access
int VectorFunctions::getsize(char* filename)
{
	FILE* file; int count;
	if (filename[0] == 0) return -1;
	if ((file = fopen(filename,"rb")) == NULL) return -1;
	if (fseek(file, 0, SEEK_END) != 0) return -1;
	count = ftell(file);
	fclose(file);
	if (count < 0) {return -1;} else {return count/sizeof(float);}
}
			
//******************************************************************************
//* auxiliary functions
//*
// return closest power of 2 greater or equal to i
int VectorFunctions::nexthipow2(int i) {int x=1; while (x < i) {x <<= 1;} return x;}

// allocate array[size] aligned for unconstrained SSE support
// return pointer to 1st element
float* VectorFunctions::sseallocf(int size)
{
#ifndef ICSTLIB_NO_SSEOPT 	
	return reinterpret_cast<float*>(_mm_malloc(size*sizeof(float), 16));
#else
	return reinterpret_cast<float*>(malloc(size*sizeof(float)));
#endif	
}
double* VectorFunctions::sseallocd(int size)
{
#ifndef ICSTLIB_NO_SSEOPT 	
	return reinterpret_cast<double*>(_mm_malloc(size*sizeof(double), 16));
#else
	return reinterpret_cast<double*>(malloc(size*sizeof(double)));
#endif	
}
int* VectorFunctions::ssealloci(int size)
{
#ifndef ICSTLIB_NO_SSEOPT 	
	return reinterpret_cast<int*>(_mm_malloc(size*sizeof(int), 16));
#else
	return reinterpret_cast<int*>(malloc(size*sizeof(int)));
#endif	
}
#ifndef ICSTLIB_NO_SSEOPT
	__m128* VectorFunctions::sseallocm128(int size) {
		return reinterpret_cast<__m128*>(_mm_malloc(size*sizeof(__m128), 16));}
	__m128d* VectorFunctions::sseallocm128d(int size) {
		return reinterpret_cast<__m128d*>(_mm_malloc(size*sizeof(__m128d), 16));}
	__m128i* VectorFunctions::sseallocm128i(int size) {
		return reinterpret_cast<__m128i*>(_mm_malloc(size*sizeof(__m128i), 16));}
#endif

// free SSE-supporting array
void VectorFunctions::ssefree(void* p)
{
#ifndef ICSTLIB_NO_SSEOPT 	
	_mm_free(p);
#else
	free(p);
#endif		
}

//******************************************************************************
//* internal functions
//*
// return n given x = 2^n, based on De Bruijn sequence
int VectorFunctions::twopownton(int x)
{
	static const int tab[32] = {0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25,
		17, 4, 8, 31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9	};
	return tab[static_cast<unsigned int>(0x077cb531*x) >> 27];
}

// helper method for isort (the actual quicksort algorithm)
void VectorFunctions::qs(float* d, int* idx, int hi, int lo)
{
	float x,tf; int i,j,ti;
	int diff = hi-lo;
	if (diff > 1) {
		x=d[lo]; i=lo-1; j=hi+1;
		while (1) {
			do {j--;} while (x > d[j]);
			do {i++;} while (x < d[i]);
			if (i < j) { 
				tf=d[i]; d[i]=d[j]; d[j]=tf;
				ti=idx[i]; idx[i]=idx[j]; idx[j]=ti;
			}
			else break;
		}    
		qs(d,idx,j,lo); qs(d,idx,hi,j+1);
	}
	else if (diff == 1) {	// special case treatment -> performance +10%
		if (d[lo] < d[hi]) {	
			tf = d[lo]; d[lo]=d[hi]; d[hi]=tf;
			ti=idx[lo]; idx[lo]=idx[hi]; idx[hi]=ti;
		}
	}
}

// lu decomposition of matrix a(n,n), return determinant
// uses Crout's algorithm with scaled partial pivoting 
float VectorFunctions::lu(float* a, int n, int* idx)
{
	int i,j,k,m,p, piv=0;
	float x,y,z, det=1.0f;
	float* re; re = new float[n];
	
	for (j=0,i=0; i<n; i++,j+=n) {re[i] = energy(a+j,n);}
	for (i=0; i<n; i++) {
		for (p=0,j=0; j<i; j++,p+=n) {
			x = a[p+i];
			for (m=i,k=p; k<(p+j); k++,m+=n) {x -= (a[k]*a[m]);}
			a[p+i] = x;
		}
		y = z = 0;
		for (p=i*n+i, j=i; j<n; j++,p+=n) {
			x = a[p];
			for (m=i, k=p-i; k<p; k++,m+=n) {x -= (a[k]*a[m]);}
			a[p] = x;
			x *= x;
			if ((x*z) >= (y*re[j])) {y=x; z = re[j]; piv=j;}
		}											
		if (idx) {idx[i] = piv;}					
		if (i != piv) {
			re[piv] = re[i];
			det *= -1.0f;
			m = n*(piv-i);
			for (j=i*n; j<(i*n+n); j++) {
				x = a[j+m];
				a[j+m] = a[j];
				a[j] = x;
			}	
		}
		p = i*n + i;
		x = a[p];
		det *= x;
		if (fabsf(det) < FLT_MIN) {delete[] re; return 0;}
		x = 1.0f/x;
		for (k=p+n, j=i+1; j<n; j++,k+=n) {a[k] *= x;}
	}

	delete[] re;
	return det;
}

// modified bessel function of the first kind, taken from Ooura's math packages
// license as found in the readme file, 19.8.08: *** Copyright(C) 1996 Takuya OOURA
// (email: ooura@mmm.t.u-tokyo.ac.jp). You may use, copy, modify this code for any
// purpose and without fee. You may distribute this ORIGINAL package. ***
// remarks: the code below is unmodified, it differs from the original package only
// in that the latter contains some additional functions, if float precision is
// sufficient, faster approximations are found readily based on Abramovitz/Stegun
double VectorFunctions::bessi0(double x)
{
    int k;
    double w, t, y;
    const static double a[65] = {
        8.5246820682016865877e-11, 2.5966600546497407288e-9, 
        7.9689994568640180274e-8, 1.9906710409667748239e-6, 
        4.0312469446528002532e-5, 6.4499871606224265421e-4, 
        0.0079012345761930579108, 0.071111111109207045212, 
        0.444444444444724909, 1.7777777777777532045, 
        4.0000000000000011182, 3.99999999999999998, 
        1.0000000000000000001, 
        1.1520919130377195927e-10, 2.2287613013610985225e-9, 
        8.1903951930694585113e-8, 1.9821560631611544984e-6, 
        4.0335461940910133184e-5, 6.4495330974432203401e-4, 
        0.0079013012611467520626, 0.071111038160875566622, 
        0.44444450319062699316, 1.7777777439146450067, 
        4.0000000132337935071, 3.9999999968569015366, 
        1.0000000003426703174, 
        1.5476870780515238488e-10, 1.2685004214732975355e-9, 
        9.2776861851114223267e-8, 1.9063070109379044378e-6, 
        4.0698004389917945832e-5, 6.4370447244298070713e-4, 
        0.0079044749458444976958, 0.071105052411749363882, 
        0.44445280640924755082, 1.7777694934432109713, 
        4.0000055808824003386, 3.9999977081165740932, 
        1.0000004333949319118, 
        2.0675200625006793075e-10, -6.1689554705125681442e-10, 
        1.2436765915401571654e-7, 1.5830429403520613423e-6, 
        4.2947227560776583326e-5, 6.3249861665073441312e-4, 
        0.0079454472840953930811, 0.070994327785661860575, 
        0.44467219586283000332, 1.7774588182255374745, 
        4.0003038986252717972, 3.9998233869142057195, 
        1.0000472932961288324, 
        2.7475684794982708655e-10, -3.8991472076521332023e-9, 
        1.9730170483976049388e-7, 5.9651531561967674521e-7, 
        5.1992971474748995357e-5, 5.7327338675433770752e-4, 
        0.0082293143836530412024, 0.069990934858728039037, 
        0.44726764292723985087, 1.7726685170014087784, 
        4.0062907863712704432, 3.9952750700487845355, 
        1.0016354346654179322
    };
    const static double b[70] = {
        6.7852367144945531383e-8, 4.6266061382821826854e-7, 
        6.9703135812354071774e-6, 7.6637663462953234134e-5, 
        7.9113515222612691636e-4, 0.0073401204731103808981, 
        0.060677114958668837046, 0.43994941411651569622, 
        2.7420017097661750609, 14.289661921740860534, 
        59.820609640320710779, 188.78998681199150629, 
        399.8731367825601118, 427.56411572180478514, 
        1.8042097874891098754e-7, 1.2277164312044637357e-6, 
        1.8484393221474274861e-5, 2.0293995900091309208e-4, 
        0.0020918539850246207459, 0.019375315654033949297, 
        0.15985869016767185908, 1.1565260527420641724, 
        7.1896341224206072113, 37.354773811947484532, 
        155.80993164266268457, 489.5211371158540918, 
        1030.9147225169564806, 1093.5883545113746958, 
        4.8017305613187493564e-7, 3.261317843912380074e-6, 
        4.9073137508166159639e-5, 5.3806506676487583755e-4, 
        0.0055387918291051866561, 0.051223717488786549025, 
        0.42190298621367914765, 3.0463625987357355872, 
        18.895299447327733204, 97.915189029455461554, 
        407.13940115493494659, 1274.3088990480582632, 
        2670.9883037012547506, 2815.7166284662544712, 
        1.2789926338424623394e-6, 8.6718263067604918916e-6, 
        1.3041508821299929489e-4, 0.001428224737372747892, 
        0.014684070635768789378, 0.13561403190404185755, 
        1.1152592585977393953, 8.0387088559465389038, 
        49.761318895895479206, 257.2684232313529138, 
        1066.8543146269566231, 3328.3874581009636362, 
        6948.8586598121634874, 7288.4893398212481055, 
        3.409350368197032893e-6, 2.3079025203103376076e-5, 
        3.4691373283901830239e-4, 0.003794994977222908545, 
        0.038974209677945602145, 0.3594948380414878371, 
        2.9522878893539528226, 21.246564609514287056, 
        131.28727387146173141, 677.38107093296675421, 
        2802.3724744545046518, 8718.5731420798254081, 
        18141.348781638832286, 18948.925349296308859
    };
    const static double c[45] = {
        2.5568678676452702768e-15, 3.0393953792305924324e-14, 
        6.3343751991094840009e-13, 1.5041298011833009649e-11, 
        4.4569436918556541414e-10, 1.746393051427167951e-8, 
        1.0059224011079852317e-6, 1.0729838945088577089e-4, 
        0.05150322693642527738, 
        5.2527963991711562216e-15, 7.202118481421005641e-15, 
        7.2561421229904797156e-13, 1.482312146673104251e-11, 
        4.4602670450376245434e-10, 1.7463600061788679671e-8, 
        1.005922609132234756e-6, 1.0729838937545111487e-4, 
        0.051503226936437300716, 
        1.3365917359358069908e-14, -1.2932643065888544835e-13, 
        1.7450199447905602915e-12, 1.0419051209056979788e-11, 
        4.58047881980598326e-10, 1.7442405450073548966e-8, 
        1.0059461453281292278e-6, 1.0729837434500161228e-4, 
        0.051503226940658446941, 
        5.3771611477352308649e-14, -1.1396193006413731702e-12, 
        1.2858641335221653409e-11, -5.9802086004570057703e-11, 
        7.3666894305929510222e-10, 1.6731837150730356448e-8, 
        1.0070831435812128922e-6, 1.0729733111203704813e-4, 
        0.051503227360726294675, 
        3.7819492084858931093e-14, -4.8600496888588034879e-13, 
        1.6898350504817224909e-12, 4.5884624327524255865e-11, 
        1.2521615963377513729e-10, 1.8959658437754727957e-8, 
        1.0020716710561353622e-6, 1.073037119856927559e-4, 
        0.05150322383300230775
    };

    w = fabs(x);
    if (w < 8.5) {
        t = w * w * 0.0625;
        k = 13 * static_cast<int>(t);
        y = (((((((((((a[k] * t + a[k + 1]) * t + 
            a[k + 2]) * t + a[k + 3]) * t + a[k + 4]) * t + 
            a[k + 5]) * t + a[k + 6]) * t + a[k + 7]) * t + 
            a[k + 8]) * t + a[k + 9]) * t + a[k + 10]) * t + 
            a[k + 11]) * t + a[k + 12];
    } else if (w < 12.5) {
        k = static_cast<int>(w);
        t = w - k;
        k = 14 * (k - 8);
        y = ((((((((((((b[k] * t + b[k + 1]) * t + 
            b[k + 2]) * t + b[k + 3]) * t + b[k + 4]) * t + 
            b[k + 5]) * t + b[k + 6]) * t + b[k + 7]) * t + 
            b[k + 8]) * t + b[k + 9]) * t + b[k + 10]) * t + 
            b[k + 11]) * t + b[k + 12]) * t + b[k + 13];
    } else {
        t = 60 / w;
        k = 9 * static_cast<int>(t);
        y = ((((((((c[k] * t + c[k + 1]) * t + 
            c[k + 2]) * t + c[k + 3]) * t + c[k + 4]) * t + 
            c[k + 5]) * t + c[k + 6]) * t + c[k + 7]) * t + 
            c[k + 8]) * sqrt(t) * exp(w);
    }
    return y;
}

//******************************************************************************
//* circular buffer
//*
CircBuffer::CircBuffer(int size)
{
	if (size > 0) {
		try {data = new float[size+1]; bsize = size+1;} catch(...) {data = NULL;}
		if (data == NULL) {bsize = 0;}
	}
	else {data = NULL; bsize = 0;}
	rptr = wptr = 0;
	for (int i=0; i<bsize; i++) {data[i]=0;}
}

CircBuffer::~CircBuffer() {if (data) {delete[] data;}}

// reset pointers, zero buffer
void CircBuffer::Reset()
{
	rptr = wptr = 0;
	for (int i=0; i<bsize; i++) {data[i]=0;}
}

// write n elements
bool CircBuffer::Write(float* d, int n)
{
	int nwptr = wptr + n;
	if (nwptr < bsize) {
		if ((rptr > wptr) && (rptr < nwptr)) {return false;}
		else {
			memcpy(data+wptr,d,n*sizeof(float));
			wptr = nwptr;
		}
	}
	else {
		nwptr -= bsize;
		if ((rptr > wptr) || (rptr < nwptr)) {return false;}
		else {
			memcpy(data+wptr,d,(bsize-wptr)*sizeof(float));
			memcpy(data,d,nwptr*sizeof(float));
			wptr = nwptr;
		}
	}
	return true;
}

// read n elements
bool CircBuffer::Read(float* d, int n)
{
	int nrptr = rptr + n;
	if (nrptr < bsize) {
		if ((wptr >= rptr) && (wptr < nrptr)) {return false;}
		else {
			memcpy(d,data+rptr,n*sizeof(float));
			rptr = nrptr;
		}
	}
	else {
		nrptr -= bsize;
		if ((wptr >= rptr) || (wptr < nrptr)) {return false;}
		else {
			memcpy(d,data+rptr,(bsize-rptr)*sizeof(float));
			memcpy(d,data,nrptr*sizeof(float));
			rptr = nrptr;
		}
	}
	return true;
}

// read n elements at an offset from the current read pointer without
// advancing it
bool CircBuffer::ReadPassive(float* d, int n, int offset)
{
	int start = rptr+offset;
	if (start < 0) {start += bsize;}
	int end = start+n;
	if (end < bsize) {
		if ((wptr >= start) && (wptr < end)) {return false;}
		else {memcpy(d,data+start,n*sizeof(float));}
	}
	else {
		end -= bsize;
		if ((wptr >= start) || (wptr < end)) {return false;}
		else {
			memcpy(d,data+start,(bsize-start)*sizeof(float));
			memcpy(d,data,end*sizeof(float));
		}
	}
	return true;
}

// return maximum number of elements to read without overflow
int CircBuffer::GetReadSize()
{
	int x = wptr - rptr;
	if (x < 0) {x += bsize;}
	return x;
}

// return maximum number of elements to write without overflow
int CircBuffer::GetWriteSize()
{
	int x = rptr - wptr - 1;
	if (x < 0) {x += bsize;}
	return x;
}

