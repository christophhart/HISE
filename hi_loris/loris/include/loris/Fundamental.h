#ifndef INCLUDE_FUNDAMENTAL_H
#define INCLUDE_FUNDAMENTAL_H
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
 * Fundamental.h
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

#include "LinearEnvelope.h"
#include "PartialList.h"
#include "F0Estimate.h" 

#include <memory>
#include <vector>

//  begin namespace
namespace Loris {

class ReassignedSpectrum;

// ---------------------------------------------------------------------------
//  class FundamentalEstimator
//
//! Base class for fundamental estimation, common storage for member
//! variable parameters, type definitions, and constants.

class FundamentalEstimator
{
public:

//  -- types --

    typedef F0Estimate value_type;

//  -- constants --    

    enum {
    
        DefaultAmpFloor = -60,          //! the default absolute amplitude threshold in dB
        
        DefaultAmpRange = 30,           //! the default floating amplitude threshold in dB
    
        DefaultFreqCeiling = 4000,      //! the default frequency threshold in Hz
    
        DefaultPrecisionOver100 = 10,   //! the default frequency precision in 1/100 Hz

        DefaultMinConfidencePct = 90    //! the default required percent confidence to
                                        //! return an estimate (100 is absolute confidence)
    };
    

//  -- lifecycle --

protected:

    //! Construct a new estimator with specified precision and
    //! other parameters given default values.
    //!
    //! The specified precision is used to terminate the iterative
    //! estimation procedure. 
    //!
    //! \param precisionHz is the precision in Hz with which the 
    //! fundamental estimates will be made.
    FundamentalEstimator( double precisionHz );
                                                        
public: 

    //! Destructor    
    virtual ~FundamentalEstimator( void );    


    //  compiler-generated copy and assignment are OK

  
//  -- parameter access --

    //! Return the absolute amplitude threshold in (negative) dB, 
    //! below which spectral peaks will not be considered in the 
    //! estimation of the fundamental (default is 30 dB).            
    double ampFloor( void ) const;

    //!	Return the amplitude range in dB, 
    //! relative to strongest peak in a frame, floating
    //! amplitude threshold (negative) below which spectral
    //! peaks will not be considered in the estimation of 
    //! the fundamental (default is 30 dB).	
    double ampRange( void ) const;

    //! Return the frequency ceiling in Hz, the
    //! frequency threshold above which spectral
    //! peaks will not be considered in the estimation of 
    //! the fundamental (default is 10 kHz).            
    double freqCeiling( void ) const;

    //! Return the precision of the estimate in Hz, the
    //! fundamental frequency will be estimated to 
    //! within this range (default is 0.1 Hz).
    double precision( void ) const;
     
//  -- parameter mutation --

    //! Set the absolute amplitude threshold in (negative) dB, 
    //! below which spectral peaks will not be considered in the 
    //! estimation of the fundamental (default is 30 dB).            
    //! 
    //! \param x is the new value of this parameter.            
    void setAmpFloor( double x );

    //!	Set the amplitude range in dB, 
    //! relative to strongest peak in a frame, floating
    //! amplitude threshold (negative) below which spectral
    //! peaks will not be considered in the estimation of 
    //! the fundamental (default is 30 dB).	
    //! 
    //! \param x is the new value of this parameter.            
    void setAmpRange( double x );

    //! Set the frequency ceiling in Hz, the
    //! frequency threshold above which spectral
    //! peaks will not be considered in the estimation of 
    //! the fundamental (default is 10 kHz). Must be
    //! greater than the lower bound.
    //! 
    //! \param x is the new value of this parameter.            
    void setFreqCeiling( double x );

    //! Set the precision of the estimate in Hz, the
    //! fundamental frequency will be estimated to 
    //! within this range (default is 0.1 Hz).
    //! 
    //! \param x is the new value of this parameter.            
    void setPrecision( double x );


protected:

//  -- parameter member variables --


    double m_precision;         //! in Hz, fundamental frequency will be estimated to 
                                //! within this range (default is 0.1 Hz)
                                
    double m_ampFloor;          //! absolute amplitude threshold below which spectral
                                //! peaks will not be considered in the estimation of 
                                //! the fundamental (default is equivalent to 60 dB
                                //! quieter than a full scale sinusoid)
    
    double m_ampRange;          //! floating amplitude threshold relative to the peak
                                //! having the largest magnitude below which spectral
                                //! peaks will not be considered in the estimation of 
                                //! the fundamental (default is equivalent to 30 dB)
        
