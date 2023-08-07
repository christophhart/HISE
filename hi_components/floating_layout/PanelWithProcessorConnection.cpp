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

PanelWithProcessorConnection::PanelWithProcessorConnection(FloatingTile* parent) :
	FloatingTileContent(parent),
	showConnectionBar("showConnectionBar"),
	followWorkspaceButton("workspace", nullptr, factory)
{
	addAndMakeVisible(followWorkspaceButton);
	addAndMakeVisible(connectionSelector = new ComboBox());
	connectionSelector->addListener(this);
	getMainSynthChain()->getMainController()->skin(*connectionSelector);

	followWorkspaceButton.setToggleModeWithColourChange(true);
	followWorkspaceButton.setTooltip("Enables updating the content when a workspace button is clicked in the patch browser");
	followWorkspaceButton.setWantsKeyboardFocus(false);

	connectionSelector->setColour(HiseColourScheme::ComponentFillTopColourId, Colours::transparentBlack);
	connectionSelector->setColour(HiseColourScheme::ComponentFillBottomColourId, Colours::transparentBlack);
	connectionSelector->setColour(HiseColourScheme::ComponentOutlineColourId, Colours::transparentBlack);
	connectionSelector->setTextWhenNothingSelected("Disconnected");

	addAndMakeVisible(indexSelector = new ComboBox());
	indexSelector->addListener(this);
	getMainSynthChain()->getMainController()->skin(*indexSelector);

	indexSelector->setColour(HiseColourScheme::ComponentFillTopColourId, Colours::transparentBlack);
	indexSelector->setColour(HiseColourScheme::ComponentFillBottomColourId, Colours::transparentBlack);
	indexSelector->setColour(HiseColourScheme::ComponentOutlineColourId, Colours::transparentBlack);
	indexSelector->setTextWhenNothingSelected("Disconnected");

	connectionSelector->setLookAndFeel(&hlaf);
	indexSelector->setLookAndFeel(&hlaf);

	connectionSelector->setWantsKeyboardFocus(false);
	indexSelector->setWantsKeyboardFocus(false);

#if USE_BACKEND

    if(parent->isOnInterface())
        return;
    
    if (CompileExporter::isExportingFromCommandLine())
        return;
    
	getMainController()->getProcessorChangeHandler().addProcessorChangeListener(this);

	dynamic_cast<BackendProcessor*>(getMainController())->workspaceBroadcaster.addListener(*this, [](PanelWithProcessorConnection& pc, const Identifier& id, Processor* p)
	{
		if (pc.shouldFollowNewWorkspace(p, id))
			pc.setContentWithUndo(p, pc.getCurrentIndex());
	});
#endif
}

PanelWithProcessorConnection::~PanelWithProcessorConnection()
{
	content = nullptr;

#if USE_BACKEND
	getMainController()->getProcessorChangeHandler().removeProcessorChangeListener(this);
#endif
}

void PanelWithProcessorConnection::paint(Graphics& g)
{
	auto bounds = getParentShell()->getContentBounds();

	const bool scb = getStyleProperty(showConnectionBar, true) && !shouldHideSelector();
    
	if (scb)
	{
		const bool connected = getProcessor() != nullptr && (!hasSubIndex() || currentIndex != -1);

		//g.setColour(Colour(0xFF3D3D3D));
		//g.fillRect(0, bounds.getY(), getWidth(), 18);

		g.setColour(connected ? getProcessor()->getColour() : Colours::white.withAlpha(0.1f));

		Path p;
		p.loadPathFromData(ColumnIcons::connectionIcon, sizeof(ColumnIcons::connectionIcon));
		p.scaleToFit(2.0, (float)bounds.getY() + 2.0f, 14.0f, 14.0f, true);
		g.fillPath(p);
	}
	
}

var PanelWithProcessorConnection::toDynamicObject() const
{
	var obj = FloatingTileContent::toDynamicObject();

	storePropertyInObject(obj, SpecialPanelIds::ProcessorId, getConnectedProcessor() != nullptr ? getConnectedProcessor()->getId() : "");
	storePropertyInObject(obj, SpecialPanelIds::Index, currentIndex);
	storePropertyInObject(obj, SpecialPanelIds::Index, currentIndex);
	storePropertyInObject(obj, SpecialPanelIds::FollowWorkspace, followWorkspaceButton.getToggleState());

	return obj;
}

