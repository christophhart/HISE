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
 * ImportLemur.C
 *
 * Implementation of class Loris::ImportLemur for importing Partials stored 
 * in Lemur 5 alpha files.
 *
 * Kelly Fitz, 10 Sept 1999
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */

#if HAVE_CONFIG_H
	#include "config.h"
#endif

#include "ImportLemur.h"
#include "BigEndian.h"
#include "LorisExceptions.h"
#include "Partial.h"
#include "PartialList.h"
#include "Breakpoint.h"
#include "Notifier.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <string>

//	in case configure wasn't run (no config.h), 
//	pick some (hopefully-) reasonable values for
//	these things and hope for the best...
#if ! defined( SIZEOF_SHORT )
#define SIZEOF_SHORT 2
#endif

#if ! defined( SIZEOF_INT )
#define SIZEOF_INT 4
#endif

#if ! defined( SIZEOF_LONG )
#define SIZEOF_LONG 4	// not for DEC Alpha!
#endif

#if SIZEOF_SHORT == 2
typedef short 			Int_16;
typedef unsigned short 	Uint_16;
#elif SIZEOF_INT == 2
typedef int 			Int_16;
typedef unsigned int 	Uint_16;
#else
#error "cannot find an appropriate type for 16-bit integers"
#endif

#if SIZEOF_INT == 4
typedef int 			Int_32;
typedef unsigned int 	Uint_32;
#elif SIZEOF_LONG == 4
typedef long 			Int_32;
typedef unsigned long 	Uint_32;
#else
#error "cannot find an appropriate type for 32-bit integers"
#endif


#if ! defined( SIZEOF_FLOAT ) || SIZEOF_FLOAT == 4
typedef float			Float_32;
#else
#error "cannot find an appropriate type for 32-bit floats"
#endif

#if ! defined( SIZEOF_DOUBLE ) || SIZEOF_DOUBLE == 8
typedef double			Double_64;
#else
#error "cannot find an appropriate type for 64-bit double-precision floats"
#endif


#if defined(HAVE_M_PI) && (HAVE_M_PI)
	const double Pi = M_PI;
#else
	const double Pi = 3.14159265358979324;
#endif		

//	begin namespace
namespace Loris {

//	-- types and ids --
enum { 
		ContainerId = 0x464f524d,				// 'FORM' 
		LEMR_ID = 0x4c454d52,					// 'LEMR' 
		AnalysisParamsID = 0x4c4d414e,			// 'LMAN' 
		TrackDataID = 0x54524b53,				// 'TRKS'
		FormatNumber = 4962 };

//	for reading and writing files, the exact sizes and
//	alignments are critical.
typedef Int_32 ID;
struct CkHeader 
{
	Int_32 id;
	Int_32 size;
};

struct ContainerCk
{
	CkHeader header;
	ID formType;
};

struct AnalysisParamsCk
{
	//Int_32 ckID;
	//Int_32 ckSize;
	CkHeader header;
	
	Int_32 formatNumber;
	Int_32 originalFormatNumber;
	
	Int_32 ftLength;			//	samples, transform length
	Float_32 winWidth;			//	Hz, main lobe width
	Float_32 winAtten;			//	dB, sidelobe attenuation
	Int_32	hopSize;			//	samples, frame length
	Float_32 sampleRate;		// 	Hz, from analyzed sample
	
