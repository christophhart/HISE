/*
  ==============================================================================

  This is an automatically generated GUI class created by the Projucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Projucer version: 5.2.0

  ------------------------------------------------------------------------------

  The Projucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright (c) 2015 - ROLI Ltd.

  ==============================================================================
*/

//[Headers] You can add your own extra header files here...
namespace hise { using namespace juce;
//[/Headers]

#include "WavetableBody.h"




//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
WavetableBody::WavetableBody (ProcessorEditor *p)
    : ProcessorEditorBody(p)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (fadeTimeLabel = new Label ("new label",
                                                  TRANS("Fade Time")));
    fadeTimeLabel->setFont (Font ("Khmer UI", 13.00f, Font::plain).withTypefaceStyle ("Regular"));
    fadeTimeLabel->setJustificationType (Justification::centredLeft);
    fadeTimeLabel->setEditable (false, false, false);
    fadeTimeLabel->setColour (Label::textColourId, Colours::white);
    fadeTimeLabel->setColour (TextEditor::textColourId, Colours::black);
    fadeTimeLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (voiceAmountLabel = new Label ("new label",
                                                     TRANS("Voice Amount")));
    voiceAmountLabel->setFont (Font ("Khmer UI", 13.00f, Font::plain).withTypefaceStyle ("Regular"));
    voiceAmountLabel->setJustificationType (Justification::centredLeft);
    voiceAmountLabel->setEditable (false, false, false);
    voiceAmountLabel->setColour (Label::textColourId, Colours::white);
    voiceAmountLabel->setColour (TextEditor::textColourId, Colours::black);
    voiceAmountLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (voiceAmountEditor = new Label ("new label",
                                                      TRANS("64")));
    voiceAmountEditor->setFont (Font ("Khmer UI", 14.00f, Font::plain).withTypefaceStyle ("Regular"));
    voiceAmountEditor->setJustificationType (Justification::centredLeft);
    voiceAmountEditor->setEditable (true, true, false);
    voiceAmountEditor->setColour (Label::backgroundColourId, Colour (0x38ffffff));
    voiceAmountEditor->setColour (Label::outlineColourId, Colour (0x38ffffff));
    voiceAmountEditor->setColour (TextEditor::textColourId, Colours::black);
    voiceAmountEditor->setColour (TextEditor::backgroundColourId, Colour (0x00000000));
    voiceAmountEditor->setColour (TextEditor::highlightColourId, Colour (0x407a0000));
    voiceAmountEditor->addListener (this);

    addAndMakeVisible (fadeTimeEditor = new Label ("new label",
                                                   TRANS("15 ms")));
    fadeTimeEditor->setFont (Font ("Khmer UI", 14.00f, Font::plain).withTypefaceStyle ("Regular"));
    fadeTimeEditor->setJustificationType (Justification::centredLeft);
    fadeTimeEditor->setEditable (true, true, false);
    fadeTimeEditor->setColour (Label::backgroundColourId, Colour (0x38ffffff));
    fadeTimeEditor->setColour (Label::outlineColourId, Colour (0x38ffffff));
    fadeTimeEditor->setColour (TextEditor::textColourId, Colours::black);
    fadeTimeEditor->setColour (TextEditor::backgroundColourId, Colour (0x00000000));
    fadeTimeEditor->setColour (TextEditor::highlightColourId, Colour (0x407a0000));
    fadeTimeEditor->addListener (this);

    addAndMakeVisible (hiqButton = new HiToggleButton ("HQ Mode"));
    hiqButton->setTooltip (TRANS("Enables HQ rendering mode (more CPU intensive)"));
    hiqButton->setButtonText (TRANS("HQ"));
    hiqButton->addListener (this);
    hiqButton->setColour (ToggleButton::textColourId, Colours::white);

	addAndMakeVisible(mipmapButton = new HiToggleButton("Refresh Mipmap"));
	mipmapButton->setTooltip(TRANS("Updates the mipmap sound when the pitch modulation goes outside the frequency range to avoid aliasing"));
	mipmapButton->setButtonText(TRANS("Refresh Mipmap"));
	mipmapButton->addListener(this);
	mipmapButton->setColour(ToggleButton::textColourId, Colours::white);

