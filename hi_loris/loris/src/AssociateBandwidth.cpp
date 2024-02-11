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
 * AssociateBandwidth.C
 *
 * Implementation of a class representing a policy for associating noise
 * (bandwidth) energy with reassigned spectral peaks to be used in
 * Partial formation.
 *
 * Kelly Fitz, 20 Jan 2000
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */

#if HAVE_CONFIG_H
	#include "config.h"
#endif

#include "AssociateBandwidth.h"
#include "Breakpoint.h"
#include "BreakpointUtils.h"
#include "LorisExceptions.h"
#include "Notifier.h"
#include "SpectralPeaks.h"

#include <algorithm>
#include <cmath>

using namespace Loris;

// ---------------------------------------------------------------------------
//	AssociateBandwidth constructor
// ---------------------------------------------------------------------------
//	Association regions are centered on all integer bin frequencies, regionWidth
//	is the total width (in Hz) of the overlapping bandwidth association regions, 
//	the region centers are spaced at half this width.
//
AssociateBandwidth::AssociateBandwidth( double regionWidth, double srate ) :
	_regionRate( 0 )
{
	if ( ! (regionWidth>0) )
		Throw( InvalidArgument, "The regionWidth must be greater than 0 Hz." );
	if ( ! (srate>0) )
		Throw( InvalidArgument, "The sample rate must be greater than 0 Hz." );
		
		
	_weights.resize( int(srate/regionWidth) );
	_surplus.resize( int(srate/regionWidth) );
	_regionRate = 2./regionWidth;
}	

// ---------------------------------------------------------------------------
//	AssociateBandwidth destructor
// ---------------------------------------------------------------------------
//
AssociateBandwidth::~AssociateBandwidth( void )
{
}

// ---------------------------------------------------------------------------
//	binFrequency
// ---------------------------------------------------------------------------
//	Compute the warped fractional bin/region frequency corresponding to 
//	freqHz. (_regionRate is the number of regions per hertz.)
//
//	Once, we used bark frequency scale warping here, but there seems to be
//	no reason to do so. The best results seem to be indistinguishable from
// 	plain 'ol 1k bins, and some results are much worse.
//	
static double binFrequency( double freqHz, double regionRate )
{
//#define Use_Barks
#ifndef Use_Barks
	return freqHz * regionRate;
#else
	//	Compute Bark frequency from Hertz.
	//	Got this formula for Bark frequency from Sciarraba's thesis.
	//	Ignore region rate when using barks
	double tmp = std::atan( ( 0.001 / 7.5 ) * freqHz );
	return  13. * std::atan( 0.76 * 0.001 * freqHz ) + 3.5 * ( tmp * tmp );
#endif
	
}

// ---------------------------------------------------------------------------
//	findRegionBelow
// ---------------------------------------------------------------------------
//	Return the index of the last region having center frequency less than
//	or equal to freq, or -1 if no region is low enough. 
//
//	Note: the zeroeth region is centered at bin frequency 1 and tapers
//	to zero at bin frequency 0! (when booger is 1.)
//
static int findRegionBelow( double binfreq, unsigned int howManyBins )
{
	const double booger = 0.;
	if ( binfreq < booger ) 
	{
		return -1;
	}
	else 
	{
		return int( std::min( std::floor(binfreq - booger), howManyBins - 1. ) );
	}
}

// ---------------------------------------------------------------------------
//	computeAlpha
// ---------------------------------------------------------------------------
//	binfreq is a warped, fractional bin frequency, and bin frequencies 
//	are integers. Return the relative contribution of a component at
//	binfreq to the bins (bw association regions) below and above
//	binfreq. 
//
static double computeAlpha( double binfreq, unsigned int howManyBins )
{
	//	everything above the center of the highest
	//	bin is lumped into that bin; i.e it does
	//	not taper off at higher frequencies:	
	if ( binfreq > howManyBins ) 
	{
		return 0.;
	}
	else 
	{
		return binfreq - std::floor( binfreq );
	}
}

// ---------------------------------------------------------------------------
//	distribute
// ---------------------------------------------------------------------------
//
static void distribute( double fractionalBin, double x, std::vector<double> & regions )
{
	//	contribute x to two regions having center
	//	frequencies less and greater than freqHz:
	int posBelow = findRegionBelow( fractionalBin, regions.size() );
	int posAbove = posBelow + 1;
	
	double alpha = computeAlpha( fractionalBin, regions.size() );
	
	if ( posAbove < regions.size() )
		regions[posAbove] += alpha * x;
	
	if ( posBelow >= 0 )
		regions[posBelow] += (1. - alpha) * x;
}

