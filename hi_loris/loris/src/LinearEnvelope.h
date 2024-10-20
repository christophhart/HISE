#ifndef INCLUDE_LINEARENVELOPE_H
#define INCLUDE_LINEARENVELOPE_H
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
 * LinearEnvelope.h
 *
 * Definition of class LinearEnvelope.
 *
 * Kelly Fitz, 23 April 2005
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */

#include "Envelope.h"
#include <map>

//  begin namespace
namespace Loris {

// ---------------------------------------------------------------------------
//  class LinearEnvelope
//
//! A LinearEnvelope represents a linear segment breakpoint function
//! with infinite extension at each end (that is, evalutaing the
//! envelope past either end of the breakpoint function yields the 
//! value at the nearest end point).
//!
//! LinearEnvelope implements the Envelope interface, described
//! by the abstract class Envelope. 
//!
//! LinearEnvelope inherits the types
//!     \li \c size_type
//!     \li \c value_type
//!     \li \c iterator
//!     \li \c const_iterator
//!
//! and the member functions
//!     \li size_type size( void ) const
//!     \li bool empty( void ) const
//!     \li iterator begin( void )
//!     \li const_iterator begin( void ) const
//!     \li iterator end( void )
//!     \li const_iterator end( void ) const
//!
//! from std::map< double, double >.
//
class LinearEnvelope : public Envelope, private std::map< double, double >
{
//  -- public interface --
public:
//  -- construction --

    //! Construct a new LinearEnvelope having no 
    //! breakpoints (and an implicit value of 0 everywhere).        
    LinearEnvelope( void );

    //! Construct and return a new LinearEnvelope having a 
    //! single breakpoint at 0 (and an implicit value everywhere)
    //! of initialValue.        
    //! 
    //! \param   initialValue is the value of this LinearEnvelope
    //!            at time 0.   
    explicit LinearEnvelope( double initialValue );

    //  compiler-generated copy, assignment, and destruction are OK.
    
//  -- Envelope interface --

    //! Return an exact copy of this LinearEnvelope
    //! (polymorphic copy, following the Prototype pattern).
    virtual LinearEnvelope * clone( void ) const;

    //! Return the linearly-interpolated value of this LinearEnvelope at 
    //! the specified time.
    //! 
    //! \param  t is the time at which to evaluate this 
    //!         LinearEnvelope.
    virtual double valueAt( double t ) const;   
        
    
//  -- envelope composition --

    //! Insert a breakpoint representing the specified (time, value) 
    //! pair into this LinearEnvelope. If there is already a 
    //! breakpoint at the specified time, it will be replaced with 
    //! the new breakpoint.
    //! 
    //! \param   time is the time at which to insert a new breakpoint
    //! \param   value is the value of the new breakpoint
    void insert( double time, double value );
    
    //! Insert a breakpoint representing the specified (time, value) 
    //! pair into this LinearEnvelope. Same as insert, retained
    //! for backwards-compatibility.
    //! 
    //! \param   time is the time at which to insert a new breakpoint
    //! \param   value is the value of the new breakpoint
    void insertBreakpoint( double time, double value ) 
         { insert( time, value ); }
         
         
    //! Add a constant value to this LinearEnvelope and return a reference
    //! to self.
    //!
    //! \param  offset is the value to add to all points in the envelope
    LinearEnvelope & operator+=( double offset );

    //! Subtract a constant value from this LinearEnvelope and return a reference
    //! to self.
    //!
    //! \param  offset is the value to subtract from all points in the envelope
    LinearEnvelope & operator-=( double offset )
    {
        return operator+=( -offset );
    }

    //! Scale this LinearEnvelope by a constant value and return a reference
    //! to self.
    //!
    //! \param  scale is the value by which to multiply to all points in 
    //!         the envelope
    LinearEnvelope & operator*=( double scale );

    //! Divide this LinearEnvelope by a constant value and return a reference
    //! to self.
    //!
    //! \param  div is the value by which to divide to all points in 
    //!         the envelope
    LinearEnvelope & operator/=( double div )
    {
        return operator*=( 1.0 / div );
    }

//  -- interface inherited from std::map --

