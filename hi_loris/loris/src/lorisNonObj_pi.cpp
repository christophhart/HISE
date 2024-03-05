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
 *	lorisNonObj_pi.C
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
 *	This file defines the non-object-based component of the Loris
 *	procedural interface.
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

//#include "AiffFile.h"
#include "Analyzer.h"
#include "BreakpointEnvelope.h"
#include "Channelizer.h"
#include "Collator.h"
#include "Dilator.h"
#include "Distiller.h"
#include "LorisExceptions.h"
#include "FrequencyReference.h"
#include "Fundamental.h"
#include "Harmonifier.h"
#include "ImportLemur.h"
#include "Morpher.h"
#include "Notifier.h"
#include "Partial.h"
#include "PartialUtils.h"
#include "Resampler.h"
#include "SdifFile.h"
#include "Sieve.h"
#include "SpcFile.h"
#include "SpectralSurface.h"
#include "Synthesizer.h"

#include <cmath>
#include <functional>
#include <list>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <set>


using namespace Loris;

/* ---------------------------------------------------------------- */
/*		non-object-based procedures
/*
/*	Operations in Loris that need not be accessed though object
	interfaces are represented as simple functions.
 */

/* ---------------------------------------------------------------- */
/*        channelize        
/*
/*	Label Partials in a PartialList with the integer nearest to
	the amplitude-weighted average ratio of their frequency envelope
	to a reference frequency envelope. The frequency spectrum is 
	partitioned into non-overlapping channels whose time-varying 
	center frequencies track the reference frequency envelope. 
	The reference label indicates which channel's center frequency
	is exactly equal to the reference envelope frequency, and other
	channels' center frequencies are multiples of the reference 
	envelope frequency divided by the reference label. Each Partial 
	in the PartialList is labeled with the number of the channel
	that best fits its frequency envelope. The quality of the fit
	is evaluated at the breakpoints in the Partial envelope and
	weighted by the amplitude at each breakpoint, so that high-
	amplitude breakpoints contribute more to the channel decision.
	Partials are labeled, but otherwise unmodified. In particular, 
	their frequencies are not modified in any way.
 */
