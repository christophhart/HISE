#ifndef INCLUDE_COLLATOR_H
#define INCLUDE_COLLATOR_H
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

#include "Distiller.h"	//	for default fade time and silent time
#include "Partial.h"
#include "PartialList.h"
#include "PartialUtils.h"

#include <algorithm>

//  begin namespace
namespace Loris {

// ---------------------------------------------------------------------------
//  class Collator
//
//! Class Collator represents an algorithm for reducing a collection
//! of Partials into the smallest collection of "equivalent" Partials
//! by joining non-overlapping Partials end to end.
//! 
//! Partials that are not labeled, that is, Partials having label 0,
//! are "collated " into groups of non-overlapping (in time)
//! Partials, and fused into a single Partial per group. 
//! "Collating" is a bit like "distilling" but non-overlapping
//! Partials are grouped without regard to frequency proximity. This
//! algorithm produces the smallest-possible number of collated Partials.
//! Thanks to Ulrike Axen for providing this optimal algorithm.
//! 
//! Collating modifies the Partial container (a PartialList). Only
//! unlabeled (labeled 0) Partials are affected by the collating
//! operation. Collated Partials are moved to the end of the 
//! collection of Partials.
//
class Collator
{
//  -- instance variables --

    double _fadeTime, _gapTime;       
        
//  -- public interface --
public:

//  -- global defaults and constants --

	enum 
	{
	
		//! Default time in milliseconds over which Partials joined by
		//! distillation fade to and from zero amplitude. Divide by 
		//!	1000 to use as a member function parameter. This parameter
		//!	should be the same in Distiller, Sieve, and Collator.
		DefaultFadeTimeMs = Distiller::DefaultFadeTimeMs,
		
		//! Default minimum duration in milliseconds of the silent 
		//! (zero-amplitude) gap between two Partials joined by 
		//!	distillation. Divide by 1000 to use as a member function 
		//!	parameter. This parameter should be the same in Distiller, 
		//!	Sieve, and Collator.
		DefaultSilentTimeMs = Distiller::DefaultSilentTimeMs
    };
    
//  -- construction --

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
    explicit
    Collator( double partialFadeTime = Collator::DefaultFadeTimeMs/1000.0,
              double partialSilentTime = Collator::DefaultSilentTimeMs/1000.0 );
     
    //  Use compiler-generated copy, assign, and destroy.
    
//  -- collating --

    //! Collate unlabeled (zero-labeled) Partials into the smallest-possible 
    //! number of Partials that does not combine any overlapping Partials.
    //! Collated Partials assigned labels higher than any label in the original 
    //! list, and appear at the end of the sequence, after all previously-labeled
    //! Partials.
    //!
    //!
    //! Return an iterator refering to the position of the first collated Partial,
    //! or the end of the collated collection if there are no collated Partials.
    //! Since collating is in-place, the Partials collection may be smaller
    //! (fewer Partials) after collating, and any iterators on the collection
    //! may be invalidated.
    //!
    //!   \param  partials is the collection of Partials to collate in-place
    //!   \return the position of the end of the range of labeled Partials,
    //!           which is either the end of the collection, or the position
    //!           of the first collated Partial, composed of unlabeled Partials
    //!           in the original collection.
    //!
    //! If compiled with NO_TEMPLATE_MEMBERS defined, then partials
    //! must be a PartialList, otherwise it can be any container type
    //! storing Partials that supports at least bidirectional iterators.
    //!
    //!  \sa Collator::collate( Container & partials )
#if ! defined(NO_TEMPLATE_MEMBERS)
    template< typename Container >
    typename Container::iterator collate( Container & partials );
#else
    inline
    PartialList::iterator collate( PartialList & partials );
#endif

    //! Function call operator: same as collate( PartialList & partials ).
#if ! defined(NO_TEMPLATE_MEMBERS)
    template< typename Container >
    typename Container::iterator operator() ( Container & partials );
#else
    PartialList::iterator operator() ( PartialList & partials );
#endif
    
    //! Static member that constructs an instance and applies
    //! it to a sequence of Partials.  Collated Partials are 
    //! labeled beginning with the label one more than the
    //! largest label in the orignal Partials.
    //!
    //! \param  partials is the collection of Partials to collate in-place
    //! \param  partialFadeTime is the time (in seconds) over
    //!         which Partials joined by collating fade to
    //!         and from zero amplitude.
    //! \param  partialSilentTime is the minimum duration (in seconds) 
    //!         of the silent (zero-amplitude) gap between two 
    //!         Partials joined by collating. 
    //! \return the position of the first collated Partial
    //!
    //! If compiled with NO_TEMPLATE_MEMBERS defined, then partials
    //! must be a PartialList, otherwise it can be any container type
    //! storing Partials that supports at least bidirectional iterators.
#if ! defined(NO_TEMPLATE_MEMBERS)
    template< typename Container >
    static typename Container::iterator 
    collate( Container & partials, double partialFadeTime,
                                   double partialSilentTime );
#else
    static inline PartialList::iterator
    collate( PartialList & partials, double partialFadeTime,
                                     double partialSilentTime );
#endif


private:

//  -- helpers --

