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
 * SpcFile.h
 *
 * Definition of SpcFile class for Partial import and export for
 * real-time synthesis in Kyma.
 *
 * Spc files always represent a number of Partials that is a power of
 * two. This is not necessary for purely-sinusoidal files, but might be
 * (not clear) for enhanced data to be properly processed in Kyma. 
 *
 * All of this is kind of disgusting right now. This code has evolved 
 * somewhat randomly, and we are awaiting full support for bandwidth-
 * enhanced data in Kyma..
 *
 * Kelly Fitz, 8 Jan 2003 
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */
#include "Marker.h"
#include "Partial.h"

#if defined(NO_TEMPLATE_MEMBERS)
#include "PartialList.h"
#endif
 
#include <string>
#include <vector>

//	begin namespace
namespace Loris {

// ---------------------------------------------------------------------------
//	class SpcFile
//
//!	Class SpcFile represents a collection of reassigned bandwidth-enhanced
//!	Partial data in a SPC-format envelope stream data file, used by the
//!	real-time bandwidth-enhanced additive synthesizer implemented on the
//!	Symbolic Sound Kyma Sound Design Workstation. Class SpcFile manages 
//!	file I/O and conversion between Partials and envelope parameter streams.
//	
class SpcFile
{
//	-- public interface --
public:

//	-- types --

	//! the type of the container of Markers
	//! stored with an SpcFile
	typedef std::vector< Marker > markers_type;		
	
	//! the type of the container of Partials
	//! stored with an SpcFile
	typedef std::vector< Partial > partials_type;	

//	-- construction --

	//!	Initialize an instance of SpcFile by importing envelope parameter 
	//!	streams from the file having the specified filename or path.
	//!
	//! \param	filename the name of the file to import
 	explicit SpcFile( const std::string & filename );

	//!	Initialize an instance of SpcFile with copies of the Partials
	//!	on the specified half-open (STL-style) range.
	//!
	//!	If compiled with NO_TEMPLATE_MEMBERS defined, this member accepts
	//!	only PartialList::const_iterator arguments.
	//!
	//! \param	begin_partials the beginning of a range of Partials to prepare
	//!			for Spc export
	//! \param	end_partials the end of a range of Partials to prepare
	//!			for Spc export
	//!	\param	midiNoteNum the fractional MIDI note number, if specified
	//!			(default is 60)
#if !defined(NO_TEMPLATE_MEMBERS)
	template<typename Iter>
	SpcFile( Iter begin_partials, Iter end_partials, double midiNoteNum = 60  );
#else
	SpcFile( PartialList::const_iterator begin_partials, 
			 PartialList::const_iterator end_partials,
			 double midiNoteNum = 60  );
#endif

	//!	Initialize an instance of SpcFile having the specified fractional
	//!	MIDI note number, and no Partials (or envelope parameter streams). 
	//!
	//!	\param	midiNoteNum the fractional MIDI note number, if specified
	//!			(default is 60)
	explicit SpcFile( double midiNoteNum = 60 );
	 
	//	copy, assign, and delete are compiler-generated
	
//	-- access --

	//!	Return a reference to the MarkerContainer (see Marker.h) for this SpcFile. 
	markers_type & markers( void );
	
	//!	Return a reference to the MarkerContainer (see Marker.h) for this SpcFile. 
	const markers_type & markers( void ) const;
	 
	//!	Return the fractional MIDI note number assigned to this SpcFile. 
	//!	If the sound has no definable pitch, note number 60.0 is used.
	double midiNoteNumber( void ) const;
	
	//!	Return a read-only (const) reference to the bandwidth-enhanced
	//! Partials represented by the envelope parameter streams in this SpcFile.
	const partials_type & partials( void ) const;
	
	//!	Return the sampling freqency in Hz for the spc data in this
	//!	SpcFile. This is the rate at which Kyma must be running to ensure
	//!	proper playback of bandwidth-enhanced Spc data.
 	double sampleRate( void ) const;
	
//	-- mutation --

	//!	Add the specified Partial to the enevelope parameter streams
	//!	represented by this SpcFile. 
	//!	
	//!	A SpcFile can contain only one Partial having any given (non-zero) 
	//!	label, so an added Partial will replace a Partial having the 
	//!	same label, if such a Partial exists.
	//!
	//!	This may throw an InvalidArgument exception if an attempt is made
	//!	to add unlabeled Partials, or Partials labeled higher than the
	//!	allowable maximum.
	//!
	//!	\param	p the Partial to add to this SpcFile
	void addPartial( const Loris::Partial & p );
	 
