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
 * Partial.C
 *
 * Implementation of class Loris::Partial.
 *
 * Kelly Fitz, 16 Aug 1999
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */

#if HAVE_CONFIG_H
	#include "config.h"
#endif

#include "Partial.h"
#include "Breakpoint.h"
#include "LorisExceptions.h"
#include "Notifier.h"

#include <algorithm>
#include <cmath>

#if defined(HAVE_M_PI) && (HAVE_M_PI)
	const double Pi = M_PI;
#else
	const double Pi = 3.14159265358979324;
#endif

//	begin namespace
namespace Loris {

//long Partial::DebugCounter = 0L;

//	comparitor for elements in Partial::container_type
typedef Partial::container_type::value_type Partial_value_type;
static 
bool order_by_time( const Partial_value_type & x, const Partial_value_type & y )
{
	//	Partial_value_type is a (time,Breakpoint) pair
	return x.first < y.first;
}

//	--- concering the type of Partial::container_type
//
//	On the surface, it would seem that a vector of (time,Breakpoint)
//	pairs would be a more efficient container for the Partial
//	parameter envelope points, and the changes required to implement
//	Partial using vector instead of map are minimal and simple. 
//
//	However, the crucial factor in that change is the expiration of
//	Partial::iterators. With map, iterators remain valid after
//	insertions and removals, but with vector they do not. So it 
//	is easy to change the container type, but it is a much harder
//	project to find all the places in Loris that rely on iterators
//	that remain valid after insertions and removals.
#undef USE_VECTOR


// -- construction --

// ---------------------------------------------------------------------------
//	Partial constructor
// ---------------------------------------------------------------------------
//!	Retun a new empty (no Breakpoints) unlabeled Partial.
//
Partial::Partial( void ) :
	_label( 0 )
{
//	++DebugCounter;
}	

// ---------------------------------------------------------------------------
//	Partial initialized constructor
// ---------------------------------------------------------------------------
//!	Retun a new Partial from a half-open (const) iterator range 
//!	of time, Breakpoint pairs.
//
Partial::Partial( const_iterator beg, const_iterator end ) :
	_breakpoints( beg._iter, end._iter ),
	_label( 0 )
{
//	++DebugCounter;
}	

// ---------------------------------------------------------------------------
//	Partial copy constructor
// ---------------------------------------------------------------------------
//!	Return a new Partial that is an exact copy (has an identical set
//!	of Breakpoints, at identical times, and the same label) of another 
//!	Partial.
//
Partial::Partial( const Partial & other ) :
	_breakpoints( other._breakpoints ),
	_label( other._label )
{
//	++DebugCounter;
}

// ---------------------------------------------------------------------------
//	Partial destructor
// ---------------------------------------------------------------------------
//!	Destroy this Partial.
//
Partial::~Partial( void )
{
//	--DebugCounter;
}	

// ---------------------------------------------------------------------------
//	operator=
// ---------------------------------------------------------------------------
//!	Make this Partial an exact copy (has an identical set of 
//!	Breakpoints, at identical times, and the same label) of another 
//!	Partial.
//
Partial & 
Partial::operator=( const Partial & rhs )
{
	if ( this != &rhs )
	{
		_breakpoints = rhs._breakpoints;
		_label = rhs._label;
	}
	return *this;
}

// -- container-dependent implementation --

// ---------------------------------------------------------------------------
//	begin
// ---------------------------------------------------------------------------
//!	Return a const iterator refering to the position of the first
//!	Breakpoint in this Partial's envelope.
//		
Partial::const_iterator Partial::begin( void ) const 
{ 
	return _breakpoints.begin(); 
}

//!	Return an iterator refering to the position of the first
//!	Breakpoint in this Partial's envelope.
//		
Partial::iterator Partial::begin( void ) 
{ 
	return _breakpoints.begin(); 
}

// ---------------------------------------------------------------------------
//	end
// ---------------------------------------------------------------------------
//!	Return a const iterator refering to the position past the last
//!	Breakpoint in this Partial's envelope. The iterator returned by
//!	end() (like the iterator returned by the end() member of any STL
//!	container) does not refer to a valid Breakpoint. 	
//
Partial::const_iterator 
Partial::end( void ) const 
{ 
	return _breakpoints.end(); 
}

//!	Return an iterator refering to the position past the last
//!	Breakpoint in this Partial's envelope. The iterator returned by
//!	end() (like the iterator returned by the end() member of any STL
//!	container) does not refer to a valid Breakpoint. 	
//
Partial::iterator 
Partial::end( void ) 
{ 
	return _breakpoints.end(); 
}

// ---------------------------------------------------------------------------
//	erase
// ---------------------------------------------------------------------------
//!	Breakpoint removal: erase the Breakpoints in the specified range,
//!	and return an iterator referring to the position after the,
//!	erased range.
//
Partial::iterator 
Partial::erase( Partial::iterator beg, Partial::iterator end )
{
	_breakpoints.erase( beg._iter, end._iter );
	return end;
}

// ---------------------------------------------------------------------------
//	findAfter
// ---------------------------------------------------------------------------
//!	Return a const iterator refering to the insertion position for a
//!	Breakpoint at the specified time (that is, the position of the first
//!	Breakpoint at a time not earlier than the specified time).
//	
Partial::const_iterator 
Partial::findAfter( double time ) const
{
#if defined(USE_VECTOR) 
	//	see note above
	Partial_value_type dummy( time, Breakpoint() );
	return std::upper_bound( _breakpoints.begin(), _breakpoints.end(), dummy, order_by_time );
#else
	return _breakpoints.lower_bound( time );
#endif
}

//!	Return an iterator refering to the insertion position for a
//!	Breakpoint at the specified time (that is, the position of the first
//!	Breakpoint at a time later than the specified time).
//	
Partial::iterator 
Partial::findAfter( double time ) 
{
#if defined(USE_VECTOR) 
	//	see note above
	Partial_value_type dummy( time, Breakpoint() );
	return std::upper_bound( _breakpoints.begin(), _breakpoints.end(), dummy, order_by_time );
#else
	return _breakpoints.lower_bound( time );
#endif
}

// ---------------------------------------------------------------------------
//	insert
// ---------------------------------------------------------------------------
//!	Breakpoint insertion: insert a copy of the specified Breakpoint in the
//!	parameter envelope at time (seconds), and return an iterator
//!	refering to the position of the inserted Breakpoint.
//
Partial::iterator 
Partial::insert( double time, const Breakpoint & bp )
{
#if defined(USE_VECTOR) 
	//	see note above
	//	find the position at which to insert the new Breakpoint:
	Partial_value_type dummy( time, Breakpoint() );
	Partial::container_type::iterator insertHere = 
		std::lower_bound( _breakpoints.begin(), _breakpoints.end(), dummy, order_by_time );
		
	//	if the time at insertHere is equal to the insertion time,
	//	simply replace the Breakpoint, otherwise insert:
	if ( insertHere->first == time )
	{
		insertHere->second = bp;
	}
	else
	{
		insertHere = _breakpoints.insert( insertHere, Partial_value_type(time, bp) );
	}
	return insertHere;
#else
    /*
    //  this allows Breakpoints to be inserted arbitrarily
    //  close together, which is no good, can cause trouble later:
    
	std::pair< container_type::iterator, bool > result = 
		_breakpoints.insert( container_type::value_type(time, bp) );
	if ( ! result.second )
    {
		result.first->second = bp;
    }
	return result.first;
    */
    
    //  do not insert a Breakpoint closer than 1ns away
    //  from the nearest existing Breakpoint:
    static const double MinTimeDif = 1.0E-9; // 1 ns
    
    //  find the insertion point for this time
    container_type::iterator pos = _breakpoints.lower_bound( time );
    
    //  the time of pos is either equal to or greater
    //  than the insertion time, if this is too close, 
    //  remove the Breakpoint at pos:
    if ( _breakpoints.end() != pos && MinTimeDif > pos->first - time )
    {
        _breakpoints.erase( pos++ );
    }
    //  otherwise, if the preceding position is too clase, 
    //  remove the Breakpoint at that position
    else if ( _breakpoints.begin() != pos && MinTimeDif > time - (--pos)->first )
    {
        _breakpoints.erase( pos++ );
    }

    //  now pos is at most one position away from the insertion point
    //  so insertion can be performed in constant time, and the new
    //  Breakpoint is at least 1ns away from any other Breakpoint:
    pos = _breakpoints.insert( pos, container_type::value_type(time, bp) );

    Assert( pos->first == time );

	return pos;

#endif
}

// ---------------------------------------------------------------------------
//	numBreakpoints
// ---------------------------------------------------------------------------
//!	Same as size(). Return the number of Breakpoints in this Partial.
//
Partial::size_type 
Partial::numBreakpoints( void ) const 
{ 	
	return _breakpoints.size(); 
}
// ---------------------------------------------------------------------------
//	size
// ---------------------------------------------------------------------------
//!	Return the number of Breakpoints in this Partial.
//
Partial::size_type 
Partial::size( void ) const 
{ 	
	return _breakpoints.size(); 
}

// ---------------------------------------------------------------------------
//	label
// ---------------------------------------------------------------------------
//!	Return the 32-bit label for this Partial as an integer.
//
Partial::label_type 
Partial::label( void ) const 
{ 	
	return _label; 
}

// ---------------------------------------------------------------------------
//	first
// ---------------------------------------------------------------------------
//!	Return a reference to the first Breakpoint in the Partial's
//!	envelope. Raises InvalidPartial exception if there are no 
//!	Breakpoints.
//
Breakpoint & 
Partial::first( void )
{
	if ( size() == 0 )
	{
		Throw( InvalidPartial, "Tried find first Breakpoint in a Partial with no Breakpoints." );
	}
#if defined(USE_VECTOR) 
	//	see note above
	return _breakpoints.front().second;
#else
	return begin().breakpoint();
#endif
}

// ---------------------------------------------------------------------------
//	first
// ---------------------------------------------------------------------------
//!	Return a const reference to the first Breakpoint in the Partial's
//!	envelope. Raises InvalidPartial exception if there are no 
//!	Breakpoints.
//
const Breakpoint & 
Partial::first( void ) const
{
	if ( size() == 0 )
	{
		Throw( InvalidPartial, "Tried find first Breakpoint in a Partial with no Breakpoints." );
	}
#if defined(USE_VECTOR) 
	//	see note above
	return _breakpoints.front().second;
#else
	return begin().breakpoint();
#endif
}

// ---------------------------------------------------------------------------
//	last
// ---------------------------------------------------------------------------
//!	Return a reference to the last Breakpoint in the Partial's
//!	envelope. Raises InvalidPartial exception if there are no 
//!	Breakpoints.
//
Breakpoint & 
Partial::last( void )
{
	if ( size() == 0 )
	{
		Throw( InvalidPartial, "Tried find last Breakpoint in a Partial with no Breakpoints." );
	}
#if defined(USE_VECTOR) 
	//	see note above
	return _breakpoints.back().second;
#else
	return (--end()).breakpoint();
#endif
}

// ---------------------------------------------------------------------------
//	last
// ---------------------------------------------------------------------------
//!	Return a const reference to the last Breakpoint in the Partial's
//!	envelope. Raises InvalidPartial exception if there are no 
//!	Breakpoints.
//
const Breakpoint & 
Partial::last( void ) const
{
	if ( size() == 0 )
	{
		Throw( InvalidPartial, "Tried find last Breakpoint in a Partial with no Breakpoints." );
	}	
#if defined(USE_VECTOR) 
	//	see note above
	return _breakpoints.back().second;
#else
	return (--end()).breakpoint();
#endif
}

// -- container-independent implementation --

// ---------------------------------------------------------------------------
//	initialPhase
// ---------------------------------------------------------------------------
//!	Return starting phase in radians, except (InvalidPartial) if there
//!	are no Breakpoints.
//
double
Partial::initialPhase( void ) const
{
	if ( numBreakpoints() == 0 )
	{
		Throw( InvalidPartial, "Tried find intial phase of a Partial with no Breakpoints." );
	}
	return first().phase();
}

// ---------------------------------------------------------------------------
//	startTime
// ---------------------------------------------------------------------------
//!	Return start time in seconds, except (InvalidPartial) if there
//!	are no Breakpoints.
//
double
Partial::startTime( void ) const
{
	if ( numBreakpoints() == 0 )
	{
		Throw( InvalidPartial, "Tried to find start time of a Partial with no Breakpoints." );
	}
	return begin().time();
}

// ---------------------------------------------------------------------------
//	endTime
// ---------------------------------------------------------------------------
//!	Return end time in seconds, except (InvalidPartial) if there
//!	are no Breakpoints.
//
double
Partial::endTime( void ) const
{
	if ( numBreakpoints() == 0 )
	{
		Throw( InvalidPartial, "Tried to find end time of a Partial with no Breakpoints." );
	}
	return (--end()).time();
}

// ---------------------------------------------------------------------------
//	absorb
// ---------------------------------------------------------------------------
//!	Absorb another Partial's energy as noise (bandwidth), 
//!	by accumulating the other's energy as noise energy
//!	in the portion of this Partial's envelope that overlaps
//!	(in time) with the other Partial's envelope.
//
void 
Partial::absorb( const Partial & other )
{
	Partial::iterator it = findAfter( other.startTime() );
	while ( it != end() && !(it.time() > other.endTime()) )
	{
		//	only non-null (non-zero-amplitude) Breakpoints
		//	abosrb noise energy because null Breakpoints
		//	are used especially to reset the Partial phase,
		//	and are not part of the normal analyasis data:
		if ( it->amplitude() > 0 )
		{
			// absorb energy from other at the time
			// of this Breakpoint:
			double a = other.amplitudeAt( it.time() );
			it->addNoiseEnergy( a * a );
		}	
		++it;
	}
}

// ---------------------------------------------------------------------------
//	setLabel
// ---------------------------------------------------------------------------
//!	Set the label for this Partial to the specified 32-bit value.
//
void 
Partial::setLabel( label_type l ) 
{ 
	_label = l; 
}

// ---------------------------------------------------------------------------
//	duration
// ---------------------------------------------------------------------------
//!	Return time, in seconds, spanned by this Partial, or 0. if there
//!	are no Breakpoints.
//
double
Partial::duration( void ) const
{
	if ( numBreakpoints() == 0 )
	{
		return 0.;
	}
	return endTime() - startTime();
}

// ---------------------------------------------------------------------------
//	erase
// ---------------------------------------------------------------------------
//!	Erase the Breakpoint at the position of the 
//!	given iterator (invalidating the iterator), and
//!	return an iterator referring to the next position,
//!	or end if pos is the last Breakpoint in the Partial.
//
Partial::iterator 
Partial::erase( iterator pos )
{
	if ( pos != end() )
	{
		iterator b= pos;
		iterator e = ++pos;
		pos = erase( b, e );
	}
	return pos;
}

// ---------------------------------------------------------------------------
//	split
// ---------------------------------------------------------------------------
//!	Break this Partial at the specified position (iterator).
//!	The Breakpoint at the specified position becomes the first
//!	Breakpoint in a new Partial. Breakpoints at the specified
//!	position and subsequent positions are removed from this
//!	Partial and added to the new Partial, which is returned.
//
Partial 
Partial::split( iterator pos )
{
	Partial res( pos, end() );
	erase( pos, end() );
	return res;
}

// ---------------------------------------------------------------------------
//	findNearest (const version)
// ---------------------------------------------------------------------------
//!	Return the insertion position for the Breakpoint nearest
//!	the specified time. Always returns a valid iterator (the
//!	position of the nearest-in-time Breakpoint) unless there
//!	are no Breakpoints.
//
Partial::const_iterator
Partial::findNearest( double time ) const
{
	//	if there are no Breakpoints, return end:
	if ( numBreakpoints() == 0 )
	{
		return end();
	}
			
	//	get the position of the first Breakpoint after time:
	Partial::const_iterator pos = findAfter( time );
	
	//	if there is an earlier Breakpoint that is closer in
	//	time, prefer that one:
	if ( pos != begin() )
	{
		Partial::const_iterator prev = pos;
		--prev;
		if ( pos == end() || pos.time() - time > time - prev.time() )
		{
			return prev;
		}
	}

	//	failing all else:	
	return pos;
} 

// ---------------------------------------------------------------------------
//	findNearest (non-const version)
// ---------------------------------------------------------------------------
//!	Return the insertion position for the Breakpoint nearest
//!	the specified time. Always returns a valid iterator (the
//!	position of the nearest-in-time Breakpoint) unless there
//!	are no Breakpoints.
//
Partial::iterator
Partial::findNearest( double time )
{
	//	if there are no Breakpoints, return end:
	if ( numBreakpoints() == 0 )
	{
		return end();
	}		
	//	get the position of the first Breakpoint after time:
	Partial::iterator pos = findAfter( time );
	
	//	if there is an earlier Breakpoint that is closer in
	//	time, prefer that one:
	if ( pos != begin() )
	{
		Partial::iterator prev = pos;
		--prev;
		if ( pos == end() || pos.time() - time > time - prev.time() )
		{
			return prev;
		}
	}

	//	failing all else:	
	return pos;
} 

// ---------------------------------------------------------------------------
//	frequencyAt
// ---------------------------------------------------------------------------
//!	Return the interpolated frequency (in Hz) of this Partial at the
//!	specified time. At times beyond the ends of the Partial, return
//!	the frequency at the nearest envelope endpoint. Throw an
//!	InvalidPartial exception if this Partial has no Breakpoints.
//
double
Partial::frequencyAt( double time ) const
{
    Breakpoint bp = parametersAt( time );
    return bp.frequency();
}

// ---------------------------------------------------------------------------
//	ShortestSafeFadeTime
// ---------------------------------------------------------------------------
//!	Define the default fade time for computing amplitude at the ends
//!	of a Partial. Floating point round-off errors make fadeTime == 0.0
//!	dangerous and unpredictable. 1 ns is short enough to prevent rounding
//!	errors in the least significant bit of a 48-bit mantissa for times
//!	up to ten hours.
//
const double Partial::ShortestSafeFadeTime = 1.0E-9;

// ---------------------------------------------------------------------------
//	amplitudeAt
// ---------------------------------------------------------------------------
//!	Return the interpolated amplitude of this Partial at the
//!	specified time. Throw an InvalidPartial exception if this 
//!	Partial has no Breakpoints. If non-zero fadeTime is specified, 
//!	then the amplitude at the ends of the Partial is coomputed using
//!	a linear fade. The default fadeTime is ShortestSafeFadeTime,
//!	see the definition of ShortestSafeFadeTime, above.
//	
double
Partial::amplitudeAt( double time, double fadeTime ) const
{
    Breakpoint bp = parametersAt( time, fadeTime );
    return bp.amplitude();
}


// ---------------------------------------------------------------------------
//	phaseAt
// ---------------------------------------------------------------------------
//!	Return the interpolated phase (in radians) of this Partial at
//!	the specified time. At times beyond the ends of the Partial,
//!	return the extrapolated from the nearest envelope endpoint
//!	(assuming constant frequency, as reported by frequencyAt()).
//!	
//! \param time is the time in seconds at which to evaluate the phase
//!
//! \throw Throw an InvalidPartial exception if this Partial has no
//!	Breakpoints.
//	
double
Partial::phaseAt( double time ) const
{
    Breakpoint bp = parametersAt( time );
    return bp.phase();
}

// ---------------------------------------------------------------------------
//	bandwidthAt
// ---------------------------------------------------------------------------
//!	Return the interpolated bandwidth (noisiness) coefficient of
//!	this Partial at the specified time. At times beyond the ends of
//!	the Partial, return the bandwidth coefficient at the nearest
//!	envelope endpoint. Throw an InvalidPartial exception if this
//!	Partial has no Breakpoints.
//	
double
Partial::bandwidthAt( double time ) const
{
    Breakpoint bp = parametersAt( time );
    return bp.bandwidth();
}

// ---------------------------------------------------------------------------
//  wrapPi
// ---------------------------------------------------------------------------
//  O'Donnell's phase wrapping function.
//
static inline double wrapPi( double x )
{
    using namespace std; // floor should be in std
    #define ROUND(x) (floor(.5 + (x)))
    const double TwoPi = 2.0*Pi;
    return x + ( TwoPi * ROUND(-x/TwoPi) );
}

// ---------------------------------------------------------------------------
//	parametersAt
// ---------------------------------------------------------------------------
//!	Return the interpolated parameters of this Partial at
//!	the specified time. If non-zero fadeTime is specified, then the
//!	amplitude at the ends of the Partial is coomputed using a 
//!	linear fade. The default fadeTime is ShortestSafeFadeTime.
//!	Throw an InvalidPartial exception if this Partial has no
//!	Breakpoints. 
//
Breakpoint
Partial::parametersAt( double time, double fadeTime ) const 
{
	if ( numBreakpoints() == 0 )
	{
		Throw( InvalidPartial, "Tried to interpolate a Partial with no Breakpoints." );
	}
	
	double freq, amp, bw, ph;			
	if ( startTime() >= time ) 
	{
		//	time is before the onset of the Partial:
		//	frequency is starting frequency, 
		//	amplitude is 0 (or fading), bandwidth is starting 
		//	bandwidth, and phase is rolled back.
		
		const Breakpoint & bp = first();
		double tstart = startTime();
		
		//  frequency:
		freq = bp.frequency();
		
		//  amplitude:
		amp = 0;
		if ( (fadeTime > 0) && ((tstart - time) < fadeTime) )
		{
			//	fade in ampltude if time is before the onset of the Partial:
			double alpha = 1. - ((tstart - time) / fadeTime);
			amp = alpha * bp.amplitude();
		}
		
        //  bandwidth:
        bw = bp.bandwidth();
        
		//  phase:
        double dp = 2. * Pi * (startTime() - time) * bp.frequency();
		ph = wrapPi( bp.phase() - dp );

	}
	else if ( endTime() <= time ) 
	{
		//	time is past the end of the Partial:
		//	frequency is ending frequency, 
		//	amplitude is 0 (or fading), bandwidth is ending 
		//	bandwidth, and phase is rolled forward.
		const Breakpoint & bp = last();	
        double tend = endTime();

		//  frequency:
		freq = bp.frequency();
		
		//  amplitude:		
		amp = 0;
		if ( (fadeTime > 0) && ((time - tend) < fadeTime) )
		{
			//	fade out ampltude if time is past the end of the Partial:
			double alpha = 1. - ((time - tend) / fadeTime);
			amp = alpha * bp.amplitude();
		}

        //  bandwidth:
        bw = bp.bandwidth();
        
        //  phase:
		double dp = 2. * Pi * (time - endTime()) * bp.frequency();
		ph = wrapPi( bp.phase() + dp );
	}
	else 
	{
        //	findAfter returns the position of the earliest
        //	Breakpoint later than time, or the end
        //	position if no such Breakpoint exists:
        Partial::const_iterator it = findAfter( time );
	
        //	interpolate between it and its predeccessor
        //	(we checked already that it is not begin or end):
        const Breakpoint & hi = it.breakpoint();
		double hitime = it.time();
        const Breakpoint & lo = (--it).breakpoint();
        double lotime = it.time();
        
        double alpha = (time - lotime) / (hitime - lotime);
		
        //  frequency:
        freq = (alpha * hi.frequency()) + ((1. - alpha) * lo.frequency());
			   
        //  amplitude:	
        amp = (alpha * hi.amplitude()) + ((1. - alpha) * lo.amplitude());

        //  bandwidth:
        bw = (alpha * hi.bandwidth()) + ((1. - alpha) * lo.bandwidth());
        
        //  phase:
        //  interpolated phase is computed from the interpolated frequency 
        //  and offset from the phase of the preceding Breakpoint:
        double favg = 0.5 * ( lo.frequency() + freq ); // + hi.frequency() );
        double dp = 2. * Pi * (time - lotime) * favg;                   
        ph = wrapPi( lo.phase() + dp );                        	
	}
	
	return Breakpoint( freq, amp, bw, ph );
}

}	//	end of namespace Loris