    addAndMakeVisible (voiceAmountLabel2 = new Label ("new label",
                                                      TRANS("Gain Values")));
    voiceAmountLabel2->setFont (Font ("Khmer UI", 13.00f, Font::plain).withTypefaceStyle ("Regular"));
    voiceAmountLabel2->setJustificationType (Justification::centredLeft);
    voiceAmountLabel2->setEditable (false, false, false);
    voiceAmountLabel2->setColour (Label::textColourId, Colours::white);
    voiceAmountLabel2->setColour (TextEditor::textColourId, Colours::black);
    voiceAmountLabel2->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (voiceAmountLabel3 = new Label ("new label",
                                                      TRANS("Wavetable Preview\n")));
    voiceAmountLabel3->setFont (Font ("Khmer UI", 13.00f, Font::plain).withTypefaceStyle ("Regular"));
    voiceAmountLabel3->setJustificationType (Justification::centredLeft);
    voiceAmountLabel3->setEditable (false, false, false);
    voiceAmountLabel3->setColour (Label::textColourId, Colours::white);
    voiceAmountLabel3->setColour (TextEditor::textColourId, Colours::black);
    voiceAmountLabel3->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (wavetableSelector = new HiComboBox ("new combo box"));
    wavetableSelector->setEditableText (false);
    wavetableSelector->setJustificationType (Justification::centredLeft);
    wavetableSelector->setTextWhenNothingSelected (TRANS("Select Wavetable"));
    wavetableSelector->setTextWhenNoChoicesAvailable (TRANS("(no choices)"));
    wavetableSelector->addListener (this);

	addAndMakeVisible(tableSlider = new HiSlider("Table Index"));
	tableSlider->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
	tableSlider->setup(getProcessor(), WavetableSynth::TableIndexValue, "Table Index");
	tableSlider->setMode(HiSlider::Mode::NormalizedPercentage);

    //[UserPreSize]

	fadeTimeLabel->setFont (GLOBAL_FONT());
    voiceAmountLabel->setFont (GLOBAL_FONT());
    voiceAmountEditor->setFont (GLOBAL_FONT());
    fadeTimeEditor->setFont (GLOBAL_FONT());

	hiqButton->setup(getProcessor(), WavetableSynth::HqMode, "HQ");
	mipmapButton->setup(getProcessor(), WavetableSynth::RefreshMipmap, "Refresh Mipmap");

	wavetableSelector->setup(getProcessor(), WavetableSynth::LoadedBankIndex, "Loaded Wavetable");

	WeakReference<Processor> owner = getProcessor();

	wavetableSelector->addItemList(dynamic_cast<WavetableSynth*>(getProcessor())->getWavetableList(), 1);

	auto wv = new WaterfallComponent(getProcessor()->getMainController(), nullptr);

	wv->displayDataFunction = [owner]()
	{
		WaterfallComponent::DisplayData d;

		if (owner.get() != nullptr)
		{
			auto ws = dynamic_cast<WavetableSynth*>(owner.get());

			d.sound = dynamic_cast<WavetableSound*>(ws->getSound(0));
			d.modValue = ws->getDisplayTableValue();
		}
		
		return d;
	};

	addAndMakeVisible(waterfall = wv);

	

    //[/UserPreSize]

	

    setSize (800, 250);


    //[Constructor] You can add your own custom stuff here..
	h = getHeight();

    //[/Constructor]
}

WavetableBody::~WavetableBody()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    fadeTimeLabel = nullptr;
    voiceAmountLabel = nullptr;
    voiceAmountEditor = nullptr;
    fadeTimeEditor = nullptr;
    hiqButton = nullptr;
    voiceAmountLabel2 = nullptr;
    voiceAmountLabel3 = nullptr;
	tableSlider = nullptr;
    wavetableSelector = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void WavetableBody::paint (Graphics& g)
{
    
}

