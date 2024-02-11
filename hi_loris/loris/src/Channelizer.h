#ifndef INCLUDE_CHANNELIZER_H
#define INCLUDE_CHANNELIZER_H
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
 * Channelizer.h
 *
 * Definition of class Loris::Channelizer.
 *
 * Kelly Fitz, 21 July 2000
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */

#include "PartialList.h"

#include <memory>

//  begin namespace
namespace Loris {

class Envelope;
class Partial;

// ---------------------------------------------------------------------------
//  Channelizer
//  
//! Class Channelizer represents an algorithm for automatic labeling of
//! a sequence of Partials. Partials must be labeled in
//! preparation for morphing (see Morpher) to establish correspondences
//! between Partials in the morph source and target sounds. 
//! 
//! Channelized partials are labeled according to their adherence to a
//! harmonic frequency structure with a time-varying fundamental
//! frequency. The frequency spectrum is partitioned into
//! non-overlapping channels having time-varying center frequencies that
//! are harmonic (integer) multiples of a specified reference frequency
//! envelope, and each channel is identified by a unique label equal to
//! its harmonic number. Each Partial is assigned the label
//! corresponding to the channel containing the greatest portion of its
//! (the Partial's) energy. 
//! 
//! A reference frequency Envelope for channelization and the channel
//! number to which it corresponds (1 for an Envelope that tracks the
//! Partial at the fundamental frequency) must be specified. The
//! reference Envelope can be constructed explcitly, point by point
//! (using, for example, the BreakpointEnvelope class), or constructed
//! automatically using the FrequencyReference class. 
//!
//! The Channelizer can be configured with a stretch factor, to accomodate
//! detuned harmonics, as in the case of piano tones. The static member
//! computeStretchFactor can compute the apppropriate stretch factor, given
//! a pair of partials. This computation is based on formulae given in 
//! "Understanding the complex nature of the piano tone" by Martin Keane
//! at the Acoustics Research Centre at the University of Aukland (Feb 2004).
//! The stretching factor must be non-negative (and is zero for perfectly
//! tunes harmonics). Even in the case of stretched harmonics, the
//! reference frequency envelope is assumed to track the frequency of
//! one of the partials, and the center frequency of the corresponding
//! channel, even though it may represent a stretched harmonic.
//! 
//! Channelizer is a leaf class, do not subclass.
//
class Channelizer
{
//  -- implementaion --
    std::auto_ptr< Envelope > _refChannelFreq;  //! the reference frequency envelope
    
    int _refChannelLabel;                       //! the channel number corresponding to the
                                                //! reference frequency (1 for the fundamental)
                                                
    double _stretchFactor;                      //! stretching factor to account for 
                                                //! detuned harmonics, as in the case of the piano; 
                                                //! can be computed using the static member
                                                //! computeStretchFactor. Should be 0 for most
                                                //! (strongly harmonic) sounds.

    double _ampWeighting;                       //! exponent for amplitude weighting in channel
                                                //! computation, 0 for no weighting, 1 for linear
                                                //! amplitude weighting, 2 for power weighting, etc.
                                                //! default is 0, amplitude weighting is a bad idea
                                                //! for many sounds
    
//  -- public interface --
public:
//  -- construction --

    //! Construct a new Channelizer using the specified reference
    //! Envelope to represent the a numbered channel. If the sound
    //! being channelized is known to have detuned harmonics, a 
    //! stretching factor can be specified (defaults to 0 for no 
    //! stretching). The stretching factor can be computed using
    //! the static member computeStretchFactor.
    //! 
    //! \param  refChanFreq is an Envelope representing the center frequency
    //!         of a channel.
    //! \param  refChanLabel is the corresponding channel number (i.e. 1
    //!         if refChanFreq is the lowest-frequency channel, and all 
    //!         other channels are harmonics of refChanFreq, or 2 if  
    //!         refChanFreq tracks the second harmonic, etc.).
    //! \param  stretchFactor is a stretching factor to account for detuned 
    //!         harmonics, default is 0. 
    //!
    //! \throw  InvalidArgument if refChanLabel is not positive.
    //! \throw  InvalidArgument if stretchFactor is negative.
    Channelizer( const Envelope & refChanFreq, int refChanLabel, double stretchFactor = 0 );
     
