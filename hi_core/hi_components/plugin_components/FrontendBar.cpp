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


FrontendBar::FrontendBar(MainController *mc_) : mc(mc_)
{
	addAndMakeVisible (outMeter = new VuMeter (0.0, 0.0, VuMeter::StereoVertical));
    outMeter->setName ("new component");

	Colour dark(0xFF333333);
	Colour bright(0xFF999999);

	addAndMakeVisible(voiceCpuComponent = new VoiceCpuBpmComponent(mc));

	addAndMakeVisible(tooltipBar = new TooltipBar());

	cpuUpdater.setManualCountLimit(10);

	addAndMakeVisible(presetSelector = new PresetComboBox());
	mc->skin(*presetSelector);

	presetSelector->setColour(MacroControlledObject::HiBackgroundColours::outlineBgColour, Colours::transparentBlack);
	presetSelector->setColour(MacroControlledObject::HiBackgroundColours::upperBgColour, Colours::black.withAlpha(0.2f));
	presetSelector->setColour(MacroControlledObject::HiBackgroundColours::lowerBgColour, Colours::black.withAlpha(0.2f));
	
	presetSelector->setTextWhenNothingSelected("Preset");
	presetSelector->setTooltip("Load a preset");

    presetSelector->addListener(this);
    
	refreshPresetFileList();

	Path savePath;

	static const unsigned char pathData[] = { 110, 109, 0, 0, 144, 65, 0, 0, 0, 64, 108, 0, 0, 176, 65, 0, 0, 0, 64, 108, 0, 0, 176, 65, 0, 0, 48, 65, 108, 0, 0, 144, 65, 0, 0, 48, 65, 99, 109, 0, 0, 192, 64, 0, 0, 208, 65, 108, 0, 0, 208, 65, 0, 0, 208, 65, 108, 0, 0, 208, 65, 0, 0, 224, 65, 108, 0, 0, 192, 64, 0, 0, 224, 65, 99, 109, 0, 0, 192, 64, 0, 0, 176, 65, 108, 0, 0, 208,
		65, 0, 0, 176, 65, 108, 0, 0, 208, 65, 0, 0, 192, 65, 108, 0, 0, 192, 64, 0, 0, 192, 65, 99, 109, 0, 0, 192, 64, 0, 0, 144, 65, 108, 0, 0, 208, 65, 0, 0, 144, 65, 108, 0, 0, 208, 65, 0, 0, 160, 65, 108, 0, 0, 192, 64, 0, 0, 160, 65, 99, 109, 0, 0, 208, 65, 0, 0, 0, 0, 108, 0, 0, 192, 65, 0, 0, 0, 0, 108, 0, 0, 192, 65, 0, 0, 80, 65, 108,
		0, 0, 0, 65, 0, 0, 80, 65, 108, 0, 0, 0, 65, 0, 0, 0, 0, 108, 0, 0, 0, 0, 0, 0, 0, 0, 108, 0, 0, 0, 0, 0, 0, 0, 66, 108, 0, 0, 0, 66, 0, 0, 0, 66, 108, 0, 0, 0, 66, 0, 0, 192, 64, 108, 0, 0, 208, 65, 0, 0, 0, 0, 99, 109, 0, 0, 224, 65, 0, 0, 240, 65, 108, 0, 0, 128, 64, 0, 0, 240, 65, 108, 0, 0, 128, 64, 0, 0, 128, 65, 108, 0, 0, 224, 65, 0, 0,
		128, 65, 108, 0, 0, 224, 65, 0, 0, 240, 65, 99, 101, 0, 0 };

	savePath.loadPathFromData(pathData, sizeof(pathData));

	addAndMakeVisible(presetSaveButton = new ShapeButton("Save Preset", Colours::white.withAlpha(.6f), Colours::white.withAlpha(0.8f), Colours::white));
	presetSaveButton->setShape(savePath, true, true, true);

	presetSaveButton->setTooltip("Save user preset");

    presetSaveButton->addListener(this);
    
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

	addAndMakeVisible(pitchSlider = new Slider("Pitch"));
	pitchSlider->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
	pitchSlider->setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
	pitchSlider->setRange(-12.0, 12.0, 0.01);
	pitchSlider->setLookAndFeel(&fklaf);
	pitchSlider->addListener(this);
	pitchSlider->setDoubleClickReturnValue(true, 0.0);
	pitchSlider->setTooltip("Global Pitch Tuning: " + String(pitchSlider->getValue(), 2) + " st");
    pitchSlider->setWantsKeyboardFocus(false);
    
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

    setSize (800, 32);

	START_TIMER();
}

