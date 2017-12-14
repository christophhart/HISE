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

ScriptComponentEditPanel::ScriptComponentEditPanel(BackendRootWindow* rootWindow, Processor* p) :
	ScriptComponentEditListener(p),
	mc(rootWindow->getBackendProcessor()),
	connectedProcessor(p)
{
	addAsScriptEditListener();

	setName("Edit Script Components");

	addAndMakeVisible(idEditor = new TextEditor());
	
	idEditor->addListener(this);
	idEditor->setFont(GLOBAL_MONOSPACE_FONT());

	idEditor->setColour(TextEditor::ColourIds::backgroundColourId, Colours::white.withAlpha(0.2f));
	idEditor->setColour(TextEditor::ColourIds::focusedOutlineColourId, Colour(SIGNAL_COLOUR));
	idEditor->setColour(Label::ColourIds::outlineWhenEditingColourId, Colour(SIGNAL_COLOUR));
	idEditor->setColour(TextEditor::ColourIds::highlightColourId, Colour(SIGNAL_COLOUR));

	Colour normal = Colours::white.withAlpha(0.6f);
	Colour over = Colours::white.withAlpha(0.8f);
	Colour down = Colours::white;

	addAndMakeVisible(copyButton = new ShapeButton("Copy", normal, over, down));
	copyButton->setShape(PathFactory::createPath(PathFactory::Copy), true, true, true);
	copyButton->addListener(this);
	copyButton->setTooltip("Copy the selected properties");

	addAndMakeVisible(pasteButton = new ShapeButton("Paste", normal, over, down));
	pasteButton->setShape(PathFactory::createPath(PathFactory::Paste), true, true, true);
	pasteButton->addListener(this);
	pasteButton->setTooltip("Paste the copied properties to the selection");
	


	addAndMakeVisible(panel = new PropertyPanel());

	panel->setLookAndFeel(&pplaf);

	updateIdEditor();
}

ScriptComponentEditPanel::~ScriptComponentEditPanel()
{
	removeAsScriptEditListener();

	panel = nullptr;
}


void ScriptComponentEditPanel::addSectionToPanel(const Array<Identifier> &idList, const String &sectionName)
{
	auto b = getScriptComponentEditBroadcaster();
	


	Array<PropertyComponent*> propertyPanelList;

	for (int i = 0; i < idList.size(); i++)
	{
		Identifier id = idList[i];

		ScriptComponentEditBroadcaster::Iterator iter(b);

		bool shouldAddProperty = true;

		while (auto sc = iter.getNextScriptComponent())
		{
			if (!sc->hasProperty(id) || sc->isPropertyDeactivated(id))
			{
				shouldAddProperty = false;
				break;
			}
		}

		if (!shouldAddProperty)
			continue;

		addProperty(propertyPanelList, id);
	}

	panel->addSection(sectionName, propertyPanelList, true);


};

void ScriptComponentEditPanel::fillPanel()
{
	auto b = getScriptComponentEditBroadcaster();

	auto sc = b->getFirstFromSelection();

	
	

	panel->clear();

	setName(sc != nullptr ? ("Edit " + sc->getName().toString()) : "Edit Script Components");

	if (sc != nullptr)
	{
		Array<Identifier> basicIds;

		basicIds.add(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::text));
		basicIds.add(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::enabled));
		basicIds.add(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::visible));
		basicIds.add(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::tooltip));
		basicIds.add(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::useUndoManager));

		addSectionToPanel(basicIds, "Basic Properties");

		Array<Identifier> parameterIds;

		parameterIds.add(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::macroControl));
		parameterIds.add(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::saveInPreset));
		parameterIds.add(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::isPluginParameter));
		parameterIds.add(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::pluginParameterName));
		parameterIds.add(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::processorId));
		parameterIds.add(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::parameterId));

		addSectionToPanel(parameterIds, "Parameter Properties");

		Array<Identifier> positionIds;

		positionIds.add(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::x));
		positionIds.add(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::y));
		positionIds.add(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::width));
		positionIds.add(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::height));

		addSectionToPanel(positionIds, "Component Size");

		Array<Identifier> colourIds;

		colourIds.add(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::bgColour));
		colourIds.add(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::itemColour));
		colourIds.add(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::itemColour2));
		colourIds.add(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::textColour));

		addSectionToPanel(colourIds, "Component Colours");

		Array<Identifier> specialProperties;

		for (int i = 0; i < sc->getNumIds(); i++)
		{
			Identifier id = sc->getIdFor(i);

			if (basicIds.contains(id) || parameterIds.contains(id) || positionIds.contains(id) || colourIds.contains(id)) continue;

			specialProperties.add(id);
		}

		addSectionToPanel(specialProperties, "Component Specific Properties");

	}
}

