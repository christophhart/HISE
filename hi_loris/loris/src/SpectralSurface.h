#ifndef INCLUDE_SPECTRALSURFACE_H
#define INCLUDE_SPECTRALSURFACE_H
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
 * SpectralSurface.h
 *
 * Definition of class SpectralSurface, a class representing 
 * a smoothed time-frequency surface that can be used to 
 * perform cross-synthesis, the filtering of one sound by the
 * time-varying spectrum of another.
 *
 * Kelly Fitz, 21 Dec 2005
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */

#include "LorisExceptions.h"
#include "Partial.h"
#include "PartialList.h"
#include "PartialUtils.h"   // for compareLabelLess

#include <algorithm>        // for sort
#include <vector>

//	begin namespace
namespace Loris {

// ---------------------------------------------------------------------------
//	Class SpectralSurface
//
//! SpectralSurface represents a smoothed time-frequency surface that 
//! can be used to perform cross-synthesis, the filtering of one sound 
//! by the time-varying spectrum of another.
//
class SpectralSurface
{
//	-- public interface --
public:
	
//	-- lifecycle --

    //! Contsruct a new SpectralSurface from a sequence of distilled
    //! Partials. 
    //! 
    //! \pre    the specified Partials must be channelized and distilled.
    //! \param  b the beginning of the sequence of Partials
    //! \param  e the end of the sequence of Partials
    //!
    //!	If compiled with NO_TEMPLATE_MEMBERS defined, then b and e
    //!	must be PartialList::iterators, otherwise they can be any type
    //!	of iterators over a sequence of Partials.
#if ! defined(NO_TEMPLATE_MEMBERS)
	template<typename Iter>
    SpectralSurface( Iter b, Iter e );
#else
    inline
    SpectralSurface( PartialList::iterator b, PartialList::iterator e );
#endif    

    //  use compiler-generated copy/assign/destroy

// --- operations ---
	
	//! Scale the amplitude of every Breakpoint in a Partial
    //! according to the amplitude of the spectral surface
    //! at the corresponding time and frequency.
    //!
    //! \param  p the Partial to modify
	void scaleAmplitudes( Partial & p );

	//! Scale the amplitudes of a sequence of Partials
    //! according to the amplitude of the spectral surface
    //! at the corresponding times and frequencies, 
    //! performing cross-synthesis, the filtering of one
    //! sound (the sequence of Partials) by the time-varying
    //! spectrum of another sound (the Partials used to
    //! construct the surface). The surface is stretched
    //! in time and frequency according to the values of
    //! the two stretch factors (default 1, no stretching)
    //! and the amount of the effect is governed by the
    //! `effect' parameter (default 1, full effect).
    //!
    //! \param  b the beginning of the sequence of Partials
    //! \param  e the end of the sequence of Partials
    //!
    //!	If compiled with NO_TEMPLATE_MEMBERS defined, then b and e
    //!	must be PartialList::iterators, otherwise they can be any type
    //!	of iterators over a sequence of Partials.
#if ! defined(NO_TEMPLATE_MEMBERS)
	template<typename Iter>
    void scaleAmplitudes( Iter b, Iter e );
#else
    inline
	void scaleAmplitudes( PartialList::iterator b, PartialList::iterator e );
#endif
    
	//! Set the amplitude of every Breakpoint in a Partial
    //! equal to the amplitude of the spectral surface
    //! at the corresponding time and frequency.
    //!
    //! \param  p the Partial to modify
	void setAmplitudes( Partial & p );
    
	//! Set the amplitudes of a sequence of Partials
    //! equal to the amplitude of the spectral surface
    //! at the corresponding times and frequencies. 
    //! This can be used to perform formant-corrected
    //! pitch shifting of a sound: construct the surface
    //! from the unmodified Partials, perform the pitch
    //! shift on the Partials, then apply the surface 
    //! to the shifted Partials using setAmplitudes.
    //! The surface is stretched
    //! in time and frequency according to the values of
    //! the two stretch factors (default 1, no stretching)
    //! and the amount of the effect is governed by the
    //! `effect' parameter (default 1, full effect).
    //!
    //! \param  b the beginning of the sequence of Partials
    //! \param  e the end of the sequence of Partials
    //!
    //!	If compiled with NO_TEMPLATE_MEMBERS defined, then b and e
    //!	must be PartialList::iterators, otherwise they can be any type
    //!	of iterators over a sequence of Partials.
#if ! defined(NO_TEMPLATE_MEMBERS)
	template<typename Iter>
    void setAmplitudes( Iter b, Iter e );
#else
    inline
	void setAmplitudes( PartialList::iterator b, PartialList::iterator e );
#endif
	
// --- access/mutation ---

