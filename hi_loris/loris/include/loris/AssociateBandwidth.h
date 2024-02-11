#ifndef INCLUDE_ASSOCIATEBANDWIDTH_H
#define INCLUDE_ASSOCIATEBANDWIDTH_H
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
 * AssociateBandwidth.h
 *
 * Definition of a class representing a policy for associating noise
 * (bandwidth) energy with reassigned spectral peaks to be used in
 * Partial formation.
 *
 * Kelly Fitz, 20 Jan 2000
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */

#include "SpectralPeaks.h"

#include <vector>

//	begin namespace
namespace Loris {

class Breakpoint;

// ---------------------------------------------------------------------------
//	class AssociateBandwidth
//
//	In the new strategy, Breakpoints are extracted and accumulated
//	as sinusoids. Spectral peaks that are not extracted (don't exceed
//	the amplitude floor) or are rejected for certain reasons, are 
//	accumulated diectly as noise (surplus). After all spectral peaks 
//	have been accumulated as noise or sinusoids, the noise is distributed 
//	as bandwidth.
//
class AssociateBandwidth
{
//	-- instance variables --
	std::vector< double > _weights;	//	weights vector for recording 
									//	frequency distribution of retained
									//	sinusoids
	std::vector< double > _surplus; //	surplus (noise) energy vector for
	 								//	accumulating the distribution of
	 								//	spectral energy to be distributed 
	 								//	as noise
	
	double _regionRate;				//	inverse of region center spacing
	
//	-- public interface --
public:
	//	construction:
	AssociateBandwidth( double regionWidth, double srate );
	~AssociateBandwidth( void );
	
	//	Perform bandwidth association on a collection of reassigned spectral peaks
	//	or ridges. The range [begin, rejected) spans the Peaks selected to form
	//	Partials. The range [rejected, end) spans the Peaks that were found in
	//	the reassigned spectrum, but rejected as too weak or too close (in 
	//	frequency) to another stronger Peak. 
	void associateBandwidth( Peaks::iterator begin, 	//	beginning of Peaks
							 Peaks::iterator rejected, 	//	first rejected Peak
							 Peaks::iterator end );		//	end of Peaks
	
		
//	-- private helpers --	
private:	
	double computeNoiseEnergy( double freq, double amp );
	
	//	These four formerly comprised the public interface
	//	to this policy, now they are all hidden behind a 
	//	single call to associateBandwidth.
	
	//	energy accumulation:
	void accumulateNoise( double freq, double amp );	
	void accumulateSinusoid( double freq, double amp  );	
	
	//	bandwidth assocation:
	void associate( SpectralPeak & pk );
	
	//	call this to wipe out the accumulated energy to 
	//	prepare for the next frame (yuk):
	void reset( void );
	
};	// end of class AssociateBandwidth

}	//	end of namespace Loris

#endif /* ndef INCLUDE_ASSOCIATEBANDWIDTH_H */