    //! Construct a new Channelizer having a constant reference frequency.
    //! The specified frequency is the center frequency of the lowest-frequency
    //! channel (for a harmonic sound, the channel containing the fundamental 
    //! Partial.
    //!
    //! \param  refFreq is the reference frequency (in Hz) corresponding
    //!         to the first frequency channel.
    //! \param  stretchFactor is a stretching factor to account for detuned 
    //!         harmonics, default is 0. 
    //!
    //! \throw  InvalidArgument if refChanLabel is not positive.
    //! \throw  InvalidArgument if stretchFactor is negative.
    Channelizer( double refFreq, double stretchFactor = 0 );
         
    //! Construct a new Channelizer that is an exact copy of another.
    //! The copy represents the same set of frequency channels, constructed
    //! from the same reference Envelope and channel number.
    //! 
    //! \param other is the Channelizer to copy
    Channelizer( const Channelizer & other );
     
    //! Assignment operator: make this Channelizer an exact copy of another. 
    //! This Channelizer is made to represent the same set of frequency channels, 
    //! constructed from the same reference Envelope and channel number as rhs.
    //!
    //! \param rhs is the Channelizer to copy
    Channelizer & operator=( const Channelizer & rhs );
     
    //! Destroy this Channelizer.
    ~Channelizer( void );
     
//  -- channelizing --

    //! Label a Partial with the number of the frequency channel containing
    //! the greatest portion of its (the Partial's) energy.
    //! 
    //! \param partial is the Partial to label.
    void channelize( Partial & partial ) const;

    //! Assign each Partial in the specified half-open (STL-style) range
    //! the label corresponding to the frequency channel containing the
    //! greatest portion of its (the Partial's) energy.
    //! 
    //! \param begin is the beginning of the range of Partials to channelize
    //! \param end is (one-past) the end of the range of Partials to channelize
    //! 
    //! If compiled with NO_TEMPLATE_MEMBERS defined, then begin and end
    //! must be PartialList::iterators, otherwise they can be any type
    //! of iterators over a sequence of Partials.
#if ! defined(NO_TEMPLATE_MEMBERS)
    template<typename Iter>
    void channelize( Iter begin, Iter end ) const;
#else
    void channelize( PartialList::iterator begin, PartialList::iterator end ) const;
#endif   

    //! Function call operator: same as channelize().
#if ! defined(NO_TEMPLATE_MEMBERS)
    template<typename Iter>
    void operator() ( Iter begin, Iter end ) const
#else
    inline
    void operator() ( PartialList::iterator begin, PartialList::iterator end ) const
#endif
         { channelize( begin, end ); }
         
    //! Compute the center frequency of one a channel at the specified
    //! time. For non-stretched harmonics, this is simply the value
    //! of the reference envelope scaled by the ratio of the specified
    //! channel number to the reference channel number. For stretched
    //! harmonics, the channel center frequency is computed using the
    //! stretch factor. See Martin Keane, "Understanding
    //! the complex nature of the piano tone", 2004, for a discussion
    //! and the source of the mode frequency stretching algorithms 
    //! implemented here.
    //!
    //! \param  time is the time (in seconds) at which to evalute 
    //!         the reference envelope
    //! \param  channel is the frequency channel (or harmonic, or vibrational     
    //!         mode) number whose frequency is to be determined
    //! \return the center frequency in Hz of the specified frequency channel
    //!         at the specified time
    double channelFrequencyAt( double time, int channel ) const;

    //! Compute the (fractional) channel number estimate for a Partial having a
    //! given frequency at a specified time. For ordinary harmonics, this
    //! is simply the ratio of the specified frequency to the reference
    //! frequency at the specified time. For stretched harmonics (as in 
    //! a piano), the stretching factor is used to compute the frequency
    //! of the corresponding modes of a massy string. See Martin Keane, 
    //! "Understanding the complex nature of the piano tone", 2004, for 
    //! the source of the mode frequency stretching algorithms 
    //! implemented here.
    //! 
    //! \param  time is the time (in seconds) at which to evalute 
    //!         the reference envelope
    //! \param  frequency is the frequency (in Hz) for wihch the channel
    //!         number is to be determined
    //! \return the channel number corresponding to the specified
    //!         frequency and time
    int computeChannelNumber( double time, double frequency ) const;
    
