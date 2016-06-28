// BlkDsp.h
//*************** block oriented signal processing library ***********************
// uses floats for efficiency -> recommended range of absolute values:
// sqrt(FLT_MIN)..1/sqrt(FLT_MIN)
// size: > 0 unless otherwise noted
//
// This code is part of the ICST DSP Library version 1.2. It is released
// under the 2-clause BSD license. Copyright (c) 2008-2010, Zurich
// University of the Arts, Beat Frei. All rights reserved.

#ifndef _ICST_DSPLIB_BLKDSP_INCLUDED
#define _ICST_DSPLIB_BLKDSP_INCLUDED

namespace icstdsp {		// begin library specific namespace

class BlkDsp
{
//******************************* user methods ***********************************
public:
//***	transforms	***
// fft,dct,hwt sizes must be a power of 2 and > 1
// fft,ifft format: re[0],im[0],..,re[size-1],im[size-1]
// realfft,realifft original domain format: re[0],re[1],..,re[size-1]
// realfft,realifft transform domain format (contains lower half spectrum): 
//		re[0],*** re[size/2] ***,re[1],im[1],..,re[size/2-1],im[size/2-1]
// realsymfft,realsymifft format (contains lower half): re[0],re[1],..,re[size]
// dct,idct,dst,idst format: re[0],re[1],..,re[size-1]
// hwt,ihwt original domain format: [0..size-1]
// hwt,ihwt transform domain format: [[0],[0],[0..1],[0..3],..,[0..size/2-1]]
// 
static void fft(float* d, int size);				// standard FFT
static void fft(double* d, int size);		
static void ifft(float* d, int size);				// standard IFFT
static void ifft(double* d, int size);
static void realfft(float* d, int size);			// FFT of real data
static void realfft(double* d, int size);
static void realifft(float* d, int size);			// IFFT to real data
static void realifft(double* d, int size);
static void realsymfft(float* d, int size);			// FFT of symmetrical real
static void realsymfft(double* d, int size);		// data
static void realsymifft(float* d, int size);		// IFFT to symmetrical real
static void realsymifft(double* d, int size);		// data
static void dct(float* d, int size);				// DCT type2
static void dct(double* d, int size);
static void idct(float* d, int size);				// IDCT type2 (= DCT type3)
static void idct(double* d, int size);
static void dst(float* d, int size);				// DST type2
static void dst(double* d, int size);
static void idst(float* d, int size);				// IDST type2
static void idst(double* d, int size);
static cpx goertzel(float* d, int size, int k);		// return k-th bin of realfft
													// of arbitrary size
static void hwt(float* d, int size);				// haar wavelet transform HWT 
static void ihwt(float* d, int size);				// inverse HWT
													
//***	signals + window functions		***
// all windows are symmetric (d[0] = d[size-1]), all signals include the end
// point, periodic versions are obtained by creating vectors of size+1 and 
// discarding the last element
//
static void linear(	float* d, int size, 			// linear segment
					float start, float end);
static void exponential(float* d, int size,			// exponential segment
						float start, float end,		// (time: time constant as 
						float time );				// fraction of segment size)
static void logspace(float* d, int size,			// logarithmically spaced
					 float start, float end);		// data points
static float sine(	float* d, int size,				// sine wave (phase:
					float periods,					// fraction of a period,
					float phase=0,					// t: phase rel. to center)
					bool center=false);				// re: endpoint phasor angle													
static float cpxphasor(float* d, int size,			// complex phasor,
					float periods, float phase=0,	// parameters s. "sine",
					bool center=false);				// phase=0: d(0 or ctr)=1+0j
static float chirp(	float* d, int size,				// linear chirp: a version
					float startpd, float endpd,		// of "sine" with linearly 
					float phase=0);					// changing frequency
static float expchirp(float* d, int size,			// exponential chirp:"sine"
					float startpd, float endpd,		// version with exponentially 
					float phase=0);					// changing frequency
static float saw(	float* d, int size,				// sawtooth wave
					float periods,					// parameters s. "sine"
					float symmetry=1.0f,			// symmetry: 0 -> |\|\,
					float phase=0,					// 0.5 -> /\/\, 1 -> /|/|
					bool center=false);				// phase=0: d(0 or ctr)=0
static void unoise(float* d, int size);				// noise: uniform (-1..1)
static void enoise(float* d, int size);				// .. exponential (var=1)
static void cnoise(float* d, int size);				// .. standard cauchy		
static void gnoise(float* d, int size,				// .. gaussian (var=1)
					int apxorder=10);				// approximation order
static void sinc(float* d, int size, double periods);// sinc window
static void triangle(float* d, int size);			// triangular window
static void hann(float* d, int size);				// hann window
static void hamming(float* d, int size);			// hamming window
static void blackman(float* d, int size);			// blackman window
static void bhw3(float* d, int size);				// blackman-harris windows:
static void bhw4(float* d, int size);				// (bhw3:-67dB, bhw4:-92dB)
static void flattop(float* d, int size);			// 5-term flat top window
static void gauss(float* d, int size, double sigma);// gaussian window
static void kaiser(float* d, int size,double alpha);// kaiser window		
							
//***	elementary real array operations	***
//
static float sum(float* d, int size);				// return sum
static float energy(float* d, int size);			// .. signal energy: <d,d>
static float power(float* d, int size);				// .. signal power: <d,d>/size
static float rms(float* d, int size);				// .. RMS value: sqrt(power)
static float norm(float* d, int size);				// .. L2 norm: sqrt(energy)
static int maxi(float* d, int size);				// .. index of max(d)
static int mini(float* d, int size);				// .. index of min(d)
static int farthesti(float* d, float r, int size);	// .. index of max(|d[i] - r|)
static int nearesti(float* d, float r, int size);	// .. index of min(|d[i] - r|)
static float normalize(float* d, int size);			// normalize d to L2 norm = 1,
													// return scale factor
static void abs(float* d, int size);				// |d| -> d, fast
static void sgn(float* d, int size);				// sgn(d) -> d, 1 or -1, fast
static void finv(float* d, int size);				// 1/d -> d, fast
static void fsqrt(float* d, int size);				// sqrt(d) -> d, fast
static void fcos(float* d, int size);				// cos(d) -> d, fast
static void fsin(float* d, int size);				// sin(d) -> d, fast
static void logabs(float* d, int size);				// ln|d| -> d, fast
static void fexp(float* d, int size);				// exp(d) -> d, fast
static void reverse(float* d, int size);			// reverse element order
static void cumsum(float* d, int size);				// cumulative sum of d -> d
static void diff(float* d, int size);				// differentiate d, norm:
													// d=linear(0..1) -> d'=1
static void integrate(float* d,						// integrate d, norm: d=1->
						int size,					// int(d)=(0..1), lf: extra
						bool lf=false,				// prec. for low f signals
						float dprev=0,				// d[-1], required if lf=t
						float dnext=0);				// d[size]		"
static void set(float* d, float c, int size);		// c -> d
static void prune(float* d, float lim, float rep,	// if |d[n]|<=lim: 
					int size);						// sign(d[n])*rep -> d[n]
static void limit(float* d, int size,				// limit d to lo..hi
				  float hi, float lo);
static void copy(float* d, float* r, int size);		// r -> d, may overlap
static void swap(float* d, float* r, int size);		// r <-> d
static void max(float* d, float* r, int size);		// max(d,r) -> d
static void min(float* d, float* r, int size);		// min(d,r) -> d
static void add(float* d, float c, int size);		// d+c -> d
static void add(float* d, float* r, int size);		// d+r -> d
static void sub(float* d, float c, int size);		// d-c -> d
static void sub(float* d, float* r, int size);		// d-r -> d
static void mul(float* d, float c, int size);		// c*d -> d
static void mul(float* d, float* r, int size);		// d*r -> d
static void mac(float* d, float* r, 				// d + c*r -> d
					float c, int size);				//
static float dotp(float* d, float* r, int size);	// return dot product: <d,r>
static float sdist(float* d, float* r, int size);	// return squared distance
static void conv(	float* d,  						// convolution(d,r):
					float* r, 						// d[0..dsize-1],r[0..rsize-1]
					int dsize,						// -> d[0..dsize+rsize-2]
					int rsize	);					// dsize>rsize for efficiency
static void fconv(	float* d, 						// fast convolution(d,r):
					float* r, 						// d[0..dsize-1],r[0..rsize-1]
					int dsize, 						// -> d[0..dsize+rsize-2]
					int rsize	);					// space of both d and r: 
													// => nexthipow2(dsize+rsize)
static void ccorr(float* d, float* r, int dsize, 	// cross correlation(d,r) -> d
					int rsize, int pts);			// d[0..dsize-1],r[0..rsize-1]
													// -> d[0..pts-1], pts<=dsize
static void fccorr(	float* d,	 					// fast cross correlation(d,r):
					float* r, 						// d[0..dsize-1], r[0..rsize-1] 
					int dsize,						// -> d[0..dsize-1]
					int rsize	);					// space of both d and r: 
													// => nexthipow2(dsize+rsize)
static void uacorr(float* d, float* r,				// unbiased autocorrelation of
					int dsize, int rsize);			// r[0..rsize-1]->d[0..dsize-1] 
													// dsize <= rsize
static void bacorr(float* d, float* r,				// biased autocorrelation of
					int dsize, int rsize);			// r[0..rsize-1]->d[0..dsize-1]
													// dsize <= rsize
static void facorr(float* d, int size);				// fast biased autocorrelation:
													// d[0..size-1] -> d[0..size-1]
													// space: d[nexthipow2(2*size)]
//***	elementary complex array operations	***
// cpx operations format: re[0],im[0],..,re[size-1],im[size-1]
//
static cpx cpxsum(float* d, int size);				// return sum
static float cpxenergy(float* d, int size);			// .. signal energy: <d,d*>
static float cpxpower(float* d, int size);			// .. signal power: <d,d*>/size
static float cpxrms(float* d, int size);			// .. RMS value: sqrt(power)
static float cpxnorm(float* d, int size);			// .. L2 norm: sqrt(energy)
static float cpxnormalize(float* d, int size);		// normalize d to cpxnorm=1,
													// return scale factor
static void cpxconj(float* d, int size);			// conjugate d: d -> d*
static void cpxinv(float* d, int size,				// 1/d -> d, fullrange = false:
				   bool fullrange=false);			// faster, |d|<1e-19 =>HUGE_VAL   
static void cpxmag(float* d, float* r, int size);	// |r| -> d		(r=d ok)
static void cpxpow(float* d, float* r, int size);	// |r|^2 -> d	(r=d ok)
static void cpxarg(float* d, float* r, int size);	// arg{r} -> d	(r=d ok)
static void cpxprune(float* d, float lim,			// if |d[n]|<=lim: rep ->
						float rep, int size);		// Re(d[n]), 0 -> Im(d[n])
static void cpxset(float* d, cpx c, int size);		// c -> d
static void cpxcopy(float* d, float* r, int size);	// r -> d, may overlap
static void cpxswap(float* d, float* r, int size);	// r <-> d
static void cpxadd(float* d, cpx c, int size);		// d+c -> d
static void cpxadd(float* d, float* r, int size);	// d+r -> d
static void cpxsub(float* d, cpx c, int size);		// d-c -> d
static void cpxsub(float* d, float* r, int size);	// d-r -> d
static void cpxmul(float* d, cpx c, int size);		// c*d -> d
static void cpxmul(float* d, float* r, int size);	// d*r -> d
static void cpxmac(float* d, float* r, 				// d + c*r -> d
					cpx c, int size);
static cpx cpxdotp(float* d, float* r, int size);	// return dot product: <d,r*>
static void cpxre(float* re, float* d, int size);	// Re(d) -> re	(re=d ok)
static void cpxim(float* im, float* d, int size);	// Im(d) -> im	(im=d ok)			
static void realtocpx(float* d, float* re,			// re -> Re(d), im -> Im(d)
						float* im, int size);		// re,im=NULL: Re(d),Im(d)=0
static void cpxptc(float* d, float* mag,			// polar (mag,arg) to
				   float* arg, int size);			// cartesian d
static void cpxcombine(float* d, float* mag,		// arg{d} and magnitude mag
					   int size);					// -> d

//***	special array operations	***
//
static void ftod(double* d, float* r, int size);	// copy float to double
static void dtof(float* d, double* r, int size);	// copy double to float
static void ftoi(int* d, float* r, int size);		// copy + clip float to int
static void itof(float* d, int* r, int size);		// copy int to float
static void interleave(float* d, float* r,int rsize,// interleave r to d, skipped
						int interval, int offset);	// d elements remain unchanged
static void deinterleave(float* d, float* r,		// deinterleave r to d
				int dsize, int interval,int offset);// (d=r ok)
static void polyval(float* d, int dsize, float* c,	// sum(i=0..order){c[i]*d^i}
					int order);						// -> d
static void polyval(float* d, int dsize, double* c,	//
					int order);						//
static void cpxpolyval(float* d, int dsize,			// polyval with complex d =
					   float* c, int order);		// {re(0),im(0)..,im(dsize-1)}
static void cpxpolyval(float* d, int dsize,			//
					   double* c, int order);		//
static void chebyval(float* d, int dsize,			// sum(i=0..order) 
						float* c, int order);		// {c[i]*T[i](d)} -> d
static void chebyval(float* d, int dsize,			//
						double* c, int order);		//
static int findpeaks(float* d, int* idx, int size);	// fill in indices of peaks of
													// d, return peak count
static int finddips(float* d, int* idx, int size);	// as findpeaks but for dips
static void isort(float* d, int* idx, int size);	// sort d for descending order
													// fill idx with corr. indices
static int select(	float* d, 						// split d[size] into 2 subsets
					float* sel,						// (d[i],i) -> sel[i]>0 ?  
					float* a, 						// (a,aidx)[j++] : (b,bidx)[k++]    
					float* b,						// return size of a, output
					int* aidx,						// options: a=NULL -> no a,b
					int* bidx,						// aidx=NULL -> no aidx,bidx
					int size	);					// add. opt: b=NULL -> no b,bidx
static int mtabinv(float* d, float x, int size);	// return i of d[i] closest to
													// but not greater than x,
													// d must increase monotonically
static void unwrap(float* d, int size);				// unwrap phase data
static void maplin(float* d, int size, 				// map d = rlo..rhi linearly 
				   float dhi, float dlo,			// to d = dlo..dhi
				   float rhi, float rlo);			//
static void xfade(float* d, float* r, int size);	// linear fade: d to r -> d
static void xfade(float* d, float* r, float* w,		// (1-w)*d + w*r -> d
				  int size);
static void pxfade(float* d, float* r, int size);	// power fade xfade version
static float linlookup(	float* d,					// fast linear interpolated 
						int dsize,					// cyclic table lookup -> d
						float* t,					// t: table[0..2^ltsize], 
						int ltsize,					// !!! t[0]=t[2^ltsize] !!!
						float start=0,				// start index [0..2^ltsize)
						float step=1.0f	);			// index increment
													// return next start index
//***	elementary real matrix operations	***
// matrix a(row,column): {a11,..,a1n,a21,..,a2n,..,am1,..,amn}
// column vector: {r1,...,rn}
//
static float mdet(float* a, int n);					// determinant of a(n,n)
static float mtrace(float* a, int n);				// trace of a(n,n)
static void mxpose(float* a, int m, int n);			// transpose a(m,n)
static void mident(float* a, int n);				// get identity matrix a(n,n)
static void mmul(float* a, float c, int m, int n);	// c*a(m,n) -> a(m,n)
static void madd(float* a, float* b, int m, int n);	// a(m,n) + b(m,n) -> a(m,n)
static void msub(float* a, float* b, int m, int n);	// a(m,n) - b(m,n) -> a(m,n)
static void mmulv(	float* d, float* a,				// matrix a(m,n) * vector r(n) 
					float* r, int m, int n	);		// -> vector d(m)
static void mtmulv(	float* d, float* a,				// transpose of matrix a(m,n) 
					float* r, int m, int n	);		// * vector r(m) -> vector d(n)
static void mmulm(	float* c, float* a, float* b,	// matrices a(m,n)*b(n,p) 
					int m, int n, int p	);			// -> matrix c(m,p)
static float minvmulm(	float* a,					// inverse of matrix b(m,m)	
						float* b,					// * matrix a(m,n) -> a(m,n)
						int m,						// return determinant of b,
						int n	);	 				// if det(b)=0: a unchanged

//***	filters	+ delays ***
//
static void delay(	float* d,						// static delay
					float* r,						// r delayed by n sampling 
					int size,						// intervals -> d
					float* c,						// c[0..n-1],cp = 
					int& cp,						// continuation data (init:0)
					int n		);					// 
static void fir(	float* d, int size,	float* b,	// static FIR filter
					int order, float* c	);			// b[0] +...+ b[order]z^-order
static void fir(	float* d, int size,	double* b,	// c[0..order-1]: continuation
					int order, float* c	);			// data (init:0)
static void iir(	float* d,						// static IIR filter
					int size,						// 1/(1 + a[1]z^-1 + ... +
					double* a,						// a[order]z^-order)
					int order,						// c[0..order-1]: continuation
					double* c	);					// data (init:0)	
static void iir1(	float* d,						// static 1st order section
					int size,						//		b[0] + b[1]z^-1
					float* a,						//	----------------------	  
					float* b,						//		  1 + a[1]z^-1
					float* c	);					// c[0]: cont. data (init:0)
static void viir1(	float* d,						// time-varying iir1 version
					int size,						// (x)s = start coefficients
					float* as, float* ae,			// (x)e = end coefficients					
					float* bs, float* be,			//
					float* c				);		//
static void biquad(	float* d,						// static direct form 1 biquad 
					int size,						//  b[0] + b[1]z^-1 + b[2]z^-2
					double* a,						//  --------------------------
					double* b,						//    1 + a[1]z^-1 + a[2]z^-2
					double* c);						// c[0..1]: cont. data (init:0)
static void med3(	float* d, int size, float* c );	// 3,5-point median filters
static void med5(	float* d, int size, float* c );	// continuation data (init:0): 												// c[0..(1,3)]: continuation
													// med3: c[0..1], med5: c[0..3]											
//***	system analysis	***
//
static void freqz(	float* mag,						// calculate frequency response		
					float* phase,					// at frequencies f[0..size-1]
					float* f,						// relative to fs	
					int size,						//					H(z) =
					double* a,						//			  
					int aorder,						// sum(i=0..border){b[i]z^-i}
					double* b,						// --------------------------
					int border	);					// sum(i=0..aorder){a[i]z^-i}
static void freqs(	float* mag,						// calculate frequency response		
					float* phase,					// at absolute frequencies 
					float* f,						// f[0..size-1]			  
					int size,						//					G(s) =
					float* a,						// 
					int aorder,						// sum(i=0..border){b[i]s^i}
					float* b,						// -------------------------
					int border	);					// sum(i=0..aorder){a[i]s^i}
static void gdelz(	float* gdelay,					// calculate group delay		
					float* f,						// at frequencies f[0..size-1]
					int size,						// relative to fs						
					double* a,						//				H(z) =
					int aorder,						// sum(i=0..border){b[i]z^-i}
					double* b,						// --------------------------
					int border	);					// sum(i=0..aorder){a[i]z^-i}
static void gdels(	float* gdelay,					// calculate group delay	
					float* f,						// at absolute frequencies 
					int size,						// f[0..size-1]											
					float* a,						//			G(s) =
					int aorder,						// sum(i=0..border){b[i]s^i}
					float* b,						// -------------------------
					int border	);					// sum(i=0..aorder){a[i]s^i}

//***	statistics	*** 
// standard versions operate directly on the signal d, pd versions process 
// pairs {unnormalized probability d[i], value r[i]}
//
static float mean(float* d, int size);				// return arithmetic mean
static float pdmean(float* d, float* r, int size);	// .. a.m. of a distribution
static float var(float* d, int size);				// .. unbiased variance		
static float pdvar(float* d, float* r, int size);	// .. u.v. of a distribution
static float aad(float* d, int size, float m);		// .. average abs deviation,
													//    m: mean or median
static float median(float* d, int size);			// .. median
static float mad(float* d, int size);				// .. median abs deviation
static float quantile(float* d, float p, int size);	// .. p-quantile
static float gmean(float* d, int size);				// .. geometric mean
static float hmean(float* d, int size);				// .. harmonic mean
static float cov(float* x, float* y, int size);		// .. covariance
static float cpears(float* x, float* y, int size);	// .. pearson correlation
static float cspear(float* x, float* y, int size);	// .. spearman rho correlation
static float ckend(float* x, float* y, int size);	// .. kendall tau correlation
static cpx linreg(		float* c,					// linear regression -> c:
						float* x,					// yest[0..size-1] = 
						float* y,					//	 c[0] + sum(i=1..n){c[i]*
						int size,					//	 x[(i-1)*size..i*size-1]}
						int n,						// y - yest -> y, optional:
						float* p=NULL,				// [p-value,f-value] -> p[0..1]
						float* sdc=NULL,			// stddev(c[0..n]) -> sdc[0..n]
						float* cov=NULL		);		// if cov[(n+1)*(n+1)] not NULL:
													// cov[0] <= 0: save cov matrix
													// cov[0] > 0: use cov matrix
													// ret: [R^2, model efficiency]
static void polyfit(	float* c,					// fit least squares polynomial 
						int d,						// to pairs (x,y)[0..size-1] 
						int size,					// weigthed with w[0..size-1]
						float* x,					// (w = NULL: no weights) -> c:
						float* y,					// yest = sum(i=0..d){c[i]*x^i}
						float* w=NULL	);			// y - yest -> y
static int histogram(	float* d, int* bin,			// create histogram bin[bins]
						int dsize, int bins,		// of data d[dsize] with range 
						float dmin, float dmax );	// dmin..dmax, r: nof outliers
static void qqpairs(	float* x,					// get quantile-quantile pairs
						float* y,					// by reordering x and y data
						int size,					// norm=true: x replaced by
						bool norm=false	);			// normally distributed data
static int outliers(	float* score,				// find potential outliers
						float* d,					// score[size]: outlier if >0
						int size,					// score range: -1..inf  
						float th=1.0f	);			// d[size]: data, th: detection
													// threshold, ret nof outliers
static float grubbs(	float* d,					// Grubbs test -> p-value
						int size,					// d[size] = data, opt. out:
						int* idx=NULL,				// idx[0] = i of farthest d[i]
						float* mv=NULL	);			// opt. in (faster):
													// mv[0,1] = mean,variance of d
static cpx ttest(		float ref,					// t-test -> [p-value, Cohens d]
						int xsize,					// number of x data points
						float xmean,				// arithmetic mean: mean(x)
						float xvar,					// unbiased variance: var(x)
						bool onesided=false,		// onesided = false(true):
						int ysize=0,				// test for mean(x) - mean(y)
						float ymean=0,				// =(<=) ref, ysize=0: 1-sample 
						float yvar=0,				// or paired 2-sample version
						float* t=NULL			);	// if not NULL: t-value -> t[0]
static float ftest(		float xvar,					// f-test -> p-value
						int xsize,					// xsize,ysize: nof x,y points
						float yvar,					// xvar,yvar: var(x),var(y)
						int ysize,					// onesided = false(true):
						bool onesided=false		);	// test for var(x) =(<=) var(y)
													// ysize <= 0: known y variance
													// size > 1e6: use Levene test 
static float levtest(	float* x,					// Levene test -> p-value
						int xsize,					// x[0..xsize-1] = x data
						float* y,					// y[0..ysize-1] = y data
						int ysize,					// onesided = false(true):
						bool onesided=false,		// test for var(x) =(<=) var(y)
						bool bf=false			);	// bf=true: Brown-Forsythe test
static cpx chi2test(	float* fo,					// chi square test ->
						float* fe,					// [p-value, chi squared]
						int size,					// fo[0..size-1] = observed,
						int rdf=1,					// frequencies, fe[0..size-1] =
						bool yates=false		);	// expected freqs or probability
													// degrees of freedom = size-rdf
													// yates=t: use yates correction
													// reqs: all fe[i]>0, size<1e6
static float binomtest(	float pa,					// binomial test -> p-value
						int asize,					// pa: probability of category A
						int tsize,					// asize: nof cat. A elements
						bool onesided=false		);	// tsize: sample size
static float cdfn(float x, float mean, float std);	// cdf(x) of normal distribution
static float cdfcn(float x, float mean, float std);	// 1-cdf(x), std = std deviation
static cpx civn(		float mean,					// a-level confidence interval
						float std,					// of normally distributed data
						float a=0.95f	);			// -> [xlo, xhi]

//***	storage	  ***
// data is stored as a raw series of float elements
// return number of elements processed or -1 if file not accessible 
// 
static int save(	float* d, int size,				// save d to file
					char* filename,					// append=true: append data 
					bool append=false	);			// if file already exists 		
static int load(	float* d, int size,				// fill d with file data,
					char* filename,					// start reading at an offset  
					int offset=0		);			// specified in elements
static int getsize(	char* filename		);			// get nof elements in file

//***	auxiliary functions	  ***
//
static int nexthipow2(int i);						// return closest 2^n >= i
static float* sseallocf(int size);					// allocate array[size] aligned
static double* sseallocd(int size);					// for unconstrained SSE support,
static int* ssealloci(int size);					// return pointer to 1st element
#ifndef ICSTLIB_NO_SSEOPT							//
	static __m128* sseallocm128(int size);			//
	static __m128d* sseallocm128d(int size);		//
	static __m128i* sseallocm128i(int size);		//
#endif												//
static void ssefree(void* p);						// free SSE-supporting array
static void PrepareTransforms();					// preallocate resources to speed
													// up transforms, call during
													// application initialization
static void UnPrepareTransforms();					// free resources allocated by
													// PrepareTransforms, call before
													// the application terminates

//----------------------------- internal only ------------------------------------
private:
static void trigwin2(float* d, int size, double c0, double c1);
static void trigwin4(float* d, int size, double c0, double c1,
							double c2, double c3);
static double bessi0(double x);
static void qs(float* d, int* idx, int hi, int lo);
static float lu(float* a, int n, int* idx=NULL);
static int twopownton(int x);
#ifndef ICSTLIB_NO_SSEOPT 	
	static void randsse(float* d, int size, int n);
#endif
};

//***************************** circular buffer **********************************
// on overflow: take no action, return false
class CircBuffer									 
{
public:											
	CircBuffer(int size=1);							// create buffer of given size	
	~CircBuffer();
	void Reset();									// reset pointers, zero buffer
	bool Write(float* d, int n);					// write n elements
	bool Read(float* d, int n);						// read n elements
	bool ReadPassive(float* d, int n, int offset);	// read n elements at an offset 
													// from the current read pointer
													// without advancing it
	int GetReadSize();								// max no of elements to read
	int GetWriteSize();								// max no of elements to write
private:											
	float* data;									// buffer memory
	int wptr;										// write pointer
	int rptr;										// read pointer
	int bsize;										// buffer size
	CircBuffer& operator = (const CircBuffer& src);
	CircBuffer(const CircBuffer& src);
};

}	// end library specific namespace

#endif

