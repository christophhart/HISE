#ifndef INCLUDE_SIEVE_H
#define INCLUDE_SIEVE_H
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
 * Sieve.h
 *
 * Definition of class Sieve.
 *
 * Lippold Haken, 20 Jan 2001
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */
 
#include "Distiller.h"	//	for default fade time and silent time
 
#if defined(NO_TEMPLATE_MEMBERS)
#include "PartialList.h"
#endif

#include "PartialPtrs.h"

//  begin namespace
namespace Loris {

// ---------------------------------------------------------------------------
//  class Sieve
//
//! A Sieve eliminating temporal overlap among Partials.
//!
//! Class Sieve represents an algorithm for identifying channelized (see also 
//! Channelizer) Partials that overlap in time, and selecting the longer
//! one to represent the channel. The identification of overlap includes 
//! the time needed for Partials to fade to and from zero amplitude in 
//!  synthesis (see also  Synthesizer) or distillation. (see also Distiller)
//! 
//! In some cases, the energy redistribution effected by the distiller
//! (see also Distiller) is undesirable. In such cases, the partials can be
//! sifted before distillation. The sifting process in Loris identifies
//! all the partials that would be rejected (and converted to noise
//! energy) by the distiller and assigns them a label of 0. These sifted
//! partials can then be identified and treated sepearately or removed
//! altogether, or they can be passed through the distiller unlabeled, and
//! crossfaded in the morphing process (see also Morpher).
//!
//!   \sa Channelizer, Distiller, Morpher, Synthesizer
//
class Sieve
{
//  -- instance variables --

    double _fadeTime; //! extra time (in seconds) added to each end of 
                      //! a Partial when determining overlap, to accomodate 
                      //! the fade to and from zero amplitude.
    
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
    explicit Sieve( double partialFadeTime = Sieve::DefaultFadeTimeMs/1000.0 );
     
    //  Use compiler-generated copy, assign, and destroy.
    
//  -- sifting --

    //! Sift labeled Partials on the specified half-open (STL-style)
    //! range. If any two Partials having same label overlap in time, keep
    //! only the longer of the two Partials. Set the label of the shorter
    //! duration partial to zero. No Partials are removed from the
    //! sequence and the sequence order is unaltered. 
    //!
    //! \param sift_begin is the beginning of the range of Partials to sift
    //! \param sift_end is (one-past) the end of the range of Partials to sift
    //! 
    //! If compiled with NO_TEMPLATE_MEMBERS defined, then sift_begin and 
    //! sift_end must be PartialList::iterators, otherwise they can be any type
    //! of iterators over a sequence of Partials.
#if ! defined(NO_TEMPLATE_MEMBERS)
    template<typename Iter>
    void sift( Iter sift_begin, Iter sift_end  );
#else
   inline
    void sift( PartialList::iterator sift_begin, PartialList::iterator sift_end  );
#endif

    //! Sift labeled Partials in the specified container
    //! If any two Partials having same label overlap in time, keep
    //! only the longer of the two Partials. Set the label of the shorter
    //! duration partial to zero. No Partials are removed from the
    //! container and the container order is unaltered. 
    //!
    //! \param  partials is the collection of Partials to sift in-place
    //! 
    //! If compiled with NO_TEMPLATE_MEMBERS defined, then partials
    //! must be a PartialList, otherwise it can be any container type
    //! storing Partials that supports at least bidirectional iterators.
#if ! defined(NO_TEMPLATE_MEMBERS)
    template< typename Container >
    void sift( Container & partials  )
#else
   inline
    void sift( PartialList & partials )
#endif
    {
        sift( partials.begin(), partials.end() );
    }
         
// -- static members --

