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
	cpuSlider->setColour(VuMeter::ColourId::outlineColour, Colours::white.withAlpha(0.6f));

	cpuSlider->setOpaque(false);

	addAndMakeVisible(voiceLabel = new Label());

	voiceLabel->setColour(Label::ColourIds::outlineColourId, Colours::white.withAlpha(0.6f));
	voiceLabel->setColour(Label::ColourIds::textColourId, Colours::white.withAlpha(0.7f));
	voiceLabel->setColour(Label::ColourIds::backgroundColourId, Colours::transparentBlack);
	voiceLabel->setFont(GLOBAL_FONT().withHeight(10.0f));

	voiceLabel->setEditable(false);

	addAndMakeVisible(panicButton = new ShapeButton("Panic", Colours::white.withAlpha(0.6f), Colours::white.withAlpha(0.5f), Colours::white.withAlpha(0.6f)));

	Path panicPath;
	panicPath.loadPathFromData(HiBinaryData::FrontendBinaryData::panicButtonShape, sizeof(HiBinaryData::FrontendBinaryData::panicButtonShape));
	
	panicButton->setShape(panicPath, true, true, false);

	panicButton->addListener(this);

	setSize(86, 24);

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
}

void VoiceCounterCpuUsageComponent::resized()
{
	

	panicButton->setBounds(0, 4, 20, 20);
	voiceLabel->setBounds(24, 11, 30, 13);
	cpuSlider->setBounds(56, 11, 30, 13);
}

void VoiceCounterCpuUsageComponent::paint(Graphics& g)
{
	if(isOpaque()) g.fillAll(findColour(Slider::ColourIds::backgroundColourId));

	g.setColour(Colours::white.withAlpha(0.6f));
	g.setFont(GLOBAL_BOLD_FONT().withHeight(10.0f));
	g.drawText("Voices", 24, 0, 50, 12, Justification::left, true);
	g.drawText("CPU", 54, 0, 30, 12, Justification::right, true);
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

	
	voiceCpuComponent->timerCallback();

	setTooltip(" ");

    setSize (800, 36);

	START_TIMER();
}

FrontendBar::~FrontendBar()
{
    outMeter = nullptr;
}


void FrontendBar::paint (Graphics& g)
{

}

void FrontendBar::resized()
{
	voiceCpuComponent->setBounds(6, 6, voiceCpuComponent->getWidth(), voiceCpuComponent->getHeight());

	outMeter->setBounds(getRight() - 28, 3, 24, getHeight() - 6);

	volumeSlider->setBounds(outMeter->getX() - 28, 12, 22, 22);
	volumeSliderLabel->setBounds(volumeSlider->getX() - 6, 1, 32, 12);

	balanceSlider->setBounds(volumeSlider->getX() - 28, 12, 22, 22);
	balanceSliderLabel->setBounds(balanceSlider->getX() - 6, 1, 32, 12);

	pitchSlider->setBounds(balanceSlider->getX() - 28, 12, 22, 22);
	pitchSliderLabel->setBounds(pitchSlider->getX() - 6, 1, 32, 12);


	int toolWidth = pitchSlider->getX() - voiceCpuComponent->getRight() - 12;

	tooltipBar->setBounds(voiceCpuComponent->getRight() + 6, 6, toolWidth, getHeight() - 12);
}



void FrontendBar::buttonClicked(Button *b)
{
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
