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

ScriptWatchTable::ScriptWatchTable(MainController *mc, BaseDebugArea *area) :
	AutoPopupDebugComponent(area),
	font (GLOBAL_FONT()),
	controller(mc)
{
	setName(getHeadline());

    // Create our table component and add it to this component..
    addAndMakeVisible (table = new TableListBox());
    table->setModel (this);

	table->getHeader().setLookAndFeel(&laf);

	table->getHeader().setSize(getWidth(), 22);

    // give it a border
    
    table->setOutlineThickness (0);

	table->getViewport()->setScrollBarsShown(true, false, false, false);

    table->setColour(ListBox::backgroundColourId, Colour(DEBUG_AREA_BACKGROUND_COLOUR));

    
	//table.getHeader().setInterceptsMouseClicks(false, false);

	table->getHeader().addColumn("Type", Type, 100);
	table->getHeader().addColumn("Name", Name, 100);
	table->getHeader().addColumn("Value", Value, 180);

	table->addMouseListener(this, true);
}


void ScriptWatchTable::timerCallback()
{
	if(table != nullptr)
    {
        table->updateContent();

        refreshStrippedSet();
        
        repaint();
    }
}

int ScriptWatchTable::getNumRows() 
{
	return strippedSet.size();
};

void ScriptWatchTable::refreshStrippedSet()
{
	NamedValueSet oldSet = NamedValueSet(strippedSet);

	changed.clear();

	strippedSet.clear();

	fillWithGlobalVariables();

	if(processor.get() == nullptr)
	{
		setScriptProcessor(nullptr, nullptr);
		return;
	}
	else
	{
		fillWithLocalVariables();

	}

	refreshChangedDisplay(oldSet);

};

void ScriptWatchTable::setScriptProcessor(ScriptProcessor *p, ScriptingEditor *editor_)
{
	processor = static_cast<Processor*>(p);
	editor = editor_;

	setName(getHeadline());

	if(processor.get() != nullptr)
	{
		showComponentInDebugArea(true);
		startTimer(400);
	}
	else
	{
		showComponentInDebugArea(false);
		strippedSet.clear();
		
		table->updateContent();

		stopTimer();
		repaint();
	}

	if(getParentComponent() != nullptr) getParentComponent()->repaint();

}
	
void ScriptWatchTable::paintRowBackground (Graphics& g, int rowNumber, int /*width*/, int /*height*/, bool rowIsSelected) 
{
	if(rowNumber % 2) g.fillAll(Colours::white.withAlpha(0.05f));

    if (rowIsSelected)
        g.fillAll (Colour(0x44000000));
}

void ScriptWatchTable::selectedRowsChanged(int /*lastRowSelected*/) {};

void ScriptWatchTable::paintCell (Graphics& g, int rowNumber, int columnId,
                int width, int height, bool /*rowIsSelected*/) 
{
	g.setColour(Colours::black.withAlpha(0.1f));

	g.drawHorizontalLine(0, 0.0f, (float)width);
	
	g.setColour (Colours::white.withAlpha(.8f));
    g.setFont (font);

    if(processor.get() != nullptr)
	{
        String text;

		switch(columnId)
		{
		case Name:

			if (rowNumber < strippedSet.size())
			{
				g.setFont(GLOBAL_MONOSPACE_FONT());

				text << strippedSet.getName(rowNumber).toString().replaceCharacter(':', '.');
			}

			
			break;

		case Type:

			if (rowNumber < strippedSet.size())
			{
				g.setFont(GLOBAL_MONOSPACE_FONT());
				text << JavascriptCodeEditor::getValueType(strippedSet.getValueAt(rowNumber));
			}

			break;

		case Value:

			if (rowNumber < strippedSet.size())
			{
				String value;

				if (strippedSet.getValueAt(rowNumber).isObject() && dynamic_cast<CreatableScriptObject*>(strippedSet.getValueAt(rowNumber).getDynamicObject()) != nullptr)
				{
					value = dynamic_cast<CreatableScriptObject*>(strippedSet.getValueAt(rowNumber).getDynamicObject())->getInstanceName();
				}
				else
				{
					value = strippedSet.getValueAt(rowNumber).toString();
				}

				text << value;

				if (changed[rowNumber])
				{
					g.setColour(Colours::red);
				}
			}
		}

		g.drawText (text, 5, 0, width - 10, height, Justification::centredLeft, true);
	}
}

