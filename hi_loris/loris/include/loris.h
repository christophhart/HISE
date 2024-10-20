#ifndef INCLUDE_LORIS_H
#define INCLUDE_LORIS_H
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
 *    loris.h
 *
 *    Header specifying C-linkable procedural interface for Loris. 
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
 *    loris.h is generated automatically from loris.h.in. Do not modify loris.h
 *
 * Kelly Fitz, 4 Feb 2002
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */
 
/* ---------------------------------------------------------------- */
/*      Version
/*
/*  Define symbols that facilitate version/release identification.
 */
 
#define LORIS_MAJOR_VERSION 1
#define LORIS_MINOR_VERSION 8
#define LORIS_SUBMINOR_VERSION 
#define LORIS_VERSION_STR "Loris 1.8"

/* ---------------------------------------------------------------- */
/*      Types
/*
/* The (class) types Breakpoint, LinearEnvelope, Partial, 
   and PartialList are imported from the Loris namespace.
   The first three are classes, the latter is a typedef
   for std::list< Loris::Partial >. 
 */
#if defined(__cplusplus)
    //    include std library list header, declaring templates
    //    is too painful and fragile:
    #include <list>
    
    //    declare Loris classes in Loris namespace:
    namespace Loris
    {
        class Breakpoint;
        class LinearEnvelope;
        class Partial;
      
        //    this typedef has to be copied from PartialList.h
        typedef std::list< Loris::Partial > PartialList;
    }
   
   // import those names into the global namespace
   using Loris::Breakpoint;
   using Loris::LinearEnvelope;
   using Loris::Partial;
   using Loris::PartialList;
#else 
    /* no classes, just declare types and use
      opaque C pointers 
    */
    typedef struct Breakpoint Breakpoint;
    typedef struct LinearEnvelope LinearEnvelope;
    typedef struct PartialList PartialList;
    typedef struct Partial Partial;
#endif

/*
   TODO
    Maybe should also have loris_label_t and loris_size_t
    defined, depending on configure.
*/

