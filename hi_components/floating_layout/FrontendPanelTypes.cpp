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

ActivityLedPanel::ActivityLedPanel(FloatingTile* parent) :
	FloatingTileContent(parent)
{
	setOpaque(true);

	startTimer(100);
}

var ActivityLedPanel::toDynamicObject() const
{
	var obj = FloatingTileContent::toDynamicObject();

	storePropertyInObject(obj, (int)SpecialPanelIds::OffImage, offName);
	storePropertyInObject(obj, (int)SpecialPanelIds::OnImage, onName);
	storePropertyInObject(obj, (int)SpecialPanelIds::ShowMidiLabel, showMidiLabel);

	return obj;
}

void ActivityLedPanel::timerCallback()
{
	const bool midiFlag = getMainController()->checkAndResetMidiInputFlag();

	setOn(midiFlag);
}

void ActivityLedPanel::fromDynamicObject(const var& object)
{
	FloatingTileContent::fromDynamicObject(object);

	showMidiLabel = getPropertyWithDefault(object, (int)SpecialPanelIds::ShowMidiLabel);

	onName = getPropertyWithDefault(object, (int)SpecialPanelIds::OnImage);

	auto& handler = getMainController()->getExpansionHandler();

	if (onName.isNotEmpty())
		on = handler.loadImageReference(PoolReference(getMainController(), onName, ProjectHandler::SubDirectories::Images));

	offName = getPropertyWithDefault(object, (int)SpecialPanelIds::OffImage);

	if (offName.isNotEmpty())
		on = handler.loadImageReference(PoolReference(getMainController(), offName, ProjectHandler::SubDirectories::Images));
}



Identifier ActivityLedPanel::getDefaultablePropertyId(int index) const
{
	if (index < (int)PanelPropertyId::numPropertyIds)
		return FloatingTileContent::getDefaultablePropertyId(index);

	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::OffImage, "OffImage");
	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::OnImage, "OnImage");
	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::ShowMidiLabel, "ShowMidiLabel");

	jassertfalse;
	return{};
}

var ActivityLedPanel::getDefaultProperty(int index) const
{
	if (index < (int)PanelPropertyId::numPropertyIds)
		return FloatingTileContent::getDefaultProperty(index);

	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::OffImage, var(""));
	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::OnImage, var(""));
	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::ShowMidiLabel, true);

	jassertfalse;
	return{};
}

void ActivityLedPanel::paint(Graphics &g)
{
	g.fillAll(Colours::black);
	g.setColour(Colours::white);

	g.setFont(getFont());

	if (showMidiLabel)
		g.drawText("MIDI", 0, 0, 100, getHeight(), Justification::centredLeft, false);

	if (auto img = isOn ? on.getData() : off.getData())
	{
		g.drawImageWithin(*img, showMidiLabel ? 38 : 2, 2, 24, getHeight(), RectanglePlacement::centred);
	}

	
}

void ActivityLedPanel::setOn(bool shouldBeOn)
{
	isOn = shouldBeOn;
	repaint();
}


MidiKeyboardPanel::MidiKeyboardPanel(FloatingTile* parent) :
	FloatingTileContent(parent),
	updater(*this),
	mpeZone({2, 16})
{
	setDefaultPanelColour(PanelColourId::bgColour, Colour(BACKEND_BG_COLOUR_BRIGHT));

	setInterceptsMouseClicks(false, true);

	keyboard = new CustomKeyboard(parent->getMainController());

	addAndMakeVisible(keyboard->asComponent());

	keyboard->setLowestKeyBase(12);
	keyboard->setUseVectorGraphics(true, false);
	setDefaultPanelColour(PanelColourId::itemColour1, Colours::white.withAlpha(0.1f));
	setDefaultPanelColour(PanelColourId::itemColour2, Colours::white);
	setDefaultPanelColour(PanelColourId::itemColour3, Colour(SIGNAL_COLOUR));

	getMainController()->getMacroManager().getMidiControlAutomationHandler()->getMPEData().addListener(this);
}

MidiKeyboardPanel::~MidiKeyboardPanel()
{
	getMainController()->getMacroManager().getMidiControlAutomationHandler()->getMPEData().removeListener(this);

	keyboard = nullptr;
}

bool MidiKeyboardPanel::showTitleInPresentationMode() const
{
	return false;
}

Component* MidiKeyboardPanel::getKeyboard() const
{
	return keyboard->asComponent();
}


void MidiKeyboardPanel::Updater::handleAsyncUpdate()
{
	if (parent.cachedData.isObject())
	{
		parent.restoreInternal(parent.cachedData);
		parent.resized();
	}
}



void MidiKeyboardPanel::mpeModeChanged(bool isEnabled)
{
	mpeModeEnabled = isEnabled;

	updater.triggerAsyncUpdate();
}

int MidiKeyboardPanel::getNumDefaultableProperties() const
{
	return SpecialPanelIds::numProperyIds;
}

