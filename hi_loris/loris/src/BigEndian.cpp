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
 * BigEndian.C
 *
 * Implementation of wrappers for stream-based binary file i/o.
 *
 * Kelly Fitz, 23 May 2000
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */

#if HAVE_CONFIG_H
	#include "config.h"
#endif

#include "BigEndian.h"
#include "LorisExceptions.h"
#include <vector>
#include <iostream>

//	begin namespace
namespace Loris {

// ---------------------------------------------------------------------------
//	bigEndianSystem
// ---------------------------------------------------------------------------
//	Return true is this is a big-endian system, false otherwise.
//
static bool bigEndianSystem( void )
{
#if defined(WORDS_BIGENDIAN)
	return true;
#elif HAVE_CONFIG_H && !defined(WORDS_BIGENDIAN)
	return false;
#else	
	static union {
		int s ;
		char c[sizeof(int)] ;
	} x ;
	bool ret = (x.s = 1, x.c[0] != 1) ? true : false;
	
	return ret; // x.c[0] != 1;
#endif
}

// ---------------------------------------------------------------------------
//	swapByteOrder
// ---------------------------------------------------------------------------
//	
static void swapByteOrder( char * bytes, int n )
{
	char * beg = bytes, * end = bytes + n - 1;
	while ( beg < end ) {
		char tmp = *end;
		*end = *beg;
		*beg = tmp;
		
		++beg;
		--end;
	}
}

// ---------------------------------------------------------------------------
//	BigEndian read
// ---------------------------------------------------------------------------
//
std::istream &
BigEndian::read( std::istream & s, long howmany, int size, char * putemHere )
{
	//	read the bytes into data:
	s.read( putemHere, howmany*size );
	
	//	check stream state:	
    if ( s )
    {
        //  if the stream is still in a good state, then
        //  the correct number of bytes must have been read:
        Assert( s.gcount() == howmany*size );

        //	swap byte order if nec.
        if ( ! bigEndianSystem() && size > 1 ) 
        {
            for ( long i = 0; i < howmany; ++i )
            {
                swapByteOrder( putemHere + (i*size), size );
            }
        }
    }
    
    return s;
}

// ---------------------------------------------------------------------------
//	BigEndian write
// ---------------------------------------------------------------------------
//
std::ostream &
BigEndian::write( std::ostream & s, long howmany, int size, const char * stuff )
{
	//	swap byte order if nec.
	if ( ! bigEndianSystem() && size > 1 ) 
	{
		//	use a temporary vector to automate storage:
		std::vector<char> v( stuff, stuff + (howmany*size) );
		for ( long i = 0; i < howmany; ++i )
		{
			swapByteOrder( & v[i*size], size );
		}
		s.write( &v[0], howmany*size );
	}
	else
	{
		//	read the bytes into data:
		s.write( stuff, howmany*size );
	}
	
	//	check stream state:
	if ( ! s.good() )
		Throw( FileIOException, "File write failed. " );
        
    return s;
}

}	//	end of namespace Loris


