// Neuro.cpp
//
// This code is part of the ICST DSP Library version 1.2. It is released
// under the 2-clause BSD license. Copyright (c) 2008-2010, Zurich
// University of the Arts, Beat Frei. All rights reserved.

#if DONT_INCLUDE_HEADERS_IN_CPP
#else
#include "common.h"
#include "Neuro.h"
#include "MathDefs.h"
#include "BlkDsp.h"
#include "SpecMath.h"
#endif



//******************************************************************************
//* Neurons
//*
// construction
Neurons::Neurons(int k, int type)
{
	if ((type == NT_SIG) || (type == NT_LIN) || (type == NT_BIN)) {ntype = type;}  
	else {ntype = NT_SIG;}
	if (k >= 0) {
		nn = k+1;
		n = VectorFunctions::sseallocf(nn);
		if (n) {n[0] = 1.0f;} else {nn = 0;}
	}
	else {n = NULL; nn = 0;}	
}

// destruction
Neurons::~Neurons() {if (n) VectorFunctions::ssefree(n);}

// return number of neurons, failed:-1
int Neurons::GetNofNeurons() {return nn-1;}

// return type of neurons
int Neurons::GetType() {return ntype;}
			
//******************************************************************************
//* Axons
//*
// construction
Axons::Axons(Neurons* in, Neurons* out)
{
	int size,nnin,nnout;
	pin = in; pon = out; w = dw = xsav = NULL; recurrent = false;
	if ((pin == NULL) || (pon == NULL)) return;
	if (pin == pon) {recurrent = true;}
	nnin = pin->GetNofNeurons() + 1;
	nnout = pon->GetNofNeurons();
	if ((nnout <= 0) || (nnin <= 0)) return;
	size = nnin*nnout;
	w = VectorFunctions::sseallocf(size);
	xsav = VectorFunctions::sseallocf(nnin);
	if (xsav == NULL) {
		if (w) {VectorFunctions::ssefree(w);}
		w = NULL;
	}
	Reset();
}

// destruction
Axons::~Axons()
{
	if (w) VectorFunctions::ssefree(w);
	if (dw) VectorFunctions::ssefree(dw);
	if (xsav) VectorFunctions::ssefree(xsav);
}

// init weights
void Axons::Reset(int mode) 
{
	int i, nnin = pin->GetNofNeurons() + 1, nnout = pon->GetNofNeurons();
	int size = nnin*nnout;
	if (w) {
		switch(mode) {
		case WI_ZERO:
			VectorFunctions::set(w,0,size); 
			break;
		case WI_ZEROBIASRNDOTHER:
			VectorFunctions::unoise(w,size);
			VectorFunctions::mul(w,1.6f/static_cast<float>(nnin),size);
			for (i=0; i<size; i+=nnin) {w[i] = 0;}
			break;
		case WI_RND:
		default:
			VectorFunctions::unoise(w,size);
			VectorFunctions::mul(w,1.6f/static_cast<float>(nnin),size);			
			break;
		}
	}	
	if (dw) {VectorFunctions::set(dw,0,size);}
	weff = w; blon = false;
}			

// update output neurons
void Axons::FeedForward() 
{
	int i,j,nnin,nnout,type;
	if (w) {
		nnin = pin->GetNofNeurons() + 1;
		nnout = pon->GetNofNeurons();
		type = pon->GetType();
		if (recurrent) {
			j=0;
			switch (type) {
			case NT_SIG:	
				for (i=1; i<=nnout; i++) {
					pon->n[i] = SpecMath::qdtanh(VectorFunctions::dotp(w+j,pin->n,nnin));
					j += nnin;
				}
				break;
			case NT_LIN:	
				for (i=1; i<=nnout; i++) {
					pon->n[i] = VectorFunctions::dotp(w+j,pin->n,nnin);
					j += nnin;
				}
				break;
			case NT_BIN:	
				for (i=1; i<=nnout; i++) {
					pon->n[i] = 
						(VectorFunctions::dotp(w+j,pin->n,nnin) < 0) ? -1.0f : 1.0f;
					j += nnin;
				}
				break;
			}
		}
		else {
			VectorFunctions::mmulv(	pon->n + 1,
							w,
							pin->n,
							nnout,
							nnin		);
			switch (type) {
			case NT_SIG:	i=1;
							while (i <= (nnout-3)) {SpecMath::qdtanh(pon->n + i); i+=4;}
							while (i <= nnout) {pon->n[i] = SpecMath::qdtanh(pon->n[i]); i++;}
							break;
			case NT_LIN:	break;
			case NT_BIN:	VectorFunctions::sgn(pon->n + 1, nnout); break;
			}
		}
	}
}

