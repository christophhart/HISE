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


#ifndef __FILTERGRAPH_H_3CCF0ED1__
#define __FILTERGRAPH_H_3CCF0ED1__

namespace hise { using namespace juce;


class FilterGraph    : public ComponentWithMiddleMouseDrag,
                       public SettableTooltipClient,
					   public SafeChangeListener,
					   public ComplexDataUIUpdaterBase::EventListener,
					   public ComplexDataUIBase::EditorBase
{
public:

	enum ColourIds
	{
		bgColour = 1024,
		lineColour,
		fillColour,
		gridColour,
		textColour,
		numColourIds
	};

	enum DrawType
	{
		Fill = 0,
		Line,
		Icon
	};

    FilterGraph (int numFilters=0, int type=Line);
    ~FilterGraph();

	struct LookAndFeelMethods
	{
		virtual ~LookAndFeelMethods() {};
		virtual void drawFilterBackground(Graphics &g, FilterGraph& fg);
		virtual void drawFilterPath(Graphics& g, FilterGraph& fg, const Path& p);
		virtual void drawFilterGridLines(Graphics &g, FilterGraph& fg, const Path& gridPath);
	};

	struct DefaultLookAndFeel : public PopupLookAndFeel,
		public LookAndFeelMethods
	{

	};

    enum TraceType
    {
        Magnitude,
        Phase
    };
    
    void paint (Graphics&);
    void resized();
    
	void addFilter(FilterType filterType);

	void addEqBand(BandType eqType);

	void changeListenerCallback(SafeChangeBroadcaster *b);
	
	int getNumFilterBands() const { return numFilters; }

	void onComplexDataEvent(ComplexDataUIUpdaterBase::EventType e, var newValue) override;

	void clearBands()
	{
		filterVector.clear();
		numFilters = 0;
		repaint();
	}

	/** Activates the given filter band. */
	void enableBand(int index, bool shouldBeEnabled)
	{
		if(auto p = filterVector[index])
			p->setEnabled(shouldBeEnabled);

		repaint();
	}

	void createGridPath();

    void setNumHorizontalLines (int newValue);
    void setFreqRange (float newLowFreq, float newHighFreq);
	void setGainRange(float maxGain);

	void setBypassed(bool shouldBeBypassed);;

    void setFilterGain (int filterNum, double gain);
    void setFilter (int filterNum, double sampleRate, double frequency, FilterType filterType);
    void setEqBand (int filterNum, double sampleRate, double frequency, double Q, float gain, BandType eqType);
    void setCustom (int filterNum, double sampleRate, std::vector <double> numCoeffs, std::vector <double> denCoeffs);
    
	void setCoefficients(int filterNum, double sampleRate, std::pair<IIRCoefficients, int> newCoefficients);


    float xToFreq (float xPos);
    float freqToX (float freq);

	float yToGain(float yPos, float maxGain) const;
	float gainToY(float gain, float maxGain) const;

    void setTraceColour (Colour newColour);
    
	Path getFilterPath() const
	{
		return tracePath;
	}

	void setUseFlatDesign(bool shouldUseFlatDesign)
	{
		useFlatDesign = shouldUseFlatDesign;
	}

    float maxdB, maxPhas;
    Colour traceColour;
    TraceType traceType;
    
	class Panel;

	void setComplexDataUIBase(ComplexDataUIBase* newData) override
	{
		if (filterData != nullptr)
			filterData->getUpdater().removeEventListener(this);

		clearBands();

        if ((filterData = dynamic_cast<FilterDataObject*>(newData)))
		{
			numFilters = filterData->getNumCoefficients();

			for (int i = 0; i < numFilters; i++)
			{
				filterVector.add(new FilterInfo());
				filterVector[i]->setCoefficients(0, filterData->getSamplerate(), filterData->getCoefficients(i));
			}
			
			filterData->getUpdater().addEventListener(this);
		}

		repaint();
	}

	void setShowLines(bool shouldShowLines)
	{
		showLines = shouldShowLines;
	}

private:

	

	

	void refreshFilterPath();

	void clearFilterPath()
	{
		float width = (float) getWidth();
		float height = (float) getHeight();

		//const DrawType drawType = Line;

		if(drawType == Line)
		{
			tracePath.startNewSubPath(-3.0f, height / 2.0f);
			tracePath.lineTo(width + 3.0f, height / 2.0f);

		}
		else if( drawType == Icon)
		{
			tracePath.startNewSubPath (0, height);
			tracePath.lineTo (0, (height / 2));
			tracePath.lineTo (width, (height / 2));
			tracePath.lineTo (width, height);
			tracePath.closeSubPath();
		}
		else
		{
			tracePath.startNewSubPath (-1, height+1);
			tracePath.lineTo (-1, (height / 2));
			tracePath.lineTo (width+1, (height / 2));
			tracePath.lineTo (width+1, height+1);
			tracePath.closeSubPath();
		}
	}

    int numHorizontalLines;
    float lowFreq, highFreq;   
    double fs;
    
	DrawType drawType;

    void mouseMove (const MouseEvent &event);
    
	FilterDataObject::Ptr filterData;
	OwnedArray<FilterInfo> filterVector;

    Path gridPath, tracePath;
	int numFilters = 0;
	bool bypassed = false;
	bool showLines = true;
	bool useFlatDesign = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilterGraph)
};



} // namespace hise

#endif  // __FILTERGRAPH_H_3CCF0ED1__
