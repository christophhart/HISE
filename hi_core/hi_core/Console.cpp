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
    textConsole->addMouseListener(this, true);


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

void Console::mouseDown(const MouseEvent &e)
{
    if(e.mods.isRightButtonDown())
    {
        PopupLookAndFeel plaf;
        PopupMenu m;
        
        m.setLookAndFeel(&plaf);
        
        m.addItem(1, "Clear Console");
        m.addItem(2, "Scroll down");
        
        const int result = m.show();
        
        if(result == 1) clear();
            else if (result == 2)
            {
                textConsole->moveCaretToEnd(false);
            }
    }
    else if (e.mods.isAltDown())
    {
        int textPos = textConsole->getTextIndexAt(e.getMouseDownX(), e.getMouseDownY());
        
        String content = textConsole->getText();
        
        int last = content.indexOfChar(textPos, ':');
        
        int first = content.substring(0, textPos).lastIndexOfChar('\n');
        
        String name = content.substring(first, last).upToFirstOccurrenceOf(":", false, false).removeCharacters("\r\n\t");
        
#if USE_BACKEND
        
        if(name.isNotEmpty())
        {
            BackendProcessorEditor *editor = findParentComponentOfClass<BackendProcessorEditor>();
            
            Processor *p = ProcessorHelpers::getFirstProcessorWithName(editor->getMainSynthChain(), name);
            
            if(p != nullptr)
            {
                editor->setRootProcessorWithUndo(p);
            }
        }

#endif
        
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

		Colour messageColour = (messagesForThisTime[i].warningLevel == Error) ? Colours::red.withBrightness(0.4f) : Colours::black;

		textConsole->setColour(TextEditor::ColourIds::textColourId, messageColour);
		textConsole->moveCaretToEnd(false);
		textConsole->insertTextAtCaret(messagesForThisTime[i].message + "\n");
	}

	
	overflowProtection = false;

	return;
};