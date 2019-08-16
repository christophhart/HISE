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

/** a component that plots a collection of filters.
@ingroup floating_tile_objects.

Just connect it to a PolyphonicFilterEffect or a CurveEQ and it will automatically update
the filter graph.
*/
class FilterGraph::Panel : public PanelWithProcessorConnection,
						   public SafeChangeListener,
						   public Timer
{
public:

	enum SpecialPanelIds
	{
		showLines,
		numSpecialPanelIds
	};

	SET_PANEL_NAME("FilterDisplay");

	Panel(FloatingTile* parent) :
		PanelWithProcessorConnection(parent)
	{
		setDefaultPanelColour(FloatingTileContent::PanelColourId::bgColour, Colour(0xFF333333));
		setDefaultPanelColour(FloatingTileContent::PanelColourId::itemColour1, Colours::white);
		setDefaultPanelColour(FloatingTileContent::PanelColourId::itemColour2, Colours::white.withAlpha(0.5f));
		setDefaultPanelColour(FloatingTileContent::PanelColourId::itemColour3, Colours::white.withAlpha(0.2f));
		setDefaultPanelColour(FloatingTileContent::PanelColourId::textColour, Colours::white);
	};

	~Panel()
	{
		if (auto p = getProcessor())
		{
			p->removeChangeListener(this);
		}
	}

	juce::Identifier getProcessorTypeId() const
	{
		return PolyFilterEffect::getClassType();
	}

	

	void changeListenerCallback(SafeChangeBroadcaster* /*b*/) override
	{
		updateCoefficients();
	}

	void timerCallback() override
	{
		if (auto filter = dynamic_cast<FilterEffect*>(getProcessor()))
		{
			if (auto filterGraph = getContent<FilterGraph>())
			{
				filterGraph->setBypassed(getProcessor()->isBypassed());

				IIRCoefficients c = filter->getCurrentCoefficients();

				if (!sameCoefficients(c, currentCoefficients))
				{
					currentCoefficients = c;

					filterGraph->setCoefficients(0, getProcessor()->getSampleRate(), dynamic_cast<FilterEffect*>(getProcessor())->getCurrentCoefficients());
				}
			}
		}
	}

	

	Component* createContentComponent(int /*index*/) override
	{
		if (auto p = getProcessor())
		{
			p->addChangeListener(this);

			auto c = new FilterGraph(1);

			c->useFlatDesign = true;
			c->showLines = false;
			

			c->setColour(ColourIds::bgColour, findPanelColour(FloatingTileContent::PanelColourId::bgColour));
			c->setColour(ColourIds::lineColour, findPanelColour(FloatingTileContent::PanelColourId::itemColour1));
			c->setColour(ColourIds::fillColour, findPanelColour(FloatingTileContent::PanelColourId::itemColour2));
			c->setColour(ColourIds::gridColour, findPanelColour(FloatingTileContent::PanelColourId::itemColour3));
			c->setColour(ColourIds::textColour, findPanelColour(FloatingTileContent::PanelColourId::textColour));

			c->setOpaque(c->findColour(bgColour).isOpaque());

			if (dynamic_cast<FilterEffect*>(p) != nullptr)
			{
				c->addFilter(FilterType::LowPass);
				startTimer(30);
			}
			else if (auto eq = dynamic_cast<CurveEq*>(p))
			{
				stopTimer();
				updateEq(eq, c);
			}

			return c;
		}

		return nullptr;
	}

	void fillModuleList(StringArray& moduleList)
	{
		fillModuleListWithType<CurveEq>(moduleList);
		fillModuleListWithType<FilterEffect>(moduleList);
	}

private:

	bool sameCoefficients(IIRCoefficients c1, IIRCoefficients c2)
	{
		for (int i = 0; i < 5; i++)
		{
			if (c1.coefficients[i] != c2.coefficients[i]) return false;
		}

		return true;
	};

	void updateCoefficients()
	{
		if (auto filterGraph = getContent<FilterGraph>())
		{
			if (auto c = dynamic_cast<CurveEq*>(getProcessor()))
			{
				for (int i = 0; i < c->getNumFilterBands(); i++)
				{
					IIRCoefficients ic = c->getCoefficients(i);

					filterGraph->enableBand(i, c->getFilterBand(i)->isEnabled());
					filterGraph->setCoefficients(i, getProcessor()->getSampleRate(), ic);
				}
			}
		}
	}

	void updateEq(CurveEq* eq, FilterGraph* c) 
	{
		c->clearBands();

		for (int i = 0; i < eq->getNumFilterBands(); i++)
		{
			addFilter(c, i, (int)eq->getFilterBand(i)->getFilterType());
		}

		int numFilterBands = eq->getNumFilterBands();

		if (numFilterBands == 0)
		{
			c->repaint();
		}
	}

	void addFilter(FilterGraph* filterGraph, int filterIndex, int filterType)
	{
		if (auto c = dynamic_cast<CurveEq*>(getProcessor()))
		{
			switch (filterType)
			{
			case CurveEq::LowPass:		filterGraph->addFilter(FilterType::LowPass); break;
			case CurveEq::HighPass:		filterGraph->addFilter(FilterType::HighPass); break;
			case CurveEq::LowShelf:		filterGraph->addEqBand(BandType::LowShelf); break;
			case CurveEq::HighShelf:	filterGraph->addEqBand(BandType::HighShelf); break;
			case CurveEq::Peak:			filterGraph->addEqBand(BandType::Peak); break;
			}

			filterGraph->setCoefficients(filterIndex, c->getSampleRate(), c->getCoefficients(filterIndex));
		}
	}

	IIRCoefficients currentCoefficients;

};

//==============================================================================
FilterGraph::FilterGraph (int numFiltersInit, int drawType_)
    :  filterVector (),
	drawType((DrawType)drawType_)
{
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

		if(showLines)
			paintGridLines(g);
		
		refreshFilterPath();

		if (useFlatDesign)
		{
			g.setColour(findColour(ColourIds::fillColour));
			g.fillPath(tracePath);
			g.setColour(findColour(ColourIds::lineColour));
			g.strokePath(tracePath, PathStrokeType(1.0f));
		}
		else
		{
			GlobalHiseLookAndFeel::fillPathHiStyle(g, tracePath, getWidth(), getHeight());
		}

		
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

void FilterGraph::setCoefficients(int filterNum, double sampleRate, IIRCoefficients newCoefficients)
{
	if (filterNum < filterVector.size())
	{
		filterVector[filterNum]->setSampleRate(sampleRate);
		filterVector[filterNum]->setCoefficients(filterNum, sampleRate, newCoefficients);

		fs = sampleRate;
		repaint();
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

} // namespace hise
