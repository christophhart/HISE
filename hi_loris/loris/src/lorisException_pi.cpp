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
 *	lorisException_pi.C
 *
 *	A component of the C-linkable procedural interface for Loris. 
 *
 *	Main components of the Loris procedural interface:
 *	- object interfaces - Analyzer, Synthesizer, Partial, PartialIterator, 
 *		PartialList, PartialListIterator, Breakpoint, BreakpointEnvelope,  
 *		and SampleVector need to be (opaque) objects in the interface, 
 * 		either because they hold state (e.g. Analyzer) or because they are 
 *		fundamental data types (e.g. Partial), so they need a procedural 
 *		interface to their member functions. All these things need to be 
 *		opaque pointers for the benefit of C.
 *	- non-object-based procedures - other classes in Loris are not so stateful,
 *		and have sufficiently narrow functionality that they need only 
 *		procedures, and no object representation.
 *	- utility functions - some procedures that are generally useful but are
 *		not yet part of the Loris core are also defined.
 *	- notification and exception handlers - all exceptions must be caught and
 *		handled internally, clients can specify an exception handler and 
 *		a notification function (the default one in Loris uses printf()).
 *
 *	This file defines the exception and notification handling functions 
 *	used in the Loris procedural interface.
 *
 * Kelly Fitz, 10 Nov 2000
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */
#if HAVE_CONFIG_H
	#include "config.h"
#endif

#include "lorisException_pi.h"


#include "loris.h"
#include "Notifier.h"

using namespace Loris;

/* ---------------------------------------------------------------- */
/*		notification and exception handlers							
/*
/*
	An exception handler and a notifier may be specified. Both 
	are functions taking a const char * argument and returning
	void.
 */

/* ---------------------------------------------------------------- */
/*        handleException        
/*
/*	Report exceptions thrown out of Loris. If no handler is 
	specified, report them using Loris' notifier and return
	without further incident.
 */
static void(*ex_handler)(const char *) = NULL;
void handleException( const char * s )
{
	if ( ex_handler )
		ex_handler( s );
	else
		notifier << s << endl;
}

/* ---------------------------------------------------------------- */
/*        setExceptionHandler        
/*
/*	Specify a function to call when reporting exceptions. The 
	function takes a const char * argument, and returns void.
 */
extern "C" 
void setExceptionHandler( void(*f)(const char *) )
{
	ex_handler = f;
}

/* ---------------------------------------------------------------- */
/*        setNotifier
/*                                                                  */
/*	Specify a notification function. The function takes a 
	const char * argument, and returns void.
 */
extern "C" 
void setNotifier( void(*f)(const char *) )
{
	//	these are guaranteed not to throw:
	setNotifierHandler( f );
	setDebuggerHandler( f );
}

