#ifndef INCLUDE_IMPORTLEMUR_H
#define INCLUDE_IMPORTLEMUR_H
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
 * ImportLemur.h
 *
 * Definition of class Loris::ImportLemur for importing Partials stored 
 * in Lemur 5 alpha files.
 *
 * Kelly Fitz, 10 Sept 1999
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */

#include "PartialList.h"
#include "LorisExceptions.h"
#include <string>

//	begin namespace
namespace Loris {

// ---------------------------------------------------------------------------
//	class ImportLemur
//
class ImportLemur
{
//	-- instance variables --
	PartialList _partials;	//	collect Partials here

//	-- public interface --
public:
//	construction:
//	(compiler can generate destructor)
	ImportLemur( const std::string & fname, double bweCutoff = 1000 );

//	PartialList access:
	PartialList & partials( void ) { return _partials; }
	const PartialList & partials( void ) const { return _partials; }
	
//	-- unimplemented --
private:
	ImportLemur( const ImportLemur & other );
	ImportLemur  & operator = ( const ImportLemur & rhs );
	
};	//	end of class ImportLemur

// ---------------------------------------------------------------------------
//	class ImportException
//
//	Class of exceptions thrown when there is an error importing
//	Partials.
//
class ImportException : public Exception
{
public: 
	ImportException( const std::string & str, const std::string & where = "" ) : 
		Exception( std::string("Import Error -- ").append( str ), where ) {}		
};

}	//	end of namespace Loris

#endif /* ndef INCLUDE_IMPORTLEMUR_H */
