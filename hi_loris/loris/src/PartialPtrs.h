#ifndef INCLUDE_PARTIALPTRS_H
#define INCLUDE_PARTIALPTRS_H
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
 *	PartialPtrs.h
 *
 *	Type definition of Loris::PartialPtrs.
 *
 *	PartialPtrs is a collection of pointers to Partials that
 *	can be used (among other things) for algorithms that operate
 *	on a range of Partials, but don't rely on access to their
 *	container.
 *
 * Kelly Fitz, 23 May 2002
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */

#include "Partial.h"
#include <iterator>
#include <vector>

//	begin namespace
namespace Loris {


// ---------------------------------------------------------------------------
//	class PartialPtrs
//
//	PartialPtrs is a typedef for a std::vectror<> of pointers to Loris
//	Partials. The oscciated bidirectional iterators are also defined as
//	PartialPtrsIterator and PartialPtrsConstIterator. Since these are
//	simply typedefs, they classes have identical interfaces to
//	std::vector, std::vector::iterator, and std::vector::const_iterator,
//	respectively.
//	
//	PartialPtrs is a collection of pointers to Partials that can be used
//	(among other things) for algorithms that operate on a range of
//	Partials, but don't rely on access to their container. A template
//	function defined in a header file can convert a range of Partials to a
//	PartialPtrs using the template free function fillPartialPtrs() (see
//	below), and pass the latter to the algorithm implementation, thereby
//	generalizing access to the algorithm across containers without
//	exposing the implementation in the header file.
//
typedef std::vector< Partial * > PartialPtrs;
typedef std::vector< Partial * >::iterator PartialPtrsIterator;
typedef std::vector< Partial * >::const_iterator PartialPtrsConstIterator;

typedef std::vector< const Partial * > ConstPartialPtrs;
typedef std::vector< const Partial * >::iterator ConstPartialPtrsIterator;
typedef std::vector< const Partial * >::const_iterator ConstPartialPtrsConstIterator;


// ---------------------------------------------------------------------------
//	fillPartialPtrs
// ---------------------------------------------------------------------------
//	Fill the specified PartialPtrs with pointers to the Partials n the
//	specified half-open (STL-style) range. This is a generally useful
//	operation that can be used to adapt algorithms to work with arbitrary
//	containers of Partials without exposing the algorithms themselves in
//	the header files.
//
template <typename Iter>
void fillPartialPtrs( Iter begin, Iter end, PartialPtrs & fillme )
{
	fillme.reserve( std::distance( begin, end ) );
	fillme.clear();
	while ( begin != end )
		fillme.push_back( &(*begin++) );
}

template <typename Iter>
void fillPartialPtrs( Iter begin, Iter end, ConstPartialPtrs & fillme )
{
	fillme.reserve( std::distance( begin, end ) );
	fillme.clear();
	while ( begin != end )
		fillme.push_back( &(*begin++) );
}

}	//	end of namespace Loris

#endif /* ndef INCLUDE_PARTIALPTRS_H */
