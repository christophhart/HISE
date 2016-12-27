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



ToggleButtonList::ToggleButtonList(StringArray& names, Listener* listener_) :
	listener(listener_)
{
	rebuildList(names);
}

void ToggleButtonList::rebuildList(const StringArray &names)
{
	removeAllChildren();
	buttons.clear();

	for (int i = 0; i < names.size(); i++)
	{
		ToggleButton* button = new ToggleButton(names[i]);
		addAndMakeVisible(button);
		button->setColour(ToggleButton::ColourIds::textColourId, Colours::white);
		
		button->setSize(250, 24);
		button->addListener(this);
		buttons.add(button);

	}

	setSize(250, buttons.size() * 26);
	resized();
}

void ToggleButtonList::buttonClicked(Button* b)
{
	const int index = buttons.indexOf(dynamic_cast<ToggleButton*>(b));
	const bool state = b->getToggleState();
	if (listener != nullptr)
	{
		listener->toggleButtonWasClicked(this, index, state);
	}
}

void ToggleButtonList::resized()
{
	int y = 0;

	for (int i = 0; i < buttons.size(); i++)
	{
		buttons[i]->setTopLeftPosition(0, y);
		y = buttons[i]->getBottom() + 2;
	}
}

void ToggleButtonList::setValue(int index, bool value, NotificationType notify /*= dontSendNotification*/)
{
	if (index >= 0 && index < buttons.size())
	{
		buttons[index]->setToggleState(value, notify);
	}
}

CustomSettingsWindow::CustomSettingsWindow(MainController* mc_) :
	mc(mc_)
{
	addAndMakeVisible(deviceSelector = new ComboBox("Driver"));
	addAndMakeVisible(soundCardSelector = new ComboBox("Device"));
	addAndMakeVisible(outputSelector = new ComboBox("Output"));
	addAndMakeVisible(sampleRateSelector = new ComboBox("Sample Rate"));
	addAndMakeVisible(bufferSelector = new ComboBox("Buffer Sizes"));
	addAndMakeVisible(sampleRateSelector = new ComboBox("Sample Rate"));
	addAndMakeVisible(diskModeSelector = new ComboBox("Hard Disk"));
	addAndMakeVisible(clearMidiLearn = new TextButton("Clear MIDI CC"));

	deviceSelector->addListener(this);
	soundCardSelector->addListener(this);
	outputSelector->addListener(this);
	bufferSelector->addListener(this);
	sampleRateSelector->addListener(this);
	diskModeSelector->addListener(this);
	clearMidiLearn->addListener(this);

	deviceSelector->setLookAndFeel(&plaf);
	soundCardSelector->setLookAndFeel(&plaf);
	outputSelector->setLookAndFeel(&plaf);
	bufferSelector->setLookAndFeel(&plaf);
	sampleRateSelector->setLookAndFeel(&plaf);
	diskModeSelector->setLookAndFeel(&plaf);
	clearMidiLearn->setLookAndFeel(&blaf);
	clearMidiLearn->setColour(TextButton::ColourIds::textColourOffId, Colours::white);
	clearMidiLearn->setColour(TextButton::ColourIds::textColourOnId, Colours::white);

	rebuildMenus(true, true);

	setSize(250, 300);
}

CustomSettingsWindow::~CustomSettingsWindow()
{
	deviceSelector->removeListener(this);
	sampleRateSelector->removeListener(this);
	bufferSelector->removeListener(this);
	soundCardSelector->removeListener(this);
	outputSelector->removeListener(this);
	clearMidiLearn->removeListener(this);
	diskModeSelector->removeListener(this);

	deviceSelector = nullptr;
	bufferSelector = nullptr;
	sampleRateSelector = nullptr;
	diskModeSelector = nullptr;
	clearMidiLearn = nullptr;
}

