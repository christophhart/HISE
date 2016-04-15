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


class DeactiveOverlay : public Component,
						public ButtonListener
{
public:

	DeactiveOverlay() :
		currentState(0)
	{
		addAndMakeVisible(resolveLicenceButton = new TextButton("Find Licence File"));
		addAndMakeVisible(resolveSamplesButton = new TextButton("Choose Sample Folder"));

		resolveLicenceButton->setLookAndFeel(&alaf);
		resolveSamplesButton->setLookAndFeel(&alaf);

		resolveLicenceButton->addListener(this);
		resolveSamplesButton->addListener(this);
	};

	void buttonClicked(Button *b);

	enum State
	{
		AppDataDirectoryNotFound,
		SamplesNotFound,
		LicenceNotFound,
		numReasons
	};

	void paint(Graphics &g)
	{
		g.setColour(Colours::black.withAlpha(0.8f));
		g.fillAll();
	}

	void setState(State s, bool value)
	{
		currentState.setBit(s, value);

		setVisible(currentState != 0);
		resized();
	}

	void resized()
	{
		if (currentState[LicenceNotFound])
		{
			resolveLicenceButton->centreWithSize(200, 32);
		}
		else if (currentState[SamplesNotFound])
		{
			resolveSamplesButton->centreWithSize(200, 32);
		}
	}
	

private:

	AlertWindowLookAndFeel alaf;

	ScopedPointer<TextButton> resolveLicenceButton;
	ScopedPointer<TextButton> resolveSamplesButton;

	BigInteger currentState;
	
};


class FrontendProcessorEditor: public AudioProcessorEditor,
							   public Timer,
							   public ComponentWithKeyboard
{
public:

	FrontendProcessorEditor(FrontendProcessor *fp);;

	void timerCallback()
	{
#if USE_COPY_PROTECTION
		if (!dynamic_cast<FrontendProcessor*>(getAudioProcessor())->unlocker.isUnlocked())
		{
			getAudioProcessor()->suspendProcessing(true);
		}
#endif
	}

	KeyboardFocusTraverser *createFocusTraverser() override { return new MidiKeyboardFocusTraverser(); };

	void paint(Graphics &g) override
	{
		g.fillAll(Colours::black);
	};

	void resized() override;

	void resetInterface()
	{
		//interfaceComponent->checkInterfaces();
	}

	Component *getKeyboard() const override { return keyboard; }

private:

	friend class FrontendBar;

	ScopedPointer<ScriptContentContainer> interfaceComponent;

	ScopedPointer<FrontendBar> mainBar;

	ScopedPointer<CustomKeyboard> keyboard;

	ScopedPointer<AboutPage> aboutPage;

	ScopedPointer<DeactiveOverlay> deactiveOverlay;

};



#endif  // FRONTENDPROCESSOREDITOR_H_INCLUDED
