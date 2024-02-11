#ifndef INCLUDE_PARTIALUTILS_H
#define INCLUDE_PARTIALUTILS_H
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
 * PartialUtils.h
 *
 *	A group of Partial utility function objects for use with STL 
 *	searching and sorting algorithms. PartialUtils is a namespace
 *	within the Loris namespace.
 *
 * This file defines three kinds of functors:
 * - Partial mutators
 * - predicates on Partials
 * - Partial comparitors
 *
 * Kelly Fitz, 6 July 2000
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */

#include "Envelope.h"
#include "Partial.h"

#include <algorithm>
#include <functional>
#include <utility>

//	begin namespace
namespace Loris {

namespace PartialUtils {

//	-- Partial mutating functors --

// ---------------------------------------------------------------------------
//	PartialMutator
//	
//! PartialMutator is an abstract base class for Partial mutators,
//! functors that operate on Partials according to a time-varying
//! envelope. The base class manages a polymorphic Envelope instance
//! that provides the time-varying mutation parameters.
//!
//! \invariant	env is a non-zero pointer to a valid instance of a 
//!            class derived from the abstract class Envelope.
class PartialMutator : public std::unary_function< Partial, void >
{
public:

	//! Construct a new PartialMutator from a constant mutation factor.
	PartialMutator( double x );

	//! Construct a new PartialMutator from an Envelope representing
	//! a time-varying mutation factor.
	PartialMutator( const Envelope & e );

	//! Construct a new PartialMutator that is a copy of another.
	PartialMutator( const PartialMutator & rhs );

	//! Destroy this PartialMutator, deleting its Envelope.
	virtual ~PartialMutator( void );
	
	//! Make this PartialMutator a duplicate of another one.
	//!
	//! \param	rhs is the PartialMutator to copy.
	PartialMutator & operator=( const PartialMutator & rhs );
	
	//! Function call operator: apply a mutation factor to the 
	//! specified Partial. Derived classes must implement this 
	//! member.
	virtual void operator()( Partial & p ) const = 0;

protected:

	//! pointer to an envelope that governs the 
	//! time-varying mutation
	Envelope * env;		
};

// ---------------------------------------------------------------------------
//	AmplitudeScaler
//	
//! Scale the amplitude of the specified Partial according to
//! an envelope representing a time-varying amplitude scale value.
//
class AmplitudeScaler : public PartialMutator
{
public:

	//! Construct a new AmplitudeScaler from a constant scale factor.
	AmplitudeScaler( double x ) : PartialMutator( x ) {}
	
	//! Construct a new AmplitudeScaler from an Envelope representing
	//! a time-varying scale factor.
	AmplitudeScaler( const Envelope & e ) : PartialMutator( e ) {}
	
	//! Function call operator: apply a scale factor to the specified
	//! Partial.
	void operator()( Partial & p ) const;
};

// ---------------------------------------------------------------------------
//	scaleAmplitude
// ---------------------------------------------------------------------------
//! Scale the amplitude of the specified Partial according to
//! an envelope representing a amplitude scale value or envelope.
//!
//! \param	p is a Partial to mutate.
//! \param	arg is either a constant scale factor or an Envelope
//!			describing the time-varying scale factor.
//
template< class Arg >
void scaleAmplitude( Partial & p, const Arg & arg )
{
	AmplitudeScaler scaler( arg );
	scaler( p );
}

// ---------------------------------------------------------------------------
//	scaleAmplitude
// ---------------------------------------------------------------------------
//! Scale the amplitude of a sequence of Partials according to
//! an envelope representing a amplitude scale value or envelope.
//!
//! \param	b is the beginning of a sequence of Partials to mutate.
//! \param	e is the end of a sequence of Partials to mutate.
//! \param	arg is either a constant scale factor or an Envelope
//!			describing the time-varying scale factor.
//
template< class Iter, class Arg >
void scaleAmplitude( Iter b, Iter e, const Arg & arg )
{
	AmplitudeScaler scaler( arg );
	while ( b != e )
	{
		scaler( *b++ );
	}
}

// ---------------------------------------------------------------------------
//	BandwidthScaler
//	
//! Scale the bandwidth of the specified Partial according to
//! an envelope representing a time-varying bandwidth scale value.
//
class BandwidthScaler : public PartialMutator
{
public:

	//! Construct a new BandwidthScaler from a constant scale factor.
	BandwidthScaler( double x ) : PartialMutator( x ) {}

	//! Construct a new BandwidthScaler from an Envelope representing
	//! a time-varying scale factor.
	BandwidthScaler( const Envelope & e ) : PartialMutator( e ) {}
	
	//! Function call operator: apply a scale factor to the specified
	//! Partial.
	void operator()( Partial & p ) const;
};

// ---------------------------------------------------------------------------
//	scaleBandwidth
// ---------------------------------------------------------------------------
//! Scale the bandwidth of the specified Partial according to
//! an envelope representing a amplitude scale value or envelope.
//!
//! \param	p is a Partial to mutate.
//! \param	arg is either a constant scale factor or an Envelope
//!			describing the time-varying scale factor.
//
template< class Arg >
void scaleBandwidth( Partial & p, const Arg & arg )
{
	BandwidthScaler scaler( arg );
	scaler( p );
}

// ---------------------------------------------------------------------------
//	scaleBandwidth
// ---------------------------------------------------------------------------
//! Scale the bandwidth of a sequence of Partials according to
//! an envelope representing a bandwidth scale value or envelope.
//!
//! \param	b is the beginning of a sequence of Partials to mutate.
//! \param	e is the end of a sequence of Partials to mutate.
//! \param	arg is either a constant scale factor or an Envelope
//!			describing the time-varying scale factor.
//
template< class Iter, class Arg >
void scaleBandwidth( Iter b, Iter e, const Arg & arg )
{
	BandwidthScaler scaler( arg );
	while ( b != e )
	{
		scaler( *b++ );
	}
}

// ---------------------------------------------------------------------------
//	BandwidthSetter
//	
//! Set the bandwidth of the specified Partial according to
//! an envelope representing a time-varying bandwidth value.
//
class BandwidthSetter : public PartialMutator
{
public:

	//! Construct a new BandwidthSetter from a constant bw factor.
	BandwidthSetter( double x ) : PartialMutator( x ) {}

	//! Construct a new BandwidthSetter from an Envelope representing
	//! a time-varying bw factor.
	BandwidthSetter( const Envelope & e ) : PartialMutator( e ) {}
	
	//! Function call operator: assign a bw factor to the specified
	//! Partial.
	void operator()( Partial & p ) const;
};

// ---------------------------------------------------------------------------
//	setBandwidth
// ---------------------------------------------------------------------------
//! Set the bandwidth of the specified Partial according to
//! an envelope representing a amplitude scale value or envelope.
//!
//! \param	p is a Partial to mutate.
//! \param	arg is either a constant scale factor or an Envelope
//!			describing the time-varying bw factor.
//
template< class Arg >
void setBandwidth( Partial & p, const Arg & arg )
{
	BandwidthSetter setter( arg );
	setter( p );
}

// ---------------------------------------------------------------------------
//	setBandwidth
// ---------------------------------------------------------------------------
//! Set the bandwidth of a sequence of Partials according to
//! an envelope representing a bandwidth value or envelope.
//!
//! \param	b is the beginning of a sequence of Partials to mutate.
//! \param	e is the end of a sequence of Partials to mutate.
//! \param	arg is either a constant scale factor or an Envelope
//!			describing the time-varying scale factor.
//
template< class Iter, class Arg >
void setBandwidth( Iter b, Iter e, const Arg & arg )
{
	BandwidthSetter setter( arg );
	while ( b != e )
	{
		setter( *b++ );
	}
}

// ---------------------------------------------------------------------------
//	FrequencyScaler
//	
//! Scale the frequency of the specified Partial according to
//! an envelope representing a time-varying bandwidth scale value.
//
class FrequencyScaler : public PartialMutator
{
public:

	//! Construct a new FrequencyScaler from a constant scale factor.
	FrequencyScaler( double x ) : PartialMutator( x ) {}
	
   //! Construct a new FrequencyScaler from an Envelope representing
	//! a time-varying scale factor.
	FrequencyScaler( const Envelope & e ) : PartialMutator( e ) {}
	
