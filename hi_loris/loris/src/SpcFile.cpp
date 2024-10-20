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
 * SpcFile.C
 *
 * Implementation of SpcFile class for Partial import and export for
 * real-time synthesis in Kyma.
 *
 * Spc files always represent a number of Partials that is a power of
 * two. This is not necessary for purely-sinusoidal files, but might be
 * (not clear) for enhanced data to be properly processed in Kyma. 
 *
 * All of this is kind of disgusting right now. This code has evolved 
 * somewhat randomly, and we are awaiting full support for bandwidth-
 * enhanced data in Kyma..
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

#include "SpcFile.h"

#include "AiffData.h"
#include "Breakpoint.h"
#include "BigEndian.h"
#include "LorisExceptions.h"
#include "Marker.h"
#include "Notifier.h"
#include "PartialUtils.h"

#include <algorithm>
#include <cmath>
#include <fstream>

#if defined(HAVE_M_PI) && (HAVE_M_PI)
    const double Pi = M_PI;
#else
    const double Pi = 3.14159265358979324;
#endif


//  Define this if SpcFiles should always have a number of 
//  Partials that is a power of two. Cannot decide whether 
//  or not they should. Lip?
#define PO2


using std::exp;
using std::log;
using std::sqrt;

//  begin namespace
namespace Loris {

// ---------------------------------------------------------------------------
//  constants, can, or should, these be made variable?
// ---------------------------------------------------------------------------

//  can't we change this? Seems like we could, but 
//  its part of the size of the magic junk in the 
//  Sose chunk.
const int LargestLabel = 256;   // max number of partials for SPC file

