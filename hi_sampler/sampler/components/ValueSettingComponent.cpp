/*
  ==============================================================================

  This is an automatically generated GUI class created by the Introjucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Introjucer version: 3.1.0

  ------------------------------------------------------------------------------

  The Introjucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright 2004-13 by Raw Material Software Ltd.

  ==============================================================================
*/

//[Headers] You can add your own extra header files here...
namespace hise { using namespace juce;
//[/Headers]

#include "ValueSettingComponent.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
ValueSettingComponent::ValueSettingComponent ()
{
    addAndMakeVisible (valueLabel = new Label ("new label",
                                               String()));
    valueLabel->setFont (Font ("Khmer UI", 14.00f, Font::plain));
    valueLabel->setJustificationType (Justification::centred);
    valueLabel->setEditable (true, true, false);
    valueLabel->setColour (Label::backgroundColourId, Colour (0x38ffffff));
    valueLabel->setColour (Label::outlineColourId, Colour (0x38ffffff));
    valueLabel->setColour (TextEditor::textColourId, Colours::black);
    valueLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));
    valueLabel->setColour (TextEditor::highlightColourId, Colour (0x407a0000));
    valueLabel->setColour (TextEditor::highlightColourId, Colour (SIGNAL_COLOUR).withAlpha(0.5f));
    valueLabel->setColour (TextEditor::ColourIds::focusedOutlineColourId, Colour(SIGNAL_COLOUR));
    valueLabel->addListener (this);

    addAndMakeVisible (descriptionLabel = new Label ("new label",
                                                     TRANS("Unused")));
    descriptionLabel->setFont (Font ("Khmer UI", 13.00f, Font::plain));
    descriptionLabel->setJustificationType (Justification::centred);
    descriptionLabel->setEditable (false, false, false);
    descriptionLabel->setColour (Label::textColourId, Colours::white);
    descriptionLabel->setColour (TextEditor::textColourId, Colours::black);
    descriptionLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (minusButton = new TextButton ("new button"));
    minusButton->setButtonText (TRANS("-"));
    minusButton->setConnectedEdges (Button::ConnectedOnRight);
    minusButton->addListener (this);
    minusButton->setColour (TextButton::buttonColourId, Colour (0x3fffffff));
    minusButton->setColour (TextButton::buttonOnColourId, Colour (0xff700000));

    addAndMakeVisible (plusButton = new TextButton ("new button"));
    plusButton->setButtonText (TRANS("+"));
    plusButton->setConnectedEdges (Button::ConnectedOnLeft);
    plusButton->addListener (this);
    plusButton->setColour (TextButton::buttonColourId, Colour (0x3fffffff));
    plusButton->setColour (TextButton::buttonOnColourId, Colour (0xff700000));

	
    //[UserPreSize]

	valueLabel->setFont (GLOBAL_FONT());
	descriptionLabel->setFont (GLOBAL_BOLD_FONT());

	valueLabel->addMouseListener(this, true);
	descriptionLabel->addMouseListener(this, true);

	currentSlider = nullptr;

    //[/UserPreSize]

    setSize (100, 32);


    //[Constructor] You can add your own custom stuff here..
    //[/Constructor]
}

ValueSettingComponent::~ValueSettingComponent()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    valueLabel = nullptr;
    descriptionLabel = nullptr;
    minusButton = nullptr;
    plusButton = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

void ValueSettingComponent::setPropertyForAllSelectedSounds(const Identifier& p, int newValue)
{
	if (currentSelection.size() != 0)
	{
		currentSelection[0]->startPropertyChange(p, newValue);
	};

	for (int i = 0; i < currentSelection.size(); i++)
	{
		const int low = currentSelection[i]->getPropertyRange(soundProperty).getStart();
		const int high = currentSelection[i]->getPropertyRange(soundProperty).getEnd();

		const int clippedValue = jlimit(low, high, newValue);

		currentSelection[i]->setSampleProperty(p, clippedValue);


	}

	SampleEditor* editor = findParentComponentOfClass<SampleEditor>();

	if (editor != nullptr)
	{
		editor->updateWaveform();
	}

	updateValue();
}

