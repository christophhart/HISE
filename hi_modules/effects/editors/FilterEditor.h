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

#ifndef __JUCE_HEADER_136726B4B6472A22__
#define __JUCE_HEADER_136726B4B6472A22__

//[Headers]     -- You can add your own extra header files here --

//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    \cond HIDDEN_SYMBOLS

	An auto-generated component, created by the Introjucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class FilterEditor  : public ProcessorEditorBody,
                      public Timer,
                      public SliderListener,
                      public ComboBoxListener
{
public:
    //==============================================================================
    FilterEditor (ProcessorEditor *p);
    ~FilterEditor();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

	void updateGui() override
	{
		modeSelector->setSelectedId((int)getProcessor()->getAttribute(MonoFilterEffect::Mode) + 1, dontSendNotification);
		freqSlider->updateValue();
		bipolarFreqSlider->updateValue();
		gainSlider->updateValue();
		qSlider->updateValue();

		MonoFilterEffect::FilterMode m = (MonoFilterEffect::FilterMode)(int)(getProcessor()->getAttribute(MonoFilterEffect::Mode));

		switch (m)
		{
		case MonoFilterEffect::OnePoleHighPass:	qSlider->setEnabled(false); gainSlider->setEnabled(false); break;
		case MonoFilterEffect::OnePoleLowPass:	qSlider->setEnabled(false); gainSlider->setEnabled(false); break;
		case MonoFilterEffect::LowPass:			qSlider->setEnabled(false); gainSlider->setEnabled(false); break;
		case MonoFilterEffect::HighPass:		qSlider->setEnabled(false); gainSlider->setEnabled(false); break;
		case MonoFilterEffect::LowShelf:		qSlider->setEnabled(true); gainSlider->setEnabled(true); break;
		case MonoFilterEffect::HighShelf:		qSlider->setEnabled(true); gainSlider->setEnabled(true); break;
		case MonoFilterEffect::Peak:			qSlider->setEnabled(true); gainSlider->setEnabled(true); break;
		case MonoFilterEffect::ResoLow:			qSlider->setEnabled(true); gainSlider->setEnabled(false); break;
		case MonoFilterEffect::StateVariableLP: qSlider->setEnabled(true); gainSlider->setEnabled(false); break;
		case MonoFilterEffect::StateVariableHP: qSlider->setEnabled(true); gainSlider->setEnabled(false); break;
		case MonoFilterEffect::MoogLP:			qSlider->setEnabled(true); gainSlider->setEnabled(false); break;
		default:								break;
		}
	};

	void timerCallback() override
	{
		IIRCoefficients c = dynamic_cast<FilterEffect*>(getProcessor())->getCurrentCoefficients();

		for(int i = 0; i < 5; i++)
		{

			if(c.coefficients[i] == 0.0) // Replace with safe check function!
			{
				
				return;
			}
		}

		if(!sameCoefficients(c, currentCoefficients))
		{
			currentCoefficients = c;

			filterGraph->setCoefficients(0, getProcessor()->getSampleRate(), dynamic_cast<FilterEffect*>(getProcessor())->getCurrentCoefficients());
		}

		freqSlider->setDisplayValue(getProcessor()->getChildProcessor(MonoFilterEffect::FrequencyChain)->getOutputValue());
		bipolarFreqSlider->setDisplayValue(getProcessor()->getChildProcessor(MonoFilterEffect::BipolarFrequencyChain)->getOutputValue());

	}

	bool sameCoefficients(IIRCoefficients c1, IIRCoefficients c2)
	{
		for(int i = 0; i < 5; i++)
		{
			if(c1.coefficients[i] != c2.coefficients[i]) return false;
		}

		return true;
	};


	int getBodyHeight() const override
	{
		return h;
	};


    //[/UserMethods]

    void paint (Graphics& g);
    void resized();
    void sliderValueChanged (Slider* sliderThatWasMoved);
    void comboBoxChanged (ComboBox* comboBoxThatHasChanged);



private:
    //[UserVariables]   -- You can add your own custom variables in this section.

	int h;

	IIRCoefficients currentCoefficients;

    //[/UserVariables]

    //==============================================================================
    ScopedPointer<HiSlider> freqSlider;
    ScopedPointer<HiSlider> qSlider;
    ScopedPointer<HiSlider> gainSlider;
	ScopedPointer<HiSlider> bipolarFreqSlider;
    ScopedPointer<ComboBox> modeSelector;
    ScopedPointer<FilterGraph> filterGraph;
    ScopedPointer<Label> label;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilterEditor)
};

//[EndFile] You can add extra defines here...
/** \endcond */

//[/EndFile]

#endif   // __JUCE_HEADER_136726B4B6472A22__
