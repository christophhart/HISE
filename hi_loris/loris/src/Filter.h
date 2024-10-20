#ifndef INCLUDE_FILTER_H
#define INCLUDE_FILTER_H
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
 * Filter.h
 *
 * Definition of class Loris::Filter, a generic digital filter of 
 * arbitrary order having both feed-forward and feedback coefficients. 
 *
 * Kelly Fitz, 1 Sept 1999
 * revised 9 Oct 2009
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */

#include "LorisExceptions.h"
#include "Notifier.h"

#include <algorithm>
#include <functional>
#include <deque>
#include <vector>

//  begin namespace
namespace Loris {

// ---------------------------------------------------------------------------
//  class Filter
//
//! Filter is an Direct Form II realization of a filter specified
//! by its difference equation coefficients and (optionally) gain,  
//! applied to the filter output (defaults to 1.). Coefficients are
//! specified and stored in order of increasing delay.
//!
//! Implements the rational transfer function
//!
//!                               -1               -nb
//!                   b[0] + b[1]z  + ... + b[nb] z
//!         Y(z) = G ---------------------------------- X(z)
//!                               -1               -na
//!                   a[0] + a[1]z  + ... + a[na] z
//!
//! where b[k] are the feed forward coefficients, and a[k] are the feedback 
//! coefficients. If a[0] is not 1, then both a and b are normalized by a[0].
//! G is the additional filter gain, and is unity if unspecified.
//!
//!
//! Filter is implemented using a std::deque to store the filter state, 
//! and relies on the efficiency of that class. If deque is not implemented
//! using some sort of circular buffer (as it should be -- deque is guaranteed
//! to be efficient for repeated insertion and removal at both ends), then
//! this filter class will be slow.
//
class Filter
{
public:

//  --- lifecycle ---

    //  default construction
    //! Construct a filter with an all-pass unity gain response.    
    Filter( void );
    
    //  initialized construction
    //! Initialize a Filter having the specified coefficients, and
    //! order equal to the larger of the two coefficient ranges.
    //! Coefficients in the sequences are stored in increasing order
    //! (lowest order coefficient first).
    //!
    //! If template members are allowed, then the coefficients
    //! can be stored in any kind of iterator range, otherwise,
    //! they must be in an array of doubles.
    //!
    //! \param ffwdbegin is the beginning of a sequence of feed-forward coefficients
    //! \param ffwdend is the end of a sequence of feed-forward coefficients
    //! \param fbackbegin is the beginning of a sequence of feedback coefficients
    //! \param fbackend is the end of a sequence of feedback coefficients   
    //! \param gain is an optional gain scale applied to the filtered signal
    //
#if !defined(NO_TEMPLATE_MEMBERS)
    template<typename IterT1, typename IterT2>
    Filter( IterT1 ffwdbegin, IterT1 ffwdend,   //  feed-forward coeffs
            IterT2 fbackbegin, IterT2 fbackend, //  feedback coeffs
            double gain = 1. );
#else
    Filter( const double * ffwdbegin, const double * ffwdend, //    feed-forward coeffs
            const double * fbackbegin, const double * fbackend, //  feedback coeffs
            double gain = 1. );
#endif

    
    // copy constructor 
    //! Make a copy of another digital filter. 
    //! Do not copy the filter state (delay line).
    Filter( const Filter & other );
    
    //  assignment operator
    //! Make a copy of another digital filter. 
    //! Do not copy the filter state (delay line).
    Filter & operator=( const Filter & rhs );
    
    //! Destructor is virtual to enable subclassing. Subclasses may specialize
    //! construction, and may add functionality, but for efficiency, the filtering
    //! operation is non-virtual.
    ~Filter( void );
    

//  --- filtering ---

    //! Compute a filtered sample from the next input sample.
    //!
    //!	\param input is the next input sample
    //! \return the next output sample
    double apply( double input );

    //! Function call operator, same as sample().
    //!
    //! \sa apply
    double operator() ( double input ) { return apply(input); }    

//  --- access/mutation ---

