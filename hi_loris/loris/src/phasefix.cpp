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
 * phasefix.C
 *
 * Implements a phase correction algorithm that perturbs slightly the 
 * frequencies or Breakpoints in a Partial so that the rendered Partial 
 * will achieve (or be closer to) the analyzed Breakpoint phases.
 *
 * Kelly Fitz, 23 Sept 04
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */
#if HAVE_CONFIG_H
	#include "config.h"
#endif

#include "phasefix.h"

#include "Breakpoint.h"
#include "BreakpointUtils.h"
#include "LorisExceptions.h"
#include "Notifier.h"
#include "Partial.h"


#include <algorithm>
#include <cmath>
#include <iostream>
#include <utility>

#if defined(HAVE_M_PI) && (HAVE_M_PI)
	const double Pi = M_PI;
#else
	const double Pi = 3.14159265358979324;
#endif


//	begin namespace
namespace Loris {

// -- local helpers -- 


// ---------------------------------------------------------------------------
//  wrapPi
//	Wrap an unwrapped phase value to the range [-pi,pi] using
//  O'Donnell's phase wrapping function.
//
double wrapPi( double x )
{
    using namespace std; // floor should be in std
    #define ROUND(x) (floor(.5 + (x)))
    const double TwoPi = 2.0*Pi;
    return x + ( TwoPi * ROUND(-x/TwoPi) );
}



// ---------------------------------------------------------------------------
//	phaseTravel
//
//	Compute the sinusoidal phase travel between two Breakpoints.
//	Return the total unwrapped phase travel.
//
double phaseTravel( const Breakpoint & bp0, const Breakpoint & bp1, 
					double dt )
{
	double f0 = bp0.frequency();
	double f1 = bp1.frequency();
	double favg = .5 * ( f0 + f1 );
	return 2 * Pi * favg * dt;
}

// ---------------------------------------------------------------------------
//	phaseTravel
//
//	Compute the sinusoidal phase travel between two Breakpoints.
//	Return the total unwrapped phase travel.
//
static double phaseTravel( Partial::const_iterator bp0, Partial::const_iterator bp1 )
{
	return phaseTravel( bp0.breakpoint(), bp1.breakpoint(), 
	                    bp1.time() - bp0.time() );
}

// -- phase correction -- 

// ---------------------------------------------------------------------------
//	fixPhaseBackward
//
//! Recompute phases of all Breakpoints on the half-open range [stopHere, pos)
//! so that the synthesized phases of those Breakpoints matches 
//! the stored phase, as long as the synthesized phase at stopHere
//! matches the stored (not recomputed) phase.
//!
//! The phase is corrected beginning at the end of the range, maintaining
//! the stored phase in the Breakpoint at pos.
//!
//! Backward phase-fixing stops if a null (zero-amplitude) Breakpoint
//! is encountered, because nulls are interpreted as phase reset points
//! in Loris. If a null is encountered, the remainder of the range
//! (the front part) is fixed in the forward direction, beginning at
//! the start of the stopHere.
//!
//! \pre    pos and stopHere are iterators on the same Partial, and
//!         pos must be not later than stopHere.
//! \pre    pos cannot be end of the Partial, it must be the postion
//!         of a valid Breakpoint.
//! \param  stopHere the position of the earliest Breakpoint whose phase might be
//!         recomputed.
//! \param  pos the position of a (later) Breakpoint whose phase is to be matched.
//!         The phase at pos is not modified.
//
void fixPhaseBackward( Partial::iterator stopHere, Partial::iterator pos )
{
    while ( pos != stopHere && 
            BreakpointUtils::isNonNull( pos.breakpoint() ) )
    {
        // pos is not the first Breakpoint in the Partial, 
        // and pos is not a Null Breakpoint.
        // Compute the correct phase for the
        // predecessor of pos.
        Partial::iterator posFwd = pos--;
        double travel = phaseTravel( pos, posFwd );
        pos.breakpoint().setPhase( wrapPi( posFwd.breakpoint().phase() - travel ) );
    }
    
    // if a null was encountered, then stop fixing backwards,
    // and fix the front of the Partial in the forward direction:
    if ( pos != stopHere )
    {
        // pos is not the first Breakpoint in the Partial,
        // and it is a Null Breakpoint (zero amplitude),
        // so it will be used to reset the phase during 
        // synthesis. 
        // The phase of all Breakpoints starting with pos
        // and ending with the Breakpoint nearest to time t
        // has been corrected. 
        // Fix phases before pos starting at the beginning
        // of the Partial.
        //
        // Dont fix pos, it has already been fixed.
        fixPhaseForward( stopHere, --pos );
    }
}

// ----------------------------------------------------------------------- ----
//	fixPhaseForward
//
//! Recompute phases of all Breakpoints on the closed range [pos, stopHere]
//! so that the synthesized phases of those Breakpoints matches 
//! the stored phase, as long as the synthesized phase at pos
//! matches the stored (not recomputed) phase. The phase at pos 
//! is modified only if pos is the position of a null Breakpoint
//! and the Breakpoint that follows is non-null.
//! 
//! Phase fixing is only applied to non-null (nonzero-amplitude) Breakpoints,
//! because null Breakpoints are interpreted as phase reset points in 
//! Loris. If a null is encountered, its phase is corrected from its non-Null
//! successor, if it has one, otherwise it is unmodified.
//!
//! \pre    pos and stopHere are iterators on the same Partial, and
//!         pos must be not later than stopHere.
//! \pre    stopHere cannot be end of the Partial, it must be the postion
//!         of a valid Breakpoint.
//! \param  pos the position of the first Breakpoint whose phase might be
//!         recomputed.
//! \param  stopHere the position of the last Breakpoint whose phase might
//!         be modified.
//
void fixPhaseForward( Partial::iterator pos, Partial::iterator stopHere )
{
    while ( pos != stopHere )
    {
        Partial::iterator posPrev = pos++;
        
        //  update phase based on the phase travel between 
        //  posPrev and pos UNLESS pos is Null:
        if ( BreakpointUtils::isNonNull( pos.breakpoint() ) )
        {
            // pos is the position of a non-Null Breakpoint, 
            // posPrev is its predecessor:            
            double travel = phaseTravel( posPrev, pos );
            
            if ( BreakpointUtils::isNonNull( posPrev.breakpoint() ) )
            {                        
                // if its predecessor of pos is non-Null, then fix
                // the phase of the Breakpoint at pos.
                pos.breakpoint().setPhase( wrapPi( posPrev.breakpoint().phase() + travel ) );
            }
            else
            {
                // if the predecessor of pos is Null, then
                // correct the predecessor's phase so that 
                // it correctly resets the synthesis phase 
                // so that the phase of the Breakpoint at 
                // pos is achieved in synthesis.
                posPrev.breakpoint().setPhase( wrapPi( pos.breakpoint().phase() - travel ) );
            }
        }
    }
}


// ---------------------------------------------------------------------------
//  fixPhaseBetween
//
//!	Fix the phase travel between two Breakpoints by adjusting the
//!	frequency and phase of Breakpoints between those two.
//!
//!	This algorithm assumes that there is nothing interesting about the
//!	phases of the intervening Breakpoints, and modifies their frequencies 
//!	as little as possible to achieve the correct amount of phase travel 
//!	such that the frequencies and phases at the specified times
//!	match the stored values. The phases of all the Breakpoints between 
//! the specified times are recomputed.
//!
//! Null Breakpoints are treated the same as non-null Breakpoints.
//!
//! \pre        b and e are iterators on the same Partials, and
//!             e must not preceed b in that Partial.
//! \pre        There must be at least one Breakpoint in the
//!             Partial between b and e.
//! \post       The phases and frequencies of the Breakpoints in the 
//!             range have been recomputed such that an oscillator
//!             initialized to the parameters of the first Breakpoint
//!             will arrive at the parameters of the last Breakpoint,
//!             and all the intervening Breakpoints will be matched.
//! \param b    The phases and frequencies of Breakpoints later than
//!             this one may be modified.
//! \param e    The phases and frequencies of Breakpoints earlier than  
//!             this one may be modified.
//
void fixPhaseBetween( Partial::iterator b, Partial::iterator e )
{  
    if ( 1 < std::distance( b, e ) )
    {
        //	Accumulate the actual phase travel over the Breakpoint
        //	span, and count the envelope segments.
        double travel = 0;
        Partial::iterator next = b;
        do
        {
            Partial::iterator prev = next++;
            travel += phaseTravel( prev, next );
        } while( next != e );

        //	Compute the desired amount of phase travel:
        double deviation = wrapPi( e.breakpoint().phase() - ( b.breakpoint().phase() + travel ) );
        double desired = travel + deviation;
        
        //	Compute the amount by which to perturb the frequencies of
        //	all the null Breakpoints between b and e.
        //
        //	The accumulated phase travel is the sum of the average frequency
        //	(in radians) of each segment times the duration of each segment
        //	(the actual phase travel is computed this way). If this sum is
        //	computed with each Breakpoint frequency perturbed (additively) 
        //	by delta, and set equal to the desired phase travel, then it
        //	can be simplified to:
        //		delta = 2 * ( phase error ) / ( tN + tN-1 - t1 - t0 )
        //	where tN is the time of e, tN-1 is the time of its predecessor,
        //	t0 is the time of b, and t1 is the time of b's successor.
        //
        Partial::iterator iter = b;
        double t0 = iter.time();
        ++iter;
        double t1 = iter.time();
        iter = e;
        double tN = iter.time();
        --iter;
        double tNm1 = iter.time();
        
        Assert( t1 < tN );	//	else there were no Breakpoints in between
        
        double delta = ( 2 * ( desired - travel ) / ( tN + tNm1 - t1 - t0 ) ) / ( 2 * Pi );
        
        //	Perturb the Breakpoint frequencies.
        next = b;
        Partial::iterator prev = next++;
        while ( next != e )
        {
            next.breakpoint().setFrequency( next.breakpoint().frequency() + delta );
            
            double newtravel = phaseTravel( prev, next );            
            next.breakpoint().setPhase( wrapPi( prev.breakpoint().phase() + newtravel ) );
            
            prev = next++;
        }
    }
    else
    {
        // Preconditions not met, cannot fix the phase travel.
        // Should raise exception?
        debugger << "cannot fix phase between " << b.time() << " and " << e.time()
                 << ", there are no Breakpoints between those times" << endl;
    }

}

// ---------------------------------------------------------------------------
//	matchPhaseFwd
//
//!	Compute the target frequency that will affect the
//!	predicted (by the Breakpoint phases) amount of
//!	sinusoidal phase travel between two breakpoints, 
//!	and assign that frequency to the target Breakpoint.
//!	After computing the new frequency, update the phase of
//!	the later Breakpoint.
//!
//! If the earlier Breakpoint is null and the later one
//! is non-null, then update the phase of the earlier
//! Breakpoint, and do not modify its frequency or the 
//! later Breakpoint.
//!
//! The most common kinds of errors are local (or burst) errors in 
//! frequency and phase. These errors are best corrected by correcting
//! less than half the detected error at any time. Correcting more
//! than that will produce frequency oscillations for the remainder of
//! the Partial, in the case of a single bad frequency (as is common
//! at the onset of a tone). Any damping factor less then one will 
//! converge eventually, .5 or less will converge without oscillating.
//! Use the damping argument to control the damping of the correction.
//!	Specify 1 for no damping.
//!
//! \pre		The two Breakpoints are assumed to be consecutive in
//!				a Partial.
//! \param		bp0	The earlier Breakpoint.
//! \param		bp1	The later Breakpoint.
//! \param		dt The time (in seconds) between bp0 and bp1.
//! \param		damping The fraction of the amount of phase error that will
//!				be corrected (.5 or less will prevent frequency oscilation 
//!				due to burst errors in phase). 
//! \param		maxFixPct The maximum amount of frequency adjustment
//!				that can be made to the frequency of bp1, expressed
//!				as a precentage of the unmodified frequency of bp1.
//!				If the necessary amount of frequency adjustment exceeds
//!				this amount, then the phase will not be matched, 
//!				but will be updated as well to be consistent with
//!				the frequencies. (default is 0.2%)
//
void matchPhaseFwd( Breakpoint & bp0, Breakpoint & bp1,
				    double dt, double damping, double maxFixPct )
{
	double travel = phaseTravel( bp0, bp1, dt );
    
    if ( ! BreakpointUtils::isNonNull( bp1 ) )
    {
        // if bp1 is null, DON'T compute a new phase,
        // because Nulls are phase reset points.

        // bp1.setPhase( wrapPi( bp0.phase() + travel ) );
    }
    else if ( ! BreakpointUtils::isNonNull( bp0 ) )
    {
        // if bp0 is null, and bp1 is not, then bp0
        // should be a phase reset Breakpoint during
        // rendering, so compute a new phase for
        // bp0 that achieves bp1's phase.
        bp0.setPhase( wrapPi( bp1.phase() - travel ) ) ;
    } 
    else
    {
        // invariant:
        // neither bp0 nor bp1 is null
        //
        // modify frequecies to match phases as nearly as possible
        double err = wrapPi( bp1.phase() - ( bp0.phase() + travel ) );
        
        //  The most common kinds of errors are local (or burst) errors in 
        //  frequency and phase. These errors are best corrected by correcting
        //  less than half the detected error at any time. Correcting more
        //  than that will produce frequency oscillations for the remainder of
        //  the Partial, in the case of a single bad frequency (as is common
        //  at the onset of a tone). Any damping factor less then one will 
        //  converge eventually, .5 or less will converge without oscillating.
        //  #define DAMPING .5
        travel += damping * err;
        
        double f0 = bp0.frequency();
        double ftgt = ( travel / ( Pi * dt ) ) - f0;
        
        #ifdef Loris_Debug
        debugger << "matchPhaseFwd: correcting " << bp1.frequency() << " to " << ftgt 
                 << " (phase " << wrapPi( bp1.phase() ) << "), ";
        #endif
        
        //	If the target is not a null breakpoint, may need to 
        //	clamp the amount of frequency modification.
        if ( ftgt > bp1.frequency() * ( 1 + (maxFixPct*.01) ) )
        {
            ftgt = bp1.frequency() * ( 1 + (maxFixPct*.01) );
        }
        else if ( ftgt < bp1.frequency() * ( 1 - (maxFixPct*.01) ) )
        {
            ftgt = bp1.frequency() * ( 1 - (maxFixPct*.01) );
        }

        bp1.setFrequency( ftgt );
        
        //	Recompute the phase according to the new frequency.
        double phi = wrapPi( bp0.phase() + phaseTravel( bp0, bp1, dt ) );
        bp1.setPhase( phi );

        #ifdef Loris_Debug
        debugger << "achieved " << ftgt << " (phase " << phi << ")" << endl;
        #endif
    }
}

// ---------------------------------------------------------------------------
//	fixFrequency
//
//!	Adjust frequencies of the Breakpoints in the 
//! specified Partial such that the rendered Partial 
//!	achieves (or matches as nearly as possible, within 
//!	the constraint of the maximum allowable frequency
//! alteration) the analyzed phases. 
//!
//! This just iterates over the Partial calling
//! matchPhaseFwd, should probably name those similarly.
//!
//!  \param     partial The Partial whose frequencies,
//!             and possibly phases (if the frequencies
//!             cannot be sufficiently altered to match
//!             the phases), will be recomputed.
//!  \param     maxFixPct The maximum allowable frequency 
//!             alteration, default is 0.2%.
//
void fixFrequency( Partial & partial, double maxFixPct )
{	
	if ( partial.numBreakpoints() > 1 )
	{	
		Partial::iterator next = partial.begin();
		Partial::iterator prev = next++;
		while ( next != partial.end() )		
		{
		    if ( BreakpointUtils::isNonNull( next.breakpoint() ) )
		    {
			    matchPhaseFwd( prev.breakpoint(), next.breakpoint(), 
				    		   next.time() - prev.time(), 0.5, maxFixPct );
		    }
			prev = next++;
		}
    }
}

}	//	end of namespace Loris
