#ifndef INCLUDE_PARTIAL_H
#define INCLUDE_PARTIAL_H
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
 * Partial.h
 *
 * Definition of class Loris::Partial, and definitions and implementations of
 * classes of const and non-const iterators over Partials, and the exception
 * class InvalidPartial, thrown by some Partial members when invoked on a 
 * degenerate Partial having no Breakpoints.
 *
 * Kelly Fitz, 16 Aug 1999
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */

#include "Breakpoint.h"
#include "LorisExceptions.h"

#include <map>
//#include <utility>
//#include <vector>

//	begin namespace
namespace Loris {

class Partial_Iterator;
class Partial_ConstIterator;

// ---------------------------------------------------------------------------
//	class Partial
//
//!	An instance of class Partial represents a single component in the
//!	reassigned bandwidth-enhanced additive model. A Partial consists of a
//!	chain of Breakpoints describing the time-varying frequency, amplitude,
//!	and bandwidth (or noisiness) envelopes of the component, and a 4-byte
//!	label. The Breakpoints are non-uniformly distributed in time. For more
//!	information about Reassigned Bandwidth-Enhanced Analysis and the
//!	Reassigned Bandwidth-Enhanced Additive Sound Model, refer to the Loris
//!	website: www.cerlsoundgroup.org/Loris/.
//!	
//!	The constituent time-tagged Breakpoints are accessible through
//!	Partial:iterator and Partial::const_iterator interfaces.
//!	These iterator classes implement the interface for bidirectional
//!	iterators in the STL, including pre and post-increment and decrement,
//!	and dereferencing. Dereferencing a Partial::itertator or
//!	Partial::const_itertator yields a reference to a Breakpoint. Additionally,
//!	these iterator classes have breakpoint() and time() members, returning
//!	the Breakpoint (by reference) at the current iterator position and the
//!	time (by value) corresponding to that Breakpoint.
//!	
//!	Partial is a leaf class, do not subclass.
//!
//!	Most of the implementation of Partial delegates to a few
//!	container-dependent members. The following members are
//!	container-dependent, the other members are implemented in 
//!	terms of these:
//!		default construction
//!		copy (construction)
//!		operator= (assign)
//!		operator== (equivalence)
//!		size
//!		insert( pos, Breakpoint )
//!		erase( b, e )
//!		findAfter( time )
//!		begin (const and non-const)
//!		end (const and non-const)
//!		first (const and non-const)
//!		last (const and non-const)
//
class Partial
{	
//	-- public interface --
public:

//	-- types --

	//!	underlying Breakpoint container type, used by 
	//!	the iterator types defined below:
	typedef std::map< double, Breakpoint > container_type;
	
	//	typedef std::vector< std::pair< double, Breakpoint > > container_type;
	//	see Partial.C for a discussion of issues surrounding the 
	//	choice of std::map as a Breakpoint container.

	//! 32 bit type for labeling Partials
	typedef int label_type;	
	
	//!	non-const iterator over (time, Breakpoint) pairs in this Partial
	typedef Partial_Iterator iterator;	
	
	//!	const iterator over (time, Breakpoint) pairs in this Partial
	typedef Partial_ConstIterator const_iterator;
	
	//! size type for number of Breakpoints in this Partial
	typedef container_type::size_type size_type;

//	-- construction --

	//! Retun a new empty (no Breakpoints) Partial.
	Partial( void );
	 
	//!	Retun a new Partial from a half-open (const) iterator range 
	//!	of time-Breakpoint pairs.
	//!
	//!	\param	beg is the beginning of the range of time-Breakpoint
	//!			pairs to insert into the new Partial.
	//!	\param	end is the end of the range of time-Breakpoint pairs
	//!			to insert into the new Partial.
	Partial( const_iterator beg, const_iterator end );
	 
	//!	Return a new Partial that is an exact copy (has an identical set
	//!	of Breakpoints, at identical times, and the same label) of another 
	//!	Partial.
	//!
	//!	\param	other is the Partial to copy.
	Partial( const Partial & other );
	 
