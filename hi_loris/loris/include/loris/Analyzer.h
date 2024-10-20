#ifndef INCLUDE_ANALYZER_H
#define INCLUDE_ANALYZER_H
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
 * Analyzer.h
 *
 * Definition of class Loris::Analyzer.
 *
 * Kelly Fitz, 5 Dec 99
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */
#include <memory>
#include <vector>
#include "LinearEnvelope.h"
#include "Partial.h"
#include "PartialList.h"
// #include "SpectralPeaks.h"

//  begin namespace
namespace Loris {

class Envelope;
class LinearEnvelopeBuilder;
// class Peaks;
// class Peaks::iterator;
//  oooo, this is nasty, need to fix it!
class SpectralPeak;
typedef std::vector< SpectralPeak > Peaks;

// ---------------------------------------------------------------------------
//  class Analyzer
//
//! Class Analyzer represents a configuration of parameters for
//! performing Reassigned Bandwidth-Enhanced Additive Analysis
//! of sampled sounds. The analysis process yields a collection 
//! of Partials, each having a trio of synchronous, non-uniformly-
//! sampled breakpoint envelopes representing the time-varying 
//! frequency, amplitude, and noisiness of a single bandwidth-
//! enhanced sinusoid. These Partials are accumulated in the
//! Analyzer.
//!
//! The core analysis parameter is the frequency resolution, the 
//! minimum instantaneous frequency spacing between partials. Most 
//! other parameters are initially configured according to this 
//! parameter (and the analysis window width, if specified).
//! Subsequent parameter mutations are independent.
//! 
//! Bandwidth enhancement:
//! Two different strategies are available for computing bandwidth
//! (or noisiness) envelope: 
//! 
//! One strategy is to construct bandwidth envelopes during analysis
//! by associating residual energy in the spectrum (after peak 
//! extraction) with the selected spectral peaks that are used 
//! to construct Partials. This is the original bandwidth enhancement
//! algorithm, and bandwidth envelopes constructed in this way may
//! be suitable for use in bandwidth-enhanced synthesis. 
//! 
//! Another stategy is to construct bandwidth envelopes during 
//! analysis by storing the mixed derivative of short-time phase, 
//! scaled and shifted so that a value of 0 corresponds
//! to a pure sinusoid, and a value of 1 corresponds to a
//! bandwidth-enhanced sinusoid with maximal energy spread
//! (minimum convergence in frequency). These bandwidth envelopes
//! are not suitable for bandwidth-enhanced synthesis, be sure
//! to set the bandwidth to 0, or to disable bandwidth enhancement
//! before rendering.
//! 
//! The Analyzer may be configured to use either of these two
//! strategies for bandwidth-enhanced analysis, or to construct
//! no bandwidth envelopes at all. If unspecified, the default
//! Analyzer configuration uses spectral residue to construct
//! bandwidth envelopes.
//! 
//! \sa storeResidueBandwidth, storeConvergenceBandwidth, storeNoBandwidth
//! 
//! For more information about Reassigned Bandwidth-Enhanced 
//! Analysis and the Reassigned Bandwidth-Enhanced Additive Sound 
//! Model, refer to the Loris website: www.cerlsoundgroup.org/Loris/.
//
class Analyzer
{
//  -- public interface --
public:

//  -- construction --

    //! Construct a new Analyzer configured with the given  
    //! frequency resolution (minimum instantaneous frequency   
    //! difference between Partials). All other Analyzer parameters     
    //! are computed from the specified frequency resolution.   
    //! 
    //! \param resolutionHz is the frequency resolution in Hz.
    explicit Analyzer( double resolutionHz );
    
    //! Construct a new Analyzer configured with the given  
    //! frequency resolution (minimum instantaneous frequency   
    //! difference between Partials) and analysis window width
    //! (main lobe, zero-to-zero). All other Analyzer parameters    
    //! are computed from the specified resolution and window width.    
    //! 
    //! \param resolutionHz is the frequency resolution in Hz.
    //! \param windowWidthHz is the main lobe width of the Kaiser
    //! analysis window in Hz.
    Analyzer( double resolutionHz, double windowWidthHz );

