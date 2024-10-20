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
 * Morpher.C
 *
 * Implementation of class Morpher.
 *
 * Kelly Fitz, 15 Oct 1999
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */

#if HAVE_CONFIG_H
    #include "config.h"
#endif

#include "Morpher.h"
#include "Breakpoint.h"
#include "Envelope.h"
#include "LorisExceptions.h"
#include "Notifier.h"
#include "Partial.h"
#include "PartialList.h"
#include "PartialUtils.h"

#include "phasefix.h" 

#include <algorithm>
#include <memory>
#include <cmath>
#include <vector>

#if defined(HAVE_M_PI) && (HAVE_M_PI)
    const double Pi = M_PI;
#else
    const double Pi = 3.14159265358979324;
#endif

//    begin namespace
namespace Loris {


const double Morpher::DefaultFixThreshold = -90; // dB, very low by default

// shaping parameter, see interpolateAmplitude:
const double Morpher::DefaultAmpShape = 1E-5;    

const double Morpher::DefaultBreakpointGap = 1E-4; // minimum time (sec) between Breakpoints in 
                                                   // morphed Partials

// helper declarations
static inline bool partial_is_nonnull( const Partial & p );



// -- construction --

// ---------------------------------------------------------------------------
//    Morpher constructor (single morph function)
// ---------------------------------------------------------------------------
//    Construct a new Morpher using the same morphing envelope for 
//    frequency, amplitude, and bandwidth (noisiness).
//
Morpher::Morpher( const Envelope & f ) :
    _freqFunction( f.clone() ),
    _ampFunction( f.clone() ),
    _bwFunction( f.clone() ),
    _freqFixThresholdDb( DefaultFixThreshold ),
    _logMorphShape( DefaultAmpShape ),
    _minBreakpointGapSec( DefaultBreakpointGap ),
    _doLogAmpMorphing( true ),
    _doLogFreqMorphing( false )
{
}

// ---------------------------------------------------------------------------
//    Morpher constructor (distinct morph functions)
// ---------------------------------------------------------------------------
//    Construct a new Morpher using the specified morphing envelopes for
//    frequency, amplitude, and bandwidth (noisiness).
//
Morpher::Morpher( const Envelope & ff, const Envelope & af, const Envelope & bwf ) :
    _freqFunction( ff.clone() ),
    _ampFunction( af.clone() ),
    _bwFunction( bwf.clone() ),
    _freqFixThresholdDb( DefaultFixThreshold ),
    _logMorphShape( DefaultAmpShape ),
    _minBreakpointGapSec( DefaultBreakpointGap ),
    _doLogAmpMorphing( true ),
    _doLogFreqMorphing( false )
{
}

// ---------------------------------------------------------------------------
//    Morpher copy constructor 
// ---------------------------------------------------------------------------
//!    Construct a new Morpher that is a duplicate of rhs.
//!
//! \param  rhs is the Morpher to duplicate
Morpher::Morpher( const Morpher & rhs ) :
    _freqFunction( rhs._freqFunction->clone() ),
    _ampFunction( rhs._ampFunction->clone() ),
    _bwFunction( rhs._bwFunction->clone() ),
    _srcRefPartial( rhs._srcRefPartial ),
    _tgtRefPartial( rhs._tgtRefPartial ),
    _freqFixThresholdDb( rhs._freqFixThresholdDb ),
    _logMorphShape( rhs._logMorphShape ),
    _minBreakpointGapSec( rhs._minBreakpointGapSec ),
    _doLogAmpMorphing( rhs._doLogAmpMorphing ),
    _doLogFreqMorphing( rhs._doLogFreqMorphing )
{
}

// ---------------------------------------------------------------------------
//    Morpher destructor
// ---------------------------------------------------------------------------
//    Destroy this Morpher.
//
Morpher::~Morpher( void )
{
}

// ---------------------------------------------------------------------------
//    Morpher assignment operator
// ---------------------------------------------------------------------------
//! Make this Morpher a duplicate of rhs.
//!
//! \param  rhs is the Morpher to duplicate
Morpher & 
Morpher::operator= ( const Morpher & rhs )
{
    if ( &rhs != this )
    {
        _freqFunction.reset( rhs._freqFunction->clone() );
        _ampFunction.reset( rhs._ampFunction->clone() );
        _bwFunction.reset( rhs._bwFunction->clone() );
        
        _srcRefPartial = rhs._srcRefPartial;
        _tgtRefPartial = rhs._tgtRefPartial;
        
        _freqFixThresholdDb = rhs._freqFixThresholdDb;
        _logMorphShape = rhs._logMorphShape;
        _minBreakpointGapSec = rhs._minBreakpointGapSec;
        
        _doLogAmpMorphing = rhs._doLogAmpMorphing;
        _doLogFreqMorphing = rhs._doLogFreqMorphing;
        
    }
    return *this;
}



// -- Partial morphing --

// ---------------------------------------------------------------------------
//    morphPartials
// ---------------------------------------------------------------------------
//! Morph a pair of Partials to yield a new morphed Partial. 
//! Dummy Partials (having no Breakpoints) don't contribute to the
//! morph, except to cause their opposite to fade out. 
//! Either (or neither) the source or target Partial may be a dummy 
//! Partial (no Breakpoints), but not both. The morphed
//! Partial has Breakpoints at times corresponding to every Breakpoint 
//! in both source Partials, omitting Breakpoints that would be
//!   closer than the minBreakpointGap to their predecessor. 
//! The new morphed Partial is assigned the specified label and returned.
//!
//! \param  src is the Partial corresponding to a morph function
//!         value of 0, evaluated at the specified time.
//! \param  tgt is the Partial corresponding to a morph function
//!         value of 1, evaluated at the specified time.
//! \param  assignLabel is the label assigned to the morphed Partial
//! \return the morphed Partial
//
Partial
Morpher::morphPartials( Partial src, Partial tgt, int assignLabel )
{  
    if ( (src.numBreakpoints() == 0) && (tgt.numBreakpoints() == 0) )
    {
        Throw( InvalidArgument, "Cannot morph two empty Partials," );
    }    
    
    Partial::const_iterator src_iter = src.begin();
    Partial::const_iterator tgt_iter = tgt.begin();
    
    // find the earliest time that a Breakpoint
    // could be added to the morph:
    double dontAddBefore = 0;
    if ( 0 < src.numBreakpoints() )
    {
        dontAddBefore = std::min( dontAddBefore, src_iter.time() );
    }
    if ( 0 < tgt.numBreakpoints() )
    {
        dontAddBefore = std::min( dontAddBefore, tgt_iter.time() );
    }

    
    //  make a new Partial:
    Partial newp;
    newp.setLabel( assignLabel );
    

    //  Merge Breakpoints from the two Partials,
    //  loop until there are no more Breakpoints to
    //  consider in either Partial.
    while ( src_iter != src.end() || tgt_iter != tgt.end() )
    {
        if ( ( tgt_iter == tgt.end() ) ||
             ( src_iter != src.end() && src_iter.time() < tgt_iter.time() ) )
        {
            //  Ran out of tgt Breakpoints, or 
            //  src Breakpoint is earlier, add it.
            //
            //  Don't insert Breakpoints arbitrarily close together, 
            //  only insert a new Breakpoint if it is later than
            //  the end of the new Partial by more than the gap time.
            if ( dontAddBefore <= src_iter.time() )
            {
                appendMorphedSrc( src_iter.breakpoint(), tgt, src_iter.time(), newp );
            }

            ++src_iter;
        }
        else 
        {
            //  Ran out of src Breakpoints, or
            //  tgt Breakpoint is earlier add it.
            //
            //  Don't insert Breakpoints arbitrarily close together, 
            //  only insert a new Breakpoint if it is later than
            //  the end of the new Partial by more than the gap time.
            if ( dontAddBefore <= tgt_iter.time() )
            {
                appendMorphedTgt( tgt_iter.breakpoint(), src, tgt_iter.time(), newp );
            }

            ++tgt_iter;
        }  
        
        if ( 0 != newp.numBreakpoints() )
        {
            // update the earliest time the next Breakpoint
            // could be added to the morph:
            dontAddBefore = newp.endTime() + _minBreakpointGapSec;
        }          
    }

	//	Recompute the phases to match the sources when the frequency 
	//	morphing function is 0 or 1.
    fixMorphedPhases( newp );
    
    return newp;
}


// ---------------------------------------------------------------------------
//    helper - GetMorphState
// ---------------------------------------------------------------------------

typedef enum { SRC = 0, TGT, INTERP } MorphState;

static inline MorphState GetMorphState( double fweight )
{
    if ( fweight <= 0 ) 
    {
        return SRC;
    }
    else if ( fweight >= 1 ) 
    {
        return TGT;
    }
    else 
    {
        return INTERP;
    }
}

// ---------------------------------------------------------------------------
//    fixMorphedPhases (helper)
// ---------------------------------------------------------------------------
//  Recompute phases for a morphed Partial, so that the synthesized phases 
//  match the source phases as closesly as possible at times when the 
//  frequency morphing function is equal to 0 or 1. 

void 
Morpher::fixMorphedPhases( Partial & newp ) const
{
    if ( 0 != newp.numBreakpoints() )
    {
        //  set the initial morph state according to the value of the
        //  frequency function at the time of the first Breakpoint in 
        //  the morphed partial
        Partial::iterator bppos = newp.begin();
        Partial::iterator lastPosCorrect = bppos;
        MorphState curstate = GetMorphState( _freqFunction->valueAt( bppos.time() ) );
        
        //  consider each Breakpoint, look for a change in the
        //  morph state at the time of each Breakpoint
        while( ++bppos != newp.end() )
        {
            MorphState nxtstate = GetMorphState( _freqFunction->valueAt( bppos.time() ) );   
            if ( nxtstate != curstate )
            {
                //  switch!
                if ( INTERP != curstate )
                {
                    // switch to INTERP
                    fixPhaseForward( lastPosCorrect, bppos );
                }
                else
                {
                    //  switch to SRC or TGT                
                    if ( newp.begin() == lastPosCorrect  )
                    {   
                        //  first transition
                        fixPhaseBackward( lastPosCorrect, bppos );
                    }
                    else
                    {
                        //   not first transition
                        fixPhaseBetween( lastPosCorrect, bppos );
                    }                
                }
                lastPosCorrect = bppos;            
                    
                curstate = nxtstate;

            }            
        }
        
        // fix the remaining phases
        fixPhaseForward( lastPosCorrect, --bppos );
    }
}


// ---------------------------------------------------------------------------
//    crossfade
// ---------------------------------------------------------------------------
//    Crossfade Partials with no correspondences.
//
//    Unlabeled Partials (having label 0) are considered to 
//    have no correspondences, so they are just faded out, and not 
//    actually morphed. This is the same as morphing each with an 
//    empty dummy Partial (having no Breakpoints). 
//
//    The Partials in the first range are treated as components of the 
//    source sound, corresponding to a morph function value of 0, and  
//    those in the second are treated as components of the target sound, 
//    corresponding to a morph function value of 1.
//
//    The crossfaded Partials are stored in the Morpher's PartialList.
//
void 
Morpher::crossfade( PartialList::const_iterator beginSrc, 
                    PartialList::const_iterator endSrc,
                    PartialList::const_iterator beginTgt, 
                    PartialList::const_iterator endTgt,
                    Partial::label_type label /* default 0 */ )
{
    Partial nullPartial;
    debugger << "crossfading unlabeled (labeled 0) Partials" << endl;
    
    long debugCounter;

    //    crossfade Partials corresponding to a morph weight of 0:
    PartialList::const_iterator it;
    debugCounter = 0;
    for ( it = beginSrc; it != endSrc; ++it )
    {
        if ( it->label() == label && 0 != it->numBreakpoints() )
        {            
            Partial newp;
            newp.setLabel( label );
            double dontAddBefore = it->startTime();

            for ( Partial::const_iterator bpPos = it->begin(); 
                  bpPos != it->end(); 
                  ++bpPos )
            {        
                //  Don't insert Breakpoints arbitrarily close together, 
                //  only insert a new Breakpoint if it is later than
                //  the end of the new Partial by more than the gap time.
                if ( dontAddBefore <= bpPos.time() )
                {
                    newp.insert( bpPos.time(), 
                                 fadeSrcBreakpoint( bpPos.breakpoint(), bpPos.time() ) );
                    dontAddBefore = bpPos.time() + _minBreakpointGapSec;
                }
            }
            
            if ( newp.numBreakpoints() > 0 )
            {
                ++debugCounter;
                _partials.push_back( newp );
            }
        }
    }
    debugger << "kept " << debugCounter << " from sound 1" << endl;

    //    crossfade Partials corresponding to a morph weight of 1:
    debugCounter = 0;
    for ( it = beginTgt; it != endTgt; ++it )
    {
        if ( it->label() == label && 0 != it->numBreakpoints() )
        {
            Partial newp;
            newp.setLabel( label );
            double dontAddBefore = it->startTime();
                            
            for ( Partial::const_iterator bpPos = it->begin(); 
                  bpPos != it->end(); 
                  ++bpPos )
            {
                //  Don't insert Breakpoints arbitrarily close together, 
                //  only insert a new Breakpoint if it is later than
                //  the end of the new Partial by more than the gap time.
                if ( dontAddBefore <= bpPos.time() )
                {
                    newp.insert( bpPos.time(),
                                 fadeTgtBreakpoint( bpPos.breakpoint(), bpPos.time() ) );           
                    dontAddBefore = bpPos.time() + _minBreakpointGapSec;
                }
            }
            
            if ( newp.numBreakpoints() > 0 )
            {
                ++debugCounter;
                _partials.push_back( newp );
            }
        }
    }
    debugger << "kept " << debugCounter << " from sound 2" << endl;
}

// ---------------------------------------------------------------------------
//    morph
// ---------------------------------------------------------------------------
//    Morph two sounds (collections of Partials labeled to indicate
//    correspondences) into a single labeled collection of Partials.
//    Unlabeled Partials (having label 0) are crossfaded. The morphed
//    and crossfaded Partials are stored in the Morpher's PartialList.
//
//    The Partials in the first range are treated as components of the 
//    source sound, corresponding to a morph function value of 0, and  
//    those in the second are treated as components of the target sound, 
//    corresponding to a morph function value of 1.
//
//    Throws InvalidArgument if either the source or target
//    sequence is not distilled (contains more than one Partial having
//    the same non-zero label).
//
//    Ugh! This ought to be a template function!
// Ugh! But then crossfade needs to be a template function.
// Maybe need to do something different with crossfade first.
//
void 
Morpher::morph( PartialList::const_iterator beginSrc, 
                PartialList::const_iterator endSrc,
                PartialList::const_iterator beginTgt, 
                PartialList::const_iterator endTgt )
{
    //    build a PartialCorrespondence, a map of labels
    //    to pairs of pointers to Partials, by making every
    //    Partial in the source the first element of the
    //    pair at the corresponding label, and every Partial
    //    in the target the second element of the pair at
    //    the corresponding label. Pointers not assigned to
    //    point to a Partial in the source or target are 
    //    initialized to 0 in the correspondence map.
    PartialCorrespondence correspondence;
    
    //    add source Partials to the correspondence map:
    for ( PartialList::const_iterator it = beginSrc; it != endSrc; ++it ) 
    {
        //    don't add the crossfade label to the set:
        if ( it->label() != 0 )
        {
            MorphingPair & match = correspondence[ it->label() ];
            if ( match.src.numBreakpoints() != 0 )
            {
                Throw( InvalidArgument, "Source Partials must be distilled before morphing." );
            }
            match.src = *it;
        }
    }
    
    //    add target Partials to the correspondence map:
    for ( PartialList::const_iterator it = beginTgt; it != endTgt; ++it ) 
    {
        //    don't add the crossfade label to the set:
        if ( it->label() != 0 )
        {
            MorphingPair & match = correspondence[ it->label() ];
            if ( match.tgt.numBreakpoints() != 0 )
            {
                Throw( InvalidArgument, "Target Partials must be distilled before morphing." );
            }
            match.tgt = *it;
        }
    }
    
    //    morph corresponding labeled Partials:
    morph_aux( correspondence );
    
    //    crossfade the remaining unlabeled Partials:
    crossfade( beginSrc, endSrc, beginTgt, endTgt );
}

// ---------------------------------------------------------------------------
//    morphBreakpoints
// ---------------------------------------------------------------------------
//!    Compute morphed parameter values at the specified time, using
//!    the source and target Breakpoints (assumed to correspond exactly
//!    to the specified time).
//!
//!    \param  srcBkpt is the Breakpoint corresponding to a morph function
//!            value of 0.
//!    \param  tgtBkpt is the Breakpoint corresponding to a morph function
//!            value of 1.
//!    \param  time is the time corresponding to srcBkpt (used
//!            to evaluate the morphing functions and tgtPartial).
//!    \return the morphed Breakpoint
//
Breakpoint
Morpher::morphBreakpoints( Breakpoint srcBkpt, Breakpoint tgtBkpt, 
                           double time  ) const
{
   double fweight = _freqFunction->valueAt( time );
   double aweight = _ampFunction->valueAt( time );
   double bweight = _bwFunction->valueAt( time );

   // compute interpolated Breakpoint parameters:
   return interpolateParameters( srcBkpt, tgtBkpt, fweight, 
                                 aweight, bweight );
}

// ---------------------------------------------------------------------------
//    morphSrcBreakpoint
// ---------------------------------------------------------------------------
//!    Compute morphed parameter values at the specified time, using
//!    the source Breakpoint (assumed to correspond exactly to the
//!    specified time) and the target Partial (whose parameters are
//!    examined at the specified time).
//!
//!    \pre    the target Partial may not be a dummy Partial (no Breakpoints).
//!
//!    \param  srcBkpt is the Breakpoint corresponding to a morph function
//!            value of 0.
//!    \param  tgtPartial is the Partial corresponding to a morph function
//!            value of 1, evaluated at the specified time.
//!    \param  time is the time corresponding to srcBkpt (used
//!            to evaluate the morphing functions and tgtPartial).
//!    \return the morphed Breakpoint
//
Breakpoint
Morpher::morphSrcBreakpoint( const Breakpoint & srcBkpt, const Partial & tgtPartial, 
                             double time  ) const
{
    if ( 0 == tgtPartial.numBreakpoints() )
    {
        Throw( InvalidArgument, "morphSrcBreakpoint cannot morph with empty Partial" );
    }
    
    Breakpoint tgtBkpt = tgtPartial.parametersAt( time );
    
    return morphBreakpoints( srcBkpt, tgtBkpt, time );
}

// ---------------------------------------------------------------------------
//    morphTgtBreakpoint
// ---------------------------------------------------------------------------
//!    Compute morphed parameter values at the specified time, using
//!    the target Breakpoint (assumed to correspond exactly to the
//!    specified time) and the source Partial (whose parameters are
//!    examined at the specified time).
//!
//!    \pre    the source Partial may not be a dummy Partial (no Breakpoints).
//!
//!    \param  tgtBkpt is the Breakpoint corresponding to a morph function
//!            value of 1.
//!    \param  srcPartial is the Partial corresponding to a morph function
//!            value of 0, evaluated at the specified time.
//!    \param  time is the time corresponding to srcBkpt (used
//!            to evaluate the morphing functions and srcPartial).
//!    \return the morphed Breakpoint
//
Breakpoint
Morpher::morphTgtBreakpoint( const Breakpoint & tgtBkpt, const Partial & srcPartial, 
                             double time  ) const
{
    if ( 0 == srcPartial.numBreakpoints() )
    {
        Throw( InvalidArgument, "morphTgtBreakpoint cannot morph with empty Partial" );
    }
    
    Breakpoint srcBkpt = srcPartial.parametersAt( time );
   
    return morphBreakpoints( srcBkpt, tgtBkpt, time );
}

// ---------------------------------------------------------------------------
//    fadeSrcBreakpoint
// ---------------------------------------------------------------------------
//! Compute morphed parameter values at the specified time, using
//! the source Breakpoint, assumed to correspond exactly to the
//! specified time, and assuming that there is no corresponding 
//! target Partial, so the source Breakpoint should be simply faded.
//!
//! \param  bp is the Breakpoint corresponding to a morph function
//!         value of 0.
//! \param  time is the time corresponding to bp (used
//!         to evaluate the morphing functions).
//! \return the faded Breakpoint
//
Breakpoint
Morpher::fadeSrcBreakpoint( Breakpoint bp, double time ) const
{
    double alpha = _ampFunction->valueAt( time );
    bp.setAmplitude( interpolateAmplitude( bp.amplitude(), 0, 
                                           alpha ) );
    return bp;
}

// ---------------------------------------------------------------------------
//    fadeTgtBreakpoint
// ---------------------------------------------------------------------------
//! Compute morphed parameter values at the specified time, using
//! the target Breakpoint, assumed to correspond exactly to the
//! specified time, and assuming that there is not corresponding 
//! source Partial, so the target Breakpoint should be simply faded.
//!
//! \param  bp is the Breakpoint corresponding to a morph function
//!         value of 1.
//! \param  time is the time corresponding to bp (used
//!         to evaluate the morphing functions).
//! \return the faded Breakpoint
//
Breakpoint
Morpher::fadeTgtBreakpoint( Breakpoint bp, double time ) const
{
    double alpha = _ampFunction->valueAt( time );
    bp.setAmplitude( interpolateAmplitude( 0, bp.amplitude(), 
                                           alpha ) );
    return bp;
}

// -- morphing function access/mutation --

// ---------------------------------------------------------------------------
//    setFrequencyFunction
// ---------------------------------------------------------------------------
//    Assign a new frequency morphing envelope to this Morpher.
//
void
Morpher::setFrequencyFunction( const Envelope & f )
{
    _freqFunction.reset( f.clone() );
}

// ---------------------------------------------------------------------------
//    setAmplitudeFunction
// ---------------------------------------------------------------------------
//    Assign a new amplitude morphing envelope to this Morpher.
//
void
Morpher::setAmplitudeFunction( const Envelope & f )
{
    _ampFunction.reset( f.clone() );
}

// ---------------------------------------------------------------------------
//    setBandwidthFunction
// ---------------------------------------------------------------------------
//    Assign a new bandwidth morphing envelope to this Morpher.
//
void
Morpher::setBandwidthFunction( const Envelope & f )
{
    _bwFunction.reset( f.clone() );
}

// ---------------------------------------------------------------------------
//    frequencyFunction
// ---------------------------------------------------------------------------
//    Return a reference to this Morpher's frequency morphing envelope.
//
const Envelope &
Morpher::frequencyFunction( void ) const 
{
    return * _freqFunction;
}

// ---------------------------------------------------------------------------
//    amplitudeFunction
// ---------------------------------------------------------------------------
//    Return a reference to this Morpher's amplitude morphing envelope.
//
const Envelope &
Morpher::amplitudeFunction( void ) const 
{
    return * _ampFunction;
}

// ---------------------------------------------------------------------------
//    bandwidthFunction
// ---------------------------------------------------------------------------
//    Return a reference to this Morpher's bandwidth morphing envelope.
//
const Envelope &
Morpher::bandwidthFunction( void ) const 
{
    return * _bwFunction;
}


// -- reference Partial label access/mutation --

// ---------------------------------------------------------------------------
//    sourceReferencePartial
// ---------------------------------------------------------------------------
//! Return the Partial to be used as a reference
//! Partial for the source sequence in a morph of two Partial
//! sequences. The reference partial is used to compute 
//! frequencies for very low-amplitude Partials whose frequency
//! estimates are not considered reliable. The reference Partial
//! is considered to have good frequency estimates throughout.
//! A default (empty) Partial indicates that no reference Partial
//! should be used for the source sequence.
//
const Partial &
Morpher::sourceReferencePartial( void ) const
{
    return _srcRefPartial;
}

// ---------------------------------------------------------------------------
//    sourceReferencePartial
// ---------------------------------------------------------------------------
//! Return the Partial to be used as a reference
//! Partial for the source sequence in a morph of two Partial
//! sequences. The reference partial is used to compute 
//! frequencies for very low-amplitude Partials whose frequency
//! estimates are not considered reliable. The reference Partial
//! is considered to have good frequency estimates throughout.
//! A default (empty) Partial indicates that no reference Partial
//! should be used for the source sequence.
//
Partial &
Morpher::sourceReferencePartial( void )
{
    return _srcRefPartial;
}

// ---------------------------------------------------------------------------
//    targetReferenceLabel
// ---------------------------------------------------------------------------
//! Return the Partial to be used as a reference
//! Partial for the target sequence in a morph of two Partial
//! sequences. The reference partial is used to compute 
//! frequencies for very low-amplitude Partials whose frequency
//! estimates are not considered reliable. The reference Partial
//! is considered to have good frequency estimates throughout.
//! A default (empty) Partial indicates that no reference Partial
//! should be used for the target sequence.
//
const Partial &
Morpher::targetReferencePartial( void ) const
{
    return _tgtRefPartial;
}
    
// ---------------------------------------------------------------------------
//    targetReferenceLabel
// ---------------------------------------------------------------------------
//! Return the Partial to be used as a reference
//! Partial for the target sequence in a morph of two Partial
//! sequences. The reference partial is used to compute 
//! frequencies for very low-amplitude Partials whose frequency
//! estimates are not considered reliable. The reference Partial
//! is considered to have good frequency estimates throughout.
//! A default (empty) Partial indicates that no reference Partial
//! should be used for the target sequence.
//
Partial &
Morpher::targetReferencePartial( void ) 
{
    return _tgtRefPartial;
}
    
// ---------------------------------------------------------------------------
//    setSourceReferencePartial
// ---------------------------------------------------------------------------
//! Specify the Partial to be used as a reference
//! Partial for the source sequence in a morph of two Partial
//! sequences. The reference partial is used to compute 
//! frequencies for very low-amplitude Partials whose frequency
//! estimates are not considered reliable. The reference Partial
//! is considered to have good frequency estimates throughout.
//! The specified Partial must be labeled with its harmonic number.
//! A default (empty) Partial indicates that no reference 
//! Partial should be used for the source sequence.
//
void 
Morpher::setSourceReferencePartial( const Partial & p )
{
    if ( p.label() == 0 )
    {
        Throw( InvalidArgument, 
               "the morphing source reference Partial must be "
               "labeled with its harmonic number" );
    }
    _srcRefPartial = p;
}

// ---------------------------------------------------------------------------
//    setSourceReferencePartial
// ---------------------------------------------------------------------------
//! Specify the Partial to be used as a reference
//! Partial for the source sequence in a morph of two Partial
//! sequences. The reference partial is used to compute 
//! frequencies for very low-amplitude Partials whose frequency
//! estimates are not considered reliable. The reference Partial
//! is considered to have good frequency estimates throughout.
//! A default (empty) Partial indicates that no reference 
//! Partial should be used for the source sequence.
//!
//! \param  partials a sequence of Partials to search
//!         for the reference Partial
//! \param  refLabel the label of the Partial in partials
//!         that should be selected as the reference
//
void 
Morpher::setSourceReferencePartial( const PartialList & partials, 
                                    Partial::label_type refLabel )
{
    if ( refLabel != 0 )
    {
        PartialList::const_iterator pos = 
            std::find_if( partials.begin(), partials.end(), 
                          PartialUtils::isLabelEqual( refLabel ) );
        if ( pos == partials.end() )
        {
            Throw( InvalidArgument, "no Partial has the specified reference label" );
        }
        _srcRefPartial = *pos;
    }
    else
    {
        _srcRefPartial = Partial();
    }
}

// ---------------------------------------------------------------------------
//    setTargetReferencePartial
// ---------------------------------------------------------------------------
//! Specify the Partial to be used as a reference
//! Partial for the target sequence in a morph of two Partial
//! sequences. The reference partial is used to compute 
//! frequencies for very low-amplitude Partials whose frequency
//! estimates are not considered reliable. The reference Partial
//! is considered to have good frequency estimates throughout.
//! The specified Partial must be labeled with its harmonic number.
//! A default (empty) Partial indicates that no reference 
//! Partial should be used for the target sequence.
//
void 
Morpher::setTargetReferencePartial( const Partial & p )
{
    if ( p.label() == 0 )
    {
        Throw( InvalidArgument, 
               "the morphing target reference Partial must be "
               "labeled with its harmonic number" );
    }
    _tgtRefPartial = p;
}

// ---------------------------------------------------------------------------
//    setTargetReferencePartial
// ---------------------------------------------------------------------------
//! Specify the Partial to be used as a reference
//! Partial for the target sequence in a morph of two Partial
//! sequences. The reference partial is used to compute 
//! frequencies for very low-amplitude Partials whose frequency
//! estimates are not considered reliable. The reference Partial
//! is considered to have good frequency estimates throughout.
//! A default (empty) Partial indicates that no reference 
//! Partial should be used for the target sequence.
//!
//! \param  partials a sequence of Partials to search
//!         for the reference Partial
//! \param  refLabel the label of the Partial in partials
//!         that should be selected as the reference
//
void 
Morpher::setTargetReferencePartial( const PartialList & partials, 
                                    Partial::label_type refLabel )
{
    if ( refLabel != 0 )
    {
        PartialList::const_iterator pos = 
            std::find_if( partials.begin(), partials.end(), 
                          PartialUtils::isLabelEqual( refLabel ) );
        if ( pos == partials.end() )
        {
            Throw( InvalidArgument, "no Partial has the specified reference label" );
        }
        _tgtRefPartial = *pos;
    }
    else
    {
        _tgtRefPartial = Partial();
    }
}


// ---------------------------------------------------------------------------
//    amplitudeShape
// ---------------------------------------------------------------------------
//    Return the shaping parameter for the amplitude moprhing
//    function (only used in new log-amplitude morphing).
//    This shaping parameter controls the 
//    slope of the amplitude morphing function,
//    for values greater than 1, this function
//    gets nearly linear (like the old amplitude
//    morphing function), for values much less 
//    than 1 (e.g. 1E-5) the slope is gently
//    curved and sounds pretty "linear", for 
//    very small values (e.g. 1E-12) the curve
//    is very steep and sounds un-natural because
//    of the huge jump from zero amplitude to
//    very small amplitude.
double Morpher::amplitudeShape( void ) const
{
    return _logMorphShape;
}

// ---------------------------------------------------------------------------
//    setAmplitudeShape
// ---------------------------------------------------------------------------
//    Set the shaping parameter for the amplitude moprhing
//    function. This shaping parameter controls the 
//    slope of the amplitude morphing function,
//    for values greater than 1, this function
//    gets nearly linear (like the old amplitude
//    morphing function), for values much less 
//    than 1 (e.g. 1E-5) the slope is gently
//    curved and sounds pretty "linear", for 
//    very small values (e.g. 1E-12) the curve
//    is very steep and sounds un-natural because
//    of the huge jump from zero amplitude to
//    very small amplitude.
//
//    x is the new shaping parameter, it must be positive.
void Morpher::setAmplitudeShape( double x )
{
    if ( x <= 0. )
    {
        Throw( InvalidArgument, "the amplitude morph shaping parameter must be positive");
    }
    _logMorphShape = x;
}

// ---------------------------------------------------------------------------
//    minBreakpointGap
// ---------------------------------------------------------------------------
//    Return the minimum time gap (secs) between two Breakpoints
//    in the morphed Partials. Morphing two
//    Partials can generate a third Partial having
//    Breakpoints arbitrarily close together in time,
//    and this makes morphs huge. Raising this 
//    threshold limits the Breakpoint density in
//    the morphed Partials. Default is 1/10 ms.
double Morpher::minBreakpointGap( void ) const
{
    return _minBreakpointGapSec;
}

// ---------------------------------------------------------------------------
//    setMinBreakpointGap
// ---------------------------------------------------------------------------
//    Set the minimum time gap (secs) between two Breakpoints
//    in the morphed Partials. Morphing two
//    Partials can generate a third Partial having
//    Breakpoints arbitrarily close together in time,
//    and this makes morphs huge. Raising this 
//    threshold limits the Breakpoint density in
//    the morphed Partials. Default is 1/10 ms.
//
//    x is the new minimum gap in seconds, it must be positive
//    
void Morpher::setMinBreakpointGap( double x )
{
    if ( x <= 0. )
    {
        Throw( InvalidArgument, "the minimum Breakpoint gap must be positive");
    }
    _minBreakpointGapSec = x;
}

// -- PartialList access --

// ---------------------------------------------------------------------------
//    partials
// ---------------------------------------------------------------------------
//    Return a reference to this Morpher's list of morphed Partials.
//
PartialList & 
Morpher::partials( void )
{ 
    return _partials; 
}

// ---------------------------------------------------------------------------
//    partials
// ---------------------------------------------------------------------------
//    Return a const reference to this Morpher's list of morphed Partials.
//
const PartialList & 
Morpher::partials( void ) const 
{ 
    return _partials; 
}

// -- helpers: morphed parameter computation --

// ---------------------------------------------------------------------------
//    morph_aux
// ---------------------------------------------------------------------------
//    Helper function that performs the morph between corresponding pairs
//    of Partials identified in a PartialCorrespondence. Called by the
//    morph() implementation accepting two sequences of Partials.
//
//    PartialCorrespondence represents a map from non-zero Partial 
//    labels to pairs of Partials (MorphingPair) that should be morphed 
//    into a single Partial that is assigned that label. 
//
void Morpher::morph_aux( PartialCorrespondence & correspondence  )
{
    PartialCorrespondence::const_iterator it;
    for ( it = correspondence.begin(); it != correspondence.end(); ++it )
    {
        Partial::label_type label = it->first;
        MorphingPair match = it->second;
        Partial & src = match.src;
        Partial & tgt = match.tgt;
       
        //  sanity check:
        //  one of those Partials must have some Breakpoints
        Assert( src.numBreakpoints() != 0 || tgt.numBreakpoints() != 0 );

        debugger << "morphing " << ( ( 0 < src.numBreakpoints() )?( 1 ):( 0 ) )
                   << " and " << ( ( 0 < tgt.numBreakpoints() )?( 1 ):( 0 ) )
                   << " partials with label " <<    label << endl;
                   
        //  &^)     HEY LOOKIE HERE!!!!!!!!!!!!!                   
        
        //  ensure that Partials begin and end at zero
        //  amplitude to solve the problem of Nulls 
        //  getting left out of morphed Partials leading to
        //  erroneous non-zero amplitude segments:
        if ( src.numBreakpoints() != 0 )
        {
            if ( src.first().amplitude() != 0.0 && src.startTime() > _minBreakpointGapSec )
            {
                double t = src.startTime() - _minBreakpointGapSec;
                Breakpoint null = src.parametersAt( t );
                src.insert( t, null );
            }
            if ( src.last().amplitude() != 0.0 )
            {
                double t = src.endTime() + _minBreakpointGapSec;
                Breakpoint null = src.parametersAt( t );
                src.insert( t, null );
            }
        }
        
        if ( tgt.numBreakpoints() != 0 )
        {            
            if ( tgt.first().amplitude() != 0.0 && tgt.startTime() > _minBreakpointGapSec )
            {
                double t = tgt.startTime() - _minBreakpointGapSec;
                Breakpoint null = tgt.parametersAt( t );
                tgt.insert( t, null );
            }
            if ( tgt.last().amplitude() != 0.0 )
            {
                double t = tgt.endTime() + _minBreakpointGapSec;
                Breakpoint null = tgt.parametersAt( t );
                tgt.insert( t, null );
            }
        }
        //  &^)     HEY LOOKIE HERE!!!!!!!!!!!!!                   
        //  the question is: after sticking nulls on the ends,
        //  should be strip nulls OFF the ends of the morphed
        //  partial? If so, how many? (ans to second is one, 
        //  cannot have both nulls appear at end of morphed,
        //  because of min gap). If we unconditionally add
        //  nulls to ends (regardless of starting and ending
        //  amps), then we can (I think) be sure that taking
        //  off one null from each end leaves the Partial in 
        //  an unmolested state.... maybe. No, its possible that
        //  the morphing function would skip over both artificial
        //  nulls, so we cannot be sure. Hmmmmm....
        //  For now, just leave the nulls on the ends,
        //  the are relatively harmless.
        //
        //  Actually, a (klugey) solution is to remember the times 
        //  of those artificial nulls, and then see if the
        //  Partial begins or ends at one of those times.
        //  No, cannot guarantee that one Partial doesn't
        //  have a null at the time we put an artificial null
        //  in the other one. Hmmmmm.....

               
        //  perform the morph between the two Partials, 
        //  save the result if it has any Breakpoints
        //  (it may not depending on the morphing functions):                          
        Partial newp = morphPartials( src, tgt, label );
        if ( partial_is_nonnull( newp ) )
        {
            _partials.push_back( newp );
        }
    }
}


// ---------------------------------------------------------------------------
//    adjustFrequency
// ---------------------------------------------------------------------------
//  Adjust frequency of low-amplitude Breakpoints to be harmonics of the
//  reference Partial, if one has been specified.
//
//  Leave the phase alone, because I don't know what we can do with it.
//
static void adjustFrequency( Breakpoint & bp, const Partial & ref, 
                             Partial::label_type harmonicNum,
                             double thresholdDb,
                             double time )
{
    if ( ref.numBreakpoints() != 0 )
    {
        //    compute absolute magnitude thresholds:
        static const double FadeRangeDB = 10;
        const double BeginFade = std::pow( 10., 0.05 * (thresholdDb+FadeRangeDB) );
                
        if ( bp.amplitude() < BeginFade )
        {
            const double Threshold = std::pow( 10., 0.05 * thresholdDb );
            const double OneOverFadeSpan = 1. / ( BeginFade - Threshold );

            double fscale = (double)harmonicNum / ref.label();

            double alpha = std::min( ( BeginFade - bp.amplitude() ) * OneOverFadeSpan, 1. );
            double fRef = ref.frequencyAt( time );
            bp.setFrequency( ( alpha * ( fRef * fscale ) ) + 
                             ( (1 - alpha) * bp.frequency() ) );
        }
    }
}

// ---------------------------------------------------------------------------
//    partial_is_nonnull
// ---------------------------------------------------------------------------
//  Helper function to examine a morphed Partial and determine whether 
//  it has any non-null Breakpoints. If not, there's no point in saving it.
//
static inline bool partial_is_nonnull( const Partial & p )
{
    for ( Partial::const_iterator it = p.begin(); it != p.end(); ++it )
    {
        if ( it.breakpoint().amplitude() != 0.0 )
        {
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
//  Helper function for performing log-domain interpolation
//  (originally was for amplitude only). 
//
//  alpha == 0 returns x, alpha == 1 returns y
//
//  It is essential to add in a small offset, so that 
//  occasional zero amplitudes do not introduce artifacts
//  (if amp is zero, then even if alpha is very small
//  the effect is to multiply by zero, because 0^x = 0, 
//  or note that log(0) is -infinity).
//
//  This shaping parameter affects the shape of the morph
//  curve only when it is of the same order of magnitude as
//  one of the sources (x or y) and the other is much larger.
//
//  When shape is very small, the curve representing the
//  morphed amplitude is very steep, such that there is a 
//  huge difference between zero amplitude and very small
//  amplitude, and this causes audible artifacts. So instead
//  use a larger value that shapes the curve more nicely. 
//  Just have to subtract this value from the morphed 
//  amplitude to avoid raising the noise floor a whole lot.
//
static inline double 
interpolateLog( double x, double y, double alpha, double shape )
{
    using std::pow;

    double s = x + shape;
    double t = y + shape;
    
    double v = ( s * pow( t / s, alpha ) ) - shape;
    
    return v;
}

// ---------------------------------------------------------------------------
//  Helper function for performing linear interpolation
//  (used to be the only kind we supported).
//
//  alpha == 0 returns x, alpha == 1 returns y
//
static inline double 
interpolateLinear( double x, double y, double alpha )
{
    double v = (x * (1-alpha)) + (y * alpha);
    
    return v;
}


// ---------------------------------------------------------------------------
//  Helper function for computing individual morphed amplitude values.
//
inline double 
Morpher::interpolateAmplitude( double srcAmp, double tgtAmp, double alpha ) const
{    
    double morphedAmp = 0;  

    if ( _doLogAmpMorphing )
    {
        //  if both are small, just return 0
        //  HEY, is this really what we want?
        static const double Epsilon = 1E-12;
        if ( ( srcAmp > Epsilon ) || ( tgtAmp > Epsilon ) )
        {
            morphedAmp = interpolateLog( srcAmp, tgtAmp, alpha, _logMorphShape );
        }
    }
    else
    {
        morphedAmp = interpolateLinear( srcAmp, tgtAmp, alpha );
    }

    //  Partial amplitudes should never be negative
    double res = std::max( 0.0, morphedAmp );                

    return res;
}

// ---------------------------------------------------------------------------
//  Helper function for computing individual morphed bandwidth values.
//
inline double 
Morpher::interpolateBandwidth( double srcBw, double tgtBw, double alpha ) const
{    
    double morphedBw = 0;
        
    if ( _doLogAmpMorphing )
    {
        //  if both are small, just return 0
        //  HEY, is this really what we want?
        static const double Epsilon = 1E-12;
        if ( ( srcBw > Epsilon ) || ( tgtBw > Epsilon ) )
        {
            morphedBw = interpolateLog( srcBw, tgtBw, alpha, _logMorphShape );
        }
    }
    else
    {
        morphedBw = interpolateLinear( srcBw, tgtBw, alpha );
    }

    //  Partial bandwidths should never be negative
    double res = std::max( 0.0, morphedBw );                
    

    return res;
}

// ---------------------------------------------------------------------------
//  Helper function for computing individual morphed frequency values.
//
inline double 
Morpher::interpolateFrequency( double srcFreq, double tgtFreq, double alpha ) const
{   
    double morphedFreq = 1;
        
    if ( _doLogFreqMorphing )
    {       
        //  guard against the extremely unlikely possibility that
        //  one of the frequencies is zero
        double shape = 0;
        if ( 0 == srcFreq || 0 == tgtFreq  )
        {
            shape = Morpher::DefaultAmpShape;
        }
        morphedFreq = interpolateLog( srcFreq, tgtFreq, alpha, shape );
    }
    else
    {
        morphedFreq = interpolateLinear( srcFreq, tgtFreq, alpha );
    }
    
    return morphedFreq;
}

// ---------------------------------------------------------------------------
//  Helper function for computing individual morphed phase values.
//
inline double 
Morpher::interpolatePhase( double srcphase, double tgtphase, double alpha ) const
{    
    // Interpolate raw absolute phase values. If the interpolated
    // phase matters at all (near the morphing function boudaries 0
    // and 1) then that will give a good target phase value, and the
    // frequency will be adjusted to match the phase. Otherwise,
    // the phase will just be recomputed to match the interpolated
    // frequency.
    //
    // Wrap the computed phase onto an appropriate range.
    // wrap the phases so that they are as similar as possible,
    // so that phase interpolation is shift-invariant.
    while ( ( srcphase - tgtphase ) > Pi )
    {
      srcphase -= 2 * Pi;
    }
    while ( ( tgtphase - srcphase ) > Pi )
    {
      srcphase += 2 * Pi;
    }

    double morphedPhase = interpolateLinear( srcphase, tgtphase, alpha );

    return std::fmod( morphedPhase, 2 * Pi );
}

// ---------------------------------------------------------------------------
//    Helper function for interpolating Breakpoint parameters
//
inline Breakpoint 
Morpher::interpolateParameters( const Breakpoint & srcBkpt, const Breakpoint & tgtBkpt,
                                double fweight, double aweight, double bweight ) const
{
    Breakpoint morphed;

    // interpolate frequencies:
    morphed.setFrequency(
        interpolateFrequency( srcBkpt.frequency(), tgtBkpt.frequency(),
                              fweight ) );

    // interpolate LOG amplitudes:
    morphed.setAmplitude( 
        interpolateAmplitude( srcBkpt.amplitude(), tgtBkpt.amplitude(), 
                              aweight ) );

    // interpolate bandwidth: 
    morphed.setBandwidth(  
        interpolateBandwidth( srcBkpt.bandwidth(), tgtBkpt.bandwidth(), 
                              bweight ) );

    // interpolate phase:
    morphed.setPhase( 
        interpolatePhase( srcBkpt.phase(), tgtBkpt.phase(), fweight ) );

    return morphed;
}

// ---------------------------------------------------------------------------
//    appendMorphedSrc
// ---------------------------------------------------------------------------
//! Compute morphed parameter values at the specified time, using
//! the source Breakpoint (assumed to correspond exactly to the
//! specified time) and the target Partial (whose parameters are
//! examined at the specified time). Append the morphed Breakpoint
//! to newp only if the source should contribute to the morph at
//! the specified time.
//!
//! If the target Partial is a dummy Partial (no Breakpoints), fade the
//! source instead of morphing.
//!
//! \param  srcBkpt is the Breakpoint corresponding to a morph function
//!         value of 0.
//! \param  tgtPartial is the Partial corresponding to a morph function
//!         value of 1, evaluated at the specified time.
//! \param  time is the time corresponding to srcBkpt (used
//!         to evaluate the morphing functions and tgtPartial).
//! \param  newp is the morphed Partial under construction, the morphed
//!         Breakpoint is added to this Partial.
//
void
Morpher::appendMorphedSrc( Breakpoint srcBkpt, const Partial & tgtPartial, 
                           double time, Partial & newp  )
{
    double fweight = _freqFunction->valueAt( time );
    double aweight = _ampFunction->valueAt( time );
    double bweight = _bwFunction->valueAt( time );
    
    //  Need to insert a null (0 amplitude) Breakpoint
    //  if src and tgt are 0 amplitude but the morphed
    //  Partial is not. In rare cases, it is possible
    //  to miss a needed null if we don't check for it 
    //  explicitly. 
    bool needNull = ( newp.numBreakpoints() != 0 ) &&
                    ( newp.last().amplitude() != 0 ) &&
                    ( srcBkpt.amplitude() == 0) &&
                    ( tgtPartial.numBreakpoints() != 0 ) &&
                    ( tgtPartial.amplitudeAt( time ) == 0 );

    //  Don't insert Breakpoints at src times if all 
    //  morph functions equal 1 (or > MaxMorphParam),
    //  and a null is not needed.
    const double MaxMorphParam = .9;
    if ( fweight < MaxMorphParam ||
         aweight < MaxMorphParam ||
         bweight < MaxMorphParam ||
         needNull )
    {
        
        // adjust source Breakpoint frequencies according to the reference
        // Partial (if a reference has been specified):
        adjustFrequency( srcBkpt, _srcRefPartial, newp.label(), _freqFixThresholdDb, time );
            
        if ( 0 == tgtPartial.numBreakpoints() )
        {
            //  no corresponding target Partial exists:
            if ( 0 == _tgtRefPartial.numBreakpoints() )
            {
                //  no reference Partial specified for tgt,
                //  fade src instead:
                newp.insert( time, fadeSrcBreakpoint( srcBkpt, time ) );
            }
            else
            {
                //  reference Partial has been provided for tgt,
                //  use it to construct a fake Breakpoint to morph
                //  with the src:
                Breakpoint tgtBkpt = _tgtRefPartial.parametersAt( time );
                double fscale = (double) newp.label() / _tgtRefPartial.label();
                tgtBkpt.setFrequency( fscale * tgtBkpt.frequency() );
                tgtBkpt.setPhase( fscale * tgtBkpt.phase() );
                tgtBkpt.setAmplitude( 0 );
                tgtBkpt.setBandwidth( 0 );
                
                // compute interpolated Breakpoint parameters:
                newp.insert( time, interpolateParameters( srcBkpt, tgtBkpt, fweight, 
                                                          aweight, bweight ) );
            }
        }    
        else
        {
            Breakpoint tgtBkpt = tgtPartial.parametersAt( time );
            
            // adjust target Breakpoint frequencies according to the reference
            // Partial (if a reference has been specified):
            adjustFrequency( tgtBkpt, _tgtRefPartial, newp.label(), _freqFixThresholdDb, time );
            
            // compute interpolated Breakpoint parameters:
            Breakpoint morphed = interpolateParameters( srcBkpt, tgtBkpt, fweight, 
                                                        aweight, bweight );
            newp.insert( time, morphed );
        }
    }
}

// ---------------------------------------------------------------------------
//    appendMorphedTgt
// ---------------------------------------------------------------------------
//! Compute morphed parameter values at the specified time, using
//! the target Breakpoint (assumed to correspond exactly to the
//! specified time) and the source Partial (whose parameters are
//! examined at the specified time). Append the morphed Breakpoint
//! to newp only if the target should contribute to the morph at
//! the specified time.
//!
//! If the source Partial is a dummy Partial (no Breakpoints), fade the
//! target instead of morphing.
//!
//! \param  tgtBkpt is the Breakpoint corresponding to a morph function
//!         value of 1.
//! \param  srcPartial is the Partial corresponding to a morph function
//!         value of 0, evaluated at the specified time.
//! \param  time is the time corresponding to srcBkpt (used
//!         to evaluate the morphing functions and srcPartial).
//! \param  newp is the morphed Partial under construction, the morphed
//!         Breakpoint is added to this Partial.
//
void
Morpher::appendMorphedTgt( Breakpoint tgtBkpt, const Partial & srcPartial, 
                           double time, Partial & newp  )
{    
    double fweight = _freqFunction->valueAt( time );
    double aweight = _ampFunction->valueAt( time );
    double bweight = _bwFunction->valueAt( time );
    
    //  Need to insert a null (0 amplitude) Breakpoint
    //  if src and tgt are 0 amplitude but the morphed
    //  Partial is not. In rare cases, it is possible
    //  to miss a needed null if we don't check for it 
    //  explicitly. 
    bool needNull = ( newp.numBreakpoints() != 0 ) &&
                    ( newp.last().amplitude() != 0 ) &&
                    ( tgtBkpt.amplitude() == 0) &&
                    ( srcPartial.numBreakpoints() != 0 ) &&
                    ( srcPartial.amplitudeAt( time ) == 0 );

    //  Don't insert Breakpoints at src times if all 
    //  morph functions equal 0 (or < MinMorphParam),
    //  and a null is not needed.
    const double MinMorphParam = .1;
    if ( fweight > MinMorphParam ||
         aweight > MinMorphParam ||
         bweight > MinMorphParam ||
         needNull )
    {
        
        // adjust target Breakpoint frequencies according to the reference
        // Partial (if a reference has been specified):
        adjustFrequency( tgtBkpt, _tgtRefPartial, newp.label(), _freqFixThresholdDb, time );

        if ( 0 == srcPartial.numBreakpoints() )
        {
            //  no corresponding source Partial exists:
            if ( 0 == _srcRefPartial.numBreakpoints() )
            {
                //  no reference Partial specified for src,
                //  fade tgt instead:
                newp.insert( time, fadeTgtBreakpoint( tgtBkpt, time ) );
            }
            else
            {
                //  reference Partial has been provided for src,
                //  use it to construct a fake Breakpoint to morph
                //  with the tgt:
                Breakpoint srcBkpt = _srcRefPartial.parametersAt( time );
                double fscale = (double) newp.label() / _srcRefPartial.label();
                srcBkpt.setFrequency( fscale * srcBkpt.frequency() );
                srcBkpt.setPhase( fscale * srcBkpt.phase() );
                srcBkpt.setAmplitude( 0 );
                srcBkpt.setBandwidth( 0 );

                // compute interpolated Breakpoint parameters:
                newp.insert( time, interpolateParameters( srcBkpt, tgtBkpt, fweight, 
                                                          aweight, bweight ) );
            }
        }
        else
        {
            Breakpoint srcBkpt = srcPartial.parametersAt( time );

            // adjust source Breakpoint frequencies according to the reference
            // Partial (if a reference has been specified):
            adjustFrequency( srcBkpt, _srcRefPartial, newp.label(), _freqFixThresholdDb, time );

            // compute interpolated Breakpoint parameters:           
            Breakpoint morphed = interpolateParameters( srcBkpt, tgtBkpt, fweight, 
                                                        aweight, bweight );
            newp.insert( time, morphed );
        }
    }
}

}    //    end of namespace Loris