void PanelWithProcessorConnection::fromDynamicObject(const var& object)
{
	FloatingTileContent::fromDynamicObject(object);

	const String id = getPropertyWithDefault(object, SpecialPanelIds::ProcessorId);

	int index = getPropertyWithDefault(object, SpecialPanelIds::Index);

	if (id.isNotEmpty())
	{
		auto p = ProcessorHelpers::getFirstProcessorWithName(getParentShell()->getMainController()->getMainSynthChain(), id);

		if (p != nullptr)
		{
			setContentWithUndo(p, index);
		}
	}

	followWorkspaceButton.setToggleStateAndUpdateIcon(getPropertyWithDefault(object, SpecialPanelIds::FollowWorkspace));
}

int PanelWithProcessorConnection::getNumDefaultableProperties() const
{
	return SpecialPanelIds::numSpecialPanelIds;
}

Identifier PanelWithProcessorConnection::getDefaultablePropertyId(int index) const
{
	if (index < (int)PanelPropertyId::numPropertyIds)
		return FloatingTileContent::getDefaultablePropertyId(index);

	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::ProcessorId, "ProcessorId");
	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::Index, "Index");
	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::FollowWorkspace, "FollowWorkspace");

	jassertfalse;
	return{};
}

var PanelWithProcessorConnection::getDefaultProperty(int index) const
{
	if (index < (int)PanelPropertyId::numPropertyIds)
		return FloatingTileContent::getDefaultProperty(index);

	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::ProcessorId, var(""));
	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::Index, var(-1));
	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::FollowWorkspace, false);

	jassertfalse;
	return{};
}

void PanelWithProcessorConnection::incIndex(bool up)
{
	int newIndex = currentIndex;

	if (up)
	{
		newIndex = jmin<int>(currentIndex + 1, indexSelector->getNumItems()-1);
	}
	else
	{
		newIndex = jmax<int>(newIndex - 1, 0);
	}

	setContentWithUndo(currentProcessor, newIndex);
}

hise::Processor* PanelWithProcessorConnection::createDummyProcessorForDocumentation(MainController* mc)
{
	ScopedPointer<FactoryType> f = new ModulatorSynthChainFactoryType(1, mc->getMainSynthChain());

	auto id = getProcessorTypeId();
	auto index = f->getProcessorTypeIndex(id);

	auto idAsString = id.toString();

	if (idAsString == "Skip" || idAsString == "unsupported")
		return nullptr;

	if (index == -1)
	{
		f = new ModulatorChainFactoryType(1, Modulation::GainMode, mc->getMainSynthChain());
		index = f->getProcessorTypeIndex(id);
	}

	if (index == -1)
	{
		f = new EffectProcessorChainFactoryType(1, mc->getMainSynthChain());
		index = f->getProcessorTypeIndex(id);
	}

	if (index == -1)
	{
		f = new MidiProcessorFactoryType(mc->getMainSynthChain());
		index = f->getProcessorTypeIndex(id);
	}

	jassert(index != -1);

	return f->createProcessor(index, "Dummy Processor");
}

void PanelWithProcessorConnection::moduleListChanged(Processor* b, MainController::ProcessorChangeHandler::EventType type)
{
	if (type == MainController::ProcessorChangeHandler::EventType::ProcessorBypassed ||
		type == MainController::ProcessorChangeHandler::EventType::ProcessorColourChange)
		return;

	if (type == MainController::ProcessorChangeHandler::EventType::ProcessorRenamed)
	{
		if (getConnectedProcessor() == b || getConnectedProcessor() == nullptr)
		{
			int tempId = connectionSelector->getSelectedId();

			refreshConnectionList();

			connectionSelector->setSelectedId(tempId, dontSendNotification);
		}
	}
	else
	{
		refreshConnectionList();
	}

	
}

void PanelWithProcessorConnection::resized()
{
	if (shouldHideSelector())
	{
		connectionSelector->setVisible(false);
		indexSelector->setVisible(false);
		followWorkspaceButton.setVisible(false);

		if (content != nullptr)
		{
			content->setVisible(true);
			content->setBounds(getLocalBounds());
		}
			
	}
	else
	{
		if (!listInitialised)
		{
			// Do this here the first time to avoid pure virtual function call...
			refreshConnectionList();
			listInitialised = true;
		}

		auto bounds = getParentShell()->getContentBounds();

		if (bounds.isEmpty())
			return;

		const bool scb = getStyleProperty(showConnectionBar, true);

		Rectangle<int> contentArea = getParentShell()->getContentBounds();

		

		if (scb)
		{
			auto topArea = contentArea.removeFromTop(18);

			topArea.removeFromLeft(topArea.getHeight());
			followWorkspaceButton.setBounds(topArea.removeFromLeft(topArea.getHeight()).reduced(2));

			connectionSelector->setVisible(!getParentShell()->isFolded());
			connectionSelector->setBounds(topArea.removeFromLeft(128));;
			topArea.removeFromLeft(5);
			indexSelector->setVisible(!getParentShell()->isFolded() && hasSubIndex());
			indexSelector->setBounds(topArea.removeFromLeft(128));
		}
		else
		{
			connectionSelector->setVisible(false);
		}

		if (content != nullptr)
		{
			if (getHeight() > 18)
			{
				content->setVisible(true);
				content->setBounds(contentArea);
			}
			else
				content->setVisible(false);
		}
	}
}

