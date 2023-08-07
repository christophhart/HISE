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
*   ===========================================================================
*/

#ifndef PLOTTER_H_INCLUDED
#define PLOTTER_H_INCLUDED

namespace hise { using namespace juce;

class ProcessorEditorHeader;
class Modulator;
class ScriptContentComponent;



//==============================================================================
/** A plotter component that displays the Modulator value
*	@ingroup hise_ui
*
*	You can add values with addValue(). This should be periodic 
*	(either within the audio callback or with a designated timer in your Editor, as
*	the plotter only writes something if new data is added.
*/
class Plotter    : public Component,
				   public SettableTooltipClient,
				   public Timer
{
public:

	enum Mode
	{
		GainMode,
		PitchMode,
		PanMode,
		numModes
	};

	Plotter();
    ~Plotter();

	enum ColourIds
	{
		backgroundColour = 0x100,
		outlineColour = 0x10,
		pathColour = 0x001,
		pathColour2,
		textColour
	};


    void paint (Graphics&) override;
    void resized() override;

	void pushLockFree(const float* buffer, int startSample, int numSamples);

	void timerCallback() override;

	void mouseDown(const MouseEvent& m) override;

	void mouseMove(const MouseEvent& m) override;

	void mouseExit(const MouseEvent& m) override;

	void setMode(Mode m);

	void setFont(const Font& f);


	void setYConverter(const Table::ValueTextConverter& newYConverter);

private:

	Font font;

	void rebuildPath();

	Path drawPath;

	Point<int> popupPosition;
	bool stickPopup = false;

	Table::ValueTextConverter yConverter;

	Mode currentMode;

	void popLockFree(float* destination, int numSamples);


	void addValues(const float* buffer, int startSample, int numSamples);

	AbstractFifo abstractFifo;
	float tempBuffer[8192];

	bool active = true;

	AudioSampleBuffer displayBuffer;
	int position = 0;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Plotter)
};


} // namespace hise

#endif  // PLOTTER_H_INCLUDED

