#ifndef INCLUDE_RESAMPLER_H
#define INCLUDE_RESAMPLER_H
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
 * Resampler.h
 *
 * Definition of class Resampler, for converting reassigned Partial envelopes
 * into more conventional additive synthesis envelopes, having data points
 * at regular time intervals. The benefits of reassigned analysis are NOT
 * lost in this process, since the elimination of unreliable data and the
 * reduction of temporal smearing are reflected in the resampled data.
 *
 * Lippold, 7 Aug 2003
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */

#include "PartialList.h"
#include "LinearEnvelope.h"

//	begin namespace
namespace Loris {

class Partial;

// ---------------------------------------------------------------------------
//	class Resampler
//
//!	Class Resampler represents an algorithm for resampling Partial envelopes
//!	at regular time intervals. Resampling makes the envelope data more suitable 
//!	for exchange (as SDIF data, for example) with other applications that
//!	cannot process raw (continuously-distributed) reassigned data. Resampling
//!	will often greatly reduce the size of the data (by greatly reducing the 
//!	number of Breakpoints in the Partials) without adversely affecting the
//!	quality of the reconstruction.
//
class Resampler
{
//	--- public interface ---
public:
//	--- lifecycle ---

    //! Initialize a Resampler having the specified uniform sampling
    //! interval. Enable phase-correct resampling, in which frequencies
    //! of resampled Partials are modified (using fixFrequency) such 
    //! that the resampled phases are achieved in synthesis. Phase-
    //! correct resampling can be disabled using setPhaseCorrect.
    //!
    //! Resampled Partials will be composed of Breakpoints at every 
    //! integer multiple of the resampling interval.
    //!
    //! \sa setPhaseCorrect
    //! \sa fixFrequency
    //!
    //! \param  sampleInterval is the resampling interval in seconds, 
    //!         Breakpoint data is computed at integer multiples of
    //!         sampleInterval seconds.
    //!
    //! \throw  InvalidArgument if sampleInterval is not positive.
    explicit Resampler( double sampleInterval );


    // --- use compiler-generated copy/assign/destroy ---
    
//  --- parameters ---

    //! Specify phase-corrected resampling, or not. If phase
    //! correct, Partial frequencies are altered slightly
    //! to match, as nearly as possible, the Breakpoint 
    //! phases after resampling. Phases are updated so that
    //! the Partial frequencies and phases are consistent after
    //! resampling.
    //!
    //! \param  correctPhase is a boolean flag specifying that 
    //!         (if true) frequency/phase correction should be
    //!         applied after resampling.
    void setPhaseCorrect( bool correctPhase );
    	
//	--- resampling ---

	//! Resample the specified Partial using the stored quanitization interval. 
	//! The Breakpoint times will comprise a contiguous sequence of all integer 
	//! multiples of the sampling interval, starting and ending with the nearest 
	//! multiples to the ends of the Partial. If phase correct resampling is 
	//! specified (the default)≤ frequencies and phases are corrected to be in 
    //! agreement and to match as nearly as possible the resampled phases. 
    //!
    //! Resampling is performed in-place. 
    //!
    //! \param  p is the Partial to resample
    void resample( Partial & p ) const;

    //! Function call operator: same as resample( p ).
    void operator() ( Partial & p ) const 
    { 
        resample( p ); 
    }
    
	//! Resample the specified Partial using the stored quanitization interval. 
	//! The Breakpoint times will comprise a contiguous sequence of all integer 
	//! multiples of the sampling interval, starting and ending with the nearest 
	//! multiples to the ends of the Partial. If phase correct resampling is 
	//! specified (the default)≤ frequencies and phases are corrected to be in 
    //! agreement and to match as nearly as possible the resampled phases. 
    //!
    //! The timing envelope represents a warping of the time axis that is 
    //! applied during resampling. The Breakpoint times in resampled Partials 
    //! will a comprise contiguous sequence of all integer multiples of the 
    //! sampling interval between the first and last breakpoints in the timing 
    //! envelope, and each Breakpoint will represent the parameters of the 
    //! original Partial at the time that is the value of the timing envelope 
    //! at that instant. 
    //! 
    //! Resampling is performed in-place. 
    //!
    //! \param  p is the Partial to resample
    //!
    //! \param  timingEnv is the timing envelope, a map of Breakpoint 
    //!         times in resampled Partials onto parameter sampling 
    //!         instants in the original Partials.
    //!
    //! \throw  InvalidArgument if timingEnv has any negative breakpoint
    //!         times or values.
    void resample( Partial & p, const LinearEnvelope & timingEnv ) const;
    