void WavetableBody::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

	auto b = getLocalBounds().withSizeKeepingCentre(600, getHeight() - 30);

	auto leftTab = b.removeFromLeft(128);
	constexpr int Margin = 10;

	wavetableSelector->setBounds(leftTab.removeFromTop(28)); leftTab.removeFromTop(Margin);
	tableSlider->setBounds(leftTab.removeFromTop(48)); leftTab.removeFromTop(Margin);
	hiqButton->setBounds(leftTab.removeFromTop(28)); leftTab.removeFromTop(Margin);

	auto t = leftTab.removeFromTop(24);

	fadeTimeLabel->setBounds(t.removeFromLeft(64));
	voiceAmountLabel->setBounds(t.removeFromLeft(64));

	

	auto l = leftTab.removeFromTop(16);

	fadeTimeEditor->setBounds(l.removeFromLeft(64));
	voiceAmountEditor->setBounds(l.removeFromLeft(64));

	leftTab.removeFromTop(Margin);

	mipmapButton->setBounds(leftTab.removeFromTop(28));

	b.removeFromLeft(Margin);

	waterfall->setBounds(b);

#if 0
    waterfall->setBounds (31, 44, getWidth() - 466, 165);
    fadeTimeLabel->setBounds (getWidth() - 271 - 79, 237, 79, 24);
    voiceAmountLabel->setBounds (getWidth() - 347 - 79, 238, 79, 24);
    voiceAmountEditor->setBounds (getWidth() - 353 - 68, 256, 68, 16);
    fadeTimeEditor->setBounds (getWidth() - 294 - 51, 256, 51, 16);
    hiqButton->setBounds (getWidth() - 149 - 128, 244, 128, 32);
    voiceAmountLabel2->setBounds ((getWidth() - 38 - 384) + 0, 18, 79, 24);
	tableSlider->setBounds(getWidth() - 294, 44, 128, 32);
    voiceAmountLabel3->setBounds (31 + 0, 17, 136, 24);
    wavetableSelector->setBounds (31 + 0, 211, getWidth() - 466, 24);
#endif
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void WavetableBody::labelTextChanged (Label* labelThatHasChanged)
{
    //[UserlabelTextChanged_Pre]
    //[/UserlabelTextChanged_Pre]

    if (labelThatHasChanged == voiceAmountEditor)
    {
        //[UserLabelCode_voiceAmountEditor] -- add your label text handling code here..
        //[/UserLabelCode_voiceAmountEditor]
    }
    else if (labelThatHasChanged == fadeTimeEditor)
    {
        //[UserLabelCode_fadeTimeEditor] -- add your label text handling code here..
        //[/UserLabelCode_fadeTimeEditor]
    }

    //[UserlabelTextChanged_Post]
    //[/UserlabelTextChanged_Post]
}

void WavetableBody::buttonClicked (Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == hiqButton)
    {
        //[UserButtonCode_hiqButton] -- add your button handler code here..
        //[/UserButtonCode_hiqButton]
    }

    //[UserbuttonClicked_Post]
    //[/UserbuttonClicked_Post]
}

