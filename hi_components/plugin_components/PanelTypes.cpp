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


CustomSettingsWindowPanel::CustomSettingsWindowPanel(FloatingTile* parent) :
	FloatingTileContent(parent)
{
	setDefaultPanelColour(PanelColourId::bgColour, Colours::black);
	setDefaultPanelColour(PanelColourId::textColour, Colours::white);

	addAndMakeVisible(viewport = new Viewport());

	window = new CustomSettingsWindow(getMainController(), parent->shouldCreateChildComponents());

	viewport->setViewedComponent(window);
	viewport->setScrollBarsShown(true, false, true, false);

	if (getMainController()->getCurrentScriptLookAndFeel() != nullptr)
	{
		slaf = new ScriptingObjects::ScriptedLookAndFeel::Laf(getMainController());
		viewport->setLookAndFeel(slaf);
	}

	window->setFont(GLOBAL_BOLD_FONT());

	
}


void CustomSettingsWindowPanel::resized()
{
	viewport->setBounds(getLocalBounds().reduced(5));

	const int delta = viewport->isVerticalScrollBarShown() ? viewport->getScrollBarThickness() : 0;

	window->setSize(getParentShell()->getContentBounds().getWidth() - 5 - delta, window->getHeight());
}


#define SET(x) storePropertyInObject(obj, (int)x, var(window->properties[(int)x]));

var CustomSettingsWindowPanel::toDynamicObject() const
{
	var obj = FloatingTileContent::toDynamicObject();

	SET(CustomSettingsWindow::Properties::Driver);
	SET(CustomSettingsWindow::Properties::Device);
	SET(CustomSettingsWindow::Properties::Output);
	SET(CustomSettingsWindow::Properties::BufferSize);
	SET(CustomSettingsWindow::Properties::SampleRate);
	SET(CustomSettingsWindow::Properties::GlobalBPM);
	SET(CustomSettingsWindow::Properties::StreamingMode);
	SET(CustomSettingsWindow::Properties::ScaleFactor);
	SET(CustomSettingsWindow::Properties::VoiceAmountMultiplier);
	SET(CustomSettingsWindow::Properties::ClearMidiCC);
	SET(CustomSettingsWindow::Properties::SampleLocation);
	SET(CustomSettingsWindow::Properties::DebugMode);
	SET(CustomSettingsWindow::Properties::UseOpenGL);

	storePropertyInObject(obj, (int)CustomSettingsWindow::Properties::ScaleFactorList, var(window->scaleFactorList));

	return obj;
}

#undef SET
#define SET(x) window->setProperty(x, getPropertyWithDefault(object, (int)x));


void CustomSettingsWindowPanel::fromDynamicObject(const var& object)
{
	FloatingTileContent::fromDynamicObject(object);

	SET(CustomSettingsWindow::Properties::Driver);
	SET(CustomSettingsWindow::Properties::Device);
	SET(CustomSettingsWindow::Properties::Output);
	SET(CustomSettingsWindow::Properties::BufferSize);
	SET(CustomSettingsWindow::Properties::SampleRate);
	SET(CustomSettingsWindow::Properties::GlobalBPM);
	SET(CustomSettingsWindow::Properties::StreamingMode);
	SET(CustomSettingsWindow::Properties::ScaleFactor);
	SET(CustomSettingsWindow::Properties::VoiceAmountMultiplier);
	SET(CustomSettingsWindow::Properties::ClearMidiCC);
	SET(CustomSettingsWindow::Properties::SampleLocation);
	SET(CustomSettingsWindow::Properties::DebugMode);
	SET(CustomSettingsWindow::Properties::UseOpenGL);

	window->refreshSizeFromProperties();

	window->setColour(CustomSettingsWindow::ColourIds::textColour, findPanelColour(FloatingTileContent::PanelColourId::textColour));
	window->setColour(CustomSettingsWindow::ColourIds::backgroundColour, findPanelColour(FloatingTileContent::PanelColourId::bgColour));
	window->setFont(getFont());


	auto list = getPropertyWithDefault(object, (int)CustomSettingsWindow::Properties::ScaleFactorList);

	if (list.isArray())
	{
		window->scaleFactorList.clear();

		for (int i = 0; i < list.size(); i++)
			window->scaleFactorList.add(list[i]);



		window->rebuildScaleFactorList();
	}
}