void ScriptComponentEditPanel::rebuildWidgets()
{
	auto sc = getScriptComponentEditBroadcaster()->getFirstFromSelection();

	if (sc == nullptr)
		return;

	updateIdEditor();
}

void ScriptComponentEditPanel::addProperty(Array<PropertyComponent*> &arrayToAddTo, const Identifier &id)
{
	ScriptComponentPropertyTypeSelector::SelectorTypes t = ScriptComponentPropertyTypeSelector::getTypeForId(id);

	static const Identifier pc("parentComponent");

	if (id == pc)
		return;

	if (t == ScriptComponentPropertyTypeSelector::SliderSelector)
	{
		HiSliderPropertyComponent *slider = new HiSliderPropertyComponent(id, this);

		arrayToAddTo.add(slider);

		slider->setLookAndFeel(&pplaf);
	}
	else if (t == ScriptComponentPropertyTypeSelector::ChoiceSelector) // All combobox properties
	{
		auto b = new HiChoicePropertyComponent(id, this);
		b->setLookAndFeel(&pplaf);

		arrayToAddTo.add(b);
		
	}
	else if (t == ScriptComponentPropertyTypeSelector::ColourPickerSelector) // All colour properties
	{
		arrayToAddTo.add(new HiColourPropertyComponent(id, this));
		arrayToAddTo.getLast()->setLookAndFeel(&pplaf);
	}
	else if (t == ScriptComponentPropertyTypeSelector::FileSelector) // File list
	{
		arrayToAddTo.add(new HiFilePropertyComponent(id, this));
		arrayToAddTo.getLast()->setLookAndFeel(&pplaf);
	}

	else if (t == ScriptComponentPropertyTypeSelector::ToggleSelector) // All toggle properties
	{
		arrayToAddTo.add(new HiTogglePropertyComponent(id, this));
		arrayToAddTo.getLast()->setLookAndFeel(&pplaf);
	}
	else
	{
		arrayToAddTo.add(new HiTextPropertyComponent(id, this, t == ScriptComponentPropertyTypeSelector::MultilineSelector));
		arrayToAddTo.getLast()->setLookAndFeel(&pplaf);
	}


}


void ScriptComponentEditPanel::updateIdEditor()
{
	auto b = getScriptComponentEditBroadcaster();

	int numSelected = b->getNumSelected();

	if (numSelected == 0)
	{
		idEditor->setReadOnly(true);
		idEditor->setText("Nothing selected", dontSendNotification);
	}
	else if (numSelected == 1)
	{
		idEditor->setReadOnly(false);
		idEditor->setText(b->getFirstFromSelection()->getName().toString(), dontSendNotification);
	}
	else
	{
		idEditor->setText("*", dontSendNotification);
		idEditor->setReadOnly(true);
	}
}

void ScriptComponentEditPanel::scriptComponentSelectionChanged()
{
	updateIdEditor();
	fillPanel();
}

void ScriptComponentEditPanel::scriptComponentPropertyChanged(ScriptComponent* sc, Identifier /*idThatWasChanged*/, const var& /*newValue*/)
{
	if (getScriptComponentEditBroadcaster()->isFirstComponentInSelection(sc))
	{
		panel->refreshAll();
	}
}

void ScriptComponentEditPanel::textEditorReturnKeyPressed(TextEditor& t)
{
	auto b = getScriptComponentEditBroadcaster();

	jassert(b->getNumSelected() == 1);

	auto sc = b->getFirstFromSelection();

	if (sc != nullptr)
	{
		auto newName = t.getText().trim().removeCharacters(" \t\n");

		if (Identifier::isValidIdentifier(newName))
		{
			ScriptingApi::Content::Helpers::renameComponent(sc->parent, sc->name, Identifier(t.getText()));
		}
		else
		{
			PresetHandler::showMessageWindow("Invalid ID", "The ID you've entered is not a valid variable name. Use CamelCase without whitespace");
		}
	}
}

