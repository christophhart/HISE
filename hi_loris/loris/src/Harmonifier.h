#ifndef INCLUDE_HARMONIFIER_H
#define INCLUDE_HARMONIFIER_H
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
 * Harmonifier.h
 *
 * Definition of class Harmonifier.
 *
 * Kelly Fitz, 26 Oct 2005
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */
#include "Envelope.h" 
#include "LorisExceptions.h" 
#include "Partial.h"
#include "PartialUtils.h"

#include <algorithm>    // for find
#include <memory>       // for auto_ptr

//	begin namespace
namespace Loris {


// ---------------------------------------------------------------------------
//	Class Harmonifier
//
//! A Harmonifier uses a reference frequency envelope to make the
//! frequencies of labeled Partials harmonic. The amount of frequency
//! adjustment can be controlled by a time-varying envelope, and a 
//! threshold can be supplied so that only quiet Partials are affected.
//
class Harmonifier
{
//	-- instance variables --

	Partial _refPartial;				//! the Partial whose frequency supplies the
										//! reference frequency envelope.
										
	double _freqFixThresholdDb;		    //!	amplitude threshold below which Partial
									    //!	frequencies are corrected according to
									    //!	a reference Partial, if specified.

	std::unique_ptr< Envelope > _weight;  //!	weighting function, when 1 harmonic
                                        //! frequencies are used, when 0 breakpoint
                                        //! frequencies are unmodified.
    
//	-- public interface --
public:

//	-- lifecycle --

    //! Construct a new Harmonifier that applies the specified 
    //! reference Partial to fix the frequencies of Breakpoints
    //! whose amplitude is below threshold_dB (0 by default,
    //! to apply only to quiet Partials, specify a threshold,
    //! like -90). 
   	Harmonifier( const Partial & ref, double threshold_dB = 0 );

    //! Construct a new Harmonifier that applies the specified 
    //! reference Partial to fix the frequencies of Breakpoints
    //! whose amplitude is below threshold_dB (0 by default,
    //! to apply only to quiet Partials, specify a threshold,
    //! like -90). The Envelope is a time-varying weighting 
    //! on the harmonifing process. When 1, harmonic frequencies
    //! are used, when 0, breakpoint frequencies are unmodified. 
   	Harmonifier( const Partial & ref, const Envelope & env, 
   	             double threshold_dB = 0 );

    //! Construct a new Harmonifier that applies the specified 
    //! reference Partial to fix the frequencies of Breakpoints
    //! whose amplitude is below threshold_dB (0 by default,
    //! to apply only to quiet Partials, specify a threshold,
    //! like -90). The reference Partial is the first Partial
    //! in the range [b,e) having the specified label. 
    //
    //! \throw  InvalidArgument if no Partial in the range [b,e)
    //!         has the specified label.
    //
#if ! defined(NO_TEMPLATE_MEMBERS)
	template<typename Iter>
   	Harmonifier( Iter b, Iter e, Partial::label_type refLabel, 
   	            double threshold_dB = 0 );
#else
   inline
   	Harmonifier( PartialList::iterator b, PartialList::iterator e, 
   	             Partial::label_type refLabel, double threshold_dB = 0 );
#endif

    //! Construct a new Harmonifier that applies the specified 
    //! reference Partial to fix the frequencies of Breakpoints
    //! whose amplitude is below threshold_dB (0 by default,
    //! to apply only to quiet Partials, specify a threshold,
    //! like -90). The reference Partial is the first Partial
    //! in the range [b,e) having the specified label. 
    //!
    //! The Envelope is a time-varying weighting 
    //! on the harmonifing process. When 1, harmonic frequencies
    //! are used, when 0, breakpoint frequencies are unmodified. 
    //
    //! \throw  InvalidArgument if no Partial in the range [b,e)
    //!         has the specified label.
    //
#if ! defined(NO_TEMPLATE_MEMBERS)
	template<typename Iter>
   	Harmonifier( Iter b, Iter e, Partial::label_type refLabel, 
   	             const Envelope & env, double threshold_dB = 0 );
#else
   inline
   	Harmonifier( PartialList::iterator b, PartialList::iterator e, 
   	             Partial::label_type refLabel, const Envelope & env,
   	             double threshold_dB = 0 );
#endif

