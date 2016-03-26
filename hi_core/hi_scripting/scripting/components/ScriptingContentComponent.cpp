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

ModulatorPeakMeter::ModulatorPeakMeter(Modulator *m) :
mod(m)
{
	addAndMakeVisible(vuMeter = new VuMeter());

	vuMeter->setType(VuMeter::MonoVertical);

	vuMeter->setColour(VuMeter::ledColour, Colour(0x88dddddd));

	startTimer(40);

	vuMeter->addMouseListener(this, true);
}

ScriptContentComponent::ScriptContentComponent(ScriptBaseProcessor *p) :
processor(p),
editedComponent(-1)
{
	setNewContent(p->getScriptingContent());

	setInterceptsMouseClicks(false, true);

	processor->addChangeListener(this);
	processor->getMainController()->addScriptListener(this, true);
}




ScriptContentComponent::~ScriptContentComponent()
{
	if (contentData.get() != nullptr)
	{
		contentData->removeChangeListener(this);

		for (int i = 0; i < contentData->getNumComponents(); i++)
		{
			contentData->getComponent(i)->removeChangeListener(this);
		}
	}

	if (processor.get() != nullptr)
	{
		processor->getMainController()->removeScriptListener(this);
		processor->removeChangeListener(this);
	};
}



void ScriptContentComponent::refreshMacroIndexes()
{
	MacroControlBroadcaster *mcb = processor->getMainController()->getMacroManager().getMacroChain();

	for(int i = 0; i < componentWrappers.size(); i++)
	{
		int macroIndex = mcb->getMacroControlIndexForProcessorParameter(processor, i);

		if(macroIndex != -1)
		{
			MacroControlBroadcaster::MacroControlledParameterData * pData = mcb->getMacroControlData(macroIndex)->getParameterWithProcessorAndIndex(processor, i);

			// Check if the name matches
			if(pData->getParameterName() != componentWrappers[i]->getComponent()->getName())
			{
				const String x = pData->getParameterName();

				mcb->getMacroControlData(macroIndex)->removeParameter(x);

				processor->getMainController()->getMacroManager().getMacroChain()->sendChangeMessage();

				debugToConsole(processor, "Index mismatch: Removed Macro Control for " + x);
			}

		}
	}
}


String ScriptContentComponent::getContentTooltip() const
{
	if (contentData != nullptr)
	{
		return contentData->tooltip;
	}
	else return String::empty;
}

Colour ScriptContentComponent::getContentColour()
{
	if (contentData != nullptr)
	{
		return contentData->colour;
	}
	else
	{
		return Colour(0xff777777);
	}
}

void ScriptContentComponent::updateValue(int i)
{
	MacroControlledObject *o = dynamic_cast<MacroControlledObject*>(componentWrappers[i]->getComponent());

	if (o != nullptr)
	{
		o->updateValue();
	}

	if (TableEditor *t = dynamic_cast<TableEditor*>(componentWrappers[i]->getComponent()))
	{
		t->setDisplayedIndex((float)contentData->components[i]->value / 127.0f);
	}

	if (Slider *s = dynamic_cast<Slider*>(componentWrappers[i]->getComponent()))
	{
		if (s->getSliderStyle() == Slider::TwoValueHorizontal)
		{
			const double min = dynamic_cast<ScriptingApi::Content::ScriptSlider*>(contentData->components[i].get())->getMinValue();
			const double max = dynamic_cast<ScriptingApi::Content::ScriptSlider*>(contentData->components[i].get())->getMaxValue();

			s->setMinValue(min, dontSendNotification);
			s->setMaxValue(max, dontSendNotification);
		}
	}
}

void ScriptContentComponent::updateValues()
{
	for (int i = 0; i < componentWrappers.size(); i++)
	{
		updateValue(i);
	}
}

