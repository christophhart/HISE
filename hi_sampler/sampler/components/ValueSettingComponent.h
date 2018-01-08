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

#ifndef __JUCE_HEADER_CB07FBFFB8C6ABE4__
#define __JUCE_HEADER_CB07FBFFB8C6ABE4__

//[Headers]     -- You can add your own extra header files here --
 namespace hise { using namespace juce;


//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Introjucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class ValueSettingComponent  : public Component,
                               public SafeChangeListener,
							   public SafeChangeBroadcaster,
                               public SliderListener,
                               public LabelListener,
                               public ButtonListener,
							   public Timer
{
public:
    //==============================================================================
    ValueSettingComponent ();
    ~ValueSettingComponent();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

    void mouseDown(const MouseEvent &e);

	void sliderValueChanged(Slider *s)
	{
        setSamplePropertyValue((int)s->getValue(), false);

		sendChangeMessage();

		updateValue();
    };
    
    bool isFileAccessingProperty() const
    {
        return soundProperty == ModulatorSamplerSound::SampleStart ||
               soundProperty == ModulatorSamplerSound::LoopXFade ||
               soundProperty == ModulatorSamplerSound::LoopStart;

    }
    
    void setSamplePropertyValue(int value, bool forceChange)
    {
        const int delta = (int)value - sliderStartValue;
        
        for(int i = 0; i < currentSelection.size(); i++)
        {
            const int newValue = dragStartValues[i] + delta;
            
            if(forceChange || !isFileAccessingProperty())
            {
                const int low = currentSelection[i]->getPropertyRange(soundProperty).getStart();
                const int high = currentSelection[i]->getPropertyRange(soundProperty).getEnd();
                
                const int clippedValue = jlimit(low, high, newValue);
                
                currentSelection[i]->setPropertyWithUndo(soundProperty, clippedValue);
            }
        };
        
        
       
    }

	void sliderDragStarted(Slider *s)
	{
		dragStartValues.clear();

		sliderStartValue = (int)s->getValue();

		for(int i = 0; i < currentSelection.size(); i++)
		{
			dragStartValues.add(currentSelection[i]->getProperty(soundProperty));
		}

		if(currentSelection.size() != 0)
		{
			currentSelection[0]->startPropertyChange(soundProperty, dragStartValues[0]);
		};
	};

	void sliderDragEnded(Slider *s)
	{
		if (currentSelection.size() != 0)
		{
            if(isFileAccessingProperty())
            {
                setSamplePropertyValue((int)s->getValue(), true);
            }
            
			currentSelection[0]->endPropertyChange(soundProperty, sliderStartValue, (int)s->getValue());
		}
	};

	void setPropertyType(ModulatorSamplerSound::Property p)
	{
		soundProperty = p;
		descriptionLabel->setText(ModulatorSamplerSound::getPropertyName(p), dontSendNotification);

		descriptionLabel->setTooltip(ModulatorSamplerSound::getPropertyName(p));

	}

	void setCurrentSelection(const SampleSelection &newSelection)
	{
		for(int i = 0; i < currentSelection.size(); i++)
		{
			if(currentSelection[i] != nullptr)
			{
				currentSelection[i]->removeChangeListener(this);
			}
		}

		currentSelection.clear();
		currentSelection.addArray(newSelection);

		for(int i = 0; i < currentSelection.size(); i++)
		{
			if (auto s = currentSelection[i].get())
			{
				s->addChangeListener(this);
			}
		}


		if(newSelection.size() != 0 && currentSlider.getComponent() != nullptr)
		{
			currentSlider->setValue(newSelection[0]->getProperty(soundProperty));
			currentSlider->setEnabled(true);
		}

		updateValue();
	};

	void changeListenerCallback(SafeChangeBroadcaster *)
	{
		updateValue();
	}

	void setPropertyForAllSelectedSounds(ModulatorSamplerSound::Property p, int newValue);;

	void changePropertyForAllSelectedSounds(ModulatorSamplerSound::Property p, int delta)
	{
		for(int i = 0;i < currentSelection.size(); i++)
		{
			const int currentValue = currentSelection[i]->getProperty(p);

			const int newValue = currentValue + delta;

			const int low = currentSelection[i]->getPropertyRange(soundProperty).getStart();
			const int high = currentSelection[i]->getPropertyRange(soundProperty).getEnd();

			const int clippedValue = jlimit(low, high, newValue);

			currentSelection[i]->setPropertyWithUndo(p, clippedValue);
		};

		sendChangeMessage();

		updateValue();
	};

	void updateValue()
	{
		if(currentSelection.size() == 0)
		{
			valueLabel->setText("", dontSendNotification);
		}
		else if (currentSelection.size() == 1)
		{
			valueLabel->setText(currentSelection[0]->getPropertyAsString(soundProperty), dontSendNotification);
		}
		else
		{
			int min = INT_MAX;
			int max = INT_MIN;

			for(int i = 0; i < currentSelection.size(); i++)
			{
				int newValue = currentSelection[i]->getProperty(soundProperty);

				min = jmin(newValue, min);
				max = jmax(newValue, max);
			};

			String text;

			if(min == max)
			{
				text << currentSelection[0]->getPropertyAsString(soundProperty);
			}
			else
			{
				text << String(min) << " - " << String(max);
			}

			valueLabel->setText(text, dontSendNotification);
		}
	}

	void setLabelColour(Colour c, Colour t)
	{
		valueLabel->setColour(Label::backgroundColourId, c);
		valueLabel->setColour(Label::textColourId, t);
		valueLabel->setColour(Label::ColourIds::textWhenEditingColourId, t);
	};

	void timerCallback()
	{
		if (currentSlider.getComponent() == nullptr)
		{
			if (isFileAccessingProperty())
			{
				for (int i = 0; i < currentSelection.size(); i++)
				{
					currentSelection[i].get()->closeFileHandle();
				}
			}

			stopTimer();
		}
	}

    //[/UserMethods]

    void paint (Graphics& g);
    void resized();
    void labelTextChanged (Label* labelThatHasChanged);
    void buttonClicked (Button* buttonThatWasClicked);



private:
    //[UserVariables]   -- You can add your own custom variables in this section.

	ModulatorSamplerSound::Property soundProperty;

	LookAndFeel_V3 laf;

	SampleSelection currentSelection;

	Array<int> dragStartValues;

	int sliderStartValue;

	Component::SafePointer<Slider> currentSlider;

    //[/UserVariables]

    //==============================================================================
    ScopedPointer<Label> valueLabel;
    ScopedPointer<Label> descriptionLabel;
    ScopedPointer<TextButton> minusButton;
    ScopedPointer<TextButton> plusButton;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ValueSettingComponent)
};

//[EndFile] You can add extra defines here...
} // namespace hise
//[/EndFile]

#endif   // __JUCE_HEADER_CB07FBFFB8C6ABE4__
