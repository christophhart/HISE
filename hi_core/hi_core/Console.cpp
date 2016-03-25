/*
  ==============================================================================

    Console.cpp
    Created: 1 Nov 2013 3:36:23pm
    Author:  Chrisboy

  ==============================================================================
*/


Console::Console(BaseDebugArea *area) :
		AutoPopupDebugComponent(area),
		line(0),
		usage(0),
		voiceAmount(0),
		overflowProtection(false),
		hostTempo(120),
		popupMode(false)
{
	setName("Console");

	addAndMakeVisible (textConsole = new TextEditor ("Text Console"));
	textConsole->setMultiLine (true);
	textConsole->setReturnKeyStartsNewLine (false);
	textConsole->setReadOnly (true);
	textConsole->setScrollbarsShown (true);
	textConsole->setCaretVisible (true);
	textConsole->setPopupMenuEnabled (true);

	textConsole->setFont(GLOBAL_MONOSPACE_FONT());
	
	textConsole->setPopupMenuEnabled(false);
    
	
	
	textConsole->setColour (TextEditor::shadowColourId, Colours::black);
	textConsole->setColour(TextEditor::backgroundColourId, Colour(DEBUG_AREA_BACKGROUND_COLOUR));



	addAndMakeVisible (clearButton = new TextButton ("Clear"));
	clearButton->setButtonText ("Clear");
	clearButton->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight);
	clearButton->addListener (this);
	clearButton->setColour (TextButton::buttonColourId, Colour (0xff7e7e7e));
	clearButton->setColour (TextButton::textColourOnId, Colours::white);
	clearButton->setColour (TextButton::textColourOffId, Colour (0xfffafafa));

	addAndMakeVisible (cpuSlider = new Slider ("new slider"));
	cpuSlider->setRange (0, 100, 1);
	cpuSlider->setSliderStyle (Slider::LinearBar);
	cpuSlider->setTextBoxStyle (Slider::TextBoxLeft, false, 80, 20);
	cpuSlider->setTextValueSuffix(" %");
	cpuSlider->setEnabled(false);
	cpuSlider->setColour(Slider::ColourIds::backgroundColourId, Colours::black.withAlpha(0.1f));
	cpuSlider->setColour(Slider::ColourIds::thumbColourId, Colours::darkred.withAlpha(0.5f));

	addAndMakeVisible(voiceLabel = new Label());
	voiceLabel->setFont (GLOBAL_FONT());
	voiceLabel->setJustificationType (Justification::centredLeft);
    voiceLabel->setEditable (false, false, false);
    voiceLabel->setColour (Label::textColourId, Colour (0x9c000000));
    voiceLabel->setColour (TextEditor::textColourId, Colours::black);
    voiceLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));
	voiceLabel->setText("Total voice amount: ", dontSendNotification);


}

Console::~Console()
{
	stopTimer();

	textConsole = nullptr;
	clearButton = nullptr;
	
	masterReference.clear();

};


void Console::resized()
{
	textConsole->setBounds(getLocalBounds());
}

void Console::buttonClicked (Button* b)
{
	ScopedLock sl(lock);
	{
		tempString.clear();
		line = 0;
		processorLines.clear();
	}

	if (b == clearButton)
	{
		textConsole->clear();
	}
}

void Console::logMessage(const String &t, WarningLevel warningLevel, const Processor *p, Colour c)
{

	if(overflowProtection) return;

	else
	{
		ScopedLock sl(lock);

		if(unprintedMessages.size() > 10)
		{
			unprintedMessages.add(ConsoleMessage("Console Overflow", Error, p, c));
			overflowProtection = true;
			return;
		}
		else
		{
			unprintedMessages.add(ConsoleMessage(t, warningLevel, p, c));
		}
	}

	if (MessageManager::getInstance()->isThisTheMessageThread())
	{
		handleAsyncUpdate();
	}
	else
	{
		triggerAsyncUpdate();
	}
};


void Console::handleAsyncUpdate()
{
	Array<ConsoleMessage> messagesForThisTime;
	messagesForThisTime.ensureStorageAllocated(unprintedMessages.size());

	if(unprintedMessages.size() != 0)
	{
		ScopedLock sl(lock);
		messagesForThisTime.addArray(unprintedMessages);
		unprintedMessages.clearQuick();
	}

	for(int i = 0; i < messagesForThisTime.size(); i++)
	{
		
		textConsole->moveCaretToEnd(false);

		const Processor *p = messagesForThisTime[i].processor;
//		jassert(p != nullptr);

		if(p != nullptr)
		{
			Colour processorColour = messagesForThisTime[i].colour.withSaturation(0.8f).withAlpha(1.0f); 
			textConsole->setColour(TextEditor::ColourIds::textColourId, processorColour);
			
			textConsole->insertTextAtCaret(p->getId() + ": ");
		}

		Colour messageColour = (messagesForThisTime[i].warningLevel == Error) ? Colours::red : Colours::black;

		textConsole->setColour(TextEditor::ColourIds::textColourId, messageColour);
		textConsole->moveCaretToEnd(false);
		textConsole->insertTextAtCaret(messagesForThisTime[i].message + "\n");
	}

	
	overflowProtection = false;

	return;
};