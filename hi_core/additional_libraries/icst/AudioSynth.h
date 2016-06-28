// AudioSynth.h
//************************** audio synthesis library *****************************
// multithreading capability for x86/AMD64/ECMA memory models:
//	Update() runs in parallel with any other method called from a separate thread
//
// This code is part of the ICST DSP Library version 1.2. It is released
// under the 2-clause BSD license. Copyright (c) 2008-2010, Zurich
// University of the Arts, Beat Frei. All rights reserved.

#ifndef _ICST_DSPLIB_AUDIOSYNTH_INCLUDED
#define _ICST_DSPLIB_AUDIOSYNTH_INCLUDED

namespace icstdsp {		// begin library specific namespace

// wavetable oscillator
// control characteristics: pitch + pitchmod exponential, wave linear 
// phase modulation input: int(-2^31..2^31-1) <-> phase(-pi..pi)
// spectrum d = [Acos1,Asin1,...,Acos(tablesize/2),Asin(tablesize/2)] with
//				A(cos,sin)x = amplitude of (cosine,sine) part of x-th harmonic
class WaveOsc
{
public:
	WaveOsc(		int tablesize,		// size of a single wave (2^n,n>1)
					int tables,			// waves per wavetable (1..65536) 
					float maxpitch,		// maximum oscillator frequency
					float minpitch,		// minimum oscillator frequency
					float smprate	);	// sample rate (>0)
	~WaveOsc();	
	void LoadTable(						// load wavetable with spectrum d
					float* d,			// and normalize to RMS = 0.5		
					int idx			);	// wave index (0..tables-1)			
	void SetPhase(	float phase		);	// set phase synchronously (0..1) 
	void Update(						// audio and control update
					float* d,			// audio out
					int samples,		// number of samples
					float invsmp,		// 1/samples
					float pitch,		// static pitch control (0..1)
					float pitchmod,		// pitch modulation control (-1..1)
					float wave,			// wave selection control (0..1)
					int* pmod		);	// audio rate PM input
private:
	float* table;						// wavetable data
	int ssize;							// total wavetable size
	int tsize;							// wavetable size per pitch version
	int wsize;							// wave size
	int wtables;						// number of waves per pitch version
	int pblocks;						// number of pitch versions 
	float gamma;						// frequency range per pitch version
	float alpha;						// pitch switching aux variable
	float beta;							// aux variable used by LoadTable
	float pcrit;						// switch waves above this pitch
	int phi;							// phase accumulator
	int pshift;							// p.a. to wave shift factor
	int pmask;							// p.a. to wave frac mask
	float pscl;							// p.a. to wave frac factor
	float pconv;						// pitch conversion factor
	float pmin;							// minimum pitch
	int dphi;							// phase increment
	float pbkconv;						// pitch block conversion factor
	int w;								// control dependent wave selector
	int wshift;							// c.d.w.s. to wave shift factor
	int wmask;							// c.d.w.s. to wave frac mask
	float wscl;							// c.d.w.s. to wave frac factor
	double wconv;						// control input to wave no factor
	int nphi,pccnt1,pccnt2;				// SetPhase aux variables
	WaveOsc& operator = (const WaveOsc& src);	// do not allow copies
	WaveOsc(const WaveOsc& src);				// and assignments
};

// ring modulator
// includes prefilters (zero @ fs/2), correction postfilter, dc trap (fc=5hz)
class RingMod
{
public:
	RingMod(float smprate);				// sample rate
	void Update(float* in1,				// audio in 1
				float* in2,				// audio in 2
				float* out,				// audio out
				int samples);			// number of samples					
private:
	float in1d,in2d,od,dc,a;
	int adidx;
	float adtab[16];
};

// noise generator
// uniformly distributed white noise, range: -1..1, RMS amplitude: 0.577 
// pink noise, accuracy: +/- 0.3db (0.00045..0.45fs), RMS amplitude: 0.577
class Noise
{
public:
	Noise();
	void SetType(int type);				// set noise type: 0=white, 1=pink
	void Update(float* out,				// audio out
				int samples);			// number of samples
private:
	int ntype;
	float s1,s2,s3;
};

// static delay line
// implements clickless delay time change by this sequence:
//	1. crossfade delayed to original signal * zlevel within 20 ms
//	2. wait new delay time
//	3. crossfade back to delayed signal within 20 ms
class Delay
{
public:
	Delay(	int maxlen,					// maximum delay time in samples
			float smprate,				// sample rate in Hz
			float zlevel=1.0f	);		// thru gain during delay change 
	~Delay();
	void SetType(int dlen);				// set delay time in samples
	void Update(float* in,				// audio in
				float* out,				// audio out
				int samples);			// number of samples	
private:
	float* d;
	float a,invxflen,zlev;
	int cp,len,mlen,tlen,nlen,state,cntdwn,xflen;
	Delay& operator = (const Delay& src);
	Delay(const Delay& src);				
};

// time varying delay line
// uses 1st order allpass interpolation
class VarDelay
{
public:
	VarDelay(float maxlen);				// set maximum delay time in samples
	~VarDelay();
	void Update(float* in,				// audio in
				float* out,				// audio out
				int samples,			// number of samples
				float invsmp,			// 1/samples
				float length);			// delay time in samples				
private:
	float* d;							
	float outd,dscl,dscl2,llim;
	int wrapmask,widx,delay,dshift,dmask,adidx;
	float adtab[16];
	VarDelay& operator = (const VarDelay& src);	
	VarDelay(const VarDelay& src);
};

// sample playback oscillator
//	1. call PreComp once to prepare a sample for playback
//	2. call Update deliberately to play it with different parameters
// loops:
//	loop end position at least 8 samples away from sample end (recommended)
//		=>	fractional and integer length allowed, unlooped playback ok
//	loop end closer than 8 samples to sample end (in case of given loop data)
//		=>	integer length allowed, specify loop length in PreComp, reloading
//			through PreComp required for unlooped playback
//	no loop support for reverse playback
// transpose < 0: reverse playback, abs(transpose) = transpose factor
// Update return values: 
//	0 normal, 1 sample end reached, -1 sample start reached in reverse direction
//  for -1 and 1:	output is zeroed with negligible CPU load in subsequent 
//					calls of Update until smpstart >= 0 is specified or the
//					playback direction has changed
class SampleOsc
{
public:
	SampleOsc();
	~SampleOsc();
	void PreComp(	float* d,			// original sample d[length] ->
					int length,			// precompensated sample d[length+16]
					int looplen=0,		// specify if loop end > length-8 
					float srate=0	);	// specify if known: original sample rate
	int Update(		float* out,			// audio output
					float* d,			// precompensated sample
					int samples,		// number of samples to process
					float invsmp,		// 1/samples
					float transpose,	// transpose factor
					int endpos,			// end position, loop end (0..length)
					int startpos=-1,	// start position (<0: continue)
					float looplen=0	);	// loop length in samples, 0:no loop	
private:
	static int instances;				// number of object instances
	static float* c;					// interpolation coefficients
	union {								// sample index
		icstdsp_int64 idx;				//
		unsigned int idxint[2];			//
	};									//
	icstdsp_int64 didx;					// sample index increment
};

// envelope generator
// control input characteristics: time exponential, level linear
// special segment indices:
//	0 = attack segment, triggered by Event(0) 	
//	release segment, set by SetParam(ENVELOPE_RELSEG,..), triggered by Event(1)
//	maxsegs = idle segment, triggered by Event(2) 	
//
// global parameter IDs
const int ENVELOPE_LMOD	= 3;			// level modulation intensity (0..1)
const int ENVELOPE_RELSEG = 6;			// index of release segment (0..maxsegs)
// segment specific parameter IDs
const int ENVELOPE_TIME	= 0;			// time constant (0..1)
const int ENVELOPE_TMOD	= 1;			// time modulation intensity (-1..1)
const int ENVELOPE_LEVEL = 2;			// end level (-1..1)
const int ENVELOPE_STYPE = 4;			// type (any segment type ID)
const int ENVELOPE_NEXTS = 5;			// index of next segment (0..maxsegs)
// segment type IDs
const float ENVELOPE_ST_CONCAVE = 0;	// finite line segment with rounded start	
const float ENVELOPE_ST_CONVEX = 1.0f;	// finite line segment with rounded end
const float ENVELOPE_ST_CLASSICATTACK = 2.0f;	// attack shape of classic ADSR
												// generator 
const float ENVELOPE_ST_LINEAR = 3.0f;			// finite line segment
const float ENVELOPE_ST_SIGMOID = 4.0f;			// finite s-shaped segment
const float ENVELOPE_ST_INFINITEEXP = 5.0f;		// infinite exponential asymptote
// preset envelopes 
const int ENVELOPE_P_ADSR = 0;			// classic ADSR, segment indices:
										// 0 attack, 1 decay+sustain, 2 release
const int ENVELOPE_P_ABDSR = 1;			// ADSR with linear breakpoint segment 
										// between attack and decay, segment
										// indices: 0 attack, 1 breakpoint,
class Envelope							// 2 decay+sustain, 3 release
{
public:
	Envelope(		float tmax,			// minimum and maximum segment
					float tmin=1.0f,	// time constant in samples
					int maxsegs=16	);	// maximum number of segments
	~Envelope();
	void SetParam(	int pid,			// parameter ID
					float value,		// parameter value
					int segment=0	);	// envelope segment (if required)
	void Preset(	int pno			);	// set preset envelope
	void Event(		int id			);	// create event: 0 keydown,1 keyup,2 idle
	void Update(	float* out,			// control output
					int values,			// number of values to generate 
					float tmod=0,		// time modulation input (-1..1)
					float lmod=0	);	// level modulation input (0..1)
private:
	float tscl1,tscl2;					// time scale factors
	int relseg;							// release segment index
	int idleseg;						// idle segment index
	int* type;							// segment data: type
	int* nextsgm;						// .. index of next segment
	float* endlevel;					// .. end level
	float* time;						// .. time constant
	float* timemodint;					// .. time modulation intensity		
	float levelmodint;					// global level modulation intensity
	int event;							// most recent event
	int evcnt1,evcnt2;					// event counters
	int sgm;							// current segment index
	int cnt;							// segment countdown counter
	float cout;							// current output value
	float a,b,c,x,dx;					// iteration variables								
	void SetSegment(int newseg,			// new segment auxiliary method	
					float startlevel,	
					float tmod,
					float lmod		);
	Envelope& operator = (const Envelope& src);
	Envelope(const Envelope& src);	
};

// controlled amplifier
class Amp
{
public:
	Amp();
	void SetType(	int curve		);	// set control characteristic:
										// 0=linear, 1=square, 2=quartic
	void Update(	float* data,		// audio data
					int samples,		// number of samples to process
					float invsmp,		// 1/samples
					float amp		);	// amplitude		
private:
	int ccurve,ncurve,adidx;						
	float a;
	float adtab[16];
};

// second order multimode filter				 
// control characteristics: frequency exponential, resonance reciprocal
// modes:	lowpass (12dB/oct), bandpass (6dB/oct), highpass (12dB/oct),
//			peaking, notch
// gain @ res=0,freq=1,mode=0, linear version: 0dB
// approximate low-level gain @ res=0,freq=1,mode=0, nonlinear version: -6dB
class ChambFilter
{
public:
	ChambFilter(	float smprate,		// sample rate
					float fmin=5.0f	);	// minimum center frequency (Hz)
	~ChambFilter();
	void SetType(	int type		);	// set filter type: 0=LP, 1=BP, 2=HP,
										// 3=peaking, 4=notch, other=bypass
	void Update(	float* data,		// audio data
					int samples,		// number of samples
					float invsmp,		// 1/samples
					float freq,			// cutoff/center frequency
					float res		);	// resonance
	void UpdateCmp(	float* data,		// 
					int samples,		// nonlinear version with amplitude
					float invsmp,		// compression, resonance up to self
					float freq,			// oscillation
					float res		);	//
private:
	static int instances;				// number of object instances
	static float* tab;					// anti-denormal pink noise table
	int adidx;							// anti-denormal table index
	float a,b,lim,d,fc,fscl1,fscl2;
	int ftype;
};

// virtual analog moog lowpass filter
// key features: low-alias wideband nonlinearity, amplitude compression,
//				 resonance up to self-oscillation with precise tuning,
//				 improved gain evolution with increasing resonance
// control characteristics: frequency exponential, resonance reciprocal
// approximate low-level gain @ res=0,freq=1: -6dB
class MoogFilter
{
public:
	MoogFilter(		float smprate,		// sample rate
					float fmin=5.0f	);	// minimum center frequency (Hz)
	~MoogFilter();
	void Update(	float* data,		// audio data
					int samples,		// number of samples
					float invsmp,		// 1/samples
					float freq,			// cutoff/center frequency (0..1)
					float res		);	// resonance (0..1)
private:
	static int instances;				// number of object instances
	static float* tab;					// anti-denormal pink noise table
	int adidx;							// anti-denormal table index
	float s1,s2,s3,s4,s5,slim;			// filter states
	float s6,s7,s8;						// decimator states
	float fscl1,fscl2;					// frequency scale factors
	float fc;							// center frequency	control
	float dfc;							// c.f.c. increment
	float rc;							// resonance control
	float drc;							// r.c. increment
	float previn;						// previous scaled input value
};

// FM oscillator
// control characteristics: pitch exponential, amplitude + feedback linear 
// phase modulation input: int(-2^31..2^31-1) <-> phase(-pi..pi)
// output: carrier -> float(-1..1), modulator -> wrap_to_int(-2^35..2^35-1)
// feedback range: 0..1 -> none..maximum stable without noise
class FMOsc
{
public:
	FMOsc(			float maxpitch,		// maximum oscillator frequency
					float minpitch,		// minimum oscillator frequency
					float smprate	);	// sample rate
	~FMOsc();	
	void SetPhase(	float phase		);	// set phase synchronously (0..1)
	void UpdateMod(						// modulator audio and control update
					int* out,			// audio out
					int samples,		// number of samples
					float invsmp,		// 1/samples
					float pitch,		// pitch control
					float amp,			// amplitude control
					float fbk,			// PM feedback control
					int* pmod,			// audio rate PM input
					bool add=false);	// t: add output to out[]
	void UpdateCar(						// carrier audio and control update
					float* out,			// audio out
					int samples,		// number of samples
					float invsmp,		// 1/samples
					float pitch,		// pitch control
					float amp,			// amplitude control
					float fbk,			// PM feedback control
					int* pmod,			// audio rate PM input
					bool add=false);	// t: add output to out[]
private:
	static int instances;				// number of object instances
	static short int* tab;				// half period cosine table
	int phi;							// phase accumulator
	int dphi;							// phase increment
	int ampl;							// amplitude (modulator output)
	int fb;								// PM feedback intensity
	int tval;							// last cosine table value
	int nphi,pccnt1,pccnt2;				// SetPhase aux variables
	float pconv;						// pitch conversion factor
	float pmin;							// minimum pitch
	float fampl;						// amplitude (carrier output)
};

// virtual analog oscillator
// control characteristics: pitch exponential, pulse width linear 
// phase modulation input: int(-2^31..2^31-1) <-> phase(-pi..pi)
class VAOsc
{
public:
	VAOsc(			float maxpitch,		// maximum oscillator frequency
					float minpitch,		// minimum oscillator frequency
					float smprate	);	// sample rate
	~VAOsc();
	void SetShape(	int shape		);	// set wave shape (0:saw, 1:pulse)
	void SetPhase(	float phase		);	// set phase synchronously (0..1)
	void Update(	float* out,			// audio out
					int samples,		// number of samples
					float invsmp,		// 1/samples
					float pitch,		// pitch control
					float pwidth,		// pulse width (0..1, pulse shape only)
					int* pmod		);	// audio rate PM input
private:
	static int instances;				// number of object instances
	static float* tab;					// zero crossing half segment
	int phi;							// phase accumulator
	int dphi;							// phase increment
	int invf;							// frequency complement
	int pw;								// pulse width
	int nphi,pccnt1,pccnt2;				// SetPhase aux variables
	float prevout;						// previous output value
	float pconv;						// pitch conversion factor
	float pmin;							// minimum pitch
	int shp;							// wave shape
};

// raw sawtooth oscillator
// produces a naive sawtooth wave without any anti-aliasing measures
// intended as a building block for more complex oscillators and LFOs
// control characteristics: pitch exponential
// phase modulation input: int(-2^31..2^31-1) <-> phase(-pi..pi)
// LFO mode (maxpitch < 0.0038*smprate): minpitch down to 1e-7*smprate
class RawSawOsc
{
public:
	RawSawOsc(		float maxpitch,		// maximum oscillator frequency
					float minpitch,		// minimum oscillator frequency
					float smprate	);	// sample rate
	void SetPhase(	float phase		);	// set phase synchronously (0..1)
	void Update(	float* out,			// audio out
					int samples,		// number of samples
					float invsmp,		// 1/samples
					float pitch,		// pitch control
					int* pmod		);	// audio rate PM input
private:
	int phi;							// phase accumulator
	int dphi;							// phase increment
	int nphi,pccnt1,pccnt2;				// SetPhase aux variables
	int mode;							// mode (0:audio, 1:LFO)
	float pconv;						// pitch conversion factor
	float pmin;							// minimum pitch
};

// convert audio to phase modulation signal
// filter input -1..1 by 1st order lowpass with additional zero @ fs/2
// and convert it to wrap_to_int(-2^34..2^34)
class AudioToPM
{
public:
	AudioToPM(	float smprate,			// sample rate
				float fc=400.0f	);		// input cutoff frequency in Hz
	void Update(float* in,				// audio in
				int* out,				// phase modulation out
				int samples,			// number of samples
				float invsmp,			// 1/samples
				float modint	);		// modulation intensity
private:
	float pmi,s1,c1,c2;
};

// hilbert transformer: generates two orthogonal output signals
// max. phase error for f = 0.00015..0.49985fs is 0.017 degrees   
class Hilbert
{
public:
	Hilbert();
	void Update(float* in,				// audio in
				float* out1,			// audio out1
				float* out2,			// audio out2
				int samples	);			// number of samples
private:
	float s[33];
	int adidx;
	float adtab[16];
};

// first order lowpass filter
// exponential cutoff frequency control characteristic
class Lowpass1
{
public:
	Lowpass1(	float smprate,			// sample rate
				float fmin=5.0f	);		// minimum center frequency (Hz)
	void Update(float* data,			// audio data
				int samples,			// number of samples
				float invsmp,			// 1/samples
				float freq		);		// cutoff frequency control			
private:
	float s,fc,fscl1,fscl2;
	int adidx;
	float adtab[16];
};

// first order highpass filter
// exponential cutoff frequency control characteristic
class Highpass1
{
public:
	Highpass1(	float smprate,			// sample rate
				float fmin=5.0f	);		// minimum center frequency (Hz)
	void Update(float* data,			// audio data
				int samples,			// number of samples
				float invsmp,			// 1/samples
				float freq		);		// cutoff frequency	control			
private:
	float s,fc,fscl1,fscl2;
	int adidx;
	float adtab[16];
};

}	// end library specific namespace

#endif

