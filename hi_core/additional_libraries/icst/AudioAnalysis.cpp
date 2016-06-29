// AudioAnalysis.cpp
//
// This code is part of the ICST DSP Library version 1.2. It is released
// under the 2-clause BSD license. Copyright (c) 2008-2010, Zurich
// University of the Arts, Beat Frei. All rights reserved.

#include "common.h"

#if DONT_INCLUDE_HEADERS_IN_CPP
#else

#include "AudioAnalysis.h"
#include "MathDefs.h"
#include "BlkDsp.h"
#include "SpecMath.h"
#endif


	AudioAnalysisBase::AudioAnalysisBase()
	{
		realFloatFFTs = new FFTProcessor((int)IppFFT::DataType::RealFloat);
		realDoubleFFTs = new FFTProcessor((int)IppFFT::DataType::RealDouble);
		complexFloatFFTs = new FFTProcessor((int)IppFFT::DataType::ComplexFloat);
		complexDoubleFFTs = new FFTProcessor((int)IppFFT::DataType::ComplexDouble);
	}

	AudioAnalysisBase::~AudioAnalysisBase()
	{
		realFloatFFTs = nullptr;
		realDoubleFFTs = nullptr;
		complexFloatFFTs = nullptr;
		complexDoubleFFTs = nullptr;
	}

	//******************************************************************************
	//* spectral analysis
	//*
	// high resolution spectral analysis
	// array sizes: (d,freq,amp,time,temp)[size],(w,dw,rw)[size+1], aic[4]
	// size = 2^n with n>0
	// freq/amp/time can be NULL, in which case the vectors are not calculated
	// time reassignment yields the energy barycenter of the WINDOWED input -> a
	// WIDE pulse is located well only if it falls into a flat region of the window
	// input:	data d, obtain from prespec: w/dw/rw/aic, just workspace: temp
	// output:	complex half spectrum of windowed d -> d
	//			reassigned frequencies relative to fs -> freq[0..size/2-1],
	//			freq[size/2..size-1] undefined
	//			reassigned time position (-0.5..0.5) relative to center -> 
	//			time[0..size/2-1], time[size/2..size-1] undefined
	//			reassigned amplitudes of peaks -> amp[0..size/2-1], no peak:0,
	//			amp[size/2..size-1] undefined 
		void AudioAnalysisBase::analyseSpectrum(float* d, float* freq, float* amp, float* time,
				int size, float* w, float* dw, float* rw, float* temp, float* aic)
	{	
		int hsize = size>>1; float x;

		// basic spectral analysis
		if (freq) {VectorFunctions::copy(freq,d,size);}
		if (time) {VectorFunctions::copy(time,d,size);}
		VectorFunctions::mul(d,w,size);
		realFloatFFTs->realfft(d,size); d[1]=0;			// raw complex spectrum -> d
		
		// common to frequency and time reassignment
		if ((freq) || (time)) {
			VectorFunctions::cpxcopy(temp,d,hsize);
			x = 1e-6f*VectorFunctions::cpxrms(temp,hsize);	// prevent large reassignment
			x = __max(x,2.0f*sqrtf(FLT_MIN));		// error for near 0 components
			VectorFunctions::cpxprune(temp,x,x,hsize);		// allowing 120 dB dynamic range							
			VectorFunctions::cpxinv(temp,hsize);				
		}
	
		// frequency reassignment
		if (freq) {
			VectorFunctions::mul(freq,dw,size);
			realFloatFFTs->realfft(freq,size); freq[1]=0;	// f reassignment spectrum -> freq
			VectorFunctions::cpxmul(freq,temp,hsize);
			VectorFunctions::cpxim(freq,freq,hsize);			// delta f -> freq
			VectorFunctions::linear(freq+hsize,hsize,0,0.5f-1.0f/static_cast<float>(size));
			VectorFunctions::add(freq,freq+hsize,hsize);		// reassigned frequencies -> freq
		}

		// time reassignment
		if (time) {
			VectorFunctions::mul(time,rw,size);					
			realFloatFFTs->realfft(time,size); time[1]=0;	// t reassignment spectrum -> time
			VectorFunctions::cpxmul(time,temp,hsize);
			VectorFunctions::cpxre(time,time,hsize);			// delta t -> time
		}

		//	amplitude reassignment
		if (!amp) return;
		float pm1,p,pp1,y; float scl = static_cast<float>(size);
		int i=1,j=2;
		amp[0] = d[0]; amp[hsize-1] = 0;				// special cases	
		if (!freq) {									//* no precise f available * 
			while (i<(hsize-1)) {
				pm1 = d[j-2]*d[j-2] + d[j-1]*d[j-1];	// get power of previous ..
				p = d[j]*d[j] + d[j+1]*d[j+1];			// current ..
				pp1 = d[j+2]*d[j+2] + d[j+3]*d[j+3];	// and next bin
				if ((pm1 <= p) && (p > pp1))			// local power max? -> peak
				{										// calc. reassigned amplitude
					y = sqrtf(p);
					pm1 = logf(pm1 + FLT_MIN);
					p = logf(p + FLT_MIN);
					pp1 = logf(pp1 + FLT_MIN);
					x = 0.25f*(pp1-pm1)/(p - 0.5f*(pp1+pm1));
					x *= x;						
					x = 1.0f+(aic[0]+(aic[1]+(aic[2]+aic[3]*x)*x)*x)*x;
					amp[i] = 2.0f*x*y;
					i+=2; j+=4;							// next bin can't be a peak
				}
				else {amp[i]=0; i++; j+=2;}				// no peak
			}
			return;
		}
		while (i<(hsize-1)) {							//* precise f available *
			x = scl*(freq[i]-freq[i+hsize]);
			if ((x < 1.0f) && (x > -1.0f))				// bins with delta f < 1
			{											// are peak candidates
				pm1 = d[j-2]*d[j-2] + d[j-1]*d[j-1];	// get power of previous ..
				p = d[j]*d[j] + d[j+1]*d[j+1];			// current ..
				pp1 = d[j+2]*d[j+2] + d[j+3]*d[j+3];	// and next bin
				if ((pm1 <= p) && (p > pp1))			// local power max? -> peak
				{										// calc. reassigned amplitude
					x *= x;								
					x = 1.0f+(aic[0]+(aic[1]+(aic[2]+aic[3]*x)*x)*x)*x;
					amp[i] = 2.0f*x*sqrtf(p);
					i+=2; j+=4;							// next bin can't be a peak
				}
				else {amp[i]=0; i++; j+=2;}				// no peak
			}
			else {amp[i]=0; i++; j+=2;}					// no peak
		}
	}
	
	// obtain spec parameters
	// input:	symmetric window w[size+1]
	// output:	dw[size+1], rw[size+1], aic[4]					
	void AudioAnalysisBase::prepareWindow(float* w, float* dw, float* rw, float* aic, int size)
	{
		// scale window
		float x = VectorFunctions::sum(w,size);
		VectorFunctions::mul(w,1.0f/x,size+1);
	
		// calculate scaled first derivative of w
		VectorFunctions::copy(dw,w,size+1);
		VectorFunctions::diff(dw,size+1); 
		x = -1.0f/(2.0f*M_PI_FLOAT*static_cast<float>(size));
		VectorFunctions::mul(dw,x,size);

		// calculate time ramped version of w
		VectorFunctions::linear(rw,size+1,-0.5f,0.5f);
		VectorFunctions::mul(rw,w,size);

		// calculate interpolation polynomial for amplitude reassignment
		float tw[1024]; float u[129]; double c[9];
		VectorFunctions::set(tw,0,1024);
		VectorFunctions::deinterleave(tw+512-8,w,17,size/16,0);
		realFloatFFTs->realfft(tw,1024); tw[1]=0;
		VectorFunctions::cpxmag(tw,tw,65);
		VectorFunctions::copy(u+64,tw,65);
		VectorFunctions::reverse(tw,65);
		VectorFunctions::copy(u,tw,64);
		x=u[64];
		for (int i=0; i<129; i++) {u[i] = x/u[i];}
		SpecMath::chebyapprox(c,8,u,129);
		SpecMath::chebytops(c,8);
		aic[0]=static_cast<float>(c[2]/c[0]);
		aic[1]=static_cast<float>(c[4]/c[0]);
		aic[2]=static_cast<float>(c[6]/c[0]);
		aic[3]=static_cast<float>(c[8]/c[0]);
	}

	//******************************************************************************
	//* analysis by decomposition to arbitrary functions
	//*
	// use the matching pursuit algorithm to approximate the data as a weighted sum
	// of atoms from the dictionary dict:
	// d[0..size-1] ~ sum(i=0..elements-1){weight[i]*atom(idx[i])[0..size-1]}
	// input:	data[0..size-1] 
	//			dict[atom0[0..size-1] atom1[0..size-1] .. atom(atoms-1)[0..size-1]]
	//				!!!	NOTE: all atoms must have a L2-norm of 1 !!!
	// output:	weight[0..elements-1], idx[0..elements-1], residual data[0..size-1]			
	void RocketScienceAnalysis1::analyseMatchingPursuit(float* weight, int* idx, int elements,
									float* dict, int atoms, float* data, int size)
	{
		int i,j;
		float* m; m = new float[atoms];
		for (i=0; i<elements; i++) {
			for (j=0; j<atoms; j++) {m[j] = VectorFunctions::dotp(dict+j*size,data,size);}
			idx[i] = VectorFunctions::farthesti(m,0,atoms);
			weight[i] = m[idx[i]];
			VectorFunctions::mac(data,dict+idx[i]*size,-weight[i],size);
		}
		delete[] m;
	}

	//******************************************************************************
	//* cepstral analysis
	//*
	// get real cepstrum from magnitude spectrum
	// size = 2^n with n>0
	// input:	magnitude lower half spectrum d[0..size]
	// output:	real lower half cepstrum d[0..size]
	void RocketScienceAnalysis1::getRealCepstrumFromMagnitude(float* d, int size)
	{
		float m = d[VectorFunctions::maxi(d,size+1)];
		VectorFunctions::limit(d,size+1,m,1e-10f*m);
		VectorFunctions::logabs(d,size+1);
		VectorFunctions::realsymifft(d,size);
	}

	// get magnitude spectrum from real cepstrum
	// size = 2^n with n>0
	// input:	real lower half cepstrum d[0..size]
	// output:	magnitude lower half spectrum d[0..size]
	void RocketScienceAnalysis1::getMagnitudeSpectrumFromRealCepstrum(float* d, int size)
	{
		VectorFunctions::realsymfft(d,size);
		VectorFunctions::fexp(d,size+1);
	}

	// calculate mel frequency cepstral coefficients (MFCC) from power spectrum
	// (mathematically more conclusive) or magnitude spectrum (see ETSI ES 201108)
	// input:	lower half spectrum d[0..size], bands = number of filter bands,
	//			cofs = number of coefficients, sample rate fs, lowest(highest)
	//			analysis frequency fmin(fmax)
	// output:	MFCC c[0..cofs-1]
	void RocketScienceAnalysis1::spectomfcc(float* d, float* c, int size, int bands,
								   int cofs, float fs, float fmax, float fmin)
	{
		int i,j; float x,y;
		int* b; b = new int[bands+2];
		float* mspec; mspec = new float[bands];

		// mel frequencies
		float mello = logf(1.0f + fmin/700.0f); 
		float melhi = logf(1.0f + fmax/700.0f);
		float meldelta = (melhi - mello)/static_cast<float>(bands+1);
		float n = 1400.0f*static_cast<float>(size)/fs;
		for (i=0; i<=(bands+1); i++) {
			b[i] = static_cast<int>(0.5f + n*(expf(mello) - 1.0f));
			b[i] = __max(0,__min(size,b[i]));
			mello += meldelta;
		}

		// mel filtering
		for (i=1; i<=bands; i++) {
			n = 1.0f/(static_cast<float>(b[i] - b[i-1]) + 1.0f);
			x = y = 0;
			for (j=b[i-1]; j<=b[i]; j++) {
				x += n;
				y += (x*d[j]);		
			}
			n = 1.0f/(static_cast<float>(b[i+1] - b[i]) + 1.0f);
			for (j=b[i]+1; j<=b[i+1]; j++) {
				x -= n;
				y += (x*d[j]);
			}
			mspec[i-1] = y;
		}

		// take logarithm
		n = mspec[VectorFunctions::maxi(mspec,bands)];
		VectorFunctions::limit(mspec,bands,n,1e-10f*n);
		VectorFunctions::logabs(mspec,bands);
	
		// DCT type 2 (trivially computed as size is usually low and no power of 2)
		float v = M_PI_FLOAT/static_cast<float>(2*bands);
		float xr,xi,wr,wi, z=0;
		c[0] = VectorFunctions::sum(mspec,bands);
		for (i=1; i<cofs; i++) {
			z += v; y=0;
			xr = cosf(z); xi = sinf(z);
			wr = 2.0f*xr*xr - 1.0f; wi = 2.0f*xr*xi;
			for (j=0; j<bands; j++) {
				y += (xr*mspec[j]);
				x=xr; xr = xr*wr - xi*wi; xi = x*wi + xi*wr;
			}
			c[i] = y;
		}

		delete[] b; delete[] mspec;
	}

	//******************************************************************************
	//* linear prediction
	//*
	// levinson-durbin recursion: solve rm[]*a[1 ..]=[err 0..0] for a[] and err
	// input:	rm[0..order] = biased autocorrelation, obtained e.g. by "bacorr"
	// output:	a[0..order] = linear prediction coefficients 
	//			k[0..order-1] = reflection coefficients
	// return:	re = maximum absolute reflection coefficient (1 if unstable)
	//			im = residual to input energy ratio
	// notes:	using biased autocorrelation guarantees a stable predictor,
	//			which will perform similar to the burg and covariance method if
	//			the input is longer than 20*order AND hamming or hann windowed 
	Complex RocketScienceAnalysis1::lpdurbin(double* a, double* k, float* rm, int order)
	{
		int i,j,ihalf;
		double temp,r,err, rsmax=0; 
		Complex rval;
		a[0] = 1.0;
		if (rm[0] < FLT_MIN) {
			for (i=0; i<order; i++) {a[i+1] = k[i] = 0;}
			rval.re = 0; rval.im = 1.0f;
			return rval;
		}
		err = static_cast<double>(rm[0]);
		for (i=1; i<=order; i++) {
			temp = static_cast<double>(rm[i]);
			for (j=1; j<i; j++) {temp += a[j]*static_cast<double>(rm[i-j]);}
			a[i] = r = k[i-1] = -temp/(err + DBL_MIN);
			temp = r*r;
			if (temp >= 1.0) {
				for (j=0; j<order; j++) {a[j+1] = k[j] = 0;}
				rval.re = rval.im = 1.0f;
				return rval;
			}
			rsmax = __max(rsmax,temp);
			err -= (err*temp);
			ihalf = i>>1;
			if (i & 1) {
				for (j=1; j<=ihalf; j++) {
					temp=a[j]; a[j] += (r*a[i-j]); a[i-j] += (r*temp);}}
			else {
				a[ihalf] += (r*a[ihalf]);
				for (j=1; j<ihalf; j++) {
					temp=a[j]; a[j] += (r*a[i-j]); a[i-j] += (r*temp);}}
		}
		rval.re = static_cast<float>(sqrt(rsmax));
		rval.im = static_cast<float>(err)/rm[0];
		return rval;
	}	

	// calculate LPC residual from original signal
	// input:	data d[0..size-1], prediction coefficients a[0..order],
	//			block continuation data c[0..order-1]: zero fill for first call  
	// output:	data d[0..size-1], c[0..order-1] for next call with continuous data
	void LpcAudioAnalysis::lpanalyze(float* d, float* c, double* a, int size, int order)
	{
		float* af; af = new float[order+1];
		VectorFunctions::dtof(af,a,order+1);
		VectorFunctions::fir(d, size, af, order, c);
		delete[] af;
	}

	// restore original signal from LPC residual using a direct form IIR filter
	// input:	data d[0..size-1], prediction coefficients a[0..order],
	//			block continuation data c[0..order-1]: zero fill for first call 
	// output:	data d[0..size-1], c[0..order-1] for next call with continuous data
	void LpcAudioAnalysis::lpdsynth(float* d, double* c, double* a, int size, int order)
		{VectorFunctions::iir(d, size, a, order, c);}

	// restore original signal from LPC residual using a lattice IIR filter,
	// the second version allows gliding linearly from one k-set to another
	// input:	data d[0..size-1], reflection coefficients k[0..order-1] or
	//			kstart[0..order-1] and kend[0..order-1] respectively
	//			block continuation data c[0..order-1]: zero fill for first call 
	// output:	data d[0..size-1], c[0..order-1] for next call with continuous data
	void LpcAudioAnalysis::lplsynth(float* d, double* c, double* k, int size, int order)
	{
		int i,j; double x;
		order--;
		for (i=0; i<size; i++) {
			x = static_cast<double>(d[i]) - k[order]*c[order];
			for (j=order-1; j>=0; j--) {
				x -= (k[j]*c[j]);
				c[j+1] = c[j] + k[j]*x;
			}
			if (fabs(x) < static_cast<double>(ANTI_DENORMAL_FLOAT)) {x=0;}
			c[0] = x; d[i] = static_cast<float>(x);
		}
	}

	void LpcAudioAnalysis::lplsynth(float* d, double* c, double* kstart, double* kend,
								 int size, int order)
	{
		int i,j; double x;
		double* k; k = new double[order];
		if (size < 2) {
			for (i=0; i<order; i++) {k[i] = 0.5*(kend[i] + kstart[i]);}
			lplsynth(d,c,k,size,order);
			delete[] k;
			return;
		}
		double* kinc; kinc = new double[order];
		double scl = 1.0/static_cast<double>(size-1);
	
		for (i=0; i<order; i++) {
			k[i] = kstart[i];
			kinc[i] = scl*(kend[i]-kstart[i]);
		}
	
		order--;
		for (i=0; i<size; i++) {
			x = static_cast<double>(d[i]) - k[order]*c[order];
			k[order] += kinc[order];
			for (j=order-1; j>=0; j--) {
				x -= (k[j]*c[j]);
				c[j+1] = c[j] + k[j]*x;
				k[j] += kinc[j];
			}
			if (fabs(x) < static_cast<double>(ANTI_DENORMAL_FLOAT)) {x=0;}
			c[0] = x; d[i] = static_cast<float>(x);	
		}

		delete[] k; delete[] kinc;
	}

	// calculate line spectral frequencies (LSFs) from even order LPC coefficients
	// input:	linear prediction coefficients a[0..order]
	//			grid =	number of bins to look for frequencies (can be separated 
	//					if at least fs/grid apart) 
	// output:	ascending line spectral frequencies f[0..order-1] relative to fs
	//			return false if frequencies are not separable on given grid
	bool LpcAudioAnalysis::lpctolsf(float* f, double* a, int order, int grid)
	{
		int i,j, horder = order/2;
		grid = VectorFunctions::nexthipow2(__max(grid,horder+1));
		float x,y, scale = 0.5f/static_cast<float>(grid);
		float* p; p = new float[grid];
		float* q; q = new float[grid];

		// split predictor polynomial into complementary polynomials
		p[horder] = 1.0f; q[horder] = -1.0f;  
		for (i=0; i<horder; i++) {
			p[i] = static_cast<float>(a[horder+i+1] + a[horder-i]);
			q[i] = static_cast<float>(a[horder+i+1] - a[horder-i]);
		}
		for (i=horder+1; i<grid; i++) {p[i] = q[i] = 0;}
	
		// evaluate polynomials on the unit circle
		VectorFunctions::dct(p,grid);
		VectorFunctions::dst(q,grid);

		// find interleaved zeros of polynomials 
		i=1; j=0; grid--;
		while ((i<grid) && (j<order)) {
			while ((i<grid) && (j<order)) {
				if ((p[i]*p[i+1]) <= 0) {		
					x = p[i]*p[i]; y = p[i+1]*p[i+1];
					if (x <= y) {
						f[j] = scale*(static_cast<float>(i) + 
								SpecMath::paraext(p[i-1]*p[i-1],x,y).re);
					}
					else {
						i++;
						if (i < grid) {
							f[j] = scale*(static_cast<float>(i) + 
									SpecMath::paraext(x,y,p[i+1]*p[i+1]).re);
						}
						else {
							f[j] = scale*(static_cast<float>(i) + 
									SpecMath::paraext(x,y,0).re);
						}
					}
					j++;
					break;
				}
				i++;
			}
			while ((i<grid) && (j<order)) {
				if ((q[i]*q[i-1]) <= 0) {
					x = q[i-1]*q[i-1]; y = q[i]*q[i];
					if (x <= y) {
						if (i > 1) {
							f[j] = scale*(static_cast<float>(i) + 
									SpecMath::paraext(q[i-2]*q[i-2],x,y).re);
						}
						else {
							f[j] = scale*(static_cast<float>(i) + 
									SpecMath::paraext(0,x,y).re);
						}
					}
					else {
						i++;
						f[j] = scale*(static_cast<float>(i) + 
								SpecMath::paraext(x,y,q[i]*q[i]).re);
					}
					j++;
					break;
				}
				i++;
			}
			i++;
		}

		// check validity of frequencies and clean up
		delete[] p; delete[] q;
		if (j < order) {return false;}
		for (i=0; i<(order-1); i++) {if (f[i+1] <= f[i]) {return false;}}
		return true;
	}

	// restore original signal from LPC residual using LSFs with an IIR filter,
	// the second version allows gliding linearly from one f-set to another
	// input:	data d[0..size-1], line spectral frequencies f[0..order-1] or
	//			fstart[0..order-1] and fend[0..order-1] respectively,
	//			block continuation data c[0..2*order]: zero fill for first call 
	// output:	data d[0..size-1], c[0..2*order] for next call with continuous data
	void LpcAudioAnalysis::lpssynth(float* d, float* f, double* c, int size, int order)
	{
		double* a; a = new double[order];
		int i,j,k; double x,y,z,tmp;
	
		// calculate filter coefficients
		for (i=0; i<order; i++) {a[i] = -2.0*cos(2.0*M_PI*f[i]);}

		// process data
		for (k=0; k<size; k++) {
			x = y = c[0]; z=0; j=0; 
			for (i=1; i < (2*order); i+=4) {
				tmp = c[i] + x*a[j]; c[i]=x;
				z += tmp;
				x += c[i+2]; c[i+2]=tmp;
				tmp = c[i+1] + y*a[j+1]; c[i+1]=y;
				z += tmp;
				y += c[i+3]; c[i+3]=tmp;
				j+=2;
			}
			z = static_cast<double>(d[k]) + z + x - y;
			if (fabs(z) < static_cast<double>(ANTI_DENORMAL_FLOAT)) {z=0;}
			c[0] = -0.5*z; d[k] = static_cast<float>(z);
		}

		delete[] a;
	}
	void LpcAudioAnalysis::lpssynth(float* d, float* fstart, float* fend, double* c,
								 int size, int order)
	{
		int i,j,k, imax = 2*order; float* f;
	
		// special case
		if (size < 2) {
			f = new float[order];
			for (i=0; i<order; i++) {f[i] = 0.5f*(fend[i] + fstart[i]);}
			lpssynth(d,f,c,size,order);
			delete[] f;
			return;
		}

	#ifdef ICSTLIB_NO_SSEOPT	
	
		double x,y,z,tmp,scl;
	
		// calculate filter coefficients and increments
		double* ar; ar = new double[order];
		double* ai; ai = new double[order];
		double* wr; wr = new double[order];
		double* wi; wi = new double[order];
		scl = 1.0/static_cast<double>(size-1);
		for (i=0; i<order; i++) {
			ar[i] = cos(2.0*M_PI*fstart[i]);
			ai[i] = sin(2.0*M_PI*fstart[i]);
			wr[i] = cos(2.0*M_PI*(scl*(fend[i]-fstart[i])));
			wi[i] = sin(2.0*M_PI*(scl*(fend[i]-fstart[i])));
		}

		// process data
		for (k=0; k<size; k++) {
			x = y = c[imax]; z=0; j=0; 
			for (i=0; i < imax; i+=4) {	
				tmp = c[i] - 2.0*x*ar[j]; c[i]=x;			// IIR filter
				z += tmp;
				x += c[i+2]; c[i+2]=tmp;
				tmp = c[i+1] - 2.0*y*ar[j+1]; c[i+1]=y;
				z += tmp;
				y += c[i+3]; c[i+3]=tmp;
		
				tmp = ar[j]*wr[j] - ai[j]*wi[j];			// coefficient update
				ai[j] = ar[j]*wi[j] + ai[j]*wr[j];
				ar[j]=tmp;
				tmp = ar[j+1]*wr[j+1] - ai[j+1]*wi[j+1];
				ai[j+1] = ar[j+1]*wi[j+1] + ai[j+1]*wr[j+1];
				ar[j+1]=tmp;
			
				j+=2;
			}
			z = static_cast<double>(d[k]) + z + x - y;
			if (fabs(z) < static_cast<double>(ANTI_DENORMAL_FLOAT)) {z=0;}
			c[imax] = -0.5*z; d[k] = static_cast<float>(z);
		}

		delete[] ar; delete[] ai; delete[] wr; delete[] wi;

	#else

		double t[4], tmp; int horder = order >> 1;
		__m128d* ar = VectorFunctions::sseallocm128d(horder);
		__m128d* ai = VectorFunctions::sseallocm128d(horder);
		__m128d* wr = VectorFunctions::sseallocm128d(horder);
		__m128d* wi = VectorFunctions::sseallocm128d(horder);
		__m128d* cl = VectorFunctions::sseallocm128d(order);
		__m128d xy,z12,r1,r0,r2,r3,r4;
	
		// calculate filter coefficients and increments
		tmp = 1.0/static_cast<double>(size-1);
		for (i=0,j=0; i<order; i+=2,j++) {
			ar[j] = _mm_set_pd(2.0*cos(2.0*M_PI*fstart[i+1]) , 2.0*cos(2.0*M_PI*fstart[i]));
			ai[j] = _mm_set_pd(2.0*sin(2.0*M_PI*fstart[i+1]) , 2.0*sin(2.0*M_PI*fstart[i]));
			wr[j] = _mm_set_pd(cos(2.0*M_PI*(tmp*(fend[i+1]-fstart[i+1]))), 
							   cos(2.0*M_PI*(tmp*(fend[i]-fstart[i])))		); 
			wi[j] = _mm_set_pd(sin(2.0*M_PI*(tmp*(fend[i+1]-fstart[i+1]))), 
							   sin(2.0*M_PI*(tmp*(fend[i]-fstart[i])))		);  
		}
	
		// process data
		for (i=0,j=0; i<order; i++,j+=2) {cl[i] = _mm_loadu_pd(c+j); }
		for (k=0; k<size; k++) {
			tmp = static_cast<double>(d[k]);
			xy = _mm_load1_pd(c+imax);
			z12 = _mm_setzero_pd();
			for (i=0,j=0; i<order; i+=2,j++) {
			
				// IIR filter
				r0 = ar[j];
				r4 = _mm_mul_pd(xy , r0);
				r4 = _mm_sub_pd(cl[i] , r4);
				cl[i] = xy;
				xy = _mm_add_pd(xy , cl[i+1]);
				cl[i+1] = r4;
				z12 = _mm_add_pd(z12 , r4);
			
				// coefficient update
				r2 = ai[j];
				r1 = _mm_mul_pd(r0 , wr[j]);
				r3 = _mm_mul_pd(r2 , wi[j]);
				r0 = _mm_mul_pd(r0 , wi[j]);
				r2 = _mm_mul_pd(r2 , wr[j]);
				ar[j] = _mm_sub_pd(r1 , r3);
				ai[j] = _mm_add_pd(r0 , r2);
			}
			_mm_storeu_pd(t , z12);
			_mm_storeu_pd(t+2 , xy);
			tmp += (t[0] + t[1] + t[2] - t[3]);
			if (fabs(tmp) < static_cast<double>(ANTI_DENORMAL_FLOAT)) {tmp=0;}
			c[imax] = -0.5*tmp; d[k] = static_cast<float>(tmp);
		}
		for (i=0,j=0; i<order; i++,j+=2) {_mm_storeu_pd(c+j , cl[i]);}
	
		VectorFunctions::ssefree(ar);
		VectorFunctions::ssefree(ai);
		VectorFunctions::ssefree(wr);
		VectorFunctions::ssefree(wi);
		VectorFunctions::ssefree(cl);

	#endif
	}

	//******************************************************************************
	//* adaptation
	//*
	// normalized LMS algorithm: estimate y based on x minimizing e = y{est}(x) - y
	// input:	source data x[0..size-1], reference data y[0..size-1]
	//			order = filter order, mu = adaptation rate: 0..1(fastest stable)
	//			current filter coefficients h[0..order]: zero fill for first call 
	//			continuation data c[0..order-1]: zero fill for first call
	// output:	error signal e[0..size-1]
	//			new filter coefficients h[0..order]
	//			c[0..order-1] for next call with continuous data
	void RocketScienceAnalysis2::nlms(float* e, float* x, float* y, float* h, float* c,
						int size, int order, float mu)
	{
		int i,j,k,contsize; float err,muerr; double temp;
		double rms = static_cast<double>(ANTI_DENORMAL_FLOAT*ANTI_DENORMAL_FLOAT);
		for (i=0; i<order; i++) {
			temp = static_cast<double>(c[i]); rms += (temp*temp);}
		if (size >= order) {contsize=order;} else {contsize=size;}
	
		for (i=0; i<contsize; i++) {		// include continuation data
			err = y[i];
			for (j=0; j<(order-i); j++) {err -= (h[j]*c[i+j]);}
			k = order-i; for (j=0; j<=i; j++) {err -= (h[k]*x[j]); k++;}
			e[i] = err;
			temp = static_cast<double>(x[i]); rms += (temp*temp);
			muerr = mu*err/static_cast<float>(rms);
			temp = static_cast<double>(c[i]); rms -= (temp*temp);
			for (j=0; j<(order-i); j++) {h[j] += (muerr*c[i+j]);}
			k = order-i; for (j=0; j<=i; j++) {h[k] += (muerr*x[j]); k++;}
		}

		for (i=contsize; i<size; i++) {		// use new data only
			err = y[i];
			k = i-order; for (j=0; j<=order; j++) {err -= (h[j]*x[k]); k++;}
			e[i] = err;
			temp = static_cast<double>(x[i]); rms += (temp*temp);
			muerr = mu*err/static_cast<float>(rms);
			k = i-order;
			temp = static_cast<double>(x[k]); rms -= (temp*temp);
			for (j=0; j<=order; j++) {h[j] += (muerr*x[k]); k++;}
		}

		j=0; for (i=contsize; i<order; i++) {c[j]=c[i]; j++;}
		for (i=size-contsize; i<size; i++) {c[j]=x[i]; j++;}
	}

	// tracking demodulator, based on a costas loop
	// frequencies and bandwidths are relative to fs, phase range: 0..2pi
	// input:	signal d[0..size-1], fstart = start frequency, pstart = start phase
	//			tbw =	tracking bandwidth, determines how quickly the carrier
	//					oscillator follows frequency changes of the input signal	
	//			dbw =	demodulation bandwidth, determines how quickly the 
	//					demodulated signal tracks phase changes of the input,
	//					dbw is internally forced to be at least 2*tbw
	//			continuation data c[0..1]: zero fill for first call 
	// output:	demodulated quadrature signal r[0..2*size-1] = {I0,Q0,I1,Q1,..}
	//			updated fstart + pstart, c[0..1] for next call with continuous data
	void RocketScienceAnalysis2::costas(float* d, float* r, int size, float tbw, float dbw,
							   float &fstart, float &pstart, float* c)
	{
		int i,j; float a,b,g,xr,xi,wr,wi,fr,fi,ar,ai,br,bi,err,tmp;

		// anti-denormal mechanics, thread-safe in practice as occasionally
		// overwriting z by another call is harmless and write to int is atomic
		static unsigned int z = 1;
		union {float ftmp; unsigned int uitmp;};
		z = 663608941*z;
		uitmp = (z >> 9) | 0x40000000;
		float adn[4] = {	0.25f*ANTI_DENORMAL_FLOAT*(ftmp + 2.0f),
							-0.15f*ANTI_DENORMAL_FLOAT*ftmp,	
							0.08f*ANTI_DENORMAL_FLOAT*(ftmp - 2.761f),
							-0.13f*ANTI_DENORMAL_FLOAT*(ftmp - 1.629f)	};

		// calculate filter coefficients
		a = __min(1.0f,19.5f*tbw);
		b = 0.25f*a*a;											// critically damped
		g = __min(1.0f,6.28f*__max(2.0f*tbw,dbw));				// max for stability
	
		// init oscillator state
		xr = cosf(pstart); xi = sinf(pstart);
		tmp = 2.0f*M_PI_FLOAT*fstart;
		fr = cosf(tmp); fi = sinf(tmp);
	
		for (i=0,j=0; i<size; i++,j+=2) {

			// input processing
			tmp = 2.0f*d[i] + adn[i&1];							// preprocessing
			wr = xr*tmp; wi = -xi*tmp;							// demodulation
			c[0] += g*(wr - c[0]); c[1] += g*(wi - c[1]);		// arm filters					
			r[j] = c[0] + adn[2]; r[j+1] = c[1] + adn[3];		//
			err = SpecMath::qdatan2f(c[1],c[0]) + 1e-8f;		// phase detection
		
			// customize error signal for oscillator update
			ai = a*err;											// ca. sin(a*err)
			ar = 1.0f - 0.5f*ai*ai;								// ca. cos(a*err)
			bi = b*err;											// ca. sin(b*err)
			br = 1.0f - 0.5f*bi*bi;								// ca. cos(b*err)
		
			// self-stabilizing quadrature oscillator
			wr = fr*ar - fi*ai; wi = fi*ar + fr*ai;				// get phase incr
			tmp = wr*xr - wi*xi; xi = wi*xr + wr*xi; xr=tmp;	// update output
			tmp = 0.5f*(3.0f - xr*xr - xi*xi);					// + normalize
			xr *= tmp; xi *= tmp;								// amplitude
			tmp = fr*br - fi*bi; fi = fi*br + fr*bi; fr=tmp;	// update frequency 
			tmp = 0.5f*(3.0f - fr*fr - fi*fi);					// + normalize
			fr *= tmp; fi *= tmp;								// freq. vector
		}
	
		// save oscillator state					
		pstart = atan2f(xi,xr);						
		fstart = atan2f(fi,fr)/(2.0f*M_PI_FLOAT);
	}

	//******************************************************************************
	//* feature extraction
	//*
	// envelope follower
	// input:	signal d[0..size-1], continuation data c: init with 0
	//			atime/rtime: attack/release time constant in sampling intervals
	//			envelope type: 0 absolute value, 1 power, 2 RMS 
	// output:	envelope r[0..size-1], continuation data c
	void EnvelopeFollowerAnalyis::envelope(float* d, float* r, float &c, int size, 
								 float atime, float rtime, int type)
	{
		int i;
		atime = 1.0f/__max(1.0f,atime);				// practical approximation
		rtime = 1.0f/__max(1.0f,rtime);				// of 1-exp(-1/time)

	#ifdef ICSTLIB_NO_SSEOPT
		float x;
		if (type == 0) {	
			for (i=0; i<size; i++) {
				x = fabsf(d[i]) + ANTI_DENORMAL_FLOAT; 
				if (x > c) {c += (atime*(x - c));} else {c += (rtime*(x - c));}
				r[i] = c;
			}	
		}
		else if (type == 1) {
			for (i=0; i<size; i++) {
				x = d[i]*d[i] + ANTI_DENORMAL_FLOAT; 
				if (x > c) {c += (atime*(x - c));} else {c += (rtime*(x - c));}
				r[i] = c;
			}
		}
		else {
			for (i=0; i<size; i++) {
				x = d[i]*d[i] + ANTI_DENORMAL_FLOAT*ANTI_DENORMAL_FLOAT; 
				if (x > c) {c += (atime*(x - c));} else {c += (rtime*(x - c));}
				r[i] = sqrtf(c);
			}
		}
	#else
		__m128 cf[4];
		cf[0] = _mm_set_ss(1.0f - atime);
		cf[1] = _mm_set_ss(atime);
		cf[2] = _mm_set_ss(1.0f - rtime);
		cf[3] = _mm_set_ss(rtime);
		VectorFunctions::copy(r, d, size);
		switch(type) {
		case 0:
			VectorFunctions::abs(r, size);
			VectorFunctions::add(r, ANTI_DENORMAL_FLOAT, size);
			break;
		case 1:
			VectorFunctions::mul(r, r, size);
			VectorFunctions::add(r, ANTI_DENORMAL_FLOAT, size);
			break;
		default:
			VectorFunctions::mul(r, r, size);
			VectorFunctions::add(r, ANTI_DENORMAL_FLOAT*ANTI_DENORMAL_FLOAT, size);
			break;
		}
		__m128 r0,r1,r2,r3,r4, cc = _mm_set_ss(c);
		for (i=0; i<size; i++) {
			r0 = _mm_load_ss(r+i);
			r2 = _mm_mul_ss(cc , cf[0]);
			r4 = _mm_mul_ss(r0 , cf[1]);
			r3 = _mm_mul_ss(cc , cf[2]);
			r1 = _mm_mul_ss(r0 , cf[3]);
			r0 = _mm_cmpgt_ss(r0 , cc);
			r2 = _mm_add_ss(r4 , r2);
			r1 = _mm_add_ss(r1 , r3);
			r2 = _mm_and_ps(r0 , r2);
			r1 = _mm_andnot_ps(r0 , r1);
			cc = _mm_or_ps(r2 , r1);
			_mm_store_ss(r+i , cc);
		}
		c = r[size-1];
		if (type >= 2) {VectorFunctions::fsqrt(r, size);}
	#endif
	}

	// fundamental frequency detector based on normalized autocorrelation
	// input:	signal d[0..size-1], type = normalization scheme:
	//			0 McLeod/Wyvill (intended for musical applications)
	//			1 Cauchy-Schwarz inequality (mathematical fundamental)
	//			2 biased Cauchy-Schwarz inequality (tunable compromise)
	//			3 YIN (optimized for speech, seemingly withdrawn patent WO02097793)
	// output:	[re,im]	= [fundamental frequency relative to fs, 
	//					   tonality measure (0 untuned..1 pure harmonic)]
	// note:	precision of "tonality" does not depend on a correct fundamental
	Complex FrequencyAnalysis::getFundamentalFrequency(float* d, int size, FrequencyAnalysis::NormalisationScheme type)
	{
		static const float clim = 0.5f;			// empirical: >50% correlation
		static const float bslope = 0.1f;		// empirical: >0
		static const float sclim = clim*clim;
		int i,j,imax; float ns1,ns2,n,rmax,nmax,temp,bias,bdec,yin; Complex res;
		int hsize = size>>1;
		int tsize=1; while (tsize < (2*size)) {tsize<<=1;}

		const bool buffer1Initialised = fundamentalFreqBuffer1.getNumSamples() == tsize;
		const bool buffer2Initialised = fundamentalFreqBuffer2.getNumSamples() == hsize;
		const bool buffer3Initialised = fundamentalFreqBuffer3.getNumSamples() == hsize;
	
		if (!buffer1Initialised ||
			!buffer2Initialised ||
			!buffer3Initialised)
		{
			// You need to initialize the buffers first!
			jassertfalse;
		}

		float* r = fundamentalFreqBuffer1.getWritePointer(0);
		float* n1 = fundamentalFreqBuffer2.getWritePointer(0);
		float* n2 = fundamentalFreqBuffer3.getWritePointer(0);
	
		// fast biased autocorrelation via FFT
		VectorFunctions::copy(r,d,size); 
	
		realFloatFFTs->fastAutoCorrelation(r,size);	

		// calculate normalization data
		ns1=0; for (i=0; i<size; i++) {ns1 += (d[i]*d[i]);}
		ns2=ns1;
		for (i=0; i<hsize; i++) {
			n1[i] = ns1; n2[i] = ns2;
			ns1 -= (d[size-1-i]*d[size-1-i]); ns2 -= (d[i]*d[i]);
		} 
	
		// find maximum of normalized parabolic interpolated autocorrelation
		// beyond the first point of complete decorrelation, save a division
		// by swapping normalization factors in comparisons
		imax=1;
		if (type != NormalisationScheme::YIN) {while ((r[imax]>=0) && (imax<hsize)) {imax++;}}
		rmax=0; nmax = 1.0f; i=imax;
		if (type == NormalisationScheme::McLeodWyvill) {					// McLeod/Wyvill
			while (i<hsize) {		
				if ((r[i] > 0) && (r[i]>r[i-1]) && (r[i]>=r[i+1])) {
					temp = 0.25f*(r[i+1]-r[i-1]);
					temp = r[i] - temp*temp/(0.5f*(r[i+1]+r[i-1]) - r[i]);
					n = n1[i] + n2[i];
					if ((rmax*n) < (temp*nmax)) {imax=i; rmax=temp; nmax=n;}
					i+=2;
				}
				else {i++;}
			}
		}
		else if (type == NormalisationScheme::CauchySchwarz) {				// Cauchy-Schwarz inequality
			while (i<hsize) {
				if ((r[i] > 0) && (r[i]>r[i-1]) && (r[i]>=r[i+1])) {
					temp = 0.25f*(r[i+1]-r[i-1]);
					temp = r[i] - temp*temp/(0.5f*(r[i+1]+r[i-1]) - r[i]);
					n = n1[i]*n2[i]; temp *= temp;
					if ((rmax*n) < (temp*nmax)) {imax=i; rmax=temp; nmax=n;}
					i+=2;
				}
				else {i++;}
			}
		}
		else if (type == NormalisationScheme::BiasedCauchySchwarz) {				// biased Cauchy-Schwarz inequality
			while (i<hsize) {
				if ((r[i] > 0) && (r[i]>r[i-1]) && (r[i]>=r[i+1])) {
					temp = 0.25f*(r[i+1]-r[i-1]);
					temp = r[i] - temp*temp/(0.5f*(r[i+1]+r[i-1]) - r[i]);
					n = n1[i]*n2[i]; temp *= temp;
					if ((rmax*n) < (temp*nmax)) {
						imax=i; rmax=temp; nmax=n;
						if (temp > (n*sclim)) {i+=2; break;}
					}
					i+=2;	
				}								
				else {i++;}
			}
			bias = 1.0f; bdec = bslope/static_cast<float>(imax);
			hsize = __min(hsize, i + static_cast<int>((1.0f-clim)*bias/bdec));
			while (i<hsize) {
				if ((r[i] > 0) && (r[i]>r[i-1]) && (r[i]>=r[i+1])) {
					temp = 0.25f*(r[i+1]-r[i-1]);
					temp = r[i] - temp*temp/(0.5f*(r[i+1]+r[i-1]) - r[i]);
					bias -= bdec; temp *= bias; temp *= temp; n = n1[i]*n2[i];
					if ((rmax*n) < (temp*nmax)) {imax=i; rmax=temp; nmax=n;}
					i+=2;
				}
				else {i++;}
			}
		}
		else {								// YIN (excl. step 6, see paper)
			yin=0; n=1.0f; bias=0; bdec = 1.0f/static_cast<float>(hsize);
			for (j=1; j<hsize; j++) {
				bias += bdec;
				temp = 1.0f+(0.5528f+(0.4624f*bias - 0.0152f)*bias)*bias;
				r[j] = temp*(n1[j] + n2[j] - 2.0f*r[j]);
				yin += r[j]; n1[j] = n*r[j]/yin; n += 1.0f; 
			}
			n1[0]=1.0f;
			hsize--;
			while (i < hsize) {
				if ((n1[i]<n1[i-1]) && (n1[i]<=n1[i+1])) {
					temp = 0.25f*(n1[i+1]-n1[i-1]);
					temp = n1[i] - temp*temp/(0.5f*(n1[i+1]+n1[i-1]) - n1[i]);
					if (nmax > temp) {
						imax=i; nmax=temp;
						if (nmax < 0.1f) {hsize = __min(hsize,1+imax+imax/5);}
					}
					i+=2;
				}								
				else {i++;}
			}
		}

		// obtain fundamental frequency and tonality measure by parabolic
		// interpolation of the normalized autocorrelation around the maximum
		switch(type)
		{
		case NormalisationScheme::McLeodWyvill:		// McLeod/Wyvill
		case NormalisationScheme::CauchySchwarz:		// Cauchy-Schwarz inequality
		case NormalisationScheme::BiasedCauchySchwarz:		// biased Cauchy-Schwarz inequality
			temp = n1[imax] + n2[imax];
			res = SpecMath::paraext(n1[imax-1] + n2[imax-1] - 2.0f*r[imax-1],
									temp - 2.0f*r[imax],
									n1[imax+1] + n2[imax+1] - 2.0f*r[imax+1]);
			if (temp >= FLT_MIN) {res.im = 1.0f - res.im/temp;} else {res.im = 0;}
			break;
		default:	// YIN
			res = SpecMath::paraext(r[imax-1],r[imax],r[imax+1]);
			res.im = 1.0f - nmax;
			break;
		}
		res.re = 1.0f/(res.re + static_cast<float>(imax));
		res.im = __max(0,__min(1.0f,res.im));

		delete[] n1; delete[] n2; delete[] r;
		return res;
	}


	void FrequencyAnalysis::prepareFundamentalFrequencyBuffers(int size)
	{
		const int hsize = size >> 1;
		int tsize = 1; 
	
		while (tsize < (2 * size)) { tsize <<= 1; }
	
		fundamentalFreqBuffer1 = AudioSampleBuffer(1, tsize);
		fundamentalFreqBuffer2 = AudioSampleBuffer(1, hsize);
		fundamentalFreqBuffer3 = AudioSampleBuffer(1, hsize);

		float* r; r = new float[tsize];
		float* n1; n1 = new float[hsize];
		float* n2; n2 = new float[hsize];

	}

	// verify given fundamental frequency against high resolution spectrum
	// input:	amplitude spectrum amp[0..size-1] as obtained from "spec",  
	//			fundamental frequency fo relative to fs (e.g. from "fundamental")
	// output:	return index i (-> freq,amp[i]) of verified fundamental frequency
	int FrequencyAnalysis::verifyFundamentalFrequency(float* amp, int size, float fo)
	{
		int idx[12]; float a[12];
		int i,j,idxlo,idxhi, hsize = size>>1; float fc,finc;

		// find peak amplitudes and frequencies on the frequency grid
		fc = finc = 0.5f*fo*static_cast<float>(size);
		for (i=0; i<12; i++) {
			idxlo = static_cast<int>(0.95f*fc + 0.5f);
			idxhi = static_cast<int>(1.05f*fc + 0.5f);
			if (idxlo <= 0) {idx[i] = 0; a[i]=0;}
			else if (idxhi >= hsize) {
				if (idxlo < hsize) {
					idx[i] = idxlo + VectorFunctions::maxi(amp+idxlo,hsize-idxlo);
					a[i] = amp[idx[i]];
				}
				else {
					for (j=i; j<12; j++) {idx[j] = hsize-1; a[j]=0;}
					break;
				}
			}
			else {
				idx[i] = idxlo + VectorFunctions::maxi(amp+idxlo,idxhi-idxlo+1);
				a[i] = amp[idx[i]];
			}
			fc += finc;
		}
	
		// calculate metrics and perform fundamental verification
		float octdown=0, orig=0, octup=0, thirdup=0;
		orig	= a[1] + a[3] + a[5] + a[7] + a[9] + a[11];
		octdown	= orig + a[0] + a[2] + a[4];
		octup	= a[3] + a[7] + a[11];
		thirdup	= a[5] + a[11];
		if (octdown > (1.1f*orig)) {i=0;}
		else if (octup > (0.9f*orig)) {i=3;}
		else if (thirdup > (0.9f*orig)) {i=5;}
		else {i=1;}
		return idx[i];
	}

	// extract harmonic spectrum and inharmonicity
	// input:	amplitude spectrum amp[0..size-1] as obtained from "spec"
	//			associated frequencies freq[0..size-1] as obtained from "spec" 
	//			fundamental frequency fo relative to fs (e.g. from "fundamental")
	// output:	indices idx[0..hms-1] of harmonics in amp[] and freq[],
	//				example: freq[idx[0]] = frequency of fundamental
	//			return inharmonicity j that approximates the n-th partial 
	//			frequency as fn = n*fo*(1+j*(n^2-1))
	float FrequencyAnalysis::harmonics(	int* idx, int hms, float* amp,
									float* freq, int size, float fo		)
	{
		int i,j,idxlo,idxhi, hsize = size>>1;
		float tmp,tmp2,fc, n=1.0f, ih=0, s0=0, s1=0;
		float fidx = fo*static_cast<float>(size);

		for (i=0; i<hms; i++) {
			tmp2 = n*n - 1.0f;
			fc = n*fidx*(1.0f + ih*tmp2);
			idxlo = static_cast<int>(0.95f*fc + 0.5f);
			idxhi = static_cast<int>(1.05f*fc + 0.5f);
			if (idxlo <= 0) {idx[i] = 0; tmp=0;}
			else if (idxhi >= hsize) {
				if (idxlo < hsize) {	
					idx[i] = idxlo + VectorFunctions::maxi(amp+idxlo,hsize-idxlo);
					tmp = tmp2*amp[idx[i]];
				}
				else {
					for (j=i; j<hms; j++) {idx[j] = hsize-1;}
					break;
				}
			}
			else {
				idx[i] = idxlo + VectorFunctions::maxi(amp+idxlo,idxhi-idxlo+1);
				tmp = tmp2*amp[idx[i]];
			}
			s0 += (tmp*(freq[idx[i]]/n - fo));
			s1 += (fo*tmp*tmp2);
			if (i >= 5) {if (s1 >= FLT_MIN) {ih = s0/s1;}}
			n += 1.0f;
		}
		if ((ih == 0) && (s1 >= FLT_MIN)) {ih = s0/s1;}
		return ih;
	}

	// zero crossing counter
	// input:	signal d[0..size-1], continuation data c: init with 0
	// output:	return estimated number of zero crossings per sample (0:no estimate)
	//			continuation data c
	float VariousAudioAnalysis::zerocross(float* d, float &c, int size)
	{
		int j, i=1, cnt=-1, last=0;
		float start=0, stop;
		if ((c*d[0]) < 0) {
			start = d[0]/(c - d[0]);
			cnt++;
		}
		else {
			while (i<size) {
				if ((d[i-1]*d[i]) < 0) {	
					start = static_cast<float>(i) + d[i]/(d[i-1] - d[i]);
					cnt++; 
					i++; 
					break;
				}
				i++;
			}
		}
		for (j=i; j<size; j++) {if ((d[j-1]*d[j]) < 0) {last=j; cnt++;}}
		c = d[size-1];
		if (cnt > 0) {	
			stop = static_cast<float>(last) + d[last]/(d[last-1] - d[last]);
			return static_cast<float>(cnt)/(stop - start);
		}
		return 0;	
	}

	// spectral flatness within a defined frequency band
	// input:	power lower half spectrum d[0..size-1] as obtained from "spec"
	//			f1,f2 = band limit frequencies relative to fs
	// output:	return spectral flatness -> 0(pure oscillations)..1(white noise)
	float VariousAudioAnalysis::spectralflatness(float* d, int size, float f1, float f2)
	{
		int n,lbin,hbin;
		float mari,mgeo, dsize = 2.0f*static_cast<float>(size);
		lbin = __max(1,__min(size-1, static_cast<int>(__min(f1,f2)*dsize + 0.5f)));
		hbin = __max(1,__min(size-1, static_cast<int>(__max(f1,f2)*dsize + 0.5f)));
		n = hbin - lbin + 1;
		mari = VectorFunctions::mean(d+lbin,n);
		mgeo = VectorFunctions::gmean(d+lbin,n);
		if (mari >= FLT_MIN) {return mgeo/mari;} else {return 1.0f;} 
	}

	// upper cutoff frequency 
	// input:	power lower half spectrum d[0..size-1] as obtained from "spec"
	//			pth =	accumulated power threshold (0.5 for cutoff frequency
	//					of 1st order lowpass spectrum, 0.95 for perceived
	//					upper band limit)
	// output:	return cutoff frequency relative to fs
	float VariousAudioAnalysis::ufc(float* d, int size, float pth)
	{
		int i, idx = size;
		float x=0, plim = pth*VectorFunctions::sum(d+1,size-1);
		if (plim < FLT_MIN) {return 0;}
		for (i=1; i<size; i++) {x += d[i]; if (x > plim) {idx=i; break;}}
		if (idx == size) {return 0.5f;}
		x = static_cast<float>(idx) + 0.5f + (plim - x)/d[idx];
		return x*0.5f/static_cast<float>(size);
	}

	// lower cutoff frequency 
	// input:	power lower half spectrum d[0..size-1] as obtained from "spec"
	//			pth =	accumulated power threshold (0.5 for cutoff frequency
	//					of 1st order highpass spectrum, 0.95 for perceived
	//					lower band limit)
	// output:	return cutoff frequency relative to fs
	float VariousAudioAnalysis::lfc(float* d, int size, float pth)
	{
		int i, idx = 0;
		float x=0, plim = pth*VectorFunctions::sum(d+1,size-1);
		if (plim < FLT_MIN) {return 0.5f;}
		for (i=size-1; i>0; i--) {x += d[i]; if (x > plim) {idx=i; break;}}
		if (idx == 0) {return 0;}
		x = static_cast<float>(idx) - 0.5f + (x - plim)/d[idx];
		return x*0.5f/static_cast<float>(size);
	}

	// spectral flux
	// compare two spectra independent of their power, return similarity measure
	// input:	magnitude half spectrum d[0..size-1] -> 
	//				well-balanced inclusion of weaker components
	//			power half spectrum d[0..size-1] -> 
	//				changes of strong components dominate
	//			continuation data c[0..size-1]: init with 0
	// output:	return spectral similarity -> 0(identical)..1(maximally different)
	//			continuation data c[0..size-1]
	float VariousAudioAnalysis::spectralflux(float* d, float* c, int size)
	{
		float n = VectorFunctions::norm(d+1,size-1);
		float x = n*c[0];
		c[0] = n;
		if (x >= FLT_MIN) {x = sqrtf(fabsf(1.0f - VectorFunctions::dotp(c+1,d+1,size-1)/x));}
		else {x = 0;}
		VectorFunctions::copy(c+1,d+1,size-1);
		return x;
	}		

	// amplitude-based attack transient detector
	// all times in sampling intervals
	// input:	signal d[0..size-1]
	//			highest sensitivity for rise times between mintime and maxtime
	//			disable detection for rtime after a transient
	//			threshold th inhibits detection for signals with amplitude < th
	//			continuation data c[0..2]: init with 0	
	// output:	transientness r[0..size-1], positive, indicates transient if > 1
	//			continuation data c[0..2]
	void VariousAudioAnalysis::transamp(float* d, float* r, float* c, int size,
								 float mintime, float maxtime, float rtime, float th)
	{
		int i; float x;
		maxtime = 1.0f/__max(1.0f,maxtime);	
		mintime = 1.0f/__max(1.0f,mintime);			
		for (i=0; i<size; i++) {
		
			x = fabsf(d[i]) + ANTI_DENORMAL_FLOAT; 
			if (x > c[0]) {c[0] += (mintime*(x - c[0]));}
			else {c[0] += (maxtime*(x - c[0]));}
		
			if (c[2] > 0) {c[1] = c[0]; c[2] -= 1.0f;}
			else {x = c[0] - c[1]; c[1] += (maxtime*x);}

			x = 1.4f*(c[0]/(c[1] + th) - 1.0f);
			r[i] = __max(0,x);
			if (r[i] > 1.0f) {c[2] = rtime;}
		}	
	}

	// spectrum-based general transient detector
	// input:	magnitude half spectrum d[0..size-1]
	//			continuation data c[0..size-1]: init with 0
	// output:	return transientness, positive, indicates transient if > 1
	//			continuation data c[0..size-1]
	float VariousAudioAnalysis::transspec(float* d, float* c, int size)
	{
		int i; float diff = ANTI_DENORMAL_FLOAT;
		for (i=1; i<size; i++) {diff += fabsf(d[i]-c[i]);}
		c[0] += 0.25f*(diff - c[0]);
		VectorFunctions::copy(c+1,d+1,size-1);
		return __max(0,1.44f*logf(diff/c[0]));
	}

	// partial tracker after McAulay/Quatieri
	// track ordering: higher index <-> higher frequency, max(tracks) = size/2
	// input:	frequency/amplitude pairs freq/amp[0..size-1] obtained from "spec"
	//			tracks = number of currently existing tracks: init with 0
	//			continuation data c[0..size]: init with 0
	// output:	freq/amp indices trkidx[0..tracks-1]
	//				usage:	freq[trkidx[2]] = frequency of track 2
	//			track links trklink[0..tracks-1]
	//				usage:	trklink[2] = old index of track that is now track 2,
	//									 -1 indicates new track
	//			tracks = updated number of tracks
	//			continuation data c[0..size]	
	void VariousAudioAnalysis::mqtracks(int* trkidx, int* trklink, int& tracks,
								 float* freq, float* amp, float* c, int size)
	{
		int i,j,k,m,lo,hi,best,second, hsize = size>>1;
		float f,fnexthigher,fsize,diff,temp;
		int* ttrk; ttrk = new int[hsize];
		float* tamp; tamp = c + hsize + 1;
		VectorFunctions::copy(tamp,amp,hsize);

		// update tracks
		tracks = __min(tracks,hsize);
		c[hsize] = -1.0f;
		for (i=0; i<tracks; i++) {

			// frequencies of current and next higher track
			f = c[i]; fnexthigher = c[i+1];			

			// range of candidate peak indices for track continuation
			fsize = 2.0f*f*static_cast<float>(hsize);
			lo = __min(hsize-1,__max(0,static_cast<int>(0.95f*fsize + 0.5f)));	
			hi = __min(hsize-1,__max(0,static_cast<int>(1.05f*fsize + 0.5f)));

			// find peak with frequency closest to those of the track,
			// if the next higher frequency peak fits better, the 2nd closest
			// peak is assigned to the track, if no peak is found, the track
			// ends
			best = second = -1; diff = FLT_MAX;
			for (j=lo; j<=hi; j++) {
				if (tamp[j] > 0) {						// is peak?
					temp = fabsf(freq[j] - f);
					if (temp < diff) {diff=temp; second=best; best=j;}
				}
			}

			// update track
			if (best < 0) {ttrk[i] = -1;}				// end track
			else if (fabsf(freq[best]-fnexthigher) >= diff) {
				ttrk[i]=best;							// assign peak to track
				tamp[best]=0;							// remove peak from amp
			}
			else {
				if (second < 0) {ttrk[i] = -1;}
				else {ttrk[i]=second; tamp[second]=0;}
			}
			trklink[i] = i;
		}

		// assign remaining peaks to new tracks while simultaneously sorting them
		lo = hi = k = m = 0;
		for (i=0; i<tracks; i++) {
			if (ttrk[i] >= 0) {
				hi = ttrk[i];
				for (j=lo; j<hi; j++) {
					if (tamp[j] > 0) {trkidx[k]=j; trklink[k]=-1; k++;}
				}
				trkidx[k] = hi; trklink[k] = m;
				k++; m++;
				lo = hi+1;
			}
		}
		for (j=lo; j<hsize; j++) {
			if (tamp[j] > 0) {trkidx[k]=j; trklink[k]=-1; k++;}
		}
		for (i=0; i<k; i++) {c[i] = freq[trkidx[i]];}
	
		// clean up
		tracks = k;
		delete[] ttrk;
	}

	//******************************************************************************
	//* resynthesis
	//*
	// add time-varying cosine wave to existing data matching frequencies and phases
	// at both ends and changing the amplitude linearly, uses cubic phase function,
	// end point not included for seamless concatenation of subsequent data segments
	// input:	audio data d[0..size-1]
	//			(start,end) frequency (fs,fe) normalized to sample rate
	//			(start,end) amplitude (as,ae)
	//			(start,end) phase (ps,pe) in radians (needn't be unwrapped!)
	// output:	audio data d[0..size-1]
	void Resynthesis::mqsyn(float* d, int size, float fs, float fe,
							  float as, float ae, float ps, float pe)
	{
		static const float TWOPI = 2.0f*M_PI_FLOAT;
		float len = static_cast<float>(size);
		float invlen = 1.0f/len;
	
		// calculate cubic phase correction
		float p,err; double a2,a3;
		err = (ps - pe + M_PI_FLOAT*(fs + fe)*len);
		p = floorf(err/TWOPI + 0.5f);
		err = (TWOPI*p - err)*invlen*invlen;
		a2 = static_cast<double>(M_PI_FLOAT*(fe - fs)*invlen + 3.0f*err);
		a3 = static_cast<double>(-2.0f*err*invlen);

		// synthesize sinusoid
		int i; double c0,c1,c2,c3,amp,ampinc,ar,ai,xr,xi,yr,yi,zr,zi,temp;
		c0 = static_cast<double>(ps);
		c1 = 2.0*M_PI*static_cast<double>(fs) + a2 + a3;
		c3 = 6.0*a3;
		c2 = 2.0*a2 + c3;
		xr = cos(c0); xi = sin(c0);
		yr = cos(c1); yi = sin(c1);
		zr = cos(c2); zi = sin(c2);
		ar = cos(c3); ai = sin(c3);
		amp = as; ampinc = invlen*(ae-as);
		for (i=0; i<size; i++) {
			d[i] += static_cast<float>(amp*xr); amp += ampinc;
			temp = xr*yr - xi*yi; xi = xi*yr + xr*yi; xr = temp;
			temp = yr*zr - yi*zi; yi = yi*zr + yr*zi; yr =temp;
			temp = zr*ar - zi*ai; zi = zi*ar + zr*ai; zr = temp;
		}
	}

	// add time-varying cosine wave to existing data matching frequencies at both
	// ends and phase at the starting point, end point phase is the integral of
	// the linearly interpolated frequency, amplitude changes linearly, end
	// point not included for seamless concatenation of subsequent data segments
	// input:	audio data d[0..size-1]
	//			(start,end) frequency (fs,fe) normalized to sample rate
	//			(start,end) amplitude (as,ae)
	//			start phase ps in radians (needn't be unwrapped!)
	// output:	audio data d[0..size-1]
	//			return start phase of next segment
	float Resynthesis::mqsyn(float* d, int size, float fs, float fe,
								float as, float ae, float ps)
	{
		static const float TWOPI = 2.0f*M_PI_FLOAT;
		int i; float t; double c1,c2,amp,ampinc,ar,ai,xr,xi,yr,yi,temp;
		float len = static_cast<float>(size);
		float invlen = 1.0f/len;
	
		c1 = static_cast<double>(TWOPI*fs);
		c2 = static_cast<double>(TWOPI*(fe-fs)*invlen);
		xr = cos(static_cast<double>(ps)); xi = sin(static_cast<double>(ps));
		yr = cos(c1 + 0.5*c2); yi = sin(c1 + 0.5*c2);
		ar = cos(c2); ai = sin(c2);
		amp = static_cast<double>(as);
		ampinc = static_cast<double>(invlen*(ae-as));
		for (i=0; i<size; i++) {
			d[i] += static_cast<float>(amp*xr); amp += ampinc;
			temp = xr*yr - xi*yi; xi = xi*yr + xr*yi; xr = temp;
			temp = yr*ar - yi*ai; yi = yi*ar + yr*ai; yr =temp;
		}
		t = 0.5f + ps/TWOPI + 0.5f*(fs+fe)*len; t -= (0.5f + floorf(t));
		return TWOPI*t;
	}

