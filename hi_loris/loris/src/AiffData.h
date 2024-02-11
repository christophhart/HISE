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
 * AiffData.h
 *
 * Declarations of import and export functions.
 *
 * Kelly Fitz, 17 Sept 2003 
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */

#include "Marker.h"

#include <string>
#include <vector>

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


//	begin namespace
namespace Loris {

//	-- chunk types --
enum 
{ 
	ContainerId = 0x464f524d,				// 'FORM' 
	AiffType = 0x41494646,					// 'AIFF' 
	CommonId = 0x434f4d4d,					// 'COMM'
	ApplicationSpecificId = 0x4150504c,		// 'APPL'
	SosEnvelopesId = 0x534f5365,			// 'SOSe'
	SoundDataId = 0x53534e44,				// 'SSND'
	InstrumentId = 0x494e5354,				// 'INST'
	MarkerId = 0x4d41524b					// 'MARK'
};

typedef Uint_32 ID;
typedef char Byte;

struct CkHeader 
{
	ID id;
	Uint_32 size;
	
	//	providing this default constructor
	//	gives clients a way to determine whether
	//	a chunk has been read and assigned:
	CkHeader( void ) : id(0), size(0) {}
};

struct ContainerCk
{
	CkHeader header;
	ID formType;
};

struct CommonCk
{
	CkHeader header;
	Int_16 channels;			// number of channels 
	Int_32 sampleFrames;		// channel independent sample frames 
	Int_16 bitsPerSample;		// number of bits per sample 
	double srate;				// sampling rate (stored in IEEE 10 byte format)
};

struct SoundDataCk
{
	CkHeader header;	
	Uint_32 offset;				
	Uint_32 blockSize;	
	
	//	sample frames follow
	std::vector< Byte > sampleBytes;
};

struct MarkerCk 
{ 
	CkHeader header;	
	Uint_16  numMarkers; 
	
	struct Marker
	{ 
		Uint_16  markerID; 
		Uint_32	 position;	 		// position in uncompressed samples
		std::string markerName; 
	}; 
	
	std::vector< MarkerCk::Marker > markers;
}; 

struct InstrumentCk
{
	CkHeader header;	
	Byte	baseNote;		/* all notes are MIDI note numbers */ 
	Byte	detune;			/* cents off, only -50 to +50 are significant */ 
	Byte	lowNote; 
	Byte	highNote; 
	Byte	lowVelocity;	/* 1 to 127 */ 
	Byte	highVelocity;	/* 1 to 127 */ 
	Int_16	gain;			/* in dB, 0 is normal */ 

	struct Loop 
	{ 
		Int_16	playMode;	/* 0 - no loop, 1 - forward looping, 2 - backward looping */ 
		Uint_16 beginLoop;	/* this is a reference to a markerID, so you always 
	                                 have to work with MARK and INST together!! */ 
		Uint_16 endLoop; 
	}; 

	Loop sustainLoop;
	Loop releaseLoop; 
}; 


struct SosEnvelopesCk
{
	CkHeader header;	
	Int_32	signature;		// For SOS, should be 'SOSe'
	Int_32	enhanced;		// 0 for sine-only, 1 for bandwidth-enhanced
	Int_32	validPartials;	// Number of partials with data in them; the file must
							// be padded out to the next higher 2**n partials;
							// this number is doubled for enhanced files
	