    //! Destructor.
    ~Harmonifier( void );

    // use compiler-generated copy and assign.

//	-- operation --

    //! Apply the reference envelope to a Partial.
    void harmonify( Partial & p ) const;
    
    //! Apply the reference envelope to all Partials in a range.    
#if ! defined(NO_TEMPLATE_MEMBERS)
	template<typename Iter>
	void harmonify( Iter b, Iter e  );
#else
    inline
    void harmonify( PartialList::iterator b, PartialList::iterator e  );
#endif

// -- static members --

	//! Static member that constructs an instance and applies
	//! it to a sequence of Partials. 
	//! Construct a Harmonifier using as reference the Partial in
	//! the specified range labeled refLabel, then apply
	//! the instance to all Partials in the range.
	//!
  	//!	\param  b is the beginning of the range of Partials to harmonify
	//!	\param  e is (one-past) the end of the range of Partials to harmonify
	//! \param  refLabel is the label of the Partial in [b,e) to
	//!         use as reference Partial. The reference Partial is the first
    //!         Partial in the range [b,e) having the specified label. 
	//! \param  threshold_dB is the amplitude below which breakpoint
	//!         frequencies are harmonified (0 by default, to apply 
    //!         only to quiet Partials, specify a threshold, like -90). 
	//! 
    //! \throw  InvalidArgument if no Partial in the range [b,e)
    //!         has the specified label.	
    //! \throw  InvalidArgument if refLabel is non-positive.	
	//!	
	//!	If compiled with NO_TEMPLATE_MEMBERS defined, then begin and end
	//!	must be PartialList::iterators, otherwise they can be any type
	//!	of iterators over a sequence of Partials.
#if ! defined(NO_TEMPLATE_MEMBERS)
	template< typename Iter >
	static 
	void harmonify( Iter b, Iter e, 
	                Partial::label_type refLabel,
                    double threshold_dB = 0 );
#else
	static inline 
	void harmonify( PartialList::iterator b, PartialList::iterator e,
	                Partial::label_type refLabel,
                    double threshold_dB = 0 );
#endif	 

	//! Static member that constructs an instance and applies
	//! it to a sequence of Partials. 
	//! Construct a Harmonifier using as reference the Partial in
	//! the specified range labeled refLabel, then apply
	//! the instance to all Partials in the range.
	//!
  	//!	\param  b is the beginning of the range of Partials to harmonify
	//!	\param  e is (one-past) the end of the range of Partials to harmonify
	//! \param  refLabel is the label of the Partial in [b,e) to
	//!         use as reference Partial. The reference Partial is the first
    //!         Partial in the range [b,e) having the specified label. 
	//! \param  env is a weighting envelope to apply to the harmonification
	//!         process: when env is 1, use harmonic frequencies, when env
	//!         is 0, breakpoint frequencies are unmodified.
	//! \param  threshold_dB is the amplitude below which breakpoint
	//!         frequencies are harmonified (0 by default, to apply 
    //!         only to quiet Partials, specify a threshold, like -90). 
	//! 
    //! \throw  InvalidArgument if no Partial in the range [b,e)
    //!         has the specified label.	
    //! \throw  InvalidArgument if refLabel is non-positive.	
	//!	
	//!	If compiled with NO_TEMPLATE_MEMBERS defined, then begin and end
	//!	must be PartialList::iterators, otherwise they can be any type
	//!	of iterators over a sequence of Partials.
#if ! defined(NO_TEMPLATE_MEMBERS)
	template< typename Iter >
	static 
	void harmonify( Iter b, Iter e, 
	                Partial::label_type refLabel,
                    const Envelope & env, double threshold_dB = 0 );
#else
	static inline 
	void harmonify( PartialList::iterator b, PartialList::iterator e,
	                Partial::label_type refLabel,
                    const Envelope & env, double threshold_dB = 0 );
#endif	 

private:

//	-- helpers --

