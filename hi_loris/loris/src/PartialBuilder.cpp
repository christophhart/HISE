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
 * PartialBuilder.C
 *
 * Implementation of a class representing a policy for connecting peaks
 * extracted from a reassigned time-frequency spectrum to form ridges
 * and construct Partials.
 *
 * This strategy attemps to follow a mFreqWarping frequency envelope when 
 * forming Partials, by prewarping all peak frequencies according to the
 * (inverse of) frequency mFreqWarping envelope. At the end of the analysis, 
 * Partial frequencies need to be un-warped by calling fixPartialFrequencies().
 *
 * The first attempt was the same as the basic partial formation strategy, 
 * but for purposes of matching, peak frequencies are scaled by the ratio
 * of the mFreqWarping envelope's value at the previous frame to its value
 * at the current frame. This was not adequate, didn't store enough history
 * so it wasn't really following the reference envelope, just using it to
 * make a local decision about how frequency should drift from one frame to
 * the next.
 *
 * Kelly Fitz, 28 May 2003
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */

#if HAVE_CONFIG_H
	#include "config.h"
#endif

#include "PartialBuilder.h"

#include "BreakpointEnvelope.h"
#include "Envelope.h"
#include "Notifier.h"
#include "Partial.h"
#include "PartialList.h"
#include "PartialPtrs.h"
#include "SpectralPeaks.h"

#include <algorithm>
#include <cmath>

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
//  HEY - remove mMaxTimeOffset and the hopTime argument, these are wrong
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------

