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


Note::Note(FloatingTile* parent) :
	FloatingTileContent(parent)
{
	addAndMakeVisible(editor = new TextEditor());
	editor->setFont(GLOBAL_BOLD_FONT());
	editor->setColour(TextEditor::ColourIds::backgroundColourId, Colours::transparentBlack);
	editor->setColour(TextEditor::ColourIds::textColourId, Colours::white.withAlpha(0.8f));
	editor->setColour(TextEditor::ColourIds::focusedOutlineColourId, Colours::white.withAlpha(0.5f));
	editor->setColour(CaretComponent::ColourIds::caretColourId, Colours::white);
	editor->addListener(this);
	editor->setReturnKeyStartsNewLine(true);
	editor->setMultiLine(true, true);

	editor->setLookAndFeel(&plaf);
}

void Note::resized()
{
	editor->setBounds(getLocalBounds().withTrimmedTop(16));
}

var Note::toDynamicObject() const
{
	var obj = FloatingTileContent::toDynamicObject();

	storePropertyInObject(obj, SpecialPanelIds::Text, editor->getText(), String());

	return obj;
}

void Note::fromDynamicObject(const var& object)
{
	FloatingTileContent::fromDynamicObject(object);

	editor->setText(getPropertyWithDefault(object, SpecialPanelIds::Text));
}


void Note::labelTextChanged(Label* )
{

}

void EmptyComponent::paint(Graphics& g)
{
	g.fillAll(c);
}

void EmptyComponent::mouseDown(const MouseEvent& event)
{
	getParentShell()->mouseDown(event);
}

MidiKeyboardPanel::MidiKeyboardPanel(FloatingTile* parent) :
	FloatingTileContent(parent)
{
	initColours();

	setInterceptsMouseClicks(false, true);

	addAndMakeVisible(keyboard = new CustomKeyboard(parent->getRootWindow()->getBackendProcessor()->getKeyboardState()));

	keyboard->setLowestVisibleKey(12);
}

void ApplicationCommandButtonPanel::setCommand(int commandID)
{
	Path p = BackendCommandIcons::getIcon(commandID);

	b->setCommandToTrigger(getParentShell()->getRootWindow()->getBackendProcessor()->getCommandManager(), commandID, true);
	b->setShape(p, false, true, true);
	b->setVisible(true);
}


void SliderPackPanel::resized()
{
	PanelWithProcessorConnection::resized();

	if (auto sp = getContent<SliderPack>())
	{
		int numSliders = sp->getNumSliders();

		int wPerSlider = getWidth() / numSliders;

		int newWidth = numSliders * wPerSlider;

		int y = sp->getY();
		int height = sp->getHeight();

		sp->setBounds((getWidth() - newWidth) / 2, y, newWidth, height);
	}
}

void SpacerPanel::paint(Graphics& g)
{
	g.setColour(getStyleColour(ColourIds::backgroundColour));
	g.fillRect(getParentShell()->getContentBounds());
}

VisibilityToggleBar::VisibilityToggleBar(FloatingTile* parent) :
	FloatingTileContent(parent)
{
	setControlledContainer(getParentShell()->getParentContainer());
}

void VisibilityToggleBar::setControlledContainer(FloatingTileContainer* containerToControl)
{
	controlledContainer = dynamic_cast<Component*>(containerToControl);
	refreshButtons();
}

void VisibilityToggleBar::addIcon(FloatingTile* ft)
{
	if (ft == getParentShell()) // don't show this, obviously
		return;

	if (ft->isEmpty())
		return;

	if (dynamic_cast<SpacerPanel*>(ft->getCurrentFloatingPanel()))
		return;

	auto icon = new Icon(ft);

	addAndMakeVisible(icon);

	buttons.add(icon);
}



void VisibilityToggleBar::refreshButtons()
{
	buttons.clear();

	if (customPanels.isEmpty())
	{
		if (controlledContainer.getComponent() != nullptr)
		{
			auto c = dynamic_cast<FloatingTileContainer*>(controlledContainer.getComponent());

			for (int i = 0; i < c->getNumComponents(); i++)
			{
				addIcon(c->getComponent(i));
			}

			
			resized();
		}
	}
	else
	{
		for (int i = 0; i < customPanels.size(); i++)
		{
			if (customPanels[i].getComponent() != nullptr)
			{
				addIcon(customPanels[i]);
			}
			else
				customPanels.remove(i--);
		}

		resized();
	}

	
}

