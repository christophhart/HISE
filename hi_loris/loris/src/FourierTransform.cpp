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
 * FourierTransform.C
 *
 * Implementation of class Loris::FourierTransform, providing a simplified
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

#if HAVE_CONFIG_H
	#include "config.h"
#endif

#include "FourierTransform.h"
#include "LorisExceptions.h"
#include "Notifier.h"

#include <cmath>
#include <complex>

#if defined(HAVE_M_PI) && (HAVE_M_PI)
	const double Pi = M_PI;
#else
	const double Pi = std::acos( -1.0 );
#endif

#if defined(HAVE_FFTW3_H) && HAVE_FFTW3_H
    #include <fftw3.h>
#elif defined(HAVE_FFTW_H) && HAVE_FFTW_H
    #include <fftw.h>
#endif

// ---------------------------------------------------------------------------
//	isPO2 - return true if N is a power of two
// ---------------------------------------------------------------------------
//  If out_expon is non-zero, return the exponent in that address.
//
static bool isPO2( unsigned int N, int * out_expon = 0 )
{
    unsigned int M = 1;
    int exp = 0;
    while ( M < N )
    {
        M *= 2;
        ++exp;
    }
    if ( 0 != out_expon && M == N )
    {
        *out_expon = exp;
    }
    return M == N;
}

// ===========================================================================
// No longer matters that fftw and this class use the same floating point
// data format. The insulating implementation class now does its job of
// really insulating clients completely from FFTW, by copying data between
// buffers of std::complex< double > and fftw_complex, rather than 
// relying on those two complex types to have the same memory layout.
// The overhead of copying is probably not significant, compared to 
// the expense of computing the transform. 
//
// about complex math functions for fftw_complex:
//
// These functions are all defined as templates in <complex>.
// Regrettably, they are all implemented using real() and 
// imag() _member_ functions of the template argument, T. 
// If they had instead been implemented in terms of the real()
// and imag() (template) free functions, then I could just specialize
// those two for the fftw complex data type, and the other template
// functions would work. Instead, I have to specialize _all_ of
// those functions that I want to use. I hope this was a learning 
// experience for someone... In the mean time, the alternative I 
// have is to take advantage of the fact that fftw_complex and 
// std::complex<double> have the same footprint, so I can just
// cast back and forth between the two types. Its icky, but it 
// works, and its a lot faster than converting, and more palatable
// than redefining all those operators.
//
// On the subject of brilliant designs, fftw_complex is defined as
// a typedef of an anonymous struct, as in typedef struct {...} fftw_complex,
// so I cannot forward-declare that type.
//
// In other good news, the planning structure got a slight name change
// in version 3, making it even more important to remove all traces of
// FFTW from the FourierTransform class definition.
//
// ===========================================================================

//	begin namespace
namespace Loris {

using std::complex;
using std::vector;

// --- private implementation class ---

// ---------------------------------------------------------------------------
//  FTimpl
//
// Insulating implementation class to insulate clients
// completely from everything about the interaction between
// Loris and FFTW. There is more copying of data between buffers,
// but this is not the expensive part of computing Fourier transforms
// and we don't have to do unsavory things that require std::complex
// and fftw_complex to have the same memory layout (we could even get
// by with single-precision floats in FFTW if necessary). Also got
// rid of lots of shared buffer stuff that just made the implementation
// lots more complicated than necessary. This one is simple, if not
// as memory efficient.
//

#if defined(HAVE_FFTW3_H) && HAVE_FFTW3_H

class FTimpl    //  FFTW version 3
{
private:

	fftw_plan plan;
	FourierTransform::size_type N;
	fftw_complex * ftIn;   
	fftw_complex * ftOut;

public:
   
	// Construct an implementation instance:
	// allocate an input buffer, and an output buffer
	// and make a plan.
	FTimpl( FourierTransform::size_type sz ) : 
	  plan( 0 ), N( sz ), ftIn( 0 ), ftOut( 0 ) 
	{      
		// allocate buffers:
		ftIn = (fftw_complex *)fftw_malloc( sizeof( fftw_complex ) * N );
		ftOut = (fftw_complex *)fftw_malloc( sizeof( fftw_complex ) * N );
		if ( 0 == ftIn || 0 == ftOut )
		{
			fftw_free( ftIn );
			fftw_free( ftOut );
			throw RuntimeError( "cannot allocate Fourier transform buffers" );
		}
	  
		//	create a plan:
		plan = fftw_plan_dft_1d( N, ftIn, ftOut, FFTW_FORWARD, FFTW_ESTIMATE );

		//	verify:
		if ( 0 == plan )
		{
			Throw( RuntimeError, "FourierTransform could not make a (fftw) plan." );
		}
	}
   