void CustomSettingsWindow::rebuildMenus(bool rebuildDeviceTypes, bool rebuildDevices)
{
	AudioProcessorDriver* driver = dynamic_cast<AudioProcessorDriver*>(mc);
	const OwnedArray<AudioIODeviceType> *devices = &driver->deviceManager->getAvailableDeviceTypes();

	bufferSelector->clear();
	sampleRateSelector->clear();
	outputSelector->clear();


	if (rebuildDeviceTypes)
	{
		deviceSelector->clear();

		for (int i = 0; i < devices->size(); i++)
		{
			deviceSelector->addItem(devices->getUnchecked(i)->getTypeName(), i + 1);
		};
	}

	AudioIODevice* currentDevice = driver->deviceManager->getCurrentAudioDevice();

	if (currentDevice != nullptr)
	{
		const int thisDevice = devices->indexOf(driver->deviceManager->getCurrentDeviceTypeObject());

		AudioIODeviceType *currentDeviceType = devices->getUnchecked(thisDevice);

		if (rebuildDeviceTypes && thisDevice != -1)
		{
			deviceSelector->setSelectedItemIndex(thisDevice, dontSendNotification);
		}

		Array<int> bufferSizes = currentDevice->getAvailableBufferSizes();

		if (bufferSizes.size() > 7)
		{
			Array<int> powerOfTwoBufferSizes;
			powerOfTwoBufferSizes.ensureStorageAllocated(6);
			if (bufferSizes.contains(64)) powerOfTwoBufferSizes.add(64);
			if (bufferSizes.contains(128)) powerOfTwoBufferSizes.add(128);
			if (bufferSizes.contains(256)) powerOfTwoBufferSizes.add(256);
			if (bufferSizes.contains(512)) powerOfTwoBufferSizes.add(512);
			if (bufferSizes.contains(1024)) powerOfTwoBufferSizes.add(1024);

			bufferSizes.swapWith(powerOfTwoBufferSizes);
		}

		outputSelector->addItemList(getChannelPairs(currentDevice), 1);
		const int thisOutputName = (currentDevice->getActiveOutputChannels().getHighestBit() - 1) / 2;
		outputSelector->setSelectedItemIndex(thisOutputName, dontSendNotification);

		if (rebuildDevices)
		{
			soundCardSelector->clear();

			StringArray soundCardNames = currentDeviceType->getDeviceNames(false);
			soundCardSelector->addItemList(soundCardNames, 1);
			const int soundcardIndex = soundCardNames.indexOf(currentDevice->getName());
			soundCardSelector->setSelectedItemIndex(soundcardIndex, dontSendNotification);
		}


		for (int i = 0; i < bufferSizes.size(); i++)
		{
			bufferSelector->addItem(String(bufferSizes[i]) + String(" Samples"), i + 1);
		}

		const int thisBufferSize = currentDevice->getCurrentBufferSizeSamples();

		bufferSelector->setSelectedItemIndex(bufferSizes.indexOf(thisBufferSize), dontSendNotification);

		Array<double> samplerates = currentDevice->getAvailableSampleRates();

		for (int i = 0; i < samplerates.size(); i++)
		{
			sampleRateSelector->addItem(String(samplerates[i], 0), i + 1);
		}

		const double thisSampleRate = currentDevice->getCurrentSampleRate();

		sampleRateSelector->setSelectedItemIndex(samplerates.indexOf(thisSampleRate), dontSendNotification);
	}

	diskModeSelector->clear();
	diskModeSelector->addItem("Fast - SSD", 1);
	diskModeSelector->addItem("Slow - HDD", 2);

	diskModeSelector->setSelectedItemIndex(driver->diskMode, dontSendNotification);
}

void CustomSettingsWindow::buttonClicked(Button* /*b*/)
{
	ScopedLock sl(mc->getLock());

	mc->getMacroManager().getMidiControlAutomationHandler()->clear();
}

