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


class SettingWindows::Content : public Component
{
public:

	Content()
	{
		addAndMakeVisible(&properties);
		properties.setLookAndFeel(&pplaf);

        auto& sb = properties.getViewport().getVerticalScrollBar();
        properties.getViewport().setScrollBarThickness(12);
        
        sf.addScrollBarToAnimate(sb);
        
		pplaf.setFontForAll(GLOBAL_BOLD_FONT());
		pplaf.setLabelWidth(190);

	}

	void resized() override
	{
		properties.setBounds(getLocalBounds());
	}

    ScrollbarFader sf;
    
	HiPropertyPanelLookAndFeel pplaf;

	PropertyPanel properties;

};

SettingWindows::SettingWindows(HiseSettings::Data& dataObject_, const Array<Identifier>& menusToShow) :
	dataObject(dataObject_),
	projectSettings("Project"),
	developmentSettings("Development"),
	audioSettings("Audio & Midi"),
	docSettings("Documentation"),
	allSettings("All"),
	applyButton("Save"),
	cancelButton("Cancel"),
	snexSettings("SNEX Workbench"),
	undoButton("Undo")
{
	if (menusToShow.isEmpty())
		settingsToShow = HiseSettings::SettingFiles::getAllIds();
	else
		settingsToShow = menusToShow;

	alaf = PresetHandler::createAlertWindowLookAndFeel();

	dataObject.addChangeListener(this);

	auto shouldShow = [this](const Identifier& id)
	{
		return settingsToShow.contains(id);
	};

#if !IS_MARKDOWN_EDITOR

	if (shouldShow(HiseSettings::SettingFiles::ProjectSettings))
	{
		addAndMakeVisible(&projectSettings);
		projectSettings.addListener(this);
		projectSettings.setLookAndFeel(&tblaf);
	}

	if (shouldShow(HiseSettings::SettingFiles::OtherSettings))
	{
		addAndMakeVisible(&developmentSettings);
		developmentSettings.addListener(this);
		developmentSettings.setLookAndFeel(&tblaf);
	}

	if (shouldShow(HiseSettings::SettingFiles::DocSettings))
	{
		addAndMakeVisible(&docSettings);
		docSettings.addListener(this);
		docSettings.setLookAndFeel(&tblaf);
	}

	if (shouldShow(HiseSettings::SettingFiles::SnexWorkbenchSettings))
	{
		addAndMakeVisible(&snexSettings);
		snexSettings.addListener(this);
		snexSettings.setLookAndFeel(&tblaf);
	}

#if IS_STANDALONE_APP

	if (shouldShow(HiseSettings::SettingFiles::AudioSettings))
	{
		addAndMakeVisible(&audioSettings);
		audioSettings.addListener(this);
		audioSettings.setLookAndFeel(&tblaf);
	}
#endif
#endif

	addAndMakeVisible(&allSettings);
	allSettings.addListener(this);
	allSettings.setLookAndFeel(&tblaf);

	addAndMakeVisible(&applyButton);
	applyButton.addListener(this);
	applyButton.setLookAndFeel(alaf);
	applyButton.addShortcut(KeyPress(KeyPress::returnKey));

	addAndMakeVisible(&cancelButton);
	cancelButton.addListener(this);
	cancelButton.setLookAndFeel(alaf);
	cancelButton.addShortcut(KeyPress(KeyPress::escapeKey));

	addAndMakeVisible(&undoButton);
	undoButton.addListener(this);
	undoButton.setLookAndFeel(alaf);
	undoButton.addShortcut(KeyPress('z', ModifierKeys::commandModifier, 'Z'));

	projectSettings.setRadioGroupId(1, dontSendNotification);
	allSettings.setRadioGroupId(1, dontSendNotification);
	developmentSettings.setRadioGroupId(1, dontSendNotification);
	audioSettings.setRadioGroupId(1, dontSendNotification);
	snexSettings.setRadioGroupId(1, dontSendNotification);
	docSettings.setRadioGroupId(1, dontSendNotification);

	addAndMakeVisible(currentContent = new Content());

	addAndMakeVisible(&fuzzySearchBox);
	fuzzySearchBox.addListener(this);
	fuzzySearchBox.setColour(TextEditor::ColourIds::backgroundColourId, Colours::white.withAlpha(0.2f));
	fuzzySearchBox.setFont(GLOBAL_FONT());
	fuzzySearchBox.setSelectAllWhenFocused(true);
	fuzzySearchBox.setColour(TextEditor::ColourIds::focusedOutlineColourId, Colour(SIGNAL_COLOUR));

	
	dataObject.data.addListener(this);

	setSize(800, 650);

	allSettings.setToggleState(true, sendNotificationSync);
}