    //! Compute the (fractional) channel number estimate for a Partial having a
    //! given frequency at a specified time. For ordinary harmonics, this
    //! is simply the ratio of the specified frequency to the reference
    //! frequency at the specified time. For stretched harmonics (as in 
    //! a piano), the stretching factor is used to compute the frequency
    //! of the corresponding modes of a massy string. See Martin Keane, 
    //! "Understanding the complex nature of the piano tone", 2004, for 
    //! the source of the mode frequency stretching algorithms 
    //! implemented here.
    //!
    //! The fractional channel number is used internally to determine
    //! a best estimate for the channel number (label) for a Partial
    //! having time-varying frequency. 
    //! 
    //! \param  time is the time (in seconds) at which to evalute 
    //!         the reference envelope
    //! \param  frequency is the frequency (in Hz) for wihch the channel
    //!         number is to be determined
    //! \return the fractional channel number corresponding to the specified
    //!         frequency and time
    double computeFractionalChannelNumber( double time, double frequency ) const;
    
    
    //! Compute the reference frequency at the specified time. For non-stretched 
    //! harmonics, this is simply the ratio of the reference envelope evaluated 
    //! at that time to the reference channel number, and is the center frequecy
    //! for the lowest channel. For stretched harmonics, the reference frequency 
    //! is NOT equal to the center frequency of any of the channels, and is also
    //! a function of the stretch factor. 
    //!
    //! \param  time is the time (in seconds) at which to evalute 
    //!         the reference envelope
    double referenceFrequencyAt( double time ) const;

//  -- access/mutation --
         
    //! Return the exponent applied to amplitude before weighting
    //! the instantaneous estimate of the frequency channel number
    //! for a Partial. zero (default) for no weighting, 1 for linear
    //! amplitude weighting, 2 for power weighting, etc.
    //! Amplitude weighting is a bad idea for many sounds, particularly
    //! those with transients, for which it may emphasize the part of
    //! the Partial having the least reliable frequency estimate.
    double amplitudeWeighting( void ) const;

    //! Set the exponent applied to amplitude before weighting
    //! the instantaneous estimate of the frequency channel number
    //! for a Partial. zero (default) for no weighting, 1 for linear
    //! amplitude weighting, 2 for power weighting, etc.
    //! Amplitude weighting is a bad idea for many sounds, particularly
    //! those with transients, for which it may emphasize the part of
    //! the Partial having the least reliable frequency estimate.
    void setAmplitudeWeighting( double expon );

    //! Return the stretching factor used to account for detuned
    //! harmonics, as in a piano tone. Normally set to 0 for 
    //! in-tune harmonics.
    //!
    //! The stretching factor is a small positive number for 
    //! heavy vibrating strings (as in pianos) for which the
    //! mass of the string significantly affects the frequency
    //! of the vibrating modes. See Martin Keane, "Understanding
    //! the complex nature of the piano tone", 2004, for a discussion
    //! and the source of the mode frequency stretching algorithms 
    //! implemented here.
    double stretchFactor( void ) const;
        