FrontendBar::~FrontendBar()
{
	presetSelector->hidePopup();
	presetSelector = nullptr;

    outMeter = nullptr;

	
}


void FrontendBar::paint (Graphics& g)
{
    g.setGradientFill(ColourGradient(Colours::black.withBrightness(0.1f), 0.0f, 0.0f,
                      Colours::black, 0.0f, (float)getHeight(), false));
    
    g.fillAll();
}

void FrontendBar::resized()
{
	voiceCpuComponent->setBounds(6, 2, voiceCpuComponent->getWidth(), voiceCpuComponent->getHeight());

	presetSelector->setBounds(voiceCpuComponent->getRight() + 6, 2, 130, 28);

	presetSaveButton->setBounds(presetSelector->getRight() + 1, 6, 20, 20);

	outMeter->setBounds(getRight() - 26, 0, 24, getHeight());

	volumeSlider->setBounds(outMeter->getX() - 28, 12, 22, 22);
	volumeSliderLabel->setBounds(volumeSlider->getX() - 6, 1, 32, 12);

	balanceSlider->setBounds(volumeSlider->getX() - 28, 12, 22, 22);
	balanceSliderLabel->setBounds(balanceSlider->getX() - 6, 1, 32, 12);

	pitchSlider->setBounds(balanceSlider->getX() - 28, 12, 22, 22);
	pitchSliderLabel->setBounds(pitchSlider->getX() - 6, 1, 32, 12);


	int toolWidth = pitchSlider->getX() - presetSaveButton->getRight() - 12;

	tooltipBar->setBounds(presetSaveButton->getRight() + 6, 4, toolWidth, getHeight() - 8);
}



void FrontendBar::buttonClicked(Button *b)
{
    if(b == presetSaveButton)
    {
        UserPresetHandler::saveUserPreset(mc->getMainSynthChain());
        
        refreshPresetFileList();
    }
}

void FrontendBar::comboBoxChanged(ComboBox *cb)
{
#if USE_BACKEND

    const File f = UserPresetHandler::getUserPresetFile(mc->getMainSynthChain(), cb->getText());
    UserPresetHandler::loadUserPreset(mc->getMainSynthChain(), f);

#else

	FrontendProcessor *fp = dynamic_cast<FrontendProcessor*>(mc);

	if (presetSelector->isFactoryPresetSelected())
	{
		fp->setCurrentProgram(cb->getSelectedId());
		
	}
	else
	{
		fp->setCurrentProgram(0);

		const File f = UserPresetHandler::getUserPresetFile(mc->getMainSynthChain(), cb->getText());
		UserPresetHandler::loadUserPreset(mc->getMainSynthChain(), f);
	}
#endif

}

void FrontendBar::refreshPresetFileList()
{
	

#if USE_BACKEND

	Array<File> fileList;
	GET_PROJECT_HANDLER(mc->getMainSynthChain()).getFileList(fileList, ProjectHandler::SubDirectories::UserPresets, "*.preset");

	presetSelector->clearPresets();

	for (int i = 0; i < fileList.size(); i++)
	{
		presetSelector->addFactoryPreset(fileList[i].getFileNameWithoutExtension(), i + 1);
	}

#else

	FrontendProcessor *fp = dynamic_cast<FrontendProcessor*>(mc);

	presetSelector->clearPresets();

	for (int i = 1; i < fp->getNumPrograms(); i++)
	{
		presetSelector->addFactoryPreset(fp->getProgramName(i), i);
	}

	File userPresetDirectory = ProjectHandler::Frontend::getUserPresetDirectory();
	Array<File> userPresets;
	userPresetDirectory.findChildFiles(userPresets, File::findFiles, false, "*.preset");

	for (int i = 0; i < userPresets.size(); i++)
	{
		presetSelector->addUserPreset(userPresets[i].getFileNameWithoutExtension(), i);
	}

	if(presetSelector->isFactoryPresetSelected()) presetSelector->setSelectedId(fp->getCurrentProgram());
	else presetSelector->setSelectedId(-1, dontSendNotification);

#endif

}

void FrontendBar::sliderValueChanged(Slider* slider)
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

void FrontendBar::timerCallback()
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
