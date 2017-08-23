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


DefaultFrontendBar::DefaultFrontendBar(MainController *mc_) : mc(mc_),
												height(32),
												overlaying(false)
{
	
	addAndMakeVisible (outMeter = new VuMeter (0.0, 0.0, VuMeter::StereoVertical));
    outMeter->setName ("new component");

	Colour dark(0xFF333333);
	Colour bright(0xFF999999);

	addAndMakeVisible(deviceSettingsButton = new ShapeButton("Device Settings", Colours::white.withAlpha(0.6f), Colours::white, Colours::white.withAlpha(0.6f)));

	static const unsigned char settings[] = { 110,109,8,103,132,67,84,212,84,67,98,3,255,131,67,84,212,84,67,159,170,131,67,25,125,85,67,159,170,131,67,73,77,86,67,98,159,170,131,67,64,29,87,67,3,255,131,67,99,198,87,67,8,103,132,67,99,198,87,67,98,22,207,132,67,99,198,87,67,219,34,133,67,65,29,
		87,67,219,34,133,67,73,77,86,67,98,219,34,133,67,25,125,85,67,22,207,132,67,84,212,84,67,8,103,132,67,84,212,84,67,99,109,202,224,133,67,213,37,87,67,108,212,190,133,67,113,201,87,67,108,102,251,133,67,94,183,88,67,108,99,3,134,67,201,214,88,67,108,103,
		175,133,67,194,126,89,67,108,156,37,133,67,152,252,88,67,108,205,211,132,67,201,63,89,67,108,72,170,132,67,255,61,90,67,108,251,164,132,67,189,95,90,67,108,70,46,132,67,189,95,90,67,108,230,250,131,67,206,64,89,67,108,24,169,131,67,83,253,88,67,108,244,
		49,131,67,47,118,89,67,108,63,34,131,67,231,133,89,67,108,76,206,130,67,20,222,88,67,108,78,15,131,67,87,202,87,67,108,154,237,130,67,223,38,87,67,108,181,110,130,67,249,211,86,67,108,224,93,130,67,17,201,86,67,108,224,93,130,67,203,219,85,67,108,115,
		237,130,67,230,116,85,67,108,39,15,131,67,149,209,84,67,108,195,210,130,67,37,227,83,67,108,202,202,130,67,221,195,83,67,108,161,30,131,67,46,28,83,67,108,156,168,131,67,31,158,83,67,108,79,250,131,67,146,90,83,67,108,203,35,132,67,127,92,82,67,108,37,
		41,132,67,213,58,82,67,108,210,159,132,67,213,58,82,67,108,59,211,132,67,35,90,83,67,108,209,36,133,67,175,157,83,67,108,0,156,133,67,194,36,83,67,108,219,171,133,67,207,20,83,67,108,197,255,133,67,126,188,83,67,108,195,190,133,67,2,208,84,67,108,91,
		224,133,67,178,115,85,67,108,156,95,134,67,170,198,85,67,108,86,112,134,67,93,209,85,67,108,86,112,134,67,143,190,86,67,108,203,224,133,67,208,37,87,67,99,109,112,6,129,67,84,140,74,67,98,83,76,128,67,84,140,74,67,176,106,127,67,74,186,75,67,176,106,
		127,67,198,46,77,67,98,176,106,127,67,221,162,78,67,83,76,128,67,121,209,79,67,112,6,129,67,121,209,79,67,98,157,192,129,67,121,209,79,67,126,86,130,67,219,162,78,67,126,86,130,67,198,46,77,67,98,125,86,130,67,74,186,75,67,157,192,129,67,84,140,74,67,
		112,6,129,67,84,140,74,67,99,109,80,170,131,67,54,178,78,67,108,142,109,131,67,240,214,79,67,108,238,217,131,67,161,128,81,67,108,60,232,131,67,213,184,81,67,108,248,81,131,67,94,229,82,67,108,110,91,130,67,127,252,81,67,108,17,201,129,67,181,116,82,
		67,108,199,126,129,67,139,59,84,67,108,72,117,129,67,232,119,84,67,108,227,160,128,67,232,119,84,67,108,249,68,128,67,136,118,82,67,108,56,101,127,67,204,253,81,67,108,227,186,125,67,7,214,82,67,108,174,130,125,67,30,242,82,67,108,71,86,124,67,216,197,
		81,67,108,230,62,125,67,127,216,79,67,108,75,198,124,67,9,180,78,67,108,58,0,123,67,183,31,78,67,108,253,195,122,67,60,12,78,67,108,253,195,122,67,182,99,76,67,108,194,197,124,67,158,171,75,67,108,93,62,125,67,105,135,74,67,108,68,102,124,67,205,220,
		72,67,108,193,73,124,67,220,164,72,67,108,195,117,125,67,217,120,71,67,108,128,99,127,67,87,97,72,67,108,235,67,128,67,121,232,71,67,108,36,142,128,67,230,33,70,67,108,180,151,128,67,170,229,69,67,108,7,108,129,67,170,229,69,67,108,2,200,129,67,178,231,
		71,67,108,251,89,130,67,144,96,72,67,108,56,47,131,67,52,136,71,67,108,149,75,131,67,177,107,71,67,108,184,225,131,67,180,151,72,67,108,105,109,131,67,168,132,74,67,108,132,169,131,67,133,169,75,67,108,50,141,132,67,247,61,76,67,108,30,171,132,67,23,
		81,76,67,108,30,171,132,67,123,249,77,67,108,76,170,131,67,56,178,78,67,99,101,0,0 };

	Path settingsPath;
	settingsPath.loadPathFromData(settings, sizeof(settings));

	deviceSettingsButton->setTooltip("Show Audio / MIDI setup");

	deviceSettingsButton->setShape(settingsPath, true, true, true);
	deviceSettingsButton->addListener(this);

#if USE_BACKEND
	deviceSettingsButton->setVisible(false);
#endif

	addAndMakeVisible(voiceCpuComponent = new VoiceCpuBpmComponent(mc));

	addAndMakeVisible(tooltipBar = new TooltipBar());

	cpuUpdater.setManualCountLimit(10);

	addAndMakeVisible(presetSelector = new PresetBox(mc));
	mc->skin(*presetSelector);

	presetSelector->setColour(MacroControlledObject::HiBackgroundColours::outlineBgColour, Colours::transparentBlack);
	presetSelector->setColour(MacroControlledObject::HiBackgroundColours::upperBgColour, Colours::black.withAlpha(0.2f));
	presetSelector->setColour(MacroControlledObject::HiBackgroundColours::lowerBgColour, Colours::black.withAlpha(0.2f));
	
	

	
	addAndMakeVisible(volumeSliderLabel = new Label());
	volumeSliderLabel->setFont(GLOBAL_BOLD_FONT().withHeight(10.0f));
	volumeSliderLabel->setText("Gain", dontSendNotification);
	volumeSliderLabel->setColour(Label::textColourId, Colours::white.withAlpha(0.6f));
	volumeSliderLabel->setEditable(false, false, false);
	volumeSliderLabel->setJustificationType(Justification::centred);

	addAndMakeVisible(balanceSliderLabel = new Label());
	balanceSliderLabel->setFont(GLOBAL_BOLD_FONT().withHeight(10.0f));
	balanceSliderLabel->setText("Pan", dontSendNotification);
	balanceSliderLabel->setColour(Label::textColourId, Colours::white.withAlpha(0.6f));
	balanceSliderLabel->setEditable(false, false, false);
	balanceSliderLabel->setJustificationType(Justification::centred);

	addAndMakeVisible(pitchSliderLabel = new Label());
	pitchSliderLabel->setFont(GLOBAL_BOLD_FONT().withHeight(10.0f));
	pitchSliderLabel->setText("Tune", dontSendNotification);
	pitchSliderLabel->setColour(Label::textColourId, Colours::white.withAlpha(0.6f));
	pitchSliderLabel->setEditable(false, false, false);
	pitchSliderLabel->setJustificationType(Justification::centred);

	addAndMakeVisible(volumeSlider = new Slider("Volume"));
	volumeSlider->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
	volumeSlider->setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
	volumeSlider->setRange(-100.0, 0.0, 0.1);
	volumeSlider->setSkewFactorFromMidPoint(-12.0);
	volumeSlider->setTextValueSuffix(" dB");
	volumeSlider->setLookAndFeel(&fklaf);
	volumeSlider->addListener(this);
	volumeSlider->setValue(Decibels::gainToDecibels(mc->getMainSynthChain()->getAttribute(ModulatorSynth::Parameters::Gain)), dontSendNotification);
	volumeSlider->setDoubleClickReturnValue(true, 0.0);
	volumeSlider->setTooltip("Main Volume: " + String(volumeSlider->getValue(), 1) + " dB");
    volumeSlider->setWantsKeyboardFocus(false);
	volumeSlider->setSize(22, 22);

	addAndMakeVisible(balanceSlider = new Slider("Balance"));
	balanceSlider->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
	balanceSlider->setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
	balanceSlider->setRange(-1.0, 1.0, 0.01);
	balanceSlider->setLookAndFeel(&fklaf);
	balanceSlider->addListener(this);
	balanceSlider->setValue(mc->getMainSynthChain()->getAttribute(ModulatorSynth::Parameters::Balance), dontSendNotification);
	balanceSlider->setDoubleClickReturnValue(true, 0.0);
	balanceSlider->setTooltip("Stereo Balance: " + String(balanceSlider->getValue() * 100.0, 0));
    balanceSlider->setWantsKeyboardFocus(false);
	balanceSlider->setSize(22, 22);

	addAndMakeVisible(pitchSlider = new Slider("Pitch"));
	pitchSlider->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
	pitchSlider->setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
	pitchSlider->setRange(-12.0, 12.0, 0.01);
	pitchSlider->setLookAndFeel(&fklaf);
	pitchSlider->addListener(this);
	pitchSlider->setDoubleClickReturnValue(true, 0.0);
	pitchSlider->setTooltip("Global Pitch Tuning: " + String(pitchSlider->getValue(), 2) + " st");
    pitchSlider->setWantsKeyboardFocus(false);
	pitchSlider->setSize(22, 22);

	outMeter->setOpaque(false);
	outMeter->setColour(VuMeter::backgroundColour, Colours::transparentBlack);
	outMeter->setColour (VuMeter::ledColour, Colours::lightgrey);
	outMeter->setColour (VuMeter::outlineColour, Colour (0x45000000));
	outMeter->setTooltip("Master output meter");
	
	tooltipBar->setColour(TooltipBar::ColourIds::iconColour, bright.withAlpha(0.6f));
	tooltipBar->setColour(TooltipBar::ColourIds::textColour, bright);
    tooltipBar->setColour(TooltipBar::ColourIds::backgroundColour, Colours::white.withAlpha(0.08f));

    setWantsKeyboardFocus(false);
	
	voiceCpuComponent->timerCallback();

	setTooltip(" ");

	setProperties(mc->getToolbarPropertiesObject());


    setSize (800, height);

	START_TIMER();
}