    //! Set the stretching factor used to account for detuned
    //! harmonics, as in a piano tone. Normally set to 0 for 
    //! in-tune harmonics. The stretching factor for massy 
    //! vibrating strings (like pianos) can be computed from 
    //! the physical characteristics of the string, or using 
    //! computeStretchFactor(). 
    //!
    //! The stretching factor is a small positive number for 
    //! heavy vibrating strings (as in pianos) for which the
    //! mass of the string significantly affects the frequency
    //! of the vibrating modes. See Martin Keane, "Understanding
    //! the complex nature of the piano tone", 2004, for a discussion
    //! and the source of the mode frequency stretching algorithms 
    //! implemented here.
    //!
    //! \throw  InvalidArgument if stretch is negative.
    void setStretchFactor( double stretch );    
    
         
// -- static members --

     
    //! Static member to compute the stretch factor for a sound having
    //! (consistently) detuned harmonics, like piano tones.
    //!
    //! The stretching factor is a small positive number for 
    //! heavy vibrating strings (as in pianos) for which the
    //! mass of the string significantly affects the frequency
    //! of the vibrating modes. See Martin Keane, "Understanding
    //! the complex nature of the piano tone", 2004, for a discussion
    //! and the source of the mode frequency stretching algorithms 
    //! implemented here.
    //!
    //! The value returned by this function MAY NOT be a valid stretch
    //! factor. If this function returns a negative stretch factor,
    //! then the specified pair of frequencies and mode numbers cannot
    //! be used to estimate the effects of string mass on mode frequency
    //! (because the negative stretch factor implies a physical 
    //! impossibility, like negative mass or negative length). 
    //!
    //! \param      fm is the frequency of the Mth stretched harmonic
    //! \param      m is the harmonic number of the harmonic whose frequnecy is fm
    //! \param      fn is the frequency of the Nth stretched harmonic
    //! \param      n is the harmonic number of the harmonic whose frequnecy is fn
    //! \returns    the stretching factor, usually a very small positive
    //!             floating point number, or 0 for pefectly tuned harmonics
    //!             (that is, if fn = n*f1).
    static double computeStretchFactor( double fm, int m, double fn, int n );


// -- simplified interface --

    //! Static member that constructs an instance and applies
    //! it to a PartialList (simplified interface). 
    //!
    //! Construct a Channelizer using the specified Envelope
    //! and reference label, and use it to channelize a
    //! sequence of Partials. 
    //!
    //! \param  partials is the sequence of Partials to 
    //!         channelize.
    //! \param  refChanFreq is an Envelope representing the center frequency
    //!         of a channel.
    //! \param  refChanLabel is the corresponding channel number (i.e. 1
    //!         if refChanFreq is the lowest-frequency channel, and all 
    //!         other channels are harmonics of refChanFreq, or 2 if  
    //!         refChanFreq tracks the second harmonic, etc.).
    //! \throw  InvalidArgument if refChanLabel is not positive.
    static 
    void channelize( PartialList & partials,
                     const Envelope & refChanFreq, int refChanLabel );

     

// -- DEPRECATED members --

    //! DEPRECATED
    //!
    //! Set the stretching factor used to account for (consistently) 
    //! detuned harmonics, as in a piano tone, from a pair of 
    //! mode (harmonic) frequencies and numbers.
    //!
    //! The stretching factor is a small positive number for 
    //! heavy vibrating strings (as in pianos) for which the
    //! mass of the string significantly affects the frequency
    //! of the vibrating modes. See Martin Keane, "Understanding
    //! the complex nature of the piano tone", 2004, for a discussion
    //! and the source of the mode frequency stretching algorithms 
    //! implemented here.
    //!
    //! The stretching factor is computed using computeStretchFactor,
    //! but only a valid stretch factor will ever be assigned. If an
    //! invalid (negative) stretching factor is computed for the
    //! specified frequencies and mode numbers, the stretch factor
    //! will be set to zero.
    //!
    //! \param      fm is the frequency of the Mth stretched harmonic
    //! \param      m is the harmonic number of the harmonic whose frequnecy is fm
    //! \param      fn is the frequency of the Nth stretched harmonic
    //! \param      n is the harmonic number of the harmonic whose frequnecy is fn
    void setStretchFactor( double fm, int m, double fn, int n );
    