void ScriptComponentEditPanel::buttonClicked(Button* b)
{
	if (b == copyButton)
	{
		copyAction();
	}
	if (b == pasteButton)
	{
		pasteAction();
	}
}

void ScriptComponentEditPanel::paint(Graphics &g)
{
	auto total = getLocalBounds();

	auto topRow = total.removeFromTop(24);
	g.setColour(Colours::black.withAlpha(JUCE_LIVE_CONSTANT_OFF(0.2f)));

	g.fillRect(topRow);

    PopupLookAndFeel::drawFake3D(g, topRow);
    
	g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xff262626)));
	g.fillRect(total);

	g.setFont(GLOBAL_BOLD_FONT());
	g.setColour(Colours::white);
	g.drawText("ID", 0, 0, 24, 24, Justification::centred);

	CopyPasteTarget::paintOutlineIfSelected(g);
}

void ScriptComponentEditPanel::resized()
{
	auto b = getLocalBounds();
	auto topRow = b.removeFromTop(24);
	topRow.removeFromLeft(24);

	const int numButtons = 2;

	auto buttonArea = topRow.removeFromRight(numButtons * 24);

	idEditor->setBounds(topRow.withHeight(23));

	copyButton->setBounds(buttonArea.removeFromLeft(24).reduced(3));
	
	pasteButton->setBounds(buttonArea.removeFromLeft(24).reduced(3));

	
	
	panel->setBounds(b);
}

void ScriptComponentEditPanel::copyAction()
{
	auto b = getScriptComponentEditBroadcaster();

	auto sc = b->getFirstFromSelection();

	if (sc != nullptr)
	{
		DynamicObject::Ptr newObject = new DynamicObject();

		String properties;
		NewLine nl;

		if (selectedComponents.getNumSelected() == 0)
		{
			PresetHandler::showMessageWindow("Nothing selected", "You need to select properties by clicking on their name", PresetHandler::IconType::Error);
			return;
		}

		for (auto p : selectedComponents)
		{
			if (p.getComponent() == nullptr)
				return;

			auto id = p->getId();

			auto value = sc->getScriptObjectProperty(id);

			properties << id.toString() << nl;

			newObject->setProperty(id, value);
		}

		var newData(newObject);

		auto clipboardContent = JSON::toString(newData, false);
		SystemClipboard::copyTextToClipboard(clipboardContent);

		debugToConsole(mc->getMainSynthChain(), "The following properties were copied to the clipboard:\n" + properties);

	}
}

void ScriptComponentEditPanel::pasteAction()
{
	auto clipboardContent = SystemClipboard::getTextFromClipboard();

	var parsedJson;

	auto result = JSON::parse(clipboardContent, parsedJson);

	if (result.wasOk())
	{
		auto set = parsedJson.getDynamicObject()->getProperties();

		auto b = getScriptComponentEditBroadcaster();

		ScriptComponentEditBroadcaster::Iterator iter(b);

		auto& undoManager = b->getUndoManager();

		undoManager.beginNewTransaction("Paste properties");

		while (auto sc = iter.getNextScriptComponent())
		{
			auto vt = sc->getPropertyValueTree();

			for (int i = 0; i < set.size(); i++)
			{
				vt.setProperty(set.getName(i), set.getValueAt(i), &undoManager);
			}
		}
	}
}

void ScriptComponentEditPanel::debugProperties(DynamicObject *properties)
{
	ScopedPointer<XmlElement> xml = new XmlElement("xml");

	properties->getProperties().copyToXmlAttributes(*xml);


}

Identifier ScriptComponentEditPanel::Panel::getProcessorTypeId() const
{
	return JavascriptProcessor::getConnectorId();
}

Component* ScriptComponentEditPanel::Panel::createContentComponent(int /*index*/)
{
	auto rootWindow = getParentShell()->getRootFloatingTile()->findParentComponentOfClass<BackendRootWindow>();

	jassert(rootWindow != nullptr);

	return new ScriptComponentEditPanel(rootWindow, getConnectedProcessor());
}

} // namespace hise