	//!	Destroy this Partial.
	~Partial( void );
	 
//	-- assignment --

	//!	Make this Partial an exact copy (has an identical set of 
	//!	Breakpoints, at identical times, and the same label) of another 
	//!	Partial.
	//!
	//!	\param	other is the Partial to copy.
	Partial & operator=( const Partial & other );

//	-- container-dependent implementation --

	//!	Return an iterator refering to the position of the first
	//!	Breakpoint in this Partial's envelope, or end() if there
	//! are no Breakpoints in the Partial.
	iterator begin( void );
	 
	//!	Return a const iterator refering to the position of the first
	//!	Breakpoint in this Partial's envelope, or end() if there
	//! are no Breakpoints in the Partial.
	const_iterator begin( void ) const;
	
	//!	Return an iterator refering to the position past the last
	//!	Breakpoint in this Partial's envelope. The iterator returned by
	//!	end() (like the iterator returned by the end() member of any STL
	//!	container) does not refer to a valid Breakpoint. 	
	iterator end( void );

	//!	Return a const iterator refering to the position past the last
	//!	Breakpoint in this Partial's envelope. The iterator returned by
	//!	end() (like the iterator returned by the end() member of any STL
	//!	container) does not refer to a valid Breakpoint. 	
	const_iterator end( void ) const;

	//!	Breakpoint removal: erase the Breakpoints in the specified range,
	//!	and return an iterator referring to the position after the,
	//!	erased range.
	//!
	//! \param 	beg is the beginning of the range of Breakpoints to erase
	//! \param	end is the end of the range of Breakpoints to erase
	//!	\return	The position of the first Breakpoint after the range
	//!			of removed Breakpoints, or end() if the last Breakpoint
	//!			in the Partial was removed.
	iterator erase( iterator beg, iterator end );

	//!	Return an iterator refering to the insertion position for a
	//!	Breakpoint at the specified time (that is, the position of the first
	//!	Breakpoint at a time later than the specified time).
	//!
	//!	\param	time is the time in seconds to find
	//!	\return The last position (iterator) at which a Breakpoint at the
	//!			specified time could be inserted (the position of the 
	//!			first Breakpoint later than time).
	iterator findAfter( double time );

	//!	Return a const iterator refering to the insertion position for a
	//!	Breakpoint at the specified time (that is, the position of the first
	//!	Breakpoint at a time later than the specified time).
	//!
	//!	\param	time is the time in seconds to find
	//!	\return The last position (iterator) at which a Breakpoint at the
	//!			specified time could be inserted (the position of the 
	//!			first Breakpoint later than time).
	const_iterator findAfter( double time ) const;

	//!	Breakpoint insertion: insert a copy of the specified Breakpoint in the
	//!	parameter envelope at time (seconds), and return an iterator
	//!	refering to the position of the inserted Breakpoint.
	//!
	//!	\param 	time is the time in seconds at which to insert the new
	//!			Breakpoint.
	//!	\param 	bp is the new Breakpoint to insert.
	//!	\return the position (iterator) of the newly-inserted 
	//!			time-Breakpoint pair.
	iterator insert( double time, const Breakpoint & bp );

	//!	Return the number of Breakpoints in this Partial.
	//!
	//!	\return	The number of Breakpoints in this Partial.
	size_type size( void ) const;
		 	
//	-- access --

	//!	Return the duration (in seconds) spanned by the Breakpoints in
	//!	this Partial. Note that the synthesized onset time will differ,
	//!	depending on the fade time used to synthesize this Partial (see
	//!	class Synthesizer).
	double duration( void ) const;
	 
	//!	Return the time (in seconds) of the last Breakpoint in this
	//!	Partial. Note that the synthesized onset time will differ,
	//!	depending on the fade time used to synthesize this Partial (see
	//!	class Synthesizer).
	double endTime( void ) const;
	 
	//!	Return a reference to the first Breakpoint in the Partial's
	//!	envelope. 
	//!
	//!	\throw InvalidPartial if there are no Breakpoints.
	Breakpoint & first( void );