    //! Construct a new Analyzer configured with the given time-varying
    //! frequency resolution (minimum instantaneous frequency   
    //! difference between Partials) and analysis window width
    //! (main lobe, zero-to-zero). All other Analyzer parameters    
    //! are computed from the specified resolution and window width.    
    //! 
    //! \param resolutionHz is the frequency resolution in Hz.
    //! \param windowWidthHz is the main lobe width of the Kaiser
    //! analysis window in Hz.
    Analyzer( const Envelope & resolutionEnv, double windowWidthHz );

    //! Construct  a new Analyzer having identical
    //! parameter configuration to another Analyzer. 
    //! The list of collected Partials is not copied.       
    //! 
    //! \param other is the Analyzer to copy.   
    Analyzer( const Analyzer & other );

    //! Destroy this Analyzer.
    ~Analyzer( void );

    //! Construct  a new Analyzer having identical
    //! parameter configuration to another Analyzer. 
    //! The list of collected Partials is not copied.       
    //! 
    //! \param rhs is the Analyzer to copy. 
    Analyzer & operator=( const Analyzer & rhs );

//  -- configuration --

    //! Configure this Analyzer with the given frequency resolution 
    //! (minimum instantaneous frequency difference between Partials, 
    //! in Hz). All other Analyzer parameters are (re-)computed from the 
    //! frequency resolution, including the window width, which is
    //! twice the resolution.      
    //! 
    //! \param resolutionHz is the frequency resolution in Hz.
    void configure( double resolutionHz );

    //! Configure this Analyzer with the given frequency resolution 
    //! (minimum instantaneous frequency difference between Partials)
    //! and analysis window width (main lobe, zero-to-zero, in Hz). 
    //! All other Analyzer parameters are (re-)computed from the 
    //! frequency resolution and window width.      
    //! 
    //! \param resolutionHz is the frequency resolution in Hz.
    //! \param windowWidthHz is the main lobe width of the Kaiser
    //! analysis window in Hz.
    //!     
    //! There are three categories of analysis parameters:
    //! - the resolution, and params that are usually related to (or
    //! identical to) the resolution (frequency floor and drift)
    //! - the window width and params that are usually related to (or
    //! identical to) the window width (hop and crop times)
    //! - independent parameters (bw region width and amp floor)
    void configure( double resolutionHz, double windowWidthHz );

	//! Configure this Analyzer with the given time-varying frequency resolution 
	//! (minimum instantaneous frequency difference between Partials)
	//! and analysis window width (main lobe, zero-to-zero, in Hz). 
	//! All other Analyzer parameters are (re-)computed from the 
	//! frequency resolution and window width.      
	//! 
	//! \param resolutionEnv is the time-varying frequency resolution 
	//!	in Hz.
	//! \param windowWidthHz is the main lobe width of the Kaiser
	//! analysis window in Hz.
	//!     
	//! There are three categories of analysis parameters:
	//! - the resolution, and params that are usually related to (or
	//! identical to) the resolution (frequency floor and drift)
	//! - the window width and params that are usually related to (or
	//! identical to) the window width (hop and crop times)
	//! - independent parameters (bw region width and amp floor)
	//
	void configure( const Envelope & resolutionEnv, double windowWidthHz );    
    
//  -- analysis --

    //! Analyze a vector of (mono) samples at the given sample rate         
    //! (in Hz) and store the extracted Partials in the Analyzer's 
    //! PartialList (std::list of Partials).    
    //! 
    //! \param  vec is a vector of floating point samples
    //! \param  srate is the sample rate of the samples in the vector 
    void analyze( const std::vector<double> & vec, double srate );
    
    //! Analyze a range of (mono) samples at the given sample rate      
    //! (in Hz) and store the extracted Partials in the Analyzer's
    //! PartialList (std::list of Partials).    
    //! 
    //! \param  bufBegin is a pointer to a buffer of floating point samples
    //! \param  bufEnd is (one-past) the end of a buffer of floating point 
    //!         samples
    //! \param  srate is the sample rate of the samples in the buffer
    void analyze( const double * bufBegin, const double * bufEnd, double srate );
    
//  -- tracking analysis --

