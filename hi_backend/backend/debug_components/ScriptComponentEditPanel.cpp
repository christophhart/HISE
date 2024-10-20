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

MARKDOWN_CHAPTER(PropertyPanelHelp)
START_MARKDOWN(Help)
ML("# Property Editor");
ML("This panel can be used to change the properties of the currently selected items with full undo support.");
ML("As soon as you have selected multiple elements, changing the property will set all controls to the given value(with the exception of the position sliders which operate relatively).If the selection has varying values for a property, it will show an asterisk **(*)** instead of the real value indicating that you are editing multiple values");

ML("## Changing the ID");
ML("The textbox at the top row can be used to change the ID of the element.**");
ML("> Remember that the ID is **ridiculously important** for almost everything - preset restoring, creating references to scripts, resolving the parent hierarchy etc. Try to pick a name once and stick to it because changing the ID of a control will become more and more painful over the time.");
ML("As soon as you have selected multiple elements, this text box becomes disabled.");

ML("## Property List");

ML("| Property | Type | Default Value | Description |");
ML("| ------ | ---- | ----- | --------------- |")
ML("| `text` | String | the ID of the control | The name or text that is being displayed.How this text is exactly shown(or if it's used at all) depends on the control type. |");
ML("| `enabled`| Boolean | true | If the control should react on mouse events.Child components will inherit this property. |");
ML("| `visible` | Boolean | true | If the control is displayed or hidden.Child components will inherit this property. |");
ML("| `tooltip` | String | Empty | A informative text that will popup if you hover over the control. |");
ML("| `useUndoManager` | Boolean | false | If enabled, value changes can be undone with the scripting calls `Engine.undo()` |");
ML("| `macroControl` | Number(1 - 8) | -1 | Connect this control to a macro control slot. |");
ML("| `linkedTo` | String | Empty | This property can be used to link certain controls to another control (the value must be the ID of the other control. In this case it will simply mirror the other one and can be used to duplicate controls on different pages. |");
ML("| `saveInPreset` | Boolean | Depends on the type | If true, this control will be saved in a user preset as well as restored on recompilation.If false, controls will not be stored in the preset and their control callback will not be fired after compilation.This is a very important property and you definitely need to know when to use it. |");
ML("| `isPluginParameter` | Boolean | false | If enabled, it exposes this control to a DAW for host automation. |");
ML("| `pluginParameterName` | String | Empty | If this control is a plugin parameter, it will use this name for displaying in the host. |");
ML("| `isMetaParameter` | Boolean | false | If this control is a plugin parameter and causes other parameters to change their values (eg. a sync button that changes the values of the delay time knob), you'll have to set this to true in order to be fully standard compliant (Logic is known to cause issues when this isn't handled properly). |");
ML("| `processorId` | Module ID | Empty | The module that is controlled by this control. |");
ML("| `parameterId` | Parameter ID | Empty | the parameter ID of the module specified above that should be controlled.Use these two properties in order to hook up the control to a single parameter using the exact same range you specified below.As soon as you need something more complex, you need to use the scripting callbacks for it. |");
ML("| `defaultValue` | Number | 0.0 | The default value for the control (if available). This value will be used at initialisation and if you load a user preset that has no stored value for this particular control (which happens if you add a control and try to load a user preset built with an older version). Sliders / Knobs will also use this as double click value. |");
ML("| `x`, `y`, `width`, `height` | Number | Various | The absolute pixel position / size of the control.You can use the sliders to change them relatively or just input a number into the text field to set all selected controls to the same value. |");
ML("| `bgColour`, `itemColour`, `itemColour2`, `textColour` | String or hex number | Various | the colours for the given control.How these colours are used differs between the control types, but in most cases, `bgColour` is the background colour and `textColour` is used for rendering the text, otherwise it would be a bit weird. |");

ML("## Copying multiple properties to a selection");
ML("- Select one control and set all its properties how you need them.");
ML("- Now select the properties by clicking on the property name. You'll notice it will be highlighted. Use the usual modifier keys for selecting multiple properties.");
ML("- Click on the **Copy** button to create a JSON containing all selected properties with their value. ");
ML("- Select all controls that you want to paste the property selection.");
ML("- Click on the **Paste** button and all selected properties will be pasted for the entire selection.");
END_MARKDOWN()
END_MARKDOWN_CHAPTER()

struct PropertyPanelWithoutText: public PropertyPanel
{
    void paint(Graphics& g) override {};
};

