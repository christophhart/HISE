#ifndef PHASEFIX_H
#define PHASEFIX_H
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
 * phasefix.h
 *
 * Functions for correcting Breakpoint phases and frequencies so that
 * stored phases match the phases that would be synthesized using the
 * Loris Synthesizer.
 *
 * Kelly Fitz, 23 Sept 04
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */
#include "Partial.h"

//	begin namespace
namespace Loris {

//  FUNCTION PROTOYPES

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
void fixPhaseBackward( Partial::iterator stopHere, Partial::iterator pos );

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
void fixPhaseForward( Partial::iterator pos, Partial::iterator stopHere );

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
void fixPhaseBetween( Partial::iterator b, Partial::iterator e );

//	fixFrequency
//
//!	Adjust frequencies of the Breakpoints in the 
//! specified Partial such that the rendered Partial 
//!	achieves (or matches as nearly as possible, within 
//!	the constraint of the maximum allowable frequency
//! alteration) the analyzed phases. 
//!
//!  \param     partial The Partial whose frequencies,
//!             and possibly phases (if the frequencies
//!             cannot be sufficiently altered to match
//!             the phases), will be recomputed.
//!  \param     maxFixPct The maximum allowable frequency 
//!             alteration, default is 0.2%.
//
void fixFrequency( Partial & partial, double maxFixPct = 0.2 );

//	fixFrequency
//
//!	Adjust frequencies of the Breakpoints in the 
//! specified Partials such that the rendered Partial 
//!	achieves (or matches as nearly as possible, within 
//!	the constraint of the maximum allowable frequency
//! alteration) the analyzed phases. 
//!
//! \param		b The beginning of a range of Partials whose 
//!             frequencies should be fixed.
//! \param		e The end of a range of Partials whose frequencies 
//!             should be fixed.
//! \param		maxFixPct The maximum allowable frequency 
//!             alteration, default is 0.2%.
//
template < class Iter >
void fixFrequency( Iter b, Iter e, double maxFixPct = 0.2 )
{
    while ( b != e )
    {
        fixFrequency( *b, maxFixPct );
        ++b;
    }
}

// --------------------- useful phase maintenance utilities ---------------------

//	matchPhaseFwd
//
//!	Compute the target frequency that will affect the
//!	predicted (by the Breakpoint phases) amount of
//!	sinusoidal phase travel between two breakpoints, 
//!	and assign that frequency to the target Breakpoint.
//!	After computing the new frequency, update the phase of
//!	the later Breakpoint.
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
				    double dt, double damping, double maxFixPct = 0.2 );

//	phaseTravel
//
//!	Compute the sinusoidal phase travel between two Breakpoints.
//!	Return the total unwrapped phase travel.
//!
//! \pre		The two Breakpoints are assumed to be consecutive in
//!				a Partial.
//! \param		bp0	The earlier Breakpoint.
//! \param		bp1	The later Breakpoint.
//! \param		dt The time (in seconds) between bp0 and bp1.
//! \return		The total unwrapped phase travel in radians.
//
double phaseTravel( const Breakpoint & bp0, const Breakpoint & bp1, double dt );

//	wrapPi
//
//!	Wrap an unwrapped phase value to the range (-pi,pi].
//!
//! \param		x The unwrapped phase in radians.
//! \return		The phase (in radians) wrapped to the range (-Pi,Pi].
//
double wrapPi( double x );


}	//	end of namespace Loris

#endif // ndef PHASEFIX_H

