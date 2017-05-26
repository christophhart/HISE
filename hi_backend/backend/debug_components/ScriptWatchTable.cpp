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

ScriptWatchTable::ScriptWatchTable(BackendRootWindow* window) :
	controller(window->getBackendProcessor())
{
	setName(getHeadline());

    addAndMakeVisible (table = new TableListBox());
    table->setModel (this);
	table->getHeader().setLookAndFeel(&laf);
	table->getHeader().setSize(getWidth(), 22);
    table->setOutlineThickness (0);
	table->getViewport()->setScrollBarsShown(true, false, false, false);
    
	table->setColour(ListBox::backgroundColourId, JUCE_LIVE_CONSTANT_OFF(Colour(0x04ffffff)));

	table->getHeader().addColumn("Type", Type, 30, 30, 30);
	table->getHeader().addColumn("Data Type", DataType, 100, 100, 100);
	table->getHeader().addColumn("Name", Name, 100, 60, 200);
	table->getHeader().addColumn("Value", Value, 180, 150, -1);

	table->getHeader().setStretchToFitActive(true);
	

	table->addMouseListener(this, true);

	addAndMakeVisible(fuzzySearchBox = new TextEditor());
	fuzzySearchBox->addListener(this);
	fuzzySearchBox->setColour(TextEditor::ColourIds::backgroundColourId, Colours::white.withAlpha(0.2f));
    fuzzySearchBox->setColour(TextEditor::ColourIds::focusedOutlineColourId, Colour(SIGNAL_COLOUR));
	fuzzySearchBox->setFont(GLOBAL_FONT());
	fuzzySearchBox->setSelectAllWhenFocused(true);

	controller->addScriptListener(this);

	rebuildLines();


}


ScriptWatchTable::~ScriptWatchTable()
{
	controller->removeScriptListener(this);

	controller = nullptr;
	allVariableLines.clear();
	processor = nullptr;
	table = nullptr;
}

void ScriptWatchTable::timerCallback()
{
	if(table != nullptr) refreshChangeStatus();
}

void ScriptWatchTable::scriptWasCompiled(JavascriptProcessor *compiledScriptProcessor)
{
	if (compiledScriptProcessor == dynamic_cast<JavascriptProcessor*>(processor.get()))
	{
		rebuildLines();
	}
}

int ScriptWatchTable::getNumRows() 
{
	return filteredIndexes.size();
};

void ScriptWatchTable::rebuildLines()
{
	allVariableLines.clear();

	if (processor.get() != nullptr)
	{
		HiseJavascriptEngine *engine = dynamic_cast<JavascriptProcessor*>(processor.get())->getScriptEngine();

		const int numRows = engine->getNumDebugObjects();

		for (int i = 0; i < numRows; i++)
		{
			allVariableLines.add(engine->getDebugInformation(i)->createTextArray());
		}

		applySearchFilter();
	}
}

void ScriptWatchTable::applySearchFilter()
{
	const String filterText = fuzzySearchBox->getText();

	if (filterText.isNotEmpty())
	{
		filteredIndexes.clear();

		for (int i = 0; i < allVariableLines.size(); i++)
		{
			bool lineContainsKeyword = false;

			for (int j = 0; j < allVariableLines[i].size(); j++)
			{
				if (allVariableLines[i][j].contains(filterText))
				{
					lineContainsKeyword = true;
					break;
				}
			}

			if (lineContainsKeyword)
			{
				filteredIndexes.add(i);
			}
		}
	}
	else
	{
		filteredIndexes.clear();

		filteredIndexes.ensureStorageAllocated(allVariableLines.size());

		for (int i = 0; i < allVariableLines.size(); i++)
		{
			filteredIndexes.add(i);
		}
	}

	String dbg = "{ ";

	for (int i = 0; i < filteredIndexes.size(); i++)
	{
		dbg << " " << String(filteredIndexes[i]) << ", ";
	}

	dbg << "}";

	DBG(dbg);

	table->updateContent();
	repaint();
}


