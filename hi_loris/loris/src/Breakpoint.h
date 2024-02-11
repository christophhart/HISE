#ifndef INCLUDE_BREAKPOINT_H
#define INCLUDE_BREAKPOINT_H
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
 * Breakpoint.h
 *
 * Definition of class Loris::Breakpoint.
 *
 * Kelly Fitz, 16 Aug 99
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */

//	begin namespace
namespace Loris {


// ---------------------------------------------------------------------------
//	class Breakpoint
//	
//!	Class Breakpoint represents a single breakpoint in the
//!	Partial parameter (frequency, amplitude, bandwidth) envelope.
//!	Instantaneous phase is also stored, but is only used at the onset of 
//!	a partial, or when it makes a transition from zero to nonzero amplitude.
//!	
//!	Loris Partials represent reassigned bandwidth-enhanced model components.
//!	A Partial consists of a chain of Breakpoints describing the time-varying
//!	frequency, amplitude, and bandwidth (noisiness) of the component.
//!	For more information about Reassigned Bandwidth-Enhanced 
//!	Analysis and the Reassigned Bandwidth-Enhanced Additive Sound 
//!	Model, refer to the Loris website: 
//!	www.cerlsoundgroup.org/Loris/.
//!	
//!	Breakpoint is a leaf class, do not subclass.
//
class Breakpoint
{
//	-- instance variables --
	double _frequency;	//!	in Hertz
	double _amplitude;	//!	absolute
	double _bandwidth;	//!	fraction of total energy that is noise energy
	double _phase;		//!	radians
	
//	-- public Breakpoint interface --

public:
//	-- construction --

	//!	Construct a new Breakpoint with all parameters initialized to 0
	//!	(needed for STL containability).
 	Breakpoint( void );	

	//!	Construct a new Breakpoint with the specified parameters.
	//!	
	//!	\param 	f is the intial frequency.
	//!	\param 	a is the initial amplitude.
	//!	\param 	b is the initial bandwidth.
	//!	\param 	p is the initial phase, if specified (if unspecified, 0 
	//!			is assumed).
 	Breakpoint( double f, double a, double b, double p = 0. );

	//	(use compiler-generated destructor, copy, and assign)
	
//	-- access --
	//!	Return the amplitude of this Breakpoint.
 	double amplitude( void ) const { return _amplitude; }

	//!	Return the bandwidth (noisiness) coefficient of this Breakpoint.
 	double bandwidth( void ) const { return _bandwidth; }

	//!	Return the frequency of this Breakpoint.
  	double frequency( void ) const { return _frequency; }

	//!	Return the phase of this Breakpoint.
	double phase( void ) const { return _phase; }
	
//	-- mutation --
	//!	Set the amplitude of this Breakpoint.
	//!
	//!	\param x is the new amplitude
 	void setAmplitude( double x ) { _amplitude = x; }

	//!	Set the bandwidth (noisiness) coefficient of this Breakpoint.
	//!
	//!	\param x is the new bandwidth
 	void setBandwidth( double x ) { _bandwidth = x; }

	//!	Set the frequency of this Breakpoint.
	//!
	//!	\param x is the new frequency.
 	void setFrequency( double x ) { _frequency = x; }

	//!	Set the phase of this Breakpoint.
	//!
	//!	\param x is the new phase.
 	void setPhase( double x ) { _phase = x; }
	 
	//!	Add noise (bandwidth) energy to this Breakpoint by computing new 
	//!	amplitude and bandwidth values. enoise may be negative, but 
	//!	noise energy cannot be removed (negative energy added) in excess 
	//!	of the current noise energy.
	//!	
	//!	\param 	enoise is the amount of noise energy to add to
	//!			this Breakpoint.
	void addNoiseEnergy( double enoise );
	
};	//	end of class Breakpoint

}	//	end of namespace Loris

#endif /* ndef INCLUDE_BREAKPOINT_H */