	//! Function call operator: apply a scale factor to the specified
	//! Partial.
	void operator()( Partial & p ) const;
};

// ---------------------------------------------------------------------------
//	scaleFrequency
// ---------------------------------------------------------------------------
//! Scale the frequency of the specified Partial according to
//! an envelope representing a frequency scale value or envelope.
//!
//! \param	p is a Partial to mutate.
//! \param	arg is either a constant scale factor or an Envelope
//!			describing the time-varying scale factor.
//
template< class Arg >
void scaleFrequency( Partial & p, const Arg & arg )
{
	FrequencyScaler scaler( arg );
	scaler( p );
}

// ---------------------------------------------------------------------------
//	scaleFrequency
// ---------------------------------------------------------------------------
//! Scale the frequency of a sequence of Partials according to
//! an envelope representing a frequency scale value or envelope.
//!
//! \param	b is the beginning of a sequence of Partials to mutate.
//! \param	e is the end of a sequence of Partials to mutate.
//! \param	arg is either a constant scale factor or an Envelope
//!			describing the time-varying scale factor.
//
template< class Iter, class Arg >
void scaleFrequency( Iter b, Iter e, const Arg & arg )
{
	FrequencyScaler scaler( arg );
	while ( b != e )
	{
		scaler( *b++ );
	}
}

// ---------------------------------------------------------------------------
//	NoiseRatioScaler
//	
//! Scale the relative noise content of the specified Partial according 
//! to an envelope representing a time-varying bandwidth scale value.
//
class NoiseRatioScaler : public PartialMutator
{
public:

	//! Construct a new NoiseRatioScaler from a constant scale factor.
	NoiseRatioScaler( double x ) : PartialMutator( x ) {}

	//! Construct a new NoiseRatioScaler from an Envelope representing
	//! a time-varying scale factor.
	NoiseRatioScaler( const Envelope & e ) : PartialMutator( e ) {}
	
	//! Function call operator: apply a scale factor to the specified
	//! Partial.
	void operator()( Partial & p ) const;
};

// ---------------------------------------------------------------------------
//	scaleNoiseRatio
// ---------------------------------------------------------------------------
//! Scale the relative noise content of the specified Partial according to
//! an envelope representing a scale value or envelope.
//!
//! \param	p is a Partial to mutate.
//! \param	arg is either a constant scale factor or an Envelope
//!			describing the time-varying scale factor.
//
template< class Arg >
void scaleNoiseRatio( Partial & p, const Arg & arg )
{
	NoiseRatioScaler scaler( arg );
	scaler( p );
}

// ---------------------------------------------------------------------------
//	scaleNoiseRatio
// ---------------------------------------------------------------------------
//! Scale the relative noise content of a sequence of Partials according to
//! an envelope representing a scale value or envelope.
//!
//! \param	b is the beginning of a sequence of Partials to mutate.
//! \param	e is the end of a sequence of Partials to mutate.
//! \param	arg is either a constant scale factor or an Envelope
//!			describing the time-varying scale factor.
//
template< class Iter, class Arg >
void scaleNoiseRatio( Iter b, Iter e, const Arg & arg )
{
	NoiseRatioScaler scaler( arg );
	while ( b != e )
	{
		scaler( *b++ );
	}
}

// ---------------------------------------------------------------------------
//	PitchShifter
//	
//! Shift the pitch of the specified Partial according to
//! the given pitch envelope. The pitch envelope is assumed to have 
//! units of cents (1/100 of a halfstep).
//
class PitchShifter : public PartialMutator
{
public:

	//! Construct a new PitchShifter from a constant scale factor.
	PitchShifter( double x ) : PartialMutator( x ) {}

   //! Construct a new PitchShifter from an Envelope representing
	//! a time-varying scale factor.
	PitchShifter( const Envelope & e ) : PartialMutator( e ) {}
	