var MidiKeyboardPanel::toDynamicObject() const
{
	var obj = FloatingTileContent::toDynamicObject();

	storePropertyInObject(obj, SpecialPanelIds::KeyWidth, keyboard->getKeyWidthBase());

	storePropertyInObject(obj, SpecialPanelIds::DisplayOctaveNumber, keyboard->isShowingOctaveNumbers());
	storePropertyInObject(obj, SpecialPanelIds::LowKey, keyboard->getRangeStartBase());
	storePropertyInObject(obj, SpecialPanelIds::HiKey, keyboard->getRangeEndBase());
	storePropertyInObject(obj, SpecialPanelIds::CustomGraphics, keyboard->isUsingCustomGraphics());
	storePropertyInObject(obj, SpecialPanelIds::DefaultAppearance, defaultAppearance);
	storePropertyInObject(obj, SpecialPanelIds::BlackKeyRatio, keyboard->getBlackNoteLengthProportionBase());
	storePropertyInObject(obj, SpecialPanelIds::ToggleMode, keyboard->isToggleModeEnabled());
	storePropertyInObject(obj, SpecialPanelIds::MidiChannel, keyboard->getMidiChannelBase());
	storePropertyInObject(obj, SpecialPanelIds::UseVectorGraphics, keyboard->isUsingVectorGraphics());
	storePropertyInObject(obj, SpecialPanelIds::UseFlatStyle, keyboard->isUsingFlatStyle());
	storePropertyInObject(obj, SpecialPanelIds::MPEKeyboard, shouldBeMpeKeyboard);
	storePropertyInObject(obj, SpecialPanelIds::MPEStartChannel, mpeZone.getStart());
	storePropertyInObject(obj, SpecialPanelIds::MPEEndChannel, mpeZone.getEnd());

	return obj;
}

void MidiKeyboardPanel::fromDynamicObject(const var& object)
{
	FloatingTileContent::fromDynamicObject(object);

	cachedData = object;

	restoreInternal(object);
}

void MidiKeyboardPanel::restoreInternal(const var& object)
{
	shouldBeMpeKeyboard = getPropertyWithDefault(object, SpecialPanelIds::MPEKeyboard);

	const bool isReallyMpeKeyboard = shouldBeMpeKeyboard && mpeModeEnabled;

	if (keyboard->isMPEKeyboard() != isReallyMpeKeyboard)
	{
		if (isReallyMpeKeyboard)
			keyboard = new hise::MPEKeyboard(getMainController());
		else
			keyboard = new CustomKeyboard(getMainController());

		addAndMakeVisible(keyboard->asComponent());
	}

	keyboard->setUseCustomGraphics(getPropertyWithDefault(object, SpecialPanelIds::CustomGraphics));

	keyboard->setRangeBase(getPropertyWithDefault(object, SpecialPanelIds::LowKey),
		getPropertyWithDefault(object, SpecialPanelIds::HiKey));

	keyboard->setKeyWidthBase(getPropertyWithDefault(object, SpecialPanelIds::KeyWidth));

	defaultAppearance = getPropertyWithDefault(object, SpecialPanelIds::DefaultAppearance);

	keyboard->setShowOctaveNumber(getPropertyWithDefault(object, SpecialPanelIds::DisplayOctaveNumber));
	keyboard->setBlackNoteLengthProportionBase(getPropertyWithDefault(object, SpecialPanelIds::BlackKeyRatio));
	keyboard->setEnableToggleMode(getPropertyWithDefault(object, SpecialPanelIds::ToggleMode));
	keyboard->setMidiChannelBase(getPropertyWithDefault(object, SpecialPanelIds::MidiChannel));
	keyboard->setUseVectorGraphics(getPropertyWithDefault(object, SpecialPanelIds::UseVectorGraphics), getPropertyWithDefault(object, SpecialPanelIds::UseFlatStyle));

	auto startChannel = (int)getPropertyWithDefault(object, SpecialPanelIds::MPEStartChannel);
	auto endChannel = (int)getPropertyWithDefault(object, SpecialPanelIds::MPEEndChannel);

	mpeZone = { startChannel, endChannel };

	if (keyboard->isMPEKeyboard())
	{
		keyboard->asComponent()->setColour(hise::MPEKeyboard::ColourIds::bgColour, findPanelColour(PanelColourId::bgColour));
		keyboard->asComponent()->setColour(hise::MPEKeyboard::ColourIds::waveColour, findPanelColour(PanelColourId::itemColour1));
		keyboard->asComponent()->setColour(hise::MPEKeyboard::ColourIds::keyOnColour, findPanelColour(PanelColourId::itemColour2));
		keyboard->asComponent()->setColour(hise::MPEKeyboard::ColourIds::dragColour, findPanelColour(PanelColourId::itemColour3));
		
		if(keyboard->isMPEKeyboard())
			dynamic_cast<hise::MPEKeyboard*>(keyboard.get())->setChannelRange(mpeZone);
	}

	if (keyboard->isUsingFlatStyle())
	{
		if (auto claf = dynamic_cast<CustomKeyboardLookAndFeel*>(&dynamic_cast<CustomKeyboard*>(keyboard.get())->getLookAndFeel()))
		{
			claf->bgColour = findPanelColour(PanelColourId::bgColour);
			claf->overlayColour = findPanelColour(PanelColourId::itemColour1);
			claf->topLineColour = findPanelColour(PanelColourId::itemColour2);
			claf->activityColour = findPanelColour(PanelColourId::itemColour3);
		}
	}
}

Identifier MidiKeyboardPanel::getDefaultablePropertyId(int index) const
{
	if (index < (int)PanelPropertyId::numPropertyIds)
		return FloatingTileContent::getDefaultablePropertyId(index);

	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::CustomGraphics, "CustomGraphics");
	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::KeyWidth, "KeyWidth");
	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::LowKey, "LowKey");
	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::HiKey, "HiKey");
	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::BlackKeyRatio, "BlackKeyRatio");
	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::DefaultAppearance, "DefaultAppearance");
	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::DisplayOctaveNumber, "DisplayOctaveNumber");
	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::ToggleMode, "ToggleMode");
	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::MidiChannel, "MidiChannel");
	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::MPEKeyboard, "MPEKeyboard");
	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::MPEStartChannel, "MPEStartChannel");
	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::MPEEndChannel, "MPEEndChannel");
	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::UseVectorGraphics, "UseVectorGraphics");
	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::UseFlatStyle, "UseFlatStyle");
	
	jassertfalse;
	return{};
}

