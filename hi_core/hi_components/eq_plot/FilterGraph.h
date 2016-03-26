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


#ifndef __FILTERGRAPH_H_3CCF0ED1__
#define __FILTERGRAPH_H_3CCF0ED1__


//==============================================================================
class FilterGraph    : public Component,
                       public SettableTooltipClient,
					   public SafeChangeListener
{
public:

	enum DrawType
	{
		Fill = 0,
		Line,
		Icon
	};

    FilterGraph (int numFilters, int type=Line);
    ~FilterGraph();

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
	

	void clearBands()
	{
		numFilters = 0;
		filterVector.clear();
	}

	void enableBand(int index, bool shouldBeEnabled)
	{
		if(index < filterVector.size())
		{
			filterVector[index]->setEnabled(shouldBeEnabled);
			repaint();
		}
	}

    void setNumHorizontalLines (int newValue);
    void setFreqRange (float newLowFreq, float newHighFreq);
    
    void setFilterGain (int filterNum, double gain);
    void setFilter (int filterNum, double sampleRate, double frequency, FilterType filterType);
    void setEqBand (int filterNum, double sampleRate, double frequency, double Q, float gain, BandType eqType);
    void setCustom (int filterNum, double sampleRate, std::vector <double> numCoeffs, std::vector <double> denCoeffs);
    
	void setCoefficients(int filterNum, double sampleRate, IIRCoefficients newCoefficients);

    float xToFreq (float xPos);
    float freqToX (float freq);

    void setTraceColour (Colour newColour);
    
	Path getFilterPath() const
	{
		return tracePath;
	}

    float maxdB, maxPhas;
    Colour traceColour;
    TraceType traceType;
    
private:

	void paintBackground(Graphics &g)
	{
		
		ColourGradient grad = ColourGradient(Colour(0xFF444444), 0.0f, 0.0f,
											 Colour(0xFF222222), 0.0f, (float)getHeight(), false);

		g.setGradientFill(grad);

		g.fillAll();

		g.setColour(Colours::lightgrey.withAlpha(0.4f));
		g.drawRect(getLocalBounds(), 1);
    


	}

	void paintGridLines(Graphics &g)
	{
		float width = (float) getWidth();
		float height = (float) getHeight();

		g.setColour (Colour (0x22ffffff));
		String axisLabel;
		axisLabel = String (maxdB, 1) + "dB";
    
		g.setFont (Font ("Arial Rounded MT", 10.0f, Font::plain));
		g.drawText (axisLabel, 6, (int) ((height - 5) / (numHorizontalLines + 1) -9.5f), 45, 12, Justification::left, false);
		g.drawText (String ("-") + axisLabel, 6, (int) (numHorizontalLines * (height - 5) / (numHorizontalLines + 1) + 3.5f), 45, 12, Justification::left, false);
    
		gridPath.clear();
		for (int lineNum = 1; lineNum < numHorizontalLines + 1; lineNum++)
		{
			float yPos = lineNum * (height - 5) / (numHorizontalLines + 1) + 2.5f;
			gridPath.startNewSubPath (0, yPos);
			gridPath.lineTo (width, yPos);
		}
    
		float order = (float)(pow (10, floor (log10 (lowFreq))));
		float rounded = order * (floor(lowFreq / order) + 1);
		for (float freq = rounded; freq < highFreq; freq += (float)(pow (10, floor (log10 (freq)))))
		{
			float xPos = freqToX (freq);
			gridPath.startNewSubPath (xPos, 2.5f);
			gridPath.lineTo (xPos, height - 2.5f);
		}

		g.setColour (Colour (0x22ffffff));   
		g.strokePath (gridPath, PathStrokeType (0.5f));
	}

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
    int numFilters;
    
	DrawType drawType;

    void mouseMove (const MouseEvent &event);
    
    OwnedArray<FilterInfo> filterVector;
    
    Path gridPath, tracePath;
    
	

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilterGraph)
};
#endif  // __FILTERGRAPH_H_3CCF0ED1__