void WavetableBody::comboBoxChanged (ComboBox* comboBoxThatHasChanged)
{
    //[UsercomboBoxChanged_Pre]
    //[/UsercomboBoxChanged_Pre]

    if (comboBoxThatHasChanged == wavetableSelector)
    {
        //[UserComboBoxCode_wavetableSelector] -- add your combo box handling code here..
        //[/UserComboBoxCode_wavetableSelector]
    }

    //[UsercomboBoxChanged_Post]
    //[/UsercomboBoxChanged_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...

//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Projucer information section --

    This is where the Projucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="WavetableBody" componentName=""
                 parentClasses="public ProcessorEditorBody, public Timer" constructorParams="ProcessorEditor *p"
                 variableInitialisers="ProcessorEditorBody(p)" snapPixels="8"
                 snapActive="0" snapShown="1" overlayOpacity="0.330" fixedSize="1"
                 initialWidth="800" initialHeight="300">
  <BACKGROUND backgroundColour="ffffff">
    <RECT pos="16 0 32M 16M" fill="solid: 43a52a" hasStroke="1" stroke="1, mitered, butt"
          strokeColour="solid: 23ffffff"/>
    <TEXT pos="24Rr 4 200 30" fill="solid: 52ffffff" hasStroke="0" text="WAVETABLE"
          fontname="Arial" fontsize="24" kerning="0" bold="1" italic="0"
          justification="34" typefaceStyle="Bold"/>
  </BACKGROUND>
  <GENERICCOMPONENT name="new component" id="5bdd135efdbc6b85" memberName="waveTableDisplay"
                    virtualName="" explicitFocusOrder="0" pos="31 44 466M 165" class="WavetableDisplayComponent"
                    params="dynamic_cast&lt;WavetableSynth*&gt;(getProcessor())"/>
  <LABEL name="new label" id="f18e00eab8404cdf" memberName="fadeTimeLabel"
         virtualName="" explicitFocusOrder="0" pos="271Rr 237 79 24" textCol="ffffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="Fade Time" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Khmer UI"
         fontsize="13" kerning="0" bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="5836a90d75d1dd4a" memberName="voiceAmountLabel"
         virtualName="" explicitFocusOrder="0" pos="347Rr 238 79 24" textCol="ffffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="Voice Amount" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Khmer UI"
         fontsize="13" kerning="0" bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="fa0dc77af8626dc7" memberName="voiceAmountEditor"
         virtualName="" explicitFocusOrder="0" pos="353Rr 256 68 16" bkgCol="38ffffff"
         outlineCol="38ffffff" edTextCol="ff000000" edBkgCol="0" hiliteCol="407a0000"
         labelText="64" editableSingleClick="1" editableDoubleClick="1"
         focusDiscardsChanges="0" fontname="Khmer UI" fontsize="14" kerning="0"
         bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="9747f9d28c74d65d" memberName="fadeTimeEditor"
         virtualName="" explicitFocusOrder="0" pos="294Rr 256 51 16" bkgCol="38ffffff"
         outlineCol="38ffffff" edTextCol="ff000000" edBkgCol="0" hiliteCol="407a0000"
         labelText="15 ms" editableSingleClick="1" editableDoubleClick="1"
         focusDiscardsChanges="0" fontname="Khmer UI" fontsize="14" kerning="0"
         bold="0" italic="0" justification="33"/>
  <GENERICCOMPONENT name="new component" id="86c524f43e825eb1" memberName="component"
                    virtualName="" explicitFocusOrder="0" pos="38Rr 43 384 165" class="SliderPack"
                    params="dynamic_cast&lt;WavetableSynth*&gt;(getProcessor())-&gt;getSliderPackData(0)"/>
  <TOGGLEBUTTON name="HQ Mode" id="dfdc6e861a38fb62" memberName="hiqButton" virtualName="HiToggleButton"
                explicitFocusOrder="0" pos="149Rr 244 128 32" tooltip="Enables HQ rendering mode (more CPU intensive)"
                txtcol="ffffffff" buttonText="HQ" connectedEdges="0" needsCallback="1"
                radioGroupId="0" state="0"/>
  <LABEL name="new label" id="bd05b8e23c53d26" memberName="voiceAmountLabel2"
         virtualName="" explicitFocusOrder="0" pos="0 18 79 24" posRelativeX="86c524f43e825eb1"
         textCol="ffffffff" edTextCol="ff000000" edBkgCol="0" labelText="Gain Values"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Khmer UI" fontsize="13" kerning="0" bold="0" italic="0"
         justification="33"/>
  <GENERICCOMPONENT name="new component" id="1099df2918c1e835" memberName="modMeter"
                    virtualName="" explicitFocusOrder="0" pos="0 211 384 24" posRelativeX="86c524f43e825eb1"
                    class="VuMeter" params=""/>
  <LABEL name="new label" id="ddc6a12ed233c301" memberName="voiceAmountLabel3"
         virtualName="" explicitFocusOrder="0" pos="0 17 136 24" posRelativeX="5bdd135efdbc6b85"
         textCol="ffffffff" edTextCol="ff000000" edBkgCol="0" labelText="Wavetable Preview&#10;"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Khmer UI" fontsize="13" kerning="0" bold="0" italic="0"
         justification="33"/>
  <COMBOBOX name="new combo box" id="f9053c2b9246bbfc" memberName="wavetableSelector"
            virtualName="HiComboBox" explicitFocusOrder="0" pos="0 211 466M 24"
            posRelativeX="5bdd135efdbc6b85" editable="0" layout="33" items=""
            textWhenNonSelected="Select Wavetable" textWhenNoItems="(no choices)"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
} // namespace hise
//[/EndFile]