    //! Static member that constructs an instance and applies
    //! it to a sequence of Partials. 
    //! Construct a Sieve using the specified Partial
    //! fade time (in seconds), and use it to sift a
    //! sequence of Partials. 
    //!
    //! \param  sift_begin is the beginning of the range of Partials to sift
    //! \param  sift_end is (one-past) the end of the range of Partials to sift
    //! \param  partialFadeTime is the extra time (in seconds)  
    //!         added to each end of a Partial to accomodate 
    //!         the fade to and from zero amplitude. The Partial fade time
    //!         must be non-negative.
    //! \throw  InvalidArgument if partialFadeTime is negative.
    //! 
    //! If compiled with NO_TEMPLATE_MEMBERS defined, then begin and end
    //! must be PartialList::iterators, otherwise they can be any type
    //! of iterators over a sequence of Partials.
#if ! defined(NO_TEMPLATE_MEMBERS)
    template< typename Iter >
    static 
    void sift( Iter sift_begin, Iter sift_end, 
              double partialFadeTime );
#else
    static inline 
    void sift( PartialList::iterator sift_begin, PartialList::iterator sift_end,
              double partialFadeTime );
#endif   

//  -- helper --
private:

   //!   Sift labeled Partials. If any two Partials having the same (non-zero)
   //!   label overlap in time (where overlap includes the fade time at both 
   //!   ends of each Partial), then set the label of the Partial having the
   //!   shorter duration to zero. Sifting is performed on a collection of 
   //!   pointers to Partials so that the it can be performed without changing 
   //!   the order of the Partials in the sequence.
   //!
   //!   \param   ptrs is a collection of pointers to the Partials in the
   //!            sequence to be sifted.
    void sift_ptrs( PartialPtrs & ptrs );

};  //  end of class Sieve

// ---------------------------------------------------------------------------
//  sift
// ---------------------------------------------------------------------------
//! Sift labeled Partials on the specified half-open (STL-style)
//! range. If any two Partials having same label overlap in time, keep
//! only the longer of the two Partials. Set the label of the shorter
//! duration partial to zero. No Partials are removed from the
//! sequence and the sequence order is unaltered. 
//!
//! \param sift_begin is the beginning of the range of Partials to sift
//! \param sift_end is (one-past) the end of the range of Partials to sift
//! 
//! If compiled with NO_TEMPLATE_MEMBERS defined, then sift_begin and 
//! sift_end must be PartialList::iterators, otherwise they can be any type
//! of iterators over a sequence of Partials.
#if ! defined(NO_TEMPLATE_MEMBERS)
template< typename Iter >
void Sieve::sift( Iter sift_begin, Iter sift_end  )
#else
inline
void Sieve::sift( PartialList::iterator sift_begin, PartialList::iterator sift_end  )
#endif
{
    PartialPtrs ptrs;
    fillPartialPtrs( sift_begin, sift_end, ptrs );
    sift_ptrs( ptrs );
}

// ---------------------------------------------------------------------------
//  sift (static)
// ---------------------------------------------------------------------------
//! Static member that constructs an instance and applies
//! it to a sequence of Partials. 
//! Construct a Sieve using the specified Partial
//! fade time (in seconds), and use it to sift a
//! sequence of Partials. 
//!
//! \param  sift_begin is the beginning of the range of Partials to sift
//! \param  sift_end is (one-past) the end of the range of Partials to sift
//! \param  partialFadeTime is the extra time (in seconds)  
//!         added to each end of a Partial to accomodate 
//!         the fade to and from zero amplitude. The Partial fade time
//!         must be non-negative.
//! \throw  InvalidArgument if partialFadeTime is negative.
//! 
//! If compiled with NO_TEMPLATE_MEMBERS defined, then begin and end
//! must be PartialList::iterators, otherwise they can be any type
//! of iterators over a sequence of Partials.
#if ! defined(NO_TEMPLATE_MEMBERS)
template< typename Iter >
void Sieve::sift( Iter sift_begin, Iter sift_end, 
                  double partialFadeTime )
#else
inline 
void Sieve::sift( PartialList::iterator sift_begin, PartialList::iterator sift_end,
                  double partialFadeTime )
#endif   
{
   Sieve instance( partialFadeTime );
   instance.sift( sift_begin, sift_end );
}

}   //  end of namespace Loris

#endif /* ndef INCLUDE_SIEVE_H */