DefaultFrontendBar::~DefaultFrontendBar()
{
	
	presetSelector = nullptr;

    outMeter = nullptr;

	
}


void DefaultFrontendBar::paint (Graphics& g)
{
	if (!isOverlaying())
	{
		g.setGradientFill(ColourGradient(Colours::black.withBrightness(0.1f), 0.0f, 0.0f,
			Colours::black, 0.0f, (float)getHeight(), false));

		g.fillAll();
	}
}

void DefaultFrontendBar::resized()
{
	int leftX = 0;
	const int spaceX = 6;

	if (voiceCpuComponent->isVisible())
	{
		voiceCpuComponent->setBounds(leftX + spaceX, (getHeight() - voiceCpuComponent->getHeight()) / 2, voiceCpuComponent->getWidth(), voiceCpuComponent->getHeight());

		leftX = voiceCpuComponent->getRight();
	}
	
	if (presetSelector->isVisible())
	{
		const int presetHeight = 28;

		presetSelector->setBounds(leftX + spaceX, (getHeight() - presetHeight) / 2, 150, presetHeight);
        
        leftX = presetSelector->getRight();

	}
	
	int rightX = getRight() - getHeight() - 4;

    if(HiseDeviceSimulator::isStandalone())
    {
        deviceSettingsButton->setBounds(rightX, 2, getHeight() - 4, getHeight() - 4);
        rightX = deviceSettingsButton->getX() - 26 - spaceX;
    }
    else
    {
        deviceSettingsButton->setVisible(false);
    }

	if (outMeter->isVisible())
	{
		outMeter->setBounds(rightX, 0, 24, getHeight());

		rightX = outMeter->getX();
	}


	if (volumeSlider->isVisible())
	{
		const int buttonWidth = volumeSlider->getWidth();

		volumeSlider->setBounds(rightX - buttonWidth - spaceX, 12, buttonWidth, buttonWidth);
		volumeSliderLabel->setBounds(Rectangle<int>(volumeSlider->getX(), 1, buttonWidth, 12).expanded(12));

		balanceSlider->setBounds(volumeSlider->getX() - buttonWidth - spaceX, 12, buttonWidth, buttonWidth);
		balanceSliderLabel->setBounds(Rectangle<int>(balanceSlider->getX(), 1, buttonWidth, 12).expanded(12));

		pitchSlider->setBounds(balanceSlider->getX() - buttonWidth - spaceX, 12, buttonWidth, buttonWidth);
		pitchSliderLabel->setBounds(Rectangle<int>(pitchSlider->getX(), 1, buttonWidth, 12).expanded(12));

		rightX = pitchSlider->getX();
	}
	
	if (tooltipBar->isVisible())
	{
		int toolWidth = rightX - leftX - 2 * spaceX;
		int toolHeight = 24;

		tooltipBar->setBounds(leftX + spaceX, (getHeight() - toolHeight) / 2, toolWidth, toolHeight);
	}	
}