    double m_freqCeiling;       //! in Hz, frequency threshold above which spectral
                                //! peaks will not be considered in the estimation of 
                                //! the fundamental (default is 10 kHz)      


};  //  end of base class FundamentalEstimator



// ---------------------------------------------------------------------------
//  class FundamentalFromSamples
//
//! Class FundamentalFromSamples represents an algorithm for 
//! time-varying fundamental frequency estimation based on
//! time-frequency reassigned spectral analysis of a sequence
//! of samples. This class is adapted from the Analyzer class 
//! (see Analyzer.h), and performs the same spectral analysis 
//! and peak extraction, but does not form Partials.
//! 
//! For more information about Reassigned Bandwidth-Enhanced 
//! Analysis and the Reassigned Bandwidth-Enhanced Additive Sound 
//! Model, refer to the Loris website: www.cerlsoundgroup.org/Loris/.
//
class FundamentalFromSamples : public FundamentalEstimator
{
//  -- public interface --

public:

//  -- lifecycle --

    //! Construct a new estimator configured with the given  
    //! analysis window width (main lobe, zero-to-zero). All other 
    //! spectrum analysis parameters are computed from the specified 
    //! window width. 
    //!
    //! The specified precision is used to terminate the iterative
    //! estimation procedure. If unspecified, the default value,
    //! DefaultPrecisionOver100 * 100 is used. 
    //! 
    //! \param  windowWidthHz is the main lobe width of the Kaiser
    //!         analysis window in Hz.
    //!
    //! \param  precisionHz is the precision in Hz with which the 
    //!         fundamental estimates will be made.    
    FundamentalFromSamples( double winWidthHz, 
                            double precisionHz = DefaultPrecisionOver100 * 0.01 );
                            
                            

    //! Destructor    
    ~FundamentalFromSamples( void );

//  -- fundamental frequency estimation --

    //  buildEnvelope
    //
    //! Construct a linear envelope from fundamental frequency 
    //! estimates taken at the specified interval in seconds
    //! starting at tbeg (seconds) and ending before tend (seconds).
    //! 
    //! \param  samps is the beginning of a sequence of samples
    //! \param  sampsEnd is the end of the sequence of samples
    //! \param  sampleRate is the sampling rate (in Hz) associated
    //!         with the sequence of samples (used to compute frequencies
    //!         in Hz, and to convert the time from seconds to samples)
    //! \param  tbeg is the beginning of the time interval (in seconds)
    //! \param  tend is the end of the time interval (in seconds)
    //! \param  interval is the time between breakpoints in the
    //!         fundamental frequency envelope (in seconds)
    //! \param  lowerFreqBound is the lower bound on the fundamental
    //!         frequency estimate (in Hz)
    //! \param  upperFreqBound is the lower bound on the fundamental
    //!         frequency estimate (in Hz)
    //! \param  confidenceThreshold is the minimum confidence level
    //!         resuired for a fundamental frequency estimate to be
    //!         added to the envelope. Lower confidence estimates are
    //!         not added, the envelope returned will not contain
    //!         breakpoints at times associated with low confidence 
    //!         estimates
    //! \return a LinearEnvelope composed of breakpoints corresponding to
    //!         the fundamental frequency estimates at samples of the span
    //!         tbeg to tend at the specified sampling interval, only estimates
    //!         having confidence level exceeding the specified confidence
    //!         threshold are added to the envelope    
    LinearEnvelope buildEnvelope( const double * sampsBeg, 
                                  const double * sampsEnd, 
                                  double sampleRate, 
                                  double tbeg, double tend, 
                                  double interval,
                                  double lowerFreqBound, double upperFreqBound, 
                                  double confidenceThreshold );
                                 
