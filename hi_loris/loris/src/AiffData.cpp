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
 * AiffData.C
 *
 * Implementation of import and export functions.
 *
 * Kelly Fitz, 17 Sept 2003 
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */

#if HAVE_CONFIG_H
	#include "config.h"
#endif

#include "AiffData.h"

#include "BigEndian.h"
#include "LorisExceptions.h"
#include "Marker.h"
#include "Notifier.h"

#include <climits>
#include <cmath>
#include <fstream>
#include <iostream>

//	begin namespace
namespace Loris {


// ---------------------------------------------------------------------------
//	extended80
// ---------------------------------------------------------------------------
//  IEEE floating point conversions, functions defined at end of file
//  (used to be in a separate file, but they aren't used anywhere else).
//

/*	struct extended80 defined below 	*/
struct extended80;

/*	conversion functions				*/
extern void ConvertToIeeeExtended(double num, extended80 * x) ;
extern double ConvertFromIeeeExtended(const extended80 * x) ;

/*	struct extended80 definition, with 
	constructors and conversion to double 								
 */
struct extended80 
{
	char data[10];
	
	extended80( double x = 0. ) { ConvertToIeeeExtended( x, this ); }
	operator double( void ) const { return ConvertFromIeeeExtended( this ); }	
};

// -- AIFF import --

// ---------------------------------------------------------------------------
//	readChunkHeader
// ---------------------------------------------------------------------------
//	Read the id and chunk size from the current file position.
//
std::istream & 
readChunkHeader( std::istream & s, CkHeader & h )
{	
    ID id = 0;
    Uint_32 sz = 0;
    BigEndian::read( s, 1, sizeof(ID), (char *)&id );
    BigEndian::read( s, 1, sizeof(Uint_32), (char *)&sz );
    
	if ( s )
	{
		h.id = id;
		h.size = sz;
	}

	return s;
}

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
readApplicationSpecifcData( std::istream & s, SosEnvelopesCk & ck, unsigned long chunkSize )
{
    Int_32 tmp_signature;
    BigEndian::read( s, 1, sizeof(Int_32), (char *)&tmp_signature );
    
    if ( tmp_signature == SosEnvelopesId )
    {
        ck.header.id = ApplicationSpecificId;	
        ck.header.size = chunkSize;
        ck.signature = SosEnvelopesId; 
        
        //	lookout! The format of this chunk is a mess, due
        //	to obsolete stuff lying around!
        BigEndian::read( s, 1, sizeof(Int_32), (char *)&ck.enhanced );
        BigEndian::read( s, 1, sizeof(Int_32), (char *)&ck.validPartials );
        s.ignore( ck.validPartials * sizeof(Int_32) );
        BigEndian::read( s, 1, sizeof(Int_32), (char *)&ck.resolution );
        s.ignore( chunkSize - (4 + ck.validPartials) * sizeof(Int_32) );
    }
    else
    {
        s.ignore( chunkSize - sizeof(Int_32) );
    }

    if ( !s )
    {
        Throw( FileIOException, "Failed to read badly-formatted AIFF file (bad ApplicationSpecific chunk)." );
    }
	
	return s;
}

// ---------------------------------------------------------------------------
//	readCommonData
// ---------------------------------------------------------------------------
//	Read the data in the Common chunk, assume the stream is correctly
//	positioned, and that the chunk header has already been read.
//
std::istream & 
readCommonData( std::istream & s, CommonCk & ck, unsigned long chunkSize )
{
    BigEndian::read( s, 1, sizeof(Int_16), (char *)&ck.channels );
    BigEndian::read( s, 1, sizeof(Int_32), (char *)&ck.sampleFrames );
    BigEndian::read( s, 1, sizeof(Int_16), (char *)&ck.bitsPerSample );
    
    if ( !s )
    {
        Throw( FileIOException, "Failed to read badly-formatted AIFF file (bad Common chunk)." );
    }
                        
	ck.header.id = CommonId;	
	ck.header.size = chunkSize;

    //	don't let this get byte-reversed:
    extended80 read_rate;
    BigEndian::read( s, sizeof(extended80), sizeof(char), (char *)&read_rate );
    ck.srate = read_rate;

	return s;
}

// ---------------------------------------------------------------------------
//	readContainer
// ---------------------------------------------------------------------------
//
std::istream & 
readContainer( std::istream & s, ContainerCk & ck, unsigned long chunkSize )
{
	ck.header.id = ContainerId;	
	ck.header.size = chunkSize;

    //	read in the chunk data:
    BigEndian::read( s, 1, sizeof(ID), (char *)&ck.formType );
    if ( !s )
    {
        Throw( FileIOException, "Failed to read badly-formatted AIFF file (bad Container chunk)." );
    }

	//	make sure its really AIFF:
	if ( ck.formType != AiffType )
	{
		std::string err("Bad form type in AIFF file: ");
		err += std::string( ck.formType, 4 );
		Throw( FileIOException, err );
	}
		
	return s;
}	

// ---------------------------------------------------------------------------
//	readInstrumentData
// ---------------------------------------------------------------------------
//
std::istream & 
readInstrumentData( std::istream & s, InstrumentCk & ck, unsigned long chunkSize )
{
	ck.header.id = InstrumentId;	
	ck.header.size = chunkSize;

    BigEndian::read( s, 1, sizeof(Byte), (char *)&ck.baseNote );
    BigEndian::read( s, 1, sizeof(Byte), (char *)&ck.detune );
    BigEndian::read( s, 1, sizeof(Byte), (char *)&ck.lowNote );
    BigEndian::read( s, 1, sizeof(Byte), (char *)&ck.highNote );
    BigEndian::read( s, 1, sizeof(Byte), (char *)&ck.lowVelocity );
    BigEndian::read( s, 1, sizeof(Byte), (char *)&ck.highVelocity );
    BigEndian::read( s, 1, sizeof(Int_16), (char *)&ck.gain );

    //	AIFFLoop is three Int_16s:
    BigEndian::read( s, 1, sizeof(Uint_16), (char *)&ck.sustainLoop.playMode );
    BigEndian::read( s, 1, sizeof(Int_16), (char *)&ck.sustainLoop.beginLoop );
    BigEndian::read( s, 1, sizeof(Int_16), (char *)&ck.sustainLoop.endLoop );
    
    BigEndian::read( s, 1, sizeof(Uint_16), (char *)&ck.releaseLoop.playMode );
    BigEndian::read( s, 1, sizeof(Int_16), (char *)&ck.releaseLoop.beginLoop );
    BigEndian::read( s, 1, sizeof(Int_16), (char *)&ck.releaseLoop.endLoop );

    if ( !s )
    {
        Throw( FileIOException, "Failed to read badly-formatted AIFF file (bad Common chunk)." );
    }
							
	return s;
}

// ---------------------------------------------------------------------------
//	readMarkerData
// ---------------------------------------------------------------------------
//
std::istream & 
readMarkerData( std::istream & s, MarkerCk & ck, unsigned long chunkSize )
{
	ck.header.id = MarkerId;	
	ck.header.size = chunkSize;

    Uint_32 bytesToRead = chunkSize;
    
    //	read in the number of Markers
    BigEndian::read( s, 1, sizeof(Uint_16), (char *)&ck.numMarkers );
    bytesToRead -= sizeof(Uint_16);
    
    for ( int i = 0; i < ck.numMarkers; ++i )
    {
        MarkerCk::Marker marker;
        BigEndian::read( s, 1, sizeof(Uint_16), (char *)&marker.markerID );
        bytesToRead -= sizeof(Uint_16);
        
        BigEndian::read( s, 1, sizeof(Uint_32), (char *)&marker.position );
        bytesToRead -= sizeof(Uint_32);
        
        //	read the size of the name string, then the characters:
        unsigned char namelength;
        BigEndian::read( s, 1, sizeof(unsigned char), (char *)&namelength );
        bytesToRead -= sizeof(unsigned char);
        
        //	need to add one to the length, because, like C-strings,
        //	Pascal strings are null-terminated, but the null character
        //	is _not_ counted in the length.
        //
        //	Correction? At least one web source says that Pascal strings
        //	are _not_ null terminated, but are padded (with an extra byte
        //	that is not part of the count) if necessary to make the total 
        //	number of bytes even (that is, the char bytes, _plus_ the count). 
        //
        //	This way seems to work with all the files I have tried (Kyma and 
        //	Peak, mostly) for AIFF and Spc.
        int ncharbytes = namelength;
        if ( ncharbytes%2 == 0 )
            ++ncharbytes;
        static char tmpChars[256];
        BigEndian::read( s, ncharbytes, sizeof(char), tmpChars );
        bytesToRead -= ncharbytes * sizeof(char);
        tmpChars[ namelength ] = '\0';
    
        //	convert to a string:
        marker.markerName = std::string( tmpChars );
        
        ck.markers.push_back( marker );
    }
    
    if ( bytesToRead > 0 )
    {
        s.ignore( bytesToRead );
    }

    if ( !s )
    {
        Throw( FileIOException, "Failed to read badly-formatted AIFF file (bad Marker chunk)." );
    }
        
	return s;
}	

// ---------------------------------------------------------------------------
//	readSamples
// ---------------------------------------------------------------------------
//	Let exceptions propogate.
//
static std::istream & 
readSamples( std::istream & s, std::vector< Byte > & bytes )
{	
	debugger << "reading " << bytes.size() << " bytes of sample data" << endl;

	//	read integer samples without byte swapping: 
	BigEndian::read( s, bytes.size(), 1, (char*)(&bytes[0]) );
	
	return s;
}

// ---------------------------------------------------------------------------
//	readSampleData
// ---------------------------------------------------------------------------
//	Read the data in the Sound Data chunk, assume the stream is correctly
//	positioned, and that the chunk header has already been read.
//
std::istream & 
readSampleData( std::istream & s, SoundDataCk & ck, unsigned long chunkSize )
{
	ck.header.id = SoundDataId;	
	ck.header.size = chunkSize;

    BigEndian::read( s, 1, sizeof(Uint_32), (char *)&ck.offset );
    BigEndian::read( s, 1, sizeof(Uint_32), (char *)&ck.blockSize );
    
    //	compute the actual number of bytes that
    //	can be read from this chunk:
    //	(chunkSize is everything after the header)
    const unsigned long howManyBytes = 
        ( chunkSize - ck.offset ) - (2 * sizeof(Uint_32));
        
    ck.sampleBytes.resize( howManyBytes, 0 );		//	could throw bad_alloc

    //	skip ahead to the samples and read them:
    s.ignore( ck.offset );
    readSamples( s, ck.sampleBytes );

    if ( !s )
    {
        Throw( FileIOException, "Failed to read badly-formatted AIFF file (bad Sound Data chunk)." );
    }
	
	return s;
}

// -- Chunk construction --

// ---------------------------------------------------------------------------
//	configureCommonCk
// ---------------------------------------------------------------------------
void 
configureCommonCk( CommonCk & ck, unsigned long nFrames, unsigned int nChans,
				   unsigned int bps, double srate  )
{
	ck.header.id = CommonId;

	//	size is everything after the header:
	ck.header.size = sizeof(Int_16) + 			//	num channels
					 sizeof(Int_32) + 			//	num frames
					 sizeof(Int_16) + 			//	bits per sample
					 sizeof(extended80);	//	sample rate
					 
	ck.channels = nChans;
	ck.sampleFrames = nFrames;
	ck.bitsPerSample = bps;
	ck.srate = srate;
	// ConvertToIeeeExtended( srate, &ck.srate );
	
}

// ---------------------------------------------------------------------------
//	configureContainer
// ---------------------------------------------------------------------------
//	dataSize is the combined size of all other chunks in file. Configure 
//	them first, then add their sizes (with headers!).
//
void 
configureContainer( ContainerCk & ck, unsigned long dataSize )
{	
	ck.header.id = ContainerId;
	
	//	size is everything after the header:
	ck.header.size = sizeof(Int_32) + dataSize;
	
	ck.formType = AiffType;
}

// ---------------------------------------------------------------------------
//	configureInstrumentCk
// ---------------------------------------------------------------------------
void 
configureInstrumentCk( InstrumentCk & ck, double midiNoteNum )
{
	ck.header.id = InstrumentId;

	//	size is everything after the header:
	ck.header.size = 
			sizeof(Byte) +				// baseFrequency
			sizeof(Byte) +				// detune
			sizeof(Byte) +				// lowFrequency
			sizeof(Byte) +				// highFrequency
			sizeof(Byte) +				// lowVelocity
			sizeof(Byte) +				// highVelocity
			sizeof(Int_16) +			// gain
			2 * sizeof(Int_16) +		// playmode for sustainLoop and releaseLoop
			2 * sizeof(Uint_16) +		// beginLoop for sustainLoop and releaseLoop
			2 * sizeof(Uint_16);		// loopEnd for sustainLoop and releaseLoop

	ck.baseNote = Byte( midiNoteNum );
	ck.detune = Byte(long( 100 * midiNoteNum ) % 100);
	if (ck.detune > 50)
	{
		ck.baseNote++;
		ck.detune -= 100;
	}
	ck.detune *= -1;

	ck.lowNote = 0;	
	ck.highNote = 127;	
	ck.lowVelocity = 1;	
	ck.highVelocity = 127;	
	ck.gain = 0;	
	ck.sustainLoop.playMode = 0;		// Sustain looping done by name, not by this
	ck.sustainLoop.beginLoop = 0;
	ck.sustainLoop.endLoop = 0;
	ck.releaseLoop.playMode = 0;		// No Looping
	ck.releaseLoop.beginLoop = 0;
	ck.releaseLoop.endLoop = 0;
}

// ---------------------------------------------------------------------------
//	configureMarkerCk
// ---------------------------------------------------------------------------
void 
configureMarkerCk( MarkerCk & ck, const std::vector< Marker > & markers, double srate  )
{
	ck.header.id = MarkerId;

	//	accumulate data size
	Uint_32 dataSize = sizeof(Uint_16);	//	num markers
	
	ck.numMarkers = markers.size();
	ck.markers.resize( markers.size() );
	for ( unsigned int j = 0; j < markers.size(); ++j )
	{
		MarkerCk::Marker & m = ck.markers[j];
		m.markerID = j+1;
		m.position = Uint_32((markers[j].time() * srate) + 0.5);
		m.markerName = markers[j].name();
		
		#define MAX_PSTRING_CHARS 254
		if ( m.markerName.size() > MAX_PSTRING_CHARS )
			m.markerName.resize( MAX_PSTRING_CHARS );
		
		//	the size of a pascal string is the number of 
		//	characters plus the size byte, plus the terminal '\0'.
		//	
		//	Actualy, at least one web source indicates that Pascal
		//	strings are not null-terminated, but that they _are_
		//	padded with an extra (not part of the count) byte
		//	if necessary to ensure that the total length (including
		//	count) is even, and this seems to work better with other
		//	programs (e.g. Kyma)
		if ( m.markerName.size()%2 == 0 )
				m.markerName += '\0';
		dataSize += sizeof(Uint_16) + sizeof(Uint_32) + (m.markerName.size() + 1);
	}

	//	must be an even number of bytes
	if ( dataSize%2 )
		++dataSize;

	ck.header.size = dataSize;
}

// ---------------------------------------------------------------------------
//	configureSoundDataCk
// ---------------------------------------------------------------------------
//
void 
configureSoundDataCk( SoundDataCk & ck, const std::vector< double > & samples, 
					  unsigned int bps  )
{
	Uint_32 dataSize = samples.size() * (bps/8);
	//	must be an even number of bytes:
	if ( dataSize % 2 ) 
	{
		++dataSize;
	}

	ck.header.id = SoundDataId;

	//	size is everything after the header:
	ck.header.size = sizeof(Uint_32) + 	//	offset
					 sizeof(Uint_32) + 	//	block size
					 dataSize;			//	sample data


	//	no block alignment:	
	ck.offset = 0;
	ck.blockSize = 0;
	
	//	convert the samples to integers stored in 
	//	big endian order in the byte vector:
	convertSamplesToBytes( samples, ck.sampleBytes, bps );
}


// -- AIFF export --

// ---------------------------------------------------------------------------
//	writeCommon
// ---------------------------------------------------------------------------
//
std::ostream &
writeCommonData( std::ostream & s, const CommonCk & ck )
{
/*
	debugger << "writing common chunk: " << endl;
	debugger << "header id: " << ck.header.id << endl;
	debugger << "size: " << ck.header.size << endl;
	debugger << "channels: " << ck.channels << endl;
	debugger << "sample frames: " << ck.sampleFrames << endl;
	debugger << "bits per sample: " << ck.bitsPerSample << endl;
	//debugger << "rate: " << _sampleRate  << endl;
*/
	
	//	write it out:
	try 
	{
		BigEndian::write( s, 1, sizeof(ID), (char *)&ck.header.id );
		BigEndian::write( s, 1, sizeof(Int_32), (char *)&ck.header.size );
		BigEndian::write( s, 1, sizeof(Int_16), (char *)&ck.channels );
		BigEndian::write( s, 1, sizeof(Int_32), (char *)&ck.sampleFrames );
		BigEndian::write( s, 1, sizeof(Int_16), (char *)&ck.bitsPerSample );
		//	don't let this get byte-reversed:
		extended80 write_rate( ck.srate );
		//ConvertToIeeeExtended( &ck.srate, write_rate );

		BigEndian::write( s, sizeof(extended80), sizeof(char), (char *)&write_rate );
	}
	catch( FileIOException & ex ) 
	{
		ex.append( "Failed to write AIFF file Common chunk." );
		throw;
	}
	
	return s;
}

// ---------------------------------------------------------------------------
//	writeContainer
// ---------------------------------------------------------------------------
//
std::ostream &
writeContainer( std::ostream & s, const ContainerCk & ck )
{
/*
	debugger << "writing container: " << endl;
	debugger << "header id: " << ck.header.id << endl;
	debugger << "size: " << ck.header.size << endl;
	debugger << "type: " << ck.formType << endl;
*/
	
	//	write it out:
	try 
	{
		BigEndian::write( s, 1, sizeof(ID), (char *)&ck.header.id );
		BigEndian::write( s, 1, sizeof(Int_32), (char *)&ck.header.size );
		BigEndian::write( s, 1, sizeof(ID), (char *)&ck.formType );
	}
	catch( FileIOException & ex ) 
	{
		ex.append( "Failed to write AIFF file Container chunk." );
		throw;
	}
	
	return s;
}
	
// ---------------------------------------------------------------------------
//	writeInstrumentData
// ---------------------------------------------------------------------------
std::ostream &
writeInstrumentData( std::ostream & s, const InstrumentCk & ck )
{
	try 
	{
		BigEndian::write( s, 1, sizeof(Int_32), (char *)&ck.header.id );
		BigEndian::write( s, 1, sizeof(Int_32), (char *)&ck.header.size );

		BigEndian::write( s, 1, sizeof(Byte), (char *)&ck.baseNote );
		BigEndian::write( s, 1, sizeof(Byte), (char *)&ck.detune );
		BigEndian::write( s, 1, sizeof(Byte), (char *)&ck.lowNote );
		BigEndian::write( s, 1, sizeof(Byte), (char *)&ck.highNote );
		BigEndian::write( s, 1, sizeof(Byte), (char *)&ck.lowVelocity );
		BigEndian::write( s, 1, sizeof(Byte), (char *)&ck.highVelocity );
		BigEndian::write( s, 1, sizeof(Int_16), (char *)&ck.gain );

		//	AIFFLoop is three Int_16s:
		BigEndian::write( s, 3, sizeof(Int_16), (char *)&ck.sustainLoop );
		BigEndian::write( s, 3, sizeof(Int_16), (char *)&ck.releaseLoop );
	}
	catch( FileIOException & ex ) 
	{
		ex.append( " Failed to write SPC file Instrument chunk." );
		throw;
	}

	return s;
}

// ---------------------------------------------------------------------------
//	writeMarkerData
// ---------------------------------------------------------------------------
std::ostream &
writeMarkerData( std::ostream & s, const MarkerCk & ck )
{
	try 
	{
		BigEndian::write( s, 1, sizeof(ID), (char *)&ck.header.id );
		BigEndian::write( s, 1, sizeof(Int_32), (char *)&ck.header.size );
		BigEndian::write( s, 1, sizeof(Uint_16), (char *)&ck.numMarkers );
		
		int markerbytes = 0;
		for ( unsigned int j = 0; j < ck.markers.size(); ++j )
		{
			const MarkerCk::Marker & m = ck.markers[j];
			BigEndian::write( s, 1, sizeof(Uint_16), (char *)&m.markerID );
			BigEndian::write( s, 1, sizeof(Uint_32), (char *)&m.position );
			
			//	the size of a pascal string is the number of 
			//	characters plus the size byte, plus the terminal '\0':
			//	Actualy, at least one web source indicates that Pascal
			//	strings are not null-terminated, but that they _are_
			//	padded with an extra (not part of the count) byte
			//	if necessary to ensure that the total length (including
			//	count) is even, and this seems to work better with other
			//	programs (e.g. Kyma)
			Uint_32 bytesToWrite = (m.markerName.size() + 1) * sizeof(char);

			// format pascal string:
			static char tmpChars[256];
			tmpChars[0] = m.markerName.size();
			std::copy( m.markerName.begin(), m.markerName.end(), tmpChars + 1 );
			tmpChars[m.markerName.size()+1] = '\0';
			
			BigEndian::write( s, bytesToWrite, sizeof(char), tmpChars );
			markerbytes += bytesToWrite;
		}
		
		//	be sure to write an even number of bytes
		if ( markerbytes%2 )
			BigEndian::write( s, 1, sizeof(char), "\0" );
		
	}
	catch( FileIOException & ex ) 
	{
		ex.append( "Failed to write AIFF file Marker chunk." );
		throw;
	}
	
	return s;
}

// ---------------------------------------------------------------------------
//	writeSamples
// ---------------------------------------------------------------------------
//	Let exceptions propogate.
//
static std::ostream & 
writeSamples( std::ostream & s, const std::vector< Byte > & bytes )
{	
//	debugger << "writing " << bytes.size() << " bytes of sample data" << endl;

	//	write integer samples without byte swapping,
	//	the bytes were constructed in the correct
	//	big endian order (see convertSamplesToBytes): 
	BigEndian::write( s, bytes.size(), 1, (char*)(&bytes[0]) );
	
	return s;
}

// ---------------------------------------------------------------------------
//	writeSampleData
// ---------------------------------------------------------------------------
//
std::ostream & 
writeSampleData( std::ostream & s, const SoundDataCk & ck )
{
/*
	debugger << "writing sample data: " << endl;
	debugger << "header id: " << ck.header.id << endl;
	debugger << "size: " << ck.header.size << endl;
	debugger << "offset: " << ck.offset << endl;
	debugger << "block size: " << ck.blockSize << endl;
*/

	//	write it out:
	try 
	{
		BigEndian::write( s, 1, sizeof(ID), (char *)&ck.header.id );
		BigEndian::write( s, 1, sizeof(Int_32), (char *)&ck.header.size );
		BigEndian::write( s, 1, sizeof(Int_32), (char *)&ck.offset );
		BigEndian::write( s, 1, sizeof(Int_32), (char *)&ck.blockSize );

		writeSamples( s, ck.sampleBytes );
	}
	catch( FileIOException & ex ) 
	{
		ex.append( "Failed to write AIFF file Container chunk." );
		throw;
	}
	
	return s;
}

// -- sample conversion --

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
					   std::vector< double > & samples, unsigned int bps )
{
	Assert( bps <= 32 );
	
	typedef std::vector< double >::size_type size_type;

	const int bytesPerSample = bps / 8;
	samples.resize( bytes.size() / bytesPerSample );

	debugger << "converting " << samples.size() << " samples of size " 
			 << bps << " bits" << endl;

	//	shift sample bytes into a long integer, and 
	//	scale to make a double:
	const double oneOverMax = std::pow(0.5, double(bps-1));
	long samp;
	
	std::vector< Byte >::const_iterator bytePos = bytes.begin();
	std::vector< double >::iterator samplePos = samples.begin();
	while ( samplePos != samples.end() )
	{
		//	assign the leading byte, so that the sign
		//	is preserved:
		samp = static_cast<char>(*(bytePos++));
		for ( size_type j = 1; j < bytesPerSample; ++j )
		{
			Assert( bytePos != bytes.end() );
			
			//	OR bytes after the most significant, so
			//	that their sign is ignored:
			samp = (samp << 8) + (unsigned char)*(bytePos++);
			
			//	cannot decide why this didn't work,
			//	instead of the add above.
			//samp |= (unsigned long)*(bytePos++);
		}

		*(samplePos++) = oneOverMax * samp;
	}
}

