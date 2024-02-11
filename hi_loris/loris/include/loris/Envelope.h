#ifndef INCLUDE_ENVELOPE_H
#define INCLUDE_ENVELOPE_H
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
 * Envelope.h
 *
 * Definition of abstract interface class Envelope.
 *
 * Kelly Fitz, 21 July 2000
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */

#include <memory>	//	 for autoptr

//	begin namespace
namespace Loris {

// ---------------------------------------------------------------------------
//	class Envelope
//
//! Envelope is an base class for objects representing real functions
//! of time.
//!
//! Class Envelope is an abstract base class, specifying interface for
//! prototypable (clonable) objects representing generic, real-valued
//! (double) functions of one real-valued (double) time argument. Derived
//! classes (like BreakpointEnvelope) must implement valueAt() and
//! clone(), the latter to support the Prototype pattern. Clients of
//! Envelope, like Morpher and Distiller, can use prototype Envelopes to
//! make their own private Envelopes.
//!
//! \sa Distiller, Envelope, Morpher
//
class Envelope
{
//	-- public interface --
public:
//	-- construction --

    // allow compiler to generate constructors

	//!	Destroy this Envelope (virtual to allow subclassing).
	virtual ~Envelope( void );

//	-- Envelope interface --

	//!	Return an exact copy of this Envelope (following the Prototype
	//!	pattern).
	virtual Envelope * clone( void ) const = 0;

	//!	Return the value of this Envelope at the specified time. 	 
	virtual double valueAt( double x ) const = 0;	
	
};	//	end of abstract class Envelope


// ---------------------------------------------------------------------------
//	class ScaleAndOffsetEnvelope
//
//! ScaleAndOffsetEnvelope is an derived Envelope class for objects 
//! representing envelopes having a scale and offset applied (in that order).

class ScaleAndOffsetEnvelope : public Envelope
{
//	-- public interface --
public:
//	-- construction --

    //! Construct a new envelope that is a scaled and offset 
    //! version of another.
    ScaleAndOffsetEnvelope( const Envelope & e, double scale, double offset ) :
    	m_env( e.clone() ),
    	m_scale( scale ),
    	m_offset( offset )
    {
    }

	//!	Construct a copy of an envelope.
	ScaleAndOffsetEnvelope( const ScaleAndOffsetEnvelope & rhs ) :
		m_env( rhs.m_env->clone() ),
    	m_scale( rhs.m_scale ),
    	m_offset( rhs.m_offset )
    {
    }

	//!	Assignment from another envelope.
	ScaleAndOffsetEnvelope & 
	operator=( const ScaleAndOffsetEnvelope & rhs )
	{
        if ( &rhs != this )
        {
            m_env.reset( rhs.m_env->clone() );
            m_scale = rhs.m_scale;
            m_offset = rhs.m_offset;
        }
        return *this;
    }

//	-- Envelope interface --

	//!	Return an exact copy of this Envelope (following the Prototype
	//!	pattern).
	ScaleAndOffsetEnvelope * clone( void ) const 
	{
		return new ScaleAndOffsetEnvelope( *this );
	}

	//!	Return the value of this Envelope at the specified time. 	 
	virtual double valueAt( double x ) const
	{
		return m_offset + ( m_scale * m_env->valueAt( x ) );
	}
	
//  -- private member variables --

private:

    std::auto_ptr< Envelope > m_env;   	
    double m_scale, m_offset;
	
};	//	end of class ScaleAndOffsetEnvelope


// ---------------------------------------------------------------------------
//	math operators
// ---------------------------------------------------------------------------

inline
ScaleAndOffsetEnvelope
operator*( const Envelope & e, double x )
{
	return ScaleAndOffsetEnvelope( e, x, 0 );
}

inline
ScaleAndOffsetEnvelope
operator*( double x, const Envelope & e )
{
	return e * x;
}




}	//	end of namespace Loris

#endif /* ndef INCLUDE_ENVELOPE_H */