// enable batch learning
void Axons::EnableBatchLearning()
{
	int size = (pin->GetNofNeurons() + 1)*pon->GetNofNeurons();
	if (w == NULL) return;
	if (!blon) {
		if (dw == NULL) {dw = VectorFunctions::sseallocf(size);}
		if (dw) {VectorFunctions::set(dw,0,size); weff = dw; blon = true;}
	}	
}

// disable batch learning, update weights
void Axons::DisableBatchLearning()
{
	int size = (pin->GetNofNeurons() + 1)*pon->GetNofNeurons();
	if (blon) {VectorFunctions::add(w,dw,size); weff = w; blon = false;}
}

// update weights using back propagation
void Axons::BackPropagate(float lrate, float* out, bool in)
{
	int i,nnin,nnout; float x;
	if (w == NULL) return;

	// output layer
	if (out) {
		switch (pon->GetType()) {
		case NT_SIG:
			for (i=1; i<pon->GetNofNeurons() + 1; i++) {
				x = pon->n[i];
				pon->n[i] = (1.0f - x*x)*(out[i-1] - x);
			}
			break;
		case NT_LIN:
		case NT_BIN:
			for (i=1; i<pon->GetNofNeurons() + 1; i++) {
				pon->n[i] = out[i-1] - pon->n[i];
			}
			break;
		}
		BackPropagate(lrate,NULL,in);
	}

	// input layer
	else if (in) {
		if (lrate != 0) {
			nnin = 	pin->GetNofNeurons() + 1; nnout = pon->GetNofNeurons();
			for (i=0; i<nnout; i++) {
				x = lrate*pon->n[i+1];
				VectorFunctions::mac(weff + i*nnin, pin->n, x, nnin);
			}
		}
	}

	// hidden layer	
	else {
		nnin = 	pin->GetNofNeurons() + 1; nnout = pon->GetNofNeurons();
		VectorFunctions::copy(xsav,pin->n,nnin);
		VectorFunctions::mtmulv(	pin->n,				// leaves room for optimization
						w,					// as the bias node should not
						(pon->n)+1,			// be replaced by the delta ->
						nnout,				// modify "mtmulv" or make a
						nnin	);			// special local version of it
		pin->n[0] = xsav[0];				// 
		switch (pin->GetType()) {
		case NT_SIG:
			for (i=1; i<nnin; i++) {pin->n[i] *= (1.0f - xsav[i]*xsav[i]);}
			break;
		case NT_LIN:
		case NT_BIN:
			break;
		}
		if (lrate != 0) {
			for (i=0; i<nnout; i++) {
				x = lrate*pon->n[i+1];
				VectorFunctions::mac(weff + i*nnin, xsav, x, nnin);
			}
		}
	}
}
						
// simple hebbian weight update based on current input/output neuron values
// bias weights are ignored
void Axons::Hebb(float lrate)
{
	int i, nnin = pin->GetNofNeurons() + 1, nnout = pon->GetNofNeurons();
	if (w == NULL) return;
	for (i=0; i<nnout; i++) {
		VectorFunctions::mac(weff + i*nnin + 1, 1 + pin->n, lrate*pon->n[i+1], nnin-1);
	}
}

// apply generalized hebbian algorithm (a.k.a. sanger's rule)
void Axons::GHA(float lrate)
{
	int i,j,k, nnin = pin->GetNofNeurons() + 1, nnout = pon->GetNofNeurons();
	float x,y;
	if (w == NULL) return;
	for (i=1; i<nnin; i++) {
		x = pin->n[i];
		k = i;
		for (j=1; j<=nnout; j++) {
			y = pon->n[j];
			x -= w[k]*y;
			weff[k] += lrate*x*y;
			k += nnin;
		}
	}
}

// read weights of n-th input neuron to d[0..output_neurons], d[0]=0
void Axons::ReadInputWeights(float* d, int n)
{
	int nnin = pin->GetNofNeurons() + 1, nnout = pon->GetNofNeurons();
	if ((w != NULL) && (n >= 0) && (n < nnin)) {
		VectorFunctions::deinterleave(d+1, w, nnout, nnin, n); d[0]=0;
	}
	else if (nnout >= 0) {VectorFunctions::set(d,0,nnout+1);}
}