	//!	Return a const reference to the first Breakpoint in the Partial's
	//!	envelope.  
	//!
	//!	\throw InvalidPartial if there are no Breakpoints.
	const Breakpoint & first( void ) const;
	 
	//!	Return the phase (in radians) of this Partial at its start time
	//!	(the phase of the first Breakpoint). Note that the initial
	//!	synthesized phase will differ, depending on the fade time used
	//!	to synthesize this Partial (see class Synthesizer).
	double initialPhase( void ) const;
	 	 
	//!	Return the 32-bit label for this Partial as an integer.
	label_type label( void ) const;

	//!	Return a reference to the last Breakpoint in the Partial's
	//!	envelope. 
	//!
	//!	\throw InvalidPartial if there are no Breakpoints.	 
	Breakpoint & last( void );
	
	//!	Return a const reference to the last Breakpoint in the Partial's
	//!	envelope. 
	//!
	//!	\throw InvalidPartial if there are no Breakpoints.	
	const Breakpoint & last( void ) const;
	 
	//!	Same as size(). Return the number of Breakpoints in this Partial.
	size_type numBreakpoints( void ) const;

	//!	Return the time (in seconds) of the first Breakpoint in this
	//!	Partial. Note that the synthesized onset time will differ,
	//!	depending on the fade time used to synthesize this Partial (see
	//!	class Synthesizer).
	double startTime( void ) const;
	 
//	-- mutation --

	//!	Absorb another Partial's energy as noise (bandwidth), 
	//!	by accumulating the other's energy as noise energy
	//!	in the portion of this Partial's envelope that overlaps
	//!	(in time) with the other Partial's envelope.
	//!
	//!	\param	other is the Partial to absorb.
	void absorb( const Partial & other );

	//!	Set the label for this Partial to the specified 32-bit value.
	void setLabel( label_type l );

	//!	Remove the Breakpoint at the position of the given
	//!	iterator, invalidating the iterator. Return a 
	//!	iterator referring to the next valid position, or to
	//!	the end of the Partial if the last Breakpoint is removed.
	//!
	//!	\param	pos is the position of the time-Breakpoint pair
	//!			to be removed.
	//!	\return The position (iterator) of the time-Breakpoint
	//!			pair after the one that was removed.
	//!	\post	The iterator pos is invalid. 
	iterator erase( iterator pos );
	 
	//!	Return an iterator refering to the position of the
	//!	Breakpoint in this Partial nearest the specified time.
	//!	
	//!	\param	time is the time to find.
	//!	\return	The position (iterator) of the time-Breakpoint
	//!			pair nearest (in time) to the specified time.
	iterator findNearest( double time );

	//!	Return a const iterator refering to the position of the
	//!	Breakpoint in this Partial nearest the specified time.
	//!	
	//!	\param	time is the time to find.
	//!	\return	The position (iterator) of the time-Breakpoint
	//!			pair nearest (in time) to the specified time.
	const_iterator findNearest( double time ) const;

	//!	Break this Partial at the specified position (iterator).
	//!	The Breakpoint at the specified position becomes the first
	//!	Breakpoint in a new Partial. Breakpoints at the specified
	//!	position and subsequent positions are removed from this
	//!	Partial and added to the new Partial, which is returned.
	//!
	//!	\param	pos is the position at which to split this Partial.
	//!	\return A new Partial consisting of time-Breakpoint pairs
	//!			beginning with pos and extending to the end of this
	//!			Partial.
	//!	\post	All positions beginning with pos and extending to
	//!			the end of this Partial have been removed.
	Partial split( iterator pos );
	 
//	-- parameter interpolation/extrapolation --

	//!	Define the default fade time for computing amplitude at the ends
	//!	of a Partial. Floating point round-off errors make fadeTime == 0.0
	//!	dangerous and unpredictable. 1 ns is short enough to prevent rounding
	//!	errors in the least significant bit of a 48-bit mantissa for times
	//!	up to ten hours.
	//!
	//!	1 nanosecond, see Partial.C
	static const double ShortestSafeFadeTime;	