var MidiKeyboardPanel::getDefaultProperty(int index) const
{
	if (index < (int)PanelPropertyId::numPropertyIds)
		return FloatingTileContent::getDefaultProperty(index);

	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::CustomGraphics, false);
	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::KeyWidth, 14);
	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::LowKey, 9);
	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::HiKey, 127);
	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::BlackKeyRatio, 0.7);
	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::DefaultAppearance, true);
	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::DisplayOctaveNumber, false);
	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::ToggleMode, false);
	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::MidiChannel, 1);
	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::MPEKeyboard, false);
	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::MPEStartChannel, 2);
	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::MPEEndChannel, 16);
	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::UseVectorGraphics, true);
	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::UseFlatStyle, false);

	jassertfalse;
	return{};
}

void MidiKeyboardPanel::paint(Graphics& g)
{
	g.setColour(findPanelColour(PanelColourId::bgColour));
	g.fillAll();
}

void MidiKeyboardPanel::resized()
{
	if (!keyboard->isMPEKeyboard() && defaultAppearance)
	{
		int width = jmin<int>(getWidth(), CONTAINER_WIDTH);

		keyboard->asComponent()->setBounds((getWidth() - width) / 2, 0, width, 72);
	}
	else
	{
		keyboard->asComponent()->setBounds(0, 0, getWidth(), getHeight());
	}
}

int MidiKeyboardPanel::getFixedHeight() const
{
	return defaultAppearance ? 72 : FloatingTileContent::getFixedHeight();
}




Note::Note(FloatingTile* parent) :
	FloatingTileContent(parent)
{
	addAndMakeVisible(editor = new TextEditor());
	editor->setFont(GLOBAL_BOLD_FONT());
	editor->setColour(TextEditor::ColourIds::backgroundColourId, Colours::transparentBlack);
	editor->setColour(TextEditor::ColourIds::textColourId, Colours::white.withAlpha(0.8f));
	editor->setColour(TextEditor::ColourIds::focusedOutlineColourId, Colours::white.withAlpha(0.5f));
	editor->setColour(TextEditor::ColourIds::highlightColourId, Colour(SIGNAL_COLOUR));
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

int Note::getNumDefaultableProperties() const
{
	return SpecialPanelIds::numSpecialPanelIds;
}

Identifier Note::getDefaultablePropertyId(int index) const
{
	if (index < (int)PanelPropertyId::numPropertyIds)
		return FloatingTileContent::getDefaultablePropertyId(index);

	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::Text, "Text");

	jassertfalse;
	return{};
}

var Note::getDefaultProperty(int index) const
{
	if (index < (int)PanelPropertyId::numPropertyIds)
		return FloatingTileContent::getDefaultProperty(index);

	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::Text, var(""));

	jassertfalse;
	return{};
}

void Note::labelTextChanged(Label*)
{

}

int Note::getFixedHeight() const
{
	return 150;
}


PerformanceLabelPanel::PerformanceLabelPanel(FloatingTile* parent) :
	FloatingTileContent(parent)
{
	addAndMakeVisible(statisticLabel = new Label());
	statisticLabel->setEditable(false, false);
	statisticLabel->setColour(Label::ColourIds::textColourId, Colours::white);

	setDefaultPanelColour(PanelColourId::textColour, Colours::white);
	setDefaultPanelColour(PanelColourId::bgColour, Colours::transparentBlack);

	statisticLabel->setFont(GLOBAL_BOLD_FONT());

	startTimer(200);
}

void PerformanceLabelPanel::timerCallback()
{
	auto mc = getMainController();

	const int cpuUsage = (int)mc->getCpuUsage();
	const int voiceAmount = mc->getNumActiveVoices();


	auto bytes = mc->getSampleManager().getModulatorSamplerSoundPool2()->getMemoryUsageForAllSamples();

	auto& handler = getMainController()->getExpansionHandler();

	for (int i = 0; i < handler.getNumExpansions(); i++)
	{
		bytes += handler.getExpansion(i)->pool->getSamplePool()->getMemoryUsageForAllSamples();
	}

	const double ramUsage = (double)bytes / 1024.0 / 1024.0;

	//const bool midiFlag = mc->checkAndResetMidiInputFlag();

	//activityLed->setOn(midiFlag);

	String stats = "CPU: ";
	stats << String(cpuUsage) << "%, RAM: " << String(ramUsage, 1) << "MB , Voices: " << String(voiceAmount);
	statisticLabel->setText(stats, dontSendNotification);
}



void PerformanceLabelPanel::fromDynamicObject(const var& object)
{
	FloatingTileContent::fromDynamicObject(object);

	statisticLabel->setColour(Label::ColourIds::textColourId, findPanelColour(PanelColourId::textColour));
	statisticLabel->setFont(getFont());
}

void PerformanceLabelPanel::resized()
{
	statisticLabel->setBounds(getLocalBounds());
}

bool PerformanceLabelPanel::showTitleInPresentationMode() const
{
	return false;
}

TooltipPanel::TooltipPanel(FloatingTile* parent) :
	FloatingTileContent(parent)
{
	setDefaultPanelColour(PanelColourId::bgColour, Colour(0xFF383838));
	setDefaultPanelColour(PanelColourId::itemColour1, Colours::white.withAlpha(0.2f));
	setDefaultPanelColour(PanelColourId::textColour, Colours::white.withAlpha(0.8f));

	addAndMakeVisible(tooltipBar = new TooltipBar());;
}

