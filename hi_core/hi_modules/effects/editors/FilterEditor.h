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

namespace hise { using namespace juce;

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
		modeSelector->setSelectedId((int)getProcessor()->getAttribute(PolyFilterEffect::Mode) + 1, dontSendNotification);
		freqSlider->updateValue();
		bipolarFreqSlider->updateValue();
		gainSlider->updateValue();
		qSlider->updateValue();

		FilterBank::FilterMode m = (FilterBank::FilterMode)(int)(getProcessor()->getAttribute(PolyFilterEffect::Mode));

		switch (m)
		{
		case FilterBank::OnePoleHighPass:	qSlider->setEnabled(false); gainSlider->setEnabled(false); break;
		case FilterBank::OnePoleLowPass:	qSlider->setEnabled(false); gainSlider->setEnabled(false); break;
		case FilterBank::LowPass:			qSlider->setEnabled(false); gainSlider->setEnabled(false); break;
		case FilterBank::HighPass:		qSlider->setEnabled(false); gainSlider->setEnabled(false); break;
		case FilterBank::LowShelf:		qSlider->setEnabled(true); gainSlider->setEnabled(true); break;
		case FilterBank::HighShelf:		qSlider->setEnabled(true); gainSlider->setEnabled(true); break;
		case FilterBank::Peak:			qSlider->setEnabled(true); gainSlider->setEnabled(true); break;
		case FilterBank::ResoLow:			qSlider->setEnabled(true); gainSlider->setEnabled(false); break;
		case FilterBank::StateVariableLP: qSlider->setEnabled(true); gainSlider->setEnabled(false); break;
		case FilterBank::StateVariableHP: qSlider->setEnabled(true); gainSlider->setEnabled(false); break;
		case FilterBank::MoogLP:			qSlider->setEnabled(true); gainSlider->setEnabled(false); break;
		default:								break;
		}
	};

	void timerCallback() override;

	void updateNameLabel(bool forceUpdate=false);

	bool sameCoefficients(FilterDataObject::CoefficientData c1, FilterDataObject::CoefficientData c2)
	{
		if(c1.second != c2.second)
			return false;

		for(int i = 0; i < 5; i++)
		{
			if(c1.first.coefficients[i] != c2.first.coefficients[i]) return false;
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

	ProcessorEditorBodyUpdater updater;

	int h;

	bool isPoly = true;

	FilterDataObject::CoefficientData currentCoefficients;

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
} // namespace hise

//[/EndFile]

#endif   // __JUCE_HEADER_136726B4B6472A22__