    //! Analyze a vector of (mono) samples at the given sample rate         
    //! (in Hz) and store the extracted Partials in the Analyzer's
    //! PartialList (std::list of Partials). Use the specified envelope
    //! as a frequency reference for Partial tracking.
    //!
    //! \param  vec is a vector of floating point samples
    //! \param  srate is the sample rate of the samples in the vector
    //! \param  reference is an Envelope having the approximate
    //!         frequency contour expected of the resulting Partials.
    void analyze( const std::vector<double> & vec, double srate, 
                  const Envelope & reference );
    
    //! Analyze a range of (mono) samples at the given sample rate      
    //! (in Hz) and store the extracted Partials in the Analyzer's
    //! PartialList (std::list of Partials). Use the specified envelope
    //! as a frequency reference for Partial tracking.
    //! 
    //! \param  bufBegin is a pointer to a buffer of floating point samples
    //! \param  bufEnd is (one-past) the end of a buffer of floating point 
    //!         samples
    //! \param  srate is the sample rate of the samples in the buffer
    //! \param  reference is an Envelope having the approximate
    //!         frequency contour expected of the resulting Partials.
    void analyze( const double * bufBegin, const double * bufEnd, double srate,
                  const Envelope & reference );
    
//  -- parameter access --

    //! Return the amplitude floor (lowest detected spectral amplitude),            
    //! in (negative) dB, for this Analyzer.                
    double ampFloor( void ) const;

    //! Return the crop time (maximum temporal displacement of a time-
    //! frequency data point from the time-domain center of the analysis
    //! window, beyond which data points are considered "unreliable")
    //! for this Analyzer.
    double cropTime( void ) const;

    //! Return the maximum allowable frequency difference between                   
    //! consecutive Breakpoints in a Partial envelope for this Analyzer.                
    double freqDrift( void ) const;

    //! Return the frequency floor (minimum instantaneous Partial               
    //! frequency), in Hz, for this Analyzer.               
    double freqFloor( void ) const;
	
	//! Return the frequency resolution (minimum instantaneous frequency        
	//! difference between Partials) for this Analyzer at the specified
	//! time in seconds. If no time is specified, then the initial resolution
	//!	(at 0 seconds) is returned.
	//! 
	//! \param time is the time in seconds at which to evaluate the 
	//!		   frequency resolution
	double freqResolution( double time = 0.0 ) const; 

    //! Return the hop time (which corresponds approximately to the 
    //! average density of Partial envelope Breakpoint data) for this 
    //! Analyzer.
    double hopTime( void ) const;

    //! Return the sidelobe attenutation level for the Kaiser analysis window in
    //! positive dB. Larger numbers (e.g. 90) give very good sidelobe 
    //! rejection but cause the window to be longer in time. Smaller numbers 
    //! (like 60) raise the level of the sidelobes, increasing the likelihood
    //! of frequency-domain interference, but allow the window to be shorter
    //! in time.
    double sidelobeLevel( void ) const;

    //! Return the frequency-domain main lobe width (measured between 
    //! zero-crossings) of the analysis window used by this Analyzer.               
    double windowWidth( void ) const;
     
    //! Return true if the phases and frequencies of the constructed
    //! partials should be modified to be consistent at the end of the
    //! analysis, and false otherwise. (Default is true.)
    bool phaseCorrect( void ) const;


//  -- parameter mutation --

    //! Set the amplitude floor (lowest detected spectral amplitude), in            
    //! (negative) dB, for this Analyzer. 
    //! 
    //! \param x is the new value of this parameter.            
    void setAmpFloor( double x );

    //! Set the crop time (maximum temporal displacement of a time-
    //! frequency data point from the time-domain center of the analysis
    //! window, beyond which data points are considered "unreliable")
    //! for this Analyzer.
    //! 
    //! \param x is the new value of this parameter.            
    void setCropTime( double x );