// make d[0..output_neurons] the weights of input neuron n
void Axons::WriteInputWeights(float* d, int n)
{
	int nnin = pin->GetNofNeurons() + 1, nnout = pon->GetNofNeurons();
	if ((w != NULL) && (n >= 0) && (n < nnin)) {
		VectorFunctions::interleave(w, d+1, nnout, nnin, n);
	}
}

// read weights of output neuron n to d[0..input_neurons]
void Axons::ReadOutputWeights(float* d, int n)
{
	int nnin = pin->GetNofNeurons() + 1, nnout = pon->GetNofNeurons();
	if ((w != NULL) && (n > 0) && (n <= nnout)) {
		VectorFunctions::copy(d, w + (n-1)*nnin, nnin);
	}
	else if (nnin > 0) {VectorFunctions::set(d,0,nnin);}
}

// make d[0..input_neurons] the weights of output neuron n
void Axons::WriteOutputWeights(float* d, int n)
{
	int nnin = pin->GetNofNeurons() + 1, nnout = pon->GetNofNeurons();
	if ((w != NULL) && (n > 0) && (n <= nnout)) {
		VectorFunctions::copy(w + (n-1)*nnin, d, nnin);
	}	
}

//******************************************************************************
//* Kohonen feature maps + vector quantization
//*
// construction
Kohonen::Kohonen(	int nodes, int vsize, float lrate, float alpha,
					float beta, float* dist							)
{
	w = nbh = dnbh = NULL; nn = nodes; vsz = vsize; ilr = lrate;

	// allocate memory for weights and neighborhood relations
	if ((nn > 0) && (vsz > 0)) {
		w = VectorFunctions::sseallocf(nn*vsz); 
		if (w == NULL) {nn = vsz = 0;}
		try {cnt = new int[nn];} catch(...) {cnt = NULL;}
		if (cnt == NULL) { 	
			if (w) {VectorFunctions::ssefree(w); w = NULL;}
			nn = vsz = 0;
		}
		if (dist) {
			nbh = VectorFunctions::sseallocf(nn*nn);
			if (nbh == NULL) {
				if (w) {VectorFunctions::ssefree(w); w = NULL;}
				if (cnt) {delete[] cnt; cnt = NULL;}
				nn = vsz = 0;
			}
			dnbh = VectorFunctions::sseallocf(nn*nn);
			if (dnbh == NULL) {
				if (w) {VectorFunctions::ssefree(w); w = NULL;}
				if (cnt) {delete[] cnt; cnt = NULL;}
				if (nbh) {VectorFunctions::ssefree(nbh); nbh = NULL;}
				nn = vsz = 0;
			}
		}
	}
	else {nn = vsz = 0;}
	
	// init weights
	initcnt = 0;
	if (w) {VectorFunctions::set(w,0,nn*vsz);}
	
	// init learning rate and neighborhood relations
	int i;
	lr = lrate; dlr = expf(-alpha);
	if (nbh) {
		for (i=0; i<(nn*nn); i++) {
			nbh[i] = lrate;
			dnbh[i] = expf(- alpha - beta*dist[i]);
		}
	}
}

// destruction	
Kohonen::~Kohonen() 
{
	if (w) VectorFunctions::ssefree(w);
	if (nbh) VectorFunctions::ssefree(nbh);
	if (dnbh) VectorFunctions::ssefree(dnbh);
	if (cnt) delete[] cnt;
}

// reset network to state after construction
void Kohonen::Reset()
{
	if (w == NULL) return;
	initcnt = 0;
	VectorFunctions::set(w,0,nn*vsz);
	lr = ilr;
	if (nbh) {VectorFunctions::set(nbh,ilr,nn*nn);}
}			

// return output node corresponding to input vector
int Kohonen::GetNode(float* d)
{
	if (w == NULL) return -1;
	float x,y, m=0;
	int i,j, k=vsz, midx=0;
	if (vsz < 64) {
		for (j=0; j<vsz; j++) {
			x = d[j] - w[j]; 
			m += (x*x);
		}
		for (i=1; i<nn; i++) {
			y=0;
			for (j=0; j<vsz; j++) {
				x = d[j] - w[k];
				y += (x*x);
				k++;
			}
			if (y < m) {m=y; midx=i;}
		}
	}	
	else {
		m = VectorFunctions::sdist(d, w, vsz);
		for (i=1; i<nn; i++) {
			y = VectorFunctions::sdist(d, w+k, vsz);
			k += vsz;
			if (y < m) {m=y; midx=i;}
		}
	}
	return midx;
}

