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
 * SpectralSurface.C
 *
 * Implementation of class SpectralSurface, a class representing 
 * a smoothed time-frequency surface that can be used to 
 * perform cross-synthesis, the filtering of one sound by the
 * time-varying spectrum of another.
 *
 * Kelly Fitz, 21 Dec 2005
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */
#include "SpectralSurface.h"

#include "BreakpointUtils.h"
#include "LorisExceptions.h"
#include "Notifier.h"
#include "Partial.h"

#include <algorithm>
#include <iterator>

namespace Loris {

// ---------------------------------------------------------------------------
//    peakAmp - local helper for addPartialAux
// ---------------------------------------------------------------------------
static double peakAmp( const Partial & p ) 
{	
	if ( 0 == p.numBreakpoints() )
	{
		return 0;
	}
	return std::max_element( p.begin(), p.end(), 
							 BreakpointUtils::compareAmplitudeLess()
							 ).breakpoint().amplitude();
}

// ---------------------------------------------------------------------------
//    findemfaster - local helper
// ---------------------------------------------------------------------------
static std::pair< const Partial *, const Partial * > 
findemfaster( double freq, double time, const std::vector< Partial > & parray )
{
	static std::vector< Partial >::size_type cacheLastHit = 0;
	
	std::vector< Partial >::size_type i = cacheLastHit;
	const Partial * p1 = 0;
	const Partial * p2 = 0;
	if ( parray[i].frequencyAt( time ) < freq )
	{
		// search up the list
		while ( i < parray.size() && parray[i].frequencyAt( time ) < freq )
		{
			++i;
		}
		if ( i > 0 )
		{
			p1 = &parray[i-1];
			cacheLastHit = i-1;
		}
		else
		{
			p1 = 0;
			cacheLastHit = 0;
		}
		if ( i < parray.size() )
		{
			p2 = &parray[i];
		}
		else
		{
			p2 = 0;
		}
	}
	else
	{
		// search down the list
		while ( i > 0 && parray[i].frequencyAt( time ) > freq )
		{
			--i;
		}
		if ( i > 0 || parray[i].frequencyAt( time ) < freq )
		{
			p1 = &parray[i];
			cacheLastHit = i;
		}
		else
		{
			p1 = 0;
			cacheLastHit = 0;
		}
		if ( i + 1 < parray.size() )
		{
			p2 = &parray[i+1];
		}
		else
		{
			p2 = 0;
		}
	}
	// debugger << "findemfaster caching " <<  cacheLastHit << endl;
	return std::make_pair(p1, p2);
}

// ---------------------------------------------------------------------------
//    smoothInTime - local helper
// ---------------------------------------------------------------------------
static double smoothInTime( const Partial & p, double t )
{
    const double spanT = 30; // ms
    const int steps = 13;
    const double incrT = (2 * spanT) / (steps - 1);
    
	double a = p.amplitudeAt( t );
	if ( 0 == a )
	{
		for (double dehr = -spanT; dehr <= spanT; dehr += incrT )
		{
			a += p.amplitudeAt( t + ( .001*dehr ) );
		}
		a = a / steps;
	}
	return a;	
}
	
// ---------------------------------------------------------------------------
//    surfaceAt - local helper
// ---------------------------------------------------------------------------
static double surfaceAt( double f, double t, const std::vector< Partial > & parray )
{
	std::pair< const Partial *, const Partial * > both = findemfaster( f, t, parray );
	const Partial * p1 = both.first;
	const Partial * p2 = both.second;
	
	double moo1 = 0, moo2 = 0, interp = 0;
	
	if ( 0 != p1 && 0 != p2 )
	{
		interp = (f - p1->frequencyAt( t )) / ( p2->frequencyAt( t ) - p1->frequencyAt( t ) );
		moo1 = smoothInTime( *p1, t );
		moo2 = smoothInTime( *p2, t );
	}
	else if ( 0 != p2 )
	{
		interp = 1;
		moo2 = smoothInTime( *p2, t );
		moo1 = moo2;
	}
	else if ( 0 != p1 )
	{
		interp = 1. / (f - p1->frequencyAt( t ));
		moo1 = smoothInTime( *p1, t );
		moo2 = 0;
	}
	else
	{
		moo1 = moo2 = interp = 0;
	}
	return ((1-interp)*moo1 + interp*moo2);
}

// ---------------------------------------------------------------------------
//    scaleAmplitudes
// ---------------------------------------------------------------------------
//! Scale the amplitude of every Breakpoint in a Partial
//! according to the amplitude of the spectral surface
//! at the corresponding time and frequency.
//!
//! \param  p the Partial to modify
//
void SpectralSurface::scaleAmplitudes( Partial & p )
{
	const double FreqScale = 1.0 / mStretchFreq;
	const double TimeScale = 1.0 / mStretchTime;

    Partial::iterator iter;
    for ( iter = p.begin(); iter != p.end(); ++iter )
    {
        Breakpoint & bp = iter.breakpoint();	
        double f = bp.frequency();
        double t = iter.time();	
            
        double ampscale = surfaceAt( FreqScale * f, TimeScale * t, mPartials ) / mMaxSurfaceAmp;

        double a = bp.amplitude() * ( (1.-mEffect) + (mEffect*ampscale) );
        bp.setAmplitude( a );
    }
}

// ---------------------------------------------------------------------------
//    setAmplitudes
// ---------------------------------------------------------------------------
//! Set the amplitude of every Breakpoint in a Partial
//! equal to the amplitude of the spectral surface
//! at the corresponding time and frequency.
//!
//! \param  p the Partial to modify
//
void SpectralSurface::setAmplitudes( Partial & p )
{
	const double FreqScale = 1.0 / mStretchFreq;
	const double TimeScale = 1.0 / mStretchTime;

    Partial::iterator iter;
    for ( iter = p.begin(); iter != p.end(); ++iter )
    {
        Breakpoint & bp = iter.breakpoint();	
        if ( 0 != bp.amplitude() )
        {
            double f = bp.frequency();
            double t = iter.time();	
                
            double surfaceAmp = surfaceAt( FreqScale * f, TimeScale * t, mPartials );
            double a = ( bp.amplitude()*(1.-mEffect) ) + ( mEffect*surfaceAmp );
            bp.setAmplitude( a );
        }
    }
}

// --- access/mutation ---

// ---------------------------------------------------------------------------
//    frequencyStretch
// ---------------------------------------------------------------------------
//! Return the amount of strecthing in the frequency dimension
//! (default 1, no stretching). Values greater than 1 stretch
//! the surface in the frequency dimension, values less than 1
//! (but greater than 0) compress the surface in the frequency
//! dimension.
//
double SpectralSurface::frequencyStretch( void ) const
{
	return mStretchFreq;
}

// ---------------------------------------------------------------------------
//    timeStretch
// ---------------------------------------------------------------------------
//! Return the amount of strecthing in the time dimension
//! (default 1, no stretching). Values greater than 1 stretch
//! the surface in the time dimension, values less than 1
//! (but greater than 0) compress the surface in the time
//! dimension.
//
double SpectralSurface::timeStretch( void ) const
{
	return mStretchTime;
}

// ---------------------------------------------------------------------------
//    effect
// ---------------------------------------------------------------------------
//! Return the amount of effect applied by scaleAmplitudes
//! and setAmplitudes (default 1, full effect). Values
//! less than 1 (but greater than 0) reduce the amount of
//! amplitude modified performed by application of the
//! surface. (This is rarely a good way of controlling the
//! amount of the effect.)
//
double SpectralSurface::effect( void ) const
{
	return mEffect;
}

// ---------------------------------------------------------------------------
//    setFrequencyStretch
// ---------------------------------------------------------------------------
//! Set the amount of strecthing in the frequency dimension
//! (default 1, no stretching). Values greater than 1 stretch
//! the surface in the frequency dimension, values less than 1
//! (but greater than 0) compress the surface in the frequency
//! dimension.
//!
//! \pre    stretch must be positive
//! \param  stretch the new stretch factor for the frequency dimension
//
void SpectralSurface::setFrequencyStretch( double stretch )
{
	if ( 0 > stretch )
	{
		Throw( InvalidArgument,     
               "SpectralSurface frequency stretch must be non-negative." );
	}
	mStretchFreq = stretch;
}

// ---------------------------------------------------------------------------
//    setTimeStretch
// ---------------------------------------------------------------------------
//! Set the amount of strecthing in the time dimension
//! (default 1, no stretching). Values greater than 1 stretch
//! the surface in the time dimension, values less than 1
//! (but greater than 0) compress the surface in the time
//! dimension.
//!
//! \pre    stretch must be positive
//! \param  stretch the new stretch factor for the time dimension
//
void SpectralSurface::setTimeStretch( double stretch )
{
	if ( 0 > stretch )
	{
		Throw( InvalidArgument,     
               "SpectralSurface time stretch must be non-negative." );
	}
	mStretchTime = stretch;
}

// ---------------------------------------------------------------------------
//    setEffect
// ---------------------------------------------------------------------------
//! Set the amount of effect applied by scaleAmplitudes
//! and setAmplitudes (default 1, full effect). Values
//! less than 1 (but greater than 0) reduce the amount of
//! amplitude modified performed by application of the
//! surface. (This is rarely a good way of controlling the
//! amount of the effect.)
//!
//! \pre    effect must be between 0 and 1, inclusive
//! \param  effect the new factor controlling the amount of 
//!         amplitude modification performed by scaleAmplitudes
//!         and setAmplitudes
//
void SpectralSurface::setEffect( double effect )
{
	if ( 0 > effect || 1 < effect )
	{
		Throw( InvalidArgument,     
               "SpectralSurface effect must be non-negative and not greater than 1." );
	}
	mEffect = effect;
}

// --- private helpers ---

// ---------------------------------------------------------------------------
//    addPartialAux
// ---------------------------------------------------------------------------
// Helper function used by constructor for adding Partials one
// by one. Still have to sort after adding all the Partials
// using this helper! This just adds the Partial and keeps track
// of the largest amplitude seen so far.
//
void SpectralSurface::addPartialAux( const Partial & p )
{
    mPartials.push_back( p );
    mMaxSurfaceAmp = std::max( mMaxSurfaceAmp, peakAmp( p ) );
}


}	//end namespace