String ScriptWatchTable::getHeadline() const
{  
	String x;
        
	x << "Watch Script Variable : " << (processor.get() == nullptr ? "Idle" : processor.get()->getId());
	return x;
}

    
void ScriptWatchTable::resized()
{
    table->setBounds(getLocalBounds());
}


void ScriptComponentEditPanel::addSectionToPanel(const Array<Identifier> &idList, const String &sectionName)
{
	ScriptingApi::Content::ScriptComponent *sc = dynamic_cast<ScriptingApi::Content::ScriptComponent *>(editedComponent.get());
	DynamicObject *properties = sc->getScriptObjectProperties();

	Array<PropertyComponent*> propertyPanelList;

	for(int i = 0; i < idList.size(); i++)
	{
		Identifier id = idList[i];

		if(sc->isPropertyDeactivated(id)) continue;

		StringArray options = sc->getOptionsFor(id);

		addProperty(propertyPanelList, properties, id, options);
	}

	panel->addSection(sectionName, propertyPanelList, true);


};


void ScriptComponentEditPanel::fillPanel()
{
	ScriptingApi::Content::ScriptComponent *sc = dynamic_cast<ScriptingApi::Content::ScriptComponent *>(editedComponent.get());

	

	setName(sc != nullptr ? ("Edit " + sc->getName().toString()) : "Edit Script Components");

	if(sc != nullptr)
	{
		Array<Identifier> basicIds;

		basicIds.add(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::text));
		basicIds.add(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::enabled));
		basicIds.add(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::visible));
		basicIds.add(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::tooltip));
		basicIds.add(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::macroControl));
		basicIds.add(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::zOrder));
		basicIds.add(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::saveInPreset));

		addSectionToPanel(basicIds, "Basic Properties");

		Array<Identifier> positionIds;

		positionIds.add(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::x));
		positionIds.add(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::y));
		positionIds.add(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::width));
		positionIds.add(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::height));

		addSectionToPanel(positionIds, "Component Position");

		Array<Identifier> colourIds;

		colourIds.add(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::bgColour));
		colourIds.add(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::itemColour));
		colourIds.add(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::itemColour2));
		colourIds.add(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::textColour));

		addSectionToPanel(colourIds, "Component Colours");

		Array<Identifier> specialProperties;

		for(int i = 0; i < sc->getNumIds(); i++)
		{
			Identifier id = sc->getIdFor(i);

			if(basicIds.contains(id) || positionIds.contains(id) || colourIds.contains(id)) continue;

			specialProperties.add(id);
		}

		addSectionToPanel(specialProperties, "Component Specific Properties");

	}
}

void ScriptComponentEditPanel::addProperty(Array<PropertyComponent*> &arrayToAddTo, DynamicObject *obj, const Identifier &id, const StringArray &options)
{
	ScriptComponentPropertyTypeSelector::SelectorTypes t = ScriptComponentPropertyTypeSelector::getTypeForId(id);

	if(t == ScriptComponentPropertyTypeSelector::ColourPickerSelector) // All colour properties
	{
		int64 colour = (int64)obj->getProperty(id);

		Colour c((uint32)colour);

		arrayToAddTo.add(new HiColourPropertyComponent(id.toString(), false, c, this));
		dynamic_cast<HiColourPropertyComponent*>(arrayToAddTo.getLast())->addChangeListener(this);
		arrayToAddTo.getLast()->setLookAndFeel(&pplaf);
	}
	else if(t == ScriptComponentPropertyTypeSelector::FileSelector) // File list
	{
		arrayToAddTo.add(new HiFilePropertyComponent(obj, id, options, this));
		dynamic_cast<HiFilePropertyComponent*>(arrayToAddTo.getLast())->addChangeListener(this);
		arrayToAddTo.getLast()->setLookAndFeel(&pplaf);
	}
	else if (t == ScriptComponentPropertyTypeSelector::ChoiceSelector) // All combobox properties
	{
		arrayToAddTo.add(new HiChoicePropertyComponent(obj, id, options, this));
		dynamic_cast<HiChoicePropertyComponent*>(arrayToAddTo.getLast())->addChangeListener(this);
		arrayToAddTo.getLast()->setLookAndFeel(&pplaf);
	}
	else if (t == ScriptComponentPropertyTypeSelector::ToggleSelector) // All toggle properties
	{
		arrayToAddTo.add(new HiTogglePropertyComponent(obj, id, this));
		dynamic_cast<HiTogglePropertyComponent*>(arrayToAddTo.getLast())->addChangeListener(this);
		arrayToAddTo.getLast()->setLookAndFeel(&pplaf);
	}
	else if (t == ScriptComponentPropertyTypeSelector::SliderSelector)
	{
		HiSliderPropertyComponent *slider = new HiSliderPropertyComponent(obj, id, this);

		ScriptComponentPropertyTypeSelector::SliderRange range = ScriptComponentPropertyTypeSelector::getRangeForId(id);

		slider->setRange(range.min, range.max, range.interval);

		arrayToAddTo.add(slider);
		slider->addChangeListener(this);
		slider->setLookAndFeel(&pplaf);
	}
	else
	{
		arrayToAddTo.add(new HiTextPropertyComponent(obj, id, this, id == Identifier("items")));
		dynamic_cast<HiTextPropertyComponent*>(arrayToAddTo.getLast())->addChangeListener(this);
		arrayToAddTo.getLast()->setLookAndFeel(&pplaf);

		if(id == Identifier("min") || id == Identifier("max"))
		{
			dynamic_cast<HiTextPropertyComponent*>(arrayToAddTo.getLast())->setUseNumberMode(true);
		}
	}


	
}

