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

namespace hise { using namespace juce;

//==============================================================================
ProcessorEditorChainBar::ProcessorEditorChainBar (ProcessorEditor *p): 
	ProcessorEditorChildComponent(p),
	insertPosition(-1),
	itemDragging(false)
{
	laf = new ChainBarButtonLookAndFeel();

	if(p->getProcessor()->getNumInternalChains() != 0)
	{
		TextButton *t = new TextButton("Processor Body");
		t->setButtonText("Body");
		t->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight);
		t->addListener (this);
		t->setColour (TextButton::buttonColourId, Colour (0x4c000000));
		t->setColour (TextButton::buttonOnColourId, Colours::white);
		t->setColour (TextButton::textColourOnId, Colour (0xCC000000));
		t->setColour (TextButton::textColourOffId, Colour (0xCCffffff));

		t->setLookAndFeel(laf);

		t->setTooltip("Show / Hide the Processor Body.");

		addAndMakeVisible(t);

		chainButtons.add(t);

		t->setWantsKeyboardFocus(false);
		t->setMouseClickGrabsKeyboardFocus(false);
		
		t->addMouseListener(this, true);


	}

	for (int i = 0; i < p->getProcessor()->getNumInternalChains(); i++)
	{
		Processor *childProcessor = p->getProcessor()->getChildProcessor(i);

		Chain *chain = dynamic_cast<Chain*>(childProcessor);

		childProcessor->addChangeListener(this);
		

		if(chain == nullptr)
		{
			// All Processors returned by getInternalChain() must be Chains!
			jassertfalse;
			return;
		}
		
		TextButton *t = new TextButton(childProcessor->getId());
		t->setButtonText(childProcessor->getId());
		t->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight);
		t->addListener (this);
		t->setColour (TextButton::buttonColourId, Colour (0x884b4b4b));
		t->setColour (TextButton::buttonOnColourId, Colours::white);
		t->setColour (TextButton::textColourOnId, Colour (0xaa000000));
		t->setColour (TextButton::textColourOffId, Colour (0x99ffffff));
		t->setColour(ChainBarButtonLookAndFeel::ColourIds::IconColour, Colour(0xaa000000));
		t->setColour(ChainBarButtonLookAndFeel::ColourIds::IconColourOff, Colour(0x99ffffff));

		t->setLookAndFeel(laf);

		t->setTooltip("Show / Hide the " + childProcessor->getId() + " chain.");

		addAndMakeVisible(t);

		chainButtons.add(t);

		numberTags.add(new NumberTag(2, 14));

		//t->setComponentEffect(numberTags.getLast());

		t->setWantsKeyboardFocus(false);
		t->setMouseClickGrabsKeyboardFocus(false);

		t->addMouseListener(this, true);
	}

	if (chainButtons.size() != 0)
	{
		chainButtons[0]->setConnectedEdges(TextButton::ConnectedEdgeFlags::ConnectedOnRight);
		chainButtons.getLast()->setConnectedEdges(TextButton::ConnectedEdgeFlags::ConnectedOnLeft);

		const bool editorHasBody = dynamic_cast<EmptyProcessorEditorBody*>(getEditor()->getBody()) == nullptr;

		if (editorHasBody)
		{
			chainButtons[0]->setToggleState(getProcessor()->getEditorState(Processor::BodyShown), dontSendNotification);
		}
		else
		{
			chainButtons[0]->setToggleState(false, dontSendNotification);
			chainButtons[0]->setEnabled(false);
		}

		refreshPanel();
	}

    setBufferedToImage(true);
    
	if (ProcessorHelpers::is<ModulatorSynth>(getProcessor()))
	{
		midiButton = chainButtons[1];


        
		startTimer(50);

		
	}

}

ProcessorEditorChainBar::~ProcessorEditorChainBar()
{
	if(getProcessor() != nullptr)
	{
		for(int i = 0; i < getProcessor()->getNumInternalChains(); i++)
		{
			getProcessor()->getChildProcessor(i)->removeChangeListener(this);
		}
	}

	chainButtons.clear();

	

}