//	begin namespace
namespace Loris {

// ---------------------------------------------------------------------------
//	construction
// ---------------------------------------------------------------------------
//  Construct a new builder that constrains Partial frequnecy
//  drift by the specified drift value in Hz.
//
PartialBuilder::PartialBuilder( double drift ) :
	mFreqWarping( new BreakpointEnvelope(1.0) ),
	mFreqDrift( drift )
{
}

// ---------------------------------------------------------------------------
//	construction
// ---------------------------------------------------------------------------
//  Construct a new builder that constrains Partial frequnecy
//  drift by the specified drift value in Hz. The frequency
//  warping envelope is applied to the spectral peak frequencies
//  and the frequency drift parameter in each frame before peaks
//  are linked to eligible Partials. All the Partial frequencies
//  need to be un-warped at the ned of the building process, by
//  calling finishBuilding().
//
PartialBuilder::PartialBuilder( double drift, const Envelope & env ) :
	mFreqWarping( env.clone() ),
	mFreqDrift( drift )
{
}

// --- local helpers for Partial building ---

// ---------------------------------------------------------------------------
//	end_frequency
// ---------------------------------------------------------------------------
//	Return the frequency of the last Breakpoint in a Partial.
//
static inline double end_frequency( const Partial & partial )
{
	return partial.last().frequency();
}

// ---------------------------------------------------------------------------
//	freq_distance
// ---------------------------------------------------------------------------
//	Helper function, used in formPartials().
//	Returns the (positive) frequency distance between a Breakpoint 
//	and the last Breakpoint in a Partial.
//
inline double 
PartialBuilder::freq_distance( const Partial & partial, const SpectralPeak & pk )
{
    double normBpFreq = pk.frequency() / mFreqWarping->valueAt( pk.time() );
    
    double normPartialEndFreq = 
        partial.last().frequency() / mFreqWarping->valueAt( partial.endTime() );
    
	return std::fabs( normPartialEndFreq - normBpFreq );
}

// ---------------------------------------------------------------------------
//	better_match
// ---------------------------------------------------------------------------
//	Predicate for choosing the better of two proposed
//	Partial-to-Breakpoint matches. Note: sometimes this
//  is used to compare two candidate Breakpoint matches
//  to the same Partial, other times to candidate Partials
//  to the same Breakpoint.
//
//	Return true if the first match is better, otherwise
//	return false.
//

bool PartialBuilder::better_match( const Partial & part, const SpectralPeak & pk1,
                                   const SpectralPeak & pk2 )
{
	Assert( part.numBreakpoints() > 0 );
	
	return freq_distance( part, pk1 ) < freq_distance( part, pk2 );
}	                                   
                                   
bool PartialBuilder::better_match( const Partial & part1, 
                                   const Partial & part2, const SpectralPeak & pk )
{
	Assert( part1.numBreakpoints() > 0 );
	Assert( part2.numBreakpoints() > 0 );
	
	return freq_distance( part1, pk ) < freq_distance( part2, pk );
}	

// --- Partial building members ---

// ---------------------------------------------------------------------------
//	buildPartials
// ---------------------------------------------------------------------------
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
//
void 
PartialBuilder::buildPartials( Peaks & peaks, double frameTime )
{	
	mNewlyEligible.clear();
	
	unsigned int matchCount = 0;	//	for debugging
		
	//	frequency-sort the spectral peaks:
	//	(the eligible partials are always sorted by
	//	increasing frequency if we always sort the
	//	peaks this way)
	std::sort( peaks.begin(), peaks.end(), SpectralPeak::sort_increasing_freq );
	
	PartialPtrs::iterator eligible = mEligiblePartials.begin();
	for ( Peaks::iterator bpIter = peaks.begin(); bpIter != peaks.end(); ++bpIter ) 
	{
		//const Breakpoint & bp = bpIter->breakpoint;
		const double peakTime = frameTime + bpIter->time();
		
		// 	find the Partial that is nearest in frequency to the Peak:
		PartialPtrs::iterator nextEligible = eligible;
		if ( eligible != mEligiblePartials.end() &&
			 end_frequency( **eligible ) < bpIter->frequency() )
		{
			++nextEligible;
			while ( nextEligible != mEligiblePartials.end() &&
					end_frequency( **nextEligible ) < bpIter->frequency() )
			{
				++nextEligible;
				++eligible;
			}
			
			if ( nextEligible != mEligiblePartials.end() &&
				 better_match( **nextEligible, **eligible, *bpIter ) )
			{
				eligible = nextEligible;
			}
		}
		
		// 	INVARIANT:
		//
		//	eligible is the position of the nearest (in frequency)
		//	eligible Partial (pointer) or it is mEligiblePartials.end().
		//
		//	nextEligible is the eligible Partial with frequency 
		//	greater than bp, or it is mEligiblePartials.end().  
              
#if defined(Debug_Loris) && Debug_Loris
        /*
		if ( nextEligible != mEligiblePartials.end() )
		{
			debugger << matchFrequency << "( " << end_frequency( **eligible )
					 << ", " << end_frequency( **nextEligible ) << ")" << endl;
		}
        */
#endif
								
		//	create a new Partial if there is no eligible Partial,
		//	or the frequency difference to the eligible Partial is 
		//	too great, or the next peak is a better match for the 
		//	eligible Partial, otherwise add this peak to the eligible
		//	Partial:
		Peaks::iterator nextPeak = //Peaks::iterator( bpIter ); ++nextPeak;
								   ++Peaks::iterator( bpIter ); //  some compilers choke on this?

        //  decide whether this match should be made:
        //  - can only make the match if eligible is not the end of the list
        //  - the match is only good if it is close enough in frequency
        //  - even if the match is good, only match if the next one is not better
        bool makeMatch = false;
        if ( eligible != mEligiblePartials.end() )
        {
            bool matchIsGood =  mFreqDrift > 
                std::fabs( end_frequency( **eligible ) - bpIter->frequency() );
            if ( matchIsGood )
            {
                bool nextIsBetter = ( nextPeak != peaks.end() &&
                                      better_match( **eligible, *nextPeak, *bpIter ) ); 
                if ( ! nextIsBetter )
                {
                    makeMatch = true;
                }
            }                       
        }
        
        Breakpoint bp = bpIter->createBreakpoint();
        
        if ( makeMatch )
        {
            //  invariant:
            //  if makeMatch is true, then eligible is the position of a valid Partial
            (*eligible)->insert( peakTime, bp );
			mNewlyEligible.push_back( *eligible );
			
			++matchCount;
        }
        else
        {
            Partial p;
            p.insert( peakTime, bp );
            mCollectedPartials.push_back( p );
            mNewlyEligible.push_back( & mCollectedPartials.back() );
        }
        
		//	update eligible, nextEligible is the eligible Partial
		//	with frequency greater than bp, or it is mEligiblePartials.end():
		eligible = nextEligible;
	}			 
	 	
	mEligiblePartials = mNewlyEligible;
	
    /*
	debugger << "PartialBuilder::buildPartials: matched " << matchCount << endl;
	debugger << "PartialBuilder::buildPartials: " << mNewlyEligible.size() << " newly eligible partials" << endl;
    */
}

// ---------------------------------------------------------------------------
//	finishBuilding
// ---------------------------------------------------------------------------
//	Un-do the frequency warping performed in buildPartials, and return 
//	the Partials that were built. After calling finishBuilding, the
//  builder is returned to its initial state, and ready to build another
//  set of Partials. Partials are returned by appending them to the 
//  supplied PartialList.
//
void
PartialBuilder::finishBuilding( PartialList & product )
{	
    //  append the collected Partials to the product list:
	product.splice( product.end(), mCollectedPartials );
    
    //  reset the builder state:
    mEligiblePartials.clear();
    mNewlyEligible.clear();
}



}	//	end of namespace Loris