SettingWindows::~SettingWindows()
{
	dataObject.data.removeListener(this);
	dataObject.removeChangeListener(this);

	if (saveOnDestroy)
	{
		for (auto id : HiseSettings::SettingFiles::getAllIds())
			save(id);
	}
	else
	{
		while (undoManager.canUndo())
			undoManager.undo();
	}
}

void SettingWindows::buttonClicked(Button* b)
{
	if (b == &allSettings)		   setContent(  settingsToShow );
	if (b == &projectSettings)	   setContent({ HiseSettings::SettingFiles::ProjectSettings, 
												HiseSettings::SettingFiles::UserSettings,
											    HiseSettings::SettingFiles::ExpansionSettings});
	if (b == &developmentSettings) setContent({	HiseSettings::SettingFiles::CompilerSettings, 
												HiseSettings::SettingFiles::ScriptingSettings, 
												HiseSettings::SettingFiles::OtherSettings});
	if (b == &audioSettings)	   setContent({ HiseSettings::SettingFiles::AudioSettings,
												HiseSettings::SettingFiles::MidiSettings});
	if (b == &docSettings)		   setContent({ HiseSettings::SettingFiles::DocSettings });
	if (b == &snexSettings)        setContent({ HiseSettings::SettingFiles::SnexWorkbenchSettings });

	if (b == &applyButton)
	{
		saveOnDestroy = true;
		destroy();
	}
	if (b == &cancelButton) destroy();

	if (b == &undoButton) undoManager.undo();
}

void SettingWindows::resized()
{
	auto area = getLocalBounds().reduced(1);

	area.removeFromTop(50);

	auto searchBar = area.removeFromTop(32);
	searchBar.removeFromLeft(32);
	searchBar.removeFromBottom(4);
	fuzzySearchBox.setBounds(searchBar);

	auto bottom = area.removeFromBottom(80);

	bottom = bottom.withSizeKeepingCentre(240, 40);

	applyButton.setBounds(bottom.removeFromLeft(80).reduced(5));
	cancelButton.setBounds(bottom.removeFromLeft(80).reduced(5));
	undoButton.setBounds(bottom.removeFromLeft(80).reduced(5));

	auto left = area.removeFromLeft(120);

	allSettings.setBounds(left.removeFromTop(40));

#if IS_MARKDOWN_EDITOR
	if(docSettings.isVisible())
		docSettings.setBounds(left.removeFromTop(40));
#else

	if(projectSettings.isVisible())
		projectSettings.setBounds(left.removeFromTop(40));
	if(developmentSettings.isVisible())
		developmentSettings.setBounds(left.removeFromTop(40));
	if(docSettings.isVisible())
		docSettings.setBounds(left.removeFromTop(40));
	if(snexSettings.isVisible())
		snexSettings.setBounds(left.removeFromTop(40));
	if(audioSettings.isVisible())
		audioSettings.setBounds(left.removeFromTop(40));
	
#endif

	currentContent->setBounds(area.reduced(10));
}


