// AudioSynth.cpp
//
// This code is part of the ICST DSP Library version 1.2. It is released
// under the 2-clause BSD license. Copyright (c) 2008-2010, Zurich
// University of the Arts, Beat Frei. All rights reserved.

#if DONT_INCLUDE_HEADERS_IN_CPP
#else
#include "common.h"
#include "CritSect.h"	// must not be included before "common.h"
#include "AudioSynth.h"
#include "MathDefs.h"
#include "BlkDsp.h"
#include "SpecMath.h"
#endif
#include <climits>


void Effect::processInplace(float *data, int numSamples)
{
	if (inplaceProcessingEnabled && numSamples < inplaceBuffer.getNumSamples())
	{
		VectorFunctions::copy(inplaceBuffer.getWritePointer(0), data, numSamples);

		processBlock(inplaceBuffer.getReadPointer(0), data, numSamples);
	}
}

//******************************************************************************
//* helper functions and objects used by different classes of AudioSynth.cpp
//*
namespace {				// begin anonymous namespace

// fill d[size] with data intended to be added to a signal to prevent denormals
void GetAntiDenormalTable(float* d, int size)
{
	VectorFunctions::unoise(d, size);
	VectorFunctions::abs(d, size);
	VectorFunctions::add(d, 0.9f, size);
	VectorFunctions::cpxconj(d, size>>1);
	VectorFunctions::add(d, 1.1f, size);
	VectorFunctions::mul(d, 0.5f*ANTI_DENORMAL_FLOAT, size);
}

}						// end anonymous namespace

//******************************************************************************
//* wavetable oscillator
//*
// construction and initialization
WaveOsc::WaveOsc(int tablesize, int tables, float maxpitch, float minpitch, 
				 float smprate)
{	
	dspInstance = new FFTProcessor((int)IppFFT::DataType::RealFloat);

	// minimum frequency to which spectral components are aliased AND desired
	// spectral components are produced if possible with specified table size
	const float ALIASBW = 18000.0f;
	
	// init pitch to frequency conversion
	float x,y;
	if (maxpitch < minpitch) {x = maxpitch; maxpitch = minpitch; minpitch = x;}
	maxpitch = __max(0.000023f*smprate,__min(0.4999f*smprate,maxpitch));
	minpitch = __max(0.000023f*smprate,__min(0.4999f*smprate,minpitch));
	pmin = 4294967296.0f*minpitch/smprate;
	pconv = logf(maxpitch/minpitch); 
	
	// init pitch-dependent wave selection
	tablesize = VectorFunctions::nexthipow2(__max(tablesize,4));
	float flim = __min(ALIASBW, 0.45f*smprate);
	gamma = 1.0f/(smprate/flim - 1.0f);
	float flow = __max(smprate/static_cast<float>(tablesize), minpitch);
	x = logf(flow/maxpitch);
	y = __max(1.0f, ceilf(x/logf(gamma)));
	pblocks = static_cast<int>(y);
	gamma = expf(x/y);
	pcrit = __min(0.99f,__max(0, logf(flow/minpitch)/pconv));
	alpha = 1.0f/(1.0f - pcrit);
	beta = flim/flow;

	// assign and init wavetable memory
	wtables = __min(__max(tables,2),65536);
	ssize = pblocks*wtables*(tablesize+1);
	try {table = new float[ssize];} catch(...) {table = NULL;}
	if (table == NULL) {
		pblocks = 1; wtables = 2; tablesize = 4;
		ssize = pblocks*wtables*(tablesize+1);
		table = new float[ssize];
		gamma = pcrit = alpha = beta = 0;
	}
	VectorFunctions::set(table,0,ssize);

	// init control-dependent wave selection
	pbkconv = static_cast<float>(65535*pblocks);
	w = 0;
	wsize = tablesize + 1;
	tsize = wsize*wtables;
	wmask = 0x7fffffff;
	wshift = 31;
	wscl = powf(2.0f,-31.0f);
	int i=1;
	while (i < wtables) {
		i <<= 1;
		wmask >>= 1;
		wshift--;
		wscl *= 2.0f;
	}
	wconv = static_cast<double>(wmask)*static_cast<double>(wtables-1);

	// init phase accumulator and its conversion to wave position
	phi = dphi = 0;
	pshift = 31;
	pmask = 0x7fffffff;
	pscl = powf(2.0f,-31.0f); 
	i = 2;
	while (i < tablesize)
	{
		i <<= 1;
		pshift--;
		pmask >>= 1;
		pscl *= 2.0f;
	}

	nphi = pccnt1 = pccnt2 = 0;
}

// destruction
WaveOsc::~WaveOsc() {if (table) delete[] table;}

// load wave
void WaveOsc::LoadTable(float* d, int idx)
{
	int i,offset,n, size = wsize-1;
	float x,y,z; float *tmp,*tmp2; 
	if ((idx < 0) || (idx >= wtables)) {return;}
	tmp = new float[size]; tmp2 = new float[wsize];
	
	// create precompensated spectrum
	VectorFunctions::copy(tmp+2,d,size-2); tmp[0] = tmp[1] = 0;
	y = 0; z = 2.0f/static_cast<float>(size);
	for (i=0; i<size; i+=2) {
		x = 1.0f + (0.485f+ 0.875f*y)*y*y;
		tmp[i] *= x; tmp[i+1] *= x; 
		y += z;
	}
	
	// create pitch-dependent tables
	offset = idx*wsize;
	x = beta;
	n = 2*static_cast<int>(x);
	if (n > 0) {VectorFunctions::copy(tmp2,tmp,n);}
	if (n < size) {VectorFunctions::set(tmp2+n,0,size-n);}
	dspInstance->realifft(tmp2,size);
	tmp2[size] = tmp2[0];						// oscillator requires identical
	y = VectorFunctions::rms(tmp2,wsize);				// first and last elements
	if (y >= FLT_MIN) {y = 0.5f/y;} else {y = 0;}
	VectorFunctions::mul(tmp2,y,wsize);
	VectorFunctions::copy(table + offset,tmp2,wsize);		
	for (i=1; i<pblocks; i++) {
		offset += tsize;
		x *= gamma;
		n = 2*static_cast<int>(x);
		if (n > 0) {VectorFunctions::copy(tmp2,tmp,n);}
		if (n < size) {VectorFunctions::set(tmp2+n,0,size-n);}
		dspInstance->realifft(tmp2,size);
		tmp2[size] = tmp2[0];
		VectorFunctions::mul(tmp2,y,wsize);
		VectorFunctions::copy(table + offset,tmp2,wsize);
	}

	delete[] tmp; delete[] tmp2;										
}												

// set phase synchronously (0..1)
void WaveOsc::SetPhase(float phase) 
{
	phase = __max(0,__min(1.0f,phase));
	nphi = SpecMath::fdtoi(1073741824.0*static_cast<double>(phase)) << 2;
	volatile int tmp = nphi;		// this trick ensures that pccnt1 is never
	if (nphi == tmp) {pccnt1++;}	// incremented before nphi has been updated	
}									// for x86/AMD64/ECMA memory models

// audio and control update
void WaveOsc::processBlock(float* d, int samples, float invsmp, float pitch, 
					 float pitchmod, float wave, int* pmod)
{
	int i,j,idx,pwave,dwave,df;
	float x,frac,cfrac,wfrac;

	// control rate processing
	wave = __max(__min(wave,1.0f),0);
	dwave = SpecMath::ffdtoi(static_cast<double>(invsmp)*
				(wconv*static_cast<double>(wave) - static_cast<double>(w)));
	x = __max(__min(alpha*(pitch - pcrit),1.0f),0);
	pwave = tsize*(SpecMath::fdtoi(static_cast<double>(pbkconv*x)) >> 16);
	x = pmin*SpecMath::fexp(pconv*__max(__min(pitch + pitchmod,1.0f),0));
	df = SpecMath::ffdtoi(static_cast<double>(invsmp)*
					(static_cast<double>(x) - static_cast<double>(dphi)));
	while (pccnt1 != pccnt2) {phi = nphi; pccnt2++;}

	// audio rate processing
	for (i=0; i<samples; i++) {

		// wave read pointer
		j = phi + pmod[i];							// get phase modulated
		idx = static_cast<int> (					// position index ..
				static_cast<unsigned int>(j)		// 
					>> pshift	);					//
		frac = pscl*static_cast<float>(j & pmask);	// and fractional part 
		cfrac = 1.0f - frac;						//
		phi += dphi;								// update + wrap phase
		dphi += df;									// update frequency
		
		// wave selection
		idx += pwave;								// pitch dependent offset
		idx += (wsize*(w >> wshift));				// wave selector offset
		wfrac = wscl*static_cast<float>(w & wmask);	//
		w += dwave;									// update wave selector	

		// calculate output as weighted sum of linear interpolated waves
		x = cfrac*table[idx] + frac*table[idx+1];
		d[i] = x + wfrac*(cfrac*table[idx+wsize] +
							frac*table[idx+wsize+1] - x);
	}
}

