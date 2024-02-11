#ifndef INCLUDE_BIGENDIAN_H
#define INCLUDE_BIGENDIAN_H
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
 * BigEndian.h
 *
 * Definition of wrappers for stream-based binary file i/o.
 *
 * Kelly Fitz, 23 May 2000
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */

#include <iosfwd>

//	begin namespace
namespace Loris {

// ---------------------------------------------------------------------------
//	class BigEndian
//
class BigEndian
{
public:
	static std::istream & read( std::istream & s, long howmany, int size, char * putemHere );
	static std::ostream & write( std::ostream & s, long howmany, int size, const char * stuff );	
};	//	end of class BigEndian


}	//	end of namespace Loris

#endif /* ndef INCLUDE_BIGENDIAN_H */