void ScriptWatchTable::refreshChangeStatus()
{
	if (processor.get() == nullptr)
	{
		setScriptProcessor(nullptr, nullptr);
		return;
	}
	else
	{
		HiseJavascriptEngine *engine = dynamic_cast<JavascriptProcessor*>(processor.get())->getScriptEngine();
        
		BigInteger lastChanged = changed;
		changed = 0;

		for (int i = 0; i < filteredIndexes.size(); i++)
		{
			const int indexInAllLines = filteredIndexes[i];

			DebugInformation *info = engine->getDebugInformation(indexInAllLines);

			if (info != nullptr)
			{
				const String currentValue = info->getTextForValue();
				const String oldValue = allVariableLines[indexInAllLines][(int)DebugInformation::Row::Value];

				if (currentValue != oldValue)
				{
					allVariableLines.set(indexInAllLines, info->createTextArray());
					changed.setBit(i, true);
				}
			}
		}

		if (lastChanged != changed || changed != 0) repaint();
	}
};


void ScriptWatchTable::mouseDoubleClick(const MouseEvent &e)
{
	if (processor.get() != nullptr)
	{
		DebugInformation *info = getDebugInformationForRow(table->getSelectedRow(0));

		if (info != nullptr)
		{
			DebugableObject *db = info->getObject();

			auto editor = dynamic_cast<JavascriptCodeEditor*>(processor->getMainController()->getLastActiveEditor());

			if (editor == nullptr)
				return;

			if (auto editorPanel = editor->findParentComponentOfClass<CodeEditorPanel>())
			{
				editorPanel->setContentWithUndo(processor, 0);

				editor = editorPanel->getContent<PopupIncludeEditor>()->getEditor();

				CodeDocument::Position pos(editor->getDocument(), info->location.charNumber);
				editor->scrollToLine(jmax<int>(0, pos.getLineNumber()));
			}

			

			

#if 0

			DebugableObject::Helpers::gotoLocation(editor.getComponent(), dynamic_cast<JavascriptProcessor*>(processor.get()), info->location);

			if (db != nullptr)
			{
				db->doubleClickCallback(e, editor.getComponent());
			}
			else
			{
				
			}
#endif
		}
	}
}