	//!	Add a Partial, assigning it the specified label (and position in the
	//!	Spc data).
	//!	
	//!	A SpcFile can contain only one Partial having any given (non-zero) 
	//!	label, so an added Partial will replace a Partial having the 
	//!	same label, if such a Partial exists.
	//!
	//!	This may throw an InvalidArgument exception if an attempt is made
	//!	to add unlabeled Partials, or Partials labeled higher than the
	//!	allowable maximum.
	//!
	//!	\param	p the Partial to add to this SpcFile
	//! \param	label the label to associate with this Partial in 
	//!			the Spc file (the Partial's own label is ignored).
	void addPartial( const Loris::Partial & p, int label );
	 
	//!	Add all Partials on the specified half-open (STL-style) range
	//!	to the enevelope parameter streams represented by this SpcFile. 
	//!	
	//!	A SpcFile can contain only one Partial having any given (non-zero) 
	//!	label, so an added Partial will replace a Partial having the 
	//!	same label, if such a Partial exists.
	//!
	//!	If compiled with NO_TEMPLATE_MEMBERS defined, this member accepts
	//!	only PartialList::const_iterator arguments.
	//!
	//!	This may throw an InvalidArgument exception if an attempt is made
	//!	to add unlabeled Partials, or Partials labeled higher than the
	//!	allowable maximum.
	//!
	//! \param	begin_partials the beginning of a range of Partials
	//!			to add to this Spc file
	//! \param	end_partials the end of a range of Partials
	//!			to add to this Spc file
#if !defined(NO_TEMPLATE_MEMBERS)
	template<typename Iter>
	void addPartials( Iter begin_partials, Iter end_partials  );
#else
	void addPartials( PartialList::const_iterator begin_partials, 
					  PartialList::const_iterator end_partials  );
#endif

	//!	Set the fractional MIDI note number assigned to this SpcFile. 
	//!	If the sound has no definable pitch, use note number 60.0 
	//! (the default).
	void setMidiNoteNumber( double nn );

	//!	Set the sampling freqency in Hz for the spc data in this
	//!	SpcFile. This is the rate at which Kyma must be running to ensure
	//!	proper playback of bandwidth-enhanced Spc data.
	//!	
	//!	The default sample rate is 44100 Hz.
	void setSampleRate( double rate );
	 	
//	-- export --

	//!	Export the phase-correct bandwidth-enhanced envelope parameter 
	//!	streams represented by this SpcFile to the file having the specified 
	//!	filename or path. 
	//!	
	//!	A nonzero endApproachTime indicates that the Partials do not include a
	//!	release or decay, but rather end in a static spectrum corresponding to the
	//!	final Breakpoint values of the partials. The endApproachTime specifies how
	//!	long before the end of the sound the amplitude, frequency, and bandwidth
	//!	values are to be modified to make a gradual transition to the static spectrum.
	//!	
	//!	If the endApproachTime is not specified, it is assumed to be zero, 
	//!	corresponding to Partials that decay or release normally.
	//!
	//! \param	filename the name of the file to create
	//! \param	endApproachTime the duration in seconds of the gradual transition
	//!			to a static spectrum at the end of the sound (default 0)
	void write( const std::string & filename, double endApproachTime = 0 );

	//!	Export the pure sinsoidal (omitting phase and bandwidth data) envelope 
	//!	parameter streams represented by this SpcFile to the file having the 
	//!	specified filename or path. 
	//!	
	//!	A nonzero endApproachTime indicates that the Partials do not include a
	//!	release or decay, but rather end in a static spectrum corresponding to the
	//!	final Breakpoint values of the partials. The endApproachTime specifies how
	//!	long before the end of the sound the amplitude, frequency, and bandwidth
	//!	values are to be modified to make a gradual transition to the static spectrum.
	//!	
	//!	If the endApproachTime is not specified, it is assumed to be zero, 
	//!	corresponding to Partials that decay or release normally.
	//!
	//! \param	filename the name of the file to create
	//! \param	endApproachTime the duration in seconds of the gradual transition
	//!			to a static spectrum at the end of the sound (default 0)
	void writeSinusoidal( const std::string & filename, double endApproachTime = 0 );