    //  1 Mar 05
    //  Found that this cannot actually be 512 for enhanced
    //  Partials, it crashes with a segmentation fault above 256. 
    //  Unfortunately, whatever is causing the problem is not
    //  using this constant, but some other hardcoded thing.
    //  if I change this to 256, then I can still export
    //  256 enhanced Partials, but I don't know what to change
    //  to allow 512! Ugh!
    //  -kel


//  this used to be hard coded into Partial, don't know
//  whether it is needed to make spc files work.
const double Fade = 0.001;

//  this always has to be 24 bits, 1 channel
const int Bps = 24;
const int Nchans = 1;


// ---------------------------------------------------------------------------
//  static helper prototypes, defined at bottom
// ---------------------------------------------------------------------------
static void 
configureEnvelopeDataCk( SoundDataCk & ck, const SpcFile::partials_type & partials );

static void 
configureSosMarkerCk( MarkerCk & ck, const std::vector< Marker > & markers  );

static void
configureSosEnvelopesCk( SosEnvelopesCk & ck );

static std::ostream & 
writeSosEnvelopesChunk( std::ostream & s, const SosEnvelopesCk & ck );

static void
configureExportStruct( const SpcFile::partials_type & partials, double midipitch,
                       double endApproachTime, int enhanced );

static unsigned long getNumSampleFrames( void );

const int SpcFile::MinNumPartials = 32;
const double SpcFile::DefaultRate = 44100.;

// -- construction --
// ---------------------------------------------------------------------------
//  SpcFile constructor from filename
// ---------------------------------------------------------------------------
//  Initialize an instance of SpcFile by importing envelope parameter 
//  streams from the file having the specified filename or path.
//
SpcFile::SpcFile( const std::string & filename ) :
    notenum_( 60 ),
    rate_( DefaultRate )
{
    readSpcData( filename );
}

// ---------------------------------------------------------------------------
//  SpcFile constructor, empty.
// ---------------------------------------------------------------------------
//  Initialize an instance of SpcFile having the specified fractional
//  MIDI note number, and no Partials (or envelope parameter streams). 
//
SpcFile::SpcFile( double midiNoteNum ) :
    notenum_( midiNoteNum ),
    rate_( DefaultRate )
{
    growPartials( MinNumPartials );
}

// ---------------------------------------------------------------------------
//  write 
// ---------------------------------------------------------------------------
//  Export the phase-correct bandwidth-enhanced envelope parameter 
//  streams represented by this SpcFile to the file having the specified 
//  filename or path. 
//  
//  A nonzero endApproachTime indicates that the Partials do not include a
//  release or decay, but rather end in a static spectrum corresponding to the
//  final Breakpoint values of the partials. The endApproachTime specifies how
//  long before the end of the sound the amplitude, frequency, and bandwidth
//  values are to be modified to make a gradual transition to the static spectrum.
//  
//  If the endApproachTime is not specified, it is assumed to be zero, 
//  corresponding to Partials that decay or release normally.
//
void 
SpcFile::write( const std::string & filename, double endApproachTime )
{
    write( filename, true, endApproachTime );
}

// ---------------------------------------------------------------------------
//  write 
// ---------------------------------------------------------------------------
//  Export the pure sinsoidal (omitting phase and bandwidth data) envelope 
//  parameter streams represented by this SpcFile to the file having the 
//  specified filename or path. 
//  
//  A nonzero endApproachTime indicates that the Partials do not include a
//  release or decay, but rather end in a static spectrum corresponding to the
//  final Breakpoint values of the partials. The endApproachTime specifies how
//  long before the end of the sound the amplitude, frequency, and bandwidth
//  values are to be modified to make a gradual transition to the static spectrum.
//  
//  If the endApproachTime is not specified, it is assumed to be zero, 
//  corresponding to Partials that decay or release normally.
//
void 
SpcFile::writeSinusoidal( const std::string & filename, double endApproachTime )
{
    write( filename, false, endApproachTime );
}

// ---------------------------------------------------------------------------
//  write 
// ---------------------------------------------------------------------------
//  Export the envelope parameter streams represented by this SpcFile to
//  the file having the specified filename or path. Export phase-correct 
//  bandwidth-enhanced envelope parameter streams if enhanced is true 
//  (the default), or pure sinsoidal streams otherwise.
//
//  
//  A nonzero endApproachTime indicates that the Partials do not include a
//  release or decay, but rather end in a static spectrum corresponding to the
//  final Breakpoint values of the partials. The endApproachTime specifies how
//  long before the end of the sound the amplitude, frequency, and bandwidth
//  values are to be modified to make a gradual transition to the static spectrum.
//  
//  If the endApproachTime is not specified, it is assumed to be zero, 
//  corresponding to Partials that decay or release normally.
//
//  This version of write is deprecated, but is handy for internal use,
//  called by the above write and writeSiunsoidal members.
//
void
SpcFile::write( const std::string & filename, bool enhanced, double endApproachTime )
{
    if ( endApproachTime < 0 )
    {
        Throw( InvalidArgument, "End Approach Time may not be negative." );
    }
    
    std::ofstream s( filename.c_str(), std::ofstream::binary );
    if ( ! s )
    {
        std::string s = "Could not create file \"";
        s += filename;
        s += "\". Failed to write Spc file.";
        Throw( FileIOException, s );
    }
    
    //  have to do this before trying to do anything else:
    configureExportStruct( partials_, notenum_, endApproachTime, enhanced );
    
    unsigned long dataSize = 0;

    CommonCk commonChunk;
    configureCommonCk( commonChunk, getNumSampleFrames(), Nchans, Bps, rate_ );
    dataSize += commonChunk.header.size + sizeof(CkHeader);
    
    SoundDataCk soundDataChunk;
    configureEnvelopeDataCk( soundDataChunk, partials_ );
    dataSize += soundDataChunk.header.size + sizeof(CkHeader);
    
    InstrumentCk instrumentChunk;
    configureInstrumentCk( instrumentChunk, notenum_ );
    dataSize += instrumentChunk.header.size + sizeof(CkHeader);

    MarkerCk markerChunk;
    if ( ! markers_.empty() )
    {
        configureSosMarkerCk( markerChunk, markers_ );
        dataSize += markerChunk.header.size + sizeof(CkHeader);
    }
    
    SosEnvelopesCk soseChunk;
    configureSosEnvelopesCk( soseChunk );
    dataSize += soseChunk.header.size + sizeof(CkHeader);
    
    ContainerCk containerChunk;
    configureContainer( containerChunk, dataSize );
    
    try 
    {
        writeContainer( s, containerChunk );
        writeCommonData( s, commonChunk );
        if ( ! markers_.empty() )
            writeMarkerData( s, markerChunk );
        writeInstrumentData( s, instrumentChunk );
        writeSosEnvelopesChunk( s, soseChunk );
        writeSampleData( s, soundDataChunk );
        
        s.close();
    }
    catch ( Exception & ex ) 
    {
        ex.append( " Failed to write Spc file." );
        throw;
    }
}

// -- access --

// ---------------------------------------------------------------------------
//  markers 
// ---------------------------------------------------------------------------
//  Return a reference to the Marker (see Marker.h) container 
//  for this SpcFile. 
//
SpcFile::markers_type & 
SpcFile::markers( void )
{
    return markers_;
}

const SpcFile::markers_type & 
SpcFile::markers( void ) const
{
    return markers_;
}

// ---------------------------------------------------------------------------
//  midiNoteNumber 
// ---------------------------------------------------------------------------
//  Return the fractional MIDI note number assigned to this SpcFile. 
//  If the sound has no definable pitch, note number 60.0 is used.
//
double 
SpcFile::midiNoteNumber( void ) const
{
    return notenum_;
}

// ---------------------------------------------------------------------------
//  partials 
// ---------------------------------------------------------------------------
//  Return a read-only (const) reference to the bandwidth-enhanced
//  Partials represented by the envelope parameter streams in this SpcFile.
//
const SpcFile::partials_type & 
SpcFile::partials( void ) const
{
    return partials_;
}

// ---------------------------------------------------------------------------
//  sampleRate 
// ---------------------------------------------------------------------------
//  Return the sampling freqency in Hz for the spc data in this
//  SpcFile. This is the rate at which Kyma must be running to ensure
//  proper playback of bandwidth-enhanced Spc data.
//
double 
SpcFile::sampleRate( void ) const
{
    return rate_;
}

// -- mutation --

// ---------------------------------------------------------------------------
//  addPartial 
// ---------------------------------------------------------------------------
//  Add the specified Partial to the enevelope parameter streams
//  represented by this SpcFile. 
//  
//  A SpcFile can contain only one Partial having any given (non-zero) 
//  label, so an added Partial will replace a Partial having the 
//  same label, if such a Partial exists.
//
//  This may throw an InvalidArgument exception if an attempt is made
//  to add unlabeled Partials, or Partials labeled higher than the
//  allowable maximum.
//
void 
SpcFile::addPartial( const Partial & p )
{
    addPartial( p, p.label() );
}

// ---------------------------------------------------------------------------
//  addPartial 
// ---------------------------------------------------------------------------
//  Add a Partial, assigning it the specified label (and position in the
//  Spc data). 
//
//  A SpcFile can contain only one Partial having any given (non-zero) 
//  label, so an added Partial will replace a Partial having the 
//  same label, if such a Partial exists.
//
//  This may throw an InvalidArgument exception if an attempt is made
//  to add unlabeled Partials, or Partials labeled higher than the
//  allowable maximum.
//
void 
SpcFile::addPartial( const Partial & p, int label  )
{
    if ( p.label() == 0 )
    {
        Throw( InvalidArgument, "Spc Partials must be labeled." );
    }
    if ( label < 1 )
    {
        Throw( InvalidArgument, "Spc Partials must have positive labels." );
    }
    if ( label > LargestLabel )
    {
        Throw( InvalidArgument, "Spc Partial label is too large, cannot have more than 256." );
    }

    if ( label > partials_.size() )
    {
        growPartials( label );
    }
        
    partials_[label - 1] = p;
    partials_[label - 1].setLabel( label );
}

// ---------------------------------------------------------------------------
//  setMidiNoteNumber 
// ---------------------------------------------------------------------------
//  Set the fractional MIDI note number assigned to this SpcFile. 
//  If the sound has no definable pitch, use note number 60.0 (the default).
//
void 
SpcFile::setMidiNoteNumber( double nn )
{
    if ( nn < 0 || nn > 128 )
    {
        Throw( InvalidArgument, "MIDI note number outside of the valid range [1,128]" );
    }
    notenum_ = nn;
}

// ---------------------------------------------------------------------------
//  setSampleRate 
// ---------------------------------------------------------------------------
//  Set the sampling freqency in Hz for the spc data in this
//  SpcFile. This is the rate at which Kyma must be running to ensure
//  proper playback of bandwidth-enhanced Spc data.
//
void 
SpcFile::setSampleRate( double rate )
{
    if ( rate <= 0 )
    {
        Throw( InvalidArgument, "Sample rate must be positive." );
    }
    rate_ = rate;
}

// -- helpers --

// ---------------------------------------------------------------------------
//  growPartials 
// ---------------------------------------------------------------------------
//
void 
SpcFile::growPartials( partials_type::size_type sz )
{
    if ( partials_.size() < sz )
    {
#ifdef PO2
        partials_type::size_type po2sz = MinNumPartials;
        while ( po2sz < sz )
        {
            po2sz *= 2;
        }
        partials_.resize( po2sz );
#else
        partials_.resize( sz );
#endif
        for ( partials_type::size_type j = 0; j < partials_.size(); ++j )
        {
            partials_[j].setLabel( j+1 );
        }
    }
}

// -- export structures --

// ---------------------------------------------------------------------------
//  Export Structures
// ---------------------------------------------------------------------------
//

//  structure for export information
struct SpcExportInfo
{
    double midipitch;       //  note number (69.00 = A440) for spc file;
                            //  this is the core parameter, others are, by default,
                            //  computed from this one
    double endApproachTime; //  in seconds, this indicates how long before the end of the sound the
                            //  amplitude, frequency, and bandwidth values are to be modified to
                            //  make a gradual transition to the spectral content at the end,
                            //  0.0 indicates no such modifications are to be done
    int numPartials;        //  number of partials in spc file
    int fileNumPartials;    //  the actual number of partials plus padding to make a 2**n value
    int enhanced;           //  true for bandwidth-enhanced spc file, false for pure sines
    double startTime;       //  in seconds, time of first frame in spc file
    double endTime;         //  in seconds, this indicates the time at which to truncate the end
                            //  of the spc file, 0.0 indicates no truncation
    double markerTime;      //  in seconds, this indicates time at which a marker is inserted in the
                            //  spc file, 0.0 indicates no marker is desired
    double sampleRate;      //  in hertz, intended sample rate for synthesis of spc file
    double hop;             //  hop size, based on numPartials and sampleRate
    double ampEpsilon;      //  small amplitude value (related to lsb value in spc file log-amp)
};

static struct SpcExportInfo spcEI;      // yikky global spc Export information


// -- export helpers by Lippold --

// ---------------------------------------------------------------------------
//  fileNumPartials
// ---------------------------------------------------------------------------
//  Find number of partials in SOS file.  This is the actual number of partials,
//  plus padding to make a 2**n value. 
//
static int fileNumPartials( int partials )
{
    if ( partials <= 32 )   
        return 32;
    if ( partials <= 64 )   
        return 64;
    if ( partials <= 128 )  
        return 128;
    else if ( partials <= 256 )
        return 256;
    else if ( partials <= LargestLabel )
        return LargestLabel;

    Throw( FileIOException, "Too many SPC partials!" );
    return LargestLabel;
}

// ---------------------------------------------------------------------------
//  envLog( )
// ---------------------------------------------------------------------------
//  For a range 0 to 1, this returns a log value, 0x0000 to 0xFFFF.
//
static unsigned long envLog( double floatingValue )
{
    static double coeff = 65535.0 / log( 32768. );

    return (unsigned long)( coeff * log( 32768.0 * floatingValue + 1.0 ) );

}   //  end of envLog( )

// ---------------------------------------------------------------------------
//  envExp( )
// ---------------------------------------------------------------------------
//  For a range 0x0000 to 0xFFFF, this returns an exponentiated value in the range 0..1.
//  This is the counterpart of SpcFile::envLog().
//
static double envExp( long intValue )
{
    static double coeff = 65535.0 / log( 32768. );

    return ( exp( intValue / coeff ) - 1.0 ) / 32768.0;

}   //  end of envExp( )

// ---------------------------------------------------------------------------
//  getPhaseRefTime
// ---------------------------------------------------------------------------
//  Find the time at which to reference phase.
//  The time will be shortly after amplitude onset, if we are before the onset.
//
static double getPhaseRefTime( int label, const Partial & p, double time  )
{
// Keep array of previous values to optimize spc export.
// This depends on this routine being called in increasing-time order.
    static double prevPRT[LargestLabel + 1];
    if ( prevPRT[label] > time && time > spcEI.startTime )
        return prevPRT[ label ]; 
            
// Go forward to nonzero amplitude.
    while ( p.amplitudeAt( time, Fade ) < spcEI.ampEpsilon && time < spcEI.endTime + spcEI.hop)
    {
        time += spcEI.hop;
    }

    prevPRT[ label ] = time;

// Use phase value at initial onset time.
    return time;
        
}

// ---------------------------------------------------------------------------
//  afbp
// ---------------------------------------------------------------------------
//  Find amplitude, frequency, bandwidth, phase value.  
//
static void afbp( const Partial & p, double time, double phaseRefTime,
                  double magMult, double freqMult, 
                  double & amp, double & freq, double & bw, double & phase)
{   
    
// Optional endApproachTime processing:
// Approach amp, freq, and bw values at endTime, and stick at endTime amplitude.
// We avoid a sudden transition when using stick-at-end-frame sustains.
// Compute weighting factor between "normal" envelope point and static point.
    if ( spcEI.endApproachTime && time > spcEI.endTime - spcEI.endApproachTime )
    {
        if ( time > p.endTime() && p.endTime() > spcEI.endTime - 2 * spcEI.hop)
            time = p.endTime();
        double wt = ( spcEI.endTime - time ) / spcEI.endApproachTime;
        amp   = magMult  * ( wt * p.amplitudeAt( time, Fade ) + 
                             (1.0 - wt) * p.amplitudeAt( spcEI.endTime, Fade )  );
        freq  = freqMult * ( wt * p.frequencyAt( time ) + (1.0 - wt) * p.frequencyAt( spcEI.endTime )  );
        bw    =            ( wt * p.bandwidthAt( time ) + (1.0 - wt) * p.bandwidthAt( spcEI.endTime )  );
        phase = p.phaseAt( time );
    }
    
// If we are before the phase reference time, or on the final frame,
// use zero amp and offset phase.
    else if ( time < phaseRefTime - spcEI.hop / 2 || time > spcEI.endTime - spcEI.hop / 2 )
    {
        amp = 0.;
        freq = freqMult * p.frequencyAt( phaseRefTime );
        bw = 0.;
        phase = p.phaseAt( phaseRefTime ) - 2. * Pi * (phaseRefTime - time) * freq;
    }
    
// Use envelope values at "time".
    else
    {
        amp = magMult * p.amplitudeAt( time, Fade );
        freq = freqMult * p.frequencyAt( time );
        bw = p.bandwidthAt( time );
        phase = p.phaseAt( time );
    }
}

// ---------------------------------------------------------------------------
//  pack
// ---------------------------------------------------------------------------
//  Pack envelope breakpoint value for interpretation by Envelope Reader sounds
//  in Kyma.  The packed result is two 24-bit quantities, lval and rval.
//
//  In lval, the log of the sine magnitude occupies the top 8 bits, the log of the
//  frequency occupies the bottom 16 bits.
//
//  In rval, the log of the noise magnitude occupies the top 8 bits, the scaled
//  linear phase occupies the bottom 16 bits. 
//
//  lval and rval are pointers to 3-bytes each, filled in by this function.
//
static void pack( double amp, double freq, double bw, double phase,
                  Byte * lbytes, Byte * rbytes )
{   

// Set phase for one hop earlier, so that Kyma synthesis target phase is correct.
// Add offset to phase for difference between Kyma and Loris representation.
    phase -= 2. * Pi * spcEI.hop * freq; 
    phase += Pi / 2;

// Make phase into range 0..1.  
    phase = std::fmod( phase, 2. * Pi );
    while ( phase < 0. ) 	// used to be if, I think it should be while
    {						// -kel 12 May 2006
        phase += 2. * Pi; 
    }
    double zeroToOnePhase = phase / (2. * Pi);

// Make frequency into range 0..1.
    double zeroToOneFreq = freq / 22050.0;      // 0..1 , 1 is 22.050 kHz

// Compute sine magnitude and noise magnitude from amp and bw.  
    double theSineMag = amp * sqrt( 1. - bw );
    double theNoiseMag = 64.0 * amp * sqrt( bw );
    if (theNoiseMag > 1.0)
        theNoiseMag = 1.0;

// Pack amp and freq into 24 least-significant bits of lval:    
// 7 bits of log-sine-amplitude with 16 bits of zero to right.
// 16 bits of log-frequency with 0 bits of zero to right.
    unsigned long lval;
    lval = ( envLog( theSineMag ) & 0xFE00 ) << 7;
    lval |= ( envLog( zeroToOneFreq ) & 0xFFFF );
    
//  store in lbytes:
//  store the sample bytes in big endian order, 
//  most significant byte first:
    const int BytesPerSample = 3;
    for ( int j = BytesPerSample; j > 0; --j )
    {
        //  mask the lowest byte after shifting:
        *(lbytes++) = 0xFF & (lval >> (8*(j-1)));
    }

// Pack noise amp and phase into 24 least-significant bits of rval:
// 7 bits of log-noise-amplitude with 16 bits of zero to right.
// 16 bits of phase with 0 bits of zero to right.
    unsigned long rval;
    rval = ( envLog( theNoiseMag ) & 0xFE00 ) << 7;
    rval  |= ( (unsigned long) ( zeroToOnePhase * 0xFFFF ) );

//  store in rbytes:
//  store the sample bytes in big endian order, 
//  most significant byte first:
    for ( int j = BytesPerSample; j > 0; --j )
    {
        //  mask the lowest byte after shifting:
        *(rbytes++) = 0xFF & (rval >> (8*(j-1)));
    }
}

// ---------------------------------------------------------------------------
//  packEnvelopes
// ---------------------------------------------------------------------------
//  The partials should be labeled and distilled before this is called.
//
static bool notEmpty( const Partial & p )  { return p.size() > 0; }

static void packEnvelopes( const SpcFile::partials_type & partials, 
                           std::vector< Byte > & bytes )
{   
//  Assert( partials.size() == spcEI.fileNumPartials );

    int frames = int( ( spcEI.endTime - spcEI.startTime ) / spcEI.hop ) + 1;
    unsigned long dataSize = 
        frames * spcEI.fileNumPartials * ( 24 / 8 ) * (spcEI.enhanced ? 2 : 1);
    bytes.clear();
    bytes.reserve( dataSize );
    
    // get the reference partial; the lowest-nonzero-labeled partial with any breakpoints
    SpcFile::partials_type::const_iterator pos = 
        std::find_if( partials.begin(), partials.end(), notEmpty );
    Assert( pos != partials.end() );
    const Partial & refPar = *pos;
    int refLabel = refPar.label();
    Assert( (refLabel - 1) == (pos - partials.begin()) );
    
    // write out one frame at a time:
    for (double tim = spcEI.startTime; tim <= spcEI.endTime; tim += spcEI.hop ) 
    {
        //  for each frame, write one value for every partial:
        //  (this loop extends to the pad partials)
        for (unsigned int label = 1; label <= spcEI.fileNumPartials; ++label ) 
        {
            double amp, freq, bw, phase;
            //  find partial with the correct label
            //  if partial with the correct is empty, 
            //  frequency-multiply the reference partial
#ifndef PO2
            if ( label > partials.size() || partials[ label - 1 ].size() == 0 )
#else
            if ( partials[ label - 1 ].size() == 0 )
#endif
            {
                //  find the reference time for the phase
                double phaseRefTime = getPhaseRefTime( label, refPar, tim );
                
                //  find amplitude, frequency, bandwidth, phase value
                double freqMult = (double) label / (double) refLabel; 
                double magMult = 0.0;
                afbp( refPar, tim, phaseRefTime, magMult, freqMult, amp, freq, bw, phase );
            }
            else
            {
                //  find the reference time for the phase
                double phaseRefTime = getPhaseRefTime( label, partials[ label - 1 ], tim );
                
                //  find amplitude, frequency, bandwidth, phase value
                afbp( partials[ label - 1 ], tim, phaseRefTime, 1, 1, amp, freq, bw, phase );
            }
            
            //  pack log amplitude and log frequency into 24-bit lval,
            //  log bandwidth and phase into 24-bit rval:
            Byte leftbytes[3], rightbytes[3];
            pack( amp, freq, bw, phase, leftbytes, rightbytes);
    
            //  pack integer samples into the Byte vector without 
            //  byte swapping, they are already correctly packed
            //  (see pack above): 
            bytes.insert( bytes.end(), leftbytes, leftbytes + 3 );
            if ( spcEI.enhanced )
            {
                bytes.insert( bytes.end(), rightbytes, rightbytes + 3 );
            }               
        }
    }
    
    Assert( bytes.size() == dataSize );
}

// ---------------------------------------------------------------------------
//  configureEnvelopeDataCk
// ---------------------------------------------------------------------------
//  Configure a special SoundDataCk for exporting Spc envelopes.
//
static void 
configureEnvelopeDataCk( SoundDataCk & ck, const SpcFile::partials_type & partials  )
{
    packEnvelopes( partials, ck.sampleBytes );
    
    ck.header.id = SoundDataId;

    //  size is everything after the header:
    ck.header.size = sizeof(Uint_32) +      //  offset
                     sizeof(Uint_32) +      //  block size
                     ck.sampleBytes.size(); //  sample data

    //  no block alignment: 
    ck.offset = 0;
    ck.blockSize = 0;
}

// ---------------------------------------------------------------------------
//  configureSosMarkerCk
// ---------------------------------------------------------------------------
//  Spc needs a special version of this, because Marker times have to be
//  rounded to the nearest frame.
//
void 
configureSosMarkerCk( MarkerCk & ck, const std::vector< Marker > & markers  )
{
    ck.header.id = MarkerId;

    //  accumulate data size
    Uint_32 dataSize = sizeof(Uint_16); //  num markers
    
    ck.numMarkers = markers.size();
    ck.markers.resize( markers.size() );
    for ( int j = 0; j < markers.size(); ++j )
    {
        MarkerCk::Marker & m = ck.markers[j];
        m.markerID = j+1;
        //m.position = Uint_32((markers[j].time() * srate) + 0.5);
        
        //  align marker with nearest frame time:
        m.position = Uint_32( markers[j].time() / spcEI.hop ) 
                     * spcEI.fileNumPartials 
                     * ( spcEI.enhanced ? 2 : 1 ); 

        m.markerName = markers[j].name();
        
        #define MAX_PSTRING_CHARS 254
        if ( m.markerName.size() > MAX_PSTRING_CHARS )
            m.markerName.resize( MAX_PSTRING_CHARS );
        
        //  the size of a pascal string is the number of 
        //  characters plus the size byte, plus the terminal '\0':
        //  
        //  Actualy, at least one web source indicates that Pascal
        //  strings are not null-terminated, but that they _are_
        //  padded with an extra (not part of the count) byte
        //  if necessary to ensure that the total length (including
        //  count) is even, and this seems to work better with other
        //  programs (e.g. Kyma)
        if ( m.markerName.size()%2 == 0 )
                m.markerName += '\0';
        dataSize += sizeof(Uint_16) + sizeof(Uint_32) + (m.markerName.size() + 1);
    }

    //  must be an even number of bytes
    if ( dataSize%2 )
        ++dataSize;

    ck.header.size = dataSize;
}


// ---------------------------------------------------------------------------
//  configureSosEnvelopesCk
// ---------------------------------------------------------------------------
//  Configure a the application-specific chunk for exporting Spc envelopes.
//
static void
configureSosEnvelopesCk( SosEnvelopesCk & ck )
{
    ck.header.id = ApplicationSpecificId;

    //  size is everything after the header:
    ck.header.size = sizeof(Uint_32) +                  // signature
                     sizeof(Uint_32) +                  // enhanced
                     sizeof(Uint_32) +                  // validPartials
                     // this last bit is a big, obsolete array, that
                     // we now use two positions in, an they aren't
                     // even the first two! Truly nasty.
                     4*LargestLabel + 8 * sizeof(Int_32);// initPhase[] et al
    
    ck.signature = SosEnvelopesId;

    ck.enhanced = spcEI.enhanced;
    
    //  the number of partials is doubled in bandwidth-enhanced spc files
    ck.validPartials = spcEI.numPartials * ( spcEI.enhanced ? 2 : 1 );
    
    //  resolution in microseconds
    ck.resolution = long( 1000000.0 * spcEI.hop );
    
    //  all partials quasiharmonic
    //  the number of partials is doubled in bandwidth-enhanced spc files
    ck.quasiHarmonic =  spcEI.numPartials * ( spcEI.enhanced ? 2 : 1); 

}

// ---------------------------------------------------------------------------
//  writeSosEnvelopesChunk
// ---------------------------------------------------------------------------
//
std::ostream & 
writeSosEnvelopesChunk( std::ostream & s, const SosEnvelopesCk & ck )
{
    //  write it out:
    try 
    {
        BigEndian::write( s, 1, sizeof(Int_32), (char *)&ck.header.id );
        BigEndian::write( s, 1, sizeof(Int_32), (char *)&ck.header.size );
        BigEndian::write( s, 1, sizeof(Int_32), (char *)&ck.signature );
        BigEndian::write( s, 1, sizeof(Int_32), (char *)&ck.enhanced );
        BigEndian::write( s, 1, sizeof(Int_32), (char *)&ck.validPartials );
        
        // The SOSresultion and SOSquasiHarmonic fields are in the phase table memory.
        //BigEndian::write( s, initPhaseLth, sizeof(Int_32), (char *)&ck.initPhase[0] );
        static const int InitPhaseLth = ( LargestLabel + 8 );
        Int_32 bogus[ InitPhaseLth ]; // obsolete initial phase array
        std::fill( bogus, bogus + InitPhaseLth, 0 );
        bogus[ ck.validPartials ] = ck.resolution;
        bogus[ ck.validPartials + 1 ] = ck.quasiHarmonic;
        BigEndian::write( s, InitPhaseLth, sizeof(Int_32), (char *)bogus );
    }
    catch( FileIOException & ex ) 
    {
        ex.append( "Failed to write AIFF file Container chunk." );
        throw;
    }
    
    return s;

}

// ---------------------------------------------------------------------------
//  computeHop
// ---------------------------------------------------------------------------
//  Find the hop size, based on number of partials and sample rate. 
//
static double computeHop( int numPartials, double sampleRate )
{
    return 2 * numPartials / sampleRate;
}


// ---------------------------------------------------------------------------
//  computeStartTime
// ---------------------------------------------------------------------------
//  Find the start time: the earliest time of any labeled partial.
//
static double computeStartTime( const SpcFile::partials_type & pars )
{
    double startTime = 1000.;
    for ( SpcFile::partials_type::const_iterator pIter = pars.begin(); pIter != pars.end(); ++pIter )
        if ( pIter->size() > 0 && startTime > pIter->startTime() && pIter->label() > 0 )
            startTime = pIter->startTime();
    return startTime;
}


// ---------------------------------------------------------------------------
//  computeEndTime
// ---------------------------------------------------------------------------
//  Find the end time: the latest time of any labeled partial.
//
static double computeEndTime( const SpcFile::partials_type & pars )
{
    double endTime = -1000.;
    for ( SpcFile::partials_type::const_iterator pIter = pars.begin(); pIter != pars.end(); ++pIter )
        if ( pIter->size() > 0 && endTime < pIter->endTime()  && pIter->label() > 0 )
            endTime = pIter->endTime();
    return endTime;
}


// ---------------------------------------------------------------------------
//  computeNumPartials
// ---------------------------------------------------------------------------
//  Find the number of partials.
//
static long computeNumPartials( const SpcFile::partials_type & pars )
{

// We purposely consider partials with no breakpoints, to allow
// a larger number of partials than actually have data.
    int numPartials = 0;
    for ( SpcFile::partials_type::const_iterator pIter = pars.begin(); pIter != pars.end(); ++pIter )
        if ( numPartials < pIter->label() )
            numPartials = pIter->label();

// To ensure a reasonable hop time, make at least 32 partials.
    return numPartials ? std::max( 32, numPartials ) : 0;
}

// ---------------------------------------------------------------------------
//  configureExportStruct
// ---------------------------------------------------------------------------
static void
configureExportStruct( const SpcFile::partials_type & plist, double midipitch,
                       double endApproachTime, int enhanced )
{   
    //  note number (69.00 = A440) for spc file
    spcEI.midipitch = midipitch;
    
    //  enhanced indicates a bandwidth-enhanced spc file; by default it is true.
    //  if enhanced is false, no bandwidth or noise information is exported.
    spcEI.enhanced = enhanced;
    
    //  endApproachTime is in seconds; by default it is zero (and has no effect).
    //  a nonzero endApproachTime indicates that the plist does not include a
    //  release, but rather ends in a static spectrum corresponding to the final
    //  breakpoint values of the partials.  the endApproachTime specifies how
    //  long before the end of the sound the amplitude, frequency, and bandwidth
    //  values are to be modified to make a gradual transition to the static spectrum.
    spcEI.endApproachTime = endApproachTime;
    
    //  number of partials in spc file
    spcEI.numPartials = computeNumPartials( plist );
    spcEI.fileNumPartials = fileNumPartials( spcEI.numPartials );
    
    //  start and end time of spc file
    spcEI.startTime = computeStartTime( plist );
    spcEI.endTime = computeEndTime( plist );

    //  in seconds, this indicates time at which a marker is inserted 
    //  in the spc file, 0.0 indicates no marker.  this is not being used currently.
    spcEI.markerTime = 0.;
        
    //  in hertz, intended sample rate for synthesis of spc file
    spcEI.sampleRate = 44100.;      
    
    //  compute hop size
    spcEI.hop = computeHop( spcEI.numPartials, spcEI.sampleRate );

    //  compute ampEpsilon, a small amplitude value twice the lsb value 
    //  of log amp in packed spc format. 
    spcEI.ampEpsilon = 2. * envExp( 0x200 );

    // Max number of partials is due to (arbitrary) size of initPhase[].
    if ( spcEI.numPartials < 1 || spcEI.numPartials > LargestLabel )
        Throw( FileIOException, "Partials must be distilled and labeled between 1 and 512." );

    debugger << "startTime = " << spcEI.startTime << " endTime = " << spcEI.endTime 
             << " hop = " << spcEI.hop << " partials = " << spcEI.numPartials << endl;
}

// ---------------------------------------------------------------------------
//  getNumSampleFrames
// ---------------------------------------------------------------------------
//  The number of exported sample frames is computed from data stored in
//  the icky global export struct.
//
static unsigned long getNumSampleFrames( void )
{
    int frames = int( ( spcEI.endTime - spcEI.startTime ) / spcEI.hop ) + 1;
    return frames * spcEI.fileNumPartials * ( spcEI.enhanced ? 2 : 1 );
}

// -- import helpers by Lippold --
// ---------------------------------------------------------------------------
//  processEnhancedPoint
// ---------------------------------------------------------------------------
//  Add ehanced-spc breakpoint to existing Loris partials.
//
static void
processEnhancedPoint( Byte * leftbytes, Byte * rightbytes, 
                      const double frameTime, 
                      Partial & par )
{
//  represent bytes as 24 bit integers:
    const int BytesPerSample = 3;   

    //  assign the leading byte, so that the sign
    //  is preserved:
    long left = static_cast<char>(*(leftbytes++));
    for ( int j = 1; j < BytesPerSample; ++j )
    {
        //  OR bytes after the most significant, so
        //  that their sign is ignored:
        left = (left << 8) + (unsigned char)*(leftbytes++);
    }
    
    long right = static_cast<char>(*(rightbytes++));
    for ( int j = 1; j < BytesPerSample; ++j )
    {
        //  OR bytes after the most significant, so
        //  that their sign is ignored:
        right = (right << 8) + (unsigned char)*(rightbytes++);
    }
//
// Unpack values.  
//
    double freq = envExp( left & 0xffff ) * 22050.0;
    double sineMag =     envExp( (left >> 7)  & 0xfe00 );
    double noiseMag = envExp( (right >> 7) & 0xfe00 ) / 64.;
    double phase = ( right & 0xffff ) * ( 2. * Pi / 0xffff );
    
    double total = sineMag * sineMag + noiseMag * noiseMag;

    double amp = sqrt( total );
    
    double noise = 0.;
    if (total != 0.)
        noise = noiseMag * noiseMag / total;
    if (noise > 1.)
        noise = 1.;
    
    phase -= Pi / 2.;
    if (phase < 0.)
        phase += 2. * Pi;

//
// Create a new breakpoint and insert it.
//  
    Breakpoint newbp( freq, amp, noise, phase );
    par.insert( frameTime, newbp );
}

// ---------------------------------------------------------------------------
//  processSineOnlyPoint
// ---------------------------------------------------------------------------
//  Add sine-only spc breakpoint to existing Loris partials.
//
static void
processSineOnlyPoint( Byte * bytes, 
                      const double frameTime, 
                      Partial & par )
{
//  represent bytes as 24 bit integers:
    const int BytesPerSample = 3;   

    //  assign the leading byte, so that the sign
    //  is preserved:
    long packed = static_cast<char>(*(bytes++));
    for ( int j = 1; j < BytesPerSample; ++j )
    {
        //  OR bytes after the most significant, so
        //  that their sign is ignored:
        packed = (packed << 8) + (unsigned char)*(bytes++);
    }

//
// Unpack values.  
//
    double freq = envExp( packed & 0xffff ) * 22050.0;
    double amp =     envExp( (packed >> 7)  & 0xfe00 );
    double noise = 0.;
    double phase = 0.;

//
// Create a new breakpoint and insert it.
//  
    Breakpoint newbp( freq, amp, noise, phase );
    par.insert( frameTime, newbp );
}

// ---------------------------------------------------------------------------
//  readSpcData
// ---------------------------------------------------------------------------
//
void 
SpcFile::readSpcData( const std::string & filename )
{
    ContainerCk containerChunk;
    CommonCk commonChunk;
    SoundDataCk soundDataChunk;
    InstrumentCk instrumentChunk;
    MarkerCk markerChunk;
    SosEnvelopesCk soseChunk;

    try 
    {
        std::ifstream s( filename.c_str(), std::ifstream::binary );
    
        //  the Container chunk must be first, read it:
        readChunkHeader( s, containerChunk.header );
        if( containerChunk.header.id != ContainerId )
            Throw( FileIOException, "Found no Container chunk." );
        readContainer( s, containerChunk, containerChunk.header.size );
        
        //  read other chunks, we are only interested in
        //  the Common chunk, the Sound Data chunk, the Markers: 
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
                case ApplicationSpecificId:
                    readApplicationSpecifcData( s, soseChunk, h.size );
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
        if ( soseChunk.signature != SosEnvelopesId )
        {
            Throw( FileIOException, 
                   "Reached end of file before finding a Spc Envelope Data chunk, this must not be a Spc file." );
        }
    }
    catch ( Exception & ex ) 
    {
        ex.append( " Failed to read Spc file." );
        throw;
    }
    
    //  all the chunks have been read, use them to initialize
    //  the SpcFile members:
    rate_ = commonChunk.srate;

    // why was this like this:
    // double rate  = commonChunk.srate;
    
    if ( instrumentChunk.header.id )
    {
        notenum_ = instrumentChunk.baseNote;
        notenum_ -= 0.01 * instrumentChunk.detune;
    }
    
    //  extract information from SOSe chunk:
    //  enhanced file format has number of partials doubled
    //  sine-only file format has proper number of partials
    bool enhanced = soseChunk.enhanced != 0;
    int numPartials = enhanced ? soseChunk.validPartials / 2 : soseChunk.validPartials;
    int numFrames = commonChunk.sampleFrames / ( fileNumPartials( numPartials ) * ( enhanced ? 2 : 1 ) );
    double hop = soseChunk.resolution * 0.000001;   // resolution is in microseconds
    
    //  read markers, need to compute times corresponding to
    //  spc frames:
    if ( markerChunk.header.id )
    {
        for ( int j = 0; j < markerChunk.numMarkers; ++j )
        {
            MarkerCk::Marker & m = markerChunk.markers[j];
            double markerTime = m.position * hop / ( fileNumPartials( numPartials ) * ( enhanced ? 2 : 1 ) );
            markers_.push_back( Marker( markerTime, m.markerName ) );
        }       
    }


    //  check for valid file
    if ( numPartials == 0 || commonChunk.bitsPerSample != 24 )
            Throw( FileIOException, "Not an SPC file." );
    if ( numPartials < MinNumPartials || numPartials > LargestLabel )
            Throw( FileIOException, "Bad number of partials in SPC file." );

    //  check the number of bytes of Spc data:  
    const int BytesPerSample = 3;
    const int PredictedNumBytes = 
        BytesPerSample * numFrames * fileNumPartials( numPartials ) * ( enhanced ? 2 : 1 );
    if ( soundDataChunk.sampleBytes.size() != PredictedNumBytes )
    {
        notifier << "Found " << soundDataChunk.sampleBytes.size() << " bytes of "
                 << commonChunk.bitsPerSample << "-bit sample data." << endl;
        notifier << "Header says there should be " << PredictedNumBytes
                 << "." << endl;
    }

    //  process SPC data points
    partials_.clear();
    growPartials( numPartials );
    Byte * bytes = &soundDataChunk.sampleBytes.front();
    for ( int frame = 0; frame < numFrames; ++frame ) 
    {
        for ( int partial = 0; partial < fileNumPartials( numPartials ); ++partial )
        {
            if (enhanced)
            {
                Byte * lbytes = bytes;
                bytes += BytesPerSample;
                Byte * rbytes = bytes;
                bytes += BytesPerSample;
                if ( partial < partials_.size() )
                    processEnhancedPoint( lbytes, rbytes, frame * hop, partials_[partial] );
            }
            else
            {
                if ( partial < partials_.size() )
                    processSineOnlyPoint( bytes, frame * hop, partials_[partial] );
                bytes += BytesPerSample;
            }
        }
    }
}

}   //  end of namespace Loris