    //! Quantize the Breakpoint times using the specified Partial using the 
    //! stored quanitization interval. Each Breakpoint in the Partial is 
    //! replaced by a Breakpoint constructed by resampling the Partial at 
    //! the nearest integer multiple of the of the resampling interval.
    //! 
    //! Quantization is performed in-place. 
    //!
    //! \param  p is the Partial to resample
    void quantize( Partial & p ) const;

	 
	//! Resample all Partials in the specified (half-open) range using this
	//! Resampler's stored quanitization interval. The Breakpoint times in 
	//! resampled Partials will comprise a contiguous sequence of all integer 
	//! multiples of the sampling interval, starting and ending with the nearest 
	//! multiples to the ends of the Partial. If phase correct resampling is 
	//! specified (the default)≤ frequencies and phases are corrected to be in 
    //! agreement and to match as nearly as possible the resampled phases. 
    //! 
    //! Resampling is performed in-place. 
	//!	
	//!	\param begin is the beginning of the range of Partials to resample
	//!	\param end is (one-past) the end of the range of Partials to resample
	//!	
	//!	If compiled with NO_TEMPLATE_MEMBERS defined, then begin and end
	//!	must be PartialList::iterators, otherwise they can be any type
	//!	of iterators over a sequence of Partials.
#if ! defined(NO_TEMPLATE_MEMBERS)
	template<typename Iter>
	void resample( Iter begin, Iter end ) const;
#else
   inline 
	void resample( PartialList::iterator begin, PartialList::iterator end  ) const;
#endif	 

	//!	Function call operator: same as resample( begin, end ).
#if ! defined(NO_TEMPLATE_MEMBERS)
	template<typename Iter>
	void operator()( Iter begin, Iter end ) const
#else
	void operator()( PartialList::iterator begin, PartialList::iterator end  ) const
#endif	 
	{ 
	   resample( begin, end ); 
	}
 
	//! Resample all Partials in the specified (half-open) range using this
	//! Resampler's stored quanitization interval. The Breakpoint times in 
	//! resampled Partials will comprise a contiguous sequence of all integer 
	//! multiples of the sampling interval, starting and ending with the nearest 
	//! multiples to the ends of the Partial. If phase correct resampling is 
	//! specified (the default)≤ frequencies and phases are corrected to be in 
    //! agreement and to match as nearly as possible the resampled phases. 
    //!
    //! The timing envelope represents a warping of the time axis that is 
    //! applied during resampling. The Breakpoint times in resampled Partials 
    //! will a comprise contiguous sequence of all integer multiples of the 
    //! sampling interval between the first and last breakpoints in the timing 
    //! envelope, and each Breakpoint will represent the parameters of the 
    //! original Partial at the time that is the value of the timing envelope 
    //! at that instant. 
    //! 
    //! Resampling is performed in-place. 
	//!	
	//!	\param  begin is the beginning of the range of Partials to resample
	//!	\param  end is (one-past) the end of the range of Partials to resample
    //! \param  timingEnv is the timing envelope, a map of Breakpoint 
    //!         times in resampled Partials onto parameter sampling 
    //!         instants in the original Partials.
	//!	
	//!	If compiled with NO_TEMPLATE_MEMBERS defined, then begin and end
	//!	must be PartialList::iterators, otherwise they can be any type
	//!	of iterators over a sequence of Partials.
#if ! defined(NO_TEMPLATE_MEMBERS)
	template<typename Iter>
	void resample( Iter begin, Iter end, const LinearEnvelope & timingEnv ) const;
#else
   inline 
	void resample( PartialList::iterator begin, PartialList::iterator end,
	               const LinearEnvelope & timingEnv) const;
#endif	 