    //! DEPRECATED
    //!
    //! Static member that constructs an instance and applies
    //! it to a sequence of Partials. 
    //! Construct a Channelizer using the specified Envelope
    //! and reference label, and use it to channelize a
    //! sequence of Partials. 
    //!
    //! \param  begin is the beginning of a sequence of Partials to 
    //!         channelize.
    //! \param  end is the end of a sequence of Partials to 
    //!         channelize.
    //! \param  refChanFreq is an Envelope representing the center frequency
    //!         of a channel.
    //! \param  refChanLabel is the corresponding channel number (i.e. 1
    //!         if refChanFreq is the lowest-frequency channel, and all 
    //!         other channels are harmonics of refChanFreq, or 2 if  
    //!         refChanFreq tracks the second harmonic, etc.).
    //! \throw  InvalidArgument if refChanLabel is not positive.
    //! 
    //! If compiled with NO_TEMPLATE_MEMBERS defined, then begin and end
    //! must be PartialList::iterators, otherwise they can be any type
    //! of iterators over a sequence of Partials.
#if ! defined(NO_TEMPLATE_MEMBERS)
    template< typename Iter >
    static 
    void channelize( Iter begin, Iter end, 
                     const Envelope & refChanFreq, int refChanLabel );
#else
    static inline 
    void channelize( PartialList::iterator begin, PartialList::iterator end,
                     const Envelope & refChanFreq, int refChanLabel );
#endif   

     
    //! DEPRECATED
    //!
    //! Static member to compute the stretch factor for a sound having
    //! (consistently) detuned harmonics, like piano tones. Legacy version
    //! that assumes the first argument corresponds to the first partial.
    //!
    //! \param      f1 is the frequency of the lowest numbered (1) partial.
    //! \param      fn is the frequency of the Nth stretched harmonic
    //! \param      n is the harmonic number of the harmonic whose frequnecy is fn
    //! \returns    the stretching factor, usually a very small positive
    //!             floating point number, or 0 for pefectly tuned harmonics
    //!             (that is, for harmonic frequencies fn = n*f1).
    static double computeStretchFactor( double f1, double fn, double n );
    
};  //  end of class Channelizer

// ---------------------------------------------------------------------------
//  channelize (sequence of Partials)
// ---------------------------------------------------------------------------
//! Assign each Partial in the specified half-open (STL-style) range
//! the label corresponding to the frequency channel containing the
//! greatest portion of its (the Partial's) energy.
//! 
//! \param begin is the beginning of the range of Partials to channelize
//! \param end is (one-past) the end of the range of Partials o channelize
//! 
//! If compiled with NO_TEMPLATE_MEMBERS defined, then begin and end
//! must be PartialList::iterators, otherwise they can be any type
//! of iterators over a sequence of Partials.
//
#if ! defined(NO_TEMPLATE_MEMBERS)
template<typename Iter>
void Channelizer::channelize( Iter begin, Iter end ) const
#else
inline
void Channelizer::channelize( PartialList::iterator begin, PartialList::iterator end ) const
#endif
{
    while ( begin != end )
    {
        channelize( *begin++ );
    }
}

// ---------------------------------------------------------------------------
//  channelize (static)
// ---------------------------------------------------------------------------
//! DEPRECATED
//!
//!   Static member that constructs an instance and applies
//!   it to a sequence of Partials. 
//!   Construct a Channelizer using the specified Envelope
//!   and reference label, and use it to channelize a
//!   sequence of Partials. 
//!
//!   \param   begin is the beginning of a sequence of Partials to 
//!            channelize.
//!   \param   end is the end of a sequence of Partials to 
//!            channelize.
//!   \param    refChanFreq is an Envelope representing the center frequency
//!             of a channel.
//!   \param    refChanLabel is the corresponding channel number (i.e. 1
//!             if refChanFreq is the lowest-frequency channel, and all 
//!             other channels are harmonics of refChanFreq, or 2 if  
//!             refChanFreq tracks the second harmonic, etc.).
//!   \throw   InvalidArgument if refChanLabel is not positive.
//! 
//! If compiled with NO_TEMPLATE_MEMBERS defined, then begin and end
//! must be PartialList::iterators, otherwise they can be any type
//! of iterators over a sequence of Partials.
//
#if ! defined(NO_TEMPLATE_MEMBERS)
template< typename Iter >
void Channelizer::channelize( Iter begin, Iter end, 
                              const Envelope & refChanFreq, int refChanLabel )
#else
inline
void Channelizer::channelize( PartialList::iterator begin, PartialList::iterator end,
                              const Envelope & refChanFreq, int refChanLabel )
#endif   
{
   Channelizer instance( refChanFreq, refChanLabel );
    while ( begin != end )
    {
        instance.channelize( *begin++ );
    }
}

}   //  end of namespace Loris

#endif /* ndef INCLUDE_CHANNELIZER_H */
