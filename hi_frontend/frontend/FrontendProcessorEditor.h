/*
  ==============================================================================

    FrontendProcessorEditor.h
    Created: 16 Oct 2014 9:33:11pm
    Author:  Chrisboy

  ==============================================================================
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
