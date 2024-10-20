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
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*	Some parts of this file are written by Sean Enderby.
*
*   ===========================================================================
*/

namespace hise { using namespace juce;


//==============================================================================
FilterGraph::FilterGraph (int numFiltersInit, int drawType_):
	drawType((DrawType)drawType_),
	numFilters(numFiltersInit)
{
	setSpecialLookAndFeel(new DefaultLookAndFeel(), true);

	setOpaque(true);

	setColour(ColourIds::bgColour, Colour(0xFF333333));
	setColour(ColourIds::lineColour, Colours::white);
	setColour(ColourIds::fillColour, Colours::white.withAlpha(0.5f));
	setColour(ColourIds::gridColour, Colours::white.withAlpha(0.2f));
	setColour(ColourIds::textColour, Colours::white);

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
	
	if(numFilters == 0 || bypassed)
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
        
		if (traceMagnitude == 0)
			return;

		jassert(traceMagnitude > 0.0f);

		for (int i = 1; i < numActiveFilters; i++)
		{
			traceMagnitude *= (float) (activeFilters [i]->getResponse (lowFreq).magnitudeValue);
		}
		traceMagnitude = 20 * log10 (traceMagnitude);
		
        tracePath.startNewSubPath(-3.0f, -3.0f);
        tracePath.startNewSubPath((float)width+6.0f, (float)height+6.0f);
        
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
        
            auto yValue = jlimit<float>(0.0f, (float)height, (height / 2) - (traceMagnitude * scaleFactor));
            
			tracePath.lineTo (xPos, yValue);
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
	

#if 0
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
#endif
}

void FilterGraph::onComplexDataEvent(ComplexDataUIUpdaterBase::EventType e, var newValue)
{
	if (e == ComplexDataUIUpdaterBase::EventType::DisplayIndex)
	{
		if (filterData->getNumCoefficients() != numFilters)
		{
			clearBands();

			for (int i = 0; i < filterData->getNumCoefficients(); i++)
				filterVector.add(new FilterInfo());

			this->numFilters = filterVector.size();
		}

		int numCoefficients = filterData->getNumCoefficients();

		IIRCoefficients empty;

		for (int i = 0; i < numCoefficients; i++)
		{
			auto co = filterData->getCoefficients(i);
			auto isEmpty = memcmp(&empty, &co, sizeof(IIRCoefficients)) == 0;
			auto shouldBeEnabled = !isEmpty;

			if (shouldBeEnabled != filterVector[i]->isEnabled())
				filterVector[i]->setEnabled(shouldBeEnabled);

			filterVector[i]->setCoefficients(0, filterData->getSamplerate(), co);
		}

		fs = filterData->getSamplerate();
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
		auto laf = getSpecialLookAndFeel<LookAndFeelMethods>();
		
		jassert(laf != nullptr);

		laf->drawFilterBackground(g, *this);

		if (showLines)
		{
			createGridPath();
			laf->drawFilterGridLines(g, *this, gridPath);
		}
		
		refreshFilterPath();

		laf->drawFilterPath(g, *this, tracePath);
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

void FilterGraph::createGridPath()
{
	float width = (float)getWidth();
	float height = (float)getHeight();



	gridPath.clear();
	for (int lineNum = 1; lineNum < numHorizontalLines + 1; lineNum++)
	{
		float yPos = lineNum * (height - 5) / (numHorizontalLines + 1) + 2.5f;
		gridPath.startNewSubPath(0, yPos);
		gridPath.lineTo(width, yPos);
	}

	float order = (float)(pow(10, floor(log10(lowFreq))));
	float rounded = order * (floor(lowFreq / order) + 1);
	for (float freq = rounded; freq < highFreq; freq += (float)(pow(10, floor(log10(freq)))))
	{
		float xPos = freqToX(freq);
		gridPath.startNewSubPath(xPos, 2.5f);
		gridPath.lineTo(xPos, height - 2.5f);
	}
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


void FilterGraph::setGainRange(float maxGain)
{
	maxdB = maxGain-6.0;
	setNumHorizontalLines(roundToInt(maxdB / 3)+1);
}

void FilterGraph::setBypassed(bool shouldBeBypassed)
{
	if (shouldBeBypassed != bypassed)
	{
		bypassed = shouldBeBypassed; repaint();
	}
}

void FilterGraph::setFilterGain (int filterNum, double gain)
{
	if (filterNum < filterVector.size())
	{
		filterVector[filterNum]->setGain(gain);
		repaint();
	}
    
}

void FilterGraph::setFilter (int filterNum, double sampleRate, double frequency, FilterType filterType)
{   
	if (filterNum < filterVector.size())
	{
		filterVector[filterNum]->setSampleRate(sampleRate);
		filterVector[filterNum]->setFilter(frequency, filterType);

		fs = sampleRate;
		repaint();
	}
    
}

void FilterGraph::setEqBand (int filterNum, double sampleRate, double frequency, double Q, float gain, BandType eqType)
{   
	if (filterNum < filterVector.size())
	{
		filterVector[filterNum]->setSampleRate(sampleRate);
		filterVector[filterNum]->setEqBand(frequency, Q, gain, eqType);

		fs = sampleRate;
		repaint();
	}
    
}

void FilterGraph::setCustom (int filterNum, double sampleRate, std::vector <double> numCoeffs, std::vector <double> denCoeffs)
{
	if (filterNum < filterVector.size())
	{
		filterVector[filterNum]->setSampleRate(sampleRate);
		filterVector[filterNum]->setCustom(numCoeffs, denCoeffs);

		fs = sampleRate;
		repaint();
	}
}

void FilterGraph::setCoefficients(int filterNum, double sampleRate, FilterDataObject::CoefficientData newCoefficients)
{
	if (filterNum < filterVector.size())
	{
		auto old = filterVector[filterNum]->getCoefficients();

		if (memcmp(&old.coefficients, newCoefficients.first.coefficients, sizeof(IIRCoefficients::coefficients)) != 0)
		{
			filterVector[filterNum]->setSampleRate(sampleRate);
			filterVector[filterNum]->setCoefficients(filterNum, sampleRate, newCoefficients);

			fs = sampleRate;
			repaint();
		}
	}
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

float FilterGraph::yToGain(float yPos, float maxGain) const
{
	if (getHeight() == 0)
		return 0.0f;

	auto normalised = jlimit(0.0f, 1.0f, yPos / (float)getHeight());

	return jmap(normalised, maxGain, -1.0f * maxGain);
}

float FilterGraph::gainToY(float gain, float maxGain) const
{
	auto normalised = (-1.0f * gain) / (maxGain * 2.0f) + 0.5f;

	normalised = jlimit(0.0f, 1.0f, normalised);

	return normalised * (float)getHeight();
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
    
	if (filterVector.size() == 0)
		return;

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


void FilterGraph::LookAndFeelMethods::drawFilterBackground(Graphics &g, FilterGraph& fg)
{
	if (fg.useFlatDesign)
	{
		g.fillAll(fg.findColour(ColourIds::bgColour));
	}
	else
	{
		ColourGradient grad = ColourGradient(Colour(0xFF444444), 0.0f, 0.0f,
			Colour(0xFF222222), 0.0f, (float)fg.getHeight(), false);

		g.setGradientFill(grad);
		g.fillAll();
		g.setColour(Colours::lightgrey.withAlpha(0.4f));
		g.drawRect(fg.getLocalBounds(), 1);
	}
}

void FilterGraph::LookAndFeelMethods::drawFilterPath(Graphics& g, FilterGraph& fg, const Path& p)
{
	if (fg.useFlatDesign)
	{
		g.setColour(fg.findColour(ColourIds::fillColour));
		g.fillPath(p);
		g.setColour(fg.findColour(ColourIds::lineColour));
		g.strokePath(p, PathStrokeType(1.0f));
	}
	else
	{
		GlobalHiseLookAndFeel::fillPathHiStyle(g, p, fg.getWidth(), fg.getHeight());
	}
}

void FilterGraph::LookAndFeelMethods::drawFilterGridLines(Graphics &g, FilterGraph& fg, const Path& gridPath)
{
	g.setColour(Colour(0x22ffffff));
	String axisLabel;
	axisLabel = String(fg.maxdB, 1) + "dB";

	auto b = fg.getLocalBounds().toFloat().removeFromLeft(300).reduced(4.0f);

	g.setFont(GLOBAL_FONT());
	g.drawText(axisLabel, b.removeFromTop(18.0f), Justification::left, false);
	g.drawText(String("-") + axisLabel, b.removeFromBottom(18.0f), Justification::left, false);

	g.setColour(Colour(0x22ffffff));
	g.strokePath(gridPath, PathStrokeType(0.5f));
}

} // namespace hise