	//	skip validPartials * sizeof(Int_32) bytes of junk here
	Int_32	resolution;		// frame duration in microseconds 
	Int_32	quasiHarmonic;	// how many of the partials are quasiharmonic
	//	skip 
	//		(4*LargestLabel + 8 - validPartials - 2) * sizeof(Int_32) 
	//	bytes of junk here	

/*				
	//	this stuff is unbelievably nasty!						
	
#define initPhaseLth ( 4*LargestLabel + 8 )
	Int_32	initPhase[initPhaseLth]; // obsolete initial phase array; is VARIABLE LENGTH array 
							// this is big enough for a max of 512 enhanced partials plus values below
//	Int_32	resolution;		// frame duration in microseconds 
	#define SOSresolution( es )   initPhase[ spcEI.enhanced \
											? 2 * spcEI.numPartials : spcEI.numPartials]	
							// follows the initPhase[] array
	
							
//	Int_32	quasiHarmonic;	// how many of the partials are quasiharmonic
	#define SOSquasiHarmonic( es )  initPhase[ spcEI.enhanced \
											? 2 * spcEI.numPartials + 1 : spcEI.numPartials + 1]	
							// follows the initPhase[] array
*/							
};

// ---------------------------------------------------------------------------
//	readChunkHeader
// ---------------------------------------------------------------------------
//	Read the id and chunk size from the current file position.
//	Let exceptions propogate.
//
std::istream &  
readChunkHeader( std::istream & s, CkHeader & h );


// ---------------------------------------------------------------------------
//	readApplicationSpecifcData
// ---------------------------------------------------------------------------
//	Read the data in the ApplicationSpecific chunk, assume the stream is 
//	correctly positioned, and that the chunk header has already been read.
//
//  Look for data specific to SPC files. Any other kind of Application 
//	Specific data is ignored.
//
std::istream & 
readApplicationSpecifcData( std::istream & s, SosEnvelopesCk & ck, unsigned long chunkSize );


// ---------------------------------------------------------------------------
//	readCommonData
// ---------------------------------------------------------------------------
//	Read the data in the Common chunk, assume the stream is correctly
//	positioned, and that the chunk header has already been read.
//
std::istream & 
readCommonData( std::istream & s, CommonCk & ck, unsigned long chunkSize );

// ---------------------------------------------------------------------------
//	readContainer
// ---------------------------------------------------------------------------
//
std::istream & 
readContainer( std::istream & s, ContainerCk & ck, unsigned long chunkSize );

// ---------------------------------------------------------------------------
//	readInstrumentData
// ---------------------------------------------------------------------------
//
std::istream & 
readInstrumentData( std::istream & s, InstrumentCk & ck, unsigned long chunkSize );

// ---------------------------------------------------------------------------
//	readMarkerData
// ---------------------------------------------------------------------------
//
std::istream & 
readMarkerData( std::istream & s, MarkerCk & ck, unsigned long chunkSize );

// ---------------------------------------------------------------------------
//	readSampleData
// ---------------------------------------------------------------------------
//	Read the data in the Sound Data chunk, assume the stream is correctly
//	positioned, and that the chunk header has already been read.
//
std::istream & 
readSampleData( std::istream & s, SoundDataCk & ck, unsigned long chunkSize );

// ---------------------------------------------------------------------------
//	configureCommonCk
// ---------------------------------------------------------------------------
void 
configureCommonCk( CommonCk & ck, unsigned long nFrames, unsigned int nChans,
				   unsigned int bps, double srate  );
				   
// ---------------------------------------------------------------------------
//	configureContainer
// ---------------------------------------------------------------------------
//	dataSize is the combined size of all other chunks in file. Configure 
//	them first, then add their sizes (with headers!).
//
void 
configureContainer( ContainerCk & ck, unsigned long dataSize );

// ---------------------------------------------------------------------------
//	configureInstrumentCk
// ---------------------------------------------------------------------------
void 
configureInstrumentCk( InstrumentCk & ck, double midiNoteNum );

// ---------------------------------------------------------------------------
//	configureMarkerCk
// ---------------------------------------------------------------------------
void 
configureMarkerCk( MarkerCk & ck, const std::vector< Marker > & markers, 
				   double srate  );

// ---------------------------------------------------------------------------
//	configureSoundDataCk
// ---------------------------------------------------------------------------
//
void 
configureSoundDataCk( SoundDataCk & ck, const std::vector< double > & samples, 
					  unsigned int bps  );

// ---------------------------------------------------------------------------
//	writeCommon
// ---------------------------------------------------------------------------
//
std::ostream &
writeCommonData( std::ostream & s, const CommonCk & ck );

// ---------------------------------------------------------------------------
//	writeContainer
// ---------------------------------------------------------------------------
//
std::ostream &
writeContainer( std::ostream & s, const ContainerCk & ck );

// ---------------------------------------------------------------------------
//	writeInstrumentData
// ---------------------------------------------------------------------------
std::ostream &
writeInstrumentData( std::ostream & s, const InstrumentCk & ck );

// ---------------------------------------------------------------------------
//	writeMarkerData
// ---------------------------------------------------------------------------
std::ostream &
writeMarkerData( std::ostream & s, const MarkerCk & ck );

// ---------------------------------------------------------------------------
//	writeSampleData
// ---------------------------------------------------------------------------
//
std::ostream & 
writeSampleData( std::ostream & s, const SoundDataCk & ck );

// ---------------------------------------------------------------------------
//	convertBytesToSamples
// ---------------------------------------------------------------------------
//	Convert sample bytes to double precision floating point samples 
//	(-1.0, 1.0). The samples vector is resized to fit exactly as many
//	samples as are represented in the bytes vector, and any prior
//	contents are overwritten.
//
void
convertBytesToSamples( const std::vector< Byte > & bytes, 
					   std::vector< double > & samples, unsigned int bps );

// ---------------------------------------------------------------------------
//	convertSamplesToBytes
// ---------------------------------------------------------------------------
//	Convert floating point samples (-1.0, 1.0) to bytes. 
//	The bytes vector is resized to fit exactly as many
//	samples as are stored in the samples vector, and any prior
//	contents are overwritten.
//
void
convertSamplesToBytes( const std::vector< double > & samples, 
					   std::vector< Byte > & bytes, unsigned int bps );
					   
} // end of namespace
