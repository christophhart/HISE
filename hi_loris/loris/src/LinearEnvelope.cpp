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
 * LinearEnvelope.C
 *
 * Implementation of class LinearEnvelope.
 *
 * Kelly Fitz, 23 April 2005
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */

#if HAVE_CONFIG_H
	#include "config.h"
#endif

#include "LinearEnvelope.h"

//	begin namespace
namespace Loris {

// ---------------------------------------------------------------------------
//	constructor
// ---------------------------------------------------------------------------
//!	Construct a new LinearEnvelope having no 
//!	breakpoints (and an implicit value of 0 everywhere).		
//
LinearEnvelope::LinearEnvelope( void )
{
}

// ---------------------------------------------------------------------------
//	constructor with initial (or constant) value
// ---------------------------------------------------------------------------
//!	Construct and return a new LinearEnvelope having a 
//!	single breakpoint at 0 (and an implicit value everywhere)
//!	of initialValue.		
//!	
//!	\param   initialValue is the value of this LinearEnvelope
//!            at time 0.	
//
LinearEnvelope::LinearEnvelope( double initialValue )
{
	insertBreakpoint( 0., initialValue );
}

// ---------------------------------------------------------------------------
//	clone
// ---------------------------------------------------------------------------
//!	Return an exact copy of this LinearEnvelope
//!	(polymorphic copy, following the Prototype pattern).
//
LinearEnvelope * 
LinearEnvelope::clone( void ) const
{
	return new LinearEnvelope( *this );
}

// ---------------------------------------------------------------------------
//	insert
// ---------------------------------------------------------------------------
//!	Insert a breakpoint representing the specified (time, value) 
//!	pair into this LinearEnvelope. If there is already a 
//!	breakpoint at the specified time, it will be replaced with 
//!	the new breakpoint.
//!	
//!	\param   time is the time at which to insert a new breakpoint
//!	\param   value is the value of the new breakpoint
//	
void
LinearEnvelope::insert( double time, double value )
{
	(*this)[time] = value;
}

// ---------------------------------------------------------------------------
//	operator+=
// ---------------------------------------------------------------------------
//! Add a constant value to this LinearEnvelope and return a reference
//! to self.
//!
//! \param  offset is the value to add to all points in the envelope
LinearEnvelope & LinearEnvelope::operator+=( double offset )
{
    for ( iterator it = begin(); it != end(); ++it )
    {
        it->second += offset;
    }
    return *this;
}

// ---------------------------------------------------------------------------
//	operator*=
// ---------------------------------------------------------------------------
//! Scale this LinearEnvelope by a constant value and return a reference
//! to self.
//!
//! \param  scale is the value by which to multiply to all points in 
//!         the envelope
LinearEnvelope & LinearEnvelope::operator*=( double scale )
{
    for ( iterator it = begin(); it != end(); ++it )
    {
        it->second *= scale;
    }
    return *this;
}

// ---------------------------------------------------------------------------
//	operator/ (non-member binary operator)
// ---------------------------------------------------------------------------
//! Divide constant value by a LinearEnvelope and return a new 
//! LinearEnvelope. No shortcut implementation for this one, 
//! don't inline.
LinearEnvelope operator/( double num, LinearEnvelope env )
{
    for ( LinearEnvelope::iterator it = env.begin(); it != env.end(); ++it )
    {
        it->second = num / it->second;
    }
    
    return env;
}

// ---------------------------------------------------------------------------
//	valueAt
// ---------------------------------------------------------------------------
//!	Return the linearly-interpolated value of this LinearEnvelope at 
//!	the specified time.
//!	
//!	\param   t is the time at which to evaluate this LinearEnvelope.
//
double
LinearEnvelope::valueAt( double t ) const
{
	//	return zero if no breakpoints have been specified:
	if ( size() == 0 ) 
	{
		return 0.;
	}

	const_iterator it = lower_bound( t );

	if ( it == begin() ) 
	{
		//	t is less than the first breakpoint, extend:
		return it->second;
	}
	else if ( it == end() ) 
	{
		//	t is greater than the last breakpoint, extend:
		// 	(no direct way to access the last element of a map)
		return (--it)->second;
	}
	else 
	{
		//	linear interpolation between consecutive breakpoints:
		double xgreater = it->first;
		double ygreater = it->second;
		--it;
		double xless = it->first;
		double yless = it->second;
		
		double alpha = (t -  xless) / (xgreater - xless);
		return ( alpha * ygreater ) + ( (1. - alpha) * yless );
	}
}

}	//	end of namespace Loris