int ProcessorEditorChainBar::getActualHeight()
{ 
#if HISE_IOS
		return chainButtons.size() != 0 ? 35 : 0; 
#else
	return chainButtons.size() != 0 ? 22 : 0; 
#endif
}

String ProcessorEditorChainBar::getShortName(const String identifier) const
{
	if(identifier == "GainModulation") return "Gain";
	else if (identifier == "PitchModulation") return "Pitch";
	else if (identifier == "Midi Processor") return "MIDI";
	else return identifier;
}

void ProcessorEditorChainBar::changeListenerCallback(SafeChangeBroadcaster*)
{
	for(int i = 1; i < chainButtons.size(); i++)
	{
		checkActiveChilds(i-1);
	}
}

Chain* ProcessorEditorChainBar::getChainForButton(Component* checkComponent)
{
	int index = chainButtons.indexOf(dynamic_cast<TextButton*>(checkComponent));

	if (index > 0)
	{
	

		return dynamic_cast<Chain*>(getEditor()->getProcessor()->getChildProcessor(index - 1));
	}
	else return nullptr;
}

void ProcessorEditorChainBar::setMidiIconActive(bool shouldBeActive) noexcept
{
	if (midiButton != nullptr)
	{
		if (!shouldBeActive && midiActive)
		{
			midiButton->setColour(ChainBarButtonLookAndFeel::ColourIds::IconColour, Colour(0xaa000000));
			midiButton->setColour(ChainBarButtonLookAndFeel::ColourIds::IconColourOff, Colour(0x99ffffff));
			repaint();
		}
		else if (shouldBeActive && !midiActive)
		{
			midiButton->setColour(ChainBarButtonLookAndFeel::ColourIds::IconColour, Colours::black.withAlpha(0.3f));
			midiButton->setColour(ChainBarButtonLookAndFeel::ColourIds::IconColourOff, Colours::white);

				

			repaint();
			startTimer(150);
		}

		midiActive = shouldBeActive;
	}
		
}

void ProcessorEditorChainBar::setDragInsertPosition(int newInsertPosition)
{
	if (itemDragging)
	{
		insertPosition = newInsertPosition;
		repaint();
	}
	else insertPosition = -1;
}


void ProcessorEditorChainBar::checkActiveChilds(int chainToCheck)
{
	TextButton *b = chainButtons[chainToCheck + 1];

	Chain * c = dynamic_cast<Chain*>((getProcessor())->getChildProcessor(chainToCheck));
			
	const int numProcessors = c->getHandler()->getNumProcessors();

	const bool hasActiveChains =  numProcessors != 0;

	b->setColour(TextButton::ColourIds::buttonColourId, hasActiveChains ? Colour (0x55cccccc) : Colour (0x4c4b4b4b));

	const String name = getShortName((getProcessor())->getChildProcessor(chainToCheck)->getId());

	b->setButtonText(name);

	numProcessorList.set(chainToCheck, numProcessors);
	repaint();
}

bool ProcessorEditorChainBar::isInterestedInDragSource(const SourceDetails & dragSourceDetails)
{
	ModuleBrowser::ModuleItem *dragSource = dynamic_cast<ModuleBrowser::ModuleItem*>(dragSourceDetails.sourceComponent.get());

	if (dragSource == nullptr) return false;


	String name = dragSourceDetails.description.toString().fromLastOccurrenceOf("::", false, false);
	String id = dragSourceDetails.description.toString().upToFirstOccurrenceOf("::", false, false);

	Component *targetComponent = getComponentAt(dragSourceDetails.localPosition);


	bool interested = false;

	if (dynamic_cast<TextButton*>(targetComponent) != nullptr)
	{
		Chain *c = getChainForButton(targetComponent);

		if (c != nullptr)
		{
			interested = c->getFactoryType()->allowType(id);
		}
		else
		{
			return true;
		}

	}
	else
	{
		return true; // hack or it is not enabled...
	}

	dragSource->setDragState(interested ? ModuleBrowser::ModuleItem::Legal : ModuleBrowser::ModuleItem::Illegal);

	

	return interested;
}

