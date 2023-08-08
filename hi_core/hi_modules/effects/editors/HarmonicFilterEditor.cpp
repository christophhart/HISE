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

namespace hise { using namespace juce;

//[/Headers]

#include "HarmonicFilterEditor.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
HarmonicFilterEditor::HarmonicFilterEditor (ProcessorEditor *p)
    : ProcessorEditorBody(p)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (sliderPackA = new SliderPack (dynamic_cast<BaseHarmonicFilter*>(getProcessor())->getSliderPackUnchecked(0)));
    sliderPackA->setName ("new component");

    addAndMakeVisible (sliderPackB = new SliderPack (dynamic_cast<BaseHarmonicFilter*>(getProcessor())->getSliderPackUnchecked(1)));
    sliderPackB->setName ("new component");

    addAndMakeVisible (sliderPackMix = new SliderPack (dynamic_cast<BaseHarmonicFilter*>(getProcessor())->getSliderPackUnchecked(2)));
    sliderPackMix->setName ("new component");

    addAndMakeVisible (label2 = new Label ("new label",
                                           TRANS("Spectrum A")));
    label2->setFont (Font ("Arial", 11.50f, Font::plain));
    label2->setJustificationType (Justification::centred);
    label2->setEditable (false, false, false);
    label2->setColour (Label::backgroundColourId, Colour (0x24000000));
    label2->setColour (Label::textColourId, Colour (0x91ffffff));
    label2->setColour (Label::outlineColourId, Colour (0x32ffffff));
    label2->setColour (TextEditor::textColourId, Colours::black);
    label2->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (label3 = new Label ("new label",
                                           TRANS("Spectrum B")));
    label3->setFont (Font ("Arial", 11.50f, Font::plain));
    label3->setJustificationType (Justification::centred);
    label3->setEditable (false, false, false);
    label3->setColour (Label::backgroundColourId, Colour (0x24000000));
    label3->setColour (Label::textColourId, Colour (0x91ffffff));
    label3->setColour (Label::outlineColourId, Colour (0x32ffffff));
    label3->setColour (TextEditor::textColourId, Colours::black);
    label3->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (label4 = new Label ("new label",
                                           TRANS("Harmonic Spectrum")));
    label4->setFont (Font ("Arial", 11.50f, Font::bold));
    label4->setJustificationType (Justification::centred);
    label4->setEditable (false, false, false);
    label4->setColour (Label::backgroundColourId, Colour (0x24000000));
    label4->setColour (Label::textColourId, Colour (0xcbffffff));
    label4->setColour (Label::outlineColourId, Colour (0x32ffffff));
    label4->setColour (TextEditor::textColourId, Colours::black);
    label4->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (filterNumbers = new HiComboBox ("new combo box"));
    filterNumbers->setEditableText (false);
    filterNumbers->setJustificationType (Justification::centredLeft);
    filterNumbers->setTextWhenNothingSelected (TRANS("Filter Numbers"));
    filterNumbers->setTextWhenNoChoicesAvailable (TRANS("(no choices)"));
    filterNumbers->addItem (TRANS("1 Filter Band"), 1);
    filterNumbers->addItem (TRANS("2 Filter Bands"), 2);
    filterNumbers->addItem (TRANS("4 Filter Bands"), 3);
    filterNumbers->addItem (TRANS("8 Filter Bands"), 4);
    filterNumbers->addItem (TRANS("16 Filter Bands"), 5);
    filterNumbers->addListener (this);

    addAndMakeVisible (qSlider = new HiSlider ("Q"));
    qSlider->setRange (4, 48, 1);
    qSlider->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    qSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    qSlider->addListener (this);

    addAndMakeVisible (crossfadeSlider = new Slider ("new slider"));
    crossfadeSlider->setRange (-1, 1, 0.01);
    crossfadeSlider->setSliderStyle (Slider::LinearBar);
    crossfadeSlider->setTextBoxStyle (Slider::NoTextBox, true, 80, 20);
    crossfadeSlider->setColour (Slider::thumbColourId, Colours::white);
    crossfadeSlider->setColour (Slider::textBoxOutlineColourId, Colour (0x32ffffff));
    crossfadeSlider->addListener (this);