void CustomSettingsWindow::flipEnablement(AudioDeviceManager* manager, const int row)
{
	AudioDeviceManager::AudioDeviceSetup config;
	manager->getAudioDeviceSetup(config);

	BigInteger& original = config.outputChannels;

	original.clear();
	original.setBit(row * 2, 1);
	original.setBit(row * 2 + 1, 1);

	config.useDefaultOutputChannels = false;

	manager->setAudioDeviceSetup(config, true);
}

String CustomSettingsWindow::getNameForChannelPair(const String& name1, const String& name2)
{
	String commonBit;

	for (int j = 0; j < name1.length(); ++j)
		if (name1.substring(0, j).equalsIgnoreCase(name2.substring(0, j)))
			commonBit = name1.substring(0, j);

	// Make sure we only split the name at a space, because otherwise, things
	// like "input 11" + "input 12" would become "input 11 + 2"
	while (commonBit.isNotEmpty() && !CharacterFunctions::isWhitespace(commonBit.getLastCharacter()))
		commonBit = commonBit.dropLastCharacters(1);

	return name1.trim() + " + " + name2.substring(commonBit.length()).trim();
}

StringArray CustomSettingsWindow::getChannelPairs(AudioIODevice* currentDevice)
{
	if (currentDevice != nullptr)
	{
		StringArray items = currentDevice->getOutputChannelNames();

		StringArray pairs;

		for (int i = 0; i < items.size(); i += 2)
		{
			const String& name = items[i];

			if (i + 1 >= items.size())
				pairs.add(name.trim());
			else
				pairs.add(getNameForChannelPair(name, items[i + 1]));
		}

		return pairs;
	}

	return StringArray();
}

void CustomSettingsWindow::comboBoxChanged(ComboBox* comboBoxThatHasChanged)
{
	AudioProcessorDriver* driver = dynamic_cast<AudioProcessorDriver*>(mc);

	if (comboBoxThatHasChanged == deviceSelector)
	{
		const String deviceName = deviceSelector->getText();

		driver->setAudioDeviceType(deviceName);

		rebuildMenus(false, true);
	}
	else if (comboBoxThatHasChanged == soundCardSelector)
	{
		const String name = soundCardSelector->getText();

		driver->setAudioDevice(name);

		rebuildMenus(false, false);

		DBG(name);
	}
	else if (comboBoxThatHasChanged == outputSelector)
	{
		const String outputName = outputSelector->getText();

		flipEnablement(driver->deviceManager, outputSelector->getSelectedItemIndex());

		//driver->setOutputChannelName(outputSelector->getSelectedItemIndex());

		//DBG(outputName);
	}
	else if (comboBoxThatHasChanged == bufferSelector)
	{
		const int bufferSize = bufferSelector->getText().getIntValue();
		driver->setCurrentBlockSize(bufferSize);
	}
	else if (comboBoxThatHasChanged == sampleRateSelector)
	{
		const double sampleRate = (double)sampleRateSelector->getText().getIntValue();
		driver->setCurrentSampleRate(sampleRate);
	}
	else if (comboBoxThatHasChanged == diskModeSelector)
	{
		const int index = diskModeSelector->getSelectedItemIndex();

		driver->diskMode = index;

		mc->getSampleManager().setDiskMode((MainController::SampleManager::DiskMode)index);
	}
}

void CustomSettingsWindow::paint(Graphics& g)
{
	g.setColour(Colours::white);

	g.setFont(GLOBAL_FONT());
	g.drawText("Driver", 0, 0, getWidth() / 2 - 30, 30, Justification::centredRight);
	g.drawText("Device", 0, 40, getWidth() / 2 - 30, 30, Justification::centredRight);
	g.drawText("Output", 0, 80, getWidth() / 2 - 30, 30, Justification::centredRight);
	g.drawText("Buffer Size", 0, 120, getWidth() / 2 - 30, 30, Justification::centredRight);
	g.drawText("Sample Rate", 0, 160, getWidth() / 2 - 30, 30, Justification::centredRight);
	g.drawText("Streaming Mode", 0, 200, getWidth() / 2 - 30, 30, Justification::centredRight);
}

