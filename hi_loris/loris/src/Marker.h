#ifndef INCLUDE_MARKER_H
#define INCLUDE_MARKER_H
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
 * Marker.h
 *
 * Definition of classes Marker and MarkerContainer representing labeled
 * time points or temporal features in imported and exported data. Used by 
 * file I/O classes AiffFile, SdifFile, and SpcFile.
 *
 * Kelly Fitz, 8 Jan 2003 
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */

#include <functional>
#include <string>
#include <vector>

//	begin namespace
namespace Loris {

// ---------------------------------------------------------------------------
//	class Marker
//
//!	Class Marker represents a labeled time point in a set of Partials
//!	or a vector of samples. Collections of Markers (see the MarkerContainer
//!	definition below) are held by the File I/O classes in Loris (AiffFile,
//!	SdifFile, and SpcFile) to identify temporal features in imported
//!	and exported data.
//
class Marker
{
//	-- public interface --
public:
//	-- construction --

	//!	Default constructor - initialize a Marker at time zero with no label.
	Marker( void );
	 
	//!	Initialize a Marker with the specified time (in seconds) and name.
	//! 
	//! \param  t is the time associated with the new Marker
	//! \param  s is the name associated with the new Marker
	Marker( double t, const std::string & s );
	 
	//!	Initialize a Marker that is an exact copy of another Marker, that is,
	//!	having the same time and name.
	//!
	//! \param  other is the Marker to copy from
	Marker( const Marker & other );
	
	//!	Make this Marker an exact copy, having the same time and name, 
	//!	as the Marker rhs.
	//!
	//! \param  rhs is the Marker to assign from
    //! \return reference to self
	Marker & operator=( const Marker & rhs );
	 
//	-- comparison --

	//!	Return true if this Marker must appear earlier than rhs in a sorted
	//!	collection of Markers, and false otherwise. 
	//! (Markers are sorted by time.)
	//!
	//! \param  rhs is the Marker to compare with this Marker
	//! \return true if this Marker's time is earlier than that of
	//!         rhs, otherwise false 
	bool operator< ( const Marker & rhs ) const;
	 
//	-- access --
    
	//!	Return a reference to the name string
	//!	for this Marker.
	std::string & name( void );

	//!	Return a const reference to the name string
	//!	for this Marker.
	const std::string & name( void ) const;
	 
	//!	Return the time (in seconds) associated with this Marker.
	double time( void ) const;

	 
//	-- mutation --
	//!	Set the name of the Marker.
	void setName( const std::string & s );
	 
	//! 	Set the time (in seconds) associated with this Marker.
	void setTime( double t );

//	-- comparitors --

	//!	Comparitor (binary) functor returning true if its first Marker
	//!	argument should appear before the second in a range sorted
	//!	by Marker name.
	struct compareNameLess : 
		public std::binary_function< const Marker, const Marker, bool >
	{
		//! Function call operator, return true if the first Marker
		//!	argument should appear before the second in a range sorted
		//!	by Marker name.
		bool operator()( const Marker & lhs, const Marker & rhs ) const 
			{ return lhs.name() < rhs.name(); }
	};
	
	//! old name for compareNameLess, legacy support
	//! \deprecated Use compareNameLess instead.
	typedef compareNameLess sortByName; 
	
	
	//!	Comparitor (binary) functor returning true if its first Marker
	//!	argument should appear before the second in a range sorted
	//!	by Marker time.
	struct compareTimeLess : 
		public std::binary_function< const Marker, const Marker, bool >
	{
		//! Function call operator, return true if the first Marker
		//!	argument should appear before the second in a range sorted
		//!	by Marker time.
		bool operator()( const Marker & lhs, const Marker & rhs ) const 
			{ return lhs.time() < rhs.time(); }
	};
	
	//! old name for compareTimeLess, legacy support
	//! \deprecated Use compareTimeLess instead
	typedef compareTimeLess sortByTime; 
	
    //! Predicate functor returning true if the name of a Marker
    //! equal to the specified string, and false otherwise.
    class isNameEqual : public std::unary_function< const Marker, bool >
    {
    public:
        //! Initialize a new instance with the specified name.
    	isNameEqual( const std::string & s ) : name(s) {}
    	
    	//! Function call operator: evaluate a Marker.
    	bool operator()( const Marker & m ) const 
    		{ return m.name() == name; }
    		
    private:	
    	std::string name;	//!	the name to compare against
    };

private:

//	-- implementation --

	double m_time;			//! the time in seconds associated with the Marker
	std::string m_name;		//! the name of the Marker
			
};	//	end of class Marker

}	//	end of namespace Loris

#endif /* ndef INCLUDE_MARKER_H */