void ScriptContentComponent::changeListenerCallback(SafeChangeBroadcaster *b)
{
	if (contentData.get() == nullptr) return;

	if (processor.get() == nullptr)
	{
		setEnabled(false);
	}

	if (getScriptProcessor() == b)
	{
		updateValues();
	}
	else if (dynamic_cast<ScriptingApi::Content::ScriptComponent*>(b) != nullptr)
	{
		updateContent(dynamic_cast<ScriptingApi::Content::ScriptComponent*>(b));
	}
	else
	{
		updateContent();
	}
}

void ScriptContentComponent::updateComponent(int i)
{
    

	if(componentWrappers[i]->getComponent() == nullptr)
    {
        jassertfalse;
        return;
    }
    
	componentWrappers[i]->getComponent()->setVisible(contentData->components[i]->getScriptObjectProperty(ScriptingApi::Content::ScriptComponent::visible));
	
	bool enabled = contentData->components[i]->getScriptObjectProperty(ScriptingApi::Content::ScriptComponent::enabled);
	
	componentWrappers[i]->getComponent()->setEnabled(enabled);

	if (contentData->components[i]->getScriptObjectProperty(ScriptingApi::Content::ScriptComponent::zOrder) == "Always on top")
	{
		componentWrappers[i]->getComponent()->setAlwaysOnTop(true);
	}
	else
	{
		componentWrappers[i]->getComponent()->setAlwaysOnTop(false);
	}

	if (componentWrappers[i]->getComponent()->getLocalBounds() != contentData->components[i]->getPosition())
	{
		componentWrappers[i]->getComponent()->setBounds(contentData->components[i]->getPosition());
	}

	componentWrappers[i]->updateComponent();
	updateValue(i);
}

void ScriptContentComponent::updateContent(ScriptingApi::Content::ScriptComponent* componentToUpdate/*=nullptr*/)
{
	if (contentData.get() == nullptr) return;

	if (componentToUpdate == nullptr)
	{

		jassert(contentData->components.size() == componentWrappers.size());

		for (int i = 0; i < contentData->components.size(); i++)
		{
			updateComponent(i);
		}

		//resized();
	}
	else
	{
		updateComponent(contentData->components.indexOf(componentToUpdate));
	}
}

void ScriptContentComponent::resized()
{
	if (!contentValid())
	{
		return;
	}

	for (int i = 0; i < componentWrappers.size(); i++)
	{
		Component *c = componentWrappers[i]->getComponent();

		if (c->getLocalBounds() != contentData->components[i]->getPosition())
		{
			c->setBounds(contentData->components[i]->getPosition());
		}
	}
}

void ScriptContentComponent::scriptWasCompiled(ScriptProcessor *p)
{
	if (p == getScriptProcessor())
	{
		setNewContent(p->getScriptingContent());
		updateContent();
	}
}

void ScriptContentComponent::setNewContent(ScriptingApi::Content *c)
{
	if (c == nullptr) return;

	contentData = c;

	contentData->addChangeListener(this);

	deleteAllScriptComponents();

	for (int i = 0; i < contentData->components.size(); i++)
	{
		contentData->components[i].get()->addChangeListener(this);

		componentWrappers.add(contentData->components[i].get()->createComponentWrapper(this, i));

		addAndMakeVisible(componentWrappers.getLast()->getComponent());
	}

	if (getParentComponent() != nullptr)
	{
		for (int i = 0; i < componentWrappers.size(); i++)
		{
			componentWrappers[i]->getComponent()->addMouseListener(getParentComponent(), true);
		}
	}

	refreshMacroIndexes();
	refreshContentButton();

	if (getWidth() != 0) setSize(getWidth(), getContentHeight());

	updateContent();
}

void ScriptContentComponent::refreshContentButton()
{
#if USE_BACKEND

	if(ScriptingEditor *e = dynamic_cast<ScriptingEditor*>(getParentComponent()))
	{
		e->checkContent();
	}

#endif

}


ScriptingApi::Content::ScriptComponent * ScriptContentComponent::getEditedComponent()
{
	if (contentData.get() != nullptr && editedComponent != -1)
	{
		return contentData->getComponent(editedComponent);
	}

	return nullptr;
}