    //  buildEnvelope
    //
    //! Construct a linear envelope from fundamental frequency 
    //! estimates taken at the specified interval in seconds
    //! starting at tbeg (seconds) and ending before tend (seconds).
    //! 
    //! \param  samps is the beginning of a sequence of samples
    //! \param  nsamps is the length of the sequence of samples
    //! \param  sampleRate is the sampling rate (in Hz) associated
    //!         with the sequence of samples (used to compute frequencies
    //!         in Hz, and to convert the time from seconds to samples)
    //! \param  tbeg is the beginning of the time interval (in seconds)
    //! \param  tend is the end of the time interval (in seconds)
    //! \param  interval is the time between breakpoints in the
    //!         fundamental frequency envelope (in seconds)
    //! \param  lowerFreqBound is the lower bound on the fundamental
    //!         frequency estimate (in Hz)
    //! \param  upperFreqBound is the lower bound on the fundamental
    //!         frequency estimate (in Hz)
    //! \param  confidenceThreshold is the minimum confidence level
    //!         resuired for a fundamental frequency estimate to be
    //!         added to the envelope. Lower confidence estimates are
    //!         not added, the envelope returned will not contain
    //!         breakpoints at times associated with low confidence 
    //!         estimates
    //! \return a LinearEnvelope composed of breakpoints corresponding to
    //!         the fundamental frequency estimates at samples of the span
    //!         tbeg to tend at the specified sampling interval, only estimates
    //!         having confidence level exceeding the specified confidence
    //!         threshold are added to the envelope    
    LinearEnvelope buildEnvelope( const double * sampsBeg, 
                                  unsigned long nsamps,
                                  double sampleRate, 
                                  double tbeg, double tend, 
                                  double interval,
                                  double lowerFreqBound, double upperFreqBound, 
                                  double confidenceThreshold )
    {
        return buildEnvelope( sampsBeg, sampsBeg + nsamps, sampleRate, 
                              tbeg, tend, interval, 
                              lowerFreqBound, upperFreqBound,
                              confidenceThreshold );
    }


    //  buildEnvelope
    //
    //! Construct a linear envelope from fundamental frequency 
    //! estimates taken at the specified interval in seconds
    //! starting at tbeg (seconds) and ending before tend (seconds).
    //! 
    //! \param  samps is the sequence of samples
    //! \param  sampleRate is the sampling rate (in Hz) associated
    //!         with the sequence of samples (used to compute frequencies
    //!         in Hz, and to convert the time from seconds to samples)
    //! \param  tbeg is the beginning of the time interval (in seconds)
    //! \param  tend is the end of the time interval (in seconds)
    //! \param  interval is the time between breakpoints in the
    //!         fundamental frequency envelope (in seconds)
    //! \param  lowerFreqBound is the lower bound on the fundamental
    //!         frequency estimate (in Hz)
    //! \param  upperFreqBound is the lower bound on the fundamental
    //!         frequency estimate (in Hz)
    //! \param  confidenceThreshold is the minimum confidence level
    //!         resuired for a fundamental frequency estimate to be
    //!         added to the envelope. Lower confidence estimates are
    //!         not added, the envelope returned will not contain
    //!         breakpoints at times associated with low confidence 
    //!         estimates
    //! \return a LinearEnvelope composed of breakpoints corresponding to
    //!         the fundamental frequency estimates at samples of the span
    //!         tbeg to tend at the specified sampling interval, only estimates
    //!         having confidence level exceeding the specified confidence
    //!         threshold are added to the envelope    
    LinearEnvelope buildEnvelope( const std::vector< double > & samps, 
                                  double sampleRate, 
                                  double tbeg, double tend, 
                                  double interval,
                                  double lowerFreqBound, double upperFreqBound, 
                                  double confidenceThreshold )
    {
        return buildEnvelope( &samps[0], &samps[0] + samps.size(), sampleRate, 
                              tbeg, tend, interval, 
                              lowerFreqBound, upperFreqBound,
                              confidenceThreshold );
    }

                                 
    //  estimateAt
    //
    //! Return an estimate of the fundamental frequency computed 
    //! at the specified time. The F0Estimate returned stores the
    //! estimate of the fundamental frequency (in Hz) and the 
    //! relative confidence (from 0 to 1) associated with that
    //! estimate.
    //!
    //! \param  sampsBeg is the beginning of a sequence of samples
    //! \param  sampsEnd is the end of the sequence of samples
    //! \param  sampleRate is the sampling rate (in Hz) associated
    //!         with the sequence of samples (used to compute frequencies
    //!         in Hz, and to convert the time from seconds to samples)
    //! \param  time is the time in seconds at which to attempt to estimate
    //!         the fundamental frequency
    //! \param  lowerFreqBound is the lower bound on the fundamental
    //!         frequency estimate (in Hz)
    //! \param  upperFreqBound is the lower bound on the fundamental
    //!         frequency estimate (in Hz)
    //! \return the estimate of fundamental frequency in Hz and the 
    //!         confidence associated with that estimate (see 
    //!         F0Estimate.h)
    
    value_type estimateAt( const double * sampsBeg, 
                           const double * sampsEnd, 
                           double sampleRate, 
                           double time,
                           double lowerFreqBound, double upperFreqBound );