	//!	Export the envelope parameter streams represented by this SpcFile to
	//!	the file having the specified filename or path. Export phase-correct 
	//!	bandwidth-enhanced envelope parameter streams if enhanced is true 
	//!	(the default), or pure sinsoidal streams otherwise.
	//!
	//!	A nonzero endApproachTime indicates that the Partials do not include a
	//!	release or decay, but rather end in a static spectrum corresponding to the
	//!	final Breakpoint values of the partials. The endApproachTime specifies how
	//!	long before the end of the sound the amplitude, frequency, and bandwidth
	//!	values are to be modified to make a gradual transition to the static spectrum.
	//!	
	//!	If the endApproachTime is not specified, it is assumed to be zero, 
	//!	corresponding to Partials that decay or release normally.
	//!	
	//!	\deprecated This version of write is deprecated, use the two-argument
	//!	versions write and writeSinusoidal. 
	//!
	//! \param	filename the name of the file to create
	//! \param	enhanced flag indicating whether to export enhanced (true)
	//!			or sinusoidal (false) data
	//! \param	endApproachTime the duration in seconds of the gradual transition
	//!			to a static spectrum at the end of the sound (default 0)
	void write( const std::string & filename, bool enhanced,
				double endApproachTime = 0 );

private:
//	-- implementation --
	partials_type partials_;	//!	Partials to store in Spc format
	markers_type markers_;		//!	AIFF Markers

	double notenum_;			//! fractional MIDI note number
	double rate_;				//! sample rate in Hz at which the data plays at the
								//! correction default pitch
	
	static const int MinNumPartials;	//!	the minimum number of Partials to export (32)
										//!	(if necessary, extra empty (silent) Partials
										//! will be exported to make up the difference between
										//!	the size of partials_ and the next larger power
										//! of two, not less than MinNumPartials
	static const double DefaultRate;	//!	the default Spc export sample rate in Hz (44kHz)

//	-- helpers --
	void readSpcData( const std::string & filename );
	void growPartials( partials_type::size_type sz );
	
};	//	end of class SpcFile


// ---------------------------------------------------------------------------
//	constructor from Partial range
// ---------------------------------------------------------------------------
//!	Initialize an instance of SpcFile with copies of the Partials
//!	on the specified half-open (STL-style) range.
//!
//!	If compiled with NO_TEMPLATE_MEMBERS defined, this member accepts
//!	only PartialList::const_iterator arguments.
//!
//! \param	begin_partials the beginning of a range of Partials to prepare
//!			for Spc export
//! \param	end_partials the end of a range of Partials to prepare
//!			for Spc export
//!	\param	midiNoteNum the fractional MIDI note number, if specified
//!			(default is 60)
#if !defined(NO_TEMPLATE_MEMBERS)
template< typename Iter >
SpcFile::SpcFile( Iter begin_partials, Iter end_partials, double midiNoteNum  ) :
#else
SpcFile::SpcFile( PartialList::const_iterator begin_partials, 
				  PartialList::const_iterator end_partials,
				  double midiNoteNum ) :
#endif
//	initializers:
	notenum_( midiNoteNum ),
	rate_( DefaultRate )
{
	growPartials( MinNumPartials );
	addPartials( begin_partials, end_partials );
}

// ---------------------------------------------------------------------------
//	addPartials 
// ---------------------------------------------------------------------------
//!	Add all Partials on the specified half-open (STL-style) range
//!	to the enevelope parameter streams represented by this SpcFile. 
//!	
//!	A SpcFile can contain only one Partial having any given (non-zero) 
//!	label, so an added Partial will replace a Partial having the 
//!	same label, if such a Partial exists.
//!
//!	If compiled with NO_TEMPLATE_MEMBERS defined, this member accepts
//!	only PartialList::const_iterator arguments.
//!
//!	This may throw an InvalidArgument exception if an attempt is made
//!	to add unlabeled Partials, or Partials labeled higher than the
//!	allowable maximum.
//!
//! \param	begin_partials the beginning of a range of Partials
//!			to add to this Spc file
//! \param	end_partials the end of a range of Partials
//!			to add to this Spc file
#if !defined(NO_TEMPLATE_MEMBERS)
template<typename Iter>
void SpcFile::addPartials( Iter begin_partials, Iter end_partials  )
#else
void SpcFile::addPartials( PartialList::const_iterator begin_partials, 
						   PartialList::const_iterator end_partials  )
#endif
{
	while ( begin_partials != end_partials )
	{
	    // do not try to add unlabeled Partials, or 
	    // Partials labeled beyond 256:
	    if ( 0 != begin_partials->label() && 257 > begin_partials->label() )
	    {
		    addPartial( *begin_partials );
		}
		++begin_partials;
	}
}

}	//	end of namespace Loris