// ---------------------------------------------------------------------------
//	convertSamplesToBytes
// ---------------------------------------------------------------------------
//	Convert floating point samples (-1.0, 1.0) to bytes. 
//	The bytes vector is resized to fit exactly as many
//	samples as are stored in the samples vector, and any prior
//	contents are overwritten.
//
//	This formats the samples as big endian bytes, that is, the most
//	significant bytes are stored earlier in the byte vector, so 
//	no byte-swapping should be performed when this byte array is 
//	written to disk, and moreover this function is specific to 
//	big-endian data storage (like AIFF files).
void
convertSamplesToBytes( const std::vector< double > & samples, 
					   std::vector< Byte > & bytes, unsigned int bps )
{
	Assert( bps <= 32 );

	typedef std::vector< Byte >::size_type size_type;

	//	grow the bytes vector, must be an even number 
	//	of bytes:
	const int bytesPerSample = bps / 8;
	size_type howManyBytes = samples.size() * bytesPerSample;
	if ( howManyBytes%2 )
		++howManyBytes;
	bytes.resize( howManyBytes );

	debugger << "converting " << samples.size() << " samples to size " 
			 << bps << " bits" << endl;

	//	shift sample bytes into a long integer, and 
	//	scale to make a double:
	const double maxSample = std::pow(2., double(bps-1));
	long samp;
	
	std::vector< Byte >::iterator bytePos = bytes.begin();
	std::vector< double >::const_iterator samplePos = samples.begin();
	while ( samplePos != samples.end() )
	{
		samp = long(*(samplePos++) * maxSample);
		
		//	store the sample bytes in big endian order, 
		//	most significant byte first:
		for ( size_type j = bytesPerSample; j > 0; --j )
		{
			//Assert( bytePos != bytes.end() );
			//	mask the lowest byte after shifting:
			*(bytePos++) = 0xFF & (samp >> (8*(j-1)));
		}
	}
}

