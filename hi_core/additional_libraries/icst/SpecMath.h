// SpecMath.h
//******************** a collection of useful math methods ***********************
// polynomials are of the form sum(i=0..d){c[i]*x^i} with real coefficients c[i]
// unless otherwise noted
//
// This code is part of the ICST DSP Library version 1.2. It is released
// under the 2-clause BSD license. Copyright (c) 2008-2010, Zurich
// University of the Arts, Beat Frei. All rights reserved.

#ifndef _ICST_DSPLIB_SPECMATH_INCLUDED
#define _ICST_DSPLIB_SPECMATH_INCLUDED

namespace icstdsp {		// begin library specific namespace

class SpecMath
{
//******************************* user methods ***********************************
public:
//***	functions	***
// quick and dirty atan2, if the general shape of atan matters more than values,
// suitable as a nonlinearity where an unlimited input must be limited by a
// function that is differentiable everywhere within a half-plane
// abs error < 0.072, quad version: x[0..3] = qdatan2f(y[0..3], x[0..3])
static float qdatan2f(float y, float x);
static void qdatan2f(float* y, float* x);

// quick and dirty tanh, a fair compromise of general shape, shape of derivative,
// and good asymptotic behaviour, suitable as a nonlinearity where an unlimited
// input must be limited by a function that is differentiable everywhere
// abs error < 0.0034 (SSE version) or 0.029 (basic version)
// quad version: x[0..3] = qdtanh(x[0..3])
static float qdtanh(float x);
static void qdtanh(float* x);

// quick and dirty medium range exp
// rel error < 1.2e-4 within recommended input range x = 0..10
// quad version: x[0..3] = qdexp(x[0..3])
static float qdexp(float x);
static void qdexp(float* x);

// fast medium range exp
// rel error < 2.3e-6 within recommended input range x = 0..10
// quad version: x[0..3] = fexp(x[0..3])
static float fexp(float x);
static void fexp(float* x);

// inverse hyperbolic sine
static float asinhf(float x);
static double asinh(double x);

// inverse hyperbolic cosine
static float acoshf(float x);
static double acosh(double x);

// inverse hyperbolic tangent
static float atanhf(float x);
static double atanh(double x);

// error function
static float erff(float x);
static double erf(double x);

// error function complement
static float erfcf(float x);
static double erfc(double x);

// probit function (inverse CDF of the standard normal distribution)
// x = 0..1
static float probit(float x);

// natural logarithm of the gamma function
// x > 0, float version: rel + abs error < 3e-7
static float gammalnf(float x);
static double gammaln(double x);

// regularized gamma function P(x,y)
// 1e6 > x > 0, y >= 0, return -1 on failure
static float rgamma(float x, float y);

// regularized beta function I(x,a,b)
// x = 0..1, 1e6 > a > 0, 1e6 > b > 0, return -1 on failure
static float rbeta(float x, float a, float b);

// bessel function of the first kind Jn(x)
static float bessj(float x, int n);

// fast rounding double to int conversion, x = -2^31..2^31-1
static int fdtoi(double x);

// fast truncating double to int conversion, x = -2^31..2^31-1
static int ffdtoi(double x);

// fast truncating split into integer and fractional part, x = 0..2^31-1
// return integer part, replace x by fractional part (0..1)
static int fsplit(double &x);

// find root of a function by bisection
// input:	f		=	pointer to the function "func" which has the form
//						"float myfunc(float x, float* p, int n)" with p
//						independent of x
//			p		=	parameters passed to "func"
//			n		=	number of parameters
//			xmax	=	maximum x included in the search
//			xmin	=	minimum x included in the search
//			maxerr	=	maximum absolute error of root,
//						0: highest precision, >0: faster			
// output:	x for which func(x) = 0
// NOTE: xmax and xmin must produce non-zero function values of opposite sign
//		 (otherwise, the function terminates but the root may be incorrect)
static float froot(	float (*f)(float,float*,int), float* p, int n,
					float xmax, float xmin, float maxerr=0			);

//***	polynomials	***
// find complex roots r[re(0) im(0)..re(d-1) im(d-1)] of polynomial with real
// coefficients c[0..d], return root count(-1: pathologic case), very fast!
static int roots(float* c, float* r, int d);
static int roots(double* c, float* r, int d);

// construct polynomial from d complex roots r[re(0) im(0)..re(d-1) im(d-1)]
// -> complex coefficients separated into real set cre[0..d] and imaginary
// set cim[0..d], normalization: cre[d]=1, cim[d]=0 
static void roottops(double* cre, double* cim, float* r, int d);

// symbolic addition of polynomials a[0..da] + b[0..db] -> a[0..max(da,db)]
static void addpoly(float* a, int da, float* b, int db);
static void addpoly(double* a, int da, double* b, int db);

// symbolic subtraction of polynomials a[0..da] - b[0..db] -> a[0..max(da,db)]
static void subpoly(float* a, int da, float* b, int db);
static void subpoly(double* a, int da, double* b, int db);

// symbolic multiplication of polynomial with constant a*c[0..d] -> c[0..d]
static void mulpoly(float* c, float a, int d);
static void mulpoly(double* c, double a, int d);

// symbolic polynomial multiplication a[0..da] x b[0..db] -> a[0..da+db]
static void pmulpoly(float* a, int da, float* b, int db);
static void pmulpoly(double* a, int da, double* b, int db);

// symbolic scaling of polynomial c[0..d] -> a^i*c[0..i..d]
static void scalepoly(float* c, float a, int d);
static void scalepoly(double* c, double a, int d);

// symbolic substitution of polynomials
// replace x of polynomial a[0..da] with polynomial b[0..db] -> a[0..da*db]
static void sspoly(float* a, int da, float* b, int db);
static void sspoly(double* a, int da, double* b, int db);
	
// reverse polynomial c[0..d]
static void revpoly(float* c, int d);
static void revpoly(double* c, int d);

// symbolic differentiation of polynomial c[0..d] -> c[0..d]
static void diffpoly(float* c, int d);
static void diffpoly(double* c, int d);

// symbolic integration of polynomial c[0..d-1] -> c[0..d]
static void integratepoly(float* c, int d);
static void integratepoly(double* c, int d);

// coefficients of d-th degree chebyshev polynomial -> c[0..d]			
static void chebypoly(double* c, int d);
													
// d-th degree chebyshev series to power series coefficients c[0..d] 
static void chebytops(double* c, int d);

// d-th degree power series to chebyshev series coefficients c[0..d]
static void pstocheby(double* c, int d);
											 										
//***	interpolation	***
// return linear interpolated value of y(x)
// specify y[0..1] = y0,y1 at x = 0,1 and x = 0..1
static float fitlin(float* y, float x);

// (re,im)=(xext,yext) of parabola y(x) given y(-1),y(0),y(1), no extremum:(0,y0) 				
static cpx paraext(float ym1, float y0, float y1);	

// fill c with coefficients of y(x) = c[0] + c[1]*x + c[2]*x^2 + c[3]*x^3 such
// that y(x) is an interpolation function for x = x0..x1 that goes through
// (x0,y0) and (x1,y1) with continuous gradients at these points
// 1st version: specify gradients m0 and m1
// 2nd version: specify y[0..3] = ym1,y0,y1,y2 at x[0..3] = xm1,x0,x1,x2
// 3rd version: fast 2nd version with constant interval x[0..3]=(-1,0,1,2)
// 4th version: as 3rd version, returns interpolated value at specified x = 0..1
static void fitcubic(double* c, float x0, float y0, float m0, 
						float x1, float y1, float m1);
static void fitcubic(double* c, float* x, float* y);
static void fitcubic(float* c, float* y);
static float fitcubic(float* y, float x);

// fill c with coefficients of chebychev approximation of tabulated y(-1..1):
// ye(x) = sum(i=0..d){c[i]*T[i](x)}, return error relative to max|y|														
static float chebyapprox(double* c, int d, float* y, int size);

//***	differential equations	***
// approximate deltay using the classic 4th order runge-kutta algorithm so that
// y(x + deltax) = y(x) + deltay given the equation dy/dx = func(y(x),p[0..n-1])
// with p independent of x
// input:	y		=	current value of y(x)
//			deltax	=	integration step
//			f		=	pointer to the function "func" which has the form
//						"float myfunc(float y, float* p, int n)"
//			p		=	parameters passed to "func"
//			n		=	number of parameters
// output:	return deltay
// usage:	define function "func" as described above
//			set y to initial value
//			repeat y += rk4(myfunc,p,n,y,deltax) to get consecutive y values
// notes:	handle systems of differential equations as follows:
//			-	call rk4 for each equation passing all system variables y0,y1,..
//				in p excluding the one that is the y of the current call,
//				save the deltay without updating the system variables
//			-	update all system variables at once by adding the deltay 
//			if "func" has to be time-dependent, create a new system variable t
//			with equation dt/dx = 1.0f and pass it to "func" in p
static float rk4(	float (*f)(float,float*,int), float* p, int n,
					float y, float deltax							);
												
//***	filter design	***
// find coefs of bilinear (b[0] + b[1]z^-1)/(1 + a[1]z^-1) that realizes a
// filter with -3dbcorner/shelvingmidpoint frequency fc (relative to fs)
// type: 0=lowpass, 2=highpass, 4=lowshelf, 5=highshelf, 7=allpass
// dbgain = shelving gain in dB
static void dzbilin(float* a, float* b, float fc, int type, float dbgain=0);

// find coefs of biquad (b[0] + b[1]z^-1 + b[2]z^-2)/(1 + a[1]z^-1 + a[2]z^-2)
// of an audio equalizer filter with -3dbcorner/center/shelvingmidpoint frequency
// fc (relative to fs) and quality factor (>0) or -bandwidth in octaves (<=0) qbw,
// type: 0=lowpass, 1=bandpass, 2=highpass, 3=notch, 4=lowshelf, 5=highshelf,
// 6=peaking, 7=allpass, dbgain = peaking/shelving gain in dB
// based on: EQ cookbook formulae by Robert Bristow-Johnson
static void eqzbiquad(double* a, double* b, float fc, float qbw, int type,
						float dbgain=0);

// convert continuous to discrete time transfer function by applying the
// bilinear z-transform, fs = sample rate
// G(s) = (d[order]s^order +...+ d[0])/(c[order]s^order +...+ c[0])
// H(z) = (b[0] +...+ b[order]z^-order)/(1 + a[1]z^-1 +...+ a[order]z^-order)
static void stoz(double* a, double* b, float* c, float* d, int order,
				 float fs);

// find coefficients of bilinear (b[1]s + b[0])/(s + a[0]) that realizes a
// filter with -3dbcorner/shelvingmidpoint frequency fc
// type: 0=lowpass, 2=highpass, 4=lowshelf, 5=highshelf, 7=allpass
// dbgain = shelving gain in dB
static void dsbilin(float* a, float* b, float fc, int type, float dbgain=0);

// find coefs of biquad (b[2]s^2 + b[1]s + b[0])/(s^2 + a[1]s + a[0]) that
// realizes a filter with -3dbcorner/center/shelvingmidpoint frequency fc
// and quality factor (>0) or -bandwidth in octaves (<=0) qbw
// type: 0=lowpass, 1=bandpass, 2=highpass, 3=notch, 4=lowshelf, 5=highshelf,
// 6=peaking, 7=allpass, dbgain = peaking/shelving gain in dB
static void dsbiquad(float* a, float* b, float fc, float qbw, int type,
					 float dbgain=0);

// calculate corner frequencies f[0..d/2-1] (d even) resp. f[0..(d-1)/2] (d odd)
// and quality factors q[0..d/2-1] (d even) resp. f[0..(d-1)/2] (d odd) of a
// d-th degree butterworth lowpass filter with corner frequency = 1 realized as
// a series of 2nd-degree lowpass filters, note: the last stage of odd degree
// filters is a 1st-order lowpass which is assigned a quality factor of 0
static void fqbutter(float* f, float* q, int d);

// calculate corner frequencies f[0..d/2-1] (d even) resp. f[0..(d-1)/2] (d odd)
// and quality factors q[0..d/2-1] (d even) resp. f[0..(d-1)/2] (d odd) of a
// d-th degree bessel lowpass filter with corner frequency = 1 realized as
// a series of 2nd-degree lowpass filters, note: the last stage of odd degree
// filters is a 1st-order lowpass which is assigned a quality factor of 0
static void fqbessel(float* f, float* q, int d);

// calculate corner frequencies f[0..d/2-1] (d even) resp. f[0..(d-1)/2] (d odd)
// and quality factors q[0..d/2-1] (d even) resp. f[0..(d-1)/2] (d odd) of a
// d-th degree chebyshev type 1 lowpass filter with corner frequency = 1 and
// passband ripple (in db) rdb realized as a series of 2nd-degree lowpass
// filters, note: the last stage of odd degree filters is a 1st-order lowpass
// which is assigned a quality factor of 0
static void fqcheby(float* f, float* q, float rdb, int d);

// convert n normalized lowpass frequencies to those of a lowpass with corner
// frequency fc, if sample rate fs > 0: map frequencies for use with bilinear
// z-transform (e.g. stoz)
static void fqlowpass(float* f, float fc, int n, float fs=0);

// convert n normalized lowpass frequencies to those of a highpass with corner
// frequency fc, if sample rate fs > 0: map frequencies for use with bilinear
// z-transform (e.g. stoz)
static void fqhighpass(float* f, float fc, int n, float fs=0);

// convert series of n normalized lowpass filters to series of 2n 2nd-order
// bandpass filters, if sample rate fs > 0: map frequencies for use with
// bilinear z-transform (e.g. stoz)
// input:	normalized lowpass frequencies f[0..n-1]
//			associated quality factors q[0..n-1], if q<0.5 a single 
//				2nd-order filter is created
//			bandpass center frequency fc and absolute bandwidth bw
// output:	bandpass center frequencies f[0..2*n-1], 0 indicates no filter
//			associated quality factors q[0..2*n-1] and gain factors g[0..2*n-1] 
static void fqbandpass(float* f, float* q, float* g, float fc, float bw,
						int n, float fs=0);
														
//----------------------------- internal only ------------------------------------
private:
static int findroot(float* k, float* r, int d);
static int findroot(double* k, double* r, int d);
static double dbesj0(double x);
static double dbesj1(double x);
};

}	// end library specific namespace

#include "SpecMathInline.h"			// inline methods

#endif