    //! Quantize all Partials in the specified (half-open) range.
    //! Each Breakpoint in the Partials is replaced by a Breakpoint
    //! constructed by resampling the Partial at the nearest
    //! integer multiple of the of the resampling interval.
    //!	
    //!	\param begin is the beginning of the range of Partials to quantize
    //!	\param end is (one-past) the end of the range of Partials to quantize
    //!	
    //!	If compiled with NO_TEMPLATE_MEMBERS defined, then begin and end
    //!	must be PartialList::iterators, otherwise they can be any type
    //!	of iterators over a sequence of Partials.
#if ! defined(NO_TEMPLATE_MEMBERS)
	template<typename Iter>
	void quantize( Iter begin, Iter end ) const;
#else
   inline 
	void quantize( PartialList::iterator begin, PartialList::iterator end  ) const;
#endif	 
   	 
// -- static members --

	//! Static member that constructs an instance and applies
	//! it to a sequence of Partials. 
	//! Construct a Resampler using the specified resampling
	//! interval, and use it to channelize a sequence of Partials. 
	//!
	//! \param  begin is the beginning of a sequence of Partials to 
	//!         resample.
	//! \param  end is the end of a sequence of Partials to 
	//!         resample.
	//! \param  sampleInterval is the resampling interval in seconds, 
	//!         Breakpoint data is computed at integer multiples of
	//!         sampleInterval seconds.
    //! \param  denseResampling is a boolean flag indicating that dense
    //!         resamping (Breakpoint at every integer multiple of the 
    //!         resampling interval) should be performed. If false (the
    //!         default), sparse resampling (Breakpoints only at multiples
    //!         of the resampling interval near Breakpoint times in the
    //!         original Partial) is performed.
	//! \throw  InvalidArgument if sampleInterval is not positive.
	//!	
	//!	If compiled with NO_TEMPLATE_MEMBERS defined, then begin and end
	//!	must be PartialList::iterators, otherwise they can be any type
	//!	of iterators over a sequence of Partials.
#if ! defined(NO_TEMPLATE_MEMBERS)
	template< typename Iter >
	static 
	void resample( Iter begin, Iter end, double sampleInterval, 
	               bool denseResampling = false );
#else
	static inline 
	void resample( PartialList::iterator begin, PartialList::iterator end,
                  double sampleInterval, bool denseResampling = false );
#endif	 

//	--- instance variables ---
private:
    
    //! the resampling interval in seconds
    double interval_;	
    