void PanelWithProcessorConnection::comboBoxChanged(ComboBox* comboBoxThatHasChanged)
{
    preSelectCallback(comboBoxThatHasChanged);
    
	if (comboBoxThatHasChanged == connectionSelector)
	{
		indexSelector->clear(dontSendNotification);
		setConnectionIndex(-1);

		if (connectionSelector->getSelectedId() == 1)
		{
			setCurrentProcessor(nullptr);
			refreshContent();
		}
		else
		{
			const String id = comboBoxThatHasChanged->getText();

			auto p = ProcessorHelpers::getFirstProcessorWithName(getMainSynthChain(), id);

			connectedProcessor = p;

			if (hasSubIndex())
			{
				refreshIndexList();
				setContentWithUndo(p, 0);
			}
			else
			{
				setConnectionIndex(-1);
				setContentWithUndo(p, -1);
			}
		}
	}
	else if (comboBoxThatHasChanged == indexSelector)
	{
		int newIndex = -1;

		if (indexSelector->getSelectedId() != 1)
		{
			newIndex = indexSelector->getSelectedId() - 2;
			setContentWithUndo(connectedProcessor.get(), newIndex);
		}
		else
		{
			setConnectionIndex(newIndex);
			refreshContent();
		}
	}
}

void PanelWithProcessorConnection::processorDeleted(Processor* /*deletedProcessor*/)
{
	jassert_message_thread;

	setContentWithUndo(nullptr, -1);
}

bool PanelWithProcessorConnection::shouldHideSelector() const
{
	if (forceHideSelector)
		return true;

#if USE_BACKEND
	return findParentComponentOfClass<ScriptContentComponent>() != nullptr ||
		findParentComponentOfClass<MarkdownPreview>() != nullptr;
#else
	return true;
#endif
}

void PanelWithProcessorConnection::refreshConnectionList()
{
	auto f = [](WeakReference<PanelWithProcessorConnection> tmp)
	{
		if (tmp.get() != nullptr)
		{
			String currentId = tmp->connectionSelector->getText();
			tmp->connectionSelector->clear(dontSendNotification);

			StringArray items;

			tmp->refreshSelector(items, currentId);
		}

		return true;
	};

	f(this);

	//getMainController()->getLockFreeDispatcher().callOnMessageThreadWhenIdle<PanelWithProcessorConnection>(this, f);
}

void PanelWithProcessorConnection::refreshSelector(StringArray &items, String currentId)
{
	fillModuleList(items);

	

	connectionSelector->addItem("Disconnect", 1);
	connectionSelector->addItemList(items, 2);

    int index = items.indexOf(currentId);
    
    if (index != -1)
        connectionSelector->setSelectedId(index + 2, dontSendNotification);
}

void PanelWithProcessorConnection::refreshSelectorValue(Processor* p, String currentId)
{
    StringArray items;
    fillIndexList(items);
    int index = items.indexOf(currentId);
    
    if (index != -1)
    {
        currentIndex = index;
        indexSelector->setSelectedId(index + 2, dontSendNotification);
        
        setCustomTitle(currentId);
        refreshTitle();
    }
}

void PanelWithProcessorConnection::refreshIndexList()
{
	String currentId = indexSelector->getText();

	indexSelector->clear(dontSendNotification);

	StringArray items;

	fillIndexList(items);

	int index = items.indexOf(currentId);

	indexSelector->addItem("Disconnect", 1);
	indexSelector->addItemList(items, 2);

	if (index != -1)
		indexSelector->setSelectedId(index + 2, dontSendNotification);
}

ModulatorSynthChain* PanelWithProcessorConnection::getMainSynthChain()
{
	return getMainController()->getMainSynthChain();
}

const ModulatorSynthChain* PanelWithProcessorConnection::getMainSynthChain() const
{
	return getMainController()->getMainSynthChain();
}