// fill d with vector of a specified output node  
void Kohonen::GetVector(float* d, int node)
{
	if ((w == NULL) || (node < 0) || (node >= nn)) {VectorFunctions::set(d,0,vsz);}
	else {VectorFunctions::copy(d,w+node*vsz,vsz);}
}

// learn input vector d
void Kohonen::Learn(float* d)
{
	if (w == NULL) return;
	if (initcnt < nn) {							// process as init vector
		VectorFunctions::copy(w+initcnt*vsz,d,vsz);
		initcnt++;
		return;
	}
	float x,y;
	int i,j, k=0, node = GetNode(d);
	if (nbh) {									// kohonen map:
		for (i=0; i<nn; i++) {					// consider node topology
			y = nbh[node*nn + i];
			for (j=0; j<vsz; j++) {
				x = y*(d[j] - w[k]);
				w[k] += x;
				k++;
			}
		}
		VectorFunctions::mul(nbh,dnbh,nn*nn);
	}
	else {										// linear vector quantization: 
		k = node*vsz;							// independent nodes
		for (j=0; j<vsz; j++) {
			x = lr*(d[j] - w[k]);
			w[k] += x;
			k++;
		}	
		lr *= dlr;
	}	
}

// perform k-means clustering on vector set d[0..vsize-1,..,..vecs*vsize-1],
// fill kmnode[0..vecs-1] with associated nodes, return iteration count
int Kohonen::KMeans(float* d, int vecs, int* kmnode)
{
	if (w == NULL) return 0;
	int i,j,k,m,p, its=0;
	bool conv;
	float scl;											
	initcnt = __min(vecs,nn);
	VectorFunctions::copy(w,d,initcnt*vsz);
	for (i=0; i<vecs; i++) {kmnode[i] = -1;}
iter:
	conv = true;
	for (i=0; i<nn; i++) {cnt[i] = 0;}
	for (i=0; i<vecs; i++) {
		k = GetNode(d + i*vsz);
		if (k != kmnode[i]) {kmnode[i] = k; conv = false;}
		cnt[kmnode[i]]++;
	}
	if (conv) return its;
	for (i=0; i<nn; i++) {
		if (cnt[i] <= 0) continue;
		m = i*vsz; 
		for (j=0; j<vsz; j++) {w[m+j] = 0;} 
		for (j=0; j<vecs; j++) {
			if (kmnode[j] == i) {
				p = j*vsz;
				for (k=0; k<vsz; k++) {w[m+k] += d[p+k];}
			}
		}
		scl = 1.0f/static_cast<float>(cnt[i]);
		for (j=0; j<vsz; j++) {w[m+j] *= scl;}
	}
	its++;
	goto iter;
}

//******************************************************************************
//* principal component analysis
//*
// compute n principal components of vecs vectors d[0..vsize-1,..,vecs*vsize-1]
// components with higher indices have lower significance
// components -> pcs[0..vsize-1,..,..n*vsize-1]
// mean of input vectors -> mean[0..vsize-1]
// relative variance explained by each component -> var[0..n-1]
// residual vectors -> d[0..vsize-1,..,..vecs*vsize-1]
// return number of computed components
int PCA::pcomp(float* pcs, float* mean, float* var, float* d,
			   int vsize, int vecs, int n)
{
	int i,j,k; float x,m,pm,vnorm, rnrg=1.0f;

	// calculate and subtract mean vector
	VectorFunctions::set(mean,0,vsize);
	for (i=0; i<vecs; i++) {VectorFunctions::add(mean,d+i*vsize,vsize);}
	VectorFunctions::mul(mean,1.0f/static_cast<float>(vecs),vsize);
	for (i=0; i<vecs; i++) {VectorFunctions::sub(d+i*vsize,mean,vsize);}
	vnorm = VectorFunctions::energy(d,vecs*vsize);
	if (vnorm >= FLT_MIN) {vnorm = 1.0f/vnorm;} else return 0;

	// get principal components by repeated calculation of the dominant one
	// via expectation maximum (may also be viewed as batch hebbian learning
	// with weight normalization) followed by deflation
	float* v = VectorFunctions::sseallocf(vsize);
	for (j=0; j<n; j++) {
		VectorFunctions::unoise(pcs,vsize);
		m = 0;
		do {
			pm = m;
			VectorFunctions::set(v,0,vsize);
			for (i=0,k=0; i<vecs; i++,k+=vsize) {
				x = VectorFunctions::dotp(pcs,d+k,vsize);
				VectorFunctions::mac(v,d+k,x,vsize);
			}
			m = VectorFunctions::norm(v,vsize);
			if (m < FLT_MIN) {VectorFunctions::ssefree(v); return j;}
			VectorFunctions::mul(v,1.0f/m,vsize);
			VectorFunctions::copy(pcs,v,vsize);
		}
		while (pm < m);
		for (i=0,k=0; i<vecs; i++,k+=vsize) {
			x = -VectorFunctions::dotp(pcs,d+k,vsize);
			VectorFunctions::mac(d+k,pcs,x,vsize);
		}
		x = vnorm*VectorFunctions::energy(d,vecs*vsize);
		var[j] = rnrg - x;
		rnrg = x;
		pcs += vsize;
	}
	VectorFunctions::ssefree(v);
	return n;
}