//******************************************************************************
//* ring modulator
//*
//* includes prefilters (zero @ fs/2), correction postfilter, dc trap (fc=5hz)
// construction
RingMod::RingMod()
{
	in1d = in2d = od = dc = 0; adidx = 0;
	a = 0.0f;

	GetAntiDenormalTable(adtab,16);
}


void RingMod::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
	a = 31.4f / jmax<float>(62.8f, (float)sampleRate);
}


void RingMod::processBlockWithTwoOutputs(const float* in1, const float* in2, float* out, int samples)
{	
	float adn[2];
	adn[0] = adtab[adidx]; 
	adn[1] = adtab[adidx+1]; 
	adidx = (adidx+2) & 0xe;

	for (int i=0; i<samples; i++)
	{
		od = 0.41666667f*(in1[i]+in1d)*(in2[i]+in2d) - 0.66666667f*od + adn[i&1];
		in1d = in1[i]; in2d = in2[i]; 
		out[i] = od - dc;
		dc += (a*out[i]);	
	}
}

//******************************************************************************
//* noise generator
//*
//* uniformly distributed white noise, range: -1..1, RMS amplitude: 0.577 
//* pink noise, accuracy: +/- 0.3db (0.00045..0.45fs), RMS amplitude: 0.577
// construction
Noise::Noise() {s1 = s2 = s3 = 0; ntype = 0;}

// set noise type: 0 = white, 1 = pink
void Noise::setNoiseType(NoiseType type) {ntype = __max(0,__min(1,(int)type));}

// audio update
void Noise::processInplace(float* out, int samples)
{
	int i; float x;
	VectorFunctions::unoise(out,samples);
	if (ntype == 1) {
		for (i=0; i<samples; i++) {		
			x = out[i] + 2.479309f*s1 - 1.9850127f*s2 + 0.5056004f*s3;
			out[i] = 0.577350f*x - 1.093526f*s1 + 0.553428f*s2 - 0.035872f*s3;
			s3 = s2; s2 = s1; s1 = x;
		}
	}
}


void Noise::processBlock(const float* /*in*/, float *out, int numSamples)
{
	processInplace(out, numSamples);
}

//******************************************************************************
//* static delay line with clickless delay time change
//* 
// construction
Delay::Delay(int maxlen, float zlevel)
{
	// assign delay line memory and init
	mlen = __max(1,maxlen);
	try {d = new float[mlen];} catch(...) {d = NULL;}
	if (d) {VectorFunctions::set(d,0,mlen);} else {mlen = 0;}
	cp = 0; len = tlen = nlen = state = cntdwn = 0; a = 1.0f;
	
	zlev = zlevel;
}

// destruction
Delay::~Delay() {if (d) delete[] d;}

// set delay time in samples
void Delay::setDelayTime(int timeInSamples) {tlen = __max(0,__min(mlen,timeInSamples));}				


void Delay::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	xflen = __max(1, static_cast<int>(0.02f*sampleRate));
	invxflen = 1.0f / static_cast<float>(xflen);

	enableInplaceProcessing(true, samplesPerBlock);
}

// audio and control update
void Delay::processBlock(const float* in, /* audio in */ float* out, /* audio out */ int samples)
{
	// audio update
	VectorFunctions::delay(out,in,samples,d,cp,len);

	// delay time change
	int i; bool proceed=false;
	if ((len != tlen) || (state != 0)) {
		do {
			switch(state) {
			case 0:	// init change sequence
				nlen = tlen;
				cntdwn = xflen;
				a = 1.0f;
				state = 1;
				proceed = true;
				break;
			case 1:	// crossfade from delayed to original signal
				if (cntdwn >= samples) {
					for (i=0; i<samples; i++) {
						a -= invxflen;
						out[i] = a*out[i] + (1.0f - a)*zlev*in[i];
					}
					cntdwn -= samples;
					proceed = false;
				}
				else {
					for (i=0; i<cntdwn; i++) {
						a -= invxflen;
						out[i] = a*out[i] + (1.0f - a)*zlev*in[i];
					}
					samples -= cntdwn;
					out += cntdwn;
					in += cntdwn;
					cntdwn = nlen + samples;
					len = nlen;
					state = 2;
					proceed = true;
				}
				break;
			case 2:	// switch length and wait until the new delay line is filled
				if (cntdwn >= samples) {
					VectorFunctions::copy(out,in,samples);
					VectorFunctions::mul(out,zlev,samples);
					cntdwn -= samples;
					proceed = false;
				}
				else {
					VectorFunctions::copy(out,in,cntdwn);
					VectorFunctions::mul(out,zlev,cntdwn);
					samples -= cntdwn;
					out += cntdwn;
					in += cntdwn;
					cntdwn = xflen;
					state = 3;
					proceed = true;
				}
				break;
			case 3:	// crossfade from original to delayed signal and finish
				if (cntdwn >= samples) {
					for (i=0; i<samples; i++) {
						a += invxflen;
						out[i] = a*out[i] + (1.0f - a)*zlev*in[i];
					}
					cntdwn -= samples;
					proceed = false;
				}
				else {
					for (i=0; i<cntdwn; i++) {
						a += invxflen;
						out[i] = a*out[i] + (1.0f - a)*zlev*in[i];
					}
					state = 0;
					proceed = false;
				}
				break;
			}
		} while (proceed);
	}
}			



//******************************************************************************
//* time varying delay line with 1st order allpass interpolation
//*
// construction
VarDelay::VarDelay(float maxlen)
{
	// assign delay line memory and init
	maxlen = __max(1.0f,maxlen);
	int len = 1;
	dshift = 31;
	dmask = 0x7fffffff;
	dscl = 2147483648.0f;
	llim = maxlen;
	while (len <= static_cast<int>(ceil(static_cast<double>(llim)))) {
		len <<= 1;
		dshift--;
		dmask >>= 1;
		dscl *= 0.5f;	
	}
	dscl2 = 1.0f/dscl;
	wrapmask = len - 1;
	outd = 0; widx = delay = adidx = 0;
	try {d = new float[len];} catch(...) {d = NULL;}
	if (d) {VectorFunctions::set(d,0,len);}
	else {d = new float[2]; len = 2; wrapmask = 1; llim = 0;}
	GetAntiDenormalTable(adtab,16);
}

// destruction
VarDelay::~VarDelay() {if (d) delete[] d;}


void VarDelay::setDelayTime(float delayTimeSamples)
{
	length = delayTimeSamples;
}

void VarDelay::prepareToPlay(double /*sampleRate*/, int samplesPerBlock)
{
	enableInplaceProcessing(true, samplesPerBlock);
}

// audio and control update
void VarDelay::processBlock(const float* in, /* audio in */ float* out, /* audio out */ int samples)
{
	float invsmp = 1.0f / (float)samples;

	float adn[2];
	adn[0] = adtab[adidx]; adn[1] = adtab[adidx+1]; adidx = (adidx+2) & 0xe;
	int i,ridx,dinc; float g;
	length = __max(0, __min(llim,length) - 0.3f);
	dinc = SpecMath::ffdtoi(static_cast<double>(invsmp)*
			(static_cast<double>(dscl*length) - static_cast<double>(delay)));
	for (i=0; i<samples; i++) {
		delay += dinc;
		d[widx] = in[i];
		ridx = (widx - (delay >> dshift)) & wrapmask;
		widx = (widx + 1) & wrapmask;
		g = dscl2*static_cast<float>(delay & dmask);
		g = 0.539f + (0.369f*g - 1.037f)*g;
		out[i] = outd = g*(d[ridx] - outd) + d[(ridx-1) & wrapmask] + adn[i&1];
	}
}			