void ScriptContentComponent::setEditedScriptComponent(ScriptingApi::Content::ScriptComponent *sc)
{
	if (sc == nullptr)
	{
		editedComponent = -1;

		setWantsKeyboardFocus(false);

		repaint();
		return;

	}

	String id = sc->getName().toString();

	// Use the text value for the slider (hack because the slider class doesn't allow a dedicated name. */
	String text = sc->getScriptObjectProperty(ScriptingApi::Content::ScriptComponent::text).toString();

	for (int i = 0; i < componentWrappers.size(); i++)
	{

		const bool isSlider = dynamic_cast<Slider*>(componentWrappers[i]->getComponent()) != nullptr;

		const bool sliderMatches = isSlider && (componentWrappers[i]->getComponent()->getName() == text || componentWrappers[i]->getComponent()->getName() == id);

		if (sliderMatches || id == componentWrappers[i]->getComponent()->getName())
		{
			editedComponent = i;

			setWantsKeyboardFocus(true);
			grabKeyboardFocus();

			repaint();
			return;
		}
	}

	editedComponent = -1;
	repaint();
}

bool ScriptContentComponent::keyPressed(const KeyPress &key)
{
	if (editedComponent == -1) return false;

	if (contentData.get() == nullptr) return false;

	ScriptingApi::Content::ScriptComponent *sc = contentData->getComponent(editedComponent);

	if (sc == nullptr) return false;

	const int keyCode = key.getKeyCode();

	const int delta = key.getModifiers().isCommandDown() ? 10 : 1;

	const int horizontalProperty = key.getModifiers().isShiftDown() ? ScriptingApi::Content::ScriptComponent::width : ScriptingApi::Content::ScriptComponent::x;
	const int verticalProperty = key.getModifiers().isShiftDown() ? ScriptingApi::Content::ScriptComponent::height : ScriptingApi::Content::ScriptComponent::y;

	int x = sc->getScriptObjectProperty(horizontalProperty);
	int y = sc->getScriptObjectProperty(verticalProperty);

	if (keyCode == KeyPress::upKey)
	{
		sc->setScriptObjectPropertyWithChangeMessage(sc->getIdFor(verticalProperty), y - delta, sendNotification);
		sc->setChanged();

		return true;
	}
	else if (keyCode == KeyPress::downKey)
	{
		sc->setScriptObjectPropertyWithChangeMessage(sc->getIdFor(verticalProperty), y + delta, sendNotification);
		sc->setChanged();
		return true;
	}
	else if (keyCode == KeyPress::leftKey)
	{
		sc->setScriptObjectPropertyWithChangeMessage(sc->getIdFor(horizontalProperty), x - delta, sendNotification);
		sc->setChanged();
		return true;
	}
	else if (keyCode == KeyPress::rightKey)
	{
		sc->setScriptObjectPropertyWithChangeMessage(sc->getIdFor(horizontalProperty), x + delta, sendNotification);
		sc->setChanged();
		return true;
	}

	return false;
}

void ScriptContentComponent::paintOverChildren(Graphics &g)
{
	if (editedComponent != -1)
	{

		Component *c = componentWrappers[editedComponent]->getComponent();

		g.setColour(Colours::white.withAlpha(0.2f));


		g.drawRect(c->getBounds(), 2);
	}
}

void ScriptContentComponent::paint(Graphics &g)
{
#if USE_BACKEND
	if(dynamic_cast<ScriptingEditor*>(getParentComponent()) != nullptr)
	{
		g.fillAll(Colours::white.withAlpha(0.05f));
		g.setGradientFill(ColourGradient(Colours::black.withAlpha(0.2f), 0.0f, 0.0f,
			Colours::transparentBlack, 0.0f, 6.0f, false));
		g.fillRect(0.0f, 0.0f, (float)getWidth(), 6.0f);
	}
#endif
}

