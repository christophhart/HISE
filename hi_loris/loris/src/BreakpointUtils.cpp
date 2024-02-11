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
 * BreakpointUtils.C
 *
 * Out-of-line Breakpoint utility functions collected in namespace BreakpointUtils.
 *
 * Kelly Fitz, 19 June 2003
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */

#if HAVE_CONFIG_H
	#include "config.h"
#endif

#include "BreakpointUtils.h"
#include "Breakpoint.h"

#include <cmath>
#if defined(HAVE_M_PI) && (HAVE_M_PI)
	const double Pi = M_PI;
#else
	const double Pi = 3.14159265358979324;
#endif

//	begin namespace
namespace Loris {

// ---------------------------------------------------------------------------
//	makeNullBefore
// ---------------------------------------------------------------------------
// 	Return a null (zero-amplitude) Breakpoint to preceed the specified 
//	Breakpoint, useful for fading in a Partial.
//
Breakpoint 
BreakpointUtils::makeNullBefore( const Breakpoint & bp, double fadeTime )
{
	Breakpoint ret( bp );
	// adjust phase
	double dp = 2. * Pi * fadeTime * bp.frequency();
	ret.setPhase( std::fmod( bp.phase() - dp, 2. * Pi ) );
	ret.setAmplitude(0.);
	ret.setBandwidth(0.);
	
	return ret;
}

// ---------------------------------------------------------------------------
//	makeNullAfter
// ---------------------------------------------------------------------------
// 	Return a null (zero-amplitude) Breakpoint to succeed the specified 
//	Breakpoint, useful for fading out a Partial.
//
Breakpoint 
BreakpointUtils::makeNullAfter( const Breakpoint & bp, double fadeTime )
{
	Breakpoint ret( bp );
	// adjust phase
	double dp = 2. * Pi * fadeTime * bp.frequency();
	ret.setPhase( std::fmod( bp.phase() + dp, 2. * Pi ) );
	ret.setAmplitude(0.);
	ret.setBandwidth(0.);

	return ret;
}

}	//	end of namespace Loris