var VisibilityToggleBar::toDynamicObject() const
{
	var obj = FloatingTileContent::toDynamicObject();

	Array<var> iconList;

	for (int i = 0; i < customPanels.size(); i++)
	{
		if (customPanels[i].getComponent() != nullptr)
		{
			iconList.add(customPanels[i]->getLayoutData().getID().toString());
		}
		else
		{
			jassertfalse;
		}
	}

	


	storePropertyInObject(obj, SpecialPanelIds::IconIds, iconList);
	storePropertyInObject(obj, SpecialPanelIds::Alignment, var(alignment.getFlags()));

	return obj;
}

void VisibilityToggleBar::fromDynamicObject(const var& object)
{
	FloatingTileContent::fromDynamicObject(object);

	alignment = Justification(getPropertyWithDefault(object, SpecialPanelIds::Alignment));

	auto l = getPropertyWithDefault(object, SpecialPanelIds::IconIds);

	if (auto list = l.getArray())
	{
		if (!list->isEmpty())
		{
			for (int i = 0; i < list->size(); i++)
			{
				const String name = list->getUnchecked(i).toString();

				// The other panels do not exist yet, so store them in a StringArray and add them on sibling change
				pendingCustomPanels.add(name);

			}
		}
	}

}

void VisibilityToggleBar::siblingAmountChanged()
{
	if (!pendingCustomPanels.isEmpty())
	{
		customPanels.clear();

		auto rootPanel = getParentShell()->getParentContainer();

		jassert(rootPanel != nullptr);

		for (int i = 0; i < pendingCustomPanels.size(); i++)
		{
			auto name = pendingCustomPanels[i];

			auto panel = FloatingTileHelpers::findTileWithId<FloatingTileContent>(rootPanel->getParentShell(), Identifier(name));

			if (panel != nullptr)
			{
				addCustomPanel(panel->getParentShell());
			}
			else
			{
				// Delete all buttons

				customPanels.clear();
				refreshButtons();
				return;
			}
		}

		pendingCustomPanels.clear();
	}

	refreshButtons();
}

void VisibilityToggleBar::resized()
{
	auto c = dynamic_cast<ResizableFloatingTileContainer*>(getParentShell()->getParentContainer());

	bool arrangeHorizontal = true;

	if (c != nullptr && !c->isVertical())
		arrangeHorizontal = false;

	int buttonSize = arrangeHorizontal ? getHeight() : getWidth();

	buttonSize = jmin<int>(buttonSize, 40);

	const int totalSize = arrangeHorizontal ? getWidth() : getHeight();

	const int totalButtonSize = buttons.size() * buttonSize;

	int offset = alignment == Justification::centred ? (totalSize-totalButtonSize)/2 :  0;

	if (getParentShell()->getParentContainer())
	{
		for (int i = 0; i < buttons.size(); i++)
		{
			if (arrangeHorizontal)
				buttons[i]->setBounds(offset, 0, buttonSize, buttonSize);
			else
				buttons[i]->setBounds(0, offset, buttonSize, buttonSize);

			offset += buttonSize + 5;
		}
	}
}

VisibilityToggleBar::Icon::Icon(FloatingTile* controlledTile_) :
	controlledTile(controlledTile_)
{
	addAndMakeVisible(button = new ShapeButton("button", colourOff, overColourOff, downColourOff));

	

	if (controlledTile.getComponent() != nullptr)
	{
		on = controlledTile->getLayoutData().isVisible();

		button->setShape(controlledTile->getIcon(), false, true, true);
	}

	refreshColour();

	button->addListener(this);
}

void VisibilityToggleBar::Icon::refreshColour()
{
	if (controlledTile.getComponent() != nullptr)
		button->setTooltip((on ? "Hide " : "Show ") + controlledTile->getCurrentFloatingPanel()->getBestTitle());

	if (on)
		button->setColours(colourOn, overColourOn, downColourOn);
	else
		button->setColours(colourOff, overColourOff, downColourOff);
}

void VisibilityToggleBar::Icon::buttonClicked(Button*)
{
	on = !controlledTile->getLayoutData().isVisible();

	controlledTile->getLayoutData().setVisible(on);
	controlledTile->getParentContainer()->refreshLayout();

	refreshColour();
}