ScriptComponentEditPanel::HiSliderPropertyComponent::HiSliderPropertyComponent(DynamicObject *properties_, Identifier id_, ScriptComponentEditPanel *panel_):
	SliderPropertyComponent(id_.toString(), 0.0, 1.0, 0.0),
	id(id_),
	properties(properties_),
	panel(panel_),
	currentValue(1.0)
{
	slider.setTextBoxStyle (Slider::TextEntryBoxPosition::TextBoxLeft, false, 80, 20);
	
    slider.setColour (Slider::backgroundColourId, Colour (0xfb282828));
    slider.setColour (Slider::thumbColourId, Colour (0xff777777));
    slider.setColour (Slider::trackColourId, Colour (0xff222222));
    slider.setColour (Slider::textBoxTextColourId, Colours::white);
    slider.setColour (Slider::textBoxOutlineColourId, Colour (0x45ffffff));

	slider.setName(id.toString());

	slider.setScrollWheelEnabled(false);

	slider.addListener(panel);

	slider.setValue(properties->getProperty(id_), dontSendNotification);
};

ScriptComponentEditPanel::HiChoicePropertyComponent::HiChoicePropertyComponent(DynamicObject *properties_, Identifier id_, const StringArray &options, ScriptComponentEditPanel *panel_):
	ChoicePropertyComponent(id_.toString()),
	id(id_),
	properties(properties_),
	panel(panel_)
{
	setLookAndFeel(&plaf);
	choices.addArray(options);

	String itemName = properties->getProperty(id);

	const int index = choices.indexOf(itemName);

	setIndex(index);
};

void ScriptComponentEditPanel::HiChoicePropertyComponent::setIndex (int newIndex)
{
	currentIndex = newIndex;
	sendSynchronousChangeMessage();
}
 	
int ScriptComponentEditPanel::HiChoicePropertyComponent::getIndex () const
{
	String currentSelection = properties->getProperty(id);

	return getChoices().indexOf(currentSelection);
}

 	
String ScriptComponentEditPanel::HiChoicePropertyComponent::getItemText () const
{
	String itemText = getChoices()[currentIndex];

	if(itemText == "Load new File")
	{
		FileChooser fc("Load new File");

		if(fc.browseForFileToOpen())
		{
			return fc.getResult().getFullPathName();
		}
	}


	return getChoices()[currentIndex];
}

ScriptComponentEditPanel::ScriptComponentEditPanel(BaseDebugArea *area) :
AutoPopupDebugComponent(area)
{
	setName("Edit Script Components");

	addAndMakeVisible(panel = new PropertyPanel());
	addAndMakeVisible(codeDragger = new CodeDragger(this));

	codeDragger->setEnabled(false);

	codeDragger->setVisible(false);

	panel->setLookAndFeel(&pplaf);
}