void DefaultFrontendBar::buttonClicked(Button* b)
{
	if (b == deviceSettingsButton)
	{
		ModalBaseWindow* bw = findParentComponentOfClass<ModalBaseWindow>();

		bw->setModalComponent(new CombinedSettingsWindow(mc), 0);
	}
}

void DefaultFrontendBar::sliderValueChanged(Slider* slider)
{
	if (slider == volumeSlider)
	{
		volumeSlider->setTooltip("Main Volume: " + String(slider->getValue(), 1) + " dB");
		
		mc->getMainSynthChain()->setAttribute(ModulatorSynth::Parameters::Gain, Decibels::decibelsToGain((float)slider->getValue()), sendNotification);
	}
	else if (slider == balanceSlider)
	{
		balanceSlider->setTooltip("Stereo Balance: " + String(slider->getValue() * 100.0, 0));

		mc->getMainSynthChain()->setAttribute(ModulatorSynth::Parameters::Balance, (float)slider->getValue(), sendNotification);
	}
	else if (slider == pitchSlider)
	{
		pitchSlider->setTooltip("Global Pitch Tuning: " + String(slider->getValue(), 2) + " st");
        mc->setGlobalPitchFactor(slider->getValue());
	}
	tooltipBar->repaint();
}

void DefaultFrontendBar::timerCallback()
{
	const float l = mc->getMainSynthChain()->getDisplayValues().outL;
	const float r = mc->getMainSynthChain()->getDisplayValues().outR;

	outMeter->setPeak(l, r);

	if (cpuUpdater.shouldUpdate())
	{
		volumeSlider->setValue(Decibels::gainToDecibels(mc->getMainSynthChain()->getAttribute(ModulatorSynth::Parameters::Gain)), dontSendNotification);
		balanceSlider->setValue(mc->getMainSynthChain()->getAttribute(ModulatorSynth::Parameters::Balance), dontSendNotification);
	}
}

