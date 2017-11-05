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
*   which must be separately licensed for cloused source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/



ScriptComponentEditPanel::ScriptComponentEditPanel(BackendRootWindow* rootWindow, Processor* p) :
	ScriptComponentEditListener(p->getMainController()),
	mc(rootWindow->getBackendProcessor()),
	connectedProcessor(p)
{
	addAsScriptEditListener();

	setName("Edit Script Components");

	addAndMakeVisible(copyFromComponent = new ComboBox());
	copyFromComponent->setLookAndFeel(&pplaf);
	copyFromComponent->setTextWhenNothingSelected("Copy properties from other component");
	copyFromComponent->setTextWhenNoChoicesAvailable("No components to copy from available");

	addAndMakeVisible(panel = new PropertyPanel());

	panel->setLookAndFeel(&pplaf);
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
	copyFromComponent->clear(dontSendNotification);

	auto sc = getScriptComponentEditBroadcaster()->getFirstFromSelection();

	if (sc == nullptr)
		return;

	auto content = sc->parent;

	for (int i = 0; i < content->getNumComponents(); i++)
	{
		auto c = content->getComponent(i);

		if (c == sc)
			continue;

		auto n = c->getName().toString();
		copyFromComponent->addItem(n, i + 1);
	}
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
#if 0
	else if (t == ScriptComponentPropertyTypeSelector::ColourPickerSelector) // All colour properties
	{

		int64 colour = (int64)obj->getProperty(id);

		Colour c((uint32)colour);

		arrayToAddTo.add(new HiColourPropertyComponent(id.toString(), false, c, this));
		dynamic_cast<HiColourPropertyComponent*>(arrayToAddTo.getLast())->addChangeListener(this);
		arrayToAddTo.getLast()->setLookAndFeel(&pplaf);
	}
#endif
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


void ScriptComponentEditPanel::scriptComponentSelectionChanged()
{
	fillPanel();
}

void ScriptComponentEditPanel::scriptComponentPropertyChanged(ScriptComponent* sc, Identifier idThatWasChanged, const var& newValue)
{
	if (getScriptComponentEditBroadcaster()->isFirstComponentInSelection(sc))
	{
		panel->refreshAll();
	}
}

void ScriptComponentEditPanel::resized()
{
	Rectangle<int> b = getLocalBounds();

	copyFromComponent->setBounds(b.removeFromTop(40).reduced(8));
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

Component* ScriptComponentEditPanel::Panel::createContentComponent(int index)
{
	auto rootWindow = getParentShell()->getRootFloatingTile()->findParentComponentOfClass<BackendRootWindow>();

	jassert(rootWindow != nullptr);

	return new ScriptComponentEditPanel(rootWindow, getConnectedProcessor());
}