void SettingWindows::paint(Graphics& g)
{
	g.fillAll(Colour((uint32)bgColour));

	auto area = getLocalBounds().reduced(1);
	auto top = area.removeFromTop(50);
	auto searchBar = area.removeFromTop(32);
	auto s_ = searchBar.removeFromBottom(4);
	auto shadow = FLOAT_RECTANGLE(s_);

	g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xFF333333)));
	g.fillRect(searchBar);
	g.setGradientFill(ColourGradient(Colours::black.withAlpha(0.2f), 0.0f, shadow.getY(), Colours::transparentBlack, 0.0f, shadow.getBottom(), false));
	g.fillRect(shadow);
	g.setColour(Colour((uint32)tabBgColour));
	g.fillRect(top);
	g.setFont(GLOBAL_BOLD_FONT().withHeight(18.0f));
	g.setColour(Colours::white);
	g.drawText("Settings", top.toFloat(), Justification::centred);

	auto bottom = area.removeFromBottom(80);
	g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour((uint32)tabBgColour)));
	g.fillRect(bottom);

	auto s_2 = bottom.removeFromTop(4);
	auto shadow2 = FLOAT_RECTANGLE(s_2);

	g.setGradientFill(ColourGradient(Colours::black.withAlpha(0.2f), 0.0f, shadow2.getY(), Colours::transparentBlack, 0.0f, shadow2.getBottom(), false));
	g.fillRect(shadow2);

	auto left = area.removeFromLeft(120);

	g.setGradientFill(ColourGradient(Colour((uint32)tabBgColour), 0.0f, 0.0f, Colour(0xFF222222), 0.0f, (float)getHeight(), false));
	g.fillRect(left);
	g.setColour(Colours::white.withAlpha(0.6f));

	static const unsigned char searchIcon[] = { 110, 109, 0, 0, 144, 68, 0, 0, 48, 68, 98, 7, 31, 145, 68, 198, 170, 109, 68, 78, 223, 103, 68, 148, 132, 146, 68, 85, 107, 42, 68, 146, 2, 144, 68, 98, 54, 145, 219, 67, 43, 90, 143, 68, 66, 59, 103, 67, 117, 24, 100, 68, 78, 46, 128, 67, 210, 164, 39, 68, 98, 93, 50, 134, 67, 113, 58, 216, 67, 120, 192, 249, 67, 83, 151,
		103, 67, 206, 99, 56, 68, 244, 59, 128, 67, 98, 72, 209, 112, 68, 66, 60, 134, 67, 254, 238, 144, 68, 83, 128, 238, 67, 0, 0, 144, 68, 0, 0, 48, 68, 99, 109, 0, 0, 208, 68, 0, 0, 0, 195, 98, 14, 229, 208, 68, 70, 27, 117, 195, 211, 63, 187, 68, 146, 218, 151, 195, 167, 38, 179, 68, 23, 8, 77, 195, 98, 36, 92, 165, 68, 187, 58,
		191, 194, 127, 164, 151, 68, 251, 78, 102, 65, 0, 224, 137, 68, 0, 0, 248, 66, 98, 186, 89, 77, 68, 68, 20, 162, 194, 42, 153, 195, 67, 58, 106, 186, 193, 135, 70, 41, 67, 157, 224, 115, 67, 98, 13, 96, 218, 193, 104, 81, 235, 67, 243, 198, 99, 194, 8, 94, 78, 68, 70, 137, 213, 66, 112, 211, 134, 68, 98, 109, 211, 138, 67,
		218, 42, 170, 68, 245, 147, 37, 68, 128, 215, 185, 68, 117, 185, 113, 68, 28, 189, 169, 68, 98, 116, 250, 155, 68, 237, 26, 156, 68, 181, 145, 179, 68, 76, 44, 108, 68, 16, 184, 175, 68, 102, 10, 33, 68, 98, 249, 118, 174, 68, 137, 199, 2, 68, 156, 78, 169, 68, 210, 27, 202, 67, 0, 128, 160, 68, 0, 128, 152, 67, 98, 163,
		95, 175, 68, 72, 52, 56, 67, 78, 185, 190, 68, 124, 190, 133, 66, 147, 74, 205, 68, 52, 157, 96, 194, 98, 192, 27, 207, 68, 217, 22, 154, 194, 59, 9, 208, 68, 237, 54, 205, 194, 0, 0, 208, 68, 0, 0, 0, 195, 99, 101, 0, 0 };

	Path path;
	path.loadPathFromData(searchIcon, sizeof(searchIcon));
	path.applyTransform(AffineTransform::rotation(float_Pi));

	path.scaleToFit((float)searchBar.getX()+4.0f, (float)searchBar.getY() + 4.0f, 20.0f, 20.0f, true);

	g.fillPath(path);
	g.setColour(Colour(0xFF666666));
	g.drawRect(getLocalBounds(), 1);
}

void SettingWindows::textEditorTextChanged(TextEditor&)
{
	setContent(currentList);
}

