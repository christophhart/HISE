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
 * Breakpoint.C
 *
 * Implementation of class Loris::Breakpoint.
 *
 * Kelly Fitz, 16 Aug 99
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */

#if HAVE_CONFIG_H
	#include "config.h"
#endif

#include "Breakpoint.h"
#include "LorisExceptions.h"
#include <cmath>

//	begin namespace
namespace Loris {


// -
// construction

// ---------------------------------------------------------------------------
//	Breakpoint default constructor
// ---------------------------------------------------------------------------
//!	Construct a new Breakpoint with all parameters initialized to 0
//!	(needed for STL containability).
Breakpoint::Breakpoint( void ) :
	_frequency( 0. ),
	_amplitude( 0. ),
	_bandwidth( 0. ),
	_phase( 0. )
{
}

// ---------------------------------------------------------------------------
//	Breakpoint constructor
// ---------------------------------------------------------------------------
//!	Construct a new Breakpoint with the specified parameters.
//!	
//!	\param f is the intial frequency.
//!	\param a is the initial amplitude.
//!	\param b is the initial bandwidth.
//!	\param p is the initial phase, if specified (if unspecified, 0 
//!	is assumed).
//
Breakpoint::Breakpoint( double f, double a, double b, double p ) :
	_frequency( f ),
	_amplitude( a ),
	_bandwidth( b ),
	_phase( p )
{
}

// ---------------------------------------------------------------------------
//	addNoiseEnergy
// ---------------------------------------------------------------------------
//!	Add noise (bandwidth) energy to this Breakpoint by computing new 
//!	amplitude and bandwidth values. enoise may be negative, but 
//!	noise energy cannot be removed (negative energy added) in excess 
//!	of the current noise energy.
//!	
//!	\param enoise is the amount of noise energy to add to
//!	this Breakpoint.
//
void 
Breakpoint::addNoiseEnergy( double enoise )
{
	//	compute current energies:
	double e = amplitude() * amplitude();	//	current total energy
	double n = e * bandwidth();				//	current noise energy
	
	//	Assert( e >= n );
	//	could complain, but its recoverable, just fix it:
	if ( e < n )
    {
		e = n;
	}
    
	//	guard against divide-by-zero, and don't allow
	//	the sinusoidal energy to decrease:
	if ( n + enoise > 0. ) 
	{
		//	if new noise energy is positive, total
		//	energy must also be positive:
		//	Assert( e + enoise > 0 );
		setBandwidth( (n + enoise) / (e + enoise) );
		setAmplitude( std::sqrt(e + enoise) );
	}
	else 
	{
		//	if new noise energy is negative, leave 
		//	all sinusoidal energy:
		setBandwidth( 0. );
		setAmplitude( std::sqrt( e - n ) );
	}
}

}	//	end of namespace Loris