void ScriptWatchTable::paint(Graphics &g)
{
	g.setColour(Colour(DEBUG_AREA_BACKGROUND_COLOUR_DARK));
	g.fillRect(0.0f, 0.0f, (float)getWidth(), 25.0f);

	g.setGradientFill(ColourGradient(Colours::black.withAlpha(0.5f), 0.0f, 25.0f,
		Colours::transparentBlack, 0.0f, 30.0f, false));
	g.fillRect(0.0f, 25.0f, (float)getWidth(), 25.0f);

	g.setColour(HiseColourScheme::getColour(HiseColourScheme::ColourIds::DebugAreaBackgroundColourId));
	g.fillRect(0, 25, getWidth(), getHeight());

	g.setColour(Colours::white.withAlpha(0.6f));

	static const unsigned char searchIcon[] = { 110, 109, 0, 0, 144, 68, 0, 0, 48, 68, 98, 7, 31, 145, 68, 198, 170, 109, 68, 78, 223, 103, 68, 148, 132, 146, 68, 85, 107, 42, 68, 146, 2, 144, 68, 98, 54, 145, 219, 67, 43, 90, 143, 68, 66, 59, 103, 67, 117, 24, 100, 68, 78, 46, 128, 67, 210, 164, 39, 68, 98, 93, 50, 134, 67, 113, 58, 216, 67, 120, 192, 249, 67, 83, 151,
		103, 67, 206, 99, 56, 68, 244, 59, 128, 67, 98, 72, 209, 112, 68, 66, 60, 134, 67, 254, 238, 144, 68, 83, 128, 238, 67, 0, 0, 144, 68, 0, 0, 48, 68, 99, 109, 0, 0, 208, 68, 0, 0, 0, 195, 98, 14, 229, 208, 68, 70, 27, 117, 195, 211, 63, 187, 68, 146, 218, 151, 195, 167, 38, 179, 68, 23, 8, 77, 195, 98, 36, 92, 165, 68, 187, 58,
		191, 194, 127, 164, 151, 68, 251, 78, 102, 65, 0, 224, 137, 68, 0, 0, 248, 66, 98, 186, 89, 77, 68, 68, 20, 162, 194, 42, 153, 195, 67, 58, 106, 186, 193, 135, 70, 41, 67, 157, 224, 115, 67, 98, 13, 96, 218, 193, 104, 81, 235, 67, 243, 198, 99, 194, 8, 94, 78, 68, 70, 137, 213, 66, 112, 211, 134, 68, 98, 109, 211, 138, 67,
		218, 42, 170, 68, 245, 147, 37, 68, 128, 215, 185, 68, 117, 185, 113, 68, 28, 189, 169, 68, 98, 116, 250, 155, 68, 237, 26, 156, 68, 181, 145, 179, 68, 76, 44, 108, 68, 16, 184, 175, 68, 102, 10, 33, 68, 98, 249, 118, 174, 68, 137, 199, 2, 68, 156, 78, 169, 68, 210, 27, 202, 67, 0, 128, 160, 68, 0, 128, 152, 67, 98, 163,
		95, 175, 68, 72, 52, 56, 67, 78, 185, 190, 68, 124, 190, 133, 66, 147, 74, 205, 68, 52, 157, 96, 194, 98, 192, 27, 207, 68, 217, 22, 154, 194, 59, 9, 208, 68, 237, 54, 205, 194, 0, 0, 208, 68, 0, 0, 0, 195, 99, 101, 0, 0 };

	Path path;
	path.loadPathFromData(searchIcon, sizeof(searchIcon));
	path.applyTransform(AffineTransform::rotation(float_Pi));

	const float xOffset = 0.0f;

	path.scaleToFit(xOffset + 4.0f, 4.0f, 16.0f, 16.0f, true);

	g.fillPath(path);
}

DebugInformation* ScriptWatchTable::getDebugInformationForRow(int rowIndex)
{
	HiseJavascriptEngine *engine = dynamic_cast<JavascriptProcessor*>(processor.get())->getScriptEngine();

	if (engine != nullptr)
	{
		const int componentIndex = filteredIndexes[rowIndex];
		return engine->getDebugInformation(componentIndex);
	}
	else
	{
		return nullptr;
	}
}

