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
 * NoiseGenerator.C
 *
 * Implementation of a class representing a gaussian noise generator, filtered and 
 * used as a modulator in bandwidth-enhanced synthesis.
 *
 * Kelly Fitz, 5 June 2003
 * revised 11 October 2009
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */

#if HAVE_CONFIG_H
	#include "config.h"
#endif

#include "NoiseGenerator.h"
#include <cmath>
#include <numeric>


//	begin namespace
namespace Loris {

// --- construction ---

// ---------------------------------------------------------------------------
//	(default) constructor
// ---------------------------------------------------------------------------
//!	Create a new noise generator with the (optionally) specified
//! seed (default is 1.0).
//!
//!	\param initSeed is the initial seed for the random number generator
//
NoiseGenerator::NoiseGenerator( double initSeed ) :
	m_useed( initSeed ),
	m_gset( 0 ),
	m_iset( false )
{
}

// ---------------------------------------------------------------------------
//	seed
// ---------------------------------------------------------------------------
//!	Re-seed the random number generator.
//!
//!	\param newSeed is the new seed for the random number generator
//
void 
NoiseGenerator::seed( double newSeed )
{
	m_useed = newSeed;
}

// --- random number generation ---

// ---------------------------------------------------------------------------
//	uniform random number generator
// ---------------------------------------------------------------------------
//	Taken from "Random Number Generators: Good Ones Are Hard To Find," 
//	Stephen Park and Keith Miller, Communications of the ACM, October 1988,
//	vol. 31, Number 10.
//
//	This version will work as long as floating point values are represented
//	with at least a 46 bit mantissa. The IEEE standard 64 bit floating point
//	format has a 53 bit mantissa.
//
//	The correctness of the implementation can be checked by confirming that
// 	after 10000 iterations, the seed, initialized to 1, is 1043618065.
//	I have confirmed this. 
//
//	I have also confirmed that it still works (is correct after 10000 
//	iterations) when I replace the divides with multiplies by oneOverM.
//
//	Returns a uniformly distributed random double on the range [0., 1.).
//
//	-kel 7 Nov 1997.
//
//	trunc() is a problem. It's not in cmath, officially, though
//	Metrowerks has it in there. SGI has it in math.h which is
//	(erroneously!) included in g++ cmath, but trunc is not imported
//	into std. For these two compilers, could just import std. But
//	trnc doesn't seem to exist anywhere in Linux g++, so use std::modf().
//	DON'T use integer conversion, because long int ins't as long 
//	as double's mantissa!
//
static inline double trunc( double x ) { double y; std::modf(x, &y); return y; }

inline double
NoiseGenerator::uniform( void )
{
	static const double a = 16807.L;
	static const double m = 2147483647.L;	// == LONG_MAX
	static const double oneOverM = 1.L / m;

	double temp = a * m_useed;
	m_useed = temp - m * trunc( temp * oneOverM );
	return m_useed * oneOverM;
}

// ---------------------------------------------------------------------------
//	gaussian_normal
// ---------------------------------------------------------------------------
//	Approximate the normal distribution using the Box-Muller transformation.
//	This is a better approximation and faster algorithm than the 12 u.v. sum.
//
//	This is slightly different than the thing I got off the web, I (have to)
//	assume (for now) that I knew what I was doing when I altered it.
//
inline double 
NoiseGenerator::gaussian_normal( void )
{
	//static int m_iset = 0;	//	boolean really, now member variables
	//static double m_gset;

	double r = 1., fac, v1, v2;
	
	if ( ! m_iset )
	{
		v1 = 2. * uniform() - 1.;
		v2 = 2. * uniform() - 1.;
		r = v1*v1 + v2*v2;
		while( r >= 1. )
		{
			// v1 = 2. * uniform() - 1.;
			v1 = v2;
			v2 = 2. * uniform() - 1.;
			r = v1*v1 + v2*v2;
		}

		fac = std::sqrt( -2. * std::log(r) / r );
		m_gset = v1 * fac;
		m_iset = true;
		return v2 * fac;
	}
	else
	{
		m_iset = false;
		return m_gset;
	}
}

// --- sample generation ---

// ---------------------------------------------------------------------------
//	sample
// ---------------------------------------------------------------------------
//!	Generate and return a new sample of Gaussian noise having zero
//! mean and unity standard deviation. Approximate the normal distribution 
//!	using the Box-Muller transformation applied to a uniform random number 
//!	generator taken from "Random Number Generators: Good Ones Are Hard To Find," 
//!	Stephen Park and Keith Miller, Communications of the ACM, October 1988,
//! vol. 31, Number 10.
//
double 
NoiseGenerator::sample( void )
{
	double sample = gaussian_normal();
	return sample;
}


}	//	end of namespace Loris
