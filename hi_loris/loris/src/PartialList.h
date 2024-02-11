#ifndef INCLUDE_PARTIALLIST_H
#define INCLUDE_PARTIALLIST_H
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
 *	PartialList.h
 *
 *	Type definition of Loris::PartialList, which is just a name
 *	for std::list< Loris::Partial >.
 *
 * Kelly Fitz, 6 March 2002
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */
 
//	Seems like we shouldn't need to include Partial.h, but 
//	without it, I can't instantiate a PartialList. I need
//	a definition of Partial for PartialList to be unambiguous.
#include "Partial.h"
#include <list>

//	begin namespace
namespace Loris {

// ---------------------------------------------------------------------------
//	class PartialList
//
//	PartialList is a typedef for a std::list<> of Loris Partials. The
//	oscciated bidirectional iterators are also defined as
//	PartialListIterator and PartialListConstIterator. Since these are
//	simply typedefs, they classes have identical interfaces to std::list,
//	std::list::iterator, and std::list::const_iterator, respectively.
//
typedef std::list< Loris::Partial > PartialList;
typedef std::list< Loris::Partial >::iterator PartialListIterator;
typedef std::list< Loris::Partial >::const_iterator PartialListConstIterator;

}	//	end of namespace Loris

#endif /* ndef INCLUDE_PARTIALLIST_H */
