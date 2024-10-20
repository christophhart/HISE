#ifndef INCLUDE_OSCILLATOR_H
#define INCLUDE_OSCILLATOR_H
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
 * Oscillator.h
 *
 * Definition of class Loris::Oscillator, a Bandwidth-Enhanced Oscillator.
 *
 * Kelly Fitz, 31 Aug 1999
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */

#include "NoiseGenerator.h"
#include "Filter.h"

//  begin namespace
namespace Loris {

class Breakpoint;

// ---------------------------------------------------------------------------
//  class Oscillator
//
//! Class Oscillator represents the state of a single bandwidth-enhanced
//! sinusoidal oscillator used for synthesizing sounds from Reassigned
//! Bandwidth-Enhanced analysis data. Oscillator encapsulates the oscillator
//! state, including the instantaneous radian frequency (radians per
//! sample), amplitude, bandwidth coefficient, and phase, and a 
//! bandlimited stochastic modulator. 
//!
//! Class Synthesizer uses an instance of Oscillator to synthesize
//! bandwidth-enhanced Partials.
//
class Oscillator
{
//  --- implementation ---

    NoiseGenerator m_modulator;     //! stochastic modulator
    Filter m_filter;                //! filter applied to the noise generator
    
    //  instantaneous oscillator state:
    double m_instfrequency;         //! radians per sample
    double m_instamplitude;         //! absolute amplitude
    double m_instbandwidth;         //! bandwidth coefficient (noise energy / total energy)
    
    //  accumulating phase state:
    double m_determphase;       //! deterministic phase in radians

//  --- interface ---
public:
//  --- construction ---

    //! Construct a new Oscillator with all state parameters initialized to 0.
    Oscillator( void );
     
    //  Copy, assignment, and destruction are free.
    //
    //  Copied and assigned Oscillators have the duplicate state
    //  variables and the filters have the same coefficients,
    //  but the state of the filter delay lines is not copied.

// --- oscillation ---
     
    //! Reset the instantaneous envelope parameters 
    //! (frequency, amplitude, bandwidth, and phase).
    //! The sample rate is needed to convert the 
    //! Breakpoint frequency (Hz) to radians per sample.
    void resetEnvelopes( const Breakpoint & bp, double srate );
      
    //! Reset the phase of the Oscillator to the specified
    //! value. This is done when the amplitude of a Partial 
    //! goes to zero, so that onsets are preserved in distilled
    //! and collated Partials.
    void setPhase( double ph );

    //! Accumulate bandwidth-enhanced sinusoidal samples modulating the
    //! oscillator state from its current values of radian frequency, amplitude,
    //! and bandwidth to the specified target values. Accumulate samples into
    //! the half-open (STL-style) range of doubles, starting at begin, and
    //! ending before end (no sample is accumulated at end). The caller must
    //! insure that the indices are valid. Target frequency and bandwidth are
    //! checked to prevent aliasing and bogus bandwidth enhancement.
    void oscillate( double * begin, double * end,
                    const Breakpoint & bp, double srate );

// --- accessors ---

    //! Return the instantaneous amplitde of the Oscillator.
    double amplitude( void ) const { return m_instamplitude; }
    
    //! Return the instantaneous bandwidth of the Oscillator.
    double bandwidth( void ) const { return m_instbandwidth; }
    
    //! Return the instantaneous phase of the Oscillator.
    double phase( void ) const { return m_determphase; }
    
    //! Return the instantaneous radian frequency of the Oscillator.
    double radianFreq( void ) const { return m_instfrequency; }
    
    //! Return access to the Filter used by this oscillator to 
    //! implement bandwidth-enhanced sinusoidal synthesis.
    Filter & filter( void ) { return m_filter; }
    
// --- static members ---

    //! Static local function for obtaining a prototype Filter
    //! to use in Oscillator construction. Eventually, allow
    //! external (client) specification of the Filter prototype.
    static const Filter & prototype_filter( void );
     
};  //  end of class Oscillator

}   //  end of namespace Loris

#endif /* ndef INCLUDE_OSCILLATOR_H */