void ScriptComponentEditPanel::sendPanelPropertyChangeMessage(Identifier idThatWasChanged)
{
	for (int i = 0; i < listeners.size(); i++)
	{
		if (listeners[i].get() != nullptr)
		{
			listeners[i]->scriptComponentChanged(editedComponent, idThatWasChanged);
		}
	}
}

void ScriptComponentEditPanel::changeListenerCallback(SafeChangeBroadcaster *b)
{
	if(editedComponent.get() == nullptr) return;

	HiPropertyComponent *hpc = dynamic_cast<HiPropertyComponent*>(b);

	jassert(hpc != nullptr);

	if(hpc != nullptr)
	{
		ScriptingApi::Content::ScriptComponent* sc = dynamic_cast<ScriptingApi::Content::ScriptComponent*>(editedComponent.get());
		sc->setScriptObjectPropertyWithChangeMessage(hpc->getId(), hpc->getValueAsVar(), sendNotification);
		sc->setChanged();
		if(dynamic_cast<HiSliderPropertyComponent*>(hpc) == nullptr && dynamic_cast<HiColourPropertyComponent*>(hpc) == nullptr) // These two will be handled only on mouse on!
		{
			sendPanelPropertyChangeMessage(hpc->getId());
		}
	}
}

void ScriptComponentEditPanel::sliderDragEnded(Slider *s)
{
	HiPropertyComponent *hpc = dynamic_cast<HiPropertyComponent*>(s->getParentComponent());

	jassert(hpc != nullptr);

	sendPanelPropertyChangeMessage(hpc->getId());
}

void ScriptComponentEditPanel::setEditedComponent(DynamicObject *o)
{
	if (editedComponent.get() != nullptr)
	{	
		sendPanelPropertyChangeMessage(Identifier());
	}
	

	showComponentInDebugArea(o != nullptr);

	panel->clear();

	editedComponent = o;

	codeDragger->setEnabled(o != nullptr);



	fillPanel();
}

void ScriptComponentEditPanel::debugProperties(DynamicObject *properties)
{
	ScopedPointer<XmlElement> xml = new XmlElement("xml");

	properties->getProperties().copyToXmlAttributes(*xml);

	
}

HiColourPropertyComponent::~HiColourPropertyComponent()
{
	//panel->sendPanelPropertyChangeMessage(getId());
}

HiColourPropertyComponent::ColourEditorComponent::ColourSelectorComp::ColourSelectorComp(ColourEditorComponent* owner_, const bool canReset) : owner(owner_),
defaultButton("Reset to Default"),
selector(ColourSelector::showAlphaChannel | ColourSelector::showColourspace | ColourSelector::showSliders)
{


	addAndMakeVisible(selector);
	selector.setName("Colour");
	selector.setCurrentColour(owner->getColour());
	selector.addChangeListener(owner);

	selector.setColour(ColourSelector::backgroundColourId, Colours::transparentBlack);

	selector.setLookAndFeel(&laf);

	if (canReset)
	{
		addAndMakeVisible(defaultButton);
		defaultButton.addListener(this);
	}

	setSize(250, 250);
}

HiColourPropertyComponent::ColourEditorComponent::ColourSelectorComp::~ColourSelectorComp()
{
	if(owner.getComponent() != nullptr)
	{
		owner->parentColourPropertyComponent->panel->sendPanelPropertyChangeMessage(owner->parentColourPropertyComponent->getId());
	}
}


String CodeDragger::getTextFromPanel()
{
	ScriptingApi::Content::ScriptComponent *sc = dynamic_cast<ScriptingApi::Content::ScriptComponent *>(panel->editedComponent.get());

	if(sc != nullptr)
	{
		String name = getNameFromPanel();

		String text;
		
		text << "// [JSON " << name << "]\n";
		
		text << "Content.setPropertiesFromJSON(\"" << name << "\", ";
		text << sc->getScriptObjectPropertiesAsJSON();
		text << ");\n";
		text << "// [/JSON " << name << "]";

		sc->setChanged(false);

		return text;
	}


	return String::empty;
}

