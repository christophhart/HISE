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
 * FrequencyReference.C
 *
 * Implementation of class FrequencyReference.
 *
 * Kelly Fitz, 3 Dec 2001
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */

#if HAVE_CONFIG_H
	#include "config.h"
#endif

#include "FrequencyReference.h"

#include "Breakpoint.h"
#include "Fundamental.h"
#include "LinearEnvelope.h"
#include "Notifier.h"
#include "Partial.h"
#include "PartialList.h"
#include "PartialUtils.h"

#include <algorithm>
#include <cmath>

//	begin namespace
namespace Loris {


// ---------------------------------------------------------------------------
//	createEstimator (static)
// ---------------------------------------------------------------------------
//  This class is now a wrapper providing the legacy interface to an improved
//  and more flexible fundamental frequency estimator. This function 
//  constructs and configures an instance of the new estimator that 
//  provides the functionality of the older (Loris 1.4 through 1.5.2) 
//  FrequencyReference class.
//

static const double Range = 50;
static const double Ceiling = 20000;
static const double Floor = -60;
static const double Precision = 0.1;
static const double Confidence = 0.9;
static const double Interval = 0.01;

FundamentalFromPartials createEstimator( void )
{
    FundamentalFromPartials eparts;
    
    eparts.setAmpFloor( Floor );
    eparts.setAmpRange( Range );
    eparts.setFreqCeiling( Ceiling );
    eparts.setPrecision( Precision );
    
    return eparts;
}

// ---------------------------------------------------------------------------
//	construction
// ---------------------------------------------------------------------------
//!	Construct a new fundamental FrequencyReference derived from the 
//!	specified half-open (STL-style) range of Partials that lies
//!	within the speficied average frequency range. Construct the 
//!	reference envelope with approximately numSamps points.
//!
//! \param	begin The beginning of a range of Partials from which to
//!			construct a frequency refence envelope.
//! \param	end The end of a range of Partials from which to
//!			construct a frequency refence envelope.
//!	\param	minFreq The minimum expected fundamental frequency.
//! \param	maxFreq The maximum expected fundamental frequency.
//! \param	numSamps The approximate number of estimate of the 
//!			fundamental frequency from which to construct the 
//!			frequency reference envelope.
//
FrequencyReference::FrequencyReference( PartialList::const_iterator begin, 
										PartialList::const_iterator end, 
										double minFreq, double maxFreq,
										long numSamps ) :
	_env( new LinearEnvelope() )
{
	if ( numSamps < 1 )
	{
		Throw( InvalidArgument, "A frequency reference envelope must have a positive number of samples." );
    }
    
	//	sanity:
	if ( maxFreq < minFreq )
	{
		std::swap( minFreq, maxFreq );
    }
    
#ifdef Loris_Debug
	debugger << "Finding frequency reference envelope in range " <<
	debugger << minFreq << " to " << maxFreq << " Hz, from " <<
	debugger << std::distance(begin,end) << " Partials" << std::endl;
#endif

	
	FundamentalFromPartials est = createEstimator();
	std::pair< double, double > span = PartialUtils::timeSpan( begin, end );
	double dt = ( span.second - span.first ) / ( numSamps + 1 );
	*_env = est.buildEnvelope( begin, end, 
                               span.first, span.second, dt,
                               minFreq, maxFreq,
                               Confidence );
}

// ---------------------------------------------------------------------------
//	construction
// ---------------------------------------------------------------------------
//!	Construct a new fundamental FrequencyReference derived from the 
//!	specified half-open (STL-style) range of Partials that lies
//!	within the speficied average frequency range. Construct the 
//!	reference envelope from fundamental estimates taken every
//! five milliseconds.
//!
//! \param	begin The beginning of a range of Partials from which to
//!			construct a frequency refence envelope.
//! \param	end The end of a range of Partials from which to
//!			construct a frequency refence envelope.
//!	\param	minFreq The minimum expected fundamental frequency.
//! \param	maxFreq The maximum expected fundamental frequency.
//
FrequencyReference::FrequencyReference( PartialList::const_iterator begin, 
										PartialList::const_iterator end, 
										double minFreq, double maxFreq ) :
	_env( new LinearEnvelope() )
{
	//	sanity:
	if ( maxFreq < minFreq )
	{
		std::swap( minFreq, maxFreq );
	}
	
#ifdef Loris_Debug
	debugger << "Finding frequency reference envelope in range " <<
	debugger << minFreq << " to " << maxFreq << " Hz, from " <<
	debugger << std::distance(begin,end) << " Partials" << std::endl;
#endif
    
	FundamentalFromPartials est = createEstimator();
	std::pair< double, double > span = PartialUtils::timeSpan( begin, end );
	*_env = est.buildEnvelope( begin, end, 
                               span.first, span.second, Interval,
                               minFreq, maxFreq,
                               Confidence );
    
}

// ---------------------------------------------------------------------------
//	copy construction
// ---------------------------------------------------------------------------
//
FrequencyReference::FrequencyReference( const FrequencyReference & other ) :
	_env( other._env->clone() )
{
}

// ---------------------------------------------------------------------------
//	destruction
// ---------------------------------------------------------------------------
//
FrequencyReference::~FrequencyReference()
{
}

// ---------------------------------------------------------------------------
//	assignment
// ---------------------------------------------------------------------------
//
FrequencyReference &
FrequencyReference::operator = ( const FrequencyReference & rhs )
{
	if ( &rhs != this )
	{
		_env.reset( rhs._env->clone() );
	}
	return *this;
}

// ---------------------------------------------------------------------------
//	clone
// ---------------------------------------------------------------------------
//
FrequencyReference * 
FrequencyReference::clone( void ) const
{
	return new FrequencyReference( *this );
}

// ---------------------------------------------------------------------------
//	valueAt
// ---------------------------------------------------------------------------
//
double
FrequencyReference::valueAt( double x ) const
{
	return _env->valueAt(x);
}

// ---------------------------------------------------------------------------
//	envelope
// ---------------------------------------------------------------------------
//	Conversion to LinearEnvelope return a BreakpointEnvelope that 
//	evaluates indentically to this FrequencyReference at all time.
//
LinearEnvelope 
FrequencyReference::envelope( void ) const 
{ 
    return *_env; 
}

// ---------------------------------------------------------------------------
//	timeOfPeakEnergy (static helper function)
// ---------------------------------------------------------------------------
//	Return the time at which the given Partial attains its
//	maximum sinusoidal energy.
//
static double timeOfPeakEnergy( const Partial & p )
{
	Partial::const_iterator partialIter = p.begin();
	double maxAmp = 
		partialIter->amplitude() * std::sqrt( 1. - partialIter->bandwidth() );
	double time = partialIter.time();
	
	for ( ++partialIter; partialIter != p.end(); ++partialIter ) 
	{
		double a = partialIter->amplitude() * 
					std::sqrt( 1. - partialIter->bandwidth() );
		if ( a > maxAmp ) 
		{
			maxAmp = a;
			time = partialIter.time();
		}
	}			
	
	return time;
}
// ---------------------------------------------------------------------------
//	IsInFrequencyRange
// ---------------------------------------------------------------------------
//	Function object for finding Partials that attain their maximum
//	sinusoidal energy at a frequency within a specified range.
//
struct IsInFrequencyRange
{
	double minFreq, maxFreq;
	IsInFrequencyRange( double min, double max ) :
		minFreq( min ),
		maxFreq( max )
	{
		//	sanity:
		if ( maxFreq < minFreq )
			std::swap( minFreq, maxFreq );
	}
	
	bool operator() ( const Partial & p )
	{
		double compareFreq = p.frequencyAt( timeOfPeakEnergy( p ) );
		return compareFreq >= minFreq  && compareFreq <= maxFreq;
	}
};

				   

}	//	end of namespace Loris