    using std::map< double, double >::size;
    using std::map< double, double >::empty;
    using std::map< double, double >::clear;
    using std::map< double, double >::begin;
    using std::map< double, double >::end;
    using std::map< double, double >::size_type;
    using std::map< double, double >::value_type;
    using std::map< double, double >::iterator;
    using std::map< double, double >::const_iterator;

};  //  end of class LinearEnvelope


//  --  binary operators (inline nonmembers) --

//! Add a constant value to a LinearEnvelope and return a new 
//! LinearEnvelope.
inline
LinearEnvelope operator+( LinearEnvelope env, double offset )
{
    env += offset;
    return env;
}

//! Add a constant value to a LinearEnvelope and return a new 
//! LinearEnvelope.
inline
LinearEnvelope operator+( double offset, LinearEnvelope env )
{
    env += offset;
    return env;
}

//! Add two LinearEnvelopes and return a new LinearEnvelope.
inline
LinearEnvelope operator+( const LinearEnvelope & e1, const LinearEnvelope & e2 )
{
	LinearEnvelope ret;
	
	//	For each breakpoint in e1, insert a breakpoint having a value
	//	equal to the sum of the two envelopes at that time.
	for ( LinearEnvelope::const_iterator it = e1.begin(); it != e1.end(); ++it )
	{
		double t = it->first;
		double v = it->second;
			
		ret.insert( t, v + e2.valueAt( t ) );
	}
	
	//	For each breakpoint in e2, insert a breakpoint having a value
	//	equal to the sum of the two envelopes at that time.
	for ( LinearEnvelope::const_iterator it = e2.begin(); it != e2.end(); ++it )
	{
		double t = it->first;
		double v = it->second;
			
		ret.insert( t, v + e1.valueAt( t ) );
	}

    return ret;
}

//! Subtract a constant value from a LinearEnvelope and return a new 
//! LinearEnvelope.
inline
LinearEnvelope operator-( LinearEnvelope env, double offset )
{
    env -= offset;
    return env;
}

//! Subtract a LinearEnvelope from a constant value and return a new 
//! LinearEnvelope.
inline
LinearEnvelope operator-( double offset, LinearEnvelope env )
{
    env *= -1.0;
    env += offset;
    return env;
}

//! Subtract two LinearEnvelopes and return a new LinearEnvelope.
inline
LinearEnvelope operator-( const LinearEnvelope & e1, const LinearEnvelope & e2 )
{
	LinearEnvelope ret;
	
	//	For each breakpoint in e1, insert a breakpoint having a value
	//	equal to the difference between the two envelopes at that time.
	for ( LinearEnvelope::const_iterator it = e1.begin(); it != e1.end(); ++it )
	{
		double t = it->first;
		double v = it->second;
			
		ret.insert( t, v - e2.valueAt( t ) );
	}
	
	//	For each breakpoint in e2, insert a breakpoint having a value
	//	equal to the difference between the two envelopes at that time.
	for ( LinearEnvelope::const_iterator it = e2.begin(); it != e2.end(); ++it )
	{
		double t = it->first;
		double v = it->second;
			
		ret.insert( t, e1.valueAt( t ) - v );
	}

    return ret;
}

//! Scale a LinearEnvelope by a constant value and return a new 
//! LinearEnvelope.
inline
LinearEnvelope operator*( LinearEnvelope env, double scale )
{
    env *= scale;
    return env;
}

//! Scale a LinearEnvelope by a constant value and return a new 
//! LinearEnvelope.
inline
LinearEnvelope operator*( double scale, LinearEnvelope env )
{
    env *= scale;
    return env;
}

//! Divide a LinearEnvelope by a constant value and return a new 
//! LinearEnvelope.
inline
LinearEnvelope operator/( LinearEnvelope env, double div )
{
    env /= div;
    return env;
}

//! Divide constant value by a LinearEnvelope and return a new 
//! LinearEnvelope. No shortcut implementation for this one, 
//! don't inline.
LinearEnvelope operator/( double scale, LinearEnvelope env );


}   //  end of namespace Loris

#endif /* ndef INCLUDE_LINEARENVELOPE_H */