String CodeDragger::getText(DynamicObject *scriptComponent)
{
	ScriptingApi::Content::ScriptComponent *sc = dynamic_cast<ScriptingApi::Content::ScriptComponent *>(scriptComponent);

	if(sc != nullptr)
	{
		String text;

		String name = sc->getName().toString();

		text << getTag(scriptComponent, false) << "\n";

		text << "Content.setPropertiesFromJSON(\"" << name << "\", ";
		text << sc->getScriptObjectPropertiesAsJSON();
		text << ");\n";

		sc->setChanged(false);

		text << getTag(scriptComponent, true);

		return text;
	}
	else return String::empty;

	

};

void CodeDragger::mouseDown(const MouseEvent &)
{
	String text = getTextFromPanel();

	String startTag;
	startTag << "[JSON " << getNameFromPanel() << "]";

	String endTag;
	endTag << "[/JSON " << getNameFromPanel() << "]";


	const int width = 13 * startTag.length();

	Image myImage(Image::ARGB, width, 20, true);
	Graphics g(myImage);
	g.setColour(Colours::red.withAlpha(0.3f));
	g.fillRoundedRectangle(0.f, 0.f, (float)width, 20.0f, 4.0f);
	g.setColour(Colours::black);
	g.setFont(GLOBAL_MONOSPACE_FONT());
	g.drawText(startTag, 0, 0, width, 20, Justification::centred, true);



	var description;

	description.insert(-1, getNameFromPanel());
	description.insert(-1, text);

	startDragging(description, this, myImage, true);
}

void CodeDragger::paint(Graphics &g)
{
	g.setColour(Colours::black.withAlpha(0.1f));
	g.fillAll();

	g.setColour(Colours::black.withAlpha(0.4f));
	g.drawRect(getBounds().reduced(1), 1);

	g.setColour(Colours::black);
	g.setFont(GLOBAL_BOLD_FONT());
	g.drawText("Drag JSON to CodeEditor", 0, 0, getWidth(), getHeight(), Justification::centred);
}

String CodeDragger::getTag(DynamicObject *scriptComponent, bool getEndTag)
{
	ScriptingApi::Content::ScriptComponent *sc = dynamic_cast<ScriptingApi::Content::ScriptComponent *>(scriptComponent);

	if(sc != nullptr)
	{
		String text;
		
		text << (getEndTag ? "// [/JSON " : "// [JSON ");

		text << sc->getName().toString();
		
		text << "]";

		return text;

	}
	else
	{
		jassertfalse;
		return String::empty;
	}

}

String CodeDragger::getNameFromPanel()
{
	ScriptingApi::Content::ScriptComponent *sc = dynamic_cast<ScriptingApi::Content::ScriptComponent *>(panel->editedComponent.get());

	if(sc != nullptr)
	{
		String text;
		
		text << sc->getName().toString();
		
		return text;
	}

	return String::empty;
}

ScriptComponentEditPanel::HiTextPropertyComponent::HiTextPropertyComponent(DynamicObject *properties_, Identifier id_, ScriptComponentEditPanel *panel_, bool useMultiLine_):
	TextPropertyComponent(id_.toString(), 80, useMultiLine_),
	useNumberMode(false),
	isMultiLine(useMultiLine_),
	id(id_),
	properties(properties_),
	panel(panel_)
{
	setText(getStringVersion(properties->getProperty(id_)));

	setLookAndFeel(&plaf);
};

void ScriptComponentEditPanel::HiTextPropertyComponent::setText (const String &newText)
{
	TextPropertyComponent::setText(newText);

	

	if(useNumberMode)
	{
		value = newText.getDoubleValue();
	}
	else
	{
		currentText = newText;
	}

	sendSynchronousChangeMessage();
}
 	
String ScriptComponentEditPanel::HiTextPropertyComponent::getText () const
{
	return currentText;
}


ScriptComponentEditPanel::HiFilePropertyComponent::HiFilePropertyComponent(DynamicObject *properties_, Identifier id_, const StringArray &options, ScriptComponentEditPanel *panel_):
	PropertyComponent(id_.toString()),
	component(this),
	properties(properties_),
	id(id_),
	panel(panel_)
{
	addAndMakeVisible(component);

	component.setLookAndFeel(&plaf);
	component.box.setLookAndFeel(&plaf);

	pooledFiles.addArray(options,1);

	component.box.addItemList(pooledFiles, 1);
	
}

