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
 *	lorisPartialList_pi.C
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
 *	This file contains the procedural interface for the Loris 
 *	PartialList (std::list< Partial >) class.
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

#include "Partial.h"
#include "Notifier.h"

#include <list>

using namespace Loris;


/* ---------------------------------------------------------------- */
/*		PartialList object interface								
/*
/*	A PartialList represents a collection of Bandwidth-Enhanced 
	Partials, each having a trio of synchronous, non-uniformly-
	sampled breakpoint envelopes representing the time-varying 
	frequency, amplitude, and noisiness of a single bandwidth-
	enhanced sinusoid.

	For more information about Bandwidth-Enhanced Partials and the  
	Reassigned Bandwidth-Enhanced Additive Sound Model, refer to
	the Loris website: www.cerlsoundgroup.org/Loris/.

	In C++, a PartialList * is a Loris::PartialList *.
 */ 

/* ---------------------------------------------------------------- */
/*        createPartialList        
/*
/*	Return a new empty PartialList.
 */
extern "C"
PartialList * createPartialList( void )
{
	try 
	{
		debugger << "creating empty PartialList" << endl;
		return new std::list< Partial >;
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in createPartialList(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in createPartialList(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	return NULL;
}

/* ---------------------------------------------------------------- */
/*        destroyPartialList        
/*
/*	Destroy this PartialList.
 */
extern "C"
void destroyPartialList( PartialList * ptr_this )
{
	try 
	{
		ThrowIfNull((PartialList *) ptr_this);

		debugger << "deleting PartialList containing " << ptr_this->size() << " Partials" << endl;
		delete ptr_this;
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in destroyPartialList(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in destroyPartialList(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
}

/* ---------------------------------------------------------------- */
/*        partialList_clear        
/*
/*	Remove (and destroy) all the Partials from this PartialList,
	leaving it empty.
 */
extern "C"
void partialList_clear( PartialList * ptr_this )
{
	try 
	{
		ThrowIfNull((PartialList *) ptr_this);
		ptr_this->clear();
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in partialList_clear(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in partialList_clear(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
}

/* ---------------------------------------------------------------- */
/*        partialList_copy        
/*
/*	Make this PartialList a copy of the source PartialList by making
	copies of all of the Partials in the source and adding them to 
	this PartialList.
 */
extern "C"
void partialList_copy( PartialList * dst, const PartialList * src )
{
	try 
	{
		ThrowIfNull((PartialList *) dst);
		ThrowIfNull((PartialList *) src);

		debugger << "copying PartialList containing " << src->size() << " Partials" << endl;
		*dst = *src;
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in partialList_copy(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in partialList_copy(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
}

/* ---------------------------------------------------------------- */
/*        partialList_size        
/*
/*	Return the number of Partials in this PartialList.
 */
extern "C"
unsigned long partialList_size( const PartialList * ptr_this )
{
	try 
	{
		ThrowIfNull((PartialList *) ptr_this);
		return ptr_this->size();
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in partialList_size(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in partialList_size(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	return 0;
}

/* ---------------------------------------------------------------- */
/*        partialList_splice        
/*
/*	Splice all the Partials in the source PartialList onto the end of
	this PartialList, leaving the source empty.
 */
extern "C"
void partialList_splice( PartialList * dst, PartialList * src )
{
	try 
	{
		ThrowIfNull((PartialList *) dst);
		ThrowIfNull((PartialList *) src);

		debugger << "splicing PartialList containing " << src->size() << " Partials" 
				 << " into PartialList containing " << dst->size() << " Partials"<< endl;
		dst->splice( dst->end(), *src );
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in partialList_splice(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in partialList_splice(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
}

/* ---------------------------------------------------------------- */
/*		Partial object interface
/*
/*	A Partial represents a single component in the
	reassigned bandwidth-enhanced additive model. A Partial consists of a
	chain of Breakpoints describing the time-varying frequency, amplitude,
	and bandwidth (or noisiness) envelopes of the component, and a 4-byte
	label. The Breakpoints are non-uniformly distributed in time. For more
	information about Reassigned Bandwidth-Enhanced Analysis and the
	Reassigned Bandwidth-Enhanced Additive Sound Model, refer to the Loris
	website: www.cerlsoundgroup.org/Loris/.
 */ 

/* ---------------------------------------------------------------- */
/*        partial_startTime        
/*
/*	Return the start time (seconds) for the specified Partial.
 */
double partial_startTime( const Partial * p )
{
    double ret = 0;
	try 
	{
		ThrowIfNull((Partial *) p);

		ret = p->startTime();
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in partial_startTime(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in partial_startTime(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	return ret;
}

/* ---------------------------------------------------------------- */
/*        partial_endTime        
/*
/*	Return the end time (seconds) for the specified Partial.
 */
double partial_endTime( const Partial * p )
{
    double ret = 0;
	try 
	{
		ThrowIfNull((Partial *) p);

		ret = p->endTime();
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in partial_endTime(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in partial_endTime(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	return ret;
}

/* ---------------------------------------------------------------- */
/*        partial_duration        
/*
/*	Return the duration (seconds) for the specified Partial.
 */
double partial_duration( const Partial * p )
{
    double ret = 0;
	try 
	{
		ThrowIfNull((Partial *) p);

		ret = p->duration();
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in partial_duration(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in partial_duration(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	return ret;
}

/* ---------------------------------------------------------------- */
/*        partial_initialPhase        
/*
/*	Return the initial phase (radians) for the specified Partial.
 */
double partial_initialPhase( const Partial * p )
{
    double ret = 0;
	try 
	{
		ThrowIfNull((Partial *) p);

		ret = p->initialPhase();
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in partial_initialPhase(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in partial_initialPhase(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	return ret;
}

/* ---------------------------------------------------------------- */
/*        partial_label        
/*
/*	Return the integer label for the specified Partial.
 */
int partial_label( const Partial * p )
{
    int ret = 0;
	try 
	{
		ThrowIfNull((Partial *) p);

		ret = p->label();
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in partial_label(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in partial_label(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	return ret;
}

/* ---------------------------------------------------------------- */
/*        partial_numBreakpoints        
/*
/*	Return the number of Breakpoints in the specified Partial.
 */
unsigned long partial_numBreakpoints( const Partial * p )
{
    unsigned long ret = 0;
	try 
	{
		ThrowIfNull((Partial *) p);

		ret = p->size();
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in partial_numBreakpoints(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in partial_numBreakpoints(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	return ret;
}

/* ---------------------------------------------------------------- */
/*        partial_frequencyAt        
/*
/*	Return the frequency (Hz) of the specified Partial interpolated
	at a particular time. It is an error to apply this function to
	a Partial having no Breakpoints.
 */
double partial_frequencyAt( const Partial * p, double t )
{
    double ret = 0;
	try 
	{
		ThrowIfNull((Partial *) p);

		ret = p->frequencyAt( t );
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in partial_frequencyAt(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in partial_frequencyAt(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	return ret;
}

/* ---------------------------------------------------------------- */
/*        partial_bandwidthAt        
/*
/*	Return the bandwidth of the specified Partial interpolated
	at a particular time. It is an error to apply this function to
	a Partial having no Breakpoints.
 */
double partial_bandwidthAt( const Partial * p, double t )
{
    double ret = 0;
	try 
	{
		ThrowIfNull((Partial *) p);

		ret = p->bandwidthAt( t );
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in partial_bandwidthAt(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in partial_bandwidthAt(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	return ret;
}

/* ---------------------------------------------------------------- */
/*        partial_phaseAt        
/*
/*	Return the phase (radians) of the specified Partial interpolated
	at a particular time. It is an error to apply this function to
	a Partial having no Breakpoints.
 */
double partial_phaseAt( const Partial * p, double t )
{
    double ret = 0;
	try 
	{
		ThrowIfNull((Partial *) p);

		ret = p->phaseAt( t );
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in partial_phaseAt(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in partial_phaseAt(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	return ret;
}

/* ---------------------------------------------------------------- */
/*        partial_amplitudeAt        
/*
/*	Return the (absolute) amplitude of the specified Partial interpolated
	at a particular time. Partials are assumed to fade out
	over 1 millisecond at the ends (rather than instantaneously).
	It is an error to apply this function to a Partial having no Breakpoints.
 */
double partial_amplitudeAt( const Partial * p, double t )
{
    double ret = 0;
	try 
	{
		ThrowIfNull((Partial *) p);

		ret = p->amplitudeAt( t, 0.001 );
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in partial_amplitudeAt(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in partial_amplitudeAt(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	return ret;
}

/* ---------------------------------------------------------------- */
/*        partial_setLabel        
/*
/* 	Assign a new integer label to the specified Partial.
 */
void partial_setLabel( Partial * p, int label )
{
	try 
	{
		ThrowIfNull((Partial *) p);

		p->setLabel( label );
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in partial_setLabel(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in partial_setLabel(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
}

/* ---------------------------------------------------------------- */
/*		Breakpoint object interface
/*
/*	A Breakpoint represents a single breakpoint in the
	Partial parameter (frequency, amplitude, bandwidth) envelope.
	Instantaneous phase is also stored, but is only used at the onset of 
	a partial, or when it makes a transition from zero to nonzero amplitude.
	
	Loris Partials represent reassigned bandwidth-enhanced model components.
	A Partial consists of a chain of Breakpoints describing the time-varying
	frequency, amplitude, and bandwidth (noisiness) of the component.
	For more information about Reassigned Bandwidth-Enhanced 
	Analysis and the Reassigned Bandwidth-Enhanced Additive Sound 
	Model, refer to the Loris website: 
	www.cerlsoundgroup.org/Loris/.
 */ 

/* ---------------------------------------------------------------- */
/*        breakpoint_getFrequency        
/*
/*	Return the frequency (Hz) of the specified Breakpoint.
 */
double breakpoint_getFrequency( const Breakpoint * bp )
{
    double ret = 0;
	try 
	{
		ThrowIfNull((Breakpoint *) bp);

		ret = bp->frequency();
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in breakpoint_getFrequency(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in breakpoint_getFrequency(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	return ret;
}

/* ---------------------------------------------------------------- */
/*        breakpoint_getAmplitude        
/*
/* 	Return the (absolute) amplitude of the specified Breakpoint.
 */
double breakpoint_getAmplitude( const Breakpoint * bp )
{
    double ret = 0;
	try 
	{
		ThrowIfNull((Breakpoint *) bp);

		ret = bp->amplitude();
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in breakpoint_getAmplitude(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in breakpoint_getAmplitude(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	return ret;
}

/* ---------------------------------------------------------------- */
/*        breakpoint_getBandwidth        
/*
/*	Return the bandwidth coefficient of the specified Breakpoint.
 */
double breakpoint_getBandwidth( const Breakpoint * bp )
{
    double ret = 0;
	try 
	{
		ThrowIfNull((Breakpoint *) bp);

		ret = bp->bandwidth();
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in breakpoint_getBandwidth(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in breakpoint_getBandwidth(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	return ret;
}

/* ---------------------------------------------------------------- */
/*        breakpoint_getPhase        
/*
/*	Return the phase (radians) of the specified Breakpoint.
 */
double breakpoint_getPhase( const Breakpoint * bp )
{
    double ret = 0;
	try 
	{
		ThrowIfNull((Breakpoint *) bp);

		ret = bp->phase();
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in breakpoint_getPhase(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in breakpoint_getPhase(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	return ret;
}

/* ---------------------------------------------------------------- */
/*        breakpoint_setFrequency        
/*
/*	Assign a new frequency (Hz) to the specified Breakpoint.
 */
void breakpoint_setFrequency( Breakpoint * bp, double f )
{
	try 
	{
		ThrowIfNull((Breakpoint *) bp);

		bp->setFrequency( f );
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in breakpoint_setFrequency(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in breakpoint_setFrequency(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
}

/* ---------------------------------------------------------------- */
/*        breakpoint_setAmplitude        
/*
/* 	Assign a new (absolute) amplitude to the specified Breakpoint.
 */
void breakpoint_setAmplitude( Breakpoint * bp, double a )
{
	try 
	{
		ThrowIfNull((Breakpoint *) bp);

		bp->setAmplitude( a );
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in breakpoint_setAmplitude(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in breakpoint_setAmplitude(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
}

/* ---------------------------------------------------------------- */
/*        breakpoint_setBandwidth        
/*
/*	Assign a new bandwidth coefficient to the specified Breakpoint.
 */
void breakpoint_setBandwidth( Breakpoint * bp, double bw )
{
	try 
	{
		ThrowIfNull((Breakpoint *) bp);

		bp->setBandwidth( bw );
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in breakpoint_setBandwidth(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in breakpoint_setBandwidth(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
}

/* ---------------------------------------------------------------- */
/*        breakpoint_setPhase        
/*
/*	Assign a new phase (radians) to the specified Breakpoint.
 */
void breakpoint_setPhase( Breakpoint * bp, double phi )
{
	try 
	{
		ThrowIfNull((Breakpoint *) bp);

		bp->setPhase( phi );
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in breakpoint_setPhase(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in breakpoint_setPhase(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
}