    //! Set the maximum allowable frequency difference between                  
    //! consecutive Breakpoints in a Partial envelope for this Analyzer.                
    //! 
    //! \param x is the new value of this parameter.            
    void setFreqDrift( double x );

    //! Set the frequency floor (minimum instantaneous Partial                  
    //! frequency), in Hz, for this Analyzer.
    //! 
    //! \param x is the new value of this parameter.            
    void setFreqFloor( double x );

    //! Set the frequency resolution (minimum instantaneous frequency       
    //! difference between Partials) for this Analyzer. (Does not cause     
    //! other parameters to be recomputed.)                                     
    //! 
    //! \param x is the new value of this parameter.            
    void setFreqResolution( double x );
    
	//! Set the time-varying frequency resolution (minimum instantaneous frequency       
	//! difference between Partials) for this Analyzer. (Does not cause     
	//! other parameters to be recomputed.)                                     
	//! 
	//! \param e is the envelope to copy for this parameter.                                        
	void setFreqResolution( const Envelope & e );    

    //! Set the hop time (which corresponds approximately to the average
    //! density of Partial envelope Breakpoint data) for this Analyzer.
    //! 
    //! \param x is the new value of this parameter.            
    void setHopTime( double x );

    //! Set the sidelobe attenutation level for the Kaiser analysis window in
    //! positive dB. More negative numbers (e.g. -90) give very good sidelobe 
    //! rejection but cause the window to be longer in time. Less negative 
    //! numbers raise the level of the sidelobes, increasing the likelihood
    //! of frequency-domain interference, but allow the window to be shorter
    //! in time.
    //! 
    //! \param x is the new value of this parameter.            
    void setSidelobeLevel( double x );

    //! Set the frequency-domain main lobe width (measured between 
    //! zero-crossings) of the analysis window used by this Analyzer.   
    //! 
    //! \param x is the new value of this parameter.            
    void setWindowWidth( double x );

    //! Indicate whether the phases and frequencies of the constructed
    //! partials should be modified to be consistent at the end of the
    //! analysis. (Default is true.)
    //!
    //! \param  TF is a flag indicating whether or not to construct
    //!         phase-corrected Partials
    void setPhaseCorrect( bool TF = true );
    
    
//  -- bandwidth envelope specification --

    enum { Default_ResidueBandwidth_RegionWidth = 2000,
           Default_ConvergenceBandwidth_TolerancePct = 10 };
           
    //! Construct Partial bandwidth envelopes during analysis
    //! by associating residual energy in the spectrum (after
    //! peak extraction) with the selected spectral peaks that
    //! are used to construct Partials. 
    //!
    //!	This is the default bandwidth-enhancement strategy.
    //! 
    //! \param regionWidth is the width (in Hz) of the bandwidth 
    //! association regions used by this process, must be positive.
    //! If unspecified, a default value is used.
    void storeResidueBandwidth( double regionWidth = Default_ResidueBandwidth_RegionWidth );
    
	//!	Construct Partial bandwidth envelopes during analysis
	//!	by storing the mixed derivative of short-time phase, 
	//!	scaled and shifted so that a value of 0 corresponds
	//!	to a pure sinusoid, and a value of 1 corresponds to a
	//! bandwidth-enhanced sinusoid with maximal energy spread
	//! (minimum sinusoidal convergence).
	//!
	//!	\param tolerance is the amount of range over which the 
	//!	mixed derivative indicator should be allowed to drift away 
	//!	from a pure sinusoid before saturating. This range is mapped
	//!	to bandwidth values on the range [0,1]. Must be positive and 
	//!	not greater than 1. If unspecified, a default value is used.
    void storeConvergenceBandwidth( double tolerancePct = 
    		0.01 * (double)Default_ConvergenceBandwidth_TolerancePct );
    
    //! Disable bandwidth envelope construction. Bandwidth 
    //! will be zero for all Breakpoints in all Partials.
    void storeNoBandwidth( void );
    
