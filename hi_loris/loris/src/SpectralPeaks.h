#ifndef INCLUDE_SPECTRALPEAKS_H
#define INCLUDE_SPECTRALPEAKS_H
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
 * SpectralPeaks.h
 *
 * Definition of a type representing a collection (vector) of 
 * reassigned spectral peaks or ridges. Shared by analysis policies.
 *
 * Kelly Fitz, 29 May 2003
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */
 
#include "Breakpoint.h"

#include <vector>

//	begin namespace
namespace Loris {

//  define a spectral peak data structure
//
//  HEY
//  Clean this mess up! Upgrade this struct into a class that can 
//  store more kinds of information (like reassignment values), and 
//  creates a Breakpoint when needed.

class SpectralPeak
{
private:

    double m_time;
    Breakpoint m_breakpoint;

public:

    //  --- lifecycle ---
    
    SpectralPeak( double t, const Breakpoint & bp ) : m_time( t ), m_breakpoint( bp ) {}
    
    
    //  --- access ---
    
    double time( void ) const { return m_time; }
    
    double amplitude( void ) const { return m_breakpoint.amplitude(); }
    double frequency( void ) const { return m_breakpoint.frequency(); }
    double bandwidth( void ) const { return m_breakpoint.bandwidth(); }
    
    //  --- mutation ---
    
    void setAmplitude( double x ) { m_breakpoint.setAmplitude(x); }
    void setBandwidth( double x ) { m_breakpoint.setBandwidth(x); }
    
    //  this REALLY shouldn't be in here...
    void addNoiseEnergy( double enoise ) { m_breakpoint.addNoiseEnergy(enoise); }
    
    //  --- Breakpoint creation ---
    
    Breakpoint createBreakpoint( void ) const { return m_breakpoint; }
    
    //  --- comparitors ---
    
    //	Comparitor for sorting spectral peaks in order of 
    //	increasing frequency, or finding maximum frequency.

    static bool sort_increasing_freq( const SpectralPeak & lhs, 
                                      const SpectralPeak & rhs )
    { 
        return lhs.frequency()  < rhs.frequency(); 
    }

    //	predicate used for sorting peaks in order of decreasing amplitude:
    static bool sort_greater_amplitude( const SpectralPeak & lhs, 
                                        const SpectralPeak & rhs )
    { 
        return lhs.amplitude() > rhs.amplitude(); 
    }
};

//	define the structure used to collect spectral peaks:
typedef std::vector< SpectralPeak > Peaks;



}	//	end of namespace Loris

#endif /* ndef INCLUDE_SPECTRALPEAKS_H */