ScriptingApi::Content::ScriptComponent * ScriptContentComponent::getScriptComponentFor(Point<int> pos)
{
	for (int i = componentWrappers.size() - 1; i >= 0; --i)
	{
		if (componentWrappers[i]->getComponent()->getBounds().contains(pos))
		{
			return contentData->getComponent(i);
		}
	}

	return nullptr;
}

ScriptContentContainer::ScriptContentContainer(ModulatorSynthChain* chain_, ModulatorSynthChainBody *editor_) :
	chain(chain_),
	editor(editor_),
	isFrontend(false)
{
	chain->getMainController()->addScriptListener(this);
	laf = new ChainBarButtonLookAndFeel();
};

ScriptContentContainer::~ScriptContentContainer()
{
	if (chain != nullptr)
	{
		chain->getMainController()->removeScriptListener(this);

		interfaces.clear();

		laf = nullptr;
	}
	

	
}

int ScriptContentContainer::getContentHeight() const
{
	const int heightOfButtonBar = interfaces.size() > 1 ? 20 : 0;

	const int marginBottom = editor != nullptr ? 4 : 0;

	const int contentHeight = currentContent != nullptr ? currentContent->getContentHeight() + marginBottom: 0;

	return heightOfButtonBar + contentHeight;
};

void ScriptContentContainer::scriptWasCompiled(ScriptProcessor *) 
{
	checkInterfaces();

	for(int i = 0; i < interfaces.size(); i++)
	{
		interfaces[i]->button->setButtonText(interfaces[i]->content->getScriptProcessor()->getId());
	}

	restoreSavedView();
		
#if USE_BACKEND 
	if(editor.getComponent() != nullptr) editor.getComponent()->refreshBodySize();
#endif

	refreshContentBounds();
}

void ScriptContentContainer::checkInterfaces()
{
	Processor::Iterator<ScriptProcessor> iter(chain, true);

	ScriptProcessor *sp;

	while((sp = iter.getNextProcessor()) != nullptr)
	{
		if(iter.getHierarchyForCurrentProcessor() > 2) break; // only check child processors of the immediate MidiProcessorChain

		// Only check Processors which are set as interface using Content.addToFront()
		if(!sp->isFront()) continue; 

		if(processorHasContent(sp)) continue;

		interfaces.add(new InterfaceScriptAndButton(sp, this));

		addAndMakeVisible(interfaces.getLast()->button);
		addAndMakeVisible(interfaces.getLast()->content);
	}

	// Check for dangling ScriptProcessors
	for(int i = 0; i < interfaces.size(); i++)
	{
		if(interfaces[i]->content->getScriptProcessor() == nullptr)
		{
			if(interfaces[i]->content == currentContent)
			{
				if(interfaces.size() <= 1)
				{
					setCurrentContent(-1, sendNotification);
				}
				else
				{
					setCurrentContent(0, sendNotification);
				}
			}
				
			interfaces.remove(i--);
		}
	}

	// If there is only one interface left, it must be visible
	if(interfaces.size() >= 1 && currentContent.getComponent() != interfaces[0]->content)
	{
		setCurrentContent(0, sendNotification);
	}
}

void ScriptContentContainer::refreshContentBounds()
{
	if(currentContent.getComponent() == nullptr) return;

	const bool useButtonBar = interfaces.size() > 1;

	const int buttonBarY = 0;
	const int contentY = 20;

	const int buttonOffset = isFrontendContainer() ? 55 : 0;

	if(useButtonBar)
	{
		int widthPerButton = isFrontendContainer() ? 100 : (int)((float)getWidth() / (float)interfaces.size());

		for(int i = 0; i < interfaces.size(); i++)
		{
			interfaces[i]->content->setBounds(0,0,0,0);
			interfaces[i]->button->setBounds(buttonOffset + i * widthPerButton, buttonBarY, widthPerButton, 20);

			interfaces[i]->button->setTooltip(interfaces[i]->content->getContentTooltip());

		}

		if(currentContent.getComponent() != nullptr)
		{
			int width = currentContent->getContentWidth();

			if(width == -1) width = getWidth();

			int offsetX = jmax<int>(0, (getWidth() - width) / 2);

			currentContent.getComponent()->setBounds(offsetX, contentY, width, currentContent->getContentHeight());
		}
	}
	else
	{
		for(int i = 0; i < interfaces.size(); i++)
		{
			interfaces[i]->button->setBounds(0,0,0,0);
		}

		if(currentContent.getComponent() != nullptr)
		{
			int width = currentContent->getContentWidth();

			if(width == -1) width = getWidth();

			int offsetX = jmax<int>(0, (getWidth() - width) / 2);

			currentContent.getComponent()->setBounds(offsetX, 3, width, currentContent->getContentHeight());
			currentContent->updateContent();
		}
	}
}

