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
 * Filter.C
 *
 * Implementation of class Loris::Filter, a generic digital filter of 
 * arbitrary order having both feed-forward and feedback coefficients. 
 *
 * Kelly Fitz, 1 Sept 1999
 * revised 9 Oct 2009
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */

#if HAVE_CONFIG_H
    #include "config.h"
#endif

#include "Filter.h"

#include <algorithm>
#include <numeric>

//  begin namespace
namespace Loris {

// ---------------------------------------------------------------------------
//  default construction
// ---------------------------------------------------------------------------
//! Construct a filter with an all-pass unity gain response.    
//
Filter::Filter( void ) :
    m_ffwdcoefs( 1, 1.0 ),
    m_fbackcoefs( 1, 1.0 ),
    m_delayline( 1, 0 ),
    m_gain( 1.0 )
{
}

// ---------------------------------------------------------------------------
//  copy construction
// ---------------------------------------------------------------------------
//! Make a copy of another digital filter. 
//! Do not copy the filter state (delay line).
//
Filter::Filter( const Filter & other ) :
    m_delayline( other.m_delayline.size(), 0. ),
    m_ffwdcoefs( other.m_ffwdcoefs ),
    m_fbackcoefs( other.m_fbackcoefs ),
    m_gain( other.m_gain )
{
    Assert( m_delayline.size() >= m_ffwdcoefs.size() - 1 );
    Assert( m_delayline.size() >= m_fbackcoefs.size() - 1 );
}

// ---------------------------------------------------------------------------
//  assignment
// ---------------------------------------------------------------------------
//! Make a copy of another digital filter. 
//! Do not copy the filter state (delay line).
//
Filter &
Filter::operator=( const Filter & rhs )
{
    if ( &rhs != this )
    {
        m_delayline.resize( rhs.m_delayline.size() );
        clear();
        
        m_ffwdcoefs = rhs.m_ffwdcoefs;
        m_fbackcoefs = rhs.m_fbackcoefs;
        m_gain = rhs.m_gain;

        Assert( m_delayline.size() >= m_ffwdcoefs.size() - 1 );
        Assert( m_delayline.size() >= m_fbackcoefs.size() - 1 );
    }
    return *this;
}

// ---------------------------------------------------------------------------
//  destructor
// ---------------------------------------------------------------------------
//! Destructor is virtual to enable subclassing. Subclasses may specialize
//! construction, and may add functionality, but for efficiency, the filtering
//! operation is non-virtual.
//
Filter::~Filter( void )
{
}


// ---------------------------------------------------------------------------
//  apply
// ---------------------------------------------------------------------------
//! Compute a filtered sample from the next input sample.
//!
//
double
Filter::apply( double input )
{ 
    // Implement the recurrence relation. m_ffwdcoefs holds the feed-forward
    // coefficients, m_fbackcoefs holds the feedback coeffs. The coefficient
    // vectors and delay lines are ordered by increasing age.

    double wn = - std::inner_product( m_fbackcoefs.begin()+1, m_fbackcoefs.end(), 
                                      m_delayline.begin(), -input );
        //  negate input, then negate the inner product
        
    m_delayline.push_front( wn );
    
    double output = std::inner_product( m_ffwdcoefs.begin(), m_ffwdcoefs.end(), 
                                        m_delayline.begin(), 0. );
    m_delayline.pop_back();
        
    return output * m_gain;
}

//  --- access/mutation ---

// ---------------------------------------------------------------------------
//  numerator
// ---------------------------------------------------------------------------
//! Provide access to the numerator (feed-forward) coefficients
//! of this filter. The coefficients are stored in order of increasing
//! delay (lowest order coefficient first).

std::vector< double > 
Filter::numerator( void )
{
    return m_ffwdcoefs;
}

// ---------------------------------------------------------------------------
//  numerator
// ---------------------------------------------------------------------------
//! Provide access to the numerator (feed-forward) coefficients
//! of this filter. The coefficients are stored in order of increasing
//! delay (lowest order coefficient first).

const std::vector< double > 
Filter::numerator( void ) const
{
    return m_ffwdcoefs;
}

// ---------------------------------------------------------------------------
//  denominator
// ---------------------------------------------------------------------------
//! Provide access to the denominator (feedback) coefficients
//! of this filter. The coefficients are stored in order of increasing
//! delay (lowest order coefficient first).

std::vector< double > 
Filter::denominator( void )
{
    return m_fbackcoefs;
}

// ---------------------------------------------------------------------------
//  denominator
// ---------------------------------------------------------------------------
//! Provide access to the denominator (feedback) coefficients
//! of this filter. The coefficients are stored in order of increasing
//! delay (lowest order coefficient first).

const std::vector< double > 
Filter::denominator( void ) const
{
    return m_fbackcoefs;
}

// ---------------------------------------------------------------------------
//  clear
// ---------------------------------------------------------------------------
//! Clear the filter state. 
//
void
Filter::clear( void )
{
    std::fill( m_delayline.begin(), m_delayline.end(), 0 );
}

}   //  end of namespace Loris