void ProcessorEditorChainBar::itemDragEnter(const SourceDetails &/*dragSourceDetails*/)
{
	itemDragging = true;

	
}

void ProcessorEditorChainBar::itemDragExit(const SourceDetails &/*dragSourceDetails*/)
{
	setDragInsertPosition(-1);
	itemDragging = false;
	
}

void ProcessorEditorChainBar::itemDropped(const SourceDetails &dragSourceDetails)
{
	ModuleBrowser::ModuleItem *dragSource = dynamic_cast<ModuleBrowser::ModuleItem*>(dragSourceDetails.sourceComponent.get());


	if (dragSource == nullptr) return;

	String name = dragSourceDetails.description.toString().fromLastOccurrenceOf("::", false, false);
	String id = dragSourceDetails.description.toString().upToFirstOccurrenceOf("::", false, false);

	Component *targetComponent = getComponentAt(dragSourceDetails.localPosition);

	if (dynamic_cast<TextButton*>(targetComponent) != nullptr)
	{
		Chain *c = getChainForButton(targetComponent);

		if (c != nullptr)
		{

			if (c->getFactoryType()->allowType(id))
			{
				int index = chainButtons.indexOf(dynamic_cast<TextButton*>(targetComponent)) - 1;

				ProcessorEditor *editorToUse = getEditor()->getPanel()->getChildEditor(index);

				Processor *newProcessor = MainController::createProcessor(c->getFactoryType(), id, name);

				c->getHandler()->add(newProcessor, nullptr);

				dynamic_cast<Processor*>(c)->setEditorState(Processor::EditorState::Visible, true, sendNotification);

				refreshPanel();
				getEditor()->sendResizedMessage();

				editorToUse->changeListenerCallback(editorToUse->getProcessor());

				editorToUse->childEditorAmountChanged();
			}
		}
	}
		
	dragSource->setDragState(ModuleBrowser::ModuleItem::Inactive);
	
	setDragInsertPosition(-1);
	itemDragging = false;
}

void ProcessorEditorChainBar::itemDragMove(const SourceDetails& dragSourceDetails)
{
	ModuleBrowser::ModuleItem *dragSource = dynamic_cast<ModuleBrowser::ModuleItem*>(dragSourceDetails.sourceComponent.get());

	if (dragSource == nullptr) return;

	canBeDropped = dragSource->getDragState() == ModuleBrowser::ModuleItem::Legal;

	int i = chainButtons.indexOf(dynamic_cast<TextButton*>(getComponentAt(dragSourceDetails.localPosition)));

	setDragInsertPosition(i);
}

void ProcessorEditorChainBar::refreshPanel()
{

	const bool editorHasBody = dynamic_cast<EmptyProcessorEditorBody*>(getEditor()->getBody()) == nullptr;

	if (editorHasBody)
	{
		const bool on = getProcessor()->getEditorState(Processor::BodyShown);

		chainButtons[0]->setToggleState(on, dontSendNotification);
		getEditor()->getBody()->setVisible(on);
	}
	else
	{
		chainButtons[0]->setToggleState(false, dontSendNotification);
		chainButtons[0]->setEnabled(false);
		
	}

	getEditor()->getPanel()->refreshSize();


	for (int i = 1; i < chainButtons.size(); i++)
	{
		const int indexOfChildProcessor = i - 1; // the first index is the body

		checkActiveChilds(indexOfChildProcessor);

		bool on = getProcessor()->getChildProcessor(indexOfChildProcessor)->getEditorState(Processor::Visible);

		chainButtons[i]->setToggleState(on, dontSendNotification);
		
		if (ModulatorSynth *m = dynamic_cast<ModulatorSynth*>(getProcessor())) // ModulatorSynths can deactivate chains they don't need.
		{
			chainButtons[i]->setVisible(!m->isChainDisabled((ModulatorSynth::InternalChains)(indexOfChildProcessor)));
		}
	}



}