void SettingWindows::setContent(SettingList list)
{
	currentContent->properties.clear();

	currentList = list;

	for (auto vt : currentList)
		fillPropertyPanel(vt, currentContent->properties, fuzzySearchBox.getText().toLowerCase());
	
	resized();
}

void SettingWindows::TabButtonLookAndFeel::drawToggleButton(Graphics& g, ToggleButton& b, bool isMouseOverButton, bool isButtonDown)
{
	auto bounds = b.getLocalBounds();
	auto s_ = bounds.removeFromBottom(3);
	auto shadow = FLOAT_RECTANGLE(s_);

	if (b.getToggleState())
	{
		g.setColour(Colour((uint32)ColourValues::bgColour));
		g.fillRect(bounds);
		g.setGradientFill(ColourGradient(Colours::black.withAlpha(0.2f), 0.0f, shadow.getY(), Colours::transparentBlack, 0.0f, shadow.getBottom(), false));
		g.fillRect(shadow);
	}
	
	if (isButtonDown)
	{
		g.setColour(Colours::white.withAlpha(0.05f));
		g.fillRect(bounds);
	}

	g.setColour(Colours::white.withAlpha(isMouseOverButton ? 1.0f : 0.8f));
	g.setFont(GLOBAL_BOLD_FONT());
	g.drawText(b.getButtonText(), bounds.reduced(5), Justification::centredRight);

	g.setColour(Colours::black.withAlpha(0.1f));
	g.drawHorizontalLine(b.getBottom()-3, (float)b.getX(), (float)b.getRight());
}



class ToggleButtonListPropertyComponent : public PropertyComponent,
	public ToggleButtonList::Listener
{
public:

	ToggleButtonListPropertyComponent(const String& name, Value v_, const StringArray& names_) :
		PropertyComponent(name),
		v(v_),
		names(names_),
		l(names_, this)
	{
		values = BigInteger((int64)v_.getValue());

		addAndMakeVisible(&l);
		setPreferredHeight(l.getHeight());
	};

	void refresh() override
	{
		auto v_ = (int64)v.getValue();

		values = BigInteger(v_);

		for (int i = 0; i < names.size(); i++)
			l.setValue(i, values[i], dontSendNotification);
	}

	void periodicCheckCallback(ToggleButtonList* /*list*/) override {}

	void toggleButtonWasClicked(ToggleButtonList* /*list*/, int index, bool value) override
	{
		values.setBit(index, value);
		v = values.toInt64();
	}

	BigInteger values;

	ToggleButtonList l;
	Value v;
	StringArray names;
};


void SettingWindows::valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier& p)
{
	ignoreUnused(p);

	const Identifier va("value");
	auto id = treeWhosePropertyHasChanged.getType();
	auto value = treeWhosePropertyHasChanged.getProperty("value");

	jassert(p == va);

	auto result = HiseSettings::Data::checkInput(id, value);

	if (result.wasOk())
	{
		dataObject.settingWasChanged(id, value);
	}
	else
	{
		if(PresetHandler::showYesNoWindow("Wrong input", result.getErrorMessage() + "\nPress OK to load the default value."))
		{
			treeWhosePropertyHasChanged.setProperty(va, dataObject.getDefaultSetting(id), nullptr);
		}
	}
}

void SettingWindows::valueTreeChildAdded(ValueTree&, ValueTree&)
{
 	dataObject.sendChangeMessage();
}

void SettingWindows::fillPropertyPanel(const Identifier& s, PropertyPanel& panel, const String& searchText)
{
	ignoreUnused(searchText);

	Array<PropertyComponent*> props;

	for (auto c : getValueTree(s))
	{
		auto searchString = c.getProperty("description").toString() + " " + c.getType().toString();
		searchString = searchString.toLowerCase();

#if USE_BACKEND
		if (searchText.isEmpty() || FuzzySearcher::fitsSearch(searchText, searchString, 0.2))
		{
			addProperty(c, props);
		}
#else
		addProperty(c, props);
#endif
	}

	if (props.size() > 0)
	{
		panel.addSection(getSettingNameToDisplay(s), props);

		for (auto pr : props)
		{
			auto n = pr->getName().removeCharacters(" ");

			auto help = HiseSettings::SettingDescription::getDescription(n);

			if (help.isNotEmpty())
			{
				MarkdownHelpButton* helpButton = new MarkdownHelpButton();
				helpButton->setFontSize(15.0f);
				helpButton->setHelpText(help);
				helpButton->attachTo(pr, MarkdownHelpButton::OverlayLeft);
			}
		}
	}
}