void DefaultFrontendBar::setProperties(DynamicObject *p)
{
	const NamedValueSet *set = &p->getProperties();

	bgColour = Colour((int)(set->getWithDefault("bgColour", (int)0xFF000000)));
	overlaying = set->getWithDefault("overlaying", false);
	height = set->getWithDefault("height", 32);

	voiceCpuComponent->setVisible(set->getWithDefault("cpuTempoVoicesShown", true));
	presetSelector->setVisible(set->getWithDefault("presetShown", true));
	tooltipBar->setVisible(set->getWithDefault("tooltipBarShown", true));
	volumeSlider->setVisible(set->getWithDefault("knobsShown", true));
	pitchSlider->setVisible(set->getWithDefault("knobsShown", true));
	balanceSlider->setVisible(set->getWithDefault("knobsShown", true));
	volumeSliderLabel->setVisible(set->getWithDefault("knobsShown", true));
	pitchSliderLabel->setVisible(set->getWithDefault("knobsShown", true));
	balanceSliderLabel->setVisible(set->getWithDefault("knobsShown", true));
	outMeter->setVisible(set->getWithDefault("outputMeterShown", true));

	int numFilmstrips = set->getWithDefault("knobNumFilmStrips", 0);

	if (numFilmstrips != 0)
	{
		String imageName = set->getWithDefault("knobFilmStrip", "");

		const Image customFilmStrip = getFilmStripImageFromString(imageName);

		const int width = customFilmStrip.getWidth();

		volumeSlider->setSize(width, width);
		pitchSlider->setSize(width, width);
		balanceSlider->setSize(width, width);

		if (customFilmStrip.isValid())
		{
			fklaf.setCustomFilmstripImage(&customFilmStrip, numFilmstrips);
		}
	}

	resized();
}

