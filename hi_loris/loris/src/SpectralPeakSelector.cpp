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
 * SpectralPeakSelector.C
 *
 * Implementation of a class representing a policy for selecting energy
 * peaks in a reassigned spectrum to be used in Partial formation.
 *
 * Kelly Fitz, 28 May 2003
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */

#if HAVE_CONFIG_H
	#include "config.h"
#endif

#include "SpectralPeakSelector.h"


#include "Breakpoint.h"
#include "Notifier.h"
#include "ReassignedSpectrum.h"


#include <cmath>    //  for abs and fabs


// define this to use local minima in frequency
// reassignment to detect "peaks", otherwise 
// magnitude peaks are used.
#define USE_REASSIGNMENT_MINS 1
//#undef USE_REASSIGNMENT_MINS


//	begin namespace
namespace Loris {

// ---------------------------------------------------------------------------
//	construction - constant resolution
// ---------------------------------------------------------------------------
SpectralPeakSelector::SpectralPeakSelector( double srate, double maxTimeCorrection ) :
	mSampleRate( srate ),
    mMaxTimeOffset( maxTimeCorrection )
{
}

// ---------------------------------------------------------------------------
//	selectPeaks
// ---------------------------------------------------------------------------
//	Collect and return magnitude peaks in the lower half of the spectrum, 
//	ignoring those having frequencies below the specified minimum, and
//	those having large time corrections.
//
//  If the minimumFrequency is unspecified, 0 Hz is used.
//
//  There are two strategies for doing. Probably each one should be a 
//  separate class, but for now, they are just separate functions.

Peaks
SpectralPeakSelector::selectPeaks( ReassignedSpectrum & spectrum, 
                                   double minFrequency )
{
#if defined(USE_REASSIGNMENT_MINS) && USE_REASSIGNMENT_MINS

    return selectReassignmentMinima( spectrum, minFrequency );
    
#else

    return selectMagnitudePeaks( spectrum, minFrequency );
    
#endif
}

// ---------------------------------------------------------------------------
//	selectReassignmentMinima (private)
// ---------------------------------------------------------------------------
Peaks
SpectralPeakSelector::selectReassignmentMinima( ReassignedSpectrum & spectrum, 
                                                double minFrequency )
{
	using namespace std; // for abs and fabs

	const double sampsToHz = mSampleRate / spectrum.size();
	const double oneOverSR = 1. / mSampleRate;
	const double minFreqSample = minFrequency / sampsToHz;
	const double maxCorrectionSamples = mMaxTimeOffset * mSampleRate;
	
	Peaks peaks;
	
	int start_j = 1, end_j = (spectrum.size() / 2) - 2;
	
	double fsample = start_j;
	do 
	{
	    fsample = spectrum.reassignedFrequency( start_j++ );
	} while( fsample < minFreqSample );
	
	for ( int j = start_j; j < end_j; ++j ) 
	{	 

	    // look for changes in the frequency reassignment,
	    // from positive to negative correction, indicating
	    // a concentration of energy in the spectrum:
	    double next_fsample = spectrum.reassignedFrequency( j+1 );
	    if ( fsample > j && next_fsample < j + 1 )
	    {
	        //  choose the smaller correction of fsample or next_fsample:
	        // (could also choose the larger magnitude?)
	        double freq;
	        int peakidx;
	        if ( (fsample-j) < (j+1-next_fsample) )
	        {
	            freq = fsample * sampsToHz;
	            peakidx = j;
	        }
	        else
	        {
	            freq = next_fsample * sampsToHz;
	            peakidx = j+1;
	        }
            
            //  still possible that the frequency winds up being
            //  below the specified minimum
            if ( freq >= minFrequency )
            {            	         
                //	keep only peaks with small time corrections:
                double timeCorrectionSamps = spectrum.reassignedTime( peakidx );
                if ( fabs(timeCorrectionSamps) < maxCorrectionSamples )
                {
                    double mag = spectrum.reassignedMagnitude( peakidx );
                    double phase = spectrum.reassignedPhase( peakidx );    			

                    //	this will be overwritten later in analysis, 
                    //	might be ignored altogether, only used if the
                    //	mixed derivative convergence indicator is stored
                    //	as bandwidth in Analyzer:
                    double bw = spectrum.convergence( j );


                    //	also store the corrected peak time in seconds, won't
                    //	be able to compute it later:
                    double time = timeCorrectionSamps * oneOverSR;
                    Breakpoint bp( freq, mag, bw, phase );
                    peaks.push_back( SpectralPeak( time, bp ) );
                }
            }
                	        
	    }
	    fsample = next_fsample;
	}
    
	/*
	debugger << "SpectralPeakSelector::selectReassignmentMinima: found " 
             << peaks.size() << " peaks" << endl;
	*/
    	
	return peaks;

}

// ---------------------------------------------------------------------------
//	selectMagnitudePeaks (private)
// ---------------------------------------------------------------------------
Peaks
SpectralPeakSelector::selectMagnitudePeaks( ReassignedSpectrum & spectrum,
                                            double minFrequency )
{
	using namespace std; // for abs and fabs

	const double sampsToHz = mSampleRate / spectrum.size();
	const double oneOverSR = 1. / mSampleRate;
	const double minFreqSample = minFrequency / sampsToHz;
	const double maxCorrectionSamples = mMaxTimeOffset * mSampleRate;
	
	Peaks peaks;
	
	int start_j = 1, end_j = (spectrum.size() / 2) - 2;
	
	double fsample = start_j;
	do 
	{
	    fsample = spectrum.reassignedFrequency( start_j++ );
	} while( fsample < minFreqSample );
	
	for ( int j = start_j; j < end_j; ++j ) 
	{	 
		if ( spectrum.reassignedMagnitude(j) > spectrum.reassignedMagnitude(j-1) && 
			 spectrum.reassignedMagnitude(j) > spectrum.reassignedMagnitude(j+1) ) 
		{				
			//	skip low-frequency peaks:
			double fsample = spectrum.reassignedFrequency( j );
			if ( fsample < minFreqSample )
				continue;

			//	skip peaks with large time corrections:
			double timeCorrectionSamps = spectrum.reassignedTime( j );
			if ( fabs(timeCorrectionSamps) > maxCorrectionSamples )
				continue;
				
			double mag = spectrum.reassignedMagnitude( j );
			double phase = spectrum.reassignedPhase( j );
			
			//	this will be overwritten later in analysis, 
			//	might be ignored altogether, only used if the
			//	mixed derivative convergence indicator is stored
			//	as bandwidth in Analyzer:
			double bw = spectrum.convergence( j );
			
			//	also store the corrected peak time in seconds, won't
			//	be able to compute it later:
			double time = timeCorrectionSamps * oneOverSR;
			Breakpoint bp ( fsample * sampsToHz, mag, bw, phase );
			peaks.push_back( SpectralPeak( time, bp ) );
						
		}	//	end if itsa peak
	}
	
    /*
	debugger << "SpectralPeakSelector::selectMagnitudePeaks: found " 
             << peaks.size() << " peaks" << endl;
    */         		
	return peaks;
}


}	//	end of namespace Loris
