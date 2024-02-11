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
 * Distiller.C
 *
 * Implementation of class Distiller.
 *
 * Kelly Fitz, 20 Oct 1999
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */

#if HAVE_CONFIG_H
	#include "config.h"
#endif

#include "Distiller.h"
#include "Breakpoint.h"
#include "BreakpointUtils.h"
#include "LorisExceptions.h"
#include "Partial.h"
#include "PartialList.h"
#include "PartialUtils.h"
#include "Notifier.h"

#include <algorithm>
#include <functional>
#include <utility>

//	begin namespace
namespace Loris {

// ---------------------------------------------------------------------------
//	Distiller constructor
// ---------------------------------------------------------------------------
//!	Construct a new Distiller using the specified fade time
//!	for gaps between Partials. When two non-overlapping Partials
//!	are distilled into a single Partial, the distilled Partial
//!	fades out at the end of the earlier Partial and back in again
//!	at the onset of the later one. The fade time is the time over
//!	which these fades occur. By default, use a 5 ms fade time.
//!	The gap time is the additional time over which a Partial faded
//!	out must remain at zero amplitude before it can fade back in.
//!	By default, use a gap time of one millisecond, to 
//!	prevent a pair of arbitrarily close null Breakpoints being
//!	inserted.
//!
//!   \param   partialFadeTime is the time (in seconds) over
//!            which Partials joined by distillation fade to
//!            and from zero amplitude. Default is 0.005 (one
//!            millisecond).
//!   \param   partialSilentTime is the minimum duration (in seconds) 
//!            of the silent (zero-amplitude) gap between two 
//!            Partials joined by distillation. (Default is
//!            0.001 (one millisecond).
//
Distiller::Distiller( double partialFadeTime, double partialSilentTime ) :
	_fadeTime( partialFadeTime ),
	_gapTime( partialSilentTime )
{
	if ( _fadeTime <= 0.0 )
	{
		Throw( InvalidArgument, "Distiller fade time must be positive." );
	}
	if ( _gapTime <= 0.0 )
	{
		Throw( InvalidArgument, "Distiller gap time must be positive." );
	}
}

// -- helpers --

// ---------------------------------------------------------------------------
//	fadeInAndOut		(STATIC)
// ---------------------------------------------------------------------------
//  Add zero-amplitude Breakpoints to the ends of a Partial if necessary.
//  Do this to all Partials before distilling to make distillation easier.
//
static void fadeInAndOut( Partial & p, double fadeTime )
{
    if ( p.first().amplitude() != 0 )
    {
        p.insert( 
            p.startTime() - fadeTime,
            BreakpointUtils::makeNullBefore( p.first(), fadeTime ) );
    }
    
    if ( p.last().amplitude() != 0 )
    {
        p.insert( 
            p.endTime() + fadeTime,
            BreakpointUtils::makeNullAfter( p.last(), fadeTime ) );
    }
}

// ---------------------------------------------------------------------------
//	merge	(STATIC)
// ---------------------------------------------------------------------------
//	Merge the Breakpoints in the specified iterator range into the
//	distilled Partial. The beginning of the range may overlap, and 
//	will replace, some non-zero-amplitude portion of the distilled
//	Partial. Assume that there is no such overlap at the end of the 
//	range (could check), because findContribution only leaves overlap
//  at the beginning of the range.
//
static void merge( Partial::const_iterator beg, 
				   Partial::const_iterator end, 
				   Partial & destPartial, double fadeTime, 
				   double gapTime = 0. )
{	
	//	absorb energy in distilled Partial that overlaps the
	//	range to merge:
	Partial toMerge( beg, end );
	toMerge.absorb( destPartial );  
    fadeInAndOut( toMerge, fadeTime );

		
    //  find the first Breakpoint in destPartial that is after the
    //  range of merged Breakpoints, plus the required gap:
	Partial::iterator removeEnd = destPartial.findAfter( toMerge.endTime() + gapTime );
    
    //  if this Breakpoint has non-zero amplitude, need to leave time
    //  for a fade in:
	while ( removeEnd != destPartial.end() &&
            removeEnd.breakpoint().amplitude() != 0 &&
            removeEnd.time() < toMerge.endTime() + gapTime + fadeTime )
    {
        ++removeEnd;
    }   
    
	//	find the first Breakpoint in the destination Partial that needs
    //  to be removed because it is in the overlap region:
	Partial::iterator removeBegin = destPartial.findAfter( toMerge.startTime() - gapTime );
        
    //  if beforeMerge has non-zero amplitude, need to leave time
    //  for a fade out:
    if ( removeBegin != destPartial.begin() )
	{
        Partial::iterator beforeMerge = --Partial::iterator(removeBegin);
        
        while ( removeBegin != destPartial.begin() &&
                beforeMerge.breakpoint().amplitude() != 0 &&
                beforeMerge.time() > toMerge.startTime() - gapTime - fadeTime )
        {
            --removeBegin;
            if ( beforeMerge != destPartial.begin() )
            {
                --beforeMerge;
            }
        }   
        
	}
	
	//	remove the Breakpoints in the merge range from destPartial:
    double rbt = (removeBegin != destPartial.end())?(removeBegin.time()):(destPartial.endTime());
    double ret = (removeEnd != destPartial.end())?(removeEnd.time()):(destPartial.endTime());
    Assert( rbt <= ret );
	destPartial.erase( removeBegin, removeEnd );

    //  how about doing the fades here instead?
    //  fade in if necessary:
	if ( removeEnd != destPartial.end() &&
         removeEnd.breakpoint().amplitude() != 0 )
	{
        Assert( removeEnd.time() - fadeTime > toMerge.endTime() );

        //	update removeEnd so that we don't remove this 
        //	null we are inserting:
        destPartial.insert( 
            removeEnd.time() - fadeTime, 
            BreakpointUtils::makeNullBefore( removeEnd.breakpoint(), fadeTime ) );
	}

    if ( removeEnd != destPartial.begin() )
    {
        Partial::iterator beforeMerge = --Partial::iterator(removeEnd);
        if ( beforeMerge.breakpoint().amplitude() > 0 )
        {
            Assert( beforeMerge.time() + fadeTime < toMerge.startTime() );
            
            destPartial.insert( 
                beforeMerge.time() + fadeTime, 
                BreakpointUtils::makeNullAfter( beforeMerge.breakpoint(), fadeTime ) );
        }
    }		
    
    //	insert the Breakpoints in the range:
	for ( Partial::const_iterator insert = toMerge.begin(); insert != toMerge.end(); ++insert )
	{
		destPartial.insert( insert.time(), insert.breakpoint() );
	}
}

// ---------------------------------------------------------------------------
//	findContribution		(STATIC)
// ---------------------------------------------------------------------------
//	Find and return an iterator range delimiting the portion of pshort that
// 	should be spliced into the distilled Partial plong. If any Breakpoint 
//	falls in a zero-amplitude region of plong, then pshort should contribute,
//	AND its onset should be retained (!!! This is the weird part!!!). 
//  Therefore, if cbeg is not equal to cend, then cbeg is pshort.begin().
//
std::pair< Partial::iterator, Partial::iterator >
findContribution( Partial & pshort, const Partial & plong, 
				  double fadeTime, double gapTime )
{
	//	a Breakpoint can only fit in the gap if there's
	//	enough time to fade out pshort, introduce a
	//	space of length gapTime, and fade in the rest
	//	of plong (don't need to worry about the fade
	//	in, because we are checking that plong is zero
	//	at cbeg.time() + clearance anyway, so the fade 
	//	in must occur after that, and already be part of 
	//	plong):
    //
    //  WRONG if cbeg is before the start of plong.
    //  Changed so that all Partials are faded in and
    //  out before distilling, so now the clearance 
    //  need only be the gap time:
	double clearance = gapTime; // fadeTime + gapTime;
	
	Partial::iterator cbeg = pshort.begin();
	while ( cbeg != pshort.end() && 
			( plong.amplitudeAt( cbeg.time() ) > 0 ||
			  plong.amplitudeAt( cbeg.time() + clearance ) > 0 ) )
	{
		++cbeg;
	}
	
	Partial::iterator cend = cbeg;
	
	// if a gap is found, find the end of the
	// range of Breakpoints that fit in that
	// gap:
	while ( cend != pshort.end() &&
			plong.amplitudeAt( cend.time() ) == 0 &&
			plong.amplitudeAt( cend.time() + clearance ) == 0 )
	{
		++cend;
	}

	// if a gap is found, and it is big enough for at
	// least one Breakpoint, then include the 
	// onset of the Partial:
	if ( cbeg != pshort.end()  )
	{
		cbeg = pshort.begin();
	}
	
	return std::make_pair( cbeg, cend );
}

// ---------------------------------------------------------------------------
//	distillSorter (static helper)
// ---------------------------------------------------------------------------
//  Helper for sorting Partials for distilling.
//
static bool distillSorter( const Partial & lhs, const Partial & rhs )
{
    double ldur = lhs.duration(), rdur = rhs.duration();
    if ( ldur != rdur )
    {        
        return ldur > rdur;      
    }
    else
    {
        //  What to do for same-duration Partials?
        //  Need to do something consistent, should look
        //  for energy?
        return lhs.startTime() < rhs.startTime();
        /*
        double lpeak = PartialUtils::peakAmp( lhs );
        double rpeak = PartialUtils::peakAmp( rhs );
        return lhs > rhs;
        */
    }
}

// ---------------------------------------------------------------------------
//	distillOne
// ---------------------------------------------------------------------------
//	Distill a list of Partials having a common label
// 	into a single Partial with that label, and append it
//  to the distilled collection. If an empty list of Partials
//  is passed, then an empty Partial having the specified
//  label is appended.
//
void Distiller::distillOne( PartialList & partials, Partial::label_type label,
                            PartialList & distilled )
{
	debugger << "Distiller found " << partials.size() 
			 << " Partials labeled " << label << endl;

	Partial newp;
    newp.setLabel( label );

    if ( partials.size() == 1 )
    {
        //  trivial if there is only one partial to distill
        newp = partials.front();
    }
	else if ( partials.size() > 0 )  //  it will be an empty Partial otherwise
    {	
    	//	sort Partials by duration, longer
    	//	Partials will be prefered:
    	partials.sort( distillSorter );
    	
    	// keep the longest Partial:
    	PartialList::iterator it = partials.begin();
    	newp = *it;
    	fadeInAndOut( newp, _fadeTime );
        	
    	//	Iterate over remaining Partials:
    	for ( ++it; it != partials.end(); ++it )
    	{            
            fadeInAndOut( *it, _fadeTime );
            
    		std::pair< Partial::iterator, Partial::iterator > range = 
    			findContribution( *it, newp, _fadeTime, _gapTime );
    		Partial::iterator cb = range.first, ce = range.second;

            //  There can only be one contributing regions because
            //  (and only because) the partials are sorted by length
            //  first!
    		
    		//	merge Breakpoints into the new Partial, if
    		//	there are any that contribute, otherwise
    		//	just absorb the current Partial as noise:
    		if ( cb != ce )
    		{
    			//	absorb the non-contributing part:
    			if ( ce != it->end() )
    			{
    				Partial absorbMe( --Partial::iterator(ce), it->end() );
    				    //  shouldn't this just be ce?
    				newp.absorb( absorbMe );
    			
                    //	There cannot be a non-contributing part
                    //	at the beginning of the Partial too, because
                    //  we always retain the beginning of the Partial.
                    //  If findContribution were changed, then 
                    //  we might also want to absorb the beginning.
                    //
    				// Partial absorbMeToo( it->begin(), cb );
    				// newp.absorb( absorbMeToo );
    			}

    			// merge the contributing part:
    			merge( cb, ce, newp, _fadeTime, _gapTime );
    		}
    		else
    		{
    			//	no contribution, absorb the whole thing:
    			newp.absorb( *it );
    		}		
    	}
    }	

    //  take the null Breakpoints off the ends
    //  should check whether this is appropriate?
    //
    // 	This is a bit of a kludge here, sometimes we must
    //	be inserting more than one null Breakpoint at the
    //	front end (at least), sometimes shows up as a Partial
    //	that begins before 0. The idea is to have no nulls at
    //	the ends, so just remove all nulls at the ends.
	while ( 0 < newp.numBreakpoints() &&
			0 == newp.begin().breakpoint().amplitude() )
	{
		newp.erase( newp.begin() );
	}

    Partial::iterator lastBpPos = newp.end();
    while ( 0 < newp.numBreakpoints() &&
			0 == (--lastBpPos).breakpoint().amplitude() )
    {
        lastBpPos = newp.erase( lastBpPos );
    }
    
    //  insert the new Partial in the distilled collection 
    //  in label order:
    distilled.insert( std::lower_bound( distilled.begin(), distilled.end(), 
                                        newp, 
                                        PartialUtils::compareLabelLess() ),
                      newp );
}

// ---------------------------------------------------------------------------
//	distill_list
// ---------------------------------------------------------------------------
//! Distill labeled Partials in a PartialList leaving only a single 
//!	Partial per non-zero label. 
//!
//!	Unlabeled (zero-labeled) Partials are left unmodified at 
//! the end of the distilled Partials.
//!
//!	Return an iterator refering to the position of the first unlabeled Partial,
//!	or the end of the distilled collection if there are no unlabeled Partials.
//! Since distillation is in-place, the Partials collection may be smaller
//! (fewer Partials) after distillation, and any iterators on the collection
//! may be invalidated.
//!
//! \post   All labeled Partials in the collection are uniquely-labeled,
//!         and all unlabeled Partials have been moved to the end of the
//!         sequence.
//! \param  partials is the collection of Partials to distill in-place
//! \return the position of the end of the range of distilled Partials,
//!         which is either the end of the collection, or the position
//!         or the first unlabeled Partial.
//
PartialList::iterator Distiller::distill_list( PartialList & partials )
{  
    //  sort the Partials by label, this is why it
    //  is so much better to distill a list!    
    partials.sort( PartialUtils::compareLabelLess() );

    //  temporary container of distilled Partials:
    PartialList distilled; 
	
	PartialList::iterator lower = partials.begin();
	while ( lower != partials.end() )
	{
		Partial::label_type label = lower->label();

        //  identify a sequence of Partials having the same label:
	    PartialList::iterator upper = 
	        std::find_if( lower, partials.end(),
	                      std::not_fn( PartialUtils::isLabelEqual( label ) ) );
                            
        //  upper is the first Partial after lower whose label is not
        //  equal to that of lower.
		//	[lower, upper) is a range of all the
		//	partials labeled `label'.

        if ( 0 != label )
        {
            //	make a container of the Partials having the same 
            //	label, and distill them:
            PartialList samelabel;
            samelabel.splice( samelabel.begin(), partials, lower, upper );
            distillOne( samelabel, label, distilled );
        }
        lower = upper;
    }
        
#if defined(Debug_Loris) && Debug_Loris
    // only unlabeled Partials should remain in partials:
    Assert( partials.end() ==
            std::find_if( partials.begin(), partials.end(), 
                          std::not1( PartialUtils::isLabelEqual( 0 ) ) ) );
#endif    
    
    //  remember where the unlabeled Partials start:
    PartialList::iterator beginUnlabeled = partials.begin(); 
    
    //  splice in the distilled Partials at the beginning:
    partials.splice( partials.begin(), distilled );

    return beginUnlabeled;
}

}	//	end of namespace Loris