const Image DefaultFrontendBar::getFilmStripImageFromString(const String &fileReference) const
{
#if USE_FRONTEND

	String poolName = ProjectHandler::Frontend::getSanitiziedFileNameForPoolReference(fileReference);

#else

	String poolName = GET_PROJECT_HANDLER(mc->getMainSynthChain()).getFilePath(fileReference, ProjectHandler::SubDirectories::Images);

#endif

	const Image image = mc->getSampleManager().getImagePool()->loadFileIntoPool(poolName, false);

	return image;
}

DynamicObject * DefaultFrontendBar::createDefaultProperties()
{
	DynamicObject *properties = new DynamicObject();

	NamedValueSet *v = &properties->getProperties();

	v->set("height", 32);
	v->set("overlaying", false);
	v->set("bgColour", 0xFF00000);
	v->set("cpuTempoVoicesShown", true);
	v->set("presetShown", true);
	v->set("tooltipBarShown", true);
    v->set("keyboard", true);
	v->set("knobsShown", true);
	v->set("knobFilmStrip", String());
	v->set("knobNumFilmStrips", 0);
	v->set("outputMeterShown", true);

	return properties;
}

String DefaultFrontendBar::createJSONString(DynamicObject *p/*=nullptr*/)
{
	return JSON::toString((p == nullptr ? createDefaultProperties() : p), false);
}


DeactiveOverlay::DeactiveOverlay() :
	currentState(0)
{
	addAndMakeVisible(descriptionLabel = new Label());

	descriptionLabel->setFont(GLOBAL_BOLD_FONT());
	descriptionLabel->setColour(Label::ColourIds::textColourId, Colours::white);
	descriptionLabel->setEditable(false, false, false);
	descriptionLabel->setJustificationType(Justification::centredTop);

#if USE_TURBO_ACTIVATE
	addAndMakeVisible(resolveLicenceButton = new TextButton("Register Product Key"));
	addAndMakeVisible(registerProductButton = new TextButton("Request offline activation file"));
	addAndMakeVisible(useActivationResponseButton = new TextButton("Activate with activation response file"));
#else
	addAndMakeVisible(resolveLicenceButton = new TextButton("Use Licence File"));
	addAndMakeVisible(registerProductButton = new TextButton("Online authentication"));
	addAndMakeVisible(useActivationResponseButton = new TextButton("Activate with activation response file"));
#endif
	addAndMakeVisible(resolveSamplesButton = new TextButton("Choose Sample Folder"));

	addAndMakeVisible(ignoreButton = new TextButton("Ignore"));

	resolveLicenceButton->setLookAndFeel(&alaf);
	resolveSamplesButton->setLookAndFeel(&alaf);
	registerProductButton->setLookAndFeel(&alaf);
	useActivationResponseButton->setLookAndFeel(&alaf);
	ignoreButton->setLookAndFeel(&alaf);

	resolveLicenceButton->addListener(this);
	resolveSamplesButton->addListener(this);
	registerProductButton->addListener(this);
	useActivationResponseButton->addListener(this);
	ignoreButton->addListener(this);
}

