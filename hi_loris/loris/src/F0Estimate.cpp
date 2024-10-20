/*
 * This is the Loris C++ Class Library, implementing analysis, 
 * manipulation, and synthesis of digitized sounds using the Reassigned 
 * Bandwidth-Enhanced Additive Sound Model.
 *
 * Loris is Copyright (c) 1999-2010 by Kelly Fitz and Lippold Haken
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY, without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 * F0Estimate.C
 *
 * Implementation of an iterative alrogithm for computing an 
 * estimate of fundamental frequency from a sequence of sinusoidal
 * frequencies and amplitudes using a likelihood estimator
 * adapted from Quatieri's Speech Signal Processing text. The 
 * algorithm here takes advantage of the fact that spectral peaks
 * have already been identified and extracted in the analysis/modeling
 * process.
 *
 * Kelly Fitz, 28 March 2006
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */

#if HAVE_CONFIG_H
	#include "config.h"
#endif

#include "F0Estimate.h"

#include "LorisExceptions.h"	// for Assert

#include "Notifier.h"

#include <algorithm>
#include <cmath>
#include <numeric>

#include <vector>
using std::vector;

#if defined(HAVE_M_PI) && (HAVE_M_PI)
	const double Pi = M_PI;
#else
	const double Pi = 3.14159265358979324;
#endif

// #if defined(HAVE_ISFINITE) && (HAVE_ISFINITE)
//     using std::isfinite;
//    
//	isfinite is not, after all, part of the standard, 
//	it is an extension. If it is not provided, the following
//	checks for NaN and finite-ness.
//
//	Use this instead.
// 	This code is taken from 
//		http://www.johndcook.com/IEEE_exceptions_in_cpp.html

#include <float.h>
inline bool IsFiniteNumber( double x )
{
	//	DBL_MAX is defined in float.h.
	//	Comparisons with NaN always fail. 
	
    return (x <= DBL_MAX && x >= -DBL_MAX);		
}
	