    //! Return the default weighing envelope (always 1).
    //! Used in template constructors.
    static Envelope * createDefaultEnvelope( void );
    
};

// ---------------------------------------------------------------------------
//	constructor
// ---------------------------------------------------------------------------
//! Construct a new Harmonifier that applies the specified 
//! reference Partial to fix the frequencies of Breakpoints
//! whose amplitude is below threshold_dB (0 by default,
//! to apply only to quiet Partials, specify a threshold,
//! like -90). The reference Partial is the first Partial
//! in the range [b,e) having the specified label. 
//! \throw  InvalidArgument if no Partial in the range [b,e)
//!         has the specified label.
//! \throw  InvalidArgument if refLabel is non-positive.
//
#if ! defined(NO_TEMPLATE_MEMBERS)
template<typename Iter>
Harmonifier::Harmonifier( Iter b, Iter e, Partial::label_type refLabel, 
                          double threshold_dB ) :
#else
inline
Harmonifier::Harmonifier( PartialList::iterator b, PartialList::iterator e, 
                          Partial::label_type refLabel, double threshold_dB ) :
#endif
    _freqFixThresholdDb( threshold_dB ),
    _weight( createDefaultEnvelope() )
{
    if ( 1 > refLabel )
    {
        Throw( InvalidArgument, "The reference label must be positive." );
    }

    b = std::find_if( b, e, PartialUtils::isLabelEqual( refLabel ) );
    if ( b == e )
	{
		Throw( InvalidArgument, "no Partial has the specified reference label" );
	}

    if ( 0 == b->numBreakpoints() )
    {
        Throw( InvalidArgument, 
               "Cannot use an empty reference Partial in Harmonizer" );
    }
    _refPartial = *b;
}

// ---------------------------------------------------------------------------
//	constructor
// ---------------------------------------------------------------------------
//! Construct a new Harmonifier that applies the specified 
//! reference Partial to fix the frequencies of Breakpoints
//! whose amplitude is below threshold_dB (0 by default,
//! to apply only to quiet Partials, specify a threshold,
//! like -90). The reference Partial is the first Partial
//! in the range [b,e) having the specified label. 
//!
//! The Envelope is a time-varying weighting 
//! on the harmonifing process. When 1, harmonic frequencies
//! are used, when 0, breakpoint frequencies are unmodified. 
//!
//! \throw  InvalidArgument if no Partial in the range [b,e)
//!         has the specified label.
//! \throw  InvalidArgument if refLabel is non-positive.
//
#if ! defined(NO_TEMPLATE_MEMBERS)
template<typename Iter>
Harmonifier::Harmonifier( Iter b, Iter e, Partial::label_type refLabel, 
                          const Envelope & env, double threshold_dB ) :
#else
inline
Harmonifier::Harmonifier( PartialList::iterator b, PartialList::iterator e, 
                          Partial::label_type refLabel, const Envelope & env,
                          double threshold_dB ) :
#endif
    _freqFixThresholdDb( threshold_dB ),
    _weight( env.clone() )
{
    if ( 1 > refLabel )
    {
        Throw( InvalidArgument, "The reference label must be positive." );
    }

    b = std::find_if( b, e, PartialUtils::isLabelEqual( refLabel ) );
    if ( b == e )
	{
		Throw( InvalidArgument, "no Partial has the specified reference label" );
	}

    if ( 0 == b->numBreakpoints() )
    {
        Throw( InvalidArgument, 
               "Cannot use an empty reference Partial in Harmonizer" );
    }
    _refPartial = *b;
}

// ---------------------------------------------------------------------------
//	harmonify
// ---------------------------------------------------------------------------
//! Apply the reference envelope to all Partials in a range.    
#if ! defined(NO_TEMPLATE_MEMBERS)
template<typename Iter>
void Harmonifier::harmonify( Iter b, Iter e  )
#else
inline
void Harmonifier::harmonify( PartialList::iterator b, PartialList::iterator e  )
#endif
{
    while ( b != e )
    {
        harmonify( *b );
        ++b;
    }
}    

// ---------------------------------------------------------------------------
//	harmonify (STATIC)
// ---------------------------------------------------------------------------
//! Static member that constructs an instance and applies
//! it to a sequence of Partials. 
//! Construct a Harmonifier using as reference the Partial in
//! the specified range labeled refLabel, then apply
//! the instance to all Partials in the range.
//!
//!	\param  b is the beginning of the range of Partials to harmonify
//!	\param  e is (one-past) the end of the range of Partials to harmonify
//! \param  refLabel is the label of the Partial in [b,e) to
//!         use as reference Partial. The reference Partial is the first
//!         Partial in the range [b,e) having the specified label. 
//! \param  threshold_dB is the amplitude below which breakpoint
//!         frequencies are harmonified (0 by default, to apply 
//!         only to quiet Partials, specify a threshold, like -90). 
//! 
//! \throw  InvalidArgument if no Partial in the range [b,e)
//!         has the specified label.	
//! \throw  InvalidArgument if refLabel is non-positive.	
//!	
//!	If compiled with NO_TEMPLATE_MEMBERS defined, then begin and end
//!	must be PartialList::iterators, otherwise they can be any type
//!	of iterators over a sequence of Partials.
#if ! defined(NO_TEMPLATE_MEMBERS)
template< typename Iter >
void Harmonifier::harmonify( Iter b, Iter e, 
                             Partial::label_type refLabel,
                             double threshold_dB )
#else
inline 
void Harmonifier::harmonify( PartialList::iterator b, PartialList::iterator e,
                             Partial::label_type refLabel,
                             double threshold_dB )
#endif	 
{
    Harmonifier instance( b, e, refLabel, threshold_dB );
    instance.harmonify( b, e );
}

// ---------------------------------------------------------------------------
//	harmonify (STATIC)
// ---------------------------------------------------------------------------
//! Static member that constructs an instance and applies
//! it to a sequence of Partials. 
//! Construct a Harmonifier using as reference the Partial in
//! the specified range labeled refLabel, then apply
//! the instance to all Partials in the range.
//!
//!	\param  b is the beginning of the range of Partials to harmonify
//!	\param  e is (one-past) the end of the range of Partials to harmonify
//! \param  refLabel is the label of the Partial in [b,e) to
//!         use as reference Partial. The reference Partial is the first
//!         Partial in the range [b,e) having the specified label. 
//! \param  env is a weighting envelope to apply to the harmonification
//!         process: when env is 1, use harmonic frequencies, when env
//!         is 0, breakpoint frequencies are unmodified.
//! \param  threshold_dB is the amplitude below which breakpoint
//!         frequencies are harmonified (0 by default, to apply 
//!         only to quiet Partials, specify a threshold, like -90). 
//! 
//! \throw  InvalidArgument if no Partial in the range [b,e)
//!         has the specified label.	
//! \throw  InvalidArgument if refLabel is non-positive.	
//!	
//!	If compiled with NO_TEMPLATE_MEMBERS defined, then begin and end
//!	must be PartialList::iterators, otherwise they can be any type
//!	of iterators over a sequence of Partials.
#if ! defined(NO_TEMPLATE_MEMBERS)
template< typename Iter >
void Harmonifier::harmonify( Iter b, Iter e, 
                             Partial::label_type refLabel,
                             const Envelope & env, double threshold_dB )
#else
inline 
void Harmonifier::harmonify( PartialList::iterator b, PartialList::iterator e,
                             Partial::label_type refLabel,
                             const Envelope & env, double threshold_dB )
#endif	 
{
    Harmonifier instance( b, e, refLabel, env, threshold_dB );
    instance.harmonify( b, e );
}

}	// namespace Loris

#endif	/* ndef INCLUDE_HARMONIFIER_H */
