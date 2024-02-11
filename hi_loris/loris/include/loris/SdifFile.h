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
 * SdifFile.h
 *
 * Definition of SdifFile class for Partial import and export in Loris.
 *
 * Kelly Fitz, 8 Jan 2003 
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */

#include "Marker.h"
#include "Partial.h"
#include "PartialList.h"
 
#include <string>
#include <vector>

//	begin namespace
namespace Loris {

// ---------------------------------------------------------------------------
//	class SdifFile
//
//!	Class SdifFile represents reassigned bandwidth-enhanced Partial 
//!	data in a SDIF-format data file. Construction of an SdifFile 
//!	from a stream or filename automatically imports the Partial
//!	data. 
//!
//!	Loris stores partials in SDIF RBEP and RBEL frames. The RBEP and RBEL
//!	frame and matrix definitions are included in the SDIF file's header.
//!	Each RBEP frame contains one RBEP matrix, and each row in a RBEP matrix
//!	describes one breakpoint in a Loris partial. The data in RBEP matrices
//!	are SDIF 32-bit floats.
//!	
//!	The six columns in an RBEP matrix are: partialIndex, frequency,
//!	amplitude, phase, noise, timeOffset. The partialIndex uniquely
//!	identifies a partial. When Loris exports SDIF data, each partial is
//!	assigned a unique partialIndex. The frequency (Hz), amplitude (0..1),
//!	phase (radians), and noise (bandwidth) are encoded the same as Loris
//!	breakpoints. The timeOffset is an offset from the RBEP frame time,
//!	specifying the exact time of the breakpoint. Loris always specifies
//!	positive timeOffsets, and the breakpoint's exact time is always be
//!	earlier than the next RBEP frame's time.
//!	
//!	Since reassigned bandwidth-enhanced partial breakpoints are
//!	non-uniformly spaced in time, the RBEP frame times are also
//!	non-uniformly spaced. Each RBEP frame will contain at most one
//!	breakpoint for any given partial. A partial may extend over a RBEP frame
//!	and have no breakpoint specified by the RBEP frame, as happens when one
//!	active partial has a lower temporal density of breakpoints than other
//!	active partials.
//!	
//!	If partials have nonzero labels in Loris, then a RBEL frame describing
//!	the labeling of the partials will precede the first RBEP frame in the
//!	SDIF file. The RBEL frame contains a single, two-column RBEL matrix The
//!	first column is the partialIndex, and the second column specifies the
//!	label for the partial.
//!
//!	If markers are associated with the partials in Loris, then a RBEM frame
//!	describing the markers will precede the first RBEP frame in the SDIF
//!	file. The RBEM frame contains two single-column RBEM matrices. The
//!	first matrix contains 32-bit floats indicating the time (in seconds)
//!	for each marker. The second matrix contains UTF-8 data, the names of
//!	each of the markers separated by the ASCII character 0.
//!	
//!	In addition to RBEP frames, Loris can also read and write SDIF 1TRC
//!	frames (refer to IRCAM's SDIF web site, www.ircam.fr/sdif/, for
//!	definitions of standard SDIF description types). Since 1TRC frames do
//!	not represent bandwidth-enhancement or the exact timing of Loris
//!	breakpoints, their use is not recommended. 1TRC capabilities are
//!	provided in Loris to allow interchange with programs that are unable to
//!	interpret RBEP frames.
//
class SdifFile
{
//	-- public interface --
public:

//	-- types --

	//! The type of marker storage in an SdifFile.
	typedef std::vector< Marker > markers_type;

	//!	The type of the Partial storage in an AiffFile.
	typedef PartialList partials_type;
	
//	-- construction --

    //! Initialize an instance of SdifFile by importing Partial data from
    //! the file having the specified filename or path.
 	explicit SdifFile( const std::string & filename );
 
    //! Initialize an instance of SdifFile with copies of the Partials
    //! on the specified half-open (STL-style) range.
    //! 
    //! If compiled with NO_TEMPLATE_MEMBERS defined, this member accepts
    //! only PartialList::const_iterator arguments.
#if !defined(NO_TEMPLATE_MEMBERS)
	template<typename Iter>
	SdifFile( Iter begin_partials, Iter end_partials  );
#else
	SdifFile( PartialList::const_iterator begin_partials, 
			  PartialList::const_iterator end_partials  );
#endif
 
