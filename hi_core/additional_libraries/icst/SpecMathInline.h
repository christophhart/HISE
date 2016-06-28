// SpecMath.h
// inline methods for SpecMath
//
// This code is part of the ICST DSP Library version 1.2. It is released
// under the 2-clause BSD license. Copyright (c) 2008-2010, Zurich
// University of the Arts, Beat Frei. All rights reserved.

#ifndef _ICST_DSPLIB_SPECMATHINLINE_INCLUDED
#define _ICST_DSPLIB_SPECMATHINLINE_INCLUDED

namespace icstdsp {		// begin library specific namespace

// quick and dirty atan2, if the general shape of atan matters more than values,
// suitable as a nonlinearity where an unlimited input must be limited by a
// function that is differentiable everywhere within a half-plane
// abs error < 0.072, quad version: x[0..3] = qdatan2f(y[0..3], x[0..3])
inline float SpecMath::qdatan2f(float y, float x)
{
	static const float PIDIV2 = 0.5f*3.14159265f;
#ifdef ICSTLIB_NO_SSEOPT 
	float z = PIDIV2*(1.0f - x/(fabsf(y) + fabsf(x) + FLT_MIN));
	return (y < 0) ? -z : z;
#else
	float rvar;
	__m128 r0,r1,r2,r3,r4;
	__m128 absmask = _mm_castsi128_ps(_mm_set_epi32(0,0,0,0x7FFFFFFF));
	__m128 signmask = _mm_castsi128_ps(_mm_set_epi32(0,0,0,0x80000000));
	__m128 pidiv2 = _mm_set_ss(PIDIV2);
	r4 = _mm_set_ss(y);
	r3 = _mm_set_ss(x);
	r0 = _mm_and_ps(r4 , absmask);
	r1 = _mm_and_ps(r3 , absmask);
	r0 = _mm_add_ss(r0 , r1);
	r4 = _mm_and_ps(r4 , signmask);
	r3 = _mm_mul_ss(r3 , pidiv2);
	r0 = _mm_add_ss(r0 , _mm_set_ss(FLT_MIN));
	r1 = _mm_rcp_ss(r0);
	r0 = _mm_mul_ss(r0 , r1);
	r2 = _mm_add_ss(r1 , r1);
	r0 = _mm_mul_ss(r0 , r1);
	r2 = _mm_sub_ss(r2 , r0);
	r2 = _mm_mul_ss(r2 , r3);
	r2 = _mm_sub_ss(pidiv2 , r2);
	r2 = _mm_xor_ps(r4 , r2);
	_mm_store_ss(&rvar , r2);
	return rvar;
#endif
}
inline void SpecMath::qdatan2f(float* y, float* x)
{	
#ifdef ICSTLIB_NO_SSEOPT 
	x[0] = qdatan2f(y[0],x[0]);
	x[1] = qdatan2f(y[1],x[1]);
	x[2] = qdatan2f(y[2],x[2]);
	x[3] = qdatan2f(y[3],x[3]);
#else
	static const float PIDIV2 = 0.5f*3.14159265f;
	__m128 r0,r1,r2,r3,r4;
	__m128 absmask = _mm_castsi128_ps(_mm_set1_epi32(0x7FFFFFFF));
	__m128 signmask = _mm_castsi128_ps(_mm_set1_epi32(0x80000000));
	__m128 pidiv2 = _mm_set1_ps(PIDIV2);
	r4 = _mm_loadu_ps(y);
	r3 = _mm_loadu_ps(x);
	r0 = _mm_and_ps(r4 , absmask);
	r1 = _mm_and_ps(r3 , absmask);
	r0 = _mm_add_ps(r0 , r1);
	r4 = _mm_and_ps(r4 , signmask);
	r3 = _mm_mul_ps(r3 , pidiv2);
	r0 = _mm_add_ps(r0 , _mm_set1_ps(FLT_MIN));
	r1 = _mm_rcp_ps(r0);
	r0 = _mm_mul_ps(r0 , r1);
	r2 = _mm_add_ps(r1 , r1);
	r0 = _mm_mul_ps(r0 , r1);
	r2 = _mm_sub_ps(r2 , r0);
	r2 = _mm_mul_ps(r2 , r3);
	r2 = _mm_sub_ps(pidiv2 , r2);
	r2 = _mm_xor_ps(r4 , r2);
	_mm_storeu_ps(x , r2);
#endif
}	

// quick and dirty tanh, a fair compromise of general shape, shape of derivative,
// and good asymptotic behaviour, suitable as a nonlinearity where an unlimited
// input must be limited by a function that is differentiable everywhere
// abs error < 0.0034 (SSE version) or 0.029 (basic version)
// quad version: x[0..3] = qdtanh(x[0..3])
inline float SpecMath::qdtanh(float x)
{
#ifdef ICSTLIB_NO_SSEOPT 
	float y = x*x;
	if (y <= 6.25f) {
		return (0.913415f + (-0.18436f + (0.024184f - 0.00127468f*y)*y)*y)*x;}
	else {
		y = 1.0f + 0.125f*fabsf(x) + 0.0078125f*y;
		y *= y; y *= y; y *= y; y *= y;
		return (x < 0) ? (1.86033f/y - 1.0f) : (1.0f - 1.86033f/y);
	}
#else
	float rvar;
	__m128 r0,r1,r2;
	r0 = _mm_set_ss(x);
	r1 = _mm_mul_ss(r0 , r0);
	r2 = _mm_mul_ss(r0 , _mm_set_ss(0.187f));
	r1 = _mm_mul_ss(r1 , r2);
	r0 = _mm_add_ss(r0 , r1);
	r2 = _mm_mul_ss(r0 , r0);
	r2 = _mm_add_ss(r2 , _mm_set_ss(1.0f));
	r0 = _mm_mul_ss(r0 , _mm_set_ss(0.5f));
	r1 = _mm_rsqrt_ss(r2);
	r2 = _mm_mul_ss(r2 , r1);
	r2 = _mm_mul_ss(r2 , r1);
	r1 = _mm_mul_ss(r1 , r0);
	r2 = _mm_sub_ss(_mm_set_ss(3.0f) , r2);
	r2 = _mm_mul_ss(r2 , r1);
	_mm_store_ss(&rvar , r2);
	return rvar;
#endif
}
inline void SpecMath::qdtanh(float* x)
{
#ifdef ICSTLIB_NO_SSEOPT 
	x[0] = qdtanh(x[0]);
	x[1] = qdtanh(x[1]);
	x[2] = qdtanh(x[2]);
	x[3] = qdtanh(x[3]);
#else
	__m128 r0,r1,r2;
	r0 = _mm_loadu_ps(x);
	r1 = _mm_mul_ps(r0 , r0);
	r2 = _mm_mul_ps(r0 , _mm_set1_ps(0.187f));
	r1 = _mm_mul_ps(r1 , r2);
	r0 = _mm_add_ps(r0 , r1);
	r2 = _mm_mul_ps(r0 , r0);
	r2 = _mm_add_ps(r2 , _mm_set1_ps(1.0f));
	r0 = _mm_mul_ps(r0 , _mm_set1_ps(0.5f));
	r1 = _mm_rsqrt_ps(r2);
	r2 = _mm_mul_ps(r2 , r1);
	r2 = _mm_mul_ps(r2 , r1);
	r1 = _mm_mul_ps(r1 , r0);
	r2 = _mm_sub_ps(_mm_set1_ps(3.0f) , r2);
	r2 = _mm_mul_ps(r2 , r1);
	_mm_storeu_ps(x , r2);
#endif
}	

// quick and dirty medium range exp
// rel error < 1.2e-4 within recommended input range x = 0..10
// quad version: x[0..3] = qdexp(x[0..3])
inline float SpecMath::qdexp(float x)
{
	x = 0.99999636f+(0.031261316f+(0.00048274797f+0.0000059490530f*x)*x)*x;
	x *= x; x *= x; x *= x; x *= x;
	return x*x;
}
inline void SpecMath::qdexp(float* x)
{
#ifdef ICSTLIB_NO_SSEOPT 
	x[0] = qdexp(x[0]);
	x[1] = qdexp(x[1]);
	x[2] = qdexp(x[2]);
	x[3] = qdexp(x[3]);
#else
	__m128 r0,r1;
	r0 = _mm_loadu_ps(x);
	r1 = _mm_mul_ps(r0 , _mm_set1_ps(0.0000059490530f));
	r1 = _mm_add_ps(r1 , _mm_set1_ps(0.00048274797f));
	r1 = _mm_mul_ps(r1 , r0);
	r1 = _mm_add_ps(r1 , _mm_set1_ps(0.031261316f));
	r1 = _mm_mul_ps(r1 , r0);
	r1 = _mm_add_ps(r1 , _mm_set1_ps(0.99999636f));
	r1 = _mm_mul_ps(r1 , r1);
	r1 = _mm_mul_ps(r1 , r1);
	r1 = _mm_mul_ps(r1 , r1);
	r1 = _mm_mul_ps(r1 , r1);
	r1 = _mm_mul_ps(r1 , r1);
	_mm_storeu_ps(x , r1);
#endif
}	

// fast medium range exp
// rel error < 2.3e-6 within recommended input range x = 0..10
// quad version: x[0..3] = fexp(x[0..3])
inline float SpecMath::fexp(float x)
{
	x = 1.0f + 
		(0.062500309f + 
			(0.0019527233f +
				(4.0866300e-5f + 
					(6.0184965e-7f + 
						1.0856791e-8f*x)*x)*x)*x)*x;
	x *= x; x *= x; x *= x;
	return x*x;
}
inline void SpecMath::fexp(float* x)
{
#ifdef ICSTLIB_NO_SSEOPT 
	x[0] = fexp(x[0]);
	x[1] = fexp(x[1]);
	x[2] = fexp(x[2]);
	x[3] = fexp(x[3]);
#else
	__m128 r0,r1;
	r0 = _mm_loadu_ps(x);
	r1 = _mm_mul_ps(r0 , _mm_set1_ps(1.0856791e-8f));
	r1 = _mm_add_ps(r1 , _mm_set1_ps(6.0184965e-7f));
	r1 = _mm_mul_ps(r1 , r0);
	r1 = _mm_add_ps(r1 , _mm_set1_ps(4.0866300e-5f));
	r1 = _mm_mul_ps(r1 , r0);
	r1 = _mm_add_ps(r1 , _mm_set1_ps(0.0019527233f));
	r1 = _mm_mul_ps(r1 , r0);
	r1 = _mm_add_ps(r1 , _mm_set1_ps(0.062500309f));
	r1 = _mm_mul_ps(r1 , r0);
	r1 = _mm_add_ps(r1 , _mm_set1_ps(1.0f));
	r1 = _mm_mul_ps(r1 , r1);
	r1 = _mm_mul_ps(r1 , r1);
	r1 = _mm_mul_ps(r1 , r1);
	r1 = _mm_mul_ps(r1 , r1);
	_mm_storeu_ps(x , r1);
#endif
}	

// fast rounding double to int conversion, x = -2^31..2^31-1
inline int SpecMath::fdtoi(double x)
{
#ifdef ICSTLIB_DEF_ROUND
	#ifdef ICSTLIB_NO_SSEOPT
		union {volatile double y; int r[2];};
		y = x;
		y += 4503603922337792.0;
		return r[0];
	#else
		return _mm_cvtsd_si32(_mm_load_sd(&x));
	#endif
#else
	#ifdef ICSTLIB_NO_SSEOPT
		return ((x >= 0) ? static_cast<int>(x + 0.5) : static_cast<int>(x - 0.5));
	#else
		__m128d r0 = _mm_load_sd(&x);
		__m128d mask = _mm_castsi128_pd(_mm_set_epi32(0,0,0x80000000,0));
		__m128d r1 = _mm_and_pd(r0 , mask);
		r1 = _mm_xor_pd(r1 , _mm_set_sd(0.5));
		r0 = _mm_add_sd(r0 , r1);
		return _mm_cvttsd_si32(r0);
	#endif
#endif
}

// fast truncating double to int conversion, x = -2^31..2^31-1
inline int SpecMath::ffdtoi(double x)
{
#ifdef ICSTLIB_NO_SSEOPT
	#ifdef ICSTLIB_DEF_ROUND
		union {volatile double y; int r[2];};
		y = fabs(x) - 0.49999999999999988;
		y += 4503603922337792.0;
		return (x < 0) ? -r[0] : r[0];
	#else
		return static_cast<int>(x);
	#endif
#else
	return _mm_cvttsd_si32(_mm_load_sd(&x));
#endif
}

// fast truncating split into integer and fractional part, x = 0..2^31-1
// return integer part, replace x by fractional part (0..1)
inline int SpecMath::fsplit(double &x)
{	
#ifdef ICSTLIB_NO_SSEOPT
	#ifdef ICSTLIB_DEF_ROUND
		union {volatile double y; int r[2];};
		y = 4503603922337792.0;
		y += (x - 0.5);
		x -= static_cast<double>(r[0]);
		return r[0];
	#else
		int y = static_cast<int>(x);
		x -= static_cast<double>(y);
		return y;
	#endif
#else
	int a; __m128d r0,r1;
	r0 = _mm_load_sd(&x);
	a = _mm_cvttsd_si32(r0);
	r1 = _mm_cvtsi32_sd(r0 , a);
	r1 = _mm_sub_sd(r0 , r1);
	_mm_store_sd(&x , r1);
	return a;
#endif
}

// return linear interpolated value of y(x)
// specify y[0..1] = y0,y1 at x = 0,1 and x = 0..1
inline float SpecMath::fitlin(float* y, float x) {return y[0] + x*(y[1]-y[0]);}

// return cubic hermite interpolated value of y(x)
// specify y[0..3] = ym1,y0,y1,y2 at x = -1,0,1,2 and x = 0..1
inline float SpecMath::fitcubic(float* y, float x)
{
	float m0,m1,mm,a;
	m0 = 0.5f*(y[2] - y[0]);
	m1 = 0.5f*(y[3] - y[1]);
	mm = y[2] - y[1];
	a = m0 + m1 - 2.0f*mm;
	return y[1] + (m0 + (mm - m0 - a + a*x)*x)*x;
}

}	// end library specific namespace

#endif