	//! Function call operator: apply a scale factor to the specified
	//! Partial.
	void operator()( Partial & p ) const;
};

// ---------------------------------------------------------------------------
//	shiftPitch
// ---------------------------------------------------------------------------
//! Shift the pitch of the specified Partial according to
//! an envelope representing a pitch value or envelope.
//!
//! \param	p is a Partial to mutate.
//! \param	arg is either a constant pitch factor or an Envelope
//!			describing the time-varying pitch factor in cents (1/100 of a 
//!         halfstep).
//
template< class Arg >
void shiftPitch( Partial & p, const Arg & arg )
{
	PitchShifter shifter( arg );
	shifter( p );
}

// ---------------------------------------------------------------------------
//	shiftPitch
// ---------------------------------------------------------------------------
//! Shift the pitch of a sequence of Partials according to
//! an envelope representing a pitch value or envelope.
//!
//! \param	b is the beginning of a sequence of Partials to mutate.
//! \param	e is the end of a sequence of Partials to mutate.
//! \param	arg is either a constant pitch factor or an Envelope
//!			describing the time-varying pitch factor in cents (1/100 of a 
//!         halfstep).
//
template< class Iter, class Arg >
void shiftPitch( Iter b, Iter e, const Arg & arg )
{
	PitchShifter shifter( arg );
	while ( b != e )
	{
		shifter( *b++ );
	}
}

//	These ones are not derived from PartialMutator, because
//	they don't use an Envelope and cannot be time-varying.

// ---------------------------------------------------------------------------
//	Cropper
//	
//! Trim a Partial by removing Breakpoints outside a specified time span.
//!	Insert a Breakpoint at the boundary when cropping occurs.
class Cropper
{
public:

    //! Construct a new Cropper from a pair of times (in seconds)
    //! representing the span of time to which Partials should be
    //! cropped.
	Cropper( double t1, double t2 ) : 
		minTime( std::min( t1, t2 ) ),
		maxTime( std::max( t1, t2 ) )
	{
	}
	
	//! Function call operator: crop the specified Partial.
    //! Trim a Partial by removing Breakpoints outside the span offset
    //! [minTime, maxTime]. Insert a Breakpoint at the boundary when 
    //! cropping occurs.
	void operator()( Partial & p ) const;
	
private:
	double minTime, maxTime;
};

// ---------------------------------------------------------------------------
//	crop
// ---------------------------------------------------------------------------
//! Trim a Partial by removing Breakpoints outside a specified time span.
//! Insert a Breakpoint at the boundary when cropping occurs.
//! 
//! This operation may leave the Partial empty, if it previously had
//! no Breakpoints in the span [t1,t2]. 
//!
//! \param	p is the Partial to crop.
//! \param	t1 is the beginning of the time span to which the Partial
//!         should be cropped.
//! \param	t2 is the end of the time span to which the Partial
//!         should be cropped.
//
inline
void crop( Partial & p, double t1, double t2 )
{
	Cropper cropper( t1, t2 );
	cropper( p );
}

// ---------------------------------------------------------------------------
//	crop
// ---------------------------------------------------------------------------
//! Trim a sequence of Partials by removing Breakpoints outside a specified 
//! time span. Insert a Breakpoint at the boundary when cropping occurs.
//! 
//! This operation may leave some empty Partials, if they previously had
//! no Breakpoints in the span [t1,t2]. 
//!
//! \param	b is the beginning of a sequence of Partials to crop.
//! \param	e is the end of a sequence of Partials to crop.
//! \param	t1 is the beginning of the time span to which the Partials
//!         should be cropped.
//! \param	t2 is the end of the time span to which the Partials
//!         should be cropped.
//
template< class Iter >
void crop( Iter b, Iter e, double t1, double t2 )
{
	Cropper cropper( t1, t2 );
	while ( b != e )
	{
		cropper( *b++ );
	}
}

// ---------------------------------------------------------------------------
//	TimeShifter
//	
//! Shift the time of all the Breakpoints in a Partial by a 
//! constant amount.
//
class TimeShifter
{
public:

	//! Construct a new TimeShifter from a constant offset in seconds.
	TimeShifter( double x ) : offset( x ) {}
	
