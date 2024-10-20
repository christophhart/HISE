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


	EmptyComponent::EmptyComponent(FloatingTile* p):
		FloatingTileContent(p)
	{
		Random r;

		setTooltip("Right click to create a Panel");

		setInterceptsMouseClicks(false, true);

		c = Colour(r.nextInt()).withAlpha(0.1f);
	}

void EmptyComponent::paint(Graphics& g)
{
	g.fillAll(c);
}

void EmptyComponent::mouseDown(const MouseEvent& event)
{
	getParentShell()->mouseDown(event);
}

#if USE_BACKEND
void ApplicationCommandButtonPanel::setCommand(int commandID)
{
	Path p = BackendCommandIcons::getIcon(commandID);

	b->setCommandToTrigger(getParentShell()->getMainController()->getCommandManager(), commandID, true);
	b->setShape(p, false, true, true);
	b->setVisible(true);
}
#endif

void SpacerPanel::paint(Graphics& g)
{
	g.setColour(findPanelColour(FloatingTileContent::PanelColourId::bgColour));
	g.fillRect(getParentShell()->getContentBounds());
}

VisibilityToggleBar::VisibilityToggleBar(FloatingTile* parent) :
	FloatingTileContent(parent)
{
	setDefaultPanelColour(PanelColourId::bgColour, HiseColourScheme::getColour(HiseColourScheme::ColourIds::EditorBackgroundColourId));

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
				auto comp = c->getComponent(i);

				if (auto compAsContainer = dynamic_cast<FloatingTileContainer*>(comp->getCurrentFloatingPanel()))
				{
					// Skip the button for the container of this panel...
					if (compAsContainer->getIndexOfComponent(getParentShell()) != -1)
						continue;
				}

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

int VisibilityToggleBar::getNumDefaultableProperties() const
{
	return SpecialPanelIds::numSpecialPanelIds;
}

Identifier VisibilityToggleBar::getDefaultablePropertyId(int index) const
{
	if (index < (int)PanelPropertyId::numPropertyIds)
		return FloatingTileContent::getDefaultablePropertyId(index);

	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::Alignment, "Alignment");
	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::IconIds, "IconIds");

	jassertfalse;
	return{};
}

var VisibilityToggleBar::getDefaultProperty(int index) const
{
	if (index < (int)PanelPropertyId::numPropertyIds)
		return FloatingTileContent::getDefaultProperty(index);

	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::Alignment, var((int)Justification::centred));
	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::IconIds, Array<var>());

	jassertfalse;
	return{};
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
	auto c = dynamic_cast<ResizableFloatingTileContainer*>(controlledContainer.getComponent());

	bool arrangeHorizontal = true;

	if (c != nullptr && !c->isVertical())
		arrangeHorizontal = false;

	int buttonSize = arrangeHorizontal ? getHeight() : getWidth();

	buttonSize = jmin<int>(buttonSize, 40);

	const int totalSize = arrangeHorizontal ? getWidth() : getHeight();

	const int totalButtonSize = buttons.size() * buttonSize;

	int offset = alignment == Justification::centred ? (totalSize-totalButtonSize)/2 :  0;

	if (c != nullptr)
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


InterfaceContentPanel::InterfaceContentPanel(FloatingTile* parent) :
	FloatingTileContent(parent)
{
	//setOpaque(true);

	if (!connectToScript())
	{
		addAndMakeVisible(refreshButton = new TextButton("Refresh"));
		refreshButton->setLookAndFeel(&laf);
		refreshButton->setColour(TextButton::ColourIds::textColourOnId, Colours::white);
		refreshButton->setColour(TextButton::ColourIds::textColourOffId, Colours::white);
		refreshButton->addListener(this);
	}

	dynamic_cast<GlobalSettingManager*>(getMainController())->addScaleFactorListener(this);
	getMainController()->addScriptListener(this);
	getMainController()->getLockFreeDispatcher().addPresetLoadListener(this);
	getMainController()->getExpansionHandler().addListener(this);
}


InterfaceContentPanel::~InterfaceContentPanel()
{
	dynamic_cast<GlobalSettingManager*>(getMainController())->removeScaleFactorListener(this);
	getMainController()->removeScriptListener(this);
	getMainController()->getLockFreeDispatcher().removePresetLoadListener(this);
	getMainController()->getExpansionHandler().removeListener(this);

	content = nullptr;
}

void InterfaceContentPanel::paint(Graphics& g)
{
	//g.fillAll(Colours::black);

	if (content == nullptr)
	{
		g.setFont(GLOBAL_BOLD_FONT());
		g.setColour(Colours::white);
		g.drawText("No interface found", getLocalBounds().toFloat().removeFromTop(40.0f), Justification::centred);
	}
}

void InterfaceContentPanel::resized()
{
	if (content != nullptr)
	{
		content->setBounds(getLocalBounds());
	}
	else if (refreshButton != nullptr)
	{
		refreshButton->centreWithSize(200, 30);
	}
}

void InterfaceContentPanel::newHisePresetLoaded()
{
	
	content = nullptr;
	connectToScript();

	resized();
}

void InterfaceContentPanel::expansionPackLoaded(Expansion* currentExpansion)
{
#if USE_BACKEND

	if (FullInstrumentExpansion::isEnabled(getMainController()))
	{
		// We need to remove the interface content because it might be dangling...

		Component::SafePointer<InterfaceContentPanel> safeThis(this);

		MessageManager::callAsync([safeThis]()
			{
				if (safeThis.getComponent() != nullptr)
				{
					safeThis.getComponent()->content = nullptr;
					safeThis.getComponent()->resized();
				}
			});
	}
#endif
}

