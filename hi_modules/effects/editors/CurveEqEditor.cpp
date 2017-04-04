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

//[Headers] You can add your own extra header files here...

#if JUCE_IOS
#else
#include "../fx/dustfft.c"
#endif

//[/Headers]

#include "CurveEqEditor.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
CurveEqEditor::CurveEqEditor (ProcessorEditor *p)
    : ProcessorEditorBody(p)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (filterGraph = new FilterGraph (0));
    filterGraph->setName ("new component");

    addAndMakeVisible (typeSelector = new FilterTypeSelector());
    typeSelector->setName ("new component");

    addAndMakeVisible (dragOverlay = new FilterDragOverlay());
    dragOverlay->setName ("new component");

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

    addAndMakeVisible (fftRangeSlider = new Slider ("new slider"));
    fftRangeSlider->setRange (-100, -30, 1);
    fftRangeSlider->setSliderStyle (Slider::LinearBar);
    fftRangeSlider->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    fftRangeSlider->setColour (Slider::thumbColourId, Colour (0xafffffff));
    fftRangeSlider->addListener (this);

    addAndMakeVisible (label = new Label ("new label",
                                          TRANS("curve eq")));
    label->setFont (Font ("Arial", 26.00f, Font::bold));
    label->setJustificationType (Justification::centredRight);
    label->setEditable (false, false, false);
    label->setColour (Label::textColourId, Colour (0x52ffffff));
    label->setColour (TextEditor::textColourId, Colours::black);
    label->setColour (TextEditor::backgroundColourId, Colour (0x00000000));


    //[UserPreSize]

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

	updateFilters();
	updateCoefficients();

	enableBandButton->setup(getProcessor(), -1, "Enable Band");

	typeSelector->setup(getProcessor(), -1, "Type");


#if(JUCE_DEBUG)
	startTimer(250);
#else
	startTimer(50);
#endif

	fftRangeSlider->setSliderStyle(Slider::SliderStyle::LinearBarVertical);

	addAndMakeVisible(fftEnableButton = new ShapeButton("Enable FFT", Colours::white.withAlpha(0.5f), Colours::white, Colours::white));
	fftEnableButton->addListener(this);
	fftEnableButton->setClickingTogglesState(true);
	fftEnableButton->setTooltip("Enable FFT plotting");

	static const unsigned char pathData[] = { 110,109,0,0,140,66,92,46,206,67,108,0,0,160,66,92,46,186,67,108,0,0,170,66,92,174,193,67,108,0,0,180,66,92,46,191,67,108,0,0,190,66,92,46,201,67,108,0,0,200,66,92,174,193,67,108,0,0,210,66,92,174,203,67,108,0,0,220,66,92,46,201,67,108,0,0,240,66,92,174,
	203,67,108,0,0,250,66,92,46,201,67,108,0,0,2,67,92,46,206,67,99,101,0,0 };

	Path path;
	path.loadPathFromData (pathData, sizeof (pathData));

	fftEnableButton->setShape(path, true, true, true);
	fftEnableButton->setOutline(Colours::white.withAlpha(0.5f), 1.0f);


    //[/UserPreSize]

    setSize (800, 320);


    //[Constructor] You can add your own custom stuff here..
	h = getHeight();
    //[/Constructor]
}

CurveEqEditor::~CurveEqEditor()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    filterGraph = nullptr;
    typeSelector = nullptr;
    dragOverlay = nullptr;
    enableBandButton = nullptr;
    freqSlider = nullptr;
    gainSlider = nullptr;
    qSlider = nullptr;
    fftRangeSlider = nullptr;
    label = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void CurveEqEditor::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    ProcessorEditorLookAndFeel::fillEditorBackgroundRectFixed(g, this, 700);
    
    //[UserPaint] Add your own custom painting code here..

    //[/UserPaint]
}

void CurveEqEditor::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    filterGraph->setBounds ((getWidth() / 2) + -60 - (504 / 2), 24, 504, 272);
    typeSelector->setBounds ((getWidth() / 2) + 209, 75, 118, 28);
    dragOverlay->setBounds (((getWidth() / 2) + -60 - (504 / 2)) + 0, 24 + 0, roundFloatToInt (504 * 1.0000f), roundFloatToInt (272 * 1.0000f));
    enableBandButton->setBounds ((getWidth() / 2) + 203, 111, 128, 32);
    freqSlider->setBounds ((getWidth() / 2) + 330 - 128, 147, 128, 48);
    gainSlider->setBounds ((getWidth() / 2) + 330 - 128, 201, 128, 48);
    qSlider->setBounds ((getWidth() / 2) + 330 - 128, 252, 128, 48);
    fftRangeSlider->setBounds ((getWidth() / 2) + -340, 46, 20, 250);
    label->setBounds ((getWidth() / 2) + 332 - 264, 16, 264, 40);
    //[UserResized] Add your own custom resize handling here..

	dragOverlay->setEq(this, dynamic_cast<CurveEq*>(getProcessor()));

	fftEnableButton->setBounds(fftRangeSlider->getX(), dragOverlay->getY(), 20, 20);

    //[/UserResized]
}