void DeactiveOverlay::buttonClicked(Button *b)
{
	if (b == resolveLicenceButton)
	{
#if USE_COPY_PROTECTION
		FileChooser fc("Load Licence key file", File(), "*" + ProjectHandler::Frontend::getLicenceKeyExtension(), true);

		if (fc.browseForFileToOpen())
		{
			File f = fc.getResult();

			String keyContent = f.loadFileAsString();

			Unlocker *ul = &dynamic_cast<FrontendProcessor*>(findParentComponentOfClass<FrontendProcessorEditor>()->getAudioProcessor())->unlocker;
			ul->applyKeyFile(keyContent);

			State returnValue = checkLicence(keyContent);

			if (returnValue == numReasons || returnValue == LicenseNotFound)
			{
				f.copyFileTo(ProjectHandler::Frontend::getLicenceKey());

				returnValue = checkLicence();
			}

			refreshLabel();

			if (returnValue != numReasons) return;

			setState(LicenseInvalid, !ul->isUnlocked());

			PresetHandler::showMessageWindow("Valid key file found.", "You found a valid key file. Please reload this instance to activate the plugin.");
		}
#elif USE_TURBO_ACTIVATE

		const String key = PresetHandler::getCustomName("Product Key", "Enter the product key that you've received along with the download link\nIt should have this format: XXXX-XXXX-XXXX-XXXX-XXXX-XXXX-XXXX");

		TurboActivateUnlocker *ul = &dynamic_cast<FrontendProcessor*>(findParentComponentOfClass<FrontendProcessorEditor>()->getAudioProcessor())->unlocker;

		setCustomMessage("Waiting for activation");
		setState(DeactiveOverlay::State::CustomErrorMessage, true);

		ul->activateWithKey(key.getCharPointer());

		setState(DeactiveOverlay::CustomErrorMessage, false);
		setCustomMessage(String());

		setState(DeactiveOverlay::State::CopyProtectionError, !ul->isUnlocked());

		if (ul->isUnlocked())
		{
			PresetHandler::showMessageWindow("Registration successful", "The software is now unlocked and ready to use.");

			FrontendProcessor* fp = dynamic_cast<FrontendProcessor*>(findParentComponentOfClass<FrontendProcessorEditor>()->getAudioProcessor());

			fp->loadSamplesAfterRegistration();
		}
#endif
	}
	else if (b == resolveSamplesButton)
	{
		FileChooser fc("Select Sample Location", ProjectHandler::Frontend::getSampleLocationForCompiledPlugin(), "*.*", true);

		if (fc.browseForDirectory())
		{
			ProjectHandler::Frontend::setSampleLocation(fc.getResult());

			const bool directorySelected = ProjectHandler::Frontend::getSampleLocationForCompiledPlugin().isDirectory();

			if (directorySelected)
			{
				FrontendSampleManager* fp = dynamic_cast<FrontendSampleManager*>(findParentComponentOfClass<AudioProcessorEditor>()->getAudioProcessor());

				fp->checkAllSampleReferences();

				if (fp->areSampleReferencesCorrect())
				{
					PresetHandler::showMessageWindow("Sample Folder changed", "The sample folder was relocated, but you'll need to open a new instance of this plugin before it can be used.");
				}

				setState(SamplesNotFound, !fp->areSampleReferencesCorrect());
			}
			else
			{
				setState(SamplesNotFound, true);
			}


		}
	}
	else if (b == registerProductButton)
	{
#if USE_COPY_PROTECTION
		Unlocker *ul = &dynamic_cast<FrontendProcessor*>(findParentComponentOfClass<FrontendProcessorEditor>()->getAudioProcessor())->unlocker;
		OnlineActivator* activator = new OnlineActivator(ul, this);
		activator->setModalBaseWindowComponent(this);
#elif USE_TURBO_ACTIVATE

		FileChooser fc("Save activation request file", File::getSpecialLocation(File::SpecialLocationType::userDesktopDirectory), "*.xml", true);

		if (fc.browseForFileToSave(true))
		{
			File f = fc.getResult();

#if JUCE_WINDOWS
			TurboActivateCharPointerType path = f.getFullPathName().toUTF16().getAddress();
#else
			TurboActivateCharPointerType path = f.getFullPathName().toUTF8().getAddress();
#endif

			FrontendProcessor* fp = dynamic_cast<FrontendProcessor*>(findParentComponentOfClass<FrontendProcessorEditor>()->getAudioProcessor());
			TurboActivateUnlocker* ul = &fp->unlocker;

			ul->writeActivationFile(ul->getProductKey().c_str(), path);

			PresetHandler::showMessageWindow("Activation file created", "Use this file to activate this machine on a computer with internet connection", PresetHandler::IconType::Info);
		}

#endif
	}
	else if (b == useActivationResponseButton)
	{
#if USE_TURBO_ACTIVATE
		FileChooser fc("Load activation response file", File::getSpecialLocation(File::SpecialLocationType::userDesktopDirectory), "*.xml", true);

		if (fc.browseForFileToOpen())
		{
			File f = fc.getResult();

#if JUCE_WINDOWS
			TurboActivateCharPointerType path = f.getFullPathName().toUTF16().getAddress();
#else
			TurboActivateCharPointerType path = f.getFullPathName().toUTF8().getAddress();
#endif

			FrontendProcessor* fp = dynamic_cast<FrontendProcessor*>(findParentComponentOfClass<FrontendProcessorEditor>()->getAudioProcessor());
			TurboActivateUnlocker* ul = &fp->unlocker;

			setCustomMessage("Waiting for activation");
			setState(DeactiveOverlay::State::CustomErrorMessage, true);

			ul->activateWithFile(path);

			setState(DeactiveOverlay::CustomErrorMessage, false);
			setCustomMessage(String());

			setState(DeactiveOverlay::State::CopyProtectionError, !ul->isUnlocked());

			if (ul->isUnlocked())
			{
				PresetHandler::showMessageWindow("Registration successful", "The software is now unlocked and ready to use.");
				fp->loadSamplesAfterRegistration();
			}

		}
#endif
	}
	else if (b == ignoreButton)
	{
		if (currentState[CustomErrorMessage])
		{
			setState(CustomErrorMessage, false);
		}
		else if (currentState[SamplesNotFound])
		{
			FrontendSampleManager* fp = dynamic_cast<FrontendSampleManager*>(findParentComponentOfClass<AudioProcessorEditor>()->getAudioProcessor());

			// Allows partial sample loading the next time
			fp->setAllSampleReferencesCorrect();

			setState(SamplesNotFound, false);
		}
	}
}

