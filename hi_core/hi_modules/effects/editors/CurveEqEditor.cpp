/*
  ==============================================================================

  This is an automatically generated GUI class created by the Introjucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Introjucer version: 4.1.0

  ------------------------------------------------------------------------------

  The Introjucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright (c) 2015 - ROLI Ltd.

  ==============================================================================
*/


namespace hise { using namespace juce;


//==============================================================================
CurveEqEditor::CurveEqEditor (ProcessorEditor *p)
    : ProcessorEditorBody(p)
{
	auto eq = dynamic_cast<CurveEq*>(p->getProcessor());

    addAndMakeVisible (typeSelector = new FilterTypeSelector());
    typeSelector->setName ("new component");

    addAndMakeVisible (dragOverlay = new FilterDragOverlay(eq));
    dragOverlay->setName ("new component");

	dragOverlay->addListener(this);

    addAndMakeVisible (enableBandButton = new HiToggleButton ("new toggle button"));
    enableBandButton->setButtonText (TRANS("Enable Band"));
    enableBandButton->addListener (this);
    enableBandButton->setColour (ToggleButton::textColourId, Colours::white);

    addAndMakeVisible (freqSlider = new HiSlider ("Frequency"));
    freqSlider->setTooltip (TRANS("Set the frequency of the selected band"));
    freqSlider->setRange (0, 20000, 1);
    freqSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    freqSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    freqSlider->addListener (this);

    addAndMakeVisible (gainSlider = new HiSlider ("Gain"));
    gainSlider->setRange (-24, 24, 0.1);
    gainSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    gainSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    gainSlider->addListener (this);

    addAndMakeVisible (qSlider = new HiSlider ("Q"));
    qSlider->setRange (0.1, 8, 1);
    qSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    qSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    qSlider->addListener (this);

    addAndMakeVisible (label = new Label ("new label",
                                          TRANS("curve eq")));
    label->setFont (Font ("Arial", 26.00f, Font::bold));
    label->setJustificationType (Justification::centredRight);
    label->setEditable (false, false, false);
    label->setColour (Label::textColourId, Colour (0x52ffffff));
    label->setColour (TextEditor::textColourId, Colours::black);
    label->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    ProcessorEditorLookAndFeel::setupEditorNameLabel(label);
    
    label->setFont(GLOBAL_BOLD_FONT().withHeight(26.0f));
    
	currentlySelectedFilterBand = -1;

	numFilters = 0;

	freqSlider->setup(getProcessor(), -1, "Frequency");
	freqSlider->setMode(HiSlider::Frequency);

	gainSlider->setup(getProcessor(), -1, "Gain");
	gainSlider->setMode(HiSlider::Decibel, -24.0, 24.0, 0.0);

	qSlider->setup(getProcessor(), -1, "Q");
	qSlider->setMode(HiSlider::Linear, 0.1, 8.0, 1.0);

	enableBandButton->setup(getProcessor(), -1, "Enable Band");

	typeSelector->setup(getProcessor(), -1, "Type");

	addAndMakeVisible(fftEnableButton = new ToggleButton("Spectrum Analyser"));
	fftEnableButton->addListener(this);
	fftEnableButton->setTooltip("Enable FFT plotting");
    
    fftEnableButton->setToggleState(eq->getDisplayBuffer(0)->isActive(), dontSendNotification);
	getProcessor()->getMainController()->skin(*fftEnableButton);

    setSize (800, 320);


	h = getHeight();
}

CurveEqEditor::~CurveEqEditor()
{
    typeSelector = nullptr;
    dragOverlay = nullptr;
    enableBandButton = nullptr;
    freqSlider = nullptr;
    gainSlider = nullptr;
    qSlider = nullptr;
    label = nullptr;

}

//==============================================================================
void CurveEqEditor::paint (Graphics& g)
{
	ProcessorEditorLookAndFeel::fillEditorBackgroundRectFixed(g, this, 700);
}

void CurveEqEditor::resized()
{
	auto b = getLocalBounds().withSizeKeepingCentre(700, getHeight() - 6);

	auto right = b.removeFromRight(148).reduced(10);

	dragOverlay->setBounds(b);

	label->setBounds(right.removeFromTop(20));

	right.removeFromTop(15);
	typeSelector->setBounds(right.removeFromTop(32));
	right.removeFromTop(5);
	enableBandButton->setBounds(right.removeFromTop(32));
	right.removeFromTop(5);
	freqSlider->setBounds(right.removeFromTop(48));
	gainSlider->setBounds(right.removeFromTop(48));
	qSlider->setBounds(right.removeFromTop(48));
	right.removeFromTop(10);
	fftEnableButton->setBounds(right.removeFromTop(32));
}

void CurveEqEditor::buttonClicked (Button* buttonThatWasClicked)
{
    if (buttonThatWasClicked == enableBandButton)
    {
		auto eq = dynamic_cast<CurveEq*>(getProcessor());

		if (currentlySelectedFilterBand != -1)
		{
			auto pIndex = CurveEq::BandParameter::numBandParameters * currentlySelectedFilterBand + CurveEq::BandParameter::Enabled;

			eq->setAttribute(pIndex, enableBandButton->getToggleState(), sendNotification);
		}
    }

	if(buttonThatWasClicked == fftEnableButton)
	{
		const bool on = fftEnableButton->getToggleState();

		auto eq = dynamic_cast<CurveEq*>(getProcessor());

		eq->enableSpectrumAnalyser(on);
	}

}

void CurveEqEditor::sliderValueChanged (Slider* sliderThatWasMoved)
{
    if (sliderThatWasMoved == freqSlider)
    {
    }
    else if (sliderThatWasMoved == gainSlider)
    {
    }
    else if (sliderThatWasMoved == qSlider)
    {
    }

	dragOverlay->updatePositions(false);
}


} // namespace hise
