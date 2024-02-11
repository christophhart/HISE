#ifndef INCLUDE_FOURIERTRANSFORM_H
#define INCLUDE_FOURIERTRANSFORM_H
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
 * FourierTransform.h
 *
 * Definition of class Loris::FourierTransform, providing a simplified
 * uniform interface to the FFTW library (www.fftw.org), version 2.1.3
 * or newer (including version 3), or to the General Purpose FFT package
 * by Takuya OOURA, http://momonga.t.u-tokyo.ac.jp/~ooura/fft.html if
 * FFTW is unavailable. 
 *
 * Kelly Fitz, 2 Jun 2006
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */
#include <complex>
#include <vector>

//	begin namespace
namespace Loris {

//  insulating implementation class, defined in FourierTransform.C
class FTimpl;

// ---------------------------------------------------------------------------
//	class FourierTransform
//
//! FourierTransform provides a simplified interface to the FFTW library 
//! (www.fftw.org). Loris uses the FFTW library to perform efficient 
//! Fourier transforms of arbitrary length. Clients store and access 
//! the in-place transform data as a sequence of std::complex< double >.
//! Samples are stored in the FourierTransform instance using subscript
//! or iterator access, the transform is computed by the transform member,
//! and the transformed samples replace the input samples, and are 
//! accessed by subscript or iterator. FourierTransform computes a complex
//! transform, so it can be used to invert a transform of real samples
//! as well. Uses the standard library complex class, which implements
//! arithmetic operations. 
//!
//! Supports FFTW versions 2 and 3.
//! Does not make use of FFTW "wisdom" to speed up transform computation.
//!
//! If FFTW is unavailable, uses instead the General Purpose FFT package
//! by Takuya OOURA, http://momonga.t.u-tokyo.ac.jp/~ooura/fft.html defined
//! in fftsg.c for power-of-two transforms, and a very slow direct DFT
//! implementation for non-PO2 transforms. 
//
class FourierTransform 
{
//	-- public interface --
public:
   
    //! An unsigned integral type large enough
    //! to represent the length of any transform.
    typedef std::vector< std::complex< double > >::size_type size_type;

    //! The type of a non-const iterator of (complex) transform samples.
    typedef std::vector< std::complex< double > >::iterator iterator;

    //! The type of a const iterator of (complex) transform samples.		
    typedef std::vector< std::complex< double > >::const_iterator const_iterator;

//	--- lifecycle ---

    //! Initialize a new FourierTransform of the specified size.
    //!
    //! \param  len is the length of the transform in samples (the
    //!         number of samples in the transform)
    //! \throw  RuntimeError if the necessary buffers cannot be 
    //!         allocated, or there is an error configuring FFTW.
    FourierTransform( size_type len );

    //! Initialize a new FourierTransform that is a copy of another,
    //! having the same size and the same buffer contents.
    //!
    //! \param  rhs is the instance to copy
    //! \throw  RuntimeError if the necessary buffers cannot be 
    //!         allocated, or there is an error configuring FFTW.
    FourierTransform( const FourierTransform & rhs );

    //! Free the resources associated with this FourierTransform.
    ~FourierTransform( void );	

//	--- operators ---
		
    //! Make this FourierTransform a copy of another, having
    //! the same size and buffer contents.
    //!
    //! \param  rhs is the instance to copy
    //! \return a refernce to this instance
    //! \throw  RuntimeError if the necessary buffers cannot be 
    //!         allocated, or there is an error configuring FFTW.
    FourierTransform & operator= ( const FourierTransform & rhs );


//	--- access/mutation ---

    //! Access (read/write) a transform sample by index.
    //! Use this member to fill the transform buffer before
    //! computing the transform, and to access the samples
    //! after computing the transform. (inlined for speed)
    //!
    //! \param  index is the index or rank of the complex
    //!         transform sample to access. Zero is the first
    //!         position in the buffer.
    //! \return non-const reference to the std::complex< double >
    //!         at the specified position in the buffer.
    std::complex< double > & operator[] ( size_type index )
    { 
        return _buffer[ index ]; 
    }

    //! Access (read-only) a transform sample by index.
    //! Use this member to fill the transform buffer before
    //! computing the transform, and to access the samples
    //! after computing the transform. (inlined for speed)
    //!
    //! \param  index is the index or rank of the complex
    //!         transform sample to access. Zero is the first
    //!         position in the buffer.
    //! \return const reference to the std::complex< double >
    //!         at the specified position in the buffer.
    const std::complex< double > & operator[] ( size_type index ) const
    { 
        return _buffer[ index ]; 
    }

    //! Return an iterator refering to the beginning of the sequence of
    //! complex samples in the transform buffer.
    //!
    //! \return a non-const iterator refering to the first position
    //!         in the transform buffer. 
    iterator begin( void )	
    { 
        return _buffer.begin(); 
    }
	
    //! Return an iterator refering to the end of the sequence of
    //! complex samples in the transform buffer.
    //!
    //! \return a non-const iterator refering to one past the last 
    //!         position in the transform buffer. 
    iterator end( void )	
    { 
        return _buffer.end(); 
    }

    //! Return a const iterator refering to the beginning of the sequence of
    //! complex samples in the transform buffer.
    //!
    //! \return a const iterator refering to the first position
    //!         in the transform buffer. 
    const_iterator begin( void ) const	
    { 
        return _buffer.begin(); 
    }
	
    //! Return a const iterator refering to the end of the sequence of
    //! complex samples in the transform buffer.
    //!
    //! \return a const iterator refering to one past the last 
    //!         position in the transform buffer. 
    const_iterator end( void ) const 	
    { 
        return _buffer.end(); 
    }

//	--- operations ---
		
    //! Compute the Fourier transform of the samples stored in the 
    //! transform buffer. The samples stored in the transform buffer
    //! (accessed by index or by iterator) are replaced by the 
    //! transformed samples, in-place. 
    void transform( void );

//	--- inquiry ---

    //! Return the length of the transform (in samples).
    //! 
    //! \return the length of the transform in samples.
    size_type size( void ) const ;
                
//	-- instance variables --
private:

    //! buffer containing the complex transform input before
    //! computing the transform, and the complex transform output
    //! after computing the transform
    std::vector< std::complex< double > > _buffer;

    // insulating implementation instance (defined in 
    // FourierTransform.C), conceals interface to FFTW
    FTimpl * _impl;

};	//	end of class FourierTransform


}	//	end of namespace Loris

#endif /* ndef INCLUDE_FOURIERTRANSFORM_H */