	// Destroy the implementation instance:
	// dump the plan.
	~FTimpl( void )
	{
		if ( 0 != plan )
		{
            fftw_destroy_plan( plan );
		}         
		
		fftw_free( ftIn );
		fftw_free( ftOut );
	}
	
	// Copy complex< double >'s from a buffer into ftIn, 
	// the buffer must be as long as ftIn.
	void loadInput( const complex< double > * bufPtr )
	{
		for ( FourierTransform::size_type k = 0; k < N; ++k )
		{
			ftIn[ k ][0] = bufPtr->real();  //  real part
			ftIn[ k ][1] = bufPtr->imag();  //  imaginary part
			++bufPtr;
		}
	}
   
	// Copy complex< double >'s from ftOut into a buffer,
	// which must be as long as ftOut.
	void copyOutput( complex< double > * bufPtr ) const
	{
		for ( FourierTransform::size_type k = 0; k < N; ++k )
		{
			*bufPtr = complex< double >( ftOut[ k ][0], ftOut[ k ][1] );
			++bufPtr;
		}
	}
    
    // Compute a forward transform.
    void forward( void )
    {
        fftw_execute( plan );
    }
    
}; // end of class FTimpl for FFTW version 3

#elif defined(HAVE_FFTW_H) && HAVE_FFTW_H

//	"die hook" for FFTW, which otherwise try to write to a
//	non-existent console.
static void fftw_die_Loris( const char * s )
{
	using namespace std; // exit might be hidden in there
	
	notifier << "The FFTW library used by Loris has encountered a fatal error: " << s << endl;
	exit(EXIT_FAILURE);
}

class FTimpl    //  FFTW version 2
{
private:

	fftw_plan plan;
	FourierTransform::size_type N;
	fftw_complex * ftIn;   
	fftw_complex * ftOut;
   
public:

	// Construct an implementation instance:
	// allocate an input buffer, and an output buffer
	// and make a plan.
	FTimpl( FourierTransform::size_type sz ) : 
	  plan( 0 ), N( sz ), ftIn( 0 ), ftOut( 0 ) 
	{      
		// allocate buffers:
		ftIn = (fftw_complex *)fftw_malloc( sizeof( fftw_complex ) * N );
		ftOut = (fftw_complex *)fftw_malloc( sizeof( fftw_complex ) * N );
		if ( 0 == ftIn || 0 == ftOut )
		{
			fftw_free( ftIn );
			fftw_free( ftOut );
			Throw( RuntimeError, "cannot allocate Fourier transform buffers" );
		}
	  
		//	create a plan:
		plan = fftw_create_plan_specific( N, FFTW_FORWARD, FFTW_ESTIMATE,
                                          ftIn, 1, ftOut, 1 );

		//	verify:
		if ( 0 == plan )
		{
			Throw( RuntimeError, "FourierTransform could not make a (fftw) plan." );
		}

        //	FFTW calls fprintf a lot, which may be a problem in
        //	non-console-enabled applications. Catch fftw_die()
        //	calls by routing the error message to our own Notifier
        //	and exiting, using the function defined above.
        //
        //	(version 2 only)
        fftw_die_hook = fftw_die_Loris;
	}
   
	// Destroy the implementation instance:
	// dump the plan.
	~FTimpl( void )
	{
		if ( 0 != plan )
		{
            fftw_destroy_plan( plan );
		}         
		
		fftw_free( ftIn );
		fftw_free( ftOut );
	}
	
	// Copy complex< double >'s from a buffer into ftIn, 
	// the buffer must be as long as ftIn.
	void loadInput( const complex< double > * bufPtr )
	{
		for ( FourierTransform::size_type k = 0; k < N; ++k )
		{
			c_re( ftIn[ k ] ) = bufPtr->real();
			c_im( ftIn[ k ] ) = bufPtr->imag();
			++bufPtr;
		}
	}
   
	// Copy complex< double >'s from ftOut into a buffer,
	// which must be as long as ftOut.
	void copyOutput( complex< double > * bufPtr ) const
	{
		for ( FourierTransform::size_type k = 0; k < N; ++k )
		{
			*bufPtr = complex< double >( c_re( ftOut[ k ] ), c_im( ftOut[ k ] ) );
			++bufPtr;
		}
	}
    
    // Compute a forward transform.
    void forward( void )
    {
        fftw_one( plan, ftIn, ftOut );	
    }
    
}; // end of class FTimpl for FFTW version 2

#else

#define SORRY_NO_FFTW  1

//  function prototype, definition in fftsg.c
extern "C" void cdft(int, int, double *, int *, double *);

//  function prototype, definition below
static void slowDFT( double * in, double * out, int N );

//  Uses General Purpose FFT (Fast Fourier/Cosine/Sine Transform) Package
//  by Takuya OOURA, http://momonga.t.u-tokyo.ac.jp/~ooura/fft.html defined
//  in fftsg.c.
//
//  In the event that the size is not a power of two, uses a (very) slow
//  direct DFT computation, defined below. In this case, the workspace
//  array is not used, and the twiddle factor array is used to store the
//  transform result.

class FTimpl    //  platform-neutral stand-alone implementation
{
private:

	double * mTxInOut;      //	input/output buffer for in-place transform                                
	double * mTwiddle;      //	storage for twiddle factors
	int * mWorkspace;		//	workspace storage

	FourierTransform::size_type N;
    
    bool mIsPO2;
   
public:

	// Construct an implementation instance:
	// allocate buffers and workspace, and
	// initialize the twiddle factors.
	FTimpl( FourierTransform::size_type sz ) : 
	  mTxInOut( 0 ), mTwiddle( 0 ), mWorkspace( 0 ), N( sz ), mIsPO2( isPO2( sz ) )
	{      
        mTxInOut = new double[ 2*N ]; 	
            //	input/output buffer for in-place transform
            
        if ( mIsPO2 )
        {    
            mTwiddle = new double[ N/2 ]; 		
                //	storage for twiddle factors
                
            mWorkspace = new int[ 2*int( std::sqrt((double)N) + 0.5 ) ];		
                //	workspace 
                
            if ( 0 == mTxInOut || 0 == mTwiddle || 0 == mWorkspace )
            {
                delete [] mTxInOut;
                delete [] mTwiddle;
                delete [] mWorkspace;
                Throw( RuntimeError, "FourierTransform: could not initialize tranform" );
            }

            mWorkspace[0] = 0;  // first time only, triggers setup    
                                // no need to do it now, it will happen the 
                                // first time a transform is computed
            // cdft_double( 2*N, -1, mTxInOut, mWorkspace, mTwiddle );
        }
        else
        {
            mTwiddle = new double[ 2*N ]; 	
                //	use for result in slowDFT 
                
            if ( 0 == mTxInOut || 0 == mTwiddle )
            {
                delete [] mTxInOut;
                delete [] mTwiddle;
                Throw( RuntimeError, "FourierTransform: could not initialize tranform" );
            }
        }
	}
   
	// Destroy the implementation instance:
	~FTimpl( void )
	{
        delete [] mTxInOut;
        delete [] mTwiddle;
        delete [] mWorkspace;
	}
	
	// Copy complex< double >'s from a buffer into ftIn, 
	// the buffer must be as long as ftIn.
	void loadInput( const complex< double > * bufPtr )
	{
		for ( FourierTransform::size_type k = 0; k < N; ++k )
		{
			mTxInOut[ 2*k ] = bufPtr->real();
			mTxInOut[ 2*k+1 ] = bufPtr->imag();
			++bufPtr;
		}
	}
   
	//  Copy complex< double >'s from ftOut into a buffer,
	//  which must be as long as ftOut. Result might be
    //  stored in the twiddle factor array if this is not
    //  power of two length DFT.
	void copyOutput( complex< double > * bufPtr ) const
	{
        double * result = mTxInOut;
        if ( !mIsPO2 )
        {
            result = mTwiddle;
        }

		for ( FourierTransform::size_type k = 0; k < N; ++k )
		{
			*bufPtr = complex< double >( result[ 2*k ], result[ 2*k+1 ] );
			++bufPtr;
		}
	}
    