    addAndMakeVisible (semiToneTranspose = new HiSlider ("Semitones"));
    semiToneTranspose->setRange (-24, 24, 1);
    semiToneTranspose->setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    semiToneTranspose->setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);
    semiToneTranspose->addListener (this);

    addAndMakeVisible (label = new Label ("new label",
                                          TRANS("harmonic filter")));
    label->setFont (Font ("Arial", 24.00f, Font::bold));
    label->setJustificationType (Justification::centredRight);
    label->setEditable (false, false, false);
    label->setColour (Label::textColourId, Colour (0x52ffffff));
    label->setColour (TextEditor::textColourId, Colours::black);
    label->setColour (TextEditor::backgroundColourId, Colour (0x00000000));


    //[UserPreSize]

    label->setFont(GLOBAL_BOLD_FONT().withHeight(28.0f));
    label2->setFont(GLOBAL_BOLD_FONT());
    label3->setFont(GLOBAL_BOLD_FONT());
    label4->setFont(GLOBAL_BOLD_FONT());
    
    
	filterNumbers->setup(getProcessor(), HarmonicFilter::NumFilterBands, "Filterband Amount");

	qSlider->setup(getProcessor(), HarmonicFilter::QFactor, "Q Factor");

	semiToneTranspose->setup(getProcessor(), HarmonicFilter::SemiToneTranspose, "Transpose");

	semiToneTranspose->setTextValueSuffix(" st");

	crossfadeSlider->setLookAndFeel(&laf);

	sliderPackMix->setEnabled(false);

	sliderPackMix->setColour(Slider::backgroundColourId, Colour(0xff3a3a3a));

	sliderPackA->setColour(Slider::backgroundColourId, Colour(0xff4a4a4a));
	sliderPackB->setColour(Slider::backgroundColourId, Colour(0xff4a4a4a));

	startTimer(30);

	sliderPackA->setSuffix(" dB");
	sliderPackB->setSuffix(" dB");

    //[/UserPreSize]

    setSize (900, 240);


    //[Constructor] You can add your own custom stuff here..
	h = getHeight();
    //[/Constructor]
}

HarmonicFilterEditor::~HarmonicFilterEditor()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    sliderPackA = nullptr;
    sliderPackB = nullptr;
    sliderPackMix = nullptr;
    label2 = nullptr;
    label3 = nullptr;
    label4 = nullptr;
    filterNumbers = nullptr;
    qSlider = nullptr;
    crossfadeSlider = nullptr;
    semiToneTranspose = nullptr;
    label = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void HarmonicFilterEditor::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.setColour (Colour (0x30000000));
    g.fillRoundedRectangle (static_cast<float> ((getWidth() / 2) - (700 / 2)), 6.0f, 700.0f, static_cast<float> (getHeight() - 12), 6.000f);

    g.setColour (Colour (0x25ffffff));
    g.drawRoundedRectangle (static_cast<float> ((getWidth() / 2) - (700 / 2)), 6.0f, 700.0f, static_cast<float> (getHeight() - 12), 6.000f, 2.000f);

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void HarmonicFilterEditor::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    sliderPackA->setBounds ((getWidth() / 2) + -142 - 192, 86, 192, 136);
    sliderPackB->setBounds ((getWidth() / 2) + 142, 86, 192, 136);
    sliderPackMix->setBounds ((getWidth() / 2) - (256 / 2), 86, 256, 104);
    label2->setBounds ((getWidth() / 2) + -142 - 192, 67, 192, 20);
    label3->setBounds ((getWidth() / 2) + 142, 67, 192, 20);
    label4->setBounds ((getWidth() / 2) - (256 / 2), 68, 256, 19);
    filterNumbers->setBounds ((getWidth() / 2) + -330, 23, 184, 28);
    qSlider->setBounds ((getWidth() / 2) + 3 - 128, 13, 128, 48);
    crossfadeSlider->setBounds ((getWidth() / 2) - (256 / 2), 199, 256, 23);
    semiToneTranspose->setBounds ((getWidth() / 2) + 153 - 128, 13, 128, 48);
    label->setBounds ((getWidth() / 2) + 339 - 264, 6, 264, 40);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void HarmonicFilterEditor::comboBoxChanged (ComboBox* comboBoxThatHasChanged)
{
    //[UsercomboBoxChanged_Pre]
    //[/UsercomboBoxChanged_Pre]

    if (comboBoxThatHasChanged == filterNumbers)
    {
        //[UserComboBoxCode_filterNumbers] -- add your combo box handling code here..
        //[/UserComboBoxCode_filterNumbers]
    }

    //[UsercomboBoxChanged_Post]
    //[/UsercomboBoxChanged_Post]
}

void HarmonicFilterEditor::sliderValueChanged (Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == qSlider)
    {
        //[UserSliderCode_qSlider] -- add your slider handling code here..
        //[/UserSliderCode_qSlider]
    }
    else if (sliderThatWasMoved == crossfadeSlider)
    {
        //[UserSliderCode_crossfadeSlider] -- add your slider handling code here..

		const double normalizedValue = (crossfadeSlider->getValue() + 1.0) / 2.0;

		getProcessor()->setAttribute(HarmonicFilter::Crossfade, (float)normalizedValue, dontSendNotification);

        //[/UserSliderCode_crossfadeSlider]
    }
    else if (sliderThatWasMoved == semiToneTranspose)
    {
        //[UserSliderCode_semiToneTranspose] -- add your slider handling code here..
        //[/UserSliderCode_semiToneTranspose]
    }

    //[UsersliderValueChanged_Post]
    //[/UsersliderValueChanged_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...

void HarmonicFilterEditor::timerCallback()
{
	const bool hasModulators = dynamic_cast<ModulatorChain*>(getProcessor()->getChildProcessor(0))->getHandler()->getNumProcessors() > 0;

	if (hasModulators)
	{
		crossfadeSlider->setEnabled(false);

		const double sliderValue = (float)getProcessor()->getAttribute(HarmonicFilter::Crossfade) * 2.0 - 1.0;

		crossfadeSlider->setValue(sliderValue, dontSendNotification);
	}
	else
	{
		crossfadeSlider->setEnabled(true);
	}

}


