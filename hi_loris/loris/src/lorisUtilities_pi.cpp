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
 *	lorisUtilities_pi.C
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
 *	This file defines the utility functions that are useful, and in many 
 *	cases trivial in C++ (usign the STL for example) but are not represented
 *	by classes in the Loris core.
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

#include "BreakpointEnvelope.h"
#include "LorisExceptions.h"
#include "Notifier.h"
#include "Partial.h"
#include "PartialUtils.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <iterator>
#include <list>
#include <memory>

using namespace Loris;

// ---------------------------------------------------------------------------
//	Functors to help apply C-callbacks to lists of Partials
//	by converting Partial references to pointer arguments.
//

struct CallWithPointer
{
	typedef void (* Func)( Partial *, void * );
	Func func;
	void * data;
	
	CallWithPointer( Func f, void * d ) : func( f ), data( d ) {}
	
	void operator()( Partial & partial ) const
	{
		func( &partial, data );
	}
};

struct PredWithPointer
{
	typedef int (* Pred)( const Partial *, void * );
	Pred pred;
	void * data;
	
	PredWithPointer( Pred p, void * d ) : pred( p ), data( d ) {}
	
	bool operator()( const Partial & partial ) const
	{
		return 0 != pred( &partial, data );
	}
};

/* ---------------------------------------------------------------- */
/*		utility functions
/*
/*	Operations for transforming and manipulating collections
   of Partials.
 */

/* ---------------------------------------------------------------- */
/*        avgAmplitude 
/*       
/*  Return the average amplitude over all Breakpoints in this Partial.
    Return zero if the Partial has no Breakpoints.
 */