void ScriptWatchTable::setScriptProcessor(JavascriptProcessor *p, ScriptingEditor *editor_)
{
	processor = dynamic_cast<Processor*>(p);
	
	setName(getHeadline());

	if(processor.get() != nullptr)
	{
		
		rebuildLines();
		startTimer(400);
	}
	else
	{
		allVariableLines.clear();
		
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
    g.setFont (GLOBAL_FONT());

	const int indexInAllLines = filteredIndexes[rowNumber];

    if(processor.get() != nullptr)
	{
        String text;

		if (columnId == Type)
		{
			const juce_wchar c = allVariableLines[indexInAllLines][columnId - 1][0]; // Only the first char...
			const float alpha = 0.4f;
			const float brightness = 0.6f;
			const float h = jmin<float>((float)height, (float)width) - 4.0f;
			const Rectangle<float> area(((float)width - h) / 2.0f, 2.0f, h, h);

			switch (c)
			{
			case 'I': g.setColour(Colours::blue.withAlpha(alpha).withBrightness(brightness)); break;
			case 'V': g.setColour(Colours::cyan.withAlpha(alpha).withBrightness(brightness)); break;
			case 'G': g.setColour(Colours::green.withAlpha(alpha).withBrightness(brightness)); break;
			case 'C': g.setColour(Colours::yellow.withAlpha(alpha).withBrightness(brightness)); break;
			case 'R': g.setColour(Colours::red.withAlpha(alpha).withBrightness(brightness)); break;
			case 'F': g.setColour(Colours::orange.withAlpha(alpha).withBrightness(brightness)); break;
			case 'E': g.setColour(Colours::chocolate.withAlpha(alpha).withBrightness(brightness)); break;
			case 'N': g.setColour(Colours::pink.withAlpha(alpha).withBrightness(brightness)); break;
			}

			g.fillRoundedRectangle(area, 5.0f);
			g.setColour(Colours::white.withAlpha(0.4f));
			g.drawRoundedRectangle(area, 5.0f, 1.0f);
			g.setFont(GLOBAL_BOLD_FONT());
			g.setColour(Colours::white);

			String type;
			type << c;
			g.drawText(type, area, Justification::centred);
		}
		else
		{
			text << allVariableLines[indexInAllLines][columnId - 1];

			g.setColour(changed[rowNumber] ? Colours::orangered : Colours::white);
			g.setFont(GLOBAL_MONOSPACE_FONT());
			g.drawText(text, 5, 0, width - 10, height, Justification::centredLeft, true);
		}
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
	table->getHeader().resizeAllColumnsToFit(getWidth());

	table->setBounds(0, 24, getWidth(), jmax<int>(0, getHeight() - 24));
	fuzzySearchBox->setBounds(24, 0, getWidth()-24, 23);
}


void ScriptComponentEditPanel::addSectionToPanel(const Array<Identifier> &idList, const String &sectionName)
{
	ScriptingApi::Content::ScriptComponent *sc = dynamic_cast<ScriptingApi::Content::ScriptComponent *>(editedComponent.get());
	DynamicObject *scriptProperties = sc->getScriptObjectProperties();

	Array<PropertyComponent*> propertyPanelList;

	for(int i = 0; i < idList.size(); i++)
	{
		Identifier id = idList[i];

		if(sc->isPropertyDeactivated(id)) continue;

		StringArray options = sc->getOptionsFor(id);

		addProperty(propertyPanelList, scriptProperties, id, options);
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
		basicIds.add(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::zOrder));
		basicIds.add(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::parentComponent));
		basicIds.add(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::useUndoManager));
		
		addSectionToPanel(basicIds, "Basic Properties");

		Array<Identifier> parameterIds;

		parameterIds.add(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::macroControl));
		parameterIds.add(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::saveInPreset));
		parameterIds.add(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::isPluginParameter));
		parameterIds.add(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::pluginParameterName));

		addSectionToPanel(parameterIds, "Parameter Properties");

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

			if(basicIds.contains(id) || parameterIds.contains(id) || positionIds.contains(id) || colourIds.contains(id)) continue;

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
		arrayToAddTo.add(new HiTextPropertyComponent(obj, id, this, t == ScriptComponentPropertyTypeSelector::MultilineSelector));
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

void ScriptComponentEditPanel::setEditedComponent(ReferenceCountedObject* o)
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


	return String();
}

String CodeDragger::getText(ReferenceCountedObject*scriptComponent)
{
	ScriptingApi::Content::ScriptComponent *sc = dynamic_cast<ScriptingApi::Content::ScriptComponent *>(scriptComponent);

	if(sc != nullptr)
	{
		String text;

		String name = sc->getName().toString();

		text << getTag(scriptComponent, false) << "\n";

		text << "Content.setPropertiesFromJSON(\"" << name << "\", ";

		const String jsonProperties = sc->getScriptObjectPropertiesAsJSON();

		if (jsonProperties == "{\r\n}") return String();

		text << jsonProperties;
		text << ");\n";

		sc->setChanged(false);

		text << getTag(scriptComponent, true);

		return text;
	}
	else return String();

	

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

String CodeDragger::getTag(ReferenceCountedObject*scriptComponent, bool getEndTag)
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
		return String();
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

	return String();
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
