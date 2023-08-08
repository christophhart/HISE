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

#ifndef __JUCE_HEADER_E4FDC9DDF5274452__
#define __JUCE_HEADER_E4FDC9DDF5274452__

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
class MdaDegradeEditor  : public ProcessorEditorBody,
                          public SliderListener
{
public:
    //==============================================================================
    MdaDegradeEditor (ProcessorEditor *p);
    ~MdaDegradeEditor();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

	void updateGui() override
	{
		for(int i = 0; i < MdaLimiterEffect::numParameters; i++)
		{
			if(getSlider(i) != nullptr)  getSlider(i)->updateValue();//setValue(getProcessor()->getAttribute(i), dontSendNotification);

		}
	}

	int getBodyHeight() const override
	{
		return h;
	};

	MdaSlider *getSlider(int parameterIndex)
	{
		switch(parameterIndex)
		{
		case MdaDegradeEffect::Headroom:	return nullptr;
		case MdaDegradeEffect::Quant:		return quantSlider;
		case MdaDegradeEffect::Rate:		return rateSlider;
		case MdaDegradeEffect::PostFilt:	return postFilterSlider;
		case MdaDegradeEffect::NonLin:		return nonLinSlider;
		case MdaDegradeEffect::DryWet:		return outputSlider;
		default:							return nullptr;
		}
	}

    //[/UserMethods]

    void paint (Graphics& g);
    void resized();
    void sliderValueChanged (Slider* sliderThatWasMoved);



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
	int h;
    //[/UserVariables]

    //==============================================================================
    ScopedPointer<MdaSlider> quantSlider;
    ScopedPointer<MdaSlider> outputSlider;
    ScopedPointer<MdaSlider> rateSlider;
    ScopedPointer<MdaSlider> postFilterSlider;
    ScopedPointer<MdaSlider> nonLinSlider;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MdaDegradeEditor)
};

//[EndFile] You can add extra defines here...
/** endcond */
} // namespace hise

//[/EndFile]

#endif   // __JUCE_HEADER_E4FDC9DDF5274452__
