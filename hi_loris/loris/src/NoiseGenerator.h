#ifndef NOISEGENERATOR_H
#define NOISEGENERATOR_H
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
 * NoiseGenerator.h
 *
 * Definition of a class representing a gaussian noise generator, filtered and 
 * used as a modulator in bandwidth-enhanced synthesis.
 *
 * Kelly Fitz, 5 June 2003
 * revised 11 October 2009
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */

//	begin namespace
namespace Loris {

// ---------------------------------------------------------------------------
//	class NoiseGenerator
//
class NoiseGenerator
{
//	--- interface ---

public:

	//!	Create a new noise generator with the (optionally) specified
	//! seed (default is 1.0).
	//!
	//!	\param initSeed is the initial seed for the random number generator
	explicit NoiseGenerator( double initSeed = 1.0 );


	//	copy and assign are free
	
	
	//!	Re-seed the random number generator.
	//!
	//!	\param newSeed is the new seed for the random number generator
	void seed( double newSeed );
	
	//	sample
	//
	//!	Generate and return a new sample of Gaussian noise having zero
	//! mean and unity standard deviation. Approximate the normal distribution 
	//!	using the Box-Muller transformation applied to a uniform random number 
	//!	generator taken from "Random Number Generators: Good Ones Are Hard To Find," 
	//!	Stephen Park and Keith Miller, Communications of the ACM, October 1988,
	//! vol. 31, Number 10.
	double sample( void );
	
	//! Function call operator, same as calling sample().
	//!	
	//!	\sa sample
	double operator() ( void ) 	{ return sample(); }
	

//	--- implementation ---
private:

	//	random number generation helpers
	inline double uniform( void );
	inline double gaussian_normal( void );


	// random number generator state variables
	double m_useed;
	double m_gset;
	bool m_iset;
	
};


}	//	end of namespace Loris

#endif	/* ndef NOISEGENERATOR_H */