//******************************************************************************
//* sample playback oscillator
//*
int SampleOsc::instances = 0;
float* SampleOsc::c = NULL;

// construction
SampleOsc::SampleOsc()
{
	ScopedLock sl(lock);

	// create interpolation table
	int i; float* tmp;
	if (c == NULL) {
		c = new float[20480];				// no check for low space demand
		tmp = new float[20480];
		VectorFunctions::set(c,1.0f,20480);
		VectorFunctions::kaiser(tmp,20480,0.7);
		VectorFunctions::mac(c,tmp,-0.9f,20480);	
		VectorFunctions::kaiser(tmp,20480,9.0);
		VectorFunctions::mul(tmp,c,20480);
		VectorFunctions::sinc(c,20480,10.0*18.3/48.0);
		VectorFunctions::mul(tmp,c,20480);
		VectorFunctions::mul(tmp,7.602131f,20480);
		for (i=0; i<10; i++) {VectorFunctions::reverse(tmp+2048*i,2048);}
		for (i=0; i<10; i++) {VectorFunctions::interleave(c,tmp+2048*i,2048,10,i);}
		delete[] tmp;	
	}

	// init states
	idx = 0; didx = static_cast<icstdsp_int64>(0x40000000) << 2;

	instances++;
	
}

// destruction
SampleOsc::~SampleOsc()
{
	ScopedLock sl(lock);					// single thread access on
	if (instances <= 1) {if (c) {delete[] c; c = NULL;}}
	instances--;
 					// single thread access off
}

