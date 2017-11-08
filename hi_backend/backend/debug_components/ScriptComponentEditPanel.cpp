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
			if (!sc->getScriptObjectProperties()->hasProperty(id) ||
				sc->isPropertyDeactivated(id))
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
		basicIds.add(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::parentComponent));
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

	

	if (t == ScriptComponentPropertyTypeSelector::SliderSelector)
	{
		HiSliderPropertyComponent *slider = new HiSliderPropertyComponent(id, this);

		ScriptComponentPropertyTypeSelector::SliderRange range = ScriptComponentPropertyTypeSelector::getRangeForId(id);

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
		idEditor->setText("Multiple elements selected", dontSendNotification);
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

void ScriptComponentEditPanel::resized()
{
	Rectangle<int> b = getLocalBounds();

	idEditor->setBounds(b.removeFromTop(40).reduced(8));
	panel->setBounds(b);
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