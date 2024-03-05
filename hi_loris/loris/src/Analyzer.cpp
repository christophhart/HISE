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
 * Analyzer.C
 *
 * Implementation of class Loris::Analyzer.
 *
 * Kelly Fitz, 5 Dec 99
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */

#if HAVE_CONFIG_H
    #include "config.h"
#endif

#include "Analyzer.h"

#include "AssociateBandwidth.h"
#include "Breakpoint.h"
#include "BreakpointEnvelope.h"
#include "Envelope.h"
#include "F0Estimate.h"
#include "LorisExceptions.h"
#include "KaiserWindow.h"
#include "Notifier.h"
#include "Partial.h"
#include "PartialPtrs.h"
#include "ReassignedSpectrum.h"
#include "SpectralPeakSelector.h"
#include "PartialBuilder.h"

#include "phasefix.h"   //  for frequency/phase fixing at end of analysis



#include <algorithm>
#include <cmath>
#include <functional>   //  for std::plus
#include <memory>
#include <numeric>      //  for std::inner_product
#include <utility>
#include <vector>

using namespace std;

#if defined(HAVE_M_PI) && (HAVE_M_PI)
    const double Pi = M_PI;
#else
    const double Pi = 3.14159265358979324;
#endif

//  begin namespace
namespace Loris {

// ---------------------------------------------------------------------------
//  helpers, used below
// ---------------------------------------------------------------------------
static double accumPeakSquaredAmps( double init, 
                                    const SpectralPeak & pk )
{
    return init + (pk.amplitude() * pk.amplitude());
}

template < class Pair >
static double compare2nd( const Pair & p1, const Pair & p2 )
{
    return p1.second < p2.second;
}

// ---------------------------------------------------------------------------
//  LinearEnvelopeBuilder
// ---------------------------------------------------------------------------
//  Base class for envelope builders that add a point (possibly) at each
//  analysis frame. 
//
//  TODO: make a dictionary of these things and allow clients to add their
//  own envelope builders and builder functions, and retrieve them after
//  analysis.
class LinearEnvelopeBuilder
{
public:
    virtual ~LinearEnvelopeBuilder( void ) {}
    virtual LinearEnvelopeBuilder * clone( void ) const = 0;
    virtual void build( const Peaks & peaks, double frameTime ) = 0;
    
    const LinearEnvelope & envelope( void ) const { return mEnvelope; }
    
    //  reset (clear) envelope, override if necesssary:
    virtual void reset( void ) { mEnvelope.clear(); }
    
protected:

    LinearEnvelope mEnvelope;   //  build this
};

// ---------------------------------------------------------------------------
//  FundamentalBuilder - for constructing an F0 envelope during analysis
// ---------------------------------------------------------------------------
class FundamentalBuilder : public LinearEnvelopeBuilder
{
    std::unique_ptr< Envelope > mFminEnv;
    std::unique_ptr< Envelope > mFmaxEnv; 
    
    double mAmpThresh, mFreqThresh;
    
    std::vector< double > amplitudes, frequencies;
    
    const double mMinConfidence;    // 0.9, this could be made a parameter, 
                                    // or raised to make estimates smoother
    
public:
    FundamentalBuilder( double fmin, double fmax, double threshDb = -60, double threshHz = 8000 ) :
        mFminEnv( new LinearEnvelope( fmin ) ), 
        mFmaxEnv( new LinearEnvelope( fmax ) ), 
        mAmpThresh( std::pow( 10., 0.05*(threshDb) ) ),
        mFreqThresh( threshHz ),
        mMinConfidence( 0.9 )
        {}

    FundamentalBuilder( const Envelope & fmin, const Envelope & fmax, 
    					double threshDb = -60, double threshHz = 8000 ) :
        mFminEnv( fmin.clone() ), 
        mFmaxEnv( fmax.clone() ), 
        mAmpThresh( std::pow( 10., 0.05*(threshDb) ) ),
        mFreqThresh( threshHz ),
        mMinConfidence( 0.9 )
        {}
        	
    FundamentalBuilder( const FundamentalBuilder & rhs ) :
        mFminEnv( rhs.mFminEnv->clone() ), 
        mFmaxEnv( rhs.mFmaxEnv->clone() ), 
        mAmpThresh( rhs.mAmpThresh ),
        mFreqThresh( rhs.mFreqThresh ),
        mMinConfidence( rhs.mMinConfidence )
        {}
    
        
	FundamentalBuilder * clone( void ) const { return new FundamentalBuilder(*this); }
	
    void build( const Peaks & peaks, double frameTime );
};

// ---------------------------------------------------------------------------
//  FundamentalBuilder::build
// ---------------------------------------------------------------------------
//
void FundamentalBuilder::build( const Peaks & peaks, double frameTime )
{
    amplitudes.clear();
    frequencies.clear();
    for ( Peaks::const_iterator spkpos = peaks.begin(); spkpos != peaks.end(); ++spkpos )
    {
        if ( spkpos->amplitude() > mAmpThresh &&
             spkpos->frequency() < mFreqThresh )
        {
            amplitudes.push_back( spkpos->amplitude() );
            frequencies.push_back( spkpos->frequency() );
        }
    }
    if ( ! amplitudes.empty() )
    {
        const double fmin = mFminEnv->valueAt( frameTime );
        const double fmax = mFmaxEnv->valueAt( frameTime );
        
        //  estimate f0
        F0Estimate est( amplitudes, frequencies, fmin, fmax, 0.1 );
        
        if ( est.confidence() >= mMinConfidence &&
             est.frequency() > fmin && est.frequency() < fmax  )
        {
            // notifier << "f0 is " << est.frequency << endl;
            //  add breakpoint to fundamental envelope
            mEnvelope.insert( frameTime, est.frequency() );
        }
    }
    
}

// ---------------------------------------------------------------------------
//  AmpEnvBuilder - for constructing an amplitude envelope during analysis
// ---------------------------------------------------------------------------
class AmpEnvBuilder : public LinearEnvelopeBuilder
{
public:
    AmpEnvBuilder( void ) {}
		
