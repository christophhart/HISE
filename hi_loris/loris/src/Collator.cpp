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
 * Collator.h
 *
 * Definition of class Collator.
 *
 * Kelly Fitz, 29 April 2005
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */

#if HAVE_CONFIG_H
	#include "config.h"
#endif

#include "Collator.h"
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
//	Collator constructor
// ---------------------------------------------------------------------------
//! Construct a new Collator using the specified fade and gap times
//! between Partials. When two Partials are joined, the collated Partial
//! fades out at the end of the earlier Partial and back in again
//! at the onset of the later one. The fade time is the time over
//! which these fades occur. By default, use a 5 ms fade time.
//!	The gap time is the additional time over which a Partial faded
//!	out must remain at zero amplitude before it can fade back in.
//!	By default, use a gap time of one millisecond, to 
//!	prevent a pair of arbitrarily close null Breakpoints being
//!	inserted. (Defaults are copied from the Distiller.)
//!
//!   \param   partialFadeTime is the time (in seconds) over
//!            which Partials joined by distillation fade to
//!            and from zero amplitude. Default is 0.005 (one
//!            millisecond).
//!   \param   partialSilentTime is the minimum duration (in seconds) 
//!            of the silent (zero-amplitude) gap between two 
//!            Partials joined by distillation. (Default is
//!            0.001 (one millisecond).
Collator::Collator( double partialFadeTime, double partialSilentTime ) :
	_fadeTime( partialFadeTime ),
	_gapTime( partialSilentTime )
{
	if ( _fadeTime <= 0.0 )
	{
		Throw( InvalidArgument, "Collator fade time must be positive." );
	}
	if ( _gapTime <= 0.0 )
	{
		Throw( InvalidArgument, "Collator gap time must be positive." );
	}
}

// -- helpers --

// ---------------------------------------------------------------------------
//	helper predicates
// ---------------------------------------------------------------------------
static bool ends_earlier( const Partial & lhs, const Partial & rhs )
{
	return lhs.endTime() < rhs.endTime();
}

struct ends_before : public std::unary_function< const Partial, bool >
{
	double t;
	ends_before( double time ) : t( time ) {}
	
	bool operator() ( const Partial & p ) const 
		{ return p.endTime() < t; }
};

// ---------------------------------------------------------------------------
//	collateAux
// ---------------------------------------------------------------------------
//! Collate unlabeled (zero labeled) Partials into the smallest
//! possible number of Partials that does not combine any temporally
//! overlapping Partials. The unlabeled Partials are
//! collated in-place.
//
void Collator::collateAux( PartialList & unlabeled  )
{
	debugger << "Collator found " << unlabeled.size() 
			 << " unlabeled Partials, collating..." << endl;
	
	// 	sort Partials by end time:
	// 	thanks to Ulrike Axen for this optimal algorithm!
	unlabeled.sort( ends_earlier );
	
	//	invariant:
	//	Partials in the range [partials.begin(), endcollated)
	//	are the collated Partials.
	PartialList::iterator endcollated = unlabeled.begin();
	while ( endcollated != unlabeled.end() )
	{
		//	find a collated Partial that ends
		//	before this one begins.
		//	There must be a gap of at least
		//	twice the _fadeTime, because this algorithm
		//	does not remove any null Breakpoints, and 
		//	because Partials joined in this way might
		//	be far apart in frequency.
		const double clearance = (2.*_fadeTime) + _gapTime;
		PartialList::iterator it = 
			std::find_if( unlabeled.begin(), endcollated, 
                          ends_before( endcollated->startTime() - clearance) );
						  
		// 	if no such Partial exists, then this Partial
		//	becomes one of the collated ones, otherwise, 
		//	insert two null Breakpoints, and then all
		//	the Breakpoints in this Partial:
		if ( it != endcollated )
		{
			Partial & addme = *endcollated;
			Partial & collated = *it;
			Assert( &addme != &collated );
			
			//	insert a null at the (current) end
			//	of collated:
			double nulltime1 = collated.endTime() + _fadeTime;
			Breakpoint null1( collated.frequencyAt(nulltime1), 0., 
							  collated.bandwidthAt(nulltime1), collated.phaseAt(nulltime1) );			
			collated.insert( nulltime1, null1 );

			//	insert a null at the beginning of
			//	of the current Partial:
			double nulltime2 = addme.startTime() - _fadeTime;
			Assert( nulltime2 >= nulltime1 );
			Breakpoint null2( addme.frequencyAt(nulltime2), 0., 
							  addme.bandwidthAt(nulltime2), addme.phaseAt(nulltime2) );			
			collated.insert( nulltime2, null2 );
	
			//	insert all the Breakpoints in addme 
			//	into collated:
			Partial::iterator addme_it;
			for ( addme_it = addme.begin(); addme_it != addme.end(); ++addme_it )
			{
				collated.insert( addme_it.time(), addme_it.breakpoint() );
			}
			
			//	remove this Partial from the list:
			endcollated = unlabeled.erase( endcollated );
		}
		else
		{
		    ++endcollated;
		}
	}
	
	debugger << "...now have " << unlabeled.size() << endl;
}

}	//	end of namespace Loris