extern "C"
double avgAmplitude( const Partial * p )
{
    try
    {
        ThrowIfNull((Partial *) p);
        return PartialUtils::avgAmplitude( *p );
    }
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in avgAmplitude(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in avgAmplitude(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}    

    return 0;
}

/* ---------------------------------------------------------------- */
/*        avgFrequency 
/*       
/*  Return the average frequency over all Breakpoints in this Partial.
    Return zero if the Partial has no Breakpoints.
 */
extern "C"
double avgFrequency( const Partial * p )
{
    try
    {
        ThrowIfNull((Partial *) p);
        return PartialUtils::avgFrequency( *p );
    }
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in avgFrequency(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in avgFrequency(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}    

    return 0;}

 
/* ---------------------------------------------------------------- */
/*        copyIf        
/*
/*	Append copies of Partials in the source PartialList satisfying the
	specified predicate to the destination PartialList. The source list
	is unmodified. The data parameter can be used to 
   supply extra user-defined data to the function. Pass 0 if no 
   additional data is needed.
 */
extern "C"
void copyIf( const PartialList * src, PartialList * dst, 
			    int ( * predicate )( const Partial * p, void * data ),
			    void * data )
{
	try 
	{
		ThrowIfNull((PartialList *) src);
		ThrowIfNull((PartialList *) dst);
		
		std::remove_copy_if( src->begin(), src->end(), std::back_inserter( *dst ),
							 std::not_fn( PredWithPointer( predicate, data ) ) );
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in copyIf(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in copyIf(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
}
			 
/* ---------------------------------------------------------------- */
/*        copyLabeled        
/*
/*	Append copies of Partials in the source PartialList having the
	specified label to the destination PartialList. The source list
	is unmodified.
 */
extern "C"
void copyLabeled( const PartialList * src, long label, PartialList * dst )
{
	try 
	{
		ThrowIfNull((PartialList *) src);
		ThrowIfNull((PartialList *) dst);
		
		std::remove_copy_if( src->begin(), src->end(), std::back_inserter( *dst ),
							 std::not_fn( PartialUtils::isLabelEqual(label) ) );
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in copyLabeled(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in copyLabeled(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
}
 
/* ---------------------------------------------------------------- */
/*        crop        
/*
/*	Trim Partials by removing Breakpoints outside a specified time span.
	Insert a Breakpoint at the boundary when cropping occurs. Remove
	any Partials that are left empty after cropping (Partials having no
	Breakpoints between t1 and t2).
 */
extern "C"
void crop( PartialList * partials, double t1, double t2 )
{
	try
	{
		ThrowIfNull((PartialList *) partials);

		notifier << "cropping " << partials->size() << " Partials" << endl;

		PartialUtils::crop( partials->begin(), partials->end(), t1, t2 );	
		
		//  remove empty Partials:
		PartialList::iterator it = partials->begin();
		while ( it != partials->end() )
		{
		    if ( 0 == it->numBreakpoints() )
		    {
		        it = partials->erase( it );
		    }
		    else
		    {
		        ++it;
		    }
		}
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in crop(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in crop(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
}

/* ---------------------------------------------------------------- */
/*        extractIf      
/*
/*	Remove Partials in the source PartialList satisfying the
	specified predicate from the source list and append them to
    the destination PartialList. The data parameter can be used to 
    supply extra user-defined data to the function. Pass 0 if no 
    additional data is needed.
 */
extern "C"
void extractIf( PartialList * src, PartialList * dst, 
                int ( * predicate )( const Partial * p, void * data ),
			 	    void * data )
{
	try 
	{
		ThrowIfNull((PartialList *) src);
		ThrowIfNull((PartialList *) dst);
        
        /*
		std::list< Partial >::iterator it = 
			std::stable_partition( src->begin(), src->end(), 
								   std::not1( PredWithPointer( predicate, data ) ) );
		
		stable_partition should work, but seems sometimes to hang, 
		especially when there are lots and lots of Partials. Do it
		efficiently by hand instead.
		
		dst->splice( dst->end(), *src, it, src->end() );
		*/
		std::list< Partial >::iterator it;
		for ( it = std::find_if( src->begin(), src->end(), PredWithPointer( predicate, data ) );
		      it != src->end(); 
		      it = std::find_if( it, src->end(), PredWithPointer( predicate, data ) ) )
	    {
	        //  the iterator passed to splice is a copy of it
	        //  before the increment, so it is not corrupted
	        //  by the splice, it is advanced to the next
	        //  position before the splice is performed.
	        dst->splice( dst->end(), *src, it++ );
	    }
		
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in extractIf(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in extractIf(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
}

/* ---------------------------------------------------------------- */
/*        extractLabeled        
/*
/*	Remove Partials in the source PartialList having the specified
    label from the source list and append them to the destination 
    PartialList.
 */
extern "C"
void extractLabeled( PartialList * src, long label, PartialList * dst )
{
	try 
	{
		ThrowIfNull((PartialList *) src);
		ThrowIfNull((PartialList *) dst);
    
        /*
		std::list< Partial >::iterator it = 
			std::stable_partition( src->begin(), src->end(), 
								        std::not1( PartialUtils::isLabelEqual(label) ) );
		
		stable_partition should work, but seems sometimes to hang, 
		especially when there are lots and lots of Partials. Do it
		efficiently by hand instead.
		
		dst->splice( dst->end(), *src, it, src->end() );
		*/
		std::list< Partial >::iterator it;
		for ( it = std::find_if( src->begin(), src->end(), PartialUtils::isLabelEqual(label) );
		      it != src->end(); 
		      it = std::find_if( it, src->end(), PartialUtils::isLabelEqual(label) ) )
	    {
	        //  the iterator passed to splice is a copy of it
	        //  before the increment, so it is not corrupted
	        //  by the splice, it is advanced to the next
	        //  position before the splice is performed.
	        dst->splice( dst->end(), *src, it++ );
	    }
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in extractLabeled(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in extractLabeled(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
}

/* ---------------------------------------------------------------- */
/*        fixPhaseAfter 
/*
/*  Recompute phases of all Breakpoints later than the specified 
    time so that the synthesized phases of those later Breakpoints 
    matches the stored phase, as long as the synthesized phase at 
    the specified time matches the stored (not recomputed) phase.
    
    Phase fixing is only applied to non-null (nonzero-amplitude) 
    Breakpoints, because null Breakpoints are interpreted as phase 
    reset points in Loris. If a null is encountered, its phase is 
    simply left unmodified, and future phases wil be recomputed 
    from that one.
 */
extern "C"
void fixPhaseAfter( PartialList * partials, double time )
{
	try 
	{
		ThrowIfNull((PartialList *) partials);
		PartialUtils::fixPhaseAfter( partials->begin(), partials->end(), time );
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in fixPhaseAfter(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in fixPhaseAfter(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
}
 
/* ---------------------------------------------------------------- */
/*        fixPhaseAt 
/*
/*  Recompute phases of all Breakpoints in a Partial
    so that the synthesized phases match the stored phases, 
    and the synthesized phase at (nearest) the specified
    time matches the stored (not recomputed) phase.
    
    Backward phase-fixing stops if a null (zero-amplitude) 
    Breakpoint is encountered, because nulls are interpreted as 
    phase reset points in Loris. If a null is encountered, the 
    remainder of the Partial (the front part) is fixed in the 
    forward direction, beginning at the start of the Partial. 
    Forward phase fixing is only applied to non-null 
    (nonzero-amplitude) Breakpoints. If a null is encountered, 
    its phase is simply left unmodified, and future phases wil be 
    recomputed from that one.
 */
extern "C"
void fixPhaseAt( PartialList * partials, double time )
{
	try 
	{
		ThrowIfNull((PartialList *) partials);
		PartialUtils::fixPhaseAt( partials->begin(), partials->end(), time );
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in fixPhaseAt(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in fixPhaseAt(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
}
 
/* ---------------------------------------------------------------- */
/*        fixPhaseBefore 
/*
/*  Recompute phases of all Breakpoints earlier than the specified 
    time so that the synthesized phases of those earlier Breakpoints 
    matches the stored phase, and the synthesized phase at the 
    specified time matches the stored (not recomputed) phase.

    Backward phase-fixing stops if a null (zero-amplitude) Breakpoint
    is encountered, because nulls are interpreted as phase reset 
    points in Loris. If a null is encountered, the remainder of the 
    Partial (the front part) is fixed in the forward direction, 
    beginning at the start of the Partial.
 */
extern "C"
void fixPhaseBefore( PartialList * partials, double time )
{
	try 
	{
		ThrowIfNull((PartialList *) partials);
		PartialUtils::fixPhaseBefore( partials->begin(), partials->end(), time );
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in fixPhaseBefore(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in fixPhaseBefore(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
}

/* ---------------------------------------------------------------- */
/*        fixPhaseBetween 
/*
    Fix the phase travel between two times by adjusting the
    frequency and phase of Breakpoints between those two times.
    
    This algorithm assumes that there is nothing interesting 
    about the phases of the intervening Breakpoints, and modifies 
    their frequencies as little as possible to achieve the correct 
    amount of phase travel such that the frequencies and phases at 
    the specified times match the stored values. The phases of all 
    the Breakpoints between the specified times are recomputed.
 */
extern "C"
void fixPhaseBetween( PartialList * partials, double tbeg, double tend )
{
	try 
	{
		ThrowIfNull((PartialList *) partials);
		PartialUtils::fixPhaseBetween( partials->begin(), partials->end(), 
		                               tbeg, tend );
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in fixPhaseBetween(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in fixPhaseBetween(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
}
 
 
/* ---------------------------------------------------------------- */
/*        fixPhaseForward
/*
/*  Recompute phases of all Breakpoints later than the specified 
    time so that the synthesized phases of those later Breakpoints 
    matches the stored phase, as long as the synthesized phase at 
    the specified time matches the stored (not recomputed) phase. 
    Breakpoints later than tend are unmodified.
    
    Phase fixing is only applied to non-null (nonzero-amplitude) 
    Breakpoints, because null Breakpoints are interpreted as phase 
    reset points in Loris. If a null is encountered, its phase is 
    simply left unmodified, and future phases wil be recomputed 
    from that one.
 */
extern "C"
void fixPhaseForward( PartialList * partials, double tbeg, double tend )
{
	try 
	{
		ThrowIfNull((PartialList *) partials);
		PartialUtils::fixPhaseForward( partials->begin(), partials->end(), 
		                               tbeg, tend );
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in fixPhaseForward(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in fixPhaseForward(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
}
 
 
/* ---------------------------------------------------------------- */
/*        forEachBreakpoint        
/*
/*	Apply a function to each Breakpoint in a Partial. The function
	is called once for each Breakpoint in the source Partial. The
	function may modify the Breakpoint (but should not otherwise attempt 
	to modify the Partial). The data parameter can be used to supply extra
	user-defined data to the function. Pass 0 if no additional data is needed.
	The function should return 0 if successful. If the function returns
	a non-zero value, then forEachBreakpoint immediately returns that value
	without applying the function to any other Breakpoints in the Partial.
	forEachBreakpoint returns zero if all calls to func return zero.
 */
int forEachBreakpoint( Partial * p,
		 			   int ( * func )( Breakpoint * p, double time, void * data ),
			 		   void * data )
{
	int result = 0;
	try 
	{
		ThrowIfNull((Partial *) p);

		Partial::iterator it;
		for ( it = p->begin(); 0 == result && it != p->end(); ++it )
		{
			result = func( &(it.breakpoint()), it.time(), data );
		}
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in forEachBreakpoint(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in forEachBreakpoint(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	
	return result;
}

			 			
/* ---------------------------------------------------------------- */
/*        forEachPartial        
/*
/*	Apply a function to each Partial in a PartialList. The function
	is called once for each Partial in the source PartialList. The
	function may modify the Partial (but should not attempt to modify
	the PartialList). The data parameter can be used to supply extra
	user-defined data to the function. Pass 0 if no additional data 
	is needed. The function should return 0 if successful. If the 
	function returns a non-zero value, then forEachPartial immediately 
	returns that value without applying the function to any other 
	Partials in the PartialList. forEachPartial returns zero if all 
	calls to func return zero.
 */
int forEachPartial( PartialList * src,
			 		int ( * func )( Partial * p, void * data ),
			 		void * data )
{
	int result = 0;
	try 
	{
		ThrowIfNull((PartialList *) src);
		
		PartialList::iterator it;
		for ( it = src->begin(); 0 == result && it != src->end(); ++it )
		{
			result = func( &(*it), data );
		}
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in forEachPartial(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in forEachPartial(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}

	return result;
}
			 		

/* ---------------------------------------------------------------- */
/*        peakAmplitude        
/*
/*  Return the maximum amplitude achieved by a Partial.
 */
extern "C"
double peakAmplitude( const Partial * p )
{
    try
    {
        ThrowIfNull((Partial *) p);
        return PartialUtils::peakAmplitude( *p );
    }
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in peakAmplitude(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in peakAmplitude(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}    

    return 0;
}

/* ---------------------------------------------------------------- */
/*        removeIf        
/*
/*	Remove from a PartialList all Partials satisfying the
	specified predicate. The data parameter can be used to 
    supply extra user-defined data to the function. Pass 0 if no 
    additional data is needed.
 */
extern "C"
void removeIf( PartialList * src, 
			      int ( * predicate )( const Partial * p, void * data ),
			      void * data )
{
	try 
	{
		ThrowIfNull((PartialList *) src);
		std::list< Partial >::iterator it = 
			std::remove_if( src->begin(), src->end(), 
							PredWithPointer( predicate, data ) );
		src->erase( it, src->end() );
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in removeIf(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in removeIf(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
}

			 
/* ---------------------------------------------------------------- */
/*        removeLabeled        
/*
/*	Remove from a PartialList all Partials having the specified label. 
 */
extern "C"
void removeLabeled( PartialList * src, long label )
{
	try 
	{
		ThrowIfNull((PartialList *) src);
		std::list< Partial >::iterator it = 
			std::remove_if( src->begin(), src->end(), 
							    PartialUtils::isLabelEqual( label ) );
		src->erase( it, src->end() );
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in removeLabeled(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in removeLabeled(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
}

/* ---------------------------------------------------------------- */
/*        scaleAmp        
/*
/*	Bad old name for scaleAmplitude.
 */
extern "C"
void scaleAmp( PartialList * partials, BreakpointEnvelope * ampEnv )
{
    scaleAmplitude( partials, ampEnv );
}

/* ---------------------------------------------------------------- */
/*        scaleAmplitude        
/*
/*	Scale the amplitude of the Partials in a PartialList according 
	to an envelope representing a time-varying amplitude scale value.
 */
extern "C"
void scaleAmplitude( PartialList * partials, BreakpointEnvelope * ampEnv )
{
	try
	{
		ThrowIfNull((PartialList *) partials);
		ThrowIfNull((BreakpointEnvelope *) ampEnv);

		notifier << "scaling amplitude of " << partials->size() << " Partials" << endl;

		PartialUtils::scaleAmplitude( partials->begin(), partials->end(), *ampEnv );
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in scaleAmplitude(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in scaleAmplitude(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
}

/* ---------------------------------------------------------------- */
/*        scaleBandwidth        
/*
/*	Scale the bandwidth of the Partials in a PartialList according 
	to an envelope representing a time-varying bandwidth scale value.
 */
extern "C"
void scaleBandwidth( PartialList * partials, BreakpointEnvelope * bwEnv )
{
	try
	{
		ThrowIfNull((PartialList *) partials);
		ThrowIfNull((BreakpointEnvelope *) bwEnv);

		notifier << "scaling bandwidth of " << partials->size() << " Partials" << endl;

		PartialUtils::scaleBandwidth( partials->begin(), partials->end(), *bwEnv );
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in scaleBandwidth(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in scaleBandwidth(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
}

/* ---------------------------------------------------------------- */
/*        setBandwidth        
/*
/*	Assign the bandwidth of the Partials in a PartialList according 
	to an envelope representing a time-varying bandwidth scale value.
 */
extern "C"
void setBandwidth( PartialList * partials, BreakpointEnvelope * bwEnv )
{
	try
	{
		ThrowIfNull((PartialList *) partials);
		ThrowIfNull((BreakpointEnvelope *) bwEnv);

		notifier << "setting bandwidth of " << partials->size() << " Partials" << endl;

		PartialUtils::setBandwidth( partials->begin(), partials->end(), *bwEnv );
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in setBandwidth(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in setBandwidth(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
}

/* ---------------------------------------------------------------- */
/*        scaleFrequency        
/*
/*	Scale the frequency of the Partials in a PartialList according 
	to an envelope representing a time-varying frequency scale value.
 */
extern "C"
void scaleFrequency( PartialList * partials, BreakpointEnvelope * freqEnv )
{
	try
	{
		ThrowIfNull((PartialList *) partials);
		ThrowIfNull((BreakpointEnvelope *) freqEnv);

		notifier << "scaling frequency of " << partials->size() << " Partials" << endl;

      	PartialUtils::scaleFrequency( partials->begin(), partials->end(), *freqEnv );
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in scaleFrequency(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in scaleFrequency(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
}

/* ---------------------------------------------------------------- */
/*        scaleNoiseRatio        
/*
/*	Scale the relative noise content of the Partials in a PartialList 
	according to an envelope representing a (time-varying) noise energy 
	scale value.
 */
extern "C"
void scaleNoiseRatio( PartialList * partials, BreakpointEnvelope * noiseEnv )
{
	try 
	{
		ThrowIfNull((PartialList *) partials);
		ThrowIfNull((BreakpointEnvelope *) noiseEnv);

		notifier << "scaling noise ratio of " << partials->size() << " Partials" << endl;

		PartialUtils::scaleNoiseRatio( partials->begin(), partials->end(), *noiseEnv );
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in scaleNoiseRatio(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in scaleNoiseRatio(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
}

/* ---------------------------------------------------------------- */
/*        shiftPitch        
/*
/*	Shift the pitch of all Partials in a PartialList according to 
	the given pitch envelope. The pitch envelope is assumed to have 
	units of cents (1/100 of a halfstep).
 */
extern "C"
void shiftPitch( PartialList * partials, BreakpointEnvelope * pitchEnv )
{
	try 
	{
		ThrowIfNull((PartialList *) partials);
		ThrowIfNull((BreakpointEnvelope *) pitchEnv);

		notifier << "shifting pitch of " << partials->size() << " Partials" << endl;
		
		PartialUtils::shiftPitch( partials->begin(), partials->end(), *pitchEnv );
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in shiftPitch(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in shiftPitch(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
}

/* ---------------------------------------------------------------- */
/*        shiftTime       
/*
/*	Shift the time of all the Breakpoints in all Partials in a 
	PartialList by a constant amount.
 */
extern "C"
void shiftTime( PartialList * partials, double offset )
{
	try 
	{
		ThrowIfNull((PartialList *) partials);

		notifier << "shifting time of " << partials->size() << " Partials" << endl;
		
		PartialUtils::shiftTime( partials->begin(), partials->end(), offset );
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in shiftTime(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in shiftTime(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
}

/* ---------------------------------------------------------------- */
/*        sortByLabel        
/*
/*	Sort the Partials in a PartialList in order of increasing label.
 * 	The sort is stable; Partials having the same label are not 
 * 	reordered.
 */
extern "C"
void sortByLabel( PartialList * partials )
{
	partials->sort( PartialUtils::compareLabelLess() );	
}

/* ---------------------------------------------------------------- */
/*        timeSpan        
/*
/*	Return the minimum start time and maximum end time
 *  in seconds of all Partials in this PartialList. The v
 *  times are returned in the (non-null) pointers tmin
 *  and tmax.
 */
extern "C"
void timeSpan( PartialList * partials, double * tmin, double * tmax )
{
    std::pair< double, double > times = 
        PartialUtils::timeSpan( partials->begin(), partials->end() );	
    if ( 0 != tmin )
    {
        *tmin = times.first;
    }
    if ( 0 != tmax )
    {
        *tmax = times.second;
    }
}

/* ---------------------------------------------------------------- */
/*        weightedAvgFrequency 
/*       
/*  Return the average frequency over all Breakpoints in this Partial, 
    weighted by the Breakpoint amplitudes. Return zero if the Partial 
    has no Breakpoints.
 */
extern "C"
double weightedAvgFrequency( const Partial * p )
{
    try
    {
        ThrowIfNull((Partial *) p);
        return PartialUtils::weightedAvgFrequency( *p );
    }
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in weightedAvgFrequency(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in weightedAvgFrequency(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}    

    return 0;
}
