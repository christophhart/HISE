#ifndef INCLUDE_BREAKPOINTUTILS_H
#define INCLUDE_BREAKPOINTUTILS_H
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
 * BreakpointUtils.h
 *
 * Breakpoint utility functions collected in namespace BreakpointUtils.
 *
 * Kelly Fitz, 6 July 2000
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */

#include "Breakpoint.h"
#include <algorithm>
#include <functional>

//	begin namespace
namespace Loris {

namespace BreakpointUtils {

//	-- free functions --

// ---------------------------------------------------------------------------
//	addNoiseEnergy
// ---------------------------------------------------------------------------
//!	Add noise (bandwidth) energy to a Breakpoint by computing new 
//!	amplitude and bandwidth values. enoise may be negative, but 
//!	noise energy cannot be removed (negative energy added) in excess 
//!	of the current noise energy.
//!	
//!	\deprecated This operation is now part of the Breakpoint interface.
//!				Use Breakpoint::addNoiseEnergy instead.
//
inline void addNoiseEnergy( Breakpoint & bp, double enoise ) 
{ 
	bp.addNoiseEnergy(enoise); 
}
 
// ---------------------------------------------------------------------------
//	makeNullBefore
// ---------------------------------------------------------------------------
//!	Return a null (zero-amplitude) Breakpoint to preceed the specified 
//!	Breakpoint, useful for fading in a Partial.
//!
//! \param	bp make a null Breakpoint to preceed this one
//! \param	fadeTime the time (in seconds) by which the new null Breakpoint
//!			should preceed bp
//! \return	a new null Breakpoint, having zero amplitude, frequency equal
//!			to that of bp, and phase computed back from that of bp
//
Breakpoint makeNullBefore( const Breakpoint & bp, double fadeTime ); // see BreakpointUtils.C


// ---------------------------------------------------------------------------
//	addNoiseEnergy
// ---------------------------------------------------------------------------
//!	Return a null (zero-amplitude) Breakpoint to succeed the specified 
//!	Breakpoint, useful for fading out a Partial.
//!
//! \param	bp make a null Breakpoint to succeed this one
//! \param	fadeTime the time (in seconds) by which the new null Breakpoint
//!			should succeed bp
//! \return	a new null Breakpoint, having zero amplitude, frequency equal
//!			to that of bp, and phase computed forward from that of bp
//
Breakpoint makeNullAfter( const Breakpoint & bp, double fadeTime ); // see BreakpointUtils.C

//	-- predicates --

// ---------------------------------------------------------------------------
//	isFrequencyBetween
//	
//!	Predicate functor returning true if its Breakpoint argument 
//!	has frequency between specified bounds, and false otherwise.
//
class isFrequencyBetween : 
	public std::unary_function< const Breakpoint, bool >
{
public:
	//! Return true if its Breakpoint argument has frequency 
	//! between specified bounds, and false otherwise.
	bool operator()( const Breakpoint & b )  const
	{ 
		return (b.frequency() > _fmin) && 
			   (b.frequency() < _fmax); 
	}
	
//	constructor:

	//! Construct a predicate functor, specifying two frequency bounds.
	isFrequencyBetween( double x, double y ) : 
		_fmin( x ), _fmax( y ) 
	{ 
		if (x>y) std::swap(x,y); 
	}
		
//	bounds:
private:
	double _fmin, _fmax;
};

//! Old name for isFrequencyBetween.
//! \deprecated use isFrequencyBetween instead.
typedef isFrequencyBetween frequency_between;

// ---------------------------------------------------------------------------
//	isNonNull
//
//!	Predicate functor returning true if a Breakpoint has non-zero 
//! amplitude, false otherwise.
//
static bool isNonNull( const Breakpoint & bp )
{
	return bp.amplitude() != 0.;
}

// ---------------------------------------------------------------------------
//	isNull
//
//!	Predicate functor returning true if a Breakpoint has zero 
//! amplitude, false otherwise.
//
static bool isNull( const Breakpoint & bp )
{
	return ! isNonNull( bp );
}

//	-- comparitors --

// ---------------------------------------------------------------------------
//	compareFrequencyLess
//	
//!	Comparitor (binary) functor returning true if its first Breakpoint
//!	argument has frequency less than that of its second Breakpoint argument,
//!	and false otherwise.
//
class compareFrequencyLess : 
	public std::binary_function< const Breakpoint, const Breakpoint, bool >
{
public:
	//! Return true if its first Breakpoint argument has frequency less 
	//! than that of its second Breakpoint argument, and false otherwise.
	bool operator()( const Breakpoint & lhs, const Breakpoint & rhs ) const
		{ return lhs.frequency() < rhs.frequency(); }
};

//! Old name for compareFrequencyLess.
//! \deprecated use compareFrequencyLess instead.
typedef compareFrequencyLess less_frequency;

// ---------------------------------------------------------------------------
//	compareAmplitudeGreater
//	
//!	Comparitor (binary) functor returning true if its first Breakpoint
//!	argument has amplitude greater than that of its second Breakpoint argument,
//!	and false otherwise.
//
class compareAmplitudeGreater : 
	public std::binary_function< const Breakpoint, const Breakpoint, bool >
{
public:
	//!	Return true if its first Breakpoint argument has amplitude greater 
	//!	than that of its second Breakpoint argument, and false otherwise.
	bool operator()( const Breakpoint & lhs, const Breakpoint & rhs ) const
		{ return lhs.amplitude() > rhs.amplitude(); }
};	

//! Old name for compareAmplitudeGreater.
//! \deprecated use compareAmplitudeGreater instead.
typedef compareAmplitudeGreater greater_amplitude;

// ---------------------------------------------------------------------------
//	compareAmplitudeLess
//	
//!	Comparitor (binary) functor returning true if its first Breakpoint
//!	argument has amplitude less than that of its second Breakpoint argument,
//!	and false otherwise.
//
class compareAmplitudeLess : 
	public std::binary_function< const Breakpoint, const Breakpoint, bool >
{
public:
	//!	Return true if its first Breakpoint argument has amplitude greater 
	//!	than that of its second Breakpoint argument, and false otherwise.
	bool operator()( const Breakpoint & lhs, const Breakpoint & rhs ) const
		{ return lhs.amplitude() < rhs.amplitude(); }
};	

}	//	end of namespace BreakpointUtils

}	//	end of namespace Loris

#endif /* ndef INCLUDE_BREAKPOINTUTILS_H */