ScriptComponentEditPanel::ScriptComponentEditPanel(MainController* mc_, Processor* p) :
	ScriptComponentEditListener(p),
	mc(mc_),
	connectedProcessor(p)
{
	addAsScriptEditListener();

	setName("Edit Script Components");

	addAndMakeVisible(idEditor = new TextEditor());
	
	idEditor->addListener(this);
	
	GlobalHiseLookAndFeel::setTextEditorColours(*idEditor);

	Colour normal = Colours::white.withAlpha(0.6f);
	Colour over = Colours::white.withAlpha(0.8f);
	Colour down = Colours::white;

	Factory f;

	addAndMakeVisible(copyButton = new ShapeButton("Copy", normal, over, down));
	copyButton->setShape(f.createPath("Copy"), true, true, true);
	copyButton->addListener(this);
	copyButton->setTooltip("Copy selected properties as JSON");

	addAndMakeVisible(pasteButton = new ShapeButton("Paste", normal, over, down));
	pasteButton->setShape(f.createPath("Paste"), true, true, true);
	pasteButton->addListener(this);
	pasteButton->setTooltip("Paste the copied properties to the selection");
	
	addAndMakeVisible(helpButton = new MarkdownHelpButton());
	helpButton->setPopupWidth(600);
	helpButton->setHelpText<PathProvider<Factory>>(PropertyPanelHelp::Help());

	addAndMakeVisible(panel = new PropertyPanelWithoutText());

    auto& sb = panel->getViewport().getVerticalScrollBar();
    
    panel->getViewport().setScrollBarThickness(13);
    
    sf.addScrollBarToAnimate(sb);
    
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

		parameterIds.add(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::linkedTo));
		parameterIds.add(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::macroControl));
		parameterIds.add(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::saveInPreset));
		parameterIds.add(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::isPluginParameter));
		parameterIds.add(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::pluginParameterName));
    parameterIds.add(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::isMetaParameter));
		parameterIds.add(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::processorId));
		parameterIds.add(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::parameterId));
		parameterIds.add(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::automationId));
		parameterIds.add(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::defaultValue));

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
		
		if (dynamic_cast<ScriptingApi::Content::ScriptFloatingTile*>(sc) != nullptr)
		{
			colourIds.add(sc->getIdFor(ScriptingApi::Content::ScriptFloatingTile::Properties::itemColour3));
		}
		
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

void ScriptComponentEditPanel::rebuildScriptedComponents()
{
	auto sc = getScriptComponentEditBroadcaster()->getFirstFromSelection();

	if (sc == nullptr)
		return;

	updateIdEditor();
}

void ScriptComponentEditPanel::addProperty(Array<PropertyComponent*> &arrayToAddTo, const Identifier &id)
{
	SharedResourcePointer<ScriptComponentPropertyTypeSelector> ptr;

	ScriptComponentPropertyTypeSelector::SelectorTypes t = ptr->getTypeForId(id);

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
	else if (t == ScriptComponentPropertyTypeSelector::CodeSelector) // add code editor
	{
		arrayToAddTo.add(new HiCodeEditorPropertyComponent(id, this));
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
		Component::SafePointer<PropertyPanel> tmp(panel);

		auto f = [tmp]()
		{
			if(tmp)
				tmp->refreshAll();
		};

		new DelayedFunctionCaller(f, 300);
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
	helpButton->setBounds(topRow.removeFromLeft(24).reduced(3));

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
		auto newObject = new DynamicObject();
		var newData(newObject);

		String prop;
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

			prop << id.toString() << nl;

			newObject->setProperty(id, value);
		}

		

		auto clipboardContent = JSON::toString(newData, false, DOUBLE_TO_STRING_DIGITS);
		SystemClipboard::copyTextToClipboard(clipboardContent);

		debugToConsole(mc->getMainSynthChain(), "The following properties were copied to the clipboard:\n" + prop);

	}
}

void ScriptComponentEditPanel::pasteAction()
{
	auto clipboardContent = SystemClipboard::getTextFromClipboard();

	var parsedJson;

	auto result = JSON::parse(clipboardContent, parsedJson);

	if (result.wasOk() && parsedJson.getDynamicObject() != nullptr)
	{
		auto set = parsedJson.getDynamicObject()->getProperties();

		auto b = getScriptComponentEditBroadcaster();

		ScriptComponentEditBroadcaster::Iterator iter(b);

		auto& undoManager = b->getUndoManager();

		while (auto sc = iter.getNextScriptComponent())
		{
			auto vt = sc->getPropertyValueTree();

			

			for (int i = 0; i < set.size(); i++)
			{
				// Just for the undo...
				vt.setProperty(set.getName(i), set.getValueAt(i), &undoManager);
			}

			ScriptComponent::ScopedPropertyEnabler spe(sc);

			sc->setPropertiesFromJSON(parsedJson);

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
	return new ScriptComponentEditPanel(getMainController(), getConnectedProcessor());
}

} // namespace hise
