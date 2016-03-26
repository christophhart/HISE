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
*   along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*	Some parts of this file are written by Sean Enderby.
*
*   ===========================================================================
*/

//==============================================================================
FilterGraph::FilterGraph (int numFiltersInit, int drawType_)
    :  filterVector (),
	drawType((DrawType)drawType_)
{
	setOpaque(true);

    numHorizontalLines = 7;
    setSize (500, 300);
    lowFreq = 20;
    highFreq = 20000;
    fs = 44100;
    maxdB = 18;
    maxPhas = 1;
    numFilters = numFiltersInit;
    traceColour = Colour (0xFF990000);
    traceType = Magnitude;
}

FilterGraph::~FilterGraph()
{
}

void FilterGraph::refreshFilterPath()
{
	
	float width = (float) getWidth();
	float height = (float) getHeight();

	tracePath.clear();
    
	float scaleFactor = (((height / 2) - (height - 5) / (numHorizontalLines + 1) - 2.5f) / maxdB);
	
	if(numFilters == 0)
	{
		clearFilterPath();
	}
	else
	{
    
		Array<FilterInfo*> activeFilters;

		for(int i = 0; i < numFilters; i++)
		{
			if(filterVector[i]->isEnabled()) activeFilters.add(filterVector[i]);
		}

		const int numActiveFilters = activeFilters.size();

		if(numActiveFilters == 0)
		{
			clearFilterPath();
			return;
		}

		float traceMagnitude = (float) (activeFilters[0]->getResponse (lowFreq).magnitudeValue);
        
		jassert(traceMagnitude > 0.0f);

		for (int i = 1; i < numActiveFilters; i++)
		{
			traceMagnitude *= (float) (activeFilters [i]->getResponse (lowFreq).magnitudeValue);
		}
		traceMagnitude = 20 * log10 (traceMagnitude);
		
		if(drawType == Line)
		{
			tracePath.startNewSubPath (-3, height / 2);
			//tracePath.lineTo (-1, (height / 2) - (traceMagnitude * scaleFactor));

		}
		else if(drawType == Icon)
		{
			tracePath.startNewSubPath (0, height);
		}
		else
		{
			tracePath.startNewSubPath (-1, height+1);
			tracePath.lineTo (-1, (height / 2) - (traceMagnitude * scaleFactor));
		}

		for (float xPos = 0; xPos < width; xPos += 1)
		{
			float freq = xToFreq (xPos);
            
			traceMagnitude = (float) (activeFilters [0]->getResponse (freq).magnitudeValue);
            
			for (int i = 1; i < numActiveFilters; i++)
			{
				traceMagnitude *= (float) (activeFilters [i]->getResponse (freq).magnitudeValue);
			}
			traceMagnitude = 20 * log10 (traceMagnitude);
        
			tracePath.lineTo (xPos, (height / 2) - (traceMagnitude * scaleFactor));
		}

		if(drawType == Line)
		{
			tracePath.lineTo (width+3, (height / 2));

		}
		else if (drawType == Icon)
		{
			tracePath.lineTo(width, height);
			tracePath.closeSubPath();
		}
		else
		{
			tracePath.lineTo (width+1, (height / 2) - (traceMagnitude * scaleFactor));
			tracePath.lineTo (width+1, height+1);

			tracePath.closeSubPath();

		}
    
		

		return;
	}
}

void FilterGraph::changeListenerCallback(SafeChangeBroadcaster *b)
{
	CurveEq *eq = dynamic_cast<CurveEq*>(b);

	if(eq != nullptr)
	{
		if(eq->getNumFilterBands() != numFilters)
		{
			clearBands();

			for(int i = 0; i < eq->getNumFilterBands(); i++)
			{
				filterVector.add(new FilterInfo());
			}
		}

		numFilters = eq->getNumFilterBands();

		for(int i = 0; i < eq->getNumFilterBands(); i++)
		{
			setCoefficients(i, eq->getSampleRate(), eq->getCoefficients(i));
			enableBand(i, eq->getFilterBand(i)->isEnabled());
		}
		
		repaint();
		
	}
}

