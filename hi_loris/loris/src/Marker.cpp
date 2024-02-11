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
 * Marker.cc
 *
 * Definition of members for Marker and MarkerContainer representing labeled
 * time points or temporal features in imported and exported data. Used by 
 * file I/O classes AiffFile, SdifFile, and SpcFile.
 *
 * Kelly Fitz, 8 Jan 2003 
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */
#if HAVE_CONFIG_H
	#include "config.h"
#endif

#include "Marker.h"

//	begin namespace
namespace Loris {

// -- construction --

// ---------------------------------------------------------------------------
//	Marker - default constructor
// ---------------------------------------------------------------------------
//	Default constructor - initialize a Marker at time zero with no label.
//
Marker::Marker( void ) :
	m_time( 0. ),
	m_name("Untitled Marker")
{
}
	 
// ---------------------------------------------------------------------------
//	Marker - constructor from time and name
// ---------------------------------------------------------------------------
//	Initialize a Marker with the specified time (in seconds) and name.
//
Marker::Marker( double t, const std::string & s ) :
	m_time( t ),
	m_name( s )
{
}

// ---------------------------------------------------------------------------
//	Marker - copy constructor
// ---------------------------------------------------------------------------
//	Initialize a Marker that is an exact copy of another Marker, that is,
//	having the same time and name.
//
Marker::Marker( const Marker & other ) :
	m_time( other.m_time ),
	m_name( other.m_name )
{
}

// ---------------------------------------------------------------------------
//	assignment operator (operator =)
// ---------------------------------------------------------------------------
//	Make this Marker an exact copy, having the same time and name, 
//	as the Marker rhs.
//
Marker & 
Marker::operator=( const Marker & rhs )
{
	if ( this != &rhs )
	{
		//	the only imaginable exception that could be generated
		//	would be an out-of-memory exception at the time of this
		//	string assignment, reserve memory first, so that if this
		//	does except, the Marker is unchanged:
		m_name.reserve( rhs.m_name.size() );
		m_name = rhs.m_name;
		m_time = rhs.m_time;		

	}
	return *this;
}

// -- comparison --

// ---------------------------------------------------------------------------
//	less-than operator (operator <)
// ---------------------------------------------------------------------------
//	Return true if this Marker must appear earlier than rhs in a sorted
//	collection of Markers, and false otherwise. (Markers are sorted by time.)
//
bool
Marker::operator< ( const Marker & rhs ) const
{
	return m_time < rhs.m_time;
}	 

// -- access --

// ---------------------------------------------------------------------------
//	name
// ---------------------------------------------------------------------------
//	Return a reference to the name string for this Marker.
//
std::string & 
Marker::name( void )
{
	return m_name;
}

// ---------------------------------------------------------------------------
//	name (const)
// ---------------------------------------------------------------------------
//	Return a const reference to the name string for this Marker.
//
const std::string & 
Marker::name( void ) const
{
	return m_name;
}
	 
// ---------------------------------------------------------------------------
//	time
// ---------------------------------------------------------------------------
//	Return the time (in seconds) associated with this Marker.
//
double 
Marker::time( void ) const
{
	return m_time;
}

// -- mutation --

// ---------------------------------------------------------------------------
//	setName
// ---------------------------------------------------------------------------
//	Set the name of the Marker.
//
void 
Marker::setName( const std::string & s )
{
	m_name = s;
}
	 
// ---------------------------------------------------------------------------
//	setName
// ---------------------------------------------------------------------------
//	Set the time (in seconds) associated with this Marker.
//
void 
Marker::setTime( double t )
{
	m_time = t;
}

}	//	end of namespace Loris