//	begin namespace
namespace Loris {

// ---------------------------------------------------------------------------
//	forward declarations for helpers, defined below
//  Q is the likelihood function, Qprime is its derivative w.r.t. frequency

static double
secant_method( const vector<double> & amps, 
               const vector<double> & freqs, 
               double f1, double f2,
               double precision );

static void
compute_candidate_freqs( const vector< double > & peak_freqs,
                         double fmin, double fmax, 
                         vector< double > & eval_freqs );                    			
static void
evaluate_Q( const vector<double> & amps, 
			const vector<double> & freqs, 
			const vector<double> & eval_freqs, 
			vector<double> & Q );
                    
static double
evaluate_Q( const vector<double> & amps, 
			const vector<double> & freqs, 
			double eval_freq,
            double norm );

static double
evaluate_Qprime( const vector<double> & amps, 
                 const vector<double> & freqs, 
                 double eval_freq );
         
static void
evaluate_Q( const vector<double> & amps, 
            const vector<double> & freqs, 
            const vector<double> & eval_freqs, 
            vector<double> & Q,
            double norm );
            
// ---------------------------------------------------------------------------
                                    
               
// ---------------------------------------------------------------------------
//  F0Estimate constructor
// ---------------------------------------------------------------------------
//  Construct from parameters of the iterative F0 estimation 
//  algorithm. Find candidate F0 estimates as integer divisors
//  of the peak frequencies, pick the highest frequency of the
//  most likely candidates, and refine that estiamte using the
//  secant method. 
//
//  Store the frequency and the normalized value of the 
//  likelihood function at that frequency (1.0 indicates that
//  all the peaks are perfect harmonics of the estimated
//  frequency).
//
//  See the F0Estimate.h for a description of the algorithm, also
//  outlined inline below.

F0Estimate::F0Estimate( const vector<double> & amps, 
                        const vector<double> & freqs, 
                        double fmin, double fmax,
                        double resolution ) :
    m_frequency( 0 ), 
    m_confidence( 0 )
{
    if ( fmin > fmax )
    {
        std::swap( fmin, fmax );
    }
	//	never consider DC (0 Hz) to be a valid fundamental
	fmin = std::max( 1., fmin );
    
    // -------------------------------------------------------------------------    
    // 1)  Identify candidate F0s as the integer divisors of the sinusoidal 
    //     frequencies provided, within the specified range (this algorithm
    //     relies on the reasonable assumption that for any frequency recognized 
    //     as a likely F0, at least one of the sinusoidal frequencies must 
    //     represent a harmonic, the likelihood function makes this same     
    
    //  First collect candidate frequencies: all integer 
    //  divisors of the peak frequencies that are between 
    //  fmin and fmax.
    vector< double > eval_freqs;
    compute_candidate_freqs( freqs, fmin, fmax, eval_freqs );

    if ( ! eval_freqs.empty() )
    {
        //  Compute a normalization factor equal to the total
        //  energy represented by all the peaks passed in
        //  amps and freqs, so that the value of the likelihood
        //  function does not depend on the overall signal 
        //  amplitude, but instead depends only on the quality
        //  of the estimate, or the confidence in the result, 
        //  and the quality of the final estimate can be evaluated
        //  by the value of the likelihood function.
        double normalization = 
            1.0 / std::inner_product( amps.begin(), amps.end(), amps.begin(), 0.0 );
            
        //  Evaluate the likelihood function at the candidate frequencies.
        vector < double > Q( eval_freqs.size() );
        evaluate_Q( amps, freqs, eval_freqs, Q, normalization );

        // -------------------------------------------------------------------------    
        // 2)  Select the highest frequency candidate that nearly maximizes the 
        //     likelihood function (because all subharmonics of the true F0 will
        //     be equal in likelihood to the true F0, but no higher frequency can
        //     be as likely).
    
        //  Find the highest frequency corresponding to a high value of Q
        //  (the most likely candidate).            

        vector<double>::size_type idx = 
            std::max_element( Q.begin(), Q.end() ) - Q.begin();
        
        double bestFreq = eval_freqs[ idx ];
        double bestQ = Q[ idx ];
        
        // -------------------------------------------------------------------------    
        // 2a) Check the likelihood of integer multiples of the best candidate,
        //     choose the highest multiple (within the specified range) that
        //     as likely as the best candidate frequency to be the new best
        //     candidate. 

        //  Check integer multiples of the best candidate frequency, 
        //  so that we can be certain that the peak doesn't 
        //  correspond to a subharmonic of the true most-likely
        //  F0 in the range [fmin,fmax].
        //
        //  While the next octave up is in range, and its likelihood
        //  is within 5% of the previously found peak, accept the
        //  octave as the better candidate (when we reach the true
        //  best candidate frequency, the next multiple should be
        //  much less likely).
        
        double nextF = 2 * bestFreq;
        double nextQ = evaluate_Q( amps, freqs, nextF, normalization );
        
        while ( fmax > nextF && ( 0.95 * bestQ ) <  nextQ )
        {
            //  update best candidate:
            bestFreq = nextF;
            bestQ = nextQ;
            
            //  consider the next multiple
            nextF += bestFreq;
            nextQ = evaluate_Q( amps, freqs, nextF, normalization );                        
        }          
  
//        notifier << "peak is : " << bestFreq
//                 << " Hz, Q: " << bestQ << endl;
        
        // -------------------------------------------------------------------------    
        // 3)  Refine the best candidate using the secant method for refining the 
        //     root of the derivative of the likelihood function in the neighborhood
        //     of the best candidate (because a peak in the likelihood function is
        //     a root of the derivative of that function).


        //  Refine this estimate using the secant method.    
        //
        //  Check the derivative function: if the slope (derivative)
        //  is positive, then assume that bestFreq is just below the
        //  root, and choose a second frequency just greater than
        //  bestFreq. Otherwise, assume that bestFreq is just above
        //  the root of the derivative function, and choose a second
        //  frequency just below bestFreq.
        
        double altFreq = bestFreq - resolution;
        if ( 0 < evaluate_Qprime( amps, freqs, bestFreq ) )
        {
            altFreq = bestFreq + resolution;
        }
        
        //  Now invoke the secant method to attempt to refine
        //  the root estimate:
        m_frequency = secant_method( amps, freqs, 
                                     bestFreq, altFreq,
                                     resolution );


//        notifier << "best candidate is : " << bestFreq
//                 << " Hz, Q: " << bestQ << endl;
//        notifier << "secant method found : " << m_frequency
//                 << " Hz, Q: " 
//                 << evaluate_Q( amps, freqs, m_frequency, normalization ) << endl;
//                 
        
        //  What if the secant method flies off to some other root? 
        //  Check that the root is still between fmin and fmax.
        if ( m_frequency < fmin || m_frequency > fmax )
        {
            //  If refining fails, just use the best candidate estimate.
            m_frequency = bestFreq;
        }
        
        
        //
        //  Could also use the bisection method, or the false position method, which 
        //  always converge. All that is required is that two points on the
        //  function (the derivative of the likelihood function, in this case)
        //  having opposite signs are used to begin the search. So we need
        //  to first find a nearby freq at which the derivative of Q evaluates
        //  with the opposite sign as bestFreq.         
        //
        
                        
        //  Compute the value of the likelihood function at this frequency.
        m_confidence = evaluate_Q( amps, freqs, m_frequency, normalization );  
        
        //  If the secant method makes things worse, then just go with the
        //  the most likely candidate.
        //
        //  This is a sanity measure, should never happen.
        if ( bestQ > m_confidence )
        {
            m_confidence = bestQ;
            m_frequency = bestFreq;
        }
        
//        notifier << "refined to: " << m_frequency
//                 << " Hz, Q: " << m_confidence << endl;
    
    }

}
                                        

// ---------------------------------------------------------------------------
//  --- local helpers ---
// ---------------------------------------------------------------------------


//  Collect candidate frequencies in eval_freqs.
//  Candidates are all integer divisors
//  between fmin and fmax of any frequency in the
//  vector of peak frequencies provided.

static void
compute_candidate_freqs( const vector< double > & peak_freqs,
                         double fmin, double fmax, 
                         vector< double > & eval_freqs )
{
	Assert( fmax > fmin );
    
    eval_freqs.clear();
    
    for ( vector< double >::const_iterator pk = peak_freqs.begin();
         pk != peak_freqs.end();
         ++pk )
    {    
        //  check all integer divisors of *pk
        double div = 1;
        double f = *pk;

        //  reject all the ones greater than fmax
        while( f > fmax )
        {
            ++div;
            f = *pk / div;
        }          
        
        //  keep the the ones that are between fmin 
        //  and fmax
        while( f >= fmin )
        {
            eval_freqs.push_back( f );
            ++div;
            f = *pk / div;
        }        
    }
    
    //  sort the candidats
    sort( eval_freqs.begin(), eval_freqs.end() );
}


// ---------------------------------------------------------------------------
//  --- likelihood function evaluation ---
// ---------------------------------------------------------------------------


//	Qterm is a functor to help compute terms
//	in the likelihood function sum.

struct Qterm
{
	double f0;
	Qterm( double f ) : f0(f) {}
	