// precompensate sample, required for every sample when loaded
void SampleOsc::PreComp(float* d, int length, int looplen, float srate)
{
	// two coefficient sets designed for a group delay of 8 samples and an
	// overall frequency response (precompensation + interpolation) intended
	// for musical applications
	// +/-0.02 dB @ 0..0.33fs, +/-0.05 dB @ 0..0.417fs  
	float bhf[17] = {	0.0028f,-0.0119f,0.0322f,-0.0709f,0.1375f,-0.2544f,
						0.4384f,-0.6334f,1.7224f,-0.6334f,0.4384f,-0.2544f,
						0.1375f,-0.0709f,0.0322f,-0.0119f,0.0028f			};
	// +/-0.06 dB @ 0..0.36fs, +/-0.18 dB @ 0..0.454fs 
	float blf[17] = {	0.0074f,-0.0251f,0.0561f,-0.1067f,0.1849f,-0.3117f,
						0.5023f,-0.7006f,1.7897f,-0.7006f,0.5023f,-0.3117f,
						0.1849f,-0.1067f,0.0561f,-0.0251f,0.0074f			};
	// filtering
	int i,j; float pad[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	looplen = __min(length,looplen);
	if (looplen > 0) {
		j = length - looplen;
		for (i=0; i<16; i++) {
			d[length+i] = d[j];
			j++;
			if (j >= length) {j -= looplen;}
		}
	}
	else {VectorFunctions::set(d+length,0,16);}
	if (srate > 47500.0f) {VectorFunctions::fir(d,length+16,bhf,16,pad);}
	else {VectorFunctions::fir(d,length+16,blf,16,pad);}
}

// audio and control update
int SampleOsc::Update(float* out, float* d, int samples, float invsmp,
			float transpose, int endpos, int startpos, float looplen)
{
	const int START_OFFSET = 4;				// depends on PreComp filter
	int i; unsigned int j,k; float acc;
	if (startpos >= 0) {
		idx = static_cast<icstdsp_int64>(__min(startpos,endpos) + START_OFFSET) << 32;
		didx = static_cast<icstdsp_int64>(4294967296.0f*transpose);
	}
	icstdsp_int64 sampleend = static_cast<icstdsp_int64>(endpos) << 32;
	icstdsp_int64 llen = __min(static_cast<icstdsp_int64>(4294967296.0f*looplen),sampleend);
	sampleend += START_OFFSET*(static_cast<icstdsp_int64>(0x40000000) << 2);
	icstdsp_int64 txfinc = static_cast<icstdsp_int64>
				(invsmp*(4294967296.0f*transpose - static_cast<float>(didx)));
	
	for (i=0; i<samples; i++) {					
		if (static_cast<icstdsp_uint64>(idx) >= 
			static_cast<icstdsp_uint64>(sampleend))
		{
			if (idx >= sampleend) {	// end of sample reached
				if (llen > 0) {
					idx -= llen;
					if (idx >= sampleend) {
						idx = (idx - sampleend)%llen + sampleend - llen;
					}
				}
				else {
					VectorFunctions::set(out+i,0,samples-i);
					didx += static_cast<icstdsp_int64>(samples-i)*txfinc;
					if (didx < 0) {idx = sampleend-1;}
					return 1;
				}
			}
			else {	// sample start reached in reverse direction
				VectorFunctions::set(out+i,0,samples-i);
				didx += static_cast<icstdsp_int64>(samples-i)*txfinc;
				if (didx > 0) {idx = 0;}
				return -1;	
			}
		}
		k = 10*(idxint[0] >> 21);
		j = idxint[1];
		acc = c[k]*d[j];		acc += (c[k+1]*d[j+1]);
		acc += (c[k+2]*d[j+2]); acc += (c[k+3]*d[j+3]);
		acc += (c[k+4]*d[j+4]); acc += (c[k+5]*d[j+5]);
		acc += (c[k+6]*d[j+6]); acc += (c[k+7]*d[j+7]);
		acc += (c[k+8]*d[j+8]); out[i] = acc + (c[k+9]*d[j+9]);
		idx += didx;
		didx += txfinc;
	}
	return 0;
}

//******************************************************************************
//* envelope generator
//*
// construction 
Envelope::Envelope(float tmax, float tmin, int maxsegs)
{
	maxsegs = __max(3,maxsegs);		// enough to support preset envelopes
	
	// prepare idle segment
	idleseg = maxsegs;
	maxsegs++;		
	
	// allocate memory
	type = new int[maxsegs]; 
	nextsgm = new int[maxsegs];
	endlevel = new float[maxsegs];
	time = new float[maxsegs];
	timemodint = new float[maxsegs];

	// init envelope
	sgm = relseg = idleseg;
	cnt = INT_MAX;
	cout = 0;
	for (int i=0; i<maxsegs; i++) {
		type[i] = 5;
		nextsgm[i] = idleseg;
		endlevel[i] = time[i] = timemodint[i] = 0;
	}
	levelmodint = 0;
	evcnt1 = evcnt2 = event = 0;
	
	// set time scale factors according to control range
	tmin = __max(1.0f,__min(500000.0f,tmin));
	tmax = __max(1.0f,__min(500000.0f,tmax));
	tscl1 = sqrtf(tmin);
	tscl2 = 0.5f*logf(tmax/tmin);
}

// destruction
Envelope::~Envelope()
{
	if (type)		{delete[] type;}
	if (nextsgm)	{delete[] nextsgm;}
	if (endlevel)	{delete[] endlevel;}
	if (time)		{delete[] time;}
	if (timemodint)	{delete[] timemodint;}
}

// control update
void Envelope::Update(float* out, int values, float tmod, float lmod)
{
	int i,pts;

	// check for events
	while (evcnt1 != evcnt2) {
		switch(event) {
		case 0:	// key down
				SetSegment(0,cout,tmod,lmod); break;
		case 1:	// key up
				SetSegment(relseg,cout,tmod,lmod); break;
		case 2:	// set to idle state
				SetSegment(idleseg,cout,tmod,lmod); break;
		}
		evcnt2++;
	}
	
	// compute envelope
st:	pts	= __min(cnt,values);
	switch(type[sgm]) {
	case 0:	// concave
		for (i=0; i<pts; i++) {x += dx; out[i] = a + b*x*x;}
		break;
	case 1:	// convex
	case 2:	// classic attack
		for (i=0; i<pts; i++) {x += dx; out[i] = a + (b + c*x)*x;}
		break;
	case 3:	// linear
		for (i=0; i<pts; i++) {x += dx; out[i] = x;}
		break;
	case 4:	// sigmoid
		for (i=0; i<pts; i++) {x += dx; out[i] = a + ((b + c*x)*x)*x;}
		break;
	case 5:	// infinite exponential
		for (i=0; i<pts; i++) {out[i] = x = a + b*x;}
		break;
	}
	cout = out[pts-1];
	cnt -= pts;
	if (cnt == 0) {
		SetSegment(nextsgm[sgm],cout,tmod,lmod);
		values -= pts;
		if (values > 0) {
			out += pts;
			goto st;
		}
	}
}

// jump to new segment
void Envelope::SetSegment(int newseg, float startlevel, float tmod, float lmod)
{
	float tmp;
	sgm = newseg;
	tmp = __max(0,__min(1.0f,time[sgm] + tmod*timemodint[sgm]));
	tmp = tscl1*SpecMath::qdexp(tscl2*tmp);
	tmp *= tmp;
	cnt = SpecMath::fdtoi(static_cast<double>(tmp));
	tmp = __max(0,__min(1.0f,lmod));
	float endlev = endlevel[sgm]*(1.0f - levelmodint*(1.0f - tmp));
	switch(type[sgm]) {
	case 0:	// concave
			a = startlevel; b = endlev - startlevel;
			x = 0; dx = 1.0f/static_cast<float>(cnt); 
			break;
	case 1:	// convex
			a = startlevel; c = startlevel - endlev;
			b = -2.0f*c;
			x = 0; dx = 1.0f/static_cast<float>(cnt); 
			break;
	case 2:	// classic attack
			a = startlevel; c = 0.66666667f*(startlevel - endlev);
			b = -2.5f*c;
			x = 0; dx = 1.0f/static_cast<float>(cnt); 
			break;
	case 3:	// linear
			x = startlevel;
			dx = (endlev - startlevel)/static_cast<float>(cnt);
			break;
	case 4:	// sigmoid
			a = startlevel; c = 2.0f*(startlevel - endlev);
			b = -1.5f*c;
			x = 0; dx = 1.0f/static_cast<float>(cnt); 
			break;
	case 5:	// infinite exponential
			a = 1.0f/static_cast<float>(cnt);
			b = 1.0f - a;
			if (fabs(endlev) > 1e-5f) {
				a *= endlev;
				cnt = INT_MAX;
				x = startlevel;
			}
			else {
				a = 0;
				if (fabs(x) > 1e-5f) {
					cnt *= 12;
					x = startlevel;	
				}
				else {
					cnt = INT_MAX;
					x = 0;
				}
			}
			break;
	}	
}

// create event
void Envelope::Event(int id)
{
	event = id;						
	volatile int tmp = event;		// this trick ensures that evcnt1 is never
	if (event == tmp) {evcnt1++;}	// incremented before event has been updated
}									// for x86/AMD64/ECMA memory models

// set envelope parameters
void Envelope::SetParam(int pid, float value, int segment)
{
	switch(pid) {
	case ENVELOPE_TIME:
		if ((segment < 0) || (segment >= idleseg)) {break;}
		time[segment] = __max(0,__min(1.0f,value));
		break;
	case ENVELOPE_TMOD:
		if ((segment < 0) || (segment >= idleseg)) {break;}
		timemodint[segment] = __max(-1.0f,__min(1.0f,value));
		break;
	case ENVELOPE_LEVEL:
		if ((segment < 0) || (segment >= idleseg)) {break;}
		endlevel[segment] = __max(-1.0f,__min(1.0f,value));
		break;
	case ENVELOPE_LMOD:
		levelmodint = __max(0,__min(1.0f,value));
		break;
	case ENVELOPE_STYPE:
		if ((segment < 0) || (segment >= idleseg)) {break;}
		type[segment] =
			__max(0,__min(5,SpecMath::fdtoi(static_cast<double>(value))));
		break;
	case ENVELOPE_NEXTS:
		if ((segment < 0) || (segment >= idleseg)) {break;}
		nextsgm[segment] =
			__max(0,__min(idleseg,SpecMath::fdtoi(static_cast<double>(value))));
		Event(2);
		break;
	case ENVELOPE_RELSEG:
		relseg = 
			__max(0,__min(idleseg,SpecMath::fdtoi(static_cast<double>(value))));
		Event(2);
		break;
	}
}

// set preset envelope
void Envelope::Preset(int pno)
{
	switch(pno) {
	case ENVELOPE_P_ADSR:
		// attack segment
		SetParam(ENVELOPE_NEXTS,1.0f,0);		
		SetParam(ENVELOPE_STYPE,ENVELOPE_ST_CLASSICATTACK,0);
		SetParam(ENVELOPE_LEVEL,1.0f,0);
		// decay + sustain segment
		SetParam(ENVELOPE_NEXTS,2.0f,1);
		SetParam(ENVELOPE_STYPE,ENVELOPE_ST_INFINITEEXP,1);
		// release segment
		SetParam(ENVELOPE_RELSEG,2.0f);
		SetParam(ENVELOPE_NEXTS,2.0f,2);
		SetParam(ENVELOPE_STYPE,ENVELOPE_ST_INFINITEEXP,2);
		SetParam(ENVELOPE_LEVEL,0,2);	
		break;
	case ENVELOPE_P_ABDSR:
		// attack segment
		SetParam(ENVELOPE_NEXTS,1.0f,0);		
		SetParam(ENVELOPE_STYPE,ENVELOPE_ST_CLASSICATTACK,0);
		// breakpoint segment
		SetParam(ENVELOPE_NEXTS,2.0f,1);		
		SetParam(ENVELOPE_STYPE,ENVELOPE_ST_LINEAR,1);
		// decay + sustain segment
		SetParam(ENVELOPE_NEXTS,3.0f,2);
		SetParam(ENVELOPE_STYPE,ENVELOPE_ST_INFINITEEXP,2);
		// release segment
		SetParam(ENVELOPE_RELSEG,3.0f);
		SetParam(ENVELOPE_NEXTS,3.0f,3);
		SetParam(ENVELOPE_STYPE,ENVELOPE_ST_INFINITEEXP,3);
		break;
	}
}

//******************************************************************************
//* controlled amplifier
//*
// construction
Amp::Amp()
{
	a = 0; ccurve = ncurve = adidx = 0;
	GetAntiDenormalTable(adtab,16);
}

// set control characteristic: 0=linear, 1=square, 2=quartic
void Amp::setCurveType(CurveType curve) {ncurve = __max(0,__min(2,(int)curve));}

// audio and control update
void Amp::processInplace(float* data, int samples)	
{
	float invsmp = 1.0f / (samples);

	float adn[2];
	adn[0] = adtab[adidx]; adn[1] = adtab[adidx+1]; adidx = (adidx+2) & 0xe;
	int i; unsigned int t; float da;

	// change characteristic
	if (ccurve != ncurve) {
		t = static_cast<unsigned int>(ncurve);	// nail current value to t
		switch(ccurve) {
		case 0:	
			if (t == 1) {a = sqrtf(fabsf(a));}
			if (t == 2) {a = sqrtf(sqrtf(fabsf(a)));}
			break;
		case 1:		
			if (t == 0) {a *= a;}
			if (t == 2) {a = sqrtf(fabsf(a));}
			break;
		case 2:
			if (t == 0) {a = a*a*a*a;}
			if (t == 1) {a *= a;}
			break;
		}
		ccurve = static_cast<int>(t);
	}	
	
	// audio update
	switch(ccurve) {
	case 0:		// linear
		da = invsmp*(__max(1e-8f,__min(1.0f,amp)) - a);
		for (i=0; i<samples; i++) {
			a += da;
			data[i] = a*data[i] + adn[i&1];
		}
		break;
	case 1:		// square
		da = invsmp*(__max(0.0001f,__min(1.0f,amp)) - a);
		for (i=0; i<samples; i++) {
			a += da;
			data[i] = a*a*data[i] + adn[i&1];
		}
		break;
	case 2:		// quartic
		da = invsmp*(__max(0.01f,__min(1.0f,amp)) - a);
		for (i=0; i<samples; i++) {
			a += da;
			data[i] = a*a*a*a*data[i] + adn[i&1];
		}
		break;
	}
}


void Amp::processBlock(const float *input, float *output, int numSamples)
{
	VectorFunctions::copy(output, input, numSamples);

	processInplace(output, numSamples);
}

//******************************************************************************
//* second order multimode filter
//*
// static member initialization
int ChambFilter::instances = 0;
float* ChambFilter::tab = NULL;
namespace {static CriticalSection csChambFilter;}

// construction 
ChambFilter::ChambFilter()
{
	ScopedLock sl(csChambFilter);					// single thread access on
	
	// create anti-denormal pink noise table if required
	if (tab == NULL) {
		tab = new float[4096];
		Noise n;
		n.setNoiseType(Noise::NoiseType::PinkNoise);
		n.processInplace(tab,4096);
		VectorFunctions::mul(tab,3.0f*ANTI_DENORMAL_FLOAT,4096);
	}

	// init frequency conversion
	

	// init states
	a = b = lim = 0;
	fc = d = 1.0f;
	ftype = 0;
	adidx = (2531*instances) & 0xfff;

	instances++;
 					// single thread access off
}

//destruction
ChambFilter::~ChambFilter()
{
	ScopedLock sl(csChambFilter);					// single thread access on
	if (instances <= 1) {if (tab) {delete[] tab; tab = NULL;}}
	instances--;
 					// single thread access off
}


void ChambFilter::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
	const float fmin = 5.0f;

	fscl1 = sqrtf(__min(1.0f, __max(0.0001f, M_PI_FLOAT*fmin / (1.22f*(float)sampleRate))));
	fscl2 = -logf(fscl1);
}