#undef SET
#define SET(x) RETURN_DEFAULT_PROPERTY_ID(index, (int)x, window->propIds[(int)x - (int)FloatingTileContent::PanelPropertyId::numPropertyIds]);


Identifier CustomSettingsWindowPanel::getDefaultablePropertyId(int index) const
{
	if (index < (int)PanelPropertyId::numPropertyIds)
		return FloatingTileContent::getDefaultablePropertyId(index);

	SET(CustomSettingsWindow::Properties::Driver);
	SET(CustomSettingsWindow::Properties::Device);
	SET(CustomSettingsWindow::Properties::Output);
	SET(CustomSettingsWindow::Properties::BufferSize);
	SET(CustomSettingsWindow::Properties::SampleRate);
	SET(CustomSettingsWindow::Properties::GlobalBPM);
	SET(CustomSettingsWindow::Properties::StreamingMode);
	SET(CustomSettingsWindow::Properties::ScaleFactor);
	SET(CustomSettingsWindow::Properties::VoiceAmountMultiplier);
	SET(CustomSettingsWindow::Properties::ClearMidiCC);
	SET(CustomSettingsWindow::Properties::SampleLocation);
	SET(CustomSettingsWindow::Properties::DebugMode);
	SET(CustomSettingsWindow::Properties::ScaleFactorList);
	SET(CustomSettingsWindow::Properties::UseOpenGL);


	jassertfalse;
	return{};
}

#undef SET
#define SET(x) RETURN_DEFAULT_PROPERTY(index, (int)x, true);

var CustomSettingsWindowPanel::getDefaultProperty(int index) const
{
	if (index < (int)PanelPropertyId::numPropertyIds)
		return FloatingTileContent::getDefaultProperty(index);

	SET(CustomSettingsWindow::Properties::Driver);
	SET(CustomSettingsWindow::Properties::Device);
	SET(CustomSettingsWindow::Properties::Output);
	SET(CustomSettingsWindow::Properties::BufferSize);
	SET(CustomSettingsWindow::Properties::SampleRate);
	SET(CustomSettingsWindow::Properties::GlobalBPM);
	SET(CustomSettingsWindow::Properties::StreamingMode);
	SET(CustomSettingsWindow::Properties::ScaleFactor);
	SET(CustomSettingsWindow::Properties::VoiceAmountMultiplier);
	SET(CustomSettingsWindow::Properties::ClearMidiCC);
	SET(CustomSettingsWindow::Properties::SampleLocation);
	SET(CustomSettingsWindow::Properties::DebugMode);
	SET(CustomSettingsWindow::Properties::UseOpenGL);

	if (index == (int)CustomSettingsWindow::Properties::ScaleFactorList) return var({ var(0.5), var(0.75), var(1.0), var(1.25), var(1.5), var(2.0) });

	jassertfalse;
	return{};
}

#undef SET

MidiChannelPanel::MidiChannelPanel(FloatingTile* parent) :
	FloatingTileContent(parent)
{
	setDefaultPanelColour(PanelColourId::bgColour, Colours::black);

	StringArray channelNames;
	channelNames.add("All Channels");
	for (int i = 0; i < 16; i++) channelNames.add("Channel " + String(i + 1));


	addAndMakeVisible(viewport = new Viewport());

	channelList = new ToggleButtonList(channelNames, this);

	viewport->setViewedComponent(channelList);

	viewport->setScrollBarsShown(true, false, true, false);

	setDefaultPanelColour(PanelColourId::textColour, Colours::white);

	if (getMainController()->getCurrentScriptLookAndFeel() != nullptr)
	{
		slaf = new ScriptingObjects::ScriptedLookAndFeel::Laf(getMainController());
		viewport->setLookAndFeel(slaf);
		channelList->setLookAndFeel(slaf);
	}

	//channelList->setLookAndFeel(&tblaf);

	HiseEvent::ChannelFilterData* channelFilterData = getMainController()->getMainSynthChain()->getActiveChannelData();

	channelList->setValue(0, channelFilterData->areAllChannelsEnabled());
	for (int i = 0; i < 16; i++)
	{
		channelList->setValue(i + 1, channelFilterData->isChannelEnabled(i));
	}
}