// ---------------------------------------------------------------------------
//	extended80
// ---------------------------------------------------------------------------
//  IEEE floating point conversions.
//

#ifndef HUGE_VAL
# define HUGE_VAL HUGE
#endif							/* HUGE_VAL */

#define FloatToUnsigned(f)((Int_32)((Int_32(f - 2147483648.0)) + (Int_32)(2147483647)) + 1)

void ConvertToIeeeExtended(double num, extended80 * x)
{
	int sign;
	int expon;
	double fMant, fsMant;
	Int_32 hiMant, loMant;
	char * bytes = x->data;

	if (num < 0) {
		sign = 0x8000;
		num *= -1;
	} else
		sign = 0;

	if (num == 0) {
		expon = 0;
		hiMant = 0;
		loMant = 0;
	} else {
		fMant = frexp(num, &expon);
		if ((expon > 16384) || !(fMant < 1)) {	/* Infinity or NaN */
			expon = sign | 0x7FFF;
			hiMant = 0;
			loMant = 0;
		}
		 /* infinity */ 
		else {					/* Finite */
			expon += 16382;
			if (expon < 0) {	/* denormalized */
				fMant = ldexp(fMant, expon);
				expon = 0;
			}
			expon |= sign;
			fMant = ldexp(fMant, 32);
			fsMant = floor(fMant);
			hiMant = FloatToUnsigned(fsMant);
			fMant = ldexp(fMant - fsMant, 32);
			fsMant = floor(fMant);
			loMant = FloatToUnsigned(fsMant);
		}
	}

	bytes[0] = expon >> 8;
	bytes[1] = expon;

	bytes[2] = hiMant >> 24;
	bytes[3] = hiMant >> 16;
	bytes[4] = hiMant >> 8;
	bytes[5] = hiMant;

	bytes[6] = loMant >> 24;
	bytes[7] = loMant >> 16;
	bytes[8] = loMant >> 8;
	bytes[9] = loMant;
	
}