//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Introjucer information section --

    This is where the Introjucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="HarmonicFilterEditor" componentName=""
                 parentClasses="public ProcessorEditorBody, public Timer" constructorParams="ProcessorEditor *p"
                 variableInitialisers="ProcessorEditorBody(p)&#10;" snapPixels="8"
                 snapActive="1" snapShown="1" overlayOpacity="0.330" fixedSize="1"
                 initialWidth="900" initialHeight="240">
  <BACKGROUND backgroundColour="ffffff">
    <ROUNDRECT pos="0Cc 6 700 12M" cornerSize="6" fill="solid: 30000000" hasStroke="1"
               stroke="2, mitered, butt" strokeColour="solid: 25ffffff"/>
  </BACKGROUND>
  <GENERICCOMPONENT name="new component" id="d050db4d14cb45f1" memberName="sliderPackA"
                    virtualName="" explicitFocusOrder="0" pos="-142Cr 86 192 136"
                    class="SliderPack" params="dynamic_cast&lt;BaseHarmonicFilter*&gt;(getProcessor())-&gt;getSliderPackData(0)"/>
  <GENERICCOMPONENT name="new component" id="a24223cdd6589f94" memberName="sliderPackB"
                    virtualName="" explicitFocusOrder="0" pos="142C 86 192 136" class="SliderPack"
                    params="dynamic_cast&lt;BaseHarmonicFilter*&gt;(getProcessor())-&gt;getSliderPackData(1)"/>
  <GENERICCOMPONENT name="new component" id="9768772bb12c50e7" memberName="sliderPackMix"
                    virtualName="" explicitFocusOrder="0" pos="0Cc 86 256 104" class="SliderPack"
                    params="dynamic_cast&lt;BaseHarmonicFilter*&gt;(getProcessor())-&gt;getSliderPackData(2)"/>
  <LABEL name="new label" id="ac19153614231d20" memberName="label2" virtualName=""
         explicitFocusOrder="0" pos="-142Cr 67 192 20" bkgCol="24000000"
         textCol="91ffffff" outlineCol="32ffffff" edTextCol="ff000000"
         edBkgCol="0" labelText="Spectrum A" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Arial" fontsize="11.5" bold="0"
         italic="0" justification="36"/>
  <LABEL name="new label" id="4b5d2e574f60cad8" memberName="label3" virtualName=""
         explicitFocusOrder="0" pos="142C 67 192 20" bkgCol="24000000"
         textCol="91ffffff" outlineCol="32ffffff" edTextCol="ff000000"
         edBkgCol="0" labelText="Spectrum B" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Arial" fontsize="11.5" bold="0"
         italic="0" justification="36"/>
  <LABEL name="new label" id="c377089af1cf7875" memberName="label4" virtualName=""
         explicitFocusOrder="0" pos="0Cc 68 256 19" bkgCol="24000000"
         textCol="cbffffff" outlineCol="32ffffff" edTextCol="ff000000"
         edBkgCol="0" labelText="Harmonic Spectrum" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Arial"
         fontsize="11.5" bold="1" italic="0" justification="36"/>
  <COMBOBOX name="new combo box" id="f9053c2b9246bbfc" memberName="filterNumbers"
            virtualName="HiComboBox" explicitFocusOrder="0" pos="-330C 23 184 28"
            posRelativeX="3b242d8d6cab6cc3" editable="0" layout="33" items="1 Filter Band&#10;2 Filter Bands&#10;4 Filter Bands&#10;8 Filter Bands&#10;16 Filter Bands"
            textWhenNonSelected="Filter Numbers" textWhenNoItems="(no choices)"/>
  <SLIDER name="Q" id="89cc5b4c20e221e" memberName="qSlider" virtualName="HiSlider"
          explicitFocusOrder="0" pos="3Cr 13 128 48" posRelativeX="f930000f86c6c8b6"
          min="4" max="48" int="1" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="new slider" id="1684e9ecd984b307" memberName="crossfadeSlider"
          virtualName="" explicitFocusOrder="0" pos="0Cc 199 256 23" thumbcol="ffffffff"
          textboxoutline="32ffffff" min="-1" max="1" int="0.01" style="LinearBar"
          textBoxPos="NoTextBox" textBoxEditable="0" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="Semitones" id="f2c32a369cff4bfd" memberName="semiToneTranspose"
          virtualName="HiSlider" explicitFocusOrder="0" pos="153Cr 13 128 48"
          posRelativeX="f930000f86c6c8b6" min="-24" max="24" int="1" style="RotaryHorizontalVerticalDrag"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1"/>
  <LABEL name="new label" id="bd1d8d6ad6d04bdc" memberName="label" virtualName=""
         explicitFocusOrder="0" pos="339Cr 6 264 40" textCol="52ffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="harmonic filter"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Arial" fontsize="24" bold="1" italic="0" justification="34"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...


} // namespace hise

//[/EndFile]