	//! Function call operator: apply a time shift to the specified
	//! Partial.
	void operator()( Partial & p ) const;
	
private:
	double offset;
};

// ---------------------------------------------------------------------------
//	shiftTime
// ---------------------------------------------------------------------------
//! Shift the time of all the Breakpoints in a Partial by a 
//! constant amount.
//!
//! \param	p is a Partial to shift.
//! \param	offset is a constant offset in seconds.
//
inline
void shiftTime( Partial & p, double offset )
{
	TimeShifter shifter( offset );
	shifter( p );
}

// ---------------------------------------------------------------------------
//	shiftTime
// ---------------------------------------------------------------------------
//! Shift the time of all the Breakpoints in a Partial by a 
//! constant amount.
//!
//! \param	b is the beginning of a sequence of Partials to shift.
//! \param	e is the end of a sequence of Partials to shift.
//! \param	offset is a constant offset in seconds.
//
template< class Iter >
void shiftTime( Iter b, Iter e, double offset )
{
	TimeShifter shifter( offset );
	while ( b != e )
	{
		shifter( *b++ );
	}
}
	
// ---------------------------------------------------------------------------
//	timeSpan
// ---------------------------------------------------------------------------
//! Return the time (in seconds) spanned by a specified half-open
//! range of Partials as a std::pair composed of the earliest
//! Partial start time and latest Partial end time in the range.
//
template < typename Iterator >
std::pair< double, double > 
timeSpan( Iterator begin, Iterator end ) 
{
	double tmin = 0., tmax = 0.;
	if ( begin != end )
	{
		Iterator it = begin;
		tmin = it->startTime();
		tmax = it->endTime();
		while( it != end )
		{
			tmin = std::min( tmin, it->startTime() );
			tmax = std::max( tmax, it->endTime() );
			++it;
		}
	}
	return std::make_pair( tmin, tmax );
}

// ---------------------------------------------------------------------------
//	peakAmplitude
// ---------------------------------------------------------------------------
//! Return the maximum amplitude achieved by a Partial. 
//!  
//! \param  p is the Partial to evaluate
//! \return the maximum (absolute) amplitude achieved by 
//!         the partial p
//
double peakAmplitude( const Partial & p );

// ---------------------------------------------------------------------------
//	avgAmplitude
// ---------------------------------------------------------------------------
//! Return the average amplitude over all Breakpoints in this Partial.
//! Return zero if the Partial has no Breakpoints.
//!  
//! \param  p is the Partial to evaluate
//! \return the average amplitude of Breakpoints in the Partial p
//
double avgAmplitude( const Partial & p );

// ---------------------------------------------------------------------------
//	avgFrequency
// ---------------------------------------------------------------------------
//! Return the average frequency over all Breakpoints in this Partial.
//! Return zero if the Partial has no Breakpoints.
//!  
//! \param  p is the Partial to evaluate
//! \return the average frequency (Hz) of Breakpoints in the Partial p
//
double avgFrequency( const Partial & p );

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
double weightedAvgFrequency( const Partial & p );

//	-- phase maintenance functions --

// ---------------------------------------------------------------------------
//	fixPhaseBefore
// ---------------------------------------------------------------------------
//! Recompute phases of all Breakpoints earlier than the specified time 
//! so that the synthesized phases of those earlier Breakpoints matches 
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
void fixPhaseBefore( Partial & p, double t );

// ---------------------------------------------------------------------------
//	fixPhaseBefore (range)
// ---------------------------------------------------------------------------
//! Recompute phases of all Breakpoints earlier than the specified time 
//! so that the synthesized phases of those earlier Breakpoints matches 
//! the stored phase, and the synthesized phase at the specified
//! time matches the stored (not recomputed) phase.
//! 
//! Backward phase-fixing stops if a null (zero-amplitude) Breakpoint
//! is encountered, because nulls are interpreted as phase reset points
//! in Loris. If a null is encountered, the remainder of the Partial
//! (the front part) is fixed in the forward direction, beginning at
//! the start of the Partial.
//!
//! \param b    The beginning of a range of Partials whose phases 
//!             should be fixed.
//! \param e    The end of a range of Partials whose phases 
//!             should be fixed.
//! \param t    The time before which phases should be adjusted.
//
template < class Iter >
void fixPhaseBefore( Iter b, Iter e, double t )
{
    while ( b != e )
    {
        fixPhaseBefore( *b, t );
        ++b;
    }
}

// ---------------------------------------------------------------------------
//	fixPhaseAfter
// ---------------------------------------------------------------------------
//! Recompute phases of all Breakpoints later than the specified time 
//! so that the synthesized phases of those later Breakpoints matches 
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
void fixPhaseAfter( Partial & p, double t );

// ---------------------------------------------------------------------------
//	fixPhaseAfter (range)
// ---------------------------------------------------------------------------
//! Recompute phases of all Breakpoints later than the specified time 
//! so that the synthesized phases of those later Breakpoints matches 
//! the stored phase, as long as the synthesized phase at the specified
//! time matches the stored (not recomputed) phase.
//! 
//! Phase fixing is only applied to non-null (nonzero-amplitude) Breakpoints,
//! because null Breakpoints are interpreted as phase reset points in 
//! Loris. If a null is encountered, its phase is simply left unmodified,
//! and future phases wil be recomputed from that one.
//!
//! \param b    The beginning of a range of Partials whose phases 
//!             should be fixed.
//! \param e    The end of a range of Partials whose phases 
//!             should be fixed.
//! \param t    The time after which phases should be adjusted.
//
template < class Iter >
void fixPhaseAfter( Iter b, Iter e, double t )
{
    while ( b != e )
    {
        fixPhaseAfter( *b, t );
        ++b;
    }
}

// ---------------------------------------------------------------------------
//	fixPhaseForward
// ---------------------------------------------------------------------------
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
//! \param p    The Partial whose phases should be fixed.
//! \param tbeg The phases and frequencies of Breakpoints later than the 
//!             one nearest this time will be modified.
//! \param tend The phases and frequencies of Breakpoints earlier than the 
//!             one nearest this time will be modified. Should be greater 
//!             than tbeg, or else they will be swapped.
//
void fixPhaseForward( Partial & p, double tbeg, double tend );

// ---------------------------------------------------------------------------
//	fixPhaseForward (range)
// ---------------------------------------------------------------------------
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
//! \param b    The beginning of a range of Partials whose phases 
//!             should be fixed.
//! \param e    The end of a range of Partials whose phases 
//!             should be fixed.
//! \param tbeg The phases and frequencies of Breakpoints later than the 
//!             one nearest this time will be modified.
//! \param tend The phases and frequencies of Breakpoints earlier than the 
//!             one nearest this time will be modified. Should be greater 
//!             than tbeg, or else they will be swapped.
//
template < class Iter >
void fixPhaseForward( Iter b, Iter e, double tbeg, double tend )
{
    while ( b != e )
    {
        fixPhaseForward( *b, tbeg, tend );
        ++b;
    }
}


// ---------------------------------------------------------------------------
//	fixPhaseAt
// ---------------------------------------------------------------------------
//! Recompute phases of all Breakpoints in a Partial
//! so that the synthesized phases match the stored phases, 
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
void fixPhaseAt( Partial & p, double t );

// ---------------------------------------------------------------------------
//	fixPhaseAt (range)
// ---------------------------------------------------------------------------
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
//! \param b    The beginning of a range of Partials whose phases 
//!             should be fixed.
//! \param e    The end of a range of Partials whose phases 
//!             should be fixed.
//! \param t    The time at which phases should be made correct.
//
template < class Iter >
void fixPhaseAt( Iter b, Iter e, double t )
{
    while ( b != e )
    {
        fixPhaseAt( *b, t );
        ++b;
    }
}

// ---------------------------------------------------------------------------
//	fixPhaseBetween
// ---------------------------------------------------------------------------
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
//! \pre      Thre must be at least one Breakpoint in the
//!           Partial between the specified times t1 and t2.
//!           If this condition is not met, the Partial is
//!           unmodified.
//! \post     The phases and frequencies of the Breakpoints in the 
//!           range have been recomputed such that an oscillator
//!           initialized to the parameters of the first Breakpoint
//!           will arrive at the parameters of the last Breakpoint,
//!           and all the intervening Breakpoints will be matched.
//!	\param p  The partial whose phases and frequencies will be recomputed. 
//!           The Breakpoint at this position is unaltered.
//! \param t1 The time before which Partial frequencies and phases will 
//!           not be modified.
//! \param t2 The time after which Partial frequencies and phases will 
//!           not be modified. Should be greater than t1, or else they
//!           will be swapped.
//
void fixPhaseBetween( Partial & p, double t1, double t2 );

// ---------------------------------------------------------------------------
//	fixPhaseBetween (range)
// ---------------------------------------------------------------------------
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
//! \pre        Thre must be at least one Breakpoint in each
//!             Partial between the specified times t1 and t2.
//!             If this condition is not met, the Partial is
//!             unmodified.
//! \post       The phases and frequencies of the Breakpoints in the 
//!             range have been recomputed such that an oscillator
//!             initialized to the parameters of the first Breakpoint
//!             will arrive at the parameters of the last Breakpoint,
//!             and all the intervening Breakpoints will be matched.
//! \param b    The beginning of a range of Partials whose phases 
//!             should be fixed.
//! \param e    The end of a range of Partials whose phases 
//!             should be fixed.
//! \param t1   The time before which Partial frequencies and phases will 
//!             not be modified.
//! \param t2   The time after which Partial frequencies and phases will 
//!             not be modified. Should be greater than t1, or else they
//!             will be swapped.
//
template < class Iter >
void fixPhaseBetween( Iter b, Iter e, double t1, double t2 )
{
    while ( b != e )
    {
        fixPhaseBetween( *b, t1, t2 );
        ++b;
    }
}


	
//	-- predicates --

// ---------------------------------------------------------------------------
//	isDurationLess
//	
//! Predicate functor returning true if the duration of its 
//! Partial argument is less than the specified duration in
//! seconds, and false otherwise.
//
class isDurationLess : public std::unary_function< const Partial, bool >
{
public:
    //! Initialize a new instance with the specified label.
	isDurationLess( double x ) : mDurationSecs(x) {}

