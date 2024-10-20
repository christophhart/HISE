#ifndef INCLUDE_FREQUENCYREFERENCE_H
#define INCLUDE_FREQUENCYREFERENCE_H
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
 * FrequencyReference.h
 *
 * Definition of class FrequencyReference.
 *
 * Kelly Fitz, 3 Dec 2001
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */

#include "Envelope.h"
#include "PartialList.h"
#include <memory>

//	begin namespace
namespace Loris {

class LinearEnvelope;

// ---------------------------------------------------------------------------
//	class FrequencyReference
//
//!	Class FrequencyReference represents a reference frequency envelope
//!	derived from an estimate of the fundamental frequency of a given range 
//! of Partials within in a specified frequency range. This reference envelope
//!	can be used for channelizing the Partials in preparation for morphing
//!	(see Channelizer.h).
//!	
//!	FrequencyReference implements the Envelope interface (see
//!	Envelope.h).
//
class FrequencyReference : public Envelope
{
//	-- instance variables --
	std::auto_ptr< LinearEnvelope > _env;
	
//	-- public interface --
public:
//	-- construction --

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
	FrequencyReference( PartialList::const_iterator begin, 
						PartialList::const_iterator end, 
						double minFreq, double maxFreq, long numSamps );
	 
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
	FrequencyReference( PartialList::const_iterator begin, 
						PartialList::const_iterator end, 
						double minFreq, double maxFreq );

	 
	//!	Construct a new FrequencyReference that is an exact copy of the
	//!	specified FrequencyReference.
	FrequencyReference( const FrequencyReference & other );
	 
	//!	Assignment operator: make this FrequencyReference an exact copy 
	//! of the specified FrequencyReference.
	FrequencyReference & operator= ( const FrequencyReference & other );
	 
	//! Destroy this FrequencyReference.
	~FrequencyReference();
	 
//	-- conversion to LinearEnvelope --

    //!	Return a LinearEnvelope that evaluates indentically to this
	//!	FrequencyReference at all time.
	LinearEnvelope envelope( void ) const;
     
//	-- Envelope interface --

	//!	Return an exact copy of this FrequencyReference (following the
	//!	Prototype pattern).
	virtual FrequencyReference * clone( void ) const;
	
	//!	Return the frequency value (in Hz) of this FrequencyReference at the
	//!	specified time.
	virtual double valueAt( double x ) const;	

};	// end of class FrequencyReference

}	//	end of namespace Loris

#endif	// ndef INCLUDE_FREQUENCYREFERENCE_H
