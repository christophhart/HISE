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
 * Channelizer.C
 *
 * Implementation of class Channelizer.
 *
 * Kelly Fitz, 21 July 2000
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */

#if HAVE_CONFIG_H
	#include "config.h"
#endif

#include "Channelizer.h"
#include "Envelope.h"
#include "LinearEnvelope.h"
#include "Partial.h"
#include "PartialList.h"
#include "Notifier.h"

#include <cmath>

//	begin namespace
namespace Loris {

// ---------------------------------------------------------------------------
//	Channelizer constructor from reference envelope
// ---------------------------------------------------------------------------
//!	Construct a new Channelizer using the specified reference
//!	Envelope to represent the a numbered channel. If the sound
//! being channelized is known to have detuned harmonics, a 
//! stretching factor can be specified (defaults to 0 for no 
//! stretching). The stretching factor can be computed using
//! the static member computeStretchFactor.
//!	
//!	\param 	refChanFreq is an Envelope representing the center frequency
//!		    of a channel.
//!	\param  refChanLabel is the corresponding channel number (i.e. 1
//!		    if refChanFreq is the lowest-frequency channel, and all 
//!		    other channels are harmonics of refChanFreq, or 2 if  
//!		    refChanFreq tracks the second harmonic, etc.).
//! \param  stretchFactor is a stretching factor to account for detuned 
//!         harmonics, default is 0. 
//!
//! \throw  InvalidArgument if refChanLabel is not positive.
//! \throw  InvalidArgument if stretchFactor is negative.
//
Channelizer::Channelizer( const Envelope & refChanFreq, int refChanLabel, double stretchFactor ) :
	_refChannelFreq( refChanFreq.clone() ),
	_refChannelLabel( refChanLabel ),
	_stretchFactor( stretchFactor ),
    _ampWeighting( 0 )
{
	if ( refChanLabel <= 0 )
	{
		Throw( InvalidArgument, "Channelizer reference label must be positive." );
	}
 	if ( stretchFactor < 0. )
	{
		Throw( InvalidArgument, "Channelizer stretch factor must be non-negative." );
	}
}

// ---------------------------------------------------------------------------
//	Channelizer constructor from constant reference frequency
// ---------------------------------------------------------------------------
//!	Construct a new Channelizer having a constant reference frequency.
//!	The specified frequency is the center frequency of the lowest-frequency
//!	channel (for a harmonic sound, the channel containing the fundamental 
//!	Partial.
//!
//!	\param	refFreq is the reference frequency (in Hz) corresponding
//!			to the first frequency channel.
//! \param  stretchFactor is a stretching factor to account for detuned 
//!         harmonics, default is 0. 
//!
//!	\throw	InvalidArgument is the reference frequency is not positive
Channelizer::Channelizer( double refFreq, double stretchFactor ) :
	_refChannelFreq( new LinearEnvelope( refFreq ) ),
	_refChannelLabel( 1 ),
	_stretchFactor( stretchFactor ),
    _ampWeighting( 0 )
{
	if ( refFreq <= 0 )
	{
		Throw( InvalidArgument, "Channelizer reference frequency must be positive." );
	}
 	if ( stretchFactor < 0. )
	{
		Throw( InvalidArgument, "Channelizer stretch factor must be non-negative." );
	}
}


// ---------------------------------------------------------------------------
//	Channelizer copy constructor 
// ---------------------------------------------------------------------------
//!	Construct a new Channelizer that is an exact copy of another.
//!	The copy represents the same set of frequency channels, constructed
//!	from the same reference Envelope and channel number.
//!	
//!	\param other is the Channelizer to copy
//
Channelizer::Channelizer( const Channelizer & other ) :
	_refChannelFreq( other._refChannelFreq->clone() ),
	_refChannelLabel( other._refChannelLabel ),
	_stretchFactor( other._stretchFactor ),
    _ampWeighting( other._ampWeighting )
{
}

// ---------------------------------------------------------------------------
//	Channelizer assignment 
// ---------------------------------------------------------------------------
//!	Assignment operator: make this Channelizer an exact copy of another. 
//!	This Channelizer is made to represent the same set of frequency channels, 
//!	constructed from the same reference Envelope and channel number as @a rhs.
//!
//!	\param rhs is the Channelizer to copy
//
Channelizer & 
Channelizer::operator=( const Channelizer & rhs )
{
	if ( &rhs != this )
	{
		_refChannelFreq.reset( rhs._refChannelFreq->clone() );
		_refChannelLabel = rhs._refChannelLabel;
		_stretchFactor = rhs._stretchFactor;
        _ampWeighting = rhs._ampWeighting;
	}
	return *this;
}


// ---------------------------------------------------------------------------
//	Channelizer destructor
// ---------------------------------------------------------------------------
//!	Destroy this Channelizer.
//
Channelizer::~Channelizer( void )
{
}

// ---------------------------------------------------------------------------
//	amplitudeWeighting
// ---------------------------------------------------------------------------
//! Return the exponent applied to amplitude before weighting
//! the instantaneous estimate of the frequency channel number
//! for a Partial. zero (default) for no weighting, 1 for linear
//! amplitude weighting, 2 for power weighting, etc.
//! Amplitude weighting is a bad idea for many sounds, particularly
//! those with transients, for which it may emphasize the part of
//! the Partial having the least reliable frequency estimate.
//
double Channelizer::amplitudeWeighting( void ) const
{
    return _ampWeighting;
}
    
// ---------------------------------------------------------------------------
//	setAmplitudeWeighting
// ---------------------------------------------------------------------------
//! Set the exponent applied to amplitude before weighting
//! the instantaneous estimate of the frequency channel number
//! for a Partial. zero (default) for no weighting, 1 for linear
//! amplitude weighting, 2 for power weighting, etc.
//! Amplitude weighting is a bad idea for many sounds, particularly
//! those with transients, for which it may emphasize the part of
//! the Partial having the least reliable frequency estimate.
//
void Channelizer::setAmplitudeWeighting( double x )
{
    _ampWeighting = x;
}

// ---------------------------------------------------------------------------
//	stretchFactor
// ---------------------------------------------------------------------------
//! Return the stretching factor used to account for detuned
//! harmonics, as in a piano tone. Normally set to 0 for 
//! in-tune harmonics. 
//!
//! The stretching factor is a small positive number for 
//! heavy vibrating strings (as in pianos) for which the
//! mass of the string significantly affects the frequency
//! of the vibrating modes. See Martin Keane, "Understanding
//! the complex nature of the piano tone", 2004, for a discussion
//! and the source of the mode frequency stretching algorithms 
//! implemented here.
//
double Channelizer::stretchFactor( void ) const
{
    return _stretchFactor;
}

// ---------------------------------------------------------------------------
//	setStretchFactor
// ---------------------------------------------------------------------------
//! Set the stretching factor used to account for detuned
//! harmonics, as in a piano tone. Normally set to 0 for 
//! in-tune harmonics. The stretching factor for massy 
//! vibrating strings (like pianos) can be computed from 
//! the physical characteristics of the string, or using 
//! computeStretchFactor().
//!
//! The stretching factor is a small positive number for 
//! heavy vibrating strings (as in pianos) for which the
//! mass of the string significantly affects the frequency
//! of the vibrating modes. See Martin Keane, "Understanding
//! the complex nature of the piano tone", 2004, for a discussion
//! and the source of the mode frequency stretching algorithms 
//! implemented here.
//!
//! \throw  InvalidArgument if stretch is negative.
//
void Channelizer::setStretchFactor( double stretch )
{
 	if ( stretch < 0. )
	{
		Throw( InvalidArgument, "Channelizer stretch factor must be non-negative." );
	}
    _stretchFactor = stretch;
}

// ---------------------------------------------------------------------------
//	setStretchFactor
// ---------------------------------------------------------------------------
//! Set the stretching factor used to account for (consistently) 
//! detuned harmonics, as in a piano tone, from a pair of 
//! mode (harmonic) frequencies and numbers.
//!
//! The stretching factor is a small positive number for 
//! heavy vibrating strings (as in pianos) for which the
//! mass of the string significantly affects the frequency
//! of the vibrating modes. See Martin Keane, "Understanding
//! the complex nature of the piano tone", 2004, for a discussion
//! and the source of the mode frequency stretching algorithms 
//! implemented here.
//!
//! The stretching factor is computed using computeStretchFactor,
//! but only a valid stretch factor will ever be assigned. If an
//! invalid (negative) stretching factor is computed for the
//! specified frequencies and mode numbers, the stretch factor
//! will be set to zero.
//!
//! \param      fm is the frequency of the Mth stretched harmonic
//! \param      m is the harmonic number of the harmonic whose frequnecy is fm
//! \param      fn is the frequency of the Nth stretched harmonic
//! \param      n is the harmonic number of the harmonic whose frequnecy is fn
void Channelizer::setStretchFactor( double fm, int m, double fn, int n )
{
    const double B = computeStretchFactor( fm, m, fn, n );
    if ( 0 < B )
    {
        _stretchFactor = B;
    }
    else
    {
        _stretchFactor = 0;
    }
}

// ---------------------------------------------------------------------------
//	computeStretchFactor (STATIC class member)
// ---------------------------------------------------------------------------
//! Static member to compute the stretch factor for a sound having
//! (consistently) detuned harmonics, like piano tones.
//!
//! The stretching factor is a small positive number for 
//! heavy vibrating strings (as in pianos) for which the
//! mass of the string significantly affects the frequency
//! of the vibrating modes. See Martin Keane, "Understanding
//! the complex nature of the piano tone", 2004, for a discussion
//! and the source of the mode frequency stretching algorithms 
//! implemented here.
//!
//! The value returned by this function MAY NOT be a valid stretch
//! factor. If this function returns a negative stretch factor,
//! then the specified pair of frequencies and mode numbers cannot
//! be used to estimate the effects of string mass on mode frequency
//! (because the negative stretch factor implies a physical 
//! impossibility, like negative mass or negative length). 
//!
//! \param      fm is the frequency of the Mth stretched harmonic
//! \param      m is the harmonic number of the harmonic whose frequnecy is fm
//! \param      fn is the frequency of the Nth stretched harmonic
//! \param      n is the harmonic number of the harmonic whose frequnecy is fn
//! \returns    the stretching factor, usually a very small positive
//!             floating point number, or 0 for pefectly tuned harmonics
//!             (that is, if fn = n*f1).
//
double 
Channelizer::computeStretchFactor( double fm, int m, double fn, int n )
{
 	if ( fm <= 0. || fn <= 0. )
	{
		Throw( InvalidArgument, "Channelizer stretched harmonic frequencies must be positive." );
	}
 	if ( m <= 0 || n <= 0 )
	{
		Throw( InvalidArgument, "Channelizer stretched harmonic numbers must be positive." );
	}
   
    //  K is a factor that depends on the frequencies 
    //  of the two stretched harmonics, equal to 1.0 for
    //  perfectly tuned (not stretched) harmonics
    const double K = (m*fn) / (n*fm);

    const double num = 1. - (K*K);
    const double denom = (K*K*m*m) - (n*n);
/*
    OLD and wrong I think
    double num = (fn*fn) - (n*n*fref*fref);
    double denom = (n*n*n*n)*(fref*fref);
*/
    
    return num / denom;
}

// ---------------------------------------------------------------------------
//	computeStretchFactor (STATIC class member)
// ---------------------------------------------------------------------------
//! Static member to compute the stretch factor for a sound having
//! (consistently) detuned harmonics, like piano tones. Legacy version
//! that assumes the first argument corresponds to the first partial.
//!
//! \param      f1 is the frequency of the lowest numbered (1) partial.
//! \param      fn is the frequency of the Nth stretched harmonic
//! \param      n is the harmonic number of the harmonic whose frequnecy is fn
//! \returns    the stretching factor, usually a very small positive
//!             floating point number, or 0 for pefectly tuned harmonics
//!             (that is, for harmonic frequencies fn = n*f1).
//
double 
Channelizer::computeStretchFactor( double f1, double fn, double n )
{
    return computeStretchFactor( f1, 1, fn, int(n + 0.5) );
}

// ---------------------------------------------------------------------------
//	referenceFrequencyAt
// ---------------------------------------------------------------------------
//! Compute the reference frequency at the specified time. For non-stretched 
//! harmonics, this is simply the ratio of the reference envelope evaluated 
//! at that time to the reference channel number, and is the center frequecy
//! for the lowest channel. For stretched harmonics, the reference frequency 
//! is NOT equal to the center frequency of any of the channels, and is also
//! a function of the stretch factor. 
//
double Channelizer::referenceFrequencyAt( double time ) const
{
    const double N = _refChannelLabel;
    double fref = _refChannelFreq->valueAt( time ) / N;
    
    if ( 0 != _stretchFactor )
    {
        double divisor = std::sqrt( 1.0 + ( _stretchFactor*N*N) );
        fref = fref / divisor;
    }
    
    return fref;
}

// ---------------------------------------------------------------------------
//	computeFractionalChannelNumber
// ---------------------------------------------------------------------------
//! Compute the (fractional) channel number estimate for a Partial having a
//! given frequency at a specified time. For ordinary harmonics, this
//! is simply the ratio of the specified frequency to the reference
//! frequency at the specified time. For stretched harmonics (as in 
//! a piano), the stretching factor is used to compute the frequency
//! of the corresponding modes of a massy string. See Martin Keane, 
//! "Understanding the complex nature of the piano tone", 2004, for 
//! the source of the mode frequency stretching algorithms 
//! implemented here.
//!
//! The fractional channel number is used internally to determine
//! a best estimate for the channel number (label) for a Partial
//! having time-varying frequency. 
//!
//! \param  time is the time (in seconds) at which to evalute 
//!         the reference envelope
//! \param  frequency is the frequency (in Hz) for wihch the channel
//!         number is to be determined
//! \return the fractional channel number corresponding to the specified
//!         frequency and time
//
double 
Channelizer::computeFractionalChannelNumber( double time, double frequency ) const
{
    double refFreq = referenceFrequencyAt( time );
    
    if ( 0 == _stretchFactor )
    {
        return frequency / refFreq;
    }
    
    /*
    const double frefsqrd = fref*fref;
    double num = sqrt( (frefsqrd*frefsqrd) + (4*stretch*frefsqrd*fn*fn) ) - (frefsqrd);
    double denom = 2*stretch*frefsqrd;
    return sqrt( num / denom );
    */
    
    //  else:
    //  avoid squaring big numbers... two sqrts kind of sucks too.
    const double rB = 1. / _stretchFactor;     // reciprocal of B, the stretch factor
    const double fratio = frequency / refFreq;
    return std::sqrt( std::sqrt( (.25 * rB * rB) + (fratio * fratio * rB) ) - (.5 * rB) );
}

// ---------------------------------------------------------------------------
//	computeChannelNumber
// ---------------------------------------------------------------------------
//! Compute the (fractional) channel number estimate for a Partial having a
//! given frequency at a specified time. For ordinary harmonics, this
//! is simply the ratio of the specified frequency to the reference
//! frequency at the specified time. For stretched harmonics (as in 
//! a piano), the stretching factor is used to compute the frequency
//! of the corresponding modes of a massy string. See Martin Keane, 
//! "Understanding the complex nature of the piano tone", 2004, for 
//! the source of the mode frequency stretching algorithms 
//! implemented here.
//! 
//! \param  time is the time (in seconds) at which to evalute 
//!         the reference envelope
//! \param  frequency is the frequency (in Hz) for wihch the channel
//!         number is to be determined
//! \return the channel number corresponding to the specified
//!         frequency and time
//
int 
Channelizer::computeChannelNumber( double time, double frequency ) const
{    
    return int( computeFractionalChannelNumber( time, frequency ) + 0.5 );
}

// ---------------------------------------------------------------------------
//	channelFrequencyAt
// ---------------------------------------------------------------------------
//! Compute the center frequency of one a channel at the specified
//! time. For non-stretched harmonics, this is simply the value
//! of the reference envelope scaled by the ratio of the specified
//! channel number to the reference channel number. For stretched
//! harmonics, the channel center frequency is computed using the
//! stretch factor. See Martin Keane, "Understanding
//! the complex nature of the piano tone", 2004, for a discussion
//! and the source of the mode frequency stretching algorithms 
//! implemented here.
//!
//! \param  time is the time (in seconds) at which to evalute 
//!         the reference envelope
//! \param  channel is the frequency channel (or harmonic, or vibrational     
//!         mode) number whose frequency is to be determined
//! \return the center frequency in Hz of the specified frequency channel
//!         at the specified time
//
double 
Channelizer::channelFrequencyAt( double time, int channel ) const
{
    const double fref = referenceFrequencyAt( time );
    double fn = channel * fref;
    
    if ( 0 != _stretchFactor )
    {
        const double scale = std::sqrt( 1.0 + (_stretchFactor*channel*channel) );
        fn = fn * scale;
    }
    return fn;
}

// ---------------------------------------------------------------------------
//	channelize (one Partial)
// ---------------------------------------------------------------------------
//!	Label a Partial with the number of the frequency channel corresponding to
//!	the average frequency over all the Partial's Breakpoints.
//!	
//!	\param partial is the Partial to label.
//
void
Channelizer::channelize( Partial & partial ) const
{
    using std::pow;

	debugger << "channelizing Partial with " << partial.numBreakpoints() << " Breakpoints" << endl;
			
	//	compute an amplitude-weighted average channel
	//	label for each Partial:
	//double ampsum = 0.;
	double weightedlabel = 0.;
	Partial::const_iterator bp;
	for ( bp = partial.begin(); bp != partial.end(); ++bp )
	{
				
		double f = bp.breakpoint().frequency();
		double t = bp.time();
		
        double weight = 1;
        if ( 0 != _ampWeighting )
        {
            //  This used to be an amplitude-weighted avg, but for many sounds, 
            //  particularly those for which the weighted avg would be very
            //  different from the simple avg, the amplitude-weighted avg
            //  emphasized the part of the sound in which the frequency estimates
            //  are least reliable (e.g. a piano tone). The unweighted 
            //  average should give more intuitive results in most cases.

            //	use sinusoidal amplitude:
            double a = bp.breakpoint().amplitude() * std::sqrt( 1. - bp.breakpoint().bandwidth() );                
            weight = pow( a, _ampWeighting );
        }
        
        weightedlabel += weight * computeFractionalChannelNumber( t, f );
	}
	
	int label = 0;
	if ( 0 < partial.numBreakpoints() ) //  should always be the case
	{        
		label = (int)((weightedlabel / partial.numBreakpoints()) + 0.5);
	}
	Assert( label >= 0 );
			
	//	assign label, and remember it, but
	//	only if it is a valid (positive) 
	//	distillation label:
	partial.setLabel( label );

}

// -- simplified interface --


// ---------------------------------------------------------------------------
//	channelize (static simplified interface)
// ---------------------------------------------------------------------------
//! Static member that constructs an instance and applies
//! it to a PartialList (simplified interface). 
//!
//! Construct a Channelizer using the specified Envelope
//! and reference label, and use it to channelize a
//! sequence of Partials. 
//!
//! \param  partials is the sequence of Partials to 
//!         channelize.
//! \param  refChanFreq is an Envelope representing the center frequency
//!         of a channel.
//! \param  refChanLabel is the corresponding channel number (i.e. 1
//!         if refChanFreq is the lowest-frequency channel, and all 
//!         other channels are harmonics of refChanFreq, or 2 if  
//!         refChanFreq tracks the second harmonic, etc.).
//! \throw  InvalidArgument if refChanLabel is not positive.
//
void 
Channelizer::channelize( PartialList & partials,
                         const Envelope & refChanFreq, int refChanLabel )
{
    Channelizer instance( refChanFreq, refChanLabel );

    for ( PartialList::iterator it = partials.begin(); it != partials.end(); ++it )
    {
        instance.channelize( *it );
    }
}


}	//	end of namespace Loris
