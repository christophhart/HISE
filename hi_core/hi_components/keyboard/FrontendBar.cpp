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
*   along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
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


VoiceCounterCpuUsageComponent::VoiceCounterCpuUsageComponent(MainController *mc_) :
mc(mc_)
{
	addAndMakeVisible(cpuSlider = new VuMeter());

	cpuSlider->setColour(VuMeter::backgroundColour, Colours::transparentBlack);
	cpuSlider->setColour(VuMeter::ColourId::ledColour, Colours::white.withAlpha(0.45f));
	cpuSlider->setColour(VuMeter::ColourId::outlineColour, Colours::white.withAlpha(0.4f));

	cpuSlider->setOpaque(false);

	addAndMakeVisible(voiceLabel = new Label());

	voiceLabel->setColour(Label::ColourIds::outlineColourId, Colours::white.withAlpha(0.4f));
	voiceLabel->setColour(Label::ColourIds::textColourId, Colours::white.withAlpha(0.7f));
	voiceLabel->setColour(Label::ColourIds::backgroundColourId, Colours::transparentBlack);
	voiceLabel->setFont(GLOBAL_FONT().withHeight(10.0f));

	voiceLabel->setEditable(false);
    
    addAndMakeVisible(bpmLabel = new Label());
    
    bpmLabel->setColour(Label::ColourIds::outlineColourId, Colours::white.withAlpha(0.4f));
    bpmLabel->setColour(Label::ColourIds::textColourId, Colours::white.withAlpha(0.7f));
    bpmLabel->setColour(Label::ColourIds::backgroundColourId, Colours::transparentBlack);
    bpmLabel->setFont(GLOBAL_FONT().withHeight(10.0f));
    
    bpmLabel->setEditable(false);

	addAndMakeVisible(panicButton = new ShapeButton("Panic", Colours::white.withAlpha(0.6f), Colours::white.withAlpha(0.8f), Colours::white));

	Path panicPath;
	panicPath.loadPathFromData(HiBinaryData::FrontendBinaryData::panicButtonShape, sizeof(HiBinaryData::FrontendBinaryData::panicButtonShape));
	
	panicButton->setShape(panicPath, true, true, false);

	panicButton->addListener(this);

    
    static const unsigned char midiPathData[] = { 110,109,0,0,226,66,92,46,226,67,98,0,0,226,66,112,2,227,67,79,80,223,66,92,174,227,67,0,0,220,66,92,174,227,67,98,177,175,216,66,92,174,227,67,0,0,214,66,112,2,227,67,0,0,214,66,92,46,226,67,98,0,0,214,66,72,90,225,67,177,175,216,66,92,174,224,67,0,0,
        220,66,92,174,224,67,98,79,80,223,66,92,174,224,67,0,0,226,66,72,90,225,67,0,0,226,66,92,46,226,67,99,109,0,128,218,66,92,110,222,67,98,0,128,218,66,112,66,223,67,79,208,215,66,92,238,223,67,0,128,212,66,92,238,223,67,98,177,47,209,66,92,238,223,67,0,
        128,206,66,112,66,223,67,0,128,206,66,92,110,222,67,98,0,128,206,66,72,154,221,67,177,47,209,66,92,238,220,67,0,128,212,66,92,238,220,67,98,79,208,215,66,92,238,220,67,0,128,218,66,72,154,221,67,0,128,218,66,92,110,222,67,99,109,0,128,203,66,92,142,220,
        67,98,0,128,203,66,112,98,221,67,79,208,200,66,92,14,222,67,0,128,197,66,92,14,222,67,98,177,47,194,66,92,14,222,67,0,128,191,66,112,98,221,67,0,128,191,66,92,142,220,67,98,0,128,191,66,72,186,219,67,177,47,194,66,92,14,219,67,0,128,197,66,92,14,219,
        67,98,79,208,200,66,92,14,219,67,0,128,203,66,72,186,219,67,0,128,203,66,92,142,220,67,99,109,0,128,188,66,92,110,222,67,98,0,128,188,66,112,66,223,67,79,208,185,66,92,238,223,67,0,128,182,66,92,238,223,67,98,177,47,179,66,92,238,223,67,0,128,176,66,
        112,66,223,67,0,128,176,66,92,110,222,67,98,0,128,176,66,72,154,221,67,177,47,179,66,92,238,220,67,0,128,182,66,92,238,220,67,98,79,208,185,66,92,238,220,67,0,128,188,66,72,154,221,67,0,128,188,66,92,110,222,67,99,109,0,0,181,66,92,46,226,67,98,0,0,181,
        66,112,2,227,67,79,80,178,66,92,174,227,67,0,0,175,66,92,174,227,67,98,177,175,171,66,92,174,227,67,0,0,169,66,112,2,227,67,0,0,169,66,92,46,226,67,98,0,0,169,66,72,90,225,67,177,175,171,66,92,174,224,67,0,0,175,66,92,174,224,67,98,79,80,178,66,92,174,
        224,67,0,0,181,66,72,90,225,67,0,0,181,66,92,46,226,67,99,109,0,128,197,66,151,79,215,67,98,243,139,173,66,151,79,215,67,0,0,154,66,148,50,220,67,0,0,154,66,151,47,226,67,98,0,0,154,66,154,44,232,67,243,139,173,66,151,15,237,67,0,128,197,66,151,15,237,
        67,98,12,116,221,66,151,15,237,67,0,0,241,66,154,44,232,67,0,0,241,66,151,47,226,67,98,0,0,241,66,148,50,220,67,13,116,221,66,151,79,215,67,0,128,197,66,151,79,215,67,99,109,0,128,197,66,151,79,218,67,98,209,247,214,66,151,79,218,67,0,0,229,66,163,209,
        221,67,0,0,229,66,151,47,226,67,98,0,0,229,66,139,141,230,67,210,247,214,66,151,15,234,67,0,128,197,66,151,15,234,67,98,47,8,180,66,151,15,234,67,0,0,166,66,139,141,230,67,0,0,166,66,151,47,226,67,98,0,0,166,66,163,209,221,67,47,8,180,66,151,79,218,67,
        0,128,197,66,151,79,218,67,99,101,0,0 };
    
    Path midiPath;
    
    
    
    midiPath.loadPathFromData (midiPathData, sizeof (midiPathData));
    
    addAndMakeVisible(midiButton = new ShapeButton("MIDI Input", Colours::white.withAlpha(0.6f), Colours::white.withAlpha(0.8f), Colours::white));
    
    midiButton->setShape(midiPath, true, true, false);
    
    midiButton->setEnabled(false);
    
    panicButton->setTooltip("MIDI Panic (all notes off)");
    
    midiButton->setTooltip("MIDI Activity LED");
    
	setSize(114, 28);

	startTimer(500);
}



