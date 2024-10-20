#ifndef INCLUDE_EXCEPTIONS_H
#define INCLUDE_EXCEPTIONS_H
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
 * LorisExceptions.h
 *
 * Definition of class Exception, a generic exception class, and 
 * commonly-used derived exception classes. 
 *
 * This file was formerly called Exception.h, but that filename caused build
 * problems on case-insensitive systems that sometimes had system headers
 * called exception.h.
 *
 * Kelly Fitz, 17 Oct 2006
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */
#include <stdexcept>
#include <string>

//	begin namespace
namespace Loris {

// ---------------------------------------------------------------------------
//	class Exception
//
//! Exception is a generic exception class for reporting exceptional 
//! circumstances in Loris. Exception is derived from std:exception, 
//! and is the base for a hierarchy of derived exception classes
//! in Loris.
//!
//
class Exception : public std::exception
{
//	-- public interface --
public:
//	--- lifecycle ---

	//! Construct a new instance with the specified description and, optionally
	//! a string identifying the location at which the exception as thrown. The
	//! Throw( Exception_Class, description_string ) macro generates a location
   //! string automatically using __FILE__ and __LINE__.
   //!
   //! \param  str is a string describing the exceptional condition
   //! \param  where is an option string describing the location in
   //!         the source code from which the exception was thrown
   //!         (generated automatically by the Throw macro).
	Exception( const std::string & str, const std::string & where = "" );
	 
	//! Destroy this Exception.
	virtual ~Exception( void ) throw() 
	{
	}

//	--- access/mutation ---

	//! Return a description of this Exception in the form of a
   //! C-style string (char pointer). Overrides std::exception::what.
   //!
   //! \return a C-style string describing the exceptional condition.
	const char * what( void ) const throw() { return _sbuf.c_str(); }
	 
	//! Append the specified string to this Exception's description,
	//! and return a reference to this Exception.
	//! 
	//! \param  str is text to append to the exception description
	//! \return a reference to this Exception.
	Exception & append( const std::string & str );
	 
	//! Return a read-only refernce to this Exception's 
	//! description string.
	//!
	//! \return a string describing the exceptional condition
	const std::string & str( void ) const 
	{ 
	   return _sbuf; 
	}

//	-- instance variables --
protected:

	//! string for storing the exception description
	std::string _sbuf;
	
};	//	end of class Exception

// ---------------------------------------------------------------------------
//	class AssertionFailure
//
//! Class of exceptions thrown when an assertion (usually representing an
//! invariant condition, and usually detected by the Assert macro) is
//! violated.
// 
class AssertionFailure : public Exception
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
   //!         (generated automatically by the Throw macro).
	AssertionFailure( const std::string & str, const std::string & where = "" ) : 
		Exception( std::string("Assertion failed -- ").append( str ), where ) 
	{
	}
	
};	//	end of class AssertionFailure

// ---------------------------------------------------------------------------
//	class IndexOutOfBounds
//
//! Class of exceptions thrown when a subscriptable object is accessed
//! with an index that is out of range.
//
class IndexOutOfBounds : public Exception
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
   //!         (generated automatically by the Throw macro).
	IndexOutOfBounds( const std::string & str, const std::string & where = "" ) : 
		Exception( std::string("Index out of bounds -- ").append( str ), where ) {}
		
};	//	end of class IndexOutOfBounds


// ---------------------------------------------------------------------------
//	class InvalidObject
//
//! Class of exceptions thrown when an object is found to be badly configured
//! or otherwise invalid.
//
class InvalidObject : public Exception
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
   //!         (generated automatically by the Throw macro).
	InvalidObject( const std::string & str, const std::string & where = "" ) : 
		Exception( std::string("Invalid configuration or object -- ").append( str ), where ) 
	{
	}
	
};	//	end of class InvalidObject

// ---------------------------------------------------------------------------
//	class InvalidIterator
//
//! Class of exceptions thrown when an Iterator is found to be badly configured
//! or otherwise invalid.
//
class InvalidIterator : public InvalidObject
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
   //!         (generated automatically by the Throw macro).
	InvalidIterator( const std::string & str, const std::string & where = "" ) : 
		InvalidObject( std::string("Invalid Iterator -- ").append( str ), where ) 
	{
	}
	
};	//	end of class InvalidIterator

// ---------------------------------------------------------------------------
//	class InvalidArgument
//
//! Class of exceptions thrown when a function argument is found to be invalid.
//
class InvalidArgument : public Exception
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
   //!         (generated automatically by the Throw macro).
	InvalidArgument( const std::string & str, const std::string & where = "" ) : 
		Exception( std::string("Invalid Argument -- ").append( str ), where ) 
	{
	}
	
};	//	end of class InvalidArgument

// ---------------------------------------------------------------------------
//	class RuntimeError
//
//! Class of exceptions thrown when an unanticipated runtime error is 
//! encountered.
//
class RuntimeError : public Exception
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
   //!         (generated automatically by the Throw macro).
	RuntimeError( const std::string & str, const std::string & where = "" ) : 
		Exception( std::string("Runtime Error -- ").append( str ), where ) 
	{
	}
	
};	//	end of class RuntimeError

// ---------------------------------------------------------------------------
//	class FileIOException
//
//! Class of exceptions thrown when file input or output fails.
//
class FileIOException : public RuntimeError
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
   //!         (generated automatically by the Throw macro).
	FileIOException( const std::string & str, const std::string & where = "" ) : 
		RuntimeError( std::string("File i/o error -- ").append( str ), where ) 
   {
   }
   
};	//	end of class FileIOException

// ---------------------------------------------------------------------------
//	macros for throwing exceptions
//
//	The compelling reason for using macros instead of inlines for all these
//	things is that the __FILE__ and __LINE__ macros will be useful.
//
#define __STR(x) __VAL(x)
#define __VAL(x) #x
#define	Throw( exType, report )												\
	throw exType( report, " ( " __FILE__ " line: " __STR(__LINE__) " )" )

#define Assert(test)														\
	do {																	\
		if (!(test)) Throw( Loris::AssertionFailure, #test );				\
	} while (false)


}	//	end of namespace Loris

#endif /* ndef INCLUDE_EXCEPTIONS_H */