	AmpEnvBuilder * clone( void ) const { return new AmpEnvBuilder(*this); }
	
    void build( const Peaks & peaks, double frameTime );

};

// ---------------------------------------------------------------------------
//  AmpEnvBuilder::build
// ---------------------------------------------------------------------------
//
void AmpEnvBuilder::build( const Peaks & peaks, double frameTime )
{
    double x = std::accumulate( peaks.begin(), peaks.end(), 0.0, accumPeakSquaredAmps );
    mEnvelope.insert( frameTime, std::sqrt( x ) );
}


// ---------------------------------------------------------------------------
//  Analyzer constructor - frequency resolution only
// ---------------------------------------------------------------------------
//! Construct a new Analyzer configured with the given  
//! frequency resolution (minimum instantaneous frequency   
//! difference between Partials). All other Analyzer parameters     
//! are computed from the specified frequency resolution.   
//! 
//! \param resolutionHz is the frequency resolution in Hz.
//
Analyzer::Analyzer( double resolutionHz )
{
    configure( resolutionHz, 2.0 * resolutionHz );
}

// ---------------------------------------------------------------------------
//  Analyzer constructor
// ---------------------------------------------------------------------------
//! Construct a new Analyzer configured with the given  
//! frequency resolution (minimum instantaneous frequency   
//! difference between Partials) and analysis window width
//! (main lobe, zero-to-zero). All other Analyzer parameters    
//! are computed from the specified resolution and window width.    
//! 
//! \param resolutionHz is the frequency resolution in Hz.
//! \param windowWidthHz is the main lobe width of the Kaiser
//! analysis window in Hz.
//
Analyzer::Analyzer( double resolutionHz, double windowWidthHz )
{
    configure( resolutionHz, windowWidthHz );
}

// ---------------------------------------------------------------------------
//  Analyzer constructor
// ---------------------------------------------------------------------------
//! Construct a new Analyzer configured with the given time-varying
//! frequency resolution (minimum instantaneous frequency   
//! difference between Partials) and analysis window width
//! (main lobe, zero-to-zero). All other Analyzer parameters    
//! are computed from the specified resolution and window width.    
//! 
//! \param resolutionHz is the frequency resolution in Hz.
//! \param windowWidthHz is the main lobe width of the Kaiser
//! analysis window in Hz.
//
Analyzer::Analyzer( const Envelope & resolutionEnv, double windowWidthHz )
{
    configure( resolutionEnv, windowWidthHz );
}

// ---------------------------------------------------------------------------
//  Analyzer copy constructor
// ---------------------------------------------------------------------------
//! Construct  a new Analyzer having identical
//! parameter configuration to another Analyzer. 
//! The list of collected Partials is not copied.       
//! 
//! \param other is the Analyzer to copy.   
//
Analyzer::Analyzer( const Analyzer & other ) :
    m_freqResolutionEnv( other.m_freqResolutionEnv->clone() ),
    m_ampFloor( other.m_ampFloor ),
    m_windowWidth( other.m_windowWidth ),
    m_freqFloor( other.m_freqFloor ),
    m_freqDrift( other.m_freqDrift ),
    m_hopTime( other.m_hopTime ),
    m_cropTime( other.m_cropTime ),
    m_bwAssocParam( other.m_bwAssocParam ),
    m_sidelobeLevel( other.m_sidelobeLevel ),
    m_phaseCorrect( other.m_phaseCorrect ),
    m_partials( other.m_partials )
{
    m_f0Builder.reset( other.m_f0Builder->clone() );
    m_ampEnvBuilder.reset( other.m_ampEnvBuilder->clone() );
}

// ---------------------------------------------------------------------------
//  Analyzer assignment
// ---------------------------------------------------------------------------
//! Construct  a new Analyzer having identical
//! parameter configuration to another Analyzer. 
//! The list of collected Partials is not copied.       
//! 
//! \param rhs is the Analyzer to copy. 
//
Analyzer & 
Analyzer::operator=( const Analyzer & rhs )
{
    if ( this != & rhs ) 
    {
        m_freqResolutionEnv.reset( rhs.m_freqResolutionEnv->clone() );
        m_ampFloor = rhs.m_ampFloor;
        m_windowWidth = rhs.m_windowWidth;
        m_freqFloor = rhs.m_freqFloor;  
        m_freqDrift = rhs.m_freqDrift;
        m_hopTime = rhs.m_hopTime;
        m_cropTime = rhs.m_cropTime;
        m_bwAssocParam = rhs.m_bwAssocParam;
        m_sidelobeLevel = rhs.m_sidelobeLevel;
        m_phaseCorrect = rhs.m_phaseCorrect;
        m_partials = rhs.m_partials;

        m_f0Builder.reset( rhs.m_f0Builder->clone() );
        m_ampEnvBuilder.reset( rhs.m_ampEnvBuilder->clone() );
                
    }
    return *this;
}

// ---------------------------------------------------------------------------
//  Analyzer destructor
// ---------------------------------------------------------------------------
//! Destroy this Analyzer.
//
Analyzer::~Analyzer( void )
{
}

// -- configuration --

// ---------------------------------------------------------------------------
//  configure
// ---------------------------------------------------------------------------
//! Configure this Analyzer with the given frequency resolution 
//! (minimum instantaneous frequency difference between Partials, 
//! in Hz). All other Analyzer parameters are (re-)computed from the 
//! frequency resolution, including the window width, which is
//!	twice the resolution.      
//! 
//! \param resolutionHz is the frequency resolution in Hz.
//
void
Analyzer::configure( double resolutionHz )
{
	configure( resolutionHz, 2.0 * resolutionHz );
}

// ---------------------------------------------------------------------------
//  configure
// ---------------------------------------------------------------------------
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
//
void
Analyzer::configure( double resolutionHz, double windowWidthHz )
{
    //  use specified resolution:
    setFreqResolution( resolutionHz );
    
    //  floor defaults to -90 dB:
    setAmpFloor( -90. );
    
    //  window width should generally be approximately 
    //  equal to, and never more than twice the 
    //  frequency resolution:
    setWindowWidth( windowWidthHz );
    
    //  the Kaiser window sidelobe level can be the same
    //  as the amplitude floor (except in positive dB):
    setSidelobeLevel( - m_ampFloor );
    
    //  for the minimum frequency, below which no data is kept,
    //  use the frequency resolution by default (this makes 
    //  Lip happy, and is always safe?) and allow the client 
    //  to change it to anything at all.
    setFreqFloor( resolutionHz );
    
    //  frequency drift in Hz is the maximum difference
    //  in frequency between consecutive Breakpoints in
    //  a Partial, by default, make it equal to one half
    //  the frequency resolution:
    setFreqDrift( .5 * resolutionHz );
    
    //  hop time (in seconds) is the inverse of the
    //  window width....really. Smith and Serra (1990) cite 
    //  Allen (1977) saying: a good choice of hop is the window 
    //  length divided by the main lobe width in frequency samples,
    //  which turns out to be just the inverse of the width.
    setHopTime( 1. / m_windowWidth );
    
    //  crop time (in seconds) is the maximum allowable time
    //  correction, beyond which a reassigned spectral component
    //  is considered unreliable, and not considered eligible for
    //  Breakpoint formation in extractPeaks(). By default, use
    //  the hop time (should it be half that?):
    setCropTime( m_hopTime );
    
    //  bandwidth association region width 
    //  defaults to 2 kHz, corresponding to 
    //  1 kHz region center spacing:
    storeResidueBandwidth();

	//  configure the envelope builders using default 
	//  parameters:
	buildFundamentalEnv( 0.99 * resolutionHz,
                         1.5 * resolutionHz );
    m_ampEnvBuilder.reset( new AmpEnvBuilder );
    
    //  enable phase-correct Partial construction:
    m_phaseCorrect = true;
}

// ---------------------------------------------------------------------------
//  configure
// ---------------------------------------------------------------------------
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
void
Analyzer::configure( const Envelope & resolutionEnv, double windowWidthHz )
{
    //  use specified resolution:
    setFreqResolution( resolutionEnv );
    
    //  floor defaults to -90 dB:
    setAmpFloor( -90. );
    
    //  window width should generally be approximately 
    //  equal to, and never more than twice the 
    //  frequency resolution:
    setWindowWidth( windowWidthHz );
    
    //  the Kaiser window sidelobe level can be the same
    //  as the amplitude floor (except in positive dB):
    setSidelobeLevel( - m_ampFloor );
    
    //  for the minimum frequency, below which no data is kept,
    //  use the frequency resolution by default (this makes 
    //  Lip happy, and is always safe?) and allow the client 
    //  to change it to anything at all.
    setFreqFloor( windowWidthHz * 0.5 );		//	!!!!!
    
    //  frequency drift in Hz is the maximum difference
    //  in frequency between consecutive Breakpoints in
    //  a Partial, by default, make it equal to one half
    //  the frequency resolution:
    setFreqDrift( windowWidthHz * 0.25 );		//	!!!!!
    
    //  hop time (in seconds) is the inverse of the
    //  window width....really. Smith and Serra (1990) cite 
    //  Allen (1977) saying: a good choice of hop is the window 
    //  length divided by the main lobe width in frequency samples,
    //  which turns out to be just the inverse of the width.
    setHopTime( 1. / m_windowWidth );
    
    //  crop time (in seconds) is the maximum allowable time
    //  correction, beyond which a reassigned spectral component
    //  is considered unreliable, and not considered eligible for
    //  Breakpoint formation in extractPeaks(). By default, use
    //  the hop time (should it be half that?):
    setCropTime( m_hopTime );
    
    //  bandwidth association region width 
    //  defaults to 2 kHz, corresponding to 
    //  1 kHz region center spacing:
    storeResidueBandwidth();

	//  configure the envelope builders using default 
	//  parameters:
	/*
	buildFundamentalEnv( *m_freqResolutionEnv * 0.99,
                         *m_freqResolutionEnv * 1.5 );
      
     */	//	!!!!!!!
	m_f0Builder.reset( 
        new FundamentalBuilder( *m_freqResolutionEnv * 0.99,
        						*m_freqResolutionEnv * 1.5,
        						-60., 8000. ) );
        
    m_ampEnvBuilder.reset( new AmpEnvBuilder );
    
    //  enable phase-correct Partial construction:
    m_phaseCorrect = true;
}

// -- analysis --
// ---------------------------------------------------------------------------
//  analyze
// ---------------------------------------------------------------------------
//! Analyze a vector of (mono) samples at the given sample rate         
//! (in Hz) and store the extracted Partials in the Analyzer's
//! PartialList (std::list of Partials). 
//! 
//! \param vec is a vector of floating point samples
//! \param srate is the sample rate of the samples in the vector 
//
void 
Analyzer::analyze( const std::vector<double> & vec, double srate )      
{ 
    BreakpointEnvelope reference( 1.0 );
    analyze( &(vec[0]),  &(vec[0]) + vec.size(), srate, reference ); 
}

// ---------------------------------------------------------------------------
//  analyze
// ---------------------------------------------------------------------------
//! Analyze a range of (mono) samples at the given sample rate      
//! (in Hz) and store the extracted Partials in the Analyzer's
//! PartialList (std::list of Partials). 
//! 
//! \param bufBegin is a pointer to a buffer of floating point samples
//! \param bufEnd is (one-past) the end of a buffer of floating point 
//! samples
//! \param srate is the sample rate of the samples in the buffer
//
void 
Analyzer::analyze( const double * bufBegin, const double * bufEnd, double srate )
{ 
    BreakpointEnvelope reference( 1.0 );
    analyze( bufBegin,  bufEnd, srate, reference ); 
}

// ---------------------------------------------------------------------------
//  analyze
// ---------------------------------------------------------------------------
//! Analyze a vector of (mono) samples at the given sample rate         
//! (in Hz) and store the extracted Partials in the Analyzer's
//! PartialList (std::list of Partials). Use the specified envelope
//! as a frequency reference for Partial tracking.
//!
//! \param vec is a vector of floating point samples
//! \param srate is the sample rate of the samples in the vector
//! \param reference is an Envelope having the approximate
//! frequency contour expected of the resulting Partials.
//
void 
Analyzer::analyze( const std::vector<double> & vec, double srate, 
                   const Envelope & reference )     
{ 
    analyze( &(vec[0]),  &(vec[0]) + vec.size(), srate, reference ); 
}


// ---------------------------------------------------------------------------
//  analyze
// ---------------------------------------------------------------------------
//! Analyze a range of (mono) samples at the given sample rate      
//! (in Hz) and store the extracted Partials in the Analyzer's
//! PartialList (std::list of Partials). Use the specified envelope
//! as a frequency reference for Partial tracking.
//! 
//! \param bufBegin is a pointer to a buffer of floating point samples
//! \param bufEnd is (one-past) the end of a buffer of floating point 
//! samples
//! \param srate is the sample rate of the samples in the buffer
//! \param reference is an Envelope having the approximate
//! frequency contour expected of the resulting Partials.
//
void 
Analyzer::analyze( const double * bufBegin, const double * bufEnd, double srate,
                   const Envelope & reference )
{ 
    //  configure the reassigned spectral analyzer, 
    //  always use odd-length windows:

    //  Kaiser window
    double winshape = KaiserWindow::computeShape( sidelobeLevel() );
    long winlen = KaiserWindow::computeLength( windowWidth() / srate, winshape );    
    if (! (winlen % 2)) 
    {
        ++winlen;
    }
    debugger << "Using Kaiser window of length " << winlen << endl;
    
    std::vector< double > window( winlen );
    KaiserWindow::buildWindow( window, winshape );
    
    std::vector< double > windowDeriv( winlen );
    KaiserWindow::buildTimeDerivativeWindow( windowDeriv, winshape );
       
    ReassignedSpectrum spectrum( window, windowDeriv );   
    
    //  configure the peak selection and partial formation policies:
    SpectralPeakSelector selector( srate, m_cropTime );
    PartialBuilder builder( m_freqDrift, reference );
    
    //  configure bw association policy, unless
    //  bandwidth association is disabled:
    std::unique_ptr< AssociateBandwidth > bwAssociator;
    if( m_bwAssocParam > 0 )
    {
        debugger << "Using bandwidth association regions of width " 
                 << bwRegionWidth() << " Hz" << endl;
        bwAssociator.reset( new AssociateBandwidth( bwRegionWidth(), srate ) );
    }
    else
    {
        debugger << "Bandwidth association disabled" << endl;
    }

    //  reset envelope builders:
    m_ampEnvBuilder->reset();
    m_f0Builder->reset();
    
    m_partials.clear();
        
    try 
    { 
        const double * winMiddle = bufBegin; 

        //  loop over short-time analysis frames:
        while ( winMiddle < bufEnd )
        {
			

            //  compute the time of this analysis frame:
            const double currentFrameTime = long(winMiddle - bufBegin) / srate;
            
			const double totalLength = (double)(bufEnd - bufBegin) / srate;

			if (threadController != nullptr && !threadController->setProgress(currentFrameTime / totalLength))
				throw Exception("cancelled");

            //  compute reassigned spectrum:
            //  sampsBegin is the position of the first sample to be transformed,
            //  sampsEnd is the position after the last sample to be transformed.
            //  (these computations work for odd length windows only)
            const double * sampsBegin = std::max( winMiddle - (winlen / 2), bufBegin );
            const double * sampsEnd = std::min( winMiddle + (winlen / 2) + 1, bufEnd );
            spectrum.transform( sampsBegin, winMiddle, sampsEnd );
            
             
            //  extract peaks from the spectrum, and thin
            Peaks peaks = selector.selectPeaks( spectrum, m_freqFloor ); 
			Peaks::iterator rejected = thinPeaks( peaks, currentFrameTime );

            //	fix the stored bandwidth values
            //	KLUDGE: need to do this before the bandwidth
            //	associator tries to do its job, because the mixed
            //	derivative is temporarily stored in the Breakpoint 
            //	bandwidth!!! FIX!!!!
            fixBandwidth( peaks );
            
            if ( m_bwAssocParam > 0 )
            {
                bwAssociator->associateBandwidth( peaks.begin(), rejected, peaks.end() );
            }
            
            //  remove rejected Breakpoints (needed above to 
            //  compute bandwidth envelopes):
            peaks.erase( rejected, peaks.end() );
            
            //  estimate the amplitude in this frame:
            m_ampEnvBuilder->build( peaks, currentFrameTime );
                        
            //  collect amplitudes and frequencies and try to 
            //  estimate the fundamental
            m_f0Builder->build( peaks, currentFrameTime );          

            //  form Partials from the extracted Breakpoints:
            builder.buildPartials( peaks, currentFrameTime );
            
            //  slide the analysis window:
            winMiddle += long( m_hopTime * srate ); //  hop in samples, truncated

        }   //  end of loop over short-time frames
        
        //  unwarp the Partial frequency envelopes:
        builder.finishBuilding( m_partials );
        
        //  fix the frequencies and phases to be consistent.
        if ( m_phaseCorrect )
        {
            fixFrequency( m_partials.begin(), m_partials.end() );
        }
        
        
        //  for debugging:
        /*
        if ( ! m_ampEnv.empty() )
        {
            LinearEnvelope::iterator peakpos = 
                std::max_element( m_ampEnv.begin(), m_ampEnv.end(), 
                                  compare2nd<LinearEnvelope::iterator::value_type> );
            notifier << "Analyzer found amp peak at time : " << peakpos->first
                     << " value: " << peakpos->second << endl;
        }
        */
    }
    catch ( Exception & ex ) 
    {
        ex.append( "analysis failed." );
        throw;
    }
}

// -- parameter access --

// ---------------------------------------------------------------------------
//  ampFloor
// ---------------------------------------------------------------------------
//! Return the amplitude floor (lowest detected spectral amplitude),            
//! in (negative) dB, for this Analyzer.                
//
double 
Analyzer::ampFloor( void ) const 
{ 
    return m_ampFloor; 
}

// ---------------------------------------------------------------------------
//  cropTime
// ---------------------------------------------------------------------------
//! Return the crop time (maximum temporal displacement of a time-
//! frequency data point from the time-domain center of the analysis
//! window, beyond which data points are considered "unreliable")
//! for this Analyzer.
//
double 
Analyzer::cropTime( void ) const 
{ 
    // debugger << "Analyzer::cropTime() is a deprecated member, and will be removed in a future Loris release." << endl;
    return m_cropTime; 
}

// ---------------------------------------------------------------------------
//  freqDrift
// ---------------------------------------------------------------------------
//! Return the maximum allowable frequency difference 
//! consecutive Breakpoints in a Partial envelope for this Analyzer.                
//
double 
Analyzer::freqDrift( void ) const 
{ 
    return m_freqDrift;
}

// ---------------------------------------------------------------------------
//  freqFloor
// ---------------------------------------------------------------------------
//! Return the frequency floor (minimum instantaneous Partial               
//! frequency), in Hz, for this Analyzer.               
//
double 
Analyzer::freqFloor( void ) const 
{ 
    return m_freqFloor; 
}

// ---------------------------------------------------------------------------
//  freqResolution
// ---------------------------------------------------------------------------
//! Return the frequency resolution (minimum instantaneous frequency        
//! difference between Partials) for this Analyzer at the specified
//! time in seconds. If no time is specified, then the initial resolution
//!	(at 0 seconds) is returned.
//! 
//! \param time is the time in seconds at which to evaluate the 
//!		   frequency resolution
//
double 
Analyzer::freqResolution( double time /* = 0.0 */ ) const 
{ 
    return m_freqResolutionEnv->valueAt( time ); 
}

// ---------------------------------------------------------------------------
//  hopTime
// ---------------------------------------------------------------------------
//! Return the hop time (which corresponds approximately to the 
//! average density of Partial envelope Breakpoint data) for this 
//! Analyzer.
//
double 
Analyzer::hopTime( void ) const 
{ 
    return m_hopTime; 
}

// ---------------------------------------------------------------------------
//  sidelobeLevel
// ---------------------------------------------------------------------------
//! Return the sidelobe attenutation level for the Kaiser analysis window in
//! positive dB. Larger numbers (e.g. 90) give very good sidelobe 
//! rejection but cause the window to be longer in time. Smaller numbers 
//! (like 60) raise the level of the sidelobes, increasing the likelihood
//! of frequency-domain interference, but allow the window to be shorter
//! in time.
//
double 
Analyzer::sidelobeLevel( void ) const 
{ 
    return m_sidelobeLevel; 
}

// ---------------------------------------------------------------------------
//  windowWidth
// ---------------------------------------------------------------------------
//! Return the frequency-domain main lobe width (measured between 
//! zero-crossings) of the analysis window used by this Analyzer.               
//
double 
Analyzer::windowWidth( void ) const 
{ 
    return m_windowWidth; 
}

// ---------------------------------------------------------------------------
//  phaseCorrect
// ---------------------------------------------------------------------------
//! Return true if the phases and frequencies of the constructed
//! partials should be modified to be consistent at the end of the
//! analysis, and false otherwise. (Default is true.)
//!
//! \param  TF is a flag indicating whether or not to construct
//!         phase-corrected Partials
bool 
Analyzer::phaseCorrect( void ) const
{
    return m_phaseCorrect;
}

// -- parameter mutation --

#define VERIFY_ARG(func, test)                                          \
    do {                                                                \
        if (!(test))                                                    \
            Throw( Loris::InvalidArgument, #func ": " #test  );         \
    } while (false)


// ---------------------------------------------------------------------------
//  setAmpFloor
// ---------------------------------------------------------------------------
//! Set the amplitude floor (lowest detected spectral amplitude), in            
//! (negative) dB, for this Analyzer. 
//! 
//! \param x is the new value of this parameter.                
//
void 
Analyzer::setAmpFloor( double x ) 
{ 
    VERIFY_ARG( setAmpFloor, x < 0 );
    m_ampFloor = x; 
}


// ---------------------------------------------------------------------------
//  setCropTime
// ---------------------------------------------------------------------------
//! Set the crop time (maximum temporal displacement of a time-
//! frequency data point from the time-domain center of the analysis
//! window, beyond which data points are considered "unreliable")
//! for this Analyzer.
//! 
//! \param x is the new value of this parameter.
//
void 
Analyzer::setCropTime( double x ) 
{ 
    VERIFY_ARG( setCropTime, x > 0 );
   // debugger << "Analyzer::setCropTime() is a deprecated member, and will be removed in a future Loris release." << endl;
    m_cropTime = x; 
}

// ---------------------------------------------------------------------------
//  setFreqDrift
// ---------------------------------------------------------------------------
//! Set the maximum allowable frequency difference between                  
//! consecutive Breakpoints in a Partial envelope for this Analyzer.                
//! 
//! \param x is the new value of this parameter.            
//
void 
Analyzer::setFreqDrift( double x ) 
{ 
    VERIFY_ARG( setFreqDrift, x > 0 );
    m_freqDrift = x; 
}

// ---------------------------------------------------------------------------
//  setFreqFloor
// ---------------------------------------------------------------------------
//! Set the frequency floor (minimum instantaneous Partial                  
//! frequency), in Hz, for this Analyzer.
//! 
//! \param x is the new value of this parameter.                    
//
void 
Analyzer::setFreqFloor( double x ) 
{ 
    VERIFY_ARG( setFreqFloor, x >= 0 );
    m_freqFloor = x; 
}

// ---------------------------------------------------------------------------
//  setFreqResolution (constant)
// ---------------------------------------------------------------------------
//! Set the frequency resolution (minimum instantaneous frequency       
//! difference between Partials) for this Analyzer. (Does not cause     
//! other parameters to be recomputed.)                                     
//! 
//! \param x is the new value of this parameter.                                        
//
void 
Analyzer::setFreqResolution( double x ) 
{ 
    VERIFY_ARG( setFreqResolution, x > 0 );
    m_freqResolutionEnv.reset( new LinearEnvelope( x ) ); 
}

// ---------------------------------------------------------------------------
//  setFreqResolution (envelope)
// ---------------------------------------------------------------------------
//! Set the time-varying frequency resolution (minimum instantaneous frequency       
//! difference between Partials) for this Analyzer. (Does not cause     
//! other parameters to be recomputed.)                                     
//! 
//! \param e is the envelope to copy for this parameter.                                        
//
void 
Analyzer::setFreqResolution( const Envelope & e ) 
{ 
	//	No mechanism exists to verify that the envelope never
	//	drops below zero, this can only be checked at analysis-time.
    // VERIFY_ARG( setFreqResolution, x > 0 );
    m_freqResolutionEnv.reset( e.clone() ); 
}

// ---------------------------------------------------------------------------
//  setSidelobeLevel
// ---------------------------------------------------------------------------
//! Set the sidelobe attenutation level for the Kaiser analysis window in
//! positive dB. Higher numbers (e.g. 90) give very good sidelobe 
//! rejection but cause the window to be longer in time. Lower 
//! numbers raise the level of the sidelobes, increasing the likelihood
//! of frequency-domain interference, but allow the window to be shorter
//! in time.
//! 
//! \param x is the new value of this parameter.    
//
void 
Analyzer::setSidelobeLevel( double x ) 
{ 
    VERIFY_ARG( setSidelobeLevel, x > 0 );
    m_sidelobeLevel = x; 
}

// ---------------------------------------------------------------------------
//  setHopTime
// ---------------------------------------------------------------------------
//! Set the hop time (which corresponds approximately to the average
//! density of Partial envelope Breakpoint data) for this Analyzer.
//! 
//! \param x is the new value of this parameter.
//
void 
Analyzer::setHopTime( double x ) 
{ 
    VERIFY_ARG( setHopTime, x > 0 );
    m_hopTime = x; 
}

// ---------------------------------------------------------------------------
//  setWindowWidth
// ---------------------------------------------------------------------------
//! Set the frequency-domain main lobe width (measured between 
//! zero-crossings) of the analysis window used by this Analyzer.   
//! 
//! \param x is the new value of this parameter.            
//
void 
Analyzer::setWindowWidth( double x ) 
{ 
    VERIFY_ARG( setWindowWidth, x > 0 );
    m_windowWidth = x; 
}

// ---------------------------------------------------------------------------
//  setPhaseCorrect
// ---------------------------------------------------------------------------
//! Indicate whether the phases and frequencies of the constructed
//! partials should be modified to be consistent at the end of the
//! analysis. (Default is true.)
//!
//! \param  TF is a flag indicating whether or not to construct
//!         phase-corrected Partials
void 
Analyzer::setPhaseCorrect( bool TF )
{
    m_phaseCorrect = TF;
}

//  -- bandwidth envelope specification --


// ---------------------------------------------------------------------------
//  storeResidueBandwidth
// ---------------------------------------------------------------------------
//!	Construct Partial bandwidth envelopes during analysis
//!	by associating residual energy in the spectrum (after
//! peak extraction) with the selected spectral peaks that
//!	are used to construct Partials. 
//!	
//!	\param regionWidth is the width (in Hz) of the bandwidth 
//!	association regions used by this process, must be positive.
//!	If unspecified, a default value is used.
//
void 
Analyzer::storeResidueBandwidth( double regionWidth ) 
{ 
    VERIFY_ARG( storeResidueBandwidth, regionWidth > 0 );
    m_bwAssocParam = regionWidth; 
}   

// ---------------------------------------------------------------------------
//  storeConvergenceBandwidth
// ---------------------------------------------------------------------------
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
//
void 
Analyzer::storeConvergenceBandwidth( double tolerance ) 
{ 
	if ( 1.0 < tolerance )
	{
		//	notify and scale, in Loris 1.5, tolerance was
		//	specified as a percent
		notifier << "Analyzer::storeConvergenceBandwidth, conergence tolerance "
					"should be positive and less than 1.0, scaling by 1/100" << endl;
		tolerance *= 0.01;
	}
	
    VERIFY_ARG( storeConvergenceBandwidth, 
    			(tolerance > 0) && (tolerance <= 1) );
    			
	//	store a negative value so that it can be 
	//	identified when used:
    m_bwAssocParam = -tolerance; 
}   

// ---------------------------------------------------------------------------
//  storeNoBandwidth
// ---------------------------------------------------------------------------
//!	Disable bandwidth envelope construction. Bandwidth 
//!	will be zero for all Breakpoints in all Partials.
//
void 
Analyzer::storeNoBandwidth( void ) 
{ 
    m_bwAssocParam = 0; 
}   

// ---------------------------------------------------------------------------
//!	Return true if this Analyzer is configured to compute
//! bandwidth envelopes using the spectral residue after
//! peaks have been identified, and false otherwise.
// ---------------------------------------------------------------------------
bool 
Analyzer::bandwidthIsResidue( void ) const
{ 
    return m_bwAssocParam > 0.; 
}

// ---------------------------------------------------------------------------
//!	Return true if this Analyzer is configured to compute
//!	bandwidth envelopes using the mixed derivative convergence
//!	indicator, and false otherwise.
// ---------------------------------------------------------------------------
bool 
Analyzer::bandwidthIsConvergence( void ) const
{ 
    return m_bwAssocParam < 0.; 
}


// ---------------------------------------------------------------------------
//! Return the width (in Hz) of the Bandwidth Association regions
//! used by this Analyzer, only if the spectral residue method is
//!	used to compute bandwidth envelopes. Return zero if the mixed
//! derivative method is used, or if no bandwidth is computed.
// ---------------------------------------------------------------------------
double 
Analyzer::bwRegionWidth( void ) const
{
	if ( m_bwAssocParam > 0 )
	{
		return m_bwAssocParam;
	}
	return 0;
}

// ---------------------------------------------------------------------------
//!	Return the mixed derivative convergence tolerance (percent)
//!	only if the convergence indicator is used to compute
//!	bandwidth envelopes. Return zero if the spectral residue
//!	method is used or if no bandwidth is computed.
// ---------------------------------------------------------------------------
double 
Analyzer::bwConvergenceTolerance( void ) const
{
	if ( m_bwAssocParam < 0 )
	{
		return - m_bwAssocParam;
	}
	return 0;
}


// -- PartialList access --

// ---------------------------------------------------------------------------
//  partials
// ---------------------------------------------------------------------------
//! Return a mutable reference to this Analyzer's list of 
//! analyzed Partials. 
//
PartialList & 
Analyzer::partials( void ) 
{ 
    return m_partials; 
}

// ---------------------------------------------------------------------------
//  partials
// ---------------------------------------------------------------------------
//! Return an immutable (const) reference to this Analyzer's 
//! list of analyzed Partials. 
//
const PartialList & 
Analyzer::partials( void ) const
{ 
    return m_partials; 
}

// ---------------------------------------------------------------------------
//  buildFundamentalEnv
// ---------------------------------------------------------------------------
//! Specify parameters for constructing a fundamental frequency 
//! envelope for the analyzed sound during analysis. The fundamental 
//! frequency estimate can be accessed by fundamentalEnv() after the 
//! analysis is complete. 
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
//
void Analyzer::buildFundamentalEnv( double fmin, double fmax, 
                                    double threshDb, double threshHz )
{
    m_f0Builder.reset( 
        new FundamentalBuilder( fmin, fmax, threshDb, threshHz ) );
}

// ---------------------------------------------------------------------------
//  fundamentalEnv
// ---------------------------------------------------------------------------
//! Return the fundamental frequency estimate envelope constructed
//! during the most recent analysis performed by this Analyzer.
//! Will be empty unless buildFundamentalEnv was invoked to enable the
//! construction of this envelope during analysis.
//
const LinearEnvelope &
Analyzer::fundamentalEnv( void ) const
{   
    return m_f0Builder->envelope(); 
}


// ---------------------------------------------------------------------------
//  ampEnv
// ---------------------------------------------------------------------------
//! Return the overall amplitude estimate envelope constructed
//! during the most recent analysis performed by this Analyzer.
//! Will be empty unless buildAmpEnv was invoked to enable the
//! construction of this envelope during analysis.
//
const LinearEnvelope & 
Analyzer::ampEnv( void ) const
{ 
    return m_ampEnvBuilder->envelope(); 
}

// -- private helpers --

// ---------------------------------------------------------------------------
//	can_mask
// ---------------------------------------------------------------------------
//	functor used for identying peaks that are too close
//	in frequency to another louder peak:
struct can_mask
{
	//	masking occurs if any (louder) peak falls
	//	in the frequency range delimited by fmin and fmax:
	bool operator()( const SpectralPeak & v )  const
	{ 
		return	( v.frequency() > _fmin ) && 
				( v.frequency() < _fmax ); 
	}
		
	//	constructor:
	can_mask( double x, double y ) : 
		_fmin( x ), _fmax( y ) 
		{ if (x>y) std::swap(x,y); }
		
	//	bounds:
private:
	double _fmin, _fmax;
};

// ---------------------------------------------------------------------------
//	negative_time
// ---------------------------------------------------------------------------
//	functor used to identify peaks that have reassigned times 
//	before 0:
struct negative_time
{
	//	negative times occur when the reassigned time
	// 	plus the current frame time is less than 0:
	bool operator()( const Peaks::value_type & v )  const
	{ 
		return 0 > ( v.time() + _frameTime );
	}
		
	//	constructor:
	negative_time( double t ) : 
		_frameTime( t ) {}
		
	//	bounds:
private:
	double _frameTime;
	
};


// ---------------------------------------------------------------------------
//	thinPeaks (HELPER)
// ---------------------------------------------------------------------------
//	Reject peaks that are too quiet (low amplitude). Peaks that are retained,
//	but are quiet enough to be in the specified fadeRange should be faded.
//	Peaks having negative times are also rejected.
//
//	This is exactly the same as the basic peak selection strategy, there
//	is no tracking here.
//	
//	Rejected peaks are placed at the end of the peak collection.
//	Return the first position in the collection containing a rejected peak,
//	or the end of the collection if no peaks are rejected.
//
//	This used to be part of SpectralPeakSelector, but it really had no place
//	there. It _should_ remove the rejected peaks, but for now, those are needed
//	by the bandwidth association strategy.
//
Peaks::iterator 
Analyzer::thinPeaks( Peaks & peaks, double frameTime  )
{
	const double ampFloordB = m_ampFloor;

	//  fade quiet peaks out over 10 dB:
	const double fadeRangedB = 10.0;

	//	compute absolute magnitude thresholds:
	const double threshold = std::pow( 10., 0.05 * ampFloordB );
	const double beginFade = std::pow( 10., 0.05 * (ampFloordB+fadeRangedB) );

	//	louder peaks are preferred, so consider them 
	//	in order of louder magnitude:
	std::sort( peaks.begin(), peaks.end(), SpectralPeak::sort_greater_amplitude );
	
    //  negative times are not real, but still might represent
    //  a noisy part of the spectrum...
	Peaks::iterator bogusTimes = 
		std::remove_if( peaks.begin(), peaks.end(), negative_time( frameTime ) );
		
	//	...get rid of them anyway
    peaks.erase( bogusTimes, peaks.end() );
    bogusTimes = peaks.end();
        
    
	Peaks::iterator it = peaks.begin();
	Peaks::iterator beginRejected = it;

    const double freqResolution = 
    	std::max( m_freqResolutionEnv->valueAt( frameTime ), 0.0 ); 
    
    
	while ( it != peaks.end() ) 
	{
		SpectralPeak & pk = *it;
		
		//	keep this peak if it is loud enough and not
		//	 too near in frequency to a louder one:
		double lower = pk.frequency() - freqResolution;
		double upper = pk.frequency() + freqResolution;
		if ( pk.amplitude() > threshold &&
			 beginRejected == std::find_if( peaks.begin(), beginRejected, can_mask(lower, upper) ) )
		{
			//	this peak is a keeper, fade its
			//	amplitude if it is too quiet:
			if ( pk.amplitude() < beginFade )
			{
				double alpha = (beginFade - pk.amplitude())/(beginFade - threshold);
				pk.setAmplitude( pk.amplitude() * (1. - alpha) );
			}
			
			//	keep retained peaks at the front of the collection:
			if ( it != beginRejected )
			{
				std::swap( *it, *beginRejected );
			}
			++beginRejected;
		}
		++it;
	}
	
	// debugger << "thinPeaks retained " << std::distance( peaks.begin(), beginRejected ) << endl;

	//  remove rejected Breakpoints:
	//peaks.erase( beginRejected, peaks.end() );
	
	return beginRejected;
}

// ---------------------------------------------------------------------------
//	fixBandwidth (HELPER)
// ---------------------------------------------------------------------------
//	Fix the bandwidth value stored in the specified Peaks. 
//	This function is invoked if the spectral residue method is
//	not used to compute bandwidth (that method overwrites the
//	bandwidth already). If the convergence method is used to 
//	compute bandwidth, the appropriate scaling is applied
//	to the stored mixed phase derivative. Otherwise, the
//	Peak bandwidth is set to zero.
//
//  The convergence value is on the range [0,1], 0 for a sinusoid, 
//  and 1 for an impulse. If convergence tolerance is specified (as
//  a negative value in m_bwAssocParam), it should be positive and 
//  less than 1, and specifies the convergence value that is to 
//  correspond to bandwidth equal to 1.0. This is achieved by scaling
//  the convergence by the inverse of the tolerance, and saturating
//  at 1.0.
void Analyzer::fixBandwidth( Peaks & peaks )
{
	
	if ( m_bwAssocParam < 0 )
	{
		double scale = 1.0 / (- m_bwAssocParam);	
			// m_bwAssocParam stores negative tolerance
	
		for ( Peaks::iterator it = peaks.begin(); it != peaks.end(); ++it )
		{
            SpectralPeak & pk = *it;
			pk.setBandwidth( std::min( 1.0, scale * pk.bandwidth() ) );
		}
	}
	else if ( m_bwAssocParam == 0 )
	{
		for ( Peaks::iterator it = peaks.begin(); it != peaks.end(); ++it )
		{
            SpectralPeak & pk = *it;
			pk.setBandwidth( 0 );
		}
	}
}

}   //  end of namespace Loris
