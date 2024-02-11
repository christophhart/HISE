#ifndef INCLUDE_PARTIALBUILDER_H
#define INCLUDE_PARTIALBUILDER_H
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
 * PartialBuilder.h
 *
 * Implementation of a class representing a policy for connecting peaks
 * extracted from a reassigned time-frequency spectrum to form ridges
 * and construct Partials.
 *
 * This strategy attemps to follow a reference frequency envelope when 
 * forming Partials, by prewarping all peak frequencies according to the
 * (inverse of) frequency reference envelope. At the end of the analysis, 
 * Partial frequencies need to be un-warped by calling fixPartialFrequencies().
 *
 * Kelly Fitz, 28 May 2003
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */
 
#include "Partial.h"
#include "PartialList.h"
#include "PartialPtrs.h"
#include "SpectralPeaks.h"

#include <memory>

//	begin namespace
namespace Loris {

class Envelope;

// ---------------------------------------------------------------------------
//	class PartialBuilder
//
//	A class representing the process of connecting peaks (ridges) on a 
//	reassigned time-frequency surface to form Partials.
//
class PartialBuilder
{
// --- public interface ---

public:

	//	constructor
    //
    //  Construct a new builder that constrains Partial frequnecy
    //  drift by the specified drift value in Hz.
	PartialBuilder( double drift );    
    
	//	constructor
    //
    //  Construct a new builder that constrains Partial frequnecy
    //  drift by the specified drift value in Hz. The frequency
    //  warping envelope is applied to the spectral peak frequencies
    //  and the frequency drift parameter in each frame before peaks
    //  are linked to eligible Partials. All the Partial frequencies
    //  need to be un-warped at the ned of the building process, by
    //  calling finishBuilding().
	PartialBuilder( double drift, const Envelope & freqWarpEnv );
	
    //  buildPartials
    //
    //	Append spectral peaks, extracted from a reassigned time-frequency
    //	spectrum, to eligible Partials, where possible. Peaks that cannot
    //	be used to extend eliglble Partials spawn new Partials.
    //
    //	This is similar to the basic MQ partial formation strategy, except that
    //	before matching, all frequencies are normalized by the value of the 
    //	warping envelope at the time of the current frame. This means that
    //	the frequency envelopes of all the Partials are warped, and need to 
    //	be un-normalized by calling finishBuilding at the end of the building
    //  process.
	void buildPartials( Peaks & peaks, double frameTime );

    //  finishBuilding
    //
	//	Un-do the frequency warping performed in buildPartials, and return 
	//	the Partials that were built. After calling finishBuilding, the
    //  builder is returned to its initial state, and ready to build another
    //  set of Partials. Partials are returned by appending them to the 
    //  supplied PartialList.
	void finishBuilding( PartialList & product );

private:

// --- auxiliary member functions ---

    double freq_distance( const Partial & partial, const SpectralPeak & pk );

    bool better_match( const Partial & part, const SpectralPeak & pk1,
		   	           const SpectralPeak & pk2 );
    bool better_match( const Partial & part1, 
	  		           const Partial & part2, const SpectralPeak & pk );                       
                       
                       
// --- collected partials ---

	PartialList mCollectedPartials;             //	collect partials here

// --- builder state variables ---
		
	PartialPtrs mEligiblePartials;
    PartialPtrs mNewlyEligible;                 // 	keep track of eligible partials here

// --- parameters ---
    	
	std::auto_ptr< Envelope > mFreqWarping;	//	reference envelope
    
	double mFreqDrift;
    	
// --- disallow copy and assignment ---
    
    PartialBuilder( const PartialBuilder & );
    PartialBuilder& operator=( const PartialBuilder & );
    
};	//	end of class PartialBuilder

}	//	end of namespace Loris

#endif /* ndef INCLUDE_PARTIALBUILDER_H */