    //! Collate unlabeled (zero labeled) Partials into the smallest
    //! possible number of Partials that does not combine any temporally
    //! overlapping Partials. Give each collated Partial a label, starting
    //! with startlabel, and incrementing. If startLabel is zero, then
    //! give each collated Partial the label zero. The unlabeled Partials are
    //! collated in-place.
    void collateAux( PartialList & unlabled );
    
};  //  end of class Collator

// ---------------------------------------------------------------------------
//  collate
// ---------------------------------------------------------------------------
//! Collate unlabeled (zero-labeled) Partials into the smallest-possible 
//! number of Partials that does not combine any overlapping Partials.
//! Collated Partials assigned labels higher than any label in the original 
//! list, and appear at the end of the sequence, after all previously-labeled
//! Partials.
//!
//! Return an iterator refering to the position of the first collated Partial,
//! or the end of the collated collection if there are no collated Partials.
//! Since collating is in-place, the Partials collection may be smaller
//! (fewer Partials) after collating, and any iterators on the collection
//! may be invalidated.
//!
//!   \param  partials is the collection of Partials to collate in-place
//!   \return the position of the end of the range of labeled Partials,
//!           which is either the end of the collection, or the position
//!           of the first collated Partial, composed of unlabeled Partials
//!           in the original collection.
//!
//! If compiled with NO_TEMPLATE_MEMBERS defined, then partials
//! must be a PartialList, otherwise it can be any container type
//! storing Partials that supports at least bidirectional iterators.
//!
//
#if ! defined(NO_TEMPLATE_MEMBERS)
template< typename Container >
typename Container::iterator 
Collator::collate( Container & partials )
#else
inline
PartialList::iterator 
Collator::collate( PartialList & partials )
#endif
{
#if ! defined(NO_TEMPLATE_MEMBERS)
    typedef typename Container::iterator Iterator;
#else
    typedef PartialList::iterator Iterator;
#endif

    // Partition the Partials into labeled and unlabeled, 
    // and collate the unlabeled ones and replace the 
    // unlabeled range.
    // (This requires bidirectional iterator support.)
    Iterator beginUnlabeled = 
       std::partition( partials.begin(), partials.end(), 
                              std::not1( PartialUtils::isLabelEqual(0) ) );
        //  this used to be a stable partition, which 
        //  is very much slower and seems unnecessary
        
    // cannot splice if this operation is to be generic
    // with respect to container, have to copy:
    PartialList collated( beginUnlabeled, partials.end() );
    // collated.splice( collated.end(), beginUnlabeled, partials.end() );
    
    //  determine the label for the first collated Partial:
    Partial::label_type labelCollated = 1;
    if ( partials.begin() != beginUnlabeled )
    {
       labelCollated = 
        1 + std::max_element( partials.begin(), beginUnlabeled, 
                              PartialUtils::compareLabelLess() )->label();
    }
    if ( labelCollated < 1 )
    {
        labelCollated = 1;
    }

    //  collate unlabeled (zero-labeled) Partials:
    collateAux( collated );
    
    //  label the collated Partials:
    for ( Iterator it = collated.begin(); it != collated.end(); ++it )
    {
        it->setLabel( labelCollated++ );
    }
    
    //  copy the collated Partials back into the source container
    //  after the range of labeled Partials     
    Iterator endCollated = 
        std::copy( collated.begin(), collated.end(), beginUnlabeled );

    //  remove extra Partials from the end of the source container
    if ( endCollated != partials.end() )
    {
        typename Iterator::difference_type numLabeled = 
            std::distance( partials.begin(), beginUnlabeled );

        partials.erase( endCollated, partials.end() );

        // restore beginUnlabeled:    
        beginUnlabeled = partials.begin();
        std::advance( beginUnlabeled, numLabeled );
    }
    return beginUnlabeled;
}

// ---------------------------------------------------------------------------
//  Function call operator 
// ---------------------------------------------------------------------------
//! Function call operator: same as collate( PartialList & partials ).
//! 
//! \sa Collator::collate( Container & partials )
//
#if ! defined(NO_TEMPLATE_MEMBERS)
template< typename Container >
typename Container::iterator Collator::operator()( Container & partials )
#else
inline
PartialList::iterator Collator::operator()( PartialList & partials )
#endif
{ 
    return collate( partials );
}

// ---------------------------------------------------------------------------
//  collate
// ---------------------------------------------------------------------------
//! Static member that constructs an instance and applies
//! it to a sequence of Partials. Collated Partials are 
//! labeled beginning with the label one more than the
//! largest label in the orignal Partials.
//!
//! \post   All Partials in the collection are uniquely-labeled,
//!         collated Partials are all at the end of the collection
//!         (after all labeled Partials).
//! \param  partials is the collection of Partials to collate in-place
//! \param  partialFadeTime is the time (in seconds) over
//!         which Partials joined by collating fade to
//!         and from zero amplitude.
//! \param  partialSilentTime is the minimum duration (in seconds) 
//!         of the silent (zero-amplitude) gap between two 
//!         Partials joined by collateation. (Default is
//!         0.0001 (one tenth of a millisecond).
//! \return the position of the first collated Partial
//!
//! If compiled with NO_TEMPLATE_MEMBERS defined, then partials
//! must be a PartialList, otherwise it can be any container type
//! storing Partials that supports at least bidirectional iterators.
//
#if ! defined(NO_TEMPLATE_MEMBERS)
template< typename Container >
typename Container::iterator 
Collator::collate( Container & partials, double partialFadeTime,
                                         double partialSilentTime )
#else
inline
PartialList::iterator 
Collator::collate( PartialList & partials, double partialFadeTime,
                                           double partialSilentTime )
#endif
{
    Collator instance( partialFadeTime, partialSilentTime );
    return instance.collate( partials );
}

}   //  end of namespace Loris

#endif /* ndef INCLUDE_COLLATOR_H */
