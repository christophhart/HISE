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
 * Notifier.C
 *
 * Definitions of the ostreams used for notification throughout the Loris 
 * library (notifier and debugger), and the c-linkable functions for assigning
 * notification handlers (of type NotificationHandler, see Notifier.h)
 * to each.
 * 
 * These ostreams use a streambuf derivative, NotifierBuf, to buffer characters
 * in a std::string, and post them (via a handler) when a newline is received.
 * NotifierBuf is defined and implemented below.
 *
 * Kelly Fitz, 28 Feb 2000
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */

#if HAVE_CONFIG_H
	#include "config.h"
#endif

#include "Notifier.h"
#include <string>
#include <cstdio>

#if defined(__GNUC__)
	//	other compilers have this problem?
	typedef int int_type;
#endif

using namespace std;

//	begin namespace
namespace Loris {


// ---------------------------------------------------------------------------
//	defaultNotifierhandler
// ---------------------------------------------------------------------------
//	By default, notifications go to stdout (no need to bring in iostreams 
//	for this. 
//
//	Is printf different from fprint to stdout? On the mac, using CW5, and
//	linking into a Python module (main() written in c), only printf works.
//
static void defaultNotifierhandler( const char * s )
{
	//cout << s << endl;
	//fprintf( stdout, "%s\n", s );
	printf( "%s\n", s );
}

// ---------------------------------------------------------------------------
//	class NotifierBuf
//
//	streambuf derivative that buffers output in a std::string 
//	and posts it to a handler (_post) when a newline is received. 
//
class NotifierBuf : public streambuf
{
//	-- public interface --
public:
//	construction:
	NotifierBuf( const std::string & s = "" ) : 
		_str(s), _post( defaultNotifierhandler ) 
		//	confirm construction:
		// { printf( "created a NotifierBuf.\n" ); }
		{} 
		
//	virtual destructor so NotifierBuf can be subclassed:
//	(use compiler generated, streambuf has virtual destructor)
	//virtual ~NotifierBuf( void );	
	
//	handler manipulation:
	NotificationHandler setHandler( NotificationHandler h ) throw()
	{
		NotificationHandler prev = _post;
		_post = h;
		return prev;
	}
	
protected:
	//	called every time a character is written:
	virtual int_type overflow( int_type c ) 
	{
		if ( c == '\n' ) {
			_post( _str.c_str() );
			_str = "";
		}
		else if ( c != EOF ) {
			char ch(c);
			_str += ch;
			//_str += static_cast<char>(c); ???
		}
		return c;
	}
	
private:
	//	buffer characters in a string:
	std::string _str;
	
	//	handler:
	NotificationHandler _post;
	
};	//	end of class NotifierBuf

// ---------------------------------------------------------------------------
//	stream instances
// ---------------------------------------------------------------------------
//	ostreams used throughout Loris for notification.
//
//	Instead of making these globals by declaring them at file scope, 
//	make them static to these initializer functions, to make sure (?)
//	that their constructors get called.
//
static NotifierBuf & notifierBuffer(void)
{
	static NotifierBuf buf;
	return buf;
}

std::ostream & getNotifierStream(void)
{
	static ostream os(&notifierBuffer());
	return os;
}


#if defined( Debug_Loris )
	static NotifierBuf & debuggerBuffer(void)
	{
		static NotifierBuf buf;
		return buf;
	}
#else
	//	to do nothing at all, need a dummy streambuf:
	struct Dummybuf : public streambuf
	{
	};
	static Dummybuf & debuggerBuffer( void )
	{
		static Dummybuf buf;
		return buf;
	}
#endif

std::ostream & getDebuggerStream(void)
{
	static ostream os(&debuggerBuffer());
	return os;
}

// ---------------------------------------------------------------------------
//	setNotifierHandler
// ---------------------------------------------------------------------------
//	Specify a new handler for notifications.
//	Does not throw.
//
extern "C" NotificationHandler 
setNotifierHandler( NotificationHandler fn )
{
	return notifierBuffer().setHandler( fn );
}

// ---------------------------------------------------------------------------
//	setDebuggerHandler
// ---------------------------------------------------------------------------
//	Specify a new handler for debugging notifications, only effective when
//	Debug_Loris is defined, otherwise debugger does nothing.
//	Does not throw.
//
extern "C" NotificationHandler 
setDebuggerHandler( NotificationHandler fn )
{
#if defined( Debug_Loris )
	return debuggerBuffer().setHandler( fn );
#else
	fn = fn;
	return NULL;
#endif
}

}	//	end of namespace Loris