void ProcessorEditorChainBar::resized()
{
	const float totalWidth = (float)getWidth() * 0.8f;
	int x = (int)((float)getWidth() * 0.1f);

	int numVisibleChains = 0;

	for (int i = 0; i < chainButtons.size(); i++)
	{
		if (chainButtons[i]->isVisible()) numVisibleChains++;
	}

    const int buttonWidth = numVisibleChains > 0 ? (int)(totalWidth / (float)numVisibleChains) : 0;

    if(buttonWidth == 0)
        return;
    
	for(int i = 0; i < chainButtons.size(); i++)
	{
		if (!chainButtons[i]->isVisible()) continue;

		chainButtons[i]->setBounds(x, 0, buttonWidth, getHeight() - 4);
		x += buttonWidth;
	}
}


void ProcessorEditorChainBar::buttonClicked (Button* buttonThatWasClicked)
{
	int index = chainButtons.indexOf(static_cast<TextButton*>(buttonThatWasClicked)) - 1;

	if(index == -1)
	{
		const bool wasOn = toggleButton(buttonThatWasClicked);

		getProcessor()->setEditorState("BodyShown", !wasOn);
		getEditor()->getBody()->setVisible(!wasOn);

		refreshPanel();

		getEditor()->sendResizedMessage();

	}
	else
	{
		const bool wasOn = toggleButton(buttonThatWasClicked);

		getProcessor()->getChildProcessor(index)->setEditorState(Processor::EditorState::Visible, !wasOn, sendNotification);

		//getEditor()->getPanel()->hideChildProcessorEditor(index, !wasOn);
		//checkActiveChilds((ModulatorSynth::InternalChains)index);

		refreshPanel();
		getEditor()->sendResizedMessage();
	}
}

void ProcessorEditorChainBar::closeAll()
{
	if (chainButtons.size() != 0)
	{
		getProcessor()->setEditorState(Processor::BodyShown, false);
		chainButtons[0]->setToggleState(false, dontSendNotification);
	}
	

	for(int i = 1; i < chainButtons.size(); i++)
	{
		getProcessor()->getChildProcessor(i - 1)->setEditorState(Processor::EditorState::Visible, false);
		
		checkActiveChilds((ModulatorSynth::InternalChains)i-1);
		chainButtons[i]->setToggleState(false, dontSendNotification);
		
	}

	getEditor()->sendResizedMessage();
}

void ProcessorEditorChainBar::paint(Graphics& g)
{
	for (auto cb : chainButtons)
	{
		if (auto c = dynamic_cast<Processor*>(getChainForButton(cb)))
		{
			g.setColour(c->getColour().withAlpha(JUCE_LIVE_CONSTANT_OFF(0.7f)));

			auto b = cb->getBounds().removeFromLeft(cb->getHeight() + 2.0f).toFloat();
			g.fillRoundedRectangle(b.reduced(JUCE_LIVE_CONSTANT_OFF(1.0f)), 2.0f);
		}
	}
}

void ProcessorEditorChainBar::paintOverChildren(Graphics &g)
{
	for (int i = 1; i < chainButtons.size(); i++)
	{
		if (chainButtons[i]->getWidth() != 0)
		{
            auto c = Colours::white.withAlpha(0.5f);
			numberRenderer.drawNumberTag(g, c, chainButtons[i]->getBounds(), 2, 14, numProcessorList[i - 1]);
		}

		
	}

	if (insertPosition != -1)
	{
		TextButton *b = chainButtons[insertPosition];
		
		if (b != nullptr)
		{
			g.setColour(canBeDropped ? Colours::green : Colours::red);
			g.drawRect(chainButtons[insertPosition]->getBounds(), 2);

			g.setColour(canBeDropped ? Colours::green.withAlpha(0.2f) : Colours::red.withAlpha(0.2f));
			g.fillRect(chainButtons[insertPosition]->getBounds());
		}	
	}
}

void ProcessorEditorChainBar::timerCallback()
{
	ModulatorSynth *synth = dynamic_cast<ModulatorSynth*>(getProcessor());

	if (synth != nullptr)
	{
		setMidiIconActive(synth->getMidiInputFlag());
	}
}

} // namespace hise