void CurveEqEditor::buttonClicked (Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == enableBandButton)
    {
        //[UserButtonCode_enableBandButton] -- add your button handler code here..
		if(currentlySelectedFilterBand != -1) filterGraph->enableBand(currentlySelectedFilterBand, enableBandButton->getToggleState());
        //[/UserButtonCode_enableBandButton]
    }

    //[UserbuttonClicked_Post]

	if(buttonThatWasClicked == fftEnableButton)
	{
		const bool on = fftEnableButton->getToggleState();

		if(on)
		{
			dragOverlay->startTimer(30);
			fftRangeSlider->setEnabled(true);
		}
		else
		{
			dragOverlay->stopTimer();
			fftRangeSlider->setEnabled(false);
			dragOverlay->clearFFTDisplay();
		}

		fftEnableButton->setColours(Colours::white.withAlpha(on ? 1.0f : 0.5f), Colours::white.withAlpha(0.7f), Colours::white.withAlpha(0.7f));


	}

    //[/UserbuttonClicked_Post]
}

void CurveEqEditor::sliderValueChanged (Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == freqSlider)
    {
        //[UserSliderCode_freqSlider] -- add your slider handling code here..
        //[/UserSliderCode_freqSlider]
    }
    else if (sliderThatWasMoved == gainSlider)
    {
        //[UserSliderCode_gainSlider] -- add your slider handling code here..
        //[/UserSliderCode_gainSlider]
    }
    else if (sliderThatWasMoved == qSlider)
    {
        //[UserSliderCode_qSlider] -- add your slider handling code here..
        //[/UserSliderCode_qSlider]
    }
    else if (sliderThatWasMoved == fftRangeSlider)
    {
        //[UserSliderCode_fftRangeSlider] -- add your slider handling code here..
		dragOverlay->setFFTRange(fftRangeSlider->getValue());
        //[/UserSliderCode_fftRangeSlider]
    }

    //[UsersliderValueChanged_Post]
	dragOverlay->updatePositions();

    //[/UsersliderValueChanged_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...

void FilterDragOverlay::setEq(CurveEqEditor* editor_, CurveEq* eq_)
{
	eq = eq_;
	editor = editor_;

	dragComponents.clear();

	for(int i = 0; i < eq->getNumFilterBands(); i++)
	{
		Point<int> point = getPosition(i);

		addFilter(point.x, point.y);
	}
}

Point<int> FilterDragOverlay::getPosition(int index)
{
	const int freqIndex = eq->getParameterIndex(index, CurveEq::BandParameter::Freq);
	const int gainIndex = eq->getParameterIndex(index, CurveEq::BandParameter::Gain);

	const int x = (int)editor->getFilterGraph()->freqToX(eq->getAttribute(freqIndex));

	const float gain = eq->getAttribute(gainIndex);

	const int height = editor->getFilterGraph()->getHeight();

	const int y = (height / 2) - (int)((gain / 24.0f) * ((float)height / 2.0f));

	return Point<int>(x,y);
}

void FilterDragOverlay::mouseDown(const MouseEvent &e)
{
    ProcessorEditor *editor2 = findParentComponentOfClass<ProcessorEditor>();
    
    if(editor2 != nullptr)
    {
        PresetHandler::setChanged(editor2->getProcessor());
    }
    
	if(!e.mods.isRightButtonDown())
	{


		const double freq = (double)editor->getFilterGraph()->xToFreq((float)e.getPosition().x);

		const double gain = Decibels::decibelsToGain(getGain(e.getPosition().y));

		eq->addFilterBand(freq, gain);

		addFilter(e.getPosition().x, e.getPosition().y, &e);

	}
}

void FilterDragOverlay::FilterDragComponent::mouseDown (const MouseEvent& e)
{
	FilterDragOverlay *o = dynamic_cast<FilterDragOverlay*>(getParentComponent());

	if(e.mods.isRightButtonDown())
	{

		o->removeFilter(this);
	}
	else
	{
		o->selectDragger(index);
		dragger.startDraggingComponent (this, e);
	}

}