void VoiceCounterCpuUsageComponent::buttonClicked(Button *)
{
	mc->allNotesOff();
}


void VoiceCounterCpuUsageComponent::timerCallback()
{
	cpuSlider->setColour(VuMeter::backgroundColour, findColour(Slider::backgroundColourId));

	voiceLabel->setColour(Label::ColourIds::backgroundColourId, findColour(Slider::backgroundColourId));

	cpuSlider->setPeak(mc->getCpuUsage() / 100.0f);
	voiceLabel->setText(String(mc->getNumActiveVoices()), dontSendNotification);
    bpmLabel->setText(String(mc->getBpm(), 0), dontSendNotification);
    
    const bool midiFlag = mc->checkAndResetMidiInputFlag();
    
    Colour c = midiFlag ? Colours::white : Colours::white.withAlpha(0.6f);
    
    midiButton->setColours(c, c, c);
    midiButton->repaint();
}

void VoiceCounterCpuUsageComponent::resized()
{
	panicButton->setBounds(0, 0, 12, 12);
    midiButton->setBounds(0, 14, 12, 12);
	voiceLabel->setBounds(17, 13, 30, 14);
    bpmLabel->setBounds(48, 13, 30, 14);
	cpuSlider->setBounds(79, 13, 30, 14);
}

void VoiceCounterCpuUsageComponent::paint(Graphics& g)
{
	if(isOpaque()) g.fillAll(findColour(Slider::ColourIds::backgroundColourId));

	g.setColour(Colours::white.withAlpha(0.4f));
	g.setFont(GLOBAL_BOLD_FONT().withHeight(10.0f));
	g.drawText("Voices", 16, 3, 50, 11, Justification::left, true);
    
	g.drawText("BPM", 44, 3, 30, 11, Justification::right, true);
    g.drawText("CPU", 76, 3, 30, 11, Justification::right, true);
}

