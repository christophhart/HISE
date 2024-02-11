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
 * Sieve.C
 *
 * Implementation of class Sieve.
 *
 * Lippold Haken, 20 Jan 2001
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */

#if HAVE_CONFIG_H
	#include "config.h"
#endif

#include "Sieve.h"
#include "Breakpoint.h"
#include "LorisExceptions.h"
#include "Notifier.h"
#include "Partial.h"
#include "PartialList.h"
#include "PartialUtils.h"

#include <algorithm>

//	begin namespace
namespace Loris {


// ---------------------------------------------------------------------------
//	Sieve constructor
// ---------------------------------------------------------------------------
//! Construct a new Sieve using the specified partial fade
//! time. If unspecified, the default fade time (same as the   
//! default fade time for the Distiller) is used.
//!
//!   \param   partialFadeTime is the extra time (in seconds)  
//!            added to each end of a Partial to accomodate 
//!            the fade to and from zero amplitude. Fade time
//!            must be non-negative. A default value is used
//!			   if unspecified.
//!   \throw  InvalidArgument if partialFadeTime is negative.
//
Sieve::Sieve( double partialFadeTime ) :
	_fadeTime( partialFadeTime )
{
	if ( _fadeTime < 0.0 )
	{
	   Throw( InvalidArgument, "the Partial fade time must be non-negative" );
	}
}

//	Definition of a comparitor for sorting a collection of pointers
//	to Partials by label (increasing) and duration (decreasing), so
//	that Partial ptrs are arranged by label, with the lowest labels
//	first, and then with the longest Partials having each label
//	before the shorter ones.
struct SortPartialPtrs :
	public std::binary_function< const Partial *, const Partial *, bool >
{
	bool operator()( const Partial * lhs, const Partial * rhs ) const 
		{ 
			return 	( lhs->label() != rhs->label() ) ?
					( lhs->label() < rhs->label() ) :
					( lhs->duration() > rhs->duration() );
		}
};

//	Definition of predicate for finding the end of a Patial *
//	range having a common label.
struct PartialPtrLabelNE :
	public std::unary_function< const Partial *, bool >
{
	int label;
	PartialPtrLabelNE( int l ) : label(l) {}

	bool operator()( const Partial * p ) const
		{ return p->label() != label; }
};


// ---------------------------------------------------------------------------
//	find_overlapping (template function)
// ---------------------------------------------------------------------------
//	Iterate over a range of Partials (presumably) with same labeling.
//	The range is specified by iterators over a collection of pointers
//	to Partials (not Partials themselves). Return the first position
//	in the specified range corresponding to a Partial that overlaps
//	(in time) the specified Partial, p, or the end of the range if
//	no Partials in the range overlap.
//
//	Overlap is defined by the minimum time gap between Partials
//	(minGapTime), so Partials that have less then minGapTime
//	between them are considered overlapping.
//
template <typename Iter>	//	Iter is the position of a Partial *
Iter
find_overlapping( Partial & p, double minGapTime, Iter start, Iter end)
{
	for ( Iter it = start; it != end; ++it ) 
	{
		//	skip if other partial is already sifted out.
		if ( (*it)->label() == 0 )
			continue;
		
		//	skip the source Partial:
		//	(identity test: compare addresses)
		//	(this is a sanity check, should not happen since
		//	src should be at position end)
		Assert( (*it) != &p );

		//  test for overlap:
		if ( p.startTime() < (*it)->endTime() + minGapTime &&
			 p.endTime() + minGapTime > (*it)->startTime() )
		{
			//  Does the overlapping Partial have longer duration?
			//	(this should never be true, since the Partials
			//	are sorted by duration)
			Assert( p.duration() <= (*it)->duration() );
			
#if Debug_Loris
			debugger << "Partial starting " << p.startTime() << ", " 
					 << p.begin().breakpoint().frequency() << " ending " 
					 << p.endTime()  << ", " << (--p.end()).breakpoint().frequency() 
					 << " zapped by overlapping Partial starting " 
					 << (*it)->startTime() << ", " << (*it)->begin().breakpoint().frequency()
					 << " ending " << (*it)->endTime() << ", " 
					 << (--(*it)->end()).breakpoint().frequency()  << endl;
#endif
			return it;
		}
	}
	
	//	no overlapping Partial found:
	return end;
}

// ---------------------------------------------------------------------------
//	sift_ptrs (private helper)
// ---------------------------------------------------------------------------
//!   Sift labeled Partials. If any two Partials having the same (non-zero)
//!   label overlap in time (where overlap includes the fade time at both 
//!   ends of each Partial), then set the label of the Partial having the
//!   shorter duration to zero. Sifting is performed on a collection of 
//!   pointers to Partials so that the it can be performed without changing 
//!   the order of the Partials in the sequence.
//!
//!   \param   ptrs is a collection of pointers to the Partials in the
//!            sequence to be sifted.
void 
Sieve::sift_ptrs( PartialPtrs & ptrs  )
{
	//	the minimum gap between Partials is twice the 
	//	specified fadeTime:
	double minGapTime = _fadeTime * 2.;
	
	//	sort the collection of pointers to Partials:
	//	(sort is by increasing label, then
	//	decreasing duration)
	std::sort( ptrs.begin(), ptrs.end(), SortPartialPtrs() );
	
	PartialPtrs::iterator sift_begin = ptrs.begin();
	PartialPtrs::iterator sift_end = ptrs.end();

	int zapped = 0;
	
	// 	iterate over labels and sift each one:
	PartialPtrs::iterator lowerbound = sift_begin;
	while ( lowerbound != sift_end )
	{
		int label = (*lowerbound)->label();
		
		//	first the first element in l after sieve_begin
		//	having a label not equal to 'label':
		PartialPtrs::iterator upperbound = 
			std::find_if( lowerbound, sift_end, PartialPtrLabelNE(label) );

#ifdef Debug_Loris
		//	don't want to compute this iterator distance unless debugging:
		debugger << "sifting Partials labeled " << label << endl;
		debugger << "Sieve found " << std::distance( lowerbound, upperbound ) << 
					" Partials labeled " << label << endl;
#endif
		//  sift all partials with this label, unless the
		//	label is 0:
		if ( label != 0 )
		{
			PartialPtrs::iterator it;
			for ( it = lowerbound; it != upperbound; ++it ) 
			{
				//	find_overlapping only needs to consider Partials on the
				//	half-open range [lowerbound, it), because all 
				//	Partials after it are shorter, thanks to the
				//	sorting of the sift_set:
				if( it != find_overlapping( **it, minGapTime, lowerbound, it ) )
				{
					(*it)->setLabel(0);
					++zapped;
				}
			} 
		}
		
		//	advance Partial set iterator:
		lowerbound = upperbound;
	}

#ifdef Debug_Loris
	debugger << "Sifted out (relabeled) " << zapped << " of " << ptrs.size() << "." << endl;
#endif
}

}	//	end of namespace Loris

