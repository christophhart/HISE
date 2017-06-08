/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for cloused source applications:
*
*   http://www.juce.com
*
*	Some parts of this file are written by Sean Enderby.
*
*   ===========================================================================
*/

#ifndef HI_FILTER_INFO_H_INCLUDED
#define HI_FILTER_INFO_H_INCLUDED

#include <complex>
#include <vector>

#ifndef double_E
#define double_E 2.71828183
#endif
//====================================================================================
enum FilterType
{
	LowPass,
	HighPass
};
    
enum BandType
{
    LowShelf,
    HighShelf,
    Peak
};

//==============================================================================
class FilterResponse
{
public:
    FilterResponse (double magnitudeInit, double phaseInit);
    ~FilterResponse();
    
    double magnitudeValue, phaseValue;
};

//============================================================================
class FilterInfo : public IIRFilter
{
public:    
    FilterInfo();
    ~FilterInfo();
    
    void setSampleRate (double sampleRate);
    void setGain (double gain);
    
    FilterResponse getResponse (double inputFrequency) const;
    
    void zeroCoeffs();
    
    void setFilter (double frequency, FilterType filterType);
    void setEqBand (double frequency, double Q, float gain, BandType eqType);
    
    void setCustom (std::vector <double> numCoeffs, std::vector <double> denCoeffs);

	void setCoefficients(int filterNum, double sampleRate, IIRCoefficients newCoefficients);

	bool isEnabled()
	{
		return enabled;
	}

	void setEnabled(bool shouldBeEnabled)
	{
		enabled = shouldBeEnabled;
	};
    
private:
    double fs;
    int numNumeratorCoeffs, numDenominatorCoeffs;

    std::vector <double> numeratorCoeffs;
    std::vector <double> denominatorCoeffs;
    double gainValue;

	bool enabled;
};

#endif