// set filter type
void ChambFilter::setFilterType(FilterType type) {ftype = (int)type;}


void ChambFilter::processBlock(const float *input, float *output, int numSamples)
{
	VectorFunctions::copy(output, input, numSamples);

	processInplace(output, numSamples);
}


void ChambFilter::processInplace(float *data, int numSamples)
{
	if (mode == WorkingMode::Normal)
	{
		Update(data, numSamples, 1.0f / (float)numSamples, freq, res);
	}
	else
	{
		UpdateCmp(data, numSamples, 1.0f / (float)numSamples, freq, res);
	}
}

// audio and control update, purely linear
void ChambFilter::Update(	float* data, int samples, float invsmp,
							float freq, float res					)
{
	int i; float dfc,dd,c,x,in,fcorr,r,f;

	// control processing
	r = 1.1313708f*(1.25f - __max(0,__min(1.0f,res)));
	r *= r;
	x = fscl1*SpecMath::qdexp(fscl2*__max(0,__min(1.0f,freq)));
	fcorr = x*(1.1045361f - 0.1045361f*__min(1.0f,r));
	r = __min(r, 2.0f-x*x);
	dfc = invsmp*(fcorr - fc);
	dd = invsmp*(r - d);

	// audio processing
	switch(ftype) {
	case 0:	// lowpass
		f = fc*fc;
		b += (f*a);
		for (i=0; i<samples; i++) {
			in = data[i] + tab[adidx];
			adidx = (adidx + 1) & 0xfff;
			fc += dfc;
			c = in - b - d*a;
			a += (f*c);
			b += (f*a);
			c = in - b - d*a;
			a += (f*c);
			b += (f*a);
			data[i] = b;
			d += dd;
			f = fc*fc;
		}
		break;
	case 1:	// bandpass
		for (i=0; i<samples; i++) {
			in = data[i] + tab[adidx];
			adidx = (adidx + 1) & 0xfff;
			f = fc*fc;
			fc += dfc;
			b += (f*a);
			c = in - b - d*a;
			a += (f*c);
			x = a;
			b += (f*a);
			c = in - b - d*a;
			a += (f*c);
			data[i] = a + x;
			d += dd;
		}
		break;
	case 2:	// highpass
		for (i=0; i<samples; i++) {
			in = data[i] + tab[adidx];
			adidx = (adidx + 1) & 0xfff;
			f = fc*fc;
			fc += dfc;
			b += (f*a);
			c = in - b - d*a;
			a += (f*c);
			b += (f*a);
			x = in - b - d*a;
			a += (f*x);
			data[i] = 0.5f*(c + x);
			d += dd;
		}
		break;
	case 3:	// peak
		for (i=0; i<samples; i++) {
			in = data[i] + tab[adidx];
			adidx = (adidx + 1) & 0xfff;
			f = fc*fc;
			fc += dfc;
			b += (f*a);
			c = in - b - d*a;
			a += (f*c);
			b += (f*a);
			x = in - b - d*a;
			a += (f*x);
			data[i] = b - c;
			d += dd;
		}
		break;
	case 4:	// notch
		for (i=0; i<samples; i++) {
			in = data[i] + tab[adidx];
			adidx = (adidx + 1) & 0xfff;
			f = fc*fc;
			fc += dfc;
			b += (f*a);
			c = in - b - d*a;
			a += (f*c);
			b += (f*a);
			c = in - b - d*a;
			a += (f*c);
			data[i] = b + c;
			d += dd;
		}
		break;
	}
}

// audio and control update
// quasi-linear compression, resonance up to self-oscillation
// !!! input/output/power detector gain subject to fine tuning !!!
void ChambFilter::UpdateCmp(float* data, int samples, float invsmp,
							float freq, float res)
{
	int i; float dfc,dd,c,x,inscl,fcorr,r,deff,f;

	// control processing
	r = 1.4f*(1.0f - __max(0,__min(1.0f,res)));
	r = r*r - 0.016f; 
	x = fscl1*SpecMath::qdexp(fscl2*__max(0,__min(1.0f,freq)));
	fcorr = x*(1.1045361f - 0.1045361f*__min(1.0f,r));
	r = __min(r, 2.0f-x*x);
	dfc = invsmp*(fcorr - fc);
	dd = invsmp*(r - d);

	// audio processing
	switch(ftype) {
	case 0:	// lowpass
		f = fc*fc;
		b += (f*a);
		for (i=0; i<samples; i++) {
			inscl = 0.5f*(data[i] + tab[adidx]);
			adidx = (adidx + 1) & 0xfff;
			lim = __min(1.0f, 0.06f*a*a + 0.98f*lim);
			deff = d*(1.0f - lim) + lim;
			fc += dfc;
			c = inscl - b - deff*a;
			a += (f*c);
			b += (f*a);
			c = inscl - b - deff*a;
			a += (f*c);
			b += (f*a);
			data[i] = b;
			d += dd;
			f = fc*fc;
		}
		break;
	case 1:	// bandpass
		for (i=0; i<samples; i++) {
			inscl = 0.5f*(data[i] + tab[adidx]);
			adidx = (adidx + 1) & 0xfff;
			lim = __min(1.0f, 0.06f*a*a + 0.98f*lim);
			deff = d*(1.0f - lim) + lim;
			f = fc*fc;
			fc += dfc;
			b += (f*a);
			c = inscl - b - deff*a;
			a += (f*c);
			x = a;
			b += (f*a);
			c = inscl - b - deff*a;
			a += (f*c);
			data[i] = 0.7f*(a + x);
			d += dd;
		}
		break;
	case 2:	// highpass
		for (i=0; i<samples; i++) {
			inscl = 0.5f*(data[i] + tab[adidx]);
			adidx = (adidx + 1) & 0xfff;
			lim = __min(1.0f, 0.06f*a*a + 0.98f*lim);
			deff = d*(1.0f - lim) + lim;
			f = fc*fc;
			fc += dfc;
			b += (f*a);
			c = inscl - b - deff*a;
			a += (f*c);
			b += (f*a);
			x = inscl - b - deff*a;
			a += (f*x);
			data[i] = 0.5f*(c + x);
			d += dd;
		}
		break;
	case 3:	// peak
		for (i=0; i<samples; i++) {
			inscl = 0.5f*(data[i] + tab[adidx]);
			adidx = (adidx + 1) & 0xfff;
			lim = __min(1.0f, 0.06f*a*a + 0.98f*lim);
			deff = d*(1.0f - lim) + lim;
			f = fc*fc;
			fc += dfc;
			b += (f*a);
			c = inscl - b - deff*a;
			a += (f*c);
			b += (f*a);
			x = inscl - b - deff*a;
			a += (f*x);
			data[i] = 0.7f*(b - c);
			d += dd;
		}
		break;
	case 4:	// notch
		for (i=0; i<samples; i++) {
			inscl = 0.5f*(data[i] + tab[adidx]);
			adidx = (adidx + 1) & 0xfff;
			lim = __min(1.0f, 0.06f*a*a + 0.98f*lim);
			deff = d*(1.0f - lim) + lim;
			f = fc*fc;
			fc += dfc;
			b += (f*a);
			c = inscl - b - deff*a;
			a += (f*c);
			b += (f*a);
			c = inscl - b - deff*a;
			a += (f*c);
			data[i] = b + c;
			d += dd;
		}
		break;
	}
}