    //! Initialize an empty instance of SdifFile having no Partials.
	SdifFile( void );
	
	//	copy, assign, and delete are compiler-generated
	
//	-- access --

    //! Return a reference to the Markers (see Marker.h) 
    //! for this SdifFile. 
	markers_type & markers( void );

    //! Return a reference to the Markers (see Marker.h) 
    //! for this SdifFile. 
	const markers_type & markers( void ) const;
	 
    //!	Return a reference to the bandwidth-enhanced
    //! Partials represented by this SdifFile.
	partials_type & partials( void );

    //!	Return a reference to the bandwidth-enhanced
    //! Partials represented by this SdifFile.
	const partials_type & partials( void ) const;
//	-- mutation --

	//! Add a copy of the specified Partial to this SdifFile.
	void addPartial( const Loris::Partial & p );
	 
    //! Add a copy of each Partial on the specified half-open (STL-style) 
    //! range to this SdifFile.
    //! 
    //! If compiled with NO_TEMPLATE_MEMBERS defined, this member accepts
    //! only PartialList::const_iterator arguments.
#if !defined(NO_TEMPLATE_MEMBERS)
	template<typename Iter>
	void addPartials( Iter begin_partials, Iter end_partials  );
#else
	void addPartials( PartialList::const_iterator begin_partials, 
					  PartialList::const_iterator end_partials  );
#endif
	 
//	-- export --

    //! Export the envelope Partials represented by this SdifFile to
    //! the file having the specified filename or path.
	void write( const std::string & path );

    //! Export the envelope Partials represented by this SdifFile to
    //! the file having the specified filename or path in the 1TRC
    //! format, resampled, and without phase or bandwidth information.
	void write1TRC( const std::string & path );
	
//	-- legacy export --

    //! Export the Partials in the specified PartialList to a SDIF file having
    //! the specified file name or path. If enhanced is true (the default),
    //! reassigned bandwidth-enhanced Partial data are exported in the
    //! six-column RBEP format. Otherwise, the Partial data is exported as
    //! resampled sinusoidal analysis data in the 1TRC format.
    //! Provided for backwards compatability.
    //! 
    //! \deprecated This function is included only for legacy support
    //!             and may be removed at any time.
	static void Export( const std::string & filename, 
						const PartialList & plist, 
						const bool enhanced = true );

private:
//	-- implementation --
	partials_type partials_;		//	Partials to store in SDIF format
	markers_type markers_;		// 	AIFF Markers
	
};	//	end of class SdifFile

// -- template members --

// ---------------------------------------------------------------------------
//	constructor from Partial range
// ---------------------------------------------------------------------------
//	Initialize an instance of SdifFile with copies of the Partials
//	on the specified half-open (STL-style) range.
//
//	If compiled with NO_TEMPLATE_MEMBERS defined, this member accepts
//	only PartialList::const_iterator arguments.
//
#if !defined(NO_TEMPLATE_MEMBERS)
template< typename Iter >
SdifFile::SdifFile( Iter begin_partials, Iter end_partials  )
#else
SdifFile::SdifFile( PartialList::const_iterator begin_partials, 
					PartialList::const_iterator end_partials )
#endif
{
	addPartials( begin_partials, end_partials );
}

// ---------------------------------------------------------------------------
//	addPartials 
// ---------------------------------------------------------------------------
//	Add a copy of each Partial on the specified half-open (STL-style) 
//	range to this SdifFile.
//	
//	If compiled with NO_TEMPLATE_MEMBERS defined, this member accepts
//	only PartialList::const_iterator arguments.
//
#if !defined(NO_TEMPLATE_MEMBERS)
template<typename Iter>
void SdifFile::addPartials( Iter begin_partials, Iter end_partials  )
#else
void SdifFile::addPartials( PartialList::const_iterator begin_partials, 
							PartialList::const_iterator end_partials  )
#endif
{
	partials_.insert( partials_.end(), begin_partials, end_partials );
}

}	//	end of namespace Loris