TooltipPanel::~TooltipPanel()
{
	tooltipBar = nullptr;
}

int TooltipPanel::getFixedHeight() const
{
	return 30;
}

bool TooltipPanel::showTitleInPresentationMode() const
{
	return false;
}

void TooltipPanel::fromDynamicObject(const var& object)
{
	FloatingTileContent::fromDynamicObject(object);

	tooltipBar->setColour(TooltipBar::backgroundColour, findPanelColour(PanelColourId::bgColour));
	tooltipBar->setColour(TooltipBar::iconColour, findPanelColour(PanelColourId::itemColour1));
	tooltipBar->setColour(TooltipBar::textColour, findPanelColour(PanelColourId::textColour));

	tooltipBar->setFont(getFont());
}

void TooltipPanel::resized()
{
	tooltipBar->setBounds(getLocalBounds());
}

PresetBrowserPanel::PresetBrowserPanel(FloatingTile* parent) :
	FloatingTileContent(parent)
{
	setDefaultPanelColour(PanelColourId::bgColour, Colours::black.withAlpha(0.97f));
    setDefaultPanelColour(PanelColourId::textColour, Colours::white);
	setDefaultPanelColour(PanelColourId::itemColour1, Colour(SIGNAL_COLOUR));

	addAndMakeVisible(presetBrowser = new PresetBrowser(getMainController()));
	
	if (parent->getMainController()->getCurrentScriptLookAndFeel() != nullptr)
	{
		scriptlaf = HiseColourScheme::createAlertWindowLookAndFeel(parent->getMainController());
		presetBrowser->setLookAndFeel(scriptlaf);
	}
}

PresetBrowserPanel::~PresetBrowserPanel()
{
	presetBrowser = nullptr;
	scriptlaf = nullptr;
}

var PresetBrowserPanel::toDynamicObject() const
{
	var obj = FloatingTileContent::toDynamicObject();

	storePropertyInObject(obj, SpecialPanelIds::ShowSaveButton, options.showSaveButtons);

	storePropertyInObject(obj, SpecialPanelIds::ShowExpansionsAsColumn, options.showExpansions);
	storePropertyInObject(obj, SpecialPanelIds::ShowFolderButton, options.showFolderButton);
	storePropertyInObject(obj, SpecialPanelIds::ShowNotes, options.showNotesLabel);
	storePropertyInObject(obj, SpecialPanelIds::ShowEditButtons, options.showEditButtons);
	storePropertyInObject(obj, SpecialPanelIds::ShowFavoriteIcon, options.showFavoriteIcons);
	storePropertyInObject(obj, SpecialPanelIds::NumColumns, options.numColumns);
	storePropertyInObject(obj, SpecialPanelIds::ColumnWidthRatio, var(options.columnWidthRatios));

	return obj;
}

void PresetBrowserPanel::fromDynamicObject(const var& object)
{
	FloatingTileContent::fromDynamicObject(object);

	options.showSaveButtons = getPropertyWithDefault(object, SpecialPanelIds::ShowSaveButton);
	options.showFolderButton = getPropertyWithDefault(object, SpecialPanelIds::ShowFolderButton);
	options.showNotesLabel = getPropertyWithDefault(object, SpecialPanelIds::ShowNotes);
	options.showEditButtons = getPropertyWithDefault(object, SpecialPanelIds::ShowEditButtons);
	options.showExpansions = getPropertyWithDefault(object, SpecialPanelIds::ShowExpansionsAsColumn);
	options.numColumns = getPropertyWithDefault(object, SpecialPanelIds::NumColumns);

	auto ratios = getPropertyWithDefault(object, SpecialPanelIds::ColumnWidthRatio);
	if (ratios.isArray())
	{
		options.columnWidthRatios.clear();
		options.columnWidthRatios.addArray(*ratios.getArray());
	}
	
	options.showFavoriteIcons = getPropertyWithDefault(object, SpecialPanelIds::ShowFavoriteIcon);
	options.backgroundColour = findPanelColour(PanelColourId::bgColour);
	options.highlightColour = findPanelColour(PanelColourId::itemColour1);
	options.textColour = findPanelColour(PanelColourId::textColour);
	options.font = getFont();
	
	presetBrowser->setOptions(options);
}

bool PresetBrowserPanel::showTitleInPresentationMode() const
{
	return false;
}

void PresetBrowserPanel::resized()
{
	presetBrowser->setBounds(getLocalBounds());
	presetBrowser->setOptions(options);
}


juce::Identifier PresetBrowserPanel::getDefaultablePropertyId(int index) const
{
	if (index < (int)PanelPropertyId::numPropertyIds)
		return FloatingTileContent::getDefaultablePropertyId(index);

	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::ShowFolderButton, "ShowFolderButton");
	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::ShowSaveButton, "ShowSaveButton");
	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::ShowNotes, "ShowNotes");
	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::ShowEditButtons, "ShowEditButtons");
	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::NumColumns, "NumColumns");
	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::ColumnWidthRatio, "ColumnWidthRatio");
	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::ShowExpansionsAsColumn, "ShowExpansionsAsColumn");
	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::ShowFavoriteIcon, "ShowFavoriteIcon");

	return Identifier();
}


var PresetBrowserPanel::getDefaultProperty(int index) const
{
	if (index < (int)PanelPropertyId::numPropertyIds)
		return FloatingTileContent::getDefaultProperty(index);

#if HISE_IOS
	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::ShowFolderButton, true);
#else
	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::ShowFolderButton, true);