//******************************************************************************
//* virtual analog moog lowpass filter
//*
// static member initialization
int MoogFilter::instances = 0;
float* MoogFilter::tab = NULL;
namespace {static CriticalSection csMoogFilter;}

// construction
MoogFilter::MoogFilter()
{
	ScopedLock sl(csMoogFilter);					// single thread access on
	
	// create anti-denormal pink noise table if required
	if (tab == NULL) {
		tab = new float[4096];
		Noise n;
		n.setNoiseType(Noise::NoiseType::PinkNoise);
		n.processInplace(tab,4096);
		VectorFunctions::mul(tab,3.0f*ANTI_DENORMAL_FLOAT,4096);
	}
	
	
	
	// init states
	rc = s1 = s2 = s3 = s4 = s5 = s6 = s7 = s8 = slim = previn = 0;
	fc = 1.0f;
	adidx = (2531*instances) & 0xfff;

	instances++;
 					// single thread access off
}

//destruction
MoogFilter::~MoogFilter()
{
	ScopedLock sl(csMoogFilter);					// single thread access on
	if (instances <= 1) {if (tab) {delete[] tab; tab = NULL;}}
	instances--;
 					// single thread access off
}


void MoogFilter::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
	const float fmin = 20.0f;

	// init frequency conversion
	fscl1 = sqrtf(__min(1.0f, __max(0.0001f, 2.5f*fmin / (float)sampleRate)));
	fscl2 = -logf(fscl1);
}

// audio and control update
void MoogFilter::processInplace(float* data, int samples)
{
	const float invsmp = 1.0f / (float)samples;

	float r,f,fcpl,inscl,x,y;
	
	// control processing	
	drc = invsmp*(1.05f*__min(1.0f,__max(1e-5f,res)) - rc);
	dfc = invsmp*(fscl1*SpecMath::qdexp(fscl2*__max(0,__min(1.0f,freq))) - fc);
	
	// audio processing
	for (int i=0; i<samples; i++) {

		// calculate and update filter coefficients
		y = fc*fc;
		x = y*(1.0f - rc);
		f = y + 0.5f*x*x;
		f = (1.25f + (-0.74375f + 0.3f*f)*f)*f;
		fcpl = 1.0f - f;
		r = rc*(1.4f + (0.108f + (-0.164f - 0.069f*f)*f)*f);
		inscl = 0.18f + 0.25f*r;
		fc += dfc;
		rc += drc;

		// filtering, 1st pass
		x = inscl*previn - r*s5;
		slim = __min(1.0f,__max(-1.0f,0.062f*x*x + 0.993f*slim));
		x *= (1.0f - slim + 0.5f*slim*slim);

		y = 0.3f*s1;
		s1 = f*x + fcpl*s1;
		y += s1;

		x = 0.3f*s2;
		s2 = f*y + fcpl*s2;
		x += s2;

		x = __min(1.0f,__max(-1.0f,x));
		x *= (1.0f - 0.33333333f*x*x);

		y = 0.3f*s3;
		s3 = f*x + fcpl*s3;
		y += s3;

		x = 0.3f*s4;
		s4 = f*y + fcpl*s4;
		x += s4;

		s5 = x;

		// filtering, 2nd pass
		previn = data[i] + tab[adidx];
		adidx = (adidx + 1) & 0xfff;
		x = inscl*previn - r*s5;
		slim = __min(1.0f,__max(-1.0f,0.062f*x*x + 0.993f*slim));
		x *= (1.0f - slim + 0.5f*slim*slim);

		y = 0.3f*s1;
		s1 = f*x + fcpl*s1;
		y += s1;

		x = 0.3f*s2;
		s2 = f*y + fcpl*s2;
		x += s2;

		x = __min(1.0f,__max(-1.0f,x));
		x *= (1.0f - 0.33333333f*x*x);

		y = 0.3f*s3;
		s3 = f*x + fcpl*s3;
		y += s3;

		x = 0.3f*s4;
		s4 = f*y + fcpl*s4;
		x += s4;

		// decimation
		data[i] = s6 = 0.19f*(x + s8) + 0.57f*(s5 + s7) - 0.52f*s6;
		s8 = s5;
		s7 = s5 = x;								
	}
}


void MoogFilter::processBlock(const float *input, float *output, int numSamples)
{
	VectorFunctions::copy(output, input, numSamples);

	processInplace(output, numSamples);
}

//******************************************************************************
//* FM oscillator
//*
//* control characteristics: frequency exponential, amplitude + feedback linear 
//* phase modulation input: int(-2^31..2^31-1) <-> phase(-pi..pi)
//* output: carrier -> float(-1..1), modulator -> wrap_to_int(-2^35..2^35-1)
//* feedback range: 0..1 -> none..maximum stable without noise
// static member initialization
int FMOsc::instances = 0;
short int* FMOsc::tab = NULL;
namespace {static CriticalSection csFMOsc;}

// construction
FMOsc::FMOsc(float maxpitch, float minpitch, float smprate)
{
	ScopedLock sl(csFMOsc);						// single thread access on
	
	// create half period cosine table if required
	int i;
	if (tab == NULL) {
		tab = new short int[16385];		// no check for low space demand
		for (i=0; i<=16384; i++) {
			tab[i] = static_cast<short int> (0.5f +
					16383.0f*cosf(M_PI_FLOAT/16384.0f*static_cast<float>(i)));
		}
	}

	// init pitch conversion
	float x;
	if (maxpitch < minpitch) {x = maxpitch; maxpitch = minpitch; minpitch = x;}
	maxpitch = __max(0.000023f*smprate,__min(0.4999f*smprate,maxpitch));
	minpitch = __max(0.000023f*smprate,__min(0.4999f*smprate,minpitch));
	pmin = 4294967296.0f*minpitch/smprate;
	pconv = logf(maxpitch/minpitch);
	
	// init states
	phi = dphi = ampl = fb = tval = nphi = pccnt1 = pccnt2 = 0; fampl = 0;

	instances++;
 						// single thread access off
}

//destruction
FMOsc::~FMOsc()
{
	ScopedLock sl(csFMOsc);						// single thread access on
	if (instances <= 1) {if (tab) {delete[] tab; tab = NULL;}}
	instances--;
 						// single thread access off
}

// set phase synchronously (0..1) 
void FMOsc::SetPhase(float phase) 
{
	phase = __max(0,__min(1.0f,phase));
	nphi = SpecMath::fdtoi(1073741824.0*static_cast<double>(phase)) << 2;
	volatile int tmp = nphi;		// this trick ensures that pccnt1 is never
	if (nphi == tmp) {pccnt1++;}	// incremented before nphi has been updated			
}									// for x86/AMD64/ECMA memory models

// carrier audio and control update
void FMOsc::UpdateCar(	float* out, int samples, float invsmp, float pitch,
						float amp, float fbk, int* pmod, bool add			)
{
	int i,df,dfb; float x,dampl;

	// control rate processing
	x = pmin*SpecMath::fexp(pconv*__max(__min(pitch,1.0f),0));
	df = SpecMath::ffdtoi(static_cast<double>(invsmp)*
					(static_cast<double>(x) - static_cast<double>(dphi)));
	dampl = invsmp*(0.000061020255f*__max(__min(amp,1.0f),1e-8f) - fampl);
	x = 0.63f*FLT_INTMAX*__max(__min(fbk,1.0f),0);
	dfb = SpecMath::ffdtoi(static_cast<double>(invsmp)*
					(static_cast<double>(x) - static_cast<double>(fb)));
	while (pccnt1 != pccnt2) {phi = nphi; tval = 0; pccnt2++;}

	// audio rate processing
	if (add) {
		for (i=0; i<samples; i++) {
			tval = static_cast<int>
				(tab[abs((phi + pmod[i] + (fb >> 15)*tval) >> 17)]);
			out[i] += (fampl*static_cast<float>(tval));
			phi += dphi;
			dphi += df;
			fb += dfb;
			fampl += dampl;
		}
	}
	else {
		for (i=0; i<samples; i++) {
			tval = static_cast<int>
				(tab[abs((phi + pmod[i] + (fb >> 15)*tval) >> 17)]);
			out[i] = fampl*static_cast<float>(tval);
			phi += dphi;
			dphi += df;
			fb += dfb;
			fampl += dampl;
		}
	}
}

