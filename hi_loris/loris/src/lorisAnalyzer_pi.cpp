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
 *	lorisAnalyzer_pi.C
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
 *	This file contains the procedural interface for the Loris Analyzer class.
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

#include "Analyzer.h"
#include "Notifier.h"

using namespace Loris;

/* ---------------------------------------------------------------- */
/*		Analyzer object interface
/*
/*	An Analyzer represents a configuration of parameters for
	performing Reassigned Bandwidth-Enhanced Additive Analysis
	of sampled waveforms. This analysis process yields a collection 
	of Partials, each having a trio of synchronous, non-uniformly-
	sampled breakpoint envelopes representing the time-varying 
	frequency, amplitude, and noisiness of a single bandwidth-
	enhanced sinusoid. 

	For more information about Reassigned Bandwidth-Enhanced 
	Analysis and the Reassigned Bandwidth-Enhanced Additive Sound 
	Model, refer to the Loris website: www.cerlsoundgroup.org/Loris/.

	In the procedural interface, there is only one Analyzer. 
   It must be configured by calling analyzer_configure before
   any of the other analyzer operations can be performed.
 */
static Analyzer * ptr_instance = 0;

/* ---------------------------------------------------------------- */
/*        analyzer_configure
/*
/*	Configure the sole Analyzer instance with the specified
	frequency resolution (minimum instantaneous frequency	
	difference between Partials). All other Analyzer parameters 	
	are computed from the specified frequency resolution. 
   
  	Construct the Analyzer instance if necessary.
   
	In the procedural interface, there is only one Analyzer. 
   	It must be configured by calling analyzer_configure before
   	any of the other analyzer operations can be performed.   
 */