	//! Function call operator: evaluate a Partial.
	bool operator()( const Partial & p ) const 
		{ return p.duration() < mDurationSecs; }
		
	//! Function call operator: evaluate a Partial pointer.
	bool operator()( const Partial * p ) const 
		{ return p->duration() < mDurationSecs; }
        
private:	
	double mDurationSecs;
};

// ---------------------------------------------------------------------------
//	isLabelEqual
//	
//! Predicate functor returning true if the label of its Partial argument is
//! equal to the specified 32-bit label, and false otherwise.
//
class isLabelEqual : public std::unary_function< const Partial, bool >
{
public:
    //! Initialize a new instance with the specified label.
	isLabelEqual( int l ) : label(l) {}
	
	//! Function call operator: evaluate a Partial.
	bool operator()( const Partial & p ) const 
		{ return p.label() == label; }
		
	//! Function call operator: evaluate a Partial pointer.
	bool operator()( const Partial * p ) const 
		{ return p->label() == label; }

private:	
	int label;
};
	
// ---------------------------------------------------------------------------
//	isLabelGreater
//	
//! Predicate functor returning true if the label of its Partial argument is
//! greater than the specified 32-bit label, and false otherwise.
//
class isLabelGreater : public std::unary_function< const Partial, bool >
{
public:
   //! Initialize a new instance with the specified label.
	isLabelGreater( int l ) : label(l) {}
	