//==============================================================================
void ValueSettingComponent::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
	g.fillAll(Colours::transparentBlack);
    //[/UserPrePaint]

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void ValueSettingComponent::resized()
{
    valueLabel->setBounds ((getWidth() / 2) - ((getWidth() - 36) / 2), 15, getWidth() - 36, 16);
    descriptionLabel->setBounds ((getWidth() / 2) - ((proportionOfWidth (1.0000f)) / 2), -4, proportionOfWidth (1.0000f), 24);
    minusButton->setBounds (1, 14, 16, 18);
    plusButton->setBounds (getWidth() - 1 - 16, 14, 16, 18);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void ValueSettingComponent::labelTextChanged (Label* labelThatHasChanged)
{
    //[UserlabelTextChanged_Pre]
    //[/UserlabelTextChanged_Pre]

    if (labelThatHasChanged == valueLabel)
    {
        //[UserLabelCode_valueLabel] -- add your label text handling code here..

		if(valueLabel->getText().containsAnyOf("CDEFGAB#"))
		{
			for(int i = 0; i < 127; i++)
			{
				if(MidiMessage::getMidiNoteName(i, true, true, 3) == valueLabel->getText())
				{
					setPropertyForAllSelectedSounds(soundProperty, i);
				}
			}
		}
		else
		{
			const int value = valueLabel->getText().getIntValue();

			setPropertyForAllSelectedSounds(soundProperty, value);
		}
        //[/UserLabelCode_valueLabel]
    }

    //[UserlabelTextChanged_Post]
    //[/UserlabelTextChanged_Post]
}

void ValueSettingComponent::buttonClicked (Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]

	int delta = 0;
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == minusButton)
    {
        //[UserButtonCode_minusButton] -- add your button handler code here..

		delta = -1;

        //[/UserButtonCode_minusButton]
    }
    else if (buttonThatWasClicked == plusButton)
    {
        //[UserButtonCode_plusButton] -- add your button handler code here..
		delta = 1;
        //[/UserButtonCode_plusButton]
    }

    //[UserbuttonClicked_Post]

	if(currentSelection.size() != 0)
	{
		currentSelection[0]->startPropertyChange(soundProperty, delta);
	};

	changePropertyForAllSelectedSounds(soundProperty, delta);

    //[/UserbuttonClicked_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...

void ValueSettingComponent::mouseDown(const MouseEvent &e)
{
#if USE_BACKEND
    ProcessorEditor *editor = findParentComponentOfClass<ProcessorEditor>();
    
    if(editor != nullptr)
    {
        PresetHandler::setChanged(editor->getProcessor());
    }
#endif
    
    if(currentSelection.size() != 0 && e.mods.isRightButtonDown())
    {
        Slider *s = new Slider();
        
        s->setSliderStyle(Slider::LinearBar);
        s->setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
        s->setRange(currentSelection[0]->getPropertyRange(soundProperty).getStart(),
                    currentSelection[0]->getPropertyRange(soundProperty).getEnd(),
                    1);
        s->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
        s->setColour (Slider::thumbColourId, Colours::white);
        s->setColour (Slider::rotarySliderOutlineColourId, Colour (0x00000000));
        s->setColour (Slider::textBoxOutlineColourId, Colour (0x00808080));
        s->addListener (this);
        
        s->setLookAndFeel(&laf);
        
        currentSlider = s;
        
        if(currentSelection.size() != 0)
        {
            s->setValue(currentSelection[0]->getSampleProperty(soundProperty), dontSendNotification);
        }
        else
        {
            s->setEnabled(false);
        }
        
        s->setSize (600, 32);
        
		if (auto root = TopLevelWindowWithOptionalOpenGL::findRoot(this))
		{
			auto rootPos = root->getLocalArea(getParentComponent(), getBoundsInParent());

			CallOutBox::launchAsynchronously(s, rootPos, root);
		}
		
        
    }
    
}

//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Introjucer information section --

    This is where the Introjucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="ValueSettingComponent" componentName=""
                 parentClasses="public Component, public SafeChangeListener, public SliderListener"
                 constructorParams="" variableInitialisers="" snapPixels="8" snapActive="1"
                 snapShown="1" overlayOpacity="0.330" fixedSize="1" initialWidth="100"
                 initialHeight="32">
  <BACKGROUND backgroundColour="2d2d2d"/>
  <LABEL name="new label" id="ecf08aed0630701" memberName="valueLabel"
         virtualName="" explicitFocusOrder="0" pos="0Cc 15 36M 16" bkgCol="38ffffff"
         outlineCol="38ffffff" edTextCol="ff000000" edBkgCol="0" hiliteCol="407a0000"
         labelText="" editableSingleClick="1" editableDoubleClick="1"
         focusDiscardsChanges="0" fontname="Khmer UI" fontsize="14" bold="0"
         italic="0" justification="36"/>
  <LABEL name="new label" id="7be2ed43072326c4" memberName="descriptionLabel"
         virtualName="" explicitFocusOrder="0" pos="0Cc -4 100% 24" textCol="ffffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="Unused" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Khmer UI"
         fontsize="13" bold="0" italic="0" justification="36"/>
  <TEXTBUTTON name="new button" id="9214493a4aad8bc7" memberName="minusButton"
              virtualName="" explicitFocusOrder="0" pos="1 15 16 16" bgColOff="3fffffff"
              bgColOn="ff700000" buttonText="-" connectedEdges="3" needsCallback="1"
              radioGroupId="0"/>
  <TEXTBUTTON name="new button" id="74eaa4a918c87828" memberName="plusButton"
              virtualName="" explicitFocusOrder="0" pos="1Rr 15 16 16" bgColOff="3fffffff"
              bgColOn="ff700000" buttonText="+" connectedEdges="3" needsCallback="1"
              radioGroupId="0"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
} // namespace hise
//[/EndFile]