// modulator audio and control update
void FMOsc::UpdateMod(	int* out, int samples, float invsmp, float pitch,
						float amp, float fbk, int* pmod, bool add			)
{
	int i,df,dampl,dfb; float x;

	// control rate processing
	x = pmin*SpecMath::fexp(pconv*__max(__min(pitch,1.0f),0));
	df = SpecMath::ffdtoi(static_cast<double>(invsmp)*
					(static_cast<double>(x) - static_cast<double>(dphi)));
	x = FLT_INTMAX*__max(__min(amp,1.0f),0);
	dampl = SpecMath::ffdtoi(static_cast<double>(invsmp)*
					(static_cast<double>(x) - static_cast<double>(ampl)));
	x = 0.63f*FLT_INTMAX*__max(__min(fbk,1.0f),0);
	dfb = SpecMath::ffdtoi(static_cast<double>(invsmp)*
					(static_cast<double>(x) - static_cast<double>(fb)));
	while (pccnt1 != pccnt2) {phi = nphi; tval = 0; pccnt2++;}

	// audio rate processing
	if (add) {
		for (i=0; i<samples; i++) {
			tval = static_cast<int>
				(tab[abs((phi + pmod[i] + (fb >> 15)*tval) >> 17)]);
			out[i] += ((ampl >> 10)*tval);
			phi += dphi;
			dphi += df;
			fb += dfb;
			ampl += dampl;
		}
	}
	else {
		for (i=0; i<samples; i++) {
			tval = static_cast<int>
				(tab[abs((phi + pmod[i] + (fb >> 15)*tval) >> 17)]);
			out[i] = (ampl >> 10)*tval;
			phi += dphi;
			dphi += df;
			fb += dfb;
			ampl += dampl;
		}
	}
}

//******************************************************************************
//* virtual analog oscillator
//*
// static member initialization
int VAOsc::instances = 0;
float* VAOsc::tab = NULL;
namespace {static CriticalSection csVAOsc;}

// construction
VAOsc::VAOsc(float maxpitch, float minpitch, float smprate)
{
	ScopedLock sl(csVAOsc);						// single thread access on
	
	// create zero crossing half segment table if required
	static float c[8] = {	0.99986f,-2.97566f,-0.23930f,7.83529f,
							-3.25094f,-11.51283f,13.50376f,-4.36023f	};
	if (tab == NULL) {
		tab = new float[4096];				// no check for low space demand
		VectorFunctions::linear(tab,4096,1.0f/8192.0f,8191.0f/8192.0f);
		VectorFunctions::polyval(tab,4096,c,7);
		VectorFunctions::mul(tab,2147483648.0f,4096);
	}

	// init pitch conversion
	float x;
	if (maxpitch < minpitch) {x = maxpitch; maxpitch = minpitch; minpitch = x;}
	maxpitch = __max(0.0000611f*smprate,__min(0.333f*smprate,maxpitch));
	minpitch = __max(0.0000611f*smprate,__min(0.333f*smprate,minpitch));
	pmin = 4294967296.0f*minpitch/smprate;
	pconv = logf(maxpitch/minpitch);
	
	// init states
	phi = dphi = shp = nphi = pccnt1 = pccnt2 = 0;
	invf = 2147483647; pw = -1073741824;
	prevout = 0;

	instances++;
 						// single thread access off
}

// destruction
VAOsc::~VAOsc()
{
	ScopedLock sl(csVAOsc);						// single thread access on
	if (instances <= 1) {if (tab) {delete[] tab; tab = NULL;}}
	instances--;
 						// single thread access off
}

// set wave shape (0:saw, 1:pulse)
void VAOsc::SetShape(int shape) {shp = __max(0,__min(1,shape));}

// set phase synchronously (0..1)
void VAOsc::SetPhase(float phase)
{
	phase = __max(0,__min(1.0f,phase));
	nphi = SpecMath::fdtoi(1073741824.0*static_cast<double>(phase)) << 2;
	volatile int tmp = nphi;		// this trick ensures that pccnt1 is never
	if (nphi == tmp) {pccnt1++;}	// incremented before nphi has been	updated		
}									// for x86/AMD64/ECMA memory models 

// audio and control update
void VAOsc::Update(float* out,	int samples, float invsmp,
					float pitch, float pwidth, int* pmod	)
{
	int i,df,dinvf,idx,sphi,sphi2,dpw,tmp; float x,x2;
	
	// common control rate processing
	x = pmin*SpecMath::fexp(pconv*__max(__min(pitch,1.0f),0));
	df = SpecMath::ffdtoi(static_cast<double>(invsmp)*
					(static_cast<double>(x) - static_cast<double>(dphi)));
	x = 562949953421312.0f/x;
	dinvf = SpecMath::ffdtoi(static_cast<double>(invsmp)*
					(static_cast<double>(x) - static_cast<double>(invf)));
	while (pccnt1 != pccnt2) {phi = nphi; pccnt2++;}

	switch(shp) {
	case 0:
		// sawtooth audio rate processing
		for (i=0; i<samples; i++) {
			sphi = phi + pmod[i];
			x = static_cast<float>(sphi);
			sphi = abs(static_cast<signed int>(sphi - 2147483647 - 1) >> 1);
			if (dphi > sphi) {
				tmp = invf + i*dinvf;
				// ideal function, slow: idx = MulDiv(sphi,4096,dphi);
				idx = __min(131071,
					(int)(((icstdsp_int64)sphi * (icstdsp_int64)tmp) >> 32)) >> 5;
				if (x >= 0) {x -= tab[idx];} else {x += tab[idx];}
			}
			out[i] = prevout = 7.16401977e-10f*x - 0.53846153f*prevout;
			phi += dphi;
			dphi += df;
		}
		break;
	case 1:	
		// pulse specific control rate processing
		x = -2147483647.0f*__max(__min(pwidth,1.0f),0);
		dpw = SpecMath::ffdtoi(static_cast<double>(invsmp)*
						(static_cast<double>(x) - static_cast<double>(pw)));

		// pulse audio rate processing
		for (i=0; i<samples; i++) {
			sphi = phi + pmod[i];
			sphi2 = (pw << 1) - sphi;
			x = static_cast<float>(sphi);
			sphi = abs(static_cast<signed int>(sphi - 2147483647 - 1) >> 1);
			x2 = static_cast<float>(sphi2);
			sphi2 = abs(static_cast<signed int>(sphi2 - 2147483647 - 1) >> 1);
			if (dphi > sphi) {
				tmp = invf + i*dinvf;
				idx = __min(131071,
					(int)(((icstdsp_int64)sphi * (icstdsp_int64)tmp) >> 32)) >> 5;
				if (x >= 0) {x -= tab[idx];} else {x += tab[idx];}	
			}
			if (dphi > sphi2) {
				tmp = invf + i*dinvf;
				idx = __min(131071,
					(int)(((icstdsp_int64)sphi2 * (icstdsp_int64)tmp) >> 32)) >> 5;
				if (x2 >= 0) {x2 -= tab[idx];} else {x2 += tab[idx];}					
			}
			out[i] = prevout = 3.58200988e-10f*(x + x2) - 0.53846153f*prevout;
			phi += dphi;
			dphi += df;
			pw += dpw;
		}
		break;
	}
	invf += (dinvf*samples);
}

//******************************************************************************
//* raw sawtooth oscillator
//*
// construction
RawSawOsc::RawSawOsc(float maxpitch, float minpitch, float smprate)
{
	// init pitch conversion
	float x;
	if (maxpitch < minpitch) {x = maxpitch; maxpitch = minpitch; minpitch = x;}
	if (maxpitch > (0.0038f*smprate)) {mode = 0; x = 0.000023f;}
	else {mode = 1; x = 1e-7f;}
	maxpitch = __max(x*smprate,__min(0.4999f*smprate,maxpitch));
	minpitch = __max(x*smprate,__min(0.4999f*smprate,minpitch));
	pmin = 4294967296.0f*minpitch/smprate;
	if (mode == 1) {pmin *= 256.0f;}
	pconv = logf(maxpitch/minpitch);

	// init states
	phi = dphi = nphi = pccnt1 = pccnt2 = 0;
}