#endif
	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::ShowSaveButton, true);
	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::ShowNotes, true);
	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::ShowEditButtons, true);
	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::NumColumns, 3);

	Array<var> defaultRatios;
	defaultRatios.insertMultiple(0, 1.0 / 3.0, 3);

	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::ColumnWidthRatio, var(defaultRatios));
	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::ShowExpansionsAsColumn, false);
	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::ShowFavoriteIcon, true);

	return var();
}

AboutPagePanel::AboutPagePanel(FloatingTile* parent) :
	FloatingTileContent(parent)
{
	setDefaultPanelColour(PanelColourId::textColour, Colours::white);
	setDefaultPanelColour(PanelColourId::itemColour1, Colours::white);
	setDefaultPanelColour(PanelColourId::bgColour, Colours::black);
}


var AboutPagePanel::toDynamicObject() const
{
	auto object = FloatingTileContent::toDynamicObject();

	storePropertyInObject(object, CopyrightNotice, showCopyrightNotice);
	storePropertyInObject(object, ShowLicensedEmail, showLicensedEmail);
	storePropertyInObject(object, ShowVersion, showVersion);
	storePropertyInObject(object, BuildDate, showBuildDate);
	storePropertyInObject(object, WebsiteURL, showWebsiteURL);
	storePropertyInObject(object, ShowProductName, showProductName);
	storePropertyInObject(object, UseCustomImage, useCustomImage);

	return object;
}

void AboutPagePanel::fromDynamicObject(const var& object)
{
	FloatingTileContent::fromDynamicObject(object);

	showCopyrightNotice = getPropertyWithDefault(object, CopyrightNotice).toString();
	showLicensedEmail = getPropertyWithDefault(object, ShowLicensedEmail);
	showVersion = getPropertyWithDefault(object, ShowVersion);
	showBuildDate = getPropertyWithDefault(object, BuildDate);
	showWebsiteURL = getPropertyWithDefault(object, WebsiteURL);
	showProductName = getPropertyWithDefault(object, ShowProductName);

	useCustomImage = getPropertyWithDefault(object, UseCustomImage);
	

	rebuildText();
}


Identifier AboutPagePanel::getDefaultablePropertyId(int index) const
{
	if (index < (int)PanelPropertyId::numPropertyIds)
		return FloatingTileContent::getDefaultablePropertyId(index);

	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::BuildDate, "BuildDate");
	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::CopyrightNotice, "CopyrightNotice");
	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::ShowLicensedEmail, "ShowLicensedEmail");
	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::ShowVersion, "ShowVersion");
	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::WebsiteURL, "WebsiteURL");
	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::ShowProductName, "ShowProductName");
	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::UseCustomImage, "UseCustomImage");

	jassertfalse;
	return{};
}

var AboutPagePanel::getDefaultProperty(int index) const
{
	if (index < (int)PanelPropertyId::numPropertyIds)
		return FloatingTileContent::getDefaultProperty(index);

	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::BuildDate, true);
	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::CopyrightNotice, false);
	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::ShowLicensedEmail, true);
	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::ShowVersion, true);
	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::WebsiteURL, true);
	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::ShowProductName, true);
	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::UseCustomImage, false);

	jassertfalse;
	return{};
}

void AboutPagePanel::paint(Graphics& g)
{
	g.fillAll(findPanelColour(PanelColourId::bgColour));

	if (useCustomImage)
	{
		if (auto img = bgImage.getData())
		{
			g.drawImageWithin(*img, 0, 0, getWidth(), getHeight(), RectanglePlacement::centred);
		}
	}

	Rectangle<float> r({ 0.0f, 0.0f, (float)getWidth(), (float)getHeight() });

	if (useCustomImage)
	{
		r = r.removeFromBottom(150.0f).reduced(10.0f);
	}

	text.draw(g, r);
}

void AboutPagePanel::rebuildText()
{
	text.clear();

	if (useCustomImage)
	{
		auto& handler = getMainController()->getExpansionHandler();
		bgImage = handler.loadImageReference(PoolReference(getMainController(), "{PROJECT_FOLDER}about.png", ProjectHandler::SubDirectories::Images));
	}
	

#if USE_FRONTEND
	const String projectName = FrontendHandler::getProjectName();

#if USE_COPY_PROTECTION
	const String licencee = dynamic_cast<FrontendProcessor*>(getMainController())->unlocker.getEmailAdress();
#endif

	const String version = FrontendHandler::getVersionString();
	
#else
	const auto& data = dynamic_cast<GlobalSettingManager*>(getMainController())->getSettingsObject();

	const String projectName = data.getSetting(HiseSettings::Project::Name);
	const String licencee = "mailMcFaceMail@mail.mail";
	const String version = data.getSetting(HiseSettings::Project::Version);

#endif

	const String hiseVersion = String(HISE_VERSION);
	const String buildTime = Time::getCompilationDate().toString(true, false, false, true);

	Font bold = getFont(); // .boldened();
	Font normal = getFont();

	Colour high = findPanelColour(PanelColourId::itemColour1);
	Colour low = findPanelColour(PanelColourId::textColour);

	NewLine nl;

	if (showProductName)
	{
		text.append(projectName + nl + nl, bold.withHeight(18.0f), high);
	}

	if (showVersion)
	{
		text.append("Version: ", bold, low);
		text.append(version + nl + nl, normal, low);
	}

#if USE_COPY_PROTECTION
	if (showLicensedEmail)
	{
		text.append("Licensed to: ", bold, low);
		text.append(licencee + nl, normal, low);
	}
#endif
	
	text.append(nl + "Built with HISE Version ", bold, low);
	text.append(hiseVersion + nl, bold, low);

	if (showBuildDate)
	{
		text.append("Build Time: ", bold, low);
		text.append(buildTime + nl + nl, normal, low);
	}

	if (showCopyrightNotice.isNotEmpty())
	{
		text.append(showCopyrightNotice + nl + nl, normal, low);
	}

	if (showWebsiteURL.isNotEmpty())
	{
		text.append(showWebsiteURL + nl, bold, low);
	}
	
}