#ifndef HUGE_VAL
# define HUGE_VAL HUGE
#endif							/* HUGE_VAL */

# define UnsignedToFloat(u)(((double)((Int_32)(u - (Int_32)(2147483647) - 1))) + 2147483648.0)

/****************************************************************
 * Extended precision IEEE floating-point conversion routine.
 ****************************************************************/

double ConvertFromIeeeExtended(const extended80 * x)
{								/* LCN */ /* ? */
	double f;
	int expon;
	Int_32 hiMant, loMant;
	const char * bytes = x->data;

	expon = ((bytes[0] & 0x7F) << 8) | (bytes[1] & 0xFF);
	hiMant = ((int)(bytes[2] & 0xFF) << 24)
		| ((int)(bytes[3] & 0xFF) << 16)
		| ((int)(bytes[4] & 0xFF) << 8)
		| ((int)(bytes[5] & 0xFF));
	loMant = ((int)(bytes[6] & 0xFF) << 24)
		| ((int)(bytes[7] & 0xFF) << 16)
		| ((int)(bytes[8] & 0xFF) << 8)
		| ((int)(bytes[9] & 0xFF));

	if (expon == 0 && hiMant == 0 && loMant == 0)
		f = 0;
	else {
		if (expon == 0x7FFF)	/* Infinity or NaN */
			f = HUGE_VAL;
		else {
			expon -= 16383;
			f = ldexp(UnsignedToFloat(hiMant), expon -= 31);
			f += ldexp(UnsignedToFloat(loMant), expon -= 32);
		}
	}

	if (bytes[0] & 0x80)
	{
		f = -f;
	}
	
	return f;
}


}	//	end of namespace Loris