	//!	Return the interpolated amplitude of this Partial at the
	//!	specified time. If non-zero fadeTime is specified, 
	//!	then the amplitude at the ends of the Partial is computed using
	//!	a linear fade. The default fadeTime is ShortestSafeFadeTime,
	//!	see the definition of ShortestSafeFadeTime, above.
	//!	
	//!	\param	time is the time in seconds at which to evaluate the 
	//!			Partial.
	//!	\param	fadeTime is the duration in seconds over which Partial
	//!			amplitudes fade at the ends. The default value is
	//!			ShortestSafeFadeTime, 1 ns.
	//!	\return	The amplitude of this Partial at the specified time.
	//! \pre	The Partial must have at least one Breakpoint.
	//!	\throw	InvalidPartial if the Partial has no Breakpoints.
	double amplitudeAt( double time, double fadeTime = ShortestSafeFadeTime ) const;

	//!	Return the interpolated bandwidth (noisiness) coefficient of
	//!	this Partial at the specified time. At times beyond the ends of
	//!	the Partial, return the bandwidth coefficient at the nearest
	//!	envelope endpoint. 
	//!	
	//!	\param	time is the time in seconds at which to evaluate the 
	//!			Partial.
	//!	\return	The bandwidth of this Partial at the specified time.
	//! \pre	The Partial must have at least one Breakpoint.
	//!	\throw	InvalidPartial if the Partial has no Breakpoints.
	double bandwidthAt( double time ) const;
	 
	//!	Return the interpolated frequency (in Hz) of this Partial at the
	//!	specified time. At times beyond the ends of the Partial, return
	//!	the frequency at the nearest envelope endpoint.
	//!	
	//!	\param	time is the time in seconds at which to evaluate the 
	//!			Partial.
	//!	\return	The frequency of this Partial at the specified time.
	//! \pre	The Partial must have at least one Breakpoint.
	//!	\throw	InvalidPartial if the Partial has no Breakpoints.
	double frequencyAt( double time ) const;
	 
	//!	Return the interpolated phase (in radians) of this Partial at
	//!	the specified time. At times beyond the ends of the Partial,
	//!	return the extrapolated from the nearest envelope endpoint
	//!	(assuming constant frequency, as reported by frequencyAt()).
	//!	
	//!	\param	time is the time in seconds at which to evaluate the 
	//!			Partial.
	//!	\return	The phase of this Partial at the specified time.
	//! \pre	The Partial must have at least one Breakpoint.
	//!	\throw	InvalidPartial if the Partial has no Breakpoints.
	double phaseAt( double time ) const;

	//!	Return the interpolated parameters of this Partial at
	//!	the specified time, same as building a Breakpoint from
	//!	the results of frequencyAt, ampitudeAt, bandwidthAt, and
	//!	phaseAt, but performs only one Breakpoint envelope search.
	//!	If non-zero fadeTime is specified, then the
	//!	amplitude at the ends of the Partial is coomputed using a 
	//!	linear fade. The default fadeTime is ShortestSafeFadeTime.
	//!	
	//!	\param	time is the time in seconds at which to evaluate the 
	//!			Partial.
	//!	\param	fadeTime is the duration in seconds over which Partial
	//!			amplitudes fade at the ends. The default value is
	//!			ShortestSafeFadeTime, 1 ns.
	//!	\return	A Breakpoint describing the parameters of this Partial 
	//!			at the specified time.
	//! \pre	The Partial must have at least one Breakpoint.
	//!	\throw	InvalidPartial if the Partial has no Breakpoints.
	Breakpoint parametersAt( double time, double fadeTime = ShortestSafeFadeTime ) const;

//	-- implementation --
private:

	label_type _label;
	container_type _breakpoints;	//	Breakpoint envelope
	 
};	//	end of class Partial

// ---------------------------------------------------------------------------
//	class Partial_Iterator
//
//!	Non-const iterator for the Loris::Partial Breakpoint map. Wraps
//!	the non-const iterator for the (time,Breakpoint) pair container
//!	Partial::container_type. Partial_Iterator implements a
//!	bidirectional iterator interface, and additionally offers time
//!	and Breakpoint (reference) access through time() and breakpoint()
//!	members.
//
class Partial_Iterator
{
//	-- instance variables --

