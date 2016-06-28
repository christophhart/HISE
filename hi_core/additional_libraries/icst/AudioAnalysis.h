// AudioAnalysis.h
//************************** audio analysis library ******************************
//
// This code is part of the ICST DSP Library version 1.2. It is released
// under the 2-clause BSD license. Copyright (c) 2008-2010, Zurich
// University of the Arts, Beat Frei. All rights reserved.

#ifndef _ICST_DSPLIB_AUDIOANALYSIS_INCLUDED
#define _ICST_DSPLIB_AUDIOANALYSIS_INCLUDED

namespace icstdsp {		// begin library specific namespace

class AudioAnalysis
{
//******************************* user methods ***********************************
public:
//***	spectral analysis	***
// high resolution spectral analysis
// array sizes: (d,freq,amp,time,temp)[size],(w,dw,rw)[size+1], aic[4]
// size = 2^n with n>0
// freq/amp/time can be NULL, in which case the vectors are not calculated
// note: time reassignment yields the energy barycenter of the WINDOWED input ->
// a WIDE pulse is located well only if it falls into a flat region of the window
// input:	data d, obtain from prespec: w/dw/rw/aic, just workspace: temp
// output:	complex half spectrum of windowed d -> d
//			reassigned frequencies relative to fs -> freq[0..size/2-1],
//			freq[size/2..size-1] undefined
//			reassigned time position (-0.5..0.5) relative to center -> 
//			time[0..size/2-1], time[size/2..size-1] undefined
//			reassigned amplitudes of peaks -> amp[0..size/2-1], no peak:0,
//			amp[size/2..size-1] undefined  
static void spec(float* d, float* freq, float* amp, float* time, int size,
				 float* w, float* dw, float* rw, float* temp, float* aic);

// obtain spec parameters
// input:	symmetric window w[size+1]
// output:	dw[size+1], rw[size+1], aic[4]								
static void prespec(float* w, float* dw, float* rw, float* aic, int size);	

//***	analysis by decomposition to arbitrary functions	***
// use the matching pursuit algorithm to approximate the data as a weighted sum
// of atoms from the dictionary dict:
// d[0..size-1] ~ sum(i=0..elements-1){weight[i]*atom(idx[i])[0..size-1]}
// input:	data[0..size-1] 
//			dict[atom0[0..size-1] atom1[0..size-1] .. atom(atoms-1)[0..size-1]]
//			***	!!! ALL ATOMS MUST HAVE A L2-NORM OF 1 !!! ***
// output:	weight[0..elements-1], idx[0..elements-1], residual data[0..size-1]	
static void matchingpursuit(float* weight, int* idx, int elements,
							float* dict, int atoms, float* data, int size);   
		
//***	cepstral analysis	***
// get real cepstrum from magnitude spectrum
// size = 2^n with n>0
// input:	magnitude lower half spectrum d[0..size]
// output:	real lower half cepstrum d[0..size]
static void magspectorceps(float* d, int size);

// get magnitude spectrum from real cepstrum
// size = 2^n with n>0
// input:	real lower half cepstrum d[0..size]
// output:	magnitude lower half spectrum d[0..size]
static void rcepstomagspec(float* d, int size); 

// calculate mel frequency cepstral coefficients (MFCC) from power spectrum
// (mathematically more conclusive) or magnitude spectrum (see ETSI ES 201108)
// typ. frame: duration = 25 ms, overlap = 50%, hamming windowed
// input:	lower half spectrum d[0..size], bands = number of filter bands,
//			cofs = number of coefficients, sample rate fs, lowest(highest)
//			analysis frequency fmin(fmax)
// output:	MFCC c[0..coeffs-1]
static void spectomfcc(float* d, float* c, int size, int bands=23, int cofs=13,
					   float fs=44100, float fmax=8000, float fmin=64);

//***	linear prediction	***
// example:	const int order = 10;
//			double a[order+1], k[order];
//			float rm[order+1], c[order]; BlkDsp::set(c,0,order);
//			BlkDsp::bacorr(rm,inputdata,order+1,1000);		
//			result = lpdurbin(a,k,rm,order);
//			if (result.re == 1.0f) {	handle unstable solution, never 
//										occurs with biased autocorrelation	}
//			lpanalyze(inputdata,c,a,1000,order);
												
// levinson-durbin recursion: solve rm[]*a[1 ..]=[err 0..0] for a[] and err
// input:	rm[0..order] = biased autocorrelation, obtained e.g. by "bacorr"
// output:	a[0..order] = linear prediction coefficients 
//			k[0..order-1] = reflection coefficients
// return:	re = maximum absolute reflection coefficient (1 if unstable)
//			im = residual to input energy ratio
// notes:	using biased autocorrelation guarantees a stable predictor, which
//			furthermore will perform similar to the burg and covariance method
//			if the input is longer than 20*order AND hamming or hann windowed	
static cpx lpdurbin(double* a, double* k, float* rm, int order);

// calculate LPC residual from original signal
// input:	data d[0..size-1], prediction coefficients a[0..order],
//			block continuation data c[0..order-1]: zero fill for first call  
// output:	data d[0..size-1], c[0..order-1] for next call with cont. data
static void lpanalyze(float* d, float* c, double* a, int size, int order);

// restore original signal from LPC residual using a direct form IIR filter
// input:	data d[0..size-1], prediction coefficients a[0..order],
//			block continuation data c[0..order-1]: zero fill for first call 
// output:	data d[0..size-1], c[0..order-1] for next call with cont. data
static void lpdsynth(float* d, double* c, double* a, int size, int order);

// restore original signal from LPC residual using a lattice IIR filter,
// the second version allows gliding linearly from one k-set to another
// input:	data d[0..size-1], reflection coefficients k[0..order-1] or
//			kstart[0..order-1] and kend[0..order-1] respectively
//			block continuation data c[0..order-1]: zero fill for first call 
// output:	data d[0..size-1], c[0..order-1] for next call with cont. data
static void lplsynth(float* d, double* c, double* k, int size, int order);
static void lplsynth(float* d, double* c, double* kstart, double* kend,
					 int size, int order);

// calculate line spectral frequencies (LSFs) from even order LPC coefficients
// input:	linear prediction coefficients a[0..order]
//			grid = number of bins (a power of 2 larger than 4*order+4) to look
//			for frequencies (can be separated if at least 2*fs/grid apart) 
// output:	ascending line spectral frequencies f[0..order-1] relative to fs
//			return false if frequencies are not separable on given grid
static bool lpctolsf(float* f, double* a, int order, int grid = 1024); 

// restore original signal from even order LPC residual using LSFs with an IIR
// filter, the second version allows gliding linearly from one f-set to another
// input:	data d[0..size-1], line spectral frequencies f[0..order-1] or
//			fstart[0..order-1] and fend[0..order-1] respectively,
//			block continuation data c[0..2*order]: zero fill for first call 
// output:	data d[0..size-1], c[0..2*order] for next call with continuous data
static void lpssynth(float* d, float* f, double* c, int size, int order);
static void lpssynth(float* d, float* fstart, float* fend, double* c,
					 int size, int order);

//***	adaptation ***
// normalized LMS algorithm: estimate y based on x minimizing e = y{est}(x) - y
// input:	source data x[0..size-1], reference data y[0..size-1]
//			order = filter order, mu = adaptation rate: 0..1(fastest stable)
//			current filter coefficients h[0..order]: zero fill for first call 
//			continuation data c[0..order-1]: zero fill for first call
// output:	error signal e[0..size-1]
//			new filter coefficients h[0..order]
//			c[0..order-1] for next call with continuous data
static void nlms(float* e, float* x, float* y, float* h, float* c,
				 int size, int order, float mu=1.0f);

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
static void costas(float* d, float* r, int size, float tbw, float dbw,
					float &fstart, float &pstart, float* c);	

//***	feature extraction	***
// envelope follower
// input:	signal d[0..size-1], continuation data c: init with 0
//			atime/rtime: attack/release time constant in sampling intervals
//			envelope type: 0 absolute value, 1 power, 2 RMS 
// output:	envelope r[0..size-1], continuation data c
static void envelope(float* d, float* r, float &c, int size, 
					 float atime=200.0f, float rtime=2000.0f, int type=0);

static void oldenvelope(float* d, float* r, float &c, int size, 
					 float atime=200.0f, float rtime=2000.0f, int type=0);

// fundamental frequency detector based on normalized autocorrelation
// input:	signal d[0..size-1], type = normalization scheme:
//			0 McLeod/Wyvill (intended for musical applications)
//			1 Cauchy-Schwarz inequality (mathematical fundamental)
//			2 biased Cauchy-Schwarz inequality (tunable compromise)
//			3 YIN (optimized for speech, seemingly withdrawn patent WO02097793)
// output:	[re,im]	= [fundamental frequency relative to fs, 
//					   tonality measure (0 untuned ... 1 pure harmonic)]
// note:	precision of "tonality" does not depend on a correct fundamental   
static cpx fundamental(float* d, int size, int type=2);

// verify given fundamental frequency against high resolution spectrum
// input:	amplitude spectrum amp[0..size-1] as obtained from "spec",  
//			fundamental frequency fo relative to fs (e.g. from "fundamental")
// output:	return index i (-> freq,amp[i]) of verified fundamental frequency
static int fundverify(float* amp, int size, float fo);

// extract harmonic spectrum and inharmonicity
// input:	amplitude spectrum amp[0..size-1] as obtained from "spec"
//			associated frequencies freq[0..size-1] as obtained from "spec" 
//			fundamental frequency fo relative to fs (e.g. from "fundamental")
// output:	indices idx[0..hms-1] of harmonics in amp[] and freq[],
//				example: freq[idx[0]] = frequency of fundamental
//			return inharmonicity j that approximates the n-th partial 
//			frequency as fn = n*fo*(1+j*(n^2-1))
static float harmonics(int* idx, int hms, float* amp, float* freq,
						int size, float fo);

// zero crossing counter
// input:	signal d[0..size-1], continuation data c: init with 0
// output:	return estimated number of zero crossings per sample (0:no estimate)
//			continuation data c
static float zerocross(float* d, float &c, int size);

// spectral flatness within a defined frequency band
// input:	power lower half spectrum d[0..size-1] as obtained from "spec"
//			f1,f2 = band limit frequencies relative to fs
// output:	return spectral flatness -> 0(pure oscillations)..1(white noise)
static float spectralflatness(float* d, int size, float f1, float f2);

// upper(lower) cutoff frequency ufc(lfc) 
// input:	power spectrum d[0..size-1]
//			pth = accumulated power threshold (0.5 for cutoff frequency
//					of 1st order lowpass(highpass) spectrum,
//					0.95 for perceived upper(lower) band limit)
// output:	return cutoff frequency relative to fs
static float ufc(float* d, int size, float pth=0.95);
static float lfc(float* d, int size, float pth=0.95);

// spectral flux
// compare two spectra independent of their power, return similarity measure
// input:	magnitude half spectrum d[0..size-1] -> 
//				well-balanced inclusion of weaker components
//			power half spectrum d[0..size-1] -> 
//				changes of strong components dominate
//			continuation data c[0..size-1]: init with 0
// output:	return spectral similarity -> 0(identical)..1(maximally different)
//			continuation data c[0..size-1]
static float spectralflux(float* d, float* c, int size);

// amplitude-based attack transient detector
// all times in sampling intervals
// input:	signal d[0..size-1]
//			highest sensitivity for rise times between mintime and maxtime
//			disable detection for rtime after a transient
//			threshold th inhibits detection for signals with amplitude < th
//			continuation data c[0..2]: init with 0	
// output:	transientness r[0..size-1], positive, indicates transient if > 1
//			continuation data c[0..2]
static void transamp(float* d, float* r, float* c, int size, float mintime=0,
					float maxtime=50.0f, float rtime=1000.0f, float th=0.001f);

// spectrum-based general transient detector
// input:	magnitude half spectrum d[0..size-1]
//			continuation data c[0..size-1]: init with 0
// output:	return transientness, positive, indicates transient if > 1
//			continuation data c[0..size-1]
static float transspec(float* d, float* c, int size);

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
static void mqtracks(	int* trkidx, int* trklink, int& tracks,
						float* freq, float* amp, float* c, int size	);

//***	resynthesis	  ***
// add time-varying cosine wave to existing data matching frequencies and phases
// at both ends and changing the amplitude linearly, uses cubic phase function,
// end point not included for seamless concatenation of subsequent data segments
// input:	audio data d[0..size-1]
//			(start,end) frequency (fs,fe) normalized to sample rate
//			(start,end) amplitude (as,ae)
//			(start,end) phase (ps,pe) in radians (needn't be unwrapped!)
// output:	audio data d[0..size-1]
static void mqsyn(float* d, int size, float fs, float fe, float as, float ae,
				  float ps, float pe);

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
static float mqsyn(float* d, int size, float fs, float fe, float as, float ae,
				   float ps);
};

}	// end library specific namespace

#endif