FrontendMacroPanel::FrontendMacroPanel(FloatingTile* parent) :
	TableFloatingTileBase(parent),
	macroChain(parent->getMainController()->getMainSynthChain()),
	macroManager(parent->getMainController()->getMacroManager())
{
	getMainController()->getMainSynthChain()->addMacroConnectionListener(this);

	setName("Macro Edit Table");
	initTable();
}

FrontendMacroPanel::~FrontendMacroPanel()
{
	getMainController()->getMainSynthChain()->removeMacroConnectionListener(this);
}

void FrontendMacroPanel::macroConnectionChanged(int macroIndex, Processor* p, int parameterIndex, bool wasAdded)
{
	updateContent();
	repaint();
}

hise::MacroControlBroadcaster::MacroControlData* FrontendMacroPanel::getData(MacroControlBroadcaster::MacroControlledParameterData* pd)
{
	for (int i = 0; i < 8; i++)
	{
		if (macroChain->getMacroControlData(i)->getParameterWithProcessorAndIndex(pd->getProcessor(), pd->getParameter()))
			return macroChain->getMacroControlData(i);
	}

	return nullptr;
}

const hise::MacroControlBroadcaster::MacroControlData* FrontendMacroPanel::getData(MacroControlBroadcaster::MacroControlledParameterData* pd) const
{
	for (int i = 0; i < 8; i++)
	{
		if (macroChain->getMacroControlData(i)->getParameterWithProcessorAndIndex(pd->getProcessor(), pd->getParameter()))
			return macroChain->getMacroControlData(i);
	}

	return nullptr;
}

int FrontendMacroPanel::getNumRows()
{
	if (!getMainController()->getMacroManager().isMacroEnabledOnFrontend())
	{
		connectionList.clear();
		return 0;
	}

	Array<WeakReference<MacroControlBroadcaster::MacroControlledParameterData>> newList;

	for (int i = 0; i < 8; i++)
	{
		auto d = macroChain->getMacroControlData(i);

		for (int j = 0; j < d->getNumParameters(); j++)
		{
			newList.add(d->getParameter(j));
		}
	}

	newList.swapWith(connectionList);

	return connectionList.size();
}

void FrontendMacroPanel::removeEntry(int rowIndex)
{
	if (auto data = connectionList[rowIndex].get())
		getData(data)->removeParameter(data->getParameterName(), data->getProcessor());
}

bool FrontendMacroPanel::setRange(int rowIndex, NormalisableRange<double> newRange)
{
	if (auto data = connectionList[rowIndex].get())
	{
		data->setRangeStart(newRange.start);
		data->setRangeEnd(newRange.end);
		return true;
	}

	return false;
}

void FrontendMacroPanel::paint(Graphics& g)
{
	if (!getMainController()->getMacroManager().isMacroEnabledOnFrontend())
	{
		g.setFont(GLOBAL_BOLD_FONT());
		g.setColour(Colour(0xFF682222));
		g.drawText("Macros are not enabled on the Front Interface", getLocalBounds().toFloat(), Justification::centred);
	}
}

bool FrontendMacroPanel::isInverted(int rowIndex) const
{
	if (auto data = connectionList[rowIndex].get())
		return data->isInverted();

	return false;
}

juce::NormalisableRange<double> FrontendMacroPanel::getFullRange(int rowIndex) const
{
	if (auto data = connectionList[rowIndex].get())
		return data->getTotalRange();

	return {};
}

juce::NormalisableRange<double> FrontendMacroPanel::getRange(int rowIndex) const
{
	if (auto data = connectionList[rowIndex].get())
		return data->getParameterRange();

	return {};
}

bool FrontendMacroPanel::isUsed(int rowIndex) const
{
	return isPositiveAndBelow(rowIndex, connectionList.size());
}

void FrontendMacroPanel::setInverted(int row, bool value)
{
	if (auto data = connectionList[row].get())
		data->setInverted(value);
}

juce::String FrontendMacroPanel::getCellText(int rowNumber, int columnId) const
{
	if (auto data = connectionList[rowNumber].get())
	{
		if (columnId == ColumnId::ParameterName)
			return data->getParameterName();
		else if (columnId == ColumnId::CCNumber)
			return getData(data)->getMacroName();
	}

	return {};
}

MidiLearnPanel::MidiLearnPanel(FloatingTile* parent) :
	TableFloatingTileBase(parent),
	handler(*getMainController()->getMacroManager().getMidiControlAutomationHandler())
{
	handler.addChangeListener(this);
	setName("MIDI Control List");
	initTable();
}

MidiLearnPanel::~MidiLearnPanel()
{
	handler.removeChangeListener(this);
}

void MidiLearnPanel::changeListenerCallback(SafeChangeBroadcaster*)
{
	updateContent();
	repaint();
}

int MidiLearnPanel::getNumRows()
{
	return handler.getNumActiveConnections();
}

void MidiLearnPanel::removeEntry(int rowIndex)
{
	auto data = handler.getDataFromIndex(rowIndex);

	handler.removeMidiControlledParameter(data.processor, data.attribute, sendNotification);
}

