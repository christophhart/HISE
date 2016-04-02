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
*   ===========================================================================
*/

#ifndef FRONTENDPROCESSOREDITOR_H_INCLUDED
#define FRONTENDPROCESSOREDITOR_H_INCLUDED

#define INCLUDE_BAR 1


class ScriptContentContainer;

class MidiKeyboardFocusTraverser : public KeyboardFocusTraverser
{
	Component *getDefaultComponent(Component *parentComponent) override;
};

class FrontendProcessorEditor: public AudioProcessorEditor,
							   public Timer
{
public:

	FrontendProcessorEditor(FrontendProcessor *fp);;

	void timerCallback()
	{
		dynamic_cast<FrontendProcessor*>(getAudioProcessor())->checkKey();
	}

	KeyboardFocusTraverser *createFocusTraverser() override { return new MidiKeyboardFocusTraverser(); };

	void paint(Graphics &g)
	{
		dynamic_cast<FrontendProcessor*>(getAudioProcessor())->checkKey();

		g.fillAll(Colours::black);

		g.setFont(13.0f);
		g.setColour(Colours::black);
		g.drawText("Samples were not loaded correctly. Plugin is not working!", getLocalBounds(), Justification::centred, false);

	};

	void resetInterface()
	{
		//interfaceComponent->checkInterfaces();
	}

	CustomKeyboard *getKeyboard() { return keyboard; }

private:

	friend class FrontendBar;

	ScopedPointer<ScriptContentContainer> interfaceComponent;

	ScopedPointer<FrontendBar> mainBar;

	ScopedPointer<CustomKeyboard> keyboard;

	ScopedPointer<AboutPage> aboutPage;

};



#endif  // FRONTENDPROCESSOREDITOR_H_INCLUDED