    //! Return the amount of strecthing in the frequency dimension
    //! (default 1, no stretching). Values greater than 1 stretch
    //! the surface in the frequency dimension, values less than 1
    //! (but greater than 0) compress the surface in the frequency
    //! dimension.
	double frequencyStretch( void ) const;
    
    //! Return the amount of strecthing in the time dimension
    //! (default 1, no stretching). Values greater than 1 stretch
    //! the surface in the time dimension, values less than 1
    //! (but greater than 0) compress the surface in the time
    //! dimension.
	double timeStretch( void ) const;
    
    //! Return the amount of effect applied by scaleAmplitudes
    //! and setAmplitudes (default 1, full effect). Values
    //! less than 1 (but greater than 0) reduce the amount of
    //! amplitude modified performed by application of the
    //! surface. (This is rarely a good way of controlling the
    //! amount of the effect.)
	double effect( void ) const;
	
    //! Set the amount of strecthing in the frequency dimension
    //! (default 1, no stretching). Values greater than 1 stretch
    //! the surface in the frequency dimension, values less than 1
    //! (but greater than 0) compress the surface in the frequency
    //! dimension.
    //!
    //! \pre    stretch must be positive
    //! \param  stretch the new stretch factor for the frequency dimension
	void setFrequencyStretch( double stretch );

    //! Set the amount of strecthing in the time dimension
    //! (default 1, no stretching). Values greater than 1 stretch
    //! the surface in the time dimension, values less than 1
    //! (but greater than 0) compress the surface in the time
    //! dimension.
    //!
    //! \pre    stretch must be positive
    //! \param  stretch the new stretch factor for the time dimension
	void setTimeStretch( double stretch );
    
    //! Set the amount of effect applied by scaleAmplitudes
    //! and setAmplitudes (default 1, full effect). Values
    //! less than 1 (but greater than 0) reduce the amount of
    //! amplitude modified performed by application of the
    //! surface. (This is rarely a good way of controlling the
    //! amount of the effect.)
    //!
    //! \pre    effect must be between 0 and 1, inclusive
    //! \param  effect the new factor controlling the amount of 
    //!         amplitude modification performed by scaleAmplitudes
    //!         and setAmplitudes
	void setEffect( double effect );
	
private:

//	-- instance variables --

	std::vector< Partial > mPartials;   //! the Partials comprising the surface are
                                        //! stored in a vector for easy random access
	double mStretchFreq;                //! stretch factor for the frequency dimension
    double mStretchTime;                //! stretch factor for the time dimension
    double mEffect;                     //! factor for controlling the amount of
                                        //! of the effect applied in setAmplitudes and
                                        //! scaleAmplitudes: 0 gives unmodified amplitudes
                                        //! 1 gives fully modified amplitudes.
    double mMaxSurfaceAmp;              //! the maximum amplitude of any Breakpoint on 
                                        //! the surface, used for normalizing the surface
                                        //! amplitude for scaleAmplitudes
    
// --- private helpers ---