void ScriptContentContainer::paint(Graphics &g)
{
	if (isFrontendContainer())
	{
		if (interfaces.size() > 1)
		{
			g.setColour(Colour(0xFF999999));
			g.setFont(GLOBAL_BOLD_FONT());
			g.drawText("Pages:", 10, 0, 100, 20, Justification::centredLeft, false);
		}

	}
}

void ScriptContentContainer::setCurrentContent(int index, NotificationType notifyEditorState)
{
	if(index == -1) return;

	if(index < interfaces.size() && interfaces[index]->content->getScriptProcessor() == nullptr)
	{
		interfaces.remove(index);

		if(interfaces.size() >= 1)
		{
			setCurrentContent(0, notifyEditorState);
		}
		else
		{
			setCurrentContent(-1, notifyEditorState);
		}
	}

	for(int i = 0; i < interfaces.size(); i++)
	{
		interfaces[i]->content->setVisible(false);
		interfaces[i]->button->setToggleState(false, dontSendNotification);
	}

	if(notifyEditorState == sendNotification)
	{
		chain->setEditorState("InterfaceShown", index);
	}
		
	currentContent = interfaces[index]->content;
	interfaces[index]->button->setToggleState(true, dontSendNotification);
	currentContent->setVisible(true);
}

void ScriptContentContainer::buttonClicked(Button *b)
{
	const int index = getIndexForButton(b);

	jassert(index != -1);

	setCurrentContent(index, sendNotification);

#if USE_BACKEND 
	if(editor.getComponent() != nullptr) editor.getComponent()->refreshBodySize();
#endif

	refreshContentBounds();
};

/** Restores the index from the editorstate. */
void ScriptContentContainer::restoreSavedView()
{
	const int index = chain->getEditorState("InterfaceShown");
	if(index >= 0 && index < interfaces.size()) setCurrentContent(index, dontSendNotification);
}

ScriptContentContainer::InterfaceScriptAndButton::InterfaceScriptAndButton(ScriptProcessor *sp, ScriptContentContainer *container)
{
	content = new ScriptContentComponent(sp);
	button = new TextButton(content->getScriptName(), "Show / hide the " + content->getScriptName());

	button->setButtonText(sp->getId());
	button->setConnectedEdges(Button::ConnectedOnLeft | Button::ConnectedOnRight);
	button->addListener(container);


	if (container->isFrontendContainer())
	{

		Colour dark(0xff222222);
		Colour bright(0xff999999);

		button->setColour(TextButton::buttonColourId, dark);
		button->setColour(TextButton::buttonOnColourId, bright);
		button->setColour(TextButton::textColourOnId, dark);
		button->setColour(TextButton::textColourOffId, bright);
	}
	else
	{
		button->setColour(TextButton::buttonColourId, Colour(0x4c4b4b4b));
		button->setColour(TextButton::buttonOnColourId, Colours::white.withAlpha(0.7f));
		button->setColour(TextButton::textColourOnId, Colour(0x77ffffff));
		button->setColour(TextButton::textColourOffId, Colour(0x45ffffff));
	}

	button->setLookAndFeel(container->laf);
}