void SettingWindows::addProperty(ValueTree& c, Array<PropertyComponent*>& props)
{
	auto value = c.getPropertyAsValue("value", &undoManager);
	auto type = c.getProperty("type").toString();
	auto name = HiseSettings::ConversionHelpers::getUncamelcasedId(c.getType());
	auto id = c.getType();

	auto items = dataObject.getOptionsFor(id);

	if (HiseSettings::Data::isFileId(id))
	{
		auto ft = (id == HiseSettings::Other::ExternalEditorPath) ? File::findFiles : File::findDirectories;

		auto fpc = new FileNameValuePropertyComponent(name, File(value.toString()), ft, value);
		props.add(fpc);
	}
	else if (HiseSettings::Data::isToggleListId(id))
	{
		auto tblpc = new ToggleButtonListPropertyComponent(name, value, items);
		props.add(tblpc);
	}
	else if (items.size() > 0)
	{
		if (items[0] == "Yes")
		{
			auto bpc = new BooleanPropertyComponent(value, name, "Enabled");

			dynamic_cast<ToggleButton*>(bpc->getChildComponent(0))->setLookAndFeel(&blaf);

			bpc->setColour(BooleanPropertyComponent::ColourIds::backgroundColourId, Colours::transparentBlack);
			bpc->setColour(BooleanPropertyComponent::ColourIds::outlineColourId, Colours::transparentBlack);

			props.add(bpc);
		}
		else
		{
			Array<var> choiceValues;

			for (auto cv : items)
				choiceValues.add(cv);

			props.add(new ChoicePropertyComponent(value, name, items, choiceValues));
		}
	}
	else
	{
		props.add(new TextPropertyComponent(value, name, 1024, name.contains("Extra")));
	}
}

juce::String SettingWindows::getSettingNameToDisplay(const Identifier& s) const
{
	return HiseSettings::ConversionHelpers::getUncamelcasedId(getValueTree(s).getType());
}

juce::ValueTree SettingWindows::getValueTree(const Identifier& s) const
{
	return dataObject.data.getChildWithName(s);
}

void SettingWindows::save(const Identifier& s)
{
	// This will be saved by the audio device manager
	if (s == HiseSettings::SettingFiles::MidiSettings || s == HiseSettings::SettingFiles::AudioSettings || s == HiseSettings::SettingFiles::GeneralSettings)
		return;

	for (auto c : getValueTree(s))
	{
		if (c.getProperty("options").toString() == "Yes&#10;No")
			c.setProperty("value", c.getProperty("value") ? "Yes" : "No", nullptr);
	}

	if(ScopedPointer<XmlElement> xml = HiseSettings::ConversionHelpers::getConvertedXml(getValueTree(s)))
    {
        auto f = dataObject.getFileForSetting(s);

        xml->writeToFile(f, "");
    }
}



void addChildWithValue(ValueTree& v, const Identifier& id, const var& newValue)
{
	static const Identifier va("value");
	ValueTree c(id);
	c.setProperty(va, newValue, nullptr);
	v.addChild(c, -1, nullptr);
}

juce::ValueTree HiseSettings::ConversionHelpers::loadValueTreeFromFile(const File& f, const Identifier& settingId)
{
	auto xml = XmlDocument::parse(f);

	if (xml != nullptr)
	{
		return loadValueTreeFromXml(xml.get(), settingId);
	}

	return ValueTree();
}