#if defined(__cplusplus)
    extern "C" {
#endif

/* ---------------------------------------------------------------- */
/*      Analyzer configuration
/*
/*  An Analyzer represents a configuration of parameters for
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

void analyze( const double * buffer, unsigned int bufferSize, 
              double srate, PartialList * partials );
/*  Analyze an array of bufferSize (mono) samples at the given sample rate           
    (in Hz) and append the extracted Partials to the given 
    PartialList.                                                 
 */
                       
void analyzer_configure( double resolution, double windowWidth );
/*  Configure the sole Analyzer instance with the specified
    frequency resolution (minimum instantaneous frequency    
    difference between Partials). All other Analyzer parameters     
    are computed from the specified frequency resolution. 
   
    Construct the Analyzer instance if necessary.
   
    In the procedural interface, there is only one Analyzer. 
    It must be configured by calling analyzer_configure before
    any of the other analyzer operations can be performed.   
 */

double analyzer_getAmpFloor( void );
/*  Return the amplitude floor (lowest detected spectral amplitude),              
    in (negative) dB, for the Loris Analyzer.                 
 */

double analyzer_getCropTime( void );
/*  Return the crop time (maximum temporal displacement of a time-
    frequency data point from the time-domain center of the analysis
    window, beyond which data points are considered "unreliable")
    for the Loris Analyzer.
 */

double analyzer_getFreqDrift( void );
/*  Return the maximum allowable frequency difference between                     
    consecutive Breakpoints in a Partial envelope for the Loris Analyzer.                 
 */

double analyzer_getFreqFloor( void );
/*  Return the frequency floor (minimum instantaneous Partial                  
    frequency), in Hz, for the Loris Analyzer.                 
 */

double analyzer_getFreqResolution( void );
/*  Return the frequency resolution (minimum instantaneous frequency          
    difference between Partials) for the Loris Analyzer.     
 */

double analyzer_getHopTime( void );
/*  Return the hop time (which corresponds approximately to the 
    average density of Partial envelope Breakpoint data) for this 
    Analyzer.
 */

double analyzer_getSidelobeLevel( void );
/*  Return the sidelobe attenutation level for the Kaiser analysis window in
    positive dB. Higher numbers (e.g. 90) give very good sidelobe 
    rejection but cause the window to be longer in time. Smaller 
    numbers raise the level of the sidelobes, increasing the likelihood
    of frequency-domain interference, but allow the window to be shorter
    in time.
 */

double analyzer_getWindowWidth( void );
/*  Return the frequency-domain main lobe width (measured between 
    zero-crossings) of the analysis window used by the Loris Analyzer.                 
 */

void analyzer_setAmpFloor( double x );
/*  Set the amplitude floor (lowest detected spectral amplitude), in              
    (negative) dB, for the Loris Analyzer.                 
 */

void analyzer_setBwRegionWidth( double x );
/*  Deprecated, use analyzer_storeResidueBandwidth instead.
 */

void analyzer_setCropTime( double x );
/*  Set the crop time (maximum temporal displacement of a time-
    frequency data point from the time-domain center of the analysis
    window, beyond which data points are considered "unreliable")
    for the Loris Analyzer.
 */

void analyzer_setFreqDrift( double x );
/*  Set the maximum allowable frequency difference between                     
    consecutive Breakpoints in a Partial envelope for the Loris Analyzer.                 
 */

void analyzer_setFreqFloor( double x );
/*  Set the amplitude floor (minimum instantaneous Partial                  
    frequency), in Hz, for the Loris Analyzer.
 */

void analyzer_setFreqResolution( double x );
/*  Set the frequency resolution (minimum instantaneous frequency          
    difference between Partials) for the Loris Analyzer. (Does not cause     
    other parameters to be recomputed.)                                     
 */

void analyzer_setHopTime( double x );
/*  Set the hop time (which corresponds approximately to the average
    density of Partial envelope Breakpoint data) for the Loris Analyzer.
 */

void analyzer_setSidelobeLevel( double x );
/*  Set the sidelobe attenutation level for the Kaiser analysis window in
    positive dB. Larger numbers (e.g. 90) give very good sidelobe 
    rejection but cause the window to be longer in time. Smaller 
    numbers raise the level of the sidelobes, increasing the likelihood
    of frequency-domain interference, but allow the window to be shorter
    in time.
 */

void analyzer_setWindowWidth( double x );
/*  Set the frequency-domain main lobe width (measured between 
    zero-crossings) of the analysis window used by the Loris Analyzer.                 
 */
 
void analyzer_storeResidueBandwidth( double regionWidth );
/*	Construct Partial bandwidth envelopes during analysis
	by associating residual energy in the spectrum (after
	peak extraction) with the selected spectral peaks that
	are used to construct Partials. 
	
	regionWidth is the width (in Hz) of the bandwidth 
	association regions used by this process, must be positive.
 */
 
void analyzer_storeConvergenceBandwidth( double tolerancePct );
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
 */

void analyzer_storeNoBandwidth( void );
/*	Disable bandwidth envelope construction. Bandwidth 
	will be zero for all Breakpoints in all Partials.
 */

double analyzer_getBwRegionWidth( void );
/*  Return the width (in Hz) of the Bandwidth Association regions
	used by this Analyzer, only if the spectral residue method is
	used to compute bandwidth envelopes. Return zero if the mixed
	derivative method is used, or if no bandwidth is computed.
 */

double analyzer_getBwConvergenceTolerance( void );
/*	Return the mixed derivative convergence tolerance
	only if the convergence indicator is used to compute
	bandwidth envelopes. Return zero if the spectral residue
	method is used or if no bandwidth is computed.
 */


/* ---------------------------------------------------------------- */
/*      LinearEnvelope object interface                                
/*
/*  A LinearEnvelope represents a linear segment breakpoint 
    function with infinite extension at each end (that is, the 
    values past either end of the breakpoint function have the 
    values at the nearest end).

    In C++, a LinearEnvelope is a Loris::LinearEnvelope.
 */
 
LinearEnvelope * createLinearEnvelope( void );
/*  Construct and return a new LinearEnvelope having no 
    breakpoints and an implicit value of 0. everywhere, 
    until the first breakpoint is inserted.            
 */

LinearEnvelope * copyLinearEnvelope( const LinearEnvelope * ptr_this );
/*  Construct and return a new LinearEnvelope that is an
    exact copy of the specified LinearEnvelopes, having 
    an identical set of breakpoints.    
 */

void destroyLinearEnvelope( LinearEnvelope * ptr_this );
/*  Destroy this LinearEnvelope.                                 
 */
 
void linearEnvelope_insertBreakpoint( LinearEnvelope * ptr_this,
                                      double time, double val );
/*  Insert a breakpoint representing the specified (time, value) 
    pair into this LinearEnvelope. If there is already a 
    breakpoint at the specified time, it will be replaced with 
    the new breakpoint.
 */

double linearEnvelope_valueAt( const LinearEnvelope * ptr_this, 
                               double time );
/*  Return the interpolated value of this LinearEnvelope at the 
    specified time.                            
 */

/* ---------------------------------------------------------------- */
/*      PartialList object interface
/*
/*  A PartialList represents a collection of Bandwidth-Enhanced 
    Partials, each having a trio of synchronous, non-uniformly-
    sampled breakpoint envelopes representing the time-varying 
    frequency, amplitude, and noisiness of a single bandwidth-
    enhanced sinusoid.

    For more information about Bandwidth-Enhanced Partials and the  
    Reassigned Bandwidth-Enhanced Additive Sound Model, refer to
    the Loris website: www.cerlsoundgroup.org/Loris/.

    In C++, a PartialList is a Loris::PartialList.
 */ 
PartialList * createPartialList( void );
/*  Return a new empty PartialList.
 */
 
void destroyPartialList( PartialList * ptr_this );
/*  Destroy this PartialList.
 */
 
void partialList_clear( PartialList * ptr_this );
/*  Remove (and destroy) all the Partials from this PartialList,
    leaving it empty.
 */
 
void partialList_copy( PartialList * ptr_this, 
                       const PartialList * src );
/*  Make this PartialList a copy of the source PartialList by making
    copies of all of the Partials in the source and adding them to 
    this PartialList.
 */
 
unsigned long partialList_size( const PartialList * ptr_this );
/*  Return the number of Partials in this PartialList.
 */
 
void partialList_splice( PartialList * ptr_this, 
                               PartialList * src );
/*  Splice all the Partials in the source PartialList onto the end of
    this PartialList, leaving the source empty.
 */
 
/* ---------------------------------------------------------------- */
/*      Partial object interface
/*
/*  A Partial represents a single component in the
    reassigned bandwidth-enhanced additive model. A Partial consists of a
    chain of Breakpoints describing the time-varying frequency, amplitude,
    and bandwidth (or noisiness) envelopes of the component, and a 4-byte
    label. The Breakpoints are non-uniformly distributed in time. For more
    information about Reassigned Bandwidth-Enhanced Analysis and the
    Reassigned Bandwidth-Enhanced Additive Sound Model, refer to the Loris
    website: www.cerlsoundgroup.org/Loris/.
 */ 

double partial_startTime( const Partial * p );
/*  Return the start time (seconds) for the specified Partial.
 */

double partial_endTime( const Partial * p );
/*  Return the end time (seconds) for the specified Partial.
 */

double partial_duration( const Partial * p );
/*  Return the duration (seconds) for the specified Partial.
 */

double partial_initialPhase( const Partial * p );
/*  Return the initial phase (radians) for the specified Partial.
 */

int partial_label( const Partial * p );
/*  Return the integer label for the specified Partial.
 */

unsigned long partial_numBreakpoints( const Partial * p );
/*  Return the number of Breakpoints in the specified Partial.
 */

double partial_frequencyAt( const Partial * p, double t );
/*  Return the frequency (Hz) of the specified Partial interpolated
    at a particular time. It is an error to apply this function to
    a Partial having no Breakpoints.
 */

double partial_bandwidthAt( const Partial * p, double t );
/*  Return the bandwidth of the specified Partial interpolated
    at a particular time. It is an error to apply this function to
    a Partial having no Breakpoints.
 */

double partial_phaseAt( const Partial * p, double t );
/*  Return the phase (radians) of the specified Partial interpolated
    at a particular time. It is an error to apply this function to
    a Partial having no Breakpoints.
 */

double partial_amplitudeAt( const Partial * p, double t );
/*  Return the (absolute) amplitude of the specified Partial interpolated
    at a particular time. Partials are assumed to fade out
    over 1 millisecond at the ends (rather than instantaneously).
    It is an error to apply this function to a Partial having no Breakpoints.
 */

void partial_setLabel( Partial * p, int label );
/*   Assign a new integer label to the specified Partial.
 */

/* ---------------------------------------------------------------- */
/*      Breakpoint object interface
/*
/*  A Breakpoint represents a single breakpoint in the
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

double breakpoint_getAmplitude( const Breakpoint * bp );
/*   Return the (absolute) amplitude of the specified Breakpoint.
 */

double breakpoint_getBandwidth( const Breakpoint * bp );
/*  Return the bandwidth coefficient of the specified Breakpoint.
 */

double breakpoint_getFrequency( const Breakpoint * bp );
/*  Return the frequency (Hz) of the specified Breakpoint.
 */

double breakpoint_getPhase( const Breakpoint * bp );
/*  Return the phase (radians) of the specified Breakpoint.
 */

void breakpoint_setAmplitude( Breakpoint * bp, double a );
/*   Assign a new (absolute) amplitude to the specified Breakpoint.
 */

void breakpoint_setBandwidth( Breakpoint * bp, double bw );
/*  Assign a new bandwidth coefficient to the specified Breakpoint.
 */

void breakpoint_setFrequency( Breakpoint * bp, double f );
/*  Assign a new frequency (Hz) to the specified Breakpoint.
 */

void breakpoint_setPhase( Breakpoint * bp, double phi );
/*  Assign a new phase (radians) to the specified Breakpoint.
 */

/* ---------------------------------------------------------------- */
/*      non-object-based procedures
/*
/*  Operations in Loris that need not be accessed though object
    interfaces are represented as simple functions.
 */

void channelize( PartialList * partials, 
                 LinearEnvelope * refFreqEnvelope, int refLabel );
/*  Label Partials in a PartialList with the integer nearest to
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

void collate( PartialList * partials );
/*  Collate unlabeled (zero-labeled) Partials into the smallest-possible 
    number of Partials that does not combine any overlapping Partials.
    Collated Partials appear at the end of the sequence, after all 
    labeled Partials.
 */

LinearEnvelope * 
createFreqReference( PartialList * partials, 
                     double minFreq, double maxFreq, long numSamps );
/*  Return a newly-constructed LinearEnvelope using the legacy 
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
 
LinearEnvelope * 
createF0Estimate( PartialList * partials, double minFreq, double maxFreq, 
                  double interval );
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

void dilate( PartialList * partials, 
             const double * initial, const double * target, int npts );
/*  Dilate Partials in a PartialList according to the given 
    initial and target time points. Partial envelopes are 
    stretched and compressed so that temporal features at
    the initial time points are aligned with the final time
    points. Time points are sorted, so Partial envelopes are 
    are only stretched and compressed, but breakpoints are not
    reordered. Duplicate time points are allowed. There must be
    the same number of initial and target time points.
 */

void distill( PartialList * partials );
/*  Distill labeled (channelized) Partials in a PartialList into a 
    PartialList containing at most one Partial per label. Unlabeled 
    (zero-labeled) Partials are left unmodified at the end of the 
    distilled Partials.
 */

void exportAiff( const char * path, const double * buffer, 
                 unsigned int bufferSize, double samplerate, int bitsPerSamp );
/*  Export mono audio samples stored in an array of size bufferSize to 
    an AIFF file having the specified sample rate at the given file path 
    (or name). The floating point samples in the buffer are clamped to the 
    range (-1.,1.) and converted to integers having bitsPerSamp bits.
 */
                 
void exportSdif( const char * path, PartialList * partials );
/*  Export Partials in a PartialList to a SDIF file at the specified
    file path (or name). SDIF data is described by RBEM and RBEL 
    matrices. 
    For more information about SDIF, see the SDIF web site at:
        www.ircam.fr/equipes/analyse-synthese/sdif/  
 */
                
void exportSpc( const char * path, PartialList * partials, double midiPitch, 
                int enhanced, double endApproachTime );
/*  Export Partials in a PartialList to a Spc file at the specified file
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

/*  Apply a reference Partial to fix the frequencies of Breakpoints
    whose amplitude is below threshold_dB. 0 harmonifies full-amplitude
    Partials, to apply only to quiet Partials, specify a lower 
    threshold like -90). The reference Partial is the first Partial
    in the PartialList labeled refLabel (usually 1). The LinearEnvelope 
    is a time-varying weighting on the harmonifing process. When 1, 
    harmonic frequencies are used, when 0, breakpoint frequencies are 
    unmodified. 
 */
void harmonify( PartialList * partials, long refLabel,
                const LinearEnvelope * env, double threshold_dB );
 
unsigned int importAiff( const char * path, double * buffer, unsigned int bufferSize, 
                         double * samplerate );
/*  Import audio samples stored in an AIFF file at the given file
    path (or name). The samples are converted to floating point 
    values on the range (-1.,1.) and stored in an array of doubles. 
    The value returned is the number of samples in buffer, and it is at
    most bufferSize. If samplerate is not a NULL pointer, 
    then, on return, it points to the value of the sample rate (in
    Hz) of the AIFF samples. The AIFF file must contain only a single
    channel of audio data. The prior contents of buffer, if any, are 
    overwritten.
 */

void importSdif( const char * path, PartialList * partials );
/*  Import Partials from an SDIF file at the given file path (or 
    name), and append them to a PartialList.
 */    

void importSpc( const char * path, PartialList * partials );
/*  Import Partials from an Spc file at the given file path (or 
    name), and return them in a PartialList.
 */    

void morph( const PartialList * src0, const PartialList * src1, 
            const LinearEnvelope * ffreq, 
            const LinearEnvelope * famp, 
            const LinearEnvelope * fbw, 
            PartialList * dst );
/*  Morph labeled Partials in two PartialLists according to the
    given frequency, amplitude, and bandwidth (noisiness) morphing
    envelopes, and append the morphed Partials to the destination 
    PartialList. Loris morphs Partials by interpolating frequency,
    amplitude, and bandwidth envelopes of corresponding Partials in 
    the source PartialLists. For more information about the Loris
    morphing algorithm, see the Loris website: 
    www.cerlsoundgroup.org/Loris/
 */

void morphWithReference( const PartialList * src0, 
                         const PartialList * src1,
                         long src0RefLabel, 
                         long src1RefLabel,
                         const LinearEnvelope * ffreq, 
                         const LinearEnvelope * famp, 
                         const LinearEnvelope * fbw, 
                         PartialList * dst );
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
 
void morpher_setAmplitudeShape( double shape );
/* Set the shaping parameter for the amplitude morphing
	function. This shaping parameter controls the slope of
	the amplitude morphing function, for values greater than
	1, this function gets nearly linear (like the old
	amplitude morphing function), for values much less than
	1 (e.g. 1E-5) the slope is gently curved and sounds
	pretty "linear", for very small values (e.g. 1E-12) the
	curve is very steep and sounds un-natural because of the
	huge jump from zero amplitude to very small amplitude.
	
	Use LORIS_DEFAULT_AMPMORPHSHAPE to obtain the default
	amplitude morphing shape for Loris, (equal to 1E-5,
	which works well for many musical instrument morphs,
	unless Loris was compiled with the symbol
	LINEAR_AMP_MORPHS defined, in which case
	LORIS_DEFAULT_AMPMORPHSHAPE is equal to
	LORIS_LINEAR_AMPMORPHSHAPE).
	
	Use LORIS_LINEAR_AMPMORPHSHAPE to approximate the linear
	amplitude morphs performed by older versions of Loris.
	
	The amplitude shape must be positive.
 */
 
extern const double LORIS_DEFAULT_AMPMORPHSHAPE;    
extern const double LORIS_LINEAR_AMPMORPHSHAPE;

void resample( PartialList * partials, double interval );
/*  Resample all Partials in a PartialList using the specified
    sampling interval, so that the Breakpoints in the Partial 
    envelopes will all lie on a common temporal grid.
    The Breakpoint times in resampled Partials will comprise a  
    contiguous sequence of integer multiples of the sampling interval,
    beginning with the multiple nearest to the Partial's start time and
    ending with the multiple nearest to the Partial's end time. Resampling
    is performed in-place. 

 */

void shapeSpectrum( PartialList * partials, PartialList * surface,
                    double stretchFreq, double stretchTime );
/*  Scale the amplitudes of a set of Partials by applying 
    a spectral suface constructed from another set.
    Stretch the spectral surface in time and frequency
    using the specified stretch factors. Set the stretch
    factors to one for no stretching.
 */

void sift( PartialList * partials );
/*  Identify overlapping Partials having the same (nonzero)
    label. If any two partials with same label
    overlap in time, set the label of the weaker
    (having less total energy) partial to zero.

 */ 

unsigned int 
synthesize( const PartialList * partials,
            double * buffer, unsigned int bufferSize,
            double srate );
/*  Synthesize Partials in a PartialList at the given sample
    rate, and store the (floating point) samples in a buffer of
    size bufferSize. The buffer is neither resized nor 
    cleared before synthesis, so newly synthesized samples are
    added to any previously computed samples in the buffer, and
    samples beyond the end of the buffer are lost. Return the
    number of samples synthesized, that is, the index of the
    latest sample in the buffer that was modified.
 */

/* ---------------------------------------------------------------- */
/*      utility functions
/*
/*  Operations for transforming and manipulating collections
    of Partials.
 */

double avgAmplitude( const Partial * p );
/*  Return the average amplitude over all Breakpoints in this Partial.
    Return zero if the Partial has no Breakpoints.
 */

double avgFrequency( const Partial * p );
/*  Return the average frequency over all Breakpoints in this Partial.
    Return zero if the Partial has no Breakpoints.
 */

void copyIf( const PartialList * src, PartialList * dst, 
             int ( * predicate )( const Partial * p, void * data ),
             void * data );
/*  Append copies of Partials in the source PartialList satisfying the
    specified predicate to the destination PartialList. The source list
    is unmodified. The data parameter can be used to 
    supply extra user-defined data to the function. Pass 0 if no 
    additional data is needed.
 */
             
void copyLabeled( const PartialList * src, long label, PartialList * dst );
/*  Append copies of Partials in the source PartialList having the
    specified label to the destination PartialList. The source list
    is unmodified.
 */

void crop( PartialList * partials, double t1, double t2 );
/*  Trim Partials by removing Breakpoints outside a specified time span.
    Insert a Breakpoint at the boundary when cropping occurs. Remove
	any Partials that are left empty after cropping (Partials having no
	Breakpoints between t1 and t2).
 */

void extractIf( PartialList * src, PartialList * dst, 
                int ( * predicate )( const Partial * p, void * data ),
                void * data );
/*  Remove Partials in the source PartialList satisfying the
    specified predicate from the source list and append them to
    the destination PartialList. The data parameter can be used to 
    supply extra user-defined data to the function. Pass 0 if no 
    additional data is needed.
 */

void extractLabeled( PartialList * src, long label, PartialList * dst );
/*  Remove Partials in the source PartialList having the specified
    label from the source list and append them to the destination 
    PartialList. 
 */

void fixPhaseAfter( PartialList * partials, double time );
/*  Recompute phases of all Breakpoints later than the specified 
    time so that the synthesized phases of those later Breakpoints 
    matches the stored phase, as long as the synthesized phase at 
    the specified time matches the stored (not recomputed) phase.
    
    Phase fixing is only applied to non-null (nonzero-amplitude) 
    Breakpoints, because null Breakpoints are interpreted as phase 
    reset points in Loris. If a null is encountered, its phase is 
    corrected from its non-Null successor, if it has one, otherwise 
    it is unmodified.
 */

void fixPhaseAt( PartialList * partials, double time );
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
    its phase is corrected from its non-Null successor, if 
    it has one, otherwise it is unmodified.
 */

void fixPhaseBefore( PartialList * partials, double time );
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

void fixPhaseBetween( PartialList * partials, double tbeg, double tend );
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

void fixPhaseForward( PartialList * partials, double tbeg, double tend );
/*  Recompute phases of all Breakpoints later than the specified 
    time so that the synthesized phases of those later Breakpoints 
    matches the stored phase, as long as the synthesized phase at 
    the specified time matches the stored (not recomputed) phase. 
    Breakpoints later than tend are unmodified.
    
    Phase fixing is only applied to non-null (nonzero-amplitude) 
    Breakpoints, because null Breakpoints are interpreted as phase 
    reset points in Loris. If a null is encountered, its phase is 
    corrected from its non-Null successor, if it has one, otherwise 
    it is unmodified.
 */

int forEachBreakpoint( Partial * p,
                       int ( * func )( Breakpoint * p, double time, void * data ),
                       void * data );
/*  Apply a function to each Breakpoint in a Partial. The function
    is called once for each Breakpoint in the source Partial. The
    function may modify the Breakpoint (but should not otherwise attempt 
    to modify the Partial). The data parameter can be used to supply extra
    user-defined data to the function. Pass 0 if no additional data is needed.
    The function should return 0 if successful. If the function returns
    a non-zero value, then forEachBreakpoint immediately returns that value
    without applying the function to any other Breakpoints in the Partial.
    forEachBreakpoint returns zero if all calls to func return zero.
 */
                         
int forEachPartial( PartialList * src,
                    int ( * func )( Partial * p, void * data ),
                    void * data );
/*  Apply a function to each Partial in a PartialList. The function
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

double peakAmplitude( const Partial * p );
/*  Return the maximum amplitude achieved by a Partial.
 */

void removeIf( PartialList * src, 
               int ( * predicate )( const Partial * p, void * data ),
               void * data );
/*  Remove from a PartialList all Partials satisfying the
    specified predicate. The data parameter can be used to 
    supply extra user-defined data to the function. Pass 0 if no 
    additional data is needed.
 */

void removeLabeled( PartialList * src, long label );
/*  Remove from a PartialList all Partials having the specified label.
 */

void scaleAmplitude( PartialList * partials, LinearEnvelope * ampEnv );
/*  Scale the amplitude of the Partials in a PartialList according 
    to an envelope representing a time-varying amplitude scale value.
 */

void scaleAmp( PartialList * partials, LinearEnvelope * ampEnv );
/*  Bad old name for scaleAmplitude.
 */
                 
void scaleBandwidth( PartialList * partials, LinearEnvelope * bwEnv );
/*  Scale the bandwidth of the Partials in a PartialList according 
    to an envelope representing a time-varying bandwidth scale value.
 */
                 
void scaleFrequency( PartialList * partials, LinearEnvelope * freqEnv );
/*  Scale the frequency of the Partials in a PartialList according 
    to an envelope representing a time-varying frequency scale value.
 */
                 
void scaleNoiseRatio( PartialList * partials, LinearEnvelope * noiseEnv );
/*  Scale the relative noise content of the Partials in a PartialList 
    according to an envelope representing a (time-varying) noise energy 
    scale value.
 */

void setBandwidth( PartialList * partials, LinearEnvelope * bwEnv );
/*  Set the bandwidth of the Partials in a PartialList according 
    to an envelope representing a time-varying bandwidth value.
 */
                 
void shiftPitch( PartialList * partials, LinearEnvelope * pitchEnv );
/*  Shift the pitch of all Partials in a PartialList according to 
    the given pitch envelope. The pitch envelope is assumed to have 
    units of cents (1/100 of a halfstep).
 */

void shiftTime( PartialList * partials, double offset );
/*  Shift the time of all the Breakpoints in a Partial by a 
    constant amount.
 */

void sortByLabel( PartialList * partials );
/*  Sort the Partials in a PartialList in order of increasing label.
    The sort is stable; Partials having the same label are not 
    reordered.
 */

void timeSpan( PartialList * partials, double * tmin, double * tmax );
/* Return the minimum start time and maximum end time
   in seconds of all Partials in this PartialList. The v
   times are returned in the (non-null) pointers tmin
   and tmax.
 */

double weightedAvgFrequency( const Partial * p );
/*  Return the average frequency over all Breakpoints in this Partial, 
    weighted by the Breakpoint amplitudes. Return zero if the Partial 
    has no Breakpoints.
 */
 
/* ---------------------------------------------------------------- */
/*      Notification and exception handlers                            
/*
/*  An exception handler and a notifier may be specified. Both 
    are functions taking a const char * argument and returning
    void.
 */

void setExceptionHandler( void(*f)(const char *) );
/*  Specify a function to call when reporting exceptions. The 
    function takes a const char * argument, and returns void.
 */

void setNotifier( void(*f)(const char *) );
/*  Specify a notification function. The function takes a 
    const char * argument, and returns void.
 */

#if defined(__cplusplus)
}    /* extern "C"     */
#endif

#endif    /* ndef INCLUDE_LORIS_H */