	//!	Provide access to the numerator (feed-forward) coefficients
	//! of this filter. The coefficients are stored in order of increasing
	//! delay (lowest order coefficient first).
	
	std::vector< double > numerator( void );

	//!	Provide access to the numerator (feed-forward) coefficients
	//! of this filter. The coefficients are stored in order of increasing
	//! delay (lowest order coefficient first).
	
	const std::vector< double > numerator( void ) const;
    
	//!	Provide access to the denominator (feedback) coefficients
	//! of this filter. The coefficients are stored in order of increasing
	//! delay (lowest order coefficient first).
	
	std::vector< double > denominator( void );

	//!	Provide access to the denominator (feedback) coefficients
	//! of this filter. The coefficients are stored in order of increasing
	//! delay (lowest order coefficient first).
	
	const std::vector< double > denominator( void ) const;
	
    
    //! Clear the filter state. 
    void clear( void );

    
private:    
    
//  --- implementation ---

    //! single delay line for Direct-Form II implementation
    std::deque< double > m_delayline;
        
    //! feed-forward coefficients
    std::vector< double > m_ffwdcoefs;  

    //! feedback coefficients
    std::vector< double > m_fbackcoefs; 
    
    //! filter gain (applied to output)
    double m_gain;      

};  //  end of class Filter



// ---------------------------------------------------------------------------
//  constructor
// ---------------------------------------------------------------------------
//! Initialize a Filter having the specified coefficients, and
//! order equal to the larger of the two coefficient ranges.
//! Coefficients in the sequences are stored in increasing order
//! (lowest order coefficient first).
//!
//! If template members are allowed, then the coefficients
//! can be stored in any kind of iterator range, otherwise,
//! they must be in an array of doubles.
//!
//! \param ffwdbegin is the beginning of a sequence of feed-forward coefficients
//! \param ffwdend is the end of a sequence of feed-forward coefficients
//! \param fbackbegin is the beginning of a sequence of feedback coefficients
//! \param fbackend is the end of a sequence of feedback coefficients   
//! \param gain is an optional gain scale applied to the filtered signal
//
#if !defined(NO_TEMPLATE_MEMBERS)
template<typename IterT1, typename IterT2>
Filter::Filter( IterT1 ffwdbegin, IterT1 ffwdend,   //  feed-forward coeffs
                IterT2 fbackbegin, IterT2 fbackend, //  feedback coeffs
                double gain ) :
#else
inline 
Filter::Filter( const double * ffwdbegin, const double * ffwdend, //    feed-forward coeffs
                const double * fbackbegin, const double * fbackend, //  feedback coeffs
                double gain ) :
#endif
    m_ffwdcoefs( ffwdbegin, ffwdend ),
    m_fbackcoefs( fbackbegin, fbackend ),
    m_delayline( std::max( ffwdend-ffwdbegin, fbackend-fbackbegin ) - 1, 0. ),
    m_gain( gain )
{
    if ( *fbackbegin == 0. )
    {
        Throw( InvalidObject, 
               "Tried to create a Filter with feeback coefficient at zero delay equal to 0.0" );
    }

    //  normalize the coefficients by 1/a[0], if a[0] is not equal to 1.0
    //  (already checked for a[0] == 0 above)
    if ( *fbackbegin != 1. )
    {
        for(auto& v: m_ffwdcoefs)
            v /= *fbackbegin;

        for(auto& v: m_fbackcoefs)
            v /= *fbackbegin;

#if 0
        //  scale all filter coefficients by a[0]:
        std::transform( m_ffwdcoefs.begin(), m_ffwdcoefs.end(), m_ffwdcoefs.begin(),
                        std::bind( std::divides<double>(), *fbackbegin ) );
        std::transform( m_fbackcoefs.begin(), m_fbackcoefs.end(), m_fbackcoefs.begin(), 
                        std::bind( std::divides<double>(), *fbackbegin ) );
#endif

        m_fbackcoefs[0] = 1.;
    }
}


}   //  end of namespace Loris

#endif /* ndef INCLUDE_FILTER_H */