void FilterDragOverlay::removeFilter(FilterDragComponent *componentToRemove)
{
	const int index = dragComponents.indexOf(componentToRemove);

	dragComponents.removeObject(componentToRemove);

	for(int i = 0; i < dragComponents.size(); i++)
	{
		dragComponents[i]->setIndex(i);
	}

	editor->bandRemoved(index);

	eq->removeFilterBand(index);
}

void FilterDragOverlay::FilterDragComponent::mouseDrag (const MouseEvent& e)
{
	dragger.dragComponent (this, e, constrainer);

	const int x = e.getEventRelativeTo(getParentComponent()).getPosition().x;
	const int y = e.getEventRelativeTo(getParentComponent()).getPosition().y;

	const double freq = jlimit<double>(20.0, 20000.0, (double)editor->getFilterGraph()->xToFreq((float)x));

	const double gain = dynamic_cast<FilterDragOverlay*>(getParentComponent())->getGain(y);

	const int freqIndex = eq->getParameterIndex(index, CurveEq::BandParameter::Freq);
	const int gainIndex = eq->getParameterIndex(index, CurveEq::BandParameter::Gain);

	eq->setAttribute(freqIndex, (float)freq, sendNotification);
	eq->setAttribute(gainIndex, (float)gain, sendNotification);



}

void FilterDragOverlay::mouseMove(const MouseEvent &e)
{
	setTooltip(String(getGain(e.getPosition().y), 1) + " dB / " + String((int)editor->getFilterGraph()->xToFreq((float)e.getPosition().x)) + " Hz");
};


void FilterDragOverlay::FilterDragComponent::mouseWheelMove(const MouseEvent &e, const MouseWheelDetails &d)
{
	if(e.mods.isCtrlDown())
	{
		double q = eq->getFilterBand(index)->getQ();

		if(d.deltaY > 0)
		{
			q = jmin<double>(8.0, q * 1.3);
		}
		else
		{
			q = jmax<double>(0.1, q / 1.3);
		}

		const int qIndex = eq->getParameterIndex(index, CurveEq::BandParameter::Q);

		eq->setAttribute(qIndex, (float)q, sendNotification);

	}
	else
	{
		getParentComponent()->mouseWheelMove(e, d);
	}


}

void FilterDragOverlay::selectDragger(int index)
{
	selectedIndex = index;

	for(int i = 0; i < dragComponents.size(); i++)
	{
		dragComponents[i]->setSelected(index == i);
	}

	editor->setSelectedFilterBand(index);

}

void FilterDragOverlay::timerCallback()
{
	const double *d = eq->getExternalData();

	const double max = FloatVectorOperations::findMaximum(d, FFT_SIZE_FOR_EQ);

	if(max == 0)
	{
		FloatVectorOperations::clear(gainValues, FFT_SIZE_FOR_EQ);

		p.clear();

		if(repaintUpdater.shouldUpdate())
		{

			clearFFTDisplay();


		}
		return;
	}

#if JUCE_IOS
    
#else
    
	const int size = FFT_SIZE_FOR_EQ;
	const int half = size / 2;

	for(int i = 0; i < half; i++)
	{
		fftData[i] = std::complex<double>(d[i] * (double)i / (double)(half), 0.0);
	}

	for(int i = half; i < size; i++)
	{
		fftData[i] = std::complex<double>(d[i] * (1.0 - (double)(i - half) / (double)half), 0.0);
	}

	DustFFT_fwdD(reinterpret_cast<double*>(fftData), size);

	for(int i = 0; i < size; i++)
	{
		fftAmpData[i] = sqrt(fftData[i].imag() * fftData[i].imag() + fftData[i].real() * fftData[i].real()) / (double)FFT_SIZE_FOR_EQ;
	}

	for(int i = 0; i < size; i++)
	{
		//gainValues[i] = 0.4f * fftAmpData[i] +  0.8f * gainValues[i];


        gainValues[i] = ((float)fftAmpData[i] > gainValues[i]) ? (float)fftAmpData[i] : 0.9f * gainValues[i];

	}

	if(repaintUpdater.shouldUpdate())
	{


		Path tempPath;

		tempPath.clear();

		const double maxGain = FloatVectorOperations::findMaximum(fftAmpData, FFT_SIZE_FOR_EQ);

		if(maxGain == 0.0) return;

		float w = (float)getWidth();
		float h = (float)getHeight();

		tempPath.startNewSubPath(0.0f, h);

		float lastX = 0.0f;

		for(int i = 2; i < FFT_SIZE_FOR_EQ / 2; i++)
		{
			const float freq = ((float) i / (float)FFT_SIZE_FOR_EQ) * (float)eq->getSampleRate();

			const float x = editor->getFilterGraph()->freqToX(freq);

			if((x - lastX) < 1.0f)
			{
				//continue;
			}

			lastX = x;

			const double gain = Decibels::gainToDecibels(gainValues[i]);

			double y = jmin<double>((double)getHeight(), (double)getHeight() * (gain / fftRange));

			if(y < 0.0) y = 0.0;


			tempPath.lineTo((float)x, (float)y);
		}



		tempPath.lineTo(w, h);

		tempPath.lineTo(0.0f, h);

		tempPath.closeSubPath();

		if(tempPath.getBounds().getHeight() != 0)
		{

			tempPath.scaleToFit(0.0f, 0.0f, w, h, false);

		}



		p = Path(tempPath);

		//FloatVectorOperations::clear(gainValues, FFT_SIZE_FOR_EQ);



		repaint();
	}
    
#endif
}

