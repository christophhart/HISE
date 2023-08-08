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

struct ValueSettingComponent::ValueSlider : public Component
{
	ValueSlider(ValueSettingComponent* c):
		valueParent(c)
	{
		addAndMakeVisible(slider);
		slider.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
		slider.setSliderStyle(Slider::LinearHorizontal);
		slider.setLookAndFeel(&slaf);
		slider.setColour(Slider::backgroundColourId, Colours::white.withAlpha(0.2f));
		slider.setColour(Slider::trackColourId, Colours::orange.withSaturation(0.4f).withAlpha(0.5f));
		slider.setColour(Slider::thumbColourId, Colour(0xFFDDDDDD));

		slider.onValueChange = [this]()
		{
			valueParent->setPropertyForAllSelectedSounds(valueParent->soundProperty, (int)slider.getValue());
		};

		setSelection(c->currentSelection);

		if (auto parent = c->findParentComponentOfClass<SamplerSubEditor>())
		{
			auto parentC = dynamic_cast<Component*>(parent);
			parentC->addAndMakeVisible(this);
			
			auto b = parentC->getLocalArea(c, c->getLocalBounds());
			b = b.withSizeKeepingCentre(600, 40).translated(0, -48);
			setWantsKeyboardFocus(false);
			setBounds(b);
		}
	}

	void resized() override
	{
		slider.setBounds(getLocalBounds().removeFromBottom(28));
	}

	void setSelection(const SampleSelection& selection)
	{
		fullRange = Range<int>(INT_MIN, INT_MAX);

		for (auto s : valueParent->currentSelection)
			fullRange = fullRange.getIntersectionWith(s->getPropertyRange(valueParent->soundProperty));

		slider.setRange(fullRange.getStart(), fullRange.getEnd(), 1);

		if(auto first = selection.getFirst())
			slider.setValue(first->getSampleProperty(valueParent->soundProperty), dontSendNotification);
	}

	void paint(Graphics& g) override
	{
		g.setColour(Colour(0xDD222222));
		g.fillAll();
		g.setColour(Colours::white.withAlpha(0.2f));
		g.drawRect(getLocalBounds().toFloat().reduced(1.0f), 1.0f);

		auto b = getLocalBounds().toFloat().reduced(8.0f, 3.0f);
		g.setFont(GLOBAL_FONT());

		

		g.drawText(String(roundToInt(slider.getValue())), b, Justification::centredTop);
		g.drawText(String(fullRange.getStart()), b, Justification::topLeft);
		g.drawText(String(fullRange.getEnd()), b, Justification::topRight);
	}

	void mouseDown(const MouseEvent& e) override
	{
		valueParent->newSlider = nullptr;
	}

	Range<int> fullRange;
	Component::SafePointer<ValueSettingComponent> valueParent;

	LookAndFeel_V4 slaf;
	Slider slider;
};

//==============================================================================
ValueSettingComponent::ValueSettingComponent (ModulatorSampler* sampler_):
	sampler(sampler_)
{
	sampler->getSampleMap()->addListener(this);

    addAndMakeVisible (valueLabel = new Label ("new label",
                                               String()));
    valueLabel->setFont (Font ("Khmer UI", 14.00f, Font::plain));
    valueLabel->setJustificationType (Justification::centred);
    valueLabel->setEditable (true, true, false);
    valueLabel->setColour (Label::backgroundColourId, Colour (0x22ffffff));
    valueLabel->setColour (Label::outlineColourId, Colour (0x55ffffff));
    valueLabel->setColour (Label::textColourId, Colours::white.withAlpha(0.8f));
	valueLabel->setColour(Label::ColourIds::textWhenEditingColourId, Colours::white.withAlpha(0.8f));
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
    descriptionLabel->setColour (Label::textColourId, Colours::white.withAlpha(0.7f));
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

	minusButton->setWantsKeyboardFocus(false);
	plusButton->setWantsKeyboardFocus(false);
	
	setWantsKeyboardFocus(true);
	setFocusContainerType(FocusContainerType::keyboardFocusContainer);
    //[UserPreSize]

	plusButton->setLookAndFeel(&cb);
	minusButton->setLookAndFeel(&cb);

	valueLabel->setFont (GLOBAL_BOLD_FONT());
	descriptionLabel->setFont (GLOBAL_FONT());

	valueLabel->addMouseListener(this, true);
	descriptionLabel->addMouseListener(this, true);

    //[/UserPreSize]

    setSize (100, 32);


    //[Constructor] You can add your own custom stuff here..
    //[/Constructor]
}