// decompose vector d[0..vsize-1] into a sum of the (optionally specified) 
// mean vector mean[0..vsize-1] and n weighted principal components
// pcs[0..vsize-1,..,..n*vsize-1] -> 
// weights of components a[0..n-1], residual d[0..vsize-1] 
void PCA::decompose(float* a, float* pcs, float* d, int vsize, int n,
					float* mean)
{
	if (mean) {VectorFunctions::sub(d,mean,vsize);}
	for (int i=0; i<n; i++) {
		a[i] = VectorFunctions::dotp(d,pcs,vsize);
		VectorFunctions::mac(d,pcs,-a[i],vsize);
		pcs += vsize;
	}
}

// compose vector d[0..vsize-1] as the sum of a residual d[0..vsize-1] and
// n principal components pcs[0..vsize-1,..,..n*vsize-1] weighted by a[0..n-1] 
// and an optional mean vector mean[0..vsize-1]
void PCA::compose(float* d, float* pcs, float* a, int vsize, int n, float* mean)
{
	if (mean) {VectorFunctions::add(d,mean,vsize);}
	for (int i=0; i<n; i++) {
		VectorFunctions::mac(d,pcs,a[i],vsize);
		pcs += vsize;
	}	
}

// use FastICA algorithm to get m independent from n principal components
// input:	principal components pcs[0..vsize-1,..,..n*vsize-1]
// output:	independent components ics[0..vsize-1,..,..m*vsize-1]
void PCA::ica(float* ics, float* pcs, int vsize, int m, int n)
{
	m = __min(m,n);
	int i,j; float x,pp,p;
	float fvsize = static_cast<float>(vsize);
	float plim = 1.0f - FLT_EPSILON*5.0f*sqrtf(fvsize*static_cast<float>(n));
	float scl = sqrtf(fvsize);
	float invscl = 1.0f/scl;
	float* v = VectorFunctions::sseallocf(vsize);	// workspace (distorted independent comp.)
	float* wtmp = new float[n];				// workspace (intermediate weight)
	float* w = new float[n*m];				// demixing weights
	float* wcur = w;						// points to currently calculated weight
	
	VectorFunctions::unoise(w,m*n);
	for (i=0; i<(n*m); i+=n) {VectorFunctions::normalize(w+i,n);}

	for (i=0; i<m; i++) {
		p = 0;
		do {
			pp = p;
			VectorFunctions::mtmulv(ics,pcs,wcur,n,vsize);
			VectorFunctions::copy(v,ics,vsize);
			VectorFunctions::mul(v,scl,vsize);
			j=0;
			while (j <= (vsize-4)) {SpecMath::qdtanh(v+j); j+=4;}
			while (j < vsize) {v[j] = SpecMath::qdtanh(v[j]); j++;}
			x = invscl*VectorFunctions::energy(v,vsize) - scl;
			VectorFunctions::mmulv(wtmp,pcs,v,n,vsize);
			VectorFunctions::mac(wtmp,wcur,x,n);
			VectorFunctions::normalize(wtmp,n);
			if (i > 0) {
				for (j=0; j<(i*n); j+=n) {
					x = VectorFunctions::dotp(wtmp,w+j,n);
					VectorFunctions::mac(wtmp,w+j,-x,n);
				}
				VectorFunctions::normalize(wtmp,n);
			}
			p = fabsf(VectorFunctions::dotp(wtmp,wcur,n));
			VectorFunctions::copy(wcur,wtmp,n);
		}
		while ((pp < p) || (p < plim));
		wcur += n;
		ics += vsize;
	}

	VectorFunctions::ssefree(v); delete[] w; delete[] wtmp;
}

