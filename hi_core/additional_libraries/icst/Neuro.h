// Neuro.h
//******************* neural networks and other classifiers **********************
//
// This code is part of the ICST DSP Library version 1.2. It is released
// under the 2-clause BSD license. Copyright (c) 2008-2010, Zurich
// University of the Arts, Beat Frei. All rights reserved.

#ifndef _ICST_DSPLIB_NEURO_INCLUDED
#define _ICST_DSPLIB_NEURO_INCLUDED

namespace icstdsp {		// begin library specific namespace

								// neuron types:
const int NT_SIG = 0;				// sigmoidal nonlinearity: tanh(x)
const int NT_LIN = 1;				// linear sum of inputs
const int NT_BIN = 2;				// binary: 1 or -1
								// Axons Reset() weight initialization modes:
const int WI_ZERO = 0;				// zero all
const int WI_ZEROBIASRNDOTHER = 1;	// zero bias weights, randomize other
const int WI_RND = 2;				// randomize all

class Neurons
{
// provides a layer of neurons with the following structure: 
//		k neurons {N1,N2,...,Nk} accessible by neuron indices 1..k 
//		a constant bias node at neuron index 0
// data exchange between neuron layers: connect "Neurons" by "Axons"
// data exchange neurons <-> outside world: direct read/write from/to n[1..k]
//
public:
	Neurons(int k,				// create k neurons, set bias node to 1
			int type=NT_SIG);	// type (default: sigmoidal nonlinearity)
	~Neurons();
	int GetNofNeurons();		// get number of neurons (success:k, failed:-1)
	int GetType();				// get type of neurons
	float* n;					// content of neurons
private:						
	int nn;						// number of internal nodes
	int ntype;					// neuron type
	Neurons& operator = (const Neurons& src);
	Neurons(const Neurons& src);
};

class Axons
{
// connects two neuron layers, performs forward and back propagation, provides
// learning functionality with continuous (default) and batch weight update
// connection:	create "Neurons"
//				create "Axons" that point to them
//				(note:	you may also connect a "Neurons" object to itself ->
//						FeedForward updates neurons serially in ascending order)
// evaluation:	write input to n[1..k] of topmost "Neurons"
//				call FeedForward top-down for every "Axons"
//				read output from n[1..k] of bottom "Neurons"
// BP learn:	evaluate input training vector (reading the output is optional)
//				call BackPropagate(learning rate,output training vector) for
//					bottom "Axons"
//				call BackPropagate(learning rate) bottom-up for remaining
//					"Axons" excluding the topmost
//				call BackPropagate(learning rate,NULL,true) for topmost "Axons"
//				(note:	if bottom=topmost "Axons" call BackPropagate(learning
//						rate,output training vector,true)	)
// apply GHA:	init weights by calling Reset(WI_ZEROBIASRNDOTHER)
//				loop:	write input vector to n[1..k] of input "Neurons"
//						call FeedForward
//						call GHA(learning rate)		
//				->	if output neurons are linear, then:
//					-	the weights of output neurons n[1..k] converge to the
//						ordered principal components of the input vector set
//					-	higher neuron index <-> less significant component
//					-	the current input vector is approximated by output
//						neuron values as sum(i=1..k){n[i] * weights of n[i]}
//					-	an input vector set with zero empirical mean yields
//						MSE-optimal components	
//
public:							// create axons with randomized weights
	Axons(	Neurons* in,		// pointer to input neurons
			Neurons* out	);	// pointer to output neurons			
	~Axons();
	void Reset(int mode=WI_RND);// init weights
	void FeedForward();			// update output neurons	
	void EnableBatchLearning();	// enable batch learning
	void DisableBatchLearning();// disable batch learning, update weights
	void BackPropagate(			// update weights using back propagation
			float lrate=0,		// learning rate
			float* out=NULL,	// if out is specified: process output neurons
			bool in=false	);	// as output layer with training vector
								// out[0..output_neurons-1] = [O1,O2,..,Ok]
								// in=true: process input neurons as input layer
	void Hebb(float lrate);		// simple hebbian weight update based on current
								// in/out neuron values, bias weights ignored
	void GHA(float lrate);		// update weights by generalized hebbian algorithm
	void ReadInputWeights(		// read weights of n-th input neuron to
			float* d, int n);	// d[0..output_neurons], n=0:bias node
	void WriteInputWeights(		// make d[0..output_neurons] the weights of
			float* d, int n);	// input neuron n, n=0:bias node, d[0] ignored
	void ReadOutputWeights(		// read weights of n-th output neuron to
			float* d, int n);	// d[0..input_neurons], d[0]=weight of bias node
	void WriteOutputWeights(	// make d[0..input_neurons] the weights 
			float* d, int n);	// of output neuron n, d[0]=weight of bias node
private:
	float* w;					// weights w[i + j*(input_neurons+1)],
								// i=0..input_neurons, j=0..output_neurons-1
	float* dw;					// batch learning delta weights, structure as w
	float* weff;				// points to currently updated weights (w or dw)
	Neurons* pin;				// pointer to input neurons
	Neurons* pon;				// pointer to output neurons
	float* xsav;				// working array used in BackPropagate 
	bool blon;					// true: batch learning on
	bool recurrent;				// true: input = output neurons
	Axons& operator = (const Axons& src);
	Axons(const Axons& src);
};

class Kohonen
{
// provides kohonen feature maps, vector quantization, and k-means clustering
// nodes are initialized from node 0 up with the first processed input vectors
// decay: value drops to value*exp(-decay) per call of "Learn"
//
public:							// create kohonen/VQ/k-means map
	Kohonen(int nodes,			// number of output nodes
			int vsize,			// input vector size
			float lrate,		// initial learning rate
			float alpha=0,		// learning rate decay
			float beta=0,		// neighborhood decay (for distance = 1)
			float* dist=NULL);	// output node topology (if any):
								// dist[nodes*nodes] = distance between nodes
	~Kohonen();
	void Reset();				// reset network to state after construction
	int GetNode(float* d);		// get output node corresponding to input vector
	void GetVector(	float* d,	// fill d with vector of a specified output node  
					int node );	//
	void Learn(	float* d );		// learn input vector d
	int KMeans(	float* d,		// perform k-means clustering on a set of input 
				int vecs,		// vectors d[0..vsize-1,..,..vecs*vsize-1], fill
				int* kmnode	);	// kmnode[0..vecs-1] with associated nodes,
private:						// return iteration count
	float* w;					// weights w[i+j*vsize], i=0..vsize-1,j=0..nodes-1
	float* nbh;					// neighborhood nbh[i+j*nodes], i,j=0..nodes-1
	float* dnbh;				// nbh decay dnbh[i+j*nodes], i,j=0..nodes-1
	int* cnt;					// vectors per node (KMeans only)
	float ilr;					// initital learning rate
	float lr;					// current learning rate
	float dlr;					// learning rate decay
	int nn;						// number of output nodes
	int vsz;					// input vector size
	int initcnt;				// init node counter
	Kohonen& operator = (const Kohonen& src);
	Kohonen(const Kohonen& src);
};

class PCA
{
// principal and independent component analysis
// component properties: orthonormal, zero mean, sign undetermined 
//
public:
static int pcomp(				// calculate n principal components of vecs 
			float* pcs,			// vectors d[0..vsize-1,..,..vecs*vsize-1]
			float* mean,		// ordering: higher index <-> lower significance
			float* var,			// components -> pcs[0..vsize-1,..,..n*vsize-1]
			float* d,			// mean of input vectors -> mean[0..vsize-1]
			int vsize,			// relative variance per component -> var[0..n-1]  
			int vecs,			// residuals -> d[0..vsize-1,..,..vecs*vsize-1]
			int n		);		// return number of computed components
static void decompose(			// decompose vector d[0..vsize-1] into a sum		
			float* a,			// of the optionally specified mean vector
			float* pcs,			// mean[0..vsize-1] and n weighted principal 
			float* d,			// components pcs[0..vsize-1,..,..n*vsize-1]
			int vsize,			// -> weights of components a[0..n-1]
			int n,				// -> residual d[0..vsize-1]
			float* mean=NULL );	// 
static void compose(			// compose vector d[0..vsize-1] as the sum of			
			float* d,			// a residual d[0..vsize-1] and n principal
			float* pcs,			// components pcs[0..vsize-1,..,..n*vsize-1]
			float* a,			// weighted by a[0..n-1] and an optional
			int vsize,			// mean vector mean[0..vsize-1]
			int n,				// 
			float* mean=NULL );	//
static void ica(				// use FastICA algorithm to get m independent 
			float* ics,			// components ics[0..vsize-1,..,..m*vsize-1]
			float* pcs,			// from n principal components
			int vsize,			// pcs[0..vsize-1,..,..n*vsize-1]
			int m,				//	
			int n		);		// 		 
};

}	// end library specific namespace

#endif