void InterfaceContentPanel::scriptWasCompiled(JavascriptProcessor *processor)
{
	if (processor == dynamic_cast<JavascriptProcessor*>(connectedProcessor.get()))
	{
		updateSize();

		
	}
}


void InterfaceContentPanel::buttonClicked(Button* /*b*/)
{
	connectToScript();
	resized();
}


void InterfaceContentPanel::scaleFactorChanged(float /*newScaleFactor*/)
{
	updateSize();
}

void InterfaceContentPanel::processorDeleted(Processor* deletedProcessor)
{
	if (FullInstrumentExpansion::isEnabled(getMainController()))
		content = nullptr;
}

bool InterfaceContentPanel::connectToScript()
{
	if (content != nullptr)
		return true;

	if (auto jsp = JavascriptMidiProcessor::getFirstInterfaceScriptProcessor(getMainController()))
	{
		if (FullInstrumentExpansion::isEnabled(getMainController()))
			jsp->addDeleteListener(this);

		addAndMakeVisible(content = new ScriptContentComponent(jsp));
		connectedProcessor = jsp;
		
		if (refreshButton != nullptr)
		{
			refreshButton->setVisible(false);
		}

		jsp->getScriptingContent()->interfaceSizeBroadcaster.addListener(*this, [](InterfaceContentPanel& ip, int, int)
		{
			ip.updateSize();
		});

		repaint();

		return true;
	}
	else
	{
		return false;
	}
}

void InterfaceContentPanel::updateSize()
{

#if USE_BACKEND
	if (auto pwsc = dynamic_cast<ProcessorWithScriptingContent*>(connectedProcessor.get()))
	{
		auto c = pwsc->getScriptingContent();

		if (c != nullptr)
		{
			auto topLevel = findParentComponentOfClass<FloatingTileDocumentWindow>();

			if (topLevel != nullptr)
			{
				topLevel->setName("Preview: " + connectedProcessor->getId());

				auto scaleFactor = dynamic_cast<GlobalSettingManager*>(connectedProcessor->getMainController())->getGlobalScaleFactor();

				setTransform(AffineTransform::scale((float)scaleFactor));
				
				topLevel->setSize((int)((float)c->getContentWidth() * scaleFactor), (int)((float)c->getContentHeight() * scaleFactor));
			}

			getParentShell()->setVital(true);
		}
	}
#endif
}


ComplexDataEditorPanel::ComplexDataEditorPanel(FloatingTile* parent, snex::ExternalData::DataType t):
	PanelWithProcessorConnection(parent),
	type(t)
{}

Component* ComplexDataEditorPanel::createContentComponent(int index)
{
	if (auto pb = dynamic_cast<ProcessorWithExternalData*>(getProcessor()))
	{
		if (isPositiveAndBelow(index, pb->getNumDataObjects(type)))
		{
			auto obj = pb->getComplexBaseType(type, index);
			return dynamic_cast<Component*>(snex::ExternalData::createEditor(obj));
		}
	}

	return nullptr;
}

void ComplexDataEditorPanel::fillIndexList(StringArray& indexList)
{
	if (auto pb = dynamic_cast<ProcessorWithExternalData*>(getProcessor()))
	{
		int numObjects = pb->getNumDataObjects(type);

		auto name = ExternalData::getDataTypeName(type);

		for (int i = 0; i < numObjects; i++)
			indexList.add(name + String(i + 1));
	}
}

Component* PlotterPanel::createContentComponent(int)
{
	auto p = new Plotter(getMainController()->getGlobalUIUpdater());
	if (auto mod = dynamic_cast<Modulation*>(getConnectedProcessor()))
	{
		mod->setPlotter(p);

		p->setColour(Plotter::backgroundColour , findPanelColour(PanelColourId::bgColour));
		p->setColour(Plotter::pathColour , findPanelColour(PanelColourId::itemColour1));
		p->setColour(Plotter::pathColour2, findPanelColour(PanelColourId::itemColour2));
		p->setColour(Plotter::outlineColour , findPanelColour(PanelColourId::itemColour3));
		p->setColour(Plotter::textColour, findPanelColour(PanelColourId::textColour));
		p->setFont(getFont());
	}

	return p;
}

juce::Identifier PlotterPanel::getProcessorTypeId() const
{
	return LfoModulator::getClassType();
}

ProcessorPeakMeter::ProcessorPeakMeter(Processor* p):
	processor(p)
{
	addAndMakeVisible(vuMeter = new VuMeter());

	setOpaque(true);

	vuMeter->setColour(VuMeter::backgroundColour, Colour(0xFF333333));
	vuMeter->setColour(VuMeter::ledColour, Colours::lightgrey);
	vuMeter->setColour(VuMeter::outlineColour, Colour(0x22000000));

	startTimer(30);
}

ProcessorPeakMeter::~ProcessorPeakMeter()
{
	stopTimer();
	vuMeter = nullptr;
	processor = nullptr;
}

void ProcessorPeakMeter::paint(Graphics& g)
{
	g.fillAll(Colour(0xFF333333));
}

void ProcessorPeakMeter::resized()
{
	if (getWidth() > getHeight())
		vuMeter->setType(VuMeter::StereoHorizontal);
	else
		vuMeter->setType(VuMeter::StereoVertical);

	vuMeter->setBounds(getLocalBounds());
}

void ProcessorPeakMeter::timerCallback()
{
	if (processor.get())
	{
		const auto& values = processor->getDisplayValues();

		vuMeter->setPeak(values.outL, values.outR);
	}
}
} // namespace hise