void CustomSettingsWindow::resized()
{
	deviceSelector->setBounds(getWidth() / 2 - 20, 0, getWidth() / 2, 30);
	soundCardSelector->setBounds(getWidth() / 2 - 20, 40, getWidth() / 2, 30);
	outputSelector->setBounds(getWidth() / 2 - 20, 80, getWidth() / 2, 30);
	bufferSelector->setBounds(getWidth() / 2 - 20, 120, getWidth() / 2, 30);
	sampleRateSelector->setBounds(getWidth() / 2 - 20, 160, getWidth() / 2, 30);
	diskModeSelector->setBounds(getWidth() / 2 - 20, 200, getWidth() / 2, 30);
	clearMidiLearn->setBounds(10, 240, getWidth() - 20, 30);
}

CombinedSettingsWindow::CombinedSettingsWindow(MainController* mc_):
	mc(mc_)
{
	setLookAndFeel(&klaf);

	addAndMakeVisible(closeButton = new ShapeButton("Close", Colours::white.withAlpha(0.6f), Colours::white, Colours::white));

	Path closeShape;

	closeShape.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::closeIcon, sizeof(HiBinaryData::ProcessorEditorHeaderIcons::closeIcon));
	closeButton->setShape(closeShape, true, true, true);
	closeButton->addListener(this);
	addAndMakeVisible(settings = new CustomSettingsWindow(mc));

	StringArray midiNames = MidiInput::getDevices();
	numMidiDevices = midiNames.size();

	addAndMakeVisible(midiSources = new ToggleButtonList(midiNames, this));
	midiSources->startTimer(4000);

	settings->setLookAndFeel(&klaf);

	AudioProcessorDriver::updateMidiToggleList(mc, midiSources);

	setSize(600, settings->getHeight() + midiSources->getHeight() + 70);

	closeButton->setTooltip("Close this dialog");


}

CombinedSettingsWindow::~CombinedSettingsWindow()
{
	closeButton = nullptr;
	settings = nullptr;
	mc = nullptr;
	midiSources = nullptr;
}

void CombinedSettingsWindow::resized()
{
	closeButton->setBounds(getWidth() - 24, 2, 20, 20);
	settings->setTopLeftPosition((getWidth() - settings->getWidth()) / 2, 40);
	midiSources->setTopLeftPosition((getWidth() - settings->getWidth()) / 2, settings->getBottom() + 15);
}

void CombinedSettingsWindow::paint(Graphics &g)
{
	g.fillAll(Colour(0xff111111).withAlpha(0.92f));

	g.setColour(Colours::white.withAlpha(0.2f));
	g.drawRect(getLocalBounds(), 1);

	g.setColour(Colours::white.withAlpha(0.1f));

	Rectangle<int> title1(1, 1, getWidth() - 2, 25);
	Rectangle<int> title2(1, settings->getBottom() - 22, getWidth() - 2, 25);

	g.fillRect(title1);
	g.fillRect(title2);

	g.setColour(Colours::white);
	g.setFont(GLOBAL_BOLD_FONT());

	g.drawText("Audio Settings", title1, Justification::centred);
	g.drawText("MIDI Inputs", title2, Justification::centred);
}

void CombinedSettingsWindow::toggleButtonWasClicked(ToggleButtonList* list, int index, bool value)
{
	const String midiName = MidiInput::getDevices()[index];

	dynamic_cast<AudioProcessorDriver*>(mc)->toggleMidiInput(midiName, value);
}

void CombinedSettingsWindow::buttonClicked(Button* b)
{
	findParentComponentOfClass<ModalBaseWindow>()->clearModalComponent();
}

void CombinedSettingsWindow::periodicCheckCallback(ToggleButtonList* list)
{
	const StringArray devices = MidiInput::getDevices();

	if (numMidiDevices != devices.size())
	{
		list->rebuildList(devices);
		numMidiDevices = devices.size();
	}
}
