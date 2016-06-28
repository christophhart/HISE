// fftooura.h
// header for Ooura's FFT package (www.kurims.kyoto-u.ac.jp/~ooura)
// n: size, isgn: 1=fwd,-1=inv, a:data, details see fftoouraf.cpp
// uses versions without workspace (no dynamic memory needed, 10% slower)
// multithreading not supported yet
// License (as found on his homepage, 28.7.08): 
// *** Copyright Takuya OOURA, 1996-2001
// You may use, copy, modify and distribute this code for any purpose 
// (include commercial use) and without fee. Please refer to this package
// when you modify this code. ***
//
// This code is part of the ICST DSP Library version 1.2. It is released
// under the 2-clause BSD license. Copyright (c) 2008-2010, Zurich
// University of the Arts, Beat Frei. All rights reserved.

#ifndef _ICST_DSPLIB_FFTOOURA_INCLUDED
#define _ICST_DSPLIB_FFTOOURA_INCLUDED

namespace icstdsp {		// begin library specific namespace 

void cdft(int n, int isgn, double *a);		// DFT	
void cdft(int n, int isgn, float *a);	
void rdft(int n, int isgn, double *a);		// real data DFT 
void rdft(int n, int isgn, float *a);
void ddct(int n, int isgn, double *a);		// DCT type 2
void ddct(int n, int isgn, float *a);
void ddst(int n, int isgn, double *a);		// DST type 2
void ddst(int n, int isgn, float *a);
void dfct(int n, double *a);				// real data symmetric DFT
void dfct(int n, float *a);
void dfst(int n, double *a);				// real data asymmetric DFT
void dfst(int n, float *a);

}	// end library specific namespace

#endif