void FilterDragOverlay::paint(Graphics &g)
{


	g.setColour(Colours::black.withAlpha(0.1f));

	g.fillPath(p);

	g.setColour(Colours::white.withAlpha(0.2f));

	g.strokePath(p, PathStrokeType(1.0f));

}

//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Introjucer information section --

    This is where the Introjucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="CurveEqEditor" componentName=""
                 parentClasses="public ProcessorEditorBody, public Timer, public FilterTypeSelector::Listener"
                 constructorParams="ProcessorEditor *p" variableInitialisers="ProcessorEditorBody(p)&#10;"
                 snapPixels="8" snapActive="1" snapShown="1" overlayOpacity="0.330"
                 fixedSize="1" initialWidth="800" initialHeight="320">
  <BACKGROUND backgroundColour="ffffff">
    <ROUNDRECT pos="0Cc 6 700 304" cornerSize="6" fill="solid: 30000000" hasStroke="1"
               stroke="2, mitered, butt" strokeColour="solid: 25ffffff"/>
  </BACKGROUND>
  <GENERICCOMPONENT name="new component" id="664814e01ba3b705" memberName="filterGraph"
                    virtualName="" explicitFocusOrder="0" pos="-60Cc 24 504 272"
                    class="FilterGraph" params="0"/>
  <GENERICCOMPONENT name="new component" id="f3e7df800e17c4bc" memberName="typeSelector"
                    virtualName="" explicitFocusOrder="0" pos="209C 75 118 28" class="FilterTypeSelector"
                    params=""/>
  <GENERICCOMPONENT name="new component" id="13694c782c19abe6" memberName="dragOverlay"
                    virtualName="" explicitFocusOrder="0" pos="0 0 100% 100%" posRelativeX="664814e01ba3b705"
                    posRelativeY="664814e01ba3b705" posRelativeW="664814e01ba3b705"
                    posRelativeH="664814e01ba3b705" class="FilterDragOverlay" params=""/>
  <TOGGLEBUTTON name="new toggle button" id="e6345feaa3cb5bea" memberName="enableBandButton"
                virtualName="HiToggleButton" explicitFocusOrder="0" pos="203C 111 128 32"
                posRelativeX="410a230ddaa2f2e8" txtcol="ffffffff" buttonText="Enable Band"
                connectedEdges="0" needsCallback="1" radioGroupId="0" state="0"/>
  <SLIDER name="Frequency" id="89cc5b4c20e221e" memberName="freqSlider"
          virtualName="HiSlider" explicitFocusOrder="0" pos="330Cr 147 128 48"
          posRelativeX="f930000f86c6c8b6" tooltip="Set the frequency of the selected band"
          min="0" max="20000" int="1" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="Gain" id="2e806a4ba1068f01" memberName="gainSlider" virtualName="HiSlider"
          explicitFocusOrder="0" pos="330Cr 201 128 48" posRelativeX="f930000f86c6c8b6"
          min="-24" max="24" int="0.10000000000000001" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="Q" id="9d5864f0ab12a1c4" memberName="qSlider" virtualName="HiSlider"
          explicitFocusOrder="0" pos="330Cr 252 128 48" posRelativeX="f930000f86c6c8b6"
          min="0.10000000000000001" max="8" int="1" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="new slider" id="2e6abe603ac2c6ea" memberName="fftRangeSlider"
          virtualName="" explicitFocusOrder="0" pos="-340C 46 20 250" thumbcol="afffffff"
          min="-100" max="-30" int="1" style="LinearBar" textBoxPos="NoTextBox"
          textBoxEditable="1" textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <LABEL name="new label" id="bd1d8d6ad6d04bdc" memberName="label" virtualName=""
         explicitFocusOrder="0" pos="332Cr 16 264 40" textCol="52ffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="curve eq" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Arial"
         fontsize="26" bold="1" italic="0" justification="34"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
//[/EndFile]