	//! Function call operator: evaluate a Partial.
	bool operator()( const Partial & p ) const 
		{ return p.label() > label; }
		
	//! Function call operator: evaluate a Partial pointer.
	bool operator()( const Partial * p ) const 
		{ return p->label() > label; }

private:	
	int label;
};
		
// ---------------------------------------------------------------------------
//	isLabelLess
//	
//! Predicate functor returning true if the label of its Partial argument is
//! less than the specified 32-bit label, and false otherwise.
//
class isLabelLess : public std::unary_function< const Partial, bool >
{
public:
   //! Initialize a new instance with the specified label.
	isLabelLess( int l ) : label(l) {}
	
	//! Function call operator: evaluate a Partial.
	bool operator()( const Partial & p ) const 
		{ return p.label() < label; }
		
	//! Function call operator: evaluate a Partial pointer.
	bool operator()( const Partial * p ) const 
		{ return p->label() < label; }

private:	
	int label;
};
		
// ---------------------------------------------------------------------------
//	isPeakLess
//	
//! Predicate functor returning true if the peak amplitude achieved by its 
//! Partial argument is less than the specified absolute amplitude, and 
//! false otherwise.
//
class isPeakLess : public std::unary_function< const Partial, bool >
{
public:
    //! Initialize a new instance with the specified peak amplitude.
	isPeakLess( double x ) : thresh(x) {}