bool MidiLearnPanel::setRange(int rowIndex, NormalisableRange<double> newRange)
{
	return handler.setNewRangeForParameter(rowIndex, newRange);
}

bool MidiLearnPanel::isInverted(int rowIndex) const
{
	auto data = handler.getDataFromIndex(rowIndex);

	if (data.used)
		return data.inverted;

	return false;
}

juce::NormalisableRange<double> MidiLearnPanel::getFullRange(int rowIndex) const
{
	auto data = handler.getDataFromIndex(rowIndex);

	if (data.used)
		return data.fullRange;

	return {};
}

juce::NormalisableRange<double> MidiLearnPanel::getRange(int rowIndex) const
{
	auto data = handler.getDataFromIndex(rowIndex);

	if (data.used)
		return data.parameterRange;

	return {};
}

bool MidiLearnPanel::isUsed(int rowIndex) const
{
	return handler.getDataFromIndex(rowIndex).used;
}

void MidiLearnPanel::setInverted(int row, bool value)
{
	if (isUsed(row))
	{
		const bool ok = handler.setParameterInverted(row, value);
		jassert(ok);
		ignoreUnused(ok);
	}
}

juce::String MidiLearnPanel::getCellText(int rowNumber, int columnId) const
{
	auto data = handler.getDataFromIndex(rowNumber);

	if (data.processor == nullptr)
	{
		return {};
	}

	if (columnId == ColumnId::ParameterName)
		return ProcessorHelpers::getPrettyNameForAutomatedParameter(data.processor, data.attribute);
	else if (columnId == ColumnId::CCNumber)
		return String(data.ccNumber);
	else
		return "";
}

TableFloatingTileBase::InvertedButton::InvertedButton(TableFloatingTileBase &owner_) :
	owner(owner_)
{
	laf.setFontForAll(owner.font);

	addAndMakeVisible(t = new TextButton("Inverted"));
	t->setButtonText("Inverted");
	t->setLookAndFeel(&laf);
	t->setConnectedEdges(Button::ConnectedOnLeft | Button::ConnectedOnRight);
	t->addListener(this);
	t->setTooltip("Invert the range of the macro control for this parameter.");
	t->setColour(TextButton::buttonColourId, Colour(0x88000000));
	t->setColour(TextButton::buttonOnColourId, Colour(0x88FFFFFF));
	t->setColour(TextButton::textColourOnId, Colour(0xaa000000));
	t->setColour(TextButton::textColourOffId, Colour(0x99ffffff));

	t->setClickingTogglesState(true);
}

void TableFloatingTileBase::InvertedButton::resized()
{
	t->setBounds(getLocalBounds().reduced(1));
}

void TableFloatingTileBase::InvertedButton::setRowAndColumn(const int newRow, bool value)
{
	row = newRow;

	t->setToggleState(value, dontSendNotification);
	t->setButtonText(value ? "Inverted" : "Normal");
}

void TableFloatingTileBase::InvertedButton::buttonClicked(Button *b)
{
	t->setButtonText(b->getToggleState() ? "Inverted" : "Normal");
	owner.setInverted(row, b->getToggleState());
}

TableFloatingTileBase::ValueSliderColumn::ValueSliderColumn(TableFloatingTileBase &table) :
	owner(table)
{
	addAndMakeVisible(slider = new Slider());

	laf.setFontForAll(table.font);

	slider->setLookAndFeel(&laf);
	slider->setSliderStyle(Slider::LinearBar);
	slider->setTextBoxStyle(Slider::TextBoxLeft, true, 80, 20);
	slider->setColour(Slider::backgroundColourId, Colour(0x38ffffff));
	slider->setColour(Slider::thumbColourId, Colour(SIGNAL_COLOUR));
	slider->setColour(Slider::rotarySliderOutlineColourId, Colours::black);
	slider->setColour(Slider::textBoxOutlineColourId, Colour(0x38ffffff));
	slider->setColour(Slider::textBoxTextColourId, Colours::black);
	slider->setTextBoxIsEditable(true);

	slider->addListener(this);
}

void TableFloatingTileBase::ValueSliderColumn::resized()
{
	slider->setBounds(getLocalBounds().reduced(1));
}

void TableFloatingTileBase::ValueSliderColumn::setRowAndColumn(const int newRow, int column, double value, NormalisableRange<double> range)
{
	row = newRow;
	columnId = column;

	slider->setRange(range.start, range.end, range.interval);
	slider->setSkewFactor(range.skew);

	slider->setValue(value, dontSendNotification);
}

void TableFloatingTileBase::ValueSliderColumn::sliderValueChanged(Slider *)
{
	auto newValue = slider->getValue();

	auto actualValue = owner.setRangeValue(row, (TableFloatingTileBase::ColumnId)columnId, newValue);

	if (newValue != actualValue)
		slider->setValue(actualValue, dontSendNotification);
}

TableFloatingTileBase::TableFloatingTileBase(FloatingTile* parent) :
	FloatingTileContent(parent),
	font(GLOBAL_FONT())
{

}