    //! Return true if this Analyzer is configured to compute
    //! bandwidth envelopes using the spectral residue after
    //! peaks have been identified, and false otherwise.
    bool bandwidthIsResidue( void ) const;
    
    //! Return true if this Analyzer is configured to compute
    //! bandwidth envelopes using the mixed derivative convergence
    //! indicator, and false otherwise.
    bool bandwidthIsConvergence( void ) const;
    
    //! Return the width (in Hz) of the Bandwidth Association regions
    //! used by this Analyzer, only if the spectral residue method is
    //! used to compute bandwidth envelopes. Return zero if the mixed
    //! derivative method is used, or if no bandwidth is computed.
    double bwRegionWidth( void ) const;

    //! Return the mixed derivative convergence tolerance (percent)
    //! only if the convergence indicator is used to compute
    //! bandwidth envelopes. Return zero if the spectral residue
    //! method is used or if no bandwidth is computed.
    double bwConvergenceTolerance( void ) const;

    //! Return true if bandwidth envelopes are to be constructed
    //!	by any means, that is, if either bandwidthIsResidue() or 
    //!	bandwidthIsConvergence() are true. Otherwise, return
    //! false.
    bool associateBandwidth( void ) const
        { return bandwidthIsResidue() || bandwidthIsConvergence(); }

    //! Deprecated, use storeResidueBandwidth or storeNoBandwidth instead.            
    void setBwRegionWidth( double x ) 
        { 
            if ( x != 0 )
            {
                storeResidueBandwidth( x ); 
            }
            else
            {
                storeNoBandwidth();
            }
        }
           

//  -- PartialList access --

    //! Return a mutable reference to this Analyzer's list of 
    //! analyzed Partials. 
    PartialList & partials( void );

    //! Return an immutable (const) reference to this Analyzer's 
    //! list of analyzed Partials. 
    const PartialList & partials( void ) const;

//  -- envelope access --

    enum { Default_FundamentalEnv_ThreshDb = -60, 
           Default_FundamentalEnv_ThreshHz = 8000 };

    //! Specify parameters for constructing a fundamental frequency 
    //! envelope for the analyzed sound during analysis. The fundamental 
    //! frequency estimate can be accessed by fundamentalEnv() after the 
    //! analysis is complete. 
    //!
    //! By default, a fundamental envelope is estimated during analysis
    //! between the frequency resolution and 1.5 times the resolution.
    //!
    //! \param  fmin is the lower bound on the fundamental frequency estimate
    //! \param  fmax is the upper bound on the fundamental frequency estimate
    //! \param  threshDb is the lower bound on the amplitude of a spectral peak
    //!         that will constribute to the fundamental frequency estimate (very
    //!         low amplitude peaks tend to have less reliable frequency estimates).
    //!         Default is -60 dB.
    //! \param  threshHz is the upper bound on the frequency of a spectral
    //!         peak that will constribute to the fundamental frequency estimate.
    //!         Default is 8 kHz.
    void buildFundamentalEnv( double fmin, double fmax, 
                              double threshDb = Default_FundamentalEnv_ThreshDb, 
                              double threshHz = Default_FundamentalEnv_ThreshHz );                                     


    //! Return the fundamental frequency estimate envelope constructed
    //! during the most recent analysis performed by this Analyzer.
    //!
    //! By default, a fundamental envelope is estimated during analysis
    //! between the frequency resolution and 1.5 times the resolution.
    const LinearEnvelope & fundamentalEnv( void ) const;

    //! Return the overall amplitude estimate envelope constructed
    //! during the most recent analysis performed by this Analyzer.
    const LinearEnvelope & ampEnv( void ) const;
    
    
//  -- legacy support --
    
    //  Fundamental and amplitude envelopes are always constructed during
    //  analysis, these members do nothing, and are retained for backwards
    //  compatibility.
    void buildAmpEnv( bool TF = true ) { TF = TF; }
    void buildFundamentalEnv( bool TF = true ) { TF = TF; }

//  -- private member variables --

private:

    std::auto_ptr< Envelope > m_freqResolutionEnv;    
    							//!  in Hz, minimum instantaneous frequency distance;
                                //!  this is the core parameter, others are, by default,
                                //!  computed from this one
    