ValueSettingComponent::~ValueSettingComponent()
{
	sampler->getSampleMap()->removeListener(this);
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    valueLabel = nullptr;
    descriptionLabel = nullptr;
    minusButton = nullptr;
    plusButton = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

void ValueSettingComponent::setCurrentSelection(const SampleSelection &newSelection)
{
	if (newSelection.isEmpty())
		newSlider = nullptr;
	else if (newSlider != nullptr)
		newSlider->setSelection(newSelection);


	currentSelection.clear();
	currentSelection.addArray(newSelection);

	updateValue();
}

std::unique_ptr<juce::ComponentTraverser> ValueSettingComponent::createKeyboardFocusTraverser()
{
	if (auto sub = findParentComponentOfClass<SamplerSubEditor>())
		return std::make_unique<SampleEditHandler::SubEditorTraverser>(dynamic_cast<Component*>(sub));

	return Component::createFocusTraverser();
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
	

	auto b = getLocalBounds().reduced(2, 0);

	descriptionLabel->setBounds(b.removeFromTop(JUCE_LIVE_CONSTANT_OFF(15)));

	minusButton->setBounds(b.removeFromLeft(b.getHeight()));
	plusButton->setBounds(b.removeFromRight(b.getHeight()));
	valueLabel->setBounds(b);
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

void ValueSettingComponent::mouseDrag(const MouseEvent& e)
{
	if (e.mods.isRightButtonDown())
		return;

	auto distX = e.getDistanceFromDragStartX();
	auto distY = e.getDistanceFromDragStartY();

	auto distToUse = 0;

	if (hmath::abs(distX) > hmath::abs(distY))
		distToUse = distX / 2;
	else
		distToUse = distY * -1;

	auto normed = (double)distToUse / (400.0);

	for (int i = 0; i < currentSelection.size(); i++)
	{
		auto r = downRanges[i];

		NormalisableRange<double> nr(r.getStart(), r.getEnd());

		auto startValue = downValues[i];
		auto newValue = jlimit(0.0, 1.0, nr.convertTo0to1(startValue) + normed);
		newValue = nr.convertFrom0to1(newValue);
		currentSelection[i]->setSampleProperty(soundProperty, newValue);
	}

	updateValue();
}

void ValueSettingComponent::mouseDoubleClick(const MouseEvent& event)
{
	if (event.mods.isRightButtonDown())
		return;

	for(auto s: currentSelection)
	{
		s->setSampleProperty(soundProperty, s->getDefaultValue(soundProperty));
	}

	updateValue();
}

void ValueSettingComponent::Dismisser::mouseDown(const MouseEvent& e)
{
	auto isParentOrSame = [&](Component* parent)
	{
		return e.eventComponent == parent || parent->isParentOf(e.eventComponent);
	};

	if (isParentOrSame(&parent) || isParentOrSame(parent.newSlider))
		return;

	parent.resetValueSlider();
}


//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...

void ValueSettingComponent::resetValueSlider()
{
	newSlider = nullptr;
	descriptionLabel->setFont(GLOBAL_FONT());
	descriptionLabel->setColour(Label::textColourId, Colours::white.withAlpha(0.7f));
	
}

void ValueSettingComponent::mouseDown(const MouseEvent &e)
{
#if USE_BACKEND
    ProcessorEditor *editor = findParentComponentOfClass<ProcessorEditor>();
    
    if(editor != nullptr)
    {
        PresetHandler::setChanged(editor->getProcessor());
    }
#endif
    
    if(!currentSelection.isEmpty())
    {
		downValues.clearQuick();
		downRanges.clearQuick();

		for(auto s: currentSelection)
		{
			downValues.add((int)s->getSampleProperty(soundProperty));
			downRanges.add(s->getPropertyRange(soundProperty));
		}

		if(e.mods.isRightButtonDown())
		{
			

			if (newSlider != nullptr)
			{
				resetValueSlider();
				return;
			}

			auto se = getSubEditorComponent(this);

			if(dismisser == nullptr)
			{
				dismisser = new Dismisser(*this, se);
			}
			
			callRecursive<ValueSettingComponent>(se, [](ValueSettingComponent* v)
			{
				v->resetValueSlider();
				return false;
			});

			newSlider = new ValueSlider(this);
			descriptionLabel->setFont(GLOBAL_BOLD_FONT());
			descriptionLabel->setColour(Label::textColourId, Colours::white);
		}
    }
}

void ValueSettingComponent::samplePropertyWasChanged(ModulatorSamplerSound* s, const Identifier& id, const var& newValue)
{
	if (currentSelection.contains(s) && soundProperty == id)
	{
		updateValue();
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