bool DeactiveOverlay::check(State s, const String &value/*=String()*/)
{
#if USE_COPY_PROTECTION
	Unlocker *ul = &dynamic_cast<FrontendProcessor*>(findParentComponentOfClass<FrontendProcessorEditor>()->getAudioProcessor())->unlocker;

	String compareValue;

	switch (s)
	{
	case DeactiveOverlay::AppDataDirectoryNotFound:
		return ProjectHandler::Frontend::getAppDataDirectory().isDirectory();
		break;
	case DeactiveOverlay::LicenseNotFound:
		return ProjectHandler::Frontend::getLicenceKey().existsAsFile();
		break;
	case DeactiveOverlay::ProductNotMatching:
		compareValue = ProjectHandler::Frontend::getProjectName() + String(" ") + ProjectHandler::Frontend::getVersionString();
		return value == compareValue;
		break;
	case DeactiveOverlay::UserNameNotMatching:
		break;
	case DeactiveOverlay::EmailNotMatching:
		break;
	case DeactiveOverlay::MachineNumbersNotMatching:
	{
		StringArray ids = ul->getLocalMachineIDs();
		return ids.contains(value);
		break;
	}
	case DeactiveOverlay::LicenseInvalid:
		return ul->isUnlocked();
		break;
	case DeactiveOverlay::SamplesNotFound:
		break;
	case DeactiveOverlay::numReasons:
		break;
	default:
		break;
	}

#else

	ignoreUnused(s, value);

#endif

	return true;
}


#define CHECK_LICENCE_PARAMETER(state, text){\
if (!check(state, text))\
{ setState(state, true); return state; }\
currentState.setBit(state, false);}


DeactiveOverlay::State DeactiveOverlay::checkLicence(const String &keyContent)
{
#if USE_COPY_PROTECTION
	String key = keyContent;

	if (keyContent.isEmpty())
	{
		key = ProjectHandler::Frontend::getLicenceKey().loadFileAsString();
	}

	StringArray lines = StringArray::fromLines(key);

	if (!check(AppDataDirectoryNotFound) || !check(LicenseNotFound) || lines.size() < 4)
	{
		setState(LicenseNotFound, true);
		return LicenseNotFound;
	}

	currentState.setBit(LicenseNotFound, false);

	const String productName = lines[0].fromFirstOccurrenceOf("Keyfile for ", false, false);
	CHECK_LICENCE_PARAMETER(ProductNotMatching, productName);

	const String keyFileMachineId = lines[3].fromFirstOccurrenceOf("Machine numbers: ", false, false);
	CHECK_LICENCE_PARAMETER(MachineNumbersNotMatching, keyFileMachineId);

	return numReasons;
#else
	ignoreUnused(keyContent);
	return numReasons;
#endif
}