    //  estimateAt
    //
    //! Return an estimate of the fundamental frequency computed 
    //! at the specified time. The F0Estimate returned stores the
    //! estimate of the fundamental frequency (in Hz) and the 
    //! relative confidence (from 0 to 1) associated with that
    //! estimate.
    //!
    //! \param  samps is the beginning of a sequence of samples
    //! \param  nsamps is the length of the sequence of samples
    //! \param  sampleRate is the sampling rate (in Hz) associated
    //!         with the sequence of samples (used to compute frequencies
    //!         in Hz, and to convert the time from seconds to samples)
    //! \param  time is the time in seconds at which to attempt to estimate
    //!         the fundamental frequency
    //! \param  lowerFreqBound is the lower bound on the fundamental
    //!         frequency estimate (in Hz)
    //! \param  upperFreqBound is the lower bound on the fundamental
    //!         frequency estimate (in Hz)
    //! \return the estimate of fundamental frequency in Hz and the 
    //!         confidence associated with that estimate (see 
    //!         F0Estimate.h)
    
    value_type estimateAt( const double * sampsBeg, 
                           unsigned long nsamps,
                           double sampleRate, 
                           double time,
                           double lowerFreqBound, double upperFreqBound )
    {
        return estimateAt( sampsBeg, sampsBeg + nsamps, sampleRate, 
                           time, lowerFreqBound, upperFreqBound );
    }
                           
    //  estimateAt
    //
    //! Return an estimate of the fundamental frequency computed 
    //! at the specified time. The F0Estimate returned stores the
    //! estimate of the fundamental frequency (in Hz) and the 
    //! relative confidence (from 0 to 1) associated with that
    //! estimate.
    //!
    //! \param  samps is the sequence of samples
    //! \param  sampleRate is the sampling rate (in Hz) associated
    //!         with the sequence of samples (used to compute frequencies
    //!         in Hz, and to convert the time from seconds to samples)
    //! \param  time is the time in seconds at which to attempt to estimate
    //!         the fundamental frequency
    //! \param  lowerFreqBound is the lower bound on the fundamental
    //!         frequency estimate (in Hz)
    //! \param  upperFreqBound is the lower bound on the fundamental
    //!         frequency estimate (in Hz)
    //! \return the estimate of fundamental frequency in Hz and the 
    //!         confidence associated with that estimate (see 
    //!         F0Estimate.h)
    
    value_type estimateAt( const std::vector< double > & samps, 
                           double sampleRate, 
                           double time,
                           double lowerFreqBound, double upperFreqBound )
    {
        return estimateAt( &samps[0], &samps[0] + samps.size(), sampleRate, 
                           time, lowerFreqBound, upperFreqBound );
    }
                              


//  -- spectral analysis parameter access/mutation --

    //! Return the frequency-domain main lobe width (in Hz measured 
    //! between zero-crossings) of the analysis window used in spectral
    //! analysis.               
    double windowWidth( void ) const;
    
    //! Set the frequency-domain main lobe width (in Hz measured 
    //! between zero-crossings) of the analysis window used in spectral
    //! analysis.   
    //! 
    //! \param  w is the new main lobe width in Hz            
    void setWindowWidth( double w );         

    

//  -- private auxiliary functions --

private: 

    //  buildSpectrumAnalyzer
    //
    //! Construct the ReassignedSpectrum that will be used to perform
    //! spectral analysis from which peak frequencies and amplitudes 
    //! will be drawn. This construction is performed in a lazy fashion,
    //! and needs to be done again when certain of the parameters change.
    //!
    //! \param  srate is the sampling frequency in Hz, needed to compute
    //!         analysis window parameters    
    void buildSpectrumAnalyzer( double srate );
        

    //  collectFreqsAndAmps
    //
    //! Perform spectral analysis on a sequence of samples, using
    //! an analysis window centered at the specified time in seconds.
    //! Collect the frequencies and amplitudes of the peaks and return
    //! them in the vectors provided. 
    //!
    //! \param  samps is the beginning of a sequence of samples
    //! \param  nsamps is the length of the sequence of Partials
    //! \param  sampleRate is the sampling rate (in Hz) associated
    //!         with the sequence of samples (used to compute frequencies
    //!         in Hz, and to convert the time from seconds to samples)
    //! \param  frequencies is a vector in which to store a sequence of
    //!         frequencies to be used to estimate the most likely 
    //!         fundamental frequency
    //! \param  amplitudes is a vector in which to store a sequence of
    //!         amplitudes to be used to estimate the most likely 
    //!         fundamental frequency
    //! \param  time is the time in seconds at which to collect frequencies
    //!         and amplitudes of spectral peaks
    