void ScriptComponentEditPanel::HiFilePropertyComponent::refresh()
{
	currentFile = properties->getProperty(id);

	const int currentIndex = pooledFiles.indexOf(currentFile);

	component.box.setSelectedItemIndex(currentIndex, dontSendNotification);
}
		
void ScriptComponentEditPanel::HiFilePropertyComponent::buttonClicked(Button *)
{
	FileChooser fc("Load File", GET_PROJECT_HANDLER(findParentComponentOfClass<BackendProcessorEditor>()->getMainSynthChain()).getSubDirectory(ProjectHandler::SubDirectories::Images));

	if(fc.browseForFileToOpen())
	{
		currentFile = GET_PROJECT_HANDLER(findParentComponentOfClass<BackendProcessorEditor>()->getMainSynthChain()).getFileReference(fc.getResult().getFullPathName(), ProjectHandler::SubDirectories::Images);

		component.box.addItem(currentFile, component.box.getNumItems()+1);
	}

	component.box.setSelectedItemIndex(component.box.getNumItems()-1, sendNotification);
}
		
void ScriptComponentEditPanel::HiFilePropertyComponent::comboBoxChanged(ComboBox *)
{
    const String fileName = component.box.getItemText(component.box.getSelectedItemIndex());
    
	currentFile = GET_PROJECT_HANDLER(findParentComponentOfClass<BackendProcessorEditor>()->getMainSynthChain()).getFileReference(fileName, ProjectHandler::SubDirectories::Images);
    
	sendSynchronousChangeMessage();
}

void HiColourPropertyComponent::ColourEditorComponent::paint(Graphics& g)
{
	g.fillAll(Colours::grey);

	g.fillCheckerBoard(getLocalBounds().reduced(2, 2),
		10, 10,
		Colour(0xffdddddd).overlaidWith(colour),
		Colour(0xffffffff).overlaidWith(colour));

	g.setColour(Colours::white.overlaidWith(colour).contrasting());
	g.setFont(GLOBAL_BOLD_FONT());
	g.drawFittedText(colour.toDisplayString(true),
		2, 1, getWidth() - 4, getHeight() - 1,
		Justification::centred, 1);
}

void HiColourPropertyComponent::ColourEditorComponent::refresh()
{
	const Colour col(getColour());

	if (col != colour)
	{
		colour = col;
		repaint();
	}
}

ScriptComponentEditPanel::HiFilePropertyComponent::CombinedComponent::CombinedComponent(HiFilePropertyComponent *parent) :
box("FileNames"),
button("Open")
{
	addAndMakeVisible(box);
	addAndMakeVisible(button);

	button.setConnectedEdges(Button::ConnectedOnLeft | Button::ConnectedOnRight);
	button.setColour(TextButton::buttonColourId, Colour(0x4c4b4b4b));
	button.setColour(TextButton::buttonOnColourId, Colours::white.withAlpha(0.7f));
	button.setColour(TextButton::textColourOnId, Colour(0xaa000000));
	button.setColour(TextButton::textColourOffId, Colour(0x99ffffff));


	box.addListener(parent);
	button.addListener(parent);
}

ScriptComponentEditPanel::HiTogglePropertyComponent::HiTogglePropertyComponent(DynamicObject *properties, Identifier id, ScriptComponentEditPanel* /*panel*/) :
PropertyComponent(id.toString())
{
	button.setLookAndFeel(&plaf);

	button.setConnectedEdges(Button::ConnectedOnLeft | Button::ConnectedOnRight);
	button.setColour(TextButton::buttonColourId, Colour(0x4c4b4b4b));
	button.setColour(TextButton::buttonOnColourId, Colours::white.withAlpha(0.7f));
	button.setColour(TextButton::textColourOnId, Colour(0xaa000000));
	button.setColour(TextButton::textColourOffId, Colour(0x99ffffff));

	button.addListener(this);

	setValue(properties->getProperty(id), dontSendNotification);

	addAndMakeVisible(button);
}

void ScriptComponentEditPanel::HiTogglePropertyComponent::setValue(bool shouldBeOn, NotificationType notifyEditor/*=sendNotification*/)
{
	on = shouldBeOn;

	button.setToggleState(on, dontSendNotification);

	if (on)
	{
		button.setButtonText("Yes");
	}
	else
	{
		button.setButtonText("No");
	}

	if (notifyEditor == sendNotification) sendSynchronousChangeMessage();
}
