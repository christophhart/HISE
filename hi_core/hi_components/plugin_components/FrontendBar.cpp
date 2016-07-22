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


FrontendBar::FrontendBar(MainController *mc_) : mc(mc_),
												height(32),
												overlaying(false)
{
	
	addAndMakeVisible (outMeter = new VuMeter (0.0, 0.0, VuMeter::StereoVertical));
    outMeter->setName ("new component");

	Colour dark(0xFF333333);
	Colour bright(0xFF999999);

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

FrontendBar::~FrontendBar()
{
	
	presetSelector = nullptr;

    outMeter = nullptr;

	
}


void FrontendBar::paint (Graphics& g)
{
	if (!isOverlaying())
	{
		g.setGradientFill(ColourGradient(Colours::black.withBrightness(0.1f), 0.0f, 0.0f,
			Colours::black, 0.0f, (float)getHeight(), false));

		g.fillAll();
	}
}

void FrontendBar::resized()
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

	}
	
	int rightX = getRight() - 26;

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

void FrontendBar::setProperties(DynamicObject *p)
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

		const Image *customFilmStrip = getFilmStripImageFromString(imageName);

		const int width = customFilmStrip->getWidth();

		volumeSlider->setSize(width, width);
		pitchSlider->setSize(width, width);
		balanceSlider->setSize(width, width);

		if (customFilmStrip->isValid())
		{
			fklaf.setCustomFilmstripImage(customFilmStrip, numFilmstrips);
		}
	}

	resized();
}

const Image * FrontendBar::getFilmStripImageFromString(const String &fileReference) const
{
#if USE_FRONTEND

	String poolName = ProjectHandler::Frontend::getSanitiziedFileNameForPoolReference(fileReference);

#else

	String poolName = GET_PROJECT_HANDLER(mc->getMainSynthChain()).getFilePath(fileReference, ProjectHandler::SubDirectories::Images);

#endif

	const Image *image = mc->getSampleManager().getImagePool()->loadFileIntoPool(poolName, false);

	return image;
}

DynamicObject * FrontendBar::createDefaultProperties()
{
	DynamicObject *properties = new DynamicObject();

	NamedValueSet *v = &properties->getProperties();

	v->set("height", 32);
	v->set("overlaying", false);
	v->set("bgColour", 0xFF00000);
	v->set("cpuTempoVoicesShown", true);
	v->set("presetShown", true);
	v->set("tooltipBarShown", true);
	v->set("knobsShown", true);
	v->set("knobFilmStrip", String());
	v->set("knobNumFilmStrips", 0);
	v->set("outputMeterShown", true);

	return properties;
}

String FrontendBar::createJSONString(DynamicObject *p/*=nullptr*/)
{
	return JSON::toString((p == nullptr ? createDefaultProperties() : p), false);
}