void MidiChannelPanel::toggleButtonWasClicked(ToggleButtonList* /*list*/, int index, bool value)
{
	HiseEvent::ChannelFilterData *newData = getMainController()->getMainSynthChain()->getActiveChannelData();

	if (index == 0)
	{
		newData->setEnableAllChannels(value);
	}
	else
	{
		newData->setEnableMidiChannel(index - 1, value);
	}
}

void MidiChannelPanel::periodicCheckCallback(ToggleButtonList* /*list*/)
{

}

void MidiChannelPanel::resized()
{
	viewport->setBounds(getParentShell()->getContentBounds().reduced(5));

	const int delta = viewport->isVerticalScrollBarShown() ? viewport->getScrollBarThickness() : 0;

	channelList->setSize(getParentShell()->getContentBounds().getWidth() - 5 - delta, channelList->getHeight());
	channelList->setColourAndFont(findPanelColour(PanelColourId::textColour), getFont());
}

MidiSourcePanel::MidiSourcePanel(FloatingTile* parent) :
	FloatingTileContent(parent)
{
	setDefaultPanelColour(PanelColourId::bgColour, Colours::black);

    StringArray midiInputs;
    
#if HISE_IOS || IS_STANDALONE_APP
    if(!parent->getMainController()->isFlakyThreadingAllowed())
        midiInputs = MidiInput::getDevices();	
#endif

	numMidiDevices = midiInputs.size();

	addAndMakeVisible(viewport = new Viewport());

	midiInputList = new ToggleButtonList(midiInputs, this);

	setDefaultPanelColour(PanelColourId::textColour, Colours::white);

	viewport->setViewedComponent(midiInputList);

	viewport->setScrollBarsShown(true, false, true, false);

	if (getMainController()->getCurrentScriptLookAndFeel() != nullptr)
	{
		slaf = new ScriptingObjects::ScriptedLookAndFeel::Laf(getMainController());
		viewport->setLookAndFeel(slaf);
		midiInputList->setLookAndFeel(slaf);
	}

	//midiInputList->setLookAndFeel(&tblaf);
	midiInputList->startTimer(4000);
	AudioProcessorDriver::updateMidiToggleList(getMainController(), midiInputList);
}

bool MidiSourcePanel::showTitleInPresentationMode() const
{
	return false;
}

void MidiSourcePanel::resized()
{
	viewport->setBounds(getParentShell()->getContentBounds().reduced(5));

	const int delta = viewport->isVerticalScrollBarShown() ? viewport->getScrollBarThickness() : 0;

	midiInputList->setSize(getParentShell()->getContentBounds().getWidth() - 5 - delta, midiInputList->getHeight());

	midiInputList->setColourAndFont(findPanelColour(PanelColourId::textColour), getFont());
}

void MidiSourcePanel::periodicCheckCallback(ToggleButtonList* list)
{
#if HISE_IOS || IS_STANDALONE_APP
	const StringArray devices = MidiInput::getDevices();
#else
	StringArray devices;
#endif

	if (numMidiDevices != devices.size())
	{
		list->rebuildList(devices);
		numMidiDevices = devices.size();
		AudioProcessorDriver::updateMidiToggleList(getMainController(), list);
	}
}

void MidiSourcePanel::toggleButtonWasClicked(ToggleButtonList* /*list*/, int index, bool value)
{
	const String midiName = MidiInput::getDevices()[index];

	dynamic_cast<AudioProcessorDriver*>(getMainController())->toggleMidiInput(midiName, value);
}

} // namespace hise