	double operator()( double amp, double freq ) const
	{
		double arg = 2*Pi*freq/f0;
		return amp*amp*std::cos(arg);
	}
};

//	evaluate_Q
//
//	Evaluate the likelihood function at the specified
//	frequency.

static double
evaluate_Q( const vector<double> & amps, 
            const vector<double> & freqs, 
            double eval_freq,
            double norm )
{
    double prod = 
        std::inner_product( amps.begin(), amps.end(),
                            freqs.begin(),
                            0.,
                            std::plus< double >(),
                            Qterm( eval_freq ) );
                            
    return prod * norm;
}    

//	evaluate_Q
//
//	Evaluate the normalized likelihood function at a range of 
//	frequencies, return the results in the vector Q.

static void
evaluate_Q( const vector<double> & amps, 
            const vector<double> & freqs, 
            const vector<double> & eval_freqs, 
            vector<double> & Q )
{
	Assert( eval_freqs.size() == Q.size() );
	Assert( amps.size() == freqs.size() );
	
    //  Compute a normalization factor equal to the total
    //  energy represented by all the peaks passed in
    //  amps and freqs, so that the value of the likelihood
    //  function does not depend on the overall signal 
    //  amplitude, but instead depends only on the quality
    //  of the estimate, or the confidence in the result, 
    //  and the quality of the final estimate can be evaluated
    //  by the value of the likelihood function.
    double etotal = std::inner_product( amps.begin(), amps.end(), amps.begin(), 0.0 );
	double norm = 1.0 / etotal;
    
    evaluate_Q( amps, freqs, eval_freqs, Q, norm );
}


                                                    
//	evaluate_Q
//
//	Evaluate the normalized likelihood function at a range of 
//	frequencies, using the normalization factor provided, and
//  return the results in the vector Q.

static void
evaluate_Q( const vector<double> & amps, 
            const vector<double> & freqs, 
            const vector<double> & eval_freqs, 
            vector<double> & Q,
            double norm )
{
	Assert( eval_freqs.size() == Q.size() );
	Assert( amps.size() == freqs.size() );
    
	//	iterate over the frequencies at which to 
	//	evaluate the likelihood function:
	vector<double>::const_iterator freq_it = eval_freqs.begin();
	vector<double>::iterator Q_it = Q.begin();
	while ( freq_it != eval_freqs.end() )
	{
        double f = *freq_it;
        
		double result = evaluate_Q( amps, freqs, f, norm );
                                                              
		*Q_it++ = result;
		++freq_it;
	}
}
            
// ---------------------------------------------------------------------------
//  --- likelihood function derivative evaluation ---
// ---------------------------------------------------------------------------


//	Qprimeterm is a functor to help compute terms
//	in the likelihood function derivative sum, used
//  in the secant method of root refinement.

struct Qprimeterm
{
	double f0;
	Qprimeterm( double f ) : f0(f) {}
	