    //! boolean flag selecting phase-corrected resampling
    //! (default is true)
    bool phaseCorrect_;
	
};	//	end of class Resampler

// ---------------------------------------------------------------------------
//	resample (sequence of Partials)
// ---------------------------------------------------------------------------
//! Resample all Partials in the specified (half-open) range using this
//! Resampler's stored sampling interval, so that the Breakpoints in 
//! the Partial envelopes will all lie on a common temporal grid.
//! The Breakpoint times in the resampled Partial will comprise a  
//! contiguous sequence of integer multiples of the sampling interval,
//! beginning with the multiple nearest to the Partial's start time and
//! ending with the multiple nearest to the Partial's end time. Resampling
//! is performed in-place. 
//!	
//!	\param begin is the beginning of the range of Partials to resample
//!	\param end is (one-past) the end of the range of Partials to resample
//!	
//!	If compiled with NO_TEMPLATE_MEMBERS defined, then begin and end
//!	must be PartialList::iterators, otherwise they can be any type
//!	of iterators over a sequence of Partials.
//
#if ! defined(NO_TEMPLATE_MEMBERS)
template<typename Iter>
void Resampler::resample( Iter begin, Iter end ) const
#else
inline 
void Resampler::resample( PartialList::iterator begin, PartialList::iterator end  ) const
#endif	 
{
	while ( begin != end )
	{
		resample( *begin++ );
	}
}

// ---------------------------------------------------------------------------
//	resample (sequence of Partials, with timing envelope)
// ---------------------------------------------------------------------------
//! Resample all Partials in the specified (half-open) range using this
//! Resampler's stored sampling interval, so that the Breakpoints in 
//! the Partial envelopes will all lie on a common temporal grid.
//! The Breakpoint times in the resampled Partial will comprise a  
//! contiguous sequence of integer multiples of the sampling interval,
//! beginning with the multiple nearest to the Partial's start time and
//! ending with the multiple nearest to the Partial's end time. Resampling
//! is performed in-place. 
//!	
//!	\param begin is the beginning of the range of Partials to resample
//!	\param end is (one-past) the end of the range of Partials to resample
//! \param  timingEnv is the timing envelope, a map of Breakpoint 
//!         times in resampled Partials onto parameter sampling 
//!         instants in the original Partials.
//!	
//!	If compiled with NO_TEMPLATE_MEMBERS defined, then begin and end
//!	must be PartialList::iterators, otherwise they can be any type
//!	of iterators over a sequence of Partials.
//
#if ! defined(NO_TEMPLATE_MEMBERS)
template<typename Iter>
void Resampler::resample( Iter begin, Iter end, const LinearEnvelope & timingEnv ) const
#else
inline 
void Resampler::resample( PartialList::iterator begin, PartialList::iterator end, 
                          const LinearEnvelope & timingEnv  ) const
#endif	 
{
	while ( begin != end )
	{
		resample( *begin++, timingEnv );
	}
}

// ---------------------------------------------------------------------------
//	quantize (sequence of Partials)
// ---------------------------------------------------------------------------
//! Quantize all Partials in the specified (half-open) range.
//! Each Breakpoint in the Partials is replaced by a Breakpoint
//! constructed by resampling the Partial at the nearest
//! integer multiple of the of the resampling interval.
//!	
//!	\param begin is the beginning of the range of Partials to quantize
//!	\param end is (one-past) the end of the range of Partials to quantize
//!	
//!	If compiled with NO_TEMPLATE_MEMBERS defined, then begin and end
//!	must be PartialList::iterators, otherwise they can be any type
//!	of iterators over a sequence of Partials.
//
#if ! defined(NO_TEMPLATE_MEMBERS)
template<typename Iter>
void Resampler::quantize( Iter begin, Iter end ) const
#else
inline 
void Resampler::quantize( PartialList::iterator begin, PartialList::iterator end  ) const
#endif	 
{
	while ( begin != end )
	{
		quantize( *begin++ );
	}
}


// ---------------------------------------------------------------------------
//	resample (static)
// ---------------------------------------------------------------------------
//! Static member that constructs an instance and applies
//! phase-correct resampling to a sequence of Partials. 
//! Construct a Resampler using the specified resampling
//! interval, and use it to channelize a sequence of Partials. 
//!
//! \param  begin is the beginning of a sequence of Partials to 
//!         resample.
//! \param  end is the end of a sequence of Partials to 
//!         resample.
//! \param  sampleInterval is the resampling interval in seconds, 
//!         Breakpoint data is computed at integer multiples of
//!         sampleInterval seconds.
//! \param  denseResampling is a boolean flag indicating that dense
//!         resamping (Breakpoint at every integer multiple of the 
//!         resampling interval) should be performed. If false (the
//!         default), sparse resampling (Breakpoints only at multiples
//!         of the resampling interval near Breakpoint times in the
//!         original Partial) is performed.
//! \throw  InvalidArgument if sampleInterval is not positive.
//!	
//!	If compiled with NO_TEMPLATE_MEMBERS defined, then begin and end
//!	must be PartialList::iterators, otherwise they can be any type
//!	of iterators over a sequence of Partials.
//
#if ! defined(NO_TEMPLATE_MEMBERS)
template< typename Iter >
void Resampler::resample( Iter begin, Iter end, double sampleInterval, 
                          bool denseResampling )
#else
inline
void Resampler::resample( PartialList::iterator begin, PartialList::iterator end,
                          double sampleInterval, bool denseResampling )
#endif	 
{
    Resampler instance( sampleInterval );
    
    if ( denseResampling )
    {
        instance.resample( begin, end );
    }
    else
    {
        instance.quantize( begin, end );
    }
}

}	//	end of namespace Loris

#endif /* ndef INCLUDE_RESAMPLER_H */

