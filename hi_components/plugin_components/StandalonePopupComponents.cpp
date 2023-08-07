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

ToggleButtonList::ToggleButtonList(const StringArray& names, Listener* listener_) :
	listener(listener_)
{
	setLookAndFeel(&btblaf);

	setSize(250, 20);

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
		
		button->setSize(250, 26);
		button->addListener(this);
		buttons.add(button);

	}

	setSize(getWidth(), buttons.size() * 28);
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

	int width = getWidth();

	for (int i = 0; i < buttons.size(); i++)
	{
		buttons[i]->setBounds(0, y, width, buttons[i]->getHeight());
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


#define ADD(x) propIds.add(Identifier(#x));

CustomSettingsWindow::CustomSettingsWindow(MainController* mc_, bool buildMenus) :
	mc(mc_),
    font(GLOBAL_BOLD_FONT())
{
	PopupLookAndFeel plaf;

	ADD(Driver);
	ADD(Device);
	ADD(Output);
	ADD(BufferSize);
	ADD(SampleRate);
	ADD(GlobalBPM);
	ADD(ScaleFactor);
	ADD(UseOpenGL);
	ADD(StreamingMode);
	ADD(VoiceAmountMultiplier);
	ADD(ClearMidiCC);
	ADD(SampleLocation);
	ADD(DebugMode);
	ADD(ScaleFactorList);
	
	setColour(ColourIds::textColour, Colours::white);

    for(int i = 0; i < (int)Properties::numProperties; i++)
    {
        properties[i] = true;
    }
    
#if HISE_USE_OPENGL_FOR_PLUGIN
    properties[(int)Properties::UseOpenGL] = true;
#else
	properties[(int)Properties::UseOpenGL] = false;
#endif

	scaleFactorList = { var(0.5), var(0.75), var(1.0), var(1.25), var(1.5), var(2.0) };

    addAndMakeVisible(deviceSelector = new ComboBox("Driver"));
    addAndMakeVisible(soundCardSelector = new ComboBox("Device"));
    addAndMakeVisible(outputSelector = new ComboBox("Output"));
    addAndMakeVisible(sampleRateSelector = new ComboBox("Sample Rate"));
    addAndMakeVisible(bufferSelector = new ComboBox("Buffer Sizes"));
    addAndMakeVisible(sampleRateSelector = new ComboBox("Sample Rate"));
        
    deviceSelector->addListener(this);
    soundCardSelector->addListener(this);
    outputSelector->addListener(this);
    bufferSelector->addListener(this);
    sampleRateSelector->addListener(this);
        
    deviceSelector->setLookAndFeel(&plaf);
    soundCardSelector->setLookAndFeel(&plaf);
    outputSelector->setLookAndFeel(&plaf);
    bufferSelector->setLookAndFeel(&plaf);
    sampleRateSelector->setLookAndFeel(&plaf);
    
	addAndMakeVisible(bpmSelector = new ComboBox("Global BPM"));
	bpmSelector->addListener(this);
	bpmSelector->setLookAndFeel(&plaf);

	addAndMakeVisible(openGLSelector = new ComboBox("Open GL"));
	addAndMakeVisible(scaleFactorSelector = new ComboBox("Scale Factor"));
	addAndMakeVisible(diskModeSelector = new ComboBox("Hard Disk"));
	addAndMakeVisible(voiceAmountMultiplier = new ComboBox("Voice Amount"));
	addAndMakeVisible(clearMidiLearn = new TextButton("Clear MIDI CC"));
	addAndMakeVisible(relocateButton = new TextButton("Change sample folder location"));
	addAndMakeVisible(debugButton = new TextButton("Toggle Debug Mode"));

	scaleFactorSelector->addListener(this);
	diskModeSelector->addListener(this);
	clearMidiLearn->addListener(this);
	relocateButton->addListener(this);
	debugButton->addListener(this);
	openGLSelector->addListener(this);

	voiceAmountMultiplier->addListener(this);
	voiceAmountMultiplier->setLookAndFeel(&plaf);

	scaleFactorSelector->setLookAndFeel(&plaf);
	diskModeSelector->setLookAndFeel(&plaf);
	clearMidiLearn->setLookAndFeel(&blaf);
	
	for (int i = 0; i < getNumChildComponents(); i++)
		HiseColourScheme::setDefaultColours(*getChildComponent(i));

	debugButton->setLookAndFeel(&blaf);
	clearMidiLearn->setColour(TextButton::ColourIds::textColourOffId, Colours::white);
	clearMidiLearn->setColour(TextButton::ColourIds::textColourOnId, Colours::white);
	relocateButton->setLookAndFeel(&blaf);
	relocateButton->setColour(TextButton::ColourIds::textColourOffId, Colours::white);
	relocateButton->setColour(TextButton::ColourIds::textColourOnId, Colours::white);
	debugButton->setColour(TextButton::ColourIds::textColourOffId, Colours::white);
	debugButton->setColour(TextButton::ColourIds::textColourOnId, Colours::white);

	if (HiseDeviceSimulator::isMobileDevice())
	{
		setProperty(Properties::ScaleFactor, false);
		setProperty(Properties::StreamingMode, false);
		setProperty(Properties::SampleLocation, false);
	}

	if(buildMenus)
		rebuildMenus(true, true);

	if (mc->getCurrentScriptLookAndFeel() != nullptr)
	{
		slaf = new ScriptingObjects::ScriptedLookAndFeel::Laf(mc);

		for (int i = 0; i < getNumChildComponents(); i++)
		{
			getChildComponent(i)->setLookAndFeel(slaf.get());
		}
	}
	else
	{
		for (int i = 0; i < getNumChildComponents(); i++)
		{
			getChildComponent(i)->setLookAndFeel(&glaf);
		}
	}

	setSize(320, 400);
}

#undef ADD



CustomSettingsWindow::~CustomSettingsWindow()
{
	deviceSelector->removeListener(this);
	sampleRateSelector->removeListener(this);
	bufferSelector->removeListener(this);
	soundCardSelector->removeListener(this);
	outputSelector->removeListener(this);
	bpmSelector->removeListener(this);
	openGLSelector->removeListener(this);

	clearMidiLearn->removeListener(this);
	relocateButton->removeListener(this);
	diskModeSelector->removeListener(this);
	
	voiceAmountMultiplier->removeListener(this);

	scaleFactorSelector->removeListener(this);
	debugButton->removeListener(this);

	deviceSelector = nullptr;
	bufferSelector = nullptr;
	sampleRateSelector = nullptr;
	diskModeSelector = nullptr;
	clearMidiLearn = nullptr;
	relocateButton = nullptr;
	debugButton = nullptr;
	voiceAmountMultiplier = nullptr;
	openGLSelector = nullptr;
}

void CustomSettingsWindow::rebuildMenus(bool rebuildDeviceTypes, bool rebuildDevices)
{
	AudioProcessorDriver* driver = dynamic_cast<AudioProcessorDriver*>(mc);
    
	

    if(HiseDeviceSimulator::isStandalone())
    {
		if (driver->deviceManager == nullptr)
		{
			jassertfalse;
			return;
		}

        const OwnedArray<AudioIODeviceType> *devices = &driver->deviceManager->getAvailableDeviceTypes();
        
        bufferSelector->clear(dontSendNotification);
        sampleRateSelector->clear(dontSendNotification);
        outputSelector->clear(dontSendNotification);
		
        
		bpmSelector->clear(dontSendNotification);
		bpmSelector->addItem("Sync to Host", 1);

		for (int i = 60; i < 180; i++)
		{
			bpmSelector->addItem(String(i) + " BPM", i);
		}

        
        if (rebuildDeviceTypes)
        {
            deviceSelector->clear(dontSendNotification);
            
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
            
			auto bufferSizes = HiseSettings::ConversionHelpers::getBufferSizesForDevice(currentDevice);
            
			for (int i = 0; i < bufferSizes.size(); i++)
			{
				bufferSelector->addItem(String(bufferSizes[i]) + String(" Samples"), i + 1);
			}

            outputSelector->addItemList(HiseSettings::ConversionHelpers::getChannelPairs(currentDevice), 1);
            const int thisOutputName = (currentDevice->getActiveOutputChannels().getHighestBit() - 1) / 2;
            outputSelector->setSelectedItemIndex(thisOutputName, dontSendNotification);
            
            if (rebuildDevices)
            {
                soundCardSelector->clear(dontSendNotification);
                
                StringArray soundCardNames = currentDeviceType->getDeviceNames(false);
                soundCardSelector->addItemList(soundCardNames, 1);
                const int soundcardIndex = soundCardNames.indexOf(currentDevice->getName());
                soundCardSelector->setSelectedItemIndex(soundcardIndex, dontSendNotification);
            }
            
            
            const int thisBufferSize = currentDevice->getCurrentBufferSizeSamples();
            
            bufferSelector->setSelectedItemIndex(bufferSizes.indexOf(thisBufferSize), dontSendNotification);
            
			auto samplerates = HiseSettings::ConversionHelpers::getSampleRates(currentDevice);

            for (int i = 0; i < samplerates.size(); i++)
            {
                sampleRateSelector->addItem(String(samplerates[i], 0), i + 1);
            }
            
            const double thisSampleRate = currentDevice->getCurrentSampleRate();
            
            sampleRateSelector->setSelectedItemIndex(samplerates.indexOf(thisSampleRate), dontSendNotification);
        }
        else
        {
            String message = "The Audio driver " + deviceSelector->getText() + " could not be opened.";
            
#if !HISE_IOS
			PresetHandler::showMessageWindow("Audio Driver Initialisation Error", message, PresetHandler::IconType::Error);
#endif
            
            driver->deviceManager->initialiseWithDefaultDevices(0, 2);
            
            if(!loopProtection)
            {
                loopProtection=true;
                rebuildMenus(true, true);
            }
        }
    }
    
	rebuildScaleFactorList();
    
	diskModeSelector->clear(dontSendNotification);
	diskModeSelector->addItem("Fast - SSD", 1);
	diskModeSelector->addItem("Slow - HDD", 2);

	voiceAmountMultiplier->clear(dontSendNotification);
	voiceAmountMultiplier->addItem(String(NUM_POLYPHONIC_VOICES) + " voices", 1);
	voiceAmountMultiplier->addItem(String(NUM_POLYPHONIC_VOICES/2) + " voices", 2);
	voiceAmountMultiplier->addItem(String(NUM_POLYPHONIC_VOICES/4) + " voices", 4);
	voiceAmountMultiplier->addItem(String(NUM_POLYPHONIC_VOICES/8) + " voices", 8);
	
	voiceAmountMultiplier->setSelectedId(driver->voiceAmountMultiplier, dontSendNotification);

	openGLSelector->addItemList({ "Yes", "No" }, 1);
	openGLSelector->setSelectedItemIndex(driver->useOpenGL ? 0 : 1, dontSendNotification);

	bpmSelector->setSelectedId(driver->globalBPM > 0.0 ? (int)driver->globalBPM : 1, dontSendNotification);

	diskModeSelector->setSelectedItemIndex(driver->diskMode, dontSendNotification);
}


void CustomSettingsWindow::refreshSizeFromProperties()
{
	int height = 0;

	for (int i = (int)Properties::Driver; i <= (int)Properties::DebugMode; i++)
	{
		if (properties[i])
			height += 40;
	}

	if (properties[(int)Properties::SampleLocation])
		height += 40;

	setSize(320, height);
}

void CustomSettingsWindow::rebuildScaleFactorList()
{
	AudioProcessorDriver* driver = dynamic_cast<AudioProcessorDriver*>(mc);

	scaleFactorSelector->clear(dontSendNotification);

	for (int i = 0; i < scaleFactorList.size(); i++)
	{
		auto scaleFactor = (double)scaleFactorList[i];
		scaleFactorSelector->addItem(String((int)(scaleFactor*100.0)) + "%", i + 1);
	}

	scaleFactorSelector->setSelectedItemIndex(scaleFactorList.indexOf(driver->getGlobalScaleFactor()), dontSendNotification);
}



void CustomSettingsWindow::buttonClicked(Button* b)
{
	if (b == relocateButton)
	{
#if USE_FRONTEND

		File oldLocation = FrontendHandler::getSampleLocationForCompiledPlugin();

#if HISE_BROWSE_FOLDER_WHEN_RELOCATING_SAMPLES
		FileChooser fc("Select new Sample folder", oldLocation);

		if (fc.browseForDirectory())
		{
			File newLocation = fc.getResult();
#else
        FileChooser fc("Select one of the .ch1 files at the new location", oldLocation, "*.ch1", true);
            
        if(fc.browseForFileToOpen())
        {
            File newLocation = fc.getResult().getParentDirectory();
#endif

			if (newLocation.isDirectory())
			{
				FrontendHandler::setSampleLocation(newLocation);

				auto& handler = mc->getSampleManager().getProjectHandler();

				handler.checkAllSampleReferences();
				
				if (handler.areSampleReferencesCorrect())
				{
					PresetHandler::showMessageWindow("Sample Folder relocated", "You need to close and reopen the plugin to complete this step", PresetHandler::IconType::Info);
				}
			}
		}

#else
		PresetHandler::showMessageWindow("Only useful in compiled plugin", "This button only works for the compiled plugin");
#endif
	}
	else if (b == clearMidiLearn)
	{
		ScopedLock sl(mc->getLock());

		mc->getMacroManager().getMidiControlAutomationHandler()->clear(sendNotification);
	}
	else if (b == debugButton)
	{
		mc->getDebugLogger().toggleLogging();
	}
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
	}
	else if (comboBoxThatHasChanged == openGLSelector)
	{
		driver->useOpenGL = comboBoxThatHasChanged->getSelectedItemIndex() == 0;
		PresetHandler::showMessageWindow("Open GL Setting changed", "Close this window and reopen it in order to apply the changes");
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
	else if (comboBoxThatHasChanged == voiceAmountMultiplier)
	{
		int newVoiceAmount = comboBoxThatHasChanged->getSelectedId();

		driver->voiceAmountMultiplier = newVoiceAmount;

		mc->rebuildVoiceLimits();
		
	}
	else if (comboBoxThatHasChanged == scaleFactorSelector)
	{
		double scaleFactor = scaleFactorList[scaleFactorSelector->getSelectedItemIndex()];
		driver->setGlobalScaleFactor(scaleFactor, sendNotificationSync);
	}
	else if (comboBoxThatHasChanged == bpmSelector)
	{
		int selectedId = bpmSelector->getSelectedId();

		if (selectedId == 1)
			selectedId = -1;

		driver->globalBPM = selectedId;
	}
	else if (comboBoxThatHasChanged == diskModeSelector)
	{
		const int index = diskModeSelector->getSelectedItemIndex();
		driver->diskMode = index;
		mc->getSampleManager().setDiskMode((MainController::SampleManager::DiskMode)index);
	}
}

#define DRAW_LABEL(id, text) if (isOn(id)) { g.drawText(text, 0, y, getWidth() / 2 - 30, 30, Justification::centredRight); y += 40; }

void CustomSettingsWindow::paint(Graphics& g)
{
    g.setColour(findColour((int)ColourIds::textColour));
    
	g.setFont(font);

	int y = 10;

	DRAW_LABEL(Properties::Driver, "Driver");
	DRAW_LABEL(Properties::Device, "Audio Device");
	DRAW_LABEL(Properties::Output, "Output");
	DRAW_LABEL(Properties::BufferSize, "Buffer Size");
	DRAW_LABEL(Properties::SampleRate, "Sample Rate");
	DRAW_LABEL(Properties::GlobalBPM, "Global BPM");
	DRAW_LABEL(Properties::ScaleFactor, "UI Zoom Factor");
	DRAW_LABEL(Properties::UseOpenGL, "Use OpenGL");
	DRAW_LABEL(Properties::StreamingMode, "Streaming Mode");
	DRAW_LABEL(Properties::VoiceAmountMultiplier, "Max Voices");

	if (isOn(Properties::ClearMidiCC))
	{
		y += 40;
	}
	if (isOn(Properties::SampleLocation))
	{

		y += 40;

		const String samplePath = GET_PROJECT_HANDLER(mc->getMainSynthChain()).getSubDirectory(ProjectHandler::SubDirectories::Samples).getFullPathName();

		g.setFont(font);
		g.drawText("Sample Location:", 15, y, getWidth() - 30, 30, Justification::centredTop);
		g.drawText(samplePath, 10, y, getWidth() - 20, 30, Justification::centredBottom);

		y += 80;
	}
}

#undef DRAW_LABEL

#define POSITION_COMBOBOX(id, comboBox) if (isOn(id)) {comboBox->setBounds(getWidth() / 2 - 20, y, getWidth() / 2, 30); y += 40; } else comboBox->setVisible(false);
#define POSITION_BUTTON(id, button) if (isOn(id)) {button->setBounds(10, y, getWidth() - 20, 30); y += 40; } else button->setVisible(false);

void CustomSettingsWindow::resized()
{
    int y = 10;
    
	POSITION_COMBOBOX(Properties::Driver, deviceSelector);
	POSITION_COMBOBOX(Properties::Device, soundCardSelector);
	POSITION_COMBOBOX(Properties::Output, outputSelector);
	POSITION_COMBOBOX(Properties::BufferSize, bufferSelector);
	POSITION_COMBOBOX(Properties::SampleRate, sampleRateSelector);
	POSITION_COMBOBOX(Properties::GlobalBPM, bpmSelector);
	POSITION_COMBOBOX(Properties::ScaleFactor, scaleFactorSelector);
	POSITION_COMBOBOX(Properties::UseOpenGL, openGLSelector);
	POSITION_COMBOBOX(Properties::StreamingMode, diskModeSelector);
	POSITION_COMBOBOX(Properties::VoiceAmountMultiplier, voiceAmountMultiplier);

	POSITION_BUTTON(Properties::ClearMidiCC, clearMidiLearn);
	POSITION_BUTTON(Properties::SampleLocation, relocateButton);

	if (isOn(Properties::SampleLocation))
		y += 40;
	
	POSITION_BUTTON(Properties::DebugMode, debugButton);
}

#undef POSITION_COMBOBOX
#undef POSITION_BUTTON


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

#if HISE_IOS
	setSize(400, settings->getHeight() + midiSources->getHeight() + 70);
#else
    setSize(600, settings->getHeight() + midiSources->getHeight() + 70);
#endif

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
	midiSources->setBounds((getWidth() - settings->getWidth()) / 2, settings->getBottom() + 15, settings->getWidth(), midiSources->getHeight());
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

void CombinedSettingsWindow::toggleButtonWasClicked(ToggleButtonList* /*list*/, int index, bool value)
{
	const String midiName = MidiInput::getDevices()[index];

	dynamic_cast<AudioProcessorDriver*>(mc)->toggleMidiInput(midiName, value);
}

void CombinedSettingsWindow::buttonClicked(Button* )
{
    
    dynamic_cast<AudioProcessorDriver*>(mc)->saveDeviceSettingsAsXml();
    ScopedPointer<XmlElement> deviceData = dynamic_cast<AudioProcessorDriver*>(mc)->deviceManager->createStateXml().release();
    dynamic_cast<AudioProcessorDriver*>(mc)->initialiseAudioDriver(deviceData);

    
	findParentComponentOfClass<ModalBaseWindow>()->clearModalComponent();
}

void CombinedSettingsWindow::periodicCheckCallback(ToggleButtonList* list)
{
	const StringArray devices = MidiInput::getDevices();

	if (numMidiDevices != devices.size())
	{
		list->rebuildList(devices);
		numMidiDevices = devices.size();

		AudioProcessorDriver::updateMidiToggleList(mc, midiSources);
	}
}


} // namespace hise