juce::ValueTree HiseSettings::ConversionHelpers::loadValueTreeFromXml(XmlElement* xml, const Identifier& settingId)
{
	ValueTree v = ValueTree::fromXml(*xml);


	static const Identifier audioDeviceId("DEVICESETUP");

	if (v.getType() == audioDeviceId)
	{
		ValueTree v2(settingId);


		if (settingId == SettingFiles::AudioSettings)
		{
#if IS_STANDALONE_APP
			addChildWithValue(v2, HiseSettings::Audio::Driver, xml->getStringAttribute("deviceType"));
			addChildWithValue(v2, HiseSettings::Audio::Device, xml->getStringAttribute("audioOutputDeviceName"));
			addChildWithValue(v2, HiseSettings::Audio::Samplerate, xml->getStringAttribute("audioDeviceRate"));
			addChildWithValue(v2, HiseSettings::Audio::BufferSize, xml->getStringAttribute("bufferSize", "512"));
#endif

			return v2;
		}
		else if (settingId == SettingFiles::MidiSettings)
		{
#if IS_STANDALONE_APP

			StringArray active;

			for (int i = 0; i < xml->getNumChildElements(); i++)
			{
				if (xml->getChildElement(i)->hasTagName("MIDIINPUT"))
				{
					active.add(xml->getChildElement(i)->getStringAttribute("name"));
				}
			}



			StringArray allInputs = MidiInput::getDevices();

			BigInteger values;

			for (auto input : active)
			{
				int index = allInputs.indexOf(input);

				if (index != -1)
					values.setBit(index, true);
			}


			addChildWithValue(v2, Midi::MidiInput, values.toInt64());
#endif

			return v2;
		}
	}

	return v;
}



juce::XmlElement* HiseSettings::ConversionHelpers::getConvertedXml(const ValueTree& v)
{
	ValueTree copy = v.createCopy();

	if (copy.getType() == SettingFiles::ProjectSettings)
	{
		auto c = v.getChildWithName(Project::RedirectSampleFolder);
		copy.removeChild(c, nullptr);
	}


	return copy.createXml().release();
}

Array<int> HiseSettings::ConversionHelpers::getBufferSizesForDevice(AudioIODevice* currentDevice)
{
	if (currentDevice == nullptr)
		return {};

	auto bufferSizes = currentDevice->getAvailableBufferSizes();

	if (bufferSizes.size() > 7)
	{
		Array<int> powerOfTwoBufferSizes;
		powerOfTwoBufferSizes.ensureStorageAllocated(6);
		if (bufferSizes.contains(64)) powerOfTwoBufferSizes.add(64);
		if (bufferSizes.contains(128)) powerOfTwoBufferSizes.add(128);
		if (bufferSizes.contains(256)) powerOfTwoBufferSizes.add(256);
		if (bufferSizes.contains(512)) powerOfTwoBufferSizes.add(512);
		if (bufferSizes.contains(1024)) powerOfTwoBufferSizes.add(1024);

		if (powerOfTwoBufferSizes.size() > 2)
			bufferSizes.swapWith(powerOfTwoBufferSizes);
	}

	auto currentSize = currentDevice->getCurrentBufferSizeSamples();

	bufferSizes.addIfNotAlreadyThere(currentSize);

	int defaultBufferSize = currentDevice->getDefaultBufferSize();

	bufferSizes.addIfNotAlreadyThere(defaultBufferSize);

	bufferSizes.sort();

	return bufferSizes;
}

Array<double> HiseSettings::ConversionHelpers::getSampleRates(AudioIODevice* currentDevice)
{

#if HISE_IOS

	Array<double> samplerates;

	samplerates.add(44100.0);
	samplerates.add(48000.0);

#else

	if (currentDevice == nullptr)
		return {};

	Array<double> allSamplerates = currentDevice->getAvailableSampleRates();
	Array<double> samplerates;

	if (allSamplerates.contains(44100.0)) samplerates.add(44100.0);
	if (allSamplerates.contains(48000.0)) samplerates.add(48000.0);
	if (allSamplerates.contains(88200.0)) samplerates.add(88200.0);
	if (allSamplerates.contains(96000.0)) samplerates.add(96000.0);
	if (allSamplerates.contains(176400.0)) samplerates.add(176400.0);
	if (allSamplerates.contains(192000.0)) samplerates.add(192000.0);

#endif

	return samplerates;

}

juce::StringArray HiseSettings::ConversionHelpers::getChannelList()
{
	StringArray sa;

	sa.add("All channels");
	for (int i = 0; i < 16; i++)
		sa.add("Channel " + String(i + 1));

	return sa;
}

} // namespace hise
