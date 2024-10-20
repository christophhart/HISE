#ifndef INCLUDE_F0ESTIMATE_H
#define INCLUDE_F0ESTIMATE_H
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
 * F0Estimate.h
 *
 * Implementation of an iterative alrogithm for computing an 
 * estimate of fundamental frequency from a sequence of sinusoidal
 * frequencies and amplitudes using a likelihood estimator
 * adapted from Quatieri's Speech Signal Processing text. The 
 * algorithm here takes advantage of the fact that spectral peaks
 * have already been identified and extracted in the analysis/modeling
 * process.
 *
 * Kelly Fitz, 28 March 2006
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */

#include <vector>

//	begin namespace
namespace Loris {

// ---------------------------------------------------------------------------
//  class F0Estimate
//
//! Represents a configuration of an iterative alrogithm for computing an 
//! estimate of fundamental frequency from a sequence of sinusoidal
//! frequencies and amplitudes using a likelihood estimator adapted
//! from Quatieri's Speech Signal Processing text. This algorithm takes 
//! advantage of the fact that spectral peaks have already been identified 
//! and extracted in the analysis/modeling process.
//!
//! The algorithm consists of the following steps:
//! 1)  Identify candidate F0s as the integer divisors of the sinusoidal 
//!     frequencies provided, within the specified range (this algorithm
//!     relies on the reasonable assumption that for any frequency recognized 
//!     as a likely F0, at least one of the sinusoidal frequencies must 
//!     represent a harmonic, the likelihood function makes this same 
//!     assumption)
//! 2)  Select the highest frequency candidate (within range) that maximizes 
//!     the likelihood function (because all subharmonics of the true F0 will
//!     be equal in likelihood to the true F0, but no higher frequency can
//!     be as likely).
//! 2a) Check the likelihood of integer multiples of the best candidate,
//!     choose the highest multiple (within the specified range) that
//!     as likely as the best candidate frequency to be the new best
//!     candidate. 
//! 3)  Refine the best candidate using the secant method for refining the 
//!     root of the derivative of the likelihood function in the neighborhood
//!     of the best candidate (because a peak in the likelihood function is
//!     a root of the derivative of that function).
//

class F0Estimate
{
private:

    double m_frequency;   //!  estimated fundamental frequency in Hz
    double m_confidence;  //!  normalized confidence for this estimate, 
                          //!  equal to 1.0 when all frequencies are perfect
                          //!  harmonics of this estimate's frequency

public:

    //  --- lifecycle ---

    //! Construct from parameters of the iterative F0 estimation 
    //! algorithm. Find candidate F0 estimates as integer divisors
    //! of the peak frequencies, pick the highest frequency of the
    //! most likely candidates, and refine that estiamte using the
    //! secant method. 
    //!
    //! Store the frequency and the normalized value of the 
    //! likelihood function at that frequency (1.0 indicates that
    //! all the peaks are perfect harmonics of the estimated
    //! frequency).

    F0Estimate( const std::vector<double> & amps, 
                const std::vector<double> & freqs, 
                double fmin, double fmax,
                double resolution );
                
    //  default copy/assign/destroy are OK


    //  Not sure whether or why these would be useful.
    //
    // F0Estimate( void ) : m_frequency( 0 ), m_confidence( 0 ) {}
    // F0Estimate( double f, double c ) : m_frequency( f ), m_confidence( c ) {}
    
    
    //  --- accessors ---
    
    //! Return the F0 frequency estimate, in Hz, for this estimate.
        
    double frequency( void ) const { return m_frequency; }
    
    //! Return the normalized confidence for this estimate, 
    //! equal to 1.0 when all frequencies are perfect
    //! harmonics of this estimate's frequency.
        
    double confidence( void ) const { return m_confidence; }
    

                    
};  //  end of class F0Estimate


}	//	end of namespace Loris

#endif  //  ndef INCLUDE_F0ESTIMATE_H
