#ifndef INCLUDE_REASSIGNEDSPECTRUM_H
#define INCLUDE_REASSIGNEDSPECTRUM_H
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
 * ReassignedSpectrum.h
 *
 * Definition of class Loris::ReassignedSpectrum.
 *
 * Kelly Fitz, 7 Dec 1999
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */

#include "FourierTransform.h"
#include <vector>

//	begin namespace
namespace Loris {

// ---------------------------------------------------------------------------
//	class ReassignedSpectrum
//
//!	Computes a reassigned short-time Fourier spectrum using the transform
//! method of Auger and Flandrin. 
//	
class ReassignedSpectrum
{
//	-- public interface --
public:
    //! An unsigned integral type large enough
    //! to represent the length of any transform.
    typedef FourierTransform::size_type size_type;

//	--- lifecycle ---

    //! Construct a new instance using the specified short-time window.
    //!	Transform lengths are the smallest power of two greater than twice the
    //!	window length.
	ReassignedSpectrum( const std::vector< double > & window );

    //! Construct a new instance using the specified short-time window and
    //! its time derivative.
    //!	Transform lengths are the smallest power of two greater than twice the
    //!	window length.
	ReassignedSpectrum( const std::vector< double > & window,
                        const std::vector< double > & windowDerivative );
    
	// compiler-generated copy, assign, and destroy are sufficient

//	--- operations ---

    //!	Compute the reassigned Fourier transform of the samples on the half open
    //!	range [sampsBegin, sampsEnd), aligning sampCenter with the center of
    //!	the analysis window.
    //!
    //! \param  sampsBegin pointer representing the beginning of 
    //!         the (half-open) range of samples to transform
    //! \param  sampCenter the sample in the range that is to be 
    //!         aligned with the center of the analysis window
    //! \param  sampsEnd pointer representing the end of 
    //!         the (half-open) range of samples to transform
    //!
    //! \pre    sampsBegin must not be past sampCenter
    //! \pre    sampsEnd must be past sampCenter
    //! \post   the transform buffers store the reassigned 
    //!         short-time transform data for the specified 
    //!         samples
	void transform( const double * sampsBegin, const double * pos, const double * sampsEnd );
	
//	--- inquiry ---

    //! Return the length of the Fourier transforms.
	size_type size( void ) const;	

    //! Return read access to the short-time window samples.
    //!	(Peers may need to know about the analysis window
    //!	or about the scale factors in introduces.)
	const std::vector< double > & window( void ) const;
	

//	--- reassigned transform access ---
        
	//! Compute and return the convergence indicator, computed from the 
	//!	mixed partial derivative of spectral phase, optionally used in 
	//!	BW enhanced analysis as a convergence indicator. The convergence
	//!	value is on the range [0,1], 0 for a sinusoid, and 1 for an impulse.
    //!
    //! \param  idx the frequency sample at which to evaluate the
    //!         transform
	double convergence( long idx ) const;

    //! Return the reassigned frequency in fractional frequency 
    //! samples computed at the specified transform index.
    //!
    //! \param  idx the frequency sample at which to evaluate the
    //!         transform
	double reassignedFrequency( long idx ) const;

    //! Return the spectrum magnitude (absolute)
    //! computed at the specified transform index.
    //!
    //! \param  idx the frequency sample at which to evaluate the
    //!         transform
	double reassignedMagnitude( long idx ) const;

    //! Return the phase in radians computed at the specified transform index.
    //!	The reassigned phase is shifted to account for the time
    //!	correction according to the corrected frequency.
    //!
    //!
    //! \param  idx the frequency sample at which to evaluate the
    //!         transform
	double reassignedPhase( long idx ) const;	

    //! Return the reassigned time in fractional samples
    //! computed at the specified transform index.
    //!
    //! \param  idx the frequency sample at which to evaluate the
    //!         transform
	double reassignedTime( long idx ) const;

//	--- reassignment operations ---
	
    //!	Compute the frequency correction at the specified frequency sample
    //! using the method of Auger and Flandrin to evaluate the partial
    //! derivative of spectrum phase w.r.t. time.
    //!
    //!	Correction is computed in fractional frequency samples, because
    //!	that's the kind of frequency domain ramp we used on our window.
    //!	sample is the frequency sample index, the nominal component 
    //!	frequency in samples. 
	double frequencyCorrection( long sample ) const;

    //!	Compute the time correction at the specified frequency sample
    //! using the method of Auger and Flandrin to evaluate the partial
    //! derivative of spectrum phase w.r.t. frequency.
    //!
    //!	Correction is computed in fractional samples, because
    //!	that's the kind of ramp we used on our window.
	double timeCorrection( long sample ) const;

//	--- legacy support ---

    //  These members are deprecated, and included only
    //  to support old code. New code should use the
    //  corresponding documented members.
    //  All of these members are deprecated.
    
    double reassignedPhase( long idx, double, double ) const
        { return reassignedPhase( idx ); }
    double reassignedMagnitude( double, long intBinNumber ) const
        { return reassignedMagnitude( intBinNumber ); }
    
    //  subscript operator
    //  The signature has changed, can no longer return a reference,
    //  but since the reference returned was const, this version should 
    //  keep most old code working, if not all.
    std::complex< double > operator[]( unsigned long idx ) const;
	
private:

//	-- window building helpers --

    //	Build a pair of complex-valued windows, one having the frequency-ramp 
    //  (time-derivative) window in the real part and the time-ramp window in the 
    //  imagnary part, and the other having the unmodified window in the real part
    //  and, if computing mixed deriviatives, the time-ramp time-derivative window
    //  in the imaginary part.
    //
    //  Input is the unmodified window function.
    void buildReassignmentWindows( const std::vector< double > & window );
    
    //	Build a pair of complex-valued windows, one having the frequency-ramp 
    //  (time-derivative) window in the real part and the time-ramp window in the 
    //  imagnary part, and the other having the unmodified window in the real part
    //  and, if computing mixed deriviatives, the time-ramp time-derivative window
    //  in the imaginary part.
    //
    //  Input is the unmodified window function and its time derivative, so the
    //  DFT kludge is unnecessary.
    void buildReassignmentWindows( const std::vector< double > & window,
                                   const std::vector< double > & windowDerivative );    

//	-- instance variables --

	//! the FourierTransform for computing magnitude and phase
	FourierTransform mMagnitudeTransform;
	
	//! the FourierTransform for computing time and frequency corrections
	FourierTransform mCorrectionTransform;
	
	//! the original short-time analysis window samples
	std::vector< double > mWindow;                          //  W(n)
	
	//! the complex window used to compute the 
    //! magnitude/phase transform
	std::vector< std::complex< double > > mCplxWin_W_Wtd;   //  real W(n), imag nW'(n)

	//! the complex window used to compute the 
    //! time/frequency correction transform
	std::vector< std::complex< double > > mCplxWin_Wd_Wt;   //  real W'(n), imag nW(n)
		
};	//	end of class ReassignedSpectrum

}	//	end of namespace Loris

#endif /* ndef INCLUDE_REASSIGNEDSPECTRUM_H */