extern "C"
void analyzer_configure( double resolution, double windowWidth, void* tc)
{
	try 
	{
      if ( 0 == ptr_instance )
      {
         debugger << "creating Analyzer" << endl;         
         ptr_instance = new Analyzer( resolution, windowWidth );
      }
      else
      {
         debugger << "configuring Analyzer" << endl;         
         ptr_instance->configure( resolution, windowWidth );
      }

	  if (tc != nullptr)
	  {
		  ptr_instance->threadController = static_cast<hise::ThreadController*>(tc);
	  }
	  
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in createAnalyzer(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in createAnalyzer(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
}

/* ---------------------------------------------------------------- */
/*        analyze        
/*
/*	Analyze an array of bufferSize (mono) samples at the given 
	sample rate (in Hz) and append the extracted Partials to the 
	given PartialList. 							
   
   	analyzer_configure must be called before any other analyzer 
   	function.
 */
extern "C"
void analyze( const double * buffer, unsigned int bufferSize, 
              double srate, PartialList * partials )
{
	if ( 0 == ptr_instance )
	{
		handleException( "analyzer_configure must be called before any other analyzer function." );
		return;
	}

	try 
	{
		ThrowIfNull((double *) buffer);
		ThrowIfNull((PartialList *) partials);
		
		//	perform analysis:
		notifier << "analyzing " << bufferSize << " samples at " <<
					srate << " Hz with frequency resolution " <<
					ptr_instance->freqResolution() << endl;
		if ( bufferSize > 0 )
		{
			ptr_instance->analyze( buffer, buffer + bufferSize, srate );
		
			//	splice the Partials into the destination list:
			partials->splice( partials->end(), ptr_instance->partials() );
		}
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in analyze(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in analyze(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
}

/* ---------------------------------------------------------------- */
/*        analyzer_getFreqResolution        
/*
/*	Return the frequency resolution (minimum instantaneous frequency  		
	difference between Partials) for this Analyzer. 	

	analyzer_configure must be called before any other analyzer 
	function.
 */
extern "C"
double analyzer_getFreqResolution( void )
{
	if ( 0 == ptr_instance )
	{
		handleException( "analyzer_configure must be called before any other analyzer function." );
		return 0;
	}

	try  
	{
		return ptr_instance->freqResolution();
	}
	catch( Exception & ex )  
	{
		std::string s("Loris exception in analyzer_getFreqResolution(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex )  
	{
		std::string s("std C++ exception in analyzer_getFreqResolution(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	return 0;
}

/* ---------------------------------------------------------------- */
/*        analyzer_setFreqResolution        
/*
/*	Set the frequency resolution (minimum instantaneous frequency  		
	difference between Partials) for this Analyzer. (Does not cause 	
	other parameters to be recomputed.) 									

	analyzer_configure must be called before any other analyzer 
	function.
 */
extern "C"
void analyzer_setFreqResolution( double x )
{
	if ( 0 == ptr_instance )
	{
		handleException( "analyzer_configure must be called before any other analyzer function." );
		return;
	}

	try  
	{
		ptr_instance->setFreqResolution( x );
	}
	catch( Exception & ex )  
	{
		std::string s("Loris exception in analyzer_setFreqResolution(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex )  
	{
		std::string s("std C++ exception in analyzer_setFreqResolution(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
}

/* ---------------------------------------------------------------- */
/*        analyzer_getAmpFloor        
/*
/*	Return the amplitude floor (lowest detected spectral amplitude),  			
	in (negative) dB, for this Analyzer. 				

	analyzer_configure must be called before any other analyzer 
	function.
 */
extern "C"
double analyzer_getAmpFloor( void )
{
	if ( 0 == ptr_instance )
	{
		handleException( "analyzer_configure must be called before any other analyzer function." );
		return 0;
	}

	try  
	{
		return ptr_instance->ampFloor();
	}
	catch( Exception & ex )  
	{
		std::string s("Loris exception in analyzer_getAmpFloor(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex )  
	{
		std::string s("std C++ exception in analyzer_getAmpFloor(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	return 0;
}

/* ---------------------------------------------------------------- */
/*        analyzer_setAmpFloor        
/*
/*	Set the amplitude floor (lowest detected spectral amplitude), in  			
	(negative) dB, for this Analyzer. 				

	analyzer_configure must be called before any other analyzer 
	function.
 */
extern "C"
void analyzer_setAmpFloor( double x )
{
	if ( 0 == ptr_instance )
	{
		handleException( "analyzer_configure must be called before any other analyzer function." );
		return;
	}

	try  
	{
		ptr_instance->setAmpFloor( x );
	}
	catch( Exception & ex )  
	{
		std::string s("Loris exception in analyzer_setAmpFloor(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex )  
	{
		std::string s("std C++ exception in analyzer_setAmpFloor(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
}

/* ---------------------------------------------------------------- */
/*        analyzer_getWindowWidth        
/*
/*	Return the frequency-domain main lobe width (measured between 
	zero-crossings) of the analysis window used by this Analyzer. 				

	analyzer_configure must be called before any other analyzer 
	function.
 */
extern "C"
double analyzer_getWindowWidth( void )
{
	if ( 0 == ptr_instance )
	{
		handleException( "analyzer_configure must be called before any other analyzer function." );
		return 0;
	}

	try  
	{
		return ptr_instance->windowWidth();
	}
	catch( Exception & ex )  
	{
		std::string s("Loris exception in analyzer_getWindowWidth(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex )  
	{
		std::string s("std C++ exception in analyzer_getWindowWidth(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	return 0;
}

/* ---------------------------------------------------------------- */
/*        analyzer_setWindowWidth        
/*
/*	Set the frequency-domain main lobe width (measured between 
	zero-crossings) of the analysis window used by this Analyzer. 				

	analyzer_configure must be called before any other analyzer 
	function.
 */
extern "C"
void analyzer_setWindowWidth( double x )
{
	if ( 0 == ptr_instance )
	{
		handleException( "analyzer_configure must be called before any other analyzer function." );
		return;
	}

	try  
	{
		ptr_instance->setWindowWidth( x );
	}
	catch( Exception & ex )  
	{
		std::string s("Loris exception in analyzer_setWindowWidth(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex )  
	{
		std::string s("std C++ exception in analyzer_setWindowWidth(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
}

/* ---------------------------------------------------------------- */
/*        analyzer_getSidelobeLevel        
/*
/*	Return the sidelobe attenutation level for the Kaiser analysis window in
	negative dB. More negative numbers (e.g. -90) give very good sidelobe 
	rejection but cause the window to be longer in time. Less negative 
	numbers raise the level of the sidelobes, increasing the liklihood
	of frequency-domain interference, but allow the window to be shorter
	in time.

	analyzer_configure must be called before any other analyzer 
	function.
 */
extern "C"
double analyzer_getSidelobeLevel( void )
{
	if ( 0 == ptr_instance )
	{
		handleException( "analyzer_configure must be called before any other analyzer function." );
		return 0;
	}

	try  
	{
		return ptr_instance->sidelobeLevel();
	}
	catch( Exception & ex )  
	{
		std::string s("Loris exception in analyzer_getSidelobeLevel(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex )  
	{
		std::string s("std C++ exception in analyzer_getSidelobeLevel(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	return 0;
}

/* ---------------------------------------------------------------- */
/*        analyzer_setSidelobeLevel        
/*
/*	Set the sidelobe attenutation level for the Kaiser analysis window in
	negative dB. More negative numbers (e.g. -90) give very good sidelobe 
	rejection but cause the window to be longer in time. Less negative 
	numbers raise the level of the sidelobes, increasing the liklihood
	of frequency-domain interference, but allow the window to be shorter
	in time.

	analyzer_configure must be called before any other analyzer 
	function.
 */
extern "C"
void analyzer_setSidelobeLevel( double x )
{
	if ( 0 == ptr_instance )
	{
		handleException( "analyzer_configure must be called before any other analyzer function." );
		return;
	}

	try  
	{
		ptr_instance->setSidelobeLevel( x );
	}
	catch( Exception & ex )  
	{
		std::string s("Loris exception in analyzer_setSidelobeLevel(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex )  
	{
		std::string s("std C++ exception in analyzer_setSidelobeLevel(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
}

/* ---------------------------------------------------------------- */
/*        analyzer_getFreqFloor        
/*
/*	Return the frequency floor (minimum instantaneous Partial  				
	frequency), in Hz, for this Analyzer. 				
   
	analyzer_configure must be called before any other analyzer 
	function.
 */
extern "C"
double analyzer_getFreqFloor( void )
{
	if ( 0 == ptr_instance )
	{
		handleException( "analyzer_configure must be called before any other analyzer function." );
		return 0;
	}

	try  
	{
      return ptr_instance->freqFloor();
	}
	catch( Exception & ex )  
	{
		std::string s("Loris exception in analyzer_getFreqFloor(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex )  
	{
		std::string s("std C++ exception in analyzer_getFreqFloor(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	return 0;
}

/* ---------------------------------------------------------------- */
/*        analyzer_setFreqFloor        
/*
/*	Set the amplitude floor (minimum instantaneous Partial  				
	frequency), in Hz, for this Analyzer.
   
	analyzer_configure must be called before any other analyzer 
	function.
 */
extern "C"
void analyzer_setFreqFloor( double x )
{
	if ( 0 == ptr_instance )
	{
		handleException( "analyzer_configure must be called before any other analyzer function." );
		return;
	}

	try  
	{
		ptr_instance->setFreqFloor( x );
	}
	catch( Exception & ex )  
	{
		std::string s("Loris exception in analyzer_setFreqFloor(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex )  
	{
		std::string s("std C++ exception in analyzer_setFreqFloor(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
}

/* ---------------------------------------------------------------- */
/*        analyzer_getFreqDrift        
/*
/*	Return the maximum allowable frequency difference between 					
	consecutive Breakpoints in a Partial envelope for this Analyzer. 				
   
	analyzer_configure must be called before any other analyzer 
	function.
 */
extern "C"
double analyzer_getFreqDrift( void )
{
	if ( 0 == ptr_instance )
	{
		handleException( "analyzer_configure must be called before any other analyzer function." );
		return 0;
	}

	try  
	{
		return ptr_instance->freqDrift();
	}
	catch( Exception & ex )  
	{
		std::string s("Loris exception in analyzer_getFreqDrift(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex )  
	{
		std::string s("std C++ exception in analyzer_getFreqDrift(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	return 0;
}

/* ---------------------------------------------------------------- */
/*        analyzer_setFreqDrift        
/*
/*	Set the maximum allowable frequency difference between 					
	consecutive Breakpoints in a Partial envelope for this Analyzer. 				
   
	analyzer_configure must be called before any other analyzer 
	function.
 */
extern "C"
void analyzer_setFreqDrift( double x )
{
	if ( 0 == ptr_instance )
	{
		handleException( "analyzer_configure must be called before any other analyzer function." );
		return;
	}

	try  
	{
		ptr_instance->setFreqDrift( x );
	}
	catch( Exception & ex )  
	{
		std::string s("Loris exception in analyzer_setFreqDrift(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex )  
	{
		std::string s("std C++ exception in analyzer_setFreqDrift(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
}

/* ---------------------------------------------------------------- */
/*        analyzer_getHopTime        
/*
/*	Return the hop time (which corresponds approximately to the 
	average density of Partial envelope Breakpoint data) for this 
	Analyzer.
   
	analyzer_configure must be called before any other analyzer 
	function.
 */
extern "C"
double analyzer_getHopTime( void )
{
	if ( 0 == ptr_instance )
	{
		handleException( "analyzer_configure must be called before any other analyzer function." );
		return 0;
	}

	try  
	{
		return ptr_instance->hopTime();
	}
	catch( Exception & ex )  
	{
		std::string s("Loris exception in analyzer_getHopTime(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex )  
	 
	{
		std::string s("std C++ exception in analyzer_getHopTime(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	return 0;
}

/* ---------------------------------------------------------------- */
/*        analyzer_setHopTime        
/*
/*	Set the hop time (which corresponds approximately to the average
	density of Partial envelope Breakpoint data) for this Analyzer.
   
	analyzer_configure must be called before any other analyzer 
	function.
 */
extern "C"
void analyzer_setHopTime( double x )
{
	if ( 0 == ptr_instance )
	{
		handleException( "analyzer_configure must be called before any other analyzer function." );
		return;
	}

	try 
	{
		ptr_instance->setHopTime( x );
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in analyzer_setHopTime(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in analyzer_setHopTime(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
}
/* ---------------------------------------------------------------- */
/*        analyzer_getCropTime        
/*
/*	Return the crop time (maximum temporal displacement of a time-
	frequency data point from the time-domain center of the analysis
	window, beyond which data points are considered "unreliable")
	for this Analyzer.
   
	analyzer_configure must be called before any other analyzer 
	function.
 */
extern "C"
double analyzer_getCropTime( void )
{
	if ( 0 == ptr_instance )
	{
		handleException( "analyzer_configure must be called before any other analyzer function." );
		return 0;
	}

	try 
	{
		return ptr_instance->cropTime();
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in analyzer_getCropTime(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in analyzer_getCropTime(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	return 0;
}

/* ---------------------------------------------------------------- */
/*        analyzer_setCropTime        
/*
/*	Set the crop time (maximum temporal displacement of a time-
	frequency data point from the time-domain center of the analysis
	window, beyond which data points are considered "unreliable")
	for this Analyzer.
   
   	analyzer_configure must be called before any other analyzer 
   	function.
 */
extern "C"
void analyzer_setCropTime( double x )
{
	if ( 0 == ptr_instance )
	{
		handleException( "analyzer_configure must be called before any other analyzer function." );
		return;
	}

	try 
	{
		ptr_instance->setCropTime( x );
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in analyzer_setCropTime(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in analyzer_setCropTime(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
}

/* ---------------------------------------------------------------- */
/*        analyzer_getBwRegionWidth        
/*
/*	Return the width (in Hz) of the Bandwidth Association regions
	used by this Analyzer.

   	analyzer_configure must be called before any other analyzer 
   	function.
 */
extern "C"
double analyzer_getBwRegionWidth( void )
{
	if ( 0 == ptr_instance )
	{
		handleException( "analyzer_configure must be called before any other analyzer function." );
		return 0;
	}

	try 
	{
		return ptr_instance->bwRegionWidth();
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in analyzer_getBwRegionWidth(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in analyzer_getBwRegionWidth(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	return 0;
}

/* ---------------------------------------------------------------- */
/*        analyzer_setBwRegionWidth        
/*
/*	Set the width (in Hz) of the Bandwidth Association regions
	used by this Analyzer.

   	analyzer_configure must be called before any other analyzer 
   	function.
 */
extern "C"
void analyzer_setBwRegionWidth( double x )
{
	if ( 0 == ptr_instance )
	{
		handleException( "analyzer_configure must be called before any other analyzer function." );
		return;
	}

	try 
	{
		ptr_instance->setBwRegionWidth( x );
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in analyzer_setBwRegionWidth(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in analyzer_setBwRegionWidth(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
}

/* ---------------------------------------------------------------- */
/*        analyzer_storeResidueBandwidth
/*
/*	Construct Partial bandwidth envelopes during analysis
	by associating residual energy in the spectrum (after
	peak extraction) with the selected spectral peaks that
	are used to construct Partials. 
	
	regionWidth is the width (in Hz) of the bandwidth 
	association regions used by this process, must be positive.

   	analyzer_configure must be called before any other analyzer 
   	function.
 */
extern "C"
void analyzer_storeResidueBandwidth( double regionWidth )
{
	if ( 0 == ptr_instance )
	{
		handleException( "analyzer_configure must be called before any other analyzer function." );
		return;
	}

	try 
	{
		ptr_instance->storeResidueBandwidth( regionWidth );
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in analyzer_storeResidueBandwidth(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in analyzer_storeResidueBandwidth(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
}

/* ---------------------------------------------------------------- */
/*        analyzer_storeConvergenceBandwidth
/*
/*	Construct Partial bandwidth envelopes during analysis
	by storing the mixed derivative of short-time phase, 
	scaled and shifted so that a value of 0 corresponds
	to a pure sinusoid, and a value of 1 corresponds to a
	bandwidth-enhanced sinusoid with maximal energy spread
	(minimum sinusoidal convergence).
	
	tolerance is the amount of range over which the 
	mixed derivative indicator should be allowed to drift away 
	from a pure sinusoid before saturating. This range is mapped
	to bandwidth values on the range [0,1]. Must be positive and 
	not greater than 1.

   	analyzer_configure must be called before any other analyzer 
   	function.
 */
extern "C"
void analyzer_storeConvergenceBandwidth( double tolerance )
{
	if ( 0 == ptr_instance )
	{
		handleException( "analyzer_configure must be called before any other analyzer function." );
		return;
	}

	try 
	{
		ptr_instance->storeConvergenceBandwidth( tolerance );
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in analyzer_storeConvergenceBandwidth(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in analyzer_storeConvergenceBandwidth(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
}

/* ---------------------------------------------------------------- */
/*        analyzer_storeNoBandwidth
/*
/*	Disable bandwidth envelope construction. Bandwidth 
	will be zero for all Breakpoints in all Partials.

   	analyzer_configure must be called before any other analyzer 
   	function.
 */
extern "C"
void analyzer_storeNoBandwidth( void )
{
	if ( 0 == ptr_instance )
	{
		handleException( "analyzer_configure must be called before any other analyzer function." );
		return;
	}

	try 
	{
		ptr_instance->storeNoBandwidth();
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in analyzer_storeNoBandwidth(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in analyzer_storeNoBandwidth(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
}

/* ---------------------------------------------------------------- */
/*        analyzer_getBwConvergenceTolerance
/*
/*	Return the mixed derivative convergence tolerance
	only if the convergence indicator is used to compute
	bandwidth envelopes. Return zero if the spectral residue
	method is used or if no bandwidth is computed.
 */
extern "C"
double analyzer_getBwConvergenceTolerance( void )
{
	if ( 0 == ptr_instance )
	{
		handleException( "analyzer_configure must be called before any other analyzer function." );
		return 0;
	}

	try 
	{
		return ptr_instance->bwConvergenceTolerance();
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in analyzer_getBwConvergenceTolerance(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in analyzer_getBwConvergenceTolerance(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}

    return 0;
}