void FilterGraph::paint (Graphics& g)
{
	if(drawType == Icon)
	{
		g.fillAll(Colour(0xff111111));
		refreshFilterPath();

		g.setGradientFill (ColourGradient (Colour (0xaaffffff),
										0.0f, 0.0f,
										Colour (0x55ffffff),
										0.0f, (float) getHeight(),
										false));

		g.fillPath(tracePath);


		g.drawRect(getLocalBounds(), 1);

	}
	else
	{
		paintBackground(g);

		paintGridLines(g);
		
		refreshFilterPath();

		KnobLookAndFeel::fillPathHiStyle(g, tracePath, getWidth(), getHeight());
	}

	
    
}

void FilterGraph::addFilter(FilterType filterType){
	this->filterVector.add(new FilterInfo());
	filterVector.getLast()->setFilter(100, filterType);
	this->numFilters = filterVector.size();
}

void FilterGraph::addEqBand(BandType eqType){
	this->filterVector.add(new FilterInfo());
	filterVector.getLast()->setEqBand(100, 1.0, 0.0, eqType);
	this->numFilters = filterVector.size();
}

void FilterGraph::resized()
{
}

void FilterGraph::setNumHorizontalLines (int newValue)
{
    numHorizontalLines = newValue;
    repaint();
}

void FilterGraph::setFreqRange (float newLowFreq, float newHighFreq)
{
    lowFreq = fabs (newLowFreq + 0.1f);
    highFreq = fabs (newHighFreq);
    repaint();
}

void FilterGraph::setFilterGain (int filterNum, double gain)
{
    filterVector [filterNum]->setGain (gain);
    repaint();
}

void FilterGraph::setFilter (int filterNum, double sampleRate, double frequency, FilterType filterType)
{    
    filterVector [filterNum]->setSampleRate (sampleRate);
    filterVector [filterNum]->setFilter (frequency, filterType);
    
    fs = sampleRate;
    repaint();
}

void FilterGraph::setEqBand (int filterNum, double sampleRate, double frequency, double Q, float gain, BandType eqType)
{    
    filterVector [filterNum]->setSampleRate (sampleRate);
    filterVector [filterNum]->setEqBand (frequency, Q, gain, eqType);
    
    fs = sampleRate;
    repaint();
}

void FilterGraph::setCustom (int filterNum, double sampleRate, std::vector <double> numCoeffs, std::vector <double> denCoeffs)
{
    filterVector [filterNum]->setSampleRate (sampleRate);
    filterVector [filterNum]->setCustom (numCoeffs, denCoeffs);
    
    fs = sampleRate;
    repaint();
}

void FilterGraph::setCoefficients(int filterNum, double sampleRate, IIRCoefficients newCoefficients)
{
	filterVector [filterNum]->setSampleRate (sampleRate);
    filterVector [filterNum]->setCoefficients(filterNum, sampleRate, newCoefficients);

	fs = sampleRate;
    repaint();

}


float FilterGraph::xToFreq (float xPos)
{
    float width = (float) getWidth();
    return lowFreq * pow ((highFreq / lowFreq), ((xPos - 2.5f) / (width - 5.0f)));
}

float FilterGraph::freqToX (float freq)
{
    float width = (float) getWidth();
    return (width - 5) * (log (freq / lowFreq) / log (highFreq / lowFreq)) + 2.5f;
}

void FilterGraph::setTraceColour (Colour newColour)
{
    traceColour = newColour;
    repaint();
}

void FilterGraph::mouseMove (const MouseEvent &)
{
	if(drawType == Icon)
	{
		return;
	}

    Point <int> mousePos =  getMouseXYRelative();
    int xPos = mousePos.getX();
    float freq = xToFreq ((float)xPos);
    
    if (traceType == Magnitude)
    {
        float magnitude = (float) (filterVector [0]->getResponse (freq).magnitudeValue);
    
        for (int i = 1; i < numFilters; i++)
        {
            magnitude *= (float) (filterVector [i]->getResponse (freq).magnitudeValue);
        }
    
        magnitude = 20 * log10 (magnitude);
    
        setTooltip (String (freq, 1) + "Hz, " + String (magnitude, 1) + "dB");
    }
    
    if (traceType == Phase)
    {
        float phase = (float) (filterVector [0]->getResponse (freq).phaseValue);
    
        for (int i = 1; i < numFilters; i++)
        {
            phase += (float) (filterVector [i]->getResponse (freq).phaseValue);
        }
        
        phase /= float_Pi;
    
        setTooltip (String (freq, 1) + "Hz, " + String (phase, 2) + String (CharPointer_UTF8 ("\xcf\x80")) + "rad");
    }
}