void VoiceCounterCpuUsageComponent::paintOverChildren(Graphics& g)
{
	g.setColour(Colours::white.withAlpha(0.7f));
	g.setFont(GLOBAL_FONT().withHeight(10.0f));
	g.drawText(String(mc->getCpuUsage()) + "%", cpuSlider->getBounds(), Justification::centred, true);
}

FrontendBar::FrontendBar(MainController *mc_) : mc(mc_)
{
	addAndMakeVisible (outMeter = new VuMeter (0.0, 0.0, VuMeter::StereoVertical));
    outMeter->setName ("new component");

	Colour dark(0xFF333333);
	Colour bright(0xFF999999);

	addAndMakeVisible(voiceCpuComponent = new VoiceCounterCpuUsageComponent(mc));

	addAndMakeVisible(tooltipBar = new TooltipBar());

	cpuUpdater.setManualCountLimit(10);

	addAndMakeVisible(presetSelector = new ComboBox());
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
	
	addAndMakeVisible(balanceSlider = new Slider("Balance"));
	balanceSlider->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
	balanceSlider->setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
	balanceSlider->setRange(-1.0, 1.0, 0.01);
	balanceSlider->setLookAndFeel(&fklaf);
	balanceSlider->addListener(this);
	balanceSlider->setValue(mc->getMainSynthChain()->getAttribute(ModulatorSynth::Parameters::Balance), dontSendNotification);
	balanceSlider->setDoubleClickReturnValue(true, 0.0);
	balanceSlider->setTooltip("Stereo Balance: " + String(balanceSlider->getValue() * 100.0, 0));

	addAndMakeVisible(pitchSlider = new Slider("Pitch"));
	pitchSlider->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
	pitchSlider->setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
	pitchSlider->setRange(-12.0, 12.0, 0.01);
	pitchSlider->setLookAndFeel(&fklaf);
	pitchSlider->addListener(this);
	pitchSlider->setDoubleClickReturnValue(true, 0.0);
	pitchSlider->setTooltip("Global Pitch Tuning: " + String(pitchSlider->getValue(), 2) + " st");

	outMeter->setOpaque(false);
	outMeter->setColour(VuMeter::backgroundColour, Colours::transparentBlack);
	outMeter->setColour (VuMeter::ledColour, Colours::lightgrey);
	outMeter->setColour (VuMeter::outlineColour, Colour (0x45000000));
	outMeter->setTooltip("Master output meter");
	
	tooltipBar->setColour(TooltipBar::ColourIds::iconColour, bright.withAlpha(0.6f));
	tooltipBar->setColour(TooltipBar::ColourIds::textColour, bright);
    tooltipBar->setColour(TooltipBar::ColourIds::backgroundColour, Colours::white.withAlpha(0.08f));

	
	voiceCpuComponent->timerCallback();

	setTooltip(" ");

    setSize (800, 32);

	START_TIMER();
}

FrontendBar::~FrontendBar()
{
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
    const File f = UserPresetHandler::getUserPresetFile(mc->getMainSynthChain(), cb->getText());
    UserPresetHandler::loadUserPreset(mc->getMainSynthChain(), f);
}

void FrontendBar::refreshPresetFileList()
{
	Array<File> fileList;

	GET_PROJECT_HANDLER(mc->getMainSynthChain()).getFileList(fileList, ProjectHandler::SubDirectories::UserPresets, "*.preset");

	presetSelector->clear();

	for (int i = 0; i < fileList.size(); i++)
	{
		presetSelector->addItem(fileList[i].getFileNameWithoutExtension(), i + 1);
	}
}

void FrontendBar::sliderValueChanged(Slider* slider)
{
	if (slider == volumeSlider)
	{
		volumeSlider->setTooltip("Main Volume: " + String(slider->getValue(), 1) + " dB");
		
		mc->getMainSynthChain()->setAttribute(ModulatorSynth::Parameters::Gain, Decibels::decibelsToGain(slider->getValue()), sendNotification);
	}
	else if (slider == balanceSlider)
	{
		balanceSlider->setTooltip("Stereo Balance: " + String(slider->getValue() * 100.0, 0));

		mc->getMainSynthChain()->setAttribute(ModulatorSynth::Parameters::Balance, slider->getValue(), sendNotification);
	}
	else if (slider == pitchSlider)
	{
		pitchSlider->setTooltip("Global Pitch Tuning: " + String(slider->getValue(), 2) + " st");
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