void TableFloatingTileBase::initTable()
{
	// Create our table component and add it to this component..
	addAndMakeVisible(table);
	table.setModel(this);

	// give it a border

	textColour = Colours::white.withAlpha(0.5f);

	setDefaultPanelColour(FloatingTileContent::PanelColourId::bgColour, Colours::transparentBlack);
	setDefaultPanelColour(FloatingTileContent::PanelColourId::itemColour1, Colours::white.withAlpha(0.5f));
	setDefaultPanelColour(FloatingTileContent::PanelColourId::itemColour2, Colours::white.withAlpha(0.5f));
	setDefaultPanelColour(FloatingTileContent::PanelColourId::itemColour3, Colours::white.withAlpha(0.5f));
	setDefaultPanelColour(FloatingTileContent::PanelColourId::textColour, textColour);

	

	table.setColour(ListBox::backgroundColourId, Colours::transparentBlack);
	table.setOutlineThickness(0);

	laf = new TableHeaderLookAndFeel();

	table.getHeader().setLookAndFeel(laf);
	table.getHeader().setSize(getWidth(), 22);
	table.getViewport()->setScrollBarsShown(true, false, true, false);
	table.getHeader().setInterceptsMouseClicks(false, false);
	table.setMultipleSelectionEnabled(false);

	auto first = getIndexName();

	auto fWidth = (int)font.getStringWidthFloat(first) + 20;

	table.getHeader().addColumn(getIndexName(), CCNumber, fWidth, fWidth, fWidth);
	table.getHeader().addColumn("Parameter", ParameterName, 70);
	table.getHeader().addColumn("Inverted", Inverted, 50, 50, 50);
	table.getHeader().addColumn("Min", Minimum, 70, 70, 70);
	table.getHeader().addColumn("Max", Maximum, 70, 70, 70);
	table.getHeader().setStretchToFitActive(true);
}

void TableFloatingTileBase::updateContent()
{
	table.updateContent();
}

void TableFloatingTileBase::fromDynamicObject(const var& object)
{
	FloatingTileContent::fromDynamicObject(object);

	table.setColour(ListBox::backgroundColourId, findPanelColour(FloatingTileContent::PanelColourId::bgColour));

	itemColour1 = findPanelColour(FloatingTileContent::PanelColourId::itemColour1);
	itemColour2 = findPanelColour(FloatingTileContent::PanelColourId::itemColour2);
	textColour = findPanelColour(FloatingTileContent::PanelColourId::textColour);

	font = getFont();
	laf->f = font;
	laf->bgColour = itemColour1;
	laf->textColour = textColour;
}

void TableFloatingTileBase::paintRowBackground(Graphics& g, int /*rowNumber*/, int /*width*/, int /*height*/, bool rowIsSelected)
{
	if (rowIsSelected)
	{
		g.fillAll(Colours::white.withAlpha(0.2f));
	}
}

void TableFloatingTileBase::resized()
{
	table.setBounds(getLocalBounds());
}

double TableFloatingTileBase::setRangeValue(int row, ColumnId column, double newRangeValue)
{
	if (isUsed(row))
	{
		auto range = getRange(row);

		if (column == Minimum)
		{
			if (range.end <= newRangeValue)
			{
				return range.end;
			}
			else
			{
				range.start = newRangeValue;

				const bool ok = setRange(row, range);
				jassert(ok);
				ignoreUnused(ok);

				return newRangeValue;
			}
		}
		else if (column == Maximum)
		{
			if (range.start >= newRangeValue)
			{
				return range.start;
			}
			else
			{
				range.end = newRangeValue;

				const bool ok = setRange(row, range);
				jassert(ok);
				ignoreUnused(ok);

				return newRangeValue;
			}
		}

		else jassertfalse;
	}

	return -1.0 * newRangeValue;
}

void TableFloatingTileBase::deleteKeyPressed(int lastRowSelected)
{
	if (isUsed(lastRowSelected))
	{
		removeEntry(lastRowSelected);
	}

	updateContent();
	repaint();
}

Component* TableFloatingTileBase::refreshComponentForCell(int rowNumber, int columnId, bool /*isRowSelected*/, Component* existingComponentToUpdate)
{
	//auto data = handler.getDataFromIndex(rowNumber);

	if (columnId == Minimum || columnId == Maximum)
	{
		ValueSliderColumn* slider = dynamic_cast<ValueSliderColumn*> (existingComponentToUpdate);

		if (slider == nullptr)
			slider = new ValueSliderColumn(*this);

		auto pRange = getRange(rowNumber);
		auto fullRange = getFullRange(rowNumber);

		const double value = columnId == Maximum ? pRange.end : pRange.start;

		slider->slider->setColour(Slider::ColourIds::backgroundColourId, Colours::transparentBlack);
		slider->slider->setColour(Slider::ColourIds::thumbColourId, itemColour1);
		slider->slider->setColour(Slider::ColourIds::textBoxTextColourId, textColour);

		slider->setRowAndColumn(rowNumber, (ColumnId)columnId, value, fullRange);

		return slider;
	}
	else if (columnId == Inverted)
	{
		InvertedButton* b = dynamic_cast<InvertedButton*> (existingComponentToUpdate);

		if (b == nullptr)
			b = new InvertedButton(*this);

		b->t->setColour(TextButton::buttonOnColourId, itemColour1);
		b->t->setColour(TextButton::textColourOnId, textColour);
		b->t->setColour(TextButton::buttonColourId, Colours::transparentBlack);
		b->t->setColour(TextButton::textColourOffId, textColour);

		b->setRowAndColumn(rowNumber, isInverted(rowNumber));

		return b;
	}
	{
		// for any other column, just return 0, as we'll be painting these columns directly.

		jassert(existingComponentToUpdate == nullptr);
		return nullptr;
	}
}

void TableFloatingTileBase::paintCell(Graphics& g, int rowNumber, int columnId, int width, int height, bool /*rowIsSelected*/)
{
	g.setColour(textColour);
	g.setFont(font);
	auto text = getCellText(rowNumber, columnId);

	g.drawText(text, 2, 0, width - 4, height, Justification::centredLeft, true);
}

} // namespace hise