    void collectFreqsAndAmps( const double * samps,
                              unsigned long nsamps,
                              double sampleRate,
                              std::vector< double > & frequencies, 
                              std::vector< double > & amplitudes,
                              double time );


//  -- private member variables --

    std::auto_ptr< ReassignedSpectrum > m_spectrum;
                                //! the spectrum analyzer 
                                
    double m_cacheSampleRate;   //! the sample rate used to construct the 

    double m_windowWidth;       //! the width of the main lobe of the window to 
                                //! be used in spectral analysis, in Hz
    
//  disallow these until they are implemented

    FundamentalFromSamples( const FundamentalFromSamples & );
    FundamentalFromSamples & operator= ( const FundamentalFromSamples & );

};   //  end of class FundamentalFromSamples



// ---------------------------------------------------------------------------
//  class FundamentalFromPartials
//
//! Class FundamentalFromPartials represents an algorithm for 
//! time-varying fundamental frequency estimation from instantaneous
//! Partial amplitudes and frequencies based on a likelihood
//!	estimator adapted from Quatieri's Speech Signal Processing text

class FundamentalFromPartials : public FundamentalEstimator
{
//  -- public interface --

public:

//  -- lifecycle --

    //! Construct a new estimator.
    //!
    //! The specified precision is used to terminate the iterative
    //! estimation procedure. If unspecified, the default value,
    //! DefaultPrecisionOver100 * 100 is used. 
    //!
    //! \param precisionHz is the precision in Hz with which the 
    //! fundamental estimates will be made.    
    FundamentalFromPartials( double precisionHz = DefaultPrecisionOver100 * 0.01 );
                                                        

    //! Destructor    
    ~FundamentalFromPartials( void );
    
    //! Construct a copy of an estimator. Nothing much to do since this class
    //! has no data members.
    FundamentalFromPartials( const FundamentalFromPartials & );
    
    //! Pass the assignment opertion up to the base class.
    FundamentalFromPartials & operator= ( const FundamentalFromPartials & );

//  -- fundamental frequency estimation --

    //  buildEnvelope
    //
    //! Construct a linear envelope from fundamental frequency 
    //! estimates taken at the specified interval in seconds 
    //! starting at tbeg (seconds) and ending before tend (seconds).
    //!
    //! \param  begin_partials is the beginning of a sequence of Partials
    //! \param  end_partials is the end of a sequence of Partials
    //! \param  tbeg is the beginning of the time interval (in seconds)
    //! \param  tend is the end of the time interval (in seconds)
    //! \param  interval is the time between breakpoints in the
    //!         fundamental frequency envelope (in seconds)
    //! \param  lowerFreqBound is the lower bound on the fundamental
    //!         frequency estimate (in Hz)
    //! \param  upperFreqBound is the lower bound on the fundamental
    //!         frequency estimate (in Hz)
    //! \param  confidenceThreshold is the minimum confidence level
    //!         resuired for a fundamental frequency estimate to be
    //!         added to the envelope. Lower confidence estimates are
    //!         not added, the envelope returned will not contain
    //!         breakpoints at times associated with low confidence 
    //!         estimates
    //! \return a LinearEnvelope composed of breakpoints corresponding to
    //!         the fundamental frequency estimates at samples of the span
    //!         tbeg to tend at the specified sampling interval, only estimates
    //!         having confidence level exceeding the specified confidence
    //!         threshold are added to the envelope
    LinearEnvelope buildEnvelope( PartialList::const_iterator begin_partials, 
                                  PartialList::const_iterator end_partials,
                                  double tbeg, double tend, 
                                  double interval,
                                  double lowerFreqBound, double upperFreqBound, 
                                  double confidenceThreshold );
                                  