    //  helper used by constructor for adding Partials one by one
    void addPartialAux( const Partial & p );
    
};

// ---------------------------------------------------------------------------
//    constructor
// ---------------------------------------------------------------------------
//! Contsruct a new SpectralSurface from a sequence of distilled
//! Partials. 
//! 
//! \pre    the specified Partials must be channelized and distilled.
//! \param  b the beginning of the sequence of Partials
//! \param  e the end of the sequence of Partials
//!
//!	If compiled with NO_TEMPLATE_MEMBERS defined, then b and e
//!	must be PartialList::iterators, otherwise they can be any type
//!	of iterators over a sequence of Partials.
//
#if ! defined(NO_TEMPLATE_MEMBERS)
template<typename Iter>
SpectralSurface::SpectralSurface( Iter b, Iter e ) :
#else
inline
SpectralSurface::SpectralSurface( PartialList::iterator b, 
                                  PartialList::iterator e ) :
#endif    
	mStretchFreq( 1.0 ),
	mStretchTime( 1.0 ),
	mEffect( 1.0 ),
    mMaxSurfaceAmp( 0.0 )
{
    //  add only labeled Partials:
    while ( b != e )
    {
        if ( b->label() != 0 )
        {
            addPartialAux( *b );
        }
        ++b;
    }
	
	// complain if the Partials were not distilled
	if ( mPartials.empty() )
	{
		Throw( InvalidArgument, "Partals need to be distilled to build a SpectralSurface" );
	}

	if ( 0 == mMaxSurfaceAmp )
	{
		Throw( InvalidArgument, "The SpectralSurface is zero amplitude everywhere!" );
	}
	
	// sort by label
	std::sort( mPartials.begin(), mPartials.end(), PartialUtils::compareLabelLess() );
}


// ---------------------------------------------------------------------------
//    scaleAmplitudes
// ---------------------------------------------------------------------------
//! Scale the amplitudes of a sequence of Partials
//! according to the amplitude of the spectral surface
//! at the corresponding times and frequencies, 
//! performing cross-synthesis, the filtering of one
//! sound (the sequence of Partials) by the time-varying
//! spectrum of another sound (the Partials used to
//! construct the surface). The surface is stretched
//! in time and frequency according to the values of
//! the two stretch factors (default 1, no stretching)
//! and the amount of the effect is governed by the
//! `effect' parameter (default 1, full effect).
//!
//! \param  b the beginning of the sequence of Partials
//! \param  e the end of the sequence of Partials
//!
//!	If compiled with NO_TEMPLATE_MEMBERS defined, then b and e
//!	must be PartialList::iterators, otherwise they can be any type
//!	of iterators over a sequence of Partials.
//
#if ! defined(NO_TEMPLATE_MEMBERS)
template<typename Iter>
void SpectralSurface::scaleAmplitudes( Iter b, Iter e )
#else
inline
void SpectralSurface::scaleAmplitudes( PartialList::iterator b, 
                                       PartialList::iterator e )
#endif
{	
	while ( b != e )
	{
		// debugger << b->label() << endl;
        scaleAmplitudes( *b );
        ++b;
	}	
}

// ---------------------------------------------------------------------------
//    setAmplitudes
// ---------------------------------------------------------------------------
//! Set the amplitudes of a sequence of Partials
//! equal to the amplitude of the spectral surface
//! at the corresponding times and frequencies. 
//! This can be used to perform formant-corrected
//! pitch shifting of a sound: construct the surface
//! from the unmodified Partials, perform the pitch
//! shift on the Partials, then apply the surface 
//! to the shifted Partials using setAmplitudes.
//! The surface is stretched
//! in time and frequency according to the values of
//! the two stretch factors (default 1, no stretching)
//! and the amount of the effect is governed by the
//! `effect' parameter (default 1, full effect).
//!
//! \param  b the beginning of the sequence of Partials
//! \param  e the end of the sequence of Partials
//!
//!	If compiled with NO_TEMPLATE_MEMBERS defined, then b and e
//!	must be PartialList::iterators, otherwise they can be any type
//!	of iterators over a sequence of Partials.
//
#if ! defined(NO_TEMPLATE_MEMBERS)
template<typename Iter>
void SpectralSurface::setAmplitudes( Iter b, Iter e )
#else
inline
void SpectralSurface::setAmplitudes( PartialList::iterator b, 
                                     PartialList::iterator e )
#endif
{	
	while ( b != e )
	{
		// debugger << b->label() << endl;
        setAmplitudes( *b );
        ++b;
	}	
}

}	// namespace Loris

#endif /* ndef INCLUDE_SPECTRALSURFACE_H */

