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
 * AiffFile.C
 *
 * Implementation of AiffFile class for sample import and export in Loris.
 *
 * Class AiffFile represents sample data in a AIFF-format samples 
 * file, and manages file I/O and sample conversion. Since the sound
 * analysis and synthesis algorithms in Loris and the reassigned
 * bandwidth-enhanced representation are monaural, AiffFile manages
 * only monaural (single channel) AIFF-format samples files.
 *
 * Kelly Fitz, 8 Jan 2003 
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */
#if HAVE_CONFIG_H
	#include "config.h"
#endif

#include "AiffFile.h"

#include "AiffData.h"
#include "LorisExceptions.h"
#include "Marker.h"
#include "Notifier.h"
#include "Synthesizer.h"

#include <algorithm>
#include <climits>
#include <fstream>
#include <vector>

//	begin namespace
namespace Loris {


// -- construction --

// ---------------------------------------------------------------------------
//	AiffFile constructor from filename
// ---------------------------------------------------------------------------
//!	Initialize an instance of AiffFile by importing sample data from
//!	the file having the specified filename or path.
//!
//!	\param filename is the name or path of an AIFF samples file
//
AiffFile::AiffFile( const std::string & filename ) :
	notenum_( 60 ),
	rate_( 1 ),		// rate will be overwritten on import
    numchans_( 1 )
{
	readAiffData( filename );
}

// ---------------------------------------------------------------------------
//	AiffFile constructor from parameters, no samples.
// ---------------------------------------------------------------------------
//!	Initialize an instance of AiffFile having the specified sample 
//!	rate, preallocating numFrames samples, initialized to zero.
//!
//!	\param samplerate is the rate at which Partials are rendered
//!	\param numFrames is the initial number of (zero) samples. If
//!	unspecified, no samples are preallocated.
//
AiffFile::AiffFile( double samplerate, size_type numFrames /* = 0 */, 
                    unsigned int numChannels /* = 1 */ ) :
	notenum_( 60 ),
	rate_( samplerate ),
    numchans_( numChannels ),
	samples_( numFrames * numChannels, 0 )
{
}

// ---------------------------------------------------------------------------
//	AiffFile constructor from sample data
// ---------------------------------------------------------------------------
//!	Initialize an instance of AiffFile from a buffer of sample
//!	data, with the specified sample rate.
//!
//!	\param buffer is a pointer to a buffer of floating point samples.
//!	\param bufferlength is the number of samples in the buffer.
//!	\param samplerate is the sample rate of the samples in the buffer.
//
AiffFile::AiffFile( const double * buffer, size_type bufferlength, double samplerate ) :
	notenum_( 60 ),
	rate_( samplerate ),
    numchans_( 1 )
{
	samples_.insert( samples_.begin(), buffer, buffer+bufferlength );
}

// ---------------------------------------------------------------------------
//	AiffFile constructor from stereo sample data
// ---------------------------------------------------------------------------
//!	Initialize an instance of AiffFile from two buffers of sample
//!	data, with the specified sample rate. Both buffers must store
//! the same number (bufferLength) of samples.
//!
//!	\param buffer_left is a pointer to a buffer of floating point samples 
//!        representing the left channel samples.
//!	\param buffer_right is a pointer to a buffer of floating point samples 
//!        representing the right channel samples.
//!	\param bufferlength is the number of samples in the buffer.
//!	\param samplerate is the sample rate of the samples in the buffer.
//
AiffFile::AiffFile( const double * buffer_left, const double * buffer_right, 
                    size_type bufferlength, double samplerate ) :
	notenum_( 60 ),
	rate_( samplerate ),
    numchans_( 2 )
{
    //  interleave the two channels in samples_
    samples_.resize( 2*bufferlength, 0. );
    size_type idx = 0;
    while( idx < samples_.size() )
    {
        samples_[ idx++ ] = *buffer_left++;
        samples_[ idx++ ] = *buffer_right++;
    }    
}

// ---------------------------------------------------------------------------
//	AiffFile constructor from sample data
// ---------------------------------------------------------------------------
//!	Initialize an instance of AiffFile from a vector of sample
//!	data, with the specified sample rate.
//!
//!	\param vec is a vector of floating point samples.
//!	\param samplerate is the sample rate of the samples in the vector.
//
AiffFile::AiffFile( const std::vector< double > & vec, double samplerate ) :
	notenum_( 60 ),
	rate_( samplerate ),
    numchans_( 1 ),
	samples_( vec.begin(), vec.end() )
{
}

// ---------------------------------------------------------------------------
//	AiffFile constructor from stereo sample data
// ---------------------------------------------------------------------------
//!	Initialize an instance of AiffFile from two vectors of sample
//!	data, with the specified sample rate. If the two vectors have different
//! lengths, the shorter one is padded with zeros.
//!
//!	\param vec_left is a vector of floating point samples representing the 
//!        left channel samples.
//!	\param vec_right is a vector of floating point samples representing the 
//!        right channel samples.
//!	\param samplerate is the sample rate of the samples in the vectors.
//
AiffFile::AiffFile( const std::vector< double > & vec_left,
                    const std::vector< double > & vec_right, 
                    double samplerate ) :
	notenum_( 60 ),
	rate_( samplerate ),
    numchans_( 2 ),
	samples_( 2 * std::max( vec_left.size(), vec_right.size() ), 0. )
{
    //  interleave the two channels in samples_
    size_type idx = 0;
    std::vector< double >::const_iterator iter_left = vec_left.begin();
    std::vector< double >::const_iterator iter_right= vec_right.begin();
    while( idx < samples_.size() )
    {
        if ( iter_left != vec_left.end() )
        {
            samples_[ idx ] = *iter_left++;
        }
        if ( iter_right != vec_right.end() )
        {
            samples_[ idx+1 ] = *iter_right++;
        }
        idx += 2;
    }    
}

// ---------------------------------------------------------------------------
//	AiffFile copy constructor 
// ---------------------------------------------------------------------------
//!	Initialize this and AiffFile that is an exact copy, having
//!	all the same sample data, as another AiffFile.
//!
//!	\param other is the AiffFile to copy
//
AiffFile::AiffFile( const AiffFile & other ) :
	notenum_( other.notenum_ ),
	rate_( other.rate_ ),
    numchans_( other.numchans_ ),
	markers_( other.markers_ ),
	samples_( other.samples_ )
{
}
	 
// ---------------------------------------------------------------------------
//	AiffFile assignment operator 
// ---------------------------------------------------------------------------
//!	Assignment operator: change this AiffFile to be an exact copy
//!	of the specified AiffFile, rhs, that is, having the same sample
//!	data.
//!
//!	\param rhs is the AiffFile to replicate
//
AiffFile & 
AiffFile::operator= ( const AiffFile & rhs )
{
	if ( &rhs != this )
	{
		// 	before modifying anything, make
		//	sure there's enough space:
		samples_.reserve( rhs.samples_.size() );
		markers_.reserve( rhs.markers_.size() );
	
		notenum_ = rhs.notenum_;
		rate_ = rhs.rate_;
        numchans_ = rhs.numchans_;
		markers_ = rhs.markers_;
		samples_ = rhs.samples_;
	}
	return *this;
}

// -- export --

// ---------------------------------------------------------------------------
//	write 
// ---------------------------------------------------------------------------
//!	Export the sample data represented by this AiffFile to
//!	the file having the specified filename or path. Export
//!	signed integer samples of the specified size, in bits
//!	(8, 16, 24, or 32).
//!
//!	\param filename is the name or path of the AIFF samples file
//!	to be created or overwritten.
//!	\param bps is the number of bits per sample to store in the
//!	samples file (8, 16, 24, or 32).If unspeicified, 16 bits
//!	is assumed.
//
void
AiffFile::write( const std::string & filename, unsigned int bps )
{
	static const unsigned int ValidSizes[] = { 8, 16, 24, 32 };
	if ( std::find( ValidSizes, ValidSizes+4, bps ) == ValidSizes+4 )
	{
		Throw( InvalidArgument, "Invalid bits-per-sample." );
	}

	std::ofstream s( filename.c_str(), std::ofstream::binary );
	if ( ! s )
	{
		std::string s = "Could not create file \"";
		s += filename;
		s += "\". Failed to write AIFF file.";
		Throw( FileIOException, s );
	}
	
	unsigned long dataSize = 0;

	CommonCk commonChunk;
	configureCommonCk( commonChunk, samples_.size() / numchans_, numchans_, bps, rate_ );
	dataSize += commonChunk.header.size + sizeof(CkHeader);
	
	SoundDataCk soundDataChunk;
	configureSoundDataCk( soundDataChunk, samples_, bps );
	dataSize += soundDataChunk.header.size + sizeof(CkHeader);
	
	InstrumentCk instrumentChunk;
	configureInstrumentCk( instrumentChunk, notenum_ );
	dataSize += instrumentChunk.header.size + sizeof(CkHeader);

	MarkerCk markerChunk;
	if ( ! markers_.empty() )
	{
		configureMarkerCk( markerChunk, markers_, rate_ );
		dataSize += markerChunk.header.size + sizeof(CkHeader);
	}
	
	ContainerCk containerChunk;
	configureContainer( containerChunk, dataSize );
	
	try 
	{
		writeContainer( s, containerChunk );
		writeCommonData( s, commonChunk );
		if ( ! markers_.empty() )
			writeMarkerData( s, markerChunk );
		writeInstrumentData( s, instrumentChunk );
		writeSampleData( s, soundDataChunk );
		
		s.close();
	}
	catch ( Exception & ex ) 
	{
		ex.append( " Failed to write AIFF file." );
		throw;
	}
}

// -- access --

// ---------------------------------------------------------------------------
//	markers 
// ---------------------------------------------------------------------------
//!	Return a reference to the Marker (see Marker.h) container 
//!	for this AiffFile. 
//
AiffFile::markers_type & 
AiffFile::markers( void )
{
	return markers_;
}

//!	Return a const reference to the Marker (see Marker.h) container 
//!	for this AiffFile. 
//
const AiffFile::markers_type & 
AiffFile::markers( void ) const
{
	return markers_;
}

// ---------------------------------------------------------------------------
//	midiNoteNumber 
// ---------------------------------------------------------------------------
//!	Return the fractional MIDI note number assigned to this AiffFile. 
//!	If the sound has no definable pitch, note number 60.0 is used.
//
double 
AiffFile::midiNoteNumber( void ) const
{
	return notenum_;
}

// ---------------------------------------------------------------------------
//	numChannels 
// ---------------------------------------------------------------------------
//!	Return the number of channels of audio samples represented by
//! this AiffFile, 1 for mono, 2 for stereo.
//
unsigned int 
AiffFile::numChannels( void ) const
{
    return numchans_;
}

// ---------------------------------------------------------------------------
//	numFrames 
// ---------------------------------------------------------------------------
//!	Return the number of sample frames represented in this AiffFile.
//!	A sample frame contains one sample per channel for a single sample
//!	interval (e.g. mono and stereo samples files having a sample rate of
//!	44100 Hz both have 44100 sample frames per second of audio samples).
//
 AiffFile::size_type  
 AiffFile::numFrames( void ) const
 {
 	return samples_.size();
 }

// ---------------------------------------------------------------------------
//	sampleRate 
// ---------------------------------------------------------------------------
//!	Return the sampling freqency in Hz for the sample data in this
//!	AiffFile.
//
double  
AiffFile::sampleRate( void ) const
{
	return rate_;
}

// ---------------------------------------------------------------------------
//	samples 
// ---------------------------------------------------------------------------
//!	Return a reference (or const reference) to the vector containing
//!	the floating-point sample data for this AiffFile.
//
AiffFile::samples_type & 
AiffFile::samples( void )
{
	return samples_;
}

//!	Return a const reference (or const reference) to the vector containing
//!	the floating-point sample data for this AiffFile.
//
const AiffFile::samples_type & 
AiffFile::samples( void ) const
{
	return samples_;
}

// -- mutation --

// ---------------------------------------------------------------------------
//	addPartial 
// ---------------------------------------------------------------------------
//! Render the specified Partial using the (optionally) specified
//! Partial fade time (see Synthesizer.h for an examplanation 
//!	of fade time), and accumulate the resulting samples into
//! the sample vector for this AiffFile. Other synthesis parameters 
//!	are taken from the Synthesizer DefaultParameters.
//!
//!	\sa Synthesizer::DefaultParameters
//!
//!	\param p is the partial to render into this AiffFile
//!	\param fadeTime is the Partial fade time for rendering
//!	the Partials on the specified range. If unspecified, the
//! fade time is taken from the Synthesizer DefaultParameters.
//
void 
AiffFile::addPartial( const Loris::Partial & p, double fadeTime )
{
    Synthesizer synth = configureSynthesizer( fadeTime );    
	synth.synthesize( p );
}

// ---------------------------------------------------------------------------
//	setMidiNoteNumber 
// ---------------------------------------------------------------------------
//!	Set the fractional MIDI note number assigned to this AiffFile. 
//!	If the sound has no definable pitch, use note number 60.0 (the default).
//!
//!	\param nn is a fractional MIDI note number, 60 is middle C.
//
void 
AiffFile::setMidiNoteNumber( double nn )
{
	if ( nn < 0 || nn > 128 )
	{
		Throw( InvalidArgument, "MIDI note number outside of the valid range [1,128]" );
	}
	notenum_ = nn;
}

// -- helpers --

// ---------------------------------------------------------------------------
//	configureSynthesizer 
// ---------------------------------------------------------------------------
//	Construct a Synthesizer for rendering Partials and set its fadeTime.
//	Modify the default synthesizer parameters with this file's sample rate
//	and, if specified (not equal to FadeTimeUnspecified), the fade time.
//
Synthesizer 
AiffFile::configureSynthesizer( double fadeTime )
{
	Synthesizer::Parameters params;
	params.sampleRate = rate_;
	
	if ( FadeTimeUnspecified != fadeTime )
	{
		params.fadeTime = fadeTime;
	}
	
	return Synthesizer( params, samples_ );
}

// ---------------------------------------------------------------------------
//	readAiffData
// ---------------------------------------------------------------------------
//	Import data from an AIFF file on disk.
//
void 
AiffFile::readAiffData( const std::string & filename )
{
	ContainerCk containerChunk;
	CommonCk commonChunk;
	SoundDataCk soundDataChunk;
	InstrumentCk instrumentChunk;
	MarkerCk markerChunk;

	try 
	{
		std::ifstream s( filename.c_str(), std::ifstream::binary );
	
		//	the Container chunk must be first, read it:
		readChunkHeader( s, containerChunk.header );
        if ( !s )
        {
			Throw( FileIOException, "File not found, or corrupted." );
        }
		if ( containerChunk.header.id != ContainerId )
        {
			Throw( FileIOException, "Found no Container chunk." );
        }
		readContainer( s, containerChunk, containerChunk.header.size );
		
		//	read other chunks, we are only interested in
		//	the Common chunk, the Sound Data chunk, the Markers: 
		CkHeader h;
		while ( readChunkHeader( s, h ) )
		{			
			switch (h.id)
			{
				case CommonId:
					readCommonData( s, commonChunk, h.size );
					if ( commonChunk.channels != 1 )
					{
						Throw( FileIOException, 
							   "Loris only processes single-channel AIFF samples files." );
					}					
					if ( commonChunk.bitsPerSample != 8 &&
						 commonChunk.bitsPerSample != 16 &&
						 commonChunk.bitsPerSample != 24 &&
						 commonChunk.bitsPerSample != 32 )
					{
						Throw( FileIOException, "Unrecognized sample size." );
					}										
					break;
				case SoundDataId:
					readSampleData( s, soundDataChunk, h.size );
					break;
				case InstrumentId:
					readInstrumentData( s, instrumentChunk, h.size );
					break;
				case MarkerId:
					readMarkerData( s, markerChunk, h.size );
					break;
				default:
					s.ignore( h.size );
			}
		}
	
		if ( ! commonChunk.header.id || ! soundDataChunk.header.id )
		{
			Throw( FileIOException, 
				   "Reached end of file before finding both a Common chunk and a Sound Data chunk." );
		}
	}
	catch ( Exception & ex ) 
	{
		ex.append( " Failed to read AIFF file." );
		throw;
	}
	
	
	//	all the chunks have been read, use them to initialize
	//	the AiffFile members:
	rate_ = commonChunk.srate;
	
	if ( instrumentChunk.header.id )
	{
		notenum_ = instrumentChunk.baseNote;
		notenum_ -= 0.01 * instrumentChunk.detune;
	}
	
	if ( markerChunk.header.id )
	{
		for ( int j = 0; j < markerChunk.numMarkers; ++j )
		{
			MarkerCk::Marker & m = markerChunk.markers[j];
			markers_.push_back( Marker( m.position / rate_, m.markerName ) );
		}		
	}
	
	convertBytesToSamples( soundDataChunk.sampleBytes, samples_, commonChunk.bitsPerSample );
	if ( samples_.size() != commonChunk.sampleFrames )
	{
		notifier << "Found " << samples_.size() << " frames of "
				 << commonChunk.bitsPerSample << "-bit sample data." << endl;
		notifier << "Header says there should be " << commonChunk.sampleFrames 
				 << "." << endl;
	}
}


}	//	end of namespace Loris