	Float_32 noiseFloor;		//	dB (negative)
	Float_32 peakAmpRange;		//	dB, floating relative amplitde threshold
	Float_32 maskingRolloff;	//	dB/Hz, peak masking curve	
	Float_32 peakSeparation;	//	Hz, minimum separation between peaks
	Float_32 freqDrift;			//	Hz, maximum track freq drift over a frame
}; 

struct TrackDataCk
{
	CkHeader header;
	Uint_32	numberOfTracks;
	Int_32	trackOrder;			// enumerated type
	// track data follows
};

struct TrackOnDisk
{
	Double_64	startTime;		// in milliseconds
	Float_32	initialPhase;
	Uint_32		numPeaks;
	Int_32		label;
};

struct PeakOnDisk
{
	Float_32	magnitude;
	Float_32	frequency;
	Float_32	interpolatedFrequency;
	Float_32	bandwidth;
	Double_64	ttn;
};

//	prototypes for import helpers:
static void importPartials( std::istream & s, PartialList & partials, double bweCutoff );
static void getPartial( std::istream & s, PartialList & partials, double bweCutoff );	
static void readChunkHeader( std::istream & s, CkHeader & ck );
static void readContainer( std::istream & s );
static void readParamsChunk( std::istream & s );
static long readTracksChunk( std::istream & s );
static void readTrackHeader( std::istream & s, TrackOnDisk & t );
static void readPeakData( std::istream & s, PeakOnDisk & p );

// ---------------------------------------------------------------------------
//	ImportLemur constructor
// ---------------------------------------------------------------------------
//	Imports immediately.
//	bweCutoff defaults to 1kHz, the default cutoff in Lemur.
//	Clients should be prepared to catch ImportErrors.
//
ImportLemur::ImportLemur( const std::string & fname, double bweCutoff /* = 1000 Hz */)
{
	std::fstream fs;
	try
	{
		fs.open( fname.c_str(), std::ios::in | std::ios::binary );
	}
	catch( std::exception & ex )
	{
		Throw( ImportException, ex.what() );
	}
	importPartials( fs, _partials, bweCutoff );
}

// ---------------------------------------------------------------------------
//	importPartials
// ---------------------------------------------------------------------------
//	Clients should be prepared to catch ImportExceptions.
//
//	THIS WON'T WORK IF CHUNKS ARE IN A DIFFERENT ORDER!!!
//	Fortunately, they never will be, since only the research 
//	version of Lemur ever wrote these files anyway.
//
static void
importPartials( std::istream & s, PartialList & partials, double bweCutoff )
{
	try 
	{
		//	the Container chunk must be first, read it:
		readContainer( s );
		
		//	read other chunks:
		bool foundParams = false, foundTracks = false;
		while ( ! foundParams || ! foundTracks )
		{
			//	read a chunk header, if it isn't the one we want, skip over it.
			CkHeader h;
			readChunkHeader( s, h );
			
			if ( h.id == AnalysisParamsID )
			{
				readParamsChunk( s );
				foundParams = true;
			}
			else if ( h.id == TrackDataID )
			{
				if (! foundParams) 	//	 I hope this doesn't happen
				{
					Throw( ImportException, 
							"Mia culpa! I am not smart enough to read the Track data before the Analysis Parameters data." );
				}							
				//	read Partials:
				for ( long counter = readTracksChunk( s ); counter > 0; --counter ) 
				{
					getPartial(s, partials, bweCutoff);
				}
				foundTracks = true;
			}
			else
			{
				s.ignore( h.size );
			}
		}

	}
	catch ( Exception & ex ) 
	{
		if ( s.eof() )
		{
			ex.append("Reached end of file before finding both a Tracks chunk and a Parameters chunk.");
		}
		ex.append( "Import failed." );
		Throw( ImportException, ex.str() );
	}

}

// ---------------------------------------------------------------------------
//	readContainer
// ---------------------------------------------------------------------------
//
static void
readContainer( std::istream & s )
{
	ContainerCk ck;		
	try 
	{
		//	read chunk header:
		readChunkHeader( s, ck.header );
		if( ck.header.id != ContainerId )
			Throw( FileIOException, "Found no Container chunk." );
		
		//	read FORM type
		BigEndian::read( s, 1, sizeof(ID), (char *)&ck.formType );		
	}
	catch( FileIOException & ex ) 
	{
		ex.append( "Failed to read badly-formatted Lemur file (bad Container chunk)." );
		throw;
	}

	//	make sure its really a Dr. Lemur file:
	if ( ck.formType != LEMR_ID ) 
	{
		Throw( ImportException, "File is not formatted correctly for Lemur 5 import." );
	}
}

// ---------------------------------------------------------------------------
//	getPartial
// ---------------------------------------------------------------------------
//	Convert any FileIOExceptions into ImportExceptions, so that clients can 
//	reasonably expect to catch only ImportExceptions.
//
static void
getPartial( std::istream & s, PartialList & partials, double bweCutoff )
{
	try 
	{
		//	read the Track header:
		TrackOnDisk tkHeader;
		readTrackHeader( s, tkHeader );
		
		//	create a Partial:
		Partial p;
		p.setLabel( tkHeader.label );
		
		//	keep running phase and time for Breakpoint construction:
		double phase = tkHeader.initialPhase;
		
		//	convert time to seconds and offset by a millisecond to 
		//	allow for implied onset (Lemur analysis data was shifted
		//	such that the earliest Partial starts at 0).
		double time = tkHeader.startTime * 0.001;	
		
		//	use this to compute phases:
		double prevTtnSec = 0.;
		
		//	loop: read Peak, create Breakpoint, add to Partial:
		for ( int i = 0; i < tkHeader.numPeaks; ++i ) {
			//	read Peak:
			PeakOnDisk pkData;
			readPeakData( s, pkData );
			
			double frequency = pkData.frequency;
			double amplitude = pkData.magnitude;
			double bandwidth = std::min( pkData.bandwidth, 1.f );
			
			//	fix bandwidth:
			//	Lemur used a cutoff frequency, below which 
			//	bandwidth was ignored; Loris does not, so 
			//	toss out that bogus bandwidth.
			if ( frequency < bweCutoff ) {
				amplitude *= std::sqrt(1. - bandwidth);
				bandwidth = 0.;
			}
			//	otherwise, adjust the bandwidth value
			//	to account for the difference in noise
			//	scaling between Lemur and Loris; this
			//	mess doubles the noise modulation index
			//	without changing the sine modulation index,
			//	see Oscillator::modulate(). 
			else {
				amplitude *= std::sqrt( 1. + (3. * bandwidth) );
				bandwidth = (4. * bandwidth) / ( 1. + (3. * bandwidth) ); 
			}

			//	update phase based on _this_ pkData's interpolated freq:
			phase +=2. * Pi * prevTtnSec * pkData.interpolatedFrequency;
			phase = std::fmod( phase, 2. * Pi );
			
			//	create Breakpoint:	
			Breakpoint bp( frequency, amplitude, bandwidth, phase );
			
			//	insert in Partial:
			p.insert( time, bp );
			
			//	update time:
			prevTtnSec = pkData.ttn * 0.001;
			time += prevTtnSec;
		}

		if ( p.duration() > 0. ) {
			partials.push_back( p );
		}
		/*
		else {
			debugger << "import rejecting a Partial of zero duration (" 
					<< tkHeader.numPeaks << " peaks read)" << endl;
		}
		*/
	}
	catch( FileIOException & ex ) {
		ex.append(  "Failed to import a partial from a Lemur file." );
		throw;
	}
	
}	

// ---------------------------------------------------------------------------
//	readChunkHeader
// ---------------------------------------------------------------------------
//	Read the id and chunk size from the current file position.
//
static void
readChunkHeader( std::istream & s, CkHeader & h )
{
	BigEndian::read( s, 1, sizeof(ID), (char *)&h.id );
	BigEndian::read( s, 1, sizeof(Int_32), (char *)&h.size );
} 

// ---------------------------------------------------------------------------
//	readTracksChunk
// ---------------------------------------------------------------------------
//	Leave file positioned at end of chunk header data and at the beginning 
//	of the first track.
//	Assumes that the stream is correctly positioned and that the chunk 
//	header has been read.
//	Returns the number of tracks to read.
//
static long 
readTracksChunk( std::istream & s )
{
	TrackDataCk ck;
	try 
	{		
		//	found it, read it one field at a time:
		BigEndian::read( s, 1, sizeof(Uint_32), (char *)&ck.numberOfTracks );
		BigEndian::read( s, 1, sizeof(Int_32), (char *)&ck.trackOrder );
	}
	catch( FileIOException & ex ) 
	{
		ex.append( "Failed to read badly-formatted Lemur file (bad Track Data chunk)." );
		throw;
	}
	
	return ck.numberOfTracks;
}

// ---------------------------------------------------------------------------
//	readParamsChunk
// ---------------------------------------------------------------------------
//	Verify that the file has the correct format and is available for reading.
//	Assumes that the stream is correctly positioned and that the chunk 
//	header has been read.
//
static void
readParamsChunk( std::istream & s )
{
	AnalysisParamsCk ck;
	try 
	{
		BigEndian::read( s, 1, sizeof(Int_32), (char *)&ck.formatNumber );
		BigEndian::read( s, 1, sizeof(Int_32), (char *)&ck.originalFormatNumber );

		BigEndian::read( s, 1, sizeof(Int_32), (char *)&ck.ftLength );
		BigEndian::read( s, 1, sizeof(Float_32), (char *)&ck.winWidth );
		BigEndian::read( s, 1, sizeof(Float_32), (char *)&ck.winAtten );
		BigEndian::read( s, 1, sizeof(Int_32), (char *)&ck.hopSize );
		BigEndian::read( s, 1, sizeof(Float_32), (char *)&ck.sampleRate );
		
		BigEndian::read( s, 1, sizeof(Float_32), (char *)&ck.noiseFloor );
		BigEndian::read( s, 1, sizeof(Float_32), (char *)&ck.peakAmpRange );
		BigEndian::read( s, 1, sizeof(Float_32), (char *)&ck.maskingRolloff );
		BigEndian::read( s, 1, sizeof(Float_32), (char *)&ck.peakSeparation );
		BigEndian::read( s, 1, sizeof(Float_32), (char *)&ck.freqDrift );
	}
	catch( FileIOException & ex ) 
	{
		ex.append( "Failed to read badly-formatted Lemur file (bad Parameters chunk)." );
		throw;
	}
	
	if ( ck.formatNumber != FormatNumber ) 
	{
		Throw( FileIOException, "File has wrong Lemur format for Lemur 5 import." );
	}
}

// ---------------------------------------------------------------------------
//	readTrackHeader
// ---------------------------------------------------------------------------
//	Read from current position.
//
static void 
readTrackHeader( std::istream & s, TrackOnDisk & t )
{
	try 
	{
		BigEndian::read( s, 1, sizeof(Double_64), (char *)&t.startTime );
		BigEndian::read( s, 1, sizeof(Float_32), (char *)&t.initialPhase );
		BigEndian::read( s, 1, sizeof(Uint_32), (char *)&t.numPeaks );
		BigEndian::read( s, 1, sizeof(Int_32), (char *)&t.label );
	}
	catch( FileIOException & ex ) 
	{
		ex.append( "Failed to read track data in Lemur 5 import." );
		throw;
	}
}

// ---------------------------------------------------------------------------
//	readPeakData
// ---------------------------------------------------------------------------
//	Read from current position.
//
static void 
readPeakData( std::istream & s, PeakOnDisk & p )
{
	try 
	{
		BigEndian::read( s, 1, sizeof(Float_32), (char *)&p.magnitude );
		BigEndian::read( s, 1, sizeof(Float_32), (char *)&p.frequency );
		BigEndian::read( s, 1, sizeof(Float_32), (char *)&p.interpolatedFrequency );
		BigEndian::read( s, 1, sizeof(Float_32), (char *)&p.bandwidth );
		BigEndian::read( s, 1, sizeof(Double_64), (char *)&p.ttn );
	}
	catch( FileIOException & ex ) 
	{
		ex.append( "Failed to read peak data in Lemur 5 import." );
		throw;
	}
}
}	//	end of namespace Loris