extern "C"
void channelize( PartialList * partials, 
				 LinearEnvelope * refFreqEnvelope, int refLabel )
{
	try 
	{
		ThrowIfNull((PartialList *) partials);
		ThrowIfNull((LinearEnvelope *) refFreqEnvelope);

		if ( refLabel <= 0 )
		{
			Throw( InvalidArgument, "Channelization reference label must be positive." );
		}
		notifier << "channelizing " << partials->size() << " Partials" << endl;

		Channelizer::channelize( *partials, *refFreqEnvelope, refLabel );		
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in channelize(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in channelize(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
}

/* ---------------------------------------------------------------- */
/*        collate        
/*
/*  Collate unlabeled (zero-labeled) Partials into the smallest-possible 
    number of Partials that does not combine any overlapping Partials.
    Collated Partials assigned labels higher than any label in the original 
    list, and appear at the end of the sequence, after all previously-labeled
    Partials.
 */
extern "C"
void collate( PartialList * partials )
{
	try 
	{
		ThrowIfNull((PartialList *) partials);

		notifier << "collating " << partials->size() << " Partials" << endl;
		
		// Uses default fade time of 1 ms, and .1 ms gap, 
		// should be parameters.
		Collator::collate( *partials, 0.001, 0.0001 );
		
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in collate(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in collate(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
}

/* ---------------------------------------------------------------- */
/*        createFreqReference        
/*
/*	Return a newly-constructed LinearEnvelope using the legacy 
    FrequencyReference class. The envelope will have approximately
    the specified number of samples. The specified number of samples 
    must be greater than 1. Uses the FundamentalEstimator 
    (FundamentalFromPartials) class to construct an estimator of 
    fundamental frequency, configured to emulate the behavior of
    the FrequencyReference class in Loris 1.4-1.5.2. If numSamps 
    is zero, construct the reference envelope from fundamental 
    estimates taken every five milliseconds.

	
	For simple sounds, this frequency reference may be a 
	good first approximation to a reference envelope for
	channelization (see channelize()).
	
	Clients are responsible for disposing of the newly-constructed 
	LinearEnvelope.
 */
extern "C"
LinearEnvelope * 
createFreqReference( PartialList * partials, double minFreq, double maxFreq, 
                     long numSamps )
{
	try 
	{
		ThrowIfNull((PartialList *) partials);
		
		//	use auto_ptr to manage memory in case 
		//	an exception is generated (hard to imagine):
		std::unique_ptr< LinearEnvelope > env_ptr;
		if ( numSamps != 0 )
		{
			env_ptr.reset( new LinearEnvelope( 
								FrequencyReference( partials->begin(), partials->end(), 
													minFreq, maxFreq, numSamps ).envelope() ) );
		}
		else
		{
			env_ptr.reset( new LinearEnvelope( 
								FrequencyReference( partials->begin(), partials->end(), 
													minFreq, maxFreq ).envelope() ) );
		}
		
		return env_ptr.release();
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in createFreqReference(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in createFreqReference(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	return NULL;
}

/* ---------------------------------------------------------------- */
/*        createF0Estimate        
/*
/* Return a newly-constructed LinearEnvelope that estimates
   the time-varying fundamental frequency of the sound
   represented by the Partials in a PartialList. This uses
   the FundamentalEstimator (FundamentalFromPartials) 
   class to construct an estimator of fundamental frequency, 
   and returns a LinearEnvelope that samples the estimator at the 
   specified time interval (in seconds). Default values are used 
   to configure the estimator. Only estimates in the specified 
   frequency range will be considered valid, estimates outside this 
   range will be ignored.
   
   Clients are responsible for disposing of the newly-constructed 
   LinearEnvelope.
 */
extern "C"
LinearEnvelope * 
createF0Estimate( PartialList * partials, double minFreq, double maxFreq, 
                  double interval, void* threadController)
{
	try 
	{
		ThrowIfNull((PartialList *) partials);
        
        const double Precision = 0.1;
        const double Confidence = 0.9;
        FundamentalFromPartials est( Precision );
		est.controller = static_cast<hise::ThreadController*>(threadController);
        
        std::pair< double, double > span = 
            PartialUtils::timeSpan( partials->begin(), partials->end() );
            
        LinearEnvelope * env_ptr = 
         new LinearEnvelope( est.buildEnvelope( partials->begin(), 
                                                partials->end(), 
                                                span.first, span.second, interval,
                                                minFreq, maxFreq,
                                                Confidence ) );                                            
		return env_ptr;
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in createF0Estimate(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in createF0Estimate(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	return NULL;
}

/* ---------------------------------------------------------------- */
/*        dilate        
/*
/*	Dilate Partials in a PartialList according to the given 
	initial and target time points. Partial envelopes are 
	stretched and compressed so that temporal features at
	the initial time points are aligned with the final time
	points. Time points are sorted, so Partial envelopes are 
	are only stretched and compressed, but breakpoints are not
	reordered. Duplicate time points are allowed. There must be
	the same number of initial and target time points.
 */
extern "C"
void dilate( PartialList * partials, 
			    const double * initial, const double * target, int npts )
{
	try 
	{
		ThrowIfNull((PartialList *) partials);
		ThrowIfNull((double *) initial);
		ThrowIfNull((double *) target);

		notifier << "dilating " << partials->size() << " Partials" << endl;
		Dilator::dilate( partials->begin(), partials->end(),
      		           initial, initial + npts, target );

	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in dilate(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in dilate(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
}

/* ---------------------------------------------------------------- */
/*        distill        
/*
/*  Distill labeled (channelized) Partials in a PartialList into a 
    PartialList containing at most one Partial per label. Unlabeled 
    (zero-labeled) Partials are left unmodified at the end of the 
    distilled Partials.
 */
extern "C"
void distill( PartialList * partials )
{
	try 
	{
		ThrowIfNull((PartialList *) partials);

		notifier << "distilling " << partials->size() << " Partials" << endl;
		
		// uses default fade time of 1 ms, should be parameter
		Distiller::distill( *partials, 0.001 );
		
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in distill(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in distill(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
}

/* ---------------------------------------------------------------- */
/*        exportAiff        
/*
/*	Export mono audio samples stored in an array of size bufferSize to 
	an AIFF file having the specified sample rate at the given file path 
	(or name). The floating point samples in the buffer are clamped to the 
	range (-1.,1.) and converted to integers having bitsPerSamp bits.
 */
extern "C"
void exportAiff( const char * path, const double * buffer, 
				     unsigned int bufferSize, double samplerate, int bitsPerSamp )
{
#if 0
	try
	{
		ThrowIfNull((double *) buffer);
	
		// do nothing if there are no samples:
		if ( bufferSize == 0 )
		{
			notifier << "no samples to write to " << path << endl;
			return;
		}

		//	write out samples:
		notifier << "writing " << bufferSize << " samples to " << path << endl;
		AiffFile fout( buffer, bufferSize, samplerate );
		fout.write( path, bitsPerSamp );
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in exportAiff(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in exportAiff(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
#endif
}

/* ---------------------------------------------------------------- */
/*        exportSdif        
/*
/*	Export Partials in a PartialList to a SDIF file at the specified
	file path (or name). SDIF data is written in the 1TRC format.  
	For more information about SDIF, see the SDIF web site at:
		www.ircam.fr/equipes/analyse-synthese/sdif/  
 */
extern "C"
void exportSdif( const char * path, PartialList * partials )
{
#if 0
	try
	{
		ThrowIfNull((PartialList *) partials);

		if ( partials->size() == 0 ) 
		{
			Throw( Loris::InvalidObject, "No Partials in PartialList to export to sdif file." );
		}
	
		notifier << "exporting sdif partial data to " << path << endl;		
		
		SdifFile fout( partials->begin(), partials->end() );
		fout.write( path );
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in exportSdif(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in exportSdif(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
#endif
}

/* ---------------------------------------------------------------- */
/*        exportSpc        
/*
/*	Export Partials in a PartialList to a Spc file at the specified file
	path (or name). The fractional MIDI pitch must be specified. The 
	enhanced parameter defaults to true (for bandwidth-enhanced spc files), 
	but an be specified false for pure-sines spc files. The endApproachTime 
	parameter is in seconds. A nonzero endApproachTime indicates that the plist does 
	not include a release, but rather ends in a static spectrum corresponding 
	to the final breakpoint values of the partials. The endApproachTime
	specifies how long before the end of the sound the amplitude, frequency, 
	and bandwidth values are to be modified to make a gradual transition to 
	the static spectrum.
 */
extern "C"
void exportSpc( const char * path, PartialList * partials, double midiPitch, 
				int enhanced, double endApproachTime )
{
#if 0
	try
	{
		ThrowIfNull((PartialList *) partials);

		if ( partials->size() == 0 )
		{
			Throw( InvalidObject, "No Partials in PartialList to export to Spc file." );
		}
		
		notifier << "exporting Spc partial data to " << path << endl;

		SpcFile fout( midiPitch );
		PartialList::size_type countPartials = 0;
		for ( PartialList::iterator iter = partials->begin(); iter != partials->end(); ++iter )
		{
			if ( iter->label() > 0 && iter->label() < 512 ) 
				 // should have a symbol defined for 512!!!
			{
				fout.addPartial( *iter );
				++countPartials;
			}
		}
		
		if ( countPartials != partials->size() )
		{
			notifier << "exporting " << countPartials << " of " 
					 << partials->size() << " Partials having labels less than 512." << endl;
		}
		if ( countPartials == 0 )
		{
			Throw( InvalidObject, "No Partials in PartialList have valid Spc labels (1-511)." );
		}
		
		if ( 0 == enhanced )
		{
			fout.writeSinusoidal( path, endApproachTime );
		}
		else
		{
			fout.write( path, endApproachTime );
		}
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in exportSpc(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in exportSdif(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
#endif
}

/* ---------------------------------------------------------------- */
/*        harmonify        
/*
/*  Apply a reference Partial to fix the frequencies of Breakpoints
    whose amplitude is below threshold_dB. 0 harmonifies full-amplitude
    Partials, to apply only to quiet Partials, specify a lower 
    threshold like -90). The reference Partial is the first Partial
    in the PartialList labeled refLabel (usually 1). The Envelope 
    is a time-varying weighting on the harmonifing process. When 1, 
    harmonic frequencies are used, when 0, breakpoint frequencies are 
    unmodified. 
 */
extern "C"
void harmonify( PartialList * partials, long refLabel,
                const LinearEnvelope * env, double threshold_dB )
{
	try 
	{
		ThrowIfNull((PartialList *) partials);
		ThrowIfNull((LinearEnvelope *) env);

		if ( partials->size() == 0 )
		{
			Throw( InvalidObject, "No Partials in PartialList to harmonify." );
		}
		
		notifier << "harmonifying " << partials->size() << " Partials" << endl;

        Harmonifier::harmonify( partials->begin(), partials->end(), refLabel,
                                *env, threshold_dB );
                                
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in harmonify(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in harmonify(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}

}

/* ---------------------------------------------------------------- */
/*        importAiff        
/*
/*	Import audio samples stored in an AIFF file at the given file
	path (or name). The samples are converted to floating point 
	values on the range (-1.,1.) and stored in an array of doubles. 
	The value returned is the number of samples in buffer, and it is at
	most bufferSize. If samplerate is not a NULL pointer, 
	then, on return, it points to the value of the sample rate (in
	Hz) of the AIFF samples. The AIFF file must contain only a single
	channel of audio data. The prior contents of buffer, if any, are 
	overwritten.
 */
extern "C"
unsigned int 
importAiff( const char * path, double * buffer, unsigned int bufferSize, 
            double * samplerate )
{
#if 0
	unsigned int howMany = 0;
	try 
	{
		//	read samples:
		notifier << "reading samples from " << path << endl;
		AiffFile f( path );
		notifier << "read " << f.samples().size() << " frames at " 
				 << f.sampleRate() << " Hz" << endl;
				
		howMany = std::min( f.samples().size(), 
							std::vector< double >::size_type( bufferSize ) );
		if ( howMany < f.samples().size() )
		{
			notifier << "returning " << howMany << " samples" << endl;
		}

		std::copy( f.samples().begin(), f.samples().begin() + howMany,
				   buffer );
		std::fill( buffer + howMany,  buffer + bufferSize, 0. );

				
		if ( samplerate )
		{
			*samplerate = f.sampleRate();
		}
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in importAiff(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in importAiff(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	return howMany;
#endif

	return 0;
}

/* ---------------------------------------------------------------- */
/*        importSdif 
/*       
/*	Import Partials from an SDIF file at the given file path (or 
	name), and append them to a PartialList. Loris reads SDIF
	files in the 1TRC format. For more information about SDIF, 
	see the SDIF web site at:
		www.ircam.fr/equipes/analyse-synthese/sdif/ 
 */	
extern "C"
void importSdif( const char * path, PartialList * partials )
{
#if 0
	try
	{
		ThrowIfNull((PartialList *) partials);

		notifier << "importing Partials from " << path << endl;
		SdifFile imp( path );
		partials->splice( partials->end(), imp.partials() );
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in importSdif(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in importSdif(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
#endif
}

/* ---------------------------------------------------------------- */
/*        importSpc 
/*       
/*	Import Partials from an Spc file at the given file path (or 
	name), and return them in a PartialList.
 */	
extern "C"
void importSpc( const char * path, PartialList * partials )
{
#if 0
	try
	{
		Loris::notifier << "importing Partials from " << path << Loris::endl;
		Loris::SpcFile imp( path );
		partials->insert( partials->end(), imp.partials().begin(), imp.partials().end() );

	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in importSpc(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in importSpc(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
#endif
}

/* ---------------------------------------------------------------- */
/*        morpher_setAmplitudeShape        
/*
/* Set the shaping parameter for the amplitude morphing
   function. This shaping parameter controls the 
   slope of the amplitude morphing function,
   for values greater than 1, this function
   gets nearly linear (like the old amplitude
   morphing function), for values much less 
   than 1 (e.g. 1E-5) the slope is gently
   curved and sounds pretty "linear", for 
   very small values (e.g. 1E-12) the curve
   is very steep and sounds un-natural because
   of the huge jump from zero amplitude to
   very small amplitude.

   Use LORIS_DEFAULT_AMPMORPHSHAPE to obtain the 
   default amplitude morphing shape for Loris, 
   (equal to 1E-5, which works well for many musical 
   instrument morphs, unless Loris was compiled
   with the symbol LINEAR_AMP_MORPHS defined, in
   which case LORIS_DEFAULT_AMPMORPHSHAPE is equal 
   to LORIS_LINEAR_AMPMORPHSHAPE).
   
   Use LORIS_LINEAR_AMPMORPHSHAPE to approximate 
   the linear amplitude morphs performed by older
   versions of Loris.

   The amplitude shape must be positive.
 */
#if !defined(LINEAR_AMP_MORPHS) || !LINEAR_AMP_MORPHS
   const double LORIS_DEFAULT_AMPMORPHSHAPE = 1E-5;    
#else  
   const double LORIS_DEFAULT_AMPMORPHSHAPE = 1E5;    
#endif

const double LORIS_LINEAR_AMPMORPHSHAPE = 1E5;

static double PI_ampMorphShape = LORIS_DEFAULT_AMPMORPHSHAPE;
extern "C"
void morpher_setAmplitudeShape( double x )
{
   if ( x <= 0. )
   {
     std::string s("Loris exception in morpher_setAmplitudeShape(): " );
     s.append( "Invalid Argument: the amplitude morph shaping parameter must be positive" );
     handleException( s.c_str() );
   }
   PI_ampMorphShape = x;
}

/* ---------------------------------------------------------------- */
/*        morph        
/*
/*	Morph labeled Partials in two PartialLists according to the
	given frequency, amplitude, and bandwidth (noisiness) morphing
	envelopes, and append the morphed Partials to the destination 
	PartialList. Loris morphs Partials by interpolating frequency,
	amplitude, and bandwidth envelopes of corresponding Partials in 
	the source PartialLists. For more information about the Loris
	morphing algorithm, see the Loris website: 
	www.cerlsoundgroup.org/Loris/
 */
extern "C"
void morph( const PartialList * src0, const PartialList * src1, 
            const LinearEnvelope * ffreq, 
            const LinearEnvelope * famp, 
            const LinearEnvelope * fbw, 
            PartialList * dst )
{
	try 
	{
		ThrowIfNull((PartialList *) src0);
		ThrowIfNull((PartialList *) src1);
		ThrowIfNull((PartialList *) dst);
		ThrowIfNull((LinearEnvelope *) ffreq);
		ThrowIfNull((LinearEnvelope *) famp);
		ThrowIfNull((LinearEnvelope *) fbw);

		notifier << "morphing " << src0->size() << " Partials with " <<
					src1->size() << " Partials" << endl;
					
		//	make a Morpher object and do it:
		Morpher m( *ffreq, *famp, *fbw );
		m.morph( src0->begin(), src0->end(), src1->begin(), src1->end() );
				
		//	splice the morphed Partials into dst:
		dst->splice( dst->end(), m.partials() );
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in morph(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in morph(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
}

/* ---------------------------------------------------------------- */
/*        morphWithReference        
/*
/*	Morph labeled Partials in two PartialLists according to the
	given frequency, amplitude, and bandwidth (noisiness) morphing
	envelopes, and append the morphed Partials to the destination 
	PartialList. Specify the labels of the Partials to be used as 
	reference Partial for the two morph sources. The reference 
	partial is used to compute frequencies for very low-amplitude 
	Partials whose frequency estimates are not considered reliable. 
	The reference Partial is considered to have good frequency 
	estimates throughout. A reference label of 0 indicates that 
	no reference Partial should be used for the corresponding
	morph source.
   
	Loris morphs Partials by interpolating frequency,
	amplitude, and bandwidth envelopes of corresponding Partials in 
	the source PartialLists. For more information about the Loris
	morphing algorithm, see the Loris website: 
	www.cerlsoundgroup.org/Loris/
 */
extern "C"
void morphWithReference( const PartialList * src0, 
                         const PartialList * src1,
                         long src0RefLabel, 
                         long src1RefLabel,
                         const LinearEnvelope * ffreq, 
                         const LinearEnvelope * famp, 
                         const LinearEnvelope * fbw, 
                         PartialList * dst )
{
	try 
	{
		ThrowIfNull((PartialList *) src0);
		ThrowIfNull((PartialList *) src1);
		ThrowIfNull((PartialList *) dst);
		ThrowIfNull((LinearEnvelope *) ffreq);
		ThrowIfNull((LinearEnvelope *) famp);
		ThrowIfNull((LinearEnvelope *) fbw);

		notifier << "morphing " << src0->size() << " Partials with " 
		         << src1->size() << " Partials" << endl;
		         
		//	make a Morpher object and do it:
		Morpher m( *ffreq, *famp, *fbw );
		
		if ( src0RefLabel != 0 )
		{
		   notifier << "using Partial labeled " << src0RefLabel;
		   notifier << " as reference Partial for first morph source" << endl;
		   m.setSourceReferencePartial( *src0, src0RefLabel );
		}
		else
		{
		   notifier << "using no reference Partial for first morph source" << endl;
		}

		if ( src1RefLabel != 0 )
		{
		   notifier << "using Partial labeled " << src1RefLabel;
		   notifier << " as reference Partial for second morph source" << endl;
		   m.setTargetReferencePartial( *src1, src1RefLabel );
		}
		else
		{
		   notifier << "using no reference Partial for second morph source" << endl;
		}
			
		m.morph( src0->begin(), src0->end(), src1->begin(), src1->end() );
				
		//	splice the morphed Partials into dst:
		dst->splice( dst->end(), m.partials() );
	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in morphWithReference(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in morphWithReference(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
}


/* ---------------------------------------------------------------- */
/*        resample
/*
/*  Resample all Partials in a PartialList using the specified
	sampling interval, so that the Breakpoints in the Partial 
	envelopes will all lie on a common temporal grid.
	The Breakpoint times in resampled Partials will comprise a  
	contiguous sequence of integer multiples of the sampling interval,
	beginning with the multiple nearest to the Partial's start time and
	ending with the multiple nearest to the Partial's end time. Resampling
	is performed in-place. 

 */
extern "C"
void resample( PartialList * partials, double interval )
{
	try 
	{
		ThrowIfNull((PartialList *) partials);
		
		notifier << "resampling " << partials->size() << " Partials" << Loris::endl;

      	Resampler::resample( partials->begin(), partials->end(), interval );

        //  remove any resulting empty Partials
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
        std::string s( "Loris exception in resample(): " );
        s.append( ex.what() );
        handleException( s.c_str() );
    }
    catch( std::exception & ex )
    {
        std::string s( "std C++ exception in resample(): " );
        s.append( ex.what() );
        handleException( s.c_str() );
    }
}

/* ---------------------------------------------------------------- */
/*        shapeSpectrum
/*  Scale the amplitudes of a set of Partials by applying 
    a spectral suface constructed from another set.
    Strecth the spectral surface in time and frequency
    using the specified stretch factors. 
 */
extern "C"
void shapeSpectrum( PartialList * partials, PartialList * surface,
                    double stretchFreq, double stretchTime )
{
	try 
	{
        ThrowIfNull((PartialList *) partials);
        ThrowIfNull((PartialList *) surface);
		
        notifier << "shaping " << partials->size() << " Partials using "
                 << "spectral surface created from " << surface->size() 
                 << " Partials" << Loris::endl;

        // uses default fade time of 1 ms, should be parameter
        SpectralSurface surf( surface->begin(), surface->end() );
        surf.setFrequencyStretch( stretchFreq );
        surf.setTimeStretch( stretchTime );
        surf.setEffect( 1.0 );  // should this be a parameter?
        surf.scaleAmplitudes( partials->begin(), partials->end() );
	}
	catch( Exception & ex )
    {
        std::string s("Loris exception in shapeSpectrum(): " );
        s.append( ex.what() );
        handleException( s.c_str() );
    }
    catch( std::exception & ex )
    {
        std::string s("std C++ exception in shapeSpectrum(): " );
        s.append( ex.what() );
        handleException( s.c_str() );
    }
}
/* ---------------------------------------------------------------- */
/*        sift
/*  Eliminate overlapping Partials having the same label
	(except zero). If any two partials with same label
	overlap in time, keep only the longer of the two.
	Set the label of the shorter duration partial to zero.

 */
extern "C"
void sift( PartialList * partials )
{
	try 
	{
        ThrowIfNull((PartialList *) partials);
		
        notifier << "sifting " << partials->size() << " Partials" << Loris::endl;

        // uses default fade time of 1 ms, should be parameter
        Sieve::sift( partials->begin(), partials->end(), 0.001 );
	}
	catch( Exception & ex )
    {
        std::string s("Loris exception in sift(): " );
        s.append( ex.what() );
        handleException( s.c_str() );
    }
    catch( std::exception & ex )
    {
        std::string s("std C++ exception in sift(): " );
        s.append( ex.what() );
        handleException( s.c_str() );
    }
}

/* ---------------------------------------------------------------- */
/*        synthesize        
/*
/*	Synthesize Partials in a PartialList at the given sample
	rate, and store the (floating point) samples in a buffer of
	size bufferSize. The buffer is neither resized nor 
	cleared before synthesis, so newly synthesized samples are
	added to any previously computed samples in the buffer, and
	samples beyond the end of the buffer are lost. Return the
	number of samples synthesized, that is, the index of the
	latest sample in the buffer that was modified.
 */
extern "C"
unsigned int synthesize( const PartialList * partials, 
				 double * buffer, unsigned int bufferSize,  
				 double srate )
{
	unsigned int howMany = 0;
	try 
	{
		ThrowIfNull((PartialList *) partials);
		ThrowIfNull((double *) buffer);

		notifier << "synthesizing " << partials->size() 
				   << " Partials at " << srate << " Hz" << endl;

		//	synthesize:
		std::vector< double > vec;
		Synthesizer synth( srate, vec );
		synth.synthesize( partials->begin(), partials->end() );
		
		// determine the number of synthesized samples
		// that will be stored:
		howMany = vec.size();
		if ( howMany > bufferSize )
		{
			howMany = bufferSize;
		}

		//	accumulate into the buffer:
		std::transform( buffer, buffer + howMany, vec.begin(), 
						buffer, std::plus< double >() );
						    

	}
	catch( Exception & ex ) 
	{
		std::string s("Loris exception in synthesize(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		std::string s("std C++ exception in synthesize(): " );
		s.append( ex.what() );
		handleException( s.c_str() );
	}
	return howMany;
}