	//! Function call operator: evaluate a Partial.
	bool operator()( const Partial & p ) const 
		{ return peakAmplitude( p ) < thresh; }
        
	//! Function call operator: evaluate a Partial pointer.
	bool operator()( const Partial * p ) const 
		{ return peakAmplitude( *p ) < thresh; }

private:	
	double thresh;
};

//	-- comparitors --

// ---------------------------------------------------------------------------
//	compareLabelLess
//	
//! Comparitor (binary) functor returning true if its first Partial
//! argument has a label whose 32-bit integer representation is less than
//! that of the second Partial argument's label, and false otherwise.
//
class compareLabelLess : 
	public std::binary_function< const Partial, const Partial, bool >
{
public:
   //! Compare two Partials, return true if its first Partial
   //! argument has a label whose 32-bit integer representation is less than
   //! that of the second Partial argument's label, and false otherwise.
	bool operator()( const Partial & lhs, const Partial & rhs ) const 
		{ return lhs.label() < rhs.label(); }

   //! Compare two Partials, return true if its first Partial
   //! argument has a label whose 32-bit integer representation is less than
   //! that of the second Partial argument's label, and false otherwise.
	bool operator()( const Partial * lhs, const Partial * rhs ) const 
		{ return lhs->label() < rhs->label(); }
};

// ---------------------------------------------------------------------------
//	compareDurationLess
//	
//! Comparitor (binary) functor returning true if its first Partial
//! argument has duration less than that of the second Partial
//! argument, and false otherwise.
//
class compareDurationLess : 
	public std::binary_function< const Partial, const Partial, bool >
{
public:
   //! Compare two Partials, return true if its first Partial
   //! argument has duration less than that of the second Partial
   //! argument, and false otherwise.
	bool operator()( const Partial & lhs, const Partial & rhs ) const 
		{ return lhs.duration() < rhs.duration(); }

   //! Compare two Partials, return true if its first Partial
   //! argument has duration less than that of the second Partial
   //! argument, and false otherwise.
	bool operator()( const Partial * lhs, const Partial * rhs ) const 
		{ return lhs->duration() < rhs->duration(); }
};

// ---------------------------------------------------------------------------
//	compareDurationGreater
//	
//! Comparitor (binary) functor returning true if its first Partial
//! argument has duration greater than that of the second Partial
//! argument, and false otherwise.
//
class compareDurationGreater : 
	public std::binary_function< const Partial, const Partial, bool >
{
public:
   //! Compare two Partials, return true if its first Partial
   //! argument has duration greater than that of the second Partial
   //! argument, and false otherwise.
	bool operator()( const Partial & lhs, const Partial & rhs ) const 
		{ return lhs.duration() > rhs.duration(); }

   //! Compare two Partials, return true if its first Partial
   //! argument has duration greater than that of the second Partial
   //! argument, and false otherwise.
	bool operator()( const Partial * lhs, const Partial * rhs ) const 
		{ return lhs->duration() > rhs->duration(); }
};

// ---------------------------------------------------------------------------
//	compareStartTimeLess
//	
//! Comparitor (binary) functor returning true if its first Partial
//! argument has start time earlier than that of the second Partial
//! argument, and false otherwise.
//
class compareStartTimeLess : 
	public std::binary_function< const Partial, const Partial, bool >
{
public:
   //! Compare two Partials, return true if its first Partial
   //! argument has start time earlier than that of the second Partial
   //! argument, and false otherwise.
	bool operator()( const Partial & lhs, const Partial & rhs ) const 
		{ return lhs.startTime() < rhs.startTime(); }

   //! Compare two Partials, return true if its first Partial
   //! argument has start time earlier than that of the second Partial
   //! argument, and false otherwise.
	bool operator()( const Partial * lhs, const Partial * rhs ) const 
		{ return lhs->startTime() < rhs->startTime(); }
};



}	//	end of namespace PartialUtils

}	//	end of namespace Loris

#endif /* ndef INCLUDE_PARTIALUTILS_H */
