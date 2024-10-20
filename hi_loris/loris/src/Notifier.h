#ifndef INCLUDE_NOTIFIER_H
#define INCLUDE_NOTIFIER_H
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
 * Notifier.h
 *
 *	A pair of dedicated streams, notifier and debugger, are used for
 *	notification throughout the Loris class library. These streams are used
 *	like cout or cerr, but they buffer their contents until a newline is
 *	receieved. Then they post their entire contents to a notification
 *	handler. The default handler just prints to stderr, but other handlers
 *	may be dynamically specified using setNotifierHandler() and
 *	setDebuggerHandler().
 *	
 *	debugger is enabled only when compiled with the preprocessor macro
 *	Debug_Loris defined. It cannot be enabled using setDebuggerHandler() if
 *	Debug_Loris is undefined.When Debug_Loris is not defined, characters
 *	streamed onto debugger are never posted nor are they otherwise
 *	accessible.
 *	
 *	Notifier.h may be included in c files. The stream declarations are
 *	omitted, but the notification handler routines are accessible.
 *	
 *
 * Kelly Fitz, 28 Feb 2000
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */


/*
 *	stream declaration, C++ only:
 */
#ifdef __cplusplus

#include <iostream>

//	begin namespace
namespace Loris {

std::ostream & getNotifierStream(void);
std::ostream & getDebuggerStream(void);

//	declare streams:
static std::ostream & notifier = getNotifierStream();
/*	This stream is used throughout Loris (and may be used by clients)
	to provide user feedback. Characters streamed onto notifier are
	buffered until a newline is received, and then the entire contents
	of the stream are flushed to the current notification handler (stderr,
	by default).
 */

static std::ostream & debugger = getDebuggerStream();
/*	This stream is used throughout Loris (and may be used by clients)
	to provide debugging information. Characters streamed onto debugger are
	buffered until a newline is received, and then the entire contents
	of the stream are flushed to the current debugger handler (stderr,
	by default).
	
	debugger is enabled only when compiled with the preprocessor macro
	Debug_Loris defined. It cannot be enabled using setDebuggerHandler()
	if Debug_Loris is undefined. When Debug_Loris is not defined,
	characters streamed onto debugger are never posted nor are they
	otherwise accessible.
 */
 
//	for convenience, import endl and ends from std into Loris:
using std::endl;
using std::ends;

}	//	end of namespace Loris

#endif	/* def __cplusplus */

/*
 *	handler assignment, c linkable:
 */

#ifdef __cplusplus
//  begin namespace
namespace Loris {
extern "C" {
#endif	//	def __cplusplus

//	These functions do not throw exceptions.
typedef void(*NotificationHandler)(const char * s);
NotificationHandler setNotifierHandler( NotificationHandler fn );
/*	Specify a new handling procedure for posting user feedback, and return
	the current handler. 
 */
 
NotificationHandler setDebuggerHandler( NotificationHandler fn );
/*	Specify a new handling procedure for posting debugging information, and return
	the current handler. This has no effect unless compiled with the Debug_Loris
	preprocessor macro defined.
 */
 
#ifdef __cplusplus
}	//	end extern "C"
}	//	end of namespace Loris
#endif	// def __cplusplus


#endif /* ndef INCLUDE_NOTIFIER_H */