    //  buildEnvelope
    //
    //! Construct a linear envelope from fundamental frequency 
    //! estimates taken at the specified interval in seconds 
    //! starting at tbeg (seconds) and ending before tend (seconds).
    //!
    //! \param  partials is the sequence of Partials
    //! \param  tbeg is the beginning of the time interval (in seconds)
    //! \param  tend is the end of the time interval (in seconds)
    //! \param  interval is the time between breakpoints in the
    //!         fundamental frequency envelope (in seconds)
    //! \param  lowerFreqBound is the lower bound on the fundamental
    //!         frequency estimate (in Hz)
    //! \param  upperFreqBound is the lower bound on the fundamental
    //!         frequency estimate (in Hz)
    //! \param  confidenceThreshold is the minimum confidence level
    //!         resuired for a fundamental frequency estimate to be
    //!         added to the envelope. Lower confidence estimates are
    //!         not added, the envelope returned will not contain
    //!         breakpoints at times associated with low confidence 
    //!         estimates
    //! \return a LinearEnvelope composed of breakpoints corresponding to
    //!         the fundamental frequency estimates at samples of the span
    //!         tbeg to tend at the specified sampling interval, only estimates
    //!         having confidence level exceeding the specified confidence
    //!         threshold are added to the envelope                                        
    LinearEnvelope buildEnvelope( const PartialList & partials, 
                                  double tbeg, double tend, 
                                  double interval,
                                  double lowerFreqBound, double upperFreqBound, 
                                  double confidenceThreshold )
    {
        return buildEnvelope( partials.begin(), partials.end(),
                              tbeg, tend, interval,
                              lowerFreqBound, upperFreqBound,
                              confidenceThreshold );
    }
    
                                 
    //  estimateAt
    //
    //! Return an estimate of the fundamental frequency computed 
    //! at the specified time. The F0Estimate returned stores the
    //! estimate of the fundamental frequency (in Hz) and the 
    //! relative confidence (from 0 to 1) associated with that
    //! estimate.
    //!
    //! \param  begin_partials is the beginning of a sequence of Partials
    //! \param  end_partials is the end of a sequence of Partials
    //! \param  time is the time in seconds at which to attempt to estimate
    //!         the fundamental frequency
    //! \param  lowerFreqBound is the lower bound on the fundamental
    //!         frequency estimate (in Hz)
    //! \param  upperFreqBound is the lower bound on the fundamental
    //!         frequency estimate (in Hz)
    //! \return the estimate of fundamental frequency in Hz and the 
    //!         confidence associated with that estimate (see 
    //!         F0Estimate.h)
    value_type estimateAt( PartialList::const_iterator begin_partials, 
                           PartialList::const_iterator end_partials,
                           double time,
                           double lowerFreqBound, double upperFreqBound );
                           
    //  estimateAt
    //
    //! Return an estimate of the fundamental frequency computed 
    //! at the specified time. The F0Estimate returned stores the
    //! estimate of the fundamental frequency (in Hz) and the 
    //! relative confidence (from 0 to 1) associated with that
    //! estimate.
    //!
    //! \param  partials is the sequence of Partials
    //! \param  time is the time in seconds at which to attempt to estimate
    //!         the fundamental frequency
    //! \param  lowerFreqBound is the lower bound on the fundamental
    //!         frequency estimate (in Hz)
    //! \param  upperFreqBound is the lower bound on the fundamental
    //!         frequency estimate (in Hz)
    //! \return the estimate of fundamental frequency in Hz and the 
    //!         confidence associated with that estimate (see 
    //!         F0Estimate.h)
    value_type estimateAt( const PartialList & partials, 
                           double time,
                           double lowerFreqBound, double upperFreqBound )
    {
            return estimateAt( partials.begin(), partials.end(),
                               time,
                               lowerFreqBound, upperFreqBound );
    }



//  -- private auxiliary functions --

private: 

    //  collectFreqsAndAmps
    //
    //! Collect the frequencies and amplitudes of a range of partials 
    //! at the specified time and return them in the vectors provided. 
    //!
    //! \param  begin_partials is the beginning of a sequence of Partials
    //! \param  end_partials is the end of a sequence of Partials
    //! \param  frequencies is a vector in which to store a sequence of
    //!         frequencies to be used to estimate the most likely 
    //!         fundamental frequency
    //! \param  amplitudes is a vector in which to store a sequence of
    //!         amplitudes to be used to estimate the most likely 
    //!         fundamental frequency
    //! \param  time is the time in seconds at which to collect frequencies
    //!         and amplitudes of the Partials
    void collectFreqsAndAmps( PartialList::const_iterator begin_partials, 
                              PartialList::const_iterator end_partials,
                              std::vector< double > & frequencies, 
                              std::vector< double > & amplitudes,
                              double time );



};   //  end of class FundamentalFromPartials



}   //  end of namespace Loris

#endif  // ndef INCLUDE_FUNDAMENTAL_H