// ---------------------------------------------------------------------------
//	computeNoiseEnergy
// ---------------------------------------------------------------------------
//	Return the noise energy to be associated with a component at freqHz.
//	_surplus contains the surplus spectral energy in each region, which is,
//	by defintion, non-negative.
//
double 
AssociateBandwidth::computeNoiseEnergy( double freq, double amp )
{
	//	don't mess with negative frequencies:
	if ( freq < 0. )
		return 0.;
	
	//	compute the fractional bin frequency 
	//	corresponding to freqHz:
	double bin = binFrequency( freq, _regionRate );
	
	//	contribute x to two regions having center
	//	frequencies less and greater than freqHz:
	int posBelow = findRegionBelow( bin, _surplus.size() );
	int posAbove = posBelow + 1;

	double alpha = computeAlpha( bin, _surplus.size() );

	double noise = 0.;
	//	Have to check for alpha == 0, because 
	//	the weights will be zero (see computeAlpha()):
	//	(ignore lowest regions)
	const int LowestRegion = 2;
	/*
	if ( posAbove < _surplus.size() && alpha != 0. && posAbove >= LowestRegion )
		noise += _surplus[posAbove] * alpha / _weights[posAbove];
	
	if ( posBelow >= LowestRegion )
		noise += _surplus[posBelow] * (1. - alpha) / _weights[posBelow];
	*/
	//	new idea, weight Partials by amplitude:	
	if ( posAbove < _surplus.size() && alpha != 0. && posAbove >= LowestRegion )
		noise += _surplus[posAbove] * alpha * amp / _weights[posAbove];
	
	if ( posBelow >= LowestRegion )
		noise += _surplus[posBelow] * (1. - alpha) * amp / _weights[posBelow];
		
	return noise;
}

// ---------------------------------------------------------------------------
//	accumulateSinusoid
// ---------------------------------------------------------------------------
//	Accumulate sinusoidal energy at frequency f and amplitude a.
//	The amplitude isn't used for anything.
//	
void
AssociateBandwidth::accumulateSinusoid( double freq, double amp )
{
	//	distribute weight at the peak frequency,
	//	don't mess with negative frequencies:
	if ( freq > 0. )
	{
		//distribute( binFrequency( freq, _regionRate ), 1., _weights );
		//	new idea: weight Partials by amplitude:
		distribute( binFrequency( freq, _regionRate ), amp, _weights );
	}
}

// ---------------------------------------------------------------------------
//	accumulateNoise
// ---------------------------------------------------------------------------
//	Accumulate a rejected spectral peak as surplus (noise) energy.
//
void
AssociateBandwidth::accumulateNoise( double freq, double amp )
{
	//	compute energy contribution and distribute 
	//	at frequency f, don't mess with negative 
	//	frequencies:
	if ( freq > 0. )
    {
		distribute( binFrequency( freq, _regionRate ), amp * amp, _surplus  );
    }
}

// ---------------------------------------------------------------------------
//	associate
// ---------------------------------------------------------------------------
//	Associate bandwidth with a single SpectralPeak.
//
void 
AssociateBandwidth::associate( SpectralPeak & pk )
{		
    pk.setBandwidth(0);
    pk.addNoiseEnergy( computeNoiseEnergy( pk.frequency(), pk.amplitude() ) );
}

// ---------------------------------------------------------------------------
//	reset
// ---------------------------------------------------------------------------
//	This is called after each distribution of bandwidth energy.
//
void
AssociateBandwidth::reset( void )
{
	std::fill( _weights.begin(), _weights.end(), 0. );
	std::fill( _surplus.begin(), _surplus.end(), 0. );
}

// ---------------------------------------------------------------------------
//	associate
// ---------------------------------------------------------------------------
//	Perform bandwidth association on a collection of reassigned spectral peaks
//	or ridges. The range [begin, rejected) spans the Peaks selected to form
//	Partials. The range [rejected, end) spans the Peaks that were found in
//	the reassigned spectrum, but rejected as too weak or too close (in 
//	frequency) to another stronger Peak. 
//
void 
AssociateBandwidth::associateBandwidth( Peaks::iterator begin, 		//	beginning of Peaks
										Peaks::iterator rejected, 	//	first rejected Peak
										Peaks::iterator end )		//	end of Peaks
{		
	if ( begin == rejected )
		return;
		
	//	accumulate retained Breakpoints as sinusoids, 
	for ( Peaks::iterator it = begin; it != rejected; ++it )
	{
		accumulateSinusoid( it->frequency(), it->amplitude() );
	}
	
	//	accumulate rejected breakpoints as noise:
	for ( Peaks::iterator it = rejected; it != end; ++it )
	{
		accumulateNoise( it->frequency(), it->amplitude() );
	}

	//	associate bandwidth with each retained Breakpoint:
	for ( Peaks::iterator it = begin; it != rejected; ++it )
	{
        //  sets bandwidth to zero, then calls addNoiseEnergy()
		associate( *it );
	}
	
	//	reset after association, yuk:
	reset();

}