    double m_ampFloor;          //!  dB, relative to full amplitude sine wave, absolute
                                //!  amplitude threshold (negative)
    
    double m_windowWidth;       //!  in Hz, width of main lobe; this might be more
                                //!  conveniently presented as window length, but
                                //!  the main lobe width more explicitly highlights
                                //!  the critical interaction with resolution
    
    // std::auto_ptr< Envelope > m_freqFloorEnv; 
    double m_freqFloor;         //!  lowest frequency (Hz) component extracted
                                //!  in spectral analysis
    
    double m_freqDrift;         //!  the maximum frequency (Hz) difference between two 
                                //!  consecutive Breakpoints that will be linked to
                                //!  form a Partial
    
    double m_hopTime;           //!  in seconds, time between analysis windows in
                                //!  successive spectral analyses
    
    double m_cropTime;          //!  in seconds, maximum time correction for a spectral
                                //!  component to be considered reliable, and to be eligible
                                //!  for extraction and for Breakpoint formation
    
    double m_bwAssocParam;      //!  formerly, width in Hz of overlapping bandwidth 
                                //!  association regions, or zero if bandwidth association
                                //!  is disabled, now a catch-all bandwidth association
                                //!  parameter that, if negative, indicates the tolerance (%)
                                //!  level used to construct bandwidth envelopes from the
                                //!  mixed phase derivative indicator
                                                        
    double m_sidelobeLevel;     //!  sidelobe attenutation level for the Kaiser analysis 
                                //!  window, in positive dB
                                
    bool m_phaseCorrect;        //!  flag indicating that phases/frequencies should be
                                //!  made consistent at the end of the analysis
                            
    PartialList m_partials;     //!  collect Partials here
        
    //! builder object for constructing a fundamental frequency
    //! estimate during analysis
    std::auto_ptr< LinearEnvelopeBuilder > m_f0Builder;

    //! builder object for constructing an amplitude
    //! estimate during analysis
    std::auto_ptr< LinearEnvelopeBuilder > m_ampEnvBuilder;

//  -- private auxiliary functions --
//	future development
/*

	//	These members make up the sequence of operations in an
	//	analysis. If analysis were ever to be made into a 
	//	template method, these would be the operations that
	//	derived classes could override. Or each of these could
	//	be represented by a strategy class.

	//!	Compute the spectrum of the next sequence of samples.
	void computeSpectrum( void );
	
	//!	Identify and select the spectral components that will be
	//!	used to form Partials. 
	void selectPeaks( void );
	
	//!	Compute the bandwidth coefficients for the Breakpoints
	//!	that are going to be used to form Partials. 
	void associateBandwidth( void );
	
	//!	Construct Partials from extracted spectral components.
	//!	Partials are built up frame by frame by appending
	//!	Breakpoints to Partials under construction, and giving
	//!	birth to new Partials using unmatched Peaks.
	void formPartials( Peaks & peaks );
*/
    //  Reject peaks that are too close in frequency to a louder peak that is
    //  being retained, and peaks that are too quiet. Peaks that are retained,
    //  but are quiet enough to be in the specified fadeRange should be faded.
    //  
    //  Rejected peaks are placed at the end of the peak collection.
    //  Return the first position in the collection containing a rejected peak,
    //  or the end of the collection if no peaks are rejected.
    Peaks::iterator thinPeaks( Peaks & peaks, double frameTime  );
                
    //  Fix the bandwidth value stored in the specified Peaks. 
    //  This function is invoked if the spectral residue method is
    //  not used to compute bandwidth (that method overwrites the
    //  bandwidth already). If the convergence method is used to 
    //  compute bandwidth, the appropriate scaling is applied
    //  to the stored mixed phase derivative. Otherwise, the
    //  Peak bandwidth is set to zero.
    void fixBandwidth( Peaks & peaks );
                    
};  //  end of class Analyzer

}   //  end of namespace Loris

#endif /* ndef INCLUDE_ANALYZER_H */