	typedef Partial::container_type BaseContainer;
	typedef BaseContainer::iterator BaseIterator;
	BaseIterator _iter;
	
//	-- public interface --
public:
//	-- bidirectional iterator interface --

	//! The iterator category, for copmpatibility with 
	//! C++ standard library algorithms 
	typedef BaseIterator::iterator_category	iterator_category;
	
	//! The type of element that can be accessed through this 
	//! iterator (Breakpoint).
	typedef Breakpoint     					value_type;
	
	//! The type representing the distance between two of these
	//! iterators.
	typedef BaseIterator::difference_type  	difference_type;
	
	//! The type of a pointer to the type of element that can 
	//! be accessed through this iterator (Breakpoint *).
	typedef Breakpoint *					pointer;

	//! The type of a reference to the type of element that can 
	//! be accessed through this iterator (Breakpoint &).
	typedef Breakpoint &					reference;

//	construction:
	
	//!	Construct a new iterator referring to no position in 
	//!	any Partial.
	Partial_Iterator( void ) {}
	
	//	(allow compiler to generate copy, assignment, and destruction)
	
//	pre-increment/decrement:

	//!	Pre-increment operator - advance the position of the iterator
	//! and return the iterator itself.
	//!
	//!	\return	This iterator (reference to self).
	//!	\pre	The iterator must be a valid position before the end
	//!			in some Partial.
	Partial_Iterator& operator ++ () { ++_iter; return *this; }

	//!	Pre-decrement operator - move the position of the iterator
	//! back by one and return the iterator itself.
	//!
	//!	\return	This iterator (reference to self).
	//!	\pre	The iterator must be a valid position after the beginning
	//!			in some Partial.
	Partial_Iterator& operator -- () { --_iter; return *this; }

//	post-increment/decrement:

	//!	Post-increment operator - advance the position of the iterator
	//! and return a copy of the iterator before it was advanced.
	//!	The int argument is unused compiler magic.
	//!	
	//!	\return	An iterator that is a copy of this iterator before 
	//!			being advanced.
	//!	\pre	The iterator must be a valid position before the end
	//!			in some Partial.
	Partial_Iterator operator ++ ( int ) { return Partial_Iterator( _iter++ ); } 

	//!	Post-decrement operator - move the position of the iterator
	//! back by one and return a copy of the iterator before it was 
	//!	decremented. The int argument is unused compiler magic.
	//!	
	//!	\return	An iterator that is a copy of this iterator before 
	//!			being decremented.
	//!	\pre	The iterator must be a valid position after the beginning
	//!			in some Partial.
	Partial_Iterator operator -- ( int ) { return Partial_Iterator( _iter-- ); } 
	
//	dereference (for treating Partial like a 
//	STL collection of Breakpoints):

	//!	Dereference operator.
	//!
	//!	\return	A reference to the Breakpoint at the position of this
	//!			iterator.
	Breakpoint & operator * ( void ) const { return breakpoint(); }


	//!	Dereference operator.
	//!
	//!	\return	A reference to the Breakpoint at the position of this
	//!			iterator.
	//Breakpoint & operator * ( void ) { return breakpoint(); }
	
	//!	Pointer operator.
	//!
	//!	\return	A pointer to the Breakpoint at the position of this
	//!			iterator.
	Breakpoint * operator -> ( void ) const  { return & breakpoint(); }

	//!	Pointer operator.
	//!
	//!	\return	A pointer to the Breakpoint at the position of this
	//!			iterator.
	//Breakpoint * operator -> ( void )  { return & breakpoint(); }
		
//	comparison:

	//!	Equality comparison operator.
	//!
	//!	\param 	lhs the iterator on the left side of the operator.
	//!	\param 	rhs the iterator on the right side of the operator.
	//!	\return	true if the two iterators refer to the same position
	//!			in the same Partial, false otherwise.
	friend bool operator == ( const Partial_Iterator & lhs, 
							  const Partial_Iterator & rhs )
		{ return lhs._iter == rhs._iter; }

