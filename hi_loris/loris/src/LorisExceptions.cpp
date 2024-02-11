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
 * LorisExceptions.C
 *
 * Implementation of class Exception, a generic exception class.
 *
 * This file was formerly called Exception.C, and had a corresponding header
 * called Exception.h but that filename caused build problems on case-insensitive 
 * systems that sometimes had system headers called exception.h. So the header
 * name was changed to LorisExceptions.h, and this source files name was
 * changed to match. 
 *
 * Kelly Fitz, 17 Oct 2006
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */

#if HAVE_CONFIG_H
	#include "config.h"
#endif

#include "LorisExceptions.h"
#include <string>

//	begin namespace
namespace Loris {

// ---------------------------------------------------------------------------
//	Exception constructor
// ---------------------------------------------------------------------------
//! Construct a new instance with the specified description and, optionally
//! a string identifying the location at which the exception as thrown. The
//! Throw( Exception_Class, description_string ) macro generates a location
//! string automatically using __FILE__ and __LINE__.
//!
//! \param  str is a string describing the exceptional condition
//! \param  where is an option string describing the location in
//!         the source code from which the exception was thrown
//!         (generated automatically byt he Throw macro).
//
Exception::Exception( const std::string & str, const std::string & where ) :
	_sbuf( str )
{
	_sbuf.append( where );
	_sbuf.append(" ");
}
	
// ---------------------------------------------------------------------------
//	append 
// ---------------------------------------------------------------------------
//! Append the specified string to this Exception's description,
//! and return a reference to this Exception.
//! 
//! \param  str is text to append to the exception description
//! \return a reference to this Exception.
//
Exception & 
Exception::append( const std::string & str )
{
	_sbuf.append(str);
	return *this;
}

}	//	end of namespace Loris
