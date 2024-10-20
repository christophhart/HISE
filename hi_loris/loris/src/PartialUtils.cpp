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
 * PartialUtils.C
 *
 *	A group of Partial utility function objects for use with STL 
 *	searching and sorting algorithms. PartialUtils is a namespace
 *	within the Loris namespace.
 *
 * Kelly Fitz, 17 June 2003
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */

#if HAVE_CONFIG_H
	#include "config.h"
#endif

#include "PartialUtils.h"

#include "Breakpoint.h"
#include "BreakpointEnvelope.h"
#include "BreakpointUtils.h"
#include "Envelope.h"
#include "Partial.h"

#include "phasefix.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <utility>

//	begin namespace
namespace Loris {

namespace PartialUtils {


// -- base class --

// ---------------------------------------------------------------------------
//	PartialMutator constructor from double
// ---------------------------------------------------------------------------
PartialMutator::PartialMutator( double x ) : 
	env( new BreakpointEnvelope( x ) ) 
{
}
	
// ---------------------------------------------------------------------------
//	PartialMutator constructor from envelope
// ---------------------------------------------------------------------------
PartialMutator::PartialMutator( const Envelope & e ) : 
	env( e.clone() ) 
{
}

// ---------------------------------------------------------------------------
//	PartialMutator copy constructor
// ---------------------------------------------------------------------------
PartialMutator::PartialMutator( const PartialMutator & rhs ) : 
	env( rhs.env->clone() ) 
{
}

// ---------------------------------------------------------------------------
//	PartialMutator destructor
// ---------------------------------------------------------------------------
PartialMutator::~PartialMutator( void )
{
	delete env;
}

// ---------------------------------------------------------------------------
//	PartialMutator assignment operator
// ---------------------------------------------------------------------------
PartialMutator &
PartialMutator::operator=( const PartialMutator & rhs )
{
	if ( this != &rhs )
	{	
		delete env;
		env = rhs.env->clone();
	}
	return *this;
}
	
// -- amplitude scaling --
	
// ---------------------------------------------------------------------------
//	AmplitudeScaler function call operator
// ---------------------------------------------------------------------------
//	Scale the amplitude of the specified Partial according to
//	an envelope representing a time-varying amplitude scale value.
//
void 
AmplitudeScaler::operator()( Partial & p ) const
{
	for ( Partial::iterator pos = p.begin(); pos != p.end(); ++pos ) 
	{		
		pos.breakpoint().setAmplitude( pos.breakpoint().amplitude() * 
									          env->valueAt( pos.time() ) );
	}	
}

// ---------------------------------------------------------------------------
//	BandwidthScaler function call operator
// ---------------------------------------------------------------------------
//	Scale the bandwidth of the specified Partial according to
//	an envelope representing a time-varying bandwidth scale value.
//
void 
BandwidthScaler::operator()( Partial & p ) const
{
	for ( Partial::iterator pos = p.begin(); pos != p.end(); ++pos ) 
	{		
		pos.breakpoint().setBandwidth( pos.breakpoint().bandwidth() * 
									   env->valueAt( pos.time() ) );
	}	
}

// ---------------------------------------------------------------------------
//	BandwidthSetter function call operator
// ---------------------------------------------------------------------------
//	Set the bandwidth of the specified Partial according to
//	an envelope representing a time-varying bandwidth value.
//
void 
BandwidthSetter::operator()( Partial & p ) const
{
	for ( Partial::iterator pos = p.begin(); pos != p.end(); ++pos ) 
	{		
		pos.breakpoint().setBandwidth( env->valueAt( pos.time() ) );
	}	
}

// ---------------------------------------------------------------------------
//	FrequencyScaler function call operator
// ---------------------------------------------------------------------------
//	Scale the frequency of the specified Partial according to
//	an envelope representing a time-varying frequency scale value.
//
void 
FrequencyScaler::operator()( Partial & p ) const
{
	for ( Partial::iterator pos = p.begin(); pos != p.end(); ++pos ) 
	{		
		pos.breakpoint().setFrequency( pos.breakpoint().frequency() * 
									   env->valueAt( pos.time() ) );
	}	
}

// ---------------------------------------------------------------------------
//	NoiseRatioScaler function call operator
// ---------------------------------------------------------------------------
//	Scale the relative noise content of the specified Partial according 
//	to an envelope representing a (time-varying) noise energy 
//	scale value.
//
void 
NoiseRatioScaler::operator()( Partial & p ) const
{
	for ( Partial::iterator pos = p.begin(); pos != p.end(); ++pos ) 
	{		
		//	compute new bandwidth value:
		double bw = pos.breakpoint().bandwidth();
		if ( bw < 1. ) 
		{
			double ratio = bw  / (1. - bw);
			ratio *= env->valueAt( pos.time() );
			bw = ratio / ( 1. + ratio );
		}
		else 
		{
			bw = 1.;
		}		
		pos.breakpoint().setBandwidth( bw );
	}	
}

// ---------------------------------------------------------------------------
//	PitchShifter function call operator
// ---------------------------------------------------------------------------
//	Shift the pitch of the specified Partial according to 
//	the given pitch envelope. The pitch envelope is assumed to have 
//	units of cents (1/100 of a halfstep).
//
void 
PitchShifter::operator()( Partial & p ) const
{
	for ( Partial::iterator pos = p.begin(); pos != p.end(); ++pos ) 
	{		
		//	compute frequency scale:
		double scale = 
			std::pow( 2., ( 0.01 * env->valueAt( pos.time() ) ) / 12. );				
		pos.breakpoint().setFrequency( pos.breakpoint().frequency() * scale );
	}	
}

// ---------------------------------------------------------------------------
//	Cropper function call operator
// ---------------------------------------------------------------------------
//	Trim a Partial by removing Breakpoints outside a specified time span.
//	Insert a Breakpoint at the boundary when cropping occurs.
//
void 
Cropper::operator()( Partial & p ) const
{
	//	crop beginning of Partial
	Partial::iterator it = p.findAfter( minTime );
	if ( it != p.begin() )    // Partial begins earlier than minTime
	{
	    if ( it != p.end() ) // Partial ends later than minTime
	    {
            Breakpoint bp = p.parametersAt( minTime );
            it = p.insert( minTime, bp );
        }
		it = p.erase( p.begin(), it );
	}
	
	//	crop end of Partial
	it = p.findAfter( maxTime );
	if ( it != p.end() ) // Partial ends later than maxTime
	{
	    if ( it != p.begin() )   // Partial begins earlier than maxTime
	    {
	    	Breakpoint bp = p.parametersAt( maxTime );
    		it = p.insert( maxTime, bp );
    		++it;   //  advance, we don't want to cut this one off
		}
		it = p.erase( it, p.end() );
	}
}

// ---------------------------------------------------------------------------
//	TimeShifter function call operator
// ---------------------------------------------------------------------------
//	Shift the time of all the Breakpoints in a Partial by a constant amount.
//
void 
TimeShifter::operator()( Partial & p ) const
{
	//	Since the Breakpoint times are immutable, the only way to 
	//	shift the Partial in time is to construct a new Partial and
	//	assign it to the argument p.
	Partial result;
	result.setLabel( p.label() );
	
	for ( Partial::iterator pos = p.begin(); pos != p.end(); ++pos ) 
	{		
		result.insert( pos.time() + offset, pos.breakpoint() );
	}	
	p = result;
}

// ---------------------------------------------------------------------------
//	peakAmplitude
// ---------------------------------------------------------------------------
//! Return the maximum amplitude achieved by a partial. 
//!  
//! \param  p is the Partial to evaluate
//! \return the maximum (absolute) amplitude achieved by 
//!         the partial p
//
double peakAmplitude( const Partial & p )
{
    double peak = 0;
    for ( Partial::const_iterator it = p.begin();
          it != p.end();
          ++it )
    {
        peak = std::max( peak, it->amplitude() );
    }
    return peak;
}

// ---------------------------------------------------------------------------
//	avgAmplitude
// ---------------------------------------------------------------------------
//! Return the average amplitude over all Breakpoints in this Partial.
//! Return zero if the Partial has no Breakpoints.
//!  
//! \param  p is the Partial to evaluate
//! \return the average amplitude of Breakpoints in the Partial p
//
double avgAmplitude( const Partial & p )
{
    double avg = 0;
    for ( Partial::const_iterator it = p.begin();
          it != p.end();
          ++it )
    {
        avg += it->amplitude();
    }
    
    if ( avg != 0 )
    {
        avg /= p.numBreakpoints();
    }
    
    return avg;
}


// ---------------------------------------------------------------------------
//	avgFrequency
// ---------------------------------------------------------------------------
//! Return the average frequency over all Breakpoints in this Partial.
//! Return zero if the Partial has no Breakpoints.
//!  
//! \param  p is the Partial to evaluate
//! \return the average frequency (Hz) of Breakpoints in the Partial p
//
double avgFrequency( const Partial & p )
{
    double avg = 0;
    for ( Partial::const_iterator it = p.begin();
          it != p.end();
          ++it )
    {
        avg += it->frequency();
    }
    
    if ( avg != 0 )
    {
        avg /= p.numBreakpoints();
    }
    
    return avg;
}


// ---------------------------------------------------------------------------
//	weightedAvgFrequency
// ---------------------------------------------------------------------------
//! Return the average frequency over all Breakpoints in this Partial, 
//! weighted by the Breakpoint amplitudes.
//! Return zero if the Partial has no Breakpoints.
//!  
//! \param  p is the Partial to evaluate
//! \return the average frequency (Hz) of Breakpoints in the Partial p
//
double weightedAvgFrequency( const Partial & p )
{
    double avg = 0;
    double ampsum = 0;
    for ( Partial::const_iterator it = p.begin();
          it != p.end();
          ++it )
    {
        avg += it->amplitude() * it->frequency();
        ampsum += it->amplitude();
    }
    
    if ( avg != 0 && ampsum != 0 )
    {
        avg /= ampsum;
    }
    else
    {
        avg = 0;
    }
    
    return avg;
}

//	-- phase maintenance functions --

// ---------------------------------------------------------------------------
//	fixPhaseBefore
//
//! Recompute phases of all Breakpoints earlier than the specified time 
//! so that the synthesize phases of those earlier Breakpoints matches 
//! the stored phase, and the synthesized phase at the specified
//! time matches the stored (not recomputed) phase.
//! 
//! Backward phase-fixing stops if a null (zero-amplitude) Breakpoint
//! is encountered, because nulls are interpreted as phase reset points
//! in Loris. If a null is encountered, the remainder of the Partial
//! (the front part) is fixed in the forward direction, beginning at
//! the start of the Partial.
//!
//! \param p    The Partial whose phases should be fixed.
//! \param t    The time before which phases should be adjusted.
//
void fixPhaseBefore( Partial & p, double t )
{
    if ( 1 < p.numBreakpoints() )
    {
        Partial::iterator pos = p.findNearest( t );
        Assert( pos != p.end() );

        fixPhaseBackward( p.begin(), pos );
    }
}

// ---------------------------------------------------------------------------
//	fixPhaseAfter
//
//! Recompute phases of all Breakpoints later than the specified time 
//! so that the synthesize phases of those later Breakpoints matches 
//! the stored phase, as long as the synthesized phase at the specified
//! time matches the stored (not recomputed) phase.
//! 
//! Phase fixing is only applied to non-null (nonzero-amplitude) Breakpoints,
//! because null Breakpoints are interpreted as phase reset points in 
//! Loris. If a null is encountered, its phase is simply left unmodified,
//! and future phases wil be recomputed from that one.
//!
//! \param p    The Partial whose phases should be fixed.
//! \param t    The time after which phases should be adjusted.
//
void fixPhaseAfter( Partial & p, double t )
{
    //  nothing to do it there are not at least
    //  two Breakpoints in the Partial   
    if ( 1 < p.numBreakpoints() )
    {
        Partial::iterator pos = p.findNearest( t );
        Assert( pos != p.end() );

        fixPhaseForward( pos, --p.end() );
    }
}

// ---------------------------------------------------------------------------
//	fixPhaseForward
//
//! Recompute phases of all Breakpoints later than the specified time 
//! so that the synthesize phases of those later Breakpoints matches 
//! the stored phase, as long as the synthesized phase at the specified
//! time matches the stored (not recomputed) phase. Breakpoints later than
//! tend are unmodified.
//! 
//! Phase fixing is only applied to non-null (nonzero-amplitude) Breakpoints,
//! because null Breakpoints are interpreted as phase reset points in 
//! Loris. If a null is encountered, its phase is simply left unmodified,
//! and future phases wil be recomputed from that one.
//!
//! HEY Is this interesting, in general? Why would you want to do this?
//!
//! \param p    The Partial whose phases should be fixed.
//! \param tbeg The phases and frequencies of Breakpoints later than the 
//!             one nearest this time will be modified.
//! \param tend The phases and frequencies of Breakpoints earlier than the 
//!             one nearest this time will be modified. Should be greater 
//!             than tbeg, or else they will be swapped.
//
void fixPhaseForward( Partial & p, double tbeg, double tend )
{
    if ( tbeg > tend )
    {
        std::swap( tbeg, tend );
    }
    
    //  nothing to do it there are not at least
    //  two Breakpoints in the Partial   
    if ( 1 < p.numBreakpoints() )
    {
        //  find the positions nearest tbeg and tend
        Partial::iterator posbeg = p.findNearest( tbeg );
        Partial::iterator posend = p.findNearest( tend );
        
        //  if the positions are different, and tend is
        //  the end, back it up
        if ( posbeg != posend && posend == p.end() )
        {
            --posend;
        }
        fixPhaseForward( posbeg, posend );
    }
}

// ---------------------------------------------------------------------------
//	fixPhaseAt
//
//! Recompute phases of all Breakpoints in a Partial
//! so that the synthesize phases match the stored phases, 
//! and the synthesized phase at (nearest) the specified
//! time matches the stored (not recomputed) phase.
//! 
//! Backward phase-fixing stops if a null (zero-amplitude) Breakpoint
//! is encountered, because nulls are interpreted as phase reset points
//! in Loris. If a null is encountered, the remainder of the Partial
//! (the front part) is fixed in the forward direction, beginning at
//! the start of the Partial. Forward phase fixing is only applied 
//! to non-null (nonzero-amplitude) Breakpoints. If a null is encountered, 
//! its phase is simply left unmodified, and future phases wil be 
//! recomputed from that one.
//!
//! \param p    The Partial whose phases should be fixed.
//! \param t    The time at which phases should be made correct.
//
void fixPhaseAt( Partial & p, double t )
{
    if ( 1 < p.numBreakpoints() )
    {
        Partial::iterator pos = p.findNearest( t );
        Assert( pos != p.end() );

        fixPhaseForward( pos, --p.end() );
        fixPhaseBackward( p.begin(), pos );
    }
}

// ---------------------------------------------------------------------------
//  fixPhaseBetween
//
//!	Fix the phase travel between two times by adjusting the
//!	frequency and phase of Breakpoints between those two times.
//!
//!	This algorithm assumes that there is nothing interesting about the
//!	phases of the intervening Breakpoints, and modifies their frequencies 
//!	as little as possible to achieve the correct amount of phase travel 
//!	such that the frequencies and phases at the specified times
//!	match the stored values. The phases of all the Breakpoints between 
//! the specified times are recomputed.
//!
//! THIS DOES NOT YET TREAT NULL BREAKPOINTS DIFFERENTLY FROM OTHERS.
//!
//! \pre        There must be at least one Breakpoint in the
//!             Partial between the specified times tbeg and tend.
//! \post       The phases and frequencies of the Breakpoints in the 
//!             range have been recomputed such that an oscillator
//!             initialized to the parameters of the first Breakpoint
//!             will arrive at the parameters of the last Breakpoint,
//!             and all the intervening Breakpoints will be matched.
//!	\param p    The partial whose phases and frequencies will be recomputed. 
//!             The Breakpoint at this position is unaltered.
//! \param tbeg The phases and frequencies of Breakpoints later than the 
//!             one nearest this time will be modified.
//! \param tend The phases and frequencies of Breakpoints earlier than the 
//!             one nearest this time will be modified. Should be greater 
//!             than tbeg, or else they will be swapped.
//
void fixPhaseBetween( Partial & p, double tbeg, double tend )
{
    if ( tbeg > tend )
    {
        std::swap( tbeg, tend );
    }

    // for Partials that do not extend over the entire
    // specified time range, just recompute phases from
    // beginning or end of the range:
    if ( p.endTime() < tend )
    {
        // OK if start time is also after tbeg, will
        // just recompute phases from start of p.
        fixPhaseAfter( p, tbeg );
    }
    else if ( p.startTime() > tbeg )
    {
        fixPhaseBefore( p, tend );
    }
    else
    {
        // invariant:
        // p begins before tbeg and ends after tend.
        Partial::iterator b = p.findNearest( tbeg );
        Partial::iterator e = p.findNearest( tend );

        // if there is a null Breakpoint n between b and e, then
        // should fix forward from b to n, and backward from
        // e to n. Otherwise, do this complicated thing.
        Partial::iterator nullbp = std::find_if( b, e, BreakpointUtils::isNull );
        if ( nullbp != e )
        {
            fixPhaseForward( b, nullbp );
            fixPhaseBackward( nullbp, e );
        }
        else
        {
            fixPhaseBetween( b, e );
        }
    }
}

}	//	end of namespace PartialUtils

}	//	end of namespace Loris

