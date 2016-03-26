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




class FrontendProcessorEditor: public AudioProcessorEditor,
							   public Timer
{
public:

	FrontendProcessorEditor(FrontendProcessor *fp):
		AudioProcessorEditor(fp)
	{
		
		addAndMakeVisible(interfaceComponent = new ScriptContentContainer(fp->getMainSynthChain(), nullptr));
		interfaceComponent->checkInterfaces();

		

		interfaceComponent->setCurrentContent(0, dontSendNotification);
		interfaceComponent->refreshContentBounds();
		interfaceComponent->setIsFrontendContainer(true);
		interfaceComponent->setBounds(0, 0, interfaceComponent->getContentWidth(), interfaceComponent->getContentHeight());

#if INCLUDE_BAR

		int barHeight = 24;
		addAndMakeVisible(mainBar = new FrontendBar(fp));
		mainBar->setBounds(0, 0, interfaceComponent->getWidth(), barHeight);
		mainBar->startTimer(50);

#else

		int barHeight = 0;

#endif

		interfaceComponent->setTopLeftPosition(0, barHeight);

		const int lowKey = 41;

		addAndMakeVisible(keyboard = new CustomKeyboard(fp->keyboardState));

		keyboard->setAvailableRange(lowKey, 127);

		keyboard->setBounds(0, interfaceComponent->getBottom(), interfaceComponent->getWidth(), 72);
		

		if(!fp->samplesCorrectlyLoaded || !fp->keyFileCorrectlyLoaded)
		{
			interfaceComponent->setVisible(false);
		}

		setSize(interfaceComponent->getContentWidth(), barHeight + interfaceComponent->getHeight() + 72);

		startTimer(4125);

		addAndMakeVisible(aboutPage = new AboutPage());

		aboutPage->setVisible(false);

		aboutPage->setBoundsInset(BorderSize<int>(80));

		aboutPage->setUserEmail(fp->unlocker.getUserEmail());	

	};

	void timerCallback()
	{
		dynamic_cast<FrontendProcessor*>(getAudioProcessor())->checkKey();
	}

	

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


private:

	friend class FrontendBar;

	ScopedPointer<ScriptContentContainer> interfaceComponent;

	ScopedPointer<FrontendBar> mainBar;

	ScopedPointer<CustomKeyboard> keyboard;

	ScopedPointer<AboutPage> aboutPage;

};



#endif  // FRONTENDPROCESSOREDITOR_H_INCLUDED
