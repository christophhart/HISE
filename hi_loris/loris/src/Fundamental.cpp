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
 * Fundamental.C
 *
 * Definition of classes for computing an estimate of time-varying
 * fundamental frequency from either a sequence of samples or a
 * collection of Partials using a frequency domain maximum likelihood 
 * algorithm adapted from Quatieri's speech signal processing textbook. 
 *
 * Kelly Fitz, 25 March 2008
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */

#if HAVE_CONFIG_H
	#include "config.h"
#endif

#include "Fundamental.h"

#include "LorisExceptions.h"
#include "KaiserWindow.h"
#include "LinearEnvelope.h"
#include "Notifier.h"
#include "PartialUtils.h"
#include "ReassignedSpectrum.h"
#include "SpectralPeakSelector.h"

#include "F0Estimate.h" 

#include <algorithm>
#include <cmath>
#include <vector>

using namespace std;

#if defined(HAVE_M_PI) && (HAVE_M_PI)
	const double Pi = M_PI;
#else
	const double Pi = 3.14159265358979324;
#endif

//	begin namespace
namespace Loris {



#define VERIFY_ARG(func, test)											\
	do {																\
		if (!(test)) 													\
			Throw( Loris::InvalidArgument, #func ": " #test  );			\
	} while (false)


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
//  FundamentalEstimator members
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------


//  -- lifecycle --

// ---------------------------------------------------------------------------
//  constructor (protected)
// ---------------------------------------------------------------------------
//! Construct a new estimator with specified precision and
//! other parameters given default values.
//!
//! The specified precision is used to terminate the iterative
//! estimation procedure. 
//!
//! \param precisionHz is the precision in Hz with which the 
//! fundamental estimates will be made.

FundamentalEstimator::FundamentalEstimator( double precisionHz ) :
    m_precision( precisionHz ),
    m_ampFloor( DefaultAmpFloor ),
    m_ampRange( DefaultAmpRange ),
    m_freqCeiling( DefaultFreqCeiling )
{
	VERIFY_ARG( FundamentalEstimator, precisionHz > 0 );
}

// ---------------------------------------------------------------------------
//  destructor
// ---------------------------------------------------------------------------
    
FundamentalEstimator::~FundamentalEstimator( void )
{
}

//  -- spectral analysis parameter access --

// ---------------------------------------------------------------------------
//	ampFloor
// ---------------------------------------------------------------------------
//! Return the absolute amplitude threshold in (negative) dB, 
//! below which spectral peaks will not be considered in the 
//! estimation of the fundamental (default is 30 dB).            
double 
FundamentalEstimator::ampFloor( void ) const
{ 
    return m_ampFloor; 
}

// ---------------------------------------------------------------------------
//	ampRange
// ---------------------------------------------------------------------------
//!	Return the amplitude range in dB, 
//! relative to strongest peak in a frame, floating
//! amplitude threshold below which spectral
//! peaks will not be considered in the estimation of 
//! the fundamental (default is 30 dB).			
//
double 
FundamentalEstimator::ampRange( void ) const 
{ 
    return m_ampRange; 
}

// ---------------------------------------------------------------------------
//	freqCeiling
// ---------------------------------------------------------------------------
//!	Return the frequency ceiling in Hz, the
//! frequency threshold above which spectral
//! peaks will not be considered in the estimation of 
//! the fundamental (default is 10 kHz).			
//
double 
FundamentalEstimator::freqCeiling( void ) const 
{ 
    return m_freqCeiling; 
}

// ---------------------------------------------------------------------------
//	precision
// ---------------------------------------------------------------------------
//!	Return the precision of the estimate in Hz, the
//! fundamental frequency will be estimated to 
//! within this range (default is 0.1 Hz).
//
double 
FundamentalEstimator::precision( void ) const 
{ 
    return m_precision; 
}

                       
//  -- spectral analysis parameter mutation --

// ---------------------------------------------------------------------------
//	setAmpFloor
// ---------------------------------------------------------------------------
//! Set the absolute amplitude threshold in (negative) dB, 
//! below which spectral peaks will not be considered in the 
//! estimation of the fundamental (default is 30 dB).            
//! 
//! \param x is the new value of this parameter.            
void 
FundamentalEstimator::setAmpFloor( double x )
{ 
	VERIFY_ARG( setAmpFloor, x < 0 );
    m_ampFloor = x; 
}

// ---------------------------------------------------------------------------
//	setAmpRange
// ---------------------------------------------------------------------------
//!	Set the amplitude range in dB, 
//! relative to strongest peak in a frame, floating
//! amplitude threshold (negative) below which spectral
//! peaks will not be considered in the estimation of 
//! the fundamental (default is 30 dB).	
//!	
//!	\param x is the new value of this parameter. 				
//
void 
FundamentalEstimator::setAmpRange( double x ) 
{ 
	VERIFY_ARG( setAmpRange, x > 0 );
    m_ampRange = x; 
}

// ---------------------------------------------------------------------------
//	setFreqCeiling
// ---------------------------------------------------------------------------
//!	Set the frequency ceiling in Hz, the
//! frequency threshold above which spectral
//! peaks will not be considered in the estimation of 
//! the fundamental (default is 10 kHz). Must be
//! greater than the lower bound.
//!	
//!	\param x is the new value of this parameter. 				
//
void 
FundamentalEstimator::setFreqCeiling( double x ) 
{ 
    m_freqCeiling = x; 
}

// ---------------------------------------------------------------------------
//	setPrecision
// ---------------------------------------------------------------------------
//!	Set the precision of the estimate in Hz, the
//! fundamental frequency will be estimated to 
//! within this range (default is 0.1 Hz).
//!	
//!	\param x is the new value of this parameter. 				
//
void 
FundamentalEstimator::setPrecision( double x ) 
{ 
	VERIFY_ARG( setPrecision, x > 0 );
    m_precision = x; 
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
//  FundamentalFromSamples members
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------

//  -- lifecycle --

// ---------------------------------------------------------------------------
//  constructor
// ---------------------------------------------------------------------------
//! Construct a new estimator configured with the given  
//! analysis window width (main lobe, zero-to-zero). All other 
//! spectrum analysis parameters are computed from the specified 
//! window width. 
//!
//! The specified precision is used to terminate the iterative
//! estimation procedure. If unspecified, the default value,
//! DefaultPrecisionOver100 * 100 is used. 
//! 
//! \param windowWidthHz is the main lobe width of the Kaiser
//! analysis window in Hz.
//!
//! \param precisionHz is the precision in Hz with which the 
//! fundamental estimates will be made.

FundamentalFromSamples::FundamentalFromSamples( double winWidthHz, 
                                                double precisionHz ) :
    m_cacheSampleRate( 0 ),
    m_windowWidth( winWidthHz ), 
    FundamentalEstimator( precisionHz )
{
	VERIFY_ARG( FundamentalFromSamples, winWidthHz > 0 );
}

// ---------------------------------------------------------------------------
//  destructor
// ---------------------------------------------------------------------------
    
FundamentalFromSamples::~FundamentalFromSamples( void )
{
}

//  -- fundamental frequency estimation --



// ---------------------------------------------------------------------------
//  buildEnvelope
// ---------------------------------------------------------------------------
//! Construct a linear envelope from fundamental frequency 
//! estimates taken at the specified interval in seconds.
//! 

LinearEnvelope 
FundamentalFromSamples::buildEnvelope( const double * sampsBeg, 
                                       const double * sampsEnd, 
                                       double sampleRate, 
                                       double tbeg, double tend, 
                                       double interval,
                                       double lowerFreqBound, double upperFreqBound, 
                                       double confidenceThreshold )
{
    //  sanity check
    if ( tbeg > tend )
    {
        std::swap( tbeg, tend );
    }

    LinearEnvelope env;
    
    std::vector< double > amplitudes, frequencies;

    double time = tbeg;
    while ( time < tend )
    {
        collectFreqsAndAmps( sampsBeg, sampsEnd-sampsBeg, sampleRate,
                             frequencies, amplitudes, time );
        if ( ! amplitudes.empty() )
        {
            F0Estimate est( amplitudes, frequencies, lowerFreqBound, upperFreqBound, 
                            m_precision );

            if ( est.confidence() >= confidenceThreshold )
            {   
                env.insert( time, est.frequency() );
            }
        }
        
        time += interval;
    }
    
    return env;            
}                                       
                             
                             
// ---------------------------------------------------------------------------
//  estimateAt
// ---------------------------------------------------------------------------
//! Return an estimate of the fundamental frequency computed 
//! at the specified time. 

FundamentalFromSamples::value_type 
FundamentalFromSamples::estimateAt( const double * sampsBeg, 
                                    const double * sampsEnd, 
                                    double sampleRate, 
                                    double time,
                                    double lowerFreqBound, double upperFreqBound )
{
    std::vector< double > amplitudes, frequencies;
    
    collectFreqsAndAmps( sampsBeg, sampsEnd-sampsBeg, sampleRate,
                         frequencies, amplitudes, time );
                         
    F0Estimate est( amplitudes, frequencies, lowerFreqBound, upperFreqBound, m_precision );

    return est;
}                                    

//  -- spectral analysis parameter access/mutation --

// ---------------------------------------------------------------------------
//  windowWidth
// ---------------------------------------------------------------------------
//! Return the frequency-domain main lobe width (in Hz) (measured between 
//! zero-crossings) of the analysis window used in spectral
//! analysis.               
double FundamentalFromSamples::windowWidth( void ) const
{
    return m_windowWidth;
}

// ---------------------------------------------------------------------------
//  setWindowWidth
// ---------------------------------------------------------------------------
//! Set the frequency-domain main lobe width (in Hz) (measured between 
//! zero-crossings) of the analysis window used in spectral
//! analysis.   
//! 
//! \param x is the new value of this parameter.            
void FundamentalFromSamples::setWindowWidth( double x )
{
	VERIFY_ARG( setWindowWidth, x > 0 );
    m_windowWidth = x; 
}



//  -- private auxiliary functions --

// ---------------------------------------------------------------------------
//  buildSpectrumAnalyzer
// ---------------------------------------------------------------------------
//! Construct the ReassignedSpectrum that will be used to perform
//! spectral analysis from which peak frequencies and amplitudes 
//! will be drawn. This construction is performed in a lazy fashion,
//! and needs to be done again when certain of the parameters change.
//!
//! The spectrum analyzer cannot be constructed without knowledge of
//! the sample rate, specified in Hz, which is needed to determine the
//! parameters of the analysis window. (The sample rate is cached in
//! this class in order that it be possible to determine whether the
//! spectrum analyzer can be reused from one estimate to another.)
//
void 
FundamentalFromSamples::buildSpectrumAnalyzer( double srate )
{
 	//	configure the reassigned spectral analyzer, 
    //	always use odd-length windows:
    const double sidelobeLevel = - m_ampFloor; // amp floor is negative
    double winshape = KaiserWindow::computeShape( sidelobeLevel );
    long winlen = KaiserWindow::computeLength( m_windowWidth / srate, winshape );    
    if ( 1 != (winlen % 2) ) 
    {
        ++winlen;
    }
    
    std::vector< double > window( winlen );
    KaiserWindow::buildWindow( window, winshape );
    
    std::vector< double > windowDeriv( winlen );
    KaiserWindow::buildTimeDerivativeWindow( windowDeriv, winshape );
   
    m_spectrum.reset( new ReassignedSpectrum( window, windowDeriv ) );    
    
    //  remember the sample rate used to build this spectrum
    //  analyzer:
    m_cacheSampleRate = srate;
}

// ---------------------------------------------------------------------------
//	sort_peaks_greater_amplitude
// ---------------------------------------------------------------------------
//	predicate used for sorting peaks in order of decreasing amplitude:
static bool sort_peaks_greater_amplitude( const SpectralPeak & lhs, 
										  const SpectralPeak & rhs )
{ 
	return lhs.amplitude() > rhs.amplitude(); 
}

// ---------------------------------------------------------------------------
//  collectFreqsAndAmps
// ---------------------------------------------------------------------------
//! Perform spectral analysis on a sequence of samples, using
//! an analysis window centered at the specified time in seconds.
//! Collect the frequencies and amplitudes of the peaks and return
//! them in the vectors provided. 
//
   
void 
FundamentalFromSamples::collectFreqsAndAmps( const double * samps,
                                             unsigned long nsamps,
                                             double sampleRate,
                                             std::vector< double > & frequencies, 
                                             std::vector< double > & amplitudes,
                                             double time )
{
    amplitudes.clear();
    frequencies.clear();

    //  build the spectrum analyzer if necessary:
    if ( m_cacheSampleRate != sampleRate ||
         0 == m_spectrum.get() )
    {
        buildSpectrumAnalyzer( sampleRate );
    }
    
    
    //	configure the peak selection and partial formation policies:    
    unsigned long winlen = m_spectrum->window().size();
    const double maxTimeCorrection = 0.25 * winlen / sampleRate;   //  one-quarter the window width
    SpectralPeakSelector selector( sampleRate, maxTimeCorrection );  
 	
    
     
    //	compute reassigned spectrum:
    //  sampsBegin is the position of the first sample to be transformed,
    //	sampsEnd is the position after the last sample to be transformed.
    //	(these computations work for odd length windows only)
    unsigned long winMiddle = (unsigned long)( sampleRate * time );
    unsigned long sampsBegin = std::max( long(winMiddle) - long(winlen / 2), 0L );
    unsigned long sampsEnd = std::min( winMiddle + (winlen / 2) + 1, nsamps );
    
    if ( winMiddle < sampsEnd )
    {
        m_spectrum->transform( samps + sampsBegin, samps + winMiddle, samps + sampsEnd );
                     
        //	extract peaks from the spectrum, no fading:
        Peaks peaks = selector.selectPeaks( *m_spectrum ); 
        
        if ( ! peaks.empty() )
        {
            //  sort the peaks in order of decreasing amplitude
            //
            //  (HEY is there any reason to do this, other than to find the largest?)
            //std::sort( peaks.begin(), peaks.end(), sort_peaks_greater_amplitude );
            Peaks::iterator maxpos = std::max_element( peaks.begin(), peaks.end(), sort_peaks_greater_amplitude );
            
            //  determine the floating amplitude threshold
            const double thresh = 
                std::max( std::pow( 10.0, - 0.05 * - m_ampFloor ), 
                          std::pow( 10.0, - 0.05 * m_ampRange ) * maxpos->amplitude() );
                        
            //  collect amplitudes and frequencies and try to 
            //  estimate the fundamental
            for ( Peaks::const_iterator spkpos = peaks.begin(); spkpos != peaks.end(); ++spkpos )
            {
                if ( spkpos->amplitude() > thresh &&
                     spkpos->frequency() < m_freqCeiling )
                {
                    amplitudes.push_back( spkpos->amplitude() );
                    frequencies.push_back( spkpos->frequency() );
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
//  FundamentalFromPartials members
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
//  constructor
// ---------------------------------------------------------------------------
//! Construct a new estimator configured with the given precision.
//! The specified precision is used to terminate the iterative
//! estimation procedure. If unspecified, the default value,
//! DefaultPrecisionOver100 * 100 is used. 
//!
//! \param precisionHz is the precision in Hz with which the 
//! fundamental estimates will be made.

FundamentalFromPartials::FundamentalFromPartials( double precisionHz ) :
    FundamentalEstimator( precisionHz )
{
}

// ---------------------------------------------------------------------------
//  copy constructor
// ---------------------------------------------------------------------------
//! Construct a copy of an estimator. Nothing much to do since this class
//! has no data members.
//

FundamentalFromPartials::FundamentalFromPartials( const FundamentalFromPartials & rhs ) :
    FundamentalEstimator( rhs )
{
}

// ---------------------------------------------------------------------------
//  destructor
// ---------------------------------------------------------------------------
    
FundamentalFromPartials::~FundamentalFromPartials( void )
{
}

// ---------------------------------------------------------------------------
//  assignment
// ---------------------------------------------------------------------------
//! Pass the assignment opertion up to the base class.
//

FundamentalFromPartials &
FundamentalFromPartials::operator=( const FundamentalFromPartials & rhs )
{
    FundamentalEstimator::operator=( rhs );
    
    return *this;
}

//  -- fundamental frequency estimation --

// ---------------------------------------------------------------------------
//  buildEnvelope
// ---------------------------------------------------------------------------
//! Construct a linear envelope from fundamental frequency 
//! estimates taken at the specified interval in seconds.
//! 

LinearEnvelope 
FundamentalFromPartials::buildEnvelope( PartialList::const_iterator begin_partials, 
                                        PartialList::const_iterator end_partials,
                                        double tbeg, double tend, 
                                        double interval,
                                        double lowerFreqBound, double upperFreqBound, 
                                        double confidenceThreshold )
{
    //  sanity check
    if ( tbeg > tend )
    {
        std::swap( tbeg, tend );
    }

    LinearEnvelope env;
    
    std::vector< double > amplitudes, frequencies;

	

    double time = tbeg;
    while ( time < tend )
    {
		if (!controller->setProgress((time - tbeg) / (tend - tbeg)))
			return env;

        collectFreqsAndAmps( begin_partials, end_partials, frequencies, amplitudes, time );
                  
        if (! amplitudes.empty() )
        {
            F0Estimate est( amplitudes, frequencies, lowerFreqBound, upperFreqBound, 
                            m_precision );
        
            if ( est.confidence() >= confidenceThreshold )
            {   
                env.insert( time, est.frequency() );
            }
        }
        
        time += interval;
    }
    
    return env;            
}                         

// ---------------------------------------------------------------------------
//  estimateAt
// ---------------------------------------------------------------------------
//! Return an estimate of the fundamental frequency computed 
//! at the specified time. 

FundamentalFromPartials::value_type 
FundamentalFromPartials::estimateAt( PartialList::const_iterator begin_partials, 
                                     PartialList::const_iterator end_partials,
                                     double time,
                                     double lowerFreqBound, double upperFreqBound )
{
    std::vector< double > amplitudes, frequencies;
    
    collectFreqsAndAmps( begin_partials, end_partials, frequencies, amplitudes, time );
                         
    F0Estimate est( amplitudes, frequencies, lowerFreqBound, upperFreqBound, m_precision );

    return est;
}  


//  -- private auxiliary functions --

// ---------------------------------------------------------------------------
//  collectFreqsAndAmps
// ---------------------------------------------------------------------------
//! Perform spectral analysis on a sequence of samples, using
//! an analysis window centered at the specified time in seconds.
//! Collect the frequencies and amplitudes of the peaks and return
//! them in the vectors provided. 
//
   
void 
FundamentalFromPartials::collectFreqsAndAmps( PartialList::const_iterator begin_partials, 
                                              PartialList::const_iterator end_partials,
                                              std::vector< double > & frequencies, 
                                              std::vector< double > & amplitudes,
                                              double time )
{
    amplitudes.clear();
    frequencies.clear();
    
    if ( begin_partials != end_partials )
    {            
        //  determine the absolute amplitude threshold 
        double thresh = std::pow( 10.0, - 0.05 * - m_ampFloor );
        
        double max_amp = 0;        
        for ( PartialList::const_iterator it = begin_partials; it != end_partials; ++it )
        {
            //  compute the sinusoidal amplitude (without bandwidth energy)
            double sine_amp = std::sqrt(1 - it->bandwidthAt( time )) * it->amplitudeAt( time );        
            double freq = it->frequencyAt( time );
            
            if ( sine_amp > thresh &&
                 freq < m_freqCeiling )
            {
                amplitudes.push_back( sine_amp );
                frequencies.push_back( freq );
            }
            
            max_amp = std::max( sine_amp, max_amp );                        
        }
        
        //  remove quietest ones - this isn't very efficient, 
        //  but it is much faster than making two passes (and 
        //  computing two sequences of sinusoidal amplitudes).
        thresh = std::pow( 10.0, - 0.05 * m_ampRange ) * max_amp;
        vector< double >::size_type N = amplitudes.size();
        vector< double >::size_type k = 0;
        while ( k < N )
        {
            if ( amplitudes[k] < thresh )
            {
                amplitudes.erase( amplitudes.begin() + k );
                frequencies.erase( frequencies.begin() + k );
                --N;
            }
            else
            {
                ++k;
            }
        }
    }
    
}

}   //  end of namespace Loris