	double operator()( double amp, double freq ) const
	{
		double arg = 2*Pi*freq/f0;
		return amp*amp*std::sin(arg)*arg/f0;
	}
};


//	evaluate_Qprime
//
//	Evaluate the derivative of the likelihood function (w.r.t. frequency)
//  at the specified frequency.

static double
evaluate_Qprime( const vector<double> & amps, 
                 const vector<double> & freqs, 
                 double eval_freq )
{
    double prod = 
        std::inner_product( amps.begin(), amps.end(),
                            freqs.begin(),
                            0.,
                            std::plus< double >(),
                            Qprimeterm( eval_freq ) );

    return prod;
}                                        

// ---------------------------------------------------------------------------
//  --- secant method of refining a root/peak estimate ---
// ---------------------------------------------------------------------------

//	secant_method
//
//	Find roots of the derivative of the likelihood
// 	function using the secant method, return the 
//  value of x (frequency) at which the roots is found.

static double
secant_method( const vector<double> & amps, 
               const vector<double> & freqs, 
               double f1, double f2, double precision )
{
	double xn = f1;
	double xnm1 = f2;
	double fxnm1 = evaluate_Qprime( amps, freqs, xnm1 );
    
    const unsigned int MaxIters = 20;
    
    unsigned int iters = 0;
    double deltax = 0.0;
	
    //  Iterate until delta is small, or blows up, 
    //  or we have iterated too many times.
	do 
    {        		        
		double fxn = evaluate_Qprime( amps, freqs, xn );

		deltax = fxn * (xn - xnm1)/(fxn - fxnm1);
        
        xnm1 = xn;
        xn = xn - deltax;	
        
        fxnm1 = fxn;

	} 	while( // fabs( deltax ) > precision && 
               IsFiniteNumber( deltax )  &&
               ++iters < MaxIters );
    
    
    //  Check whether delta blew up. If it did, revert to the
    //  previous value of x.
    
    if ( ! IsFiniteNumber( deltax )  )
    {
        xn = xnm1;
    }
	
	return xn;
}

#if 0
// ---------------------------------------------------------------------------
//  --- local helpers - dumb old way ---
// ---------------------------------------------------------------------------

               
static void
compute_eval_freqs( double fmin, double fmax, 
                    vector<double> & eval_freqs );	
                    
static vector<double>::const_iterator
choose_peak( const vector<double> & Q );

// ---------------------------------------------------------------------------
//  F0Estimate constructor -- iterative method
// ---------------------------------------------------------------------------
//	Iteratively compute the value of the likelihood function
//	at a range of frequencies around the peak likelihood.
//	Store the maximum value when the range of likelihood
//	values computed is less than the specified resolution.
//  Store the frequency and the normalized value of the 
//  likelihood function at that frequency (1.0 indicates that
//  all the peaks are perfect harmonics of the estimated
//  frequency).
    
void 
F0Estimate::construct_iterative_method( const vector<double> & amps, 
                                        const vector<double> & freqs, 
                                        double fmin, double fmax,
                                        double resolution )
{

    //	when the frequency range is small, few samples are
    //	needed, but initially make sure to sample at least
    //	every 20 Hz. 
    // Scratch that, 20 Hz isn't fine enough, could miss a 
    // peak that way, try 2 Hz. There might be some room to
    // adjust this parameter to trade off speed for robustness.
	int Nsamps = std::max( 8, (int)std::ceil((fmax-fmin)*0.5) );
	vector<double> eval_freqs, Q;
	double peak_freq = fmin, peak_Q = 0;
	
	//	invariant:
	//	the likelihood function for the estimate of the fundamental
	//	frequency is maximized at some frequency between
	//	fmin and fmax (stop when that range is smaller
	//	than the resolution)
	do
	{
		//	determine the frequencies at which to evaluate
		//	the likelihood function
		eval_freqs.resize( Nsamps );
		compute_eval_freqs( fmin, fmax, eval_freqs );
		
		//	evaluate the likelihood function at those 
		//	frequencies:
		Q.resize( Nsamps );
		evaluate_Q( amps, freqs, eval_freqs, Q );
		
		//	find the highest frequency at which the likelihood
		//	function peaks:
		vector<double>::const_iterator peak = choose_peak( Q );
		int peak_idx = peak - Q.begin();
		peak_Q = *peak;
		peak_freq = eval_freqs[ peak_idx ];
		
		//	update search range:
		fmin = eval_freqs[ std::max(peak_idx - 1, 0) ];
		fmax = eval_freqs[ std::min(peak_idx + 1, Nsamps - 1) ];
		Nsamps = std::max( 8, (int)std::ceil((fmax-fmin)*0.05) );
		
	} while ( (fmax - fmin) > resolution );

    m_frequency = peak_freq;
    m_confidence = peak_Q;
}

//	compute_eval_freqs
//
//	Fill the frequency vector with a sampling
//	of the range [fmin,fmax].
//
//  (used by dumb old iterative method)
//
static void
compute_eval_freqs( double fmin, double fmax, 
					vector<double> & eval_freqs )
{
	Assert( fmax > fmin );
	
	double delta = (fmax-fmin)/(eval_freqs.size()-1);
	double f = fmin;
	vector<double>::iterator it = eval_freqs.begin();
	while( it != eval_freqs.end() )
	{
		*it++ = f;
		f += delta;
	}
	eval_freqs.back() = fmax;
}

//	choose_peak
//
//	Return the position of last peak that 
//	in the vector Q.
//
static vector<double>::const_iterator
choose_peak( const vector<double> & Q )
{
	Assert( !Q.empty() );
	
	double Qmax = *std::max_element( Q.begin(), Q.end() );
	vector<double>::const_iterator it = (Q.end()) - 1;
	double tmp = *it;
	
   // this threshold determines how strong the 
   // highest-frequency peak in the likelihood 
   // function needs to be relative to the overall
   // peak. For strongly periodic signals, this can
   // be quite near to 1, but for things that are 
   // somewhat non-harmonic, setting it too high
   // gives octave errors. Cannot tell whether errors
   // will be introduced by having it too low.
	const double threshold = 0.85 * Qmax;
	while( (it != Q.begin()) && ((*it < threshold) || (*it < *(it-1))) )
	{
		--it;
		tmp = *it;
	}
	
	return it;
}

#endif

}	//	end of namespace Loris