    // Compute a forward transform.
    void forward( void )
    {        
        if ( mIsPO2 )
        {
            cdft( 2*N, -1, mTxInOut, mWorkspace, mTwiddle );
        }
        else
        {
            slowDFT( mTxInOut, mTwiddle, N );
        }
    }
    
}; // end of class platform-neutral stand-alone FTimpl 

#endif

// --- FourierTransform members ---

// ---------------------------------------------------------------------------
//	FourierTransform constructor
// ---------------------------------------------------------------------------
//! Initialize a new FourierTransform of the specified size.
//!
//! \param  len is the length of the transform in samples (the
//!         number of samples in the transform)
//! \throw  RuntimeError if the necessary buffers cannot be 
//!         allocated, or there is an error configuring FFTW.
//
FourierTransform::FourierTransform( size_type len ) :
	_buffer( len ),
	_impl( new FTimpl( len ) )
{
	//	zero:
	std::fill( _buffer.begin(), _buffer.end(), 0. );
}

// ---------------------------------------------------------------------------
//	FourierTransform copy constructor
// ---------------------------------------------------------------------------
//! Initialize a new FourierTransform that is a copy of another,
//! having the same size and the same buffer contents.
//!
//! \param  rhs is the instance to copy
//! \throw  RuntimeError if the necessary buffers cannot be 
//!         allocated, or there is an error configuring FFTW.
//
FourierTransform::FourierTransform( const FourierTransform & rhs ) :
	_buffer( rhs._buffer ),
	_impl( new FTimpl( rhs._buffer.size() ) ) // not copied
{
}

// ---------------------------------------------------------------------------
//	FourierTransform destructor
// ---------------------------------------------------------------------------
//! Free the resources associated with this FourierTransform.
//
FourierTransform::~FourierTransform( void )
{	
   delete _impl;
}

// ---------------------------------------------------------------------------
//	FourierTransform assignment operator
// ---------------------------------------------------------------------------
//! Make this FourierTransform a copy of another, having
//! the same size and buffer contents.
//!
//! \param  rhs is the instance to copy
//! \return a refernce to this instance
//! \throw  RuntimeError if the necessary buffers cannot be 
//!         allocated, or there is an error configuring FFTW.
//
FourierTransform &
FourierTransform::operator=( const FourierTransform & rhs )
{
   if ( this != &rhs )
   {
      _buffer = rhs._buffer;
      
      // The implementation instance is not assigned, 
      // but a new one is created.
      delete _impl;
      _impl = 0;
      _impl = new FTimpl( _buffer.size() );
   }
   
   return *this;
}

// ---------------------------------------------------------------------------
//	size
// ---------------------------------------------------------------------------
//! Return the length of the transform (in samples).
//! 
//! \return the length of the transform in samples.
FourierTransform::size_type 
FourierTransform::size( void ) const 
{ 
   return _buffer.size(); 
}
	
// ---------------------------------------------------------------------------
//	transform
// ---------------------------------------------------------------------------
//! Compute the Fourier transform of the samples stored in the 
//! transform buffer. The samples stored in the transform buffer
//! (accessed by index or by iterator) are replaced by the 
//! transformed samples, in-place. 
//
void
FourierTransform::transform( void )
{
    // copy data into the transform input buffer:
    _impl->loadInput( &_buffer.front() );
    
    //	crunch:	
    _impl->forward();
    
    // copy the data out of the transform output buffer:
    _impl->copyOutput( &_buffer.front() );
}


// --- slow non-power-of-two DFT implementation ---

#if defined(SORRY_NO_FFTW) 

// ---------------------------------------------------------------------------
//	slowDFT
// ---------------------------------------------------------------------------
//  Non-power-of-two DFT implementation. in and out cannot be the same,
//  and each is 2*N long, storing interleaved complex numbers.
//  This version is only used when FFTW is unavailable.
//
static
void slowDFT( double * in, double * out, int N )
{
#if 1 
    // slow DFT 
    // This version of the direct DFT computation is tolerably
    // speedy, even for rather long transforms (like 8k).
    // There is only one expensive call to std::polar, and
    // twiddle factors are updated by multiplying. This
    // causes some loss in numerical accuracy, worse for
    // longer transforms, but even for 10k long transforms,
    // accuracy is within hundredths of a percent in my experiments.
    
    const std::complex< double > eminj2pioN = 
        std::polar( 1.0, -2.0 * Pi / N );
              
    std::complex< double > Wn = 1;
    for ( int n = 0; n < N; ++n )
    {
        std::complex< double > Wkn = 1;
        std::complex< double > Xkn = 0;
        for ( int k = 0; k < N; ++k )
        {
            Xkn += std::complex< double >( in[ 2*k ], in[ 2*k+1 ] ) * Wkn;
            Wkn *= Wn;
        }
        
        out[ 2*n ] = Xkn.real();
        out[ 2*n+1 ] = Xkn.imag();
        Wn *= eminj2pioN;
    }

#else 
    // very, very slow
    // This version of the direct DFT computation is slightly
    // more accurate, increasingly so for longer transforms, 
    // but it is so much slower (4-5x) that the small improvement
    // in accuracy is probably not worth the extra wait. For
    // short transforms, the accuracy of both algorithms is
    // very high, and for long transforms, this algorithm is
    // too slow.
    //
    // Both algorithms are retained here, so that this 
    // tradeoff can be re-evaluated if necessary.

    for ( int n = 0; n < N; ++n )
    {
        std::complex< double > Xkn = 0;
        for ( int k = 0; k < N; ++k )
        {
            std::complex< double > Wkn = std::polar( 1.0, -2.0 * Pi * k * n / N );
            Xkn += std::complex< double >( in[ 2*k ], in[ 2*k+1 ] ) * Wkn;
        }
        
        out[ 2*n ] = Xkn.real();
        out[ 2*n+1 ] = Xkn.imag();
    }
    
#endif    
}

#endif  //  defined(SORRY_NO_FFTW)

}	//	end of namespace Loris