void PanelWithProcessorConnection::changeContentWithUndo(int newIndex)
{
	if (currentIndex != newIndex)
	{
		setContentWithUndo(currentProcessor, newIndex);
	}
}

void PanelWithProcessorConnection::setContentWithUndo(Processor* newProcessor, int newIndex)
{
	StringArray indexes;
	fillIndexList(indexes);

	refreshIndexList();
    
	ScopedPointer<ProcessorConnection> connection = new ProcessorConnection(this, newProcessor, newIndex, getAdditionalUndoInformation());

	connection->perform();

	connection = nullptr;

	if (newIndex != -1)
	{
		indexSelector->setSelectedId(newIndex + 2, dontSendNotification);
	}

}

void PanelWithProcessorConnection::refreshTitle()
{
	auto titleToUse = hasCustomTitle() ? getCustomTitle() : getTitle();

	if (getProcessor() && !hasCustomTitle())
	{
		titleToUse << ": " << getConnectedProcessor()->getId();
	}

	setDynamicTitle(titleToUse);

	resized();
	repaint();
}

void PanelWithProcessorConnection::refreshContent()
{
	if (getConnectedProcessor())
		connectionSelector->setText(getConnectedProcessor()->getId(), dontSendNotification);
	else
		connectionSelector->setSelectedId(1, dontSendNotification);

	indexSelector->setSelectedId(currentIndex + 2, dontSendNotification);

	if (getProcessor() == nullptr || (hasSubIndex() && currentIndex == -1))
	{
		content = nullptr;
	}
	else
	{
		getProcessor()->addDeleteListener(this);

		content = nullptr;
		content = createContentComponent(currentIndex);
			
		if(content != nullptr)
			addAndMakeVisible(content);
	}

	refreshTitle();
		

	contentChanged();
}

PanelWithProcessorConnection::ProcessorConnection::ProcessorConnection(PanelWithProcessorConnection* panel_, Processor* newProcessor_, int newIndex_, var additionalInfo_) :
	panel(panel_),
	newProcessor(newProcessor_),
	newIndex(newIndex_),
	additionalInfo(additionalInfo_)
{
	oldIndex = panel->currentIndex;
	oldProcessor = panel->currentProcessor.get();
}

bool PanelWithProcessorConnection::ProcessorConnection::perform()
{
	if (panel.getComponent() != nullptr)
	{
		panel->setCurrentProcessor(newProcessor.get());
		panel->refreshIndexList();

		if(newIndex != -1)
			panel->currentIndex = newIndex;
		
		panel->refreshContent();
		
		return true;
	}

	return false;
}

bool PanelWithProcessorConnection::ProcessorConnection::undo()
{
	if (panel.getComponent())
	{
		panel->currentIndex = oldIndex;
		panel->setCurrentProcessor(oldProcessor.get());
		panel->refreshContent();
		panel->performAdditionalUndoInformation(additionalInfo);
		return true;
	}

	return false;
}

void PanelWithProcessorConnection::setContentForIdentifier(Identifier idToSearch)
    {
        auto parentContainer = getParentShell()->getParentContainer();
        
        if (parentContainer != nullptr)
        {
            FloatingTile::Iterator<PanelWithProcessorConnection> iter(parentContainer->getParentShell());
            
            while (auto p = iter.getNextPanel())
            {
                if (p == this)
                    continue;
                
                if (p->getProcessorTypeId() != idToSearch)
                    continue;
                
				auto currentIndex = jmax(0, p->getCurrentIndex());

                p->setContentWithUndo(getProcessor(), currentIndex);
            }
        }
    }

bool PanelWithProcessorConnection::showTitleInPresentationMode() const
{
	return !forceHideSelector;
}

void PanelWithProcessorConnection::setCurrentProcessor(Processor* p)
{
	if (currentProcessor.get() != nullptr)
	{
		currentProcessor->removeDeleteListener(this);
	}

	currentProcessor = p;
	connectedProcessor = currentProcessor;
}

void PanelWithProcessorConnection::setConnectionIndex(int newIndex)
{
	currentIndex = newIndex;
}

void PanelWithProcessorConnection::setForceHideSelector(bool shouldHide)
{
	forceHideSelector = shouldHide;
		
}

bool PanelWithProcessorConnection::shouldFollowNewWorkspace(Processor* p, const Identifier& id) const
{
	return followWorkspaceButton.getToggleState() && id == getProcessorTypeId();
}
} // namespace hise