	//!	Inequality comparison operator.
	//!
	//!	\param 	lhs the iterator on the left side of the operator.
	//!	\param 	rhs the iterator on the right side of the operator.
	//!	\return	false if the two iterators refer to the same position
	//!			in the same Partial, true otherwise.
	friend bool operator != ( const Partial_Iterator & lhs, 
							  const Partial_Iterator & rhs )
		{ return lhs._iter != rhs._iter; }
	
//	-- time and Breakpoint access --

	//!	Breakpoint accessor.
	//!
	//!	\return	A const reference to the Breakpoint at the position of this
	//!			iterator.
	Breakpoint & breakpoint( void ) const 
		{ return _iter->second; }

	//!	Breakpoint accessor.
	//!
	//!	\return	A reference to the Breakpoint at the position of this
	//!			iterator.
	//Breakpoint & breakpoint( void ) 
	//	{ return _iter->second; }

	//!	Time accessor.
	//!
	//!	\return	The time in seconds of the Breakpoint at the position 
	//!			of this iterator.
	double time( void ) const  
		{ return _iter->first; }

//	-- BaseIterator conversions --
private:
	//	construction by GenericBreakpointContainer from a BaseIterator:
	Partial_Iterator( const BaseIterator & it ) :
		_iter(it) {}

	friend class Partial;
	
	//	befriend  Partial_ConstIterator, 
	//	for const construction from non-const:
	friend class Partial_ConstIterator;	
	
};	//	end of class Partial_Iterator

// ---------------------------------------------------------------------------
//	class Partial_ConstIterator
//
//!	Const iterator for the Loris::Partial Breakpoint map. Wraps
//!	the non-const iterator for the (time,Breakpoint) pair container
//!	Partial::container_type. Partial_Iterator implements a
//!	bidirectional iterator interface, and additionally offers time
//!	and Breakpoint (reference) access through time() and breakpoint()
//!	members.
//
class Partial_ConstIterator
{
//	-- instance variables --
	typedef Partial::container_type BaseContainer;
	typedef BaseContainer::const_iterator BaseIterator;
	BaseIterator _iter;
	
//	-- public interface --
public:
//	-- bidirectional iterator interface --

	//! The iterator category, for copmpatibility with 
	//! C++ standard library algorithms 
	typedef BaseIterator::iterator_category	iterator_category;
	
	//! The type of element that can be accessed through this 
	//! iterator (Breakpoint).
	typedef Breakpoint     					value_type;
	
	//! The type representing the distance between two of these
	//! iterators.
	typedef BaseIterator::difference_type  	difference_type;
	
	//! The type of a pointer to the type of element that can 
	//! be accessed through this iterator (const Breakpoint *).
	typedef const Breakpoint *					pointer;

	//! The type of a reference to the type of element that can 
	//! be accessed through this iterator (const Breakpoint &).
	typedef const Breakpoint &					reference;

//	construction:

	//!	Construct a new iterator referring to no position in 
	//!	any Partial.
	Partial_ConstIterator( void ) {}
	
	//!	Construct a new const iterator from a non-const iterator.
	//!
	//!	\param	other a non-const iterator from which to make
	//!			a read-only copy.
	Partial_ConstIterator( const Partial_Iterator & other ) :
		_iter( other._iter ) {}
	
	//	(allow compiler to generate copy, assignment, and destruction):

//	pre-increment/decrement:

	//!	Pre-increment operator - advance the position of the iterator
	//! and return the iterator itself.
	//!
	//!	\return	This iterator (reference to self).
	//!	\pre	The iterator must be a valid position before the end
	//!			in some Partial.
	Partial_ConstIterator& operator ++ () { ++_iter; return *this; }

	//!	Pre-decrement operator - move the position of the iterator
	//! back by one and return the iterator itself.
	//!
	//!	\return	This iterator (reference to self).
	//!	\pre	The iterator must be a valid position after the beginning
	//!			in some Partial.
	Partial_ConstIterator& operator -- () { --_iter; return *this; }

//	post-increment/decrement:

	//!	Post-increment operator - advance the position of the iterator
	//! and return a copy of the iterator before it was advanced.
	//!	The int argument is unused compiler magic.
	//!	
	//!	\return	An iterator that is a copy of this iterator before 
	//!			being advanced.
	//!	\pre	The iterator must be a valid position before the end
	//!			in some Partial.
	Partial_ConstIterator operator ++ ( int ) { return Partial_ConstIterator( _iter++ ); } 

	//!	Post-decrement operator - move the position of the iterator
	//! back by one and return a copy of the iterator before it was 
	//!	decremented. The int argument is unused compiler magic.
	//!	
	//!	\return	An iterator that is a copy of this iterator before 
	//!			being decremented.
	//!	\pre	The iterator must be a valid position after the beginning
	//!			in some Partial.
	Partial_ConstIterator operator -- ( int ) { return Partial_ConstIterator( _iter-- ); } 
	
//	dereference (for treating Partial like a 
//	STL collection of Breakpoints):

	//!	Dereference operator.
	//!
	//!	\return	A const reference to the Breakpoint at the position of this
	//!			iterator.
	const Breakpoint & operator * ( void ) const { return breakpoint(); }

	//!	Pointer operator.
	//!
	//!	\return	A const pointer to the Breakpoint at the position of this
	//!			iterator.
	const Breakpoint * operator -> ( void ) const { return & breakpoint(); }
	
//	comparison:

	//!	Equality comparison operator.
	//!
	//!	\param 	lhs the iterator on the left side of the operator.
	//!	\param 	rhs the iterator on the right side of the operator.
	//!	\return	true if the two iterators refer to the same position
	//!			in the same Partial, false otherwise.
	friend bool operator == ( const Partial_ConstIterator & lhs, 
							  const Partial_ConstIterator & rhs )
		{ return lhs._iter == rhs._iter; }

	//!	Inequality comparison operator.
	//!
	//!	\param 	lhs the iterator on the left side of the operator.
	//!	\param 	rhs the iterator on the right side of the operator.
	//!	\return	false if the two iterators refer to the same position
	//!			in the same Partial, true otherwise.
	friend bool operator != ( const Partial_ConstIterator & lhs, 
							  const Partial_ConstIterator & rhs )
		{ return lhs._iter != rhs._iter; }
	
//	-- time and Breakpoint access --

	//!	Breakpoint accessor.
	//!
	//!	\return	A const reference to the Breakpoint at the position of this
	//!			iterator.
	const Breakpoint & breakpoint( void ) const 
		{ return _iter->second; }

	//!	Time accessor.
	//!
	//!	\return	The time in seconds of the Breakpoint at the position 
	//!			of this iterator.
	double time( void ) const  
		{ return _iter->first; }
	
//	-- BaseIterator conversions --
private:
	//	construction by GenericBreakpointContainer from a BaseIterator:
	Partial_ConstIterator( BaseIterator it ) :
		_iter(it) {}
	
	friend class Partial;

};	//	end of class Partial_ConstIterator

// ---------------------------------------------------------------------------
//	class InvalidPartial
//
//! Class of exceptions thrown when a Partial is found to be badly configured
//! or otherwise invalid.
//
class InvalidPartial : public InvalidObject
{
public: 

	//! Construct a new instance with the specified description and, optionally
	//! a string identifying the location at which the exception as thrown. The
	//! Throw( Exception_Class, description_string ) macro generates a location
   //! string automatically using __FILE__ and __LINE__.
   //!
   //! \param  str is a string describing the exceptional condition
   //! \param  where is an option string describing the location in
   //!         the source code from which the exception was thrown
   //!         (generated automatically byt he Throw macro).
	InvalidPartial( const std::string & str, const std::string & where = "" ) : 
		InvalidObject( std::string("Invalid Partial -- ").append( str ), where ) {}
		
};	//	end of class InvalidPartial

}	//	end of namespace Loris

#endif /* ndef INCLUDE_PARTIAL_H */