// set phase synchronously (0..1)
void RawSawOsc::SetPhase(float phase)
{
	phase = __max(0,__min(1.0f,phase));
	nphi = SpecMath::fdtoi(1073741824.0*static_cast<double>(phase)) << 2;
	volatile int tmp = nphi;		// this trick ensures that pccnt1 is never
	if (nphi == tmp) {pccnt1++;}	// incremented before nphi has been	updated		
}									// for x86/AMD64/ECMA memory models 

// audio and control update
void RawSawOsc::Update(float* out, int samples, float invsmp,
					   float pitch, int* pmod)
{
	int i,df; float x;
	
	// control rate processing
	x = pmin*SpecMath::fexp(pconv*__max(__min(pitch,1.0f),0));
	df = SpecMath::ffdtoi(static_cast<double>(invsmp)*
					(static_cast<double>(x) - static_cast<double>(dphi)));
	while (pccnt1 != pccnt2) {phi = nphi; pccnt2++;}

	// audio rate processing
	if (mode == 0) {	// audio mode
		for (i=0; i<samples; i++) {
			out[i] = 4.65661287e-10f*static_cast<float>(phi + pmod[i]);
			phi += dphi;
			dphi += df;
		}
	}
	else {	// LFO mode
		for (i=0; i<samples; i++) {
			out[i] = 4.65661287e-10f*static_cast<float>(phi + pmod[i]);
			phi += (dphi>>8);
			dphi += df;
		}
	}
}

//******************************************************************************
//* convert audio to phase modulation signal
//*
//* filter input -1..1 by 1st order lowpass with additional zero @ fs/2 and
//* convert it to wrap_to_int(-2^34..2^34)
// construction
AudioToPM::AudioToPM(float smprate, float fc)
{
	float x;
	if (smprate > 0) {x = M_PI_FLOAT*__min(__max(fc,5.0f)/smprate,0.499f);}
	else {x = M_PI_FLOAT*0.499f;}
	x = 2.0f*sinf(x)/(sinf(x) + cosf(x));
	c1 = x*536870912.0f;
	c2 = 1.0f - x;
	pmi = s1 = 0;
}

// audio and control update
void AudioToPM::Update(	float* in, int* out, int samples, float invsmp,
						float modint									)
{
	// control rate processing
	float x, dpmi = invsmp*(c1*__max(__min(modint,1.0f),1e-8f) - pmi);

	// audio rate processing
	float adn[2] = {ANTI_DENORMAL_FLOAT, -0.28f*ANTI_DENORMAL_FLOAT};
	for (int i=0; i<samples; i++) {
		x = pmi*in[i] + c2*s1 + adn[i & 1];
		out[i] = SpecMath::fdtoi(static_cast<double>(x + s1)) << 4;
		s1 = x;
		pmi += dpmi;
	}
}

//******************************************************************************
//* hilbert transformer: generates two orthogonal output signals
//*
//* max. phase error for f = 0.00015..0.49985fs is 0.017 degrees
// construction
Hilbert::Hilbert()
{
	VectorFunctions::set(s,0,33);
	adidx = 0;
	GetAntiDenormalTable(adtab,16);
}

// audio and control update
void Hilbert::Update(float* in, float* out1, float* out2, int samples)
{
	float adn[2];
	adn[0] = adtab[adidx]; adn[1] = adtab[adidx+1]; adidx = (adidx+2) & 0xe;
	float xa,xb,adin;
	for (int i=0; i<samples; i++) {
		adin = in[i] + adn[i&1];

		// out1 filter chain: 8 allpasses + 1 unit delay
		xa = s[1] - 0.999533593f*adin;		s[1] = s[0];
		s[0] = adin + 0.999533593f*xa;
		xb = s[3] - 0.997023120f*xa;		s[3] = s[2];
		s[2] = xa + 0.997023120f*xb;
		xa = s[5] - 0.991184054f*xb;		s[5] = s[4];
		s[4] = xb + 0.991184054f*xa;
		xb = s[7] - 0.975597057f*xa;		s[7] = s[6];
		s[6] = xa + 0.975597057f*xb;
		xa = s[9] - 0.933889435f*xb;		s[9] = s[8];
		s[8] = xb + 0.933889435f*xa;
		xb = s[11] - 0.827559364f*xa;		s[11] = s[10];
		s[10] = xa + 0.827559364f*xb;
		xa = s[13] - 0.590957946f*xb;		s[13] = s[12];
		s[12] = xb + 0.590957946f*xa;
		xb = s[15] - 0.219852059f*xa;		s[15] = s[14];
		s[14] = xa + 0.219852059f*xb;
		out1[i] = s[32]; s[32] = xb;

		// out2 filter chain: 8 allpasses
		xa = s[17] - 0.998478404f*adin;		s[17] = s[16];
		s[16] = adin + 0.998478404f*xa;
		xb = s[19] - 0.994786059f*xa;		s[19] = s[18];
		s[18] = xa + 0.994786059f*xb;
		xa = s[21] - 0.985287169f*xb;		s[21] = s[20];
		s[20] = xb + 0.985287169f*xa;
		xb = s[23] - 0.959716311f*xa;		s[23] = s[22];
		s[22] = xa + 0.959716311f*xb;
		xa = s[25] - 0.892466594f*xb;		s[25] = s[24];
		s[24] = xb + 0.892466594f*xa;
		xb = s[27] - 0.729672406f*xa;		s[27] = s[26];
		s[26] = xa + 0.729672406f*xb;
		xa = s[29] - 0.413200818f*xb;		s[29] = s[28];
		s[28] = xb + 0.413200818f*xa;
		out2[i] = s[31] - 0.061990080f*xa;	s[31] = s[30];
		s[30] = xa + 0.061990080f*out2[i];
	}
}

//******************************************************************************
//* first order lowpass filter
//*
//* exponential cutoff frequency control characteristic
// construction
Lowpass1::Lowpass1(float smprate, float fmin)
{
	fscl1 = sqrtf(2.0f*__min(0.495f,__max(0.00005f,fmin/smprate)));
	fscl2 = -logf(fscl1) + 0.5f*logf(2.0f*0.495f);
	fc = s = 0; adidx = 0;
	GetAntiDenormalTable(adtab,16);
}

// audio and control update
void Lowpass1::Update(float* data, int samples, float invsmp, float freq)
{
	// control processing
	float y, x = fscl1*SpecMath::qdexp(fscl2*__max(0,__min(1.0f,freq)));
	y = x*x; y *= y; y *= y;
	float dfc = invsmp*((1.25331414f + (0.10023925f*y - 0.35355339f)*x)*x - fc);
	
	// audio processing
	float adn[2];
	adn[0] = adtab[adidx]; adn[1] = adtab[adidx+1]; adidx = (adidx+2) & 0xe;
	for (int i=0; i<samples; i++) {
		fc += dfc;
		x = fc*fc*(data[i] - s + adn[i&1]);
		data[i] = x + s;
		s = x + data[i];
	}
}

//******************************************************************************
//* first order highpass filter
//*
//* exponential cutoff frequency control characteristic
// construction
Highpass1::Highpass1(float smprate, float fmin)
{
	fscl1 = sqrtf(2.0f*__min(0.495f,__max(0.00005f,fmin/smprate)));
	fscl2 = -logf(fscl1) + 0.5f*logf(2.0f*0.495f);
	fc = s = 0; adidx = 0;
	GetAntiDenormalTable(adtab,16);
}

// audio and control update
void Highpass1::Update(float* data, int samples, float invsmp, float freq)
{
	// control processing
	float y, x = fscl1*SpecMath::qdexp(fscl2*__max(0,__min(1.0f,freq)));
	y = x*x; y *= y; y *= y;
	float dfc = invsmp*((1.25331414f + (0.10023925f*y - 0.35355339f)*x)*x - fc);
	
	// audio processing
	float adn[2];
	adn[0] = adtab[adidx]; adn[1] = adtab[adidx+1]; adidx = (adidx+2) & 0xe;
	for (int i=0; i<samples; i++) {
		fc += dfc;
		x = fc*fc*(data[i] - s + adn[i&1]);
		data[i] -= (x + s);
		s += (2.0f*x);
	}
}