String DeactiveOverlay::getTextForError(State s) const
{
	switch (s)
	{
	case DeactiveOverlay::AppDataDirectoryNotFound:
		return "The application directory is not found. (The installation seems to be broken. Please reinstall this software.)";
		break;
	case DeactiveOverlay::SamplesNotFound:
		return "The sample directory could not be located. \nClick below to choose the sample folder.";
		break;
	case DeactiveOverlay::LicenseNotFound:
	{
#if USE_COPY_PROTECTION
		return "This computer is not registered.\nClick below to authenticate this machine using either online authorization or by loading a license key.";
		break;
#else
		return "";
#endif
	}

	case DeactiveOverlay::ProductNotMatching:
		return "The license key is invalid (wrong plugin name / version).\nClick below to locate the correct license key for this plugin / version";
		break;
	case DeactiveOverlay::MachineNumbersNotMatching:
		return "The machine ID is invalid / not matching.\nClick below to load the correct license key for this computer (or request a new license key for this machine through support.";
		break;
	case DeactiveOverlay::UserNameNotMatching:
		return "The user name is invalid.\nThis means usually a corrupt or rogued license key file. Please contact support to get a new license key.";
		break;
	case DeactiveOverlay::EmailNotMatching:
		return "The email name is invalid.\nThis means usually a corrupt or rogued license key file. Please contact support to get a new license key.";
		break;
	case DeactiveOverlay::CopyProtectionError:
	{
#if USE_TURBO_ACTIVATE
		auto ul = &dynamic_cast<FrontendProcessor*>(findParentComponentOfClass<FrontendProcessorEditor>()->getAudioProcessor())->unlocker;
		return ul->getErrorMessage();
#else
		return "";
#endif
	}
	case DeactiveOverlay::LicenseInvalid:
	{
#if USE_COPY_PROTECTION
		return "The license key is malicious.\nPlease contact support.";
#else
		return "";
#endif
	}
	case DeactiveOverlay::LicenseExpired:
	{
#if USE_COPY_PROTECTION
		return "The license key is expired. Press OK to reauthenticate (you'll need to be online for this)";
#else
		return "";
#endif
	}
	case DeactiveOverlay::CustomErrorMessage:
		return customMessage;
	case DeactiveOverlay::CriticalCustomErrorMessage:
		return customMessage;
	case DeactiveOverlay::numReasons:
		break;
	default:
		break;
	}

	return String();
}

void DeactiveOverlay::resized()
{
	useActivationResponseButton->setVisible(false);

	if (currentState != 0)
	{
		descriptionLabel->centreWithSize(getWidth() - 20, 150);
	}



	if (currentState[CustomErrorMessage])
	{
		resolveLicenceButton->setVisible(false);
		registerProductButton->setVisible(false);
		resolveSamplesButton->setVisible(false);
		ignoreButton->setVisible(true);

		ignoreButton->centreWithSize(200, 32);
	}

	if (currentState[SamplesNotFound])
	{
		resolveLicenceButton->setVisible(false);
		registerProductButton->setVisible(false);
		resolveSamplesButton->setVisible(true);
		ignoreButton->setVisible(true);

		resolveSamplesButton->centreWithSize(200, 32);
		ignoreButton->centreWithSize(200, 32);

		ignoreButton->setTopLeftPosition(ignoreButton->getX(),
			resolveSamplesButton->getY() + 40);
	}

	if (currentState[LicenseNotFound] ||
		currentState[LicenseInvalid] ||
		currentState[MachineNumbersNotMatching] ||
		currentState[UserNameNotMatching] ||
		currentState[ProductNotMatching])
	{
		resolveLicenceButton->setVisible(true);
		registerProductButton->setVisible(true);
		resolveSamplesButton->setVisible(false);
		ignoreButton->setVisible(false);

		resolveLicenceButton->centreWithSize(200, 32);
		registerProductButton->centreWithSize(200, 32);

		resolveLicenceButton->setTopLeftPosition(registerProductButton->getX(),
			registerProductButton->getY() + 40);
	}
	else if (currentState[CopyProtectionError])
	{
		resolveLicenceButton->setVisible(true);

		resolveSamplesButton->setVisible(false);
		ignoreButton->setVisible(false);

		String text = getTextForError(DeactiveOverlay::State::CopyProtectionError);

		if (text == "Connection to the server failed.")
		{
			registerProductButton->setVisible(true);
			resolveLicenceButton->setVisible(false);
			useActivationResponseButton->setVisible(true);

			registerProductButton->centreWithSize(200, 32);
			useActivationResponseButton->centreWithSize(200, 32);

			useActivationResponseButton->setTopLeftPosition(registerProductButton->getX(), registerProductButton->getY() + 40);

		}
		else
		{
			registerProductButton->setVisible(false);
			resolveLicenceButton->centreWithSize(200, 32);
		}

		if (text.contains("TurboActivate.dat"))
		{
			resolveLicenceButton->setVisible(false);
			registerProductButton->setVisible(false);
		}
	}

	if (currentState[CriticalCustomErrorMessage])
	{
		resolveLicenceButton->setVisible(false);
		registerProductButton->setVisible(false);
		resolveSamplesButton->setVisible(false);
		ignoreButton->setVisible(false);
	}
}

#undef CHECK_LICENCE_PARAMETER
