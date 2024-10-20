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
 *	lorisBpEnvelope_pi.C
 *
 *	A component of the C-linkable procedural interface for Loris. 
 *
 *    Main components of this interface:
 *    - version identification symbols
 *    - type declarations
 *    - Analyzer configuration
 *    - LinearEnvelope (formerly BreakpointEnvelope) operations
 *    - PartialList operations
 *    - Partial operations
 *    - Breakpoint operations
 *    - sound modeling functions for preparing PartialLists
 *    - utility functions for manipulating PartialLists
 *    - notification and exception handlers (all exceptions must be caught and
 *        handled internally, clients can specify an exception handler and 
 *        a notification function. The default one in Loris uses printf()).
 *
 *	This file defines the procedural interface for the Loris 
 *	BreakpointEnvelope class.
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

#include "loris.h"
#include "lorisException_pi.h"

#include "LinearEnvelope.h"
#include "Notifier.h"

using namespace Loris;

/* ---------------------------------------------------------------- */
/*		LinearEnvelope object interface								
/*
/*	A LinearEnvelope represents a linear segment breakpoint 
	function with infinite extension at each end (that is, the 
	values past either end of the breakpoint function have the 
	values at the nearest end).
 */
 
/* ---------------------------------------------------------------- */
/*        createLinearEnvelope
/*
/*	Construct and return a new LinearEnvelope having no 
	breakpoints and an implicit value of 0. everywhere, 
	until the first breakpoint is inserted.			
 */
extern "C"
LinearEnvelope * createLinearEnvelope( void )
{
	try 
	{
		debugger << "creating LinearEnvelope" << endl;
		return new LinearEnvelope();
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in createLinearEnvelope(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in createLinearEnvelope(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	return NULL;
}

/* ---------------------------------------------------------------- */
/*        copyLinearEnvelope
/*
/*	Construct and return a new LinearEnvelope that is an
	exact copy of the specified LinearEnvelopes, having 
	an identical set of breakpoints.	
 */
extern "C"
LinearEnvelope * copyLinearEnvelope( const LinearEnvelope * ptr_this )
{
	try 
	{
		debugger << "copying LinearEnvelope" << endl;
		return new LinearEnvelope( *ptr_this );
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in copyLinearEnvelope(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in copyLinearEnvelope(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	return NULL;
}

/* ---------------------------------------------------------------- */
/*        destroyLinearEnvelope       
/*
/*	Destroy this LinearEnvelope. 								
 */
extern "C"
void destroyLinearEnvelope( LinearEnvelope * ptr_this )
{
	try 
	{
		ThrowIfNull((LinearEnvelope *) ptr_this);
		
		debugger << "deleting LinearEnvelope" << endl;
		delete ptr_this;
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in destroyLinearEnvelope(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in destroyLinearEnvelope(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
}

/* ---------------------------------------------------------------- */
/*        linearEnvelope_insertBreakpoint      
/*
/*	Insert a breakpoint representing the specified (time, value) 
	pair into this LinearEnvelope. If there is already a 
	breakpoint at the specified time, it will be replaced with 
	the new breakpoint.
 */
extern "C"
void linearEnvelope_insertBreakpoint( LinearEnvelope * ptr_this,
                                      double time, double val )
{
	try 
	{
		ThrowIfNull((LinearEnvelope *) ptr_this);
		
		debugger << "inserting point (" << time << ", " << val 
				   << ") into LinearEnvelope" << endl;
		ptr_this->insertBreakpoint(time, val);
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in linearEnvelope_insertBreakpoint(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in linearEnvelope_insertBreakpoint(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
}

/* ---------------------------------------------------------------- */
/*        linearEnvelope_valueAt     
/*
/*	Return the interpolated value of this LinearEnvelope at the 
	specified time.							
 */
extern "C"
double linearEnvelope_valueAt( const LinearEnvelope * ptr_this, 
                               double time )
{
	try 
	{
		ThrowIfNull((LinearEnvelope *) ptr_this);
		return ptr_this->valueAt(time);
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in linearEnvelope_valueAt(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in linearEnvelope_valueAt(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	return 0;
}

